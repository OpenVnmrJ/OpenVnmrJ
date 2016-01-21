#!/bin/sh
# '@(#)jvnmruser.sh 22.1 03/24/08 1999-2004 '
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
# This script displays system current VNMR user(s)
# that do not exist in VnmrJ userlist file

ostype=`"$vnmrsystem"/bin/vnmr_uname`
if [ x$ostype = "xDarwin" ]
then
   users=`ls /Users`
   for user in $users
   do
      if [ -d "/Users/$user/vnmrsys" ]
      then
         grep -w $user "$vnmrsystem/adm/users/userlist" 1>&2 > /dev/null
         if [ $? -ne 0 ]
         then
            echo "$user"
         fi
      fi
   done
elif [ x$ostype = "xInterix" ]
then
    netpath=`/usr/bin/winpath2unix "${SYSTEMROOT}/system32/net.exe"`
    lines=`$netpath user | /usr/bin/wc -l`
    "$netpath" user | /usr/bin/awk 'BEGIN { FS = " "; linecount = "'"$lines"'"} 
    {
	if ( NR != 2 && NR != linecount-1)
	{
	    for(i = 1; i < NF; i++)
	    {
		tt=system("/usr/bin/grep -w " $i " " "/vnmr/adm/users/userlist 1>&2 > /dev/null")
			
                if( tt ) {
		    print $i
		}
	    } 
       	}
    }' 
else    
   if [ x$ostype = "xLinux" ]
   then
      PROG=awk
   else
      PROG=nawk
   fi
   $PROG '
   BEGIN { FS = ":" } 
  
   {
      if ( system("test -d "  $6"/vnmrsys") == 0 ) {

         tt=system("grep -w " $1 " " "'$vnmrsystem'" "/adm/users/userlist 1>&2 > /dev/null")

         if( tt ) {
            print $1
         }
      }
   }
   ' /etc/passwd
fi
