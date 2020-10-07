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

# PATH=/vnmr/bin2:${PATH}:/usr/ucb
# export PATH

if [ $# -eq 0 -o $# -gt 1 ]
then
   echo " vnmr_iplot error: "
   echo "  Usage --  vnmr_iplot filename "
   exit 1
fi

if [ x$vnmrsystem = "x" ]
then
   source /vnmr/user_templates/.vnmrenvsh
fi

vjclasspath=$vnmrsystem/java/vnmrj.jar
vjclasspath=/home/vjbuild/jvnmr1/vnmrj.jar

if [ x$VGLRUN = "x" ]
then
    vjshell=""
else
    vjshell="$VGLRUN"
fi

iplotData=$1
itype="exp"
sysdir=$vnmrsystem
userdir=$vnmruser
shtoolcmd="/bin/sh"
shtooloption="-c"
javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

$vjshell $javabin \
        -classpath $vjclasspath \
        -Dsysdir=$sysdir -Duserdir=$userdir \
        -Duser=$USER -Dpersona=$itype \
        -Dsfudirwindows="$SFUDIR" -Dsfudirinterix="$SFUDIR_INTERIX" \
        -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" \
        -DiplotDatafile=$iplotData \
        -Djava.library.path="/vnmr/lib" \
        vnmr.ui.VnmrPlot &

