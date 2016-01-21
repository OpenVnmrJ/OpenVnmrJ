: '@(#)setacq.sh 22.1 03/24/08 1991-1996 '
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
: /bin/sh

make_etc_files () {

# this creates the acqpresent file in /etc

    (cd /etc; cat /dev/null >acqpresent)

# this creates the statpresent file in /etc

    (cd /etc; cat /dev/null >statpresent)
}

# SunOS-only script function

rm_sh_dev() {

    if (test -c /dev/rsh0)
    then
        rm -f /dev/rsh0
    fi
}

# SOLARIS-ONLY Script Function

make_devlink() {
    touch /etc/devlink.tab
    if test $? -ne 0
    then
        echo "error:  no write access to /etc/devlink.tab"
    else
        echo "type=ddi_pseudo;name=sh;minor=character	rsh\N0" >>/etc/devlink.tab
    fi

#  The new entry for devlink.tab has a <TAB> between "character" and "rsh"

}

# SOLARIS-ONLY Script Function

#  A UNITYplus HAL will fool the Streaming Tape driver into thinking it (the HAL)
#  is a Tape.  We want to remove entries from st.conf that  specify a Tape at SCSI
#  target address 2 or 3.

#  We first locate st.conf, in either /kernel/drv (likely) or /usr/kernel/drv
#  (less likely).  The awk program locates those entries with target = 2 or 3.
#  Because entries typically span more than one line, the Field Separator is
#  set to NewLine while the Record Separator is set to an empty line.  If the
#  first character of each field in a selected entry is the pound sign, then
#  the script concludes this entry has already been removed and proceeds as
#  if it has not been selected.
#

fix_stconf() {
    if test -f /kernel/drv/st.conf
    then
        st_conf_path="/kernel/drv/st.conf"
    else
        if test -f /usr/kernel/drv/st.conf
        then
            st_conf_path="/usr/kernel/drv/st.conf"
        else
            return 0
        fi
    fi

    awk '
    BEGIN { line = 0; prevl="" }
    /target=[23]/	{
				if (line > 0) {
      					fchar = substr( prevl, 1, 1 )
					if (fchar != "#")
					 prevl = "#" prevl
					print prevl
				}

				prevl = $0
      				fchar = substr( prevl, 1, 1 )
				if (fchar != "#")
				  prevl = "#" prevl

				line = line+1
				break
			}

#  The break statement serves to skip the next section.
#  without it control falls through to this default section.

			{
				if (line > 0)
				  print prevl
				prevl = $0
				line = line+1
			}

#  do not forget to print out the last line of the file

    END { print prevl }
    ' <$st_conf_path >st.conf_tmp
    mv -f st.conf_tmp $st_conf_path
}

#  verify the current process is root


case `uname -r` in
    4*) user=`whoami`
        ;;
    5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
        ;;
esac

if (test ! $user = "root")
then
   echo "Please login as Root"
   echo "then restart $0"
   exit 1
fi
 
# set current working directory
#
# remove dependence on presence of a "/vnmr" link or directory for script
# check that pwd is a vnmr directory
# then make changes relative to this.
#                                               Greg B.
vnmrdir=`pwd`
cdir=`basename "$vnmrdir"`
if (test x"$cdir" != "xbin")
then
   echo "Current working directory is '$vnmrdir'."
   echo "Please change to the vnmr/bin directory."
   echo "then restart $0"
   exit 1
fi
 
# Operating System
# Note that Solaris (on sun hardware) calls itself SunOS
# use OS revision to distinguish between the two

OS_NAME=`uname -s`
if (test ! $OS_NAME = "SunOS")
then
   echo "$0 suitable for Sun-based systems only"
   echo "$0 exits"
   exit 1
fi

osmajor=`uname -r | awk 'BEGIN { FS = "." } { print $1 }'`
if test $osmajor != "4"
then

#  Solaris-only...
#  Verify programs are present

    if test ! -s ../solkernel/sh 
    then
        echo "Error:  Software driver for spectrometer console is not present"
        echo "$0 exits"
    fi
    if test ! -s ../solkernel/sh.conf
    then
        echo "Error:  Configuration file for spectrometer console is not present"
        echo "$0 exits"
    fi

#  remove entries from st.conf that could be the HAL...

    fix_stconf

#  Verify /etc/devlink.tab file
#  Required for /dev/rsh0 to get created automatically

    grep "type=ddi_pseudo;name=sh" /etc/devlink.tab >devlink.temp
    if test ! -s devlink.temp
    then
        make_devlink
    fi
    rm -f devlink.temp

#  Remove preexisting HAL driver

    halmodno=`modinfo | awk '$6 == "sh" { print $1 }'`
    if test $halmodno
    then
        modunload -i $halmodno
        rem_drv sh
    fi

    rm -f /kernel/drv/sh /kernel/drv/sh.conf
    cp -p ../solkernel/sh      /kernel/drv/sh
    cp -p ../solkernel/sh.conf /kernel/drv/sh.conf

    echo "Installing NMR Console software ...  this takes a moment"

    hal_ok=0
    for scsi_id in 2 3
    do
        (cd /kernel/drv;						\
         sed 's%target=.%target='$scsi_id'%' sh.conf >sh.conf_new;	\
         mv -f sh.conf_new sh.conf;					\
        )

        add_drv sh 2>/dev/null
        mt -f /dev/rsh0 rewind 2>/dev/null
        if [ $? -eq 0 ]
        then
            hal_ok=1
            break
        fi

        rem_drv sh
    done

    if [ x$hal_ok = x0 ]
    then
        echo "NMR Console not found"
        echo "Check Spectrometer Console and Differential Box"
        exit
    fi

    real_hal=`ls -l /dev/rsh0 | awk '{ print $NF }'`
    (cd /dev; chmod 666 $real_hal)

# Arrange for Acqproc to start at system bootup

    cp -p ../rc.vnmr /etc/init.d
    (cd /etc/rc3.d; if [ ! -f S19rc.vnmr ]; then \
	ln -s ../init.d/rc.vnmr S19rc.vnmr; fi)
    (cd /etc/rc0.d; if [ ! -f K19rc.vnmr ]; then \
	ln -s ../init.d/rc.vnmr K19rc.vnmr; fi)
    make_etc_files
    
    echo "NMR Console software installation complete"

    exit
fi

##########################################################
#  Remainder of this script covers SunOS only, not Solaris
##########################################################

#
#   make HAL device entry
#
#   make /dev/rsh0
#   for SUN-3 V3.5, major device number for acquisition interface is 45
#   for SUN-4 V3.2, major device number for acquisition interface is 41
#   for SUN-3 or SUN-4, V4.0.3; major device number for acquisition interface is 68
#   for SPARCstation 1, V4.0.3c;  major device number is 72
#   for V4.1 and V4.1.1, major device number for acquisition interface is 104
#   for V4.1.2, major device number for acquisition interface is 105
#   for V4.1.3, major device number for acquisition interface is 108 for sun4c,
#     109 for sun4m
#   for V4.1.4, major device number for acquisition interface is 110

# Only handles SUN OS V4.1.1, V4.1.2 and V4.1.3

SOS_LEVEL=`uname -r`

if (test $SOS_LEVEL = "4.1.1")
then
    rm_sh_dev
    mknod /dev/rsh0 c 104 0
    chmod 666 /dev/rsh0
else
    if (test $SOS_LEVEL = "4.1.2")
    then
       rm_sh_dev
       mknod /dev/rsh0 c 105 0
       chmod 666 /dev/rsh0
    else
       if (test $SOS_LEVEL = "4.1.3" -o $SOS_LEVEL = "4.1.3C" -o $SOS_LEVEL = "4.1.3_U1")
       then
           rm_sh_dev
           if (test `arch -k` = "sun4m")
           then
               mknod /dev/rsh0 c 109 0
           else
               mknod /dev/rsh0 c 108 0
           fi
           chmod 666 /dev/rsh0
       else
           if (test $SOS_LEVEL = "4.1.4")
           then
               rm_sh_dev
               mknod /dev/rsh0 c 110 0
               chmod 666 /dev/rsh0
           else
               echo "This version of $0 only suitable for V4.1.1 through V4.4.4 of SunOS"
               echo "$0 exits"
               exit 1
           fi
       fi
    fi
fi

# change directory to the main VNMR directory
cd ..
#
# Check for acquisition software
#
if (test ! -r ./acq/autshm.out) || (test ! -r ./acq/rhmon.out)
then
  echo "Acquisition Software not present"
  exit 1
fi

make_etc_files

#
# modify /etc/rc to start Acqproc at bootup
#
if (test -f /etc/rc.vnmr)
then
    rm /etc/rc.vnmr
fi
cp ./rc.vnmr /etc/rc.vnmr
#
# replace the old way of checking for rc.vnmr
grep '/vnmr/rc.vnmr' /etc/rc >rc_tmp
if (test -s rc_tmp)
then
    sed s/\\\/vnmr/\\\/etc/g /etc/rc >rc_tmp
    mv rc_tmp /etc/rc
fi
# add automatic startup of Acqproc
grep '/etc/rc.vnmr' /etc/rc >rc_tmp
if (test ! -s rc_tmp)
then
    cp /etc/rc /etc/rc.bk
    sed '$i\
\
# changes for VNMR\
\
if [ -f /etc/rc.vnmr ]; then\
  sh /etc/rc.vnmr\
fi' /etc/rc >rc_tmp
    mv rc_tmp /etc/rc
fi
#
#  cleanup
#
if (test -f rc_tmp)
then
    rm rc_tmp
fi

#  install VNMR kernel, backup original in vmunix.orig if it doesn't exist yet

if (test -s ./`arch -k`kernel/vmunix)
then
    echo "Installing VNMR version of `arch -k` UNIX kernel"
    type=`arch -k`
    echo "  `strings ./${type}kernel/vmunix | egrep '^Sun UNIX|^SunOS (Release)'"
    if (test ! -s /vmunix.orig )
    then
       echo "backup of orignal UNIX kernel is in vmunix.orig"
       mv /vmunix /vmunix.orig
    fi
    mv ./`arch -k`kernel/vmunix /vmunix
    newos=1
else
    newos=0
fi

#  reboot the host computer if new kernel installed unless directed not to

if (test $newos != 0)
then
    echo -n "Shutdown and reboot the spectrometer? (y or n) [y]: "
    read a
    if (test x$a = 'xn' -o x$a = 'xN')
    then
	echo "Be sure to reboot your system as soon as possible !!"
    else
        /etc/shutdown -fr now
    fi
fi
