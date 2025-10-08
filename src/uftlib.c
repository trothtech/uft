/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftlib.c (C program source)
 *              Unsolicited File Transfer client/server library
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-04-08 (Tuesday)
 *
 *        Note: This is in response to the need for an actual LIBRARY.
 *              Individual functions will move here as needed.
 *
 *        Note: This is for UFT but includes MSG functions also.
 *

needed:
int uftc_open(char*,char*,int*);
uftc_write()
uftc_read()
int uftc_close(int*);

 */

#ifndef PREFIX
 #define PREFIX "/usr"
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <arpa/inet.h>
 #include <pwd.h>
 #include <errno.h>
#endif

#include <ctype.h>

#define __USE_XOPEN
#include <time.h>

/* OpenVM is a POSIX environment for VM/CMS so set the OECS symbol    */
#ifdef          __OPEN_VM
 #ifndef        OECS
  #define       OECS
 #endif
#endif

#ifdef UFT_DO_SYSLOG
 #include <syslog.h>
#endif

/* The following three lines are related to the XMITMSGX package      *
 * which is maintained separately from the UFT package.               */
#include "xmitmsgx/xmitmsgx.h"
char *xmmprefix = PREFIX;            /* tells XMM to share our prefix */
static struct MSGSTRUCT uftmsgs;      /* info for the message handler */

#include "uft.h"

int uftcflag;

static char agstring[256] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

int uftlogfd = -1;

/* ---------------------------------------------------------------------
 *    This routine handles message FORMATTING (not message delivery).
 *    It's a different way of doing gettext() type processing
 *    which is compatible with the 'XMITMSG' command and APPLMSG macro
 *    from VM/CMS land. (POSIX UFT shares the same messages with CMS.)
 */
int uftx_message(char*mo,int ml,                    /* buffer, buflen */
                 int mn,                            /* message number */
                 char*mq,                                   /* caller */
                 int mc,char*mv[])                      /* msgc, msgv */
  { static char _eyecatcher[] = "uftx_message()";
    int rc;
    char *p;

    /* Open the messages file, read it, get ready for service.        */
    rc = xmopen("uft",0,&uftmsgs);        /* FIXME: check indirection */
    if (rc != 0) return rc;

    if (mn < 0) mn = 0 - mn;   /* force message number to be positive */

    /* do we need this? */
    uftmsgs.msglevel = 0;

    /* using pfxmaj and pfxmin is definitely outside the XMITMSGX API */
    strncpy(uftmsgs.pfxmaj,"UFT",4);
    strncpy(uftmsgs.pfxmin,mq,4);
    /* also remember to up-case the latter */
    uftmsgs.pfxmin[3] = 0x00;
    for (p = uftmsgs.pfxmin; *p != 0x00; p++) if (islower(*p)) *p = toupper(*p);

    /* Generate a message and store it as a string.                   */
    rc = xmstring(mo,ml,mn,mc,(unsigned char**)mv,&uftmsgs);

    return rc;
  }

/* -------------------------------------------------------- User Message
 *  This routine attempts to deliver a message to a logged-on user.
 *  This is somewhat crude: we toss the work of finding the user
 *  and delivering the message to the 'write' command. But be careful:
 *  on MS Windows there is a completely different 'write' command.
 */
int uftd_message(char*user,char*text)
  { static char _eyecatcher[] = "uftd_message()";
    int rc, fd;
    char fn[64], ts[256];

    /* create and open a temporary file to hold the message text      */
    strcpy(fn,"/tmp/uftd-message-XXXXXX");
    fd = mkstemp(fn);

    /* dump the message text into that file and add a newline         */
    write(fd,text,strlen(text));
    write(fd,"\n",1);
    close(fd);

    /* now invoke 'write' taking the temporary file as input          */
    sprintf(ts,"write %s 0< %s 1> /dev/null 2> /dev/null",user,fn);
    rc = system(ts);
    unlink(fn);

    /* return code is that of the command issued via system() call    */
    return rc;
  }

/* ---------------------------------------------------------------------
 *    NOTE: this routine is NOT prototyped and not called from outside
 */
int msgd_xmsg_sock(char*user,char*buff,int bl)
  { static char _eyecatcher[] = "msgd_xmsg_sock()";
#ifdef UFT_POSIX
    int rc, sd;
    char fn[256];
    struct sockaddr_un msgsockst;

    /* first try using AF_UNIX (that is AF_LOCAL, named socket)       */
    rc = sd = socket(AF_UNIX,SOCK_STREAM,0); if (rc < 0) return rc;

    snprintf(fn,sizeof(fn),"%s/%s/.msgsock",UFT_SPOOLDIR,user);

    /* the server should be listening on this named socket */
    msgsockst.sun_family = AF_UNIX;
    strncpy(msgsockst.sun_path,(char*)fn,sizeof(msgsockst.sun_path)-1);

    /* if this connects then return socket descriptor and we're done */
    rc = connect(sd,(struct sockaddr *)&msgsockst,
                                    sizeof(struct sockaddr_un));

#ifdef _MSG_TRY_HOMEDIR
    if (rc < 0) {
    snprintf(fn,sizeof(fn),"%s/.msgsock",uftx_home(user));

    /* the server should be listening on this named socket */
    msgsockst.sun_family = AF_UNIX;
    strncpy(msgsockst.sun_path,(char*)fn, sizeof(msgsockst.sun_path)-1);

    /* if this connects then return socket descriptor and we're done */
    rc = connect(sd,(struct sockaddr *)&msgsockst,
                                    sizeof(struct sockaddr_un));
      }
#endif
    if (rc < 0) { close(sd); return rc; }

    rc = tcpwrite(sd,buff,bl);
    close(sd);
    return rc;
#else
    return -1;
#endif
  }

/* ---------------------------------------------------------------------
 *    NOTE: this routine is NOT prototyped and not called from outside
 */
int msgd_xmsg_fifo(char*user,char*buff,int bl)
  { static char _eyecatcher[] = "msgd_xmsg_fifo()";
    int rc, fd;
    char fn[256];

    snprintf(fn,sizeof(fn),"%s/%s/.msgpipe",UFT_SPOOLDIR,user);
#ifdef UFT_POSIX
    rc = fd = open(fn,O_WRONLY|O_NDELAY);
#else
    rc = fd = open(fn,O_WRONLY);
#endif

#ifdef _MSG_TRY_HOMEDIR
    if (rc < 0) {
    snprintf(fn,sizeof(fn),"%s/.msgpipe",uftx_home(user));
    rc = fd = open(fn,O_WRONLY|O_NDELAY);
#endif
    if (rc < 0) return rc;

    rc = write(fd,buff,bl);
    close(fd);
    return rc;
  }

/* -------------------------------------------------------- User Message
 *    This routine attempts to deliver a message to a local user.
 *       Calls: msgd_xmsg_sock(), msgd_xmsg_fifo(), uftd_message()
 */
int msgd_umsg(char*user,char*text,char*from)
  { static char _eyecatcher[] = "msgd_umsg()";
    int rc, i, l, fd;
    char *p, *q, buffer[4096], fn[256], un[64], *mv[8];

    /* parse the supplied user name */
    p = user; i = 0;
    while (i < sizeof(un) - 1 && *p != 0x00 && *p != '@')
      { un[i++] = *p++; }
    un[i] = 0x00;

    /* start at the start of the buffer and note its size for limit   */
    q = buffer; i = 0;
    l = sizeof(buffer) - 2;

    /* first thing in the batch is the message text (no tag or var)   */
    p = text;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }       /* terminate copied string */

    /* we don't always care about the message type but we always tell */
    p = "MSGTYPE=UMSG";         /* 1 - MSG       */
/*  p = "MSGTYPE=WMSG";         ** 2 - WNG, WMSG */
/*  p = "MSGTYPE=CMSG";         ** 3 - CPCONIO   */
/*  p = "MSGTYPE=SMSG";         ** 4 - SMSG      */
/*  p = "MSGTYPE=VMSG";         ** 5 - VMCONIO   */
/*  p = "MSGTYPE=EMSG";         ** 6 - EMSG      */
/*  p = "MSGTYPE=IMSG";         ** 7 - IMSG      */
/*  p = "MSGTYPE=FMSG";         ** 8 - SCIF      */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* this is obvious to the receiving user but we include it anyway */
    p = "MSGUSER=";              /* who is this message to? (obvious) */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = user;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* helps to know the sender of this message (user@host style)     */
    p = "MSGFROM=";                      /* who is this message from? */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = from;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* double NULL marks end of environment variables                 */
    if (i < l) { *q++ = 0x00; i++; }

    /* at this point we have a buffer and we know its length          */
    /* try: socket, home socket, FIFO, home FIFO, 'write'             */

    /* -------- try socket ------------------------------------------ */
    rc = msgd_xmsg_sock(un,buffer,i);
    if (rc >= 0) return rc;            /* zero or positive is success */

    /* -------- try FIFO -------------------------------------------- */
    rc = msgd_xmsg_fifo(un,buffer,i);
    if (rc >= 0) return rc;            /* zero or positive is success */

    /* -------- try brute force ------------------------------------- */
      { char bff[256], *bfh;

        strncpy(bff,from,sizeof(bff)-1);
        bfh = bff;
        while (*bfh != 0x00 && *bfh != '@') bfh++;
        if (*bfh == '@') *bfh++ = 0x00;

        mv[1] = bfh; mv[2] = bff; mv[3] = text;    /* From &1(&2): &3 */
        rc = uftx_message(buffer,sizeof(buffer)-1,96,"SRV",4,mv);

/* reusing p ... and this needs to be fixed in the XMM package */
        p = buffer; while (*p != ' ' && *p != 0x00) p++;
                    while (*p == ' ') p++;

        rc = uftd_message(user,p);
      }

    return rc;
  }

/* -------------------------------------------------- File Announce FANN
 *    This routine announces the arrival of a file.
 *       Calls: msgd_xmsg_sock(), msgd_xmsg_fifo(), uftd_message()
 */
int uftd_fann(char*user,char*spid,char*from)
  { static char _eyecatcher[] = "uftd_fann()";
    int rc, i, l, fd;
    char *p, *q, buffer[4096], fn[256], un[64], *mv[8];

    /* parse the supplied user name */
    p = user; i = 0;
    while (i < sizeof(un) - 1 && *p != 0x00 && *p != '@')
      { un[i++] = *p++; }
    un[i] = 0x00;

    /* start at the start of the buffer and note its size for limit   */
    q = buffer; i = 0;
    l = sizeof(buffer) - 2;

    mv[0] = "";
    mv[1] = spid; mv[2] = user; mv[3] = from;         /* three tokens */
    rc = uftx_message(q,l,94,"SRV",4,mv);          /* previously 1004 */
    if (rc < 0) return rc;

    while (*q != 0x00 && i < l) { q++; i++; }
    if (i < l) { q++; i++; }      /* skip one more past end of string */

    /* we don't always care about the message type but we always tell */
    p = "MSGTYPE=IMSG";         /* 7 - IMSG      */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }    /* terminate and skip pointer */

    /* this is obvious to the receiving user but we include it anyway */
    p = "MSGUSER=";                 /* who is this file to? (obvious) */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = user;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }    /* terminate and skip pointer */

    /* this is in the message but here too (varies from VM IMSG)      */
    p = "MSGFROM=";                          /* who is the file from? */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = from;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }    /* terminate and skip pointer */

    /* indicate the "spool ID"                                        */
    p = "UFTSPID=";
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = spid;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }    /* terminate and skip pointer */

    /* double NULL marks end of environment variables                 */
    if (i < l) { *q++ = 0x00; i++; }

    /* at this point we have a buffer and we know its length          */
    /* try: socket, home socket, FIFO, home FIFO, 'write'             */

    /* -------- try socket ------------------------------------------ */
    rc = msgd_xmsg_sock(un,buffer,i);
    if (rc >= 0) return rc;            /* zero or positive is success */

    /* -------- try FIFO -------------------------------------------- */
    rc = msgd_xmsg_fifo(un,buffer,i);
    if (rc >= 0) return rc;            /* zero or positive is success */

    /* -------- try brute force ------------------------------------- */
    rc = uftd_message(un,buffer);

    /* -------- merge uftdlmsg logic here --------------------------- */
#ifdef UFT_DO_SYSLOG
    openlog("uftd",LOG_PID|LOG_CONS,LOG_UUCP);
    syslog(LOG_INFO,"%s",buffer);
#endif

    return rc;
  }

/* � Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: userid.c
 *              return the login name associated with this process
 *      Author: Rick Troth, Rice University, Information Systems
 *        Date: 1994-Jul-26
 *
 * 1995-Apr-17: added useridg() function
 *
 */

/* -------------------------------------------------------------- USERID
 *    return login name from the best of several standard sources
 *
 *        Note: Size is limited to 63 characters because we force it
 *              to lower case (ths is POSIX!) using a private buffer.
 */
char *uftx_user()
  { static char _eyecatcher[] = "uftx_user()";
#ifdef UFT_POSIX
    int i;
    char *u;
    static char ut[64];
    struct passwd *pwdent;

    u = "";

    /* first try effective uid key into passwd */
/*  if (*u == 0x00)                           this would be redundant */
    pwdent = getpwuid(geteuid());
    if (pwdent) u = pwdent->pw_name; if (u == 0x0000) u = "";

    /* next try real uid key into passwd */
    if (*u == 0x00) {
    pwdent = getpwuid(getuid());
    if (pwdent) u = pwdent->pw_name; if (u == 0x0000) u = ""; }

    /* skating on thin ice: try USER environment variable */
    if (*u == 0x00) {
    u = getenv("USER"); if (u == 0x0000) u = ""; }

    /* last resort: try LOGNAME environment variable */
    if (*u == 0x00) {
    u = getenv("LOGNAME"); if (u == 0x0000) u = ""; }

    /* force the username to lower case (ths is POSIX!) */
    i = 0;
    while (*u != 0x00 && i < sizeof(ut) - 1)
      { if (isupper(*u)) ut[i] = tolower(*u);
                    else ut[i] = *u;
        i++; u++;
      } ut[i] = 0x00;
    return ut;
#else
    return "";
#endif
  }

#ifdef UFT_POSIX
#ifndef _OE_SOCKETS
/* ------------------------------------------------------------- USERIDG
 *    "g" for GECOS field, return personal name string, if available
 */
char *useridg()
  {
    char       *g;
    struct passwd *pwdent;

    /*  if the user set one, take that  */
    g = getenv("NAME");
    if (g != 0x0000 && *g != 0x00) return g;

    /*  next, try GECOS for effective uid key into passwd  */
    pwdent = getpwuid(geteuid());
    if (pwdent) return pwdent->pw_gecos;

    /*  next, try GECOS field for real uid key into passwd  */
    pwdent = getpwuid(getuid());
    if (pwdent) return pwdent->pw_gecos;

    /*  give up!  */
    return uftx_user();
  }
#endif
#endif

/* � Copyright 1996, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: imsg.c
 *              issue informational messages to yourself
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Jun-11, built from msgd.c, msgcat.c,
 *                              and from msgc.c (tell.c)
 *
 *       To do: an option to issue IMSGs to others
 *
 *              An IMSG is a local thing.
 *              At this time, IMSGs from user@host
 *              are meaningless unles host is the local system.
 *
 */

/* ------------------------------------------------------------ SENDIMSG
 */
int sendimsg ( char *user , char *text )
  {
    char        buffer[4096], *p, *from;
    int         fd, i;

    errno = 0;
        sprintf(buffer,"%s/.msgpipe",getenv("HOME"));
        fd = open(buffer,O_WRONLY);
    if (fd < 0) return fd;

    /*  build the buffer;  begin at offset zero  */
    i = 0;

    /* copy the message text first */
    p = text;  while (*p) buffer[i++] = *p++;  buffer[i++] = 0x00;

    /*  now environment variables;  first, who from?  */
    p = "MSGFROM=";  while (*p) buffer[i++] = *p++;
    p = from;  while (*p) buffer[i++] = *p++;  buffer[i++] = 0x00;

    /*  what type of message?  (MSP if by way of this server)  */
    p = "MSGTYPE=IMSG";  while (*p) buffer[i++] = *p++;
                                             buffer[i++] = 0x00;

    /*  also ... who's it too?  (in case that isn't obvious)  */
    p = "MSGUSER=";  while (*p) buffer[i++] = *p++;
    p = user;  while (*p) buffer[i++] = *p++;  buffer[i++] = 0x00;

    /*  an additional NULL terminates the environment buffer  */
    buffer[i++] = 0x00;

    /*  hand it off;  feed the FIFO  (hoping there's a listener)  */
    (void) write(fd,buffer,i);

    /*  all done,  so close the file descriptor  */
    (void) close(fd);

    /*  get outta here  */
    return 0;
  }

/* ------------------------------------------------------------ MSGLOCAL
 */
int msglocal(char*user,char*text)
  { static char _eyecatcher[] = "msglocal()";
    int         fd, i, j;
    char        temp[UFT_BUFSIZ], *from;

    /*  a 'mknod' with 622 perms (writable) might work too  */

    /*  if there's no listener ...  */
    if (fd < 0 && errno == ENXIO)
      {
        /*  launch our special application to listen  */
#ifdef UFT_POSIX
        fd = open(temp,O_WRONLY|O_NDELAY);
#else
        fd = open(temp,O_WRONLY);
#endif
        /*  ... or NOT ...  */
      }
    /* if (fd < 0) */ return fd;
  }

/* ----------------------------------------------------------- UFTX_HOME
 *    Return a pointer to a string with the home directory of this user.
 */
char*uftx_home(char*user)
  { static char _eyecatcher[] = "uftx_home()";
#ifdef UFT_POSIX
    int         i, uuid;
    struct passwd *pwdent;
    static char homedir[256];

    errno = 0;
    pwdent = getpwnam(user);
    if (pwdent == NULL) { perror("uftx_home()"); homedir[0] = 0x00; }
    else strncpy(homedir,pwdent->pw_dir,sizeof(homedir)-1);

    return homedir;
#else
    return "";
#endif
  }

/* ------------------------------------------------------------- GETLINE
 *        Name: GETLINE/UFTXGETS/UFTXRCVS
 *              common Get/Receive String function
 *   Operation: Reads a CR/LF terminated string from stream s
 *              into buffer b. Returns the length of that string.
 *      Author: Rick Troth, Ithaca NY, Houston TX (METRO)
 *        Date: 1993-Sep-19, Oct-20
 *
 *        Note: modified 1996-Jun-16 to support OpenEdition EBCDIC
 *
 *        Note: modified 2025-04-11 to have "safe" buffer limit
 *              but still byte-at-a-time slow, sorry about that
 *
 *    See also: putline() in this source unit
 *
 */
int uftx_getline(int s,char*b,int l)
  { static char _eyecatcher[] = "uftx_getline()";
    char *p;
    int i, rc;

#ifdef  OECS
    char        snl;
    snl = '\n';
#endif

    p = b; i = 0;
    while (i < l)
      { rc = read(s,p,1); if (rc != 1)                  /* get a byte */
        rc = recv(s,p,1,0); if (rc != 1)                  /* Win hack */
        rc = read(s,p,1); if (rc != 1)                   /* try again */
        rc = recv(s,p,1,0); if (rc != 1) return -1;       /* Win hack */
        /* above worked fine with read() til Windows demanded recv()  */
        switch (*p)
          {
#ifdef  OECS
            case 0x0A:                      /* found an ASCII newline */
                *p = 0x00;                    /* terminate the string */
                /* on an EBCDIC system? */
                if (snl != 0x0A) (void) stratoe(b);
                break;
            case 0x15:                     /* found an EBCDIC newline */
                *p = 0x00;                    /* terminate the string */
                /* on an ASCII system? */
                if (snl != 0x15) (void) stretoa(b);
                break;
#else
            case '\n':                     /* found a generic newline */
                *p = 0x00;                    /* terminate the string */
                break;
#endif
            default:
                break;
          }
        if (*p == 0x00) break;                     /* NULL terminates */
        p++; i++;                      /* increment pointer and index */
      }
    *p = 0x00;                        /* NULL terminate, even if NULL */

    if (i > 0 && b[i-1] == '\r')           /* is there a trailing CR? */
      { i = i - 1; p--;           /* shorten the length and backspace */
        *p = 0x00; }                    /* and remove the trailing CR */

    return i;
  }

/* ------------------------------------------------------------- PUTLINE
 *        Name: PUTLINE/UFTXPUTS
 *              common Put String function
 *   Operation: Writes the NULL terminated string from buffer b
 *              to socket s with NL (UNIX text) line termination.
 *      Author: Rick Troth, Ithaca, NY / Houston, TX (METRO)
 *        Date: 1993-Sep-19, Oct-20
 *
 *    See also: getline() in this source unit
 *
 *        Note: this routine limits lines written to 4095 bytes
 *
 */
int uftx_putline(int s,char*b,int l)
  { static char _eyecatcher[] = "uftx_putline()";
    int         i,  j;
    char        temp[4096];

    /* if null buffer pointer then return zero now */
    if (b == NULL) return 0;

    /* if length given as zero then compute string length  and cap it */
    if (l == 0) l = strlen(b);
    if (l >= sizeof(temp)) l = sizeof(temp) - 1;

    /* copy to temporary buffer just to be safe */
    for (i = 0; b[i] != 0x00 && i < l; i++) temp[i] = b[i];
    /* apply newline and [re]terminate the string */
    temp[i++] = '\n'; temp[i] = 0x00;

    j = write(s,temp,i);
    if (j < 0) j = send(s,temp,i,0);
    /* above worked fine with write() until Windows demanded send()   */

    /* normal return code is number of bytes written (incl newline)   */
    if (j != i) return -1;
    return i;
  }

/*
 *              Thanks to Bill Hunter at the University of Alabama
 *              for reporting certain problems with AIX.
 *
 *              Thanks to David Lippke for the idea of a FIFO in the
 *              home directory which can be attached from any listener.
 */

/**********************************************************************/

/* Copyright 1994, 1996, 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: msgc.c (tell.c)
 *              a multi-mode 'tell' command for UNIX
 *      Author: Rick Troth, Rice University, Houston, Texas, USA
 *              Rick Troth, rogue programmer, Cedarville, Ohio, USA
 *        Date: 1994-Jul-25 and prior ... and following
 */

/* ------------------------------------------------------------ MSGC_UFT
 *    Try sending a message to a user via UFT "user message hack".
 *    The FILE command provides two things we need for this:
 *      the name of the sender, and
 *      an authentication token (if available)
 */
int msgc_uft(char*user,char*text)
  { static char _eyecatcher[] = "msgc_uft()";
    int rc, mysock, i;
    char buffer[4096], agentkey[256], un[256], *p, hn[256];

    /* open /var/run/uft/agent.key for the magical AGENT string       */
    rc = mysock = open("/var/run/uft/agent.key",O_RDONLY);
    if (rc >= 0)
      { rc = read(mysock,agentkey,sizeof(agentkey)-1);
        if (rc > 0) agentkey[rc] = 0x00;
        else agentkey[0] = 0x00;
        close(mysock);
      } else agentkey[0] = 0x00;
    p = agentkey; while (*p > ' ') p++; *p = 0x00;     /* trim string */

    /* parse the supplied user name separating the host part          */
    p = user; i = 0;
    while (i < sizeof(un) - 1 && *p != 0x00 && *p != '@')
      { un[i++] = *p++; }
    un[i] = 0x00;

    /* parse the host part looking for an optional port number        */
    if (*p == '@') p++;
    if (*p == 0x00) p = "localhost"; 
    snprintf(hn,sizeof(hn),"%s:%d",p,UFT_PORT);

    /* connect to the UFT server */
    rc = mysock = tcpopen(hn,0,0);
    if (rc < 0) { if (errno != 0) perror("tcpopen()"); return rc; }

    /* look for the herald */
    rc = tcpgets(mysock,buffer,sizeof(buffer)-1);
    if (rc < 0)
      { if (errno != 0) perror("tcpgets():");
fprintf(stderr,"msgc_uft(): failed to fetch herald; RC = %d\n",rc);
        return rc; }

    /* send a "FILE 0 from auth" command */
    if (*agentkey == 0x00)
         snprintf(buffer,sizeof(buffer)-1,"FILE 0 %s -",uftx_user());
    else snprintf(buffer,sizeof(buffer)-1,"FILE 0 %s AGENT %s",uftx_user(),agentkey);
    rc = tcpputs(mysock,buffer);
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
    if (rc < 0) { if (errno != 0) perror("uftc_wack()");
                  close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send a "MSG user text" command */
    snprintf(buffer,sizeof(buffer)-1,"MSG %s %s",un,text);
    rc = tcpputs(mysock,buffer);
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
    if (rc < 0) { if (errno != 0) perror("uftc_wack()");
                  close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send an "ABORT" command (because we're not sending a file */
    rc = tcpputs(mysock,"ABORT");
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
    if (rc < 0) { if (errno != 0) perror("uftc_wack()");
                  close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send a "QUIT" command to close the session */
    rc = tcpputs(mysock,"QUIT");
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
    if (rc < 0) { if (errno != 0) perror("uftc_wack()");
                  close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* give a little lag time ... just in case */
    sleep(2);

    /* close the client end of the socket */
    close(mysock);

    return 0;
  }

/* ----------------------------------------------------------- UFTC_WACK
 *        Name: uftcwack.c
 *              UFT Client "Wait for ACK" function
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Mar-09, Nov-21 (Decatur)
 *              2025-09-03 for Windoze
 * Copyright 1995-2025 Richard M. Troth, all rights reserved.
 */
int uftc_wack(int s,char*b,int l)
  { static char _eyecatcher[] = "uftc_wack()";
    int         i;
    char       *p;

    while (1)
      { i = tcpgets(s,b,l);
//      b[i] = 0x00;
//      if (i > 2) if (b[i-1] == 0x0a) b[--i] = 0x00;
//      if (i > 1) if (b[i-1] == 0x0d) b[--i] = 0x00;
//fprintf(stderr,"WACK: '%s'\n",b);
        if (i < 0)
          { /* broken pipe or network error */
            b[0] = 0x00; return i; }
        switch (b[0])
          { case 0x00:                       /* NULL ACK (deprecated) */
                (void) strncpy(b,"2XX ACK (NULL)",l);
                return 0;
            case '6':                   /* write to stdout, then loop */
                p = b;
                while (*p != ' ' && *p != 0x00) p++;
                                 if (*p == ' ') p++;
                if (*p != 0x00) fprintf(stdout,"%s\n",p);
                break;
            case '1':   case '#':   case '*':   /* discard, then loop */
                break;
            case '2':                          /* simple ACK, is okay */
                return 2;
            case '3':                /* or "more required", also okay */
                return 3;
            case '4':                       /* "4" means client error */
                return 4;
            case '5':                   /* and "5" means server error */
                return 5;
            default:                                /* protocol error */
                return -1;
          }
        if (uftcflag & UFT_VERBOSE) if (b[0] != 0x00) /* uftx_putline(2,b,0); */
                                               fprintf(stderr,"%s\n",b);
      }
  }

/* ----------------------------------------------------------- UFTC_OPEN
 *    open a connection to the UFT server - direct TCP or via proxy
 */
int uftc_open(char*peer,char*prox,int*pipe)
  { static char _eyecatcher[] = "uftc_open()";
    int s;
    char temp[256];

    /* if the supplied peer is bogus then stop right here             */
    if (peer == NULL && *peer == 0x00) return -1; /* FIXME: set errno */

    /* tack-on the TCP port number                                    */
    snprintf(temp,sizeof(temp)-1,"%s:%d",peer,UFT_PORT);

    /* if a proxy string was provided then try connecting that way    */
    if (prox != NULL && *prox != 0x00) return uftx_proxy(temp,prox,pipe);

    /* normal connection is direct TCP with built-in DNS resolution   */
    s = tcpopen(temp,0,0);
/*  if (s < 0) { perror(peer); return -1; }            ** open failed */
    if (s < 0) return s;                               /* open failed */
    pipe[0] = pipe[1] = s;      /* input and output are the same here */
    return 0;
  }

/* ---------------------------------------------------------- UFTC_WRITE
 */
int uftc_write() { return 0; }

/* ----------------------------------------------------------- UFTC_READ
 */
int uftc_read() { return 0; }

/* ---------------------------------------------------------- UFTC_CLOSE
 *    close the pair of file descriptors talking to the server
 */
int uftc_close(int*pipe)
  { static char _eyecatcher[] = "uftc_close()";
    int rc;
    rc = 0;
    if (pipe[0] >= 0) rc = close(pipe[0]);
    if (rc < 0) return rc;
    if (pipe[1] >= 0 && pipe[1] != pipe[0]) rc = close(pipe[1]);
    if (rc < 0) return rc;
    pipe[0] = pipe[1] = -1;
    return 0;
  }

/* ----------------------------------------------------------- UFTD_AGCK
 *    This routine handles an AGENT inquiry commant. (agent check)
 *    Return values: 2 ACK, 4 NAK client, 5 NAK server
 */
int uftd_agck(char*k)
  { static char _eyecatcher[] = "uftd_agck()";
    int rc, fd;
    char *p;

    /* clean-up arguments of the AGENT command */
    while (*k <= ' ' && *k != 0x00) k++;
    if (*k == 0x00) return 4;   /* client did not supply agent string */
    p = k; while (*p > ' ') p++; *p = 0x00;    /* discard excess args */

    /* load the AGENT string from file if not previously loaded       */
    if (agstring[0] == 0x00)
      { rc = fd = open("/var/run/uft/agent.key",O_RDONLY);
        if (rc < 0) { perror("uftd_agck(): open()"); return 5; }
        rc = read(fd,agstring,sizeof(agstring)-1);
        if (rc < 0) { perror("uftd_agck(): read()");
                      close(fd); agstring[0] = 0x00; return 5; } }
    p = agstring; while (*p > ' ') p++; *p = 0x00;     /* trim string */

    /* do the strings match? */
    if (strcmp(k,agstring) == 0) return 2;

    return 5;
  }

/* -------------------------------------------------------------- GETENV
 *    Returns a pointer to the value of the requested variable,
 *    or points to the end of the environment buffer.
 */
char*uftx_getenv(char*var,char*env)
  { static char _eyecatcher[] = "uftx_getenv()";
    char   *p, *q;

    /* confirm sane function arguments                                */
    if (var == NULL) return var;
    if (*var == 0x00) return var;
    if (env == NULL) return env;
    if (*env == 0x00) return env;

    /* scan the "environment" looking for variables                   */
    p = env; while (*p)
      { q = var;
        while (*p == *q && *p != 0x00 &&
                           *q != 0x00 && *p != '=') { p++; q++; }
        if (*p == '=' && *q == 0x00) { p++; return p; }
        while (*p != 0x00) p++; p++;
        if (*p == 0x00) break; }

    return p;
  }

/*  p = "MSGTYPE=UMSG";         ** 1 - MSG       */
/*  p = "MSGTYPE=WMSG";         ** 2 - WNG, WMSG */
/*  p = "MSGTYPE=CMSG";         ** 3 - CPCONIO   */
/*  p = "MSGTYPE=SMSG";         ** 4 - SMSG      */
/*  p = "MSGTYPE=VMSG";         ** 5 - VMCONIO   */
/*  p = "MSGTYPE=EMSG";         ** 6 - EMSG      */
/*  p = "MSGTYPE=IMSG";         ** 7 - IMSG      */
/*  p = "MSGTYPE=FMSG";         ** 8 - SCIF      */

/* ------------------------------------------------------------ BASENAME
 *    Returns a pointer to the filename at the enf of a path.
 */
char*uftx_basename(char*s)
  { static char _eyecatcher[] = "uftx_basename()";
    char *p;
    p = s;
    while (*s != 0x00) switch (*s)
      { case '/': case '\\': p = s; p++;
                 s++; break;
        default: s++; break; }
    return p;
  }

/* -------------------------------------------------------------- PARSE1
 *    Terminates a string AFTER the first blank-delimited token.
 */
char*uftx_parse1(char*s)
  { static char _eyecatcher[] = "uftx_parse1()";
    char *p;
    p = s;
    while (*p > ' ') p++;        /* skip past all blanks and controls */
    *p = 0x00;
    return s;
  }

/* --------------------------------------------------------------- PROXY
 *    Launch a proxy program with its stdin and stdout connected,
 *    something like ... ProxyCommand='netcat -x 127.0.0.1:9050 %h %p'
 *
 *    NOTE: this routine does *not* tack-on the TCP port number
 */
int uftx_proxy(char*host,char*prox,int*fd)
  { static char _eyecatcher[] = "uftx_proxy()";
#ifdef UFT_POSIX
    int rc, us[2], ds[2], argc;
    char myhost[256], *port, myprox[256], *p, *argv[32];

    /* copy hostname (with port) into private storage for parsing     */
    strncpy(myhost,host,sizeof(myhost)-1);
    host = port = myhost;
    while (*port != ':' && *port > ' ') port++;
    if (*port == ':') *port++ = 0x00;
    p = port; while (*p != ':' && *p > ' ') p++; if (*p == ':') *p = 0x00;
    if (*port == 0x00) port = "608";       /* generic would be better */

    /* parse the proxy command into a traditional argc/argv pair      */
    strncpy(myprox,prox,sizeof(myprox)-1);
    argc = 0; p = myprox;
    while (*p != 0x00 && argc < 30)
      { argv[argc] = p;
        while (*p > ' ') p++; if (*p == ' ') *p++ = 0x00;
        if (strcmp(argv[argc],"%h") == 0) argv[argc] = host;
        if (strcmp(argv[argc],"%p") == 0) argv[argc] = port;
        argc++; }
    argv[argc] = NULL;

    rc = pipe(ds);                   /* establish the downstream pipe */
    /* proxy reads from ds[0] so application writes to ds[1]          */
    /* ds[0] must be duplicated to FD0 after fork() but before exec() */
    if (rc < 0) { rc = errno; if (rc == 0) rc = -1; return rc; }

    rc = pipe(us);                     /* establish the upstream pipe */
    /* proxy writes to us[1] so application reads from us[0]          */
    /* us[1] must be duplicated to FD1 after fork() but before exec() */
    if (rc < 0) { rc = errno; if (rc == 0) rc = -1;
                        close(ds[0]); close(ds[1]); return rc; }

    rc = fork();          /* fork the execution between two processes */
    if (rc < 0) { rc = errno; if (rc == 0) rc = -1;
                        close(ds[0]); close(ds[1]);
                        close(us[0]); close(us[1]); return rc; }
    if (rc > 0) { close(ds[0]); close(us[1]); sleep(1);
                      fd[0] = us[0]; fd[1] = ds[1]; return 0; }

    /* if the return from for() is zero then we are the child process */
                  close(ds[1]); close(us[0]);
      close(0); dup(ds[0]); close(ds[0]);
      close(1); dup(us[1]); close(us[1]);

    /* now replace this executable with the proxy program             */
    rc = execvp(argv[0],argv);

    if (rc < 0) { rc = errno; if (rc == 0) rc = -1; return rc; }
#endif
    return -1;
  }

/* ------------------------------------------------------------ UFT_STAT
 *    This routine parses a UFT "control file" on the receiving host.
 *    Compare with standard Unix/POSIX stat() system call.
 *
 *        Note: a UFT "spool file" should not have ".cf" or ".df"
 *              or similar qualifiers. It should be numeric, four digits
 *              (unless more digits are needed), and comprised of
 *              (at least) the ".cf" and ".df" physical Unix files.
 */
int uft_stat(char*sid,struct UFTSTAT*us)
  { static char _eyecatcher[] = "uft_stat()";
#ifdef UFT_POSIX
    int rc, fd, i, e, cl;
    char sn[256], cb[4096], *p, *q;
    struct  stat  statbuf;
    struct  tm  uft_stat_time;

    /* take the spool ID as numeric (akin to the inode in stat)       */
    us->uft_ino = atoi(uftx_basename(sid));

    /* process the spool ID into a pathname, ultimately absolute      */
    if (*sid == '/') strncpy(us->uft_sidp,sid,sizeof(us->uft_sidp)-1);
               else snprintf(us->uft_sidp,sizeof(us->uft_sidp)-1,
                     "%s/%s/%04d",UFT_SPOOLDIR,uftx_user(),us->uft_ino);

    /* construct the full path to the name of the control file        */
    snprintf(sn,sizeof(sn)-1,"%s.cf",us->uft_sidp);

    /* try to open the control file and load it into memory           */
    rc = fd = open(sn,O_RDONLY);
    if (rc < 0) { us->uft_sidp[0] = 0x00; return rc; }
    rc = fstat(fd,&statbuf);
    if (rc < 0) { close(fd); us->uft_sidp[0] = 0x00; return rc; }
    rc = cl = read(fd,cb,sizeof(cb)-1);
    e = errno; close(fd); errno = e;
    if (rc < 0) { us->uft_sidp[0] = 0x00; return rc; }
    cb[cl] = 0x00;
    us->uft_stime = statbuf.st_mtime;     /* time stamp of spool file */
               /* see us->uft_mtime for time stamp of file being sent */

    /* process the control file into a POSIX-like "environment" block */
    i = 0; p = q = cb;
    while (i < cl)
      { if (*p == '\n') *p = 0x00;
        if (*p != '\'') *q++ = *p;
        p++; i++; }
    *q++ = 0x00;
    *q++ = 0x00;        /* double null marks end of environment block */

    /* dummy out all single-byte fields in the struct                 */
    us->uft_type = us->uft_cc = us->uft_class = us->uft_rudev =
                   us->uft_hold = us->uft_keep = us->uft_msg = '-';

    /* TYPE is the most important attribute and may be followed by cc */
    p = uftx_getenv("TYPE",cb);
    us->uft_type = *p;
    while (*p > ' ') p++; while (*p == ' ') p++;
    us->uft_cc = *p;
    /* FIXME: we should vet the us->uft_type for known legit values   */

    /* try to get the file size based on the data component           */
    sprintf(sn,"%s.df",us->uft_sidp);
    rc = stat(sn,&statbuf);
    if (rc == 0) us->uft_size = statbuf.st_size;
    /* or SIZE is taken from the FILE statement and is an estimate    */
    else us->uft_size = uftx_atoi(uftx_getenv("SIZE",cb));

    us->uft_mtime = us->uft_stime;
/*  us->uft_mtime       ** time stamp (of the original, if provided)  */
    p = uftx_getenv("DATE",cb);
    if (p != 0x0000) if (*p != 0x00)
/*          { strptime(p,"%F %T %Z",&uft_stat_time);    not universal */
      { strptime(p,"%Y-%m-%d %T %Z",&uft_stat_time);
        us->uft_mtime = mktime(&uft_stat_time); }
    /* FIXME: we also need XDATE                                      */

    us->uft_nlink = 1;       /* default is one copy (e.g., for print) */
    p = uftx_getenv("COPY",cb); if (p == 0x0000 || *p == 0x00)
    p = uftx_getenv("COPIES",cb); if (p != 0x0000 && *p != 0x00)
    us->uft_nlink = uftx_atoi(p);

/*  us->uft_blksize     ** akin to LRECL */

    /* CLASS may optionally also indicate a unit-record device type   */
    p = uftx_getenv("CLASS",cb);
    if (p != 0x0000 && *p != 0x00)
      { us->uft_class = *p++;
        if (*p == ' ') p++;
        if (*p == 'P' && *p == 'p') p++;   /* PRT ==> R and PUN ==> U */
        us->uft_rudev = *p; }              /* PRT ==> R and PUN ==> U */

    /* us->uft_hold         ** mostly a VM or other mainframe concept */
    p = uftx_getenv("HOLD",cb);
    if (p != 0x0000) if (*p != 0x00) us->uft_hold = *p;

    /* us->uft_keep         ** mostly a VM or other mainframe concept */
    p = uftx_getenv("KEEP",cb);
    if (p != 0x0000) if (*p != 0x00) us->uft_keep = *p;

    /* us->uft_msg          ** mostly a VM or other mainframe concept */
    p = uftx_getenv("MSG",cb);
    if (p != 0x0000) if (*p != 0x00) us->uft_msg = *p;

    /* NAME is optional (but needed when you actually receive)        */
    strncpy(us->uft_name,uftx_getenv("NAME",cb),sizeof(us->uft_name)-1);

/*  us->uft_mode;       ** UFT "xperm" protection */
    p = uftx_getenv("PROT",cb);
/*  strncpy(                                                          */

    /* USER is who the file is *to*                                   */
    p = uftx_getenv("USER",cb); if (p != 0x0000) if (*p != 0x00)
        strncpy(us->uft_user,p,sizeof(us->uft_user));
           else us->uft_user[0] = 0x00;

    /* best figuring of sending user and sending host using several   */
us->uft_from[0] = 0x00;
    p = uftx_getenv("REMOTE",cb);
    if (p != 0x0000) if (*p != 0x00)
      { if (*p == '@')
          { q = uftx_getenv("FROM",cb);
            if (q != 0x0000) if (*q != 0x00)
            snprintf(us->uft_from,sizeof(us->uft_from),"%s%s",q,p); }
        else strncpy(us->uft_from,p,sizeof(us->uft_from)); }
    else { q = uftx_getenv("FROM",cb);
            if (q != 0x0000) if (*q != 0x00)
            strncpy(us->uft_from,q,sizeof(us->uft_from)); }

/*
      TYPE              uft_type        TYPE
      CC                uft_cc          TYPE (second token)
      CLASS             uft_class       CLASS
      devtype           uft_rudev       CLASS (second token)
      HOLD              uft_hold        HOLD
      KEEP              uft_keep        KEEP
      MSG               uft_msg         MSG
      COPY              uft_nlink       COPY | COPIES
                        user            REMOTE | FROM
                        host            REMOTE
      date stamp on SID
      SID               uft_ino
      NAME              name            NAME
 */

    p = uftx_getenv("FORM",cb); if (p != 0x0000) if (*p != 0x00)
        strncpy(us->uft_form,p,sizeof(us->uft_form));
           else us->uft_form[0] = 0x00;

    p = uftx_getenv("DIST",cb); if (p != 0x0000) if (*p != 0x00)
        strncpy(us->uft_dist,p,sizeof(us->uft_dist));
           else us->uft_dist[0] = 0x00;

    p = uftx_getenv("DEST",cb); if (p != 0x0000) if (*p != 0x00)
        strncpy(us->uft_dest,p,sizeof(us->uft_dest));
           else us->uft_dest[0] = 0x00;

    p = uftx_getenv("TITLE",cb); if (p != 0x0000) if (*p != 0x00)
        strncpy(us->uft_title,p,sizeof(us->uft_title));
           else us->uft_title[0] = 0x00;

    return 0;
#else
    return -1;
#endif
  }

/* ---------------------------------------------------------------------
 *    Remove all files (per known extensions) for this spool file.
 */
int uft_purge(struct UFTSTAT*us)
  { static char _eyecatcher[] = "uft_purge()";
    int rc, i;
    char sidn[64];
    struct  stat  statbuf;
    char *exts[] = {  UFT_EXT_CONTROL,  /*    ".cf" ** control/meta   */
                      UFT_EXT_DATA,     /*    ".df" ** data           */
                      UFT_EXT_EXTRA,    /*    ".ef" ** resource fork  */
                      UFT_EXT_LIST,     /*    ".lf" ** 'ls -l' format */
                      UFT_EXT_WORK,     /*    ".wf" ** work file      */
                      ""   };       /* empty string marks end of list */

    /* loop through the standard filename extensions deleting all     */
    i = 0;
    while (*exts[i] != 0x00) {
        /* name the physical file */
        snprintf(sidn,sizeof(sidn)-1,"%s%s",us->uft_sidp,exts[i]);
        /* does it exist? */
        rc = stat(sidn,&statbuf); if (rc == 0)
        /* set owner write permission */
        rc = chmod(sidn,statbuf.st_mode|S_IWUSR); if (rc == 0)
        /* delete it */
        rc = unlink(sidn);
        /* next */
        i++;                 }
    /* ignoring failed unlink() calls ... at least for the time being */

    return 0;
  }

/* ----------------------------------------------------------- UFTX_ATOI
 *    Convert string to integer recognizing K or M qualifiers.
 */
int uftx_atoi(char*s)
  { static char _eyecatcher[] = "uftx_atoi()";
    int i;
    i = 0;
    while (1)
      { switch (*s)
          { case 0x00: return i; break;
            case 'K': case 'k': i = i * 1024; return i; break;
            case 'M': case 'm': i = i * 1048576; return i; break;
            default: i = i * 10 + (*s & 0x0F); break; }
        s++; }
    return i;
  }

/* ------------------------------------------------------------ READSPAN
 *    Read a "spanning record". When reading from a pipe, the requested
 *    number of bytes might not be available to just one read() call.
 *    This function performs as many read()s as needed until the desired
 *    number of bytes are acquired. This is how we explicitly discard
 *    any record structure that UNIX may have learned about.
 */

int uft_readspan(int s,char*b,int c)
  { static char _eyecatcher[] = "uft_readspan()";
    int         i,  j;

    for (j = 0; c > 0; )
      { i = read(s,&b[j],c);
        if (i < 0) return i;
        if (i < 1) break;
        j = j + i;
        c = c - i; }
    return j;
  }

/* ------------------------------------------------------------ UFTDL699
 *    Loop on multiline content returning 699 replies to the client.
 */
int uftdl699(int s,char*b)
  { static char _eyecatcher[] = "uftdl699()";
    int i;
    char *p, pb[256];
    while (*b != 0x00)
      { p = pb; i = 4;
        *p++ = '6'; *p++ = '9'; *p++ = '9'; *p++ = ' ';
        while (*b >= ' ' && i < sizeof(pb)-1) { *p++ = *b++; i++; }
        *p = 0x00;
        uftdstat(s,pb);
        if (*b != 0x00) b++;
      }
    return 0;
  }

/* ------------------------------------------------------------ UFTDSTAT
 *    Writes a line to the stream indicated by sock (UFT client)
 *    and attempts to log that line in tf (temp) and/or cf (meta).
 *    Moved to the library from UFTD source 2025-06-02 (Monday).
 */
void uftdstat(int sock,char*zlda)
  { static char _eyecatcher[] = "uftdstat()";
    char        buff[256];

    (void) tcpputs(sock,zlda);              /* write it to the client */
    (void) snprintf(buff,sizeof(buff)-1,"#>%s",zlda);
/*  if (tf >= 0) (void) uftx_putline(tf,buff,0);  */
/*  if (cf >= 0) (void) uftx_putline(cf,buff,0);  */
    if (uftlogfd >= 0) uftx_putline(uftlogfd,buff,0);   /* and log it */

    return;
  }

/* � Copyright 1995-2025, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftctext.c (C program source)
 *              Unsolicited File Transfer client text clarifier
 *              Converts NL (UNIX) delimited lines to CR/LF
 *              delimited (Internet NVT) lines in a buffer.
 *      Author: Rick Troth, Houston, Texas, USA (METRO)
 *        Date: 1995-Apr-13
 *
 *        NOTE: This source merged into UFTLIB 2025-09-12.
 */

/* ------------------------------------------------------------ UFTCTEXT
 */
int uftctext(int s,char*b,int l)
  { static char _eyecatcher[] = "uftctext()";
    char        t[UFT_BUFSIZ] /* , *p */ ;
    int         i, j, k;

    k = l / 2;
    if (k > UFT_BUFSIZ) k = UFT_BUFSIZ;

    j = read(s,t,k);
    if (j < 1)
    j = read(s,t,k);
    if (j < 0) return i;

/*  OLD CODE  **
    p = t;
    for (i = 0; i < j; i++)
      {
        if (*p == '\n')
          {
            b[i] = '\r';
            i++;  j++;
          }
        b[i] = *p++;
      }
 */
    j = htonb(b,t,j);

    return j;
  }

/* � Copyright 1997-2025 Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftcprot.c / uftcperm.c (C program source)
 *              converts a stat() st_mode value into a
 *              U:RWX, G:RWX, W:RWX, S:RWED formatted string
 *
 *        NOTE: This source merged into UFTLIB 2025-09-12.
 */

/* ------------------------------------------------------------ UFTCPROT
 *    Note: this routine uses a static buffer thus is not thread safe.
 */
char *uftcprot(mode_t prot)
  { static char _eyecatcher[] = "uftcprot()";
    static char buffer[256];
    char        *p;

    p = buffer;

    /*  user permissions  */
    if (prot & (S_IRUSR | S_IWUSR | S_IXUSR))
      { *p++ = 'U';  *p++ = ':';
        if (prot & S_IRUSR) *p++ = 'R';
        if (prot & S_IWUSR) *p++ = 'W';
        if (prot & S_IXUSR) *p++ = 'X'; }
/*
        if (prot & S_IREAD) *p++ = 'R';
        if (prot & S_IWRITE) *p++ = 'W';
        if (prot & S_IEXEC) *p++ = 'X';
 */

    /*  group permissions  */
    if (prot & (S_IRGRP | S_IWGRP | S_IXGRP))
      { if (p != buffer) *p++ = ',';
        *p++ = 'G';  *p++ = ':';
        if (prot & S_IRGRP) *p++ = 'R';
        if (prot & S_IWGRP) *p++ = 'W';
        if (prot & S_IXGRP) *p++ = 'X'; }

    /*  world permissions  */
    if (prot & (S_IROTH | S_IWOTH | S_IXOTH))
      { if (p != buffer) *p++ = ',';
        *p++ = 'W';  *p++ = ':';
        if (prot & S_IROTH) *p++ = 'R';
        if (prot & S_IWOTH) *p++ = 'W';
        if (prot & S_IXOTH) *p++ = 'X'; }

    /*  terminate the string and return  */
    *p++ = 0x00;
    return buffer;
  }

/* ------------------------------------------------------------ MSGWRITE
 *  Try stock UNIX 'write' command if local user.                DEFUNCT
 */
int msgwrite(char*user,char*text)
  { char        temp[256];
    (void) sprintf(temp,"echo \"%s\" | write %s",text,user);
    return system(temp); }

/* ------------------------------------------------------------ MSGSMTPS
 *  Try SMTP "send" command. (not always implemented)            DEFUNCT
 */
int msgsmtps(char*user,char*text) { return -1; }

/* ------------------------------------------------------------ MSGSMTPM
 *  Try SMTP mail. (advantage is direct -vs- queued)             DEFUNCT
 */
int msgsmtpm(char*user,char*text) { return -1; }

/* ------------------------------------------------------------- MSGMAIL
 *  Try queued mail (sendmail) as a last resort.                 DEFUNCT
 */
int msgmail(char*user,char*text) { return -1; }


