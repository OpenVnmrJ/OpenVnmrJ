: '@(#)load.nmrj.sh 22.1 03/24/08 1991-1994 '
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

# This is the start-up script for loading Vnmr software
# load.nmr first determinaes which system is running (SunOS, Solaris
# IRX, IRIX64 or AIX). Based on that it will gather the right files
# to pass to `ins.xxx' for installation choices.
#
# Determine OS type, can be overwritten by first argument.
#


#This script also responsible for loading JRE enviroment inorder to
#start running LoadNmr
#
#could check for /vnmr/JRE directory


set_system_stuff() {

   ostype=`uname -s`
   case $ostype in

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
 
   my_file="/tmp/testfile_please_remove"
   touch $my_file
   chown $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       rm -f $my_file
       echo "\nUser \"$1\" does not exist,"
       echo "use Solaris Admintool to create user and rerun $0"
       echo "\nAborting this program............."
       echo " "
       exit 1
   else
       nmr_user=$1
   fi
}

test_group() {

   my_file="/tmp/testfile_please_remove"
   touch $my_file
   chgrp $1 $my_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       rm -f $my_file
       echo "\nGroup \"$1\" does not exist,"
       echo "use Solaris Admintool to create group and rerun $0"
       echo "\nAborting this program............."
       echo " "
       exit 1
   else
       nmr_group=$1
   fi
}

get_nic_val() {

  t_val=` awk ' BEGIN {  FS="\""
                       nicName="'$1'"
                       nicCount=0
                    }
 
                    { if ($4==nicName) nicCount++ }  

                END { print nicCount }  
              ' < /etc/path_to_inst
        `
  case $1 in
          "le" ) le_val=$t_val ;;
         "hme" ) hme_val=$t_val ;;
             * ) ;;
  esac

}

#-------------------------------------------------
#  MAIN Main main
#-------------------------------------------------

#user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
#if [ x$user != "xroot" ]
#then
#   echo "You must be root to load VNMR and its options."
#   echo "Become root then start load.nmr again."
#   exit 1
#fi

if [ $# -gt 3 ]
then
    echo "\nUsage:  load.nmrj [user name] [group name] [destination directory]"
    exit 1
fi

set_system_stuff

le_val=0 
hme_val=0
get_nic_val "le"
get_nic_val "hme"

nmr_user="vnmr1"
nmr_group="nmr"
dest_dir="/export/home/vnmr"

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

src_code_dir=${base_dir}/code #will be passed to LoadVnmr
export ostype

java -classpath /usr25/chin/jloadnmr LoadNmr $src_code_dir $dest_dir $nmr_user $nmr_group $os_type $le_val $hme_val
