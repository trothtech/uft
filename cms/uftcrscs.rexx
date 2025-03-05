/* © Copyright 1992-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCRSCS REXX
 *              Pipeline stage to hand-off a SIFT/UFT job to RSCS
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *        Date: 1992-Oct-06, 1993-Feb-22
 *              2025-02-11
 *
 *        Note: This stage presently sends all files in binary (PUNCH).
 *              This is *not* how things should work in the long run.
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argo = arg0 || ':'
argl = '*' || argo

Parse Upper Arg host . '(' . ')' .
user = ""
If Index(host,'@') > 0 Then Parse Var host user '@' host
If host = "" Then Do
    'CALLPIPE COMMAND XMITMSG 2687 (ERRMSG | CONSOLE'
    Say argo "missing argument"
    Exit 24
    End /* If .. Do */

'PEEKTO'        /* verify existence of input stream */
If rc ^= 0 Then Do
    prc = rc
    'CALLPIPE COMMAND XMITMSG 2914 (ERRMSG | CONSOLE'
    Say argo "no input stream"
    Exit prc
    End /* If .. Do */

'CALLPIPE COMMAND IDENTIFY | VAR IDENTITY'
If rc ^= 0 Then Exit rc
Parse Var identity userid . hostid . rscsid .

/* find an unused virtual address */
Address "CMS" 'GETFMADR 200'
If rc ^= 0 Then Exit rc
Parse Pull . . tmp .

/* pre-set some variables */
type = "I";  cc = "";  dev = "";  class = ""
form = "";
Parse Value Diag(08,'QUERY TIME') With . . time tz . date . '15'x .
name = "";  dest = "";  dist = ""
fcb = "";  ucs = ""

/* read commands from the input stream until "DATA" */
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then /* Exit rc */ Leave

    Parse Var line cmnd args; Upper cmnd
    Select /* cmnd */
        When cmnd = "FILE" Then nop
        When cmnd = "USER" Then Do
            If user = "" Then Parse Upper Var line . user '@' .
            End /* When .. Do */
        When cmnd = "NAME" Then name = args
        When cmnd = "TYPE" Then Parse Upper Var line . type cc .
        When cmnd = "CLASS" Then Parse Upper Var line . class dev .
        When cmnd = "FORM" Then Parse Upper Var line . form .
        When cmnd = "DEST" Then Parse Upper Var line . dest .
        When cmnd = "DIST" | cmnd = "BIN" | cmnd = "BOX" ,
                           Then Parse Upper Var line . dist .
        When cmnd = "FCB" | cmnd = "CTAPE" ,
                           Then Parse Upper Var line . fcb .
        When cmnd = "UCS" | cmnd = "CHARS" | cmnd = "TRAIN" ,
            | cmnd = "FONT" Then Parse Upper Var line . ucs .
        When  cmnd = "DATE"     Then
            Parse Var line . date time tz .
        When  cmnd = "DATA"     Then  Leave
        Otherwise nop
        End /* Select cmnd */

    'READTO'

    End /* Do Forever */

'READTO'
If rc ^= 0 Then Exit rc

Select
    When type = "V" & cc = "M" Then Do
        /* spool-to-spool (VM to VM) transfer */
        If dev = "" Then dev = "PRT"
        pipe = "DEBLOCK CMS"
        End /* When .. Do */
    When type = "A" | type = "T" | type = "E" Then Do
        /* text file (ASCII or EBCDIC file) */
        dev = "PRT"
        pipe = "SPEC x09 1 1-* NEXT"
        End /* When .. Do */
    When type = "I" | type = "B" | type = "U" Then Do
        /* binary file (unstructured octet stream) */
        dev = "PUN"
        pipe = "FBLOCK 80 00 | SPEC x41 1 1-* NEXT"
        End /* When .. Do */
    When type = "N" Then Do
        /* IBM NETDATA format */
        dev = "PUN"
        pipe = "FBLOCK 80 00 | SPEC x41 1 1-* NEXT"
        End /* When .. Do */
    When type = "V" Then Do
        /* variable-length records */
        dev = "PRT"
        pipe = "SPEC x09 1 1-* NEXT"
        End /* When .. Do */
    Otherwise Do
        /* treat all others as binary */
        dev = "PUN"
        pipe = "FBLOCK 80 00 | SPEC x41 1 1-* NEXT"
        End /* Otherwise Do */
    End /* Select */

/* create a disposable URO device */
If dev = "CON" Then dev = "PRT"
Parse Value DiagRC(08,'DEFINE' dev tmp) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then Do;  Say rs;  Exit rc;  End

/* feed this to RSCS */
Parse Value DiagRC(08,'SPOOL' tmp 'TO' rscsid ,
        'NOHOLD NOCONT') With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then Do;  Say rs;  Exit rc;  End
Call Diag 08, 'TAG DEV' tmp host user

/* apply attributes */
If class ^= "" Then Call Diag 08, 'SPOOL' tmp 'CLASS' class
If form ^= "" Then Call Diag 08, 'SPOOL' tmp 'FORM' form
If dest ^= "" Then Call Diag 08, 'SPOOL' tmp 'DEST' dest
If dist ^= "" Then Call Diag 08, 'SPOOL' tmp 'DIST' dist
If fcb ^= "" Then Call Diag 08, 'SPOOL' tmp 'FCB' fcb
If ucs ^= "" Then Call Diag 08, 'SPOOL' tmp 'UCS' ucs

/* now send the file */
'CALLPIPE *: |' pipe '| URO' tmp
If rc ^= 0 Then Exit rc

/* is it named? */
If name ^= "" Then Do
    Parse Upper Value Translate(name,' ','.') With fn ft .
    name = "NAME" fn ft
    End /* If .. Do */

/* relieve ourself of this temporary UR device */
Parse Value DiagRC(08,'CLOSE' tmp name) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 & rs ^= "" Then Say rs
Parse Value DiagRC(08,'DETACH' tmp) With 1 rc 10 . 17 rs '15'x .
If rc ^= 0 & rs ^= "" Then Say rs

Exit


