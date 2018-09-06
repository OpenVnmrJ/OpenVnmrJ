#! /bin/csh
# '@(#)atregbuilt.sh 22.1 03/24/08 2003-2005 '
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

setenv TCLDIR        "$vnmrsystem/tcl"
setenv TCL_LIBRARY   "$TCLDIR/tcllibrary"
set TCLSH = "$vnmrsystem/bin/tclsh"
set cmd = "$vnmrsystem/tcl/bin/atregbuilt.tcl"

"$TCLSH" "$cmd" "$1" "$2" "$3" "$4" "$5" "$6" < /dev/null > /dev/null
