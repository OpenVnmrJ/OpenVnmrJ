#! /bin/csh
# '@(#)lxcd.sh 22.1 03/24/08 2003-2004 '
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
if  (x$1 == "x") then
  echo "must supply the directory of the VnmrJ_LX CD"
  exit 1
endif
if ! -d $1 then
  echo "directory $1 does not exist"
  exit 1
endif
rm -f $1/code/vnmrsetup
(cd $1/code; Sget scripts vnmrsetupLX.sh; make vnmrsetupLX; rm -f vnmrsetupLX.sh; mv vnmrsetupLX vnmrsetup)

