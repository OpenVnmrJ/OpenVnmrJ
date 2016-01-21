: '@(#)vnmrj.img.sh 22.1 03/24/08 1999-2001 '
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
#  theme could be default, ocean, aqua, emerald, ruby
#
: /bin/sh

debug="n"
itype="exp"
laf="metal"
theme="default"

if test $# -gt 0
then
    if test $1 = "-debug"
    then
        debug="y"
	if test $# -gt 1
        then
           itype=$2
        fi
    else 
        itype=$1

    fi
fi

if test $itype = "adm"
then
    laf="metal"
fi

if test x$debug = "xy"
then

    $vnmrsystem/jre/bin/java -mx256m -splash:/vnmr/iconlib/Splash_VnmrJ.png \
    -classpath $vnmrsystem/java/vnmrj.jar -Ddbhost=localhost -Dsysdir=$vnmrsystem -Duserdir=$vnmruser -Duser=$USER -Dfont="Dialog plain 14" -Dlookandfeel=$laf -Dtheme=$theme -DqueueArea=yes -Dbatchupdates=yes -Dpersona=$itype -Dsavepanels=10 -DshowMem=yes -DSQAllowNesting=false vnmr.ui.VNMRFrame &

else

    $vnmrsystem/jre/bin/java -mx256m -splash:/vnmr/iconlib/Splash_VnmrJ.png \
    -classpath $vnmrsystem/java/vnmrj.jar -Ddbhost=localhost -Dsysdir=$vnmrsystem -Duserdir=$vnmruser -Duser=$USER -Dfont="Dialog plain 14" -Dlookandfeel=$laf -Dtheme=$theme -DqueueArea=yes -Dbatchupdates=yes -Dpersona=$itype -Dsavepanels=10 -DSQAllowNesting=false vnmr.ui.VNMRFrame >>/dev/null &

fi
