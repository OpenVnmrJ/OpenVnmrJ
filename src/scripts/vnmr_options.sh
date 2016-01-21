#! /bin/sh
#  '@(#)ins_vnmr.sh  1991-2008 '
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
# $1 is password
# $2 is filename

#-----------------------------------------------
if (test x$vnmrsystem = "x")
then
   source /vnmr/user_templates/.vnmrenvsh
fi

taroption="jxf"

cd $vnmrsystem/adm/options
$vnmrsystem/bin/decode $1 $2 tmp.tar
# Fails on Win-7 because the line has a field in which the name has a space within it e.g. 'NT Services'
# thus throwing off the count, result in the "5th" field to be NOT  the size
# i=`ls -l tmp.tar | awk 'BEGIN { FS=" " } {print $5}'`  

# ls -s gives the size in blocks, e.g. "2 tmp.tar" 
i=`ls -s tmp.tar | awk 'BEGIN { FS=" " } {print $1}'`

# if the wrong password was used the file size will be zero
if [ $i -gt 0 ]
then
  cd $vnmrsystem
  (umask 0; tar $taroption adm/options/tmp.tar)
  rm -f adm/options/tmp.tar
  echo "0"
else
  echo "-1"
fi
