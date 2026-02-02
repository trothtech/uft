/* © Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCTCP2 REXX
 *              Pipeline stage to feed a SIFT job to UFTD via TCP/IP
 *              This is a replacement for UFTCTCP REXX.
 *              The rationale for the re-write is
 *              to avoid mixing Rexx/Sockets with CMS Pipelines.
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-09-15
 *    See also: UFTCFILE, UFTCUSER, UFTCHOST, UFTXDSPL
 *    Revision: 2.0.16
 *
 *        Note: UFTCTCP2 is not a user-level pipeline stage.
 *
 *       Input: a SIFT job (a batch mode UFT transaction representation)
 *      Output: logging info prefixed with a tag indicating this stage
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argl = '*' || arg0 || ':'                              /* for logging */

/* parse command line arguments */
Parse Arg host port . '(' opts ')' .
If host = "" Then host = "localhost"
If POS('@',host) > 0 Then Parse Var host user '@' host
                     Else user = ""
Parse Var host host ':' port
If port = "" Then port = 608

/* extract the TCP/IP service VM ID from Rexx/Sockets                 */
Parse Value Socket("INITIALIZE","TCPCHECK") With rc rs
If rc = 0 Then Do
    Parse Var rs . . tcpid .
    Call Socket "TERMINATE", "TCPCHECK"
End ; Else tcpid = "TCPIP"
If tcpid ^= "" Then tcpid = "USER" tcpid

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
'ADDPIPE *.OUTPUT.TCP: | TCPCLIENT' hisaddr port tcpid '| *.INPUT.TCP:'
If rc ^= 0 Then Exit rc

/* load ASCII/EBCDIC translate tables                                 */
/* 'CALLPIPE < POSIX TCPXLBIN | STEM AEX.' ; If rc ^= 0 Then ...      */
    aex.2 = '00010203372D2E2F1605250B0C0D0E0F101112133C3D322618193F271C1D1E1F'x ,
         || '405A7F7B5B6C507D4D5D5C4E6B604B61F0F1F2F3F4F5F6F7F8F97A5E4C7E6E6F'x ,
         || '7CC1C2C3C4C5C6C7C8C9D1D2D3D4D5D6D7D8D9E2E3E4E5E6E7E8E9ADE0BD5F6D'x ,
         || '79818283848586878889919293949596979899A2A3A4A5A6A7A8A9C04FD0A107'x ,
         || '202122232415061728292A2B2C090A1B30311A333435360838393A3B04143EFF'x ,
         || '41AA4AB19FB26AB5BBB49A8AB0CAAFBC908FEAFABEA0B6B39DDA9B8BB7B8B9AB'x ,
         || '6465626663679E687471727378757677AC69EDEEEBEFECBF80FDFEFBFCBAAE59'x ,
         || '4445424643479C4854515253585556578C49CDCECBCFCCE170DDDEDBDC8D8EDF'x
    aex.3 = '000102039C09867F978D8E0B0C0D0E0F101112139D8508871819928F1C1D1E1F'x ,
         || '80818283840A171B88898A8B8C050607909116939495960498999A9B14159E1A'x ,
         || '20A0E2E4E0E1E3E5E7F1A22E3C282B7C26E9EAEBE8EDEEEFECDF21242A293B5E'x ,
         || '2D2FC2C4C0C1C3C5C7D1A62C255F3E3FF8C9CACBC8CDCECFCC603A2340273D22'x ,
         || 'D8616263646566676869ABBBF0FDFEB1B06A6B6C6D6E6F707172AABAE6B8C6A4'x ,
         || 'B57E737475767778797AA1BFD05BDEAEACA3A5B7A9A7B6BCBDBEDDA8AF5DB4D7'x ,
         || '7B414243444546474849ADF4F6F2F3F57D4A4B4C4D4E4F505152B9FBFCF9FAFF'x ,
         || '5CF7535455565758595AB2D4D6D2D3D530313233343536373839B3DBDCD9DA9F'x
aex.1 = XRANGE('00'x,'FF'x)     /* need this even if we have TCPXLBIN */

buffer = ""             /* initially empty input (from server) buffer */
u = "-"
qs = 65024
qs = 32256              /* recommended burst size */
grc = 0

/* read the herald from the server - standard for UFT since forever   */
/* a UFT/1 server should send a 1xx herald                            */
/* a UFT/2 server should send a 2xx herald                            */
line = getline()
If grc ^= 0 Then Exit grc
retry = 5 ; Do While line = "" & retry > 0
                     line = getline() ; retry = retry - 1 ; End
If line = "" Then Do
    Address "COMMAND" 'XMITMSG 44 (APPLID UFT CALLER TCP ERRMSG'
    Exit 1
End /* If .. Do */

/* does the server want UFT/1 or UFT/2? (and we hope for the latter)  */
If Left(line,1) = '2' Then uft = 2
                      Else uft = 1        /* really REALLY not likely */
'OUTPUT' line                                              /* logging */
'OUTPUT' argl "UFT level" uft                              /* logging */

type = "";  cc = ""
/* send the commands (parmeters; ie: the "meta" file) and controls    */
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then Leave

    /* echo the command from our input stage and then parse it        */
/*  'OUTPUT' line                                          ** logging */
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

    /* if user specified on command line then do NOT send from stream */
    If Abbrev("USER",cmnd,4) & ^meta & user ^= "" Then Do
        'OUTPUT' argl "USER statement overridden/ignored"  /* logging */
        'READTO'
        Iterate
    End /* If .. Do */

    /* but do collect USER commands if not specified on command line  */
    If Abbrev("USER",cmnd,4) & ^meta Then Parse Var line . u .

    /* watch the swizzle - FILE statement might signal USER statement */
    If Abbrev("FILE",cmnd,4) & ^meta & user ^= "" Then Do
        'OUTPUT' line                                      /* logging */
        Call PUTLINE line          /* go ahead and send FILE stmt now */
        Parse Value uftcwack() With ac as        /* and expect an ACK */
        If ac ^= 0 Then Do
            'OUTPUT' as ; rc = ac ; Leave ; End
        /* force next line sent to be the requisite USER statement    */
        line = "USER" user
        'OUTPUT' line
    End

    /* send a packet */
    If meta Then line = "META" line
    'OUTPUT' line                                          /* logging */
    Call PUTLINE line

    /* for certain commands (e.g., comments), DON'T wait for ACK      */
    If Left(line,1) = '*' | Left(line,1) = '#' Then Do
        'READTO' ; Iterate ; End

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
        pipe = "BLOCK" qs "CMS"
    End /* When .. Do */
    When type = "A" | type = "T" Then Do
        /* text file; send as ASCII with 0x0A+0x0D */
        pipe = "MAKETEXT NETWORK | FBLOCK" qs
    End /* When .. Do */
    When type = "E" Then Do
        /* text file; send as EBCDIC with 0x15 */
        pipe = "BLOCK" qs "TEXTFILE"
    End /* When .. Do */
    When type = "I" | type = "B" | type = "U" Then Do
        /* binary file (unstructured octet stream) */
        pipe = "FBLOCK" qs
    End /* When .. Do */
    When type = "N" Then Do
        /* IBM NETDATA format (generated by upstream stages) */
        pipe = "FBLOCK" qs
    End /* When .. Do */
    When type = "V" Then Do
        /* variable-length records */
        pipe = "BLOCK" qs "CMS"
    End /* When .. Do */
    Otherwise Do
        /* all others, treat as binary */
        pipe = "FBLOCK" qs
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
    'OUTPUT' "DATA" Length(data)                           /* logging */
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
'OUTPUT' argl j "records sent;" i "bytes sent"             /* logging */

/* provide an explicit 8712 for clarity saying "the file got through" */
date = Date("S") || "-" || Time()
If user = "" Then user = u
Address "COMMAND" 'XMITMSG 8712 "*" USER HOST DATE' ,
                        '(APPLID UFT CALLER TCP VAR'
If rc = 0 Then 'OUTPUT' argl message.1                     /* logging */

/* send an EOF command */
'OUTPUT' "EOF"                                             /* logging */
Call PUTLINE "EOF"
Call UFTCWACK                      /* ignoring the response condition */

/* send a QUIT command */
'OUTPUT' "QUIT"                                            /* logging */
Call PUTLINE "QUIT"
Call UFTCWACK                      /* ignoring the response condition */

/* -- EXIT ---------------------------------------------------------- */

Return

/* ------------------------------------------------------------- GETLINE
 *    Read a line of plain text from the UFT server.
 *        Note: This routine switches input streams and presumes
 *              that all other 'PEEKTO' and 'READTO' want stream 0
 *              so it explicitly switches back to stream 0.
 */
GETLINE: Procedure Expose buffer uft aex. grc

'SELECT INPUT TCP'

retry = 5
Do While POS('0A'x,buffer) = 0 & POS('00'x,buffer) = 0 ,
                               & retry > 0

    'PEEKTO RECORD'
    If rc = 0 Then Do ; buffer = buffer || record ; 'READTO' ; End
              Else Do ; grc = rc ; Return "" ; End
    retry = retry - 1

End /* Do While */

Parse Var buffer line '0A'x buffer
Parse Var line line '0D'x .
Parse Var line line '00'x .

'SELECT INPUT 0'

Return Translate(line,aex.2,aex.1)

/* ------------------------------------------------------------ UFTCWACK
 *    Wait for ACK.
 *  Response codes starting with '2' are ACK, '1' are informational
 *  (thus, iterate), '5' and '4' error, '3' means "send more".
 */
UFTCWACK: Procedure Expose buffer uft aex. grc
Do Forever
    line = getline() ; If grc ^= 0 Then Return 1
    If line = "" Then line = getline() ; If grc ^= 0 Then Return 1
/*  If line = "" Then Return 0 "ACK (NULL)"                           */
    If line = "" Then Return 1 "ACK (NULL)"
    If Left(line,1) = '1' Then Iterate
    If Left(line,1) = '2' Then Return 0 line
    If Left(line,1) = '3' & uft = 2 Then Return 0 line
    If Left(line,1) = '4' Then Return 1 line
    If Left(line,1) = '5' Then Return 1 line
    If Left(line,1) = '6' Then Iterate    /* FIXME: need better logic */
    If Left(line,1) = '+' Then Return 1 line               /* defunct */
    If Left(line,1) = '-' Then Return 1 line               /* defunct */
    End /* If .. Do */
Return 1 line

/* ------------------------------------------------------------- PUTLINE
 *    Write a line of plain text to the UFT server.
 */
PUTLINE: Procedure Expose argl aex.
Parse Arg line
line = Translate(line,aex.3,aex.1) || '0D0A'x
'CALLPIPE VAR LINE | *.OUTPUT.TCP:'
If rc = 0 Then Return 0
Return rc

/* ------------------------------------------------------------ HOSTADDR
 *    return the IP address for of supplied hostname
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
 *    return the internet hostname for the given IP address
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


