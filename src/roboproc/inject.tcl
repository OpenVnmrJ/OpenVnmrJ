#
#

proc  inject { RackLoc SampleNumber SampleZone SampleVolume SampleHeight SampleDepth } {

  global ProbeVolume MaxFlow SampleWellRate SampleZSpeed ProbeFastRate

  gPuts "--------------- inject.tcl-------------------\n"

# ----  Start of putSample Script  ----------------------------------

  set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"

# ----- Switch gas valve to vent NMR Probe to Atmosphere  ----------
  gSetContacts 2 0

  set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"

# ---- If Sample Volume is less than probe volume then make up the 
# ---- difference with "push solvent"
# ---- push solvent  1st then sample
#
  set PushVolume [expr ($ProbeVolume - $SampleVolume)]
  if {$PushVolume < 0.0} { set PushVolume 0.0 }

# ------  Aspirate Push Solvent (if Volume > 0) ------------------
  if { $PushVolume > 0.0 } {
     set msg [format "Z%dS%dAP" $SampleZone $SampleNumber ]
     gWriteDisplay "$msg"
     gFlush $PushVolume $MaxFlow 0.0
  }

# ------  Move to the Sample Location -----------------------------
  gMove2Sample $RackLoc $SampleZone $SampleNumber

# ------ Wait for syringe to complete ------------------
  gStopTestAll

# ------  Put Needle into liquid ----------------------------------
  gMove2LiqLevel $RackLoc $SampleZone $SampleNumber $SampleHeight $SampleDepth

# ------  Aspirate Sample ----------------------------------------
  set msg [format "Z%dS%dAS" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"

#------ to follow the liquid while aspirating uncomment the following
#       line that obtains the proper Z speed for the volume and sample location
#
# set SampleZSpeed [ gZSpeed $RackLoc $SampleZone $SampleNumber $SampleWellRate ]
#

  gAspirate  $SampleVolume $SampleWellRate $SampleZSpeed

  gMoveZ2Top

# ------ Move to Injector, to inject Sample
  gMove2InjectorPort 
 
# ------ Inject Sample + Push Volume == ProbeVolume  -------------
  set msg [format "Z%dS%dDS" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  gDispense  $ProbeVolume $ProbeFastRate 0

  set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"

}
