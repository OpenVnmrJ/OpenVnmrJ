#!/bin/sh
#sudo mount -t cifs //169.254.0.1/public /mnt/cifs --verbose -o user=guest
mnt=/mnt/vnmr1
if [ $# > 1 ]; then
  mnt=$1
fi

umount $mnt
