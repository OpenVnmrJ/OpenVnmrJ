#! /bin/sh
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
#   aka load.nmr 
#
# This is the start-up script for loading Vnmr software
# "loadvnmr" first determinaes which system is running (SunOS, Solaris
# IRX, IRIX64 or AIX). Based on that it will gather the right files
# to pass to `LoadNmr' for installation choices.
#
# "loadvnmr" could be executed with user and group option
# ex: loadvnmr <chin> <software> .
# vnmr1 and nmr as user and group by default.
#

#This script also responsible for loading JRE enviroment in order to
#start running LoadNmr
#

set_system_stuff() {

   distroName="na"
   distroType="na"
   ostype=`uname -s`
   case x$ostype in

         "xLinux" )   os_type="rht"
                      sysV="y"
                      JRE="jre.linux"
                      default_dir="/home"
                      rhelrlvl=" "
                      distroName=`cat /etc/issue`
                      if [ -f /etc/debian_version ]; then
                         distroType="debian"
                      elif [ -f /etc/redhat-release ]; then
                         distroType="rhel"
                         if [ "$(cat /etc/issue | grep 'release 4' > /dev/null;echo $?)" == "0" ]; then
                          rhelrlvl="4"
                         elif [ "$(cat /etc/issue | grep 'release 5' > /dev/null;echo $?)" == "0" ]; then
                           rhelrlvl="5"
                         elif [ "$(cat /etc/issue | grep 'release 6' > /dev/null;echo $?)" == "0" ]; then
                           rhelrlvl="5"     # avoid a messy if down below
                         fi
                      elif [ -f /etc/SuSE-release ]; then
                         distroType="suse"
                      fi 
                      ;;

         "xIRIX" )    os_type="sgi"
                      sysV="y"
                      default_dir="/usr/people"
                      ;;

         "xIRIX64" )  ostype="IRIX"
                      os_type="sgi"
                      sysV="y"
                      default_dir="/usr/people"
                      ;;

            "xAIX" )  
                      os_type="ibm"
                      sysV="y"
                      default_dir="/home"
                      ;;

                 * )  osver=`uname -r`
                      osmajor=`echo $osver | awk 'BEGIN { FS = "." } { print $1 }'`

                      if [ $osmajor = "5" ]
                      then
                         ostype="SOLARIS"
                         os_type="sol"
                         sysV="y"
                         default_dir="/export/home"
			 if test ! -d $default_dir
			 then
                             default_dir="/space"
			 fi
                      else
                         ostype="SunOS"
                         os_type="sos"
                         sysV="n"
                         default_dir="/home"
                      fi
                      ;;
   esac
}

# echo without newline - needs to be different for BSD and System V
nnl_echo() {
   if test x$sysV = "x"
   then
      echo "error in echo-no-new-line: sysV not defined"
      exit 1
   fi

   if test $sysV = "y"
   then
      if test $# -lt 1
      then
         echo
      else
         echo "$*\c"
      fi
   else
      if test $# -lt 1
      then
         echo
      else
         echo -n $*
      fi
   fi
}

test_user() {
 
   my_file=${adm_base_dir}/testfile_please_remove
   touch $my_file
   chown $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       rm -f $my_file
       echo "\nUser \"$1\" does not exist,"
       echo "use Solaris Admintool to create user and rerun $0"
       echo "\nAborting this program....."
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
       echo "use Solaris Admintool to create group and rerun $0"
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
        "xsol" ) GREP="/usr/xpg4/bin/grep" ;;
             * ) GREP="grep" ;;
    esac

    versionLine=`grep -i VERSION ${base_dir}/vnmrrev`
    major=`echo $versionLine | awk 'BEGIN { FS = " " } { print $3 }'`
    minor=`echo $versionLine | awk 'BEGIN { FS = " " } { print $5 }'`
    versionLine=`grep VnmrJ_SE ${base_dir}/vnmrrev | wc -l`
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
       date=`grep "[0-9]*[0-9], *\b[2-9][0-9][0-9][0-9]\b" ${base_dir}/vnmrrev`
       month=`echo $date |  awk 'BEGIN { FS = " " } { print $1 }'`
       day=`echo $date |  awk 'BEGIN { FS = " " } { print $2 }'`
       year=`echo $date |  awk 'BEGIN { FS = " " } { print $3 }'`

       if [ x$month = "xJanuary" ]
       then 
          dmonth="01"
       else if [ x$month = "xFebruary" ]
       then 
          dmonth="02"
       else if [ x$month = "xMarch" ]
       then 
          dmonth="03"
       else if [ x$month = "xApril" ]
       then 
          dmonth="04"
       else if [ x$month = "xMay" ]
       then 
          dmonth="05"
       else if [ x$month = "xJune" ]
       then 
          dmonth="06"
       else if [ x$month = "xJuly" ]
       then 
          dmonth="07"
       else if [ x$month = "xAugust" ]
       then 
          dmonth="08"
       else if [ x$month = "xSeptember" ]
       then 
          dmonth="09"
       else if [ x$month = "xOctober" ]
       then 
          dmonth="10"
       else if [ x$month = "xNovember" ]
       then 
          dmonth="11"
       else
          dmonth="12"
       fi fi fi fi fi fi fi fi fi fi fi 
 
       dday=`basename $day ,`

       dest_dir="${default_dir}/${vname}${major}${minor}_${year}-${dmonth}-${dday}"
    else
       dest_dir="${default_dir}/${vname}${major}_${minor}"
    fi
}

disableSelinux() {

 if [ x$distroType  != "xdebian" ] ; then
    #   SELINUX=disabled   (possible values: enforcing, permissive, disabled)
   selinux=`grep SELINUX /etc/selinux/config | grep -i disabled`
   # echo "str: $str"
   if [ -z "$selinux" ] ; then
       echo ""
       echo ""
       echo "Disabling SELinux, System Reboot Required."
       echo "You must reboot the system prior to continuing the install."
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


#disablefirewall() {
#
#   if [ x$distroType  != "xdebian" ] ; then
#     ipmod=`/sbin/lsmod | grep _tables`
#     # if ip_tables loaded into kernel then we should just turn it off
#     # even though it might already be off.
#     if [ ! -z "$ipmod" ]; then
#        /sbin/service iptables save  > /dev/null  2>&1
#        /sbin/service iptables stop  > /dev/null  2>&1
#        /sbin/chkconfig iptables off  > /dev/null  2>&1
#        # ipv6 may not be active but just turn it off  to be sure
#        /sbin/service ip6tables save  > /dev/null  2>&1
#        /sbin/service ip6tables stop  > /dev/null  2>&1
#        /sbin/chkconfig ip6tables off  > /dev/null  2>&1
#     fi
#   fi
#
#}
#-------------------------------------------------
#  MAIN Main main
#-------------------------------------------------
# Exit for AIX and IRIX and IRIX64

# remove this for release
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
os=`uname -s`

case x$os in
    xAIX|xIRIX|xIRIX64|xSunOS ) 
		echo Not for $os
		exit;;
esac

# get base_dir first so we have it at all times
#
firstchar=`echo $0 | cut -c1-1`
if [ x$firstchar = "x/" ]  #absolute path
then
   base_dir=`dirname $0`
else
   if [ x$firstchar = "x." ]  #relative path
   then
       if [ x`dirname $0` = "x." ]
       then
           base_dir=`pwd`
       else
           base_dir=`pwd`/`dirname $0 | sed 's/.\///'`
       fi
   else
      base_dir=`pwd`/`dirname $0`
   fi
fi

#
# Login the user as a root user
# Use the "su" command to ask for password and run the installer
#

# need the distro type for root or sudo login
set_system_stuff

#
#  First make sure the firewall and SELinux are OFF.
#

#
# for VnmrJ 3.0 we must check if require RHEL packages have been installed
# prior to installing VnmrJ
#
# only test on RHEL 5.x
if [ "x$distroType" = "xrhel" -a "x$rhelrlvl" = "x5" -o "x$distroType" = "xdebian" ]; then
# any argument with load.nmr will skip this test
  if [ ! -e /tmp/vskippkgchk ]; then
    dirpath=`pwd`
    # check for required RPM packages
    sh $dirpath/code/chksystempkgs
    if [ $? -ne 0 ]; then
      # echo the Standby because it takes several seconds to copy and bring up the next xmessage
      echo " Standby........ "
      mkdir -p /tmp/agilent_preinstall
      # dont let the copy complain about the TuboVNC directory that it can't copy, that's OK
      cp $dirpath/code/linux/* /tmp/agilent_preinstall > /dev/null 2>&1
      # change permission so that even if another user does the install
      # we won't get permission errors
      chmod 777 /tmp/agilent_preinstall
      chmod go+w  /tmp/agilent_preinstall/*

      if [ -x /usr/bin/xmessage ]; then
        /usr/bin/xmessage -center -default OK -buttons OK:0 -file rpmInstruction.txt -timeout 300
        # just so the user has another chance of copy/paste
        echo " "
        echo "cd /tmp/agilent_preinstall"
        echo "./installpkgs"
        echo " "
        exit 0
      else
        echo " "
        echo "================================================================== "
        echo " "
        echo "There are RHEL packages required by VnmrJ that are not installed"
        echo " "
        echo "VnmrJ Installation can not proceed until the required Linux packages are Installed. "
        echo " "
        echo " "
        echo "Please following the instructions below to install the Linux packages: "
        echo " "
        echo "Eject the VnmrJ Installation CD and go to the directory /tmp/agilent_preinstall"
        echo "Insert your systems RHEL 5 Installation DVD and run the installpkgs script"
        echo "e.g.: "
        echo " "
        echo "cd /tmp/agilent_preinstall"
        echo "./installpkgs"
        echo " "
        echo " "
        echo "After completion Eject RHEL DVD, insert the VnmrJ CD and start the VnmrJ install."
        echo " "
        echo "================================================================== "
        echo " "
        sleep 30
        exit 1
      fi
    fi 
  fi
fi

notroot=0
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To install VnmrJ you will need to be the system's root user."
  echo "Or type cntrl-C to exit."
  echo
  #
  # determine if running from mount CD/DVD media
  # if so, need to remount with exec privileges
  # base_dir would be the mount point if CD/DVD
  #
  mntline=`df --type=iso9660 2>&1 | grep $base_dir`
#  echo "mntline: $mntline"
  if [ ! -z "$mntline" ] ; then
     # This is probably a noexec mounted CD/DVD. Interactively asking for root password
     # via su or sudo doesn't work.  So force user to do this prior to running
     # shell script
     if [ -f /etc/debian_version ]; then
       echo "Please rerun this script using: sudo sh $0".
     else
       echo "Please su to root, Then rerun this script: sh $0".
     fi
     exit 0
  fi

  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password"
     if [ x$distroType = "xdebian" ]; then
        sudo $base_dir/load.nmr $* ;
     else
        su root -c "$base_dir/load.nmr $*";
     fi
     s=$?
     t=`expr $t - 1`
     echo " "
     if [ -f /tmp/reboot ] ; then
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
# determine if running from mount CD/DVD media
# if so, need to remount with exec privileges
# base_dir would be the mount point if CD/DVD
#
mntline=`df --type=iso9660 2>&1 | grep $base_dir`
# echo "mntline: $mntline"
if [ ! -z "$mntline" ] ; then
  mount -o remount,ro,exec /dev/dvd
fi


# remove reboot flag for SELinux disabling
rm -f /tmp/reboot

# two possible install options plus 3 args
if [ $# -gt 4 ]   
then
    echo "\nUsage:  $0 [install_opts] [user name] [group name] [destination directory]"
    exit 1
fi

host_name=`uname -n`
JRE="jre"
set_system_stuff

#
#  First make sure the firewall and SELinux are OFF.
#

# check firewall since we can disable this without reboot
# disablefirewall

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
jre_base_dir=$adm_base_dir
 
if [ ! -d $adm_base_dir ]
then
    mkdir -p $adm_base_dir
fi

if [ $# -eq 1 -a x$1 != "xvnmr1" ]
then  
   if [ x$1 != "xSun" ]
   then
      test_user $1
   fi
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
   calldir=`dirname $calldir`
done

src_code_dir=${base_dir}/code
export ostype

theuserid=`id | tr '()' '  ' | cut -f2 -d' '`
thewindowid=`who -m | awk '{ print $1 }'`
isremotewindow=`echo $DISPLAY | tr ':' ' ' | cut -f1 -d' '`
if [ x$isremotewindow = "x" ]
then
  if [ x$theuserid = "xroot" -a x$theuserid != x$thewindowid ]
  then
    echo ""
    if [ x$ostype = "xLinux" ]
    then
        xhost + $host_name
    else
        su $thewindowid -c "/usr/openwin/bin/xhost + $host_name"
    fi
  fi
fi

echo "Starting the VnmrJ installation program..."

jre_base_dir=$src_code_dir
java_cmd=$jre_base_dir/${JRE}/bin/java

cd $adm_base_dir
cp $src_code_dir/VnmrAdmin.jar .

$java_cmd -classpath $jre_base_dir/${JRE}/lib/rt.jar:$adm_base_dir/VnmrAdmin.jar LoadNmr $base_dir $dest_dir $nmr_user $user_dir $nmr_group $os_type $1

cd $adm_base_dir
rm ./VnmrAdmin.jar

if [ -e /tmp/vskippkgchk ]; then
   rm -f /tmp/vskippkgchk
fi

# remove preinstall temp directory
if [ -d /tmp/agilent_preinstall ] ; then
    # 1st copy the log file into /vnmr/adm/log
   cp  /tmp/agilent_preinstall/pkgInstall.log* /vnmr/adm/log
   rm -rf /tmp/agilent_preinstall
fi
# echo "\n>>>>>>  Finished the VnmrJ installation program.  <<<<<<"

