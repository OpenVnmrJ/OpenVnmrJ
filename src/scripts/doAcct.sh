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

SCRIPT=$(basename "$0")

usage() {
    cat <<EOF

usage:
    $SCRIPT        get accounting records with default startdate and endDate
    $SCRIPT cut    get accounting records with default startdate and endDate and remove
                   records prior to and including endDate

    The default endDate is the current date.
    The default startDate is one month prior to the current date.
    The option  -from yyyy/mm/dd    specifies an alternate startDate
    The option  -to   yyyy/mm/dd    specifies an alternate endDate

EOF
    exit 1
}

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
doCut=0
startDate=$(date -d "$sYear"/"$sMon"/"$day" +"%b %d %Y")
endDate=$(date +"%b %d %Y")

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -cut | cut)        doCut=1; ;;
        -f | -from)        startDate=$(date -d "$2" +"%b %d %Y"); shift ;;
        -t | -to)          endDate=$(date -d "$2" +"%b %d %Y"); shift ;;
        -h | -help)        usage ;;
        *)
            # unknown option
            echo "unrecognized argument: $key"
            usage
            ;;
    esac
    shift
done

if [[ "x${startDate}" = "x" ]] || [[ "x${endDate}" = "x" ]] ;
then
   usage
fi

javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

if [[ ${doCut} -eq 1 ]] ;
then
   $javabin -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" -cut
   $javabin -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" \
            -in "${saveDir}"/acctLog.xml -summary -summaryCsv -acctCsv
else
   $javabin -jar $vnmrsystem/java/account.jar -saveDir "${saveDir}" \
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
