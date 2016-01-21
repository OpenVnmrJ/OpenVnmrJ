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
basedir=`pwd`
n=`basename ${basedir}`
if [ x$n != "xcryomon" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"cryomon"
   # proguard is a symlink to proguard version being used
   proguarddir=${javadir}"/3rdParty/ProGuard/proguard/"
   # java is a symlink to java version being used
   javadir=${javadir}"/3rdParty/java"
else
   # basedir = basedir
   javadir=`dirname ${basedir}`
   javadir=`dirname ${javadir}`
   # proguard is a symlink to proguard version being used
   proguarddir=${javadir}"/3rdParty/ProGuard/proguard/"
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
/usr/bin/time ${javadir}/jre/bin/java -mx128m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar @${basedir}/Cryomon.pro > ${basedir}/cryomon.pro.log 2>&1
mv ${basedir}/cryomon.pro.map ${basedir}/cryomon.pro.map.$fileext
mv ${basedir}/cryomon.pro.log ${basedir}/cryomon.pro.log.$fileext
cp ${basedir}/cryomon.prodir/cryomon.jar ${basedir}/cryomon.jar.pro
#../../3rdParty/java/bin/java -jar ../../3rdParty/ProGuard/proguard4.3/lib/proguard.jar @VnmrJ.pro
