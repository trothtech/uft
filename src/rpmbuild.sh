#!/bin/sh
#
#         Name: rpmbuild.sh (shell script)
#               build an RPM from the "spec file"
#         Date: 2023-03-22 (Wed)
#
#

#PREFIX                 # set from $STAGING
#APPLID                 # taken from makefile, supplied as argument $1
#VERSION                # taken from makefile, supplied as argument $2
#UNAMEM                 # derived from `uname -m`
#STAGING                # (see below)

# run from the resident directory
cd `dirname "$0"`

# two arguments from the makefile
APPLID="$1"
if [ -z "$APPLID" ] ; then echo "rpmbuild: missing APPLID"
    echo "rpmbuild: you're doing it wrong, drive this from 'make'"
    exit 1 ; fi
VERSION="$2"
if [ -z "$VERSION" ] ; then echo "rpmbuild: missing VERSION"
    echo "rpmbuild: you're doing it wrong, drive this from 'make'"
    exit 1 ; fi

STAGING=`pwd`/rpmbuild.d

# establish certain variables
UNAMEM=`uname -m | sed 's#^i.86$#i386#' | sed 's#^armv.l$#arm#'`
# RPM fixups
if [ ! -s .rpmseq ] ; then echo "0" > .rpmseq ; fi
RELEASE=`cat .rpmseq`
export UNAMEM STAGING RELEASE

#
# we're moving more settings into the config artifacts
. ./configure.sh
# CFLAGS, PREFIX, SYSTEM, LDFLAGS, SHFLAGS, LOCDIR, LOCALE
if [ -z "$MAKE" ] ; then MAKE=make ; fi

#
# process the skeletal spec file into a usable RPM spec file
rm -f $APPLID.spec
$MAKE STAGING=$STAGING UNAMEM=$UNAMEM RELEASE=$RELEASE $APPLID.spec
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# configure the package normally
./configure # --prefix=$PREFIX
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

# 
# 'just make' - build all deliverables
$MAKE all # just short of doing 'make install'
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
$MAKE rexx 2> /dev/null
$MAKE java 2> /dev/null

#
# override the PREFIX for the install step
$MAKE PREFIX=$STAGING install
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# make it "properly rooted"
mkdir -p $STAGING$PREFIX
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#mv $STAGING/*bin $STAGING/lib* $STAGING/share $STAGING$PREFIX/.
#RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

for D in bin lib lib64 libexec sbin share include ; do
    mv $STAGING/$D $STAGING$PREFIX/. 2> /dev/null
done
if [ -d $STAGING/src ] ; then
    mkdir -p $STAGING$PREFIX/src
    mv $STAGING/src $STAGING$PREFIX/src/$APPLID
fi

#
# dump the heavy lifting on the 'rpmbuild' command
rpmbuild -bb --nodeps $APPLID.spec
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# recover the resulting package file ... yay!
mv $HOME/rpmbuild/RPMS/$UNAMEM/$APPLID-$VERSION-$RELEASE.$UNAMEM.rpm .
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
cp -p $APPLID-$VERSION-$RELEASE.$UNAMEM.rpm $APPLID.rpm

# increment the sequence number for the next build
expr $RELEASE + 1 > .rpmseq

exit


