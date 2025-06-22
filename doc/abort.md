# ABORT

UFT protocol ABORT command

## Command Format

The format of the `ABORT` command is:

    ABORT

The `ABORT` command has no parameters, arguments, or options.

## Command Response

The successful response to `ABORT` is a 200 ACK.

While it is difficult to imaging a situation where `EOF`
woule either fail or would be rejected, if somehow that happens,
the response would be a 500 NAK.

## Command Sequencing

`ABORT` can come any time after a `FILE` command.


