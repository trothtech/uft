/* ----------------------------------------------------------------- �CS
 * ASCII to EBCDIC and vice-versa code conversion routines.
 * Tables included here are based on ASCII conforming to the ISO8859-1
 * Latin 1 character set and EBCDIC conforming to the IBM Code Page 37
 * Latin 1 character set (except for three pairs of characters in 037).
 * This C source requires header file AECS.H.
 */
 
#include "aecs.h"
 
/* ------------------------------------------------------------- CHRATOE
 * Translate an ASCII character into EBCDIC.
 */
int chratoe(c)
  int     c;
  {
    return (ebc8859[c]);
  }
 
/* ------------------------------------------------------------- STRATOE
 * Translate an ASCII string into EBCDIC in place. Return length.
 */
int stratoe(string)
  unsigned char *string;
  {
    int i;
 
    for (i = 0; (string[i] = ebc8859[string[i]]) != 0x00; i++);
    return (i);
  }
 
/* ------------------------------------------------------------- CHRETOA
 * Translate an EBCDIC character into ASCII.
 */
int chretoa(c)
  int     c;
  {
    return (asc8859[c]);
  }
 
/* ------------------------------------------------------------- STRETOA
 * Translate an EBCDIC string into ASCII in place. Return length.
 */
int stretoa(string)
  unsigned char *string;
  {
    int i;
 
    for (i = 0; (string[i] = asc8859[string[i]]) != 0x00; i++);
    return (i);
  }
