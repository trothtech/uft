/* © Copyright 1995-2025, Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: cpq.c (C program source)
 *              for BITNET folks with a habit, CPQuery <something>
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Oct-15 and following
 *
 */

#include <string.h>
#include <stdio.h>

#include "uft.h"
 #include <errno.h>
#ifdef UFT_POSIX

#endif

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  {
    char        temp[UFT_BUFSIZ], cpqs[UFT_BUFSIZ], *host, *proxy, *arg1;
    int         rc, i, j, fd[2];
    char       *arg0, *p;

    /* note command name and set defaults */
    arg0 = uftx_basename(argv[0]);
    host = "localhost";
    proxy = "";

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;
            case 'h':   i++;
                        host = argv[i];
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],6) > 0)
                  { sprintf(temp,"%s: %s Remote CPQUERY client",
                                arg0,UFT_VERSION);
                    uftx_putline(2,temp,0);
                    return 0; } else           /* exit from help okay */
                if (abbrev("--host",argv[i],6) > 0)
                  { i++; host = argv[i]; } else
                if (abbrev("--proxy",argv[i],7) > 0)
                  { i++; proxy = argv[i]; } else
                if (abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                  { sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                    uftx_putline(2,temp,0);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                        uftx_putline(2,temp,0);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

/*                  sprintf(temp,"%s: %s Remote CPQUERY client",
                                arg0,UFT_VERSION);
                    uftx_putline(2,temp,0);                           */

    /* verify sufficient arguments */
    if ((argc - i) < 1)
      { sprintf(temp,
                "Usage: %s [-h <host>] <something>",argv[0]);
        uftx_putline(2,temp,0);
        return 24; }

    /* connect */
    sprintf(temp,"%s:%d",host,608);
    rc = uftc_open(temp,proxy,fd);
#ifdef UFT_POSIX
    if (rc != 0) { if (errno != 0) perror(temp); return 1; }
#else
    if (rc != 0) { perror(temp); return 1; }
#endif
    /* r = fd[0] for read and s = fd[1] for send */

    /* read and discard the herald */
     uftx_getline(fd[0],temp,UFT_BUFSIZ);

    /* join commandline args into one CPQuery string */
    cpqs[0] = 0x00;
    for (j = i; j < argc; j++)
      { strcat(cpqs,argv[j]);
        strcat(cpqs," "); }
/*  uftx_putline(1,cpqs,0);  */

    /* send the CPQ request to the remote UFT server */
    sprintf(temp,"CPQ %s",cpqs);
    tcpputs(fd[1],temp);

    /* ... and wait for the ACK */
    while (1)
      { uftx_getline(fd[0],temp,UFT_BUFSIZ);
        p = temp;  while (*p > ' ') p++;  while (*p <= ' ') p++;
        if (temp[0] == '6') uftx_putline(1,p,0);
/*      else if (temp[0] != '2') uftx_putline(2,p,0);              // */
        if (temp[0] != '1' && temp[0] != '6') break; }

    /* exit cleanly */
    tcpputs(fd[1],"QUIT");
    uftc_wack(fd[0],temp,UFT_BUFSIZ);

    uftc_close(fd);

    return 0;
  }


