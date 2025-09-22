# VM/CMS UFT

This is the VM/CMS implementation of UFT.

UFTD EXEC establishes a TCP listener on port 608 for handling incoming
UFT protocol transactions. It spools files to the reader queue of
individual users.

## No Compilation

This code is all Rexx and Pipelines. It does not need to be compiled.

You will need to configure VM TCP/IP to authorize the daemon for port 608.

## Deliverables

The programs and files included here are:

* `SF EXEC`         - a modified version of SENDFILE EXEC

* `UFTD EXEC`       - UFT daemon (wrapper)

* `UFTD REXX`       - UFT daemon (gem for TCP traffic)

* `UFTXDSPL REXX`   - de-spooler (for a receivable file)

* `UFTCTCP2 REXX`   - re-sender (for a de-spooled file)

* `UFTUME MESSAGES` - the message repository source

The message repository must be processed with `genmsg`
creating `UFTUME TEXT`.

Use `cp q rdr` or `rdrlist` to list receivables

Use `receive` to receive a file


