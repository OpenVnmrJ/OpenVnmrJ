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
if [ x$n != "xprobeid" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"probeid"
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
/usr/bin/time ${javadir}/jre/bin/java -mx128m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar @${basedir}/ProbeId.pro > ${basedir}/probeid.pro.log  2>&1
if [ -f ${basedir}/probeid.pro.map ]; then
   mv ${basedir}/probeid.pro.map ${basedir}/probeid.pro.map.$fileext
fi
if [ -f ${basedir}/probeid.pro.log ]; then
   mv ${basedir}/probeid.pro.log ${basedir}/probeid.pro.log.$fileext
fi
if [ -f ${basedir}/probeid.pro.dump ]; then
   mv ${basedir}/probeid.pro.dump ${basedir}/probeid.pro.dump.$fileext
fi

# ProGuard builds a non-executable probeid.jar.pro file, so use the
# unobfuscated one until we can address the problem.
#cp ${basedir}/probeid.prodir/probeid.jar ${basedir}/probeid.jar.pro
rm -f ${basedir}/probeid.jar.pro; ln -s ${basedir}/probeid.jar ${basedir}/probeid.jar.pro
