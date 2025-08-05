# EOF

UFT protocol `EOF` command (end of file)

This command signals the end of the current file. <br/>
The UFT clients send an `EOF` command when a file has been sent
in its entirety. The server closes all intermediate streams and/or files
on the receiving end and makes the file available to the recipient.

## Command Format

The format of the `EOF` command is:

    EOF

The `EOF` command has no parameters, arguments, or options.

## Command Response

`EOF` should result in a 200 ACK.

If there is an error, it would be a server-side condition
and therefore would generate a 500 NAK.

Any file in progress should be properly marked as complete by the
presence of `EOF`. A following `QUIT` command signals the server to
close the TCP stream. `QUIT` preceeding `EOF` is not recommended.

## Command Sequencing

`EOF` must come after all `DATA` statements
and should come before `QUIT`.


