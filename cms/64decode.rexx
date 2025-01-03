/* 64DECODE REXX: CMS Pipelines filter to decode Base64 (MIME) data  */

/*

 RFC 1521                          MIME                    September 1993

 5.2.  Base64 Content-Transfer-Encoding

    The Base64 Content-Transfer-Encoding is designed to represent
    arbitrary sequences of octets in a form that need not be humanly
    readable.  The encoding and decoding algorithms are simple, but the
    encoded data are consistently only about 33 percent larger than the
    unencoded data.  This encoding is virtually identical to the one used
    in Privacy Enhanced Mail (PEM) applications, as defined in RFC 1421.
    The base64 encoding is adapted from RFC 1421, with one change: base64
    eliminates the "*" mechanism for embedded clear text.

    A 65-character subset of US-ASCII is used, enabling 6 bits to be
    represented per printable character. (The extra 65th character, "=",
    is used to signify a special processing function.)

       NOTE: This subset has the important property that it is
       represented identically in all versions of ISO 646, including US
       ASCII, and all characters in the subset are also represented
       identically in all versions of EBCDIC.  Other popular encodings,
       such as the encoding used by the uuencode utility and the base85
       encoding specified as part of Level 2 PostScript, do not share
       these properties, and thus do not fulfill the portability
       requirements a binary transport encoding for mail must meet.

    The encoding process represents 24-bit groups of input bits as output
    strings of 4 encoded characters. Proceeding from left to right, a
    24-bit input group is formed by concatenating 3 8-bit input groups.
    These 24 bits are then treated as 4 concatenated 6-bit groups, each
    of which is translated into a single digit in the base64 alphabet.
    When encoding a bit stream via the base64 encoding, the bit stream
    must be presumed to be ordered with the most-significant-bit first.
    That is, the first bit in the stream will be the high-order bit in
    the first byte, and the eighth bit will be the low-order bit in the
    first byte, and so on.

    Each 6-bit group is used as an index into an array of 64 printable
    characters. The character referenced by the index is placed in the
    output string. These characters, identified in Table 1, below, are
    selected so as to be universally representable, and the set excludes
    characters with particular significance to SMTP (e.g., ".", CR, LF)
    and to the encapsulation boundaries defined in this document (e.g.,
    "-").

                             Table 1: The Base64 Alphabet

       Value Encoding  Value Encoding  Value Encoding  Value Encoding
            0 A            17 R            34 i            51 z
            1 B            18 S            35 j            52 0
            2 C            19 T            36 k            53 1
            3 D            20 U            37 l            54 2
            4 E            21 V            38 m            55 3
            5 F            22 W            39 n            56 4
            6 G            23 X            40 o            57 5
            7 H            24 Y            41 p            58 6
            8 I            25 Z            42 q            59 7
            9 J            26 a            43 r            60 8
           10 K            27 b            44 s            61 9
           11 L            28 c            45 t            62 +
           12 M            29 d            46 u            63 /
           13 N            30 e            47 v
           14 O            31 f            48 w         (pad) =
           15 P            32 g            49 x
           16 Q            33 h            50 y

    The output stream (encoded bytes) must be represented in lines of no
    more than 76 characters each.  All line breaks or other characters
    not found in Table 1 must be ignored by decoding software.  In base64
    data, characters other than those in Table 1, line breaks, and other
    white space probably indicate a transmission error, about which a
    warning message or even a message rejection might be appropriate
    under some circumstances.

    Special processing is performed if fewer than 24 bits are available
    at the end of the data being encoded.  A full encoding quantum is
    always completed at the end of a body.  When fewer than 24 input bits
    are available in an input group, zero bits are added (on the right)
    to form an integral number of 6-bit groups.  Padding at the end of
    the data is performed using the '=' character.  Since all base64
    input is an integral number of octets, only the following cases can
    arise: (1) the final quantum of encoding input is an integral
    multiple of 24 bits; here, the final unit of encoded output will be
    an integral multiple of 4 characters with no "=" padding, (2) the
    final quantum of encoding input is exactly 8 bits; here, the final
    unit of encoded output will be two characters followed by two "="
    padding characters, or (3) the final quantum of encoding input is
    exactly 16 bits; here, the final unit of encoded output will be three
    characters followed by one "=" padding character.

    Because it is used only for padding at the end of the data, the
    occurrence of any '=' characters may be taken as evidence that the
    end of the data has been reached (without truncation in transit).  No
    such assurance is possible, however, when the number of octets
    transmitted was a multiple of three.

    Any characters outside of the base64 alphabet are to be ignored in
    base64-encoded data.  The same applies to any illegal sequence of
    characters in the base64 encoding, such as "====="

    Care must be taken to use the proper octets for line breaks if base64
    encoding is applied directly to text material that has not been
    converted to canonical form.  In particular, text line breaks must be
    converted into CRLF sequences prior to base64 encoding. The important
    thing to note is that this may be done directly by the encoder rather
    than in a prior canonicalization step in some implementations.

       NOTE: There is no need to worry about quoting apparent
       encapsulation boundaries within base64-encoded parts of multipart
       entities because no hyphen characters are used in the base64
       encoding.
*/

Signal On Error
Signal On Novalue
Signal On Syntax


Parse Upper Arg text .                /* Keyword "TEXT" is allowed.  */


If text == 'TEXT'                     /* Is it text or binary?       */

   Then reformat =,                   /* Text, so reformat:          */
      'xlate 1-* from 819 to 1047 |', /*   Translate to EBCDIC.      */
      'deblock crlf terminate |'      /*   Deblock.                  */

Else reformat = ''                    /* Binary, so leave it alone.  */


MIMEtab =,                            /* Define translate table.     */
   'A 00   B 01   C 02   D 03   E 04   F 05   G 06   H 07',
   'I 08   J 09   K 0A   L 0B   M 0C   N 0D   O 0E   P 0F',
   'Q 10   R 11   S 12   T 13   U 14   V 15   W 16   X 17',
   'Y 18   Z 19   a 1A   b 1B   c 1C   d 1D   e 1E   f 1F',
   'g 20   h 21   i 22   j 23   k 24   l 25   m 26   n 27',
   'o 28   p 29   q 2A   r 2B   s 2C   t 2D   u 2E   v 2F',
   'w 30   x 31   y 32   z 33   0 34   1 35   2 36   3 37',
   '4 38   5 39   6 3A   7 3B   8 3C   9 3D   + 3E   / 3F'


'CALLPIPE (endchar ? name 64DECODE)', /* Decode Base64 input:        */
      '*: |',                         /* Read from the pipeline.     */
      'xlate 1-* 00-FF !' MIMEtab'|', /* Convert from printable.     */
      'change /!// |',                /* Ignore "=" and illegals.    */
      'fblock 76 |',                  /* Make sure multiples of 4.   */
   'l: locate 76 |',                  /* Divert if not full block.   */
      'vchar 8 6 |',                  /* Compress 4 to 3.            */
   'f: fanin |',                      /* Bring back short last line. */
       reformat,                      /* Possibly xlate and reblock. */
      '*:',                           /* Write to the pipeline.      */
'?',
   'l: |',                            /* Here if short block.        */
      'fblock 4 |',                   /* Split into 4-byte chunks.   */
   'm: locate 4 |',                   /* Divert short last chunk.    */
      'vchar 8 6 |',                  /* Compress 4 to 3.            */
   'i: faninany |',                   /* Merge in last chunk.        */
      'join * |',                     /* Rejoin into single record.  */
   'f:',                              /* Route back to mainline.     */
'?',
   'm: |',                            /* Here if 2 or 3 bytes.       */
   'n: locate 3 |',                   /* Divert if not 3 bytes.      */
      'vchar 8 6 |',                  /* Compress 4 to 3.            */
      'chop 2 |',                     /* Third byte is not data.     */
   'i:',                              /* Join to rest of last record.*/
'?',
   'n: |',                            /* Here if only two bytes.     */
      'vchar 8 6 |',                  /* Compress 4 to 3.            */
      'chop 1 |',                     /* Second byte is not data.    */
   'i:'                               /* Join to rest of last record.*/


Error: Exit RC*(RC<>12)               /* RC = 0 if end-of-file.      */


/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*                        Error Subroutines                          */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

Novalue:

Say 'Novalue condition for symbol' Condition('D') 'on line' ,
   Sigl':' Sourceline(Sigl)

Exit 20                               /* Exit with error setting.    */


Syntax:

Say 'Syntax error on line' Sigl':' Sourceline(Sigl)
Say 'Error was:' Errortext(RC)

Exit 24                               /* Exit with error setting.    */
