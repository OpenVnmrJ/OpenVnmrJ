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
#   aka load.nmr 
#
# This is the start-up script for loading Vnmr software
#
# "loadvnmr" could be executed with user and group option
# ex: loadvnmr <chin> <software> .
# vnmr1 and nmr as user and group by default.
#
# set -x

xhost + > /dev/null

set_system_stuff() {
   ostype=$(uname -s)
   os_type="rht"
   default_dir="/home"
   if [ -f /etc/debian_version ]; then
      distroType="debian"
   else
      distroType="rhel"
   fi 
}

test_user() {
 
   my_file=${adm_base_dir}/testfile_please_remove
   touch $my_file
   chown $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       rm -f $my_file
       echo " "
       echo "User \"$1\" does not exist,"
       echo "use Linux admintool to create user and"
       echo "rerun $0"
       echo "aborting this program....."
       echo " "
       exit 1
   else
       nmr_user=$1
   fi
}

test_group() {

   my_file=${adm_base_dir}/testfile_please_remove
   touch $my_file
   chgrp $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       rm -f $my_file
       echo "\nGroup \"$1\" does not exist,"
       echo "use Linux Admintool to create group and rerun $0"
       echo "\nAborting this program........"
       echo " "
       exit 1
   else
       nmr_group=$1
   fi
}

form_dest_dir() {

    case  x$os_type in
        "xrht" ) GREP="/bin/grep" ;;
             * ) GREP="grep" ;;
    esac

    versionLine=$(grep -i VERSION ${base_dir}/vnmrrev)
    major=$(echo $versionLine | awk 'BEGIN { FS = " " } { print $3 }')
    minor=$(echo $versionLine | awk 'BEGIN { FS = " " } { print $5 }')
    versionLine=$(grep VnmrJ_SE ${base_dir}/vnmrrev | wc -l)
    if [ $versionLine -eq 1 ]
    then
       vname=openvnmrjSE_
    else
       vname=openvnmrj_
    fi

    $GREP -i -E 'alpha|beta|devel|test'  ${base_dir}/vnmrrev 2> /dev/null
    if [ $? -eq 0 ]
    then
       # match "01, 2010", or "1, 2010", or "01,    2010"
       #"[0-9]* match zero or more 0-9 numbers
       # " *" match zero or more white spaces
       # \b \b  word bounderies, e.g. match a 4 digit number
       date=$(grep "[0-9]*[0-9], *\b[2-9][0-9][0-9][0-9]\b" ${base_dir}/vnmrrev)
       month=$(echo $date |  awk 'BEGIN { FS = " " } { print $1 }')
       day=$(echo $date |  awk 'BEGIN { FS = " " } { print $2 }')
       year=$(echo $date |  awk 'BEGIN { FS = " " } { print $3 }')

       if [ x$month = "xJanuary" ]; then 
          dmonth="01"
       elif [ x$month = "xFebruary" ]; then 
          dmonth="02"
       elif [ x$month = "xMarch" ]; then 
          dmonth="03"
       elif [ x$month = "xApril" ]; then 
          dmonth="04"
       elif [ x$month = "xMay" ]; then 
          dmonth="05"
       elif [ x$month = "xJune" ]; then 
          dmonth="06"
       elif [ x$month = "xJuly" ]; then 
          dmonth="07"
       elif [ x$month = "xAugust" ]; then 
          dmonth="08"
       elif [ x$month = "xSeptember" ]; then 
          dmonth="09"
       elif [ x$month = "xOctober" ]; then 
          dmonth="10"
       elif [ x$month = "xNovember" ]; then 
          dmonth="11"
       else
          dmonth="12"
       fi
 
       dday=$(basename $day ,)

       dest_dir="${default_dir}/${vname}${major}${minor}_${year}-${dmonth}-${dday}"
    else
       dest_dir="${default_dir}/${vname}${major}_${minor}"
    fi
}

disableSelinux() {

 if [ x$distroType  != "xdebian" ] ; then
    #   SELINUX=disabled   (possible values: enforcing, permissive, disabled)
   selinux=$(grep SELINUX /etc/selinux/config | grep -i disabled)
   # echo "str: $str"
   if [ -z "$selinux" ] ; then
       echo ""
       echo ""
       echo "Disabling SELinux, System Reboot Required."
       echo "You must reboot the system prior to continuing the install."
       echo "After the reboot, login again, open a terminal window,"
       echo "change to the dvdimage directory, and rerun ./load.nmr"
       echo ""
       echo ""
       # replace the two possibilites enforcing or permissive, to be disabled
       cat /etc/selinux/config | sed s/SELINUX=[eE][nN][fF][oO][rR][cC][iI][nN][gG]/SELINUX=disabled/ | sed s/SELINUX=[pP][eE][rR][mM][iI][sS][sS][iI][vV][eE]/SELINUX=disabled/ > /etc/selinux/config.mod
       rm -f /etc/selinux/config
       mv  /etc/selinux/config.mod /etc/selinux/config
       chmod 644 /etc/selinux/config
       # SELinux reboot flag file, to break out of the enter root password 
       # loop in main.
       touch /tmp/reboot
       exit 1
   fi
  fi
}

finalInstructions() {
    cat <<EOF

On all systems:
    1. Update all users.
       You can use vnmrj adm for this
       See Configure -> Users-> Update users...
       Or each user can run /vnmr/bin/makeuser
    2. In the OpenVnmrJ interface from the
       Edit (non-imaging) or Tools (imaging) menu,
       select 'System Settings...' and then click
       'System config'


EOF
}

#-------------------------------------------------
#  MAIN Main main
#-------------------------------------------------

npids=$(pgrep Expproc)
if [[ ! -z $npids ]]; then
   echo "Must stop acquisition communications before installing OpenVnmrJ"
   exit 1
fi
npids=$(pgrep Vnmrbg)
if [[ ! -z $npids ]]; then
   echo "Must exit OpenVnmrJ or VnmrJ before installing new OpenVnmrJ"
   exit 1
fi

# loop though the arguments and check for the install option key word
# skippkgchk, NOTE this must be the first argument
# Otherwise the user name, group and destination args will be incorrect
for arg in "$@"
do
   if [ "x$arg" = "xskippkgchk" ]
   then
      # create a file, since if we are not root, then this script we be 
      # called recursively as root to missing the skippkgchk argument
      touch /tmp/vskippkgchk
      shift
   fi
done


reboot="n"

# get base_dir first so we have it at all times
#
firstchar=$(echo $0 | cut -c1-1)
if [ x$firstchar = "x/" ]  #absolute path
then
   base_dir=$(dirname $0)
else
   if [ x$firstchar = "x." ]  #relative path
   then
       if [ x$(dirname $0) = "x." ]
       then
           base_dir=$(pwd)
       else
           base_dir=$(pwd)/$(dirname $0 | sed 's/.\///')
       fi
   else
      base_dir=$(pwd)/$(dirname $0)
   fi
fi
if [[ ${base_dir#/root} != ${base_dir} ]]
then
   echo "Can not install OpenVnmrJ from /root"
   echo "Move $base_dir to /tmp"
   exit 1
fi

# Handle case where "base_dir" has a space in the path
base_link=/tmp/ovjInstallDir
test_dir=$(echo "$base_dir" | sed 's/ //g')
orig_base_dir=${base_dir}
if [[ $test_dir != "$base_dir" ]]
then
   ln -s "$base_dir" $base_link
   base_dir=$base_link
fi

#
# Login the user as a root user
# Use the "su" command to ask for password and run the installer
#

# need the distro type for root or sudo login
set_system_stuff

notroot=0
userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  if [ x$distroType = "xdebian" ]; then
     echo "Installing OpenVnmrJ for Ubuntu."
  else
     echo "Installing OpenVnmrJ for RHEL / CentOS"
  fi
  echo "Or type cntrl-C to exit."
  echo
  #
  # determine if running from mount CD/DVD media
  # if so, need to remount with exec privileges
  # base_dir would be the mount point if CD/DVD
  #
  mntline=$(df --type=iso9660 2>&1 | grep "$orig_base_dir")
#  echo "mntline: $mntline"
  if [ ! -z "$mntline" ] ; then
     # This is probably a noexec mounted CD/DVD. Interactively asking for root password
     # via su or sudo doesn't work.  So force user to do this prior to running
     # shell script
     if [ -f /etc/debian_version ]; then
       echo "Please rerun this script using: sudo bash $0".
     else
       echo "Please su to root, Then rerun this script: bash $0".
     fi
     rm -f $base_link
     exit 0
  fi

  s=1
  t=3
  while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
     if [ x$distroType = "xdebian" ]; then
        echo "If requested, please enter the admin (sudo) password"
        sudo $base_dir/load.nmr $* ;
     else
        echo "Please enter this system's root user password"
        su root -c "$base_dir/load.nmr $*";
     fi
     s=$?
     t=$((t-1))
     echo " "
     if [ -f /tmp/reboot ] ; then
        rm -f $base_link
        exit 1
     fi
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again"
      echo ""
  fi
  exit
fi

#
# User is now root.
#

#
# for OpenVnmrJ we must check if require RHEL packages have been installed
# prior to installing VnmrJ
#
if [ "x$distroType" = "xrhel" -o "x$distroType" = "xdebian" ]; then
# any argument with load.nmr will skip this test
  if [ ! -e /tmp/vskippkgchk ]; then
    dirpath=$base_dir
    # check for required RPM packages
    echo "Checking for Linux packages required by OpenVnmrJ"
    $dirpath/code/chksystempkgs > /dev/null 2>&1
    if [ $? -ne 0 ]; then
      echo "Package update may take up to 40 minutes to complete..........."
      mkdir -p /tmp/ovj_preinstall
      chmod 777 /tmp/ovj_preinstall
      acqProg=""
      if [[ -f $dirpath/code/rht/vnmrs.opt ]]; then
          acqProg="ddr"
      fi
      if [[ -f $dirpath/code/rht/inova.opt ]]; then
          acqProg="mi"
      fi
      if [[ -f $dirpath/code/rht/b12.opt ]]; then
          acqProg="b12"
      fi
      $dirpath/code/installpkgs "$@" $acqProg
      err=$?
      if [ $err -ne 0 ]; then
         if [[ $err -ne 2 ]]; then
           cat $dirpath/code/rpmInstruction.txt
         fi
         exit 2
      fi
    fi 
  fi
fi

# remove reboot flag for SELinux disabling
rm -f /tmp/reboot

# loop though the arguments and check for the install option key word
# noPing, NOTE this must be the first argument
# Otherwise the user name, group and destination args will be incorrect
for arg in "$@"
do
   if [ "x$arg" = "xnoPing" ]
   then
      shift
   fi
done


# two possible install options plus 3 args
if [ $# -gt 4 ]   
then
    echo "\nUsage:  $0 [install_opts] [user name] [group name] [destination directory]"
    rm -f $base_link
    exit 1
fi

host_name=$(uname -n)
set_system_stuff

#
#  First make sure the firewall and SELinux are OFF.
#

# if SELinux was enabled, then disable, but a system reboot is required.
# Stop installation.
disableSelinux

# On systems with xview installed, the /etc/profile.d/openwin.csh
# file causes csh scripts to fail with the message
# "break: Not in while/foreach."  Remove this file.
rm -f /etc/profile.d/openwin.csh

form_dest_dir
#dest_dir="$default_dir/vnmr"
nmr_user="vnmr1"
user_dir="$default_dir"
nmr_group="nmr"

adm_base_dir="/var/tmp"
 
if [ ! -d $adm_base_dir ]
then
    mkdir -p $adm_base_dir
fi

if [ $# -eq 1 -a x$1 != "xvnmr1" ]
then  
   test_user $1
else 
   if [ $# -eq 2 ]
   then  
      if [ x$1 != "xvnmr1" ]
      then
         test_user $1
      fi

      if [ x$2 != "xnmr" ]
      then
         test_group $2
      fi 
   fi
fi 

if [ $# -eq 3 ]
then
   dest_dir=$3

   if [ x$1 != "xvnmr1" ]
   then
      test_user $1
   fi

   if [ x$2 != "xnmr" ]
   then
      test_group $2
   fi
fi

calldir=$base_dir
fromcdrom="false"
while [ x$calldir != "x/" ]
do
   if [ x$calldir = "x/cdrom" ]
   then
      fromcdrom="true"
   fi
   calldir=$(dirname $calldir)
done

src_code_dir=${base_dir}/code
export ostype

theuserid=$(id | tr '()' '  ' | cut -f2 -d' ')
thewindowid=$(who -m | awk '{ print $1 }')
isremotewindow=$(echo $DISPLAY | tr ':' ' ' | cut -f1 -d' ')
if [ x$isremotewindow = "x" ]
then
  if [ x$theuserid = "xroot" -a x$theuserid != x$thewindowid ]
  then
    echo ""
    xhost + $host_name > /dev/null
  fi
fi

echo "Starting the OpenVnmrJ installation program..."

# Save pipe directory in case we need to copy it
if [[ -d /vnmr ]] ; then
   oldVnmr=$(readlink /vnmr)
   oldJava=$(echo $PATH | grep jre)
   if [[ ! -z $oldJava ]]; then
      export PATH=$(echo $PATH | sed 's|/vnmr/jre/bin:||')
   fi
else
   oldVnmr=""
fi

cd $adm_base_dir
cp $src_code_dir/VnmrAdmin.jar .
insLog=/tmp/vnmrjInstallLog
newgrp=/tmp/newgrp
rm -f $insLog
rm -f $newgrp

java -classpath $adm_base_dir/VnmrAdmin.jar LoadNmr $base_dir $dest_dir $nmr_user $user_dir $nmr_group $os_type $1
if [ $? -ne 0 ]
then
   rm -f $insLog
   rm -f /tmp/.ovj_installed
   rm -f $newgrp
else
# remove preinstall temp directory
   if [ -d /tmp/ovj_preinstall ] ; then
    # 1st copy the log file into /vnmr/adm/log
      logFile=$(ls /tmp/ovj_preinstall/pkgInstall.log* >& /dev/null)
      if [ $? -eq 0 ]
      then
         cp  /tmp/ovj_preinstall/pkgInstall.log* /vnmr/adm/log
      fi
      rm -rf /tmp/ovj_preinstall
   fi
   if [ -f $insLog ]
   then
      chmod 666 $insLog
   fi
fi

cd $adm_base_dir
rm ./VnmrAdmin.jar

if [ -e /tmp/vskippkgchk ]; then
   rm -f /tmp/vskippkgchk
fi

if ! hash python 2> /dev/null; then
   if [ -d /vnmr/web ]; then
      mv /vnmr/web /vnmr/web_off
   fi
fi

if [ -e /tmp/.ovj_installed ]; then
   nmr_user=$(/vnmr/bin/fileowner /vnmr/vnmrrev)
   opFile="/vnmr/adm/users/operators/operatorlist"
   chown $nmr_user:$nmr_group $opFile
   chmod 644 $opFile
   echo "Configuring $nmr_user with the standard configuration (stdConf)"
   echo "Configuring $nmr_user with the standard configuration (stdConf)" >> $insLog
   if [ x$distroType = "xdebian" ]; then
      sudo -i -u $nmr_user /vnmr/bin/Vnmrbg -mback -n1 stdConf >> $insLog
   else
      su - $nmr_user -c "/vnmr/bin/Vnmrbg -mback -n1 stdConf >> $insLog"
   fi
   if [ ! -d /home/walkup ] || [ ! -d /home/service ]
   then
      echo ""
      echo "Standard configurations include the walkup and service accounts."
      echo "Would you like to make then now? (y/n) "
      read ans
      echo " "
      if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
         accts='walkup service'
         for name in $accts
         do
            if [ ! -d /home/$name ]
            then
               echo "Making $name account"
               echo " " >> $insLog
               echo "Making $name account" >> $insLog
               /vnmr/bin/makeuser $name /home $nmr_group y >> $insLog
               echo "Adding $name as Locator account"
               if [ x$distroType = "xdebian" ]; then
                  sudo -i -u $nmr_user /vnmr/bin/create_pgsql_user $name >> $insLog 2> /dev/null
                  echo "Adding $name to OpenVnmrJ configuration files"
                  sudo -i -u $nmr_user /vnmr/bin/ovjUser $name
                  echo "Configuring $name with the standard configuration (stdConf)"
                  echo "Configuring $name with the standard configuration (stdConf)" >> $insLog
                  sudo -i -u $name /vnmr/bin/Vnmrbg -mback -n1 stdConf >> $insLog 2> /dev/null
                  echo "User $name has an initial password of abcd1234"
                  echo ""
               else
                  su - $nmr_user -c "/vnmr/bin/create_pgsql_user $name 2>> $insLog"
                  echo "Adding $name to OpenVnmrJ configuration files"
                  su - $nmr_user -c "/vnmr/bin/ovjUser $name"
                  echo "Configuring $name with the standard configuration (stdConf)"
                  echo "Configuring $name with the standard configuration (stdConf)" >> $insLog
                  su - $name -c "/vnmr/bin/Vnmrbg -mback -n1 stdConf >> $insLog" 2> /dev/null
                  echo "User $name has no initial password"
                  echo ""
               fi
            fi
         done
      fi
   fi

   echo " "
   echo "The latest version of NMRPipe can be installed."
   echo "Depending on network speed, it can take from"
   echo "10 to 60 minutes."
   echo "Would you like to install it now? (y/n) "
   read ans
   if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
      echo "Installing NMRPipe" >> $insLog
      if [ x$distroType = "xdebian" ]; then
         sudo -i -u $nmr_user /vnmr/bin/ovjGetpipe -l $insLog
      else
         su - $nmr_user -c "/vnmr/bin/ovjGetpipe -l $insLog"
      fi
      echo " "
   elif [[ x$oldVnmr != "x" ]] ; then
      if [[ -d $oldVnmr/nmrpipe ]] ; then
         echo "Collecting NMRpipe from $oldVnmr"
         cd $oldVnmr
         tar cf - nmrpipe | (cd /vnmr && tar xpf -)
      fi
   fi
   echo " "
   echo "New updates of NMRPipe may be installed at any time by running"
   echo "/vnmr/bin/ovjGetpipe"
   echo "To see all the NMRPipe installation options, such as installing"
   echo "NMRPipe without network access, use"
   echo "/vnmr/bin/ovjGetpipe -h"
   echo " "

   
   if [ ! -d /vnmr/help/WebHelp ]
   then
      echo " "
      echo "The VnmrJ 4.2 manuals can be installed."
      echo "Depending on network speed, it can take from"
      echo "5 to 35 minutes."
      echo "Would you like to install them now? (y/n) "
      read ans
      if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
         echo "Installing VnmrJ manuals" >> $insLog
         if [ x$distroType = "xdebian" ]; then
            sudo -i -u $nmr_user /vnmr/bin/ovjGetManuals -l $insLog
         else
            su - $nmr_user -c "/vnmr/bin/ovjGetManuals -l $insLog"
         fi
         echo " "
      else
         echo "The VnmrJ manuals may be installed at any time by running"
         echo "/vnmr/bin/ovjGetManuals"
      fi
      echo " "
      echo "To see all the manuals installation options, such as installing"
      echo "the manuals without network access, use"
      echo "/vnmr/bin/ovjGetManuals -h"
      echo " "
   fi

   if  [[ ! -d /vnmr/fidlib/Ethylindanone ]]; then
      echo " "
      echo "The VnmrJ 4.2 fidlib can be installed."
      echo "Depending on network speed, it can take from"
      echo "5 to 35 minutes."
      echo "Would you like to install it now? (y/n) "
      read ans
      if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
         echo "Installing VnmrJ fidlib" >> $insLog
         if [ x$distroType = "xdebian" ]; then
            sudo -i -u $nmr_user /vnmr/bin/ovjGetFidlib
         else
            su - $nmr_user -c "/vnmr/bin/ovjGetFidlib"
         fi
         echo " "
      else
         echo "The VnmrJ fidlib may be installed at any time by running"
         echo "/vnmr/bin/ovjGetFidlib"
      fi
      echo " "
      echo "To see all the fidlib installation options, such as installing"
      echo "the it without network access, use"
      echo "/vnmr/bin/ovjGetFidlib -h"
      echo " "
   fi

   echo "Shall this system be configured as a spectrometer."
   if [ -d /vnmr/acq/download ] || [ -d /vnmr/acq/vxBoot ]
   then
      echo "This involves setting up the network and downloading the"
      echo "acquisition console software"
   fi
   echo "Would you like to configure it now? (y/n) "
   read ans
   if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
      echo "Configuring system."
      /vnmr/bin/setacq
      echo " "
   else
      echo "The system may be configured as a spectrometer."
      echo "  1. Log in as the OpenVnmrJ adminstrator account, $nmr_user."
      echo "  2. Exit all OpenVnmrJ programs."
      echo "  3. Run /vnmr/bin/setacq"
   fi
   finalInstructions
fi

if [ -f $insLog ]
then
   date=$(date +%Y%m%d-%H%M)
   mv $insLog /vnmr/adm/log/vnmr$date
   chown $nmr_user /vnmr/adm/log/vnmr$date
   echo "Log written to: /vnmr/adm/log/vnmr$date"
fi

if [ -e /tmp/.ovj_installed ]; then
   echo ""
   echo "Installation of OpenVnmrJ is now complete"
   echo "The package OpenVnmrJ*.zip and the dvdimage directory"
   echo "may now be removed"
   echo ""
fi
rm -f /tmp/.ovj_installed

if [[ -z $oldVnmr ]]; then
   echo " "
   echo "To update OpenVnmrJ with configuration, shim, probe, operator, etc. information"
   echo "from a previous installation, copy the ovjUpdate script to /vnmr/bin on the"
   echo "previous host computer. Executing ovjUpdate on that computer will create"
   echo "a patch that can be installed on this computer"
   echo " "
fi


rm -f $base_link
# echo "\n>>>>>>  Finished the VnmrJ installation program.  <<<<<<"

if [ -f $newgrp ]
then
   rm -f $newgrp
   echo ""
   echo "Group of $nmr_user has been changed"
   echo "This requires a system reboot"
   echo ""
   echo "Would you like to reboot now? (y/n) "
   read ans
   echo " "
   if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
     if [ x$distroType = "xdebian" ]; then
        sudo reboot
     else
        reboot
     fi
   else
      echo "Please reboot before using OpenVnmrJ"
      echo "or unexpected results may occur."
      echo " "
   fi
fi
