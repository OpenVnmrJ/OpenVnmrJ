: '@(#)loadcd2.sh 22.1 03/24/08 1991-1994 '
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
#! /bin/sh
: '@(#)loadcd.sh 13.1 10/10/97 1991-1997 '
# This is the start-up script for loading Vnmr software
# load.nmr first determinaes which system is running (SunOS, Solaris
# IRX, IRIX64 or AIX). Based on that it will gather the right files
# to pass to `ins.xxx' for installation choices.
#
# Determine OS type, can be overwritten by first argument.
#
if [ $# -eq 1 ]
then
   ostype=$1
else
   ostype=`uname -s`
   if [ $ostype = "SunOS" ]
   then
      rev=`uname -r`
      if [ $rev -ge 5.3 ]
      then
	 ostype="SOLARIS"
      fi
   fi
fi
#
# check if we are really root
#
user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
if [ x$user != "xroot" ]
then
   echo "You must be root to load VNMR and its options."
   echo "Become root then start load.nmr again."
   exit 1
fi


#
# construct the source directory
#
pwd=`pwd`
dir=`dirname $0`
if [ x$dir = "x." ]
then
   source_dir=$pwd
else
   source_dir=$pwd/$dir
fi
source_dir_code=$source_dir/code
nmr_adm="vnmr1"
nmr_group="nmr"
export nmr_adm nmr_group ostype

case $ostype in
   SunOS )
	osexten="sos"
	export osexten
	$source_dir_code/ins.sos 0 inova.sos inova.opt
	;;
   SOLARIS )
	osexten="sol"
	export osexten
	$source_dir_code/ins.sol 0 mercvx.sol mercvx.opt
	;;
   AIX )
	osexten="ibm"
	export osexten
	$source_dir_code/ins.ibm 0 ibm.ibm ibm.opt
	;;
   IRIX | IRIX64)
	osexten="sgi"
	export osexten
	$source_dir_code/ins.sgi 0 sgi.sgi sgi.opt
	;;
esac

