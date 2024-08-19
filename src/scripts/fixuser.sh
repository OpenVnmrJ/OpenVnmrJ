#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
# set -x

curr_user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
if [ "x$curr_user" = "xroot" ]
then
  echo "$0 must be run as a user, not root"
  exit 1
fi

cd $HOME/vnmrsys
#  remove contents of seqlib
#  use word count program (wc) so script variable will have a value
#  that `test' sees as a single argument

if [[ -d seqlib ]]
then
    tmpval=$((cd seqlib; ls) | wc -c)
    if [[ $tmpval != "0" ]]
    then
       echo -n  "Remove compiled pulse sequences (y or n) [y]: "
       read yesno
       if [[ "x$yesno" != "xn" ]] && [[ "x$yesno" != "xno" ]]
       then
        
	   echo "  removing old pulse sequences"
   	   rm -f seqlib/*
       fi
    fi
fi

#  remove make file and binary files in psg (*.ln, *.a, *.so.* *.o)
#  One level of evaluation ($filespec => *.a) - Use double quotes
#  Expand implicit wildcard ($filespec => libpsglib.a) - no quote characters
#  redirect error output to /dev/null, to avoid messages if no such files exist

if [[ -d psg ]] && [[ -f psg/libpsglib.so.6.0 ]]
then
   echo -n  "Remove compiled PSG elements (y or n) [y]: "
   read yesno
   if [[ "x$yesno" != "xn" ]] && [[ "x$yesno" != "xno" ]]
   then
      for filespec in "*.a" "*.so*" "*.ln" "*.o" "makeuserpsg*"
      do
         tmpval=`(cd psg; ls $filespec 2>/dev/null) | wc -c`
         if [[ $tmpval != "0" ]]
         then
	    echo "  removing '$filespec' from psg subdirectory"
	    (cd psg; rm -f $filespec)
         fi
      done
   fi
fi

if [[ -d persistence ]]
then
   echo -n  "Reset persistent Java information in persistence directory (y or n) [y]: "
   read yesno
   if [[ "x$yesno" = "xn" ]] || [[ "x$yesno" = "xno" ]]
   then
      exit 1
   fi
fi

fileList="LocatorHistory_default LocatorHistory_edit_panel LocatorHistory_protocols \
           LocatorHistory_trash TagList session"

if [[ $# -eq 0 ]]; then
   for file in $fileList
   do
      if [[ -f persistence/$file ]]; then
         mv -f persistence/$file persistence/bk_$file
      fi
   done
   echo "persistence directory cleaned."
else
   for file in $fileList
   do
      if [[ -f persistence/bk_$file ]]; then
         mv -f persistence/bk_$file persistence/$file
      fi
   done
   echo "persistence directory restored"
fi
