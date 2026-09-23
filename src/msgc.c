/* © Copyright 1994, 1996, 2025, 2026 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: msgc.c, tell.c (C program source)
 *              a multi-mode 'tell' command for UNIX
 *      Author: Rick Troth, Rice University, Houston, Texas, USA
 *              Rick Troth, rogue programmer, Cedarville, Ohio, USA
 *        Date: 1994-Jul-25 and prior ... and following
 *
 *        Note: as of circa 2025 this program only uses UFT protocol,
 *              the "user message hack", other methods dropped for now
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
    char    msgbuf[4096], *arg0, *proxy, *ptitle, *mv[8], *targ;

    ptitle = "Internet TELL client";                 /* program title */

    /* note command name and set defaults */
    arg0 = uftx_basename(argv[0]);
    proxy = "";
    uftcflag = 0x00000000;                     /* reset all flag bits */

    /* process command-line options                                   */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':                   /* help                       */
            case 'v':   (void) sprintf(msgbuf,
                                "%s: %s Internet TELL client",
                                arg0,UFT_VERSION);
                        fprintf(stderr,"%s\n",msgbuf);
                        return 0;
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (uftx_abbrev("--version",argv[i],5) > 0)
                  { fprintf(stderr,"%s: %s %s\n",arg0,UFT_VERSION,ptitle);
                    return 0; } else           /* exit from help okay */

                if (uftx_abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else

                if (uftx_abbrev("--proxy",argv[i],7) > 0)
                  { i++; proxy = argv[i]; } else

                if (uftx_abbrev("--dossl",argv[i],7) > 0)
                  { uftcflag |= UFT_DOSSL; } else
                if (uftx_abbrev("--nossl",argv[i],7) > 0)
                  { uftcflag |= UFT_NOSSL; } else

                  { mv[0] = arg0; mv[1] = argv[i];
                    rc = uftx_msgprtl(3,"MSG",2,mv);
                    if (rc < 0) fprintf(stderr,"%s: invalid option %s\n",arg0,argv[i]);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default: mv[0] = arg0; mv[1] = argv[i];
                rc = uftx_msgprtl(3,"MSG",2,mv);
                if (rc < 0) fprintf(stderr,"%s: invalid option %s\n",arg0,argv[i]);
                return 1;                   /* exit on invalid option */
                break;
          }
      }

    /* be sure we still have enough args (minimum 2) left over        */
    if ((argc - i) < 2)
      { rc = uftx_msgprtl(16,"MSG",0,NULL);
        if (rc < 0) fprintf(stderr,"Missing operand(s).\n");
        return 1; }

    targ = argv[i];     /* first of remaining arguments is the target */

    /* any additional arguments would comprise a command-line message */
    if ((argc - i) >= 2)
      { k = 0;
        while (i < argc && k < sizeof(msgbuf)-1)
          { for (j = 0; argv[i][j] != 0x00; j++)
            msgbuf[k++] = argv[i][j];
            msgbuf[k++] = ' ';
            i++; }
        msgbuf[k++] = 0x00;
        rc = msgc_uft(argv[1],msgbuf,proxy); }

    /* ... or ... this logic presently unused ... take input as msgs  */
    else while (1)
      { rc = uftx_getline(0,msgbuf,sizeof(msgbuf)-1);
        if (rc < 0) break;
        if (msgbuf[0] == '.' && msgbuf[1] == 0x00) break;
        rc = msgc_uft(argv[1],msgbuf,proxy);
        if (rc < 0) break; }

    if (rc < 0) { return 1; }
           else { return 0; }
  }


