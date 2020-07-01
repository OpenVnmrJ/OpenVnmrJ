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
  echo "To install OpenVnmrJ you will need to be the system's root user."
  echo "Or type cntrl-C to exit."
  echo
  s=1
  t=3
  while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
     echo "Please enter this system's root user password"
     if [ x$distroType = "xdebian" ]; then
        sudo $0 $* ;
     else
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

ovjRepo() {
   cat >/etc/yum.repos.d/openvnmrj.repo  <<EOF

[openvnmrj]
name=OpenVnmrJ repository
baseurl=file://$repoPath
gpgcheck=0
enabled=1
EOF
}


if [[ $# -eq 1 ]]; then
  repoPath=$1
else
  repoPath=$(dirname $(dirname $(readlink -f $0)))
  repoPath=$repoPath/openvnmrj.repo
fi

if [[ ! -d $repoPath ]]; then
  echo "OpenVnmrJ repository $repoPath does not exist"
  exit 1
fi

rm -f /etc/yum.repos.d/openvnmrj.repo
if ! hash createrepo 2> /dev/null; then
   yum -y install --disablerepo="*" $repoPath/createrepo* $repoPath/drpm* > /dev/null
fi
createrepo $repoPath
ovjRepo
if hash sysytemctl 2> /dev/null; then
   systemctl stop packagekit > /dev/null 2>&1
fi
npids=$(ps -ef  | grep PackageKit | grep -v grep | awk '{ printf("%d ",$2) }')
for prog_pid in $npids
do
  kill $prog_pid
  sleep 2
done
yum clean all
exit

