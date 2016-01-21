#! /bin/sh
# @(#)cryomon.sh 22.1 03/24/08 2005 
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

# Run the CryogenMonitor Program 

# Usage examples:
# (1) cryomon
#	Start up with GUI in "reader" mode
# (2) cryomon -master
#	Start with GUI in "master" mode
# (3) cryomon -nogui -master
#	Start without GUI in "master" mode
#
# optional flags
#
#  -debug    turn on debugging output
#  -vnmrj    use vnmrj laf
#
PROG=$0

# Notify nmrwebd that CryoMon is starting so that it can defer to CryoMon
# Only one listener can connect to the Cryogen Monitor at a time
notify_nmrwebd()
{
  if [[ -e /vnmr/web/run/nmrwebd.pid ]]; then
    nmrwebd_pid=`cat /vnmr/web/run/nmrwebd.pid`
    if [[ "$nmrwebd_pid" != "" ]]; then
      nmrwebd_running=`ps -p ${nmrwebd_pid} | egrep '(nmrwebd|python)' | wc -l`
      if [[ $nmrwebd_running > 0 ]]; then
        kill -USR1 $nmrwebd_pid
        sleep 1
      fi
    fi
  fi
}

# Defaults:

# Set defaults for SYSDIR and USERDIR
if [ x"$vnmrsystem" = "x" ]
then
    SYSDIR=/vnmr
else
    SYSDIR=$vnmrsystem
fi

if [ x"$vnmruser" = "x" ]
then
  # Probably wrong if run from background:
  USERDIR=$HOME/vnmrsys
else
  # This is likely to be wrong if running in background from the Procs:
  USERDIR=$vnmruser
fi


# Read the command flags:
FLAGSDONE="false"
EXEC=""
DEBUG=""
VNMRJ=""
while [ "$FLAGSDONE" = "false" ]; do
    case $1 in
	-userdir) USERDIR=$2
             shift
             shift;;
	-sysdir) SYSDIR=$2
             shift
             shift;;
    -master) MASTER="-master"
	     notify_nmrwebd
             shift;;
    -nogui)  GUIFLAG="-noGui"
             shift;;
    -debug)  DEBUG="-debug"
             shift;;
    -vnmrj)  VNMRJ="-vnmrj"
             shift;;
        *) FLAGSDONE="t";;
    esac
done


# Note: The "eval" is needed to get the quotes right in the command string
ostype=`uname -s`
if [ x$ostype = "xInterix" ]
then
  eval $SYSDIR/bin/cryomon.exe $GUIFLAG &


else
  #echo -------- vnmruser=$vnmruser
  #echo -------- USERDIR=$USERDIR
  #echo -------- DISPLAY=$DISPLAY

  eval $SYSDIR/jre/bin/java -mx128m -classpath $SYSDIR/java/cryomon.jar -Dsysdir=$SYSDIR -Duserdir=$USERDIR vnmr.cryomon.CryoMonitorControls  $VNMRJ $GUIFLAG $MASTER $DEBUG &

fi

