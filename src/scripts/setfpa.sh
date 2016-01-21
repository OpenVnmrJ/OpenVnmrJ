: '@(#)setfpa.sh 22.1 03/24/08 1991-1996 '
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
#  If /dev/fpa is not present, it merely warns the user

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

if [ ! -f /dev/fpa ]
then
    echo "Warning:  no entry /dev/fpa in the file system"
fi

# get list of all _fpa files 

filelist=`ls *_fpa 2>/dev/null`           # stderr to /dev/null

# remove the _fpa from name

nlist=`echo $filelist | sed s/\\_fpa//g` 

# for each file, link the base name to the fpa version

for xfile in $nlist
do
    echo "Configuring $xfile for fpa"
    if [ -f $xfile ]; then
        rm $xfile
    fi
    ln ${xfile}_fpa $xfile
done
