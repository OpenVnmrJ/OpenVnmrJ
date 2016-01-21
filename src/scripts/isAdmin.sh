: '@(#)isAdmin.sh 22.1 03/24/08 2003-2005 '
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

if [ $# -ne 0 ]
then
    admin=`net user "$1" | grep "Local Group Memberships" | grep "*Administrators" | awk '{print $1}'`
    echo $admin
fi


