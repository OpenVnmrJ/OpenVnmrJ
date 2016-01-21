# '@(#)p_required '
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

# The p_required file is an optional file called by patchinstall.
# The patchinstall script installs the content of the patch. This file
# is used test whether any prerequisits for patch are met.
# A typical one is whether an earlier patch is installed.

# The parameter $vnmrsystem will be set. Its typical value will be /vnmr
# The parameter $patch_adm_dir will be set. It is the directory where patch
# information is kept.
# To cause the patch installation to stop, touch the
# file $patch_temp_dir/patchAbort

reqID="4.2_LNX_DDR_105"
if [ ! -d $patch_adm_dir/$reqID ]
then
   echo ""
   echo "Required patch $reqID is not installed"
   echo ""
   touch $patch_temp_dir/patchAbort
fi

