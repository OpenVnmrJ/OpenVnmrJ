: '@(#)kermit.sh 22.1 03/24/08 1991-1996 '
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
# Shellscript for starting kermit server on Sun3 and Sun4 computers
#
# June 25, 1993   Sandy Farmer
#

# ---- check port validity
if (test ! x$1 = "xa")
then
   if (test ! x$1 = "xb")
   then
      echo "Invalid port assigned to Kermit process"
      echo "Syntax:  kermit a     or     kermit b"
      exit
   fi
fi

# ----  start up Sun KERMIT program in foreground
if (test `arch` = "sun3")
then
   kermit3.89 -l /dev/tty$1 -b 9600 -x -q
else
   kermit4.89 -l /dev/tty$1 -b 9600 -x -q
fi
