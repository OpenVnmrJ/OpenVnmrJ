#
#

proc transfer { XferVolume XferRackLoc XferZone XferSample XferHeight XferFlowRate } {

  global RackLoc SampleNumber SampleZone SampleHeight

  gPuts "--------------------------- proc transfer.tcl\n"

  gPuts "gTransfer $XferVolume $XferRackLoc $XferZone $XferSample $XferHeight \
	 $XferFlowRate $RackLoc $SampleZone $SampleNumber $SampleHeight $XferFlowRate\n"

  gTransfer $XferVolume $XferRackLoc $XferZone $XferSample $XferHeight \
	    $XferFlowRate $RackLoc $SampleZone $SampleNumber $SampleHeight $XferFlowRate

  gPuts "--------------------------- transfer.tcl done\n"
}
