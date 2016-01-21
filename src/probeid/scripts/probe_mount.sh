#!/bin/sh
#sudo mount -t cifs //169.254.0.1/public /mnt/cifs --verbose -o user=guest
mnt=/mnt/probe
if [ $# > 1 ]; then
  mnt=$1
fi

if [ ! -e $mnt ]; then
  mkdir -p $mnt
fi

mount -t cifs //169.254.0.1/vnmr1 $mnt --verbose -o user=vnmr1,password=varian1
