/* © Copyright 1995, 2024, Richard M. Troth, all rights reserved. <plaintext>
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
 *        Note: This implementation does not accept a burst-size
 *              parameter on the DATA command. It should.
 *
 *        Note: Unlike some other UFTCMAIN followers,
 *              this stage does not fill-in a USER command.
 *              If the source stream contains no USER command,
 *              then the transaction will fail.
 */

/*  identify this stage to itself  */
Parse Source . . arg0 .
argl = '*' || arg0 || ':'

/*  parse command line arguments  */
Parse Arg host port . '(' . ')' .
If host = "" Then host = "localhost"
If Index(host,'@') > 0 Then Parse Var host . '@' host
Parse Var host host ':' port
If port = "" Then port = 608
temp = "UFT" || Right(Time('S'),5,'0')
buffer = ""

/*  Verify REXX/Sockets Version 2  */
Parse Value Socket("VERSION") With rc . ver .
If ver < 2 Then /* Exit -1 */ Exit 0

/*  Initialize RXSOCKET  */
Parse Value Socket('Initialize', temp) With rc rs
If rc ^= 0 Then Do
    Parse Var rs errno etext
    If errno ^= "ESUBTASKALREADYACTIVE" Then Do
        tcprc = rc
        'OUTPUT' argl rs
        /* Exit tcprc */ Exit 0
        End  /*  If .. Do  */
    Call Socket 'Terminate', temp
    Parse Value Socket('Initialize', temp) With rc rs
    If rc ^= 0 Then Do
        tcprc = rc
        'OUTPUT' argl rs
        /* Exit tcprc */ Exit 0
        End  /*  If .. Do  */
    End  /*  If .. Do  */

/*  Get a socket descriptor (TCP protocol)  */
Parse Value Socket('Socket', 'AF_INET', 'Sock_Stream') With rc rs     .
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    /* Exit tcprc */ Exit 0
    End  /*  If .. Do  */
Parse Var rs s .

/*  Figure out the target host address  */
Parse Value Socket('Resolve', host) With rc rs
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    /* Exit tcprc */ Exit 0
    End  /*  If .. Do  */
Parse Var rs hisaddr hisname .

/*  Connect to the UFT server  */
Parse Value Socket('Connect', s, 'AF_INET' port hisaddr) With rc rs
If rc ^= 0 Then Do
    Parse Var rs errno etext
    If errno ^= "EINPROGRESS" Then Do
        tcprc = rc
        'OUTPUT' argl rs
        /* Exit tcprc * (tcprc ^= 61) */ Exit 0
        End  /*  If .. Do  */
    End  /*  If .. Do  */

/*  Read the herald from the server.  */
line = getline(s)
/*  UFT/1 or UFT/2?  */
If Left(line,1) = '2' Then uft = 2
                      Else uft = 1
'OUTPUT' line
'OUTPUT' argl "UFT =" uft

type = "";  cc = ""
/*  Send the commands (parmeters; ie: the "meta" file)  */
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then Leave

    /*  Echo the command from our input stage and parse it.  */
    'OUTPUT' line
    Parse Upper Var line cmnd .

    /*  watch for DATA,  which breaks this loop  */
    If Abbrev("DATA",cmnd,4) Then Do
        rc = 0
        Leave
        End  /*  If .. Do  */
    If Abbrev("TYPE",cmnd,4) Then Parse Upper Var line . type cc .

    /*  Send a packet  */
    Call PUTLINE s, line

    /*  for some commands,  DON'T wait for ACK  */
    If Left(line,1) = '*' | Left(line,1) = '#' Then Do
        'READTO'
        Iterate
        End  /*  If .. Do  */

    /*  Recover some response  (ACK or NAK)  */
    Parse Value uftcwack(s) With ac as
    If ac ^= 0 Then Do
        'OUTPUT' as
        rc = ac
        If Left(as,1) ^= "4" Then Leave
        End  /*  If .. Do  */

    'READTO'

    End  /*  Do  While  */

If rc ^= 0 Then Exit rc
'READTO'
If rc ^= 0 Then Exit rc

/*                                                                    *
 *  OK ... now we've sent the control records (meta data).            *
 *  If it was fully accepted (no NAKs), then send the body.           *
 *                                                                    */

/*  apply canonicalization for transport  */
Select
    When type = "V" & cc = "M" Then Do
        /*  spool-to-spool (VM to VM) transfer  */
        pipe = "BLOCK 61440 CMS"
        End  /*  When .. Do  */
    When type = "A" | type = "T" Then Do
        /*  text file; send as ASCII  */
        pipe = "MAKETEXT NETWORK | FBLOCK 61440"
        End  /*  When .. Do  */
    When type = "E" Then Do
        /*  text file; send as EBCDIC  */
        pipe = "BLOCK 61440 TEXTFILE"
        End  /*  When .. Do  */
    When type = "I" | type = "B" | type = "U" Then Do
        /*  binary file (unstructured octet stream)  */
        pipe = "FBLOCK 61440"
        End  /*  When .. Do  */
    When type = "N" Then Do
        /*  IBM NETDATA format  */
        pipe = "FBLOCK 61440"
        End  /*  When .. Do  */
    When type = "V" Then Do
        /*  variable-length records  */
        pipe = "BLOCK 61440 CMS"
        End  /*  When .. Do  */
    Otherwise Do
        /*  all others, treat as binary  */
        pipe = "FBLOCK 61440"
        End  /*  Otherwise Do  */
    End  /*  Select  */

/*  apply canonicalization for transport  */
'ADDPIPE *.INPUT: |' pipe '| *.INPUT:'
If rc ^= 0 Then Exit rc

j = 0;  i = 0
/*  Send the body of the file  (data)  */
Do Forever

    'PEEKTO DATA'
    If rc ^= 0 Then Leave

    /*  tell the server it's coming  */
    Call PUTLINE s, "DATA" Length(data)

    If uft = 2 Then Do
        /*  Recover some response  (ACK or NAK)  */
        Parse Value uftcwack(s) With ac as
        If ac ^= 0 Then Do
            'OUTPUT' as
            rc = ac
            Leave
            End  /*  If .. Do  */
        End  /*  If .. Do  */

    /*  Send the "burst"  */
    Parse Value Socket('Write', s, data) With rc rs
    If rc ^= 0 Then Do
        Say rs
        Leave
        End  /*  If .. Do  */

    /*  Recover some response  (ACK or NAK)  */
    Parse Value uftcwack(s) With ac as
    If ac ^= 0 Then Do
        'OUTPUT' as
        rc = ac
        Leave
        End  /*  If .. Do  */

    j = j + 1
    i = i + Length(data)

    'READTO'

    End  /*  Do  Forever  */

If rc ^= 0 & rc ^= 12 Then Exit rc

/*  log this completion  */
'OUTPUT' argl j "records sent;" i "bytes sent"

/*  Send an EOF command  */
'OUTPUT' "EOF"
Call PUTLINE s, "EOF"
Call UFTCWACK s                 /*  ignoring the response  */

/*  Send a QUIT command  */
'OUTPUT' "QUIT"
Call PUTLINE s, "QUIT"
Call UFTCWACK s                 /*  ignoring the response  */

/* -- EXIT ---------------------------------------------------------- */

/*  All done, relinquish our socket descriptor  */
Parse Value Socket("CLOSE",s) With rc rs
If rc ^= 0 Then Do
    tcprc = rc
    'OUTPUT' argl rs
    Exit tcprc
    End

/*  Tell RXSOCKET that we are done with this IUCV path  */
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
    End  /*  Do While  */
Parse Var buffer line '0A'x buffer
Parse Var line line '0D'x .
Parse Var line line '00'x .
'CALLPIPE VAR LINE | SPEC 1-* 1 /0D0A/ X2C NEXT' ,
    '| MAKETEXT LOCAL | VAR LINE'
Return line

/* ------------------------------------------------------------ UFTCWACK
 *  Response codes starting with '2' are ACK,  '1' informational
 *  (thus, iterate),  '5' and '4' error,  '3' meaning "send more".
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
    If Left(line,1) = '+' Then Return 0 line    /*  defunct  */
    If Left(line,1) = '-' Then Return 1 line    /*  defunct  */
    End  /*  If .. Do  */
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































/*
 *        Name: UFTCRSCS REXX
 *              Pipeline stage to hand-off a SIFT/UFT job to RSCS
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *        Date: 1992-Oct-06, 1993-Feb-22
 *
 *        Note: This stage presently sends all files in binary (PUNCH).
 *              This is *not* how things should work in the long run.
 */

