/*
 *        Name: UFTCUSER REXX
 *              inserts USER record in the SIFT job
 *              Sender-Initiated File Transfer
 *      Author: Rick Troth, Rice University, Information Systems
 *              Rick Troth, Houston, TX (METRO)
 *        Date: 1993-Feb-20, Oct-20
 *
 *        Note: UFTCUSER is not a user-level pipeline stage.
 */

Parse Arg user . '(' .

If user = "" Then Do
    Address "COMMAND" 'XMITMSG 386'
    Exit 24
    End  /*  If  ..  Do  */

Do Forever
    'PEEKTO RECORD'
    'OUTPUT' record
    If Left(record,1) ^= '*' Then Leave
    End  /*  Do  Forever  */

/*  identify this stage to the job stream  */
'OUTPUT' "*UFTCUSER: sending to" user

/*  strip-off any trailing  "@host.domain"  part  */
Parse Var user user '@' .

'OUTPUT' "USER" user
If rc ^= 0 Then Exit rc * (rc ^= 12)

'SHORT'

Exit rc

