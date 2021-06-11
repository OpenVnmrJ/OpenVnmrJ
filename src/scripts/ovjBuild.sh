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

SCRIPT=$(basename "$0")

ovj_usage() {
    cat <<EOF

usage:
    $SCRIPT [options...]

    $SCRIPT will build OpenVnmrJ from the latest sources on
    github. Internet access is required. When used for the first
    time, the toolchain required to build OpenVnmrJ will be installed.
    This will require root or sudo access.

    $SCRIPT tests internet access by "pinging" www.github.com.
    The ping command may fail due to a firewall blocking it.
    If you are sure the system is connected to the internet
    and want to bypass this "ping" test, use
    $SCRIPT noPing

    OpenVnmrJ will be built in an ovjbuild directory in your HOME
    directory.

options:
    -h|--help                 Display this help information

EOF
    exit 1
}

if [[ $(uname -s) != "Linux" ]]; then
    echo "$SCRIPT can only be used on Linux systems"
    exit 1
fi

noPing=0
for arg in "$@"
do
  if [[ "x$arg" = "xnoPing" ]]; then
     noPing=1
  elif [[ "x$arg" = "x-h" ]]; then
      ovj_usage
  elif [[ "x$arg" = "x--help" ]]; then
      ovj_usage
  else
      echo "unrecognized argument: $arg"
      ovj_usage
  fi
done

URL="www.github.com"
ping -c 1 -q -W 1 $URL > /dev/null 2>&1
if [[ $? -eq 0 ]] || [[ $noPing -eq 1 ]] ; then
    echo "Test for internet access to $URL passed"
else
    echo "Internet access to $URL failed"
    echo "This is tested by doing \"ping $URL\". The ping"
    echo "command may also fail due to a firewall blocking it."
    echo "If you are sure the system is connected to the internet"
    echo "and want to bypass this \"ping\" test, use"
    echo "$SCRIPT noPing"
    echo ""
    exit 1
fi

runToolChain=0

if [[ ! -d $HOME/ovjbuild ]]; then
    mkdir $HOME/ovjbuild
    runToolChain=1
fi
cd $HOME/ovjbuild
if [[ ! -d ovjTools ]]; then
    echo "Cloning ovjTools repository"
    git clone https://github.com/OpenVnmrJ/ovjTools.git
    cp -r ovjTools/bin bin
    runToolChain=1
else
    cd ovjTools
    echo "Updating ovjTools repository"
    git pull 2> /dev/null
    cd ..
fi
if [[ ! -d bin ]]; then
    cp -r ovjTools/bin bin
    runToolChain=1
fi
if [[ ! -d logs ]]; then
    mkdir logs
fi
if [[ ! -d OpenVnmrJ ]]; then
    echo "Cloning OpenVnmrJ repository"
    git clone https://github.com/OpenVnmrJ/OpenVnmrJ.git
else
    echo "Cleaning OpenVnmrJ repository"
    cd OpenVnmrJ
    rm -rf src
    git checkout src
    cd ..
fi

cd bin
if [[ $runToolChain -eq 1 ]]; then
    logFile=$HOME/ovjbuild/logs/toolChainLog
    prog=$HOME/ovjbuild/bin/toolChain
    echo "Installing toolChain"
    echo ""
    echo "The progress of installing the tool chain may be followed"
    echo "in a separate terminal window with the command"
    echo "  tail -f $logFile"
    echo ""
    userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
    if [ $userId != "uid=0(root)" ]; then
      echo ""
      s=1
      t=3
      while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
        if [ -x /usr/bin/dpkg ]; then
          echo "If requested, enter the admin (sudo) password"
          (sudo $prog $* ;) &> $logFile
        else
          echo "Please enter this system's root user password"
          (su root -c "$prog $*";) &> $logFile
        fi
        s=$?
        t=$((t-1))
        echo " "
      done
      if [ $t = 0 ]; then
        echo "Access denied. Type cntrl-C to exit this window."
        echo "Type $0 to start again"
        echo ""
        exit
      fi
  fi
fi

echo "Building OpenVnmrJ"
echo ""
echo "The progress of building OpenVnmrJ may be followed"
echo "in a separate terminal window with the command"
echo "  tail -f $HOME/ovjbuild/logs/makeovjlog"
echo ""
./buildovj

echo ""
echo "$0 complete"
echo "The DVD images are in $HOME/ovjbuild"
echo ""
