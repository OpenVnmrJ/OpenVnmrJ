#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
 
#-----------------------------------------------
#  Main MAIN main program starts here
#-----------------------------------------------
os_version=$1    #sol ibm sgi rht
shift            #because sh does not use $10 and up
cons_type=$1     #inova ... g2000
shift
src_code_dir=$1  #/cdom/cdrom0/code
shift
dest_dir=$1      #/export/home/vnmr
shift
did_vnmr=$1      #was vnmr installed  y or n
shift
nmr_adm=$1       #vnmr1
shift
nmr_group=$1     #nmr
shift
nmr_home=$1      #/space or /export/home or /
shift
vnmr_link=$1     #yes
shift
man_link=$1      #no
shift
gen_list=`echo $1 | sed 's/+/ /g'`  #agr8 or 9 is a list, items separated by a "*"
shift
opt_list=`echo $1 | sed 's/+/ /g'`

login_user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`

if [ "x$vnmrNoPostAction" = "xTRUE" ]
then
# Java ProgressMeter is expecting this text string in order to complete.
   echo "Post Action Skipped. "
   exit 0
fi
if [ "x$vnmrsystem" = "x" ]
then
   vnmrsystem="/vnmr"
fi
osname=`uname -s`
rootuser="root"

if [  -r /etc/debian_version ]
then
   lflvr="debian"
else
   lflvr="rhat"
fi


echo "Starting Post Installation Actions"

# echo "NMR Owner = $nmr_adm"
# echo "NMR Owner HomeDir = $nmr_home"
# echo "NMR Group = $nmr_group"
# echo "NMR Destination directory= $dest_dir"

# echo "did_vnmr: $did_vnmr"
# set -x

if [ x$did_vnmr = "xy" ]; 
then
   echo "Running VnmrJ Admin, Please Update User(s)"
   cmd="$dest_dir/bin/vnmrj admin -Ddebug=needupdateuser"
   if [ "x$login_user" != "x$nmr_adm" ];
   then
       # echo "Switching to administrator $nmr_adm and running $cmd"
       if [ x$lflvr != "xdebian" ]
       then
          xhost + > /dev/null
          su -l $nmr_adm -s /bin/csh -c "setenv DISPLAY $DISPLAY; $cmd" > /dev/null 2>&1
       else
          ## echo "sudo -i -u $nmr_adm $cmd"
          sudo -i -u $nmr_adm $cmd
       fi
   else
      $cmd
   fi
fi
#
# There is a check in ProgressMonitor.java for the below message
# Java ProgressMeter is expecting this text string in order to complete.
echo " "
echo "Post Action Completed. "
if [ -f $dest_dir/tmp/.ovj_installed ]; then
   sleep 5
fi
exit 0
