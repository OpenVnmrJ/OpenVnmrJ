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

: ${OVJ_DOWNLOAD=""}
: ${OVJ_INSTALL=""}
: ${OVJ_LOG=""}
: ${OVJ_VECHO="echo"}

ovj_usage() {
    cat <<EOF

usage:
    $SCRIPT [options...]

    $SCRIPT will install the VnmrJ 4.2 fidlib into the OpenVnmrJ
    environment.  If your system has internet access, then entering
    $SCRIPT will do the installion.  Any currently installed fidlib
    will be backed up.

    $SCRIPT tests internet access by "pinging" www.dropbox.com.
    The ping command may also fail due to a firewall blocking it.
    If you are sure the system is connected to the internet
    and want to bypass this "ping" test, use
    $SCRIPT noPing

    If your system does not have internet access, then one can use a
    two-step process.

    On the system with internet access, use

    $SCRIPT -d <Path_To_A_Shared_Directory>

    This will download fidlib to a directory. Then on the system running
    OpenVnmrJ, provide access to the directory, for example, via a USB drive,
    and use

    $SCRIPT -i <Shared_Directory_Where_Files_Were_Downloaded>

options:
    -d|--download directory   Directory in which to download fidlib
    -h|--help                 Display this help information
    -i|--install directory    Directory from which to install fidlib
    -l|--log file             File for log messages. If a relative path name is
                              used the log file will be placed in /vnmr/fidlib
    -nv|--no-verbose          Turn off verbose output
    -v|--verbose              Use verbose output (default)
    -vv|--debug               Debug script

EOF
    exit 1
}

noPing=0

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -d|--download)          OVJ_DOWNLOAD="$2"; shift    ;;
        -h|--help)              ovj_usage                   ;;
        -i|--install)           OVJ_INSTALL="$2"; shift     ;;
        -l|--log)               OVJ_LOG="$2"; shift     ;;
        -nv|--no-verbose)       OVJ_VECHO=":" ;;
        -v|--verbose)           OVJ_VECHO="echo" ;;
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

url="https://www.dropbox.com/s/j1p7fu82dgwvlf1/vnmrjFidlib.zip?dl=0"

downloadMac() {
   $OVJ_VECHO "Downloading fidlib to MacOS file system"
   if [ "x${OVJ_LOG}" = "x" ] ; then
      $OVJ_VECHO "Downloading VnmrJ 4.2 fidlib"
      curl -L -O $url
   else
      $OVJ_VECHO "Downloading VnmrJ 4.2 fidlib"
      echo "Downloading VnmrJ 4.2 fidlib" >> ${OVJ_LOG}
      curl -L -O $url 2>> ${OVJ_LOG}
   fi
}

downloadLinux() {
   $OVJ_VECHO "Downloading fidlib to Linux file system"
   if [ "x${OVJ_LOG}" = "x" ] ; then
      $OVJ_VECHO "Downloading VnmrJ 4.2 fidlib"
      wget -nv $url
   else
      $OVJ_VECHO "Downloading VnmrJ 4.2 fidlib"
      echo "Downloading VnmrJ 4.2 fidlib" >> ${OVJ_LOG}
      wget  -nv $url 2>> ${OVJ_LOG}
   fi
}

downloadFiles() {
   $OVJ_VECHO "Download VnmrJ fidlib to $1"
   if [ -d "$1" ]; then
      cd "$1"
      touch "aDuMmYfIlE" >& /dev/null
      if [ -a  "aDuMmYfIlE" ]; then
         rm -f "aDuMmYfIlE"
         if [ "x${OVJ_LOG}" != "x" ] ; then
            if [ ${OVJ_LOG:0:1} = '/' ]; then
               $OVJ_VECHO "Log file is ${OVJ_LOG}"
            else
               $OVJ_VECHO "Log file is /vnmr/fidlib/${OVJ_LOG}"
            fi
         fi
         if [ x$(uname -s) = "xDarwin" ]; then
            downloadMac
         elif [ x$(uname -s) = "xLinux" ]; then
            downloadLinux
         else
            echo "Can only download VnmrJ fidlib with Linux or MacOS systems"
            return 1
         fi
      else
         echo "No write permission for directory $1"
         return 1
      fi
   else
      echo "Directory $1 does not exist"
      return 1
   fi
   return 0
}

checkNetwork() {
   local URL="www.dropbox.com"
   ping -c 1 -q -W 1 $URL > /dev/null 2>&1
   if [[ $? -eq 0 ]] || [[ $noPing -eq 1 ]] ; then
      $OVJ_VECHO "Test for internet access to $URL passed"
      return 0
   else
      echo "Internet access to $URL failed"
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

if [ "x${OVJ_INSTALL}" = "x" ] ; then
    checkNetwork
    if [[ $? -ne 0 ]]; then
        exit 1
    fi
fi
if [ "x${OVJ_DOWNLOAD}" != "x" ] ; then
    downloadFiles ${OVJ_DOWNLOAD}
    exit 0
fi
cd /vnmr/
rm -rf fidlibtmp 
mkdir fidlibtmp
if [ ! -d "/vnmr/fidlibtmp" ]; then
   echo "No permission to write to /vnmr."
   echo "Rerun as OpenVnmrJ system administrator (vnmr1)."
   return 1
fi
date=`date +%Y_%m_%d.%H:%M`
if [ -d /vnmr/fidlib ]; then
   $OVJ_VECHO "Saving current fidlib as fidlib_${date}"
   mv "fidlib" fidlib_${date}
fi

# A * is appended because dropbox adds characters to the end of the file name
filename="vnmrjFidlib.zip"

mv fidlibtmp  "fidlib"
cd "fidlib"
$OVJ_VECHO "VnmrJ fidlib installation started"
if [ "x${OVJ_INSTALL}" != "x" ] ; then
   $OVJ_VECHO "Copying VnmrJ fidlib from ${OVJ_INSTALL}"
   cp ${OVJ_INSTALL}/${filename}* .
else
   downloadFiles "/vnmr/fidlib"
fi
if [[ $? -ne 0 ]]; then
   cd ..
   $OVJ_VECHO " "
   if [ "x${OVJ_INSTALL}" != "x" ] ; then
      $OVJ_VECHO "Copying fidlib from ${OVJ_INSTALL} failed"
   else
      $OVJ_VECHO "fidlib download failed"
   fi
   $OVJ_VECHO " "
   rmdir fidlib
   if [[ -d fidlib_${date} ]]; then
      mv fidlib_${date} "fidlib"
      $OVJ_VECHO "Restoring previous version of fidlib"
   fi
   exit 1
fi
if [ x$(uname -s) = "xDarwin" ]; then
   ditto -x -k  ${filename}* /vnmr/fidlib
else
   unzip -q ${filename}*
fi
rm -f ${filename}*
$OVJ_VECHO ""
$OVJ_VECHO "VnmrJ 4.2 fidlib installation complete"
$OVJ_VECHO ""
exit 0
