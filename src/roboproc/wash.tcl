#
#

# ------  Routine to Rinse Needle  ------------
proc RinseNeedle { } {
   global NeedleRinseVolume NeedleRinseRate

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

  global ProbeVolume RinseDeltaVol NumRinses MaxFlow ProbeFastRate ProbeSlowVol ProbeSlowRate RinseExtraVol NeedleRinseVolume NeedleRinseRate

#  gWriteDisplay " Rinse " /* max of 8 chars */


  set RinseVol [ expr ($ProbeVolume + $RinseDeltaVol)]

  gMoveZ2Top 

  for { set x 1 } { $x <= $NumRinses } { incr x } {

     gFlush $RinseVol $MaxFlow 0.0
     gMove2InjectorPort
     gFlush $RinseVol 0.0 $ProbeFastRate

     gDelayMsec 300
     gSetContacts 2 1
     gDelayMsec 500

     set vol $ProbeSlowVol
     if {$vol > ($RinseVol + $RinseExtraVol)} {
        set vol [expr $RinseVol + $RinseExtraVol]
     }
     gAspirate $vol $ProbeSlowRate 0.0
     gAspirate [expr ($RinseVol - $vol + $RinseExtraVol)] $ProbeFastRate 0.0

     gDelayMsec 1500
     gSetContacts 2 0 
     gDelayMsec 300

     gMoveZ2Top
     gMove2RinseStation
     gDispense  [expr ($RinseVol + $RinseExtraVol)] $MaxFlow 0.0
     gFlush $NeedleRinseVolume $NeedleRinseRate $NeedleRinseRate
     gMoveZ2Top

#    increment each rinse volume by RinseDeltaVol, uncomment the following line
#    incr RinseVol $RinseDeltaVol 
  }

}
# ===================================================================

proc wash { who } {

   global SampleNumber SampleZone 


  if { $who == "needle" } {

# ------  Rinse Needle, Gilson calls this the probe --------
  set msg [format "Z%dS%dRN" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  RinseNeedle 

  } elseif { $who == "probe" } {
# ------  Rinse the NMR Probe ------------------------------
  set msg [format "Z%dS%dRP" $SampleZone $SampleNumber ]
  gWriteDisplay "$msg"
  RinseNmrProbe 
  } else {
     error "Usage: wash [needle | probe]"
  }
}

