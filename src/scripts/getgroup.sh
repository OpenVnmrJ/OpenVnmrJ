: '@(#)getgroup.sh 22.1 03/24/08 2003-2004 '
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

#! /bin/sh -f

ostype=`uname -s`

if [ x$ostype = "xDarwin" -o  x$ostype = "xInterix" ]
then
   nmr_group=`id -gn`
else
   cd "$vnmrsystem"
   nmr_group=`ls -ld . | awk '{ print $4 }'`
fi
echo $nmr_group


