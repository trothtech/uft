/* © Copyright 1997-2025 Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftcprot.c / uftcperm.c
 *              converts a stat() mode value into a
 *              U:RWX, G:RWX, W:RWX, S:RWED formatted string
 *
 *        NOTE: This source is due for merge into UFTC or UFTLIB.
 */

#include        <sys/types.h>
#include        <sys/stat.h>

/* ------------------------------------------------------------ UFTCPROT
 */
char *uftcprot(mode_t prot)
  { static char _eyecatcher[] = "uftcprot()";
    static char buffer[256];
    char        *p;

    p = buffer;

    /*  user permissions  */
    if (prot & (S_IRUSR | S_IWUSR | S_IXUSR))
      { *p++ = 'U';  *p++ = ':';
        if (prot & S_IRUSR) *p++ = 'R';
        if (prot & S_IWUSR) *p++ = 'W';
        if (prot & S_IXUSR) *p++ = 'X'; }
/*
        if (prot & S_IREAD) *p++ = 'R';
        if (prot & S_IWRITE) *p++ = 'W';
        if (prot & S_IEXEC) *p++ = 'X';
 */

    /*  group permissions  */
    if (prot & (S_IRGRP | S_IWGRP | S_IXGRP))
      { if (p != buffer) *p++ = ',';
        *p++ = 'G';  *p++ = ':';
        if (prot & S_IRGRP) *p++ = 'R';
        if (prot & S_IWGRP) *p++ = 'W';
        if (prot & S_IXGRP) *p++ = 'X'; }

    /*  world permissions  */
    if (prot & (S_IROTH | S_IWOTH | S_IXOTH))
      { if (p != buffer) *p++ = ',';
        *p++ = 'W';  *p++ = ':';
        if (prot & S_IROTH) *p++ = 'R';
        if (prot & S_IWOTH) *p++ = 'W';
        if (prot & S_IXOTH) *p++ = 'X'; }

    /*  terminate the string and return  */
    *p++ = 0x00;
    return buffer;
  }


