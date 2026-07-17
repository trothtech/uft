# TYPE

UFT protocol `TYPE` command

The UFT `TYPE` command is very significant:
it tells the remote system how to canonize the file being sent.

## Command Format

The `TYPE` command tells the receiving system how the file
should be processed. (Plain text or binary or something else.) <br/>
It takes one argument, a canonization indicator such as `A` or `I`.

`TYPE` optionally takes a second argument, a carriage control indicator.
The CC indicator is meaningful for print material.

The format of the `TYPE` command is:

    TYPE type [cc]

where

    type    is the canonization type

    cc      is an optional format or "carriage control" indicator

## Allowed Values

Acceptable values for type are ...

* A for plain text (colloquially "ASCII")
* I for "image" (colloquially "binary")
* N for Netdata, an IBM format heavily used in mainframe file transfer

All other values are considered experimental.

Acceptable values for CC are ...

* C for common print file carriage control (popular in FORTRAN)
* M for "machine" carriage control (seen in mainframe environments)

## Command Response

The usual response from `TYPE` is a 200 series ACK.

Not all UFT implementations support the cc parameter. <br/>
Server implementations which do not support the cc parameter
should respond with a 400 NAK to a `TYPE` command with cc.

## Command Sequencing

`TYPE` should follow `USER`.


