#!/bin/sh
# '@(#)patchmake.sh ' 
#
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#

#----------------------------------------------------------
#  patchmake "path-name of patch file"
#
#  The patchmake script builds a custom patch. The directory containing
#  the patch contents must be supplied as an argument.
#
#  A patch .ptc file contains the following files:
#  patch.zip contains the files that will be installed into the $vnmrsystem directory
#  checksum  contains the checksum of the patch.zip file. Used for validation.
#  The patch.zip file has an optional Readme file describing the content of the patch.
#  The patch.zip file has an optional p_install script to do additional tasks by the patch.
#----------------------------------------------------------

# set -x
#-----------------------------------------------------------

#-----------------------------------------------------------
#
#                Main
#-----------------------------------------------------------

if [ $# -lt 1 ]
then
    echo ""
    echo "Usage:   patchmake \"patch directory\" "
    exit 1
fi

if [ ! -d $1 ]
then
   echo "Patch directory $1 does not exist"
   exit 1
fi

curDir=`pwd`
cd $1
date=`date +%F`
if [ $# -eq 2 ]
then
   patchname=$2.ptc
else
   patchname=custom_$date.ptc
fi
zip -ry patch.zip *
os=`uname -s`
if [ x$os = "xDarwin" ]
then
   csum=`/sbin/md5 -q patch.zip`
else
   csum=`md5sum patch.zip | awk '{print $1}'`
fi
echo $csum > checksum
zip $patchname checksum patch.zip
rm -f checksum patch.zip
mv $patchname $curDir/.
cd $curDir

echo ""
echo "-- Patch $curDir/$patchname made -----"
echo ""
