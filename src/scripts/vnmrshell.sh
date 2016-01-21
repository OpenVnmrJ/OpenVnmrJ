: '@(#)vnmrshell.sh 22.1 03/24/08 1991-1996 '
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

#  Normally X-windows is in use.  It is only necessary to check for
#  SunView if the OS is SunOS and the major revision is 4 (not Solaris)

xw_vs_sv() {
    if test `uname -s` = "SunOS"
    then
        majorrev=`uname -r | awk '{ print substr( $0, 1, 1 ) }'`
        if test $majorrev -eq 4
	then
            vxrtoolpath=`which vxrTool`
            echo $vxrtoolpath | awk '
BEGIN { window="-sv" }
/binx/ { window="-xw" }
END { print window }'
        else
	    echo "-xw"
        fi
    else
        echo "-xw"
    fi
}

window=`xw_vs_sv`

if test $window = "-sv"
then
    vxrTool $*
else
    (xterm $*)
fi
