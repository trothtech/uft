/*
 *
 *
 *
 *
 *
 */

/* add requisite Q RDR on input */
'ADDPIPE CP QUERY READER * | *.INPUT:'
If rc ^= 0 Then Exit rc

/* if no other output then attach the console */
'STREAMSTATE OUTPUT'
If rc = 4 Then rc = 0
If rc = 8 Then rc = 0
If rc = 12 Then 'ADDPIPE *.OUTPUT: | CONSOLE'
If rc ^= 0 Then Exit rc

/* collect RSCS server IDs and related info */
'CALLPIPE COMMAND IDENTIFY (ALL | STEM NETS.'
If rc ^= 0 Then nets.0 = 0
If nets.0 > 0 Then Parse Var nets.1 luser . lhost .
Else Parse Value Diag(08,'QUERY USERID') With luser . lhost .

'READTO'
If rc ^= 0 Then Exit rc

title = "t-c-h----- cpy user     host         size" ,
                       "date       time  spid name"
'CALLPIPE VAR TITLE | *.OUTPUT:'
If rc ^= 0 Then Exit rc

Do Forever

    'PEEKTO RECORD'
    If rc ^= 0 Then Leave

    /* ORIGINID FILE CLASS RECORDS  CPY HOLD FORM     DEST     KEE... */
    Parse Var record 1 orig 9 . 10 spid 14 . ,
                     15 cl 16 . 17 dev 20 . 21 recs 29 . ,
                     30 cpy 33 . 34 hold 38 . 39 form 47 . 48 dest .
    Select /* dev */
        When dev = "PRT" Then t = "A"
        When dev = "PUN" Then t = "I"
        When dev = "CON" Then t = "A"
        Otherwise             t = "S"
    End /* Select dev */
/* FIXME: dev PUN and content NETDATA should be TYPE N <<<<<<<<<<<<<< */
    Select /* hold */
        When Left(hold,1) = "U" Then h = "H"
        When Left(hold,1) = "S" Then h = "S"
        Otherwise                    h = "-"
    End /* Select hold */

    /* get the date and name and other supplemental info              */
    Parse Value Diag(08,'QUERY READER *' spid 'ALL ISODATE') With . '15'x rs
    Parse Var rs . 39 date 49 . 50 time 55 . 59 name type . '15'x .
    name = name type

    /* get the size in pages of this file and convert to bytes        */
    Parse Value Diag(08,'QUERY READER *' spid 'TBL') With . '15'x rs
    Parse Var rs . 77 size 81 . '15'x .
    If ^Datatype(size,"W") Then size = 0
    size = size * 4096
    If Length(size) < 8 Then size = Right(size,8)

    /* if the file came in remotely then extract sending user & host  */
    isnet = 0
    Do i = 1 to nets.0
        Parse Var nets.i . . . . netid .
        If Strip(orig) = Strip(netid) Then isnet = 1
    End /* Do For */
    If isnet Then Parse Value ,
        Diag(08,'QUERY TAG FILE' spid) With . . . host user .
    Else Do ; user = orig ; host = lhost ; End
    If user = "" Then user = orig
    If host = "" Then host = lhost

    /* write it out */
    o = t || "-" || cl || "-" || h || "-----" Right(cpy,3) ,
        Left(user,8) Left(host,8) size date time Right(spid,4) name
    o = Strip(o,"T")
    'CALLPIPE VAR O | *.OUTPUT:'

    /* consume input record - ready for next */
    'READTO'
    If rc ^= 0 Then Leave

End /* Do Forever */

Exit


