#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 

# Default Declarations
#

SCRIPT="$(basename "$0")"
onerror() {
    echo "$(tput setaf 1)$SCRIPT: Error on line ${BASH_LINENO[0]}, exiting.$(tput sgr0)"
    exit 1
}
trap onerror ERR

if [ x$workspacedir = "x" ]
then
   workspacedir=$HOME
fi

gitdir=$workspacedir/OpenVnmrJ
vnmrdir=$gitdir/../vnmr
standardsdir=$gitdir/../options/standard
optionsdir=$gitdir/../options
optmdir=$gitdir/../options/console/mercury
optidir=$gitdir/../options/console/inova
consoledir=
consoledir=$gitdir/../console

ShowPermResults=-100

Code="code"
Tarfiles="tarfiles"
 
taroption="xfBp"
cpoption="-rp"

#
# files needed for loading from cd
#
SubDirs="				\
		rht			\
		tarfiles		\
		linux		        \
		tmp			\
		"

RmOptFiles="				\
                rht/inova.rht           \
                rht/inova.opt           \
                rht/mercplus.rht        \
                rht/mercplus.opt        \
		"
#---------------------------------------------------------------------------
setperms()
{
   if [ $# -lt 4 ]
   then
     echo 'Usage - setperms "directory name" "dir permissions" "file permissions" "executable permissions"'
     echo ' E.g. "setperms /sw2/cdimage/code/tmp/wavelib 775 655 755" or "setperms /common/wavelib g+rx g+r g+x" '
     exit 0
   fi
   dirperm=$2
   fileperm=$3
   execperm=$4
   
   
   if [ $# -lt 5 ]
   then
      if [ $ShowPermResults -gt 0 ]
      then
         echo "" 
      fi
      indent=0
   else
      indent=$5
   fi
   
   pars=`(cd $1; ls)`
   for setpermfile in $pars
   do
      #indent to proper place
      if [ $ShowPermResults -gt 0 ]
      then
         spaces=$indent
         pp=""
         while [ $spaces -gt 0 ]
         do
           pp='.'$pp
           spaces=$(( spaces - 1 ))
         done
      fi
   
      #test for director, file, executable file
      if [ -d $1/$setpermfile ]
      then
         if [ $ShowPermResults -gt 0 ]
         then
            echo "${pp}chmod $dirperm $setpermfile/"
         fi
         chmod $dirperm $1/$setpermfile
         if [ $ShowPermResults -gt 0 ]
         then
            indent=$(( indent + 4 ))
         fi
         setperms $1/$setpermfile $dirperm $fileperm $execperm $indent
         indent=$(( indent - 4 ))
      elif [ -f $1/$setpermfile ]
      then
         if [ -x $1/$setpermfile ]
         then
            if [ $ShowPermResults -gt 0 ]
            then
                echo "${pp}chmod $execperm $setpermfile*"
            fi
            chmod $execperm $1/$setpermfile
         else
            if [ $ShowPermResults -gt 0 ]
            then
               echo "${pp}chmod $fileperm $setpermfile"
            fi
            chmod $fileperm $1/$setpermfile
         fi
#      else
#         echo file:  $1/$setpermfile not modified
      fi
   done
}

# routine to force "Tarring xyz    for:" to same length
tarring () { printf " \n   Tarring %-21s for:" "$1" | tee -a $log_file ; }

#
#  tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles${dir}.tar" "*"
#
tarIt() {
   # echo "tar $1 -cjf $2 $3"
   tar "$1" -cjf "$2" $3
}

#---------------------------------------------------------------------------
#
# usage: getSize directory
#
# getSize $dest_dir/tmp
# getSize $dest_dir/jre.Linux
getSize()
{
    size_name=$(du -sk $1)
    Size=$(echo $size_name | awk 'BEGIN { FS = " " } { print $1 }')
}

getSubject()
{
   case x$1 in
     x3D_shimming )
       Subject="3D_Shimming"
     ;;

     xBackprojection )
       Subject="Backprojection"
     ;;
     
     xBiosolids )
       Subject="Biosolidspack"
     ;;

     xBIR )
       Subject="BIR_Shapes"
     ;;
     
     xcraft )
       Subject="CRAFT"
     ;;

     xCSI )
       Subject="CSI"
     ;;
     
     xdicom )
       Subject="Imaging_or_Triax"
     ;;

     xDiffusion )
       Subject="Diffusion"
     ;;
     
     xDOSY )
       # Subject="DOSY"
       Subject="DOSY_for_VnmrJ"
     ;;

     xGilson )
       Subject="VAST"
     ;;
     
     xGradient_Shimm )
       Subject="VNMR"
     ;;
     
     xIMAGE )
       Subject="Imaging_or_Triax"
     ;;

     xLC )
       Subject="LC-NMR"
     ;;

     xMR400FH )
       Subject="VNMR"
     ;;

     xP11 )
      # Subject="P11"
      Subject="Secure_Environments"
      ;;
     
     xpipe )
       Subject="VnmrJ_NMRPipe"
     ;;

     xSolids )
       Subject="VNMR"
     ;;
     
     xSTARS )
       Subject="STARS"
     ;;
     
     xVAST )
       Subject="VAST"
     ;;

     xMDD )
       Subject="MDD"
     ;;

     xservicetools )
       Subject="Servicetools"
     ;;

    xVnmrJ )
       Subject="VNMR"
     ;;

     * )
       Subject=$1
     ;;
  esac
}

#---------------------------------------------------------------------------
#
# usage: makeTOC  <tarfile> <category>  <TOCfile TOCfile ...>
#
#   tarfile:   com.tar, jre.tar ...,    these are NMR sofware packages
#   category:  VNMR, JMol, userlib ..., Nmr package names to be selected 
#				        when loading Nmr software
#   TOCfile:   rht/vnmrs.rht rht/mr400.rht, ..., these files contain a list
#					of needed sofware for particular system
makeTOC()
{
   ( #usage: makeTOC  <tarfile>  <category>  <TOCfile TOCfile ...>
     tfile=$1
     shift
     cat=$1
     shift
     flist=$*
     dir=$Tarfiles

     for i in $flist
     do
        echo "${cat} $Size $Code/$dir/$tfile" >> $dest_dir_code/$i
        systemname=$(basename $i)
        echo -n "  $systemname" | tee -a $log_file
     done
     rm -rf $dest_dir_code/tmp/*
   )
}

log_this(){

   if [ ! -d $dest_dir_code/tmp ]
   then
       mkdir -p $dest_dir_code/tmp
   else
       rm -rf $dest_dir_code/tmp/*
   fi

   echo "" | tee -a $log_file
   echo "" | tee -a $log_file
   echo -n "$1" | tee -a $log_file
}

create_support_dirs () {

   cd $1
   echo -n "$Code " | tee -a $log_file
   if [ ! -d $Code ]
   then
      mkdir -p $Code
   fi
   cd $Code
   dirs=$SubDirs
   for file in $dirs
   do
      echo -n "$file " | tee -a $log_file
      if [ ! -d $file ]
      then
         mkdir -p $file
      fi
   done
   echo "" | tee -a $log_file
   echo "" | tee -a $log_file
   echo "Clearing *.opt files and tmp:" | tee -a $log_file
   cd $dest_dir_code
   for file in $RmOptFiles
   do
      echo -n "$file " | tee -a $log_file
      rm -rf $file
      touch $file
   done
   rm -rf $dest_dir_code/tmp/*
}                                                                                                 
drop_vnmrs_ () {
   vnmrs_list=$(ls vnmrs_*)
   for file in $vnmrs_list 
   do
      newfln=$(echo $file | cut -c7-)
      rm -f $newfln
      mv $file $newfln
   done
}


#############################################################
#              MAIN Main main
#############################################################

curr_dir=`pwd`

LoadVnmrJ="y"


Vnmr="VNMR"

interact="no"
console="vnmrs"

   DefaultDestDir="$gitdir/../$dvdBuildName2"
   DefaultFiniDir="none"
   DefaultLogDir="$gitdir/../logs"
   DefaultLogFile="$gitdir/../logs/gitmicdoutlog"
   log_file=$DefaultLogFile
   rm -rf $log_file
   echo "Writing log to '$log_file' file"
   dest_dir=$DefaultDestDir
   echo ""
   useDasho="y"
   notifySW="n"

# create log directory if not present
if [ ! -d $DefaultLogDir ]
then
      mkdir -p $DefaultLogDir
fi



echo ""
dest_dir_code=$dest_dir/$Code

echo
echo "log_file	= $log_file +++++++++++++"
echo "dest_dir	= $dest_dir +++++++++++++"
echo
echo "notifySW	= $notifySW +++++++++++++"
#echo "com_answer	= $com_answer +++++++++++++"
#echo "par_answer	= $par_answer +++++++++++++"
#echo "gnu_answer	= $gnu_answer +++++++++++++"
#echo "user_answer	= $user_answer +++++++++++++"
echo
echo

if [ ! -d $dest_dir ]
then
   mkdir -p $dest_dir
fi

if [ ! -r $log_file ]
then
   touch $log_file
fi

echo "Writing files to '$dest_dir'" | tee -a $log_file
echo "" | tee -a $log_file
echo "Creating needed subdirectories:" | tee -a $log_file

create_support_dirs $dest_dir

echo "" | tee -a $log_file
echo `date` | tee -a $log_file
echo "" | tee -a $log_file
echo "M a k i n g   O p e n V n m r J   M I   D V D   I m a g e" | tee -a $log_file

echo $VnmrRevId  | tee -a $log_file
echo $(date '+%B %d, %Y') | tee -a $log_file


#============== COMMON FILES =============================================
echo "" | tee -a $log_file
log_this  "PART I -- COMMON TAR FILES -- $dest_dir_code/$Tarfiles"
# Let's copy and tar the Common files and log it.
vnmrList=$(ls $vnmrdir)

for dir in  $vnmrList 
do

      #log_this "   Tarring $dir		for : "
      tarring "$dir"
      cd $vnmrdir
      tar --exclude=.gitignore -cf - ${dir} | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      if [[ $dir = "maclib" ]]; then
         rm -f maclib/mtune_ddr
         rm -f maclib/_sw_ddr
      fi
      setperms ./ 755 644 755
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      getSize $dest_dir_code/tmp
      makeTOC ${dir}.tar $Vnmr  rht/inova.rht	\
                                rht/mercplus.rht
done

#============== CONSOLE SPECIFIC FILES ===============================
if [ x$consoledir != "x" ]
then

echo "" | tee -a $log_file
log_this  "PART IA -- CONSOLE TAR FILES -- $dest_dir_code/$Tarfiles"
      dir="inova"
      tarring "$dir"
      cd ${consoledir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar $Vnmr  rht/inova.rht
      dir="mercury"
      tarring "$dir"
      cd ${consoledir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar $Vnmr  rht/mercplus.rht
fi

#============== STANDARD OPTIONS FILES ===============================
echo "" | tee -a $log_file
log_this  "PART II -- STANDARD OPTIONS TAR FILES -- $dest_dir_code/$Tarfiles"
standardList=$(ls $standardsdir)
for dir in $standardList
do
  case x$dir in
    xBiosolids | xGilson | xIMAGE | xLC )              # Inova only
      #log_this "   Tarring $dir		for : "
      tarring "$dir"
      cd ${standardsdir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore --exclude=SConstruct* -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore --exclude=SConstruct" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar "$Subject"  rht/inova.rht
   ;;

      * )
      #log_this "   Tarring $dir		for : "
      tarring "$dir"
      cd ${standardsdir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore --exclude=SConstruct* -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore --exclude=SConstruct" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar "$Subject"  rht/inova.rht	\
                                     rht/mercplus.rht
   ;;
   esac
done

echo "" | tee -a $log_file
log_this  "PART III -- CONSOLE SPECIFIC STANDARD OPTIONS TAR FILES -- $dest_dir_code/$Tarfiles"
optList=$(ls $optmdir)
for dir in $optList
do
      #log_this " Tarring $dir           for : "
      tarring "$dir"
      cd $optmdir/$dir
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption - )
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      #tar --exclude=.gitignore -cjf  $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}_merc.tar" "*"
      makeTOC "${dir}_merc.tar" "$Subject"  rht/mercplus.rht
done

optList=$(ls $optidir)
for dir in $optList
do
      #log_this " Tarring $dir           for : "
      tarring "$dir"
      cd $optidir/$dir
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption - )
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      #tar --exclude=.gitignore -cjf  $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}_inova.tar" "*"
      makeTOC "${dir}_inova.tar" "$Subject"  rht/inova.rht
done

optionsList=$(ls $optionsdir)
for dir in $optionsList
do

case x$dir in
   xbin | xpassworded | xstandard | xconsole | xBackprojection | xBiosolids )
   #  do nothing here
   ;;

   xcode )
      log_this " Tarring install files "
      echo "" | tee -a $log_file
       cd $optionsdir/$dir
       tar --exclude=.gitignore -cf - * | (cd $dest_dir_code; tar $taroption -)
       cp $gitdir/src/scripts/ins_vnmr2.sh $dest_dir_code/ins_vnmr
       chmod 755 $dest_dir_code/ins_vnmr
       cp $gitdir/src/scripts/upgrade.nmr.sh $dest_dir/upgrade.nmr
       chmod 755 $dest_dir/upgrade.nmr
       cp $gitdir/Notes.txt $gitdir/Install.md $dest_dir/.
       chmod 644 $dest_dir/Notes.txt $dest_dir/Install.md
       cp $gitdir/src/scripts/vjpostinstallaction.sh $dest_dir_code/vjpostinstallaction
       chmod 755 $dest_dir_code/vjpostinstallaction
       cp $gitdir/src/common/user_templates/.??* $dest_dir_code/.
       cp $gitdir/src/common/user_templates/profile $dest_dir_code/.
       cd $dest_dir
       rm -rf load.nmr
#      setup nolonger used.
#       mv code/setup setup
       ln -s code/vnmrsetup load.nmr
       ln -s code/rpmInstruction.txt rpmInstruction.txt
       ln -s code/installpkgs ovjGetRepo
       cp $gitdir/src/scripts/ovjUseRepo.sh ovjUseRepo
       chmod +x ovjUseRepo
   ;;

   xlicense )
      log_this " Tarring license files "
      echo "" | tee -a $log_file
      cd $optionsdir/
      tar --exclude=.gitignore -cf - $dir | (cd $dest_dir; tar $taroption -)
   ;;

   * )
      #log_this " Tarring $dir           for : "
      tarring "$dir"
      cd $optionsdir/$dir
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption - )
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      #tar --exclude=.gitignore -cjf  $dest_dir_code/$Tarfiles/${dir}.tar *
      tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar $Vnmr  rht/inova.rht   \
                                rht/mercplus.rht
   ;;
esac
done
       

#============== INSTALLATION FILES =========================================
echo "" | tee -a $log_file
log_this "PART IV -- INSTALLATION FILES -- $dest_dir"
# copy some of the installation programs
#Add jre to DVD

       dir="jre"
      #log_this "   Tarring jre                     for : "
       tarring "$dir"
       cd $OVJ_TOOLS/java
       tar --exclude=.gitignore -cf $dest_dir_code/$Tarfiles/jre.tar jre
       # tarIt "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/jre.tar" "jre"
       cd $dest_dir_code
       tar xpf $Tarfiles/jre.tar
       rm -rf jre.linux
       mv jre jre.linux
       getSize $dest_dir_code/jre.linux
       makeTOC jre.tar $Vnmr rht/inova.rht      \
                             rht/mercplus.rht
       rm $Tarfiles/jre.tar     # exception

#
#  VJ cdrom only
#
   echo "" | tee -a $log_file
   echo " Copying icons" | tee -a $log_file
   cd $dest_dir_code
   if [ ! -d $dest_dir_code/icon ]
   then
      mkdir -p $dest_dir_code/icon
   fi
     cp $gitdir/src/gif/inova.gif icon/.
     cp $gitdir/src/gif/mercplus.gif icon/.
     cd $dest_dir_code/rht
     cp $gitdir/src/gif/installMI install
     chmod 777 $dest_dir_code/rht
     chmod 666 install

   VnmrRevId=$(grep RevID ${gitdir}/src/vnmr/revdate.c | cut -s -d\" -f2)
   RevFileName="OpenVnmrJ"`echo $VnmrRevId | awk '{print $3}'`
   cd $dest_dir_code/../
   echo " Writing Revision File '$RevFileName':"  | tee -a $log_file
   echo "$VnmrRevId" > vnmrrev
   echo "`date '+%B %d, %Y'`" >> vnmrrev
   cat vnmrrev | tee -a $log_file
   echo " "
   echo " "
#   echo "CD Build ID, based on Current git sha1 "  | tee -a $log_file
#   echo " "
#   cat $vnmrdir/adm/sha1/CD_Build_Id.txt | tee -a $log_file
#   echo " "
   rm -f  $RevFileName
   ln -s vnmrrev $RevFileName

#Create a system checksums file to validate Part11 system
if [ x$LoadP11 = "xy" ]
then

    mkdir -p $dest_dir_code/tmp
    cd $dest_dir_code/tmp
     
    tar xvf $dest_dir_code/$Tarfiles/combin.tar
    tar xvf $dest_dir_code/$Tarfiles/vnmrj.tar
    tar xvf $dest_dir_code/$Tarfiles/vnmrjbin.tar
    tar xvf $dest_dir_code/$Tarfiles/java.tar
    tar xvf $dest_dir_code/$Tarfiles/vnmrjadmjar.tar
    tar xvf $dest_dir_code/$Tarfiles/wobbler.tar
    tar xvf $dest_dir_code/$Tarfiles/acqbin2.tar
    tar xvf $dest_dir_code/$Tarfiles/acqbin.tar
    tar xvf $dest_dir_code/$Tarfiles/bin.tar
    tar xvf $dest_dir_code/$Tarfiles/unibin.tar
    tar xvf $dest_dir_code/$Tarfiles/binx.tar

    mkdir -p adm/p11
    #bin/vnmrMD5 -l /vcommon/p11/sysList vnmrsystem > adm/p11/syschksm

    #pack checksum file together within com.tar
    #tar -rf $dest_dir_code/$Tarfiles/com.tar  adm/p11/syschksm

    cd $dest_dir_code
    rm -rf tmp
fi

#---------------------------------------------------------------------------
   
echo "DONE == ovjmiout == ` date +"%F %T"` ==" | tee -a $log_file
