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

   packagecommonlist='make gcc gcc-c++ gdb libtool binutils automake strace autoconf glibc-devel expect rsh-server tftp-server mutt k3b ghostscript ImageMagick'
   if [[ $version -eq 7 ]] || [[ $version -eq 8 ]]
   then
#     for RHEL 7.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      package71list='libgfortran motif java-1.8.0-openjdk'
      package32Bitlist='rsh libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 mesa-libGL-devel mesa-libGL mesa-libGLU'
      package32Bitlist='rsh libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686'
      packagelist="$packagecommonlist $package71list $package32Bitlist"
      if [[ $version -eq 8 ]]; then
        packagelist="$packagelist tcsh compat-openssl10"
      fi
   elif [ $version -eq 6 ]
   then
#     for RHEL 6.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      package61list='libgfortran openmotif gnome-power-manager'
      package32Bitlist='rsh libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 mesa-libGL-devel mesa-libGL mesa-libGLU'
      packagelist="$packagecommonlist $package61list $package32Bitlist"
   else
      packagecommonlist='compat-libf2c-34 compat-gcc-34-g77 openmotif22 gnome-power-manager'
   # for RHEL 5.X  separate since these are no longer on the 6.x Installation DVD.
      package5xlist='sharutils rarpd hwbrowser'
      packagelist="$packagecommonlist $package5xlist";
   fi

else
   echo "Checking for Ubuntu / Debian packages required by OpenVnmrJ"
   . /etc/lsb-release
   distmajor=${DISTRIB_RELEASE:0:2}
   packagecommonlist='tcsh make gcc gfortran expect openssh-server mutt sharutils sendmail-cf gnome-power-manager kdiff3 ghostscript imagemagick xterm'
   if [ $distmajor -ge 16 ] ; then
     packageXlist='openjdk-8-jre bc libmotif-dev'
   elif [ $distmajor -ge 14 ] ; then
     packageXlist='openjdk-6-jre libmotif-dev'
   elif [ $distmajor -gt 10 ] ; then
     packageXlist='openjdk-6-jre ia32-libs lesstif2-dev'
   else
     packageXlist='ia32-sun-java6-bin ia32-libs lesstif2-dev'
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
