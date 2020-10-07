#! /bin/sh
: '@(#)simplemovie.sh 1.0 07/18/11 2011-2016 '
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

if [ x$vnmrsystem = "x" ]
then
   source /vnmr/user_templates/.vnmrenvsh
fi

if [ x$VGLRUN = "x" ]
then
    vjshell=""
else
    vjshell="$VGLRUN"
fi

javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

jclasspath=$vnmrsystem/java/simplemovie.jar

$vjshell "$javabin"  \
       -classpath $jclasspath \
       SimpleMovie $* &

