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
# establish some directories
basedir=`pwd`
n=`basename ${basedir}`
if [ x$n != "xmanagedb" ]
then
   javadir=`dirname ${basedir}`
   basedir=${basedir}/"managedb"
   dashodir=${javadir}"/3rdParty/DashO/"
#   javadir=${javadir}"/3rdParty/jdk1.6.0_01"
   javadir=${javadir}"/3rdParty/java"
else
   # basedir = basedir
   javadir=`dirname ${basedir}`
   javadir=`dirname ${javadir}`
   dashodir=${javadir}"/3rdParty/DashO/"
#   javadir=${javadir}"/3rdParty/jdk1.6.0_01"
   javadir=${javadir}"/3rdParty/java"
fi
echo VNMRJ BASENAME: $basedir
echo $javadir

#
# setup directory for DashO
# and create the symbolic links to the com and org directories
#
if [ ! -d ${basedir}/managedb ]
then
  mkdir -p ${basedir}/managedb
fi
if [ ! -h ${basedir}/managedb/com ]
then
  ( cd managedb; ln -s ../classes/com .  )
fi
if [ ! -h ${basedir}/managedb/org ]
then
  ( cd managedb; ln -s ../classes/org .  )
fi
( cd $basedir/managedb ; rm -rf *.jar *.class postgresql vnmr )
cp=${basedir}:${dashodir}/DashOPro_3.3/DashoPro.jar
dashojars=`ls ${dashodir}/DashOPro_3.3/lib/*.jar`
for dj in $dashojars;do
   cp=$cp:$dj
done
CLASSPATH=$cp:${javadir}/lib:${javadir}/jre/lib:
echo $CLASSPATH
echo "starting Dasho..."
cd ${basedir}
${javadir}/bin/java -Xmx64000000 -classpath $CLASSPATH DashoPro -f -v ./ManageDB.dox
fileext="`date '+%Y_%m_%d'`"
mv managedb.map managedb.map.$fileext
mv managedb.log managedb.log.$fileext
cd $basedir/managedb
jar -cvf managedb.jar.dasho *
cp -p managedb.jar.dasho ..
