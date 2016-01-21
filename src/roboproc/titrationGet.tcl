#
# 
 
gPuts "------->  titrationGet  <---------- \n"
gPuts "---- SampleNumber $SampleNumber\n"

# add offset to present sample for the return location of sample

set SampleNumber [expr $SampleNumber + 1 ]

gPuts "---- Return Location $SampleNumber\n"

gPuts "------->  titrationGet  Done <---------- \n"
