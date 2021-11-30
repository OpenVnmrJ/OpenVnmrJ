: '@(#)seqgen.sh 22.1 03/24/08 1991-2007 '
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
#** seqgen:  shell script for performing pulse sequence generation  **
#**          at the UNIX level.   (Bourne shell)                    **
#**                                                                 **
#**          The pulse sequence file to be compiled is supplied as  **
#**          the argument to the seqgen shell script, e.g.,         **
#**          "seqgen test".  Note that the ".c" file extension is   **
#**          NOT necessary when specifying the pulse sequence file, **
#**          although if it is, it is removed, so test or test.c    **
#**          are both acceptable.                                   **
#**          The script vnmrseqgen does the real work.
#*********************************************************************

if [ "x$#" = "x0" ]; then
  echo "Usage: seqgen pulseSequence1 pulseSequence2 ..."
  exit
fi

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

#**********************************************************
#**  Call Vnmrbg to execute seqgen. This enables appdir psglibs
#**********************************************************
rm -f "$vnmruser"/psglib/.seqgen
for name in $@
do
  echo $name >> "$vnmruser"/psglib/.seqgen
done
cmd="seqgen(userdir+'/psglib/.seqgen',$#)"
Vnmrbg -mback -n0 "$cmd"
rm -f "$vnmruser"/psglib/.seqgen
