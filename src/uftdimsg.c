/* � Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftdimsg.c
 *              signal the user that a file has arrived
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Oct-21
 *
 *        NOTE: This source is due for merge into UFTD or UFTLIB.
 */

#include <stdio.h>
#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64)
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <netdb.h>
 #include <pwd.h>
 #include <errno.h>
#endif

#include "uft.h"

/* ------------------------------------------------------------------ */
int uftdimsg(char*user,char*file,char*from,char*type)
  /* spoolid (file) is character in case spoolids go non-numeric      *
   * (discouraged) or the number of digits increases (likely)         */
  { static char _eyecatcher[] = "uftdimsg()";

    char        imsg[UFT_BUFSIZ], *p;
    int         i;
    struct  passwd  *pwdent;
    char        pipe[256];
    int         fd;

    /*  first try  /tmp/$USER.msgpipe  */
    (void) sprintf(pipe,"/tmp/%s.msgpipe",user);
#ifdef UFT_POSIX
    fd = open(pipe,O_WRONLY|O_NDELAY);
#else
    fd = open(pipe,O_WRONLY);
#endif

    /*  if that didn't work,  then try other locations  */
#ifdef UFT_POSIX
    if (fd < 0 && errno == ENOENT)
      {
        /*  a 'mknod' with 622 perms (writable) might work too  */
        pwdent = getpwnam(user);
        if (pwdent) sprintf(pipe,"%s/.msgpipe",pwdent->pw_dir);
        /*  else (void) sprintf(pipe,"/home/%s/.msgpipe",user);  */
        else (void) sprintf(pipe,"%s/%s/.msgpipe",UFT_SPOOLDIR,user);
        /*  else (void) sprintf(pipe,".msgpipe");  */

        fd = open(pipe,O_WRONLY|O_NDELAY);
      }
#endif

    /*  if there's no listener ...  */
    if (fd < 0 && errno == ENXIO)
      {
        /*  launch our special application to listen  */
        /*  (but we don't have one!)  */
        /*  then re-try the open of the FIFO  */
#ifdef UFT_POSIX
        fd = open(pipe,O_WRONLY|O_NDELAY);
#else
        fd = open(pipe,O_WRONLY);
#endif
      }
    if (fd < 0) return fd;

    switch (type[0])
      {
        case 'A':  case 'a':  case 'T':  case 't':
                type = "TXT";  break;
        case 'I':  case 'i':  case 'B':  case 'b':  case 'U':  case 'u':
                type = "BIN";  break;
        case 'V':  case 'v':
                type = "VAR";  break;   /*  or  "V16"  */
        case 'C':  case 'c':  case 'P':  case 'p':
                type = "PRT";  break;
        case 'M':  case 'm':
                type = "MAIL";  break;
        case 'N':  case 'n':
                type = "NETDATA";  break;
        default:
                type = "UFT";
      }

    /* message text comes first */
    (void) sprintf(imsg,"%s file %s from %s",type,file,from);
    i = 0;  while (imsg[i] != 0x00) i++;  i++;

    /* environment style strings follow */
    p = "MSGTYPE=I";            /*  denote that this is an "I msg"  */
    while (*p != 0x00) imsg[i++] = *p++;  imsg[i++] = 0x00;

    /* terminate the message buffer (double NULL) */
    imsg[i++] = 0x00;   imsg[i++] = 0x00;

    (void) tcpwrite(fd,imsg,i);
    (void) close(fd);

    return 0;
  }


