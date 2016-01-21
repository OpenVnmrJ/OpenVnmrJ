: '@(#)roadkill.sh 22.1 03/24/08 1991-1996 '
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
: /bin/sh
: 		Author: Greg Brissey  3/3/89
#
# Set permission so that all users can kill or start Acqproc
#
echo " "
echo "This script changes the permissions on Acqproc and killacq"
echo "to allow anyone to start and stop Acqproc with root permissions."
echo " "
echo " "
sleep 1
echo -n  "Continue?  y or n: "
read answr
if test ! x$answr = "xy"
then
   echo "Script Aborted"
   exit 1
fi
if (test ! `whoami` = "root")
then
   echo "Please login as Root"
   echo "then restart" 
   exit 1
fi
if (test -d /vnmr/acqbin)
then
  ( cd /vnmr/acqbin;
  /etc/chown root Acqproc killacq;
  chmod ug+x Acqproc killacq;
  chmod ug+s Acqproc killacq;
  chmod ug+x startacqproc killacqproc;
  chmod g+w /vnmr/acqbin )
else
  echo "No /vnmr/acqbin Directory Present on System"
  exit 1
fi
exit 0
