# USER

UFT protocol USER command

A UFT client sends the `USER` command to name intended recipient
of the file being sent. The recipient is typically an end user,
but may be a robot or simply a queue.

## Command Format

The format of the `USER` command is:

    USER user

where

    user    is the name of the user or queue to receive the file
            Quoting of special characters is not discussed.

## Command Response

If the user exists, the server sends a 200 ACK.

If the user does not exist, the server sends a 500 NAK.

## Command Sequencing

`USER` must follow `FILE` and should preceed all other commands.


