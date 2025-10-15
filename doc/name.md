# [META] NAME

UFT protocol `NAME` command

A UFT client uses the `NAME` command to name the file being sent.

Note that a filename is optional in UFT, unlike FTP and other protocols,
because, while files are named in normal use, a file being sent
might be the output of a command (and thus not yet named) or might be
intended for printing (where the name is not really applicable).

## Command Format

The format of the FILE command is:

    [META] NAME filename

where

    filename   is the name of the file about to be sent

Quoting of special characters is not discussed.

The `[META]` prefix is explicitly optional for the `NAME` command.

## Command Response

`NAME` should return a 200 series ACK unless the filename provided
cannot be represented, and cannot be reliably converted, on the
receiving system. If the name cannot be applied, the result is a
500 series NAK (server error).

While a name is not required for the file being sent,
the `NAME` command must have an argument. If no argument is provided
the result is a 400 series NAK (client error). In plain language,
you cannot use `NAME` without an argument remove a name from a UFT file.

## Command Sequencing

`NAME` is a meta command, even if not prefixed with "META" 
and therefore must follow `TYPE` and preceed `DATA`.


