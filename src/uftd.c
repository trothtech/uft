/* Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
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

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "uft.h"
char           *arg0;
int             tf, cf, df;     /* temp, meta, and data file handles */
char            tffn[64], cffn[64], dffn[64];
int             ef;             /* new aux-data (ext attr) file */
                                /* can it hold the Mac resource fork? */
char            effn[64];

struct  UFTFILE  uftfile0;

extern int uftcflag, uftlogfd;    

/* ----------------------------------------------------------- UFTD_PREF
 *    Preface a string ahead of the current content of another string.
 *    "head" (first arg) is the intended head-of string
 *    "tail" (second arg) is the intended end-of string and the target
 *    "size" (third arg) is the size of the buffer holding "tail"
 */
int uftd_pref(char*head,char*tail,int size)
  { static char _eyecatcher[] = "uftd_pref()";
    int l1, l2, i, j;

    l1 = strlen(head);
    l2 = strlen(tail);
    if (l1 + l2 > size - 1)
      { errno = EOVERFLOW;         /* best but is more about wordsize */
        /* alternative constants might be EMSGSIZE, ENOMEM, or ENOSPC */
        return -1; }

    /* shift the current content down the string (shift bytes right)  */
    i = l2; j = l2 + l1;
    while (i >= 0) tail[j--] = tail[i--];

    /* copy the new head-of-string content into place (fill-in left)  */
    for (i = 0; i < l1; i++) tail[i] = head[i];

    return 0;
  }

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftd.c main()";
    char        line[256], temp[256], user[64], auth[256], *p, *q;
    int         n, uuid, i, rc;
    char        type[16], seqs[8], from[256];
    char        wffn[64];
    char        *mv[8], bss[16];

    uftcflag = 0x00000000;      /* default */

#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: starting\n");
#endif

    /* under what name are we running? */
    arg0 = argv[0];

    /* preset some things */
    n = tf = cf = df = ef = -1;
    user[0] = 0x00;  auth[0] = 0x00;
    type[0] = 0x00;  seqs[0] = 0x00;  from[0] = 0x00;
    uftlogfd = -1;

    uftfile0.type[0]    = '?';  uftfile0.type[1]    = 0x00;
    uftfile0.cc[0]      = '-';  uftfile0.cc[1]      = 0x00;
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
        (void) uftdstat(1,temp);    /* error to client but no logging */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 522 spool dir unavail\n");
#endif
        return 1; }                    /* spool directory unavailable */
    /* Leave control in the sysadmin's hands as much as possible.     */
    /* DO NOT create the UFT spool directory; let him do it manually. */

    /* get next temp control file in sequence */
    n = uftdnext();
    if (n < 0)
      { /* "no spoolids available" (or no permission?) */
        (void) sprintf(temp,"523 workspace sequence error. (%d)",errno);
        (void) uftdstat(1,temp);    /* error to client but no logging */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 523 sequence error %d\n",n);
#endif
        return n; }         /* workspace sequence error "no spoolids" */

    /* now open a temporary control file for logging */
    (void) sprintf(tffn,"%s/%04d.lf",UFT_SPOOLDIR,n);
    uftlogfd = tf = open(tffn,O_RDWR|O_CREAT,S_IRUSR);
    if (tf < 0)
      { (void) sprintf(temp,"524 server temp file error. (%d)",errno);
        (void) uftdstat(1,temp);        /* stat and logging ... maybe */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 524 temp file error\n");
#endif
        return tf; }                   /* open of control file failed */

    (void) sprintf(temp,"#*%s server",UFT_VERSION);
    (void) uftx_putline(tf,temp,0);                        /* logging */

    /* all okay thus far, now send the herald (informative, but ACK)  */

    (void) gethostname(temp,sizeof(temp));
    mv[0] = arg0;     /* message formatter does not actually use this */
    mv[1] = temp;
    mv[2] = UFT_PROTOCOL;
    mv[3] = /* UFT_VERSION */ UFT_VRM;
    sprintf(bss,"%d",BUFSIZ);
    mv[4] = bss;

#ifdef          UFT_ANONYMOUS
//  (void) sprintf(temp,"*anonymous");
    rc = uftx_message(line,sizeof(line),224,"SRV",5,mv);
/*    224 *anonymous UFT/2 UFT/redacted 0 ; ready.                    */
#else
    rc = uftx_message(line,sizeof(line),223,"SRV",5,mv);
/*    223 &1 UFT/2 POSIXUFT/&3 &4 ; ready.                            */
#endif

/*  (void) sprintf(line,"112 %s %s %s %d ready.",                     */
/*  (void) sprintf(line,"222 %s %s %s %d ; ready.",                // */
/* 222 ibmisv.marist.edu UFT/2 VMCMSUFT/1.10.5 ; ready.               */
/*              temp,UFT_PROTOCOL,UFT_VERSION,BUFSIZ);             // */

    p = line;
    while (*p != 0x00 && *p != ' ') p++;    /* skip past message code */
    if (*p == ' ') p++; if (*p == ' ') p++; if (*p == ' ') p++;
    (void) uftdstat(1,p);        /* 200 series herald indicates UFT/2 */

    /* who's on the other end of the socket? */
    (void) tcpident(0,from,256);      /* IDENT got a bad rap and died */
    (void) sprintf(line,"REMOTE=%s",from);
    (void) uftx_putline(tf,line,0);                        /* logging */
    (void) strncpy(uftfile0.from,from,sizeof(uftfile0.from));

    /* where exactly does this item fit? */
#ifndef         UFT_ANONYMOUS
    (void) sprintf(temp,"118 SEQ=%04d (server)",n);
    (void) uftdstat(1,temp);                      /* stat and logging */
#endif

    /* loop forever on commands from client */
    while (1)
      { /*  read a line  */
        if (uftx_getline(0,line,sizeof(line)-1) < 0)
          { (void) sprintf(temp,"#*socket closed w/o QUIT.");
/*          (void) uftdstat(1,temp);                                  */
            /* the following should be handled outside of the loop    */
            if (*cffn != 0x00 && *wffn != 0x00) rename(wffn,cffn);
            if (*dffn != 0x00) (void) unlink(dffn);
            if (*effn != 0x00) (void) unlink(effn);
            break; }                 /* FIXME: need to set an RC here */

        /* skip leading white space */
        for (p = line; *p <= ' ' && *p != 0x00; p++);

        /* don't even log empty lines - skip the next few statements  */
        if (*p == 0x00) continue;

        /* log everything as comments in the control file(s) */
        (void) sprintf(temp,"#<%s",p);
        if (tf >= 0) (void) uftx_putline(tf,temp,0);       /* logging */
/*      if (cf >= 0) (void) uftx_putline(cf,temp,0);                  */

        /* don't process comments any further, just log them */
        if (*p == '*') continue;
        if (*p == '#') continue;

        /* copy, upcase, and delimit the verb from any following args */
        for (q = line; *p > ' '; *q++ = toupper(*p++));
        if (*q != 0x00) *q++ = 0x00;
        p = line;
        /* q now points to the args (if any), and p to the verb       */

        /* differentiate "META" from other commands ----------------- */
        if (abbrev("META",p,4))
          { char *p2, *q2;                      /* alternate pointers */

            /* skip leading white space */
            for (p2 = q; *p2 <= ' ' && *p2 != 0x00; p2++);

            /* copy, upcase, and delimit the verb from the args       */
            for (q2 = q; *p2 > ' '; *q2++ = toupper(*p2++));
            if (*q2 != 0x00) *q2++ = 0x00;
            p2 = q;
            /* q2 now points to the args (if any), and p2 to metaverb */

/* FIXME: we need a safety scan both here and "traditional" (below)   */

/*          if (abbrev("NAME",p,2)) ... then put it into the struct   */

            /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p2,q2);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);   /* logging */
            if (cf >= 0) (void) uftx_putline(cf,temp,0);

            /* ACK to the client */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);              /* stat and logging */
            continue;                           /* continue after ACK */
          }

        /* --------------------------------------------- FILE command */
        if (abbrev("FILE",p,4))
          { /* parse another (SIZE) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            (void) sprintf(temp,"SIZE='%s'",p);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);   /* logging */
            if (cf >= 0) (void) uftx_putline(cf,temp,0);
            /* parse another (FROM) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            if (*p != 0x00)
              { (void) sprintf(temp,"FROM='%s'",p);
                if (tf >= 0) (void) uftx_putline(tf,temp,0);
                if (cf >= 0) (void) uftx_putline(cf,temp,0);
                if (*from == '@') { uftd_pref(p,from,sizeof(from));
                    strncpy(uftfile0.from,from,sizeof(uftfile0.from)); }
              }
            /* parse another (AUTH) */
            for (p = q; *q > ' '; q++);
            if (*q != 0x00) *q++ = 0x00;
            if (*p != 0x00)
              { (void) strncpy(auth,p,256);
                (void) sprintf(temp,"AUTH='%s'",auth);
                if (tf >= 0) (void) uftx_putline(tf,temp,0);
                if (cf >= 0) (void) uftx_putline(cf,temp,0); }

/* FIXME: if AUTH=AGENT then parse more and check it
//              if (strcasecmp(auth,"AGENT") == 0)
//                { for (p = q; *q > ' '; q++);
//                  if (*q != 0x00) *q++ = 0x00;
//                  if (*p != 0x00)
//                  uftd_agck(,p,)
//                }                                                   */

            else (void) strncpy(auth,"N/A",256);

            /* ACK to the client */
            (void) tcpputs(1,"200 ACK FILE");
            continue;                           /* continue after ACK */
          }

        /* --------------------------------------------- EXIT command */
        /* --------------------------------------------- QUIT command */
        if (abbrev("EXIT",p,2) || abbrev("QUIT",p,3))
          { (void) seteuid(0);
            if (df >= 0 && cf >= 0) unlink(tffn);       /* QUIT, EXIT */
            break; }         /* "221 goodbye" follows outside of loop */

        /* --------------------------------------------- HELP command */
        if (abbrev("HELP",p,1))
          { (void) sprintf(temp,"114 HELP: protocol: %s",UFT_PROTOCOL);
            (void) tcpputs(1,temp);
#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"114 HELP: server: %s",UFT_VERSION);
            (void) tcpputs(1,temp);
#endif
            (void) tcpputs(1,"114 HELP: commands:");
            (void) tcpputs(1,"114 HELP: FILE <size> <from> <auth>");
            (void) tcpputs(1,"114 HELP: USER <recipient>");
            (void) tcpputs(1,"114 HELP: TYPE <filetype>");
            (void) tcpputs(1,"114 HELP: NAME <filename>");
            (void) tcpputs(1,"114 HELP: DATA <burst_size>");
            (void) tcpputs(1,"114 HELP: QUIT");
            (void) tcpputs(1,"214 HELP: end of HELP");
            continue;                           /* continue after ACK */
          }

        /* --------------------------------------------- USER command */
        if (abbrev("USER",p,1))
          { /* one at a time, please */
            if (user[0] != 0x00)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);  /* signal 4xx NAK to client */
                continue; }                 /* continue after 4xx NAK */

            /* make sure FILE command preceeded USER command */
            if (auth[0] == 0x00)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);  /* signal 4xx NAK to client */
                continue; }                 /* continue after 4xx NAK */

            /* discard any extra tokens */
            p = user;
            while (*q > ' ') *p++ = tolower(*q++);
            *p = 0x00;

            /* now log it */
            (void) sprintf(temp,"USER='%s'",user);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);   /* logging */
            if (cf >= 0) (void) uftx_putline(cf,temp,0);

            /* now try to run with it */
            uuid = uftduser(user);
            if (uuid < 0)
              { (void) sprintf(temp,
                    "532 %s %d; no such local user or queue %s.",
                        user,errno,user);
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 532 no such local user %d\n",uuid);
#endif
                user[0] = 0x00;
                continue; }        /* return uuid; ** should be errno */

            /* get the next sequence number for this user */
            n = uftdnext();
            if (n < 0 && errno == EACCES)
              { (void) seteuid(0);
                n = uftdnext(); }
            if (n < 0)
              { (void) sprintf(temp,
                          "527 user slot error UID=%d EUID=%d ERRNO=%d",
                               getuid(),geteuid(),errno);
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 527 user slot error %d\n",n);
#endif
                user[0] = 0x00;
                continue; } /* return n; ** should be errno */
            else (void) sprintf(seqs,"%04d",n);

            /* okay so far; report the sequence number */
#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"119 SEQ=%s (user)",seqs);
            (void) uftdstat(1,temp);                   /* spontaneous */
#endif

            /* now open the *real* control file (meta file) */
            (void) sprintf(cffn,"%04d.cf",n);
            (void) sprintf(wffn,"%04d.wf",n);
            cf = open(wffn,O_WRONLY|O_CREAT,S_IRUSR);
            if (cf < 0)
              { (void) sprintf(temp,"534 %d; meta file error.",
                        errno);
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 534 meta file error %d\n",cf);
#endif
                continue; }          /* return cf; ** should be errno */

            /* belt and suspenders: chown meta file for AIX */
/*          (void) chown(cffn,uuid);    */
/*          (void) chown(wffn,uuid);    */
            (void) chown(wffn,uuid,UFT_GID);
            /* and move previously stored statements into it */
            (void) uftdmove(cf,tf);

            /* and open the data file */
            (void) sprintf(dffn,"%04d.df",n);
            df = open(dffn,O_WRONLY|O_CREAT,S_IRUSR);
            if (df < 0)
              { (void) sprintf(temp,
                        "535 %d; user data file error.",errno);
/*                          FIXME: register this in the messages file */
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
                (void) close(cf);       (void) close(tf);
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 535 user data file error\n");
#endif
                continue; }          /* return df; ** should be errno */

            /* belt and suspenders: chown data file for AIX */
/*          (void) chown(dffn,uuid);    */
            (void) chown(dffn,uuid,UFT_GID);

            /* open AUX data file (extended attr or "resource fork")  */
            (void) sprintf(effn,"%04d.ef",n);
            ef = open(effn,O_WRONLY|O_CREAT,S_IRUSR);
            if (ef < 0)
              { (void) sprintf(temp,
                        "535 %d; user auxdata file error.",errno);
/*                          FIXME: register this in the messages file */
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
                (void) close(cf);       (void) close(tf);
                (void) close(df);
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 535 user auxdata file error\n");
#endif
                continue; }          /* return ef; ** should be errno */

            /* belt and suspenders: chown file for AIX */
/*          (void) chown(effn,uuid);    */
            (void) chown(effn,uuid,UFT_GID);

            /* all okay! */
            (void) sprintf(temp,"208 %s; user %s okay",user,user);
            (void) uftdstat(1,temp);              /* stat and logging */
            continue;                           /* continue after ACK */
          }

        /* --------------------------------------------- DATA command */
        if (abbrev("DATA",p,3))
          { if (df < 0)                  /* data file was not opened? */
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);          /* stat and logging */
                continue; }                 /* continue after 4xx NAK */
            i = atoi(q);
            (void) sprintf(temp,"313 %d; send %d bytes of data.",i,i);
            (void) uftdstat(1,temp);              /* stat and logging */

            /* eat the indicated number of bytes before next command  */
            i = uftddata(df,0,i);
            if (i < 0)
              { (void) sprintf(temp,"513 %d; server data error.",errno);
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
                (void) close(df);
                (void) close(cf);
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 513 server data error\n");
#endif
              continue; } /* return n; ** should be errno */

            (void) sprintf(temp,"213 %d; received %d bytes of data.",i,i);
            (void) uftdstat(1,temp);       /* ACK -- stat and logging */
            uftfile0.size += i;
            continue;                       /* ACK */
          }

        /* ---------------------------------------------------------- */
        if (abbrev("AUXDATA",p,4))
          { if (ef < 0)
              { (void) sprintf(temp,"403 protocol sequence error.");
                (void) uftdstat(1,temp);  /* signal 4xx NAK to client */
                continue; }                 /* continue after 4xx NAK */
            i = atoi(q);
            (void) sprintf(temp,"313 %d; send %d bytes of data.",i,i);
            (void) uftdstat(1,temp);              /* stat and logging */

            i = uftddata(ef,0,i);
            if (i < 0)
              { (void) sprintf(temp,"513 %d; server data error.",errno);
                (void) uftdstat(1,temp);  /* signal 5xx NAK to client */
                (void) close(ef); (void) close(df); (void) close(cf);
#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: 513 server data error\n");
#endif
                continue; }                 /* continue after 5xx NAK */

            (void) sprintf(temp,"213 %d; received %d bytes of data.",i,i);
            (void) uftdstat(1,temp);       /* ACK -- stat and logging */
            continue;                         /* ACK */
          }

        /* ---------------------------------------------- EOF command */
        if (abbrev("EOF",p,1))
          { /* close files */
            (void) close(ef); ef = -1;
            (void) close(df); df = -1;
            (void) close(cf); cf = -1;

            /* rename the work file as the control file */
            (void) rename(wffn,cffn);

            /* belt and suspenders for AIX: chown the user files      */
            (void) chown(cffn,uuid,UFT_GID);
            (void) chown(dffn,uuid,UFT_GID);
            (void) chown(effn,uuid,UFT_GID);

            /* and clear filenames */
            effn[0] = 0x00;  dffn[0] = 0x00;
            cffn[0] = 0x00;  wffn[0] = 0x00;

            /* at this point, signal ACK to client */
            (void) tcpputs(1,"200 ACK EOF");    /* ACK */

            /* notify the local user that a file has arrived -------- */
/*          (void) uftdimsg(user,seqs,from,type);  // see uftd_fann() */
/*          (void) uftdlmsg(user,seqs,from,type);           // SYSLOG */
            (void) uftdlist(atoi(seqs),from);             /* ala 'ls' */

            seteuid(0);                      /* go back to being root */
            uftd_fann(user,seqs,from);       /* this is a better IMSG */

            /* now lose the spoolid number and clear username         */
            user[0] = 0x00;           /* now reset the user value ... */
            n = -1;                /* ... and lose the spoolid number */

            /* get back into the UFT spool directory */
            if (chdir(UFT_SPOOLDIR) < 0) break;   /* FIXME: should be 5xx */

            /* all clear; kill the log file */
            close(tf); uftlogfd = tf = -1; unlink(tffn);
            /* but don't bother clearing that string
               because we're about to reload it */

            /* and get another server sequence number */
            n = uftdnext();  if (n < 0) break;
            (void) sprintf(tffn,"%s/%04d.cf",UFT_SPOOLDIR,n);
            uftlogfd = tf = open(tffn,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
            if (tf < 0) break;
#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"118 SEQ=%04d (server)",n);
            (void) uftdstat(1,temp);       /* ACK -- stat and logging */
#endif
            continue;           /* ACK */
          }

        /* -------------------------------------------- ABORT command */
        if (abbrev("ABORT",p,1))
          { /* close files */
            close(ef); ef = -1;
            close(df); df = -1;
            close(cf); cf = -1;

            /* discard the work file and would-be deliverables        */
            (void) unlink(wffn);
            (void) unlink(cffn);         /* this is probably an error */
            (void) unlink(dffn);
            (void) unlink(effn);

            /* and clear filenames */
            effn[0] = 0x00;  dffn[0] = 0x00;
            cffn[0] = 0x00;  wffn[0] = 0x00;

            /* at this point, signal ACK to client */
            (void) tcpputs(1,"226 ACK ABORT");      /* "rollback" ACK */

            /* now lose the spoolid number, clear user, reset eUID    */
            n = -1;                    /* now lose the spoolid number */
            user[0] = 0x00;           /* now reset the user value ... */
            (void) seteuid(0);       /* ... and go back to being root */

            /* get back into the UFT spool directory */
            if (chdir(UFT_SPOOLDIR) < 0) break;   /* FIXME: should be 5xx */

            /* all clear; now kill the log file */
            close(tf); uftlogfd = tf = -1; unlink(tffn);

            /* and get another server sequence number */
            n = uftdnext();  if (n < 0) break;
            (void) sprintf(tffn,"%s/%04d.cf",UFT_SPOOLDIR,n);
            uftlogfd = tf = open(tffn,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
            if (tf < 0) break;
#ifndef         UFT_ANONYMOUS
            (void) sprintf(temp,"118 SEQ=%04d (server)",n);
            (void) uftdstat(1,temp);       /* ACK -- stat and logging */
#endif

            continue;           /* ACK */
          }

        /* -------------------------------------------- AGENT command */
        if (abbrev("AGENT",p,4))
          { int rc;             /* we might should have RC everywhere */
            rc = uftd_agck(q);
            switch (rc)
              {
                case 2: sprintf(temp,"200 ACK AGENT"); break;
                case 4: sprintf(temp,"400 NAK AGENT"); break;
                case 5: sprintf(temp,"500 NAK AGENT"); break;
                default: sprintf(temp,"500 NAK AGENT");
              }
            uftdstat(1,temp);            /* send ACK or NAK to client */
            continue; }                  /* continue after ACK or NAK */



        /* ---------------------------------------------- CPQ command */
        if (abbrev("CPQ",p,3))
          { int rc;             /* we might should have RC everywhere */
            char cpqstr[256], *r;

            rc = uftdcpq(q,cpqstr,sizeof(cpqstr)-1);
            switch (rc) { case 0:
                              uftdl699(1,cpqstr);
                              uftdstat(1,"200 ACK");
                              break;
                          case 2: case 3: case 4: case 5:
                              r = cpqstr; while (*r > ' ') r++;
                                          while (*r == ' ') r++;
                              uftdstat(1,r);      /* stat and logging */
                              break;
                          default:
                              uftdstat(1,"500 internal error");
                              break; }
            continue; }                     /* continue after 4xx NAK */



        /* ---------------------------------------------- MSG command */
        if (abbrev("MSG",p,1))                /* p points to the verb */
          { int rc;             /* we might should have RC everywhere */
            char *u, *m;

            /* parse args of MSG command for user and message text    */
            u = m = q;                        /* q points to the args */
            while (*m != ' ' && *m != 0x00) { *m = tolower(*m); m++; }
            if (*m == ' ') *m++ = 0x00;

            /* now try to deliver the message                         */
            rc = msgd_umsg(u,m,from);
            if (rc < 0) sprintf(temp,"500 MSG RC=%d",rc);
                   else sprintf(temp,"200 %s; %s okay",p,p);
            uftdstat(1,temp);          /* signal ACK or NAK to client */
            continue;                    /* continue after ACK or NAK */
          }

        /* --------------------------------------------- TYPE command */
        if (abbrev("TYPE",p,1))
          { /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);
            if (cf >= 0) (void) uftx_putline(cf,temp,0);
            type[0] = q[0];             type[1] = 0x00;
            uftfile0.type[0] = q[0];    uftfile0.type[1] = 0x00;

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);          /* signal ACK to client */
            continue;                           /* continue after ACK */
          }

        /* --------------------------------------------- NAME command */
        if (abbrev("NAME",p,2))
          {
            /* put this variable SAFELY into the control file         */
            q = uftx_basename(q);
            uftx_parse1(q);
            sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);
            if (cf >= 0) (void) uftx_putline(cf,temp,0);

            /*  copy filename into member in structure  */
            (void) strncpy(uftfile0.name,q,sizeof(uftfile0.name));

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);          /* signal ACK to client */
            continue;                           /* continue after ACK */
          }

        /* known attribute? --------------------- other META commands */
        if (abbrev("DATE",p,2)  |   abbrev("XDATE",p,2) |
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
          { /*  put this variable into the control file  */
            (void) sprintf(temp,"%s='%s'",p,q);
            if (tf >= 0) (void) uftx_putline(tf,temp,0);
            if (cf >= 0) (void) uftx_putline(cf,temp,0);

            /*  ACK to the client  */
            (void) sprintf(temp,"200 %s; %s okay",p,p);
            (void) uftdstat(1,temp);          /* signal ACK to client */
            continue;                           /* continue after ACK */
          }

        /*  otherwise ... some unknown command  */
        (void) sprintf(temp,"402 %s; %s not implemented.",p,p);
        (void) uftdstat(1,temp);          /* signal 4xx NAK to client */
                                            /* continue after 4xx NAK */
      }

    /*  better to clean-up partials here  */
    if (cf > 0) { close(cf); /* maybe unlink dangling wffn */ }
    if (df > 0) { close(df); /* maybe unlink dangling dffn */ }
    if (ef > 0) { close(ef); /* maybe unlink dangling effn */ }

    (void) tcpputs(1,"221 goodbye.");
    (void) close(tf);

#ifdef _UFT_DEBUG
    fprintf(stderr,"UFTD: normal termination\n");
#endif

    sleep(2);

    return 0;
  }


