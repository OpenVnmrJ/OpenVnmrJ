: '@(#)killstat.sh 22.1 03/24/08 1991-1996 '
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
: use sh
:	search for Acqstat in process table 
:       If the argument to killstat matchs the -P option
:       of the Acqstat then kill that Acqstat
:

#  Get a list of Acqstat PIDs to be killed in one fell swoop.
#  Although each grep command would pick itself up (as a process), neither will
#  pick up the other grep process.  That is, grep -P<process ID> will miss the
#  grep Acqstat, and vice versa.  Prior to the awk command, we have a complete
#  line of ps output for each command; the awk program eliminates everything
#  except the Acqstat PID.

pidlist=`ps -ef | grep Acqstat | grep "\-P$1" | awk '{ print $2 }'`
for acqpid in $pidlist
do
  kill -2 $acqpid
done
