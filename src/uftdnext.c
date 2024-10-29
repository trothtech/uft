/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *	  Name: uftdnext.c
 *		Unsolicited File Transfer daemon "next" routine
 *		returns next available SEQuence number
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#include "uft.h"

/* ------------------------------------------------------------ UFTDNEXT
 */
int uftdnext()
  {
    int 	i, n, n0, sf;
    char	temp[256], *seq;

    /*  start with our preferred SEQuence file name  */
    seq = UFT_SEQFILE;
    /*  switch to alternate if we run into errors  */

    /*  first,  try to open the sequence file  */
#ifdef	WEAKOPEN
    sf = open(seq,O_RDWR|O_CREAT);
    if (sf < 0 && errno == EINVAL)
      {
	seq = UFT_SEQFILE_ALT;
	sf = open(seq,O_RDWR|O_CREAT);
      }
#else
    sf = open(seq,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (sf < 0 && errno == EINVAL)
      {
	seq = UFT_SEQFILE_ALT;
	sf = open(seq,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
      }
#endif
    if (sf < 0) return sf;
#ifdef	WEAKOPEN
    (void) sprintf(temp,"chmod 664 %s",seq);
    (void) system(temp);
#endif
    /*  (it would help if the above resulted in exclusive access)  */

    /*  now extract the sequence number  */
    i = read(sf,temp,8);
    if (i < 0)
      {
	(void) close(sf);
	return i;
      }
    temp[++i] = 0x00; 	/*  make sure string terminates */
    n0 = atoi(temp);		/*  convert string to int  */

    /*  increment the sequence number until a free slot is found  */
    for (n = n0; ++n; n != n0)
      {
	if (n > 9999) n = 1;	/*  wrap at 10000  */
				/*  0000 is reserved  */
	(void) sprintf(temp,"%04d.cf",n);
	if (access(temp,0)) break;
    /*  loop while  <nnnn>.cf  exists  */
    /*  (should also check for <nnnn>.df)  */
      }

    /*  if we've been around once,  don't continue  */
    if (n == n0)
      {
/*	(void) netline(1,"5XX no slots available!");  */
	(void) close(sf);
	/*  implies there are 10000 files in this directory!  */
	errno = ENOENT;
	return -1;
      }

    /*  now back-up to the start of the sequence file  */
    (void) lseek(sf,0,0);	/*  "rewind"  */
    /*  we need locking;  we also should check for errors here  */

    /*  write the new sequence number  */
    (void) sprintf(temp,"%04d",n);
    i = uft_putline(sf,temp);
    if (i < 0)
      {
	(void) close(sf);
	return i;
      }

    /*  and close it  */
    (void) close(sf);

    /*  and return this wonderful number  */
    return n;
  }


