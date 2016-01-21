: '@(#)autoloadcd.sh 22.1 03/24/08 1991-1994 '
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
#! /bin/sh
: '@(#)loadcd.sh 14.3 06/30/99 1991-1997 '
# This is the start-up script for loading Vnmr software
# load.nmr first determinaes which system is running (SunOS, Solaris
# IRX, IRIX64 or AIX). Based on that it will gather the right files
# to pass to `ins.xxx' for installation choices.
#
# Determine OS type, can be overwritten by first argument.
#

set_system_stuff() {
#  ostype:  IBM: AIX  Sun: SunOS  SGI: IRIX

    ostype=`uname -s`
    if [ x$ostype = "xIRIX64" ]
    then
      ostype="IRIX"
    fi

    if [ x$ostype = "xAIX" ]
    then
        sysV="y"
        default_dir="/home"
    else if [ x$ostype = "xIRIX" ]
         then
             sysV="y"
             default_dir="/usr/people"
         else
             osver=`uname -r`
             osmajor=`echo $osver | awk 'BEGIN { FS = "." } { print $1 }'`
             if test $osmajor = "5"
             then
                sysV="y"
                default_dir="/export/home"
                ostype="solaris"
             else
                sysV="n"
                default_dir="/home"
             fi
         fi
    fi
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

   date=`date +%y%m%d.%H:%M`
   my_file="/tmp/my.file."$date
   touch $my_file
   chown $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then   
       rm -f $my_file
       echo "\nUser \"$1\" does not exist, use Solaris Admintool to create user and rerun $0"
       echo "Aborting this program............."
       echo " "
       exit 1
   else
       nmr_adm=$1
   fi 

   return 1
}

################ Main ######################
 
set_system_stuff

ostype=`uname -s`
if [ $ostype = "SunOS" ]
then
   rev=`uname -r`
   if [ $rev -ge 5.3 ]
   then   
         ostype="SOLARIS"
   fi
fi
 
 
nmr_adm="vnmr1"
nmr_group="nmr"

if [ $# -eq 1 ]
then  
   test_user $1
else 
   if [ $# -eq 2 ]
   then  
      test_user $1

      date=`date +%y%m%d.%H:%M`
      my_file="/tmp/my.file."$date
      touch $my_file
      chgrp $2 $my_file 2>/dev/null
      if [ $? -ne 0 ]
      then
          rm -f $my_file
          echo "\nGroup \"$2\" does not exist, use Solaris Admintool to create group and rerun $0"
          echo "Aborting this program............." 
          echo " "
          exit 1
      else
          nmr_group=$2
      fi
   fi 
fi
 
 
#
# Login the user as a root user
# Use the "su" command to ask for password and run the installer
#

notroot=0
userId=`/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To install VNMR you will need to be the system's root user."
  echo "Or type cntrl-C to exit.\n"
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password \n"
     su root -c "./load.nmr ${ARGS}";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again \n"
  fi
  exit
fi

#
# User is now root.
#

# construct the source directory
pwd=`pwd`
dir=`dirname $0`
if [ x$dir = "x." ]
then
   source_dir=$pwd
else
   source_dir=$pwd/$dir
fi

source_dir_code=$source_dir/code

export nmr_adm nmr_group ostype

case $ostype in
   SunOS )
	osexten="sos"
	export osexten
	$source_dir_code/ins.sos inova.sos inova.opt
	;;
   SOLARIS )
	osexten="sol"
	export osexten
#	/rdvnmr/.cdrom04.06/code/ins.sol inova.sol inova.opt mercvx.sol 
	$source_dir_code/ins.sol inova.sol inova.opt mercplus.sol mercplus.opt mercvx.sol mercvx.opt mercury.sol mercury.opt g2000.sol g2000.opt uplus.sol uplus.opt unity.sol unity.opt
	;;
   AIX )
	osexten="ibm"
	export osexten
	$source_dir_code/ins.ibm ibm.ibm ibm.opt
	;;
   IRIX | IRIX64)
	osexten="sgi"
	export osexten
	$source_dir_code/ins.sgi sgi.sgi sgi.opt
	;;
esac


if [ $notroot = 1 ]
then
   echo "All done, type cntrl-C to exit window"
fi

