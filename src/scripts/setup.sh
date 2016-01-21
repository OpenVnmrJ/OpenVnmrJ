#! /bin/sh
#
# this script copies itself off the DVD so that it can be executed
# to umount and remount the DVD with exec option
# without this remount with exec permission load.nmr can not be run
#
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
   echo ""
   echo "Script requires root permissions"
   echo "Please su to root, Then rerun this script".
   echo ""
   exit 0
fi
if [ $# = 0 ]; then

   #determine the mount point of the DVD
   # ls -l /dev/dvd gives the real device.
   #  lrwxrwxrwx 1 root root 4 Dec 16 00:08 /dev/dvd -> scd0
   dvddev=`ls -l /dev/dvd | awk 'BEGIN { FS = ">" } { print $2 }'`
   # mount - give the mounted FS
   # /dev/scd0 on /media/test3 type iso9660 (rw,noexec,nosuid,nodev,uid=501)
   # grep for the device scd0, then extract the mount point
   mntpt=`mount | grep $dvddev | awk 'BEGIN { FS = " "} {print $3}'`

   # copy the script off the DVD
   cp -p ${mntpt}/setup /tmp
   cd /tmp
   # invoke the script from the background so that this process exits
   # thus the DVD is not 'Busy' so that it can be umounted.
   (sh /tmp/setup doit & )
   exit 0
else
  echo "Remounting & Executing load.nmr"
  sleep 1
  # was cdrom, however systems with both cdrom & dvd caused a problem
  # thus use /dev/dvd to use appropriate drive
  umount -f /dev/dvd
  mkdir -p /media/vnmrj
  mount /dev/dvd /media/vnmrj
  cd /media/vnmrj
  ./load.nmr
fi

