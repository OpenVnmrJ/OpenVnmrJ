: '@(#)loadpwd.sh 22.1 03/24/08 2003-2004 '
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
# This scripts loads password protected files via a patch.
# 
# Syntax: load.pwd password filename
# 
vnmr="/vnmr"

if [ $# -lt 2 ]
then
   echo Usage: $0 passwd filename
   echo Example: $0 ab-cde bird.pwd
   echo You must be the VnmrJ Administrator to load these files.
   exit
fi

admin=`ls -l /vnmr/vnmrrev | awk '{print $3}'`
if test $USER != $admin
then
    echo "Switching to the VnmrJ Administrator $admin "
    su - $admin -c "$0 $1 $2"
    exit
fi

cd /tmp
$vnmr/bin/decode.sol $1 $vnmr/$2 tmp.tar
cd $vnmr
tar xvf /tmp/tmp.tar
rm /tmp/tmp.tar
