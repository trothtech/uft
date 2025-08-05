# QUIT

UFT protocol `QUIT` command

UFT clients should send a `QUIT` command to finish a transaction.
Any file-in-transit should be properly marked as complete by the
presence of a preceding `EOF` command. `QUIT` signals the server
to close the TCP stream.

## Command Format

The format of the `QUIT` command is:

    QUIT

The QUIT command has no parameters, arguments, or options.

## Command Response

The response to `QUIT` is 221 (an ACK)
and the server then closes its end of the connection.

## Command Sequencing

`QUIT` should follow `EOF`.

In absense of a proper `EOF`,
`QUIT` clearly indicates that `EOF` effects should be applied.


