# [META] DATE

UFT protocol `DATE` command

This meta command provides a date stamp for the file being sent.

The date/time value is three blank-delimited words:
year-month-day, hours:minutes:seconds, abbreviated timezone.

## Command Format

The format of the `META DATE` command is:

    [META] DATE date time zone

where

    date        is the date in year-month-day form

    time        is the time of day in the usual form, hh:mm:ss

    zone        is the timezone, a three-letter value per international standards

For historical reasons, `META` is not always required.
Older UFT servers will accept `DATE` as a metadata statement
without the `META` prefix.

## Command Response

The usual response from `META DATE` is a 200 ACK.

Being meta data, UFT implementations which do not process dates
or do not recognize the format described here should simply
stash the information where it can be used in post procesing.

## Command Sequencing

`META DATE`, and all other meta statements, must follow `TYPE`
and preceed `DATA`.


