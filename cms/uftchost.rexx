/*
 *        Name: UFTCHOST REXX
 *              passes the SIFT job to the target host
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, TX (METRO)
 *        Date: 1993-Feb-20, Oct-20
 *       Calls: UFTCRSCS, UFTCTCP, UFTCMAIL
 *
 *        Note: UFTCHOST is not a user-level pipeline stage.
 */

Parse Arg host . '(' .
If host = "" Then Do
    'CALLPIPE COMMAND XMITMSG 2687 (ERRMSG | CONSOLE'
    /*  "Missing parameters."  */
    Exit 24
    End  /*  If  ..  Do  */

'PEEKTO'        /* verify existence of input stream */
If rc ^= 0 Then Do
    'CALLPIPE COMMAND XMITMSG 2914 (ERRMSG | CONSOLE'
    /*  "no input stream"  /  "too few input streams"  */
    Exit rc
    End  /*  If  ..  Do  */

/*  strip off any leading  "user@"  part  */
If Index(host,'@') > 0 Then Parse Var host . '@' host

If Index(host,'.') = 0 Then
    'CALLPIPE *: | UFTCRSCS' host '| *:'
Else Do
    Trace "OFF"
    /*  First,  try using TCP directly.  If that   *
     *  does't work,  then try sending it as mail  */
    'CALLPIPE *: | UFTCTCP'  host '| *:';   If rc ^= 0 Then
    'CALLPIPE *: | UFTCMAIL' host '| *:'
    End  /*  If  ..  Do  */

Exit rc

