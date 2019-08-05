#! /bin/bash
#
# Copyright (C) 2018  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
# set -x

ping -W 1 -c 1 www.github.com > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "Must be connected to the internet for $0 to function"
   exit 1
fi

if [ ! -x /usr/bin/dpkg ]; then
 rel=centos-release
 if [ ! -f /etc/centos-release ]; then
   if [ ! -f /etc/redhat-release ]; then
      echo "$0 can only be used for CentOS or RedHat systems"
      exit 1
   else
      rel=redhat-release
   fi
 fi
 version=`rpm -q $rel | cut -d'-' -f3`

# Determine if some packages are already installed. If they are, do not remove them.

# perl-homedir creates a perl5 directory in every acct. Removing it fixes it so it does not do that.
 perlHomeInstalled=0
 if [ "$(rpm -q perl-homedir | grep 'not installed' > /dev/null;echo $?)" != "0" ]
 then
   perlHomeInstalled=1
 fi
 epelInstalled=0
 if [ "$(rpm -q epel-release | grep 'not installed' > /dev/null;echo $?)" != "0" ]
 then
   epelInstalled=1
 fi
 turbovncFileInstalled=0
 if [ -f /etc/yum.repos.d/TurboVNC.repo ]
 then
   turbovncFileInstalled=1
 fi

#for RHEL 7 and Centos 7
# remove gnome-power-manager tk tcl
# rename openmotif22 to motif.i686
# recompile fortran with gfortran and remove compat-libf2c-34 compat-gcc-34-g77 
#   and add libgfortran
# recompile Xrecon to remove libtiff dependence

# kernel-headers
# kernel-devel
# libidn
# sendmail-cf
# xinetd

 commonList='
  make
  gcc
  gcc-c++
  gdb
  libtool
  binutils
  automake
  strace
  autoconf
  expect
  rsh
  rsh-server
  tftp-server
  libgfortran
  mutt
  ghostscript
 '

# Must list 32-bit packages, since these are no longer installed along with the 64-bit versions
 bit32List='
  mesa-libGL-devel
  mesa-libGL
  mesa-libGLU
  mesa-libGLU-devel
 '

 pipeList='
  xorg-x11-fonts-100dpi
  xorg-x11-fonts-ISO8859-1-100dpi
  xorg-x11-fonts-75dpi
  xorg-x11-fonts-ISO8859-1-75dpi
  xorg-x11-fonts-misc
 '

# The following were removed from the CentOS 6.8 kickstart
# gnome-python2-desktop
# compat-gcc-34-g77
# mutt
# openmotif22
# openmotif-devel.i686
# rpmforge-release
# tigervnc
# tigervnc-server
# @basic-desktop
# @desktop-platform all files present
# @desktop-platform-devel
# @eclipse
# @general-desktop
# rsh
# rsh-server
# sendmail-cf
# unix2dos
# @server-platform
# @server-platform-devel
# @server-policy
# @tex
# @virtualization
# @workstation-policy  installs gdm, already present
# @additional-devel
# @base
# @core
# @desktop-debugging
# @development
# @directory-client
# @emacs
# @fonts
# @graphical-admin-tools
# @input-methods
# @internet-browser
# @java-platform
# @legacy-x
# @network-file-system-client
# @performance
# @perl-runtime
# @print-client
# @remote-desktop-clients
# @technical-writing
# @virtualization-client
# @virtualization-platform
# @x11
# libgnomeui-devel
# libbonobo-devel
# kdegraphics

 package68List=' 
  @debugging
  @graphics
  libstdc++
  libstdc++-devel
  glibc
  glibc-devel
 '

 offList=' 
  libgcrypt-devel
  libXinerama-devel
  xorg-x11-proto-devel
  startup-notification-devel
  junit
  libXau-devel
  libXrandr-devel
  popt-devel
  libdrm-devel
  libxslt-devel
  libglade2-devel
  gnutls-devel
  pax
  python-dmidecode
  oddjob
  wodim
  sgpio
  genisoimage
  device-mapper-persistent-data
  systemtap-client
  abrt-gui
  desktop-file-utils
  ant
  rpmdevtools
  javapackages-tools
  rpmlint
  samba-winbind
  certmonger
  pam_krb5
  krb5-workstation
  netpbm-progs
  libXmu
  libXp
  perl-DBD-SQLite
  libvirt-java
  python-psycopg2
  xinetd
  xterm
  createrepo
  gconf-editor
  gsl-devel
  hplip-gui
  icedtea-web
  lm_sensors
  logwatch
  recode
  syslinux-extlinux
  perl-Compress-Raw-Bzip2
 '

 item68List='
  motif
  libX11
  libXt
  mtools
  expect
  gpm
  ImageMagick
  k3b
  minicom
  pexpect
  postgresql-docs
  postgresql-odbc
  postgresql-server
  PyGreSQL
  sharutils
  telnet
  tftp-server
  a2ps
  compat-libstdc++-33
  dos2unix
  gitk
  gnuplot
  samba
  tftp
  fuseiso
 '
#These do not need to be installed for running OpenVnmrJ
#Just if you intend to build it
 buildList='
  motif-devel
  libXmu-devel
  libXaw-devel.i686
  libXext-devel.i686
  libX11-devel.i686
  libXau.i686
  libX11.i686
  libXi.i686
  libXt.i686
  libXaw.i686
  libXpm.i686
  libXt-devel.i686
  libXtst-devel.i686
  ncurses-libs.i686
  ncurses-devel.i686
  libxml2.i686
  libxml2-devel.i686
  alsa-lib.i686
  atk-devel.i686
  glibc-devel.i686
  gtk2-devel.i686
  libidn-devel.i686
  libstdc++-devel.i686
  expat.i686
  expat-devel.i686
  kde-baseapps-libs
  atk.i686
  gtk2.i686
  libidn.i686
 '
# from epel-release
 epelList='
  scons
  meld
  x11vnc
  ntfsprogs
  fuse-ntfs-3g
  kdiff3
 '
 if [ $version -ne 7 ]; then
#  Add older motif packages
   ver6List='
      openmotif22
      openmotif.i686
      openmotif-devel.i686
   '
   packagelist="$ver6List $item68List $commonList $bit32List $pipeList"
 else
   packagelist="$item68List $commonList $bit32List $pipeList"
   packagelist="$item68List $commonList $pipeList"
 fi

 postfix=`date +"%F_%T"`
 if [ -d /tmp/ovj_preinstall ]; then
   logfile="/tmp/ovj_preinstall/pkgInstall.log_$postfix"
 else
   logfile="/tmp/pkgInstall.log_$postfix"
 fi

# The PackageKit script often holds a yum lock.
# This prevents this script from executing
 npids=$(ps -ef  | grep PackageKit | grep -v grep | awk '{ printf("%d ",$2) }')
 for prog_pid in $npids
 do
   kill $prog_pid
 done

 echo "You can monitor the progress in a separate terminal window with the command"
 echo "tail -f $logfile"
 echo "Installing standard packages (1 of 3)"
 yum -y install $package68List &> $logfile
 yum -y upgrade glibc glibc-devel libstdc++ libstdc++-devel libX11 libXt &>> $logfile

 echo "Installing required packages (2 of 3)"
 yumList=''
 for xpack in $packagelist
 do
#  rpm -q $xpack
   if [ "$(rpm -q $xpack | grep 'not installed' > /dev/null;echo $?)" == "0" ]
   then
#     echo "VnmrJ required RHEL package \"$xpack\" NOT installed"
      yumList="$yumList $xpack"
   fi
 done
 if [ "x$yumList" != "x" ]; then
   yum -y install $yumList &>> $logfile
 fi
 yum -y install glibc.i686 glibc-devel.i686 libstdc++.i686 libstdc++-devel.i686 libX11.i686 libXt.i686 &>> $logfile

# perl-homedir creates a perl5 directory in every acct. This fixes it so it does not do that.
 if [ $perlHomeInstalled -eq 0 ]
 then
   echo "Removing perl-home rpm" &>> $logfile
   yum -y erase perl-homedir &>> $logfile
 fi

 echo "Installing additional packages (3 of 3)"
 if [ $epelInstalled -eq 0 ]
 then
   yum -y install epel-release &>> $logfile
 fi
 yum -y install $epelList &>> $logfile
 if [ $epelInstalled -eq 0 ]
 then
   yum -y erase epel-release &>> $logfile
 fi
#Add turbovnc
 if [ "$(rpm -q turbovnc | grep 'not installed' > /dev/null;echo $?)" == "0" ]
 then
   if [ $turbovncFileInstalled -eq 0 ]
   then
      cat >/etc/yum.repos.d/TurboVNC.repo  <<EOF

[TurboVNC]
name=TurboVNC official RPMs
baseurl=https://sourceforge.net/projects/turbovnc/files
gpgcheck=1
gpgkey=http://pgp.mit.edu/pks/lookup?op=get&search=0x6BBEFA1972FEB9CE
enabled=1
exclude=turbovnc-*.*.9[0-9]-*
EOF
   fi
   yum -y install turbovnc &>> $logfile
   if [ $turbovncFileInstalled -eq 0 ]
   then
      rm -f /etc/yum.repos.d/TurboVNC.repo
   fi
 fi

 dir=`dirname "$(readlink -f $0))"`
 if [ "$(rpm -q gftp | grep 'not installed' > /dev/null;echo $?)" == "0" ]
 then
   if [ -f $dir/linux/gftp-2.0.19-4.el6.rf.x86_64.rpm ]
   then
      yum -y install $dir/linux/gftp-2.0.19-4.el6.rf.x86_64.rpm &>> $logfile
   fi
 fi
 if [ "$(rpm -q numlockx | grep 'not installed' > /dev/null;echo $?)" == "0" ]
 then
   if [ -f $dir/linux/numlockx-1.2-6.el7.nux.x86_64.rpm ]
   then
      yum -y install $dir/linux/numlockx-1.2-6.el7.nux.x86_64.rpm &>> $logfile
   fi
 fi

 echo "CentOS / RedHat package installation complete"
 echo " "
else
 distrover=$(lsb_release -rs)
 distmajor=${distrover:0:2}
 if [ $distmajor -lt 14 ] ; then
     echo "Only Ubuntu 14 or newer is supported"
     echo " "
     exit 1
 fi
 postfix=`date +"%F_%T"`
 if [ -d /tmp/ovj_preinstall ]; then
   logfile="/tmp/ovj_preinstall/pkgInstall.log_$postfix"
 else
   logfile="/tmp/pkgInstall.log_$postfix"
 fi

 echo "You can monitor the progress in a separate terminal window with the command"
 echo "tail -f $logfile"
 echo "Installing standard packages (1 of 2)"
# The unattended-upgrade script often holds a yum lock.
# This prevents this script from executing
 npids=$(ps -ef  | grep unattended-upgrade | grep -v grep | awk '{ printf("%d ",$2) }')
 for prog_pid in $npids
 do
   kill -s 2 $prog_pid
 done
 dpkg --add-architecture i386
 apt-get -qq update
# apt-get -qq -y dist-upgrade
# Prevent packages from presenting an interactive popup
 export DEBIAN_FRONTEND=noninteractive
 apt-get install -y csh make expect bc git scons g++ gfortran \
      openssh-server mutt sharutils sendmail-cf gnome-power-manager \
      kdiff3 ghostscript imagemagick postgresql \
      gedit dos2unix zip cups gnuplot gnome-terminal enscript &> $logfile
 echo "Installing version specific packages (2 of 2)"
 if [ $distmajor -gt 16 ] ; then
   # these are needed to build
    apt-get install -y gdm3 gnome-session openjdk-8-jre \
      lib32stdc++-7-dev libc6-dev-i386 libglu1-mesa-dev libgsl-dev &>> $logfile
 elif [ $distmajor -gt 14 ] ; then
   # these are needed to build
    apt-get install -y gdm3 gnome-session openjdk-8-jre \
      lib32stdc++-5-dev libc6-dev-i386 libglu1-mesa-dev libgsl-dev &>> $logfile
 else
    # these are needed to build
    apt-get install -y openjdk-6-jre lib32stdc++-4.8-dev \
            libc6-dev-i386 libglu1-mesa-dev libgsl0-dev &>> $logfile
 fi
 # apt-get uninstalls these if an amd64 version is installed for something else >:(
 # so install them last...
 apt-get install -y libmotif-dev libx11-dev libxt-dev libx11-dev:i386 libxt-dev:i386 &>> $logfile
 unset DEBIAN_FRONTEND
 echo "dash dash/sh boolean false" | debconf-set-selections &>> $logfile
 dpkg-reconfigure -u dash &>> $logfile
 echo "Ubuntu package installation complete"
 echo " "
fi
exit 0
