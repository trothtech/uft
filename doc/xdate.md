# [META] XDATE

UFT protocol `XDATE` command

This meta command provides a date stamp for the file being sent.

The value is a decimal representation of the number of seconds
since the Unix epoch, 1970-January-01 midnight UTC.

## Command Format

The format of the `META XDATE` command is:

    [META] XDATE timestamp

where

    timestamp   is the number of seconds since the Unix epoch

The timestamp value is a decimal representation.

For historical reasons, `META` is not always required.
Older UFT servers will accept `XDATE` as a metadata statement
without the `META` prefix.

## Command Response

The usual response from `META XDATE` is a 200 ACK.

Being meta data, UFT implementations which do not recognize "xdate"
should simply stash the information where it can be used in post procesing.

## Command Sequencing

`META XDATE`, and all other meta statements, must follow `TYPE`
and preceed `DATA`.


