# DATA

UFT protocol DATA command

A UFT client uses the `DATA` command to signal "start of data".
There are two flavors of `DATA` command: immediate and batch.
The batch version is a special case that only applies to use
between UFT programs running on the same system.

## Command Format

The format of the "on the wire" `DATA` command is:

    DATA burst-size

where

    burst-size      is the number of octets to be sent
                    before the next command will be sent

## Command Response

The server returns 300 "send more" and switches into data mode.

After the indicated number of bytes have been received,
the server sends 200 ACK and switches back into command mode.

The batch variant of the protocol (called "SIFT") is a special case
where `DATA` indicates the end of all meta data and the start of
the file body. With SIFT, because it is wholly local, the file content
continues to the end of the physical SIFT file. Understand that this
is only for UFT programs running on the same system.

## Command Sequencing

`DATA` must come after all `META` commands,
which in turn should come after `TYPE`.


