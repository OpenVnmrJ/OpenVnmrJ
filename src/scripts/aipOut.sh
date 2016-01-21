#!/bin/bash
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

# Set action
if [ $# -lt 1 ] ; then
  echo aipOut: must have at least \"action\" argument 
  exit
fi
action=$1


# fdf2nifti and fdf2avw
if [[ $action = fdf2nifti ]] || [[ $action = fdf2avw ]]; then

  # aipOut "action" "-in" "indir" "-out" "outdir"
  if [ $# -lt 5 ] || [[ $2 != -in ]] || [[ $4 != -out ]] ; then
    echo aipOut: $action requires \"-in\" \"indir\" \"-out\" \"outdir\" arguments in order
    exit
  fi
  indir=$3
  outdir=$5

  # Check indir exists
  if [ ! -d $indir ] ; then
    echo aipOut: input directory $indir does not exist
    exit
  fi

  # Set output extension .nii for fdf2nifti or .avw for fdf2avw
  if [[ $action = fdf2nifti ]] ; then 
    outext=".nii"
  else
    outext=".avw"
  fi

  # Set data test
  test="[^:punct:]fdf"

  # Set input extension
  inext="*.img"

  # Make the output directory if it doesn't exist
  if [ ! -d $outdir ] ; then
    mkdir $outdir
  fi

  # Convert data including sub-directories up to 5 levels down
  dir=""                                                            # initialize dir
  dirext="/*"                                                       # initialize dirext
  for (( i=0; i<6; i++ )) do                                        # work through sub-directories up to 5 levels down
    for j in $indir$dir ; do                                        # loop through $indir + (/*) x i
      if [ -d "$j" ] ; then                                         # if $indir + (/*) x i is a directory
        if [ "$(ls -A $j)" ] ; then                                 # if $indir + (/*) x i is not empty directory
          data=`ls $j | grep -o $test`                              # check $indir + (/*) x i for test data
          if [ "${#data}" -gt "0" ] ; then                          # if $indir + (/*) x i contains test data
            dirpath=$j                                              # set dirpath equal to the directory
            for (( k=i; k>0; k-- )) do                              # loop to strip components and store in an array
              tmp=`basename $dirpath`                               # get final component of dirpath
              dirpath=${dirpath%/*}                                 # strip the final component from dirpath
              if [[ "$tmp" = $inext ]] || [[ $k = $i ]] ; then      # if the component has a $inext extension
                out[$k]=${tmp%.*}                                   # strip the $inext extension and store in 'out' array
                out[$k]=${out[$k]}$outext                           # add $outext extension to 'out' array
              else                                                  # otherwise
                out[$k]=$tmp                                        # just leave the final component as is
              fi
              out[$k]="/"${out[$k]}                                 # add leading / to 'out' array
            done
            outpath=""                                              # initialize outpath
            for (( k=1; k<=i; k++ )) do                             # loop through 'out' array
              outpath=$outpath${out[$k]}                            # set outpath
              if [[ $k < $i ]] && [ ! -d $outdir$outpath ] ; then   # if directory doesn't exist 
                echo ""
                echo making $outdir$outpath                         # make it
                mkdir $outdir$outpath
              fi
            done
            # Run the conversion
            echo ""
            echo aipOut: executing $action -indir $j -outdir $outdir$outpath 
            echo ""
            $action -indir $j -outdir $outdir$outpath
          fi
        fi
      fi
    done
    dir=$dir$dirext                                                 # add trailing /* to dir
  done

fi


# FdfToDcm and dicom_store
if [[ $action = FdfToDcm ]] || [[ $action = FdfToDcmToServer ]]; then

  # aipOut "action" "-fdf"/"-fid" ["-np"] "-in" "indir" "-out" "outdir" ["-cfg" "confdir"]
  if [ $# -lt 6 ] || ([[ $2 != -fdf ]] && [[ $2 != -fid ]]) ; then
    echo aipOut: $action action requires the following arguments in order:
    echo '        '\"-fdf\"/\"-fid\" [\"-np\"] \"-in\" \"indir\" \"-out\" \"outdir\" [\"-cfg\" \"confdir\"] 
    exit
  fi
  type=$2
  ptag=""
  confdir=""
  if [[ $3 = -np ]] ; then
    ptag=$3
    if [ $# -lt 7 ] || [[ $4 != -in ]] || [[ $6 != -out ]] ; then    
      echo aipOut: $action action requires the following arguments in order:
      echo '        '\"-fdf\"/\"-fid\" [\"-np\"] \"-in\" \"indir\" \"-out\" \"outdir\" [\"-cfg\" \"confdir\"] 
      exit
    fi
    indir=$5
    outdir=$7
    if [ $# -eq 9 ] && [[ $8 = -cfg ]] ; then
      confdir=$9
    fi
  else
    if [[ $3 != -in ]] || [[ $5 != -out ]] ; then    
      echo aipOut: $action action requires the following arguments in order:
      echo '        '\"-fdf\"/\"-fid\" [\"-np\"] \"-in\" \"indir\" \"-out\" \"outdir\" [\"-cfg\" \"confdir\"] 
      exit
    fi
    indir=$4
    outdir=$6
    if [ $# -eq 8 ] && [[ $7 = -cfg ]] ; then
      confdir=$8
    fi
  fi

  # Check indir exists
  if [ ! -d $indir ] ; then
    echo aipOut: input directory $indir does not exist
    exit
  fi

  # For dicom2server make output directory
  if [ ! -d $outdir ] ; then
    echo making $outdir
    mkdir $outdir
  fi

  # Set output extension to .dcm
  outext=".dmc"

  # Set fid directory extension, data test and input extension according to type (fdf or fid)
  if [[ $type = -fdf ]] ; then
    fid=""
    test="[^:punct:]fdf"
    inext="*.img"
  else
    fid=".fid"
    test="fid"
    inext="*.fid"
    # Strip .fid extension since it will always get added to search fid directories
    if [[ "$indir" = $inext ]] ; then
      indir=${indir%.*}
    fi
  fi

  # Convert data including sub-directories up to 5 levels down
  dir=""                                                            # initialize dir
  dirext="/*"                                                       # initialize dirext
  for (( i=0; i<6; i++ )) do                                        # work through sub-directories up to 5 levels down
    for j in $indir$dir$fid ; do                                    # loop through $indir + (/*) x i [.fid]
      if [ -d "$j" ] ; then                                         # if $indir + (/*) x i [.fid] is a directory
        if [ "$(ls -A $j)" ] ; then                                 # if $indir + (/*) x i [.fid] is not empty directory
          data=`ls $j | grep -o $test`                              # check $indir + (/*) x i [.fid] for test data
          if [ "${#data}" -gt "0" ] ; then                          # if $indir + (/*) x i [.fid] contains test data
            dirpath=$j                                              # set dirpath equal to the directory
            for (( k=i; k>0; k-- )) do                              # loop to strip components and store in an array
              tmp=`basename $dirpath`                               # get final component of dirpath
              dirpath=${dirpath%/*}                                 # strip the final component from dirpath
              if [[ "$tmp" = $inext ]] || [[ $k = $i ]] ; then      # if component has a $inext extension
                out[$k]=${tmp%.*}                                   # strip the $inext extension and store in 'out' array
                out[$k]=${out[$k]}$outext                           # add $outext extension to 'out' array
              else                                                  # otherwise
                out[$k]=$tmp                                        # just leave the component as is
              fi
              out[$k]="/"${out[$k]}                                 # add leading / to 'out' array
            done
            outpath=""                                              # initialize outpath
            for (( k=1; k<=i; k++ )) do                             # loop through 'out' array
              outpath=$outpath${out[$k]}                            # set outpath
              if [ ! -d $outdir$outpath ] ; then                    # if directory doesn't exist 
                echo ""
                echo making $outdir$outpath                         # make it
                mkdir $outdir$outpath
              fi
            done
            # Run the conversion
            echo ""
            echo aipOut: executing FdfToDcm $ptag -if $j -of $outdir$outpath 
            echo ""
            FdfToDcm $ptag -if $j -of $outdir$outpath
            # Store on server
            if [[ $action = FdfToDcmToServer ]]; then
              echo aipOut: executing dicom_store $outdir$outpath $confdir
              dicom_store $outdir$outpath $confdir
            fi
          fi
        fi
      fi
    done
    dir=$dir$dirext                                                 # add trailing /* to dir
  done

  if [[ $action = FdfToDcmToServer ]]; then
    echo ""
    echo aipOut: executing rm -rf $outdir
    rm -rf $outdir
  fi

fi
