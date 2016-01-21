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
 set -x
basedir=`pwd`
n=`basename ${basedir}`
if [ x$n != "xvnmrj" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"vnmrj"
   # proguard is a symlink to proguard version being used
   proguarddir=${javadir}"/3rdParty/ProGuard/proguard"
   # java is a symlink to java version being used
   javadir=${javadir}"/3rdParty/java"
else
   # basedir = basedir
   javadir=`dirname ${basedir}`
   javadir=`dirname ${javadir}`
   if [ `uname -s` != "Interix" ] 
   then
     # proguard is a symlink to proguard version being used
     proguarddir=${javadir}"/3rdParty/ProGuard/proguard"
     # java is a symlink to java version being used
     javadir=${javadir}"/3rdParty/java"
   else
     # synlinks don't work so use the real path for Inerix
     proguarddir=${javadir}"/3rdParty/ProGuard/proguard4.4"
     # Java is run native in windows thus paths need to be translate to windows
     proguarddir=`unixpath2win $proguarddir`
     # Windows JRE
     javadir=$javadir"/3rdparty/winJre6"
   fi
fi
echo VNMRJ BASENAME: $basedir
echo $javadir

fileext="`date '+%Y_%m_%d'`"
echo "Starting ProGuard..."
set -x
if [ `uname -s` = "Linux" ]
then
   CLASSPATH=${javadir}/lib:${javadir}/jre/lib
   echo $CLASSPATH
   /usr/bin/time ${javadir}/jre/bin/java -mx256m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar @${basedir}/VnmrJ.pro > ${basedir}/vnmrj.pro.log 2>&1
elif [ `uname -s` = "Darwin" ]
then
   /usr/bin/time /usr/bin/java -Xmx256m -jar ${proguarddir}/lib/proguard.jar @${basedir}/VnmrJ.pro > ${basedir}/vnmrj.pro.log 2>&1
elif [ `uname -s` = "Interix" ]
then
    # java is run Windows Native so path java uses must be WIndow paths not Interix
    CLASSPATH=`unixpath2win -m ${javadir}/lib:${javadir}/jre/lib`
    javadir="/dev/fs/C/Program Files/Java/jdk1.6.0_23"
    probasedir=`unixpath2win $basedir`
    # had to use a different .pro file since it has a hard code path in it, thus a windows path version was needed.
    "${javadir}/bin/java.exe" -mx128m -classpath $CLASSPATH -jar ${proguarddir}/lib/proguard.jar @${probasedir}/VnmrJ_Win.pro > ${basedir}/vnmrj.pro.log 2>&1
fi
mv ${basedir}/vnmrj.pro.map ${basedir}/vnmrj.pro.map.$fileext
mv ${basedir}/vnmrj.pro.log ${basedir}/vnmrj.pro.log.$fileext
cp ${basedir}/vnmrj.prodir/vnmrj.jar ${basedir}/vnmrj.jar.pro
#../../3rdParty/java/bin/java -jar ../../3rdParty/ProGuard/proguard4.3/lib/proguard.jar @VnmrJ.pro
