: '@(#)vnmredit.sh 22.1 03/24/08 1991-1996 '
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
#
#  vi is selected automatically in two situations
#     no vnmr editor (vnmreditor not defined or the 0-length string)
#     no vnmr edit command to go with the editor

if [ x"$vnmreditor" = "x" ]
then
    vnmreditor="vi"
fi

vnmreditpath="$vnmrsystem"/bin/vnmr_${vnmreditor}
if [ ! -x "$vnmreditpath" ]
then
    vnmreditpath="$vnmrsystem"/bin/vnmr_vi
fi

"$vnmreditpath" $@
