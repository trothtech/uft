/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *	  Name: uftctext.c
 *		Unsolicited File Transfer client text clarifier
 *		Converts NL (UNIX) delimited lines to CR/LF
 *		delimited (Internet NVT) lines in a buffer.
 *	Author: Rick Troth, Houston, Texas, USA (METRO)
 *	  Date: 1995-Apr-13
 *
 */

#include <unistd.h>

#include	"uft.h"
#include        "tcpio.h"

/* ------------------------------------------------------------ UFTCTEXT
 */
int uftctext(int s,char*b,int l)
  { static char _eyecatcher[] = "uftctext()";
    char	t[BUFSIZ] /* , *p */ ;
    int 	i, j, k;

    k = l / 2;
    if (k > BUFSIZ) k = BUFSIZ;

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


