/* © Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftddata.c
 *              Unsolicited File Transfer daemon "data" routine
 *
 */

#include <fcntl.h>

#include        "uft.h"

/* ------------------------------------------------------------ UFTDDATA
 *  Similar calling syntax to read(),
 *  from, to, count,  in this case  fd, fd, int.
 */
int uftddata(int o,int i,int n)
  { static char _eyecatcher[] = "uftddata()";
    int         j, k, l;
    char        b[BUFSIZ];
    l = n;
    while (n > 0)
      {
        j = read(i,b,n);
        if (j < 0) return j;
        k = write(o,b,j);
        if (k < 0) return k;
        n -= j;
      }
    return l;
  }


