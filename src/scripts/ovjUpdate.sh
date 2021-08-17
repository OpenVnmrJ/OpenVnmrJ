#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
# set -x

#-----------------------------------------------
# Collect files for use with a new installation
# At this time, it does not deal with VAST, LC, or probeid

collectFiles()
{
   echo "Collecting system and printer information"
   if [[ -r conpar ]]; then
      cp conpar $toDir/conpar.prev
   fi
   cp devicenames $toDir/.
   cp devicetable $toDir/.
   echo "Collecting solvent information"
   cp solventlist $toDir/.
   cp solventppm $toDir/.
   cp solvents $toDir/.
   echo "Collecting probe information"
   tar -cf - probes | (cd $toDir; tar -xpf -)
   rm -f $toDir/probes/probe.tmplt
   echo "Collecting shim information"
   tar -cf - shims | (cd $toDir; tar -xpf -)

   if [ -r gshimdir ]; then
      echo "Collecting gradient shim information"
      tar -cf - gshimdir | (cd $toDir; tar -xpf -)
   fi

#   if [[ -r probeid ]]; then
#      if [[ -e ./probeid/cache ]]; then
#         if [ ! -h ./probeid/mnt ]; then
#            zip -ry $toDir/probeid.zip ./probeid/cache)
#         else
#            zip -ry $toDir/probeid.zip ./probeid/mnt ./probeid/cache)
#         fi
#      fi
#   fi
   if [ -d imaging/gradtables ]; then
      echo "Collecting imaging gradient information"
      mkdir $toDir/imaging
      if [ -f imaging/coilIDs ]; then
         tar -cf - imaging/gradtables imaging/coilIDs imaging/grad.tmplt | (cd $toDir; tar -xpf -)
         # replace 156_100S with 156_100_HD in coilIDs
         (  cd "$toDir"/imaging;
            grep 156_100S coilIDs >  /dev/null;
            if [ $? -eq 0 ]; then
               sed 's/156_100S/156_100_HD/' coilIDs > coilIDs.new
               mv coilIDs.new coilIDs
               rm -f gradtables/156_100S
            fi
         )

      else
         tar -cf - imaging/gradtables | (cd $toDir; tar -xpf -)
         rm -f gradtables/156_100S
      fi
   fi
   if [ -d imaging/decclib ]; then
      echo "Collecting imaging ECC information"
      if [ ! -d ${toDir}/imaging ]; then
         mkdir $toDir/imaging
      fi
      tar -cf - imaging/decclib | (cd $toDir; tar -xpf -)
   fi
   if [ -d fastmap ]; then
      echo "Collecting fastmap information"
      tar -cf - fastmap | (cd $toDir; tar -xpf -)
   fi
   if [ -d mollib ]; then
      echo "Collecting mollib information"
      tar -cf - mollib | (cd $toDir; tar -xpf -)
   fi

   if [ -d cryo/cryomon ]; then
      echo "Collecting cryomon information"
      tar -cf - cryo/cryomon cryo/probecal | (cd $toDir; tar -xpf -)
   fi

   if [ -d amptables ]; then
      echo "Collecting amp table information"
      tar -cf - amptables | (cd $toDir; tar -xpf -)
   fi

   if [ -d adm/users ]; then
      echo "Collecting user and operator information"
      list="adm/users/group adm/users/profiles adm/users/userlist adm/users/operators"
      if [[ -f adm/users/properties ]]; then
         list="$list adm/users/properties"
      fi
      if [[ -f adm/users/administrators ]]; then
         list="$list adm/users/administrators"
      fi
      tar -cf - $list | (cd $toDir; tar -xpf -)
      ( cd "$toDir"/adm/users/operators; 
        files="\
             automation.conf \
             automation.en.conf \
             automation.ja.conf \
              "
        for file in $files
        do
           if [ -f $file ]; then
              grep ChangeTime $file > /dev/null;
              if [ $? -ne 0 ]; then
                 cat $file | sed 's/SampleReuse/ChangeTime     180\
SampleReuse/' > auto.conf2
                 mv auto.conf2 $file
              fi
              grep Agilent $file > /dev/null;
              if [ $? -eq 0 ]; then
                 cat $file | grep -v "# Copyright" |
                             grep -v "# This software" |
                             grep -v "# information of" |
                             grep -v "# Use, " |
                             grep -v "# prior" > auto.conf2
                 mv auto.conf2 $file
              fi
           fi
        done
      )
   fi
   if [ -d dicom/conf ]; then
      echo "Collecting dicom information"
      tar -cf - dicom/conf | (cd $toDir; tar -xpf -)
   fi
   if [[ -d gshimlib ]]; then
      echo "Collecting gradient shimlib information"
      tar -cf - gshimlib | (cd $toDir; tar -xpf -)
   fi
   if [ -d tune/methods ]; then
      echo "Collecting tune information"
         # Don't copy over the standard methods or Qtune stuff,
    # but include the probe files (which can have pretty much any name).
      cat >$toDir/tune.exclude <<EOF
tune/methods
tune/manifest
tune/manual
tune/OptimaFirmware/Optima.bin
EOF
      tar -cf - -X $toDir/tune.exclude tune| (cd $toDir; tar -xpf -)
      rm -f $toDir/tune.exclude
   fi
   if [ -d adm/accounting/accounts ]; then
      echo "Collecting accounting information"
      tar -cf - adm/accounting/accounts | (cd $toDir; tar -xpf -)
      afile=$toDir/adm/accounting/accounts/accounting.prop
      if [ -f $afile ]; then
         grep -s agilentlogo $afile > /dev/null
         if [ $? -eq 0 ]; then
            sed 's/agilentlogo/vnmrjNameBW/' $afile > $afile.new
            mv $afile.new $afile
         fi
      fi
   fi
   if [ -f adm/accounting/acctLog.xml ]; then
      tar -cf - adm/accounting/acctLog.xml | (cd $toDir; tar -xpf -)
   fi
   if [ -f adm/accounting/loggingParamList ]; then
      tar -cf - adm/accounting/loggingParamList | (cd $toDir; tar -xpf -)
   fi
   if [ -f msg/noHelpOverlay ]; then
      mkdir $toDir/msg
      touch $toDir/msg/noHelpOverlay
   fi
   if [[ -f pgsql/persistence/LocatorOff ]] ; then
      mkdir -p $toDir/pgsql/persistence
      touch $toDir/pgsql/persistence/LocatorOff
   fi
   echo "Vnmrbg -mback -n- -s config auto > /dev/null" > $toDir/p_install
}
#-----------------------------------------------
#  Main MAIN main program starts here
#-----------------------------------------------

test="aA.ptc"
touch $test >& /dev/null
if [[ -f $test ]]; then
  rm $test
else
  echo "You need to run this command from a directory in which you have write permission"
  exit 1
fi
patch="custom_previousInstall"
rm -f ${patch}.ptc
echo "Enter the vnmr directory from which to collect the files [/vnmr] ? "
read vnmr
if [[ "x$vnmr" = "x" ]]; then
   vnmr="/vnmr"
fi
if [[ ! -d $vnmr ]]; then
   echo "The $vnmr directory does not exist"
   exit 1
fi
toDir=/tmp/ovjBkup
if [[ -d ${toDir} ]]; then
   rm -rf $toDir
fi
mkdir ${toDir}
(cd $vnmr; collectFiles)
echo "Making patch"
/vnmr/bin/patchmake $toDir ${patch} > /dev/null
rm -rf $toDir
dir=$(pwd)
echo " "
echo "Move the $dir/${patch}.ptc file to the new system and run"
echo " patchinstall ${patch}.ptc"
echo " "

exit 0
