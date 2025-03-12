/* © Copyright 1993-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTD REXX
 *              Universal File Transfer server pipeline stage
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas, USA
 *        Date: 1993-Apr-20, Jun-27, 1995-Jan-25
 *
 *        Note: recommend you run this in v-machine UFTD with class B.
 *
 *       Calls: GETFMADR, GLOBALV, XMITMSG, ADDPIPE and CALLPIPE
 *
 *   Variables: vrm0, arg0, argo,
 *              date, time, tz,
 *    GlobalVs: vrm, uft, verbose, localhost, hostid,
 *              server_pipe_attach
 *  Dynam Vars: remote_host, remote_addr, remote_port, remote_user,
 *   Temp Vars: rc, args, opts, vars, qrc, line, meta, code,
 *              combo, rh, op, addr, count, i, line.
 *  SPOOL Vars: form, dest, dist, copy, class, fcb, ucs,
 *              hold, keep, msg, seq,
 *    UFT Vars: user, name, type, auth, lrecl, recfm,
 *              open, dev, cc,
 */

/*  set some initial values  */
vrm0 = "1.10.6"                 /* to coincide with the POSIX version */

/*  identify this stage  */
Parse Source . . arg0 .
argo = arg0 || ':'

/*  fetch environment variables from TCPSHELL  */
Address COMMAND 'GLOBALV SELECT' arg0 'GET HOSTID VRM VERBOSE' ,
    'REMOTE_HOST REMOTE_ADDR REMOTE_USER REMOTE_IDENT' ,
        'UFT LOCALHOST SERVER_PIPE_ATTACH'
If vrm ^= vrm0 Then ,
    Address COMMAND 'XMITMSG 1200 VRM0 VRM (APPLID UFT ERRMSG'
If remote_host = "" Then remote_host = remote_addr
If remote_user = "" Then remote_user = remote_ident

/* the value of uft must be 1 or 2 ... preferably 2 */
If ^Datatype(uft,'N') Then uft = 1

/* do some args/opts parsing to get "dynamic" environmental values    */
Parse Upper Arg args "(" opts ")" vars

/* convert var=val pairs from args/vars to blank-delimited opts pairs */
'CALLPIPE VAR ARGS | PAD 1 | SPLIT | LOCATE /=/ | STEM WORK.'
'CALLPIPE VAR VARS | PAD 1 | SPLIT | LOCATE /=/ | STEM WORK. APPEND'
'CALLPIPE STEM WORK. | CHANGE /=/ / | JOIN * / /' ,
      '| APPEND STRLITERAL // | VAR COMBO'

/* let CMS style options take precedence over modified var=val pairs  */
opts = combo opts

/* and process those critters */
Do While opts ^= ""
    Parse Upper Var opts op opts
    Select /* op */
        When op = "REMOTE_HOST" Then ,
            Parse Upper Var opts remote_host opts
        When op = "REMOTE_ADDR" Then ,
            Parse Upper Var opts remote_addr opts
        When op = "REMOTE_PORT" Then ,
            Parse Upper Var opts remote_port opts
        When op = "REMOTE_USER" Then ,
            Parse Upper Var opts remote_user opts
        Otherwise nop
    End /* Select op */
End /* Do While */

/* fixup remote hostname from memory (from GlobalV) if possible */
rh = _hostname(remote_addr)
If rh ^= "" Then remote_host = rh

Say argo "FROM" remote_user || '@' || remote_host

Select /* verbose */
    When verbose = "ON"       Then verbose = 1
    When verbose = "OFF"      Then verbose = 0
    When verbose = "YES"      Then verbose = 1
    When verbose = "NO"       Then verbose = 0
    When verbose = "VERBOSE"  Then verbose = 1
    When verbose = "NOVERBOSE" Then verbose = 0
    Otherwise nop
    End  /*  Select verbose  */
If ^Datatype(verbose,'W') Then verbose = 1
verbose = (verbose ^= 0)

/*  attach a text translator output stream  */
'ADDPIPE *.OUTPUT: | MAKETEXT -NETWORK | *.OUTPUT:'
If rc ^= 0 Then Exit rc
If verbose Then ,
'ADDPIPE *.OUTPUT: | CONSOLE | *.OUTPUT:'

/*  attach a text translator loop for input  */
'ADDSTREAM BOTH LINE'
If rc ^= 0 Then Exit rc
'ADDPIPE *.OUTPUT.LINE: | SPEC 1-* 1' ,
    '/0D0A/ X2C NEXT | MAKETEXT -LOCAL | ELASTIC | *.INPUT.LINE:'
If rc ^= 0 Then Exit rc
If verbose Then ,
'ADDPIPE *.INPUT.LINE: | CONSOLE | *.INPUT.LINE:'

/*  find a free virtual address  */
Address CMS 'GETFMADR 200'
If rc ^= 0 Then Exit rc
Parse Pull . . addr .

/*  send a "hello" to our client  */
If uft > 1 Then ,
'CALLPIPE COMMAND XMITMSG 222 "' || localhost || '" "' || uft || '"' ,
    '"' || vrm || '" (APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
Else ,
'CALLPIPE COMMAND XMITMSG 112 "' || localhost || '" "' || uft || '"' ,
    '"' || vrm || '" (APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'

/*  set a number of defaults  */
user = "SYSTEM"
name = ""
type = 'I'      /*  image (binary) is safest for unspecified types  */
cc = ""
Parse Value Diag(08,'QUERY TIME') With . . time tz . date . '15'x .

form = "STANDARD";  dest = "OFF"
dist = "LOCAL";     copy = 1;       class = 'A'

hold = "OFF";   fcb  = "OFF";   ucs  = "OFF"
keep = "OFF";   msg  = "OFF";   seq  = "*UFT"

dev = ""
auth = ""

/*  initialize the data buffer  */
'PEEKTO BUFFER'
i = 0; open = 0
size = 0; from = ""; auth = ""; user = ""

/*  loop forever,  breaking out when needed  */
Do Forever

    line = getline()

    If rc ^= 0 Then Do
        qrc = rc
        If open Then Call ABORT
        rc = qrc
        Leave
        End  /*  When ..  Do  */

    /* try several times to get a line from the client */
    If line = "" Then line = getline()
    If line = "" Then line = getline()
    If line = "" Then line = getline()
    If line = "" Then line = getline()
    If line = "" Then line = getline()
    If line = "" Then Do
        If open Then Call ABORT
        rc = 12
        Leave
        End  /*  When ..  Do  */

/*  If verbose Then Say argo line        */
    meta = 0
    Parse Upper Var line verb .
    If verb = "META" Then Do
        Parse Upper Var line . verb .
        Parse       Var line . line
        meta = 1
        End  /*  If .. Do  */
    code = 0

    Select  /*  verb  */

        When ^meta & verb = "" Then nop
        When ^meta & Left(verb,1) = '*' Then If verbose Then Say line
        When ^meta & Left(verb,1) = '#' Then If verbose Then Say line

        When verb = "NOP" | verb = "NOOP" Then ,
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'

        When verb = "AGENT" Then Do
            Parse Var line . agch agrs .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End /* When .. Do */
        /* above is not actually implemented ... in case no obvious */

        When verb = "FILE" Then Do
            Parse Var line . size from auth .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "USER" Then Do
            Parse Upper Var line . user .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "TYPE" Then Do
            Parse Upper Var line . type cc .
            Say argo "TYPE" type
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "NAME" Then Do
            Parse Var line . name
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        /*  this is not entirely right;  DATE should be ISO format  */
        When verb = "DATE" Then Do
            Parse Upper Var line . date time tz .
            If Index(date,'.') > 0 Then Do
                Parse Var date yy '.' mm '.' dd '.' .
                date = Right(mm,2,'0') || '/' || ,
                       Right(dd,2,'0') || '/' || ,
                       Right(yy,2,'0')
                End
            If tz = "" Then tz = "N/A"
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        /*  stuff meaningful on print jobs  */
        When verb = "FORM" Then Do
            Parse Upper Var line . form .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When Abbrev("DESTINATION",verb,4) Then Do
            Parse Upper Var line . dest .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When Abbrev("DISTRIBUTION",verb,4) Then Do
            Parse Upper Var line . dist .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "COPY" | Abbrev("COPIES",verb,4) Then Do
            Parse Upper Var line . copy .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When Abbrev("CLASS",verb,2) Then Do
            Parse Upper Var line . class dev .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        /*  stuff meaningful on IBM print jobs  */
        When Abbrev("CTAPE",verb,2) | verb = "FCB"  Then Do
            Parse Upper Var line . fcb .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When Abbrev("CHARSET",verb,2) | verb = "UCS"  Then Do
            Parse Upper Var line . ucs .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "HOLD" Then Do
            Parse Upper Var line . hold .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When verb = "KEEP" Then Do
            Parse Upper Var line . keep .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        /*  stuff meaningful for record-oriented files  */
        When Abbrev("RECLEN",verb,4) | verb = "LRECL" | ,
             Abbrev("RECORD_LENGTH",verb,8) Then Do
            Parse Upper Var line . lrecl .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        When Abbrev("RECFMT",verb,4) | ,
             Abbrev("RECORD_FORMAT",verb,8) Then Do
            Parse Upper Var line . recfm .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            End  /*  When .. Do  */

        /* Here's a collision for ya:                                 *
         * If the command is MSG, is it the spool file setting        *
         * or is it an interactive message to send to a user?         */
        When verb = "MSG" & ^meta Then Do
            /* if it's the meta command or is ON or OFF then this     */
            Parse Upper Var line . tst txt
            If txt = "" & (tst = "OFF" | tst = "ON") Then Do
                msg = tst
                rc = 0
            End /* If .. Do */ ; Else Do
            /* if not a meta command then send message to named user  */
                Parse Var line . . txt
                tmt = tst "From" remote_host
                Select
                    When remote_user ^= "" Then ,
                        tmt = tmt || "(" || remote_user || "):" txt
                    When from ^= "" Then ,
                        tmt = tmt || "(" || from || "):" txt
                    Otherwise tmt = tmt || ":" txt
                End /* Select */
        'CALLPIPE VAR TMT | SPEC /MSGNOH / 1 1-* NEXT | CP | STEM RS.'
                If rc = 1 Then ,
        'CALLPIPE VAR TMT | SPEC /MSG / 1 1-* NEXT | CP | STEM RS.'
                xrc = rc
                'CALLPIPE STEM RS. | SPEC /199 / 1 1-* NEXT' ,
                    '| *.OUTPUT:'
                rc = xrc
            End /* Else Do */
            Select /* rc */
                When rc = 0 Then ,
                    'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                    'CALLER SRV NOHEADER | *.OUTPUT:'
                When rc = 45 Then ,
                    'CALLPIPE COMMAND XMITMSG 545 (APPLID UFT' ,
                    'CALLER SRV NOHEADER | *.OUTPUT:'
                When rc = 57 Then ,
                    'CALLPIPE COMMAND XMITMSG 557 (APPLID UFT' ,
                    'CALLER SRV NOHEADER | *.OUTPUT:'
                Otherwise ,
                    'CALLPIPE COMMAND XMITMSG 500 (APPLID UFT' ,
                    'CALLER SRV NOHEADER | *.OUTPUT:'
            End /* Select */
        End /* When .. Do */

        /* a BITNETism, because I like it */
        When verb = "CPQ"  Then Do
            Parse Upper Var line . cpq
            /* LOGMSG, USER user, USERS, NAMES, TIME, etc. */
            'CALLPIPE VAR CPQ | SPEC /QUERY / 1 1-* NEXT' ,
                '| CP | SPEC /199 / 1 1-* NEXT | *.OUTPUT:'
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When Abbrev("SEQUENCE",verb,3) Then Do
            Parse Upper Var line . seq .
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When verb = "QUIT" Then Do
            If open Then Call CLOSE
            /* send a "goodbye" to the client ... maybe already gone  */
            'CALLPIPE COMMAND XMITMSG 221 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
            /* If server_pipe_attach != "" Then 'READTO' */
            Leave
        End /* When .. Do */

        When verb = "HELP" Then Do
            'CALLPIPE COMMAND XMITMSG 114' ,
                '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
            'CALLPIPE < UFTDHELP TXT' ,
                '| SPEC /114 / 1 1-* NEXT | *.OUTPUT:'
            'CALLPIPE COMMAND XMITMSG 214' ,
                '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When verb = "DATA" Then Do
            Parse Var line . count .
            If count = "" Then Leave
            If Datatype(count,'W') Then Call DATA
            Else 'CALLPIPE COMMAND XMITMSG 401' ,
                '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When Abbrev("AUXDATA",verb,4) Then Do
            Parse Var line . count .
            If count = "" Then Leave
            If Datatype(count,'W') Then Call AUXDATA
            Else 'CALLPIPE COMMAND XMITMSG 401' ,
                '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When verb = "EOF" | verb = "CLOSE" Then Do
            If open Then Call CLOSE
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When verb = "ABORT" Then Do
            If open Then Call ABORT
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
        End /* When .. Do */

        When ^meta Then Do
            'CALLPIPE COMMAND XMITMSG 402 "' || verb || '"' ,
                '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
            code = 402
            End  /*  Otherwise Do  */

        Otherwise Do
            'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
                'CALLER SRV NOHEADER | *.OUTPUT:'
        End /* Otherwise Do */

    End /* Select verb */

    /* if not open then accumulate lines of meta data */
    If ^open Then Do
        i = i + 1
        line.i = line
    End /* If .. Do */

    If rc ^= 0 Then Leave

End /* Do Forever */

/* close the spool file (if one was open) */
qrc = rc
If open Then Call CLOSE
rc = qrc

Exit rc

/* ---------------------------------------------------------------- DATA
 *  Read a burst of data then return to command mode.
 *
 *    Presumes: file output stream is FILE and default output is 0
 */
DATA:

If ^open Then Call OPEN

If uft > 1 Then ,
    'CALLPIPE COMMAND XMITMSG 323 "' || count || '"' ,
        '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
Else 'CALLPIPE COMMAND XMITMSG 123 "' || count || '"' ,
    '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'

'SELECT OUTPUT FILE'

If Length(buffer) > count Then Do
    'OUTPUT' Left(buffer,count)
    buffer = Substr(buffer,count+1)
    count = 0
    End  /*  If  ..  Do  */

Else If Length(buffer) > 0 Then Do
    'OUTPUT' buffer
    count = count - Length(buffer)
    buffer = ""
    End  /*  Else If .. Do  */

Do While count > 0

    'READTO';  'PEEKTO BUFFER'
    If rc ^= 0 Then Do
        Call ABORT
        Exit 12
        End  /*  If  ..  Do  */

    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If rc ^= 0 Then Do
        Call ABORT
        Exit 12
        End  /*  If  ..  Do  */

    If Length(buffer) > count Then Do
        'OUTPUT' Left(buffer,count)
        buffer = Substr(buffer,count+1)
        count = 0
        End  /*  If  ..  Do  */

    Else If Length(buffer) > 0 Then Do
        'OUTPUT' buffer
        count = count - Length(buffer)
        buffer = ""
        End  /*  Else  If  ..  Do  */

    End  /*  Do  While  */

'SELECT OUTPUT 0'

'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
    'CALLER SRV NOHEADER | *.OUTPUT:'

Return

/* ------------------------------------------------------------- AUXDATA
 *  Read a burst of AUXILIARY data then return to command mode.
 */
AUXDATA:

If ^open Then Call OPEN

If uft > 1 Then ,
    'CALLPIPE COMMAND XMITMSG 323 "' || count || '"' ,
        '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'
Else 'CALLPIPE COMMAND XMITMSG 123 "' || count || '"' ,
    '(APPLID UFT CALLER SRV NOHEADER | *.OUTPUT:'

/*
'SELECT OUTPUT AUXD'
 */

If Length(buffer) > count Then Do
/*
    'OUTPUT' Left(buffer,count)
 */
    buffer = Substr(buffer,count+1)
    count = 0
    End  /*  If  ..  Do  */

Else If Length(buffer) > 0 Then Do
/*
    'OUTPUT' buffer
 */
    count = count - Length(buffer)
    buffer = ""
    End  /*  Else If .. Do  */

Do While count > 0

    'READTO';  'PEEKTO BUFFER'
    If rc ^= 0 Then Do
        Call ABORT
        Exit 12
        End  /*  If  ..  Do  */

    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If Length(buffer) = 0 Then Do 'READTO'; 'PEEKTO BUFFER'; End
    If rc ^= 0 Then Do
        Call ABORT
        Exit 12
        End  /*  If  ..  Do  */

    If Length(buffer) > count Then Do
/*
        'OUTPUT' Left(buffer,count)
 */
        buffer = Substr(buffer,count+1)
        count = 0
        End  /*  If  ..  Do  */

    Else If Length(buffer) > 0 Then Do
/*
        'OUTPUT' buffer
 */
        count = count - Length(buffer)
        buffer = ""
        End  /*  Else  If  ..  Do  */

    End  /*  Do  While  */

/*
'SELECT OUTPUT 0'
 */

'CALLPIPE COMMAND XMITMSG 200 (APPLID UFT' ,
    'CALLER SRV NOHEADER | *.OUTPUT:'

Return

/* ---------------------------------------------------------------- OPEN
 *        Sets: dev addr open
 */
OPEN:

i = i + 1
line.i = "FROM" from || '@' || remote_host
line.0 = i

/*  Figure out how to SPOOL this file.  */
Select  /*  type  */

    When type = "A" | type = "T" Then Do
        If dev = "" Then dev = "PRT"
        pipe = 'MAKETEXT -LOCAL'
        If cc = 'C' Then pipe = pipe '| ASATOMC'
                    Else pipe = pipe '| SPEC .09. X2C 1 1-* NEXT'
        End  /*  When  ..  Do  */

    When type = "E" Then Do
        If dev = "" Then dev = "PRT"
        pipe = 'DEBLOCK LINEND 15 | DROP LAST'
        If cc = 'C' Then pipe = pipe '| ASATOMC'
                    Else pipe = pipe '| SPEC .09. X2C 1 1-* NEXT'
        End  /*  When  ..  Do  */

    When type = "V" Then Do
        If dev = "" Then dev = "PRT"
        pipe = 'DEBLOCK CMS'
        If cc = 'C' Then pipe = pipe '| ASATOMC'
                    Else pipe = pipe '| SPEC .09. X2C 1 1-* NEXT'
        End  /*  When  ..  Do  */

    When type = 'B' | type = 'I' | type = 'U' Then Do
        dev = "PUN"
        pipe = 'FBLOCK 80 00 | SPEC .41. X2C 1 1-* NEXT'
        End  /*  When  ..  Do  */

    When type = "M" Then Do
        If dev = "" Then dev = "PRT"
        pipe = 'MAKETEXT -LOCAL | UFTDMAIL' user from ,
            '| SPEC .09. X2C 1 1-* NEXT'
       user = mailer
       End  /*  When  ..  Do  */

    Otherwise Do            /*  treat it as binary  */
        dev = "PUN"
        pipe = 'FBLOCK 80 00 | SPEC .41. X2C 1 1-* NEXT'
        End  /*  Otherwise  Do  */

    End  /*  Select  type  */

/*  Create a temporary virtual unit-record device.  */
Call Diag 08, 'DEFINE' dev addr

/*  we don't do forwarding  */
Parse Upper Var user user '@' .
Call Diag 08, 'SPOOL' addr 'TO' user

/*  set a few parameters  */
Call Diag 08, 'SPOOL' addr 'FORM' form
Call Diag 08, 'SPOOL' addr 'DEST' dest
Call Diag 08, 'SPOOL' addr 'DIST' dist
Call Diag 08, 'SPOOL' addr 'COPY' copy
Call Diag 08, 'SPOOL' addr 'CLASS' class

/*  build a TAG that RDRLIST will understand  */
Parse Upper Var from from '@' morf .
Parse Upper Var remote_host host '.' .
If host = "" Then host = morf
If host = "" Then host = "N/A"
If from = "" Then from = "N/A"
xmm1 = "*UFT"; xmm2 = "-"
xmm3 = Left(host,8); xmm4 = Left(from,8)
xmm5 = Right(date,8); xmm6 = Right(time,8); xmm7 = Left(tz,3)
'CALLPIPE COMMAND XMITMSG 9001 XMM1 XMM2 XMM3 XMM4 XMM5 XMM6 XMM7' ,
'(APPLID UFT CALLER SRV NOHEADER' ,
    '| TAKE FIRST | VAR UTAG'
Parse Value Diagrc(08,'TAG DEV' addr utag) With 1 rc 10 . 17 rs '15'x .

/*  insert a signature (magic number) for UFT  */
/*  'CALLPIPE COMMAND XMITMSG 0 (APPLID UFT CALLER SRV NOHEADER' ,
    '| SPEC .03. X2C 1 .*. NEXT 1-* NEXT | URO' addr                  */
'CALLPIPE VAR VRM | SPEC .03. X2C 1 .*UFTD/. NEXT 1-* NEXT | URO' addr
/*  send the "meta file" as NOP records  */
'CALLPIPE STEM LINE. | SPEC .03. X2C 1 1-* NEXT | URO' addr
'ADDSTREAM OUTPUT FILE'
'ADDPIPE *.OUTPUT.FILE: |' pipe '| URO' addr

open = 1
i = 0

Return

/* --------------------------------------------------------------- CLOSE
 *  closes and severs file output stream,  retaining file contents
 *
 *        Uses: user host name addr tz argo
 *        Sets: _name fn ft rc rs open
 *    Presumes: file output stream is FILE and default output is 0
 */
CLOSE:

'SELECT OUTPUT FILE'
'SEVER OUTPUT'
'SELECT OUTPUT 0'

Parse Value Reverse(name) With _name '/' . ':' .
Parse Upper Value Reverse(_name) with fn '.' ft '.' .
If fn = "" & ft ^= "" Then fn = from
If fn ^= "" Then ,
'CALLPIPE CP CLOSE' addr 'NAME' Left(fn,8) Left(ft,8) '| STEM RS.'
Else rs.0 = 0
'CALLPIPE CP CLOSE' addr '| STEM RS. APPEND'
Call Diag 08, 'DETACH' addr

/*  pass a comment about results back to the client  */
'CALLPIPE STEM RS. | SPEC /199 / 1 1-* NEXT | *.OUTPUT:'

/*  message the user that this file has arrived  */
'CALLPIPE COMMAND XMITMSG 9002 SEQ USER HOST FROM DATE TIME TZ' ,
    '(APPLID UFT CALLER SRV NOHEADER | TAKE FIRST | VAR UMSG' ,
        '| SPEC /199 / 1 1-* NEXT | *.OUTPUT:'
/*  '(APPLID UFT CALLER SRV NOHEADER | TAKE FIRST | VAR UMSG | CONSOLE'  */
Parse Value Diagrc(08,'MSGNOH' user umsg) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then ,
Parse Value Diagrc(08,'MSG' user umsg) With 1 rc 10 . 17 rs '15'x .
/*  Say "* File (" || seq || ") spooled to" user ,
    "-- origin" host || '(' || from || ')' Date('U') Time() tz        */

/*  mark the file as closed  */
open = 0

/*  reset everything  */
user = "SYSTEM";        name = ""
type = 'A';             cc = ""
Parse Value Diag(08,'QUERY TIME') With . . time tz . date . '15'x .
form = "STANDARD";      dest = "OFF";           dist = "LOCAL"
copy = 1;               class = 'A';            dev = ""
hold = "OFF";   fcb  = "OFF";   ucs  = "OFF"
keep = "OFF";   msg  = "OFF"

Return

/* --------------------------------------------------------------- ABORT
 *  closes and severs file output stream,  discarding file contents
 *
 *        Uses: addr
 *        Sets: open
 *    Presumes: file output stream is FILE and default output is 0
 */
ABORT:

'SELECT OUTPUT FILE'
'SEVER OUTPUT'
'SELECT OUTPUT 0'

Call Diag 08, 'SPOOL' addr 'PURGE'
Call Diag 08, 'DETACH' addr

open = 0

user = "SYSTEM";        name = ""
type = 'A';             cc = ""
Parse Value Diag(08,'QUERY TIME') With . . time tz . date . '15'x .
form = "STANDARD";      dest = "OFF";           dist = "LOCAL"
copy = 1;               class = 'A';            dev = ""
hold = "OFF";   fcb  = "OFF";   ucs  = "OFF"
keep = "OFF";   msg  = "OFF"

Return

/*

Parse Var name fn '.' ft '.' .

'ADDPIPE *.OUTPUT: | BLOCK 255 NETDATA | *.OUTPUT:'
Parse Var ls fn ft fm f l r b date time .
'CALLPIPE INMR123' node user fn ft 'A1' ,
    'V' 0 0 0 date time '(NOSPOOL | *:'
'CALLPIPE *: | SPEC x00 1 1-* NEXT | *:'
'OUTPUT' '20'x || "INMR06"

 */

/* ------------------------------------------------------------- GETLINE
 */
GETLINE: Procedure Expose buffer
Do While Index(buffer,'0A'x) = 0 & Index(buffer,'00'x) = 0
    'READTO';  'PEEKTO PACKET'
    If rc ^= 0 Then Leave
    buffer = buffer || packet
    End  /*  Do While  */
Parse Var buffer line '0A'x buffer
Parse Var line line '0D'x .
Parse Var line line '00'x .
/*  elastic stage makes this not cause a stall  */
'CALLPIPE VAR LINE | *.OUTPUT.LINE:'
'CALLPIPE *.INPUT.LINE: | VAR LINE'
Parse Var line line '15'x .
Return line

/* ------------------------------------------------------------- PUTLINE
 */
PUTLINE: Procedure
Parse Arg line
'CALLPIPE VAR LINE | *.OUTPUT:'
Return rc

/* --------------------------------------------------------------- AGENT
 *  Confirm client is an agent.
 */
AGENT:
Return 0


/*

AGENT

AUXDATA

need to support:
XDATE
OWNER
GROUP
XPERM
VERSION

QUERY

 */


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

/* if we got an address then stash it for later reference */
If val ^= "" Then Call Value var, val, "SESSION NSLOOKUP"

Return val

/* ------------------------------------------------------------ HOSTNAME
 *  return the internet hostname for the given IP address
 *  "$" stores the name which goes with the supplied address
 */
_hostname: Procedure
Parse Arg h . , .
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


