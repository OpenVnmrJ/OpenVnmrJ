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
: ${OVJ_PIPETEST=0}

ovj_usage() {
    cat <<EOF

usage:
    $SCRIPT [options...]

    $SCRIPT will install NMRPipe into the OpenVnmrJ environment. If your system
    has intrenet access, then entering $SCRIPT will do the installion. Any currently
    installed NMRPipe will be backed up.

    If your system does not have internet access but does have access to shared
    directories with systems that do have internet access, then one can use a
    two-step process.

    On the system with internet access, use

    $SCRIPT -d <Path_To_A_Shared_Directory>

    This will download the files to a shared directory. Then on the system running
    OpenVnmrJ and has access to the shared directory, use

    $SCRIPT -i <Shared_Directory_Where_Files_Were_Downloaded>

options:
    -d|--download directory   Directory in which to download NMRPipe files
    -h|--help                 Display this help information
    -i|--install directory    Directory from which to install NMRPipe files
    -l|--log file             File for log messages
    -nv|--no-verbose)         Turn off verbose output
    -t|--test                 Perform nmmrPipe installation tests
    -v|--verbose)             Use verbose output (default)
    -vv|--debug)              Debug script

EOF
    exit 1
}

# process flag args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        -d|--download)          OVJ_DOWNLOAD="$2"; shift    ;;
        -h|--help)              ovj_usage                   ;;
        -i|--install)           OVJ_INSTALL="$2"; shift     ;;
        -l|--log)               OVJ_LOG="$2"; shift     ;;
        -nv|--no-verbose)       OVJ_VECHO=":" ;;
        -t|--test)              OVJ_PIPETEST="$2"; shift    ;;
        -v|--verbose)           OVJ_VECHO="echo" ;;
        -vv|--debug)            set -x ;;
        *)
            # unknown option
            echo "unrecognized argument: $key"
            ovj_usage
            ;;
    esac
    shift
done

downloadMac() {
   $OVJ_VECHO "Downloading NMRPipe files to MacOS file system"
   if [ "x${OVJ_LOG}" = "x" ] ; then
      $OVJ_VECHO "Downloading file 1 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/install.com
      $OVJ_VECHO "Downloading file 2 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/binval.com
      $OVJ_VECHO "Downloading file 3 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/NMRPipeX.tZ
      $OVJ_VECHO "Downloading file 4 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/s.tZ
      $OVJ_VECHO "Downloading file 5 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/dyn.tZ
      $OVJ_VECHO "Downloading file 6 of 7"
      curl -O https://www.ibbr.umd.edu/nmrpipe/talos.tZ
      $OVJ_VECHO "Downloading file 7 of 7"
      curl -O https://spin.niddk.nih.gov/bax/software/smile/plugin.smile.tZ
   else
      $OVJ_VECHO "Downloading file 1 of 7"
      echo "Downloading file 1 of 7 (install.com)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/install.com &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 2 of 7"
      echo "Downloading file 2 of 7 (binval.com)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/binval.com &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 3 of 7"
      echo "Downloading file 3 of 7 (NMRPipeX.tz)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/NMRPipeX.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 4 of 7"
      echo "Downloading file 4 of 7 (s.tz)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/s.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 5 of 7"
      echo "Downloading file 5 of 7 (dyn.tz)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/dyn.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 6 of 7"
      echo "Downloading file 6 of 7 (talos.tz)" &>> ${OVJ_LOG}
      curl -O https://www.ibbr.umd.edu/nmrpipe/talos.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 7 of 7"
      echo "Downloading file 7 of 7 (plugin.smile.tz)" &>> ${OVJ_LOG}
      curl -O https://spin.niddk.nih.gov/bax/software/smile/plugin.smile.tZ &>> ${OVJ_LOG}
   fi
}

downloadLinux() {
   $OVJ_VECHO "Downloading NMRPipe files to Linux file system"
   if [ "x${OVJ_LOG}" = "x" ] ; then
      $OVJ_VECHO "Downloading file 1 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/install.com
      $OVJ_VECHO "Downloading file 2 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/binval.com
      $OVJ_VECHO "Downloading file 3 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/NMRPipeX.tZ
      $OVJ_VECHO "Downloading file 4 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/s.tZ
      $OVJ_VECHO "Downloading file 5 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/dyn.tZ
      $OVJ_VECHO "Downloading file 6 of 7"
      wget https://www.ibbr.umd.edu/nmrpipe/talos.tZ
      $OVJ_VECHO "Downloading file 7 of 7"
      wget https://spin.niddk.nih.gov/bax/software/smile/plugin.smile.tZ
   else
      $OVJ_VECHO "Downloading file 1 of 7"
      echo "Downloading file 1 of 7 (install.com)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/install.com &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 2 of 7"
      echo "Downloading file 2 of 7 (binval.com)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/binval.com &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 3 of 7"
      echo "Downloading file 3 of 7 (NMRPipeX.tz)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/NMRPipeX.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 4 of 7"
      echo "Downloading file 4 of 7 (s.tz)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/s.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 5 of 7"
      echo "Downloading file 5 of 7 (dyn.tz)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/dyn.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 6 of 7"
      echo "Downloading file 6 of 7 (talos.tz)" &>> ${OVJ_LOG}
      wget  -nv https://www.ibbr.umd.edu/nmrpipe/talos.tZ &>> ${OVJ_LOG}
      $OVJ_VECHO "Downloading file 7 of 7"
      echo "Downloading file 7 of 7 (plugin.smile.tz)" &>> ${OVJ_LOG}
      wget  -nv https://spin.niddk.nih.gov/bax/software/smile/plugin.smile.tZ &>> ${OVJ_LOG}
   fi
}

downloadFiles() {
   local URL="www.ibbr.umd.edu"
   local pingRecv=0
   $OVJ_VECHO "Download NMRPipe files to $1"
   if [ -d "$1" ]; then
      cd "$1"
      touch "aDuMmYfIlE" >& /dev/null
      if [ -a  "aDuMmYfIlE" ]; then
         rm -f "aDuMmYfIlE"
         pingRecv=`ping -c 1 -q -W 1 $URL | grep received | awk '{ print $4 }'`
         
         if [ ${pingRecv} -eq 1 ]; then
            $OVJ_VECHO "Test for internet access to $URL passed"
            if [ x$(uname -s) = "xDarwin" ]; then
               downloadMac
            elif [ x$(uname -s) = "xLinux" ]; then
               downloadLinux
            else
               echo "Can only download NMRPipe files with Linux or MacOS systems"
               return 1
            fi
         else
            echo "Internet access to $URL failed"
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
}

cleanup() {
   cd /vnmr/nmrpipe
   rm -f ./install.com
   rm -f ./binval.com
   rm -f ./NMRPipeX.tZ
   rm -f ./s.tZ
   rm -f ./dyn.tZ
   rm -f ./talos.tZ
   rm -f ./plugin.smile.tZ
   if [ x`uname -s` = "xDarwin" ]; then
      rm -rf nmrbin.linux nmrbin.linux212_64 nmrbin.linux9 nmrbin.mac nmrbin.mac11
   elif [ x`uname -s` = "xLinux" ]; then
      rm -rf nmrbin.linux nmrbin.linux9 nmrbin.mac nmrbin.mac11 nmrbin.mac11_64
   fi
}
#
# Main program starts here
#

if [ "x${OVJ_DOWNLOAD}" != "x" ] ; then
    downloadFiles ${OVJ_DOWNLOAD}
    exit 1
fi
cd /vnmr/
rm -rf nmrpipetmp 
mkdir nmrpipetmp
if [ ! -d "/vnmr/nmrpipetmp" ]; then
   echo "No permission to write to /vnmr."
   echo "Rerun as OpenVnmrJ system administrator (vnmr1)."
   return 1
fi
date=`date +%Y_%m_%d.%H:%M`
if [ -d /vnmr/nmrpipe ]; then
   $OVJ_VECHO "Saving current NMRPipe as NMRPipe_${date}"
   mv nmrpipe nmrpipe_${date}
fi
mv nmrpipetmp  nmrpipe
cd nmrpipe
if [ "x${OVJ_INSTALL}" != "x" ] ; then
   $OVJ_VECHO "Copying NMRPipe files from ${OVJ_INSTALL}"
   cp ${OVJ_INSTALL}/install.com .
   cp ${OVJ_INSTALL}/binval.com .
   cp ${OVJ_INSTALL}/NMRPipeX.tZ .
   cp ${OVJ_INSTALL}/s.tZ .
   cp ${OVJ_INSTALL}/dyn.tZ .
   cp ${OVJ_INSTALL}/talos.tZ .
   cp ${OVJ_INSTALL}/plugin.smile.tZ .
else
   downloadFiles "/vnmr/nmrpipe"
fi
$OVJ_VECHO "NMRPipe installation started"
chmod a+r  *.tZ
chmod u+rx *.com
if [ ${OVJ_PIPETEST} -eq 1 ]; then
   if [ "x${OVJ_LOG}" = "x" ] ; then
      ./install.com +dest /vnmr/nmrpipe
   else
      ./install.com +dest /vnmr/nmrpipe &>> ${OVJ_LOG}
   fi
else
   if [ "x${OVJ_LOG}" = "x" ] ; then
      ./install.com +dest /vnmr/nmrpipe +nopost
   else
      ./install.com +dest /vnmr/nmrpipe +nopost &>> ${OVJ_LOG}
   fi
fi
cleanup
$OVJ_VECHO ""
$OVJ_VECHO "NMRPipe installation complete"
$OVJ_VECHO ""
