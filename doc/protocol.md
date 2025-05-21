# UFT Protocol

This is an overview of Unsolicited File Transfer (UFT) protocol
in markdown format.

UFT follows a client/server model.
The UFT client connects on TCP port 608 to the UFT server.
As soon as the connection is accepted, the UFT server sends a herald.
The UFT client then sends commands, ending with `QUIT`.
The UFT server responds to each command with a numeric code indicating
acknowledgement (success) or negative acknowledgement (error or failure).


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

`FILE` (or `PIPE`), `USER`, `TYPE`, and `DATA` are essential.

Note that `TYPE` might seem to be a meta command,
but it remains a primary command because canonicalization is a
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
* server acknowledges with a 300 code, "feed me more" (the data)

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

These are optional and not widely implemented.
`CPQ` and `MSG` are specifically for emulating IBM "NJE" networking.

## UFT Meta Verbs

* META NAME
* META DATE
* META XDATE
* META TITLE
* META OWNER
* META GROUP
* META XPERM
* META VERSION
* META RECFMT
* META RECLEN (or META LRECL)
* META CLASS
* META FORM
* META HOLD
* META COPY (or META COPIES)
* META FCB
* META UCS
* META DEST
* META DIST
* META SEQ
* META NOTIFY

All of the above were originally implemented without the `META` prefix
and there remain implementations which do not require `META` for these
attribute commands. It is best for a server to tolerate either form.
But new attributes or similar meta-data *must* be prefixed with `META`
for clarity and for simplifying implementations.

## UFT Response Codes

UFT response code numbers are unique. <br/>
Any single response code can be mapped to a specific condition, status,
or meaning and can thus be accurately mapped to human-readable text
in national language or regional dialect.

UFT response codes are three-digit integers.
The range of the code indicates any of a half dozen conditions.

| range      | indication                                                |
| ---------- | --------------------------------------------------------- |
| 100s range | a spontaneous response from the server, an "info" message |
| 200s range | acknowledgment (ACK)                                      |
| 300s range | more input required                                       |
|            | the client must supply additional information or data     |
| 400s range | temporary error (NAK), a client error                     |
| 500s range | permanent error (NAK), a server error (not always terminal)   |
| 600s range | a required response from the server                       |
|            | contrast with 100s range spontaneous responses            |

When the server receives a connection, it immediately presents a
222 messages as the "herald". When the client sends `QUIT`, the server
replies with a 221 message and closes the connection. Both 222 and 221
are success indications.

## UFT File Types (canonicalizations)

| type       | canonicalization or translation                           |
| ---------- | --------------------------------------------------------- |
|  `TYPE A`  | ASCII, Internet plain text with CR/LF delimited lines     |
|            | (0x0D/0x0A, see NVT format), alias `TYPE T`               |
|  `TYPE I`  | IMAGE, image (binary), alias `TYPE B`                     |
|            | above two types are in keeping with FTP semantics         |
|  `TYPE N`  | NETDATA, an IBM encoding                                  |

Types `A` and `I` are mandatory.

Type `N` is recommended, at least on receive, if you will be
receiving files from IBM mainframe systems such as z/VM or z/OS.

The following types are rarely used but defined for the sake of
developers and experimenters.

| type       | canonicalization or translation                           |
| ---------- | --------------------------------------------------------- |
|  `TYPE V`  | variable length record-oriented                           |
|            | each record is preceeded by a 16-bit length in network byte order |
|  `TYPE E`  | EBCDIC, IBM mainframe plain text                          |
|            | with EBCDIC NL delimited lines (0x15)                     |
|  `TYPE M`  | "mail", an RFC 822 message (canonicalized as `TYPE A`)    |
|            | for those UFT server implementations which support it     |

Any type which a receiver does not recognize or implement should be
treated as `TYPE I` or something which saves the data stream as-is.


