# UFT over SSL

As of this writing, the UFT project does not employ SSL directly,
but both client and server can be "wrapped" using OpenSSL and STunnel.

This applies to the Unix/Linux implementation.
Invoking SSL support on VM/CMS is theoretically easier,
but not discussed in this document.

## Wrapping UFTD with STunnel

The TCP port to use for SSL-encrypted UFT is 5608.
UFT protocol does not have a `STARTTLS` or similar switch command
to change a cleartext session to an encrypted session.
TCP port 608 is full time cleartext and TCP port 5608 is full time SSL.

The following STunnel configuration defines a TLS/SSL front-end
to the UFT server.

    [ufts]
    accept  = 5608
    cert = /etc/uft/private/uftd.pem
    exec = /usr/libexec/uftd
    execArgs = uftd

The `uftd` executable remains unchanged
and is invoked by `stunnel` for incoming connections on TCP port 5608.

## Getting a Server Certificate

You will need a PKI "server certificate" in order for UFT over SSL
to be considered viable (by connecting clients). There are so many ways
to acquire a server cert that we won't go into that here. In the end,
you will need a PEM-encoded concatenation of your private key,
the server certificate (the "leaf" cert), and any intermediate
certificates. (The root cert must be in the trust store of the client.)

Save your combined server cert here ...

    /etc/uft/private/uftd.pem

## Running UFTC (SF) over OpenSSL `s_client`

Once you have UFTD available via SSL,
you can use the standard `sf` command and the `--proxy` option
to carry the transaction via `s_client` from the OpenSSL suite.

    sf --proxy 'openssl s_client -quiet -verify_quiet -connect %h -port 5608' -a textfile.txt user@host

`openssl s_client` has many options, some of which might be helpful.
This document shows the basic invocation.

## Anonymized UFTD

## UFT as a Tor Hidden Service

## Sending to a Tor `.onion` Address

## Variations

Here we show the UFTD server certificate at ...

    /etc/uft/private/uftd.pem

You can actually put that combined (certificates and private key) file
anywhere you like as long as the STunnel configuration references it.

For that matter, you can house the STunnel configuration anywhere
as long as `stunnel` is invoked against that file.


