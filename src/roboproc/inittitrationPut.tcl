#
#

# initTitrationPut.tcl special protocol for 1st sample of titration

#
#  Transfer Parameter Values  
#  User needs to changes these to their needs !
#
set XferVolume  50
set XferRackLoc 1
set XferZone 1
set XferSample 9
set XferHeight 50
set XferFlowRate 4


gPuts "------->  initTitrationPut  <---------- \n"

gPuts "---- transfer $XferVolume $XferRackLoc $XferZone $XferSample $XferHeight $XferFlowRate\n"

transfer $XferVolume $XferRackLoc $XferZone $XferSample $XferHeight $XferFlowRate

gPuts "---- mix $RackLoc $SampleZone $SampleNumber $MixHeight $MixVolume $MixTimes $MixFlowRate\n"

mix $RackLoc $SampleZone $SampleNumber $MixHeight $MixVolume $MixTimes $MixFlowRate

gPuts "------->  initTitrationPut  Done <---------- \n"
