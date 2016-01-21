#!/bin/sh
# 'vjpostinstallaction.sh 2009 '
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

# lsb_release not present on RHEL 5.X,  so just comment them out, not used anyway.
#distro=`lsb_release -is`    # RedHatEnterpriseWS; Ubuntu
#distrover=`lsb_release -rs` # 4, 5; 8.04, 9.04, etc.
# distrover=`cat /etc/redhat-release | sed -r 's/[^0-9]+//' | sed -r 's/[^0-9.]+$//'`    # yield 5.1, 5.3 , 6.1, etc..
# VersionNoDot=`cat /etc/redhat-release | sed -e 's#[^0-9]##g' -e 's#7[0-2]#73#'`    # yields 51, 53, 61, etc.
# MajorVersionNum=`cat /etc/redhat-release | sed -e 's#[^0-9]##g' | cut -c1`            # yields  5, 5,  6,  etc.


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
   cmd="$dest_dir/bin/vnmrj admin -debug needupdateuser"
   if [ "x$login_user" != "x$nmr_adm" ];
   then
       # echo "Switching to administrator $nmr_adm and running $cmd"
       if [ x$lflvr != "xdebian" ]
       then
          # echo "touch $nmr_home/$nmr_adm/.vnmrsilent"
          # echo "su - $nmr_adm -c $cmd"

          # check for admin home being NFS mount, if it is, we probably will
          # not be able to create the .vnmrsilent files as root
          # so we will abort and tell user, to manual run VnmrJ Admin
          admstr=`mount | grep nfs | grep "^$nmr_home"`
          if [ ! -z "$admstr" ]
          then
             echo " "
             echo "Note: "
             echo "  Appears the $nmr_adm home directory \"$nmr_home\" is NFS mounted."
             echo "  VnmrJ Admin will have to be run manually"
             echo "  to update the users."
             echo " "
          else
             # create the .vnmrsilent so the .login doesn't ask for the display
             # get home dir of admin
             admhomedir=`eval echo ~$nmr_adm`
             touch $admhomedir/.vnmrsilent
             chmod 777 $admhomedir/.vnmrsilent
             #touch $nmr_home/$nmr_adm/.vnmrsilent
             #chmod 777 $nmr_home/$nmr_adm/.vnmrsilent
             xhost + > /dev/null
             su - $nmr_adm -c "$cmd" > /dev/null 2>&1
             sleep 5
          fi
       else
          # check for admin home being NFS mount, if it is, we probably will
          # not be able to create the .vnmrsilent files as root
          # so we will abort and tell user, to manual run VnmrJ Admin
          admstr=`mount | grep nfs | grep $nmr_home`
          if [ ! -z "$admstr" ]
          then
             echo " "
             echo "Note: "
             echo "  Appears the $nmr_adm home directory \"$nmr_home\" is NFS mounted."
             echo "  VnmrJ Admin will have to be run manually"
             echo "  to update the users."
             echo " "
          else
             # create the .vnmrsilent so the .login doesn't ask for the display
             # echo "sudo -u $nmr_adm $cmd"
             # create the .vnmrsilent so the .login doesn't ask for the display
             # echo "touch $nmr_home/$nmr_adm/.vnmrsilent"
             # get home dir of admin
             admhomedir=`eval echo ~$nmr_adm`
             touch $admhomedir/.vnmrsilent
             chmod 777 $admhomedir/.vnmrsilent
             #touch $nmr_home/$nmr_adm/.vnmrsilent
             ## echo "chmod 777 $nmr_home/$nmr_adm/.vnmrsilent"
             #chmod 777 $nmr_home/$nmr_adm/.vnmrsilent
             ## echo "sudo -i -u $nmr_adm $cmd"
             sudo -i -u $nmr_adm $cmd
          fi
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
exit 0
