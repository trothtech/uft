/* ------------------------------------------------------------ READSPAN
 *  Read a "spanning record". When reading from a pipe, the requested
 *  number of bytes might not be available to just one read() call.
 *  This function performs as many read()s as needed until the requested
 *  number of bytes are acquired. This is how we explicitly discard
 *  any record structure that UNIX may have learned about.
 */

#include <unistd.h>


int uft_readspan(int s,char*b,int c)
  { static char _eyecatcher[] = "uft_readspan()";
    int         i,  j;

    for (j = 0; c > 0; )
      { i = read(s,&b[j],c);
        if (i < 0) return i;
        if (i < 1) break;
        j = j + i;
        c = c - i; }
    return j;
  }


