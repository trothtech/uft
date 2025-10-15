# MSG

UFT protocol `MSG` command

The `MSG` command in UFT is similar in function to the MSG command
found in IBM RSCS. Use `MSG` to send messages to users on remote
systems via the UFT servers there.

## Command Format

The format of the `MSG` command is:

    MSG user message ...

where

    user        is the user to receive the message

    message     is the plain text message to be sent

## Command Response

Success or failure may depend on whether the intended receipient user
is signed on and whether or not the user has enabled receiving messages.
A successful response from `MSG` is a 200 series ACK.

        Not all UFT implementations support MSG.

## Command Sequencing

`MSG` is not inherently related to file transfer and can therefore
be sent at any time. It may help to send a `FILE` command before `MSG`
because that can provide identification of the sending user.
A sequence like `FILE` / `MSG` / `ABORT` is a perfectly legal way
to provide the identity of the sending user.

## Usage Notes

If the sending system (client system) is running an IDENT server then
the receiving UFT server can derive the identity of the sending user
without the `FILE` / `MSG` / `ABORT` sequence.

If the receiving UFT server cannot identify the sending user
it may reject the message.


