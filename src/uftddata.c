/* © Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftddata.c (C program source)
 *              Unsolicited File Transfer daemon "data" routine
 *
 *        NOTE: This source is due for merge into UFTD or UFTLIB.
 */

#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64)
 #include <winsock2.h>
#else
 #include <sys/socket.h>
 #include <netdb.h>
#endif

#include "uft.h"

/* ------------------------------------------------------------ UFTDDATA
 *  Similar calling syntax to read(),
 *  from, to, count,  in this case  fd, fd, int.
 */
int uftddata(int o,int i,int n)
  { static char _eyecatcher[] = "uftddata()";
    int         j, k, l;
    char        b[UFT_BUFSIZ];
    l = n;
    while (n > 0)
      { j = tcpread(i,b,n);
        if (j < 0) return j;
        k = tcpwrite(o,b,j);
        if (k < 0) return k;
        n -= j; }
    return l;
  }


