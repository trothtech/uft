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

* `sf`       - sendfile <br/>
Location: /usr/bin/sf

* `rls`      - list receivables <br/>
Location: /usr/bin/rls

* `rcv`      - receive a file <br/>
Location: /usr/bin/rcv

* `rrm`      - remove (purge) a file <br/>
Location: /usr/bin/rrm

* `rstat`    - "stat" a receivable file <br/>
Location: /usr/bin/

* `uftcndd`  - Netdata decoder <br/>
Location: /usr/bin/

* `tell`     - send an interactive message <br/>
Location: /usr/bin/tell

* `cpq`      - query utility <br/>
Location: /usr/bin/cpq

* `uftd`     - UFT daemon <br/>
Location: /usr/libexec/uftd

* `ufta`     - anonymized UFT daemon <br/>
Location: /usr/libexec/ufta

* `uftxdspl` - de-spooler (for a receivable file) <br/>
Location: /usr/libexec/uftxdspl

* `uftctcp2` - re-sender (for a de-spooled file) <br/>
Location: /usr/libexec/uftctcp2

* `uft.msgs` - the message repository <br/>
Location: (varies, see next)

The message repository must be found via locale settings and so has
a number of possible residence locations, including (for example) ...

    /usr/share/nls/C/
    /usr/share/locale/en_US/

 ... for locales such as "C" or "en_US".


