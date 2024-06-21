: '@(#)fixpsg.sh 22.1 03/24/08 1991-2006 '
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
#    fixpsg   -   UNIX shellscript to allow vnmr1 to fix    #
#		  PSG with updated PSG modules              #
#		  it executes the "makeuserpsg"             #
#		  makefile stored in /vnmr/psg.		    #
#                                                           #
#    04/1996      If the /vnmr/lib directory exists, copy   #
#		  the newly made libray into it.  Both      #
#		  shared and static libraries are copied    #
#		  as well as a link for the shared library  #
#############################################################

#  Certain programs have to be present ...

if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi

osname=`vnmr_uname`

# main program starts here


makename="makeuserpsg.lnx"

cd $vnmrsystem/psg
if (test ! -f $makename)
then
   echo " "
   echo " "
   echo "$makename does not exist in system PSG directories."
   echo "This is an irrecoverable error."
   echo " "
   exit 1
fi

if test $osname = "Linux"
then
   Wextra=""
   gcc -Q --help=warning | grep Wformat-overflow >& /dev/null
   if [[ $? -eq 0 ]]
   then
      Wextra=${Wextra}" -Wno-format-overflow"
   fi
   arch=""
   file $vnmrsystem/lib/libpsglib.so | grep "32-bit" $file >& /dev/null
   if [[ $? -eq 0 ]]; then
      arch="-m32"
   fi
   rm -f *.o
   make -e -s -f makeuserpsg.lnx CFLAGS="-O -fPIC ${arch} ${Wextra}" LDFLAGS="-shared ${arch}" fixlib
else
   if test $osname = "Darwin"
   then
      rm -f *.o
      make -s -e -f makeuserpsg.lnx macfixlib
   fi
fi

chmod 755 libpsglib.a

if (test -d $vnmrsystem/lib)
then
   librarylist=libpsglib.*
   for file in $librarylist
   do
      rm -f $vnmrsystem/lib/$file
      mv $file $vnmrsystem/lib
   done
fi

exit 0
