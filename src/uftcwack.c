/* Copyright 1995-2025 Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftcwack.c
 *              UFT Client "Wait for ACK" function
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Mar-09, Nov-21 (Decatur)
 *
 */

#include        <string.h>
#include        <errno.h>
#include        "uft.h"
extern  int     uftcflag;

/* ------------------------------------------------------------ UFTCWACK
 */
int uftcwack(int s,char*b,int l)
  { static char _eyecatcher[] = "uftcwack()";
    int         i;
    char       *p;

    while (1)
      { errno = 0;
        i = tcpgets(s,b,l);
        if (i < 0)
          { /* broken pipe or network error */
            b[0] = 0x00; return i; }
        switch (b[0])
          { case 0x00:                       /* NULL ACK (deprecated) */
                (void) strncpy(b,"2XX ACK (NULL)",l);
                return 0;
            case '6':                   /* write to stdout, then loop */
                p = b;
                while (*p != ' ' && *p != 0x00) p++;
                if (*p != 0x00) (void) uft_putline(1,++p);
            case '1':   case '#':   case '*':   /* discard, then loop */
                break;
            case '2':                          /* simple ACK, is okay */
                return 2;
            case '3':                /* or "more required", also okay */
                return 3;
            case '4':                       /* "4" means client error */
                return 4;
            case '5':                   /* and "5" means server error */
                return 5;
            default:                                /* protocol error */
                return -1;
          }
        if (uftcflag & UFT_VERBOSE) if (b[0] != 0x00) uft_putline(2,b);
      }
  }


