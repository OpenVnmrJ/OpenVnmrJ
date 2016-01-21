: '@(#)groupadd.sh 22.1 03/24/08 2003-2004 '
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

# To add a group in Windows.

if [ $# -lt 1 ]
then
    echo "Usage: addgroup groupname [user]"
    exit
fi

if [ $# -eq 1 ]
then
    net localgroup $1 /ADD 
elif [ $# -eq 2 ]
then
    net localgroup $1 $2 /ADD
fi
