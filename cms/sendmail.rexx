/* Copyright 1994, Richard M. Troth, all rights reserved.    <plaintext>
 *
 *        Name: SENDMAIL REXX
 *              Pass a RC 822 conformant text stream to the MAILER.
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas, USA
 *        Date: 1993-Nov-05, 14
 *        Note: This stage is supposed to function quite
 *              like the UNIX equivalent,  /usr/lib/sendmail.
 *              Not all of that versions features are implemented.
 *              In fact,  the only one that really works is
 *
 *                      sendmail -t
 */

mailer = "MAILER"
smtp = "SMTP"

/*  identify self and RSCS via IDENTIFY  */
'CALLPIPE COMMAND IDENTIFY | VAR IDENTITY'
If rc ^= 0 Then Exit rc
Parse Var identity userid . nodeid . rscsid . '15'x .
optuid = "$" || userid

/*  find an available virtual address  */
Address CMS 'GETFMADR'
If rc ^= 0 Then Exit rc
Parse Pull . . va .

/*  parse command line arguments and options  */
Parse Arg target
target = Strip(target)
If Left(target,1) = "-" Then opts = ""
                        Else Parse Var target target '(' opts ')' .
Address COMMAND 'GLOBALV SELECT' optuid 'GET SENDMAIL'
opts = sendmail opts            /*  string together with defaults  */
Upper opts

/*  set some default defaults  */
method = "RFC822"   /*  or BSMTP or SMTP or RSCS  */
server = ""
device = "PRT"
class = "M"
form = "QU-MAIL"
type = "MAIL"

/*  process options  */
Do While opts ^= ""
    Parse Var opts op opts
    Select  /*  op  */
        When Abbrev("RFC822",op,2) Then method = "RFC822"
        When Abbrev("BSMTP",op,1)  Then method = "BSMTP"
        When Abbrev("SMTP",op,2)   Then method = "SMTP"
        When Abbrev("RSCS",op,1)   Then method = "RSCS"
        When Abbrev("DEVICE",op,1) Then Parse Var opts device opts
        When Abbrev("SERVER",op,2) Then Parse Var opts server opts
        When Abbrev("VIA",op,3)    Then Parse Var opts server opts
        When Abbrev("CLASS",op,2)  Then Parse Var opts class opts
        When Abbrev("FORM",op,1)   Then Parse Var opts form opts
        When Abbrev("TYPE",op,1)   Then Parse Var opts type opts
        Otherwise Do
            Address COMMAND 'XMITMSG 3 OP (ERRMSG'
            Exit 24
            End  /*  Otherwise Do  */
        End  /*  Select op  */
    End  /*  Do While  */

/*  process options,  UNIX style  */
Do While Left(target,1) = "-"
    Parse Var target op target
    Upper op
    Select  /*  op  */
        When Abbrev("-RFC822",op,5) Then method = "RFC822"
        When Abbrev("-BSMTP",op,6)  Then method = "BSMTP"
        When Abbrev("-SMTP",op,5)   Then method = "SMTP"
        When Abbrev("-RSCS",op,5)   Then method = "RSCS"
        When Abbrev("-DEVICE",op,4) Then Parse Var target device target
        When Abbrev("-SERVER",op,3) Then Parse Var target server target
        When Abbrev("-VIA",op,4)    Then Parse Var target server target
        When Abbrev("-CLASS",op,3)  Then Parse Var target class target
        When Abbrev("-FORM",op,3)   Then Parse Var target form target
        When Abbrev("-TYPE",op,3)   Then Parse Var target type target
        When Abbrev("-T",op,2)      Then nop
        Otherwise Do
            Address COMMAND 'XMITMSG 3 OP (ERRMSG'
            Exit 24
            End  /*  Otherwise Do  */
        End  /*  Select op  */
    End  /*  Do While  */

/*  be sure that we actually have something to send  */
'PEEKTO'
If rc ^= 0 Then Exit rc * (rc ^= 12)

/*  match-up sub-parameters with method  */
Select  /*  method  */

    When method = "RFC822" Then Do
        If server = "" Then server = mailer
        If device = "" Then device = "PRT"
        End  /*  When .. Do  */

    When method = "BSMTP" Then Do
        If server = "" Then server = smtp
        If device = "" Then device = "PRT"
        End  /*  When .. Do  */

    When method = "SMTP" Then Do
        server = "";    device = ""
        End  /*  When .. Do  */

    When method = "RSCS" Then Do
        If server = "" Then server = rscsid
        If device = "" Then device = "PRT"
        End  /*  When .. Do  */

    End  /*  Select method  */

/*  will this involve another v-machine by SPOOL?  */
If device ^= "" Then Do

    Select  /*  device  */
        When Abbrev("PUNCH",device,2) | Abbrev("PCH",device,3) Then ,
            opcode = "x41"
        When Abbrev("VAFP",device,1) | Abbrev("AFP",device,3) Then Do
            device = "PRT"
            opcode = "x5A"
            End  /*  When  ..  Do  */
        When Abbrev("PRINT",device,2) | Abbrev("PRT",device,3) Then ,
            opcode = "x09"
        End  /*  Select  device  */

    /*  define a temporary device  */
    Parse Value Diagrc(08,'DEFINE' device va) ,
        With 1 rc 10 . 17 rs '15'x .
    If rc ^= 0 Then Do
        Say rs
        Exit rc
        End  /*  If  ..  Do  */

    /*  set SPOOLing parameters  */
    Parse Value Diagrc(08,'SPOOL' va 'TO' server) ,
        With 1 rc 10 . 17 rs '15'x .
    If rc ^= 0 & server = "MAILER" & method = "RFC822" Then Do
        server = smtp
        method = "BSMTP"
        Parse Value Diagrc(08,'SPOOL' va 'TO' server) ,
            With 1 rc 10 . 17 rs '15'x .
        End  /*  If .. Do  */
    If rc ^= 0 Then Do
        Say rs
        Exit rc
        End  /*  If  ..  Do  */

    Parse Value Diagrc(08,'SPOOL' va 'CLASS' class) ,
        With 1 rc 10 . 17 rs '15'x .
    If rc ^= 0 Then Do
        Say rs
        Exit rc
        End  /*  If  ..  Do  */

    Parse Value Diagrc(08,'SPOOL' va 'FORM' form) ,
        With 1 rc 10 . 17 rs '15'x .
    If rc ^= 0 Then Do
        Say rs
        Exit rc
        End  /*  If  ..  Do  */

    Parse Value Diag(08,'SPOOL' va 'NOMSG') With .

    End  /*  If .. Do  */

/*
 *  This is the point of no return.   Up 'til now we haven't consumed
 *  any of our input stream,  leaving it available to other use(s)
 *  should the caller decide to do so.   But this is about to change:
 */

/*  made it this far,  now eat the header  */
i = 0
Do Forever

    'READTO RECORD'
    If rc ^= 0 Then Leave
    If Strip(record) = "" Then Leave
    If Left(record,72) = Copies('=',72) Then Leave
    i = i + 1
    head.i = record
    record = Translate(record,' ','05'x)
    If Left(record,1) = ' ' Then val = Strip(record)
                            Else Parse Var record tag ':' val
    Upper tag
    If tag = "BCC" & method ^= "RFC822" Then i = i - 1
    If tag = "TO" | tag = "CC" | tag = "BCC" Then Do While val ^= ""
        If Left(Strip(val),1) = '"' Then Parse Var val . '"' . '"' val
        Parse Var val u ',' val
        If Index(u,'<') > 0 Then Parse Var u . '<' u '>' .
                            Else Parse Var u u '(' . ')' .
        target = target u
        End  /*  If .. Do While  */

    End  /*  Do Forever  */

/*  tack on the obligatory empty line  */
i = i + 1
head.i = ""
head.0 = i

/*  do we need a BSMTP/SMTP envelope?  */
If method = "BSMTP" | method = "SMTP" Then Do

    smtp.1 = "HELO" hostname()
    smtp.2 = "MAIL FROM: <" || Userid() || '@' || hostname() || ">"
    i = 3
    Do While target ^= ""
        Parse Var target to target
        smtp.i = "RCPT TO: <" || to || ">"
        i = i + 1
        End  /*  Do While  */
    smtp.i = "DATA"
    smtp.0 = i

    tail.1 = "."
    tail.2 = "QUIT"
    tail.0 = 2

    End  /*  If .. Do  */

Else Do; smtp.0 = 0; tail.0 = 0; End

/*  feed this to the transfer agent  */
'CALLPIPE STEM SMTP. | PAD 1 | SPEC' opcode '1 1-* NEXT | URO' va
If rc ^= 0 Then Exit rc
'CALLPIPE STEM HEAD. | PAD 1 | SPEC' opcode '1 1-* NEXT | URO' va
If rc ^= 0 Then Exit rc
'CALLPIPE *:         | PAD 1 | SPEC' opcode '1 1-* NEXT | URO' va
If rc ^= 0 Then Exit rc
'CALLPIPE STEM TAIL. | PAD 1 | SPEC' opcode '1 1-* NEXT | URO' va
If rc ^= 0 Then Exit rc

/*  if we used a temporary virtual URO device,  get rid of it now  */
If device ^= "" Then Do
    Parse Value Diag(08,'CLOSE' va 'NAME' Userid() type) ,
        With 1 rc 10 . 17 rs '15'x .
    Parse Value Diag(08,'DETACH' va) ,
        With 1 rc 10 . 17 rs '15'x .
    End  /*  If .. Do  */

Exit


