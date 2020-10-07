#! /bin/bash
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

javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

"$javabin" -Dsysdir="$vnmrsystem" -Duserdir="$vnmruser" -Duser=$USER -cp "$vnmrsystem"/java/jplot.jar PlotConfig $* &
