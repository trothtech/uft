/*
 *
 *        Name: xmmwin.c (C program source)
 *              special support for XMM/XMITMSGX on MS Windows
 *      Author: Rick Troth, rogue programmer
 *        Date: 2026-05-09 (Sat)
 *
 */

#include "xmmwin.h"

/* void openlog(const char *__ident, int __option, int __facility); */
void openlog(char*ident,int option,int facility) { return; }
void closelog(void) { return; }

/* NOTE: This is NOT a general purpose SYSLOG service function.       */
/*    It is specifically for supporting XMM/XMITMSGX use of SYSLOG.   */
void syslog(int priority,char*syslgfmt,char*syslgtxt) { return ; }

#include <stdlib.h>

char*getlogin()
  {
    char *u;
    u = getenv("USER");
    if (u != NULL) if (*u != 0x00) return u;
    u = getenv("USERNAME");
    if (u != NULL) if (*u != 0x00) return u;
    return "?";
  }


