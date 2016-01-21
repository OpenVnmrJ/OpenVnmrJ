: '@(#)vbg.sh 22.1 03/24/08 1991-1996 '
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
############################################################
#                                                          #
#    vbg   -   UNIX shellscript to run VNMR processing in  #
#              background                                  #
#                                                          #
#                          S. Farmer   July 20, 1988       #
#                                                          #
############################################################


if [ $# = 0 ]
then
   echo " "
   echo "Experiment number must be specified."
   echo "Consult manual entry for operational details."
   echo " "
   exit
else
   if [ $# = 1 ]
   then
      echo " "
      echo "Command string must be specified."
      echo "Consult manual entry for operational details."
      echo " "
      exit
   else
      if [ $# = 2 ]
      then
         cd "$vnmruser"/exp$1
         Vnmr -mback -n$1 "$2" > exp$1_bgp.log &
      else
         if [ $# = 3 ]
         then
            cd "$vnmruser"/exp$1
            Vnmr -mback -n$1 "$2" > $3_bgp.log &
         else
            echo " "
            echo "Enter only a maximum of three arguments."
            echo "Consult manual entry for operational details."
            echo " "
            exit
         fi
      fi
   fi
fi
