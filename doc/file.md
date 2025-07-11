# FILE

UFT protocol FILE command

This command indicates the start of a transaction to send a file.

## Command Format

The format of the `FILE` command is:

    FILE size from auth

where

    size    is the estimated size of the file about to be sent
            The size specified need not be exact. (see below)

    from    is the sign-on name of the sending user
            or the name of an equivalent sending entity

    auth    is an authentication token, best AGENT or a dash

## Command Response

The usual response is a 200 series ACK. <br/>
If a server must reject any of the parameters indicated,
it should return a 500 series NAK.

The size value is used to answer the question "is there room
for the file about to be sent?". The server should reject
the file with a 500 NAK if there is not sufficient space for it.
Clients, however, are not required to supply a precise value,
especially if it is not available.

The only supported authentications at this time are AGENT and IDENT.

## Command Sequencing

The `FILE` command must come before any others.


