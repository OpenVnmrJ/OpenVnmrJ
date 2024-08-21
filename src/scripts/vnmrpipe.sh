#!/bin/csh
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
if (!($?NMRBASE)) then
   if ( -f /vnmr/nmrpipe/com/nmrInit.link.com ) then
      source /vnmr/nmrpipe/com/nmrInit.link.com
   else
      set ostype=`uname -s`
      if ( x$ostype == "xLinux" ) then
         source /vnmr/nmrpipe/com/vj_nmrInit.linux9.com
      else # MacOS
         source /vnmr/nmrpipe/com/vj_nmrInit.mac.com
      endif
   endif
   source /vnmr/nmrpipe/dynamo/com/dynInit.com
endif

if ($#argv == 1) then
   $1
else
   $1 $argv[2-]
endif
