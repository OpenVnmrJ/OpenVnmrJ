#! /bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
#
# set -x
if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
admin=$($vnmrsystem/bin/fileowner $vnmrsystem/vnmrrev)
if [ $USER != $admin ]
then
   echo "OpenVnmrj accounting tool can only be used by the"
   echo "OpenVnmrJ administrator account $admin"
   exit
fi

saveDate=$(date +"%Y_%m_%d")
saveDir=$HOME/vnmrsys/accounting/${saveDate}

$vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar -saveDir ${saveDir} -cut
$vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar -saveDir ${saveDir} -in ${saveDir}/acctLog.xml -summary -summaryCsv -acctCsv

host=$(hostname)
curDate=$(date +"%b %d %Y")

cat $vnmrsystem/adm/accounting/acctHeader.ps | sed "s/HOSTNAME/${host}/" | sed "s/DATE/${curDate}/" > ${saveDir}/summary.ps
filename=${saveDir}/summary.txt
while IFS='' read -r line || [[ -n "$line" ]]; do
   echo "($line) show" >> ${saveDir}/summary.ps
   echo newline >> ${saveDir}/summary.ps
done < $filename
cat $vnmrsystem/adm/accounting/acctTail.ps >> ${saveDir}/summary.ps

convert ${saveDir}/summary.ps -flatten ${saveDir}/summary.png

exit 0
