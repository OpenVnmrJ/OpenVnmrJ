#!/bin/sh
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
#
# S99scanlog.sh
# Starting scanlog daemon, system V style

case "$1" in
'start')
        if [ -r /usr/varian/sbin/scanlog ]
	then
                echo 'starting scanlog daemon'

                /usr/varian/sbin/scanlog &
		echo $! > /vnmr/adm/scanlogpid
		chmod 600 /vnmr/adm/scanlogpid
        fi
        ;;

'stop')
	if [ -r /vnmr/adm/scanlogpid ]
	then
        	scl_id=`cat /vnmr/adm/scanlogpid`
		if [ x$scl_id != x ]
		then
		    /usr/bin/kill -9 $scl_id
		fi
	fi
        ;;

*)
        echo "Usage: $0 { start | stop }"
        exit 1
        ;;
esac
exit 0
