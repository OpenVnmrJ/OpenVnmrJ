: '@(#)checkspace.sh 22.1 03/24/08 1991-1994 '
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
#Called by LoadNmr to determine the available disk space.

this_dir=$1
osname=`uname -s`

while [ ! -d $this_dir ]
do
    this_dir=`dirname $this_dir`
done

cmd="df -k"
if [ x$osname = "xInterix" ]
then
    cmd="df -kP"
fi

# if df -k prints the output on two lines, then it's the 3rd argument, else
# it's the 4th argument.
tmps=`$cmd $this_dir 2>/dev/null | awk ' { 
	if (NR != 1 && NF == 6) print $4 
	if (NR != 1 && NF == 5) print $3
    } '`

if [ x$tmps = "x" ]
then
   size="0"
else
   size=$tmps
   # make sure there is room for the database as well, in addition to VnmrJ
   if [ ! -d /vnmr/pgsql ]
   then
       size=`expr $size - 25000`
   else
       tmps=`du -ks /vnmr/pgsql | awk ' { print $1 }'`
       size=`expr $size - $tmps`
   fi
fi

echo $size
