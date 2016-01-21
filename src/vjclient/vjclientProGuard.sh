#! /bin/sh
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
#set +x
basedir=`pwd -P`
n=`basename ${basedir}`
if [ x$n != "xvjclient" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"vjclient"
   # proguard is a symlink to proguard version being used
   proguarddir=${javadir}"/3rdParty/ProGuard/proguard"
   # java is a symlink to java version being used
   javadir=${javadir}"/3rdParty/java"
else
   # basedir = basedir
   javadir=`dirname ${basedir}`
   javadir=`dirname ${javadir}`
   # proguard is a symlink to proguard version being used
   proguarddir=${javadir}"/3rdParty/ProGuard/proguard"
   # java is a symlink to java version being used
   javadir=${javadir}"/3rdParty/java"
fi
echo VNMRJ BASENAME: $basedir
echo $javadir

CLASSPATH=${javadir}/lib:${javadir}/jre/lib
echo $CLASSPATH
fileext="`date '+%Y_%m_%d'`"
echo "Starting ProGuard..."
set -x
/usr/bin/time ${javadir}/jre/bin/java -mx128m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar -verbose @${basedir}/vjclient.pro > ${basedir}/vjclient.pro.log  2>&1
mv ${basedir}/vjclient.pro.map ${basedir}/vjclient.pro.map.$fileext
mv ${basedir}/vjclient.pro.log ${basedir}/vjclient.pro.log.$fileext
cp ${basedir}/vjclient.prodir/vjclient.jar ${basedir}/vjclient.jar.pro
#../../3rdParty/java/bin/java -jar ../../3rdParty/ProGuard/proguard4.3/lib/proguard.jar @VnmrJ.pro
