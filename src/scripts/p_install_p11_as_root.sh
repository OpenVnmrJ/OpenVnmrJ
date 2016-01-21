#! /bin/sh
# '1991-2004 '
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

# This is the part of p_install for part11 that required root access.
# This is separated out so that it can be run as a single command
# from p_install as root


nmr_adm=`ls -l $vnmrsystem/vnmrrev | awk '{print $3}'`
nmr_group=`ls -l $vnmrsystem/vnmrrev | awk '{print $4}'`
rootuser="root"
dest_dir=$vnmrsystem

sbindir="/usr/varian/sbin"
domainname=`/bin/domainname`

( cd ${dest_dir}/bin
   cp chchsums "$sbindir"
   cp /vnmr/bin/vnmrMD5 /tmp
   mv auconvert auevent auinit aupurge aureduce auredt aupw \
      makeP11checksums vnmrMD5 killau killch scanlog "$sbindir"

   mv -f ${dest_dir}/bin/S99scanlog "$sbindir"
   mv -f ${dest_dir}/bin/setupscanlog "$sbindir"

   "$sbindir"/setupscanlog

   #To have the scanlog started at system boot up
   cp -p ${sbindir}/S99scanlog /etc/init.d
     ( cd /etc/rc1.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
     ( cd /etc/rc3.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
     ( cd /etc/rc5.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
)
#Need to reboot the computer for this to work


"$sbindir"/makeP11checksums $dest_dir $nmr_adm $nmr_group

#moved over from the patch
chmod 644 "$dest_dir"/adm/users/profiles/accPolicy

p11config="$dest_dir"/p11/part11Config
parent="/home"

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

    # Make the directory if necessary
    if [ ! -d "$p11dir" ]
    then
        mkdir -p "$p11dir"
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

    # Make the directory if necessary
if [ ! -d "$auditdir" ]
then
   mkdir -p "$auditdir"
fi

chown ${nmr_adm}:${nmr_group} "$auditdir" "$p11dir"
chmod 755 "$auditdir" "$p11dir"

cd "$sbindir"
chown $rootuser:$rootuser auconvert aureduce auevent auinit aupurge aupw \
	     auredt makeP11checksums vnmrMD5 killau killch scanlog chchsums
chmod 700 auconvert aureduce auevent auinit aupurge aupw auredt \
	  makeP11checksums vnmrMD5 killau killch scanlog chchsums

# Set sticky bit for files in /vnmr/p11/bin
chmod ug+sw "$dest_dir/p11/bin/safecp"
chmod ug+sw "$dest_dir/p11/bin/writeAaudit"
chmod ug+sw "$dest_dir/p11/bin/writeTrash"

# fix the /etc/sudoers file to contain p11 stuff
# 'gawk' and 'rht' args assume redhat Linux for this p_install to run on
# the 'yes' is to enable p11
"$dest_dir"/bin/sudoins gawk yes rht $nmr_adm
