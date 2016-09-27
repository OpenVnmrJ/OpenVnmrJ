#! /bin/sh
# 'chksystempkgs.sh 2008-2009 '
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
#set -x
# check for the packages required for installation of VnmrJ on RHEL 5.X
status=0

#
# rpm (RedHat) vs dpkg (Debian)
#
if [ ! -x /usr/bin/dpkg ]; then
   packagecommonlist='make gcc gcc-c++ gdb libtool binutils automake strace autoconf glibc-devel expect rsh-server xinetd tftp-server kernel-headers kernel-devel libidn compat-libf2c-34 compat-gcc-34-g77 libXp libXmu openmotif22 tk tcl mutt sendmail-cf gnome-power-manager k3b ghostscript ImageMagick'

   # for RHEL 5.X  separate since these are no longer on the 6.x Installation DVD.
   package5xlist='sharutils rarpd hwbrowser'

   # for RHEL 6.X must list 32-bit packages, since these are no longer installed along with the 64-bit versions
   package61list='rsh libXau.i686 libX11.i686 libXext-devel.i686 libXtst-devel.i686 libXi.i686 libstdc++.i686 libstdc++-devel.i686 glibc.i686 glibc-devel.i686 libXt.i686 libXt-devel.i686 ncurses-libs.i686 ncurses-devel.i686 openmotif.i686 openmotif-devel.i686 mesa-libGL-devel mesa-libGL-devel.i686 tcl.i686 tcl-devel.i686 tk.i686 tk-devel.i686 libtiff.i686 libtiff-devel.i686 atk.i686 gtk2.i686 mesa-libGL mesa-libGL.i686 mesa-libGLU mesa-libGLU.i686 libidn.i686 libxml2.i686 expat.i686 libXaw.i686 libXpm.i686'

   if [ "$(cat /etc/issue | grep 'release 6' > /dev/null;echo $?)" == "0" ]; then
      # this list excludes sharutils and hwbrowser, sharutils will be installed from our DVD, RHEL 6.1 appears not to have hwbrowser
      packagelist="$packagecommonlist $package61list"
   else
      # add sharutils separately for RHEL 5.x , sharutils is not on the DVD like 5.X
      # add hwbrowser separately for RHEL 5.x , hwbrowser appears to be unavailable for RHEL 6.1
      # add rarpd     separately for RHEL 5.x , hwbrowser appears to be unavailable for RHEL 6.1
      packagelist="$packagecommonlist $package5xlist";
   fi

else
   # In Ubuntu 11 & 12 the ia32-sun-java6-bin package has been dropped from the repositories due to licensing issues with Oracle
   #   12.04
   distrover=`lsb_release -rs`
   #   12
   distmajor=`expr substr $distrover 1 2`
   if [ $distmajor -ge 14 ] ; then
     packagelist='openjdk-6-jre csh libmotif-dev make gcc expect ethtool tcl8.4-dev tk8.4-dev rarpd rsh-server tftpd openssh-server mutt sharutils sendmail-cf gnome-power-manager k3b kdiff3 ghostscript imagemagick'
   elif [ $distmajor -gt 10 ] ; then
     packagelist='openjdk-6-jre ia32-libs csh lesstif2-dev make gcc expect ethtool tcl8.4-dev tk8.4-dev rarpd rsh-server tftpd openssh-server mutt sharutils sendmail-cf gnome-power-manager k3b kdiff3 ghostscript imagemagick'
   else
     packagelist='ia32-sun-java6-bin ia32-libs csh lesstif2-dev make gcc expect ethtool tcl8.4-dev tk8.4-dev rarpd rsh-server tftpd openssh-server mutt sharutils sendmail-cf gnome-power-manager k3b kdiff3 ghostscript imagemagick'
   fi
fi
# Note: Ubuntu libg2c is nolonger used, must recompile code with gfortran and use libgfortran3  (DOSY dependency)
#       The package is available from HARDY repo  Updates: libg2c0 libg2c0-dev ;  
#       http://packages.ubuntu.com/search?searchon=contents&keywords=libg2c&mode=filename&suite=hardy-updates&arch=any
#       libidn is contained within the ia32-libs

for xpack in $packagelist
do
   if [ ! -x /usr/bin/dpkg ]; then
      # not installed then install dkms package rpm
      if [ "$(rpm -q $xpack | grep 'not installed' > /dev/null;echo $?)" == "0" ]
      then
        echo "VnmrJ required RHEL package \"$xpack\" NOT installed"
        status=1 
      ##else
      ##  echo "VnmrJ required RHEL package \"$xpack\" installed"
      fi

   else
      if [ "$(dpkg --get-selections $xpack 2>&1 | grep 'install' > /dev/null;echo $?)" != "0" ] 
      then
          echo "VnmrJ required Ubuntu package \"$xpack\" NOT installed"
          status=1
      ##else
      ##   echo "VnmrJ required Ubuntu package \"$xpack\" installed"
      fi
   fi
done
exit $status
