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

ostype=`uname -s`

if [ x$vnmrsystem = "x" ]
then
   if [ x$ostype = "xInterix" ]; then
      . /vnmr/user_templates/.vnmrenvbash
   else
      source /vnmr/user_templates/.vnmrenvsh
   fi
fi

if [ x$VGLRUN = "x" ]
then
    vjshell=""
else
    vjshell="$VGLRUN"
fi

javabin="$vnmrsystem/jre/bin/java"

jclasspath=$vnmrsystem/java/simplemovie.jar

if [ x$ostype = "xInterix" ]
then
   jclasspath=`/bin/unixpath2win "$jclasspath"`
   javabin="/vnmr/jre/bin/java.exe"
   $vjshell "$javabin"  \
       -classpath $jclasspath \
       -Djava.library.path="C:/SFU/vnmr/lib" \
       SimpleMovie $* &
else
  $vjshell "$javabin"  \
       -classpath $jclasspath \
       SimpleMovie $* &
fi

