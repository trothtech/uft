# [META] XPERM

UFT protocol `XPERM` command

This meta command indicates the Unix-style permission bits
of the file as it appears on the sending side.

The value is an octal representation of the Unix filesystem
permission bits in effect on the file when sent.

## Command Format

The format of the `META XPERM` command is:

    [META] XPERM permissions

where

    permissions     is the octal representation permission bits

Commonly, the last nine bits (low order of numeric representation)
are considered significant for files in transit. These are read, write,
and execute for user, group, and others respectively.

For historical reasons, `META` is not always required.
Older UFT servers will accept `XPERM` as a metadata statement
without the `META` prefix.

## Command Response

The usual response from `META XPERM` is a 200 ACK.

Being meta data, UFT implementations which do not handle file permissions
should simply stash the information where it can be used in post procesing.

## Command Sequencing

`META XPERM`, and all other meta statements, must follow `TYPE`
and preceed `DATA`.


