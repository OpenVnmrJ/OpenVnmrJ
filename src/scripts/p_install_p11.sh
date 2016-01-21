#! /bin/sh
# '@(#)p_install.sh 22.1 03/24/08 1991-2004 '
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

###p_install "file to install" "dir, where to process" "dir, where to save"

patch_process_dir=$2
patch_saved_dir=$3
fix_psg="no"
acq_reboot="no"

mv $patch_process_dir/p_remove $patch_saved_dir

head -20 $patch_saved_dir/p_remove > $patch_saved_dir/p_remove.tmp 

for File in $1
do

  if [ x$patch_category = "xall" ]
  then
     check=`echo $File | awk 'BEGIN { FS = "/" } { print $2 }'`
     case "x$check" in

        "xacq" )  acq_reboot="yes"
                 ;;
        "xpsg" )  fix_psg="yes"
                 ;;
            * )  #Do nothing
                 ;;
     esac
  fi


  if [ -f $vnmrsystem/$File ]
  then
      mv $vnmrsystem/$File $patch_saved_dir  #save the original for changing mind later
      mv $patch_process_dir/$File $vnmrsystem/$File  #This is the bug fixed file
      echo "mv `basename $vnmrsystem/$File` `dirname $vnmrsystem/$File`" >> $patch_saved_dir/p_remove.tmp
  else

     newdir=`dirname $vnmrsystem/$File`
     if [ ! -d $newdir ]
     then
        mkdir -p $newdir
     fi

     mv $patch_process_dir/$File $vnmrsystem/$File  #This is the bug fixed file
     echo "mv `basename $vnmrsystem/$File` `dirname $vnmrsystem/$File`" >> $patch_saved_dir/p_remove.tmp
  fi
done

tail -n +21 $patch_saved_dir/p_remove >> $patch_saved_dir/p_remove.tmp
mv $patch_saved_dir/p_remove.tmp $patch_saved_dir/p_remove  

#To ensure that Vnmr and Expproc have a correct permission
#if [ -f $vnmrsystem/acqbin/Expproc ]
#then
#    chmod 6775 $vnmrsystem/bin/Vnmr
#    chmod 6775 $vnmrsystem/acqbin/Expproc
#else
#    chmod 775 $vnmrsystem/bin/Vnmr
#fi

#Linux system does not have Vnmr program
#if [ -f $vnmrsystem/bin/Vnmr ]
#then
#    chmod 775 $vnmrsystem/bin/Vnmr
#fi

#cd $vnmrsystem/bin
#case $p_console_type in
#        gem )  if [ -f ./vconfig ]
#               then 
#                  rm ./vconfig
#                  ln -s gconfig vconfig 
#               fi
#               ;;
#        mer )  if [ -f ./vconfig ]
#               then 
#                  rm ./vconfig
#                  ln -s kconfig vconfig 
#               fi
#               ;;
#          * )
#               ;;
#esac

#the next lines for acq category only
#ls /tftpboot/vxBoot | grep sym > /dev/null
#if [ $? != 0 ]
#then
#    echo "big kernel"
#    cp $vnmrsystem/acq/vxBoot.big/* /tftpboot/vxBoot
#else
#    echo "small kernel"
#    cp $vnmrsystem/acq/vxBoot.small/* /tftpboot/vxBoot
#fi

#cp $vnmrsystem/acq/apmon /tftpboot  #need to be root #uniqe to mercury
#for inova have to find out kernel's size (small or big) before copying to /tftpboot
#chmod -R 755 $patch_saved_dir #might not need if permission set properly

if [ x$fix_psg = "xyes" ]
then
    echo ""
    $vnmrsystem/bin/fixpsg
    echo ""
fi

if [ x$acq_reboot = "xyes" ]
then
    #load_kernel
    echo ""
    echo ""
    echo "Please, login as root and run $vnmrsystem/bin/setacq"
fi


# Part 11
if [ -f $vnmrsystem/p11/part11Config ]
then
    echo "Please enter this system's root user password"
    su root -c "$vnmrsystem/bin/p_install_p11_as_root"
fi

