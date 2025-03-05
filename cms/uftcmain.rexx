/* © Copyright 1993-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTCMAIN REXX (uftcmain.rexx)
 *    Formerly: UFTCHOST REXX and UFTCUSER REXX
 *              Unsolicited File Transfer client protocol stage.
 *              Reads a SIFT job, inserts USER record if needed,
 *              and directs it via UFT (over TCP), NJE (via RSCS),
 *              or as SIFT (MIME mail).
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas (METRO)
 *        Date: 1993-Feb-20, Oct-20
 *              2025-02-11
 *
 *       Types: ABC-EF--I----N------UV---- supported
 *              ---D---H----M---------WX-- defined
 *              ------G--JKL--OPQRST----YZ reserved
 */

/* identify this stage to itself */
Parse Source . . arg0 .
argo = arg0 || ':'
argl = '*' || argo

Parse Arg user . '(' . ')' .
Parse Var user _u1 '@' _h1

/* define a stream for passing SIFT job to transport stage */
'ADDSTREAM OUTPUT SIFT'
If rc ^= 0 Then Exit rc

'PEEKTO'        /* verify existence of input stream */
If rc ^= 0 Then Do
    Address COMMAND 'XMITMSG 2914 (ERRMSG'
    /* "no input stream" or "too few input streams" */
    Exit rc
    End /* If .. Do */

/* an index for the meta. stem */
i = 0
file = ""
user = ""

/* consume the metafile */
Do Forever

    /* get a record without consuming it */
    'PEEKTO RECORD'
    If rc ^= 0 Then Leave

    /* strip excesse blanks and parse it */
    record = Strip(record)
    Parse Upper Var record verb .

    Select /* verb */

        When Abbrev("FILE",verb,1) Then file = record
        When Abbrev("USER",verb,1) Then Parse Var record . user .
        When Abbrev("DATA",verb,1) Then Leave

        Otherwise Do
            /* collect this meta record */
            i = i + 1
            meta.i = record
            End /* Otherwise Do */

        End /* Select verb */

    /* now consume the record */
    'READTO'

    End /* Do Forever */

If rc ^= 0 Then Exit rc
'READTO'
If rc ^= 0 Then Exit rc

/* make it a proper stem variable */
meta.0 = i

/* parse the USER value into USER and HOST */
Parse Var user _u2 '@' _h2
user = _u1;  If user = "" Then user = _u2
host = _h1;  If host = "" Then host = _h2

/* confirm user on command line or in job */
If user = "" Then Do
    Address COMMAND 'XMITMSG 387 "USER" (ERRMSG'
    Exit 24
    End /* If .. Do */
user = user || '@' || host

/* switch to SIFT output stream */
'SELECT OUTPUT SIFT'
If rc ^= 0 Then Exit rc

/* select delivery stage */
Select /* host */

    When host = "" Then Do
        'ADDPIPE (END !) *.OUTPUT.SIFT: | UFTCLCL' user ,
            '| Q: FANINANY | *.OUTPUT.0: ! *.OUTPUT.0: | Q:'
        If rc ^= 0 Then Exit rc
        'OUTPUT' file
        End /* When .. Do */
    When Index(host,'.') = 0 & Userid() ^= "RSCS" Then Do
        'ADDPIPE (END !) *.OUTPUT.SIFT: | UFTCRSCS' user ,
            '| Q: FANINANY | *.OUTPUT.0: ! *.OUTPUT.0: | Q:'
        If rc ^= 0 Then Exit rc
        'OUTPUT' file
        End /* When .. Do */
    Otherwise Do
        Trace "OFF"
        /* First, try using TCP directly. If that   *
         * does't work, then try sending it as mail */
        'ADDPIPE (END !) *.OUTPUT.SIFT: | UFTCTCP' user ,
            '| Q: FANINANY | *.OUTPUT.0: ! *.OUTPUT.0: | Q:'
        If rc ^= 0 Then Exit rc
        'OUTPUT' file
        If rc = 12 Then Do
            'SEVER OUTPUT'
/* need to reset the aggregate return code here!! */
            'ADDPIPE (END !) *.OUTPUT.SIFT: | UFTCMAIL' user ,
                '| Q: FANINANY | *.OUTPUT.0: ! *.OUTPUT.0: | Q:'
            If rc ^= 0 Then Exit rc
            'OUTPUT' file
            End  /* If .. Do */
        End  /* Otherwise Do */

    End /* Select host */

/* check return code writing "FILE" command */
If rc ^= 0 Then Exit rc

/* identify this stage to the job stream */
'OUTPUT' argl "sending to" user
If rc ^= 0 Then Exit rc

/* [re]place the USER command into the job */
Parse Var user user '@' .
'OUTPUT' "USER" user
If rc ^= 0 Then Exit rc

/* now the rest of the meta-data */
'CALLPIPE STEM META. | *:'
If rc ^= 0 Then Exit rc

/* mark start of file body */
'OUTPUT' "DATA"
If rc ^= 0 Then Exit rc

/* and now pass the rest as-is */
'CALLPIPE *: | *:'
If rc ^= 0 Then Exit rc

Exit


