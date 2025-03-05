/* © Copyright 1993-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCHOST REXX
 *              passes the SIFT job to the target host stage
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, TX (METRO)
 *        Date: 1993-Feb-20, Oct-20
 *              2025-02-11
 *       Calls: UFTCRSCS, UFTCTCP, UFTCMAIL
 *
 *        Note: UFTCHOST is not a user-level pipeline stage.
 */

Parse Arg host . "(" .
If host = "" Then Do
    'CALLPIPE COMMAND XMITMSG 386 (ERRMSG | CONSOLE'
    /* Missing operand(s) */
    Exit 24 ; End

'PEEKTO'        /* verify existence of input stream */
If rc ^= 0 Then Do
    'CALLPIPE COMMAND XMITMSG 1606 (ERRMSG | CONSOLE'
    /* No primary input specified */
    Exit rc ; End

/* snag some global variables for proper behavior */
Address "COMMAND" 'GLOBALV SELECT UFTD GET RSCSID SMTPID'

If rscsid = "*" Then rscsid = ""
If rscsid = "" Then Address "COMMAND" 'GLOBALV SELECT UFT GET RSCSID'
If rscsid = "*" Then rscsid = ""

If smtpid = "*" Then smtpid = ""
If smtpid = "" Then Address "COMMAND" 'GLOBALV SELECT UFT GET SMTPID'
If smtpid = "*" Then smtpid = ""

/* strip off any leading  "user@" part */
If Index(host,"@") > 0 Then Parse Var host . "@" host

If Index(host,".") = 0 & rscsid ^= "" Then ,
    'CALLPIPE *: | UFTCRSCS' host '| *:'
Else Do
    Trace "OFF"
    /* First try sending the file using TCP directly. */
    'CALLPIPE *: | UFTCTCP'  host '| *:';
    /* If that fails, and we know SMTPD, then try sending as mail. */
    If rc ^= 0 & smtpid ^= "" Then ,
    'CALLPIPE *: | UFTCMAIL' host '| *:'
    End  /*  If  ..  Do  */

Exit rc


