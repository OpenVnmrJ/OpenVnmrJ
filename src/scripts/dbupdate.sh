#!/bin/csh
# '@(#)dbupdate.sh 22.1 03/24/08 1991-2003 '
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
# Script to start up managedb to update the database running under 'nice'.
# The optional arg "slow_ms" is used to slow things down internally so as
# not to use all of the cpu time available.  
# slow_ms = 0 is full speed
# slow_ms = 1000 would be using about 2-5 % of the cpu
# Remember, it is running under 'nice' so that any other process will be
# able to take the cpu away from this update anyway.  The default
# slow_ms for 'forever' is 1000.  The default slow_ms for 'once' is 0.


if ( $#argv < 1 ) then
   echo "usage: dbupdate stop | once [slow_ms] | forever [slow_ms]"
   exit 0
endif


if ( $1 == "stop" )  then 
    # Get pids of java managedb.jar processes
    set pslist = `ps -A -o pid,args | \grep -w java | \grep -v grep| \grep managedb | awk '{ print $1 }'`

    if ( $#pslist )  then
        while ($#pslist)     
            if ( $#pslist ) then
                echo  kill  $pslist[1]
                kill  $pslist[1]
                shift pslist
            endif
        end
        exit 0
    else
        echo "No dbupdate found"
        exit 0
    endif
endif

set pslist = `ps -A -o pid,args | \grep -w java | \grep -v grep| \grep managedb | awk '{ print $1 }'`

# if one is running and we were not asked to kill it, then error out
if ( $#pslist > 0 ) then
   echo "dbupdate is already running, cannot start another one"
   exit 0
else if ( $1 == "once" )  then
     # Run it once
     if ( $#argv > 1 ) then
         nice managedb update $2
     else
         nice managedb update
     endif
else if ( $1 == "forever" )  then 
     # Run it in a loop
     if ( $#argv > 1 ) then
         nice managedb updateloop $2
     else
         nice managedb updateloop
     endif
else
     echo "usage: dbupdate stop | once [slow_ms] | forever [slow_ms]"
endif
