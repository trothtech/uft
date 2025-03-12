/* ------------------------------------------------------------- GETLINE
 *	  Name: GETLINE/UFTXGETS/UFTXRCVS
 *		common Get/Receive String function
 *   Operation: Reads a CR/LF terminated string from stream s
 *		into buffer b.  Returns the length of that string.
 *	Author: Rick Troth, Ithaca NY, Houston TX (METRO)
 *	  Date: 1993-Sep-19, Oct-20
 *
 *	  Note: modified 1996-Jun-16 to support OpenEdition EBCDIC
 *		See the README file for more information.
 *
 *    See also: putline.c, netline.c
 *
 */

#include <unistd.h>

#ifdef		__OPEN_VM
#ifndef 	OECS
#define 	OECS
#endif
#endif

/* ------------------------------------------------------------------ */
int uft_getline(int s,char*b)
  { static char _eyecatcher[] = "uft_getline()";
    char       *p;
    int 	i;

#ifdef	OECS
    char	snl;
    snl = '\n';
#endif

    p = b;
    while (1)
      {
	if (read(s,p,1) != 1)		/*  get a byte  */
	if (read(s,p,1) != 1) return -1;	/*  try again  */
	switch (*p)
	  {
#ifdef	OECS
	    case 0x0A:		/*  found an ASCII newline  */
		*p = 0x00;	/*  terminate the string  */
		/*  on an EBCDIC system?  */
		if (snl != 0x0A) (void) stratoe(b);
		break;
	    case 0x15:		/*  found an EBCDIC newline  */
		*p = 0x00;	/*  terminate the string  */
		/*  on an ASCII system?  */
		if (snl != 0x15) (void) stretoa(b);
		break;
#else
	    case '\n':		/*  found a generic newline  */
		*p = 0x00;	/*  terminate the string  */
		break;
#endif
	    default:
		break;
	  }
	if (*p == 0x00) break;		/*  NULL terminates  */
	p++;				/*  increment pointer  */
      }
    *p = 0x00;		/*  NULL terminate,  even if NULL  */

    i = p - b;		/*  calculate the length  */
    if (i > 0 && b[i-1] == '\r')	/*  trailing CR?  */
      {
	i = i - 1;	/*  shorten length by one  */
	p--;		/*  backspace  */
	*p = 0x00;	/*  remove trailing CR  */
      }

    return i;
  }


