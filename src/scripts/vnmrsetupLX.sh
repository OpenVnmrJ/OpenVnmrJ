#! /bin/sh
#
# '@(#)vnmrsetupLX.sh 22.1 03/24/08 2003-2004 '
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
# This is the start-up script for loading VnmrJ software
#
# "load.nmr" could be executed with user and group option
# ex: load.nmr <chin> <software> .
# vnmr1 and nmr as user and group by default.
# This version only supports Linux installation
#

#This script also responsible for loading JRE enviroment in order to
#start running LoadNmr
#

set_system_stuff() {

   ostype=`uname -s`
   case x$ostype in

         "xLinux" )   os_type="rht"
                      sysV="y"
                      JRE="jre.linux"
                      default_dir="/home"
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

    versionLine=`grep VERSION ${base_dir}/vnmrrev`
    major=`echo $versionLine | awk 'BEGIN { FS = " " } { print $3 }'`
    minor=`echo $versionLine | awk 'BEGIN { FS = " " } { print $5 }'`

    $GREP -i -E 'alpha|beta|devel'  ${base_dir}/vnmrj 2> /dev/null
    if [ $? -eq 0 ]
    then
       date=`grep 200 ${base_dir}/vnmrj`
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

       dest_dir="${default_dir}/vnmrj_${major}${minor}_${year}-${dmonth}-${dday}"
    else
       dest_dir="${default_dir}/vnmrj_${major}_${minor}"
    fi
}

#-------------------------------------------------
#  MAIN Main main
#-------------------------------------------------
# Exit for AIX and IRIX and IRIX64
os=`uname -s`

case x$os in
    xAIX|xIRIX|xIRIX64|xSunOS ) 
		echo ""
                echo "This VnmrJ_LX CD supports Linux only"
                echo "Use VnmrJ 1.1 D for Sun computers"
		echo ""
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

notroot=0
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To install VnmrJ you will need to be the system's root user."
  echo "Or type cntrl-C to exit.\n"
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password \n"
     su root -c "$base_dir/load.nmr $*";
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

if [ $# -gt 3 ]
then
    echo "\nUsage:  $0 [user name] [group name] [destination directory]"
    exit 1
fi

host_name=`uname -n`
JRE="jre"
set_system_stuff

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

$java_cmd -classpath $jre_base_dir/${JRE}/lib/rt.jar:$adm_base_dir/VnmrAdmin.jar LoadNmr $base_dir $dest_dir $nmr_user $user_dir $nmr_group $os_type

cd $adm_base_dir
rm ./VnmrAdmin.jar
(cd /vnmr/tcl/bin; rm -f tclsh; ln -s /usr/bin/tclsh tclsh)

# echo "\n>>>>>>  Finished the VnmrJ installation program.  <<<<<<"

