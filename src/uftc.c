/* Copyright 1994-2026 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftc.c, sendfile.c (C program source)
 *              Unsolicited (or Universal) File Transfer client
 *              *finally* an Internet SENDFILE for Unix
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1994-Jun-30, 1995-Jan-22 ... and following ... 2025
 *
 */

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#include "uft.h"

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftc.c main()";
    int         i, fd0, size, copy, fda, rc, uftxflag, nop, chf;
    char        temp[256], targ[256], b[UFT_BUFSIZ], akey[256], *mv[16],
               *host, *name, *type, *auth, *class, *proxy, *ptitle,
               *flga, *flgb;
    struct  stat  uftcstat;
    time_t      mtime;
    mode_t      prot;
    struct tm *gmtstamp;
    char   *arg0;
    int     uftv;           /* protocol level (1 or 2) */
    UFTFD   ufd, *ufdp;

    ptitle = "Internet SENDFILE client";             /* program title */
    ufdp = &ufd;

    /* note command name and set defaults */
            arg0 = uftx_basename(argv[0]);
    uftcflag = UFT_BINARY;      /* default */
    name = type = class = "";
    auth = "-";                /* no particular authentication scheme */
    copy = 0;
    proxy = "";
    uftxflag = 0x0000;                         /* reset all flag bits */
    flga = flgb = "";
    nop = chf = 0;           /* winnowing and chaffing off by default */

    /* process command-line options                                   */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;
            case 'a':   case 'A':       /* ASCII (ie: plain text)     */
                        uftcflag &= ~UFT_BINARY;
                        type = "A";
                        uftxflag |= UFT_DOTRANS;
                        flga = argv[i];
                        break;
            case 'b':   case 'B':       /* BINARY                     */
            case 'i':   case 'I':       /* aka IMAGE                  */
                        uftcflag |= UFT_BINARY;
                        type = "I";
                        uftxflag |= UFT_NOTRANS;
                        flgb = argv[i];
                        break;
#ifdef  OECS
            case 'e':   case 'E':       /* EBCDIC (IBM plain text)    */
                        uftcflag |= UFT_BINARY;
                        type = "E";
                        break;
#endif
            case 't':   case 'T':       /* sender-specified TYPE      */
                        i++; type = argv[i];
                        break;
            case 'n':   case 'N':       /* NAME of the file           */
                        i++; name = argv[i];
                        break;
            case 'c':   case 'C':       /* CLASS                      */
                        i++; class = argv[i];
                        break;
            case 'm':   case 'M':       /* TYPE=M for email           */
                        uftcflag &= ~UFT_BINARY;
                        type = "M";
                        break;
            case '#':   /* COPY -or- COPIES                           */
                        i++; copy = atoi(argv[i]);
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (uftx_abbrev("--version",argv[i],6) > 0)
                  { sprintf(temp,"%s: %s %s",arg0,UFT_VERSION,ptitle);
                    fprintf(stderr,"%s\n",temp);
                    return 0; } else           /* exit from help okay */

                if (uftx_abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                if (uftx_abbrev("--ascii",argv[i],5) > 0 ||
                    uftx_abbrev("--text",argv[i],6) > 0)
                  { uftcflag &= ~UFT_BINARY; type = "A";
                    uftxflag |= UFT_DOTRANS; flga = argv[i]; } else
                if (uftx_abbrev("--binary",argv[i],5) > 0 ||
                    uftx_abbrev("--image",argv[i],4) > 0)
                  { uftcflag |= UFT_BINARY; type = "I";
                    uftxflag |= UFT_NOTRANS; flgb = argv[i]; } else
#ifdef  OECS
                if (uftx_abbrev("--ebcdic",argv[i],8) > 0)
                  { uftcflag |= UFT_BINARY; type = "E"; } else
#endif
                if (uftx_abbrev("--proxy",argv[i],7) > 0)
                  { i++; proxy = argv[i]; } else
                if (uftx_abbrev("--type",argv[i],6) > 0)
                  { i++; type = argv[i]; } else
                if (uftx_abbrev("--name",argv[i],6) > 0)
                  { i++; name = argv[i]; } else

                if (uftx_abbrev("--chaff",argv[i],7) > 0)
                  { i++; chf = atoi(argv[i]); } else
                if (uftx_abbrev("--noops",argv[i],6) > 0 ||
                    uftx_abbrev("--nops",argv[i],5) > 0)
                  { i++; nop = atoi(argv[i]); } else

                if (uftx_abbrev("--class",argv[i],4) > 0)
                  { i++; class = argv[i]; } else
/*                          --dest
                            --dist --ms --mailstop
                            --form
                            --title                                   */
                if (uftx_abbrev("--mail",argv[i],6) > 0 ||
                    uftx_abbrev("--email",argv[i],4) > 0)
                  { uftcflag &= ~UFT_BINARY; type = "M"; } else
                if (uftx_abbrev("--copy",argv[i],4) > 0 ||
                    uftx_abbrev("--copies",argv[i],4) > 0)
                  { i++; copy = atoi(argv[i]); } else
                  { mv[0] = arg0; mv[1] = argv[i];
                    rc = uftx_msgprtl(3,"CLI",2,mv);
                    if (rc < 0) fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default: mv[0] = arg0; mv[1] = argv[i];
                rc = uftx_msgprtl(3,"CLI",2,mv);
                if (rc < 0) fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                return 1;                   /* exit on invalid option */
                break;
          }
      }

    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      { sprintf(temp,"%s: %s %s",arg0,UFT_VERSION,ptitle);
        fprintf(stderr,"%s\n",temp); }
    temp[0] = 0x00;

    /* be sure we still have enough args (min 2) left over */
    if ((argc - i) < 2)
      { /* (void) system("xmitmsg -2 386"); */
        (void) sprintf(temp,
                "Usage: %s [ -a | -i ] <file> [to] <someone>",arg0);
        fprintf(stderr,"%s\n",temp);
        (void) sprintf(temp,
                "          [ -n name ] [ -c class ]");
        fprintf(stderr,"%s\n",temp);
        if (uftcflag & UFT_VERBOSE) return 0;  /* exit from help okay */
                              else  return 1; }       /* missing args */
    temp[0] = 0x00;

    /* make sure the user indicated ASCII or IMAGE but not both       */
    if ((uftxflag & UFT_DOTRANS) && (uftxflag & UFT_NOTRANS))
      { mv[1] = flga;     mv[2] = flgb;
        rc = uftx_message(temp,sizeof(temp)-1,66,"CLI",3,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",temp); else
        fprintf(stderr,"%s: conflicting options\n",arg0);
        return 1; }

    /* alternatively: auto-detect translation based on file content   */
#ifndef UFT_AUTOTYPE
    if (!(uftxflag & (UFT_DOTRANS|UFT_NOTRANS)))
      { rc = uftx_message(temp,sizeof(temp)-1,16,"CLI",1,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",temp); else
        fprintf(stderr,"%s: missing options\n",arg0);
        return 1; }
#endif

    /* flag some known canonization types */
    switch (type[0])
      {
        case 'a':   case 'A':
        case 'm':   case 'M':
        case 't':   case 'T':   uftcflag &= ~UFT_BINARY;
                                break;
        case 'b':   case 'B':
        case 'i':   case 'I':
        case 'n':   case 'N':
        case 'u':   case 'U':   uftcflag |= UFT_BINARY;
                                break;
      }

    /* open the input file, if not stdin */
    if (argv[i][0] == '-' && argv[i][1] == 0x00)
      { fd0 = dup(0); }
    else
      { if (name[0] == 0x00) name = argv[i];
        fd0 = open(argv[i],O_RDONLY); }

    /* verify that the open() worked or issue an error message        */
    if (fd0 < 0)
      { if (*name != 0x00) { if (errno != 0) perror(name); }
                   else    { if (errno != 0) perror("stdin"); }
        mv[1] = /* name; */ argv[i];
        uftx_msgprtl(23,"CLI",2,mv);   /* 23 E unable to open file &1 */
        return 1; }                       /* open file to send failed */

    /* do we have any ideas about this file? */
    if (fstat(fd0,&uftcstat) == 0)
      { size = uftcstat.st_size;
        mtime = uftcstat.st_mtime;
        prot = uftcstat.st_mode; }
    else
      { size = 0;  mtime = 0;  prot = 0; }

    /* better peer authentication than IDENT is AGENT (see protocol)  */
    if (*proxy == 0x00) fda = open("/var/run/uft/agent.key",O_RDONLY);
                   else fda = -1;
    if (fda >= 0)
      { i = read(fda,akey,sizeof(akey)-1);
        if (i > 0) akey[i] = 0x00;
        if (i > 0) { i--; if (akey[i] == '\n') akey[i] = 0x00; }
        if (i > 0) { i--; if (akey[i] == '\r') akey[i] = 0x00; }
        if (i > 0) auth = "AGENT"; else akey[i] = 0x00;
        close(fda); } else

    /* see if we're running IDENT locally (long story!)               */
    if (*proxy == 0x00 && auth[0] == '-')
      { sprintf(temp,"%s:%d","localhost",IDENT_PORT);
        ufd.fd1 = tcpopen(temp,0,0);    /* simple local TCP port test */
        if (ufd.fd1 >= 0) { auth = "IDENT"; close(ufd.fd1); } }

    /* open a socket to the server */
    (void) strcpy(targ,argv[argc-1]);
    host = targ;              /* targ later used for the USER command */
    while (*host != 0x00 && *host != '@') host++;
    if (*host == '@') *host++ = 0x00; else host = "localhost";

    /* try now to connect with our peer (TCP, proxy, SSL)             */
    rc = ufts_open(host,proxy,ufdp);
    if (rc != 0)      /* if TLS/SSL failed then retry using cleartext */
    rc = uftx_open(host,proxy,ufdp);
    if (rc != 0) { if (errno != 0) perror(host);
        mv[0] = arg0; mv[1] = host;  /* cannot connect to target host */
        uftx_msgprtl(20,"CLI",2,mv);   /* 20 E target UFT not reached */
        close(fd0); return 1; }
/*  r = ufd.fd0; s = ufd.fd1;    // r (0) for read and s (1) for send */
    if (uftcflag & UFT_VERBOSE) {
if (ufd.fdt == UFT_FD_SOCKET) fprintf(stderr,"connection is standard TCP\n");   /* TRIAGE */
if (ufd.fdt == UFT_FD_SSL) fprintf(stderr,"connection is SSL\n");   /* TRIAGE */
                                }

    /* wait for the herald from the server */
    rc = i = uftx_gets(ufdp,temp,sizeof(temp));
    /* all other server-to-client traffic should use uftx_wack()      */
    if (i < 0)
      { if (errno != 0) perror(host);
        uftx_close(ufdp); close(fd0);    /* close connection and file */
        mv[1] = host;       /* failed reading herald from target host */
        uftx_msgprtl(21,"CLI",2,mv);    /* 21 E failed reading herald */
        return 1; }              /* read of herald from server failed */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

    /* figure out what protocol version the server likes (hope for 2) */
    uftv = temp[0] & 0x0F;            /* compatible w ASCII or EBCDIC */
    if (uftv < 1) uftv = 1;      /* no such thing as negative version */

    /* look for expected error indicators in the herald               */
    if (uftv > 2)     /* not actually the UFT level but an error code */
      { uftx_close(ufdp); close(fd0);    /* close connection and file */
        mv[1] = host;        /* error reading herald from target host */
        uftx_msgprtl(25,"CLI",2,mv);     /* 25 E error reading herald */
          { char *p; int e;
            p = temp; while (*p > ' ') p++; *p++ = 0x00;
            e = atoi(temp);
            if (e == 568 || e == 599) uftx_msgprtl(68,"CLI",0,mv);
                else {  mv[1] = p; uftx_msgprtl(99,"CLI",2,mv); } }
        return 1; }                  /* the herald indicated an error */

    if (uftcflag & UFT_VERBOSE)
      { sprintf(temp,"%d",uftv); mv[1] = temp;      /* protocol level */
        uftx_msgprtl(88,"CLI",2,mv); }  /* 88 I UFT protocol level &1 */
    /* (above is only good for UFT1 or UFT2 but there is no UFT3)     */

    /* identify this client to the server */
    (void) sprintf(temp,"#%s client %s",UFT_PROTOCOL,UFT_VERSION);
    /* FIXME: what about anonymous? */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//  (void) tcpputs(ufd.fd1,temp);     /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,temp,0);     /* FIXME: check that return code */
    /* there is no ACK for comments so don't wait for one here        */

        /* the normal sequence for a UFT transaction is ...           */
        /* ----- step 1 -------------------------------- FILE command */
        /* ----- step 2 -------------------------------- USER command */
        /* ----- step 3 -------------------------------- TYPE command */
        /* ----- step 4 ------------------------------- META commands */
        /* --------------- NAME, DATE, XDATE, PROT, XPERM, CLASS, etc */
        /* ----- step 5 ------------------------------- DATA commands */
        /* ----- step 6 --------------------------------- EOF command */
        /* ----- step 7 -------------------------------- QUIT command */

    /* start the transaction ----------------------- the FILE command */
    (void) sprintf(temp,"FILE %d %s %s %s",size,uftx_user(),auth,akey);
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//  (void) tcpputs(ufd.fd1,temp);     /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,temp,0);     /* FIXME: check that return code */
    i = uftx_wack(ufdp,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else fprintf(stderr,"%s\n",temp);
        uftx_close(ufdp); return 1; }   /* FIXME: remember file close */
    if (uftcflag & UFT_VERBOSE || i == 5) fprintf(stderr,"%s\n",temp);
    if (i == 5)                         /* a 500 NAK here is terminal */
      { uftx_close(ufdp); return 1; }   /* FIXME: remember file close */

    /* tell the server who it's for ---------------- the USER command */
    (void) sprintf(temp,"USER %s",targ);
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//  (void) tcpputs(ufd.fd1,temp);     /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,temp,0);     /* FIXME: check that return code */
    i = uftx_wack(ufdp,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else fprintf(stderr,"%s\n",temp);
        uftx_close(ufdp); close(fd0); return 1; }
    if (uftcflag & UFT_VERBOSE || i == 5) fprintf(stderr,"%s\n",temp);
    if (i == 5)                         /* a 500 NAK here is terminal */
      { uftx_close(ufdp); close(fd0); return 1; }

    /* signal the type for canonization ------------ the TYPE command */
    if (type == 0x0000 || type[0] == 0x00)
      { if (uftcflag & UFT_BINARY) type = "I";
                /* "applcation/octet-stream" */
        else type = "A";    /* "text/plain" */ }
    (void) sprintf(temp,"TYPE %s",type);
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//  (void) tcpputs(ufd.fd1,temp);     /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,temp,0);     /* FIXME: check that return code */
    i = uftx_wack(ufdp,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else fprintf(stderr,"%s\n",temp);
        uftx_close(ufdp); return 1; }   /* FIXME: remember file close */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
    if (i == 5)                         /* a 500 NAK here is terminal */
      { uftx_close(ufdp); return 1; }   /* FIXME: remember file close */

    /* does this file have a name? ------------------- a META command */
    if (name != 0x0000 && name[0] != 0x00)
      { name = uftx_basename(name);       /* exclude the path from it */
        sprintf(temp,"NAME %s",name);    /* but skip META prefix here */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//      i = tcpputs(ufd.fd1,temp);    /* FIXME: need uftx_puts() here */
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0)
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp); }

    /* do we have a time stamp for this file? */
    if (mtime != 0)
      { gmtstamp = gmtime(&mtime);
        if (gmtstamp->tm_year < 1900)
            gmtstamp->tm_year += 1900;
        gmtstamp->tm_mon = gmtstamp->tm_mon + 1;
/*                         %Y-%m-%d, the ISO 8601 date format         */
        sprintf(temp,"DATE %04d-%02d-%02d %02d:%02d:%02d %s",
                gmtstamp->tm_year, gmtstamp->tm_mon,
                gmtstamp->tm_mday, gmtstamp->tm_hour,
                gmtstamp->tm_min, gmtstamp->tm_sec, "GMT");
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//      i = tcpputs(ufd.fd1,temp);    /* FIXME: need uftx_puts() here */
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

        /* also send it as number-of-seconds Unix epoch offset value  */
        (void) sprintf(temp,"META XDATE %ld",mtime);
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//      i = tcpputs(ufd.fd1,temp);    /* FIXME: need uftx_puts() here */
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp); }

    /* do we have a protection bit pattern on this file?              */
    if (prot != 0)
      { (void) sprintf(temp,"META PROT %s",uftcprot(prot));
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//      i = tcpputs(ufd.fd1,temp);    /* FIXME: need uftx_puts() here */
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

        /* also send it as bits in octal format                       */
        (void) sprintf(temp,"META XPERM 0%o",prot);
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
//      i = tcpputs(ufd.fd1,temp);    /* FIXME: need uftx_puts() here */
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp); }

    /* does this file have a specific class?           a META command */
    if (class != 0x0000 && class[0] != 0x00)
      { (void) sprintf(temp,"META CLASS %s",class);
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
        rc = uftx_puts(ufdp,temp,0);      /* FIXME: check return code */
        i = uftx_wack(ufdp,temp,sizeof(temp));
        if (i < 0)
          { if (errno != 0) (void) perror(arg0);
            else fprintf(stderr,"%s\n",temp);
            uftx_close(ufdp); return 1; }    /* FIXME: file close too */
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp); }

    /* process the data - everything goes down the pipe mostly as-is  */

    /* now send the file down the pipe */
    while (1)
      { if (uftcflag & UFT_BINARY)              /* get binary content */
          { rc = i = uft_readspan(fd0,b,UFT_BUFSIZ); if (rc == 0)
            rc = i = uft_readspan(fd0,b,UFT_BUFSIZ); if (rc < 1) break; }
        else                                   /* get textual content */
          { rc = i = uftctext(fd0,b,UFT_BUFSIZ); if (rc == 0)
            rc = i = uftctext(fd0,b,UFT_BUFSIZ); if (rc < 1) break; }

//      sprintf(temp,"DATA %d",i); tcpputs(ufd.fd1,temp);   /* uftx_puts() here */
        sprintf(temp,"DATA %d",i);
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);
        uftx_puts(ufdp,temp,0);
        rc = uftx_wack(ufdp,temp,sizeof(temp));      /* expect 3 here */
        if (rc != 3) break;
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

//      rc = tcpwrite(ufd.fd1,b,i); /* send the data - we live for this */
        rc = uftx_write(ufdp,b,i);    /* send data - we live for this */
        rc = uftx_wack(ufdp,temp,sizeof(temp));      /* expect 2 here */
        if (rc != 2) break;
        if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp); }

    /* -------------------------------------------------------------- */

    /* close the file handle */
    (void) close(fd0);

    /* send an "EOF" command to indicate clean end-of-file            */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"EOF\n");
//  rc = tcpputs(ufd.fd1,"EOF");      /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,"EOF",0);     /* FIXME: check the return code */
    if (rc < 0) { uftx_close(ufdp); return 1; }

    /* wait for ACK */
    rc = uftx_wack(ufdp,temp,sizeof(temp)-1);
    if (rc < 0) { uftx_close(ufdp); return 1; }
    if (rc != 2) fprintf(stderr,"%s\n",temp); else
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

    /* tell the server we're done - send a "QUIT" command to close    */
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"QUIT\n");
//  rc = tcpputs(ufd.fd1,"QUIT");     /* FIXME: need uftx_puts() here */
    rc = uftx_puts(ufdp,"QUIT",0);    /* FIXME: check the return code */
    if (rc < 0) { uftx_close(ufdp); return 1; }

    /* wait for ACK */
    rc = uftx_wack(ufdp,temp,sizeof(temp)-1);
    if (rc < 0) { uftx_close(ufdp); return 1; }
    if (rc != 2) fprintf(stderr,"%s\n",temp); else
    if (uftcflag & UFT_VERBOSE) fprintf(stderr,"%s\n",temp);

    /* arbitrary delay so the server can catch up if needed */
    sleep(2);

    /* close the socket */
    uftx_close(ufdp);

    /* get outta here */
    return 0;
 }


