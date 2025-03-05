/* © Copyright 1993-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: TCPSHELL REXX
 *              General purpose TCP/IP socket-to-pipeline interface
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1993-Feb-29..1995-Jun-29
 *
 *     Co-reqs:
 *              CMS Pipelines
 *              REXX/Sockets (RXSOCKET V2)
 *              VM TCP/IP V2
 *
 *    GlobalVs: ARG0 (group TCPSHELL) names the group
 *              SERVER_NAME - FQDN of this server's host system
 *              SERVER_PORT - TCP port at which this server listens
 *
 *   On Launch: REMOTE_HOST - FQDN of client's system (once connected)
 *              REMOTE_ADDR - dotted decimal IP address of client
 *              REMOTE_PORT - TCP port of client's connection
 *
 *     Unloved: SERVER_PIPE_ATTACH - which means of attaching sub-pipes
 *              LOCALHOST
 *              LOCALPORT
 *              HOSTNAME
 *              HOSTADDR
 *              REMOTE_IDENT
 *              REMOTE_SYSTEM
 *              REMOTE_USER
 *              CLIENT
 */

verbose = 1;  binary = 0;  ident = 0;  nslookup = 1
wait = 5

Parse Source . . arg0 .
argo = arg0 || ':'   /* arg0 is about to change to match the service! */
argp = arg0

/*  we need a name and a port  */
Parse Arg test
If Left(Strip(test),1) = '(' Then Do
    Parse Arg '(' opts ')' localport arg0 args
    opt0 = ""
    End /* If .. Do */
/* still have to support old syntax */
Else Do
    Parse Upper Arg localport arg0 args '(' opt0 ')' .
    opts = ""
    End /* Else Do */

If localport = "" Then Do
    Address COMMAND 'XMITMSG 386 (ERRMSG'
    Exit 24
    End  /*  If  ..  Do  */

If ^Datatype(localport,'N') Then Do
    Address COMMAND 'XMITMSG 70 LOCALPORT (ERRMSG'
    Address COMMAND 'XMITMSG 8205 (ERRMSG'
    Exit 24
    End  /*  If  ..  Do  */

If arg0 = "" Then Do
    Address COMMAND 'XMITMSG 386 (ERRMSG'
    Exit 24
    End  /*  If  ..  Do  */

/* deprecated options processing */
If opts = "" Then Do
    If opt0 ^= "" Then args = args '('
    Do While opt0 ^= ""
        Parse Var opt0 op opt0
        Select /* op */
            When Abbrev("BINARY",op,3) Then binary = 1
            Otherwise args = args op
            End /* Select op */
        End  /* Do While */
    End /* If .. Do */

add_call = "CALLPIPE"
dynamic = ""
/* preferred options processing */
Do While opts ^= ""
    Parse Upper Var opts op opts
    Select /* op */
        When  Abbrev("BINARY",op,3)     Then binary = 1
        When  Abbrev("NOBINARY",op,3)   Then binary = 0
        When  Abbrev("TEXT",op,1)       Then binary = 0
        When  Abbrev("ASCII",op,1)      Then binary = 0
        When  Abbrev("IDENT",op,2)      Then ident = 1
        When  Abbrev("NOIDENT",op,3)    Then ident = 0
        When  Abbrev("VERBOSE",op,1)    Then verbose = 1
        When  Abbrev("NOVERBOSE",op,3)  Then verbose = 0
        When  Abbrev("TERSE",op,5)      Then verbose = 0
        When  Abbrev("NSLOOKUP",op,2)   Then nslookup = 1
        When  Abbrev("DNS",op,3)        Then nslookup = 1
        When  Abbrev("NONSLOOKUP",op,3) Then nslookup = 0
        When  Abbrev("NODNS",op,3)      Then nslookup = 0
        When  Abbrev("SINGLE",op,1)     Then add_call = "CALLPIPE"
        When  Abbrev("MULTIPLE",op,1)   Then add_call = "ADDPIPE"
        When  Abbrev("NOMULTIPLE",op,3) Then add_call = "CALLPIPE"
        When  Abbrev("DYNAMIC",op,3)    Then ,
            Parse Upper Var opts dynamic opts
        Otherwise Address COMMAND 'XMITMSG 3 OP (ERRMSG'
        End /* Select op */
    End /* Do While */

/*  a restart timestamp if running verbose  */
If verbose Then Do
    time = Date('S') Time()
    Address COMMAND 'XMITMSG 2323 ARG0 TIME (ERRMSG'
    End  /*  If .. Do  */

/*  note what we're running as  (what service)  */
Address COMMAND 'GLOBALV SELECT TCPSHELL PUT ARG0'
/*  and then make that the default GLOBALV group  */
Address COMMAND 'GLOBALV SELECT' arg0

/*  must have REXX/Sockets version 2 or above  */
Parse Value Socket("VERSION") With rc . ver .
If ver < 2 Then Do
    Say argo arg0 "server requires REXX/Sockets (RXSOCKET version 2)"
    Address "COMMAND" 'XMITMSG 1440 (ERRMSG'
    Address "COMMAND" 'XMITMSG 3958 (ERRMSG'
    Exit -1
    End  /*  If  ..  Do  */

/*  initialize a socket set  */
Parse Value Socket("INITIALIZE",arg0) With rc rs
If rc ^= 0 Then Do
    Say argo rs
    Exit rc
    End  /*  If  ..  Do  */
Parse Var rs . maxdesc svm .
If verbose Then Say argo "MAXDESC" maxdesc
If verbose Then Say argo "SVM" svm

/*  get FQDN of this host  */
Parse Value Socket("GETDOMAINNAME") With rc rs
If rc ^= 0 Then Do
    Say argo rs
    Call Socket "TERMINATE", arg0
    Exit rc
    End  /*  If  ..  Do  */
Parse Var rs dn .
Parse Value Socket("GETHOSTNAME") With rc rs
If rc ^= 0 Then Do
    Say argo rs
    Call Socket "TERMINATE", arg0
    Exit rc
    End  /*  If  ..  Do  */
Parse Var rs hn .
localhost = hn || '.' || dn
'CALLPIPE VAR LOCALHOST | XLATE LOWER | VAR LOCALHOST'

/*  stuff local hostname and port number omtp GlobalV storage  */
Address COMMAND 'GLOBALV SELECT' arg0 'PUT LOCALHOST LOCALPORT'

/*  alsu supply CGI form of these variables  */
'CALLPIPE VAR LOCALHOST | XLATE LOWER | VAR SERVER_NAME'
server_port = localport
Address COMMAND 'GLOBALV SELECT' arg0 'PUT SERVER_NAME SERVER_PORT'
If verbose Then Say argo "SERVER_NAME" server_name
/* If verbose Then */ Say argo "SERVER_PORT" server_port

/*  new with the multi-threaded version  */
server_pipe_attach = add_call
Address COMMAND 'GLOBALV SELECT' arg0 'PUT SERVER_PIPE_ATTACH'

If verbose Then Do
    Address COMMAND 'XMITMSG 8201 ARG0 (ERRMSG'
    Address COMMAND 'XMITMSG 740 (ERRMSG'
    End  /*  If .. Do  */

/*  attach CONSOLE stage if no input  */
'STREAMSTATE INPUT'
If rc = 12 Then
'ADDPIPE CONSOLE ASYNC | *.INPUT:'
If rc ^= 0 Then Exit rc

/*  prefix console input with "CONSOLE"  */
'ADDPIPE *.INPUT: | SPEC /CONSOLE / 1 1-* N | *.INPUT:'
If rc ^= 0 Then Exit rc

/*  attach TCPLISTEN stage to input  */
'ADDPIPE (END !) TCPLISTEN' localport 'USERID' svm ,
    '| A: FANINANY | *.INPUT:' ,
    '! *.INPUT: | A:'
If rc ^= 0 Then Exit rc

/*  calculate a wait value  */
wait = Strip(wait)
If Left(wait,1) ^= "+" Then wait = "+" || wait

/* pipe = arg0 args '(' opt0 */
pipe = arg0 args
If ^binary Then ,
    pipe = 'MAKETEXT LOCAL | UNTAB -8 |' pipe '| MAKETEXT NETWORK'

/*  set a "ready prompt"  */
argp = argp || '/' || arg0
Say argp "Ready;"

/*  loop on input records  */
Do Forever

    'PEEKTO RECORD'
    If rc ^= 0 Then Leave
    prefix = Left(record,8)

    Select  /*  prefix  */

        When prefix = "CONSOLE " Then Do
            Parse Upper Var record +8 verb .
            If Abbrev("STOP",verb,4) Then Leave
            If Abbrev("QUIT",verb,4) Then Leave
            If Abbrev("EXIT",verb,4) Then Leave
            If Abbrev("SHUTDOWN",verb,4) Then Leave
            Parse Value CONSOLE(record) With rc rs
            If rs ^= "" Then Say argo rs
            Select  /*  rc  */
                When rc = 0 Then Say argp "Ready;"
                When rc = -3 Then ,
                    Address "COMMAND" 'XMITMSG 15 ARG0 (ERRMSG'
                Otherwise Say argp "Ready(" || rc || ");"
                End  /*  Select rc  */
            End  /*  When .. Do  */

        When prefix = "pipetcp " Then Do
            Parse Value PIPETCP(record) With rc rs
            If rs ^= "" Then Say argo rs
            If rc ^= 0 Then Leave
            'CALLPIPE LITERAL' wait '| DELAY | HOLE'
            End  /*  When .. Do  */

        End  /*  Select prefix  */

    'READTO'
    If rc ^= 0 Then Leave

    End

If verbose Then Do
    time = Date('S') Time()
    Address COMMAND 'XMITMSG 2324 ARG0 TIME (ERRMSG'
    End  /*  If .. Do  */

/*  terminate the socket set  */
Parse Value Socket("TERMINATE",arg0) With rc rs
If rc ^= 0 Then Do
    Say argo rs
    Exit rc
    End  /*  If  ..  Do  */

Exit

/* ------------------------------------------------------------ TAPIDENT
 *
 *        Name: TAPIDENT (REXX function)
 *              Return "ownership" information about a socket.
 *    Requires: REXX/Sockets
 *        Date: 1994-Feb-01, 03
 *
 *        Note: TCP/IDENT protocol was always controversial
 *              and has been deprecated for many years.
 *              Its use here is disabled by default.
 */
TAPIDENT: Procedure

Return 0 ""

iport = 113
Parse Arg s , z

/*
 *  Presume that RXSOCKET version 1 doesn't exist anymore.
 */
Parse Value Socket('Version') With rc rs
Parse Var rs name version date .
If version < 2 Then Return -1 rs

/*  the following are for REXX/Sockets,  not Pipelines  */
If z = "" Then Do

    Parse Value Socket('GetPeerName',s) With rc rs
    If rc ^= 0 Then Return rc rs
    Parse Var rs fafam fport fhost .

    Parse Value Socket('GetSockName',s) With rc rs
    If rc ^= 0 Then Return rc rs
    Parse Var rs lafam lport lhost .

    End  /*  If .. Do  */

Else Parse Var z fport fhost lport lhost .

/*
 *  Safe bet that REXX/Sockets has already been initialized.
 */

/*
 *  Request a new socket descriptor (TCP protocol)
 */
Parse Value Socket('Socket','AF_INET','Sock_Stream') With rc rs
If rc ^= 0 Then Return rc rs
Parse Var rs t .

/*
 *  Set this socket to translate ASCII <---> EBCDIC.
 */
Parse Value Socket("SETSOCKOPT",t,"SOL_SOCKET","SO_ASCII","ON") ,
    With rc rs
If rc ^= 0 Then Do
    Call Socket 'Close', t
    Return rc rs
    End  /*  If  ..  Do  */

/*  And build a "name" structure.  */
name = 'AF_INET' iport fhost

/*
 *  Connect to the TAP/IDENT server there.
 */
Parse Value Socket('Connect',t,name) With rc rs
If rc ^= 0 Then Do
    Call Socket 'Close', t
    Return rc rs
    End  /*  If  ..  Do  */

/*
    send:
    LPORT , FPORT <CR><LF>
 */
data = fport ',' lport '0D25'x
Parse Value Socket("WRITE",t,data) With rc rs
If rc ^= 0 Then Do
    Call Socket 'Close', t
    Return rc rs
    End  /*  If  ..  Do  */

/*
 *  Read the response from the TAP/IDENT server.
 */
Parse Value Socket("READ",t,61440) With rc rs
If rc ^= 0 Then Do
    Call Socket 'Close', t
    Return rc rs
    End  /*  If  ..  Do  */
Parse Var rs bc data
Parse Var data data '25'x .
Parse Var data data '0D'x .

/*
 *  All done, relinquish our socket descriptor.
 */
Call Socket 'Close', t

Return 0 data

/* ------------------------------------------------------------- PIPETCP
 *  process incoming TCP connections
 */
PIPETCP: Procedure Expose arg0 argo pipe nslookup verbose ident ,
                        server_port add_call dynamic

Parse Arg record
/*
Parse Var record +16 stuff +40
Parse Var stuff +4 stuff
Say argo "..." c2x(stuff)
 */

Parse Var record +64 ipv4 +16 .
Parse Var ipv4 1 _c_afam +2 _c_port +2 _c_addr +4 .
Parse Var _c_addr _h1 +1 _h2 +1 _h3 +1 _h4 +1

/*  identify the remote in the environment  */
hostname = ""; hostaddr = c2d(_h1) || "." ,
    || c2d(_h2) || "." || c2d(_h3) || "." || c2d(_h4)
If nslookup Then Do
    Parse Value Socket("RESOLVE",hostaddr) With rc rs
    If rc = 0 Then Parse Var rs hostaddr hostname .
    End  /*  If .. Do  */
Address COMMAND 'GLOBALV SELECT' arg0 'PUT HOSTNAME HOSTADDR'

/*  preferred variables;  CGI compatible  */
remote_addr = hostaddr
If verbose Then Say argo "REMOTE_ADDR" remote_addr
'CALLPIPE VAR HOSTNAME | XLATE LOWER | VAR REMOTE_HOST'
If verbose Then Say argo "REMOTE_HOST" remote_host
remote_port = c2d(_c_port)
If verbose Then Say argo "REMOTE_PORT" remote_port
Address COMMAND 'GLOBALV SELECT' arg0 ,
    'PUT REMOTE_ADDR REMOTE_HOST REMOTE_PORT'

/*  Per RFC 1413, try to identify the user on the other end.  */
system = "";  userid = ""
If ident Then Do
    Parse Var record +56 sock +4 .
    sock = c2d(sock)
/*  Parse Value TAPIDENT(sock) With rc rs  */
    Parse Value TAPIDENT(, remote_port remote_addr server_port ) ,
        With rc rs
    If rc = 0 Then Do
        Parse Var rs ':' code ':' user
        Upper code
        If code = "USERID" Then Do
            Parse Var user system ':' userid
            system = Strip(system);  userid = Strip(userid)
            End  /*  If .. Do  */
        End  /*  If .. Do  */
    End  /*  If .. Do  */
REMOTE_IDENT = userid
REMOTE_USER = ""
REMOTE_SYSTEM = system
Address COMMAND 'GLOBALV SELECT' arg0 ,
    'PUT REMOTE_IDENT REMOTE_SYSTEM REMOTE_USER'
If verbose Then Say argo "REMOTE_USER" remote_user remote_ident

If hostname = "" Then client = userid || '@' || host
                 Else client = userid || '@' || hostname
Address COMMAND 'GLOBALV SELECT' arg0 'PUT CLIENT'
If verbose Then Say argo "Accepted" sock "at" Time() "client" client

/* clear this, just in case */
Address COMMAND 'GLOBALV SELECT' arg0 'SET QUIT'

If 0 Then Do
/* refresh disk access (same procedure as used by GONE EXEC) */
'CALLPIPE COMMAND QUERY DISK | DROP | STEM STEM.'
Do i = 1 to stem.0
    Parse Var stem.i . 8 va 12 fm .
    If Left(va,3) = "DIR" Then Iterate
    Address COMMAND 'DISKWRIT' Left(fm,1)
    If rc = 1 Then Address COMMAND 'ACCESS' va fm
    End  /*  Do  For  */
End /* bypass */

/* Here is a hack to promote multi-stream operation.                  *
 * Global variables are not the best way to pass info to the          *
 * sub-stage when it runs concurrently with other sub-stages.         *
 * So instead, pass dynamic variables via the command line.           *
 * Works for both CALLPIPE and (especially) ADDPIPE.                  */
Select /* dynamic */
    When dynamic = "VARS" Then Do
        vars = ""
        If remote_addr ^= "" Then vars = vars "REMOTE_ADDR=" || remote_addr
        If remote_host ^= "" Then vars = vars "REMOTE_HOST=" || remote_host
        If remote_port ^= "" Then vars = vars "REMOTE_PORT=" || remote_port
    End /* When .. Do */
    When dynamic = "OPTS" Then Do
        vars = "("
        If remote_addr ^= "" Then vars = vars "REMOTE_ADDR" remote_addr
        If remote_host ^= "" Then vars = vars "REMOTE_HOST" remote_host
        If remote_port ^= "" Then vars = vars "REMOTE_PORT" remote_port
    End /* When .. Do */
    Otherwise vars = ""
End /* Select */

/*  Now here is where we do all of the real work.                     *
 *  When the gem goes to end-of-file,  then the pipeline              *
 *  terminates and the socket is closed for another iteration.        */
add_call 'LITERAL' c2x(record) '| STRIP | SPEC 1-* X2C 1' ,
    '| LOOP: FANIN | TCPDATA | ELASTIC |' pipe vars '| LOOP:'
If rc ^= 0 Then Return rc
If add_call = "ADDPIPE" Then 'CALLPIPE LITERAL +3 | DELAY'
Return rc

/* ------------------------------------------------------------- CONSOLE
 *  process virtual machine console input
 */
CONSOLE: Procedure
Parse Arg record
Parse Upper Var record +8 record
Address "CMS" record
Return rc


