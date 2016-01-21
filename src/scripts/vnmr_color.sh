#! /bin/csh
# '@(#)vnmr_color.sh 22.1 03/24/08 1991-1996 '
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

setenv TCLDIR "$vnmrsystem"/tcl
setenv TCL_LIBRARY "$TCLDIR"/tcllibrary
setenv TK_LIBRARY "$TCLDIR"/tklibrary

cd "$TCLDIR"/bin
./pl_color "$1" "$2"&
