/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftlib.c
 *              Unsolicited File Transfer client/server library
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-04-08 (Tuesday)
 *
 *        Note: This is in response to the need for an actual LIBRARY.
 *              Individual functions will move here as needed.
 *
 *        Note: This is for UFT but includes MSG functions also.
 *
 */

#ifndef PREFIX
 #define PREFIX "/usr"
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <ctype.h>

#ifdef          __OPEN_VM
 #ifndef        OECS
  #define       OECS
 #endif
#endif

#include "xmitmsgx/xmitmsgx.h"
char *xmmprefix = PREFIX;
static struct MSGSTRUCT uftmsgs;

#include "uft.h"

extern  int  uftcflag;

static char agstring[256] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

/* ---------------------------------------------------------------------
 *    This routine handles message FORMATTING (not message delivery).
 *    It's a different way of doing gettext() type processing.
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
 *  and delivering the message to the 'write' command.
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
    int rc, sd;
    char fn[256];
    struct sockaddr_un msgsockst;

    /* first try using AF_UNIX (that is AF_LOCAL, named socket)       */
    rc = sd = socket(AF_UNIX,SOCK_STREAM,0); if (rc < 0) return rc;

    snprintf(fn,sizeof(fn),"%s/%s/.msgsock",UFT_SPOOLDIR,user);
//fprintf(stderr,"sock %s\n",fn);

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

    rc = write(sd,buff,bl);
    close(sd);
    return rc;
  }

/* ---------------------------------------------------------------------
 *    NOTE: this routine is NOT prototyped and not called from outside
 */
int msgd_xmsg_fifo(char*user,char*buff,int bl)
  { static char _eyecatcher[] = "msgd_xmsg_fifo()";
    int rc, fd;
    char fn[256];

    snprintf(fn,sizeof(fn),"%s/%s/.msgpipe",UFT_SPOOLDIR,user);
//fprintf(stderr,"pipe %s\n",fn);
    rc = fd = open(fn,O_WRONLY|O_NDELAY);

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
    q = buffer;
    l = sizeof(buffer) - 2;

    /* first thing in the batch is the message text (no tag or var)   */
    p = text;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* we don't always care about the message type but we always say  */
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
        int topts;

        strncpy(bff,from,sizeof(bff)-1);
        bfh = bff;
        while (*bfh != 0x00 && *bfh != '@') bfh++;
        if (*bfh == '@') *bfh++ = 0x00;

//      topts = uftmsgs.msgopts;
//      uftmsgs.msgopts |= MSGFLAG_NOCODE;
        mv[1] = bfh; mv[2] = bff; mv[3] = text;    /* From &1(&2): &3 */
        rc = uftx_message(buffer,sizeof(buffer)-1,8416,"SRV",4,mv);
//fprintf(stderr,"%s\n",buffer);
//      uftmsgs.msgopts = topts;

/* reusing p ... and this needs to be fixed in the XMM package */
        p = buffer; while (*p != ' ' && *p != 0x00) p++;
                    while (*p == ' ') p++;

//fprintf(stderr,"%s\n",buffer);
//fprintf(stderr,"%s\n",p);
//      rc = uftd_message(user,buffer);
        rc = uftd_message(user,p);
//fprintf(stderr,"uftd_message() returned %d\n",rc);
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
    q = buffer;
    l = sizeof(buffer) - 2;

//  topts = uftmsgs.msgopts;
//  uftmsgs.msgopts |= MSGFLAG_NOCODE;
    mv[1] = spid; mv[2] = user; mv[3] = from;
    rc = i = uftx_message(q,l,1004,"SRV",4,mv);
//  uftmsgs.msgopts = topts;
    if (rc < 0) return rc;

    i = i + 1;          /* we expect uftx_message() to NULL terminate */
    q = & q[i]; l = l - i;
    if (i < l) { *q++ = 0x00; i++; }     /* but terminate here anyway */

    /* we don't always care about the message type but we always say  */
/*  p = "MSGTYPE=UMSG";         ** 1 - MSG       */
/*  p = "MSGTYPE=WMSG";         ** 2 - WNG, WMSG */
/*  p = "MSGTYPE=CMSG";         ** 3 - CPCONIO   */
/*  p = "MSGTYPE=SMSG";         ** 4 - SMSG      */
/*  p = "MSGTYPE=VMSG";         ** 5 - VMCONIO   */
/*  p = "MSGTYPE=EMSG";         ** 6 - EMSG      */
    p = "MSGTYPE=IMSG";         /* 7 - IMSG      */
/*  p = "MSGTYPE=FMSG";         ** 8 - SCIF      */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* this is obvious to the receiving user but we include it anyway */
    p = "MSGUSER=";                 /* who is this file to? (obvious) */
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    p = user;
    while (*p != 0x00 && i < l) { *q++ = *p++; i++; }
    if (i < l) { *q++ = 0x00; i++; }

    /* this is in the message but here too (varies from VM IMSG)      */
    p = "MSGFROM=";                          /* who is the file from? */
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
    rc = uftd_message(un,buffer);

    return rc;
  }

/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
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
 */
char *uftx_user()
  { static char _eyecatcher[] = "uftx_user()";
    char       *u;
    extern  char       *getenv();

#ifndef _OE_SOCKETS
    struct passwd *pwdent;

    /*  first try effective uid key into passwd  */
    pwdent = getpwuid(geteuid());
    if (pwdent) return pwdent->pw_name;

    /*  next try real uid key into passwd  */
    pwdent = getpwuid(getuid());
    if (pwdent) return pwdent->pw_name;
#endif

    /*  thin ice,  try USER env var  */
    u = getenv("USER");
    if (u != 0x0000 && u[0] != 0x00) return u;

    /*  last resort, try LOGNAME env var  */
    u = getenv("LOGNAME");
    if (u != 0x0000 && u[0] != 0x00) return u;

    /*  give up!  */
    return "";
  }

#ifndef _OE_SOCKETS
/* ------------------------------------------------------------- USERIDG
 *  "g" for GECOS field, return personal name string, if available
 */
char *useridg()
  {
    char       *g;
    extern  char       *getenv();
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


/**********************************************************************/


/* © Copyright 1996, Richard M. Troth, all rights reserved.  <plaintext>
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
 *              At this time,  IMSGs from  user@host
 *              are meaningless unles  host  is the local system.
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
  { static char *eyecatch = "msglocal()";

    int         fd, i, j;
    char        temp[BUFSIZ], *from;

    /*  a 'mknod' with 622 perms (writable) might work too  */

    /*  if there's no listener ...  */
    if (fd < 0 && errno == ENXIO)
      {
        /*  launch our special application to listen  */
        fd = open(temp,O_WRONLY|O_NDELAY);
        /*  ... or NOT ...  */
      }
    if (fd < 0) return fd;
  }

/* ----------------------------------------------------------- UFTX_HOME
 *    Return a pointer to a string with the home directory of this user.
 */
char*uftx_home(char*user)
  { static char _eyecatcher[] = "uftx_home()";
    int         i, uuid;
    struct passwd *pwdent;
    static char homedir[256];

    errno = 0;
    pwdent = getpwnam(user);
    if (pwdent == NULL) { perror("uftx_home()"); homedir[0] = 0x00; }
    else strncpy(homedir,pwdent->pw_dir,sizeof(homedir)-1);

    return homedir;
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
    char       *p;
    int         i;

#ifdef  OECS
    char        snl;
    snl = '\n';
#endif

    p = b; i = 0;
    while (i < l)
      { if (read(s,p,1) != 1)                           /* get a byte */
        if (read(s,p,1) != 1) return -1;                 /* try again */
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

//fprintf(stderr,"msgc_uft(): starting\n");                    // MSGC_UFT

    /* open /var/run/uft/agent.key for the magical AGENT string       */
    rc = mysock = open("/var/run/uft/agent.key",O_RDONLY);
//fprintf(stderr,"msgc_uft(): open() returned %d\n",rc);       // MSGC_UFT
    if (rc >= 0)
      { rc = read(mysock,agentkey,sizeof(agentkey)-1);
//fprintf(stderr,"msgc_uft(): read() returned %d\n",rc);       // MSGC_UFT
        if (rc > 0) agentkey[rc] = 0x00;
        else agentkey[0] = 0x00;
        close(mysock);
      } else agentkey[0] = 0x00;
    p = agentkey; while (*p > ' ') p++; *p = 0x00;     /* trim string */
//if (agentkey[0] != 0x00)
//fprintf(stderr,"msgc_uft(): agent string %s\n",agentkey);    // MSGC_UFT

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
    if (rc < 0) { perror("tcpopen()"); return rc; }

    /* look for the herald */
    rc = tcpgets(mysock,buffer,sizeof(buffer)-1);
//fprintf(stderr,"msgc_uft(): tcpgets() returned %d\n",rc);    // MSGC_UFT
//fprintf(stderr,"%s\n",buffer);                               // MSGC_UFT

    /* send a "FILE 0 from auth" command */
    if (*agentkey == 0x00)
         snprintf(buffer,sizeof(buffer)-1,"FILE 0 %s -",uftx_user());
    else snprintf(buffer,sizeof(buffer)-1,"FILE 0 %s AGENT %s",uftx_user(),agentkey);
    rc = tcpputs(mysock,buffer);
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }
//fprintf(stderr,"msgc_uft(): sent FILE command\n");           // MSGC_UFT

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
//fprintf(stderr,"msgc_uft(): uftc_wack() returned %d\n",rc);  // MSGC_UFT
    if (rc < 0) { perror("uftc_wack()"); close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send a "MSG user text" command */
    snprintf(buffer,sizeof(buffer)-1,"MSG %s %s",un,text);
    rc = tcpputs(mysock,buffer);
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }
//fprintf(stderr,"msgc_uft(): sent MSG command\n");            // MSGC_UFT

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
//fprintf(stderr,"msgc_uft(): uftc_wack() returned %d\n",rc);  // MSGC_UFT
    if (rc < 0) { perror("uftc_wack()"); close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send an "ABORT" command (because we're not sending a file */
    rc = tcpputs(mysock,"ABORT");
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }
//fprintf(stderr,"msgc_uft(): sent ABORT command\n");          // MSGC_UFT

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
//fprintf(stderr,"msgc_uft(): uftc_wack() returned %d\n",rc);  // MSGC_UFT
    if (rc < 0) { perror("uftc_wack()"); close(mysock); return rc; }
    if (rc != 2) { fprintf(stderr,"%s\n",buffer); close(mysock); return rc; }

    /* send a "QUIT" command to close the session */
    rc = tcpputs(mysock,"QUIT");
    if (rc < 0) { perror("tcpputs()"); close(mysock); return rc; }
//fprintf(stderr,"msgc_uft(): sent QUIT command\n");           // MSGC_UFT

    /* wait for ACK */
    rc = uftc_wack(mysock,buffer,sizeof(buffer)-1);
//fprintf(stderr,"msgc_uft(): uftc_wack() returned %d\n",rc);  // MSGC_UFT
    if (rc < 0) { perror("uftc_wack()"); close(mysock); return rc; }
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
 * Copyright 1995-2025 Richard M. Troth, all rights reserved.
 */
int uftc_wack(int s,char*b,int l)
  { static char _eyecatcher[] = "uftc_wack()";
    int         i;
    char       *p;

    while (1)
      { errno = 0;
        i = tcpgets(s,b,l);
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
                if (*p != 0x00) (void) uftx_putline(1,++p,0);
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
        if (uftcflag & UFT_VERBOSE) if (b[0] != 0x00) uftx_putline(2,b,0);
      }
  }

/* ----------------------------------------------------------- UFTD_AGCK
 *    This routine handles an AGENT inquiry commant.
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

/* ------------------------------------------------------------ MSGWRITE
 *  Try stock UNIX 'write' command if local user.
 */
int msgwrite(user,text)
  char   *user, *text;
  {
    char        temp[256];
    (void) sprintf(temp,"echo \"%s\" | write %s",text,user);
    return system(temp);
  }

/* ------------------------------------------------------------ MSGSMTPS
 *  Try SMTP "send" command. (not always implemented)
 */
int msgsmtps(char*user,char*text) { return -1; }

/* ------------------------------------------------------------ MSGSMTPM
 *  Try SMTP mail. (advantage is direct -vs- queued)
 */
int msgsmtpm(char*user,char*text) { return -1; }

/* ------------------------------------------------------------- MSGMAIL
 *  Try queued mail (sendmail) as a last resort.
 */
int msgmail(char*user,char*text) { return -1; }


