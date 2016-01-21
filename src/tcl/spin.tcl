#!./vnmrwish -f
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

vnmrinit $argv $env(vnmrsystem)

set sendsethw 0
set setspeedOff 0
set setrateOff 0
set old_setspeed [vnmrinfo spinSetSpeed]
set setspeed_slow [expr int($old_setspeed)]
set setspeed_khz [expr int($old_setspeed/1000)*1000]
set setspeed_hz [expr $old_setspeed - $setspeed_khz]
set old_setrate [vnmrinfo spinSetRate]
set setrate_khz [expr int($old_setrate/1000)*1000]
set setrate_hz [expr $old_setrate - $setrate_khz]
#  current_frame values:  0  initialize to no frames
#  current_frame values:  1  Low Speed Spinner display
#  current_frame values:  2  High Speed Spinner display
#  current_frame values:  3  High Speed Air Flow display
set current_frame 0

proc setframes {} {
  global current_frame fakevar select
  set select [vnmrinfo spinSelect]
  if {$select == 0} {
    set userate [vnmrinfo spinUseRate]
    if {$userate == 1} {
      if {$current_frame != 3} {
        .speedtitle configure -text "Spin Air Flow Control"
        pack forget .speedframe .speedscale .speedscale_khz .speedscale_hz
        pack .rateframe .ratescale_khz .ratescale_hz -in .set \
             -side top -anchor w
        .speedrate configure -state normal
        set current_frame 3
        set fakevar 1
      }
    } else {
      if {$current_frame != 2} {
        .speedtitle configure -text "Spin Speed Control"
        pack forget .rateframe .ratescale_khz .ratescale_hz \
                    .speedscale
        pack .speedframe .speedscale_khz .speedscale_hz -in .set \
             -side top -anchor w
        .speedrate configure -state normal
        set current_frame 2
        set fakevar 0
      }
    }
  } else {
    if {$current_frame != 1} {
      .speedtitle configure -text "Spin Speed Control"
      pack forget .rateframe .ratescale_khz .ratescale_hz \
                  .speedscale_khz .speedscale_hz
      pack .speedframe .speedscale -in .set -side top -anchor w
      .speedrate configure -state disabled
      set current_frame 1
      set fakevar 0
    }
  }

}

proc vnmrInitVars {interval} {
   global onoff expcontrol select errorcontrol
   global old_setspeed old_setrate
   global setspeed_slow setspeed_khz setspeed_hz
   global setrate_khz setrate_hz
   global setspeedOff setrateOff
   global fakevar sendsethw

   set onoff [vnmrinfo spinOnOff]
   if {$setspeedOff == 0} {
      set setval [vnmrinfo spinSetSpeed]
      if {$setval != $old_setspeed} {
         if {$sendsethw == 1} {
           vnmrsend "sethw('spin',$old_setspeed) setvalue('spin',$old_setspeed)"
         } else {
            set old_setspeed $setval
            set setspeed_slow [expr int($old_setspeed)]
            set setspeed_khz [expr int($old_setspeed/1000)*1000]
            set setspeed_hz [expr $old_setspeed - $setspeed_khz]
         }
      }
   }
   if {$setrateOff == 0} {
      set setval [vnmrinfo spinSetRate]
      if {$setval > 65535} {
         set setval 65535
      }
      if {$setval != $old_setrate} {
         if {$sendsethw == 1} {
            vnmrsend sethw('srate',$old_setrate)
         } else {
            set old_setrate $setval
            set setrate_khz [expr int($old_setrate/1000)*1000]
            set setrate_hz [expr $old_setrate - $setrate_khz]
         }
      }
   }
   set sendsethw 0
   set expcontrol [vnmrinfo spinExpControl]
   if {$expcontrol == 1} {
      .interlock.warn configure -state disabled
      .interlock.on configure -state disabled
      set errorcontrol 0
   } else {
      if {[.interlock.warn cget -state] == "disabled"} {
         .interlock.warn configure -state normal
         .interlock.on configure -state normal
      }
      set errorcontrol [vnmrinfo spinErrorControl]
   }
   setframes
   after $interval [list vnmrInitVars $interval]
}

proc toggleExpControl {} {
   global expcontrol
   if {$expcontrol == 1} {
      vnmrsend {vnmrinfo('set','spinExpControl',0)}
   } else {
      vnmrsend {vnmrinfo('set','spinExpControl',1)}
   }
}

proc toggleSlow {} {
   vnmrsend vnmrinfo('set','spinSelect',500)
   vnmrsend vnmrinfo('set','spinSetSpeed',-1)
   setSpeed dummy
}

proc toggleFast {} {
   vnmrsend vnmrinfo('set','spinSelect',0)
   set userate [vnmrinfo spinUseRate]
   if {$userate == 0} {
      vnmrsend vnmrinfo('set','spinSetSpeed',-1)
      setSpeed_s dummy
   } else {
      vnmrsend vnmrinfo('set','spinSetRate',-1)
      setRate dummy
   } 
}

proc toggleAirFlow {} {
   global fakevar
   if {$fakevar == 0} {
     vnmrsend vnmrinfo('set','spinUseRate',1)
     vnmrsend vnmrinfo('set','spinSetRate',-1)
     setRate dummy
   } else {
     vnmrsend vnmrinfo('set','spinUseRate',0)
     vnmrsend vnmrinfo('set','spinSetSpeed',-1)
     setSpeed_s dummy
   }
}

proc setSpeed setval {
   global setspeedOff sendsethw old_setspeed
   
   set setspeedOff 0
   set setrateOff 1
   set sendsethw 1
   set old_setspeed [.speedscale get]
}

proc setSpeed_s setval {
   global setspeedOff sendsethw old_setspeed
   
   set khz [.speedscale_khz get]
   set hz [.speedscale_hz get]
   set old_setspeed [expr $khz + $hz]
   set setspeedOff 0
   set setrateOff 1
   set sendsethw 1
}

proc setRate setval {
   global setrateOff sendsethw old_setrate
   global setrate_khz setrate_hz
   
   set khz [.ratescale_khz get]
   if {$khz > 65000} {
      set khz 65000
      set setrate_khz 65000
   }
   set hz [.ratescale_hz get]
   set old_setrate [expr $khz + $hz]
   if {$old_setrate > 65535} {
      set old_setrate 65535
      set setrate_hz 535
   }
   set setrateOff 0
   set setspeedOff 1
   set sendsethw 1
}

set hostname [exec uname -n]
wm title . "Spinner Control for $hostname"
wm iconname . Spinner
wm minsize . 10 10

frame .set -relief groove -borderwidth 4
frame .speedframe
frame .rateframe
label .speedtitle
radiobutton .off -text "Turn spinner off"  -relief flat \
             -variable onoff -value 0 -highlightthickness 0
radiobutton .speed -text "Turn spinner on at "  -relief flat \
             -variable onoff -value 1 -highlightthickness 0
radiobutton .rate -text "Turn spinner on at "  -relief flat \
             -variable onoff -value 1 -highlightthickness 0
label .speedval -textvariable old_setspeed
label .hz -text " Hz"
label .rateval -text "air flow of " -textvariable old_setrate
scale .speedscale -label "" -orient horizontal -command setSpeed \
      -from 0 -to 50 -tickinterval 10 \
      -length 12c -variable setspeed_slow
scale .speedscale_khz -label "" -orient horizontal -command setSpeed_s \
      -from 0 -to 30000 -tickinterval 5000 -resolution 1000 \
      -length 12c -variable setspeed_khz
scale .speedscale_hz -label "" -orient horizontal -command setSpeed_s \
      -from 0 -to 1000 -tickinterval 100 \
      -length 12c -variable setspeed_hz
scale .ratescale_khz -label "" -orient horizontal -command setRate \
      -from 0 -to 65000 -tickinterval 10000 -resolution 1000 \
      -length 12c -variable setrate_khz
scale .ratescale_hz -label "" -orient horizontal -command setRate \
      -from 0 -to 1000 -tickinterval 100 \
      -length 12c -variable setrate_hz
pack .speed .speedval .hz -in .speedframe -side left -fill x -anchor w
pack .rate .rateval -in .rateframe -side left -fill x -anchor w
pack .speedtitle -in .set -side top -anchor n
pack .off -in .set -side top -anchor w

bind .off <Button-1> {
    vnmrsend sethw('spinner','off')
}
bind .speed <Button-1> {
    global old_setspeed
    vnmrsend "sethw('spin',$old_setspeed) setvalue('spin',$old_setspeed)"
}
bind .rate <Button-1> {
    global old_setrate
    vnmrsend sethw('srate',$old_setrate)
}

frame .interlock -relief groove -borderwidth 4
label .interlock.title -text "Experiment Control"
checkbutton .interlock.cntl -text "Allow spin control in an experiment with go"\
             -relief flat \
            -variable expcontrol -anchor w -highlightthickness 0
radiobutton .interlock.warn -text "Issue a warning after a spinner error"  \
             -relief flat \
             -variable errorcontrol -value 2 -anchor w -highlightthickness 0
radiobutton .interlock.on -text "Stop acquisition after a spinner error" \
             -relief flat \
             -variable errorcontrol -value 1 -anchor w -highlightthickness 0
pack .interlock.title -anchor n
pack .interlock.cntl -side top -anchor w -fill x
pack .interlock.warn .interlock.on -side top -padx 10 -anchor w -fill x
bind .interlock.cntl <Button-1> {toggleExpControl}
bind .interlock.warn <Button-1> \
      {vnmrsend vnmrinfo('set','spinErrorControl',2)}
bind .interlock.on <Button-1> \
      {vnmrsend vnmrinfo('set','spinErrorControl',1)}

frame .select -relief groove -borderwidth 4
label .selecttitle -text "Low Speed - High Speed Spinner Selection"
radiobutton .selectlow -text "Low speed spinner (liquids style)" \
             -relief flat -anchor w \
             -variable select -value 500 -highlightthickness 0
radiobutton .selecthigh -text "High speed spinner (solids style)"  \
            -relief flat  -anchor w\
             -variable select -value 0 -highlightthickness 0
checkbutton .speedrate -text "Set spinner air flow instead of speed"\
             -relief flat \
            -variable fakevar -anchor w -highlightthickness 0
pack .selecttitle -in .select -side top -anchor n
pack .selectlow .selecthigh -in .select -side top -fill x -anchor w
pack .speedrate -in .select -side top -padx 10 -fill x -anchor w
bind .selectlow <Button-1> {toggleSlow}
bind .selecthigh <Button-1> {toggleFast}
bind .speedrate <Button-1> {toggleAirFlow}

vnmrInitVars 500
setframes
pack .set .interlock .select -side top -anchor w -fill x
