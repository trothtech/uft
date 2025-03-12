/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftd.c
 *              Universal File Transfer daemon
 *              Unsolicited File Transfer daemon
 *      Author: Rick Troth, Houston, Texas, USA (concerto)
 *        Date: 1995-Jan-17, Apr-30, Nov-04
 *
 */

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
extern int errno;

#include <ctype.h>
#include <stdlib.h>

#include "uft.h"
char           *arg0;
int             tf, cf, df;     /* temp, meta, and data file handles */
char            tffn[64], cffn[64], dffn[64];
int             ef;             /* new aux-data (ext attr) file */
                                /* can it hold the Mac resource fork? */
char            effn[64];

struct  UFTFILE  uftfile0;

/* ------------------------------------------------------------ UFTDSTAT
 *  Writes a line to the stream indicated by sock (UFT client)
 *  and attempts to log that line in tf (temp) and/or cf (meta).
 */
int uftdstat(int sock,char*zlda)
  { static char _eyecatcher[] = "uftdstat()";
    char        buff[256];

    (void) tcpputs(sock,zlda);
    (void) sprintf(buff,"#>%s",zlda);
    if (tf >= 0) (void) uft_putline(tf,buff);
/*  if (cf >= 0) (void) uft_putline(cf,buff);  */
  }

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char *eyecatch = "uftd.c main()";

    char        line[256], temp[256], user[64], auth[256], *p, *q;
    int         n, uuid, i;
    char        type[16], seqs[8], from[256];
    char        wffn[64];

    /* under what name are we running? */
    arg0 = argv[0];

    /* preset some things */
    n = tf = cf = df = ef = -1;
    user[0] = 0x00;  auth[0] = 0x00;
    type[0] = 0x00;  seqs[0] = 0x00;  from[0] = 0x00;

    uftfile0.type[0]    = '?';  uftfile0.type[1]    = 0x00;
    uftfile0.cc[0]      = '-';  uftfile0.cc[1]    = 0x00;
    uftfile0.hold[0]    = '-';  uftfile0.hold[1]    = 0x00;
    uftfile0.class[0]   = '-';  uftfile0.class[1]   = 0x00;
    uftfile0.devtype[0] = '-';  uftfile0.devtype[1] = 0x00;
    uftfile0.keep[0]    = '-';  uftfile0.keep[1]    = 0x00;
    uftfile0.msg[0]     = '-';  uftfile0.msg[1]     = 0x00;
    uftfile0.copies = 1;
    uftfile0.size = 0;
    uftfile0.name[0]    = 0x00;
    uftfile0.from[0]    = 0x00;

    /* If we're not running as root (or if we don't at least           *
     * own the UFT spooling directory) then we're hopeless.            *
     * But we might also want to be in the UFT_GROUP (typically 0).    */
    (void) setgid(UFT_GID);

    /* work from the UFT spool directory */
    n = chdir(UFT_SPOOLDIR);
    if (n < 0)
      { (void) sprintf(temp,"522 spool directory unavailable (%d).",errno);
/*      (void) sprintm(temp,"uft","srv",522,'E',0,NULL); */
        (void) uftdstat(1,temp);
        return n; }
    /* to leave control in the sysadmin's hands as much as possible,
       DON'T create the UFT spool directory; let him do it manually   */

    /* get next temp control file in sequence */
    n = uftdnext();
    if (n < 0)
      { /* "no spoolids available" (or no permission?) */
        (void) sprintf(temp,"523 workspace sequence error. (%d)",errno);
        (void) uftdstat(1,temp);
        return n; }

    /* now open a temporary control file (meta file) */
    (void) sprintf(tffn,"%s/%04d.lf",UFT_SPOOLDIR,n);
    tf = open(tffn,O_RDWR|O_CREAT,S_IREAD);
    if (tf < 0)
      { (void) sprintf(temp,"524 server temp file error. (%d)",errno);
        (void) uftdstat(1,temp);
        return tf; }

    (void) sprintf(temp,"#*%s server",UFT_VERSION);
    (void) uft_putline(tf,temp);

    /* all okay thus far, now send the herald (informative, but ACK)  */

#ifdef          UFT_ANONYMOUS
    (void) sprintf(temp,"*anonymous");
#else
    (void) gethostname(temp,sizeof(temp));
#endif
/*  (void) sprintf(line,"112 %s %s %s %d ready.",                     */
    (void) sprintf(line,"222 %s %s %s %d ; ready.",
/* 222 ibmisv.marist.edu UFT/2 VMCMSUFT/1.10.5 ; ready.               */
                temp,UFT_PROTOCOL,UFT_VERSION,BUFSIZ);
    (void) uftdstat(1,line);

    /* who's on the other end of the socket? */
    (void) tcpident(0,from,256);      /* IDENT got a bad rap and died */
    (void) sprintf(line,"REMOTE=%s",from);
    (void) strncpy(uftfile0.from,from,sizeof(uftfile0.from));
    (void) uft_putline(tf,line);

    /* where exactly does this item fit? */
#ifndef         UFT_ANONYMOUS
    (void) sprintf(temp,"118 SEQ=%04d (server)",n);
    (void) uftdstat(1,temp);
#endif

    /* loop forever on commands from client */
    while (1)
      { /*  read a line  */
        if (uft_getline(0,line) < 0)
          { (void) sprintf(temp,"#*socket closed w/o QUIT.");
/*          (void) uftdstat(1,temp);                                  */
            (void) sprintf(line,"#*%s",temp);
            if (tf >= 0) (void) uft_putline(tf,line);
            if (cf >= 0) (void) uft_putline(cf,line);
            /* the following should be handled outside of the loop    */
            if (*cffn != 0x00 && *wffn != 0x00) rename(wffn,cffn);
            if (*dffn != 0x00) (void) unlink(dffn);
            if (*effn != 0x00) (void) unlink(effn);
            break; }

        /* skip leading white space */
        for (p = line; *p <= ' ' && *p != 0x00; p++);

        /* don't even log empty lines */
        if (*p == 0x00) continue;

        /* log everything as comments in the control file(s) */
        (void) sprintf(temp,"#<%s",p);
        if (tf >= 0) (void) uft_putline(tf,temp);
/*      if (cf >= 0) (void) uft_putline(cf,temp);                     */

        /* don't process comments any further, just log them */
        if (*p == '*') continue;
        if (*p == '#') continue;

        /* copy, upcase, and delimit the verb */
        for (q = line; *p > ' '; *q++ = toupper(*p++));
        if (*q != 0x00) *q++ = 0x00;
        p = line;
        /* q now points to args (if any), p to verb */

/* -------------------------------------------------------- FILE command
 */
        if (abbrev("FILE",p,4))
          { /* parse another (SIZE) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            (void) sprintf(temp,"SIZE='%s'",p);
            if (tf >= 0) (void) uft_putline(tf,temp);
            if (cf >= 0) (void) uft_putline(cf,temp);
            /* parse another (FROM) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            if (*p != 0x00)
              { (void) sprintf(temp,"FROM='%s'",p);
                if (tf >= 0) (void) uft_putline(tf,temp);
                if (cf >= 0) (void) uft_putline(cf,temp); }
            /* parse another (AUTH) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            if (*p != 0x00)
              { (void) strncpy(auth,p,256);
                (void) sprintf(temp,"AUTH='%s'",auth);
                if (tf >= 0) (void) uft_putline(tf,temp);
                if (cf >= 0) (void) uft_putline(cf,temp); }
            else (void) strncpy(auth,"N/A",256);
            (void) tcpputs(1,"200 ACK FILE");
            continue;
          }

/* -------------------------------------------------------- EXIT command
 * -------------------------------------------------------- QUIT command
 */
        if (abbrev("EXIT",p,2) || abbrev("QUIT",p,3))
          { (void) seteuid(0);
            if (df >= 0 && cf >= 0)
                (void) unlink(tffn);
            break; }

/* -------------------------------------------------------- HELP command
 */
        if (abbrev("HELP",p,1))
          { (void) sprintf(temp,"114 HELP: protocol: %s",UFT_PROTOCOL);
            (void) tcpputs(1,temp);
            (void) sprintf(temp,"114 HELP: server: %s",UFT_VERSION);
            (void) tcpputs(1,temp);
            (void) tcpputs(1,"114 HELP: commands:");
            (void) tcpputs(1,"114 HELP: FILE <size> <from> <auth>");
            (void) tcpputs(1,"114 HELP: USER <recipient>");
            (void) tcpputs(1,"114 HELP: TYPE <filetype>");
            (void) tcpputs(1,"114 HELP: NAME <filename>");
            (void) tcpputs(1,"114 HELP: DATA <burst_size>");
            (void) tcpputs(1,"114 HELP: QUIT");
            (void) tcpputs(1,"214 HELP: end of HELP");
            continue;
          }

/* -------------------------------------------------------- USER command
 */
        if (abbrev("USER",p,1))
          { /* one at a time, please */
            if (user[0] != 0x00)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);
                continue; }

            /* make sure FILE command preceeded USER command */
            if (auth[0] == 0x00)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);
                continue; }

            /* discard any extra tokens */
            p = user;
            while (*q > ' ') *p++ = tolower(*q++);
            *p = 0x00;

            /* now log it */
            (void) sprintf(temp,"USER='%s'",user);
            if (tf >= 0) (void) uft_putline(tf,temp);
            if (cf >= 0) (void) uft_putline(cf,temp);

            /* now try to run with it */
            uuid = uftduser(user);
            if (uuid < 0)
              { (void) sprintf(temp,
                    "532 %s %d; no such local user or queue %s.",
                        user,errno,user);
                (void) uftdstat(1,temp);
                return uuid; } /* should be errno */

            /* get the next sequence number for this user */
            n = uftdnext();
            if (n < 0 && errno == EACCES)
              { (void) seteuid(0);
                n = uftdnext(); }
            if (n < 0)
              { (void) sprintf(temp,
                          "527 user slot error UID=%d EUID=%d ERRNO=%d",
                               getuid(),geteuid(),errno);
                (void) uftdstat(1,temp);
                return n; } /* should be errno */
            else (void) sprintf(seqs,"%04d",n);

            /* okay so far; report the sequence number */
#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"119 SEQ=%s (user)",seqs);
            (void) uftdstat(1,temp);
#endif

            /* now open the *real* control file (meta file) */
            (void) sprintf(cffn,"%04d.cf",n);
            (void) sprintf(wffn,"%04d.wf",n);
            cf = open(wffn,O_WRONLY|O_CREAT,S_IREAD);
            if (cf < 0)
              { (void) sprintf(temp,"534 %d; meta file error.",
                        errno);
                (void) uftdstat(1,temp);
                return cf; } /* should be errno */
            /* belt and suspenders: chown meta file for AIX */
/*          (void) chown(cffn,uuid);    */
/*          (void) chown(wffn,uuid);    */
            (void) chown(wffn,uuid,UFT_GID);
            /* and move previously stored statements into it */
            (void) uftdmove(cf,tf);

            /* and open the data file */
            (void) sprintf(dffn,"%04d.df",n);
            df = open(dffn,O_WRONLY|O_CREAT,S_IREAD);
            if (df < 0)
              { (void) sprintf(temp,
                        "535 %d; user data file error.",errno);
                (void) uftdstat(1,temp);
                (void) close(cf);       (void) close(tf);
                return df; } /* should be errno */
            /* belt and suspenders: chown data file for AIX */
/*          (void) chown(dffn,uuid);    */
            (void) chown(dffn,uuid,UFT_GID);

            /* and open the AUX data file */
            /* (extended attributes or resource fork) */
            (void) sprintf(effn,"%04d.ef",n);
            ef = open(effn,O_WRONLY|O_CREAT,S_IREAD);
            if (ef < 0)
              { (void) sprintf(temp,
                        "535 %d; user auxdata file error.",errno);
                (void) uftdstat(1,temp);
                (void) close(cf);       (void) close(tf);
                (void) close(df);
                return ef; } /* should be errno */
            /* belt and suspenders: chown file for AIX */
/*          (void) chown(effn,uuid);    */
            (void) chown(effn,uuid,UFT_GID);

            /* all okay! */
            (void) sprintf(temp,"208 %s; user %s okay",user,user);
            (void) uftdstat(1,temp);
            continue;
          }

/* -------------------------------------------------------- DATA command
 */
        if (abbrev("DATA",p,3))
          { if (df < 0)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);
                continue; }
            i = atoi(q);
            (void) sprintf(temp,"313 %d; send %d bytes of data.",i,i);
            (void) uftdstat(1,temp);

            i = uftddata(df,0,i);
            if (i < 0)
              { (void) sprintf(temp,"513 %d; server data error.",errno);
                (void) uftdstat(1,temp);
                (void) close(df);
                (void) close(cf);
                return n; } /* should be errno */
            (void) sprintf(temp,"213 %d; received %d bytes of data.",i,i);
            (void) uftdstat(1,temp);
            uftfile0.size += i;
            continue;
          }

        if (abbrev("AUXDATA",p,4))
          { if (ef < 0)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);
                continue; }
            i = atoi(q);
            (void) sprintf(temp,"313 %d; send %d bytes of data.",i,i);
            (void) uftdstat(1,temp);

            i = uftddata(ef,0,i);
            if (i < 0)
              { (void) sprintf(temp,"513 %d; server data error.",errno);
                (void) uftdstat(1,temp);
                (void) close(ef);
                (void) close(df);
                (void) close(cf);
                return n; } /* should be errno */
            (void) sprintf(temp,"213 %d; received %d bytes of data.",i,i);
            (void) uftdstat(1,temp);
            continue;
          }

/* --------------------------------------------------------- EOF command
 */
        if (abbrev("EOF",p,1))
          { /* close files */
            (void) close(ef);   ef = -1;
            (void) close(df);   df = -1;
            (void) close(cf);   cf = -1;

            /* rename the work file as the control file */
            (void) rename(wffn,cffn);

            /* belt and suspenders for AIX: chown user files */
/*          (void) chown(cffn,uuid);    */
            (void) chown(cffn,uuid,UFT_GID);
/*          (void) chown(dffn,uuid);    */
            (void) chown(dffn,uuid,UFT_GID);
/*          (void) chown(effn,uuid);    */
            (void) chown(effn,uuid,UFT_GID);

            /* now lose the spoolid number */
            n = -1;

            /* and clear filenames */
            effn[0] = 0x00;  dffn[0] = 0x00;
            cffn[0] = 0x00;  wffn[0] = 0x00;

            /* at this point, signal ACK to client */
            (void) tcpputs(1,"200 ACK EOF");

            /* make notification of file's arrival */
            (void) uftdimsg(user,seqs,from,type); /* IMSG */
            (void) uftdlmsg(user,seqs,from,type); /* SYSLOG */
            (void) uftdlist(atoi(seqs),from); /* ala 'ls' */
            user[0] = 0x00;  /* now reset that value */

            /* go back to being root */
            (void) seteuid(0);

            /* get back into the UFT spool directory */
            if (chdir(UFT_SPOOLDIR) < 0) break;

            /* all clear; kill the log file */
            (void) close(tf);   tf = -1;
            (void) unlink(tffn);
            /* but don't bother clearing that string
               because we're about to reload it */

            /* and get another server sequence number */
            n = uftdnext();  if (n < 0) break;
            (void) sprintf(tffn,"%s/%04d.cf",UFT_SPOOLDIR,n);
            tf = open(tffn,O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
            if (tf < 0) break;

#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"118 SEQ=%04d (server)",n);
            (void) uftdstat(1,temp);
#endif

            continue;
          }

/* --------------------------------------------------------- CPQ command
 */
        if (abbrev("CPQ",p,3))
          { (void) sprintf(temp,"402 CPQ; CPQ not implemented.");
            (void) uftdstat(1,temp);
            continue; }

/* --------------------------------------------------------- MSG command
 */
        if (abbrev("MSG",p,1))
          { (void) sprintf(temp,"402 MSG; MSG not implemented.");
            (void) uftdstat(1,temp);
            continue; }

/* -------------------------------------------------------- TYPE command
 */
        if (abbrev("TYPE",p,1))
          {
            /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uft_putline(tf,temp);
            if (cf >= 0) (void) uft_putline(cf,temp);

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);

            type[0] = q[0];             type[1] = 0x00;
            uftfile0.type[0] = q[0];    uftfile0.type[1] = 0x00;

            continue;
          }

/* -------------------------------------------------------- NAME command
 */
        if (abbrev("NAME",p,2))
          {
            /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uft_putline(tf,temp);
            if (cf >= 0) (void) uft_putline(cf,temp);

            /*  copy filename into member in structure  */
            (void) strncpy(uftfile0.name,q,sizeof(uftfile0.name));

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);
            continue;
          }

/* ------------------------------------------------- other META commands
 */
        /*  is it a known "attribute" command?  */
        if (abbrev("TYPE",p,2)  |  
            abbrev("DATE",p,2)  |   abbrev("XDATE",p,2) |
            abbrev("PERM",p,4)  |   abbrev("CHARSET",p,5) |
            abbrev("UCS",p,3)   |   abbrev("TRAIN",p,2) |
            abbrev("RECFMT",p,4) |  abbrev("RECORD_FORMAT",p,8) |
            abbrev("LRECLEN",p,5) |
            abbrev("RECLEN",p,4) |  abbrev("RECORD_LENGTH",p,8) |
            abbrev("CLASS",p,2) |   abbrev("FORM",p,2)|
            abbrev("FCB",p,3)   |   abbrev("CTAPE",p,2) |
            abbrev("DESTINATION",p,4) |
            abbrev("DISTRIBUTION",p,4) |
            abbrev("TITLE",p,2))
          {
            /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uft_putline(tf,temp);
            if (cf >= 0) (void) uft_putline(cf,temp);

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);
            continue;
          }

        /*  otherwise ... some unknown command  */
        (void) sprintf(temp,"402 %s; %s not implemented.",p,p);
        (void) uftdstat(1,temp);
      }

    /*  better to clean-up partials here  */

    /*  if (rc == 0)  */
    (void) tcpputs(1,"221 goodbye.");
    (void) close(tf);

    return 0;
  }


