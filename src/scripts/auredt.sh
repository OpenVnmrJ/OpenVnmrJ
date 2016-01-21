: '@(#)auredt.sh 22.1 03/24/08 1999-2002 '
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
#!/usr/bin/sh

#auredt.sh
#This script will do "all" the root editing jobs 
#it's executed via sudoers list by nmr admin

#Usage: 1) auredt filepath value
#       2) auredt key value

   key=$1
   val=$2
   file=
   tempfile=/tmp/auredt.tmp


   case $key in 
       "RETRIES=" )
	      file="/etc/default/login"
              ;;

       "MAXWEEKS=" | "MINWEEKS=" | "PASSLENGTH=" )
	      file="/etc/default/passwd"
              ;;

       *)  #Restore/delete user
             
              if [ x$3 = "xRestore" ]
              then
                   sed '/'$val'/d' $key > $tempfile
                   mv $tempfile $key
              else  #add user to deleted list

                 if [ ! -f $key ]
                 then
                    echo $val > $key
                 else
                    echo $val >> $key
                 fi
              fi 
              exit
              ;;
   esac


   cat $file |
   nawk ' /'$key'/{ $0="'$key''$val'" } { print } ' > $tempfile
   mv $tempfile $file

