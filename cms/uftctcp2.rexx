/* © Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCTCP2 REXX
 *              Pipeline stage to feed a SIFT job to UFTD via TCP/IP
 *              This is a replacement for UFTCTCP REXX.
 *              The rationale for the re-write is to avoid mixing
 *              Rexx/Sockets with CMS Pipelines.
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-09-15
 *    See also: UFTCTCP REXX, UFTCFILE, UFTCUSER, UFTCHOST
 *
 *        Note: UFTCTCP2 is not a user-level pipeline stage.
 *
 *       Input: a SIFT job (a batch mode UFT transaction representation)
 *      Output: logging info prefixed with a tag indicating this stage
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argl = '*' || arg0 || ':'                                  /* logging */

/* parse command line arguments */
Parse Arg host port . '(' . ')' .
If host = "" Then host = "localhost"
If Index(host,'@') > 0 Then Parse Var host user '@' host
                       Else user = ""
Parse Var host host ':' port
If port = "" Then port = 608

/* figure out the target host address */
hisaddr = _hostaddr(host)
If hisaddr = "" Then Do

    /* Initialize Rexx/Sockets */
    temp = "UFT" || Right(Time('S'),5,'0')
    Parse Value Socket('Initialize',temp) With rc rs
    If rc ^= 0 Then Do
        Parse Var rs errno etext
        If errno ^= "ESUBTASKALREADYACTIVE" Then Do
            tcprc = rc ; 'OUTPUT' argl rs                  /* logging */
            Exit tcprc ; End
        Call Socket 'Terminate', temp
        Parse Value Socket('Initialize',temp) With rc rs
        If rc ^= 0 Then Do
            tcprc = rc ; 'OUTPUT' argl rs                  /* logging */
            Exit tcprc ; End
    End /* If .. Do */

    /* address-from-name not yet known, try the resolver */
    Parse Value Socket('Resolve',host) With rc rs
    If rc ^= 0 Then Do
        tcprc = rc ; 'OUTPUT' argl rs                      /* logging */
        Call Socket 'Terminate', temp
        Exit tcprc ; End
    Call Socket 'Terminate', temp       /* no Rexx/Sockets here after */

    /* if that worked then save this IP address for the next go-round */
    Parse Var rs hisaddr hisname .
    _var = "$" || hisaddr
    Call Value _var, hisname, "SESSION NSLOOKUP"
    Upper hisname
    _var = "#" || hisname
    Call Value _var, hisaddr, "SESSION NSLOOKUP"

End /* If .. Do */


/* set-up a stream for the TCP/IP client driver */
'ADDSTREAM BOTH TCP'
If rc ^= 0 Then Exit rc

/* connect that TCP/IP client driver to contact the UFT server        */
'ADDPIPE *.OUTPUT.TCP: | TCPCLIENT' hisaddr port '| *.INPUT.TCP:'
If rc ^= 0 Then Exit rc

/* load ASCII/EBCDIC translate tables **
'CALLPIPE < POSIX TCPXLBIN | STEM AEX.'
If rc ^= 0 Then Do
    aex.2 = "" ** a2e **
    aex.3 = "" ** e2a **
End
aex.1 = XRANGE('00'x,'FF'x)                                           */

buffer = ""
u = "-"

/* read the herald from the server. */
line = getline()
/* does the server want UFT/1 or UFT/2? (and we hope for the latter)  */
If Left(line,1) = '2' Then uft = 2
                      Else uft = 1        /* really REALLY not likely */
'OUTPUT' line
'OUTPUT' argl "UFT level" uft

type = "";  cc = ""
/* Send the commands (parmeters; ie: the "meta" file) */
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then Leave

    /* echo the command from our input stage and then parse it        */
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
        'OUTPUT' argl "USER statement overridden/ignored"
        'READTO'
        Iterate
    End /* If .. Do */

    /* but do collect USER commands if none specified on command line */
    If Abbrev("USER",cmnd,4) & ^meta Then Parse Var line . u .

    /* watch the swizzle - FILE statement might signal USER statement */
    If Abbrev("FILE",cmnd,4) & ^meta & user ^= "" Then Do
        Call PUTLINE line          /* go ahead and send FILE stmt now */
        Parse Value uftcwack() With ac as        /* and expect an ACK */
        If ac ^= 0 Then Do
            'OUTPUT' as ; rc = ac ; Leave ; End
        /* force next line sent to be the requisite USER statement    */
        line = "USER" user
        'OUTPUT' line
    End

    /* send a packet */
    Call PUTLINE line

    /* for some commands, DON'T wait for ACK */
    If Left(line,1) = '*' | Left(line,1) = '#' Then Do
        'READTO'
        Iterate
    End /* If .. Do */

    /* Recover some response (ACK or NAK) */
    Parse Value uftcwack() With ac as
    If ac ^= 0 Then Do
        'OUTPUT' as
        rc = ac
        If Left(as,1) ^= "4" Then Leave          /* RC 4 is non fatal */
    End /* If .. Do */

    'READTO'
    If rc ^= 0 Then Leave

End /* Do Forever */

If rc ^= 0 Then Exit rc
'READTO'
If rc ^= 0 Then Exit rc

/*                                                                    *
 * OK ... now we've sent the control records (the meta data).         *
 * If it was fully accepted (no NAKs), then send the body.            *
 *                                                                    */

/* apply canonization for transport */
Select
    When type = "V" & cc = "M" Then Do
        /* spool-to-spool (VM to VM) transfer */
        pipe = "BLOCK 65024 CMS"
    End /* When .. Do */
    When type = "A" | type = "T" Then Do
        /* text file; send as ASCII with 0x0A+0x0D */
        pipe = "MAKETEXT NETWORK | FBLOCK 65024"
    End /* When .. Do */
    When type = "E" Then Do
        /* text file; send as EBCDIC with 0x15 */
        pipe = "BLOCK 65024 TEXTFILE"
    End /* When .. Do */
    When type = "I" | type = "B" | type = "U" Then Do
        /* binary file (unstructured octet stream) */
        pipe = "FBLOCK 65024"
    End /* When .. Do */
    When type = "N" Then Do
        /* IBM NETDATA format (generated by upstream stages) */
        pipe = "FBLOCK 65024"
    End /* When .. Do */
    When type = "V" Then Do
        /* variable-length records */
        pipe = "BLOCK 65024 CMS"
    End /* When .. Do */
    Otherwise Do
        /* all others, treat as binary */
        pipe = "FBLOCK 65024"
    End /* Otherwise Do */
End /* Select */

/* apply canonization for transport */
'PEEKTO'
'ADDPIPE *.INPUT: |' pipe '| *.INPUT:'
If rc ^= 0 Then Exit rc

j = 0;  i = 0
/* Send the body of the file (the actual data) */
Do Forever

    'PEEKTO DATA'
    If rc ^= 0 Then Leave

    /* tell the server it's coming */
    Call PUTLINE "DATA" Length(data)

    If uft = 2 Then Do
        /* recover some response (ACK or NAK) */
        Parse Value uftcwack() With ac as
        If ac ^= 0 Then Do ; 'OUTPUT' as ; rc = ac ; Leave ; End
    End /* If .. Do */

    /* send the "burst" */
    'CALLPIPE VAR DATA | *.OUTPUT.TCP:'
    If rc ^= 0 Then Leave

    /* Recover some response (ACK or NAK) */
    Parse Value uftcwack() With ac as
    If ac ^= 0 Then Do ; 'OUTPUT' as ; rc = ac ; Leave ; End

    j = j + 1                               /* increment record count */
    i = i + Length(data)                      /* increment byte count */

    'READTO'
    If rc ^= 0 Then Leave

End /* Do Forever */

If rc ^= 0 & rc ^= 12 Then Exit rc

/* log this completion */
'OUTPUT' argl j "records sent;" i "bytes sent"

/* provide an explicit 8712 for clarity saying "the file got through" */
date = Date("S") || "-" || Time()
If user = "" Then user = u
Address "COMMAND" 'XMITMSG 8712 "*" USER HOST DATE' ,
                        '(APPLID UFT CALLER TCP VAR'
If rc = 0 Then 'OUTPUT' argl message.1

/* send an EOF command */
'OUTPUT' "EOF"
Call PUTLINE "EOF"
Call UFTCWACK                      /* ignoring the response condition */

/* send a QUIT command */
'OUTPUT' "QUIT"
Call PUTLINE "QUIT"
Call UFTCWACK                      /* ignoring the response condition */

/* -- EXIT ---------------------------------------------------------- */

Return

/* ------------------------------------------------------------- GETLINE
 *    Read a line of plain text from the server.
 */
GETLINE: Procedure Expose buffer

Do While Index(buffer,'0A'x) = 0 & Index(buffer,'00'x) = 0
    'CALLPIPE *.INPUT.TCP: | VAR TMPBUF'
    If rc ^= 0 Then Return ""
    buffer = buffer || tmpbuf
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
Do Forever
    line = getline()
    If line = "" Then Return 0 "ACK (NULL)"
    If Left(line,1) = '1' Then Iterate
    If Left(line,1) = '2' Then Return 0 line
    If Left(line,1) = '3' & uft = 2 Then Return 0 line
    If Left(line,1) = '4' Then Return 1 line
    If Left(line,1) = '5' Then Return 1 line
    If Left(line,1) = '+' Then Return 0 line               /* defunct */
    If Left(line,1) = '-' Then Return 1 line               /* defunct */
    End /* If .. Do */
Return 1 line

/* ------------------------------------------------------------- PUTLINE
 *    Write a line of plain text to the server.
 */
PUTLINE: Procedure Expose argl
Parse Arg line
'CALLPIPE VAR LINE | MAKETEXT NETWORK | VAR LINE | *.OUTPUT.TCP:'
If rc = 0 Then Return 0
Return rc

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


