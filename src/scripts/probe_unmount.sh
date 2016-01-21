#!/bin/sh
#sudo mount -t cifs //169.254.0.1/public /mnt/cifs --verbose -o user=guest
mnt=/mnt/probe
if [ $# -gt 0 ]; then
  mnt=$1
fi

/bin/umount $mnt
