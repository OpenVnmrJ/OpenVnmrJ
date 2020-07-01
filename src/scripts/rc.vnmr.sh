#!/bin/sh
# chkconfig: 5 99 99
# description: Saves and restores
### BEGIN INIT INFO
# Required-Start: $network
# Default-Start: 5
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
# Note that all "echo" commands are in parentheses.  This is done
# because all commands that redirect the output to "/dev/console"
# must be done in a child of the main shell, so that the main shell
# does not open a terminal and get its process group set.  Since
# "echo" is a builtin command, redirection for it will be done
# in the main shell unless the command is run in a subshell.
#
# usage:
#    $0 start          - start procs and nmrwebd
#    $0 start procs    - start just the procs
#    $0 start nmrwebd  - start just nmrwebd
#    $0 stop           - stop procs and nmrwebd
#    $0 stop procs     - stop procs
#    $0 stop nmrwebd   - stop nmrwebd

# set -x 

if [[ -d /etc/rc.d ]]; then
   . /etc/rc.d/init.d/functions
fi

nmrwebd_root=/vnmr/web
nmrwebd_home=${nmrwebd_root}/scripts
nmrwebd_exe=nmrwebd
nmrwebd_path=${nmrwebd_home}/${nmrwebd_exe}
nmrwebd_pidfile=${nmrwebd_root}/run/${nmrwebd_exe}.pid

start_nmrwebd () {
    if [ `id -u` -ne 0 ]
    then
        echo "permission denied (must be superuser)" |
             logger -s -p daemon.error -t nmrwebd 2>&1
        exit 4
    fi
        
    if [[ -x $nmrwebd_path ]]; then
        python_version=`python -V 2>&1 | cut -d' ' -f2`
        if [[ ! "$python_version" < "2.6" ]]; then
           (echo "     Starting NMR Web Service $nmrwebd_exe") >/dev/console
           (cd $nmrwebd_home && ./$nmrwebd_exe >/dev/console &)
        fi
    fi
}

stop_nmrwebd () {
   if [ `id -u` -ne 0 ]
   then
      echo "permission denied (must be superuser)" |
           logger -s -p daemon.error -t nmrwebd 2>&1
      exit 4
   fi

   if [[ -e $nmrwebd_pidfile ]]; then
       (echo "     Stopping NMR Web Service $nmrwebd_exe") >/dev/console
       pid=`cat $nmrwebd_pidfile`
       kill -TERM $pid >/dev/null
       rm -f $nmrwebd_pidfile
   fi
}

start_proc () {
(echo  'starting Vnmr services: ')			>/dev/console
# check for acqpresent in /vnmr/acqbin, if there, clear acqqueue
(echo  "     clearing $vnmrsystem/tmp ") >/dev/console
(cd $vnmrsystem/tmp; rm -f exp*)
if [ -f $vnmrsystem/acqbin/acqpresent ]; then
   (echo "     clearing $vnmrsystem/acqqueue ") >/dev/console
   (cd $vnmrsystem/acqqueue; rm -f exp*)
   ($vnmrsystem/acqbin/startStopProcs >/dev/console &)
   bootp_d=/usr/sbin/bootpd
   if [ -f $bootp_d ]
   then
      ($bootp_d -s > /dev/console &)
      (echo '     starting bootpd') >/dev/console
   fi
fi
(echo '.')							>/dev/console
}

stop_proc () {
   ($vnmrsystem/acqbin/startStopProcs >/dev/console &)
}


vnmrsystem=/vnmr
case $# in
    0)
	start_proc
	start_nmrwebd
	;;

    *)
	case $1 in
	    'start')
		if [[ $# == 1 || $# > 1 && "$2" == "procs" ]]; then
		    start_proc
		fi
		if [[ $# == 1 || $# > 1 && "$2" == "nmrwebd" ]]; then
		    start_nmrwebd
		fi
		;;

	    'stop')
		if [[ $# == 1 || $# > 1 && "$2" == "procs" ]]; then
		    stop_pro:
		fi
		if [[ $# == 1 || $# > 1 && "$2" == "nmrwebd" ]]; then
		    stop_nmrwebd
		fi
		;;

	esac
	;;
esac
