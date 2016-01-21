#! /bin/sh
#
# rework tail to cover old style 

#set -x

#printf " %s" "$*" 

os=`vnmr_uname`

#echo $*  | grep -s " -n " > /dev/null
printf " %s" "$*"  | grep -s " -n " > /dev/null
if [ $? -eq 0 ]
then
   #echo "Found ' -n '"
   if [ x$os = "xSOLARIS" ]
   then
      /usr/xpg4/bin/tail $*
   else
      /usr/bin/tail $*
   fi
else
   #echo "Did not find ' -n '"
   if [ x$os = "xSOLARIS" ]
   then
      /usr/xpg4/bin/tail -n $*
   else
      /usr/bin/tail -n $*
   fi

fi
   

