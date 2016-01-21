#! /bin/csh
# '@(#)killft3d.sh 22.1 03/24/08 1991-2002 '
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
#
# killft3d C-shell shellscript
#

if (${#argv} < 1) then
   echo ""
   echo "killft3d:  usage error  -  must specify the experiment number"
   echo ""
   exit -1
endif

set ostype=`vnmr_uname`

set npid = ""

if (x$ostype == xSunOS) then
  set npid = `ps ax | awk '$5 ~ /ft3d/ { for (i = 5; i <= NF;) { if ($i == "-e") { i = i + 1; if ($i == '$1') printf("%d", $1); } i = i + 1; } }' $2`
else
  set npid = `ps -ef | awk '$8 ~ /ft3d/ { for (i = 8; i <= NF;) { if ($i == "-e"){ i = i + 1; if ($i == '$1') printf("%d", $2); } i = i + 1; } }' $2`
endif

if ($npid != "") then
   foreach pid3d ($npid)
      set filename = ""
      if (x$ostype == xSunOS) then
        set filename = `ps ax | awk '$5 ~ /ft3d/ { if ($1 == '$pid3d') { for (i = 5; i <= NF;) { if ($i == "-o") { i = i + 1; printf("%s", $i); } i = i + 1; } } }' $3`
      else
        set filename = `ps -ef | awk '$8 ~ /ft3d/ { if ($2 == '$pid3d') { for (i = 5; i <= NF;) { if ($i == "-o") { i = i + 1; printf("%s", $i); } i = i + 1; } }}' $3`
      endif

      kill -9 $pid3d
      if ("$filename" == "") then
         set filename = "${vnmruser}/exp$1/datadir3d/data"
      endif

      rm -rf "$filename"
   end
endif
