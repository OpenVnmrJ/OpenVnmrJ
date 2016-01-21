#!/bin/csh

# split Dimension length 'ndim' into 'base' multiples (with two options)
# agrv0: Dim  base  tout    flag 
# e.g    60   2     NDSIZES  y 


# tout = NDSIZES | DSIZES | MULT 
#        4         2 2 2 2  16
# last flag (y or n)  y means do splitting, while n means no splitting
 
set ndim=$1 
set base=$2
set tout=$3
set flag=$4

 

if ( $flag == n ) then
switch($tout)
case "NDSIZES": 
echo "1"
breaksw
case "DSIZES":  
echo "$ndim"
breaksw
case "MULT":    
echo "$ndim"
breaksw
endsw

else

set mult=1;set i=0;set string=""
while ($mult < $ndim )
 set string=("$string $base");
 @ mult = $mult * $base
 @ i++
end

switch($tout)
case "NDSIZES": 
echo $i
breaksw
case "DSIZES":  
echo "$string"
breaksw
case "MULT":    
echo "$mult"
breaksw
endsw

endif


exit
