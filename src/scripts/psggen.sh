: '@(#)psggen.sh 22.1 03/24/08 1991-2006 '
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
#    02/1996	  Extended to define PATH so the search for #
#		  executables starts with GNU and then goes #
#		  to /usr/ccs/bin on Solaris.  Corrects an  #
#		  obscure bug which results when the shell  #
#		  finds the (non-functioning) /usr/ucb/cc   #
#		  compiler.				    #
#							    #
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
   if test $osname = "Interix"
   then
     makename="makeuserpsg"
   else
     makename="makeuserpsg.lnx"
   fi
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
if test $osname = "Linux"
then
   rm -f *.o
   make -e -s -f makeuserpsg.lnx lib
else
   if test $osname = "Darwin"
   then
      rm -f *.o
      make -e -s -f makeuserpsg.lnx maclib
   else
      if test $osname = "Interix"
      then
         rm -f *.o
         make -e -s -f makeuserpsg libwin
      else
         PATH="$GNUDIR"/bin:/usr/ccs/bin:"$PATH"
         export PATH
         make -e -s -f makeuserpsg libsol
         chmod 755 libpsglib.a
      fi
   fi
fi
find . ! -name "*.so" -type l -exec rm {} \;

exit 0
