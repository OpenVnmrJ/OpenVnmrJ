#! /bin/csh
# '@(#)hermes.sh 22.1 03/24/08 2003-2004 '
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

set TCLDIR=$vnmrsystem/tcl
setenv TCL_LIBRARY $TCLDIR/tcllibrary
setenv TK_LIBRARY  $TCLDIR/tklibrary

cd $TCLDIR/bin
./vnmrwish -f hermes.tk "$1" &

