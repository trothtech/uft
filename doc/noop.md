# NOOP

UFT protocol `NOOP` command

Use the `NOOP` command to safely do nothing.

Many programming languages, including most machine languages,
have a no-op or "no operation" instruction or command or statement.
Often the no-op is just a place holder (good for zapping systems later).
In some caes the no-op is used as a timing hack.

UFT protocol has a `NOOP` command which serves the same purpose
and may be useful in security context. It may also facilitate testing.

## Command Format

The format of the `NOOP` command is:

    NOOP random-argument-string

where

    random-argument-string

is an optional string of one or more arguments.
All arguments are ignore because `NOOP` does nothing.

## Command Response

The `NOOP` command responds with an ACK (200 series) in all cases.

## Example Usage

The `NOOP` command can be used in testing to confirm proper ACK/NAK
handshake between the UFT server and the UFT client.

In security context where encryption is involved, it is often useful
to salt the payload with "chaff", extra information which is not meaningful
to the conversation or transaction. Such a payload is considered to be
more difficult to analyze and therefore more difficult for an attacker
to crack. The `NOOP` statement in UFT serves this purpose.


