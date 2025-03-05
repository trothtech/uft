/* © Copyright 1992-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: UFTXINMR REXX
 *              Unsolicited File Transfer common INMR stage
 *              Generates an INMR deck using INMR123
 *              which is supplied with CMS Pipelines.
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, Texas (METRO)
 *        Date: 1992-Nov-07, 1993-Feb-22, Oct-08
 *
 *        Note: UFTXINMR is not a user-level pipelines stage
 */

server = 0
Parse Upper Arg send fn ft fm to user at node . '(' . ')' .

If send ^= "SEND" Then Do
    Address "COMMAND" 'XMITMSG 385 SEND (ERRMSG'
    Exit 24
    End  /*  If  ..  Do  */
If to ^= "TO" Then Do
    Address "COMMAND" 'XMITMSG 70 TO (ERRMSG | CONSOLE'
    Exit 24
    End  /*  If  ..  Do  */
If at ^= "AT" Then Do
    Address "COMMAND" 'XMITMSG 70 AT (ERRMSG | CONSOLE'
    Exit 24
    End  /*  If  ..  Do  */

/*  If we're running as UFTD,  then make up some numbers  */
If server Then ls = fn ft fm "V 65535 0 0" Date('U') Time()

/*  If file is disk resident,  discover some characteristics  */
Else Do
    'CALLPIPE CMS LISTFILE' fn ft fm '(SHORTDATE | VAR RS | DROP | VAR LS'
    If rc ^= 0 Then Do
        Say rs
        Exit rc
        End  /*  If  ..  Do  */
    End  /*  If  ..  Do  */

'ADDPIPE *.OUTPUT: | BLOCK 255 NETDATA | *.OUTPUT:'
Parse Var ls fn ft fm f l r b date time .
'CALLPIPE INMR123' node user fn ft fm ,
    f l r b date time '(NOSPOOL | *:'
If server Then 'CALLPIPE *: | SPEC x00 1 1-* NEXT | *:'
          Else 'CALLPIPE <' fn ft fm '| SPEC x00 1 1-* NEXT | *:'
'OUTPUT' '20'x || "INMR06"

Exit

