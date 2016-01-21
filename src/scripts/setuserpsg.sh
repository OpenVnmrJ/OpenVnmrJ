: '@(#)setuserpsg.sh 22.1 03/24/08 1991-1996 '
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
###########################################################
#                                                         #
#   setuserpsg   -   UNIX shellscript to either setup or  #
#                    reconfigure the user PSG directory   #
#                                                         #
###########################################################
#                                                         #
#   libpsglib.a    -    user modifiable PSG library       #
#                                                         #
#   libparam.a     -    Varian supplied PSG library       #
#                                                         #
###########################################################
# Main MAIN main


# This shellscript is called by PSGGEN.
# Check for user PSG directory.  If one does not exist, create
# and initialize it.

if test `vnmr_uname` = "IRIX"
then
    echo "setuserpsg aborted for IRIX."
    exit 1
fi

echo " "
echo " "
if (test ! -f "$vnmrsystem"/lib/libparam.a)
then
   echo " "
   echo " "
   echo "No Varian PSG library was found in system directory."
   echo "Specifically, $vnmrsystem/lib/libparam.a does not exist."
   echo "This is an irrecoverable error."
   echo " "
   exit 1
fi
if (test ! -d "$vnmruser"/psg)
then
   mkdir "$vnmruser"/psg
   echo "Creating user PSG directory..."

   cd "$vnmruser"/psg

   if test -f "$vnmrsystem"/lib/libpsglib.a
   then
         cp -p "$vnmrsystem"/lib/libpsglib.a libpsglib.a
         echo "Copying User PSG library from system directory..."
   fi
else
   cd "$vnmruser"/psg
fi

if test -f libpsglib.a
then
   chmod 755 libpsglib.a
fi

exit 0
