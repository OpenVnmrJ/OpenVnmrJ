#!/bin/sh
# '@(#)patchinstall.sh 22.1 03/24/08 1991-2004 ' 
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

#----------------------------------------------------------
#The format is
#     patchinstall "full path-name of patch file"
#example
#     patchinstall /usr24/chin/patch.big/test.acq/5.3BacqSOLino101.tar.Z
#where :
#     5.3B  is Vnmr software version ; BETA Special:  ##b#
#                                      ex: 61b1 is version 6.1 BETA 1
#     acq   is the category under $vnmrsystem such as: acq, psg, adm ...
#                                                      mac for maclib
#                                                      acb for acqbin
#                                                      edy for eddylib
#     SOL   for SOLARIS could be AIX, IRI ...
#     ino   for inova could be gem for geminy
#                              mer for mercury
#                              upl for unity+
#                              uni for unity
#                              all for all system, such as "adm" category
#
#     101   is the count, could be from anywhere within 3 digit range
#     .tar.Z  the patch file needs to be tarred and compressed
#----------------------------------------------------------

common_env() {

#  ostype:  IBM: AIX  Sun: SunOS  SGI: IRIX
 
    ostype=`$vnmrsystem/bin/vnmr_uname`
 
    if [ x$ostype = "xDarwin" ]
    then 
        osver=`uname -r`
        sysV="n" 
        ktype="MAC"
        atype="mac"
        user=`id -un`
    elif [ x$ostype = "xInterix" ]
    then
        osver=`uname -r`
        sysV="n" 
        ktype="WIN"
        atype="win"
        user=`id -un`
    elif [ x$ostype = "xIRIX" ]
    then
        osver=`uname -r`
        sysV="y"
        ktype="IRIX"
        atype="sgi"
        user=`whoami`
    elif [ x$ostype = "xLinux" ]
    then
        osver=`uname -r`
        sysV="y"
        ktype="Linux"
        atype="rht"
        user=`/usr/bin/id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
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

#-----------------------------------------------------------
#  Get value of vnmrsystem
#  If not defined, ask for its value
#  Use /vnmr as the default
#  make sure directory exists
#-----------------------------------------------------------
get_vnmrsystem() {

    if [ "x$vnmrsystem" = "x" ]
    then
        nnl_echo "Please enter location of VNMR system directory [/vnmr]: "
        read vnmrsystem
        if test "x$vnmrsystem" = "x"
        then
            vnmrsystem="/vnmr"
        fi
    fi
 
    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, exit patchinstall"
        exit
    fi
    export vnmrsystem
}

#-----------------------------------------------------------
# do_lot_of_checks <.Z file>
#-----------------------------------------------------------
do_lot_of_checks () {

   ###Is this patch file exist (and readable)
   if [ ! -r "$patchfile" ]
   then
       echo ""
       echo "$0:   $patchfile is not readable"
       echo ""
       return 1
   fi

   patch_filename=`basename "$patchfile"`
   teststring=`basename "$patch_filename" | cut -c17-`

   ###Is this file has the right format
   if [ x$teststring != "x.tar.Z" ]
   then
       echo ""
       echo " $patchfile   not correct VNMR patch file format"
       echo ""
       return 1
   fi

   patch_id=`echo "$patch_filename" | sed  's/.tar.Z//'`

   is_beta=`echo "$patch_filename" | cut -c3`

   patch_version=`echo "$patch_filename" | cut -c1-3`
   patch_revision=`echo "$patch_filename" | cut -c4`
   patch_category=`echo "$patch_filename" | cut -c5-7`
   patch_ostype=`echo "$patch_filename" | cut -c8-10`
   patch_contype=`echo "$patch_filename" | cut -c11-13`

   export patch_version patch_revision patch_category patch_ostype patch_contype

   ###Verify that is the correct ostype 
   ostype=`"$vnmrsystem"/bin/vnmr_uname`
   case $ostype in
                      Linux )  ostype="LNX"
                               ;;
                    SOLARIS )  ostype="SOL"
                               ;;
                     Darwin )  ostype="MAC"
                               ;;
                    Interix )  ostype="WIN"
                               ;;
                      SunOS )  ostype="SUN"
                               ;;
                          * )  #ibm is ok with AIX
                               ;;
   esac

   if [ x$patch_ostype != "xANY" ]
   then
      if [ x$patch_ostype != x$ostype ]
      then
          echo ""
          echo "Wrong host's OS software for \"$patchfile\" "
          echo ""
          return 1
      fi
   fi

   if [ -r "$vnmrsystem"/vnmrrev ]
   then
        vnmr_rev=`grep VERSION "$vnmrsystem"/vnmrrev`
        console_name=`sed -n '3,3p' "$vnmrsystem"/vnmrrev`
   else 
        if [ -x /usr/xpg4/bin/tail ]; then
          tail="/usr/xpg4/bin/tail"
        else
          tail="tail"
        fi
        ttt=`grep -n "Console" "$vnmrsystem"/conpar | cut -c1-2`
        console_at=`expr $ttt + 1`
        console_name=`$tail -n +$console_at "$vnmrsystem"/conpar | head -n 1 | awk 'BEGIN { FS = "\"" }{ print $2 }'`

        #because MercuryVX has the same Console value in conpar as Mercury.
	# for MERCURYplus there always is a vnmrrev
        if [ \( x$console_name = "xmercury" \) -a \( -s "$vnmrsystem"/acqbin/Expproc \) ]
        then
            console_name="mercvx"
        fi
        vnmr_rev=`strings "$vnmrsystem"/bin/Vnmr | grep "VERSION"`
   fi

   if [ x$is_beta != "xb" ]
   then
      #checking for the first word of the string
      vrev=`echo $vnmr_rev | awk ' {print $1}'`
      if [ x$vrev != x"VERSION" ]
      then
	  vnmr_rev=`echo $vnmr_rev | awk '{ $1 = "" ; print }'`
      else
          vnmr_rev=" $vnmr_rev"
      fi

      ###the space before VERSION and the above $vnmr_rev are there for a reason
      if [ x"$vnmr_rev" != x" VERSION $patch_version REVISION $patch_revision" ]
      then
         echo ""
         echo "$patchfile :   not correct revision for this VNMR system"
         echo ""
         return 1
      fi
   fi

   case $console_name in
            "g2000" ) p_console_type="gem"
                      ;;
           "mercvx" ) p_console_type="mvx"
                      ;;
           "mercplus" ) p_console_type="mpl"
                      ;;
                  * ) p_console_type=`echo $console_name | cut -c1-3`
                      ;;
   esac

   export p_console_type

   ### "all" is good for all console
   if [ x$patch_contype != "xall" ]
   then
       ###Verify that is the correct console type 
       if [ "x$p_console_type" != "x$patch_contype" ]
       then
           echo ""
           echo "Patch \"$patchfile\" is not suitable for this console"
           echo ""
           return 1
       fi
   fi


   return 0
}


#-----------------------------------------------------------
# do_jumbo_patch <.Z filefullname> <patch_temp_dir> <patch_adm_dir>
#-----------------------------------------------------------
do_jumbo_patch () {

   jumbo_name=`basename "$patchfile"`

   jumbo_id=`echo "$jumbo_name" | sed  's/.tar.Z//'`

   j_patch_temp_dir=$2/$jumbo_id
   j_patch_saved_dir=$3/$jumbo_id
   j_patch_saved_tmp_dir=$3/tmp$jumbo_id

   if [ ! -d "$j_patch_temp_dir" ]
   then
     mkdir "$j_patch_temp_dir"
   fi

   if [ ! -d "$j_patch_saved_dir" ]
   then
       mkdir "$j_patch_saved_tmp_dir"
   else
       echo ""
       echo "This system has already been upgraded with patch $jumbo_id"
       echo ""
       return 1
   fi

   echo ""
   echo "-- Installing patch $jumbo_id -----"
   cd "$j_patch_temp_dir"

   if [ x`uname -s` = "xLinux" -o x`uname -s` = "xDarwin" ]
   then
       Zfilelist=`zcat "$patchfile" | tar xvf - 2> /dev/null`
   else
       Zfilelist=`zcat "$patchfile" | tar xvf - 2> /dev/null | awk '{print $2}' | sed 's/,/ /'`
   fi

   for File in "$Zfilelist"
   do
       do_small_patch "$j_patch_temp_dir"/"$File" "$j_patch_temp_dir" "$j_patch_saved_tmp_dir"
       if [ $? -ne 0 ]
       then
           rm -rf "$j_patch_saved_tmp_dir"
           return 1
       fi
   done
  
   mv "$j_patch_saved_tmp_dir" "$j_patch_saved_dir"

   return 0
}


#-----------------------------------------------------------
# do_small_patch <.Z filefullname> <patch_temp_dir> <patch_adm_dir>
#                calls  p_install
#-----------------------------------------------------------
do_small_patch () {

   patchfile=$1
   do_lot_of_checks "$patchfile"
   if [ $? -ne 0 ]
   then 
       return 1
   fi
   
   patch_process_dir=$2/$patch_id
   patch_saved_dir=$3/$patch_id             #ex: /vnmr/adm/patch/5.3BacqSOLino101
   patch_saved_tmp_dir=$3/tmp$patch_id


   # This is where this the patch record kept
   if [ ! -d "$patch_saved_dir" ]
   then
       mkdir "$patch_saved_tmp_dir"
   else
       echo ""
       echo "This system has already been upgraded with patch $patch_id"
       echo ""
       return 1
   fi
 
   if [ ! -d "$patch_process_dir" ]
   then
        mkdir "$patch_process_dir"
   fi

   echo "      Installing patch $patch_id -----"

   cd "$patch_process_dir"

   #Bread and butter

   if [ x`uname -s` = "xLinux" -o x`uname -s` = "xInterix" ]
   then
       Filelist=`zcat "$patchfile" | tar xvf - 2> /dev/null | awk '! /\/$/ &&  $1 !~ /^p_/ {print $0}'`
   elif [ x`uname -s` = "xDarwin" ]
   then
       Filelist=`zcat "$patchfile" | tar tf - 2> /dev/null | awk '! /\/$/ &&  $1 !~ /^p_/ {print $0}'`
       zcat "$patchfile" | tar xvf -  2> /dev/null
   else
       Filelist=`zcat "$patchfile" | tar xvf - 2> /dev/null | grep -v ' 0 bytes' | awk '$2 !~ /^p_/ {print $2}' | sed 's/,/ /'`
   fi

   if [ -f "$patch_process_dir"/p_install ]
   then
       "$patch_process_dir"/p_install "$Filelist" "$patch_process_dir" "$patch_saved_tmp_dir"
   else
       rm -rf "$patch_saved_tmp_dir" "$patch_process_dir"  #clean up
       echo ""
       echo " Problem installing patch $patch_id , No p_install file"
       echo ""
       return 1
   fi

   #successfuly installed
   mv "$patch_saved_tmp_dir" "$patch_saved_dir"
   
   if [ x$patch_size = "xjumbo" ]
   then  
       echo "   DONE"
   else
       echo ""
   fi

   case "$patch_category" in
 
        "acb" )  su_acqproc=1
                 acqproc_id=$patch_id
                 ;;
        "acq" )  reboot=1
                 reboot_id=$patch_id
                 ;;
        "psg" )  fix_psg=1
                 ;;
        "all" )  #Do not know whats to do yet
                 ;;
            * )  #Do nothing I guess
                 ;;
   esac
   return 0
}


#-----------------------------------------------------------
# load_kernel
#-----------------------------------------------------------
load_kernel () {

   case $patch_contype in
       "ino" ) ls /tftpboot/vxBoot | grep sym > /dev/null
               if [ $? != 0 ] 
               then    
                   #big kernel
                   cp "$vnmrsystem"/acq/vxBoot.big/* /tftpboot/vxBoot
               else 
                   #small kernel
                   cp "$vnmrsystem"/acq/vxBoot.small/* /tftpboot/vxBoot
               fi 
               ;;
       "mer" ) cp "$vnmrsystem"/acq/apmon /tftpboot  #need to be root #uniqe to mercury
               echo "At the Vnmr command line type   load='y' su   to save the current shim set"
               ;;
           * )
               ;;
   esac
   return 0
}


#-----------------------------------------------------------
# clean_up
#-----------------------------------------------------------
clean_up () {

   #Just go somewhere else, for removing $patch_temp_dir
   cd "$vnmrsystem"
   rm -rf "$patch_temp_dir"
}

 
#-----------------------------------------------------------
#
#                Main
#-----------------------------------------------------------

patch_temp_dir="$vnmrsystem"/tmp/patch
patch_adm_dir="$vnmrsystem"/adm/patch

common_env

if [ $# -ne 1 ]
then
    echo ""
    echo "Usage:   $0 \"patched filename\" "
    exit 1
fi

get_vnmrsystem

# Verify that the current user is a privileged user
case `uname -r` in
    4*) user=`whoami`
        ;;
    5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
        ;;
esac

if touch "$vnmrsystem"/testpermission 2>/dev/null
then
     rm -f "$vnmrsystem"/testpermission
     user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
     if [ x$user = "xroot" ]
     then
          echo "Patch, can not be loaded by root, Exit patchinstall."
          exit 1
     fi
else
     echo "$user do not have permission to write onto $vnmrsystem directory"
     echo ""
     echo "Please, login as a privileged user , then rerun $0"
     echo ""
     exit 1
fi

if [ x`dirname $1` = "x." ]
then
   patchfile=`pwd`/$1
else
   patchfile=$1
fi

do_lot_of_checks "$patchfile"

if [ $? -ne 0 ]
then
    exit 1
fi

if [ ! -d "$patch_temp_dir" ]  # /vnmr/tmp/patch, removed at the end
then 
     mkdir "$patch_temp_dir"
fi

if [ ! -d "$vnmrsystem"/adm ]   # Some systems (SGI) do not have an adm directory.
then
    mkdir "$vnmrsystem"/adm
fi

if [ ! -d "$patch_adm_dir" ]   # /vnmr/adm/patch
then
    mkdir "$patch_adm_dir"
fi


###The patch name with "gen" (generic) in it is the Jumbo patch
if [ x$patch_category = "xgen" ]
then 
    patch_size="jumbo"
    msg_id=$patch_id
    do_jumbo_patch "$patchfile" "$patch_temp_dir" "$patch_adm_dir"
    if [ $? -ne 0 ]
    then
        clean_up
        exit 1
    fi
else
    #if [ x$patch_category = "xacq" ]
    #then
    #     loadk=1
    #fi

    patch_size="small"
    msg_id=$patch_id
    echo ""
    do_small_patch "$patchfile" "$patch_temp_dir" "$patch_adm_dir"

    if [ $? -ne 0 ]
    then
       clean_up
       exit 1
    fi
fi

clean_up

if [ $fix_psg ]
then 
    "$vnmrsystem"/bin/fixpsg
fi

if [ $reboot ]
then 
    #load_kernel
    echo ""
    echo "Please, login as root and run $vnmrsystem/bin/setacq before rebooting the console"
    echo ""
    echo "You must reboot the NMR console"
    echo "for the patch $reboot_id to take effect"
fi

if [ $su_acqproc ]
then 
    echo ""
    echo "Please run \"su acqproc\" from a unix shell for the patch $acqproc_id to take effect"
fi

echo ""
echo "-- Patch $msg_id installation complete -----"
echo ""
