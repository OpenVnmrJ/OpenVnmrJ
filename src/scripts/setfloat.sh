: '@(#)setfloat.sh 22.1 03/24/08 1991-1996 '
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

#  C-shell script to select correct version of ``Vnmr'' and
#  ``fitspec'' for the floating point hardware on the current
#  system.  The entry ``master'' is a link to ``Vnmr''
#
# modified to handle the general case of name_fpa files in bin
# with Vnmr being a special case	3/21/91
#					Greg B.
#
# removed `master' as a link to VNMR.  06/14/91

# set current working directory

#
# remove dependence on presence of a "/vnmr" link or directory for script
# check that pwd is a vnmr directory
# then make changes relative to this.
#                                               Greg B.
vnmrdir=`pwd`
cdir=`basename $vnmrdir`
if (test x$cdir != "xbin")
then
   echo "Current working directory is '$vnmrdir'."
   echo "Please change to the vnmr/bin directory."
   echo "then restart $0"
   exit 1
fi


# SUN-4 stuff.  All required files exist.  Thus this routine is a no-op
# Keep the message so the installation as described remains the same

if (test `arch` = "sun4")
then
    echo "Configuring VNMR for SPARC architeture"
else

# get list of all files that need to be set (i.e. _68881 & _fpa files)
# the assumption is if there is a _fpa file then there is a _68881 file
   filelist=`ls *_fpa 2>/dev/null`	# stderr to /dev/null

# remove the _fpa from name
   nlist=`echo $filelist | sed s/\\_fpa//g` 

#  Stuff common to SUN-3, either 68881 or fpa

   if (fpa_test)
   then
        echo "Configuring for Floating Point Accelerator"
	isfpa=1
   else
        echo "Configuring for Floating Point Co-processor"
	isfpa=0
   fi

   for xfile in $nlist
   do
    echo $xfile
    if [ -f $xfile ]; then
        rm $xfile
    fi

#  decide if an FPA exists and make appropriate links

    if (test $isfpa -eq 1)
    then
	ln ${xfile}_fpa $xfile
    else
	ln ${xfile}_68881 $xfile
    fi
   done
fi
