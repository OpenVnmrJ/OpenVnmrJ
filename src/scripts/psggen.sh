#!/bin/bash
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
#############################################################
#                                                           #
#    psggen   -   UNIX shellscript to allow user to create  #
#		  new user PSG library in the user's PSG    #
#		  directory; it executes the "makeuserpsg"  #
#		  makefile stored in /vnmr/psg.		    #
#                                                           #
#############################################################


# Check for SETUSERPSG shellscript; if present, execute it.

if (test ! -r "$vnmrsystem"/bin/setuserpsg)
then
   echo " "
   echo " "
   echo "SETUSERPSG does not exist in the system BIN directory."
   echo "This is an irrecoverable error."
   echo " "
   exit 1
else
   "$vnmrsystem"/bin/setuserpsg

   if [ $? = 1 ]
   then
      exit 1
   fi
fi


# Check for user PSG directory and for MAKEUSERSPSG makefile

osname=`vnmr_uname`
if (test ! -d "$vnmruser"/psg)
then
   echo " "
   echo " "
   echo "User PSG directory does not exist."
   echo "SETUSERPSG must be run before executing PSGGEN."
   echo " "
   exit 1
else
   makename="makeuserpsg.lnx"
   cd "$vnmruser"/psg
   if (test ! -f $makename)
   then
      cd "$vnmrsystem"/psg
      if (test ! -f $makename)
      then
         echo " "
         echo " "
         echo "$makename does not exist in user or system PSG directories."
         echo "This is an irrecoverable error."
         echo " "
         exit 1
      else
         cp -p $makename "$vnmruser"/psg/$makename
      fi
   fi
fi

cd "$vnmruser"/psg
arch=""
if test $osname = "Linux"
then
   rm -f *.o
# Silence warnings from newer gcc compilers
   Wextra=""
   gcc -Q --help=warning | grep Wunused-but-set-variable >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra="-Wno-unused-but-set-variable"
   fi
   gcc -Q --help=warning | grep Wunused-result >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-unused-result"
   fi
   gcc -Q --help=warning | grep Wmisleading-indentation >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-misleading-indentation"
   fi
   gcc -Q --help=warning | grep Wstringop-truncation >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-stringop-truncation"
   fi
   gcc -Q --help=warning | grep Wformat-overflow >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-format-overflow"
   fi
   file -L $vnmrsystem/lib/libpsglib.so | grep "32-bit" $file >& /dev/null
   if [[ $? -eq 0 ]]; then
      arch="-m32"
   fi
   make -e -s -f makeuserpsg.lnx CFLAGS="-O -fPIC ${arch} ${Wextra}" lib
else
   if test $osname = "Darwin"
   then
      rm -f *.o
      make -e -s -f makeuserpsg.lnx maclib
   fi
fi
find . ! -name "*.so" -type l -exec rm {} \;

exit 0
