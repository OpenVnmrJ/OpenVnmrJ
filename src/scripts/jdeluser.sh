: '@(#)jdeluser.sh 22.1 03/24/08 1999-2002 '
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
#!/bin/sh

# This script will delete user's entries from passwd, group and shadow files,
# if user_home_directory exist, that will be moved to .user_home_directory
# for future references

#Usage: jdeluser username

if [ $# -eq 1 ]
then
    hdir=`grep ${1}: /etc/passwd | awk 'BEGIN{FS=":"}{print $6}`
    if [ x"$hdir" != "x" ]
    then
	/usr/sbin/userdel $1 2>&1 >/dev/null
	#sometimes the referred directory does not exist for some reason
	if [ -d "$hdir" ]
	then
		dpath=`dirname "$hdir"`
		dname=`basename "$hdir"`
		mv -f "$hdir" "${dpath}/.${dname}"
	fi
	echo "0"

    else
        #user does not exist
	echo "1"
    fi
else
    echo "1"
fi
