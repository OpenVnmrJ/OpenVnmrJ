: '@(#)vnmr_uname.sh 22.1 03/24/08 1991-1996 '
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
#  vnmr_uname calls uname -s and returns one of four values
#  SunOS, SOLARIS, AIX, or IRIX
#  It localizes platform variations of uname

ostype=`uname -s`
if [ x$ostype = "xSunOS" ]
then 
    osver=`uname -r`
    case $osver in
       5*) ostype="SOLARIS"
            ;;
        *)  
            ;;
    esac
fi
if [ x$ostype = "xIRIX64" ]
then 
   ostype="IRIX"
fi
echo $ostype
