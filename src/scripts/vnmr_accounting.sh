#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
#
# set -x
if [ x"$vnmrsystem" = "x" ]
then
   vnmrsystem=/vnmr
fi
admin=$($vnmrsystem/bin/fileowner $vnmrsystem/vnmrrev)
if [ "$USER" != "$admin" ]
then
   echo "OpenVnmrj accounting tool can only be used by the"
   echo "OpenVnmrJ administrator account $admin"
   exit
fi
javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

LANG=en_US.UTF8
$javabin -jar $vnmrsystem/java/account.jar "$@"
exit 0
