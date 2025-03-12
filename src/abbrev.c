/* -------------------------------------------------------------- ABBREV
 * Returns length of info if info is an abbreviation of informat.
 * Returns zero if info does not match or is shorter than minlen.
 * Comparison is not case sensitive.
 */

#include <ctype.h>

int abbrev(char*informat,char*info,int minlen)
  { static char _eyecatcher[] = "abbrev()";
    int     i;
    for (i = 0; info[i] != 0x00; i++)
        if (toupper(informat[i]) != toupper(info[i])) return 0;
    if (i < minlen) return 0;
    return i;
  }


