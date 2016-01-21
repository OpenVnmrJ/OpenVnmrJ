: '@(#)rkermit.sh 22.1 03/24/08 1991-1996 '
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
# Shellscript for starting kermit server from the RRI mapping computer
#
# September 19, 1991   Sandy Farmer
#

# ----  start up Sun KERMIT program from the RRI mapping computer
if (`arch` == "sun3") then
   kermit3
else
   kermit4
endif
