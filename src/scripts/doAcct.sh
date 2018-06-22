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
if [ x"$vnmrsystem" = "x" ]
then
   vnmrsystem=/vnmr
fi
admin=$($vnmrsystem/bin/fileowner $vnmrsystem/vnmrrev)
if [ "$USER" != "$admin" ]
then
   echo "OpenVnmrj accounting tool can only be used by the"
   echo "OpenVnmrJ administrator account $admin"
   exit
fi

saveDate=$(date +"%Y_%m_%d")
saveDir=$HOME/vnmrsys/accounting/${saveDate}

eYear=$(date +"%Y")
eMon=$(date +"%m")
day=$(date +"%d")
if [[ $eMon == "01" ]];
then
   sMon=12
   sYear=$((eYear-1))
else
   sMon=$((eMon-1))
   sYear=$eYear
fi
startDate=$(date -d "$sYear"/"$sMon"/"$day" +"%b %d %Y")
endDate=$(date +"%b %d %Y")

if [[ $# -gt 0 ]] ;
then
   $vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" -cut
   $vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" \
            -in "${saveDir}"/acctLog.xml -summary -summaryCsv -acctCsv
else
   $vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" \
            -startDate "${startDate}" -endDate "${endDate}" -summary -summaryCsv -acctCsv
fi

host=$(hostname)

sed "s/HOSTNAME/${host}/" $vnmrsystem/adm/accounting/acctHeader.ps |
                sed "s/ENDDATE/${endDate}/" |
                sed "s/STARTDATE/${startDate}/" > "${saveDir}"/summary.ps

filename=${saveDir}/summary.txt
while IFS='' read -r line || [[ -n "$line" ]]; do
   echo "($line) show" >> "${saveDir}"/summary.ps
   echo newline >> "${saveDir}"/summary.ps
done < "$filename"
cat $vnmrsystem/adm/accounting/acctTail.ps >> "${saveDir}"/summary.ps

convert "${saveDir}"/summary.ps -flatten "${saveDir}"/summary.png

exit 0
