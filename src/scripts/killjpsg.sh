: '@(#)killjpsg.sh 22.1 03/24/08 1999-2002 '
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
#
# killjpsg script to check & terminate if a jpsg is running and communicating to Vnmr
#

quiet=0
if [ $# = 1 ]; then
  if [ $1 = "quiet" ]; then
      quiet=1
  fi
fi

if [ ! -f /vnmr/acqqueue/jinfo1.$USER ]; then
  if [ $quiet != 1 ]; then
    echo "Jpsg is not running or communicating to Vnmr"
  fi


else       

  # PID registered in /vnmr/acqqueue/jinfo1.<USER NAME>

  jpsgPID=` cat /vnmr/acqqueue/jinfo1.$USER | awk '{ print $2 }' `

  jpsgProc=` ps -ef | grep $jpsgPID | grep -v grep `

  if [ "x$jpsgProc" != "x" ]; then

    jpsgowner=`ps -ef | grep $jpsgPID | grep -v grep | awk '{ print $1}' `

    if [ ! $jpsgowner = $LOGNAME ]; then
      if [ $quiet != 1 ]; then
        echo "current login user $LOGNAME does not own Jpsg process. Cannot terminate Jpsg"
        ps -ef | grep $jpsgPID | grep -v grep | awk '{ print "Jpsg  owner=" $1 " PID=" $2 " " $10 }'
        echo "Jpsg process is running"
      fi
    else
      if [ $quiet != 1 ]; then
        echo "terminating Jpsg  .. \c"
      fi
      kill -9 $jpsgPID
      sleep 1
      jpsgProc2=` ps -ef | grep $jpsgPID | grep -v grep `
      if [ "x$jpsgProc2" != "x" ]; then
       if [ $quiet != 1 ]; then
         echo "unable to kill Jpsg process"
         echo "Jpsg process is running"
       fi
      else
       /usr/bin/rm -f /vnmr/acqqueue/jinfo1.$USER
       if [ $quiet != 1 ]; then
         echo " done"
       fi
      fi
    fi

  else

    if [ $quiet != 1 ]; then
      echo "Jpsg is not running or communicating to Vnmr"
    fi

  fi


fi

# end
