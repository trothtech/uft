/* � Copyright 1993-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCTCP REXX
 *              Pipeline stage to feed a SIFT job to UFTD via TCP/IP
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *        Date: 1993-Feb-22
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Jan-24
 *
 *        Note: UFTCTCP is not a user-level pipeline stage.
 *
 *    See also: UFTCFILE, UFTCUSER, UFTCHOST
 *
 *        Note: Unlike some other UFTCMAIN followers,
 *              this stage does not always fill-in a USER statement.
 *              If the source stream should provide a USER statement
 *              that is taken as-is unless overriddent by arguments.
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argl = '*' || arg0 || ':'

/* parse command line arguments */
Parse Arg host port . '(' . ')' .
If host = "" Then host = "localhost"
If Index(host,'@') > 0 Then Parse Var host user '@' host
                       Else user = ""
Parse Var host host ':' port
If port = "" Then port = 608
temp = "UFT" || Right(Time('S'),5,'0')
buffer = ""
seq = "*"
u = "-"

/* Verify REXX/Sockets Version 2 ... realy REALLY unlikely to not be  */
Parse Value Socket("VERSION") With rc . ver .
If ver < 2 Then /* Exit -1 */ Exit 0

/* Initialize RXSOCKET */
Parse Value Socket('Initialize', temp) With rc rs
If rc ^= 0 Then Do
    Parse Var rs errno etext
    If errno ^= "ESUBTASKALREADYACTIVE" Then Do
        tcprc = rc
        'OUTPUT' argl rs
        Exit tcprc
    End /* If .. Do */
    Call Socket 'Terminate', temp
    Parse Value Socket('Initialize', temp) With rc rs
    If rc ^= 0 Then Do
        tcprc = rc
        'OUTPUT' argl rs
        Exit tcprc
    End /* If .. Do */
End /* If .. Do */

/* Get a socket descriptor (TCP protocol) */
Parse Value Socket('Socket','AF_INET','Sock_Stream') With rc rs
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    Call Socket 'Terminate', temp
    Exit tcprc
End /* If .. Do */
Parse Var rs s .

/* Figure out the target host address */
hisaddr = _hostaddr(host)
If hisaddr = "" Then Do

    /* address-from-name not yet known, try the resolver */
    Parse Value Socket('Resolve',host) With rc rs
    If rc ^= 0 Then Do
        tcprc = rc ; 'OUTPUT' argl rs
        Call Socket 'Terminate', temp
        Exit tcprc ; End

    /* if that worked then save the info for next go-round */
    Parse Var rs hisaddr hisname .
    _var = "$" || hisaddr
    Call Value _var, hisname, "SESSION NSLOOKUP"
    Upper hisname
    _var = "#" || hisname
    Call Value _var, hisaddr, "SESSION NSLOOKUP"

End /* If .. Do */

/* Connect to the UFT server */
Parse Value Socket('Connect',s,'AF_INET' port hisaddr) With rc rs
If rc ^= 0 Then Do
    Parse Var rs errno etext
    If errno ^= "EINPROGRESS" Then Do
        tcprc = rc
        'OUTPUT' argl rs
        Call Socket 'Terminate', temp
        Exit tcprc * (tcprc ^= 61)
    End /* If .. Do */
End /* If .. Do */

/* Read the herald from the server. */
line = getline(s)
/* UFT/1 or UFT/2? */
If Left(line,1) = '2' Then uft = 2
                      Else uft = 1        /* really REALLY not likely */
'OUTPUT' line
'OUTPUT' argl "UFT level" uft

type = "";  cc = ""
/* Send the commands (parmeters; ie: the "meta" file) */
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then Leave

    /* Echo the command from our input stage and then parse it. */
    'OUTPUT' line
    Parse Upper Var line cmnd .
    If cmnd = "META" Then Do
        Parse Var line . line
        Parse Upper Var line cmnd .
        meta = 1
    End ; Else meta = 0

    /* watch for a DATA statement, which breaks us out of this loop   */
    If Abbrev("DATA",cmnd,4) & ^meta Then Do
        rc = 0 ; Leave ; End

    /* snag the TYPE indicator for canonicalization (see below)       */
    If Abbrev("TYPE",cmnd,4) & ^meta Then ,
        Parse Upper Var line . type cc .

    /* if user specified on command line then do NOT sent from stream */
    If Abbrev("USER",cmnd,4) & ^meta & user ^= "" Then Do
        'OUTPUT' argl "USER statement overridden"
        'READTO'
        Iterate
    End /* If .. Do */

    /* but do collect USER commands if none specified on command line */
    If Abbrev("USER",cmnd,4) & ^meta Then Parse Var line . u .

    /* watch the swizzle - FILE statement might signal USER statement */
    If Abbrev("FILE",cmnd,4) & ^meta & user ^= "" Then Do
        Call PUTLINE s, line       /* go ahead and send FILE stmt now */
        Parse Value uftcwack(s) With ac as       /* and expect an ACK */
        If ac ^= 0 Then Do
            'OUTPUT' as ; rc = ac ; Leave ; End
        /* force next line sent to be the requisite USER statement    */
        line = "USER" user
        'OUTPUT' line
    End

    /* Send a packet */
    Call PUTLINE s, line

    /* for some commands, DON'T wait for ACK */
    If Left(line,1) = '*' | Left(line,1) = '#' Then Do
        'READTO'
        Iterate
    End /* If .. Do */

    /* Recover some response (ACK or NAK) */
    Parse Value uftcwack(s) With ac as
    If ac ^= 0 Then Do
        'OUTPUT' as
        rc = ac
        If Left(as,1) ^= "4" Then Leave
    End /* If .. Do */

    'READTO'

End /* Do Forever */

If rc ^= 0 Then Exit rc /* and kill the socket */
'READTO'
If rc ^= 0 Then Exit rc /* and kill the socket */

/*                                                                    *
 * OK ... now we've sent the control records (the meta data).         *
 * If it was fully accepted (no NAKs), then send the body.            *
 *                                                                    */

/* apply canonicalization for transport */
Select
    When type = "V" & cc = "M" Then Do
        /* spool-to-spool (VM to VM) transfer */
        pipe = "BLOCK 61440 CMS"
    End /* When .. Do */
    When type = "A" | type = "T" Then Do
        /* text file; send as ASCII with 0x0A+0x0D */
        pipe = "MAKETEXT NETWORK | FBLOCK 61440"
    End /* When .. Do */
    When type = "E" Then Do
        /* text file; send as EBCDIC with 0x15 */
        pipe = "BLOCK 61440 TEXTFILE"
    End /* When .. Do */
    When type = "I" | type = "B" | type = "U" Then Do
        /* binary file (unstructured octet stream) */
        pipe = "FBLOCK 61440"
    End /* When .. Do */
    When type = "N" Then Do
        /* IBM NETDATA format (generated by upstream stages) */
        pipe = "FBLOCK 61440"
    End /* When .. Do */
    When type = "V" Then Do
        /* variable-length records */
        pipe = "BLOCK 61440 CMS"
    End /* When .. Do */
    Otherwise Do
        /* all others, treat as binary */
        pipe = "FBLOCK 61440"
    End /* Otherwise Do */
End /* Select */

/* apply canonicalization for transport */
'ADDPIPE *.INPUT: |' pipe '| *.INPUT:'
If rc ^= 0 Then Exit rc

j = 0;  i = 0
/* Send the body of the file (the actual data) */
Do Forever

    'PEEKTO DATA'
    If rc ^= 0 Then Leave

    /* tell the server it's coming */
    Call PUTLINE s, "DATA" Length(data)

    If uft = 2 Then Do
        /* Recover some response (ACK or NAK) */
        Parse Value uftcwack(s) With ac as
        If ac ^= 0 Then Do
            'OUTPUT' as
            rc = ac
            Leave
        End /* If .. Do */
    End /* If .. Do */

    /* Send the "burst" */
    Parse Value Socket('Write',s,data) With rc rs
    If rc ^= 0 Then Do
        Say rs
        Leave
    End /* If .. Do */

    /* Recover some response (ACK or NAK) */
    Parse Value uftcwack(s) With ac as
    If ac ^= 0 Then Do
        'OUTPUT' as
        rc = ac
        Leave
    End /* If .. Do */

    j = j + 1
    i = i + Length(data)

    'READTO'

End /* Do Forever */

If rc ^= 0 & rc ^= 12 Then Exit rc

/* log this completion */
'OUTPUT' argl j "records sent;" i "bytes sent"

/* provide an explicit 8712 for clarity saying "the file got through" */
date = Date("S") || "-" || Time()
If user = "" Then user = u
Address "COMMAND" 'XMITMSG 8712 SEQ USER HOST DATE' ,
                        '(APPLID UFT CALLER TCP VAR'
If rc = 0 Then 'OUTPUT' argl message.1

/* Send an EOF command */
'OUTPUT' "EOF"
Call PUTLINE s, "EOF"
Call UFTCWACK s                    /* ignoring the response condition */

/* Send a QUIT command */
'OUTPUT' "QUIT"
Call PUTLINE s, "QUIT"
Call UFTCWACK s                    /* ignoring the response condition */

/* -- EXIT ---------------------------------------------------------- */

/* All done, relinquish our socket descriptor */
Parse Value Socket("CLOSE",s) With rc rs
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    Call Socket 'Terminate', temp
    Exit tcprc
End

/* Tell RXSOCKET that we are done with this IUCV path */
Parse Value Socket("TERMINATE") With rc rs
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    Exit tcprc
End

Return

/* ------------------------------------------------------------- GETLINE
 */
GETLINE: Procedure Expose buffer
Parse Arg sock
Do While Index(buffer,'0A'x) = 0 & Index(buffer,'00'x) = 0
    Parse Value Socket('READ',sock) With rc bc data
    If rc ^= 0 Then Return ""
    buffer = buffer || data
    End /* Do While */
Parse Var buffer line '0A'x buffer
Parse Var line line '0D'x .
Parse Var line line '00'x .
'CALLPIPE VAR LINE | SPEC 1-* 1 /0D0A/ X2C NEXT' ,
    '| MAKETEXT LOCAL | VAR LINE'
Return line

/* ------------------------------------------------------------ UFTCWACK
 *  Wait for ACK
 *  Response codes starting with '2' are ACK, '1' are informational
 *  (thus, iterate), '5' and '4' error, '3' means "send more".
 */
UFTCWACK: Procedure Expose buffer uft
Parse Arg s
Do Forever
    line = getline(s)
    If line = "" Then Return 0 "ACK (NULL)"
    If Left(line,1) = '1' Then Iterate
    If Left(line,1) = '2' Then Return 0 line
    If Left(line,1) = '3' & uft = 2 Then Return 0 line
    If Left(line,1) = '4' Then Return 1 line
    If Left(line,1) = '5' Then Return 1 line
    If Left(line,1) = '+' Then Return 0 line    /* defunct */
    If Left(line,1) = '-' Then Return 1 line    /* defunct */
    End /* If .. Do */
Return 1 line

/* ------------------------------------------------------------- PUTLINE
 */
PUTLINE: Procedure Expose argl
Parse Arg sock, line
'CALLPIPE VAR LINE | MAKETEXT NETWORK | VAR LINE'
Parse Value Socket('Write', sock, line) With rc rs
If rc = 0 Then Return 0
tcprc = rc
'OUTPUT' argl rs
Return tcprc

/* ------------------------------------------------------------ HOSTADDR
 *  return IP address for the supplied hostname
 *  "#" stores the number which goes with the supplied name
 */
_hostaddr: Procedure
Parse Upper Arg h . , .
If h = "" Then Return h
var = "#" || h

/* if the address for this host is already known then return it */
val = Value(var,,"SESSION NSLOOKUP")
If val = "VAL" Then val = ""
If val /= "" Then Return val

/* try the lookup */
Address "COMMAND" 'PIPE VAR H | hostbyname | VAR VAL'
If rc /= 0 Then val = ""
If val = "VAL" Then val = ""

If val = "" Then val = h

/* if we got an address then stash it for later reference */
If val ^= "" Then Call Value var, val, "SESSION NSLOOKUP"

Return val

/* ------------------------------------------------------------ HOSTNAME
 *  return the internet hostname for the given IP address
 *  "$" stores the name which goes with the supplied address
 */
_hostname: Procedure
If h = "" Then Return h
var = "$" || h

/* if this host is already known then return it as-is */
val = Value(var,,"SESSION NSLOOKUP")
If val = "VAL" Then val = ""
If val /= "" Then Return val

/* if the remote address is IPv6 then skip the lookup */
If POS(":",h) > 0 Then Do
    val = "[" || h || "]"
    Call Value var, val, "SESSION NSLOOKUP"
    Return val
End

/* try the lookup */
Address "COMMAND" 'PIPE VAR H | HOSTBYADDR | VAR VAL'
If rc /= 0 Then val = ""
If val = "VAL" Then val = ""

/* wrap failing address in parenthesis */
If rc /= 0 Then Do
    val = "(" || h || ")"
    Call Value var, val, "SESSION NSLOOKUP"
    Return val
End

/* if we got nuthin then return address as-is */
If val = "" Then Return h

/* otherwise set this for future reference */
Call Value var, val, "SESSION NSLOOKUP"

Return val


