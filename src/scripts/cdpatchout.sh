: '@(#)cdpatchout.sh 22.1 03/24/08 1999-2002 '
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
DefaultLogFln="/vnmrcd/patchcdromlog"
DefaultDestDir="/vnmrcd/patchcdimage"
DefaultFiniDir=`date '+/rdvnmr/.cdrompatch%m.%d'`


echo ""
echo "M a k i n g   S o l a r i s   P a t c h   C D R O M"
echo ""
umask 2
   echo "^GUse an absolute path for log !!"
   echo "This script changed directory many times"
   echo "And will write the log in that directory^G"
   echo "Enter destination for log   [$DefaultLogFln]: \c"
   read answer
   if [ x$answer = "x" ]
   then
      log_fln=$DefaultLogFln
   else
      log_fln=$answer
   fi
   if test -f $log_fln
   then
      echo "'$log_fln' exists, overwrite? [y]: \c"
      read answer
      if [ x$answer = "x" ]
      then
         answer="y"
      fi
      if [ x$answer != "xy" ]
      then
         exit
      fi
      rm -rf $log_fln
   fi
   echo "Writing log to '$log_fln'"
   echo ""
#
# Make nice heading in log file
#
echo ""  > $log_fln
echo "L o g   F o r   M a k i n g   C D R O M - I m a g e   F o r   P a t c h"  >> $log_fln
echo ""  >> $log_fln
#
#
# ask for destination  directory
#
   echo "enter destination directory [$DefaultDestDir]: \c"
   read answer
   if [ x$answer = "x" ]
   then
      dest_dir=$DefaultDestDir
   else
      dest_dir=$answer
   fi
   if  test -d $dest_dir
   then
      echo "'$dest_dir' exists, overwite? [y]: \c"
      read answer
      if [ x$answer = "x" ]
      then
         answer="y"
      fi
      if [ x$answer != "xy" ]
      then
         abort
      fi
   else
      mkdir -p $dest_dir
   fi
   echo ""
   echo "Writing files to '$dest_dir'" | tee -a $log_fln
   dest_dir_code=$dest_dir/$Code



   echo "enter Finial directory [$DefaultFiniDir]: \c"
   read answer
   if [ x$answer = "x" ]
   then
      fini_dir=$DefaultFiniDir
   else
      fini_dir=$answer
   fi

   if [ x$fini_dir = "xnone" ]
   then
      echo ""
      echo "No Write to Finial Directory will be made. " | tee -a $log_fln
      echo ""
   else

      if  test  -d $fini_dir
      then
         echo "'$fini_dir' exists, overwite? [y]: \c"
         read answer
         if [ x$answer = "x" ]
         then
            answer="y"
         fi
         if [ x$answer != "xy" ]
         then
            abort
         fi
      else
         mkdir -p $fini_dir
         if  [ ! -d $fini_dir ]
         then
            echo "Could not create Final Directory: $fini_dir, Aborting. " | tee -a $log_fln
            exit 1
         fi
      fi
      echo ""
      echo "Writing results to Final Directory: $fini_dir "| tee -a $log_fln
 fi

 echo ""
 echo ""

 cd $dest_dir
 if test ! -d patch 
 then
    mkdir patch
 fi
 
# echo "volstart " | tee -a $log_fln
# cp $sourcedir/sysscripts/volstartpatch $dest_dir/volstart
# chmod 777 $dest_dir/volstart
 
 echo "solsetup " | tee -a $log_fln
 cp $sourcedir/sysscripts/solsetup $dest_dir/load.patches
 chmod 777 $dest_dir/load.patches
 
 echo "solpatchupdate " | tee -a $log_fln
 cp $commondir/bin/solpatchupdate	patch/
 chmod 777 patch/solpatchupdate

#
#===PATCHES ====================================================
  
# copy Solaris patches
   echo "" | tee -a $log_fln
   echo "Patches for Solaris" | tee -a $log_fln
   rm -rf $dest_dir/patch/Sol*
   cd /vcommon
   tar -chf - patch | (cd $dest_dir; tar xfBp -)

#
#===COPY TO /RDVNMR  ===========================================
 if [ x$fini_dir != "xnone" ]
   then
     echo "Write CD Image to Destination Place: $fini_dir" | tee -a $log_fln
     cd $dest_dir
     tar -cf - . | (cd $fini_dir; tar xfBp -)
fi

