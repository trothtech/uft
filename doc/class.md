# [META] CLASS

UFT protocol META CLASS command

The UFT `META CLASS` command is meaningful for print jobs. <br/>
It tells the receiving system how to classify the file.
For printing systems this can affect routing and/or priority.

## Command Format

`META CLASS` takes one argument.
The "class" is usually a single letter.

`META CLASS` optionally takes a second argument. <br/>
Systems with a native spooling system may recognize (and might utilize)
a queuing device type.

The format of the `META CLASS` command is:

    [META] CLASS class [devtype]

where

    class       is the canonization type

    devtype     is an optional format or "carriage control" indicator

For historical reasons, `META` is not always required.
Some older UFT servers will accept `CLASS` as a metadata statement.

## Command Response

The usual response from `META CLASS` is a 200 ACK.

Being meta data, UFT implementations which do not recognize "class"
should simply stash the information where it can be used in post procesing.

Not all UFT implementations support the `CLASS` statement apart from the
`META` prefix. In such a case the server should respond with a 400 NAK.

## Command Sequencing

`META CLASS`, and all other meta statements, must follow `TYPE`
and preceed `DATA`.


