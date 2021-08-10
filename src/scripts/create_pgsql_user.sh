#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
# called by VnmrAdmin.java
# usage: "create_pgsql_user user_name"
# attention! the VnmrAdmin checks for strings "invalid user" and "DONE"

/usr/bin/id $1 1> /dev/null 2> /dev/null
if [ $? != 0 ]
then 
    echo "invalid user"
    exit
fi
if [ x$vnmrsystem = "x" ]
then
   vnmrsystem="/vnmr"
fi
createuser -d -h /tmp -S -R $1 2> /dev/null

echo "DONE"

