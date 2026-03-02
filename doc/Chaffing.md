# Chaffing

UFT protocol allows "chaffing" of the transaction which may improve
privacy. When combined with encryption (SSL/TLS), chaffing in UFT may
improve security by making brute force decryption more difficult.

## Chaffing and Winnowing

Respected cryptographer Ron Rivest promoted the idea of chaffing messages
and published about it in 1998. As described, Rivest's method requires
some sort of key or a table for the recipient to know which packets
are chaff and which are desired/valid.

https://en.wikipedia.org/wiki/Chaffing_and_winnowing

## Chaffing in UFT

UFT does not implement chaffing and winnowing as described by Rivest,
but there are two commands which the sender can insert which allow
mixing "chaff" into an otherwise standard transaction.

UFT allows comments which are ignored by the receiver. <br/>
A UFT comment begins with either a hash symbol `#` or an asterisk `*`.
Comments are terminated with end-of-line (CR/LF). Comments are not
acknowledged by the receiving server; they are simply discarded.
A UFT comment can contain any printable text, even a limited number
of unprintable characters.

UFT also has a no-op command `NOP`. <br/>
`NOP` (or `NOOP`) is a command which does nothing, but which is (like all commands)
acknowledged with a 200 series response, meaning "success". The arguments
to a `NOP` are ignored and can contain the same content as a comment.

While this means of chaffing appears to be less robust than Rivest's,
it does not require a key or table or shift of the protocol.

## Winnowing in UFT

Removing chaff from a UFT transaction is automatic. <br/>
Any properly designed UFT server will silently discard all comments
and will correctly acknowledged `NOP` commands. When `NOP` and/or comments
are used by the sender for chaffing, the chaff is naturally removed
as part of the transaction.

When a UFT transaction with chaffing is wrapped in encryption,
the whole is more difficult for an attacker to crack.


