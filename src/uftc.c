/* Copyright 1994-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftc.c, sendfile.c
 *              Unsolicited File Transfer client
 *              *finally* an Internet SENDFILE for Unix
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1994-Jun-30, 1995-Jan-22 ... and following
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

char   *arg0;
int     uftv;           /* protocol level (1 or 2) */

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftc.c main()";
    int         i, fd0, s, size, r, copy, fda, rc;
    char        temp[256], targ[256], b[BUFSIZ], akey[256],
               *host, *name, *type, *auth, *class, *proxy;
    struct  stat    uftcstat;
    time_t      mtime;
    mode_t      prot;
    struct tm *gmtstamp;

    /* note command name and set defaults */
    arg0 = uftx_basename(argv[0]);
    uftcflag = UFT_BINARY;      /* default */
    name = type = class = "";
    auth = "-";         /* no particular authentication scheme */
    copy = 0;
    proxy = "";

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      {
        switch (argv[i][1])
          {
            case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;
            case 'a':   case 'A':       /* ASCII (ie: plain text)     */
                        uftcflag &= ~UFT_BINARY;
                        type = "A";
                        break;
            case 'b':   case 'B':       /* BINARY                     */
            case 'i':   case 'I':       /* aka IMAGE                  */
                        uftcflag |= UFT_BINARY;
                        type = "I";
                        break;
#ifdef  OECS
            case 'e':   case 'E':       /* EBCDIC (IBM plain text)    */
                        uftcflag |= UFT_BINARY;
                        type = "E";
                        break;
#endif
            case 't':   case 'T':       /* sender-specified TYPE      */
                        i++;   
                        type = argv[i];
                        break;
            case 'n':   case 'N':       /* NAME of the file           */
                        i++;
                        name = argv[i];
                        break;
            case 'c':   case 'C':       /* CLASS                      */
                        i++;   
                        class = argv[i];
                        break;
            case 'm':   case 'M':       /* TYPE=M for email           */
                        uftcflag &= ~UFT_BINARY;
                        type = "M";
                        break;
            case '#':   /* COPY -or- COPIES                           */
                        i++;    
                        copy = atoi(argv[i]);
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],6) > 0)
                  { sprintf(temp,"%s: %s Internet SENDFILE client",
                                arg0,UFT_VERSION);
                    uftx_putline(2,temp,0);
                    return 0; } else           /* exit from help okay */
                if (abbrev("--ascii",argv[i],5) > 0 ||
                    abbrev("--text",argv[i],6) > 0)
                  { uftcflag &= ~UFT_BINARY; type = "A"; } else
                if (abbrev("--binary",argv[i],5) > 0 ||
                    abbrev("--image",argv[i],4) > 0)
                  { uftcflag |= UFT_BINARY; type = "I"; } else
#ifdef  OECS
                if (abbrev("--ebcdic",argv[i],8) > 0)
                  { uftcflag |= UFT_BINARY; type = "E"; } else
#endif
                if (abbrev("--proxy",argv[i],7) > 0)
                  { i++; proxy = argv[i]; } else
                if (abbrev("--type",argv[i],6) > 0)
                  { i++; type = argv[i]; } else
                if (abbrev("--name",argv[i],6) > 0)
                  { i++; name = argv[i]; } else
                if (abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                if (abbrev("--class",argv[i],4) > 0)
                  { i++; class = argv[i]; } else
/*                          --dest
                            --dist --ms --mailstop
                            --form
                            --title                                   */
                if (abbrev("--mail",argv[i],6) > 0 ||
                    abbrev("--email",argv[i],4) > 0)
                  { uftcflag &= ~UFT_BINARY; type = "M"; } else
                if (abbrev("--copy",argv[i],4) > 0 ||
                    abbrev("--copies",argv[i],4) > 0)
                  { i++; copy = atoi(argv[i]); } else
                  { sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                    uftx_putline(2,temp,0);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    (void) sprintf(temp,"%s: invalid option %s",
                                arg0,argv[i]);
                        (void) uftx_putline(2,temp,0);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      {
        (void) sprintf(temp,"%s: %s Internet SENDFILE client",
                arg0,UFT_VERSION);
        (void) uftx_putline(2,temp,0); }

    /* be sure we still have enough args (min 2) left over */
    if ((argc - i) < 2)
      { /* (void) system("xmitmsg -2 386"); */
        (void) sprintf(temp,
                "Usage: %s [ -a | -i ] <file> [to] <someone>",arg0);
        (void) uftx_putline(2,temp,0);
        (void) sprintf(temp,
                "          [ -n name ] [ -c class ]");
        (void) uftx_putline(2,temp,0);
        if (uftcflag & UFT_VERBOSE) return 0;  /* exit from help okay */
                              else  return 1; }       /* missing args */

    /* flag some known canonicalization types */
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

    /* verify that the open() worked */
    if (fd0 < 0)
      { if (*name != 0x00) (void) perror(name);
                   else    (void) perror("stdin");
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
      { (void) sprintf(temp,"%s:%d","localhost",IDENT_PORT);
        s = tcpopen(temp,0,0);      /* simple test to see if it opens */
        if (s >= 0) { auth = "IDENT"; close(s); } }

    /* open a socket to the server */
    (void) strcpy(targ,argv[argc-1]);
    host = targ;              /* targ later used for the USER command */
    while (*host != 0x00 && *host != '@') host++;
    if (*host == '@') *host++ = 0x00; else host = "localhost";

    (void) sprintf(temp,"%s:%d",host,UFT_PORT);

    /* open a connection to the UFT server - direct TCP or via proxy  */
    if (*proxy != 0x00)
      { int fd[2];
        rc = uftx_proxy(temp,proxy,fd);
        if (rc != 0) return rc;                        /* open failed */
        r = fd[0]; s = fd[1];            /* r for read and s for send */
      } else {
        s = tcpopen(temp,0,0);
        if (s < 0) { (void) perror(host); return 1; }  /* open failed */
        r = s;                           /* r for read and s for send */
      }

    /* wait for the herald from the server */
    i = tcpgets(r,temp,sizeof(temp));        /* all others uftc_wack() */
    if (i < 0)
      { (void) perror(host);              /* FIXME: remember to close */
        return 1; }              /* read of herald from server failed */
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);

    /* figure out what protocol version the server likes */
    uftv = temp[0] & 0x0F;
    if (uftv < 1) uftv = 1;
    if (uftcflag & UFT_VERBOSE)
      { (void) sprintf(temp,"%s: UFT protocol %d",arg0,uftv);
        (void) uftx_putline(2,temp,0); }
    /* (above is only good for UFT1 or UFT2) */

    /* identify this client to the server */
    (void) sprintf(temp,"#%s client %s",UFT_PROTOCOL,UFT_VERSION);
    (void) tcpputs(s,temp);
    /* NO ACK FOR COMMENTS SO DON'T WAIT FOR ONE HERE */
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);

    /* start the transaction */
/*  (void) sprintf(temp,"FILE %d %s %s",size,uftx_user(),auth);       */
    (void) sprintf(temp,"FILE %d %s %s %s",size,uftx_user(),auth,akey);
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    (void) tcpputs(s,temp);
    i = uftc_wack(r,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else (void) uftx_putline(2,temp,0);
        return 1; }                      /* FIXME: remember to close */
    if (uftcflag & UFT_VERBOSE || i == 5) (void) uftx_putline(2,temp,0);
    if (i == 5) { return 5; }           /* a 500 NAK here is terminal */

    /* tell the server who it's for */
    (void) sprintf(temp,"USER %s",targ);
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    (void) tcpputs(s,temp);
    i = uftc_wack(r,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else (void) uftx_putline(2,temp,0);
        return 1; }
    if (uftcflag & UFT_VERBOSE || i == 5) (void) uftx_putline(2,temp,0);
    if (i == 5) { return 5; }           /* a 500 NAK here is terminal */

    /* signal the type for canonicalization */
    if (type == 0x0000 || type[0] == 0x00)
      { if (uftcflag & UFT_BINARY) type = "I";
                /* "applcation/octet-stream" */
        else type = "A";    /* "text/plain" */ }
    (void) sprintf(temp,"TYPE %s",type);
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    (void) tcpputs(s,temp);
    i = uftc_wack(r,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else (void) uftx_putline(2,temp,0);
        return 1; }
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    if (i == 5) { return 5; }           /* a 500 NAK here is terminal */

    /* does this file have a name? */
    if (name != 0x0000 && name[0] != 0x00)
      { name = uftx_basename(name);
        sprintf(temp,"NAME %s",name);
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        i = tcpputs(s,temp);
        i = uftc_wack(r,temp,sizeof(temp));
        if (i < 0)
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0); }

    /* do we have a time stamp for this file? */
    if (mtime != 0)
/*    { gmtstamp = localtime(&mtime);                                 */
      { gmtstamp = gmtime(&mtime);
        if (gmtstamp->tm_year < 1900)
            gmtstamp->tm_year += 1900;
        gmtstamp->tm_mon = gmtstamp->tm_mon + 1;
/*                         %Y-%m-%d, the ISO 8601 date format         */
        sprintf(temp,"DATE %04d-%02d-%02d %02d:%02d:%02d %s",
                gmtstamp->tm_year, gmtstamp->tm_mon,
                gmtstamp->tm_mday, gmtstamp->tm_hour,
                gmtstamp->tm_min, gmtstamp->tm_sec, tzname[0]);
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        i = tcpputs(s,temp);
        i = uftc_wack(r,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);

        (void) sprintf(temp,"META XDATE %ld",mtime);
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        i = tcpputs(s,temp);
        i = uftc_wack(r,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0); }

    /* do we have a protection bit pattern on this file? */
    if (prot != 0)
      { (void) sprintf(temp,"META PROT %s",uftcprot(prot));
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        i = tcpputs(s,temp);
        i = uftc_wack(r,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0); }

    /* does this file have a specific class? */
    if (class != 0x0000 && class[0] != 0x00)
      { (void) sprintf(temp,"CLASS %s",class);
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        i = tcpputs(s,temp);
        i = uftc_wack(r,temp,sizeof(temp));
        if (i < 0)
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0); }

    /* -------------------------------------------------------------- */

    /* now send the file down the pipe */
    while (1)
      { if (uftcflag & UFT_BINARY)
          { i = uft_readspan(fd0,b,BUFSIZ); if (i == 0)
            i = uft_readspan(fd0,b,BUFSIZ);
 if (i < 1) break; }
        else
          { i = uftctext(fd0,b,BUFSIZ); if (i == 0)
            i = uftctext(fd0,b,BUFSIZ);
 if (i < 1) break; }
        (void) sprintf(temp,"DATA %d",i);
        if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
        (void) tcpputs(s,temp);
        rc = uftc_wack(r,temp,sizeof(temp));           /* expect 3 here */
        (void) tcpwrite(s,b,i);   /* send the data - we live for this */
        i = uftc_wack(r,temp,sizeof(temp));           /* expect 2 here */
        if (i < 0)
          { if (errno != 0) (void) perror(arg0);
            else (void) uftx_putline(2,temp,0);
            return 1; }
        if (uftcflag & UFT_VERBOSE || i == 5) (void) uftx_putline(2,temp,0);
        if (i == 5) { return 5; } }     /* a 500 NAK here is terminal */

    /* -------------------------------------------------------------- */

    /* close the file handle */
    (void) close(fd0);

    /* signal end-of-file to the server */
    (void) sprintf(temp,"EOF");
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    (void) tcpputs(s,temp);
    i = uftc_wack(r,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else (void) uftx_putline(2,temp,0);
        return 1; }
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);

    /* tell the server we're done - should now close the connection   */
    (void) sprintf(temp,"QUIT");
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);
    (void) tcpputs(s,temp);
    i = uftc_wack(r,temp,sizeof(temp));
    if (i < 0)
      { if (errno != 0) (void) perror(arg0);
        else (void) uftx_putline(2,temp,0);
        return 1; }
    if (uftcflag & UFT_VERBOSE) (void) uftx_putline(2,temp,0);

    /* arbitrary delay so the server can catch up if needed */
    (void) sleep(2);

    /* close the socket */
    (void) close(s);

    /* get outta here */
    return 0;
 }

/*
        -a  ASCII (plain text)
        -i  image (binary)

        -#  copies
        -q
        -f
 */


