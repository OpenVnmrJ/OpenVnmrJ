#!/bin/bash
#
# Copyright (C) 2017  Dan Iverson
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#
#Uncomment next line for debugging output
#set -x

SCRIPT=$0

ovj_usage() {
    cat <<EOF

usage:

    $SCRIPT will install VirtualBox into the Linux environment.
    If your system has internet access, then entering $SCRIPT
    will do the installion.

    $SCRIPT tests internet access by "pinging" google.com.
    The ping command may also fail due to a firewall blocking it.
    If you are sure the system is connected to the internet
    and want to bypass this "ping" test, use
    $SCRIPT noPing
EOF
    exit 1
}

noPing=0

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -h|--help)              ovj_usage                   ;;
        noPing)                 noPing=1; shift ;;
        -vv|--debug)            set -x ;;
        *)
            # unknown option
            echo "unrecognized argument: $key"
            ovj_usage
            ;;
    esac
    shift
done


userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [ $userId != "uid=0(root)" ]; then
  echo
  echo "Installing VirtualBox"
  echo
  s=1
  t=2
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



checkNetwork() {
#   The below seems to have disabled ping. Use google instead
    local URL="google.com"
    local pingRecv=0
    pingRecv=`ping -c 1 -q -W 1 $URL | grep received | awk '{ print $4 }'`
         
    if [[ ${pingRecv} -eq 1 ]] || [[ $noPing -eq 1 ]] ; then
        echo "Test for internet access passed"
        return 0
    else
        echo "Internet access failed"
        echo "This is tested by doing \"ping $URL\". The ping"
        echo "command may also fail due to a firewall blocking it."
        echo "If you are sure the system is connected to the internet"
        echo "and want to bypass this \"ping\" test, use"
        echo "$SCRIPT noPing"
        echo ""
        return 1
    fi
}

#
# Main program starts here
#

checkNetwork
if [[ $? -ne 0 ]]; then
   exit 1
fi

if [ -x /usr/bin/dpkg ]; then
    grep -v virtualbox /etc/apt/sources.list > /etc/apt/tmp
    source /etc/os-release
    echo "deb https://download.virtualbox.org/virtualbox/debian $UBUNTU_CODENAME contrib" >> /etc/apt/tmp
    mv /etc/apt/tmp /etc/apt/sources.list
    wget -q https://www.virtualbox.org/download/oracle_vbox_2016.asc -O- | apt-key add -
    wget -q https://www.virtualbox.org/download/oracle_vbox.asc -O- | apt-key add -
    apt-get update
    apt-get -y install virtualbox-6.1
else
    if [ -f /etc/centos-release ]; then
       rel=centos-release
#      remove all characters up to the first digit
       version=$(cat /etc/$rel | sed -E 's/[^0-9]+//')
    elif [ -f /etc/redhat-release ]; then
       rel=redhat-release
#      remove all characters up to the first digit
       version=$(cat /etc/$rel | sed -E 's/[^0-9]+//')
    else
#      Assume Linux variant is like CentOS 8
       version="8.0"
    fi
# remove all characters from end including first dot
    releasever=${version%%.*}
    cat <<EOF > /etc/yum.repos.d/Oracle.repo
[virtualbox]
name=Oracle Linux / RHEL / CentOS-$releasever / $basearch - VirtualBox
baseurl=http://download.virtualbox.org/virtualbox/rpm/el/$releasever/x86_64
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://www.virtualbox.org/download/oracle_vbox.asc
EOF
    yum clean all
    yum makecache
    yum -y install gcc make perl VirtualBox-6.1
fi
#   If the install fails, may need to do the following
#   sudo apt-get remove VirtualBox-6.1
#   sudo apt-get autoremove 'virtualbox*'
#   sudo apt-get install linux-headers-5.3.0-1044-gke
#   The actual headers may be different. The error message
#   should identify the correct ones.
#   Then rerun ovjGetVB
echo ""
echo "VirtualBox installation complete"
echo "You can start VirtualBox by running"
echo "virtualbox"
echo "from a terminal."
if [ -x /usr/bin/dpkg ]; then
    echo "You can add it to your favorites list"
    echo "by entering virtualbox in the Activities search box and then"
    echo "right-clicking the VirtualBox Icon and selecting \"Add to favorites\""
fi
echo ""
exit
