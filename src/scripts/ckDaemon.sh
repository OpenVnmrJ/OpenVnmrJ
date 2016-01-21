
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
#!/bin/sh
#

   scankey=""

   # I cannot figure out how to get get ps in Linux to give the actual
   # name of the scanlog process.  However, the pid of the process is
   # saved by S99scanlog when it starts scanlog.  It is saved in
   # /vnmr/adm/scanlogpid.  We will just check to see if that pid is
   # still a running pid.  If so, it must be running.
   
   # Get the pid
   pid=`cat /vnmr/adm/scanlogpid`

   # Have ps only output the pid
   scankey=`ps -o pid | grep $pid` 

   if [ x$scankey = "x$pid" ] 
   then
     echo "scanlog daemon is running."
   else
     echo "WARNING: scanlog daemon is not running."
   fi
   
