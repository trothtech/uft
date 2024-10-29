/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *	  Name: userid.c
 *		return the login name associated with this process
 *	Author: Rick Troth, Rice University, Information Systems
 *	  Date: 1994-Jul-26
 *
 * 1995-Apr-17: added useridg() function
 *
 */

#include <unistd.h>

#ifndef _OE_SOCKETS
#include	<pwd.h>
#include	<sys/types.h>
#endif
#include	<stdlib.h>

/* -------------------------------------------------------------- USERID
 *  return login name from the best of several usable sources
 */
char *userid()
  {
    char       *u;
    extern  char       *getenv();

#ifndef _OE_SOCKETS
    struct  passwd     *pwdent;

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
 *  "g" for GECOS field,  return personal name string,  if available
 */
char *useridg()
  {
    char       *g;
    extern  char       *getenv();
    struct  passwd     *pwdent;

    /*  if the user set one,  take that  */
    g = getenv("NAME");
    if (g != 0x0000 && *g != 0x00) return g;

    /*  next,  try GECOS for effective uid key into passwd  */
    pwdent = getpwuid(geteuid());
    if (pwdent) return pwdent->pw_gecos;

    /*  next,  try GECOS field for real uid key into passwd  */
    pwdent = getpwuid(getuid());
    if (pwdent) return pwdent->pw_gecos;

    /*  give up!  */
    return userid();
  }
#endif


