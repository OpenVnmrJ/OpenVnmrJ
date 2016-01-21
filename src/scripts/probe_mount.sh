#!/bin/sh
mnt=/mnt/probe
if [ $# -gt 0 ]; then
  mnt=$1
fi

if [ ! -e $mnt ]; then
  /bin/mkdir -p $mnt
fi

mounted=`mount | grep -c -e " $mnt "` # surrounding spaces are required
if [ "$mounted" != "0" ]; then
  exit 1
fi

/bin/mount -t cifs //169.254.0.1/vnmr1 $mnt --verbose -o user=vnmr1,password=varian1,noserverino
