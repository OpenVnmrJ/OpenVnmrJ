#!/bin/bash
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
#*********************************************************************
#** seqgenupdate:  shell script to move psglib and seqlib files     **
#**  back to the appdir from which they came.                       **
#*********************************************************************

if [ "x$#" = "x0" ]; then
  Vnmrbg -mback -n0 "seqgenupdate"
  exit
fi

# If arguments are give, a seqgen will be called first.
# therefore, do some preliminary checks to make sure
# seqgen will work

# Check to establish USER and LOGNAME equivalence
# (Prevents problems with one user using UNIX command "su"
# to become some other user)

if [[ ! -z $LOGNAME ]] && [[ $USER != $LOGNAME ]]; then
   echo "Username: "$USER"  and Logname: "$LOGNAME"  do not agree."
   echo ""
   exit
fi

if [[ x$vnmrsystem = "x" ]]
then
   source /vnmr/user_templates/.vnmrenvsh
fi

#  Check for USER directory for compiled pulse sequences (SEQLIB). If
#  it is not present, create it.

if  [ ! -d "$vnmruser"/seqlib ]; then
   mkdir "$vnmruser"/seqlib
fi

# Check for USER pulse sequence directory (PSGLIB)

if [ ! -d "$vnmruser"/psglib ]; then
   mkdir "$vnmruser"/psglib
fi

if [ ! -d "$vnmruser"/psglib -o ! -d "$vnmruser"/seqlib ]; then
   echo "Cannot create user's 'psglib' and/or 'seqlib' Directories."
   exit
fi

#****************************************
#**  Call Vnmrbg to execute seqgenupdate.
#****************************************

cmd="seqgenupdate('$@')"
Vnmrbg -mback -n0 "$cmd"
