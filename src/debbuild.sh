#!/bin/sh
#
#         Name: debbuild.sh (shell script)
#               build a .deb package file from Debian "control"
#         Date: 2025-07-14 (Mon)
#     See also: rpmbuild.sh script in this package
#

#PREFIX                 # set from $STAGING                            +
#APPLID                 # taken from makefile, supplied as argument $1
#VERSION                # taken from makefile, supplied as argument $2
#UNAMEM                 # derived from `uname -m`
#STAGING                # (see below)

# run from the resident directory
cd `dirname "$0"`
D=`pwd`

# I wish the following two were not hard-coded

APPLID="$1"
if [ -z "$APPLID" ] ; then echo "debbuild: missing APPLID"
    echo "debbuild: you're doing it wrong, drive this from 'make'"
    exit 1 ; fi

VERSION="$2"
if [ -z "$VERSION" ] ; then echo "rdebbuild missing VERSION"
    echo "debbuild: you're doing it wrong, drive this from 'make'"
    exit 1 ; fi

STAGING=`pwd`/debbuild.d

#
UNAMEM=`uname -m`
UNAMEM=`uname -m | sed 's#^i.86$#i386#' | sed 's#^armv.l$#arm#'`
# Debian fixups
if [ "$UNAMEM" = "x86_64" ] ; then UNAMEM=amd64 ; fi

export UNAMEM STAGING

#
# we're moving more settings into the config artifacts
. ./configure.sh
# CFLAGS, PREFIX, SYSTEM, LDFLAGS, SHFLAGS, LOCDIR, LOCALE
if [ -z "$MAKE" ] ; then MAKE=make ; fi

#
# process the skeletal dctl file into a usable Debian control file
rm -f $APPLID.dctl
$MAKE STAGING=$STAGING UNAMEM=$UNAMEM RELEASE=$RELEASE $APPLID.dctl
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# clean up from any prior run
$MAKE clean 1> /dev/null 2> /dev/null
rm -rf $STAGING
#find . -print | grep ';' | xargs -r rm

#
# configure the package normally
#./configure --prefix=/usr
./configure --prefix=$PREFIX
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# 'just make'
$MAKE
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# override the PREFIX for the install step
$MAKE PREFIX=$STAGING install
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# make it "properly rooted"
mkdir -p $STAGING
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
mkdir $STAGING$PREFIX
mv $STAGING/*bin $STAGING/lib* $STAGING/share $STAGING$PREFIX/.
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# move the control file into place
mkdir $STAGING/DEBIAN
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
cp -p $APPLID.dctl $STAGING/DEBIAN/control
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# build the DEB file (and keep a log of the process)
rm -f $APPLID.deb.log
echo "+ dpkg-deb --build debbuild.d"
        dpkg-deb --build debbuild.d 2>&1 | tee $APPLID.deb.log
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
rm $APPLID.dctl

#
# recover the  resulting package file ... yay!
mv debbuild.d.deb $APPLID.deb
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
cp -p $APPLID.deb $APPLID-$VERSION-$UNAMEM.deb
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# remove temporary build directory
rm -r $STAGING

exit


