# UFT Commands

These are the commands provided in the UFT project.

UFT is mostly a command-line facility as of this writing.
On z/VM there are full-screen tools, native to the operating system,
which can work with UFT. But there are no equivalents on Unix/Linux.

## Commands

* `sf`

`sf` is "sendfile".
On VM/CMS it is a modified SENDFILE EXEC.
On POSIX (Unix/Linux) it is a stand-alone C program.

* `tell`

On POSIX (Unix/Linux) `tell` is a stand-alone C program
using supplemental functions in the protocol to send interactive messages.
(The protocol itself is primarily about sending files.)

There is no equivalent `tell` for VM/CMS in the UFT project.
The CMS `tell` command works with IBM's RSCS product.

* `rcv`

Use the `rcv` shell script on Unix and Unix-like systems
to receive a UFT file.

On CMS, use the IBM-supplied `receive` command.

* `rls`

Use the `rls` shell script on Unix and Unix-like systems
to list files waiting on your virtual loading dock.

On z/VM systems, use `q rdr` or `#cp q rdr` to list incoming files.

* `rrm`

Use the `rrm` shell script on Unix and Unix-like systems
to remove files (to purge them) from your virtual loading dock.

On z/VM systems, `purge rdr` or `#cp purge rdr`.

* `rstat`

Use the `rstat` shell script on Unix and Unix-like systems
to display attributes of a file in your virtual loading dock.

Every incoming UFT file will have at least a `.cf` and `.df`
physical file representing it and you can source the `.cf` file
in your own shell scripts. `rstat` is simply a wrapper to clean-up
the information and present it in a more concise form.

On z/VM, there is no equivalent, but some file attributes can be
gathered from the `cp q rdr *spoolid*` command.

* `cpq`

Use the `cpq` command on Unix and Unix-like systems
to query selected information from a UFT server.
The information provided mimics the behavior of IBM RSCS servers.

There is no counterpart for VM/CMS as of time of writing.

* `uftxdspl`

The `uftxdspl` command on Unix and Unix-like systems retrieves attributes
from the `.cf` file and content from the `.df` file of a UFT file
waiting in your virtual reader (waiting on your virtual loading dock).

The output from `uftxdspl` consists of metadata statements,
followed by `DATA` on a line by itself, and then the body of the file.

`uftxdspl` does not process the file body,
so the combined output will often consist of binary content.

On VM/CMS, `uftxdspl` is a CMS Pipelines "gem".
It's output is similar to what is described here for the POSIX variant.

* `uftctcp2`

The `uftctcp2` command on Unix and Unix-like systems processes a "job"
with a structure similar to the output from `uftxdspl` and attempts to
deliver it to the named target user@host.

The first part of the input stream must be UFT commands for the metadata
of the file being sent. There must then be a `DATA` statement (with no
size token, just the word "DATA") followed by the content or file body.

On VM/CMS, `uftxdspl` is a CMS Pipelines "gem".
It's input is similar to what is described here for the POSIX variant.

* `uftcndd`

Use the `uftcndd` command on Unix and Unix-like systems to convert
IBM Netdata format into local plain text. It is assumed that the file sent
was originally a plain text file. (If it was binary or machine code,
then results are undefined.)

`uftcndd` is used by the `rcv` shell script to handle files sent
via UFT from z/VM and z/OS systems.

There is no counterpart on z/VM systems because Netdata processing is
already built-into the CMS commands and utilities.

* `uftd`

`uftd` is the server daemon.
It is not a user command. It is intended to be launched via INETD or XINETD.


