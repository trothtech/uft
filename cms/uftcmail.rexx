/* © Copyright 1992-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCMAIL REXX
 *              Pipeline stage to hand-off a SIFT job to MAILER
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas (METRO)
 *        Date: 1992-Oct-06, 1993-Feb-22, Nov-12
 *       Calls: sendmail (Pipelines gem that mimics Unix 'sendmail')
 *
 *        Note: UFTCMAIL is not a user-level pipeline stage
 */

version = "1.0"         /* MIME version, not related to UFT version */
loadmsg = 0             /* load/unload message repository */

Parse Arg host . '(' . ')' .
If host = "" Then Do
    Address "COMMAND" 'XMITMSG 2687 (ERRMSG'
    Exit 24
    End  /*  If  ..  Do  */
If Index(host,"@") > 0 Then Parse Var host user '@' host
                       Else user = ""

'PEEKTO'        /* verify existence of input stream */
If rc ^= 0 Then Do
    Address "COMMAND" 'XMITMSG 2914 (ERRMSG'
    Exit 12
    End  /*  If  ..  Do  */

/*  attach 'sendmail' stage to consume RFC 822 stream  */
'ADDSTREAM OUTPUT MAIL'
If rc ^= 0 Then Exit rc
/* the '-t' flag means to look for the addressee in the stream */
'ADDPIPE *.OUTPUT.MAIL: | sendmail -t'
If rc ^= 0 Then Exit rc

/*  get some local site variables (timezone and such)  */
'CALLPIPE CMS IDENTIFY | XLATE LOWER | VAR IDENTIFY'
Parse Var identify localuser . localhost . . date time tz day .
Upper tz

/*  say where we're comin' from  */
'CALLPIPE LITERAL MIME-Version:' version '| *.OUTPUT.MAIL:'
If rc ^= 0 Then Exit rc

targets = user
name = ""
i = 0
boundary = "SIFT." || localhost || '.' || localuser ,
    || '.' || Date('B') || '.' || Translate(Time(),'.',':')

/*  now suck-in the SIFT job stream from the preceding stage  */
type = "";  cc = ""
Do Forever

    'PEEKTO LINE'
    If rc ^= 0 Then /* Exit rc */ Leave

    'OUTPUT' line
    If rc ^= 0 Then Leave

    Parse Var line cmnd args; Upper cmnd
    Select  /*  cmnd   */
        When cmnd = "FILE"     Then Do
            Parse Var line . size from auth .
            i = i + 1;  meta.i = line
            End  /*  When  ..  Do  */
        When cmnd = "USER"     Then Do
            targets = targets || ',' || Strip(args)
            i = i + 1;  meta.i = line
            End  /*  When  ..  Do  */
        When cmnd = "NAME"     Then Do
            name = Strip(args)
            i = i + 1;  meta.i = line
            End  /*  When  ..  Do  */
        When cmnd = "TYPE" Then Do
            Parse Var line . type cc .
            i = i + 1;  meta.i = line
            End  /*  When  ..  Do  */
        When cmnd = "CLASS"    | ,
             cmnd = "FORM"     | ,
             cmnd = "RECFM"    | ,
             cmnd = "LRECL"    | ,
             cmnd = "DEST"     Then Do
            i = i + 1;  meta.i = line
            End  /*  When  ..  Do  */
        When cmnd = "DIST"     | ,
        When cmnd = "BIN"      | ,
        When cmnd = "BOX"      Then Do
            i = i + 1;  meta.i = "DIST" Strip(args)
            End  /*  When  ..  Do  */
        When cmnd = "FCB"      | ,
        When cmnd = "CTAPE"    Then Do
            i = i + 1;  meta.i = "FCB" Strip(args)
            End  /*  When  ..  Do  */
        When cmnd = "UCS"      | ,
        When cmnd = "CHARS"    | ,
        When cmnd = "TRAIN"    Then Do
            i = i + 1;  meta.i = "UCS" Strip(args)
            End  /*  When  ..  Do  */
        When cmnd = "DATE"     Then Do
            i = i + 1;  meta.i = line
         /* Parse Var line . date time tz . */
            End  /*  When  ..  Do  */
        When cmnd = "DATA"     Then Leave
        Otherwise Do
            i = i + 1;  meta.i = line
            End  /*  Otherwise Do  */
        End  /*  Select  cmnd   */

    'READTO'
    If rc ^= 0 Then Leave

    End  /*  Do  Forever   */

If rc ^= 0 Then Exit rc

'READTO'
If rc ^= 0 Then Exit rc

meta.0 = i

/*  apply canonicalization for transport  */
Select
    When type = "V" & cc = "M" Then Do
        /*  spool-to-spool (VM to VM) transfer  */
        pipe = "BLOCK 61440 CMS | 64ENCODE"
        End  /*  When .. Do  */
    When type = "A" | type = "T" Then Do
        /*  text file; send as ASCII  */
        pipe = "SPEC 1-* 1"
        End  /*  When .. Do  */
    When type = "E" Then Do
        /*  text file; send as EBCDIC  */
        pipe = "BLOCK 61440 TEXTFILE | 64ENCODE"
        End  /*  When .. Do  */
    When type = "I" | type = "B" | type = "U" Then Do
        /*  binary file (unstructured octet stream)  */
        pipe = "FBLOCK 61440 | 64ENCODE"
        End  /*  When .. Do  */
    When type = "N" Then Do
        /*  IBM NETDATA format  */
        pipe = "FBLOCK 61440 | 64ENCODE"
        End  /*  When .. Do  */
    When type = "V" Then Do
        /*  variable-length records  */
        pipe = "BLOCK 61440 CMS | 64ENCODE"
        End  /*  When .. Do  */
    Otherwise Do
        /*  all others, treat as binary  */
        pipe = "FBLOCK 61440 | 64ENCODE"
        End  /*  Otherwise Do  */
    End  /*  Select  */

/*  write the expected RFC822 stuff  */
Parse Var date mm '/' dd '/' yy
If Datatype(yy,'N') & yy < 100 Then yyyy = yy + 1900
                               Else yyyy = yy
'CALLPIPE LITERAL' "Date:        " dd mon(mm) yyyy time tz ,
    '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL' "From:        " from || '@' || localhost ,
    '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL' "Subject:      FILE" name ,
    '| *.OUTPUT.MAIL:'

Parse Var targets . ',' user ',' targets
If targets = "" Then
    'CALLPIPE LITERAL' "To:          " user || '@' || host ,
        '| *.OUTPUT.MAIL:'
Else Do
    'CALLPIPE LITERAL' "To:          " user || '@' || host || ',' ,
        '| *.OUTPUT.MAIL:'
    Parse Var targets . user ',' targets
    Do While targets ^= ""
        'CALLPIPE LITERAL' "             " user || '@' || host || ',' ,
            '| *.OUTPUT.MAIL:'
        Parse Var targets . user ',' targets
        End  /*  Do  While  */
    'CALLPIPE LITERAL' "             " user || '@' || host ,
        '| *.OUTPUT.MAIL:'
    End  /*  Else  Do  */

/*  and write the necessary MIME tags  */
'CALLPIPE LITERAL Content-Type:' "Multipart/X-SIFT;" ,
    "boundary=" || boundary '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL    | *.OUTPUT.MAIL:'

/*  identify that this is a SIFT job in "part zero"  */
'CALLPIPE COMMAND XMITMSG 9822 (APPLID UFT NOHEADER | STEM MIMEZERO.'
If rc = 16 Then Do
    loadmsg = 1
    Address COMMAND 'SET LANGUAGE (ADD UFT USER'
    'CALLPIPE COMMAND XMITMSG 9822 (APPLID UFT NOHEADER | STEM MIMEZERO.'
    End  /*  If .. Do */
If rc = 0 Then 'CALLPIPE STEM MIMEZERO. | *.OUTPUT.MAIL:'
'CALLPIPE LITERAL --' || boundary '| *.OUTPUT.MAIL:'

/*  now the file attributes (the metafile)  */
'CALLPIPE LITERAL' "Content-Type: X-SIFT/metafile" '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL' "Content-ID: META" name '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL    | *.OUTPUT.MAIL:'
'CALLPIPE STEM META. | *.OUTPUT.MAIL:'
'CALLPIPE LITERAL --' || boundary '| *.OUTPUT.MAIL:'

/*  and finally, the file contents  */
'CALLPIPE LITERAL' "Content-ID: DATA" name '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL' "Content-Type: X-SIFT/datafile"  '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL' "Content-Transfer-Encoding: BASE64" ,
    '| *.OUTPUT.MAIL:'
'CALLPIPE LITERAL       | *.OUTPUT.MAIL:'

/*  apply canonicalization for transport  */
'CALLPIPE *: |' pipe '| *.OUTPUT.MAIL:'

/*  mark end of multipart package  */
'CALLPIPE LITERAL --' || boundary || '-- | *.OUTPUT.MAIL:'

/*
If loadmsg Then Address COMMAND 'SET LANGUAGE (DEL UFT USER'
 */

Exit

/* ----------------------------------------------------------------- MON
 *  Something to return a month string in a form common to e-mail.
 */
MON: Procedure

Parse Arg mm
If ^Datatype(mm,'N') Then Return mm

/*  first try extracting month name from the message repository  */
'CALLPIPE COMMAND XMITMSG 8559 (APPLID DMS' ,
    'FORMAT' mm 'LINE 2 NOHEADER | VAR MONTH'
If rc = 0 Then Return month

/*  if that failed, then you're stuck with AMENG equivalents  */
Select  /*  mm  */
    When mm = 1  Then Return "Jan"
    When mm = 2  Then Return "Feb"
    When mm = 3  Then Return "Mar"
    When mm = 4  Then Return "Apr"
    When mm = 5  Then Return "May"
    When mm = 6  Then Return "Jun"
    When mm = 7  Then Return "Jul"
    When mm = 8  Then Return "Aug"
    When mm = 9  Then Return "Sep"
    When mm = 10 Then Return "Oct"
    When mm = 11 Then Return "Nov"
    When mm = 12 Then Return "Dec"
    Otherwise Return mm
    End  /*  Select  mm  */


