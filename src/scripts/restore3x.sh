: '@(#)restore3x.sh 22.1 03/24/08 2003-2008 '
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
#! /bin/sh

# This scripts copies files from VnmrJ 2.3A or later
# to another VnmrJ directory, so we can restore that copy of VnmrJ
# to the console, restoreing NDDS 3x 

if [ -f /vnmr/vnmrrev ]
then
   echo "This version of VnmrJ is in ${vnmrsystem}:"
   head -2 ${vnmrsystem}/vnmrrev
else
  echo "Cannot find /vnmr/vnmrrev, cannot proceed"
  exit 1
fi

echo ""
read -p "Enter the full path of the VnmrJ version you want to restore: " restore_dir

echo ""
if [ -f ${restore_dir}/vnmrrev ]
then
   echo "This will restore ${restore_dir}"
   head -2 ${restore_dir}/vnmrrev
else
   echo "cannot find ${restore_dir}/vnmrrev, cannot proceed"
   exit 1
fi

touch ${restore_dir}/remove_me >/dev/null 2>/dev/null
if [ $? -ne 0 ]
then
   rm -f  ${restore_dir}/remove_me
   echo ""
   echo "No write permission to ${restore_dir}, cannot proceed"
   exit 1
else
  rm -f ${restore_dir}/remove_me
fi


abc="555"
if [ $abc=1 ]
then

cd ${restore_dir}/bin
rm -f lnvsetacq.orig	#don't complain if it does not exist
mv lnvsetacq lnvsetacq.orig
cp /vnmr/bin/lnvsetacq2 lnvsetacq
cd ${restore_dir}/acqbin
rm -f consoledownload42x	#avoid permission problems
cp /vnmr/acqbin/consoledownload42x .
rm -f testconf42x		#avoid permission problems
cp /vnmr/acqbin/testconf42x .
rm -f flashia42x		#avoid permission problems
cp /vnmr/acqbin/flashia42x .
rm -f consoledownload3x		#avoid permission problems
cp /vnmr/acqbin/consoledownload3x .
rm -f testconf3x		#avoid permission problems
cp /vnmr/acqbin/testconf3x .
rm -f flashia3x			#avoid permission problems
cp /vnmr/acqbin/flashia3x .
cd ${restore_dir}/acq/download
rm -f nddslib.o			#avoid permission problems
cp nvScript nddslib.o
rm -f  nddslib.md5		#avoid permission problems
cp nvScript.md5 nddslib.md5

fi

echo "Done"
