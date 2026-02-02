/* © Copyright 2026 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: RSTAT REXX
 *              Pipeline stage to derive 'rstat' output
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-02-02
 *    Revision: 2.0.17
 *
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argl = '*' || arg0 || ':'                              /* for logging */

uc = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
lc = "abcdefghijklmnopqrstuvwxyz"

/* parse command line for spool file ID number */
Parse Arg spid . '(' opts ')' .

/* does the file exist? */
Parse Value DiagRC(08,'QUERY READER *' spid) With ,
    1 rc 10 . 17 rs '15'x rdr '15'x .
If rc ^= 0 Then Do ; Say rs ; Exit rc ; End

/* if the file exists then parse the response from CP */
Parse Var rdr 1 from 9 . 15 class 16 . 17 devtype 20 . ,
        21 records 29 . 30 cpy 33 . 34 hold 38 . ,
        39 form 47 . 48 dest 56 . 57 keep 61 . 62 msg 65 .
from = Translate(Strip(from),lc,uc)
If hold = "NONE" Then hold = ""
If Datatype(cpy,"W") Then If cpy = 1 Then cpy = ""
If devtype = "PUN" Then type = "I"
                   Else type = "A"
cc = ""

/* also get the "table" data */
Parse Value DiagRC(08,'QUERY READER *' spid 'TBL') With ,
    1 rc 10 . 17 rs '15'x tbl '15'x .
If rc ^= 0 Then Do ; Say rs ; Exit rc ; End

/* and parse that */
Parse Var tbl 1 . ,
        30 flash 35 . 36 fcb 39 . 40 mdfy 45 . ,
        46 flshc 51 . 52 load 56 . 57 chars . 77 size 81 .
If Datatype(size,'W') Then size = (size * 4) || "K"

/* get and parse the long form query */
Parse Value DiagRC(08,'QUERY READER *' spid 'ALL ISODATE') With ,
    1 rc 10 . 17 rs '15'x info '15'x .
If rc ^= 0 Then Do ; Say rs ; Exit rc ; End

Parse Var info 1 . ,
        39 date 49 . 50 time 58 . 59 fn 67 . ,
            69 ft 77 . 78 dist 86 .
fn = Strip(fn) ; ft = Strip(ft)
If fn ^= "" | ft ^= "" Then name = Translate(fn||"."||ft,lc,uc)
                       Else name = ""
Parse Value Diag(08,'QUERY TIME ISODATE') With . . . tz . '15'x .
date = date time tz

/* retrieve the "tag" on the file */
Parse Value DiagRC(08,'QUERY TAG FILE' spid) With ,
    1 rc 10 . 17 rs '15'x .
If rc ^= 0 Then Do ; Say rs ; Exit rc ; End
tag = rs
If Left(tag,1) = "(" Then tag = ""

/* what kind of file is this? */
If devtype = "PUN" | 1 Then Do
    Call Diag 08, 'SPOOL 00C CL *'         /* set for any spool class */
    Call Diag 08, 'ORDER RDR' spid
    'CALLPIPE COMMAND RDR' class '| VAR RDRTYPE'
    If Left(rdrtype,1) = "N" Then type = "N"
    Say "*" rdrtype
End /* If .. Do */

/* if no other output, attach console */
'STREAMSTATE OUTPUT'
If rc = 12 Then 'ADDPIPE *.OUTPUT: | CONSOLE'
If rc ^= 0 Then Exit rc

If type  ^= "" Then 'OUTPUT' "TYPE" type cc
If name ^=  "" Then 'OUTPUT' "NAME" name
If size ^=  "" Then 'OUTPUT' "SIZE" size
If date  ^= "" Then 'OUTPUT' "META DATE" date
If class ^= "" Then 'OUTPUT' "META CLASS" class devtype
If cpy   ^= "" Then 'OUTPUT' "META COPIES" cpy
If hold  ^= "" Then 'OUTPUT' "META HOLD" hold
If form  ^= "" Then 'OUTPUT' "META FORM" form
If dest  ^= "" Then 'OUTPUT' "META DEST" dest
If dist  ^= "" Then 'OUTPUT' "META DIST" dist
If keep  ^= "" Then 'OUTPUT' "META KEEP" keep
/* msg   ^= "" Then 'OUTPUT' "META MSG" msg                           */
If tag   ^= "" Then 'OUTPUT' "META TAG" tag

Exit


