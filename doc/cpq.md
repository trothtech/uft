# CPQ

UFT protocol `CPQ` command

The `CPQ` command in UFT is similar in function to the CPQ command
in IBM's RSCS product. Use the `CPQ` command to find out information
about a server's host system, including users currently logged on.

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
a 400 series NAK. (client error, even though this is a server fault)

Here are the usual parameters recognized by `CPQ`:

* cplevel - control program level, kernel level, similar to `uname -a`
* cpuid - a unique CPU identifier, something like `dmidecode -s system-serial-number`
* indicate - system load, akin to `uptime` load levels
* logmsg - logon message or `/etc/motd` content
* names - usernames of those users who are currently logged on
* time - time of day at the target system
* user userid - report if a particular user is logged on

## Examples

Query remote time.

Client sends

    cpq time

Server responds

    699 TIME IS 15:34:46 UTC Wed 2025-10-15   
    200 ACK

Query remote logon message.

Client sends

    cpq logmsg

Server responds

    699 FreeBSD 10.3-RELEASE (GENERIC) #0 r297264: Fri Mar 25 02:10:02 UTC 2016
    699
    699 Welcome to FreeBSD!
    699
    200 ACK

## Command Sequencing

A `CPQ` command can come at any time during a transaction.


