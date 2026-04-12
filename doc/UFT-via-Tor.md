# UFT via Tor

If you are running the C Tor server program
then enabling UFT via Tor is relatively easy. 

Once enabled, sending via Tor is simply a matter of
invoking the client `sf` with the `--proxy` option.

## Anonymized UFTD

With the `-DUFT_ANONYMOUS` compile-time flag,
or more easily via ...

    make anonymous

 ... the UFT server is built in such a way that it will reveal less
to connecting clients about the target system. (thus "anonymous")
The latter option will produce a `ufta` executable which you can
manually install to `/usr/libexec`.

It is recommended to run this flavor of UFTD on local TCP port 1608.
When running under `xinetd` use the ...

    port = 1608
    user = root
    server = /usr/libexec/ufta
    only_from = 127.0.0.1

 ... statements in the `ufta` service control file. That last statement,
`only_from`, will minmize leakage of the fact that you're running a
Tor-enabled UFTD. Take similar action if you're running older `inetd`.

## UFT as a Tor Hidden Service

Tor "Hidden Services", more popularly called "`.onion addresses",
are established via the `HiddenServiceDir` and `HiddenServicePort`
directives in the Tor "RC" file.

    HiddenServiceDir  /etc/tor/uft/
    HiddenServicePort  608  127.0.0.1:1608

Tor will create an asymmetric key pair and derive an `.onion` hostname.
You should retrieve the hostname from ...

    /etc/tor/uft/hostname

## Sending to a Tor `.onion` Address

There is no change needed to `sf` for enabling Tor.
Just use a proxy such as `nc` (`netcat`) when running it, like ...

    sf --proxy ' nc -x 127.0.0.1:9050 %h %p ' -a textfile.txt user@longhostname.onion

This presumes Tor is providing SOCKS service on local TCP port 9050,
which is the default. `netcat` will carry the UFT session via proxy.
Everything else about the `sf` operation is the same as for normal UFT.

## Variations

Building an anonymized UFTD is optional.
You can use a standard UFTD with Tor, but it will report information
back to clients including the actual hostname of the server.
Otherwise the UFT operation is the same. You will, of course, have to
code `/usr/libexec/uftd` for XINETD or INETD and/or use the standard
port 608 when defining the hidden service.

The Tor "hidden service" directory is shown in this doc as ...

    /etc/tor/uft/

You can use any directory you like.
Simply look for the `hostname` file in that directory.


