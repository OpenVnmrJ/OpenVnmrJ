#! /bin/csh
# '@(#)atrecord.sh 22.1 03/24/08 2003-2006 '
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
   set TCLSH = /usr/bin/tclsh
   set cmd = "$vnmrsystem/tcl/bin/atrecord.tcl"
else
   if ( "x$ostype" == "xInterix" ) then
      cd $vnmrsystem/tcl/bin
      set TCLSH = "./tclsh84.exe"
      setenv TCL_LIBRARY   "../tcllibrary"
      set cmd = "./atrecord.tcl"
   else
      setenv TCLDIR        "$vnmrsystem/tcl"
      setenv TCL_LIBRARY   "$TCLDIR/tcllibrary"
      set TCLSH = "$vnmrsystem/tcl/bin/tclsh"
      set cmd = "$vnmrsystem/tcl/bin/atrecord.tcl"
   endif
endif

"$TCLSH" "$cmd" "$1" "$2" "$3" "$4" $argv[5-] < /dev/null > /dev/null
