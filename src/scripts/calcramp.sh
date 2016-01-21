: '@(#)calcramp.sh 22.1 03/24/08 1999-2002 '
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
#!/bin/csh

# The argument is the 90 deg pulse in 0.1usec units (pw90 x 10)
# The pulseshape name is generated so that when pw90 is a whole number of usecs (N)
# the name is Ng.RF, otherwise the name is NNg.RF 

set pw10=$1

# @ last = ( $pw10 - ( ( $pw10 / 10 ) * 10 ) )
# if ( $last > 0 ) then
  set file='comp_'$pw10'g.RF'
# else
#   @ pw = $pw10 / 10
#   set file='comp_'$pw'g.RF'
# endif

echo "# $file" > $file

@ steps = ( ( ( 4 * $pw10 ) + 8 ) / 2 )

echo "# Total steps =  $steps" >> $file

@ steps = $steps - 4

echo "# Ramp steps = $steps" >> $file
echo "# " >> $file

echo "0.00	1023.0	1.0    0" >> $file
echo "0.00	1023.0	1.0    0" >> $file

set step=0
while ($step < $steps )
@ phase = ( ( ( $step * 360000 ) / $steps ) + 5 )
@ dphase = $phase / 1000
@ fphase = ( ( $phase - ( $dphase * 1000 ) ) / 10 )
if ( $fphase < 10 ) then
  set zed='0'
else
  set zed=''
endif
echo "${dphase}.${zed}${fphase}	1023.0	1.0" >> $file
@ step ++
end
echo "${dphase}.${zed}${fphase}	1023.0	1.0    0" >> $file
echo "${dphase}.${zed}${fphase}	1023.0	1.0    0" >> $file

exit
  
