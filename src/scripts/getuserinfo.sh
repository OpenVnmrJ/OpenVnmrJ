: '@(#)getuserinfo.sh 22.1 03/24/08 2003-2004 '
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

# Returns the userinfo in the format fullname:homedir.

if [ $# -lt 1 ]
then 
    echo "Usage: getuserinfo username"
    echo ""
    exit
fi

osname=`uname -s`
if [ x$osname = "xInterix" ]
then
    home_dir="$HOME"
    # If no HOME set, use static path
    if [ "x$home_dir" = "x" ]
    then
	home_dir="/dev/fs/C/SFU/home/$1"
    fi
    home_dir=`unixpath2win $home_dir`

    # Don't know Full Name, so just return username
    # Use print -R instead of echo to avoid \v, \n etc from being special
    print -R "$1;$home_dir"
else
    userinfo=`getent passwd $1 | awk 'BEGIN {FS=":"} {print $5":"$6}'`
    echo $userinfo
fi
