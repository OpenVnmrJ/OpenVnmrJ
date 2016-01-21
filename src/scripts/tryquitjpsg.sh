: '@(#)tryquitjpsg.sh 22.1 03/24/08 2003-2004 '
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
# tryquitjpsg 
# called from Vnmr on exit. Will terminate Jpsg is no other Vnmr with same user id running
#

quiet=0
if [ $# = 1 ]; then
  if [ $1 = "quiet" ]; then
      quiet=1
  fi
fi

if [ ! -f /vnmr/acqqueue/jinfo1.$USER ]; then

  # jpsg is not running
  if [ $quiet != 1 ]; then
    echo "Jpsg is not running or communicating to Vnmr"
  fi


else       

  # jpsg is running
  numvnmrs=`/usr/bin/ps -ef | /usr/bin/grep "master Vnmr" | /usr/bin/grep $USER | /usr/bin/grep -v /usr/bin/grep | /usr/bin/wc | /usr/bin/awk '{ print $1 }' `

  if [ $numvnmrs = 1 ]; then
   # only one Vnmr running, OK to kill jpsg
     /vnmr/bin/killjpsg quiet
  fi

fi

# end
