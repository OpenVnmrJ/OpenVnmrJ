: '@(#)chksudocmd.sh 22.1 03/24/08 1999-2002 '
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
#! /usr/bin/sh

# chksudocmd.sh

# This script verifies that if a command is available to user through sudo
#
# Usage: chksudocmd user command
#
# command could be either a full pathname command or just a command
# returns 0 if OK
# returns 1 if errors

if [ $# -ne 2 ]
then 
    #echo "$0 must be called with 2 arguments"
    echo 1
    exit
fi

sudoers="/etc/sudoers"
ck_user=$1
ck_cmd=$2

sudo_str=`grep $ck_user $sudoers`
if [ x"$sudo_str" = "x" ]
then 
    #echo "\nUser $ck_user does not exist in sudo list\n"
    echo 1
    exit
fi

firstchar=`echo $ck_cmd | cut -c1-1`

for i in `echo $sudo_str | awk 'BEGIN {FS=":"} {print $2}' | sed 's/,//g'`
do
     if [ x$firstchar != "x/" ]
     then
          i=`basename $i`
     fi

     if [ x$ck_cmd =  x$i ]
     then
          echo 0
          exit
     fi
done

echo 1
