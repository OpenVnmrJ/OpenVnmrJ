#!/bin/sh
#
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

if [ x$workspacedir = "x" ]
then
   workspacedir=$HOME
fi

gitdir=$workspacedir/git-repo
vnmrdir=$gitdir/../vnmr
standardsdir=$gitdir/../options/standard
optionsdir=$gitdir/../options
optddrdir=$gitdir/../options/console/ddr
consoledir=
consoledir=$gitdir/../console

ShowPermResults=-100

Code="code"
Tarfiles="tarfiles"
 
#
# determine if pbzip2 is installed and is newer than v1.0.2
# pbzip2 allows parallel bzip2 compression using all the processor cores
#
usePBZIP2() {
   usepbzip='n'
   if [ -x /usr/sbin/pbzip2 -o -x /usr/bin/pbzip2 ] ; then
      # pbzip2 v1.0.3 and above work with pipes, v1.0.2 does not 
      revminor=`pbzip2 -V 2>&1 | awk 'BEGIN { FS = "." } /v1/ { print $3 }' | awk 'BEGIN { FS = " " } { print $1 }'`
      revmajor=`pbzip2 -V 2>&1 | awk 'BEGIN { FS = "." } /v1/ { print $2 }'`
     if [ $revmajor -ge 0 -a $revminor -gt 2 ] ; then
        usepbzip='y'
     fi
   else
     usepbzip='n'
   fi
   # echo "usepbzip: $usepbzip"
}

taroption="xfBp"
cpoption="-rp"

ostype=`uname -s`
ostype="Linux"

#
# files needed for loading from cd
#
SubDirs="				\
		rht			\
		tarfiles		\
		linux		\
		tmp			\
		"

RmOptFiles="				\
		rht/vnmrs.rht		\
		rht/vnmrs.opt		\
		rht/mr400.rht		\
		rht/mr400.opt		\
		rht/propulse.opt	\
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
           spaces=`expr $spaces - 1`
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
            indent=`expr $indent + 4`
         fi
         setperms $1/$setpermfile $dirperm $fileperm $execperm $indent
         indent=`expr $indent - 4`
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
tarring()
{
  #echo $1
  len=`expr length "$1"`
  #echo $len
  echo " " | tee -a $log_file
  echo -n " " | tee -a $log_file
  echo -n " " | tee -a $log_file
  echo -n " " | tee -a $log_file
  str=`echo "Tarring $1"`
  ns=`expr 22 - $len`
  #echo spaces: $ns
  echo -n $str | tee -a $log_file
  while [ $ns -gt 0 ];  do
    #str=`expr "$str"" "`
    echo -n " " | tee -a $log_file
    ns=`expr $ns - 1`
  done
  #echo \'$str\'
  echo -n "for:" | tee -a $log_file

}

#
# tar bzip2 options tarfilename dir2tar or files2tar
#  tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles${dir}.tar" "*"
#
tarbzip2() {
   if [ "x$usepbzip" = "xy" ] ; then
       # echo "tar $1 -cf $2 --use-compress-program pbzip2  $3"
       # tar $1 -cf $2 --use-compress-program pbzip2  $3    this varient does not owrk on RHEL 6.X so used the second form below  GMB.
       tar $1 -c $3 | pbzip2 -c >  $2
   else
       # echo "tar $1 -cjf $2 $3"
       tar $1 -cjf $2 $3
   fi
}

#---------------------------------------------------------------------------
#
# usage: getSize directory
#
# getSize $dest_dir/tmp
# getSize $dest_dir/jre.Linux
getSize()
{
    size_name=`du -sk $1`
    Size=`echo $size_name | awk 'BEGIN { FS = " " } { print $1 }'`
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
        systemname=`basename $i`
        nnl_echo "  $systemname" | tee -a $log_file
     done
     rm -rf $dest_dir_code/tmp/*
   )
}

nnl_echo() {
    
    case x$ostype in

	"x")
            echo "error in echo-no-new-line: ostype not defined"
            exit 1
            ;;

        "xSOLARIS")
            echo "$*\c"
            ;;

        "xLinux")
            echo -n "$*"
            ;;

        *)
            echo -n "$*"
            ;;
    esac
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
   nnl_echo "$1" | tee -a $log_file
}

findcore() {
   find . -name core -exec rm {} \;
}

create_support_dirs () {

   cd $1
   nnl_echo "$Code " | tee -a $log_file
   if [ ! -d $Code ]
   then
      mkdir -p $Code
   fi
   cd $Code
   dirs=$SubDirs
   for file in $dirs
   do
      nnl_echo "$file " | tee -a $log_file
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
      nnl_echo "$file " | tee -a $log_file
      rm -rf $file
      touch $file
   done
   rm -rf $dest_dir_code/tmp/*
}                                                                                                 
drop_vnmrs_ () {
   vnmrs_list=`ls vnmrs_*`
   for file in $vnmrs_list 
   do
      newfln=`echo $file | cut -c7-`
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

# determine if pbipz2 is installed
usePBZIP2
interact="no"
console="vnmrs"

   DefaultDestDir="$gitdir/../$dvdBuildName1"
   DefaultFiniDir="none"
   DefaultLogDir="$gitdir/../logs"
   DefaultLogFile="$gitdir/../logs/gitcdoutlog"
   log_file=$DefaultLogFile
   rm -rf $log_file
   echo "Writing log to '$log_file' file"
   dest_dir=$DefaultDestDir
   echo ""
   useDasho="y"
   notifySW="n"

distro=`lsb_release -is`    # RedHatEnterpriseWS; Ubuntu


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
echo "M a k i n g   V n m r J   C D R O M   I m a g e" | tee -a $log_file

echo $VnmrRevId  | tee -a $log_file
echo `date '+%B %d, %Y'` | tee -a $log_file


#============== COMMON FILES =============================================
echo "" | tee -a $log_file
log_this  "PART I -- COMMON TAR FILES -- $dest_dir_code/$Tarfiles"
# Let's copy and tar the Common files and log it.
vnmrList=`ls $vnmrdir`

for dir in  $vnmrList 
do

      #log_this "   Tarring $dir		for : "
      tarring "$dir"
      cd $vnmrdir
      tar --exclude=.gitignore -cf - ${dir} | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      getSize $dest_dir_code/tmp
      makeTOC ${dir}.tar $Vnmr  rht/vnmrs.rht	\
				rht/mr400.rht   \
                                rht/propulse.rht
done

# add postgress
#log_this "   Tarring $dir		for : "
      dir="pgsql"
      tarring "$dir"
      cd $OVJ_TOOLS
      tar --exclude=.gitignore -cf - ${dir} | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      getSize $dest_dir_code/tmp
      makeTOC ${dir}.tar $Vnmr  rht/vnmrs.rht	\
				rht/mr400.rht   \
                                rht/propulse.rht

#============== CONSOLE SPECIFIC FILES ===============================
if [ x$consoledir != "x" ]
then

echo "" | tee -a $log_file
log_this  "PART IA -- CONSOLE TAR FILES -- $dest_dir_code/$Tarfiles"
  dir="ddr"
      tarring "$dir"
      cd ${consoledir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar $Vnmr  rht/vnmrs.rht   \
                                rht/mr400.rht   \
                                rht/propulse.rht
fi

#============== STANDARD OPTIONS FILES ===============================
echo "" | tee -a $log_file
log_this  "PART II -- STANDARD OPTIONS TAR FILES -- $dest_dir_code/$Tarfiles"
standardList=`ls $standardsdir`
for dir in $standardList
do
      #log_this "   Tarring $dir		for : "
      tarring "$dir"
      cd ${standardsdir}/${dir}
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption -)

      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      getSize $dest_dir_code/tmp
      # tar --exclude=.gitignore --exclude=SConstruct* -cjf $dest_dir_code/$Tarfiles/${dir}.tar *
      tarbzip2 "--exclude=.gitignore --exclude=SConstruct" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar "$Subject"  rht/vnmrs.rht	\
				     rht/mr400.rht      \
                                     rht/propulse.rht
done

echo "" | tee -a $log_file
log_this  "PART III -- CONSOLE SPECIFIC STANDARD OPTIONS TAR FILES -- $dest_dir_code/$Tarfiles"
optList=`ls $optddrdir`
for dir in $optList
do
      #log_this " Tarring $dir           for : "
      tarring "$dir"
      cd $optddrdir/$dir
      tar -cf - * | (cd $dest_dir_code/tmp; tar $taroption - )
      cd $dest_dir_code/tmp
      setperms ./ 755 644 755
      getSubject ${dir}
      #tar --exclude=.gitignore -cjf  $dest_dir_code/$Tarfiles/${dir}.tar *
      tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}_ddr.tar" "*"
      makeTOC "${dir}_ddr.tar" "$Subject"  rht/vnmrs.rht   \
                                rht/mr400.rht   \
                                rht/propulse.rht

done

optionsList=`ls $optionsdir`
for dir in $optionsList
do

case x$dir in
   xbin | xpassworded | xstandard | xconsole )
   #  do nothing here
   ;;

   xcode )
      log_this " Tarring install files "
      echo "" | tee -a $log_file
       cd $optionsdir/$dir
       tar --exclude=.gitignore -cf - * | (cd $dest_dir_code; tar $taroption -)
       cp $gitdir/src/scripts/ins_vnmr2.sh $dest_dir_code/ins_vnmr
       chmod 755 $dest_dir_code/ins_vnmr
       cp $gitdir/src/scripts/vjpostinstallaction.sh $dest_dir_code/vjpostinstallaction
       chmod 755 $dest_dir_code/vjpostinstallaction
       cp $gitdir/src/common/user_templates/.??* $dest_dir_code/.
       cd $dest_dir
       rm -rf load.nmr
#      setup nolonger used.
#       mv code/setup setup
       ln -s code/vnmrsetup load.nmr
       ln -s code/rpmInstruction.txt rpmInstruction.txt
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
      tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/${dir}.tar" "*"
      makeTOC ${dir}.tar $Vnmr  rht/vnmrs.rht   \
                                rht/mr400.rht   \
                                rht/propulse.rht
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
       # tarbzip2 "--exclude=.gitignore" "$dest_dir_code/$Tarfiles/jre.tar" "jre"
       cd $dest_dir_code
       tar xpf $Tarfiles/jre.tar
       rm -rf jre.linux
       mv jre jre.linux
       getSize $dest_dir_code/jre.linux
       makeTOC jre.tar $Vnmr rht/vnmrs.rht      \
                             rht/mr400.rht      \
                             rht/propulse.rht
       rm $Tarfiles/jre.tar     # exception

#Nirvana CD only
touch $dest_dir_code/.nv

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
     cp $gitdir/src/vnmrj/src/vnmr/images/vnmrsjsu.png icon/propulse.png
     cp $gitdir/src/gif/vnmrs.gif icon/.
     cp $gitdir/src/gif/mr400.gif icon/.
     cp $gitdir/src/vnmrj/src/vnmr/images/mr400dd2.png icon/.
     cp $gitdir/src/vnmrj/src/vnmr/images/vnmrsdd2.png icon/.
     cd $dest_dir_code/rht
     cp $gitdir/src/gif/install .
     chmod 777 $dest_dir_code/rht
     chmod 666 install
     cp mr400.opt mr400dd2.opt
     cp mr400.rht mr400dd2.rht
     cp vnmrs.opt vnmrsdd2.opt
     cp vnmrs.rht vnmrsdd2.rht

   VnmrRevId=`grep RevID ${gitdir}/src/vnmr/revdate.c | cut -s -d\" -f2`
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
   
echo "DONE == ovjout == ` date +"%F %T"` ==" | tee -a $log_file
