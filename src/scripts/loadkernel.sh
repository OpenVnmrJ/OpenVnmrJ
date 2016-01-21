: '@(#)loadkernel.sh 19.2 06/02/08 1991-2008 '
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
vnmrsystem=/vnmr

common_env() {

#  ostype:  IBM: AIX  Sun: SunOS  SGI: IRIX

    ostype=`"$vnmrsystem"/bin/vnmr_uname`

    if [ x$ostype = "xAIX" ]
    then
        osver=`uname -v`.`uname -r`
        sysV="y"
        ktype="AIX"
        atype="ibm"
        user=`whoami`
    else if [ x$ostype = "xIRIX" ]
         then
             osver=`uname -r`
             sysV="y"
             ktype="IRIX"
             atype="sgi"
             user=`whoami`
    else if [ x$ostype = "xLinux" ]
         then
             osver=`uname -r`
             sysV="y"
             ktype="IRIX"
             atype="lnx"
             user=`whoami`
         else
             osver=`uname -r`
             atype="sun4"
             if [ x$ostype = "xSOLARIS" ]
             then
                sysV="y"
                ktype="solaris"
                user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
             else
                sysV="n"
                ktype=`uname -m`
                user=`/usr/ucb/whoami`
             fi
         fi
    fi
    fi
}      

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

are_you_root() {
                    # verify the current process is root
case `uname -r` in
    34*) user=`/usr/bin/whoami`
        ;;
    53*) user=`/usr/bin/whoami`
        ;;
    4*) user=`/usr/ucb/whoami`
        ;;
    5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
        ;;
esac
 
if (test ! $user = "root")
then
   echo "To run \"$0\" you must be Root"
   echo "Please login as Root, then restart \"$0\" again ."
   echo " "
   exit 1
fi
}

################## Main program starts here ###################
clear
echo "\n\n"
common_env
are_you_root      # verify the current process is root?

#--------------------------------------------------------------
#
# This is where the 162 kernel stored .
#
#--------------------------------------------------------------
if [ -d /tftpboot/vxBoot ]
then
    cd /tftpboot/vxBoot
    ls | /bin/grep  diag > /dev/null
    if [ $? -eq 0 ]
    then
        echo "Your console currently runs on \"diag kernel\""
        echo " "
    else
	ls | /bin/grep sym > /dev/null
	if [ $? -eq 0 ]
	then
           echo "Your console currently runs on \"big kernel\""
           echo " "
	else
           echo "Your console currently runs on \"small kernel\""
           echo " "
	fi
    fi
else 
	echo "Cannot find /tftpboot/vxBoot"
	echo "Your system is not configured correctly"
        echo "Check your system Vnmr configuration"
        echo "Did you run the \"setacq\" script?"
        exit
fi

#--------------------------------------------------------------
#
# Checking for the availabilities
# This script was written with the Diag. task in mind
# if there isn't a copy of vxBoot.diag, the script will be terminated .
#
#---------------------------------------------------------------
if [ -d "$vnmrsystem"/acq ]
then
    echo "Kernels present:"
    if [ -d "$vnmrsystem"/acq/vxBoot.small ] 
    then
       echo "Small fast kernel present - standard configuration"
    fi
    if [ -d "$vnmrsystem"/acq/vxBoot.big ] 
    then
       echo "Big more messages kernel available"
    fi
    if [ -d "$vnmrsystem"/acq/vxBoot.diag ]
    then
      echo "Diagnostics (testing only) available"
    fi
   nnl_echo "Do you still want to go through this program? (y or n) [y]:"
   read answer
   echo " " 
   if [ x$answer = "xn" ]
   then 
         echo "Aborting this program............."
         echo " "
         exit 1
   fi
else
   echo "Your system is not configured correctly"
   echo "Check your system Vnmr configuration"
   exit
fi
 
#--------------------------------------------------------------
#
# Let's select the desired kernel
#
#--------------------------------------------------------------
    mercvx=1
    /bin/grep -q -E 'mercvx|mercplus' "$vnmrsystem"/vnmrrev
    if [ $? -ne 0 ]
    then
       mercvx=0
    fi
    if [ $mercvx -eq 1 ]
    then 
       echo "Please enter the number for the kernel "
       echo "which you want to boot"
           echo "        vxBoot.small = 1 "
       nnl_echo "        vxBoot.big   = 2       :"
    else
       echo "Please enter a number accordingly to the kernel "
       echo "which you want to boot"
       echo "        vxBoot.small (PPC included)  = 1 "
       if [ ! -d "$vnmrsystem"/acq/vxBoot.diag ]
       then
         nnl_echo "        vxBoot.big (PPC included)  = 2       :"
       else
         echo "        vxBoot.big (PPC included)  = 2 "
         nnl_echo "                       vxBoot.diag  = 3       :"
       fi
    fi
    read kernel
    echo "Your response was : $kernel"
    nnl_echo "Are you sure? (y or n) [y]:"
    read answer
    echo " "
    if [ x$answer = "xn" ]
    then
       echo "Aborting this program............."
       echo " "
       exit 1
    else
       if [ ! -d "$vnmrsystem"/acq/vxBoot.diag -a x$kernel = x3 ]
       then 
          echo "\"$kernel\" is a wrong choice at this point "
          echo " "
          exit 
       else 
          if [ ! -f "$vnmrsystem"/acq/vwScript.big  ]
          then 
            cp  "$vnmrsystem"/acq/vwScript "$vnmrsystem"/acq/vwScript.big
          fi

          case "$kernel" in
            1)
               rm -f /tftpboot/vxBoot/*
               rm -f "$vnmrsystem"/acq/vxBoot
               cp -p "$vnmrsystem"/acq/vxBoot.small/*  /tftpboot/vxBoot
               ln -s "$vnmrsystem"/acq/vxBoot.small    "$vnmrsystem"/acq/vxBoot
               if [ $mercvx -eq 0 ]
               then
                  rm -f "$vnmrsystem"/acq/vxBootPPC
                  cp -p "$vnmrsystem"/acq/vxBootPPC.small/vxWorks  /tftpboot/vxBoot/vxWorksPPC
                  ln -s "$vnmrsystem"/acq/vxBootPPC.small          "$vnmrsystem"/acq/vxBootPPC
               fi
               echo "a \"small\" kernel is now ready for booting"
               ;;
            2)
               rm -f /tftpboot/vxBoot/*
               rm -f "$vnmrsystem"/acq/vxBoot
               cp -p "$vnmrsystem"/acq/vxBoot.big/vxWorks*  /tftpboot/vxBoot
               ln -s "$vnmrsystem"/acq/vxBoot.big           "$vnmrsystem"/acq/vxBoot
               if [ $mercvx -eq 0 ]
               then
                  rm -f "$vnmrsystem"/acq/vxBootPPC
                  cp -p "$vnmrsystem"/acq/vxBootPPC.big/vxWorks     /tftpboot/vxBoot/vxWorksPPC
                  cp -p "$vnmrsystem"/acq/vxBootPPC.big/vxWorks.sym /tftpboot/vxBoot/vxWorksPPC.sym
                  ln -s "$vnmrsystem"/acq/vxBootPPC.big             "$vnmrsystem"/acq/vxBootPPC
               fi
               echo "a \"big\" kernel is now ready for booting"
               ;;
            3)
               rm -f /tftpboot/vxBoot/*
               rm -f "$vnmrsystem"/acq/vxBoot
               rm -f "$vnmrsystem"/acq/vxBootPPC
               cp -p "$vnmrsystem"/acq/vxBoot.big/* /tftpboot/vxBoot
               cp -p "$vnmrsystem"/acq/vxBoot.diag/vwScript "$vnmrsystem"/acq/vwScript
               cp -p "$vnmrsystem"/acq/vxBoot.diag/vwScript /tftpboot/vxBoot/vwScript.diag
               echo "\"Diag.\" kernel is now ready for booting"
               ;;
            *)
               echo "\" $kernel\" is not a correct response"
               exit
               ;;
          esac
          if [ $mercvx -eq 0 ]
          then
             cp -p "$vnmrsystem"/acq/vxBoot.auto/vxWorks.auto /tftpboot/vxBoot
          fi
       fi 
       echo " "
       echo "Please, REBOOT your console by PRESSING the RESET BUTTON"
       echo "on console's CPU board to activate new kernel ."
       echo " "
    fi
