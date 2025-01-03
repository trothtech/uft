/* © Copyright 1994, 1995, 1996, 1997, 2024, Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCFILE REXX
 *              Unsolicited File Transfer client file reader stage
 *              builds a SIFT job and feeds it to UFTCMAIN
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas (METRO)
 *        Date: 1993-Feb-19, Oct-08
 *       Calls: UFTXINMR
 *
 *        Note: UFTCFILE is not a user-level pipeline stage.
 *
 *        Note: ABC-EF--I----N---R--UV---- supported
 *              ---D---H----M---------WX-- defined
 *              ------G--JKL--OPQ-ST----YZ undefined
 */

type = 'A'

Parse Arg fn ft fm . user . host . '(' opts ')' .

/*  any options?  */
Do While opts ^= ""
    Parse Upper Var opts op opts
    Select  /*  op  */
        When  Abbrev("TYPE",op,1)       Then
            Parse Upper Var opts type opts
        When  Abbrev("ASCII",op,1)      Then  type = 'A'
        When  Abbrev("BINARY",op,1)     Then  type = 'I'
        Otherwise  Do
            Address "COMMAND" 'XMITMSG 3 OP (ERRMSG'
            Exit 24
            End  /*  Otherwise  Do  */
        End  /*  Select  op  */
    End  /*  Do  While  */

If fm = '-' Then Do
    /* file is a pipeline */
    size = 0; f = 'V'; l = 0; r = 0; b = 0
    Parse Value Diag(08,'QUERY TIME') ,
        With . . time tz . date . '15'x .
End /* If .. Do */

Else Do
    /* file is disk resident thtn discover some characteristics */
    'CALLPIPE CMS LISTFILE' fn ft fm '(DATE | VAR RS | DROP | VAR LS'
    If rc ^= 0 Then Do ; Say rs ; Exit rc ; End
    Parse Var ls fn ft fm f l r b date time .
    fm = Left(fm,1)
    'CALLPIPE COMMAND QUERY DISK' fm '| DROP | VAR QD'
    Parse Var qd . 8 . . . . . bs .
    If Datatype(bs,'N') & Datatype(b,'N') Then size = b * bs
    tz = ""
End /* Else Do */

If Index(date,"/") > 0 Then Do
    Parse Var date mm '/' dd '/' yy
    If Datatype(yy,'N') Then If yy < 100 Then yy = yy + 1900
    If yy < 1960 Then yy = yy + 100
    date = yy || '-' || mm || '-' || dd
End /* If .. Do */

'CALLPIPE CP QUERY USERID | XLATE LOWER | VAR FROM'
Parse Var from from .
'OUTPUT' "FILE" size from
/* 'OUTPUT' "USER" user */

Select /* type */
    When type = '*'              Then Do
        /* dotted hostname implies non-IBM target */
        If Index(host,'.') = 0 Then type = 'N'
                               Else type = 'A'
        c = ""
    End /* When .. Do */
    When Abbrev("CC",type,1)     Then Do
        type = 'A'; c = 'C'
    End /* When .. Do */
    When Abbrev("RECORD",type,1) Then Do
        type = f
        If type = 'F' Then c = l
                      Else c = ""
    End /* When .. Do */
    Otherwise  c = ""
    End /* Select type */
'OUTPUT' "TYPE" type c

name = fn || '.' || ft
'CALLPIPE VAR NAME | XLATE LOWER | SPEC /NAME / 1 1-* NEXT | *:'

'OUTPUT' "DATE" date time tz

'OUTPUT' "RECFM" f
'OUTPUT' "LRECL" l

/*  signal the server "here comes the file body"  */
'OUTPUT' "DATA"

/* reformatting is handled by later stages,
   but NETDATA processing must apply now. */
If Abbrev("NETDATA",type,1) Then ,
     'CALLPIPE UFTXINMR SEND' fn ft fm 'TO' user 'AT' host '| *:'
Else 'CALLPIPE <' fn ft fm '| *:'

Exit rc


