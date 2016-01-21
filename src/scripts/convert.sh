#! /bin/ksh
#
#set -x
# Note: for this to work on Windows /vnmr/bin must be prepended to the PATH set in
#       SFU/etc/profile/lcl. and the full path the the ImageMagic install directory
#       should also be added (e.g. /dev/fs/C/"Program Files"/ImageMagick-6.6-7.Q8)
pgrm=`which convert.exe`
if [ -x "$pgrm" ] 
then
#	print -R -n "convert " $*
   "$pgrm" $* > /dev/null
else
    print "ImageMagick is not installed"
fi
