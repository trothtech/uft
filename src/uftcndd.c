/* Copyright 2025, 2026 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftcndd.c (C program source)
 *              UFT Client NETDATA Decoder
 *        Date: 2025-03-18,19 (Tue,Wed) following St. Patty's Day
 *      Author: Rick Troth, Cedarville, Ohio, USA
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
    int rc, fd, i, plen, type, uftxflag;
    char buffer[65536], *part, bufalt[65536], *arg0, *ptitle, *mv[16];

    /* initialize the UFTNDIO struct                                  */
    struct UFTNDIO ndio;
    ndio.buffer = buffer;
    ndio.bufmax = sizeof(buffer);
    ndio.buflen = 0;
    ndio.bufdex = 0;

    /* note command name and set defaults */
            arg0 = uftx_basename(argv[0]);
    ptitle = "UFT Client NETDATA Decoder";           /* program title */
    uftxflag = 0x0000;                         /* reset all flag bits */

    /*          UFT_VERBOSE     0x4000                                */
    /*          UFT_DOTRANS     0x2000         ** translation implied */
    /*          UFT_NOTRANS     0x1000         ** translation not fit */

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftxflag |= UFT_VERBOSE;
                        break;
            case 'a':   case 'A':       /* ASCII (ie: plain text)     */
            case 't':   case 'T':
                        uftxflag |= UFT_DOTRANS;
                        break;
            case 'b':   case 'B':       /* BINARY                     */
            case 'i':   case 'I':       /* aka IMAGE                  */
                        uftxflag |= UFT_NOTRANS;
                        break;
#ifdef  OECS
            case 'e':   case 'E':       /* EBCDIC (IBM plain text)    */
//                      uftxflag |= UFT_NOTRANS;
                        break;
#endif

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (uftx_abbrev("--version",argv[i],6) > 0)
                  { sprintf(buffer,"%s: %s %s",arg0,UFT_VERSION,ptitle);
                    uftx_putline(2,buffer,0);
                    return 0; } else           /* exit from help okay */
                if (uftx_abbrev("--verbose",argv[i],6) > 0)
                  { uftxflag |= UFT_VERBOSE; } else
                if (uftx_abbrev("--ascii",argv[i],5) > 0 ||
                    uftx_abbrev("--text",argv[i],6) > 0)
                        uftxflag |= UFT_NOTRANS; else
                if (uftx_abbrev("--binary",argv[i],5) > 0 ||
                    uftx_abbrev("--image",argv[i],4) > 0)
                        uftxflag |= UFT_DOTRANS; else
#ifdef  OECS
//              if (uftx_abbrev("--ebcdic",argv[i],8) > 0)
//                { uftxflag |= UFT_BINARY; type = "E"; } else
#endif
                  { mv[1] = argv[i];
                rc = uftx_message(buffer,sizeof(buffer)-1,3,"CLI",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                        return 1; }         /* exit on invalid option */
                        break;
/* ------------------------------------------------------------------ */

            default:    mv[1] = argv[i];
                rc = uftx_message(buffer,sizeof(buffer)-1,3,"CLI",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* user may indicated ASCII or IMAGE (or neither) but not both    */
    if ((uftxflag & UFT_DOTRANS) && (uftxflag & UFT_NOTRANS))
      { mv[1] = "--text"; mv[2] = "--binary";
        rc = uftx_message(buffer,sizeof(buffer)-1,66,"CLI",3,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
        fprintf(stderr,"%s: conflicting options\n",arg0);
        return 1; }

    /* announcement (iff verbose option requested) */
    if (uftxflag & UFT_VERBOSE)
      { sprintf(buffer,"%s: %s %s",arg0,UFT_VERSION,ptitle);
        uftx_putline(2,buffer,0); }
    buffer[0] = 0x00;

    /* ensure sufficient arguments ... but no extras                  */
    argc = argc - i;
    if (argc < 1)
      { rc = uftx_message(buffer,sizeof(buffer)-1,16,"CLI",1,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
        fprintf(stderr,"%s: missing filename\n",arg0);
        return 1; }
    if (argc > 1)
      { mv[1] = argv[i+1];
        rc = uftx_message(buffer,sizeof(buffer)-1,70,"CLI",2,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
        fprintf(stderr,"%s: excess arguments\n",arg0);
        return 1; }

    /* try opening the Netdata file                                   */
    rc = fd = open(argv[i],O_RDONLY);
    if (rc < 0) { if (errno != 0) perror("uftx_getndr"); return 1; }

    while (1)
      {
        rc = uftx_getndr(fd,&ndio,&type,&part,&plen);
        if (rc < 0) { if (errno != 0) perror("uftx_getndr"); /* break; */ }
        if (rc < 0) fprintf(stderr,"uftx_getndr() returned %d\n",rc);
        if (rc < 0) break;
/* fprintf(stderr,"TYPE %02X\n",type);                                */

        /* auto-detect translation if not explicitly indicated above  */
        if (!(uftxflag & (UFT_DOTRANS|UFT_NOTRANS)))
            rc = uftx_autotype(part,plen,&uftxflag);
        if (!(uftxflag & (UFT_DOTRANS|UFT_NOTRANS)))
          { rc = uftx_message(buffer,sizeof(buffer)-1,16,"CLI",1,mv);
            if (rc >= 0) fprintf(stderr,"%s\n",buffer); else
            fprintf(stderr,"%s: missing options\n",arg0);
            return 1; }

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


