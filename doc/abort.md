# ABORT

UFT protocol `ABORT` command

This command signals an abort of the current transaction
and indicates that the current file should be abandoned.

## Command Format

The format of the `ABORT` command is:

    ABORT

The `ABORT` command has no parameters, arguments, or options.

## Command Response

The successful response to `ABORT` is a 200 ACK.

While it is difficult to imaging a situation where `ABORT`
woule either fail or would be rejected, if somehow that happens,
the response would be a 500 NAK (server error).

## Command Sequencing

`ABORT` can come any time after a `FILE` command.


