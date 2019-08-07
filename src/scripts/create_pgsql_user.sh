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
# Are we using the newer postgres or the old version distributed with vnmrj?
# If createuser exists in $vnmrsystem/pgsql/bin/, use it, 
# else use system version.
file="$vnmrsystem/pgsql/bin/createuser"
if [ -f "$file" ]
then
    # Old one, use appropriate path and args
    $vnmrsystem/pgsql/bin/createuser -a -q -d -h /tmp $1
else
    # New one, use no path and appropriate args
    createuser -d -h /tmp -S -R $1 2> /dev/null
fi

echo "DONE"

