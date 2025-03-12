/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *	  Name: uftduser.c
 *		Unsolicited File Transfer daemon "user" function
 *		Returns a non-negative uid on success.
 *		Returns zero (root) for valid queues
 *		which have no real user to back them up.
 *
 *		Thanks to Bill Hunter at the University of Alabama
 *		for reporting certain problems with AIX here.
 *
 */

#include <pwd.h>
#include <errno.h>
extern	int	errno;
#include <unistd.h>
#include <sys/stat.h>

#include "uft.h"

/* ------------------------------------------------------------ UFTDUSER
 *  Move into the specified user's UFT sub-dir, possibly creating it,
 *  and try to seteuid() to that user too.
 */
int uftduser(char*user)
  { static char _eyecatcher[] = "uftduser()";
    int 	i, uuid;
    struct passwd *pwdent;

    /* we'll try to make this non-zero later */
    uuid = 0;

    /* pseudo-users are supported;  that is,  one can 'sendfile'
	to a user that doesn't exist iff the sub-directory exists */
    pwdent = getpwnam(user);

    /* does the directory exist already? */
    i = chdir(user);

    /* if we are avoiding metadata leakage then try "anonymous" */
#ifdef          UFT_ANONYMOUS
    if (i < 0 && errno == ENOENT) i = chdir("anonymous");
#endif

    /* some error;  should we create a sub-dir? */
    if (i < 0 && errno == ENOENT)
      {
	if (pwdent == NULL) return -1;
	if (mkdir(user,0770) < 0) return i;
	if (pwdent != NULL)
	(void) chown(user,pwdent->pw_uid,UFT_GID);
	i = chdir(user);
      }

    /* errors persist!  bail out! */
    if (i < 0) return i;

    /* if the user exists,  try chowning the SEQuence file(s)
	and the directory,  if that works,  set effective UID */
    if (pwdent != NULL)
      {
	uuid = pwdent->pw_uid;
	(void) chown(UFT_SEQFILE,uuid,UFT_GID);
	(void) chmod(UFT_SEQFILE,0660);
	(void) chown(UFT_SEQFILE_ALT,uuid,UFT_GID);
	(void) chmod(UFT_SEQFILE_ALT,0660);
	if (chown(".",uuid,UFT_GID) == 0) (void) seteuid(uuid);
      }

    /* return the uid (non-negative) on success */
    return uuid;
  }


