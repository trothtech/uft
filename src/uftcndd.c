/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftcndd.c (C program source)
 *              UFT Client NETDATA Decoder
 *        Date: 2025-03-18,19 (Tue,Wed) following St. Patty's Day
 *
 */

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "uft.h"

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftcndd.c main()";
    int rc, fd, i, plen, type;
    char buffer[65536], *part, bufalt[65536];

    /* initialize the UFTNDIO struct                                  */
    struct UFTNDIO ndio;
    ndio.buffer = buffer;
    ndio.bufmax = sizeof(buffer);
    ndio.buflen = 0;
    ndio.bufdex = 0;

    /* try opening the Netdata file                                   */
    rc = fd = open(argv[1],O_RDONLY);
    if (rc < 0) { if (errno != 0) perror("uftx_getndr"); return 1; }

    while (1)
      {
        rc = uftx_getndr(fd,&ndio,&type,&part,&plen);
        if (rc < 0) { if (errno != 0) perror("uftx_getndr"); /* break; */ }
        if (rc < 0) fprintf(stderr,"uftx_getndr() returned %d\n",rc);
        if (rc < 0) break;
/* fprintf(stderr,"TYPE %02X\n",type);                                */

        if (type & UFT_ND_CTRL)                   /* a control record */
          { if (memcmp(part,UFT_ND_INMR06,6) == 0) break;
          } else switch (type) {              /* a non-control record */
            case UFT_ND_FIRST:
                memcpy(bufalt,part,plen);
                i = plen;
                break;
            case UFT_ND_NONE:
                memcpy(&bufalt[i],part,plen);
                i = i + plen;
                break;
            case UFT_ND_LAST:
                memcpy(&bufalt[i],part,plen);
                i = i + plen;
                uftx_e2l(1,bufalt,i);
                i = 0;
                break;
            case UFT_ND_FIRST|UFT_ND_LAST:
                uftx_e2l(1,part,plen);
                break;
            default:
                fprintf(stderr,"mixed records\n");
                break;
          }
      }

    close(fd);
    if (rc < 0) return 1;
           else return 0;
  }


