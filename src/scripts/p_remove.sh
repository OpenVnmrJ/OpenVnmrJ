#!/bin/sh
# '@(#)p_remove.sh 22.1 03/24/08 1991-2004 '
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

vnmrsystem=/vnmr

patch_saved_dir=$1
cd $patch_saved_dir

# If someone edit this file should recheck p_install
# The lines between #########s fil in by p_install
# as the patch installation took place
########################################################


########################################################
cd $vnmrsystem
rm -r $patch_saved_dir

echo "	    patch `basename $patch_saved_dir` removed -----"
echo ""
