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

setenv TCLDIR        "$vnmrsystem/tcl"
setenv TCL_LIBRARY   "$TCLDIR/tcllibrary"
set TCLSH = "$vnmrsystem/bin/tclsh"
set cmd = "$vnmrsystem/tcl/bin/atrecord.tcl"

"$TCLSH" "$cmd" "$1" "$2" "$3" "$4" $argv[5-] < /dev/null > /dev/null
