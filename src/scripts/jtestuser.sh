: '@(#)jtestuser.sh 22.1 03/24/08 1999-2002 '
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
#!/bin/sh

if [ $# -ne 1 ]
then
   echo "-1"
   exit
fi

if [ x`uname -s` = "xDarwin" ]
then
   user=`finger $1 2> /dev/null` 
   user=`echo $user | awk '{print $1}'`
   if [ x$user =  "x" ]
   then
      echo "-1"
   else
      echo "0"
   fi
else
   my_file=/tmp/testfile_please_remove
   touch $my_file

   chown $1 $my_file 2>/dev/null
   if [ $? -eq 0 ]
   then
      /bin/rm -f $my_file
      echo "0"
   else
      /bin/rm -f $my_file
      echo "-1"
   fi
fi
