: /bin/sh
: '@(#)loadpatch.sh 22.1 03/24/08 1991-1996 '
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

#
#  set up the OS type and other environment parameters
#
common_env() {

#  ostype:  IBM: AIX  Sun: SunOS or SOLARIS  SGI: IRIX
#  the system this script is running on defines the ostype
#  vnmros has a similar meaning, except the tape defines it

    ostype=`uname -s`
    if [ x$ostype = "xIRIX64" ]
    then
       ostype="IRIX"
    fi
    vnmros="SunOS"
    if (test -f IBM_patch)
    then
	vnmros="AIX"
    else if (test -f SGI_patch)
	 then
	     vnmros="IRIX"
         else if (test -f SOL_patch)
             then
                 vnmros="SOLARIS"
	     else
		 echo " "
		 echo "Error: Patch Type File Missing "
		 echo " "
		 sleep 30
		 exit 1
             fi
         fi
    fi

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
         else
             osver=`uname -r`
             osmajor=`echo $osver | awk 'BEGIN { FS = "." } { print $1 }'`
             atype=`uname -m | awk '
                 /^sun4/ { print "sun4" }
                 /^sun3/ { print "sun3" }
             '`
             if test $osmajor = "5"
             then
           	sysV="y"
                ostype="SOLARIS"
                ktype="solaris"
             else
          	sysV="n"
                ktype=`uname -m`
             fi
#  get the user name according to the protocol

	     case $osver in
	      4*) user=`whoami`
		  ;;
	      5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
		  ;;
	     esac
	 fi
    fi
}

#
#  uses the proper method of echoing a line without a CR for BSD or System V OS
#
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

#
#  copies a directory to another using tar to maintain all symbolic links, etc..
#
cpdir() {
echo " Copying $srcdir to $destdir "
if [ -d $srcdir ]
then
  if [ -d $destdir ]
  then
     nnl_echo " Delete Existing '$destdir' directory? ( y or n ) [y]: "
     read a
     if [ x$a != "xn" ]
     then
        rm -rf $destdir
     else
         echo " Installation of Patch Terminated"
         sleep 30
         exit 0
     fi
  fi
  mkdir $destdir
  if [ -d $destdir ]
  then
    cd $srcdir ; tar -cfb - 20 * | ( cd $destdir; tar -xpBfb - 20)
  else
    echo " "
    echo "Error: Unable to create destination directory: "
    echo "       '$destdir'"
    echo "        Patch Installation Terminated"
    echo " "
    sleep 30
    exit 1
  fi
else
  echo "Error:  Source Directory $srcdir missing: "
  echo "        Patch Installation Terminated"
  sleep 30
  exit 0
fi
}

#
# Determine if Big or Small vxWorks kernel is being used 
#
bigorsmall() {
if [ -d /tftpboot/vxBoot ]
then
    kernel=""
    cd /tftpboot/vxBoot
    ls | /usr/bin/grep  diag > /dev/null
    if [ $? -eq 0 ]
    then
        kernel="diag"
    else
	ls | /usr/bin/grep sym > /dev/null
	if [ $? -eq 0 ]
	then
           kernel="big"
	else
           kernel="small"
	fi
    fi
else 
        echo " "
	echo "Your system is not configured correctly"
        echo "Check your system Vnmr cofiguration"
        echo "Did you run the \"setacq\" script?"
        echo " "
        sleep 20
        exit
fi
}

cwdname=`pwd`
cmdname=`basename $0`
common_env

# if argument is not "proceed" then fork a cmdtool window to do the install from and run again
if [ x$cmdname = "xloadpatch" ]
then
  if [ x$1 != "xproceed" ]
  then
   cmdtool loadpatch proceed &
   exit 0
  fi
elif [ x$cmdname = "xremovepatch" ]
then
  if [ x$1 != "xproceed" ]
  then
   cmdtool removepatch proceed &
   exit 0
  fi
else
  echo "$cmdname unknown"
  sleep 10
  exit 1
fi


if [ x$cmdname = "xloadpatch" ]
then

dateval=`date +%y%m%d.%H:%M`
echo " "
date
echo " "

if [ x$ostype != x$vnmros ]
then
   echo " "
   echo " Error:  $cmdname must be run in the $vnmros system "
   echo " "
   sleep 15
   exit 1
fi

if [ x$user != "xvnmr1" ]
then
   echo " "
   echo " Error:   $cmdname must be run by vnmr1"
   echo " "
   sleep 15
   exit 1
fi

echo " Installing Inova Patch for 5MHz and L200 otional hardware."
echo " Original Files will be saved, and this patch can be backed out at any"
echo " time.  "
echo " "
echo " Backups will be in /vnmr/psg.bkup.date, /vnmr/acq.bkup.date, "
echo " /vnmr/lib.bkup.date and conpar.bkup.date (date is expanded the  "
echo " present date and time). The patch files will be placed in "
echo " /vnmr/psg and /vnmr/acq,  /vnmr/lib will be update by fixpsg "
echo " "
echo " Note: please reboot the console , 'su acqproc' and recompile"
echo " pulse sequences for this patch to take effect"
echo " "
#
nnl_echo "Proceed with patch? ( y or n ) [y]: "
read a
if [ x$a = "xn" ]
then
   exit 1
fi

bigorsmall

cd $cwdname

if [ x$kernel = "xsmall" ]
then
  echo " "
  echo "Your console currently using the \"small kernel\""
  echo "After loading the patch is complete, Login as root "
  echo "and using \"loadkernel\" RELOAD the small kernel. "
  echo "Otherwise the patch will not take effect and strange"
  echo "behavior might be exhibited by the system."
  echo "Don't forget to reboot the console. "
  echo " "
elif [ x$kernel = "xbig" ]
then
  echo " "
  echo "Your console currently using the \"big kernel\""
  echo "After loading the patch is complete, please reboot "
  echo "your console "
  echo " "
else
  echo " "
  echo "Your console is improperly configure to load this patch"
  echo "Login as root, and use loadkernel to have your console"
  echo "running either the small or big kernel"
  echo "Then loadpatch again"
  echo " "
  echo " Installation terminated "
  sleep 20
  exit 
fi

echo " Moving patch tar file to /vnmr/tmp"
set -x
rm -f /vnmr/tmp/*.tar.Z
rm -f /vnmr/tmp/*.tar
cp -p ./*.tar.Z /vnmr/tmp
uncompress /vnmr/tmp/*.tar.Z
set +x

echo " Backing up psg directory"
srcdir="/vnmr/psg"
destdir="/vnmr/psg.bkup.$dateval"
cpdir

echo " Backing up acq directory"
srcdir="/vnmr/acq"
destdir="/vnmr/acq.bkup.$dateval"
cpdir

echo " Backing up psg library directory"
srcdir="/vnmr/lib"
destdir="/vnmr/lib.bkup.$dateval"
cpdir

echo " Backing up conpar"
cp -p /vnmr/conpar /vnmr/conpar.bkup.$dateval

echo " Backing up showconsole"
cp -p /vnmr/bin/showconsole /vnmr/bin/showconsole.bkup.$dateval

echo " Backing up vconfig"
cp -p /vnmr/bin/vconfig /vnmr/bin/vconfig.bkup.$dateval


echo " Backup Complete. "
echo "$dateval Inova 5.2F 5MHz & L200 patch installed" > /vnmr/Vnmr5.2F_Patch.installed
chmod 444 /vnmr/Vnmr5.2F_Patch.installed
echo " Backup Complete. "
echo " Installing Patchs "
set -x
cd /vnmr
tar -xvf /vnmr/tmp/Vnmr5.2F_patch.tar
rm -f /vnmr/tmp/*.tar
set +x
echo "Patch files installed."
echo "Updating conpar"
sed -e 's/6 "l" "n" "p" "q" "s" "w"/8 "l" "n" "p" "q" "s" "t" "u" "w"/' \
 conpar.bkup.$dateval > conpar
echo "conpar Updated"
nnl_echo "Need to recompile psg, Proceed with compile? ( y or n ) [y]: "
read a
if [ x$a = "xn" ]
then
   exit 1
else
# might need tobe sure that vnmrsystem is set proper at this point
   fixpsg
fi
echo "VNMR 5.2F Inova  Software Patch Complete."
if [ x$kernel = "xsmall" ]
then
   echo "Don't forget to run loadkernel and reboot the console"
else
   echo "Please reboot console now"
fi
sleep 500
exit 0



else
#  remove patch

if [ -f /vnmr/Vnmr5.2F_Patch.installed ]
then

 bigorsmall

 echo " Removing Inova Patch for 5MHz and L200 otional hardware."
 findpatch="cat < /vnmr/Vnmr5.2F_Patch.installed | awk '{ print \$1 }'"
 dateval=`eval $findpatch`
 cd /vnmr
 echo "Restoring psg"
 mv psg psg.tmp
 set -x
 mv psg.bkup.$dateval psg
 set +x
 echo "Restoring psg library"
 mv lib lib.tmp
 set -x
 mv lib.bkup.$dateval lib
 set +x
 echo "Restoring acq"
 mv acq acq.tmp
 set -x
 mv acq.bkup.$dateval acq
 set +x
 echo "Restoring conpar"
 mv conpar conpar.tmp
 set -x
 mv conpar.bkup.$dateval conpar
 set +x
 echo "Restoring showconsole"
 mv bin/showconsole bin/showconsole.tmp
 set -x
 mv bin/showconsole.bkup.$dateval bin/showconsole
 set +x
 echo "Restoring vconfig"
 mv bin/vconfig bin/vconfig.tmp
 set -x
 mv bin/vconfig.bkup.$dateval bin/vconfig
 set +x
 chmod 644 Vnmr5.2F_Patch.installed
 rm -f Vnmr5.2F_Patch.installed
 rm -rf *.tmp

 if [ x$kernel = "xsmall" ]
 then
    echo "As root run loadkernel and reload the small kerenl, then reboot the console"
 else
    echo "Please reboot console now"
 fi

else
 echo "Patch never installed or the Installation File"
 echo "'Vnmr5.2F_Patch.installed' has beed deleted"
 echo "Patch can not be removed"
 sleep 15
 exit 0
fi

fi
