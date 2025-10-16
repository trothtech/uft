/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftctcp2.c (C program source)
 *              Unsolicited File Transfer file [re]sending utility
 *              follows the behavior of the UFTCTCP2 REXX gem from CMS
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-09-09
 *
 *              This program consumes a "SIFT job" as input
 *              translating it into a UFT transaction with a server.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "uft.h"

extern int uftcflag;

/* ---------------------------------------------------------------- SEND
 *    Send the given SIFT record to the UFT server and wait for ACK.
 *    Note: this routine may be replaced by a general write/send later.
 *    Note: this routine sends a single command, not the whole file.
 */
int _sift_send(int fd[],char*b,int l)
  { static char _eyecatcher[] = "_sift_send()";
    int rc;
    char buffer[256];                                   /* _sift_send */

    /* this is silly but fits with possible future UFTLIB logic       */
    if (l < 1) l = strlen(b);
    else if (b[l] != 0x00) b[l] = 0x00;

    /* send the command record */
    rc = tcpputs(fd[1],b);
    if (rc < 0) return rc;

    /* wait for ACK */
    rc = uftc_wack(fd[0],buffer,sizeof(buffer)-1);
    if (rc < 0) return rc;
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); return rc; }

    return rc;
  }

/* ---------------------------------------------------------------- FILE
 *    Send a FILE command (not the file itself) to the server.
 *    Note: this routine sends a FILE command, not the file content.
 */
int _sift_file(int fd[],UFTSTAT*us)
  { static char _eyecatcher[] = "_sift_file()";
    int rc;
    char buff[256];                                     /* _sift_file */

/*  fd = open()                                                    // */
/*    {                                                            // */
/*      sprintf(buff,"FILE %d %s %s %s",us.uft_size,               // */
/* FIXME: add AGENT logic here for AUTH token                      // */
/*    } else                                                       // */
    sprintf(buff,"FILE %d %s -",us->uft_size,uftx_user());
    rc = _sift_send(fd,buff,0);

    return rc;
  }

/* --------------------------------------------------------------- ABORT
 *    This routine gets called when something went wrong.
 *    If the connection is open then try an orderly "ABORT".
 *    Note: this routine does NOT close the socket.
 */
int _sift_abort(int fd[])
  { static char _eyecatcher[] = "_sift_abort()";
    int rc;
    char buffer[256];                                  /* _sift_abort */

    /* send an "ABORT" command (because we're not sending a file */
    rc = tcpputs(fd[1],"ABORT");
    if (rc < 0) return rc;

    /* wait for ACK */
    rc = uftc_wack(fd[0],buffer,sizeof(buffer)-1);
    if (rc < 0) return rc;
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); return rc; }

    /* send a "QUIT" command to cleanly end the session */
    rc = tcpputs(fd[1],"QUIT");
    if (rc < 0) return rc;

    /* wait for ACK */
    rc = uftc_wack(fd[0],buffer,sizeof(buffer)-1);
    if (rc < 0) return rc;
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); return rc; }

    return 0;
  }

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftctcp2.c main()";
    int rc, fd[2], meta, i;
    char *p, *q, *arg0, *ptitle,
         line[1024], copy[1024], buff[256], user[256], host[256];
    struct UFTSTAT us;

    /* variables for the message formatter */
    int mn, mc; char *mv[16];

    ptitle = "Internet SENDFILE 'SIFT' processor";   /* program title */

    /* note command name and set defaults */
    mv[0] = arg0 = uftx_basename(argv[0]);
    mn = mc = rc = 0; mv[1] = "";
    user[0] = host[0] = 0x00;

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],6) > 0)
                  { sprintf(buff,"%s: %s %s",arg0,UFT_VERSION,ptitle);
                    uftx_putline(1,buff,0);
                    return 0; } else           /* exit from help okay */
                if (abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                  { mv[1] = argv[i];
                rc = uftx_message(buff,sizeof(buff)-1,3,"TCP",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buff); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    mv[1] = argv[i];
                rc = uftx_message(buff,sizeof(buff)-1,3,"TCP",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buff); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      { sprintf(buff,"%s: %s %s",arg0,UFT_VERSION,ptitle);
        uftx_putline(1,buff,0); buff[0] = 0x00; }

    /* be sure we still have enough args */
    if ((argc - i) < 0)
      { /* system("xmitmsg -2 386"); */
        sprintf(buff,"Usage: %s [user@host]",arg0);
        uftx_putline(1,buff,0); buff[0] = 0x00;
        if (uftcflag & UFT_VERBOSE) return 0;  /* exit from help okay */
                              else  return 1; }       /* missing args */

    /* limit command line arguments to just one */
    if ((argc - i) > 1)
      { /* system("xmitmsg -2 386"); */
        sprintf(buff,"Usage: %s [user@host] excess",arg0);
        uftx_putline(1,buff,0); buff[0] = 0x00;
                                    return 1; }        /* excess args */

    /* our one and only command line argument is optional user@host   */
    if ((argc - i) == 1)
      { p = argv[i]; q = user;
        while (*p != 0x00 && *p != ' ' && *p != '@') *q++ = *p++;
        *q = 0x00; if (*p == '@') p++;
                     q = host;
        while (*p != 0x00 && *p != ' ') *q++ = *p++; *q = 0x00; }

    /* begin actual processing */

    /* a marker to indicate we have not received a FILE command yet   */
    us.uft_size = -1;

    /* file descriptors mark that we are not connected to the server  */
    fd[0] = fd[1] = -1;

    /* process the metadata (before the "DATA" statement)             */
    while (1)
      { /* read a line */
        rc = uftx_getline(0,line,sizeof(line)-1);
        if (rc < 0) { mc = 0; mn = 26; break; }

        /* skip leading white space */
        for (p = line; *p <= ' ' && *p != 0x00; p++);
        /* don't process comments nor empty lines, just skip them all */
        if (*p == 0x00) continue;
        if (*p == '*') continue;
        if (*p == '#') continue;

        /* duplicate it so we can parse a copy */
        line[sizeof(line)-1] = 0x00;     /* ensure end-of-line marker */
        strcpy(copy,line);                      /* now make that copy */

        /* upcase and delimit the verb from any following arguments   */
        q = p; while (*q > ' ') { *q = toupper(*q); q++; }
        if (*q != 0x00) *q++ = 0x00;            /* terminate the verb */
        /* q now points to the args (if any) and p points to the verb */

        meta = 0;        /* not a META command unless prefixed (next) */

        /* --------------------------------------------- META command */
        /* differentiate "META" from other commands ----------------- */
        if (abbrev("META",p,4))
          { char *p2, *q2;                      /* alternate pointers */

            /* if this comes before a FILE command that is an error   */
            if (us.uft_size < 0) { mc = 0; mn = 45; rc = -1; break; }

            /* skip leading white space */
            for (p2 = q; *p2 <= ' ' && *p2 != 0x00; p2++);

            /* copy, upcase, and delimit the verb from the args       */
            q2 = p2; while (*q2 > ' ') { *q2 = toupper(*q2); q2++; }
            if (*q2 != 0x00) *q2++ = 0x00;      /* terminate the verb */
            /* q2 now points to the args (if any), and p2 to metaverb */

            /* the META command *must* be followed by an argument     */
            if (*q2 == 0x00) { mc = 0; mn = 16; rc = -1; break; }

            q = q2; p = p2;
            /* q now points to the args (if any) and p to the verb    */
            meta = 1; }  /* set meta flag but do NOT skip next checks */

        /* the normal sequence for a UFT transaction is ...           */
        /* ------------ 1 ------------------------------ FILE command */
        /* ------------ 2 ------------------------------ USER command */
        /* ------------ 3 ------------------------------ TYPE command */
        /* ------------ 4 ----------------------------- META commands */
        /* ------------ 5 ------------------------------ DATA command */

        /* --------------------------------------------- FILE command */
        /* This record should come in *before* we connect to server.  */
        if (abbrev("FILE",p,4) && meta == 0)   /* FILE size from auth */
          { /* if we already got a FILE command that is an error      */
            if (us.uft_size >= 0) { mc = 0; mn = 45; rc = -1; break; }

            /* parse next token (SIZE) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            us.uft_size = uftx_atoi(p);

            /* negative size on a FILE command is an error            */
            if (us.uft_size < 0)
              { mv[1] = p; mv[2] = "SIZE"; mc = 3; mn = 90;
                rc = -1; break; }

            /* parse next token (FROM) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            if (*p != 0x00) strncpy(us.uft_from,p,sizeof(us.uft_from));
            /* skip next token (AUTH) */
            for (p = q; *q > ' '; q++);                  /* vestigial */
            if (*q != 0x00) *q++ = 0x00;                 /* vestigial */

            if (fd[0] >= 0 && fd[1] >= 0) _sift_file(fd,&us);

            continue; }                             /* loop on header */

        /* --------------------------------------------- USER command */
        /* This record enables the connection if it has @host part.   */
        if (abbrev("USER",p,1) && meta == 0)              /* NON META */
          { /* if this comes before a FILE command that is an error   */
            if (us.uft_size < 0) { mc = 0; mn = 45; rc = -1; break; }

            /* point to token and discard any extra tokens            */
            p = q; while (*q > ' ') *q++; *q = 0x00;

            /* if user token is blank or missing that is an error     */
            if (*p == 0x00) { mc = 0; mn = 16; rc = -1; break; }

            /* remember this entire user token in the UFTSTAT struct  */
            strncpy(us.uft_user,p,sizeof(us.uft_user));

            /* look for an "@host" part and parse that out if found   */
            q = p; while (*q != '@' && *q != 0x00 && *q != ' ') q++;
            if (*q == ' ') *q = 0x00;    /* redundant terminate token */
            if (*q == '@') *q++ = 0x00;
            if (*host != 0x00) q = host;    /* command line overrides */

            /* USER statement drives an OPEN if not already open      */
            rc = uftc_open(q,NULL,fd);
            if (rc != 0) { mv[1] = q; mc = 2; mn = 1049; break; }
/* 1049    E Unable to connect to remote UFT server &1             // */

            /* wait for herald from server - the rest via uftc_wack() */
            rc = tcpgets(fd[0],buff,sizeof(buff));
            if (rc < 0) { mc = 0; mn = 26; break; }      /* wrong msg */
            /* should we retain anything from the herald?             */

            rc = _sift_file(fd,&us);         /* send the FILE command */

            if (*user != 0x00) p = user;    /* command line overrides */
            sprintf(buff,"USER %s",p);         /* make a USER command */
            rc = _sift_send(fd,buff,0);     /* send that USER command */

            continue; }                             /* loop on header */

        /* --------------------------------------------- TYPE command */
        if (abbrev("TYPE",p,1) && meta == 0)              /* NON META */
          { us.uft_type = *q;
            /* if this comes before a FILE command that is an error   */
            if (us.uft_size < 0) { mc = 0; mn = 45; rc = -1; break; }

            q++; while (*q == ' ') q++;   /* if second token it's ... */
            if (*q != 0x00) us.uft_cc = *q;  /* ... optional CC value */

            _sift_send(fd,copy,0);                /* send this record */
            continue; }                             /* loop on header */

        /* --------------------------------------------- NAME command */
        if (abbrev("NAME",p,2))                     /* META okay here */
          { /* if this comes before a FILE command that is an error   */
            if (us.uft_size < 0) { mc = 0; mn = 45; rc = -1; break; }

            /* record the name token in the UFTSTAT struct            */
            if (*p != 0x00) strncpy(us.uft_name,p,sizeof(us.uft_name));

            _sift_send(fd,copy,0);                /* send this record */
            continue; }                             /* loop on header */

        /* --------------------------------------------- NOOP command */
        if (abbrev("NOOP",p,2) || abbrev("NOP",p,3)) continue;

        /* --------------------------------------------- DATA command */
        if (abbrev("DATA",p,3) && meta == 0) break;

        /* --------------------------- illegal commands in batch mode */
        if (abbrev("EXIT",p,2)     ||
            abbrev("HELP",p,1)     ||
            abbrev("EOF",p,1)      ||
            abbrev("ABORT",p,1)    ||
            abbrev("QUIT",p,3)     ||
            abbrev("AUXDATA",p,4)  ||
            abbrev("AGENT",p,4)    ||
            abbrev("CPQ",p,3)      ||
           (abbrev("MSG",p,1) && meta == 0))
              { mv[1] = p; mc = 2; mn = 81; rc = -1 ; break; }

        /* -------------------------------------------- META commands */
        if (abbrev("DATE",p,2)      ||  abbrev("XDATE",p,2)     ||
            abbrev("PROT",p,4)      ||  abbrev("XPERM",p,5)     ||
            abbrev("RECFMT",p,4)    ||
            abbrev("RECLEN",p,4)    ||  abbrev("LRECLEN",p,5)   ||
            abbrev("CLASS",p,2)     ||
            abbrev("FORM",p,2)      ||
            abbrev("DESTINATION",p,4) ||
            abbrev("DISTRIBUTION",p,4) ||
            abbrev("TITLE",p,2)     ||
            abbrev("OWNER",p,3)     ||  abbrev("GROUP",p,3)     ||
            abbrev("VERSION",p,3)   ||
            abbrev("HOLD",p,3)      ||
            abbrev("COPY",p,2)      ||  abbrev("COPIES",p,4)    ||
            abbrev("KEEP",p,3))
            _sift_send(fd,copy,0);

/*         (abbrev("MSG",p,1) && meta != 0))                       // */
/*          abbrev("CHARSET",p,5)   ||                             // */
/*          abbrev("UCS",p,3)       ||  abbrev("TRAIN",p,2) ||     // */
/*          abbrev("FCB",p,3)       ||  abbrev("CTAPE",p,2) ||     // */
/*          abbrev("SEQ",p,3)       ||                             // */
/*          abbrev("NOTIFY",p,3)                                   // */
/*        { **  put this variable into the control file  ** }      // */

      }                                 /* end of header-loading loop */

    /* generic error handler */
    if (rc < 0)  {
        if (fd[0] >= 0 && fd[1] >= 0) { _sift_abort(fd); uftc_close(fd); }
        rc = uftx_message(line,sizeof(line)-1,mn,"TCP",mc,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",line);
        if (*copy != 0x00) fprintf(stderr,"%s\n",copy);
        return 1; }

    /* process the data (everything following the "DATA" statement)   */

    /* switch TYPE=A versus TYPE=I ---------------------------------- */
    switch (us.uft_type)
      {
        case 'A': case 'a': uftcflag &= ~UFT_BINARY;
            if (uftcflag & UFT_VERBOSE) fprintf(stderr,"UFTCTCP2: plain text\n");
            break;
        case 'N': case 'n':
        case 'I': case 'i': uftcflag |= UFT_BINARY;
            if (uftcflag & UFT_VERBOSE) fprintf(stderr,"UFTCTCP2: binary\n");
            break;
        default:
            fprintf(stderr,"UFTCTCP2: bogus canonization '%c'\n",us.uft_type);
            break;
      }

    /* process the data (everything following the "DATA" statement)   */
    if (uftcflag & UFT_BINARY) while (1)
          { rc = i = uft_readspan(0,buff,sizeof(buff)); if (rc == 0)
            rc = i = uft_readspan(0,buff,sizeof(buff)); if (rc < 1) break;
            sprintf(line,"DATA %d",i); tcpputs(fd[1],line);
            rc = uftc_wack(fd[0],line,sizeof(line));      /* expect 3 */
            if (rc != 3) break;
            tcpwrite(fd[1],buff,i);     /* send it - we live for this */
            rc = uftc_wack(fd[0],line,sizeof(line));      /* expect 2 */
            if (rc != 2) break; }
    else /* plain text */ while (1)
          { rc = i = uftctext(0,buff,sizeof(buff)); if (rc == 0)
            rc = i = uftctext(0,buff,sizeof(buff)); if (rc < 1) break;
            sprintf(line,"DATA %d",i); tcpputs(fd[1],line);
/*      if (uftcflag & UFT_VERBOSE || i == 5) uftx_putline(2,line,0); */
            rc = uftc_wack(fd[0],line,sizeof(line));      /* expect 3 */
            if (rc != 3) break;
            tcpwrite(fd[1],buff,i);     /* send it - we live for this */
            rc = uftc_wack(fd[0],line,sizeof(line));      /* expect 2 */
            if (rc != 2) break; }

    /* send an "EOF" command to indicate clean end-of-file            */
    rc = tcpputs(fd[1],"EOF");
    if (rc < 0) { uftc_close(fd); return 1; }

    /* wait for ACK */
    rc = uftc_wack(fd[0],line,sizeof(line)-1);
    if (rc < 0) { uftc_close(fd); return 1; }
    if (rc != 2) fprintf(stderr,"%s\n",line);

    /* send a "QUIT" command to cleanly end the session */
    rc = tcpputs(fd[1],"QUIT");
    if (rc < 0) { uftc_close(fd); return 1; }

    /* wait for ACK */
    rc = uftc_wack(fd[0],line,sizeof(line)-1);
    if (rc < 0) { uftc_close(fd); return 1; }
    if (rc != 2) fprintf(stderr,"%s\n",line);

    /* arbitrary delay so the server can catch up if needed */
    sleep(2);

    /* close the socket */
    uftc_close(fd);

    /* get outta here */
    return 0;
  }


