/* © Copyright 1993-2026 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCHOST REXX
 *              passes the UFT batch job to the target host stage
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, TX (METRO)
 *        Date: 1993-Feb-20, Oct-20
 *              2025-02-11
 *       Calls: UFTCRSCS, UFTCTCP2, UFTCMAIL
 *
 *        Note: UFTCHOST is not a user-level pipeline stage.
 */

Parse Arg host . "(" opts ")" .
If host = "" Then Do                               /* missing operand */
    'CALLPIPE COMMAND XMITMSG 386 (ERRMSG | CONSOLE'
    Exit 24 ; End

'PEEKTO'                         /* confirm existence of input stream */
If rc ^= 0 Then Do                      /* no primary input specified */
    xrc = rc
    'CALLPIPE COMMAND XMITMSG 1606 (ERRMSG | CONSOLE'
    Exit xrc ; End

/* snag some global variables for proper behavior */
Address "COMMAND" 'GLOBALV SELECT UFTD GET RSCSID SMTPID'

/* process command-line options */
ssl = 0
Do While opts ^= ""
    Parse Upper Var opts op opts
    Select /* op */
        When Abbrev("SSL",op,3)      Then ssl = 1
        When Abbrev("NOSSL",op,3)    Then ssl = 0
        When Abbrev("TLS",op,3)      Then ssl = 1
        When Abbrev("NOTLS",op,3)    Then ssl = 0
        When Abbrev("SECURE",op,3)   Then ssl = 1
        When Abbrev("NOSECURE",op,3) Then ssl = 0
        Otherwise Do
            'XMITMSG 3 OP (ERRMSG CALLER UFT'; Exit 24; End
    End /* Select op */
End

If rscsid = "*" Then rscsid = ""
If rscsid = "" Then Address "COMMAND" 'GLOBALV SELECT UFT GET RSCSID'
If rscsid = "*" Then rscsid = ""

If smtpid = "*" Then smtpid = ""
If smtpid = "" Then Address "COMMAND" 'GLOBALV SELECT UFT GET SMTPID'
If smtpid = "*" Then smtpid = ""

/* strip off any leading "user@" part */
If Index(host,"@") > 0 Then Parse Var host . "@" host

If ssl Then tls = "(SSL"; Else tls = ""                /* TLS and SSL */

If Index(host,".") = 0 & rscsid ^= "" Then ,
    'CALLPIPE *: | UFTCRSCS' host '| *:'
Else Do
    /* First try sending the file using TCP directly (SSL or clear).  */
    'CALLPIPE *: | UFTCTCP2' host tls '| *:'           /* TLS and SSL */
    /* If that fails, *and* we know SMTPD, then try sending as mail.  */
    If rc ^= 0 & smtpid ^= "" Then ,
    'CALLPIPE *: | UFTCMAIL' host '| *:'
End /* If .. Do */

Exit rc


