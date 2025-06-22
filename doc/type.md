# TYPE

UFT protocol TYPE command

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

## Command Response

The usual response from `TYPE` is a 200 ACK.

Not all UFT implementations support the *cc* parameter. <br/>
Server implementations which do not support the *cc* parameter
should respond with a 400 NAK.

## Command Sequencing

`TYPE` should follow `USER`.


