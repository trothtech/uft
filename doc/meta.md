# META

UFT protocol meta commands for file attributes

The `META` command was added to UFT protocol soon after the publication
of RFC 1440 to facilitate arbitrary attribute extensions and to provide
distinction between attributes and primary commands.

## Command Format

The format of the `META` command is:

    META attribute value

For example:

    META NAME filename

where

    filename    is the name of the file about to be sent

## Command Response

Unless otherwise indicated,
all `META` commands result in a 200 series ACK.

Primary commands should not be used as attribute names.
For example, `USER` should not be sent as metadata. It is not
an attribute. Even `TYPE` (though it might seem to be an attribute)
is excluded from the META suite. See the exclusions section below.

## Command Sequencing

`META` should come after `TYPE` and must come after `USER`.

`META` must come before `DATA`.

## Exclusions

The following command verbs are not allowed to follow `META`:

    ABORT
    AGENT
    AUXDATA
    CPQ
    DATA
    EOF
    FILE
    HELP
    META
    MSG
    NOOP
    PIPE
    QUIT
    TYPE
    USER

The following are not command verbs but are similarly
not allowed following `META`:

    AUTH
    FROM
    REMOTE
    SIZE

## Command Sequencing

All `META` commands must follow `TYPE` and preceed `DATA`.


