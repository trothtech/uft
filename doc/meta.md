# META

UFT protocol META commands for file attributes

## Command Format

The format of the `META` command is:

    META attribute value

## Command Response

Unless otherwise indicated,
all `META` commands result in a 200 series ACK.

Primary commands should not be used as attribute names.
In other words, `USER` should not be sent as metadata. It is not
an attribute. Even `TYPE` (though it might seem to be an attribute)
is excluded from the META suite.

## Command Sequencing

`META` should come after `TYPE` and must come after `USER`.

`META` must come before `DATA`.


