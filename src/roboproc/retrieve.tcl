#
#

proc retrieve { RackLoc SampleNumber SampleZone SampleVolume SampleHeight SampleDepth } {

  global ProbeSlowVol ProbeSlowRate ProbeVolume ProbeFastRate SampleExtraVol SampleWellRate SampleKeepFlag MaxFlow
# retrieve
#  gWriteDisplay "Retrieve"

# ------ Just incase the arm has been moved, Move to Injector
  gMoveZ2Top
  gMove2InjectorPort 
 
# ------ Inject Sample + Push Volume == ProbeVolume  -------------

# ----- Switch gas valve to put pressure on Probe  ----------
  gSetContacts 2 1

# -----  Get back Sample from NMR Probe  ---------------
  set msg [format "Z%dS%dAS" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  set vol $ProbeSlowVol
  if {$vol > ($ProbeVolume + $SampleExtraVol)} {
     set vol [expr $ProbeVolume + $SampleExtraVol]
  }
  gAspirate  $vol  $ProbeSlowRate 0.0
  gAspirate  [expr ($ProbeVolume + $SampleExtraVol - $vol)] $ProbeFastRate 0.0
  gDelayMsec 1500
  gSetContacts 2 0
  gDelayMsec 300
	

  set PushVolume [expr ($ProbeVolume - $SampleVolume)]

  gMoveZ2Top

  if { $SampleKeepFlag > 0 } {
#     ------ Put Sample Back into Vial  -------------------------
#     gMove2Sample $RackLoc $SampleZone $SampleNumber
#     gMoveZ  [expr ($SampleZTop + 80)]
     gMove2Sample $RackLoc $SampleZone $SampleNumber
     gMove2LiqLevel $RackLoc $SampleZone $SampleNumber $SampleHeight NOSEEK

     set msg [format "Z%dS%dDS" $SampleZone $SampleNumber ]
     gWriteDisplay "$msg"

# ------ Wait for all action to stop  ------------------
     gStopTestAll

     gDispense [expr ($SampleVolume + $SampleExtraVol)] $SampleWellRate 0.0

     gMoveZ2Top

#    ------ Flush any Push Solvent from Neddle  ---------------
    if { $PushVolume > 0.0 } {
#      Dispense Push Solvent
       set msg [format "Z%dS%dDP" $SampleZone $SampleNumber ]
       gWriteDisplay "$msg"
       gMove2RinseStation
       gFlush $PushVolume 0 $MaxFlow
       gMoveZ2Top
     }
  } else {
#   --- Don't Keep Sample, just flush down the rinse station 
    set msg [format "Z%dS%dFS" $SampleZone $SampleNumber ]
    gWriteDisplay "$msg"
    gMove2RinseStation
    set TotalVol [expr ($ProbeVolume + $SampleExtraVol)]
    gFlush $TotalVol 0 $MaxFlow
  }  

  gWriteDisplay ""
}
