: '@(#)vnmr_jadmin.sh 22.1 03/24/08 1991-1994 '
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
#! /bin/sh
#To start VNMR Admintool
java -classpath $vnmrsystem/java/VnmrAdmin.jar -Dsysdir=$vnmrsystem -Duserdir=$vnmruser -Duser=$USER -DbackingStore=false  VnmrAdmin &
