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

# set -x
# Writes the /etc/sudoers.d/99_openvnmrj

if [[ $# -lt 2 ]] ; then 
    nmr_adm=$(ls -l /vnmr/vnmrrev | awk '{print $3}')
    nmr_group=$(ls -l /vnmr/vnmrrev | awk '{print $4}')
else
   nmr_adm=$1
   nmr_group=$2
fi
host_name=$(uname -n)
abindir="/vnmr/acqbin"
sbindir="/vnmr/p11/sbin"
vbindir="/vnmr/bin"
file=/etc/sudoers.d/16openvnmrj
     
sudoFile() {
   cat <<EOF | sed s:USER:${nmr_adm}: | \
               sed s:GROUP:${nmr_group}: | \
               sed s:HOST:${host_name}: | \
               sed s:ABIN:"${abindir}": | \
               sed s:SBIN:"${sbindir}": | \
               sed s:VBIN:"${vbindir}": > ${file}
# SUDO file for OpenVnmrJ
#
USER HOST = NOPASSWD: \\
   VBIN/dtsharcntrl,\\
   VBIN/makeuser,\\
   VBIN/jtestgroup,\\
   VBIN/jtestuser,\\
   VBIN/vcmdr,\\
   VBIN/vcmdm,\\
   ABIN/startStopProcs,\\
   /usr/bin/getent,\\
   /usr/bin/passwd,\\
   /usr/sbin/useradd,\\
   /bin/mkdir,\\
   /bin/chown,\\
   /usr/sbin/userdel,\\
   SBIN/auconvert,\\
   SBIN/auevent,\\
   SBIN/auinit,\\
   SBIN/aupurge,\\
   SBIN/aureduce,\\
   SBIN/auredt,\\
   SBIN/aupw,\\
   SBIN/killau,\\
   SBIN/killch,\\
   SBIN/chchsums,\\
   SBIN/makeP11checksums,\\
   SBIN/scanlog

%GROUP HOST = NOPASSWD: \\
   VBIN/probe_mount,\\
   VBIN/probe_unmount

EOF
}

#Open Source Sudo package
# check for Redhat's then and Debian (Ubuntu) sudo locations
# reversed check, RHEL 6.X has both /usr/local/bin and /usr/bin sudo, however the /usr/local/bin version is 32-bit
# and the 32-bit libpam.so lib is not installed. With the search reversed we get the proper RHEL 6.X 64-bit sudo version. GMB
if [ -x /usr/bin/sudo ]; then
      SUDO="/usr/bin/sudo"
elif [ -x /usr/local/bin/sudo ]; then
      SUDO="/usr/local/bin/sudo"
else
      SUDO=""
fi

#  Need to be root to configure sudo
userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [[ $userId != "uid=0(root)" ]]; then
   echo
   echo "To run $0 you will need to be the system's root user,"
   echo "or type cntrl-C to exit."
   echo
   s=1
   t=3
   while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
      echo "Please enter this system's root user password"
      echo
      if [ -f /etc/debian_version ]; then
         sudo $0 $* ;
      else
         su root -c "$0 $*";
      fi
      s=$?
      t=$((t-1))
      echo " "
   done
   if [[ $t = 0 ]]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo
   fi
   exit 0
fi

#Check is /etc/sudoers has been previously modified 
if [[ ! -e $file ]] ; then
   sufile=/etc/sudoers
   line=$(grep varian $sufile)
   if [[ ! -z $line ]] ; then
      cat $sufile | grep -v varian > ${sufile}.tmp
      mv -f ${sufile}.tmp $sufile
      chown root:root $sufile
      chmod 0440 $sufile
   fi
fi
sudoFile
chown root:root $file
chmod 0440 $file
if [[ $# -lt 2 ]] ; then 
   echo "SUDO setup complete"
   echo " "
fi
