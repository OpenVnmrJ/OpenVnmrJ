#! /bin/csh -f
#:'@(#)vnmr.sh 22.1 03/24/08 1991-1996 '
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

# If we are at the console, do an autostart window

if ($#argv == 1) then
   switch ($1)
        case -[Vv]:
        case -[Vv]er:
        case -[Vv]ersion:
                echo
                cat "$vnmrsystem"/vnmrrev
                echo
                exit
                breaksw
        default:
                exit
                breaksw
   endsw
endif

set ostype = `vnmr_uname`
if ( $ostype == "AIX" ) then
   if ( `tty | sed 's/[1-9]/0/'` == "/dev/hft/0" ) then
        echo -n " Motif Windows starting (^C aborts) "
        xinit -bs
   else
        vn&
   endif
   exit 1
else if ( $ostype == "IRIX" ) then
        vn&
        exit 1
endif
# User is in the SUN environment
if (`tty` == "/dev/console") then
   echo "OpenWindows starting (^C aborts)"
   setenv DISPLAY unix:0.0
   openwin -noauth
else
   vn&
endif
