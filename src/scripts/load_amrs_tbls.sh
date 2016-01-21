#!/bin/sh
sysdir=/vnmr
vfs_tbl_dir=vfstables

if [[ $# > 0 ]]; then
  ctrl=$1
else
  ctrl="rf1"
fi

dirpath=$sysdir/$vfs_tbl_dir/$ctrl
path=$dirpath/atten.txt

mkdir -p $dirpath

echo "load ffs:rf_amrs.4th" | nc $1 5555 > $path
lines=`wc -l $path | awk '{ print $1 }'`
echo "loaded $lines entry table from $ctrl to $path"
