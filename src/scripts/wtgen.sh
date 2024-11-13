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
#************************************************************************
#** wtgen:  shell script for compiling and link loading user-written   **
#**         weighting function routines.   (Bourne shell)              **
#**                                                                    **
#**         The source code for the weighting function ("wf") routine  **
#**         is located in $VNMRUSER/WTLIB.  The file name for this     **
#**         source code is supplied as the only argument to the shell  **
#**         script, e.g., "wtgen test".  Note that the ".c" file       **
#**         extension is NOT necessary when specifying the "wf file"   **
#**         name.  If the extension is supplied, it is removed so      **
#**         "test" and "test.c" are both acceptable as input argu-     **
#**         ments to "wtgen".  The output of "wtgen" is an executable  **
#**         program which is stored in $VNMRUSER/WTLIB.                **
#************************************************************************

echo " "
echo "Beginning Weighting Routine Generation Process:"
echo " "


# Check to establish USER and LOGNAME equivalence
if (test ! $USER = $LOGNAME)
then
   echo "Username: "$USER"  and Logname: "$LOGNAME"  do not agree."
   echo " "
   exit
fi

if test x"$vnmrsystem" = "x"
then
   source /vnmr/bin/profile
fi

echo "Checking for necessary files and directories..."

# Check for SYSTEM bin directory
if (test ! -d "$vnmrsystem"/bin)
then
   echo "System bin directory does not exist."
   echo " "
   exit
fi


# Check for appropriate files in SYSTEM bin directory
if (test ! -r "$vnmrsystem"/bin/usrwt.o)
then
   echo "Object file for user-written weighting routines cannot be accessed."
   echo " "
   exit
fi

if (test ! -r "$vnmrsystem"/bin/weight.h)
then
   echo "Header file for user-written weighting routines cannot be accessed."
   echo " "
   exit
fi


# Check for USER weighting function directory (WTLIB)
if (test ! -d "$vnmruser"/wtlib)
then
   mkdir "$vnmruser"/wtlib
   chmod 755 "$vnmruser"/wtlib
   echo "User weighting routine directory has been created."
   echo " "
   exit
fi


# Removes the *.c file extension (if present) from the name of the
# weighting function routine
file=`basename "$1" .c`

if (test ! -r "$vnmruser"/wtlib/$file.c)
then
   echo "Weighting function routine cannot be accessed."
   echo " "
   exit
fi


# Create soft links to necessary files in SYSTEM bin directory
echo "Creating soft links to appropriate system files..."
cd "$vnmruser"/wtlib
if test -f usrwt.o
then
   rm usrwt.o
fi

if test -f weight.h
then
  rm weight.h
fi

ln -s "$vnmrsystem"/bin/usrwt.o usrwt.o
ln -s "$vnmrsystem"/bin/weight.h weight.h


# Compile user weighting function routine
echo "Starting compilation and link loading..."

cc -O $file.c usrwt.o -lm -o $file > errmsg

if test -f $file
then
   rm errmsg
else
   echo " "
   echo "Weighting function routine did not compile."
   echo "Error messages are contained in the file 'errmsg'."
   rm usrwt.o weight.h
   echo " "
   exit
fi

rm usrwt.o weight.h
echo " "
echo "Process Complete."
echo " "
echo " "
