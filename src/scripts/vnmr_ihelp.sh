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

echo $1
if test $# -gt 0
then
    acroread $1  >& /dev/null &
fi
