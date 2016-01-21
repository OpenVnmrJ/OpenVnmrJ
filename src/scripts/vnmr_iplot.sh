#! /bin/sh
: '@(#)vnmr_iplot.sh 22.1 03/24/08 1991-2006 '
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

#if (test x$vnmrsystem = "x")
if [ x$vnmrsystem = "x" ]
then
   if [ x$ostype = "xInterix" ]; then
      . /vnmr/user_templates/.vnmrenvbash
   else
      source /vnmr/user_templates/.vnmrenvsh
   fi
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
ostype=`uname -s`
sysdir=$vnmrsystem
userdir=$vnmruser
shtoolcmd="/bin/sh"
shtooloption="-c"
javabin="$vnmrsystem/jre/bin/java"

if [ x$ostype = "xInterix" ]
then
   vjclasspath=`/bin/unixpath2win "$vjclasspath"`
   sysdir=`/bin/unixpath2win "$sysdir"`
   userdir=`/bin/unixpath2win "$userdir"`
   shtoolcmd="$SFUDIR\common\ksh.bat"
   shtooloption="-lc"
   javabin="/vnmr/jre/bin/java.exe"
fi


if [ x$ostype = "xInterix" ]
then
     $vjshell "$javabin"  \
       -classpath $vjclasspath \
       -Dsysdir=$sysdir -Duserdir=$userdir \
       -Duser=$USER -Dpersona=$itype \
       -Dsfudirwindows="$SFUDIR" -Dsfudirinterix="$SFUDIR_INTERIX" \
       -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" \
       -DiplotDatafile=$iplotData \
       -Djava.library.path="C:/SFU/vnmr/lib" \
       vnmr.ui.VnmrPlot 
else
     $vjshell $vnmrsystem/jre/bin/java \
        -classpath $vjclasspath \
        -Dsysdir=$sysdir -Duserdir=$userdir \
        -Duser=$USER -Dpersona=$itype \
        -Dsfudirwindows="$SFUDIR" -Dsfudirinterix="$SFUDIR_INTERIX" \
        -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" \
        -DiplotDatafile=$iplotData \
        -Djava.library.path="/vnmr/lib" \
        vnmr.ui.VnmrPlot &
fi

