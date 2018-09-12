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
   if [ -f /etc/centos-release ]; then
      rel=centos-release
   else
      rel=redhat-release
   fi
   version=$(rpm -q $rel | cut -d'-' -f3)
   packagecommonlist='make gcc gcc-c++ gdb libtool binutils automake strace autoconf glibc-devel expect rsh-server xinetd tftp-server kernel-headers kernel-devel libidn libXp libXmu mutt sendmail-cf k3b ghostscript ImageMagick'
   if [ $version -eq 7 ]
   then
#     for RHEL 7.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      package71list='libgfortran motif'
      package32Bitlist='rsh libXau.i686 libX11.i686 libXext-devel.i686 libXtst-devel.i686 libXi.i686 libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 libXt.i686 libXt-devel.i686 ncurses-libs.i686 ncurses-devel.i686 motif.i686 motif-devel.i686 mesa-libGL-devel mesa-libGL-devel.i686 atk.i686 gtk2.i686 mesa-libGL mesa-libGL.i686 mesa-libGLU mesa-libGLU.i686 libidn.i686 libxml2.i686 expat.i686 libXaw.i686 libXpm.i686'
      packagelist="$packagecommonlist $package71list $package32Bitlist"
   elif [ $version -eq 6 ]
   then
#     for RHEL 6.X must list 32-bit packages, since these are no longer installed with the 64-bit versions
      package61list='libgfortran openmotif22 gnome-power-manager'
      package32Bitlist='rsh libXau.i686 libX11.i686 libXext-devel.i686 libXtst-devel.i686 libXi.i686 libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 libXt.i686 libXt-devel.i686 ncurses-libs.i686 ncurses-devel.i686 openmotif.i686 openmotif-devel.i686 mesa-libGL-devel mesa-libGL-devel.i686 atk.i686 gtk2.i686 mesa-libGL mesa-libGL.i686 mesa-libGLU mesa-libGLU.i686 libidn.i686 libxml2.i686 expat.i686 libXaw.i686 libXpm.i686'
      packagelist="$packagecommonlist $package61list $package32Bitlist"
   else
      packagecommonlist='compat-libf2c-34 compat-gcc-34-g77 openmotif22 gnome-power-manager'
   # for RHEL 5.X  separate since these are no longer on the 6.x Installation DVD.
      package5xlist='sharutils rarpd hwbrowser'
      packagelist="$packagecommonlist $package5xlist";
   fi

else
   # In Ubuntu 11 & 12 the ia32-sun-java6-bin package has been dropped from the repositories due to licensing issues with Oracle
   #   12.04
   distrover=$(lsb_release -rs)
   #   12
   distmajor=${distrover:0:2}
   packagecommonlist='csh make gcc expect ethtool rarpd rsh-server tftpd openssh-server mutt sharutils sendmail-cf gnome-power-manager k3b kdiff3 ghostscript imagemagick'
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
#       libidn is contained within the ia32-libs

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
      if [ "$(dpkg --get-selections $xpack 2>&1 | grep 'install' > /dev/null;echo $?)" != "0" ] 
      then
          echo "OpenVnmrJ required Ubuntu package \"$xpack\" not installed"
          status=1
      fi
   done
fi
exit $status
