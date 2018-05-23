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
   set ostype=`uname -s`
   if ( x$ostype == "xLinux" ) then
      if ( -f /vnmr/nmrpipe/com/nmrInit.linux212_64.com ) then
         source /vnmr/nmrpipe/com/nmrInit.linux212_64.com
      else
         source /vnmr/nmrpipe/com/vj_nmrInit.linux9.com
      endif
   else if ( x$ostype == "xDarwin" ) then
      if ( -f /vnmr/nmrpipe/com/nmrInit.mac11_64.com ) then
         source /vnmr/nmrpipe/com/nmrInit.mac11_64.com
      else
         source /vnmr/nmrpipe/com/vj_nmrInit.mac.com
      endif
   else # Interix
      source /vnmr/nmrpipe/com/vj_nmrInit.winxp.com
   endif
   source /vnmr/nmrpipe/dynamo/com/dynInit.com
endif

if ($# == 1) then
   $1
else
   $1 $argv[2-]
endif
