# UFT over SSL

As of this writing, the POSIX UFT client has built-in TLS/SSL support
and can be compiled and linked against OpenSSL. The POSIX UFT server
has always run under INETD or XINETD and remains so. You can, however,
easily "wrap" the server using STunnel. You can also use the proxy mode
to drive traffic through the `openssl s_client` utility.

Understand that built-in SSL/TLS is a feature of the *client*
and not of the server because the server is designed to run under
control of a launching daemon such as XINETD or INETD or `stunnel`.

This applies to the Unix/Linux implementation.
Invoking SSL support on VM/CMS is theoretically easier,
but not discussed in this document.

## Wrapping UFTD with STunnel

The TCP port to use for SSL-encrypted UFT is 5608.
UFT protocol does not have a `STARTTLS` or similar switch command
to change from a cleartext session to an encrypted session.
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

Not having a Berkeley socket to work with,
the `uftd` executable will recognize the `REMOTE_HOST`
environment variable which `stunnel` will set.

## Getting a Server Certificate

You will need a PKI "server certificate" in order for UFT over SSL
to be considered viable (by connecting clients). There are so many
ways to acquire a server cert that we won't go into that here.
In the end, you will need a PEM-encoded concatenation of your
private key, the server certificate (the "leaf" cert),
and any intermediate certificates.

Save your combined server cert here ...

    /etc/uft/private/uftd.pem

The root certificate under which your "leaf" is issued must be in
the trust store of the client. It should NOT be in the concatenation
of certificates represented by `uftd.pem`.

## Running UFTC (SF) over OpenSSL `s_client`

Once you have UFTD available via SSL,
you can use the standard `sf` command and the `--proxy` option
to carry the transaction via `s_client` from the OpenSSL suite.
This works even if `sf` is compiled with built-in SSL support.

    sf --proxy 'openssl s_client -quiet -verify_quiet -connect %h -port 5608' -a textfile.txt user@host

`openssl s_client` has many options, some of which might be helpful.
This document shows the basic invocation.

But, as mentioned, SSL/TLS can be compiled-in to the client.
Built-in TLS/SSL capability lets you skip the `--proxy` hack for
ordinary SSL-protected UFT operation.

## Variations

Here we show the UFTD server certificate at ...

    /etc/uft/private/uftd.pem

You can actually put that combined (certificates and private key) file
anywhere you like as long as the STunnel configuration references it.

For that matter, you can house the STunnel configuration anywhere
as long as `stunnel` is invoked against that file.

## Built-In

To compile-in support for SSL into the UFT client, modify your
`makefile.in` (before configuring) or `makefile` (after configuring)
with `-DUFT_SSL` at the end of the `CFLAGS` definition statement and
with `-lssl -lcrypto` at the end of the `LIBS` definition statement.
Also be certain, of course, to have the development libraries and
header files for OpenSSL installed. Then `make` as you would normally.

This has been tested with several levels of OpenSSL.
Depending on your build environment, `LIBS` might also need `-lpthread`.

## Bugs

The above `openssl s_client` proxy command requires OpenSSL version 3.
In OpenSSL version 1, the command lacks the `-port` option. But older
releases of UFT (2.0 and prior) do not properly process "`%h:%p`" replacement.

The `openssl s_client` command provided in LibreSSL should work
but has not been tested.


