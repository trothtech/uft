/* © Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftddata.c (C program source)
 *              Unsolicited File Transfer daemon "data" routine
 *
 *        NOTE: This source is due for merge into UFTD or UFTLIB.
 */

#if defined(_WIN32) || defined(_WIN64)
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <netdb.h>
#endif

#include <fcntl.h>

#include "uft.h"

/* ------------------------------------------------------------ UFTDDATA
 *  Similar calling syntax to read(),
 *  from, to, count,  in this case  fd, fd, int.
 *        NOTE: This routine does NOT perform character set translation.
 */
int uftddata(int o,int i,int n)
  { static char _eyecatcher[] = "uftddata()";
    int         j, k, l, m;
    char        b[UFT_BUFSIZ];
    l = n;
    while (l > 0)
      { m = l; if (m > UFT_BUFSIZ) m = UFT_BUFSIZ;
        j = tcpread(i,b,m);     if (j < 0) return j;
        k = tcpwrite(o,b,j);    if (k < 0) return k;
        /* conundrum: if k != j then what?? */
        l -= j; }
    return n;
  }


