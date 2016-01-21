: '@(#)vnmr_cdump.sh 22.1 03/24/08 1999-2002 '
# 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#  vnmr_cdump
#
#  Usage  vnmr_cdump filename plottername
#

file=$1
if [ $# -eq 1 ]
then
  if [ x$vnmrjadmlpfmt = "x" ]
  then
    type=ps
  else
    type=$vnmrjadmlpfmt
  fi
else
  type=$3
fi
if [ x$type = "xpcl" ] 
then
  file=$1.tmp
  gs -sDEVICE=laserjet -sOutputFile=$file -q - $1
  rm -f $1
fi

if [ $# -eq 1 ]
then
  if [ x$vnmrjadmlp = "x" ]
  then
    lp -s -c -o nobanner $file
  else
    lp -s -c -o nobanner -d $vnmrjadmlp $file
  fi
else
  if [ $# -eq 7 ]
  then
# DICOM
    createdicom -monochrome1 -infile $file -tag $5 -outfile $6 -dpi $7 -printer $2
    dicomlpr $6 -printer $2
    rm $6
  else
    lp -s -c -o nobanner -d $2 $file
  fi
fi
rm -f $file
