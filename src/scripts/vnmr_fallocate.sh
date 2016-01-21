#! /bin/sh
#
# Use system fallocate to pre-allocate a file of the desired size (bytes)
# vnmr_fallocate filename size
# return 0 - success
#        1 - failure
#
# fallocate [-n] [-o offset] -l length filename
# lenght in bytes.  Suffixes of k, m, g, t, p, e may be specified to denote KiB, MiB, GiB, etc.
# e.g. fallocate -l size filename
#
# required util-linux-ng package to be installed
#

#
# check for two arguments
#
if [ $# -ne 2 ]; then
   echo "0"
   exit 0
fi
cmd="/usr/bin/fallocate"
#
# confirm command is present
#
if [ -x "/usr/bin/fallocate" ] ; then
   # echo "$cmd -l $2 $1"
   # succesful fallocate returns 0 otherwise 1
   $cmd -l $2 $1
   echo $?
else
  echo "0"
fi
exit 0

#=====================================================================
#: 'tst_vnmr_fallocate'
#set -x
# if [ -h /usr/lib64/libkonq.so.5  -a ! -h /usr/lib64/libkonq.so.4 ] 
# then 
#            ( cd /usr/lib64 ; ln -s libkonq.so.5 libkonq.so.4 >/dev/null 2>&1 )
# fi   
#
#./vnmr_fallocate.sh
#echo $?
#
#./vnmr_fallocate.sh tmpfile
#echo $?
#
#./vnmr_fallocate.sh ./tmpfile 512k
#echo $?
#=====================================================================

#
