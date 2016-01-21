: '@(#)mkvnmrjadmin.sh 22.1 03/24/08 1999-2000 '
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
: /bin/sh

if test $# -lt 2
then
  echo "Usage: $0 vnmrsystem username"
  exit
fi

vnmrsystem=$1
user=$2
export vnmrsystem user

#$vnmrsystem/bin/makeuser $user > /dev/null 2> /dev/null << +++
ostype=`uname -s`
if [ x$ostype != "xInterix" ]
then
    $vnmrsystem/bin/makeuser $user << +++
y
y
y
y
+++
else
    homedir=`"$vnmrsystem"/bin/getuserinfo "$user" | awk '{FS=";"} {print $2}' | awk '{FS=" "} {print $1}'`
    homedir_interix=`winpath2unix "$homedir"`
    homedir_interix=`/bin/dirname "$homedir_interix"`
    group=`"$vnmrsystem"/bin/getgroup`
    "$vnmrsystem"/bin/makeuser "$user" "$homedir_interix" "$group" y
fi
echo "Automatic configuration of user account '$user' done."

