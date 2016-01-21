#! /bin/sh
cd /var/log
filelist=`ls /var/log/wtmp*`
rm -f /vnmr/adm/tmp/last
for file in $filelist
do
   last -f $file >> /vnmr/adm/tmp/last.wtmp
done
grep -v begin /vnmr/adm/tmp/last.wtmp > /vnmr/adm/tmp/last.txt
rm -f /vnmr/adm/tmp/last.wtmp
echo "Done vnmr_last"
