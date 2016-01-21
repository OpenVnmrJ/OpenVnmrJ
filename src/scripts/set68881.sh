: '@(#)set68881.sh 22.1 03/24/08 1991-1996 '
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
#  Scripts only checks for sun3 as the host computer

#  added check to verify current user is "root"

if (test ! `whoami` = "root")
then
   echo "Please login as root to use $0"
   exit 1
fi

if [ `arch` != "sun3" ]
then
    echo "cannot execute $0 on a "`arch`
    exit 1
fi

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

# get list of all _68881 files 

filelist=`ls *_68881 2>/dev/null`           # stderr to /dev/null

# remove the _68881 from name

nlist=`echo $filelist | sed s/\\_68881//g` 

# for each file, link the base name to the fpa version

for xfile in $nlist
do
    echo "Configuring $xfile for floating point coprocessor"
    if [ -f $xfile ]; then
        rm $xfile
    fi
    ln ${xfile}_68881 $xfile
done
