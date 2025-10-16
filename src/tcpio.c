/* © Copyright 1995-2025, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: tcpio.c (C program source)
 *              various TCP utility functions
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Apr-19
 *
 *   Functions:
 *              character set conversion:
 *              htonc(char)
 *              ntohc(char)
 *              htonz(string)
 *              ntohz(string)
 *              htonb(target,source,length)
 *              ntohb(target,source,length)
 *              (above akin to htons(), ntohs(), htonl() and ntohl())
 *
 */

#if defined(_WIN32) || defined(_WIN64)
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <netdb.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tcpio.h"

#define         TCPSMALL        256
#define         TCPLARGE        4096

int     tcp_ubuf[TCPLARGE];
char    tcp_umsg[TCPSMALL];

/* if we're on IBM OpenVM, define OECS */
#ifdef          __OPEN_VM
#ifndef         OECS
#define         OECS
#endif
#endif

/* if we're on IBM OpenEdition, define OECS */
#ifdef          _OE_SOCKETS
#ifndef         OECS
#define         OECS
#endif
#endif

/* ------------------------------------------------------------- TCPOPEN
 *  Tries to mimick open(path,flags[,mode])
 *  but connects to a TCP port,  not a local file.
 */
int tcpopen(char*host,int flag,int mode)
  { static char _eyecatcher[] = "tcpopen()";
    int         s, i, port, rc, j;
    struct sockaddr name;
    struct hostent *hent, myhent;
    char       *myhental[2], myhenta0[4], myhenta1[4];
    char        temp[TCPSMALL], *p, *q;



#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsa;
    rc = WSAStartup(MAKEWORD(2,2),&wsa);
    if (rc != 0) {
        fprintf(stderr,"Windows socket subsytsem could not be initialized.\n");
        fprintf(stderr,"Error Code: %d. Exiting..\n", WSAGetLastError());
        return -1; }
#endif


    /*  parse host address and port number by colon  */
    p = host; host = temp; i = 0;
    while (i < TCPSMALL && *p != 0x00 && *p != ':')
        host[i++] = *p++; host[i++] = 0x00;
    if (*p != ':') port = 0;
    else
      {
        p++; q = p;
        while (i < TCPSMALL && *q != 0x00 && *q != ':')
            temp[i++] = *q++; temp[i++] = 0x00;
        port = atoi(p);
      }

    /*  figure out where to connect  */
    hent = gethostbyname(host);
    if (hent == NULL)
      {
        /*  netDB lookup failed;  numeric address supplied?  */
        p = host;
        if (*p < '0' || '9' < *p) return -1;
        hent = &myhent;
        hent->h_addr_list = myhental;   /*  address list  */
        hent->h_addr_list[0] = myhenta0;/*  address 0  */
        hent->h_addr_list[1] = myhenta1;/*  address 1  */
        hent->h_addrtype = AF_INET;
        hent->h_length = 4;

        /*  try to pick-apart the string as dotted decimal  */
        hent->h_addr_list[0][0] = atoi(p);
        while (*p != '.' && *p != 0x00) p++; p++;
        if (*p < '0' || '9' < *p) return -1;
        hent->h_addr_list[0][1] = atoi(p);
        while (*p != '.' && *p != 0x00) p++; p++;
        if (*p < '0' || '9' < *p) return -1;
        hent->h_addr_list[0][2] = atoi(p);
        while (*p != '.' && *p != 0x00) p++; p++;
        if (*p < '0' || '9' < *p) return -1;
        hent->h_addr_list[0][3] = atoi(p);

        /*  dotted decimal worked!  now terminate the list  */
        hent->h_addr_list[1][0] = 0;    hent->h_addr_list[1][1] = 0;
        hent->h_addr_list[1][2] = 0;    hent->h_addr_list[1][3] = 0;
        /*  better form might be to use NULL pointer?  */
        hent->h_addr_list[1] = NULL;

        /*  and what else do we need to set?  */
        hent->h_name = host;
        /*  should probably call gethostbyaddr()
            at this point;  maybe in the next rev  */
      }

    /*  gimme a socket  */
    s = socket(AF_INET,SOCK_STREAM,0);
    if (s < 0)
      {
/*
        (void) perror("socket()");
 */
        return s;
      }

    /*  build that structure  */
    name.sa_family = AF_INET;
    name.sa_data[0] = (port >> 8) & 0xFF;
    name.sa_data[1] = port & 0xFF;

    /*  try address one-by-one  */
    for (i = 0; hent->h_addr_list[i] != NULL; i++)
      {
        /*  any more addresses?  */
        if (hent->h_addr_list[i] == NULL) break;
        if (hent->h_addr_list[i][0] == 0x00) break;

        /*  fill-in this address to the structure  */
        for (j = 0; j < hent->h_length; j++)
            name.sa_data[j+2] = hent->h_addr_list[i][j];
        name.sa_data[j+2] = 0x00;       /*  terminate  */

        /*  note this attempt  */
        (void) sprintf(tcp_umsg,"trying %d.%d.%d.%d\n",name.sa_data[2],
                name.sa_data[3],name.sa_data[4],name.sa_data[5]);

        /*  can we talk?  */
        rc = connect(s, &name, 16);
        if (rc == 0) return s;
      }

    /*  can't seem to reach this host on this port  :-(  */
    (void) close(s);
    if (rc < 0)
      {
/*
        (void) perror("connect()");
 */
        return rc;
      }
    return -1;
  }

/* -------------------------------------------------------------- MXOPEN
 *  Like  tcpopen(),  but connects to a Mail eXchanger IP host.
 */
int mxopen(char*host,int flag,int mode)
  { static char _eyecatcher[] = "mxopen()";
    return -1;
  }

/* ------------------------------------------------------------ TCPCLOSE
 */
int tcpclose(int fd)
  { static char _eyecatcher[] = "tcpclose()";
    return close(fd);
  }

/* ------------------------------------------------------------- TCPGETS
 *   Operation: Reads a CR/LF terminated string from socket s
 *              into buffer b.  Returns the length of that string.
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Apr-19
 *              2025-09-03 for Windoze
 *     Returns: length of string or negative on error
 *
 *    See also: getline.c, putline.c
 */
int tcpgets(int s,char*b,int l)
  { static char _eyecatcher[] = "tcpgets()";
    char       *p;
    int         i, rc;

#ifdef  OECS
    char        snl;
    snl = '\n';
#endif

    p = b;
    for (i = 0; i < l; i++)
      { rc = read(s,p,1); if (rc != 1)                  /* get a byte */
        rc = recv(s,p,1,0); if (rc != 1)                  /* Win hack */
        rc = read(s,p,1); if (rc != 1)                   /* try again */
        rc = recv(s,p,1,0); if (rc != 1) return -1;       /* Win hack */
        /* above worked fine with read() til Windows demanded recv()  */
        switch (*p)
          {
#ifdef  OECS
            case 0x0A:          /*  found an ASCII newline  */
                *p = 0x00;      /*  terminate the string  */
                /*  on an EBCDIC system?  */
                if (snl != 0x0A) (void) stratoe(b);
                break;
            case 0x15:          /*  found an EBCDIC newline  */
                *p = 0x00;      /*  terminate the string  */
                /*  on an ASCII system?  */
                if (snl != 0x15) (void) stretoa(b);
                break;
#else
            case '\n':          /*  found a generic newline  */
                *p = 0x00;      /*  terminate the string  */
                break;
#endif
            default:
                break;
          }
        if (*p == 0x00) break;          /*  NULL terminates  */
        p++;                            /*  increment pointer  */
      }
    *p = 0x00;          /*  NULL terminate,  even if NULL  */

    i = p - b;          /*  calculate the length  */
    if (i > 0 && b[i-1] == '\r')        /*  trailing CR?  */
      {
        i = i - 1;      /*  shorten length by one  */
        p--;            /*  backspace  */
        *p = 0x00;      /*  remove trailing CR  */
      }

    return i;
  }

/* ------------------------------------------------------------- TCPPUTS
 *   Operation: Writes the NULL terminated string from buffer b
 *              to socket s with CR/LF (network text) line termination.
 *              Returns number of bytes written, less line delimiter.
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Apr-19
 *
 *    See also: getline.c, putline.c
 */
int tcpputs(int s,char*b)
  { static char _eyecatcher[] = "tcpputs()";
    int         i,  j;
    char        temp[4096];

#ifdef  OECS
    char        snl;
    snl = '\n';
#endif

    /*  copy to buffer because we'll modify slightly  */
    for (i = 0; b[i] != 0x00 && i < 4094; i++) temp[i] = b[i];
    temp[i] = 0x00;
#ifdef  OECS
    /*  on an EBCDIC system?  */
    if (snl == 0x15) (void) stretoa(temp);
    temp[i+0] = 0x0D;
    temp[i+1] = 0x0A;
#else
    temp[i+0] = '\r';
    temp[i+1] = '\n';
#endif

    /* write entire string, WITH line interpolation, at once          */
    j = write(s,temp,i+2);
    if (j < 0) j = send(s,temp,i+2,0);
    /* above worked fine with write() until Windows demanded send()   */

    if (j != i+2) return -1;
    return i;
  }

/* ------------------------------------------------------------ TCPWRITE
 */
int tcpwrite(int fd,char*s,int n)
  { static char _eyecatcher[] = "tcpwrite()";
    int rc;

    rc = write(fd,s,n);
    if (rc < 0) rc = send(fd,s,n,0);
    /* above worked fine with write() until Windows demanded send()   */

    return rc;
  }

/* ------------------------------------------------------------- TCPREAD
 */
int tcpread(int fd,char*s,int n)
  { static char _eyecatcher[] = "tcpread()";
    int rc;

    rc = read(fd,s,n);
    if (rc < 0) recv(fd,s,n,0);
    /* above worked fine with read() until Windows demanded recv()    */

    return rc;
  }

/* ------------------------------------------------------------ TCPIDENT
 *
 *        Name: tcpident.c
 *              who's on the other end of this TCP socket?
 *      Author: Rick Troth, Rice University, Information Systems
 *        Date: 1995-Apr-19
 *
 *              This is the part that was done on Rice time,
 *              prompting the "R" in the version string.
 *              It was to shore-up the last requirements
 *              for the implementation that Rice might keep.
 *              Sadly (to me) someone yanked it (UFT entirely)
 *              the very first day I was gone.
 */

#ifndef         NULL
#define         NULL            0x0000
#endif

#define         HOST_BSZ        128
#define         USER_BSZ        64
#define         TEMP_BSZ        256

#define         IDENT_PORT      113

int tcpident(int sock,char*buff,int size)
  { static char _eyecatcher[] = "tcpident()";
    struct  sockaddr    sadr;
    struct  hostent    *hent;
    int         i, rc, slen, styp, soff;
    char        temp[TEMP_BSZ];
    char        hadd[16];       /*  is that enough?  */
    char        host[HOST_BSZ];
    char        user[USER_BSZ];
    int         plcl, prmt;
    char       *p;

    /*  preload a few storage areas  */
    host[0] = 0x00;
    user[0] = 0x00;

    /*  first,  tell me about this end  */
    slen = sizeof(sadr);
    rc = getsockname(sock,&sadr,&slen);
    if (rc != 0)
      { /* perror("getsockname()"); */
        if (rc < 0) return rc;
                else return -1;
      }
    styp = sadr.sa_family;

    /*  where's the offset into the address?  */
    switch (styp)
      {
        case AF_INET:   soff = 2;   slen = 4;
                        break;
        default:        soff = 2;
                        break;
      }

    /*  and snag that port number  */
    plcl = 0;
    for (i = 0; i < soff; i++)
        plcl = (plcl << 8) + (sadr.sa_data[i] & 0xFF);

/*
(void) sprintf(temp,"PORT=%d (mine)",plcl);
(void) netline(2,temp);
 */

    /*  what's the host on the other end?  */
    slen = sizeof(sadr);
    rc = getpeername(sock,&sadr,&slen);
    if (rc != 0)
      { /* perror("getpeername()"); */
        if (rc < 0) return rc;
                else return -1;
      }
    styp = sadr.sa_family;

    /*  where's the offset into the address?  */
    switch (styp)
      {
        case AF_INET:   soff = 2;   slen = 4;
                        break;
        default:        soff = 2;
                        break;
      }

    /*  now copy the address  */
    for (i = 0; i < slen; i++)
        hadd[i] = sadr.sa_data[i+soff];

    /*  and snag that port number  */
    prmt = 0;
    for (i = 0; i < soff; i++)
        prmt = (prmt << 8) + (sadr.sa_data[i] & 0xFF);

/*
(void) sprintf(temp,"PORT=%d (yours)",prmt);
(void) netline(2,temp);
 */
    /*  what host is at that address?  */
    hent = gethostbyaddr(hadd,slen,styp);
    if (hent == NULL)
      { /* perror("gethostbyaddr()"); */
        if (rc < 0) return rc;
                else return -1;
      }
    strncpy(host,hent->h_name,HOST_BSZ);    /*  keep it  */
    host[HOST_BSZ-1] = 0x00;    /*  safety net  */

/*
(void) sprintf(temp,"HOST=%s (yours)",host);
(void) netline(2,temp);
 */

#ifdef USE_IDENT
    /*  try a little IDENT client/server action  */
    (void) sprintf(temp,"%s:%d",host,IDENT_PORT);
    sock = tcpopen(temp,0,0);
    if (sock >= 0)
      {
        /*  build and send the IDENT request  */
        (void) sprintf(temp,"%d , %d",prmt,plcl);
        (void) tcpputs(sock,temp);
        (void) tcpgets(sock,temp,TEMP_BSZ);

        for (p = temp; *p != 0x00 && *p != ':'; p++);
        if (*p == ':')
          {
            p++;
            while (*p != 0x00 && *p <= ' ') p++;
/*  (void) netline(2,p);  */
            if (strncmp(p,"USERID",6) == 0)
              {
                while (*p != 0x00 && *p != ':') p++;
                if (*p == ':') p++;
                while (*p != 0x00 && *p != ':') p++;
                if (*p == ':') p++;
                while (*p != 0x00 && *p <= ' ') p++;
                (void) strncpy(user,p,USER_BSZ);
              }
          }
      }
#else
    user[0] = 0X00;
#endif

    (void) sprintf(buff,"%s@%s",user,host);

    return 0;
  }

/* include the following code only if supporting IBM OpenEdition/USS  */
#ifdef          OECS
#include        "aecs.h"

/* --------------------------------------------------------------- HTONC
 *  Host-to-Network, alpha (character)
 */
unsigned char htonc(unsigned char c)
  { static char _eyecatcher[] = "htonc()";
#if     '\n' == 0x15
    return (asc8859[c]);
#else
    return c;
#endif
  }

/* --------------------------------------------------------------- NTOHC
 *  Network-to-Host, alpha (character)
 */
unsigned char ntohc(unsigned char c)
  { static char _eyecatcher[] = "ntohc()";
#if     '\n' == 0x15
    return (ebc8859[c]);
#else
    return c;
#endif
  }

/* --------------------------------------------------------------- HTONZ
 *  Host-to-Network, alpha (Z-string)
 */
int htonz(unsigned char*s)
  { static char _eyecatcher[] = "htonz()";
#if     '\n' == 0x15
    int i;
    for (i = 0; (s[i] = asc8859[s[i]]) != 0x00; i++);
    return i;
#else
    return strlen(s);
#endif
  }

/* --------------------------------------------------------------- NTOHZ
 *  Network-to-Host, alpha (Z-string)
 */
int ntohz(unsigned char*s)
  { static char _eyecatcher[] = "ntohz()";
#if     '\n' == 0x15
    int i;
    for (i = 0; (s[i] = ebc8859[s[i]]) != 0x00; i++);
    return i;
#else
    return strlen(s);
#endif
  }

#endif

/* --------------------------------------------------------------- HTONB
 *  Host-to-Network, alpha (block)
 */
int htonb(unsigned char*p,unsigned char*q,size_t l)
  { static char _eyecatcher[] = "htonb()";
    unsigned char v;
    int         i, j;

    v = 0x00;
    j = 0;
    for (i = 0; i < l; i++)
      {
#if     '\n' == 0x15
    if (q[i] == '\n')
      {
        if (v != '\r') p[j++] = 0x0D;
        p[j++] = 0x0A;
      } else
    p[j++] = asc8859[q[i]];
#else
        if (q[i] == '\n' && v != '\r')
                p[j++] = '\r';
        p[j++] = q[i];
#endif
        v = q[i];
      }
    return j;
  }

/* --------------------------------------------------------------- NTOHB
 *  Network-to-Host, alpha (block)
 */
int ntohb(unsigned char*p,unsigned char*q,size_t l)
  { static char _eyecatcher[] = "ntohb()";
    unsigned char v;
    int         i, j;

    v = 0x00;
    j = 0;
    for (i = 0; i < l; i++)
      {
        if (q[i] == 0x0A && v != 0x0D) j--;
#if     '\n' == 0x15
        p[j++] = ebc8859[q[i]];
#else
        p[j++] = q[i];
#endif
        v = q[i];
      }
    return j;
  }


