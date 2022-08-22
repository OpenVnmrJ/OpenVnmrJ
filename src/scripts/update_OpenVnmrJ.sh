#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 

#  Update OpenVnmrJ
#  First argument is location of previous VnmrJ 4.2 installation
#  Second argument is locations of OpenVnmrJ installation
#

# set -x

ovj=$1
prev=$2

if [ ! -d $prev ]
then
  if [ $# -lt 3 ]
  then
    echo "Update source directory $prev does not exist"
  fi
  exit 1
fi
vnmr_rev=`grep VERSION "$prev"/vnmrrev`
vnmr_rev_1=`echo $vnmr_rev | awk '{ print tolower($1) }'`
vnmr_rev_3=`echo $vnmr_rev | awk '{ print tolower($3) }'`
if [ $vnmr_rev_1 != "openvnmrj" ]
then
  if [ $vnmr_rev_3 != "4.2" ]
  then
    if [ $# -lt 3 ]
    then
      echo "Can only update OpenVnmrJ from an existing VnmrJ 4.2 or OpenVnmrJ installation"
    fi
    exit 1
  fi
fi
echo "Updating OpenVnmrJ from $prev"

if [ -d $prev/biopack/fidlib ] && [ -d $ovj/biopack ] && [ ! -d $ovj/biopack/fidlib ]
then
  echo "Collecting biopack files"
  cd $prev/biopack
  newdir=$ovj/biopack
  tar cf - bin BioPack.dir/BP_doc fidlib 2> /dev/null | (cd $newdir && tar xpf -)
fi
if [ -d $prev/help ] && [ ! -d $ovj/help ]
then
  echo "Collecting help"
  cd $prev
  newdir=$ovj
  tar cf - help | (cd $newdir && tar xpf -)
fi
if [ -d $prev/fidlib/Ethylindanone ] && [ ! -d $ovj/fidlib/Ethylindanone ]
then
  echo "Collecting fidlib"
  cd $prev
  newdir=$ovj
  rm -rf $newdir/fidlib
  tar cf - fidlib | (cd $newdir && tar xpf -)
fi

if [ ! -d $ovj/imaging ]
then
  exit 0
fi

if [ -d $prev/imaging/data ] && [ ! -d $ovj/imaging/data ]
then
  echo "Collecting imaging data"
  cd $prev/imaging
  newdir=$ovj/imaging
  tar cf - data | (cd $newdir && tar xpf -)
fi
