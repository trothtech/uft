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

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  {
    char        temp[4096], cpqs[4096], *host, *proxy, *arg1;
    int         rc, i, j;
    char       *arg0, *p, *ptitle, *mv[8];
    UFTFD       ufd, *ufdp;

    ptitle = "Remote CPQUERY client";                /* program title */
    ufdp = &ufd;

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
                if (uftx_abbrev("--version",argv[i],6) > 0)
                  { sprintf(temp,"%s: %s Remote CPQUERY client",
                                arg0,UFT_VERSION);
                    fprintf(stderr,"%s\n",temp);
                    return 0; } else           /* exit from help okay */
                if (uftx_abbrev("--host",argv[i],6) > 0)
                  { i++; host = argv[i]; } else
                if (uftx_abbrev("--proxy",argv[i],7) > 0)
                  { i++; proxy = argv[i]; } else
                if (uftx_abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                  { sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                    fprintf(stderr,"%s\n",temp);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                        fprintf(stderr,"%s\n",temp);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

/*                  sprintf(temp,"%s: %s Remote CPQUERY client",
                                arg0,UFT_VERSION);
**                  uftx_putline(2,temp,0);
                    fprintf(stderr,"%s\n",temp);                      */
    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      { sprintf(temp,"%s: %s %s",arg0,UFT_VERSION,ptitle);
        fprintf(stderr,"%s\n",temp); }
    temp[0] = 0x00;

    /* verify sufficient arguments */
    if ((argc - i) < 1)
      { fprintf(stderr,"Usage: %s [-h <host>] <something>\n",argv[0]);
        return 1; }

    /* try now to connect with the server (TCP, proxy, SSL)           */
    rc = ufts_open(host,proxy,ufdp); if (rc < 0)
    rc = uftx_open(host,proxy,ufdp);
    if (rc != 0) { if (errno != 0) perror(host);
        mv[0] = arg0; mv[1] = host;  /* cannot connect to target host */
        uftx_msgprtl(20,"CLI",2,mv);   /* 20 E target UFT not reached */
        return 1; }
    /* r = ufd.fd0 for read and s = ufd.fd1 for send */

    /* read and discard the herald */
    rc = uftx_gets(ufdp,temp,sizeof(temp)-1);
    if (rc < 0) { if (errno != 0) perror("uftx_gets()");
        if (temp[0] != 0x00) fprintf(stderr,"%s\n",temp);
        ufts_close(ufdp); return 1; }
    if (rc < 0) { if (errno != 0) perror(host);
        uftx_close(ufdp);                     /* close the connection */
        mv[0] = arg0; mv[1] = host;      /* failed herald from target */
        uftx_msgprtl(21,"CLI",2,mv);    /* 21 E failed reading herald */
        return 1; }              /* read of herald from server failed */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

    /* join commandline args into one CPQuery string */
    cpqs[0] = 0x00;
    for (j = i; j < argc; j++)
      { strcat(cpqs,argv[j]);
        strcat(cpqs," "); }
/*  uftx_putline(1,cpqs,0);  */

    /* send the CPQ request to the remote UFT server */
    sprintf(temp,"CPQ %s",cpqs);
    rc = uftx_puts(ufdp,temp,0);

    /* ... and wait for the ACK */
    while (1)
      { rc = uftx_gets(ufdp,temp,sizeof(temp)-1);
        p = temp;  while (*p > ' ') p++;  while (*p <= ' ') p++;
        if (temp[0] == '6') /* uftx_putline(1,p,0); */ fprintf(stdout,"%s\n",p);
        else if (temp[0] != '1' &&
temp[0] != '2') fprintf(stderr,"%s\n",temp);
        if (temp[0] != '1' && temp[0] != '6') break; }

    /* exit cleanly */
    uftx_puts(ufdp,"QUIT",0);
    rc = uftx_wack(ufdp,temp,sizeof(temp)-1);

    ufts_close(ufdp);

    return 0;
  }


