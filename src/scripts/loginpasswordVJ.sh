: '@(#)loginpasswordVJ.sh 22.1 03/24/08 1999-2002 '
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

# The loginpasswordVJ sets the librarypath for expect, and
# calls the script.

: /bin/sh

LD_LIBRARY_PATH=/vnmr/java:/vnmr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

if test $# -gt 0
then
    ostype=`uname -s`
    if [ x$ostype != "xLinux" ]
    then
	/vnmr/bin/expect "$@"
    else
	/usr/bin/expect "$@"
    fi
fi
