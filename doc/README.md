# UFT

Unsolicited File Transfer

This is the documentation folder.

## UFT Primary Control Verbs

* FILE
* PIPE
* USER
* TYPE
* DATA
* AUXDATA
* EOF
* ABORT
* META
* QUIT

`FILE`, `USER`, `TYPE`, and `DATA` are mandatory.

Note that `TYPE` might seem to be a meta command,
but it remains a primary command because canonization is a
central feature of UFT.

A sample sequence might be ...
* client connects
* server sends the herald, a 200 code
* client sends `FILE` *size* *from* *auth*
* server acknowledges with a 200 code
* client sends `USER` *recipient*
* server acknowledges with a 200 code
* client sends `TYPE A` indicating a plain text file
* server acknowledges with a 200 code
* client sends `DATA 12345`
* server acknowledges with a 300 code, "feed me more"

The server switches to "data mode" to consume the next 12345 bytes
sent from the client.

* client sends 12345 bytes of file content
* server acknowledges with a 200 code and switches back to "command mode"
* client sends `EOF`
* server acknowledges with a 200 code
* client sends `QUIT`
* server acknowledges with a 200 code and closes its end of the connection
* client closes its end of the connection

The above transaction uses 6 primary commands and does not require any
meta commands nor secondary commands. Simplicity is the first objective.

## UFT Supplemental Commands

* CPQ
* MSG
* AGENT
* HELP
* NOOP

## UFT Meta Verbs

* META NAME
* META OWNER
* META GROUP
* META DATE
* META XDATE
* META PROT
* META XPERM
* META RECFMT
* META RECLEN
* META VERSION
* META CLASS
* META COPY (or META COPIES)
* META FORM
* META HOLD
* META DEST
* META DIST
* META SEQ
* META FCB
* META UCS
* META TITLE
* META NOTIFY

## Response Codes

| range      | indication                                                |
| ---------- | --------------------------------------------------------- |
| 100s range | a spontaneous response from the server, an "info" message |
| 200s range | acknowledgment (ACK)                                      |
| 300s range | more input required                                       |
|            | the client must supply additional information or data     |
| 400s range | temporary error (NAK), typically a client error           |
| 500s range | permanent error (NAK), typically a server error           |
| 600s range | a required response from the server                       |
|            | contrast with 100s range spontaneous responses            |

UFT response codes are unique, but the first digit indicates basic
success/failure and flow control.

## UFT File Types (canonizations)

| type       | canonization or translation                             |
| ---------- | ------------------------------------------------------- |
|  `TYPE A`  | ASCII, Internet plain text with CR/LF delimited lines   |
|            | (0x0D/0x0A, see NVT format), alias `TYPE T`             |
|  `TYPE I`  | IMAGE, image (binary), alias `TYPE B`                   |
|            | above two types are in keeping with FTP semantics       |
|  `TYPE N`  | NETDATA, an IBM encoding                                |

Types `A` and `I` are mandatory.

Type `N` is recommended, at least on receive, if you will be
sending files to/from IBM mainframe systems such as z/VM or z/OS.

The following types are rarely used but defined for the sake of
developers and experimenters.

| type       | canonization or translation                             |
| ---------- | ------------------------------------------------------- |
|  `TYPE V`  | variable length record-oriented                         |
|            | each record is preceeded by a 16-bit length in network byte order |
|  `TYPE E`  | EBCDIC, IBM mainframe plain text                        |
|            | with EBCDIC NL delimited lines (0x15)                   |
|  `TYPE M`  | "mail", an RFC 822 message                              |
|            | for those UFT server implementations which support it   |

Any type which a receiver does not recognize or implement should be
treated as `TYPE I` or something which saves the data stream as-is.


