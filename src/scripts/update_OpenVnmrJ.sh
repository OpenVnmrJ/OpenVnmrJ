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
if [ -f $prev/imaging/seqlib/gems ] && [ ! -f $ovj/imaging/seqlib/gems ]
then
  echo "Collecting imaging files"
  cd $prev/imaging
  newdir=$ovj/imaging
  macList="   \
              acq2Dsense  \
              aip2Dmask   \
              aip2Dsense  \
              aip2Dsmap   \
              csi2d       \
              gems        \
              gemsll_prep \
              steam       \
              steami      \
          "

  for mac in $macList
  do
      if [ -f maclib/$mac ] && [ ! -f $newdir/maclib/$mac ]
      then
        cp maclib/$mac $newdir/maclib/$mac
      fi
  done
  parList="   \
              acsi2d.par    \
              csi2d.par     \
              csi3d.par     \
              epsi3d.par    \
              fepsi3d.par   \
              gemsir.par    \
              gemsll.par    \
              gems.par      \
              laser_csi.par \
              sgems.par     \
              steam2.par    \
              steamcsi.par  \
              steam.par     \
          "

  for par in $parList
  do
      if [ -d parlib/$par ] && [ ! -d $newdir/parlib/$par ]
      then
        cp -r parlib/$par $newdir/parlib/$par
      fi
  done
  seqList="   \
              acsi2d    \
              csi2d     \
              csi3d     \
              epsi3d    \
              fepsi3d   \
              gems      \
              gems_fc   \
              gemsir    \
              gemsll    \
              gemsshim  \
              laser_csi \
              sgems     \
              steam2    \
              steam     \
          "

  for seq in $seqList
  do
      if [ -f seqlib/$seq ] && [ ! -f $newdir/seqlib/$seq ]
      then
        cp seqlib/$seq $newdir/seqlib/$seq
        cp psglib/"$seq".c $newdir/psglib/"$seq".c
      fi
      if [ -f templates/vnmrj/protocols/"$seq".xml ] && [ ! -f $newdir/templates/vnmrj/protocols/"$seq".xml ]
      then
        cp templates/vnmrj/protocols/"$seq".xml $newdir/templates/vnmrj/protocols/"$seq".xml
      fi
      if [ -d templates/layout/"$seq" ] && [ ! -d $newdir/templates/layout/"$seq" ]
      then
        cp -r templates/layout/"$seq" $newdir/templates/layout/"$seq"
      fi
  done
  defList="   \
              acq2Dsense.xml    \
              proc2Dmask.xml    \
              proc2Dsense.xml   \
              proc2Dsmap.xml    \
          "
  for def in $defList
  do
      if [ -f templates/layout/default/$def ] && [ ! -f $newdir/templates/layout/default/$def ]
      then
        cp templates/layout/default/$def $newdir/templates/layout/default/$def
      fi
      if [ -f templates/layout/fsems/$def ] && [ ! -f $newdir/templates/layout/fsems/$def ]
      then
        cp templates/layout/fsems/$def $newdir/templates/layout/fsems/$def
      fi
  done
fi
