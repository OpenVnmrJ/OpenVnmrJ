#!/bin/sh 
#
# simulate accelerated cyrogen level changes
#
n2dir='-'
hedir='-'
n2=60
he=70
timestamp=`date +"%y-%m-%d.%H:%M:%S"`
cryoData=/vnmr/cryo/cryomon/cryomonData.txt
cryoDataBackup=${cryoData}.${timestamp}

echo "hit enter to drain the cryogens"
read x

backup()
{
   echo "saving cryogen level data  to ${cryoDataBackup}"
   cp -p ${cryoData} ${cryoDataBackup}
}

cleanup() 
{
   echo "restoring cryogen level data"
   mv -f $cryoDataBackup $cryoData;
   exit 0
}

backup
trap cleanup SIGINT

while [ 1 ] ; do
  n2=`expr $n2 $n2dir 5`
  he=`expr $he $hedir 3`
  if (( $n2 < 0 )); then n2=1; n2dir='+'; fi
  if (( $n2 > 99 )); then n2=99; n2dir='-'; fi
  if (( $he < 0 )); then he=1; hedir='+'; fi
  if (( $he > 99 )); then he=99; hedir='-'; fi
  msg=`printf "22:44:17:04:42,0%d,0%d,12345" $he $n2`
  echo $msg >> /vnmr/cryo/cryomon/cryomonData.txt
  sleep 1
done

