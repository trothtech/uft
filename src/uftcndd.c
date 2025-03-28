/*
 *
 *        Name: uftcndd.c (C program source)
 *              UFT Client NETDATA Decoder
 *        Date: 2025-03-18,19 (Tue,Wed) following St. Patty's Day
 *
 *        Note: this is an initial cut, very rough, but needed, so here goes
 *
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "uft.h"

#include "aecs.c"

/* -------------------------------------------------------------- GETNDR
 *  Get a NETDATA Record
 *    need: fd, buffer, bufmax, buflen, bufdex
 *    give: output (pointer), outlen (size/len), and adjust the buffer
 */
int uft_getndr(int fd,struct UFTNDIO*uftndio,int*flag,char**output,int*outlen)
  {
    int rc, l, m;
    char *p;
    if (uftndio->buflen == 0)
      { uftndio->bufdex = 0;
// DEV printf("uft_getndr(): reading\n");
        rc = read(fd,uftndio->buffer,uftndio->bufmax-1);
        if (rc < 0) return rc;
        uftndio->buflen = rc; }

    /* if we have passed the limit then return an indicator to say so */
    if (uftndio->bufdex >= uftndio->buflen) return -1;

    p = uftndio->buffer;
    l = p[uftndio->bufdex];
    l = 0xff & l;

    m = p[uftndio->bufdex+1];
    m = 0xff & m;
    *flag = m;
// DEV printf("uft_getndr(): seglen %d control %02x\n",l,m);

    *output = &p[uftndio->bufdex+2];
    uftndio->bufdex += l;
    *outlen = --l;
    *outlen = --l;

    return 0;
  }

/* ---------------------------------------------------------------------
 *    translate this record from EBCDIC to ASCII
 */
int uftcnddp(char*p,int l)
  {
    int i;
    char temp[256];

//  for (i = 0; i < l; i++) temp[i] = chretoa(p[i]);
    for (i = 0; i < l; i++) temp[i] = p[i];
    temp[i] = 0x00;
    stretoa(temp);
    printf("%s",temp);

    return 0;
  }

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftcndd.c main()";
    int rc, fd, i, j, k, l, n, plen, type;
    char buffer[65536], *part;

    struct UFTNDIO ndio;
    ndio.buffer = buffer;
    ndio.bufmax = sizeof(buffer);
    ndio.buflen = 0;
    ndio.bufdex = 0;

    rc = fd = open(argv[1],O_RDONLY);
    if (rc < 0) return 1;

    i = 0;
    while (1)
      {
        char *b;

        rc = uft_getndr(fd,&ndio,&type,&part,&plen);
//printf("RC %d\n",rc);
        if (rc < 0) break;
//      sleep(1);

        b = part;
        if (type & UFT_ND_CTRL)
          {
// DEV      printf("uftcndd: %02x control %02x%02x%02x%02x%02x%02x\n",
//              type,b[0]&0xff,b[1]&0xff,b[2]&0xff,
//                   b[3]&0xff,b[4]&0xff,b[5]&0xff);
//          if (memcmp(b,"\xc9\xd5\xd4\xd9\xf0\xf6",6) == 0) break;
            if (memcmp(b,UFT_ND_INMR06,6) == 0) break;
          } else {
//          printf("uftcndd: %02x data %d\n",type,plen);
            uftcnddp(b,plen);
            if (type & UFT_ND_LAST) printf("\n");
          }

//      i = i + n + 1;
      }

    close(fd);
    return 0;
  }


