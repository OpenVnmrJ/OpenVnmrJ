# !/bin/sh
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#

if test $# -gt 0
then
   ostype=`uname -s`
   if [ x$ostype = "xDarwin" ]
   then
     open $1 &
   else
     firefox $1  >& /dev/null &
   fi
fi
