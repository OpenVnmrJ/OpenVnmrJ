: '@(#)setmapping.sh 22.1 03/24/08 1991-1996 '
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
: /csh
if test ! -d $vnmruser/psglib
then
   mkdir $vnmruser/psglib
   chmod 755 $vnmruser/psglib
fi

if test -d $vnmruser/psglib
then
   if test -f $vnmruser/psglib/rri.c
   then
      mv $vnmruser/psglib/rri.c $vnmruser/psglib/rri.c.bk
   fi
   cp rri.c $vnmruser/psglib/.
   chmod 755 $vnmruser/psglib/rri.c
   seqgen rri.c
else
   echo "User PSGLIB does not exist and cannot be created."
   echo ""
   exit
fi

if test ! -d $vnmruser/parlib
then
   mkdir $vnmruser/parlib
   chmod 755 $vnmruser/parlib
fi

if test -d $vnmruser/parlib
then
   if test -d $vnmruser/parlib/rri.par
   then
      mv $vnmruser/parlib/rri.par $vnmruser/parlib/rri.par.bk
   fi
   cp -r rri.par $vnmruser/parlib/.
   chmod -R 755 $vnmruser/parlib/rri.par
else
   echo "User PARLIB does not exist and cannot be created."
   echo ""    
   exit  
fi

if test ! -d $vnmruser/maclib
then
   mkdir $vnmruser/maclib
   chmod 755 $vnmruser/maclib
fi

if test -d $vnmruser/maclib
then
   if test -f $vnmruser/maclib/processmap
   then
      mv $vnmruser/maclib/processmap $vnmruser/maclib/processmap.bk
   fi
   if test -f $vnmruser/maclib/runmap
   then
      mv $vnmruser/maclib/runmap $vnmruser/maclib/runmap.bk
   fi

   if test -f $vnmrsystem/maclib/getplane
   then
      cp processmap $vnmruser/maclib/.
      cp runmap $vnmruser/maclib/.
   else
      cp processmap32 $vnmruser/maclib/processmap
      cp runmap32 $vnmruser/maclib/runmap
   fi

   chmod 755 $vnmruser/maclib/processmap
   chmod 755 $vnmruser/maclib/runmap
else
   echo "User MACLIB does not exist and cannot be created."
   echo ""     
   exit
fi
