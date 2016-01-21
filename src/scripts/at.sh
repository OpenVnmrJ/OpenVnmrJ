#! /bin/csh
# '@(#)at.sh 22.1 03/24/08 2003-2005 '
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

set ostype = `uname -s`
if ( "x$ostype" == "xLinux" ) then
   set WISH = "$vnmrsystem/tcl/bin/vnmrWish"
   set cmd = "$vnmrsystem/tcl/bin/at.tcl"
else
   if ( "x$ostype" == "xInterix" ) then
      cd $vnmrsystem/tcl/bin
      set WISH = "./wish84.exe"
      setenv TCL_LIBRARY   "../tcllibrary"
      setenv TK_LIBRARY    "../tklibrary"
      set cmd = "./at.tcl"
   else
      setenv TCLDIR        "$vnmrsystem/tcl"
      setenv TCL_LIBRARY   "$TCLDIR/tcllibrary"
      setenv TK_LIBRARY    "$TCLDIR/tklibrary"
      set WISH = "$vnmrsystem/tcl/bin/vnmrWish"
      set cmd = "$vnmrsystem/tcl/bin/at.tcl"
   endif
endif

"$WISH" "$cmd" "$1" < /dev/null > /dev/null
