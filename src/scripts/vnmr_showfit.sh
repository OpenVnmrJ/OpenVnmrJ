: '@(#)vnmr_showfit.sh 22.1 03/24/08 1991-1996 '
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
: showfit
:  show the result of the most recent fit
:  reinterpret fitspec.outpar and place in fitspec.output
awk '
BEGIN { FS=","
printf("LINE  FREQUENCY (Hz)  HEIGHT  LINEWIDTH (Hz)  GAUSSIAN FRAC.  INTEGRAL\n")
}
{
if (NF<4) {
         $4=0.0
        }
printf "%2d %14.3f %10.2f %11.2f %13.2f %14.2f\n", NR, $1, $2, $3, $4, $2*$3
}' $1/fitspec.outpar > $1/fitspec.output
