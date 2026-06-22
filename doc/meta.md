# META

UFT protocol meta commands for file attributes

The `META` command was added to UFT protocol soon after the publication
of RFC 1440 to facilitate arbitrary attribute extensions and to provide
distinction between attributes and primary commands.

The item indicated in any `META` command is referred to here as a "tag"
though it might be considered a "variable" or a "name" in implementations.

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

## Prohibited Characters

Digits 0 through 9 and letters A through Z are explicitly allowed
in both META tags and in META values. Leading numeric
characters is not recommended for META tags in order to
achieve best interoperability with host operating systems. META tags
are not case sensitive and should be sent in upper case.

Characters which are prohibited from use in either META tags
or in META values include:

&
=
;
?
$
!
^
(
)
"
'
`
{
}
|
[
]
\
<
>

Characters which are prohibited from use in META tags
but which are acceptable in META values include:

*
.
,
/
#
-
+
_
~
%
:
@


