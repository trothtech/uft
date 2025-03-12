#!/bin/sh
#
#         Name: rpmbuild.sh (shell script), originally buildrpm.sh
#               build an RPM from the "spec file"
#         Date: 2024-03-17 (Sun) St. Patty's Day
#     See also: script by similar name in the VMLINK package
#

#PREFIX                 # set from $STAGING                            +
#APPLID                 # taken from makefile, supplied as argument $1
#VERSION                # taken from makefile, supplied as argument $2
#RELEASE                # replaces RPMSEQ
#UNAMEM                 # derived from `uname -m`
#STAGING                # from the makefile

# run from the resident directory
cd `dirname "$0"`
D=`pwd`

# I wish the following two were not hard-coded
APPLID="$1"
if [ -z "$APPLID" ] ; then echo "missing APPLID - you're doing it wrong, drive this from 'make'" ; exit 1 ; fi

#VERSION=`grep '^#define' uft.h | grep UFT_VERSION \
#                        | awk -F\" '{print $2}' | awk -F/ '{print $2}'`
VERSION="$2"
if [ -z "$VERSION" ] ; then echo "missing VERSION - you're doing it wrong, drive this from 'make'" ; exit 1 ; fi

if [ ! -s .rpmseq ] ; then echo "1" > .rpmseq ; fi
RELEASE=`cat .rpmseq`

STAGING=`pwd`/rpmbuild.d

#
UNAMEM=`uname -m`
UNAMEM=`uname -m | sed 's#^i.86$#i386#' | sed 's#^armv.l$#arm#'`

export UNAMEM RELEASE STAGING

#
# we're moving more settings into the config artifacts
. ./configure.sh

#
# process the skeletal spec file into a usable spec file
rm -f uft.spec
make STAGING=$STAGING UNAMEM=$UNAMEM RELEASE=$RELEASE uft.spec
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# clean up from any prior run
make clean 1> /dev/null 2> /dev/null
rm -rf $STAGING
#find . -print | grep ';' | xargs -r rm

#
# configure the package normally
./configure
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# 'just make'
#make
#make all # just short of doing 'make install'
make sf uftd
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# override the PREFIX for the install step
make PREFIX=$STAGING install
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# make it "properly rooted"
mkdir $STAGING/usr
mv $STAGING/bin $STAGING/sbin $STAGING/usr/.
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi

#
# build the RPM file (and keep a log of the process)
rm -f uft.rpm.log
echo "+ rpmbuild -bb --nodeps uft.spec"
        rpmbuild -bb --nodeps uft.spec 2>&1 | tee uft.rpm.log
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
rm uft.spec

#
# recover the  resulting package file ... yay!
cp -p $HOME/rpmbuild/RPMS/$UNAMEM/$APPLID-$VERSION-$RELEASE.$UNAMEM.rpm .
#                          UNAMEM  APPLID- VERSION- RELEASE. UNAMEM
RC=$? ; if [ $RC -ne 0 ] ; then exit $RC ; fi
cp -p $APPLID-$VERSION-$RELEASE.$UNAMEM.rpm uft.rpm

#
# remove temporary build directory
rm -r $STAGING

# increment the sequence number for the next build
expr $RELEASE + 1 > .rpmseq

exit


