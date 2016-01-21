: '@(#)vnmr_usemark.sh 22.1 03/24/08 1991-1996 '
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
: usemark - copy mark1d.out information into fitspec.inpar
sed '1d' < $1/mark1d.out | \
awk '
 {if (NF==2) {
             printf "%12.2f, %12.2f, %12.2f\n", $1, $2, "'$2'"
            }
  else if (NF==4) {
             shift=($1+$3)/2
             width=($1-$3)
             printf "%12.2f, %12.2f, %12.2f\n", shift, $2, width
            }
}' | sort -t, +0n -1n -r > $1/fitspec.inpar
