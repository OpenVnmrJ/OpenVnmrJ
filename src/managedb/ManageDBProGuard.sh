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
if [ x$n != "xmanagedb" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"managedb"
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

fileext="`date '+%Y_%m_%d'`"
echo "Starting ProGuard..."
set -x
if [ `uname -s` != "Darwin" ]
then
   CLASSPATH=${javadir}/lib:${javadir}/jre/lib
   echo $CLASSPATH
   /usr/bin/time ${javadir}/jre/bin/java -mx128m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar @${basedir}/ManageDB.pro > ${basedir}/managedb.pro.log 2>&1
else
   /usr/bin/time /usr/bin/java -mx128m -jar ${proguarddir}/lib/proguard.jar @${basedir}/ManageDB.pro > ${basedir}/managedb.pro.log 2>&1
fi
mv ${basedir}/managedb.pro.map ${basedir}/managedb.pro.map.$fileext
mv ${basedir}/managedb.pro.log ${basedir}/managedb.pro.log.$fileext
cp ${basedir}/managedb.prodir/managedb.jar ${basedir}/managedb.jar.pro
