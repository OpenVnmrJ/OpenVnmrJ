: /bin/sh
# 
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

if [ -f $prev/psg/psgmain.cpp ]
then
  if [ -d $prev/acq/download -a ! -d $ovj/acq/download ]
  then
    echo "Collecting console files"
    cd $prev/acq
    newdir=$ovj/acq
    zip -ryq $newdir/b.zip download download3x
    cd $newdir
    unzip -qn b.zip
    rm b.zip
    cd $prev/acqbin
    newdir=$ovj/acqbin
    zip -ryq $newdir/b.zip testconf* consoledownload* flashia*
    cd $newdir
    unzip -qn b.zip
    rm b.zip
  fi
else
  if [ -d $prev/acq/vxBoot -a ! -d $ovj/acq/vxBoot ]
  then
    echo "Collecting console files"
    cd $prev/acq
    newdir=$ovj/acq
    zip -ryq $newdir/b.zip tms320dsp.ram vxB*
    cd $newdir
    unzip -qn b.zip
    rm b.zip
  fi
fi
if [ -d $prev/lib ]
then
  echo "Collecting libraries"
  cd $prev/lib
  newdir=$ovj/lib
  zip -ryq $newdir/b.zip *
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/bin ]
then
  echo "Collecting misc programs"
  cd $prev
  newdir=$ovj
  if [ -f bin/wkhtmltopdf -a ! -f $newdir/bin/wkhtmltopdf ]
  then
    cp bin/wkhtmltopdf $newdir/bin/wkhtmltopdf
  fi
  if [ -f bin/dcdump -a ! -f $newdir/bin/dcdump ]
  then
    cp bin/dcdump $newdir/bin/dcdump
  fi
  if [ -f bin/dciodvfy -a ! -f $newdir/bin/dciodvfy ]
  then
    cp bin/dciodvfy $newdir/bin/dciodvfy
  fi
  if [ -f bin/FdfToDcm -a ! -f $newdir/bin/FdfToDcm ]
  then
    cp bin/FdfToDcm $newdir/bin/FdfToDcm
  fi
  if [ -f bin/vjLCAnalysis -a ! -f $newdir/bin/vjLCAnalysis ]
  then
    cp bin/vjLCAnalysis $newdir/bin/vjLCAnalysis
  fi
  if [ -d kermit -a ! -d $newdir/kermit ]
  then
    cp -r kermit $newdir/.
  fi
fi
if [ -d $prev/craft -a -d $ovj/craft -a ! -d $ovj/craft/Bayes3 ]
then
  echo "Collecting craft files"
  cd $prev/craft
  newdir=$ovj/craft
  zip -ryq $newdir/b.zip Bayes3 clusterlib fidlib
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/biopack -a -d $ovj/biopack -a ! -d $ovj/biopack/fidlib ]
then
  echo "Collecting biopack files"
  cd $prev/biopack
  newdir=$ovj/biopack
  zip -ryq $newdir/b.zip bin BioPack.dir/BP_doc fidlib
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/nmrpipe -a ! -d $ovj/nmrpipe ]
then
  echo "Collecting nmrpipe"
  cd $prev
  newdir=$ovj
  zip -ryq $newdir/b.zip nmrpipe
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/help -a ! -d $ovj/help ]
then
  echo "Collecting help"
  cd $prev
  newdir=$ovj
  zip -ryq $newdir/b.zip help
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/fidlib -a ! -d $ovj/fidlib ]
then
  echo "Collecting fidlib"
  cd $prev
  newdir=$ovj
  zip -ryq $newdir/b.zip fidlib
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -d $prev/imaging/data -a ! -d $ovj/imaging/data ]
then
  echo "Collecting imaging data"
  cd $prev/imaging
  newdir=$ovj/imaging
  zip -ryq $newdir/b.zip data
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
if [ -f $prev/imaging/seqlib/gems -a ! -f $ovj/imaging/seqlib/gems ]
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
      if [ -f maclib/$mac -a ! -f $newdir/maclib/$mac ]
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
      if [ -d parlib/$par -a ! -d $newdir/parlib/$par ]
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
      if [ -f seqlib/$seq -a ! -f $newdir/seqlib/$seq ]
      then
        cp seqlib/$seq $newdir/seqlib/$seq
        cp psglib/"$seq".c $newdir/psglib/"$seq".c
      fi
      if [ -f templates/vnmrj/protocols/"$seq".xml -a ! -f $newdir/templates/vnmrj/protocols/"$seq".xml ]
      then
        cp templates/vnmrj/protocols/"$seq".xml $newdir/templates/vnmrj/protocols/"$seq".xml
      fi
      if [ -d templates/layout/"$seq" -a ! -d $newdir/templates/layout/"$seq" ]
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
      if [ -f templates/layout/default/$def -a ! -f $newdir/templates/layout/default/$def ]
      then
        cp templates/layout/default/$def $newdir/templates/layout/default/$def
      fi
      if [ -f templates/layout/fsems/$def -a ! -f $newdir/templates/layout/fsems/$def ]
      then
        cp templates/layout/fsems/$def $newdir/templates/layout/fsems/$def
      fi
  done
  cd $prev
  newdir=$ovj
  binList="   \
              echoscu    \
              storescp    \
              storescu    \
          "
  for def in $binList
  do
      if [ -f dicom/bin/$def -a ! -f $newdir/dicom/bin/$def ]
      then
        cp dicom/bin/$def $newdir/dicom/bin/$def
      fi
  done
fi
if [ -d $prev/sudo.lnx -a ! -d $ovj/sudo.lnx ]
then
  echo "Collecting sudo files"
  cd $prev
  newdir=$ovj
  zip -ryq $newdir/b.zip sudo.lnx
  cd $newdir
  unzip -qn b.zip
  rm b.zip
fi
