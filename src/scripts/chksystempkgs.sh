#! /bin/bash
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
# set -x
# check for the packages required for installation of OpenVnmrJ
status=0

setDistMajor() {
   distmajor=16
   if [[ -f /etc/lsb-release ]]; then
      . /etc/lsb-release
      distmajor=${DISTRIB_RELEASE:0:2}
   elif [[ -f /etc/os-release ]]; then
      . /etc/os-release
      distmajor=${VERSION_ID:0:2}
   elif [[ ! -z $(type -t hostnamectl) ]]; then
      ver=$(hostnamectl | grep "Operating" | cut -d " " -f 4)
      distmajor=${ver:0:2}
   fi
}

#
# rpm (RedHat and CentOS) vs dpkg (Debian)
#
if [ ! -x /usr/bin/dpkg ]; then
   echo "Checking for RHEL / CentOS packages required by OpenVnmrJ"
   if [ -f /etc/centos-release ]; then
      rel=centos-release
   else
      rel=redhat-release
   fi
 # remove all characters up to the first digit
   version=$(cat /etc/$rel | sed -E 's/[^0-9]+//')
 # remove all characters from end including first dot
   version=${version%%.*}

   packagecommonlist='make gcc gcc-c++ gdb libtool binutils automake strace autoconf glibc-devel expect tftp-server mutt ghostscript ImageMagick'
   if [[ $version -eq 7 ]] || [[ $version -ge 8 ]]
   then
#     for RHEL 7.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      javalist='openjdk'
      package71list='libgfortran motif'
      package32Bitlist='rsh libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 mesa-libGL-devel mesa-libGL mesa-libGLU'
      package32Bitlist='libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686'
      packagelist="$packagecommonlist $package71list $package32Bitlist"
      if [[ $version -eq 8 ]]; then
        packagelist="$packagelist tcsh compat-openssl10 rsh rsh-server"
      fi
   elif [ $version -eq 6 ]
   then
      javalist=''
#     for RHEL 6.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      package61list='libgfortran openmotif gnome-power-manager'
      package32Bitlist='rsh libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 mesa-libGL-devel mesa-libGL mesa-libGLU'
      packagelist="$packagecommonlist $package61list $package32Bitlist"
   else
      echo "OpenVnmrJ requires RHEL / CentOS version 6 or newer"
      javalist=''
      packagelist=''
      status=1 
   fi

else
   echo "Checking for Ubuntu / Debian packages required by OpenVnmrJ"
   setDistMajor
   packagecommonlist='tcsh make gcc gfortran expect openssh-server mutt sharutils sendmail-cf gnome-power-manager kdiff3 ghostscript imagemagick xterm'
   if [ $distmajor -ge 22 ] ; then
     packageXlist='default-jre bc libmotif-dev'
   elif [ $distmajor -gt 16 ] ; then
     packageXlist='openjdk-8-jre bc libmotif-dev'
   fi
   packagelist="$packagecommonlist $packageXlist";
fi

if [ ! -x /usr/bin/dpkg ]; then
   for xpack in $packagelist
   do
      # not installed then install dkms package rpm
      if [ "$(rpm -q $xpack | grep 'not installed' > /dev/null;echo $?)" = "0" ]
      then
        echo "OpenVnmrJ required RHEL / CentOS package \"$xpack\" not installed"
        status=1 
      fi
   done
   for xpack in $javalist
   do
      if [ "$(rpm -qa | grep $xpack  > /dev/null;echo $?)" != "0" ]; then
        echo "$xpack java package not installed"
        status=1 
      fi
   done

else
   for xpack in $packagelist
   do
      if [ "$(dpkg --get-selections $xpack 2>&1 | grep -w 'install' > /dev/null;echo $?)" != "0" ] 
      then
          echo "OpenVnmrJ required Ubuntu package \"$xpack\" not installed"
          status=1
      fi
   done
fi
exit $status
