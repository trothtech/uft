# UFT

Unsolicited File Transfer

Here, "unsolicited" does not mean "unwanted"
but rather that the recipient did not fetch the file
and the sender might not have sign-on credentials on the target host.

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
originally maintained in this project but is now included in the new home.

See the `cms` directory.


