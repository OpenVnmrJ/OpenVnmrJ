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

#----------------------------------------------------------
#  patchinstall "path-name of patch file"
#
#  The patch name is encoded with the VnmrJ version, OS, Console
#  and patch number.
#  These attributes are separated by underscores in the path name.
#  For example  3.2_LNX_VNMRS_101.ptc
#  and          3.2A_LNX_vnmrsddr2_102.ptc
#  The names are case-insensitive.
#  Patches have a .ptc suffix. They are actually zip files, but files
#  with a .zip suffix are often blocked by email systems.
#
#  The VnmrJ software versions are of the form VERSION x.y REVISION z
#  and are in the first line of the /vnmr/vnmrrev file.
#  The first field of the patch name can match the VERSION or the VERSION
#  and REVISION. The special key ANY will match any VnmrJ version.
#
#  The second field of the patch name signifies the computer operating system.
#  Supported OS values are LNX, MAC, and WIN. The special key ANY will match
#  any OS.
#
#  The third field of the patch name signifies the spectrometer console.
#  Supported values are VNMRS, VNMRSDD2, MR400, MR400DD2, Inova, MercuryVX,
#  and MercuryPlus. This third field can also be set to keywords that represent
#  groups of spectrometer consoles. The keyword Mercury applies to both MercuryVX
#  and MercuryPlus. The keyowrd MMI applies to MercuryVX, MercuryPlus, and Inova.
#  The keyword DDR applies to VNMRS, VNMRSDD2, MR400, MR400DD2, and ProPulse.
#  The special key ANY will match any console. The console value is taken from
#  the third line of the /vnmr/vnmrrev file.
#
#  The fourth and final field is a patch version. Generally, three ranges of
#  patch versions are made.
#  The 100 series patchs are the main patch. These patches are cumulative.
#  Each subsequent patch in the series contains all the contents of the previous
#  patches. So one can install a 103 patch, for example, without first installing
#  the 101 and 102 patches.
#  The 300 series patchs are "hot-fixes" to solve an urgent problem. The 300
#  patchs are generally single purpose patches. They are not cumulative.
#  The 500 series patches are also single purpose patches, often to support
#  new PC or OS versions.
#
#  Patches whose name begins with custom_ will be installed. This name is used by
#  the patchmake tool for on-site patch building.
#
#  A patch .ptc file contains the following files:
#  patch.zip contains the files that will be installed into the $vnmrsystem directory
#  checksum  contains the checksum of the patch.zip file. Used for validation.
#  The patch.zip file has an optional Readme file describing the content of the patch.
#  The patch.zip file has an optional p_install script to do additional tasks by the patch.
#  The patch.zip file has an optional p_required file to test for required patches
#----------------------------------------------------------

# set -x
#-----------------------------------------------------------

#-----------------------------------------------------------
#  Get value of vnmrsystem
#  If not defined, ask for its value
#  Use /vnmr as the default
#  make sure directory exists
#-----------------------------------------------------------
get_vnmrsystem() {

    if [ "x$vnmrsystem" = "x" ]
    then
       vnmrsystem=/vnmr
    fi
 
    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, exit patchinstall"
        exit
    fi
    export vnmrsystem
}

# Check if patch can be installed on this system
checkPatchName () {

   ok=1

   ###Does this patch file exist and is it readable
   if [ ! -f "$patchfile" ]
   then
       echo ""
       echo "$0:   $patchfile not found!"
       echo ""
       ok=0
   elif [ ! -r "$patchfile" ]
   then
       echo ""
       echo "$0:   $patchfile is not readable"
       echo ""
       ok=0
   elif [ ! -s "$patchfile" ]
   then
       echo ""
       echo "$0:   $patchfile is empty"
       echo ""
       ok=0
   fi

   patch_filename=`basename "$patchfile" .ptc`
   ###Does this file have the right format
   if [ x$patch_filename.ptc != x`basename "$patchfile"` ]
   then
       echo ""
       echo " $patchfile is not a VnmrJ (*.ptc) patch name"
       echo ""
       ok=0
   fi

#  Patch name information
   patch_vers=`echo $patch_filename | awk 'BEGIN { FS = "_" } { print tolower($1) }'`
#  echo "VnmrJ Version: $patch_vers"
   patch_os=`echo $patch_filename | awk 'BEGIN { FS = "_" } { print tolower($2) }'`
#  echo "OS Version: $patch_os"
   patch_cons=`echo $patch_filename | awk 'BEGIN { FS = "_" } { print tolower($3) }'`
#  echo "Console: $patch_cons"
   patch_index=`echo $patch_filename | awk 'BEGIN { FS = "_" } { print tolower($4) }'`
#  echo "Patch index: $patch_index"

#  Install any patch named custom
   if [ x$patch_vers == "xcustom" ]
   then
      patch_index=0
      export patch_index
      if [ $ok -eq 1 ]
      then
          return 0
      else
          return 1
      fi
   else
      if [[ $patch_index = *[!0-9]* ]]; then
         patch_index=0
      fi
   fi
   
#  vnmrrev file information
   vnmr_rev=`grep VERSION "$vnmrsystem"/vnmrrev`
   vnmr_rev_1=`echo $vnmr_rev | awk '{ print tolower($3) }'`
   vnmr_rev_2=`echo $vnmr_rev | awk '{ print tolower($3)tolower($5) }'`
#  echo "Installed VnmrJ: $vnmr_rev"
#  echo "Installed VnmrJ VERSION:  $vnmr_rev_1"
#  echo "Installed VnmrJ REVISION: $vnmr_rev_2"
   console_name=`sed -n '3,3p' "$vnmrsystem"/vnmrrev`
   console_type=`echo $console_name | cut -c1-2`
   spectrometer=$console_name
   case $console_type in
        me )    spectrometer="MERCURYplus/-Vx"
                ;;
        mv )    spectrometer="MERCURY-Vx"
                ;;
        mp )    spectrometer="MERCURYplus"
                ;;
        in )    spectrometer="UNITY INOVA"
                ;;
        vn )    spectrometer="VNMRS"
                ;;
        mr )    spectrometer="MR-400"
                ;;
        mv )    spectrometer="MERCURY-Vx"
                ;;
        pr )    spectrometer="ProPulse"
                ;;
        * )
                ;;
   esac
   if [ `echo $console_name | grep -ic dd2` -gt 0 ]; then
       spectrometer="$spectrometer DD2"
   fi
   echo "Installed for $spectrometer spectrometers"


   if [ x$patch_vers != "xany" ]
   then
      if [ x$patch_vers != x$vnmr_rev_1 -a x$patch_vers != x$vnmr_rev_2 ]
      then
            echo ""
            echo "$patchfile  is not for the installed VnmrJ version"
            echo ""
            ok=0
      fi
   fi

   ###Verify that is the correct ostype 
   if [ x$patch_os != "xany" ]
   then
      ostype=`uname -s`
      osname=$ostype
      case $ostype in
          Linux )    ostype="lnx"
                     ;;
          Darwin )   ostype="mac"
                     osname="OS X"
                     ;;
          Interix )  ostype="win"
                     osname="MS Windows"
                     ;;
          * ) 
                     ;;
      esac

      if [ x$patch_os != x$ostype ]
      then
          echo ""
          echo "$patchfile  cannot be installed under $osname"
          echo ""
          ok=0
      fi
   fi

   ###Verify that is the correct console
   if [ x$patch_cons != "xany" ]
   then
#     Only need to match two characters.
#     mv for MercuryVx
#     mp for MecruryPlus
#     me for MecruryPlus and MercuryVx
#     in for Inova
#     vn for VNMRS and VNMRSDD2
#     mr for MR400 and MR400DD2
#     mm for MercuryPlus and MercuryVx and Inova
#     dd for VNMRS and VNMRSDD2 and MR400 and MR400DD2
#     pr for ProPulse
#     patch_type is the first two characters from the console section of the patch name
#     console_type is the first two characters from the console name in /vnmr/vnmrrev
      patch_type=`echo $patch_cons | cut -c1-2`
      ok2=0
      if [ x$patch_type = x$console_type ]
      then
          ok2=1
      fi
      if [ $ok2 -eq 0 ]
      then
         if [ x$patch_type = "xmv" -o x$patch_type = "xmp" ]
         then
            if [ x$console_type = "xme" ]
            then
               ok2=1
            fi
         fi
      fi
      if [ $ok2 -eq 0 ]
      then
#        Check for MMI group
         if [ x$patch_type = "xmm" ]
         then
            if [ x$console_type = "xme" -o x$console_type = "xin" ]
            then
               ok2=1
            fi
         fi
      fi
      if [ $ok2 -eq 0 ]
      then
#        Check for DDR group
         if [ x$patch_type = "xdd" ]
         then
            if [ x$console_type = "xvn" -o x$console_type = "xmr" -o x$console_type = "xpr" ]
            then
               ok2=1
            fi
         fi
      fi

      if [ $ok2 -eq 0 ]
      then
          echo ""
          echo "$patchfile  is not suitable for $spectrometer"
          echo ""
          ok=0
      fi
   fi


   export patch_index
   if [ $ok -eq 1 ]
   then
       return 0
   else
       return 1
   fi
}

#-----------------------------------------------------------
# validatePatch
# Make sure checksum matches and patch has not already been installed
# For 100 series patches, make sure newer 100 patch has not already
# been installed.
#-----------------------------------------------------------
validatePatch () {

   cd $patch_temp_dir
   unzip -q $patchfile
   if [ $? -ne 0 ]
   then
       echo "Patch $patch_id has invalid format"
       return 1
   fi
   if [ ! -f patch.zip ]
   then
      echo "Patch $patch_id is empty"
      return 1
   fi
   if [ -f checksum ]
   then
       os=`uname -s`
       if [ x$os = "xDarwin" ]
       then
          csum=`/sbin/md5 -q patch.zip`
       else
          csum=`md5sum patch.zip | awk '{print $1}'`
       fi
       psum=`cat checksum | awk '{print $1}'`
       if [ x$csum != x$psum ]
       then
          echo "Based on checksums, patch $patch_id has been corrupted"
          return 1
       fi
       rm checksum
   else
       echo "Patch $patch_id is missing the checksum validation file"
       return 1
   fi
   if [ -d "$patch_save_dir" ]
   then
       echo ""
       echo "This system has already been upgraded with patch $patch_id"
       echo ""
       return 1
   fi

   if [ $patch_index -ge 100 -a $patch_index -lt 200 ]
   then
      if [ -f $patch_adm_dir/.patchlist ]
      then
          list=`cat $patch_adm_dir/.patchlist | awk 'BEGIN { FS = "_" } { print tolower($4) }'`
          for file in $list
          do
             echo $file | grep 1[0-9][0-9] >& /dev/null
             if [ $? -eq 0 ]
             then
                 if [ $file -lt 200 -a $file -gt $patch_index ]
                 then
                    echo "Newer 100 series patch already installed"
                    return 1
                 fi
             fi
          done
      fi
   fi
   return 0
}



#-----------------------------------------------------------
# installPatch Installs the files.
#              calls  p_install
#-----------------------------------------------------------
installPatch () {

   # This is where this the patch record is kept
   if [ -d "$patch_save_dir" ]
   then
       echo ""
       echo "This system has already been upgraded with patch $patch_id"
       echo ""
       return 1
   fi
 

   cd "$patch_temp_dir"

   FileList=`unzip -Z -1 patch.zip`
   unzip -q patch.zip

   if [ -f p_required ]
   then
       chmod +x "$patch_temp_dir"/p_required
       source "$patch_temp_dir"/p_required
       if [ -f "$patch_temp_dir"/patchAbort ]
       then
          return 1
       fi
   fi

   echo "      Installing patch $patch_id -----"
   tmp_save=$patch_adm_dir/tmp$patch_id
   mkdir "$tmp_save"

   for File in $FileList
   do
      if [ x$File = "xReadme" ]
      then
         mv $File $tmp_save/$File
         continue
      fi
      if [ x$File = "xp_install" ]
      then
         continue
      fi
      if [ x$File = "xp_required" ]
      then
         mv "$patch_temp_dir"/p_required "$tmp_save"/p_required
         chmod -x "$tmp_save"/p_required
         continue
      fi
      # If 'File' is a directory and it already exists in
      # the VnmrJ system directory, no action is required.
      if [ -d $vnmrsystem/$File ]
      then
         continue
      fi
      # If 'File' is a directory and it does not already exist in
      # the VnmrJ system directory, update p_uninstall to remove it
      if [ -d $patch_temp_dir/$File ]
      then
         printf "rm -rf %s\n" $vnmrsystem/$File >> $tmp_save/p_uninstall
         continue
      fi

      check=`echo $File | awk 'BEGIN { FS = "/" } { print $1 }'`
      case "x$check" in

        "xacq" )  reboot=yes
                 ;;
        "xacqbin" )  su_acqproc=yes
                 ;;
        "xpsg" )  fix_psg=yes
                 ;;
            * )  #Do nothing
                 ;;
      esac

      if [ -f $vnmrsystem/$File ]
      then
         dir=`dirname $File`
         if [ ! -d $tmp_save/$dir ]
         then
            mkdir -p $tmp_save/$dir
         fi
         #save the original
         mv $vnmrsystem/$File $tmp_save/$dir
         #This is the bug fixed file
         mv $patch_temp_dir/$File $vnmrsystem/$File
      else

         newdir=`dirname $vnmrsystem/$File`
         if [ ! -d $newdir ]
         then
            mkdir -p $newdir
         fi

         #This is the bug fixed file
         mv $patch_temp_dir/$File $vnmrsystem/$File
         printf "rm -f %s\n" $vnmrsystem/$File >> $tmp_save/p_uninstall
      fi
   done


   if [ -f "$patch_temp_dir"/p_install ]
   then
       chmod +x "$patch_temp_dir"/p_install
       source "$patch_temp_dir"/p_install
       mv "$patch_temp_dir"/p_install "$tmp_save"/p_install
       chmod -x "$tmp_save"/p_install
   fi

   #successfuly installed
   mv "$tmp_save" "$patch_save_dir"
   printf  "$patch_id\n" >> $patch_adm_dir/.patchlist
   
   return 0
}

# Update checksums if it is an SE system
fixSE () {

    if [ -f /usr/varian/sbin/makeP11checksums ]
    then
       nmr_adm=`ls -l $vnmrsystem/vnmrrev | awk '{print $3}'`
       nmr_group=`ls -l $vnmrsystem/vnmrrev | awk '{print $4}'`
       echo "Updating SE checksums"
#Open Source Sudo package
# check for Redhat's then and Debian (Ubuntu) sudo locations
# reversed check, RHEL 6.X has both /usr/local/bin and /usr/bin sudo,
# however the /usr/local/bin version is 32-bit and the 32-bit
# libpam.so lib is not installed. With the search reversed we
# get the proper RHEL 6.X 64-bit sudo version. GMB
       if [ -x /usr/bin/sudo ]; then
             SUDO="/usr/bin/sudo"
       elif [ -x /usr/local/bin/sudo ]; then
             SUDO="/usr/local/bin/sudo"
       else
             SUDO=""
       fi
       $SUDO /usr/varian/sbin/makeP11checksums $vnmrsystem $nmr_adm $nmr_group
    fi
    cd $vnmrsystem
    dir=`/bin/pwd`
    parent=`dirname $dir`
   # Check the part11Config file for a part11Dir entry, if found, use it
   # else default to $parent/vnmrp11 and write this to the config file
   p11dir=`grep part11Dir: /vnmr/p11/part11Config | awk 'BEGIN {FS=":"} {print $2}'`
   if [ x$p11dir = "x" ]
   then
      # If no path given in config file, default to $parent/vnmrp11
      p11dir="$parent/vnmrp11"
      echo "Setting part11Dir to default ($p11dir)"
      /bin/ed -s /vnmr/p11/part11Config > /dev/null << THEEND
/part11Dir:/
d
i
part11Dir:$p11dir
.
w
q
THEEND
    fi
   # Check the part11Config file for a auditDir entry, if found, use it
   # else default to $parent/vnmrp11/auditTrails and write this to the config file
   auditdir=`grep auditDir: /vnmr/p11/part11Config | awk 'BEGIN {FS=":"} {print $2}'`
   if [ x$auditdir = "x" ]
   then
      # If no path given in config file, default to $parent/vnmrp11/auditTrails
      auditdir="$parent/vnmrp11/auditTrails"
      echo "Setting auditDir to default ($auditdir)"
      /bin/ed -s /vnmr/p11/part11Config  > /dev/null << THEEND
/auditDir:/
d
i
auditDir:$auditdir
.
w
q
THEEND
    fi

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

if [ $# -ne 1 ]
then
    echo ""
    echo "Usage:   $0 \"patch filename\" "
    exit 1
fi
user=`id -un`
if [ x$user = "xroot" ]
then
    echo "Patch can not be loaded by root. Exit patchinstall."
    exit 1
fi


###Is this an old format patch
teststring=`basename "$1" | cut -c17-`
if [ x$teststring = "x.tar.Z" ]
then
   echo ""
   echo "Using version 1 of patchinstall. Patchuninstall is not available"
   patchinstall_ver1 $1
   exit 1
fi

get_vnmrsystem

if touch "$vnmrsystem"/testpermission 2>/dev/null
then
     rm -f "$vnmrsystem"/testpermission
else
     echo "$user does not have permission to write onto $vnmrsystem directory"
     echo ""
     echo "Please, login as a privileged user , then rerun $0"
     echo ""
     exit 1
fi

firstchar=`echo $1 | cut -c1-1`
if [ x$firstchar = "x/" ]  #absolute path
then
   patchfile=$1
else
   patchfile=`pwd`/$1
fi

patch_id=`basename $patchfile .ptc`

checkPatchName "$patchfile"
if [ $? -ne 0 ]
then
    exit 1
fi

patch_temp_dir="$vnmrsystem"/tmp/patch
patch_adm_dir="$vnmrsystem"/adm/patch
patch_save_dir=$patch_adm_dir/$patch_id

rm -rf "$patch_temp_dir"
mkdir "$patch_temp_dir"

validatePatch
if [ $? -ne 0 ]
then
   clean_up
   exit 1
fi

if [ ! -d "$patch_adm_dir" ]
then
    mkdir -p "$patch_adm_dir"
fi

fix_psg="no"
reboot="no"
su_acqproc="no"

installPatch
if [ $? -ne 0 ]
then
   clean_up
   exit 1
fi

clean_up

if [ -f $vnmrsystem/p11/part11Config ]
then
   fixSE
fi

if [ x$fix_psg = "xyes" ]
then 
    printf  "fix_psg=yes\n" >> $patch_save_dir/p_uninstall
    "$vnmrsystem"/bin/fixpsg
fi

if [ x$reboot = "xyes" ]
then 
    printf  "reboot=yes\n" >> $patch_save_dir/p_uninstall
    echo ""
    echo "Console software has been updated."
    echo "Login as root and run $vnmrsystem/bin/setacq"
    echo "for the patch to take effect"
    echo ""
elif [ x$su_acqproc = "xyes" ]
then 
    printf  "su_acqproc=yes\n" >> $patch_save_dir/p_uninstall
    echo ""
    echo "Console communication software has been updated."
    echo "Restart the communication software (su acqproc)"
    echo "for the patch to take effect"
    echo ""
fi

echo ""
echo "-- Patch $patch_id installation complete -----"
echo ""
