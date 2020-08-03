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
#  patchuninstall <patchName>
#
#  The patchuninstall script will uninstalled patches.
#  If it is called with a patchName and that patch was the
#  last one installed on the first, it will be uninstalled.
#
#  If patchuninstall is called without arguments, it will
#  interactively remove patches in reverse order. It will
#  stop after the first patch that is not uninstalled.
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
        printf "Please enter location of VnmrJ system directory [/vnmr]: "
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
# uninstallPatch uninstalls the files.
#              calls  p_uninstall
#-----------------------------------------------------------
uninstallPatch () {

   dir=$patch_adm_dir/$patch_id

   if [ ! -d $dir ]
   then
      echo "      Patch $patch_id already uninstalled -----"
      return 1
   fi

   echo "      Uninstalling patch $patch_id -----"

   cd "$dir"

   rm -f Readme
   rm -f p_install
   rm -f p_required

   if [ -f p_uninstall ]
   then
      chmod +x p_uninstall
      source $dir/p_uninstall
      rm p_uninstall
   fi
   zip -qry patch.zip *
   mv patch.zip $vnmrsystem
   cd $vnmrsystem
   rm -rf $dir
   unzip -q -o patch.zip
   rm patch.zip
   return 0
}

# Update checksums if it is an SE system
fixSE () {

    cmd = "/vnmr/p11/sbin/makeP11checksums"
    if [ -f $cmd ]
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
       $SUDO $cmd $vnmrsystem $nmr_adm $nmr_group
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
#
#                Main
#-----------------------------------------------------------

get_vnmrsystem

patch_adm_dir="$vnmrsystem"/adm/patch
if [ ! -f $patch_adm_dir/.patchlist ]
then
    echo "No patches to uninstall."
    exit 1
fi

user=`id -un`
if [ x$user = "xroot" ]
then
    echo "Patch can not be uninstalled by root."
    exit 1
fi

if touch "$vnmrsystem"/testpermission 2>/dev/null
then
     rm -f "$vnmrsystem"/testpermission
else
     echo "$user does not have permission to write onto $vnmrsystem directory"
     echo ""
     echo "Please, login as a privileged user , then rerun patchuninstall"
     echo ""
     exit 1
fi

fix_psg="no"
reboot="no"
su_acqproc="no"
unpatched=0
export fix_psg reboot su_acqproc

ok=2
while [ $ok != 0 ]
do
   patch_id=`tail -n 1 $patch_adm_dir/.patchlist`
   if [ x$patch_id != x ]
   then
      if [ $# -eq 1 -a x$patch_id = x$1 ]
      then
         uninstallPatch
         sed '$ d' $patch_adm_dir/.patchlist > $patch_adm_dir/.patchlist_tmp
         mv $patch_adm_dir/.patchlist_tmp $patch_adm_dir/.patchlist
         unpatched=1
         ok=0
      elif [ $# -eq 1 -a x$patch_id != x$1 ]
      then
         echo "patchuninstall with an argument can only uninstall the last patch $patch_id"
         ok=0
      else
         if [ $ok = 2 ]
         then
            echo "Current patches:"
            cat $patch_adm_dir/.patchlist
            echo ""
            ok=1
         fi
         printf "Remove patch $patch_id (y/n) [y]: "
         read yesno
         if [ x$yesno != "xn" ]
         then
            uninstallPatch
            sed '$ d' $patch_adm_dir/.patchlist > $patch_adm_dir/.patchlist_tmp
            mv $patch_adm_dir/.patchlist_tmp $patch_adm_dir/.patchlist
            unpatched=1
         else
            ok=0
         fi
      fi
   else
      ok=0
      rm -f $patch_adm_dir/.patchlist
   fi
done

if [ -f $vnmrsystem/p11/part11Config -a $unpatched = 1 ]
then
   fixSE
fi

if [ x$fix_psg = "xyes" ]
then 
    "$vnmrsystem"/bin/fixpsg
fi

if [ x$reboot = "xyes" ]
then 
    echo ""
    echo "Console software has been updated."
    echo "Login as root and run $vnmrsystem/bin/setacq"
    echo "for the patch to take effect"
    echo ""
elif [ x$su_acqproc = "xyes" ]
then 
    echo ""
    echo "Console communication software has been updated."
    echo "Restart the communication software with acqcomm"
    echo "for the patch to take effect"
    echo ""
fi

echo ""
echo "-- patchuninstall complete -----"
echo ""
