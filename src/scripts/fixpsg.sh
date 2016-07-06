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
#    02/1996	  Extended to define PATH so the search for #
#		  executables starts with GNU and then goes #
#		  to /usr/ccs/bin on Solaris.  Corrects an  #
#		  obscure bug which results when the shell  #
#		  finds the (non-functioning) /usr/ucb/cc   #
#		  compiler.				    #
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
verify_ccs()
{
   if (test ! -x /usr/ccs/bin/$1)
   then
      echo "'$1' program missing from /usr/ccs/bin"
      echo "This means most likely that you have not loaded the"
      echo "Solaris Developers option, which is required for fixpsg."
      echo "This is an irrecoverable error."
      echo " "
      exit 1
   fi
}

# main program starts here


if test $osname = "Linux" -o $osname = "Darwin"
then
   makename="makeuserpsg.lnx"
else
   if test $osname = "Interix"
   then
      makename="makeuserpsg"
   else
      if (test ! -x $vnmrsystem/gnu/bin/cc)
      then
         echo "GNU C compiler not installed"
         echo "You must install this option before using fixpsg"
         echo "This is an irrecoverable error."
         exit 1
      fi
      verify_ccs make
      verify_ccs ar
      verify_ccs ld
      makename="makeuserpsg"
   fi
fi

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
   rm -f *.o
   make -s -e -f makeuserpsg.lnx fixlib
else
   if test $osname = "Darwin"
   then
      rm -f *.o
      make -s -e -f makeuserpsg.lnx macfixlib
   else
      if test $osname = "Interix"
      then
         rm -f *.o
         make -s -e -f makeuserpsg fixlibwin
      else
         PATH=$GNUDIR/bin:/usr/ccs/bin:$PATH
         export PATH
         make -s -e -f makeuserpsg fixlibsol
      fi
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
