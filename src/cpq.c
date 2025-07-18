/* © Copyright 1995-2025, Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: cpq.c
 *              for BITNET folks with a habit, CPQuery <something>
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Oct-15 and following
 *
 */

#include        <string.h>
#include        <stdio.h>

#include        "uft.h"

#ifndef         BUFSIZ
#define         BUFSIZ          4096
#endif

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(argc,argv)
  int     argc;
  char   *argv[];
  {
    char        temp[BUFSIZ], cpqs[BUFSIZ], *host, *proxy, *arg1;
    int         s, i, j;
    char       *arg0, *p;

    /*  establish defaults  */
    host = "localhost";

    arg0 = argv[0];

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

/*
                    sprintf(temp,"%s: %s Remote CPQUERY client",
                                arg0,UFT_VERSION);
                    uftx_putline(2,temp,0);
 */

    /*  verify sufficient arguments (1)  */
    if ((argc - i) < 1)
      { (void) sprintf(temp,
                "Usage: %s [-h <host>] <something>",argv[0]);
        uftx_putline(2,temp,0);
        return 24; }

    /*  connect  */
    (void) sprintf(temp,"%s:%d",host,608);
    s = tcpopen(temp,0,0);
    if (s < 0)
      { (void) perror(host);
        return s; }

    /*  read and discard the herald  */
     uftx_getline(s,temp,BUFSIZ);

    /*  join commandline args into CPQuery string  */
    cpqs[0] = 0x00;
    for (j = i; j < argc; j++)
      {
        strcat(cpqs,argv[j]);
        strcat(cpqs," "); }
/*  uftx_putline(1,cpqs,0);  */

    /*  send the CPQ request  */
    (void) sprintf(temp,"CPQ %s",cpqs);
    (void) tcpputs(s,temp);

    /*  and wait for the ACK  */
    while (1)
      { uftx_getline(s,temp,BUFSIZ);
        p = temp;  while (*p > ' ') p++;  while (*p <= ' ') p++;
        if (temp[0] == '6') uftx_putline(1,p,0);
//      else if (temp[0] != '2') uftx_putline(2,p,0);
        if (temp[0] != '1' && temp[0] != '6') break; }

    /*  exit cleanly  */
    (void) tcpputs(s,"QUIT");
    uftc_wack(s,temp,BUFSIZ);

    return 0;
  }


