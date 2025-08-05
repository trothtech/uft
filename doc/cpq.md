# CPQ

UFT protocol `CPQ` command

The `CPQ` command in UFT is similar in function to the CPQ command
in IBM's RSCS product. Use the `CPQ` command to find out information
about the server's host system, including users currently logged on.

z/VM users should consult the CP QUERY command to get some idea
of what kind of information the UFT `CPQ` command provides.

## Command Format

The format of the `CPQ` command is:

    CPQ parameter [sub-parameter]

where

    parameter    is a required item or value to be queried

    sub-parameter    is a secondary item or value for some parameters

## Command Response

`CPQ` returns each line of requested information as a 600 series response.

After all lines of the requested information have been returned,
`CPQ` indicates terminal success with a 200 series ACK.

Any parameter not recognized results in a 400 series NAK. (client error)

## Usage Notes

Not all UFT implementations support `CPQ`. <br/>
An implementation which does not handle `CPQ` must return
a 400 series NAK. (client error)

Here are the usual parameters recognized by `CPQ`:

* cplevel - control program level, kernel level
* cpuid - a unique CPU identifier
* indicate - system load
* logmsg - logon message or /etc/motd
* names - usernames of those users who are currently logged on
* time - time of day at the target system
* user userid - report if a particular user is logged on


