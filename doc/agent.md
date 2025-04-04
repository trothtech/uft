# AGENT

UFT protocol AGENT command

Use the AGENT command to confirm the authenticity of a client
acting on behalf of a peer. This will likely be a client connecting
from the same IP address as the known peer, but the AGENT function
adds assurance and is difficult for an un-privileged user to spoof.

## Command Format

The format of the AGENT command is:

    AGENT *verifierstring*

where

    *verifierstring*

is the peer verifier, a sort of "key", which is confirmed or denied
by the UFT server.

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


