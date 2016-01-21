: '@(#)vnmrprint.old.sh 22.1 03/24/08 1991-1996 '
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
#  vnmrprint
#
#  General purpose print shell for the vxr system
#
#  Usage  vnmrprint filename [printcap]
#
#  If no printcap is entered, lp will be the default
#  Note that PostScript printers do not use portrait
#
if [ $# = 1 ]   #  No printerName given, use default
   then
      expand $1 | sed -e 's/^//' | fold -80 | /vnmr/bin/portrait | lpr -s -h -Plp 
else
   if [ $# = 2 ]  # if printerName given, find out which one
      then
         expand $1 | sed -e 's/^//' | fold -80 | /vnmr/bin/portrait | lpr -s -h -P$2 
   else
      if [ $# = 3 ]  # printerType is given, remove file after printing
         then
            if [ $3 = PS_A ]  # If PostScript printer,  do not use portrait
               then
                  expand $1 | sed -e 's/^//' | fold -80 | lpr -s -h -P$2 
            else
               expand $1 | sed -e 's/^//' | fold -80 | /vnmr/bin/portrait | lpr -s -h -P$2 
            fi
            rm $1
      else
         echo vnmrprint error: Usage --  vnmrprint filename \[printcap\]
      fi
   fi
fi
