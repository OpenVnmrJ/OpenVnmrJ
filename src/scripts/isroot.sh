: '@(#)isroot.sh 22.1 03/24/08 2003-2004 '
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

# !/bin/sh

# To check if the current user for Windows is the root user.

rootuser=`/usr/bin/id -u`
if [ x$rootuser = "x197108" -o x$rootuser = "x1049076" ]
then
   rootuser="0"
else
   rootuser="1"
fi

echo $rootuser
