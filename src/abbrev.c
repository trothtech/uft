/* -------------------------------------------------------------- ABBREV
 * Returns length of info if info is an abbreviation of informat.
 * Returns zero if info does not match or is shorter than minlen.
 */
int abbrev(char*informat,char*info,int minlen)
  {
    int     i;
    for (i = 0; info[i] != 0x00; i++)
        if (informat[i] != info[i]) return 0;
    if (i < minlen) return 0;
    return i;
  }




