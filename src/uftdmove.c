/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *	  Name: uftdmove.c
 *		Unsolicited File Transfer daemon "move" routine
 *		Moves control file contents accumulated thus far
 *		from server space into user space.
 *
 *        NOTE: This source is due for merge into UFTD or UFTLIB.
 */

#include <unistd.h>

/* ------------------------------------------------------------ UFTDMOVE
 */
int uftdmove(int a,int b)
  { static char _eyecatcher[] = "uftdmove()";
    int 	i, j;
    char	q[4096];
    (void) lseek(b,0,0);	/*  "rewind"  */
    while (1)
      {
	i = read(b,q,4096);
	if (i < 1) break;
	j = write(a,q,i);
	if (j < i) break;
      }
    return 0;
  }


