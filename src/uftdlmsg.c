/* © Copyright 1996-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftdlmsg.c
 *              syslog the arrival of this file
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1996-Dec-10
 *
 * this routine cuts SYSLOG records something like
 *      mmm dd hh:mm:ss localhost uftd[]: ttt file nnn to uuu from fff
 *
 */

#include <syslog.h>
#include <fcntl.h>

#include "uft.h"

/* the following must persist until we can figure out IBM USS pre-processing */
#ifdef OECS
 #ifndef LOG_PID
  #define LOG_PID    0x01
 #endif
 #ifndef LOG_CONS
  #define LOG_CONS   0x02
 #endif
 #ifndef LOG_UUCP
  #define LOG_UUCP (8<<3)
 #endif
 #ifndef LOG_INFO
  #define LOG_INFO      6
 #endif
#endif

/* ------------------------------------------------------------------ */
int uftdlmsg(char*user,char*file,char*from,char*type)
  { static char _eyecatcher[] = "uftdlmsg()";

/*  (void) openlog("UFT",LOG_CONS,LOG_UUCP);  */
    (void) openlog("uftd",LOG_PID|LOG_CONS,LOG_UUCP);

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

    (void) syslog(LOG_INFO,
                "%s file %s to %s from %s",
                        type,file,user,from);

    (void) closelog();

    return 0;
  }


