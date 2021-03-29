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
# With MacOs Catalina (10.15), the / partition is read-only
# The /etc/sythetic.conf file provides a mechanism to have
# the symbolic link /vnmr point to the OpenVnmrJ files
#
# set -x

ostype=$(uname -s)

if [[ $ostype != "Darwin" ]]; then
   echo "$0: only required for MacOS systems"
   echo ""
   exit 1
fi

osVer=$(sw_vers -productVersion | tr "." " " | awk '{print $1}')
osSubVer=$(sw_vers -productVersion | tr "." " " | awk '{print $2}')

if ! [[ $osVer -gt 10 ]] && ! [[ $osSubVer -ge 15 ]]; then
   echo "$0: only required for MacOS Catalina (10.15) systems or newer"
   exit 1
fi

#-------------------------------------------------
#  MAIN Main main
#-------------------------------------------------


# get base_dir first so we have it at all times
#
firstchar=$(echo $0 | cut -c1-1)
if [ x$firstchar = "x/" ]  #absolute path
then
   base_dir=$(dirname $0)
else
   if [ x$firstchar = "x." ]  #relative path
   then
       if [ x$(dirname $0) = "x." ]
       then
           base_dir=$(pwd)
       else
           base_dir=$(pwd)/$(dirname $0 | sed 's/.\///')
       fi
   else
      base_dir=$(pwd)/$(dirname $0)
   fi
fi
script=$(basename $0)

#
# Login the user as a root user
# Use the "su" command to ask for password and run the installer
#

userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [ $userId != "uid=0(root)" ]; then
  echo

  s=1
  t=3
  while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
     echo "If requested, please enter the admin (sudo) password"
     sudo $base_dir/$script
     s=$?
     t=$((t-1))
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the process again"
      echo ""
  fi
  exit
fi

#
# User is now root.
#

ovjApps=$(ls -d /Applications/OpenVnmrJ*app)
revmajor=2
revminor=0
for app in $ovjApps
do
   ver=$(basename $app .app | tr "_" " " | awk '{print $2}')
   major=$(echo $ver | tr "." " " | awk '{print $1}')
   minor=$(echo $ver | tr "." " " | awk '{print $2}')
   if [[ $major -gt $revmajor ]]; then
      revmajor=$major
      revminor=$minor
   elif [[ $minor -gt $revminor ]]; then
      revminor=$minor
   fi
done

if [[ -f /etc/synthetic.conf ]]; then
   grep -w vnmr /etc/synthetic.conf
   if [[ $? -eq 0 ]]; then
      grep -vw vnmr /etc/synthetic.conf > /etc/synthetic
      mv /etc/synthetic /etc/synthetic.conf
   fi
   printf "vnmr\t/Applications/OpenVnmrJ_%d.%d.app/Contents/Resources/OpenVnmrJ\n" $revmajor $revminor >> /etc/synthetic.conf  
else
   printf "vnmr\t/Applications/OpenVnmrJ_%d.%d.app/Contents/Resources/OpenVnmrJ\n" $revmajor $revminor > /etc/synthetic.conf  
fi

rm -f /System/Volumes/Data/vnmr

echo "This requires a system reboot"
echo ""
echo "Would you like to reboot now? (y/n) "
read ans
echo " "
if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
   reboot
else
   echo "OpenVnmrJ will not function until system is rebooted."
   echo " "
fi

