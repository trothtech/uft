/* © Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTXDSPL REXX (CMS Pipelines "gem")
 *              de-spool a file into a SIFT job (UFT usable format)
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Jan-25
 *              2025-02-11
 *
 *        Note: UFTXDSPL is not a user-level pipeline stage.
 *              The usual output is to the UFTCMAIN stage.
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argo = arg0 || ':'
argl = '*' || argo

uc = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
lc = "abcdefghijklmnopqrstuvwxyz"

/* parse command line for spool file ID number */
Parse Arg sid . '(' . ')' .

/* does the file exist? */
Parse Value DiagRC(08,'QUERY READER' sid) With ,
    1 rc 10 . 17 rs '15'x rdr '15'x .
If rc ^= 0 Then Do
    Say rs
    Exit rc
End /* If .. Do */

/* now parse the response from CP */
Parse Var rdr 1 from 9 . 15 class 16 . 17 devtype 20 . ,
        21 records 29 . 30 cpy 33 . 34 hold 38 . ,
        39 form 47 . 48 dest 56 . 57 keep 61 . 62 msg 65 .
from = Translate(Strip(from),lc,uc)

/* also get the "table" data */
Parse Value DiagRC(08,'QUERY READER' sid 'TBL') With ,
    1 rc 10 . 17 rs '15'x tbl '15'x .
If rc ^= 0 Then Do
    Say rs
    Exit rc
End /* If .. Do */

/* and parse that */
Parse Var tbl 1 . ,
        30 flash 35 . 36 fcb 39 . 40 mdfy 45 . ,
        46 flshc 51 . 52 load 56 . 57 chars . 77 size 81 .
If Datatype(size,'W') Then size = (size * 4) || "K"

/* and finally, get and parse the long form query */
Parse Value DiagRC(08,'QUERY READER' sid 'ALL') With ,
    1 rc 10 . 17 rs '15'x info '15'x .
If rc ^= 0 Then Do
    Say rs
    Exit rc
End /* If .. Do */

Parse Value Diag(08,'QUERY DATEFORMAT') With . . . df . '15'x .
If df = "ISODATE" Then Parse Var info 1 . ,
        39 date 49 . 50 time 58 . 59 fn 67 . ,
            69 ft 77 . 78 dist 86 .
                  Else Parse Var info 1 . ,
        39 date 44 . 45 time 53 . 54 fn 62 . ,
            64 ft 72 . 73 dist 81 .
fn = Strip(fn) ; ft = Strip(ft)
If fn ^= "" | ft ^= "" Then fn = fn || "." || ft
                           Else name = ""
fn = Translate(fn,lc,uc)

/* spin-up a temporary virtual reader */
Address CMS 'GETFMADR'
If rc ^= 0 Then Exit rc
Parse Pull . . va .
Parse Value DiagRC(08,'DEFINE READER' va) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then Do
    Say rs
    Exit rc
End /* If .. Do */
Call Diag 08, 'SPOOL' va 'HOLD CLASS *'

/* order this file to top-of-queue */
Parse Value DiagRC(08,'ORDER READER' sid) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then Do
    Say rs
    Call Diag 08, 'DETACH' va
    Exit rc
End /* If .. Do */

/* attach reader stage to input */
'ADDPIPE READER' va 'FILE' sid '| *.INPUT:'
If rc ^= 0 Then Do;  Call Diag 08, 'DETACH' va;  Exit rc;  End

/* examine the TAG record (should be a NOP CCW) */
'READTO TAG'
Parse Var tag 1 _cc 2 tag
If rc ^= 0 Then Do ; Call Diag 08, 'DETACH' va ; Exit rc ; End

/* examine second record, the first real record */
'PEEKTO RECORD'
If rc ^= 0 Then Do ; Call Diag 08, 'DETACH' va ; Exit rc ; End
Parse Var record 1 _cc 2 data

/* output the SIFT job header records */
'OUTPUT' "FILE" size from "-"
'OUTPUT' argl "SPOOLID" sid "OWNER" Userid()
tag = Strip(tag)
Parse Var tag tagnode taguser tagprio tagopts
If tag ^= "" Then 'OUTPUT' "*TAG" tag

cc = ""
Select
    When type = "MAIL" | type = "NOTE"  Then    Call XMAILUFT
    When Left(data,4) = "*UFT"          Then    Call READYUFT
    When Substr(data,3,4) = "INMR"      Then    Call NDATAUFT
    Otherwise                                   Call SPOOLUFT
End /* Select */

/* output the SIFT job header records */
'OUTPUT' "TYPE" type cc
If fn ^=    "" Then 'OUTPUT' "META NAME" fn

If class ^= "" Then 'OUTPUT' "META CLASS" class devtype
If cpy   ^= "" Then 'OUTPUT' "META COPIES" cpy
If hold  ^= "" Then 'OUTPUT' "META HOLD" hold
If form  ^= "" Then 'OUTPUT' "META FORM" form
If dest  ^= "" Then 'OUTPUT' "META DEST" dest
If dist  ^= "" Then 'OUTPUT' "META DIST" dist
If keep  ^= "" Then 'OUTPUT' "META KEEP" keep
If msg   ^= "" Then 'OUTPUT' "META MSG" msg

/*  pass the file contents  */
'OUTPUT' "DATA"       /* this is a special case of the "data" command */
'CALLPIPE *: | *:'

'SEVER INPUT'
Call Diag 08, 'CLOSE' va
Call Diag 08, 'DETACH' va

Exit

/* ------------------------------------------------------------ READYUFT
 * Say "Looks like a UFT-ready file."
 * Consume leading no-op CCWs for metadata then strip CCWs.
 */
READYUFT:
dt = ""         /* sender's DEVTYPE */
Do Forever
    'PEEKTO RECORD'
    If rc ^= 0 Then Leave
    Parse Var record 1 _cc 2 data
    If _cc ^= '03'x Then Leave
    Parse Upper Var data verb .
    If verb = "META" Then Parse Upper Var data . verb .
    Select /* verb */
        When Abbrev("FILE",verb,1)    Then nop
        When Abbrev("USER",verb,1)    Then nop
        When Abbrev("TYPE",verb,1)    Then Parse Var data . type cc .
        When Abbrev("NAME",verb,1)    Then Parse Var data . fn .
        When Abbrev("CLASS",verb,2)   Then Parse Var data . class dt .
        When Abbrev("COPIES",verb,2)  Then Parse Var data . cpy .
        When Abbrev("COPY",verb,2)    Then Parse Var data . cpy .
        When Abbrev("HOLD",verb,2)    Then Parse Var data . hold .
        When Abbrev("FORM",verb,2)    Then Parse Var data . form .
        When Abbrev("DEST",verb,2)    Then Parse Var data . dest .
        When Abbrev("DIST",verb,2)    Then Parse Var data . dist .
        When Abbrev("KEEP",verb,4)    Then Parse Var data . keep .
        When Abbrev("MSG",verb,3)     Then Parse Var data . msg .
        When Abbrev("DATA",verb,3)    Then nop
        Otherwise 'OUTPUT' data
    End /* Select verb */
    'READTO'
    If rc ^= 0 Then Leave
End /* Do Forever */

/* did the sender provide a DEVTYPE? */
If dt ^= "" Then devtype = dt

/* if it looks like a VM-to-VM transfer, then ... */
If type = "V" & cc = "M" Then Return

/* else, remove CCWs from stream, and possibly pad the records */
'ADDPIPE *.INPUT: | NLOCATE 1.1' '000300'x '| SPEC 2-* 1 | *.INPUT:'
If rc ^= 0 Then Do;  Call Diag 08, 'DETACH' va;  Exit rc;  End
If devtype = "PUN" Then 'ADDPIPE *.INPUT: | PAD 80 | *.INPUT:'

Return

/* ------------------------------------------------------------ NDATAUFT
 * Say "Looks like a NETDATA file."
 * Strip the CCWs.
 */
NDATAUFT:
type = "N"
/* remove CCWs from stream, and possibly pad the records */
'ADDPIPE *.INPUT: | NLOCATE 1.1' '000300'x '| SPEC 2-* 1 | *.INPUT:'
If rc ^= 0 Then Do;  Call Diag 08, 'DETACH' va;  Exit rc;  End
If devtype = "PUN" Then 'ADDPIPE *.INPUT: | PAD 80 | *.INPUT:'

Return

/* ------------------------------------------------------------ SPOOLUFT
 * Say "Looks like an ordinary spool file."
 * Machine carriage control and do not strip the CCWs.
 */
SPOOLUFT:
/* variable length records with machine carriage control ("VM", cute) */
type = "V M"
Return

/* ------------------------------------------------------------ XMAILUFT
 * Based on the filetype, treat this as email.
 * Strip the CCWs.
 */
XMAILUFT:
type = "M"
/* remove CCWs from stream, and check for RFC 822 header */
'ADDPIPE *.INPUT: | NLOCATE 1.1' '000300'x '| SPEC 2-* 1 | *.INPUT:'
If rc ^= 0 Then Do;  Call Diag 08, 'DETACH' va;  Exit rc;  End
'PEEKTO RECORD'

Return

/*
ok  FILE
dd  PIPE
??  USER
ok  TYPE
ok  DATA
dd  AUXDATA
dd  EOF
dd  ABORT
ok  META
dd  QUIT

yy  NAME
    DATE
    XDATE
    TITLE
+   OWNER
    GROUP
    XPERM
    VERSION
    RECFMT
    RECLEN
yy  CLASS
yy  FORM
yy  HOLD
yy  COPY|COPIES
    FCB
    UCS
yy  DEST
yy  DIST
    SEQ
 */

/*

 TAG

>>-TAg--+-+-DEv--device---+--+--------------+-+----------------><
        | '-FIle--spoolid-'  '-| Location |-' |
        '-QUery--+-DEv--device---+------------'
                 '-FIle--spoolid-'

|--locid--+-----------------------------------+-----------------|
          |      .-50-------.                 |
          +-JOB--+----------+-----------------+
          |      '-priority-'                 |
          |         .-50--------------------. |
          +-SYSTEM--+-----------------------+-+
          |         '-priority--| Options |-' |
          |         .-50--------------------. |
          '-userid--+-----------------------+-'
                    '-priority--| Options |-'

'CP TAG DEV PUN' tagnode taguser tagprio tagopts

 */


