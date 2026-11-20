# Message Sending Hack

UFT protocol is designed for sending files, but most of the systems
that UFT emulates or replaces also support sending interactive messages.
The `MSG` command supports this and can be used for sending messages
apart from the need for sending an actual file. Here is how.

## Start as a File Job

While the `MSG` command (sans `META`) can be sent at any time,
the user sending the message, and the authentication of that user,
requires a `FILE` command come first:

    FILE size from auth

What is meaningful here is *from* and *auth*.
The *size* can be zero (0) because in the message sending hack
there is no file data to convey.

    from        is the user sending the message

    auth        is the kind of authentication employed
                (Most viable as of this writing is `AGENT`.)

The usual response is a 200 series ACK. <br/>
If the receiving server rejects any of the parameters indicated,
it should return a 500 series NAK.

## Send the Message

After a `FILE` command,
the server has the name of a sending user (hopefully authenticated).
The client can then send a `MSG` command:

    MSG user message ...

where

    user        is the user to receive the message

    message     is the plain text message to be sent

Success or failure may depend on whether the intended receipient user
is signed on and whether or not the user has enabled receiving messages.
A successful response from `MSG` is a 200 series ACK.

## Then Bail Out

After the message has been sent, the client should cancel the
implied file transaction. It should send an `ABORT` command
and then `QUIT` and close the connection.

    ABORT

If the receiving UFT server cannot identify the sending user
it may reject the message.

Not all UFT implementations support MSG.


