#!/bin/bash
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
#  acqcomm - run startStopProcs with sudo
#            On newer systems with systemd in use, unless the
#            procs are started by the systemd system, they will
#            be terminated when the user that starts them logs out.
#
#  acqcomm        - start the acquisition communication system if
#                   it is not running and stop it if it is running
#  acqcomm start  - start the acquisition communication system
#  acqcomm stop   - stop  the acquisition communication system
#  acqcomm status - report status of acquisition communication system
#  acqcomm help   - display usage of acqcomm
#
# set -x

if [[ $# -gt 0 ]]; then
   npids=$(pgrep Expproc)
   if [[ $1 = "help" ]]; then
      echo ""
      cat /vnmr/manual/acqcomm
      echo ""
      exit
   elif [[ $1 = "status" ]]; then
      if [[ -z $npids ]]; then
         echo "Acquisition communication system is stopped"
      else
         echo "Acquisition communication system is started"
      fi
      exit
   elif [[ $1 = "start" ]] && [[ ! -z $npids ]]; then
      echo "Acquisition communication system is already started"
      exit
   elif [[ $1 = "stop" ]] && [[ -z $npids ]]; then
      echo "Acquisition communication system is already stopped"
      exit
   fi
fi

sudo /vnmr/acqbin/startStopProcs
