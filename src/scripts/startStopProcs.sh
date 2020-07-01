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
#  startStopProcs - This script is run as root
#                    On newer systems with systemd in use, unless the
#                    procs are started by the systemd system, they will
#                    be terminated when the user that starts them logs out.
#

# set -x

start_nmrwebd() {
  nmrwebd_root=/vnmr/web
  nmrwebd_home=${nmrwebd_root}/scripts
  nmrwebd_exe=nmrwebd
  nmrwebd_path=${nmrwebd_home}/${nmrwebd_exe}
  nmrwebd_pidfile=${nmrwebd_root}/run/${nmrwebd_exe}.pid
  if [ -x $nmrwebd_path ]; then
     (cd $nmrwebd_home && ./$nmrwebd_exe 2> /dev/null &)
  fi
}

vnmrsystem=/vnmr
npids=$(pgrep Expproc)
if [[ -z $npids ]]; then
   echo "Starting Acquisition communications."
   if [[ -f /usr/lib/systemd/system/vnmr.service ]]; then
      systemctl start vnmr.service
   else
      ${vnmrsystem}/acqbin/ovjProcs &
      start_nmrwebd
   fi
else
   echo "Stopping Acquisition communications."
   ${vnmrsystem}/acqbin/ovjProcs
fi
