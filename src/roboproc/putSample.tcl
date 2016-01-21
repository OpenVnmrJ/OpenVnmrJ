#
#

# ------  Routine to Rinse Needle  ------------
proc RinseNeedle { } {
   global NeedleRinseVolume NeedleRinseRate MaxFlow

  gMoveZ2Top

  gFlush $NeedleRinseVolume $NeedleRinseRate 0.0 

# Move to Rinse Station while syringe is working
  gMove2RinseStation

  gFlush $NeedleRinseVolume 0.0 $NeedleRinseRate

  gMoveZ2Top

}
# ===================================================================

# ------  Routine to Rinse NMR Probe  ------------
proc RinseNmrProbe { } {
#  global ProbeVolume MaxFlow ProbeInjectFlow ProbeRetrievVol ProbeRetrievFlow ProbeExtraVol SampleRinseVol

   global ProbeVolume ProbeFastRate ProbeSlowRate ProbeSlowVol RinseRetrievExtraVol RinseDeltaVol NumRinses MaxFlow NeedleRinseVolume NeedleRinseRate

#  gWriteDisplay " Rinse " /* max of 8 chars */

  set RinseVol [ expr ($ProbeVolume + $RinseDeltaVol)]

  for { set x 1 } { $x <= $NumRinses } { incr x } {

     gFlush $RinseVol $MaxFlow 0.0
     gMove2InjectorPort
     gFlush $RinseVol 0.0 $ProbeFastRate

     gDelayMsec 300

     gSetContacts 2 1
 
     gDelayMsec 500

     gAspirate $ProbeSlowVol $ProbeSlowRate 0.0

     gAspirate [expr ($RinseVol - $ProbeSlowVol + $RinseRetrievExtraVol)] $ProbeFastRate 0.0

     gDelayMsec 1500

     gSetContacts 2 0 
 
     gDelayMsec 300

     gMoveZ2Top
     gMove2RinseStation
     gDispense  [expr ($RinseVol + $RinseRetrievExtraVol)] $MaxFlow 0.0
     gFlush $NeedleRinseVolume $NeedleRinseRate $NeedleRinseRate
     gMoveZ2Top

#    increment each rinse volume by RinseDeltaVol, uncomment the following line
#    incr RinseVol $RinseDeltaVol 
  }

}
# ===================================================================


# ----  Start of putSample Script  ----------------------------------

  set msg [format "R%dZ%dS%d" $RackLoc $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"

# ----- Switch gas valve to vent NMR Probe to Atmosphere  ----------
  gSetContacts 2 0

# ------  Rinse Needle, Gilson calls this the probe --------
  set msg [format "Z%dS%dRN" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  RinseNeedle 

# ------  Rinse the NMR Probe ------------------------------
  set msg [format "Z%dS%dRP" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  RinseNmrProbe 

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
     gFlush $PushVolume $MaxFlow 0
  }

# ------  Move to the Sample Location -----------------------------
  gMove2Sample $RackLoc $SampleZone $SampleNumber

# ------  Put Needle into liquid ----------------------------------
  gMoveZ $SampleZTop

# ------  Aspirate Sample ----------------------------------------
  set msg [format "Z%dS%dAS" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
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
