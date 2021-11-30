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
#   libpsglib.so    -    user modifiable PSG library      #
#                                                         #
#   libparam.so     -    supplied PSG library             #
#                                                         #
###########################################################
# Main MAIN main


# This shellscript is called by PSGGEN.
# Check for user PSG directory.  If one does not exist, create
# and initialize it.

echo " "
echo " "
if (test ! -d "$vnmruser"/psg)
then
   mkdir "$vnmruser"/psg
   echo "Creating user PSG directory..."

   cd "$vnmruser"/psg

   if test -f "$vnmrsystem"/lib/libpsglib.so
   then
         cp -p "$vnmrsystem"/lib/libpsglib.so libpsglib.so
         echo "Copying User PSG library from system directory..."
   fi
else
   cd "$vnmruser"/psg
fi

if test -f libpsglib.so
then
   chmod 755 libpsglib.so
fi

exit 0
