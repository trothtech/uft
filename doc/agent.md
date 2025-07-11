# AGENT

UFT protocol `AGENT` command

Use the `AGENT` command to confirm the authenticity of a client
acting on behalf of a peer. This will likely be a client connecting
from the same IP address as the known peer, but the AGENT function
adds assurance and is difficult for an un-privileged user to spoof.

Note: this command is not issued by a sending UFT client
but is issued by a receiving UFT server (in client mode, of course)
which has contacted the UFT server on the sending end.
This is a mind bender.

## Command Format

The format of the `AGENT` command is:

    AGENT verifierstring

where

    verifierstring

is the peer verifier, a sort of "key", which is confirmed or denied
by the UFT server which processes the command.

## Command Response

The `AGENT` command responds with either an ACK (200 series) or NAK
(either 400 series or 500 series). If the server responds with 200 ACK,
then the originating client has been proven authentic as a sending agent.

If the server processing the `AGENT` command returns a 500 series NAK,
it means that the agent string did not match and the originating client
is not a genuine sending agent for users on the sending host.

If the server processing the command responds with a 400 series NAK,
it means that it does not understand the `AGENT` command (not implemented).

## In Plain Language

The `AGENT` command is saying, in effect, "I was given this string by a
sending client assuring that I'm talking to you. Is the string correct?".
If the string matches, then the response should be a 200 series message.
If the string does not match, then the response should be a 500 series message.

A 400 series response indicates that the target system does not support `AGENT`.
This has security implications: how can a sending UFT client provide
an agent string if the UFT server on that host cannot process the
`AGENT` command?

## Security

The agent string (the verifier string) is arbitrary
and implementation dependent.
We recommend a SHA256 message digest of some protected reference.
Longer hashes are stronger, but the SHA256 is perhaps more practical,
being only 64 bytes when represented in hexadecimal.

The verifier string must never be returned by the server.
That would render it copyable by a rogue client.

The verifier string is to be provided by a client, as illustrated above.
The client will either be operating as a privileged service or will have
acquired the agent string through secure means. The fact that a client
posesses a correct verifier string confirms its peer authenticity.

The agent string may change at any time, based on conditions at the
owning peer, even on each client transaction.


