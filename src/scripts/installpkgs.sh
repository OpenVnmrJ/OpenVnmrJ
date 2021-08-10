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

userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [ $userId != "uid=0(root)" ]; then
  echo
  if [[ "x$(basename $0)" = "xovjGetRepo" ]]; then
     echo "Downloading Linux packages for OpenVnmrJ."
  else
     echo "Installing Linux packages for OpenVnmrJ."
  fi
  echo "Or type cntrl-C to exit."
  echo
  s=1
  t=3
  while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
    if [ -x /usr/bin/dpkg ]; then
      echo "If requested, enter the admin (sudo) password"
      sudo $0 $* ;
    else
      echo "Please enter this system's root user password"
      su root -c "$0 $*";
    fi
    s=$?
    t=$((t-1))
    echo " "
  done
  if [ $t = 0 ]; then
    echo "Access denied. Type cntrl-C to exit this window."
    echo "Type $0 to start the installation program again"
    echo ""
  fi
  exit
fi


turboRepo() {
  cat >/etc/yum.repos.d/TurboVNC.repo  <<EOF

[TurboVNC]
name=TurboVNC official RPMs
baseurl=https://sourceforge.net/projects/turbovnc/files
gpgcheck=1
gpgkey=https://sourceforge.net/projects/turbovnc/files/VGL-GPG-KEY
       https://sourceforge.net/projects/turbovnc/files/VGL-GPG-KEY-1024
enabled=1
exclude=turbovnc-*.*.9[0-9]-*
EOF
}

if [ -x /usr/bin/dpkg ]; then
   if [[ -f /etc/apt/sources.ovj ]]; then
     ovjRepo=1
   else
     ovjRepo=0
   fi
else
   if [[ -f /etc/yum.repos.d/openvnmrj.repo ]]; then
     ovjRepo=1
   else
     ovjRepo=0
   fi
fi

repoGet=0
repoArg=""
if [[ "x$(basename $0)" = "xovjGetRepo" ]]; then
  osType=$(uname -s | awk '{ print tolower($0) }')
  if [[ "${osType:0:5}" != linux ]]; then
     echo "Cannot use $0 on $osType"
     exit 1
  fi
  repoGet=1
  if [[ $ovjRepo -eq 1 ]]; then
    if [ -x /usr/bin/dpkg ]; then
       mv /etc/apt/sources.ovj /etc/apt/sources.list
    else
       rm -f /etc/yum.repos.d/openvnmrj.repo
    fi
    ovjRepo=0
  fi
  repoPath=$(dirname $(dirname $(readlink -f $0)))
  repoPath=$(dirname $repoPath)/openvnmrj.repo
  if [ -x /usr/bin/dpkg ]; then
     repoArg="-d -o dir::cache=$repoPath/dnld --reinstall"
     rm -rf $repoPath
     mkdir -p $repoPath/dnld
     chmod -R 777 $repoPath 
  else
     repoArg="--download_path=$repoPath"
     repoArg="--downloadonly --downloaddir=$repoPath"
  fi
fi

noPing=0
ddrAcq=0
miAcq=0
b12Acq=0
for arg in "$@"
do
  if [[ "x$arg" = "xnoPing" ]]; then
    noPing=1
  fi
  if [[ "x$arg" = "xddr" ]]; then
    ddrAcq=1
  fi
  if [[ "x$arg" = "xmi" ]]; then
    miAcq=1
  fi
  if [[ "x$arg" = "xb12" ]]; then
    b12Acq=1
  fi
done

if [[ $ovjRepo -eq 0 ]] && [[ $noPing -eq 0 ]]
then
  ping -W 1 -c 1 google.com > /dev/null 2>&1
  if [ $? -ne 0 ]
  then
    echo "Must be connected to the internet for $0 to function"
    echo "This is tested by doing \"ping google.com\". The ping"
    echo "command may also fail due to a firewall blocking it."
    echo "If you are sure the system is connected to the internet"
    echo "and want to bypass this \"ping\" test, use"
    echo "./load.nmr noPing"
    echo "or"
    echo "$0 noPing"
    echo ""
    exit 2
  fi
fi

postfix=$(date +"%F_%H_%M_%S")
if [ -d /tmp/ovj_preinstall ]; then
  logfile="/tmp/ovj_preinstall/pkgInstall.log_$postfix"
else
  logfile="/tmp/pkgInstall.log_$postfix"
fi
touch $logfile
chmod 666 $logfile

if [ ! -x /usr/bin/dpkg ]; then
  if [ -f /etc/centos-release ]; then
    rel=centos-release
#   remove all characters up to the first digit
    version=$(cat /etc/$rel | sed -E 's/[^0-9]+//')
  elif [ -f /etc/redhat-release ]; then
    rel=redhat-release
#   remove all characters up to the first digit
    version=$(cat /etc/$rel | sed -E 's/[^0-9]+//')
  else
#   Assume Linux variant is like CentOS 8
    version="8.0"
  fi
# remove all characters from end including first dot
  version=${version%%.*}

# Determine if some packages are already installed.
# If they are, do not remove them.

# perl-homedir creates a perl5 directory in every acct.
# Removing it fixes it so it does not do that.
  perlHomeInstalled=0
  if [ "$(rpm -q perl-homedir |
    grep 'not installed' > /dev/null;echo $?)" != "0" ]
  then
    perlHomeInstalled=1
  fi
  epelInstalled=0
  if [ "$(rpm -q epel-release |
    grep 'not installed' > /dev/null;echo $?)" != "0" ]
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
  tftp-server
  libgfortran
  mutt
  ghostscript
 '
  if [ $version -lt 8 ]; then
    commonList="$commonList rsh rsh-server"
  else
    commonList="$commonList csh compat-openssl10 compat-libgfortran-48"
  fi

# Must list 32-bit packages, since these are no longer
# installed along with the 64-bit versions
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
# @debugging
# @graphics

  package68List=' 
  libstdc++
  libstdc++-devel
  glibc
  glibc-devel
  libX11
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
  samba
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
  gsl-devel
  hplip-gui
  icedtea-web
  lm_sensors
  logwatch
  recode
  syslinux-extlinux
 '

  item68List='
  libXt
  motif
  mtools
  expect
  gpm
  minicom
  telnet
  tftp-server
  dos2unix
  gitk
  gnuplot
  gsl
  tftp
  xinetd
  xterm
  createrepo
  perl-Compress-Raw-Bzip2
 '
  if [ $version -lt 8 ]; then
    item68Listb='
    gimp
    ImageMagick
    k3b
    pexpect
    postgresql-docs
    postgresql-odbc
    postgresql-server
    PyGreSQL
    sharutils
    a2ps
    fuseiso
    gconf-editor
  '
    item68List="$item68List $item68Listb"
  fi
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
  ntfsprogs
  fuse-ntfs-3g
  kdiff3
 '
  if [ $version -lt 8 ]; then
    epelList="$epelList scons meld x11vnc"
  else
    epelList="$epelList kdiff3 k3b ImageMagick rsh rsh-server"
    if [ ! -z $(type -t subscription-manager) ]; then
        epelList="$epelList sharutils"
    fi
  fi
  if [ $version -lt 7 ]; then
#  Add older motif package
    packageList="openmotif $item68List $commonList $bit32List $pipeList"
  else
    packageList="$item68List $commonList $pipeList java-1.8.0-openjdk libnsl"
  fi


#  The PackageKit script often holds a yum lock.
#  This prevents this script from executing
#  On CentOS 7, the systemctl command should stop the PackageKit
  if [ $version -ge 7 ]; then
    systemctl --now --runtime mask packagekit > /dev/null 2>&1
  fi
  npids=$(ps -ef  | grep PackageKit | grep -v grep |
	   awk '{ printf("%d ",$2) }')
  for prog_pid in $npids
  do
    kill $prog_pid
    sleep 2
  done
#  Try a second time
  npids=$(ps -ef  | grep PackageKit | grep -v grep |
           awk '{ printf("%d ",$2) }')
  for prog_pid in $npids
  do
    kill $prog_pid
  done
#  If it will not die, exit
  npids=$(ps -ef  | grep PackageKit | grep -v grep |
	   awk '{ printf("%d ",$2) }')
  if [ ! -z $npids ]
  then
    echo "CentOS / RedHat PackageKit is preventing installation"
    echo "Please try again in 5-10 minutes,"
    echo "after this tool completes its task."
    exit 1
  fi

  echo "You can monitor the progress in a separate terminal window with the command"
  echo "tail -f $logfile"

  yum68List=''
  for xpack in $package68List
  do
    yum68List="$yum68List ${xpack}.i686"
  done
  yum68List="$yum68List ncurses-libs.i686"
  if [[ $repoGet -eq 1 ]]; then
    echo "Downloading standard packages (1 of 3)"
    echo "Downloading standard packages (1 of 3)" > $logfile
    chmod 666 $logfile
    yum -y install $repoArg $package68List &>> $logfile
#   in CentOS 8, yum clears the downloaddir prior to the download
    if [ $version -eq 8 ]; then
      repoPathTmp=${repoPath}.tmp
      mkdir $repoPathTmp
      cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
    fi
    yum -y install $package68List &>> $logfile
    yum -y upgrade $repoArg $package68List &>> $logfile
    if [ $version -eq 8 ]; then
      cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
    fi
  fi
  echo "Installing standard packages (1 of 3)"
  echo "Installing standard packages (1 of 3)" &>> $logfile
  chmod 666 $logfile
  if [[ $ovjRepo -eq 1 ]]; then
    yum -y install --disablerepo="*" --enablerepo="openvnmrj" $package68List &>> $logfile
    yum -y upgrade --disablerepo="*" --enablerepo="openvnmrj" $package68List &>> $logfile
  else
    if [ $version -lt 8 ]; then
      yum -y install $package68List &>> $logfile
    fi
    yum -y upgrade $package68List &>> $logfile
  fi
  if [[ $repoGet -eq 1 ]]; then
    echo "Downloading required packages (2 of 3)"
    echo "Downloading required packages (2 of 3)" >> $logfile
    yum -y install $repoArg $packageList &>> $logfile
    if [ $version -eq 8 ] && [ -z $(type -t subscription-manager) ]; then
      # Capitalization of PowerTools causes problems with 8.3
      yum -y --enablerepo=?ower?ools install $repoArg sharutils &>> $logfile
      if [ $version -eq 8 ]; then
        cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
      fi
    fi
    yum -y install $repoArg $yum68List &>> $logfile
    if [ $version -eq 8 ]; then
      cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
    fi
  fi
  echo "Installing required packages (2 of 3)"
  echo "Installing required packages (2 of 3)" >> $logfile
  yumList=''
  for xpack in $packageList
  do
    if [ "$(rpm -q $xpack | grep 'not installed' > /dev/null;echo $?)" == "0" ]
    then
      yumList="$yumList $xpack"
    fi
  done
  if [[ $ovjRepo -eq 1 ]]; then
    if [ $version -eq 8 ]; then
      if [ "$(rpm -q sharutils | grep 'not installed' > /dev/null;echo $?)" == "0" ]
      then
        yumList="$yumList sharutils"
      fi
    fi
    if [ "x$yumList" != "x" ]; then
      yum -y install --disablerepo="*" --enablerepo="openvnmrj" $yumList &>> $logfile
    fi
    yum -y install --disablerepo="*" --enablerepo="openvnmrj" $yum68List &>> $logfile
  else
    if [ "x$yumList" != "x" ]; then
      yum -y install $yumList &>> $logfile
    fi
    if [ $version -eq 8 ] && [ -z $(type -t subscription-manager) ]; then
      if [ "$(rpm -q sharutils | grep 'not installed' > /dev/null;echo $?)" == "0" ]
      then
        # Capitalization of PowerTools causes problems with 8.3
        yum -y --enablerepo=?ower?ools install sharutils &>> $logfile
      fi
    fi
    yum -y install $yum68List &>> $logfile
  fi

# perl-homedir creates a perl5 directory in every acct. This fixes it so it does not do that.
  if [ $perlHomeInstalled -eq 0 ]
  then
    echo "Removing perl-home rpm" &>> $logfile
    yum -y erase perl-homedir > /dev/null 2>&1
  fi

 # No need to add repos if OVJ repo is being used
  if [[ $ovjRepo -eq 1 ]]; then
    epelInstalled=1
    turbovncFileInstalled=1
  fi
  if [ $epelInstalled -eq 0 ]
  then
    if [ -z $(type -t subscription-manager) ]; then
        yum -y install epel-release &>> $logfile
    else
        yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm &>> $logfile
	if [[ $(subscription-manager repos --list-enabled |
		grep -i codeready > /dev/null; echo $?) != 0 ]]; then
          ARCH=$(/bin/arch)
          subscription-manager repos --enable "codeready-builder-for-rhel-8-${ARCH}-rpms" &>> $logfile
        fi
    fi
  fi
  if [[ $repoGet -eq 1 ]]; then
    echo "Downloading additional packages (3 of 3)"
    echo "Downloading additional packages (3 of 3)" >> $logfile
    yum -y install $repoArg $epelList &>> $logfile
    if [ $version -eq 8 ]; then
      cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
    fi
  fi
  echo "Installing additional packages (3 of 3)"
  echo "Installing additional packages (3 of 3)" >> $logfile

  if [[ $ovjRepo -eq 1 ]]; then
    yum -y install --disablerepo="*" --enablerepo="openvnmrj" $epelList &>> $logfile
  else
    yum -y install $epelList &>> $logfile
  fi
  if [ $epelInstalled -eq 0 ]
  then
    yum -y erase epel-release &>> $logfile
  fi
  if [ "$(rpm -q turbovnc | grep 'not installed' > /dev/null;echo $?)" == "0" ]
  then
    if [ $turbovncFileInstalled -eq 0 ]
    then
      turboRepo
    fi
    if [[ $repoGet -eq 1 ]]; then
      yum -y install $repoArg turbovnc &>> $logfile
      if [ $version -eq 8 ]; then
        cp -f $repoPath/* $repoPathTmp > /dev/null 2>&1
      fi
    fi
    if [[ $ovjRepo -eq 1 ]]; then
      yum -y install --disablerepo="*" --enablerepo="openvnmrj" turbovnc &>> $logfile
    else
      yum -y install turbovnc &>> $logfile
    fi
    if [ $turbovncFileInstalled -eq 0 ]
    then
      rm -f /etc/yum.repos.d/TurboVNC.repo
    fi
  fi

  dir=$(dirname $0)
  if [ "$(rpm -q gftp | grep 'not installed' > /dev/null;echo $?)" == "0" ]
  then
    if [ -f $dir/linux/gftp-2.0.19-4.el6.rf.x86_64.rpm ]
    then
      yum -y install --disablerepo="*" $dir/linux/gftp-2.0.19-4.el6.rf.x86_64.rpm &>> $logfile
    fi
  fi
  if [ "$(rpm -q numlockx | grep 'not installed' > /dev/null;echo $?)" == "0" ]
  then
    if [ -f $dir/linux/numlockx-1.2-6.el7.nux.x86_64.rpm ]
    then
      yum -y install --disablerepo="*" $dir/linux/numlockx-1.2-6.el7.nux.x86_64.rpm &>> $logfile
    fi
  fi

  if [ $version -ge 7 ]; then
    systemctl unmask packagekit > /dev/null 2>&1
  fi
  if [[ $ovjRepo -eq 1 ]]; then
     rm -f /etc/yum.repos.d/openvnmrj.repo
  fi
  chown $(stat -c "%U.%G" $0) $logfile
  if [[ $repoGet -eq 1 ]]; then
    if [ $version -eq 8 ]; then
      rm -rf $repoPath
      mv $repoPathTmp $repoPath
    fi
    chown -R $(stat -c "%U.%G" $0) $repoPath
    echo "CentOS / RedHat package download complete"
    echo "Packages stored in $repoPath"
  else
    echo "CentOS / RedHat package installation complete"
  fi
  echo " "
else
  . /etc/lsb-release
  distmajor=${DISTRIB_RELEASE:0:2}
  if [ $distmajor -lt 14 ] ; then
    echo "Only Ubuntu 14 or newer is supported"
    echo " "
    exit 1
  fi

  echo "You can monitor progress in a separate terminal window with the command"
  echo "tail -f $logfile"
  if [[ $repoGet -eq 1 ]]; then
     echo "Downloading standard packages (1 of 2)"
     echo "Downloading standard packages (1 of 2)" > $logfile
  else
     echo "Installing standard packages (1 of 2)"
     echo "Installing standard packages (1 of 2)" > $logfile
  fi
# The unattended-upgrade script often holds a yum lock.
# This prevents this script from executing
  if [[ -x /bin/systemctl ]]; then
    systemctl --now --runtime mask unattended-upgrades > /dev/null 2>&1
  fi
  npids=$(ps -ef  | grep unattended-upgrade | grep -v grep |
	  awk '{ printf("%d ",$2) }')
  for prog_pid in $npids
  do
    kill -s 2 $prog_pid
    sleep 2
  done
# Try a second time
  npids=$(ps -ef  | grep unattended-upgrade | grep -v grep |
	  awk '{ printf("%d ",$2) }')
  for prog_pid in $npids
  do
    kill -s 2 $prog_pid
  done
# If it will not die, exit
  npids=$(ps -ef  | grep unattended-upgrade | grep -v grep |
	  awk '{ printf("%d ",$2) }')
  if [ ! -z $npids ]
  then
    echo "Ubuntu unattended-update is preventing installation"
    echo "Please try again in 5-10 minutes, after this tool completes its task."
    exit 1
  fi
  acqInstall=""
  if [[ $b12Acq -eq 1 ]]; then
      acqInstall="libusb-dev"
  else
      dpkg --add-architecture i386
      if [[ $ddrAcq -eq 1 ]]; then
          acqInstall="rarpd rsh-client rsh-server tftp-hpa tftpd-hpa"
      fi
      if [[ $miAcq -eq 1 ]]; then
          acqInstall="$acqInstall tftp-hpa tftpd-hpa"
      fi
  fi
  apt-get -qq update
# apt-get -qq -y dist-upgrade
  if [[ $ovjRepo -eq 1 ]]; then
     repoArg="--allow-unauthenticated"
  fi
# Prevent packages from presenting an interactive popup
  export DEBIAN_FRONTEND=noninteractive
  if [[ $repoGet -eq 1 ]]; then
     acqInstall="rarpd rsh-client rsh-server tftp-hpa tftpd-hpa"
     apt-get -y install dpkg-dev &>> $logfile
     dpkg --add-architecture i386
  fi
  apt-get $repoArg -y install tcsh make expect bc git scons g++ gfortran \
      openssh-server mutt sharutils sendmail-cf gnome-power-manager \
      kdiff3 libcanberra-gtk-module ghostscript imagemagick vim xterm \
      gedit dos2unix zip cups gnuplot gnome-terminal enscript rpcbind \
      $acqInstall &>> $logfile
  if [[ $repoGet -eq 1 ]]; then
     echo "Downloading version specific packages (2 of 2)"
     echo "Downloading version specific packages (2 of 2)" >> $logfile
  else
     echo "Installing version specific packages (2 of 2)"
     echo "Installing version specific packages (2 of 2)" >> $logfile
  fi
  if [ $distmajor -gt 18 ] ; then
   # these are needed to build
    installList='gdm3
                 gnome-session
                 openjdk-8-jre
                 lib32stdc++-8-dev
                 libc6-dev
                 libglu1-mesa-dev
                 libgsl-dev'
    if [[ $b12Acq -ne 1 ]]; then
       installList="$installList libcrypt1:i386"
    fi
    apt-get $repoArg -y install $installList &>> $logfile
  elif [ $distmajor -gt 16 ] ; then
   # these are needed to build
    apt-get $repoArg -y install gdm3 gnome-session openjdk-8-jre \
      lib32stdc++-7-dev libc6-dev-i386 libglu1-mesa-dev libgsl-dev &>> $logfile
  elif [ $distmajor -gt 14 ] ; then
   # these are needed to build
    apt-get $repoArg -y install postgresql gdm3 gnome-session openjdk-8-jre \
      lib32stdc++-5-dev libc6-dev-i386 libglu1-mesa-dev libgsl-dev &>> $logfile
  else
    # these are needed to build
    apt-get $repoArg -y install postgresql openjdk-6-jre lib32stdc++-4.8-dev \
            libc6-dev-i386 libglu1-mesa-dev libgsl0-dev &>> $logfile
  fi
  if [[ $ovjRepo -eq 1 ]]; then
     repoPath=$(head -n 1 /etc/apt/sources.list | awk '{print $5}')
     repoPath=${repoPath:5}
     fontList=$(cd $repoPath && ls -1 fonts-* | tr "_" " " | awk '{print $1}' )
  else
     fontList=$(apt-cache search -n "^fonts-*" | awk '{print $1}' |
             grep -v "^fonts-al" |
             grep -v "^fonts-ancient" |
             grep -v "^fonts-aoy" |
             grep -v "^fonts-arab" |
             grep -v "^fonts-arp" |
             grep -v "^fonts-aru" |
	     grep -v "^fonts-ba" | 
	     grep -v "^fonts-ben" | 
             grep -v "^fonts-comp" |
             grep -v "^fonts-cn" |
	     grep -v "^fonts-cw" | 
	     grep -v "^fonts-ddc" | 
	     grep -v "^fonts-deji" | 
	     grep -v "^fonts-deva" | 
	     grep -v "^fonts-dz" | 
	     grep -v "^fonts-ee" | 
	     grep -v "^fonts-far" | 
	     grep -v "^fonts-freefar" | 
             grep -v "^fonts-garg" |
             grep -v "^fonts-gfs" |
	     grep -v "^fonts-gu" | 
             grep -v "^fonts-hanazono" |
             grep -v "^fonts-horai" |
             grep -v "^fonts-hos" |
             grep -v "^fonts-ibm" |
             grep -v "^fonts-ind" |
             grep -v "^fonts-ipa" |
             grep -v "^fonts-k" |
	     grep -v "^fonts-lara" | 
	     grep -v "^fonts-ldco" | 
	     grep -v "^fonts-lexi" | 
	     grep -v "^fonts-lg-a" | 
	     grep -v "^fonts-lk" | 
             grep -v "^fonts-lohit" |
	     grep -v "^fonts-manch" | 
	     grep -v "^fonts-mathe" | 
	     grep -v "^fonts-migmix" | 
	     grep -v "^fonts-mik" | 
	     grep -v "^fonts-misa" | 
	     grep -v "^fonts-mlym" | 
             grep -v "^fonts-mplus" |
	     grep -v "^fonts-mo" | 
             grep -v "^fonts-na" |
             grep -v "^fonts-noto" |
	     grep -v "^fonts-olds" | 
	     grep -v "^fonts-or" | 
	     grep -v "^fonts-pa" | 
	     grep -v "^fonts-or" | 
	     grep -v "^fonts-or" | 
             grep -v "^fonts-rict" |
             grep -v "^fonts-rit" |
             grep -v "^fonts-s" |
	     grep -v "^fonts-t" | 
             grep -v "^fonts-uk" |
	     grep -v "^fonts-um" | 
             grep -v "^fonts-un" |
             grep -v "^fonts-vlg" |
             grep -v "^fonts-wo" |
             grep -v "^fonts-wqy" |
             grep -v "^fonts-yo" |
             grep -v "^fonts-yr" 
            )
  fi
  apt-get $repoArg -y install $fontList &>> $logfile
# apt-get uninstalls these if an amd64 version is installed for something else >:(
# so install them last...
  apt-get $repoArg -y install libmotif-dev libx11-dev libxt-dev &>> $logfile
  unset DEBIAN_FRONTEND
  if [[ -x /bin/systemctl ]]; then
    systemctl unmask unattended-upgrades > /dev/null 2>&1
  fi
  if [[ $ovjRepo -eq 1 ]]; then
     mv /etc/apt/sources.ovj /etc/apt/sources.list
  fi
  if [[ $repoGet -eq 1 ]]; then
     mv $repoPath/dnld/archives/* $repoPath
     rm -rf $repoPath/dnld $repoPath/partial $repoPath/lock
     cd $repoPath
     dpkg-scanpackages . /dev/null | gzip -c9 > Packages.gz
     echo "Ubuntu package download complete"
  else
     echo "dash dash/sh boolean false" | debconf-set-selections &>> $logfile
     dpkg-reconfigure -u dash &>> $logfile
     echo "Ubuntu package installation complete"
  fi
  echo " "
fi
exit 0
