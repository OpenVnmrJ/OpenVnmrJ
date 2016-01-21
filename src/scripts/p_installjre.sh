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

chmod 755 $vnmrsystem/jre
mv $vnmrsystem/jre $patch_saved_dir
mv $patch_process_dir/p_remove $patch_saved_dir
mv $patch_process_dir/jre $vnmrsystem


