/* Copyright 1994, 1996, 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: msgc.c (tell.c)
 *              a multi-mode 'tell' command for UNIX
 *      Author: Rick Troth, Rice University, Houston, Texas, USA
 *              Rick Troth, rogue programmer, Cedarville, Ohio, USA
 *        Date: 1994-Jul-25 and prior ... and following
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <libgen.h>

#include "uft.h"

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "msgc.c main()";
    int     rc, i, j, k;
    char    msgbuf[4096], *arg0;

    uftcflag = 0x00000000;      /* default */
    arg0 = uftx_basename(argv[0]);

    /* process command-line options                                   */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      {
        switch (argv[i][1])
          {
            case '?':                   /* help                       */
            case 'v':   (void) sprintf(msgbuf,
                                "%s: %s Internet TELL client",
                                arg0,UFT_VERSION);
                        (void) uftx_putline(2,msgbuf,0);
                        return 0;
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],5) > 0)
                  { sprintf(msgbuf,"%s: %s Internet TELL client",
                                arg0,UFT_VERSION);
                    uftx_putline(2,msgbuf,0);
                    return 0; } else           /* exit from help okay */
                  { sprintf(msgbuf,"%s: invalid option %s",
                                arg0,argv[i]);
                    uftx_putline(2,msgbuf,0);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    (void) sprintf(msgbuf,"%s: invalid option %s",
                                arg0,argv[i]);
                        (void) uftx_putline(2,msgbuf,0);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* be sure we still have enough args (min 2) left over */
    if (argc < 2)
      { /* (void) system("xmitmsg -2 386"); */
        fprintf(stderr,"Missing operand(s).\n");
        return 1; }

    /*  parse them  */
    if (argc > 2)
      { k = 0;
        for (i = 2; i < argc; i++)
          { for (j = 0; argv[i][j] != 0x00; j++)
            msgbuf[k++] = argv[i][j];
            msgbuf[k++] = ' '; }
        msgbuf[k++] = 0x00;
        rc = msgc_uft(argv[1],msgbuf); }
    else while (1)
      { (void) uftx_getline(0,msgbuf,sizeof(msgbuf)-1);
        if (msgbuf[0] == '.' && msgbuf[1] == 0x00) break;
        rc = msgc_uft(argv[1],msgbuf); }
    if (rc < 0) return 1;
           else return 0;
  }


