# UFT

Unsolicited File Transfer

Here, "unsolicited" does not mean "unwanted"
but rather that the recipient did not fetch the file
and the sender might not have sign-on credentials on the target host.

UFT is sometimes expanded to "Universal File Transfer",
a name with less baggage.

This repository has both POSIX and VM/CMS components.

## UFT for POSIX Systems

The POSIX (Unix, Linux, and other Unix-like systems)
source is in the `src` directory.

The usual sequence of steps to build UFT on a POSIX system is ...

    ./configure
    make
    make install
    make clean

The `configure` script is not very sophisticated.
The only option it takes is `--prefix`.

## UFT for VM/CMS

UFT mimics the operation of IBM's RSCS and other IBM mainframe
file transfer networking (specifically NJE). So naturally, there is a
VM/CMS implementation of UFT. The VM/CMS implementation was not
originally maintained in this project but is now included in this new home.

See the `cms` directory.

## NJE

NJE stands for "Network Job Entry" and is a
well established protocol for sending files and interactive messages.
UFT provides the same service but with an open protocol.

There are several solutions available to enable NJE networking
on Unix and Unix-like systems such as Linux. UFT exists to simplify
topology and interconnect. In plain English, you don't have to run
an NJE server or daemon on your Linux systems to use UFT. You can
send files with only the UFT client. (If you need to receive files
then of course you do need the server.)

## Download

Pre-compiled and otherwise runnable packages are available from

http://www.casita.net/pub/uft/


