: '@(#)makeP11checksums.sh 22.1 03/24/08 2003-2004 '
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
#!/bin/sh

if test $# -lt 1
then
   echo Usage: makeP11checksums /vnmr vnmr1 nmr
   echo    or: makeP11checksums /vnmr
   echo    or: makeP11checksums path
   exit
fi

# from ins_vnmr (or installation 3 aargs are given
# but not invoked as SUDO thus below it will ask for input
# this set vnmrsystem and exports it if 3 args given the 1st being vnmrsystem aka /vnmr
# someone who knows how and when this script is called for P11 need to clean up these conflicting tests.
# GMB 8/13/2012
if [ $# -gt 2 ] ; then
   if [ x"$vnmrsystem" = "x" ] ; then
      vnmrsystem=$1    # /vnmr
      export vnmrsystem
   fi
fi


if test x"$vnmrsystem" = "x"
then
    # with 5.3 the vnmrsystem env is not set when VnmrJ adm sudo is executed
    # thus test if this has been envoke as a sudo command
    # could test SUDO_USER, SUDO_GID, or SUDO_COMMAND
    # if SUDO_USER has a value then don't ask for vnmrsystem just default 
    # to /vnmr     GMB/GRS 8/10/2009
    if [ "x" == "x$SUDO_USER" ]; then
       echo -n  "Please enter location of VNMR system directory [/vnmr]: "
       read vnmrsystem
       if test x"$vnmrsystem" = "x"
       then
           vnmrsystem="/vnmr"
       fi
    else
       vnmrsystem="/vnmr"
    fi
    export vnmrsystem

fi

if test ! -d "$vnmrsystem"
then
    echo "$vnmrsystem does not exist, cannot proceed:"
    exit
fi

if test $1 = "/vnmr"
then

    nmr_adm=vnmr1
    nmr_group=nmr
    if test $# -gt 2
    then
       nmr_adm=$2 
       nmr_group=$3 
    fi
    
    cd /vnmr 
    cp /usr/varian/sbin/vnmrMD5 /tmp

    p11Config=/vnmr/p11/part11Config
    if [ -f $p11Config ]
    then
      rm -rf /vnmr/p11/checksums/*
      grep ":checksum:" $p11Config | while read line
      do
        path=`echo $line | awk 'BEGIN { FS = ":" } {if($2=="checksum") print $3}'`
  
        if [ -d $path ]
        then
          /usr/varian/sbin/makeP11checksums $path
        else
           if [ -f $path ] 
           then
             /usr/varian/sbin/makeP11checksums $path
           fi
        fi

      done
      if [ -d /vnmr/p11/checksums ]
      then
         chown -R ${nmr_adm}:${nmr_group} /vnmr/p11/checksums
      fi
    fi

    admp11dir=/vnmr/adm/p11
    if [ ! -d $admp11dir ]
    then
       mkdir $admp11dir
    fi

    sysListAll=/vnmr/adm/p11/sysListAll
    if [ -f $sysListAll ]
    then
       rm -f $sysListAll
    fi

    /vnmr/bin/chVJlist -l /vnmr/bin > $sysListAll
    /vnmr/bin/chVJlist -l /vnmr/p11/bin >> $sysListAll
    /vnmr/bin/chVJlist -l /usr/varian/sbin >> $sysListAll
    /vnmr/bin/chVJlist -l /vnmr/adm/bin >> $sysListAll
    /vnmr/bin/chVJlist -l /vnmr/acqbin >> $sysListAll
    /vnmr/bin/chVJlist -l /vnmr/java >> $sysListAll
    /vnmr/bin/chVJlist -l /vnmr/maclib >> $sysListAll

    sysList=/vnmr/adm/p11/sysList
    if [ -f $sysList ]
    then
       rm -f $sysList
    fi
    /vnmr/bin/chVJlist -l /vnmr/bin > $sysList
    /vnmr/bin/chVJlist -l /vnmr/p11/bin >> $sysList
    /vnmr/bin/chVJlist -l /usr/varian/sbin >> $sysList
    /vnmr/bin/chVJlist -l /vnmr/adm/bin >> $sysList
    /vnmr/bin/chVJlist -l /vnmr/acqbin >> $sysList
    /vnmr/bin/chVJlist -l /vnmr/java >> $sysList

    notice=/vnmr/adm/p11/notice
    if [ ! -f  $notice ]
    then
        touch $notice
    fi

    syschksm=/vnmr/adm/p11/syschksm
    /tmp/vnmrMD5 -l $sysList $vnmrsystem > $syschksm
    rm /tmp/vnmrMD5

    chown ${nmr_adm}:${nmr_group} $sysListAll $sysList $notice $syschksm
    chmod 644 $sysListAll $sysList $notice $syschksm

else

   if [ ! -d $1 ]
   then
      if [ ! -f $1 ] 
      then
	echo Error: file $1 does not exist.
        exit	
      fi
   fi

   name=`basename $1`
   dir=/vnmr/p11/checksums/$name

   if [ ! -d $dir ]
   then
      mkdir -p $dir
   fi

   str=`date '+%T-%m-%d-%y'`
   path=$dir/checksum.$str

   /usr/varian/sbin/vnmrMD5 -f $1 $1 $path

   echo $path
fi
