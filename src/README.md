# POSIX UFT

This is the source content, primarily C, for the Unix/Linux/POSIX
implementation of UFT.

`uftd` is launched (typically by `inetd` or `xinetd`) for handling
traffic on TCP port 608 and stores incoming files under `/usr/spool/uft`.
A per-user sub-directory is created, on demand, for each recipient.

## Recipe

The package follows the standard recipe:

    ./configure
    make
    make install

## Deliverables

The programs and files produced from this source are:

* `sf`       - sendfile
Location: /usr/bin/sf

* `rls`      - list receivables
Location: /usr/bin/rls

* `rcv`      - receive a file
Location: /usr/bin/rcv

* `rstat`    - "stat" a receivable file
Location: /usr/bin/

* `uftcndd`  - Netdata decoder
Location: /usr/bin/

* `tell`     - send an interactive message
Location: /usr/bin/tell

* `cpq`      - query utility
Location: /usr/bin/cpq

* `uftd`     - UFT daemon
Location: /usr/libexec/uftd

* `ufta`     - anonymized UFT daemon
Location: /usr/libexec/ufta

* `uftxdspl` - de-spooler (for a receivable file)
Location: /usr/libexec/uftxdspl

* `uftctcp2` - re-sender (for a de-spooled file)
Location: /usr/libexec/uftctcp2

* `uft.msgs` - the message repository
Location: (varies)

The message repository must be found via locale settings and so has
a number of possible residence locations, including (for example) ...

    /usr/share/nls/C/
    /usr/share/locale/en_US/

 ... for locales such as "C" or "en_US".


