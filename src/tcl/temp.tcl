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
set settempOff [vnmrinfo tempOnOff]
set old_setpoint [vnmrinfo tempSetPoint]
set mintemp -150
set maxtemp 200
set tempinterval 50

proc vnmrInitVars {interval} {
   global onoff expcontrol errorcontrol
   global old_setpoint
   global settempOff sendsethw

   set onoff [vnmrinfo tempOnOff]
   if {$settempOff == 0} {
      set setval [vnmrinfo tempSetPoint]
      if {$setval != $old_setpoint} {
         if {$sendsethw == 1} {
           vnmrsend "sethw('temp',$old_setpoint):\$e temp=$old_setpoint"
         } else {
            set old_setpoint $setval
         }
      }
   }
   set sendsethw 0
   set expcontrol [vnmrinfo tempExpControl]
   if {$expcontrol == 1} {
      .interlock.warn configure -state disabled
      .interlock.on configure -state disabled
      set errorcontrol 0
   } else {
      if {[.interlock.warn cget -state] == "disabled"} {
         .interlock.warn configure -state normal
         .interlock.on configure -state normal
      }
      set errorcontrol [vnmrinfo tempErrorControl]
   }
   after $interval [list vnmrInitVars $interval]
}

proc toggleExpControl {} {
   global expcontrol
   if {$expcontrol == 1} {
      vnmrsend {vnmrinfo('set','tempExpControl',0)}
   } else {
      vnmrsend {vnmrinfo('set','tempExpControl',1)}
   }
}

proc setTemp {} {
   global settempOff sendsethw
   
   set settempOff 0
   set sendsethw 1
}

proc resetflash {} {
   vnmrsend sethw('vt','reset')
   set fg [.resetlabel cget -foreground]
   for {set i 0} {$i < 10} {incr i} {
      .resetlabel configure -foreground red
      update idletasks
      after 250
      .resetlabel configure -foreground $fg
      update idletasks
      after 250
   }
   vnmrsend "write('line3',' ')"
}

set hostname [exec uname -n]
wm title . "Temperature Control for $hostname"
wm iconname . Temp
wm minsize . 10 10

frame .set -relief groove -borderwidth 4
frame .vtframe
label .vttitle -text "Temperature Control"
radiobutton .off -text "Turn temperature control off"  -relief flat \
             -variable onoff -value 0 -highlightthickness 0
radiobutton .vt -text "Turn temperature control on"  -relief flat \
             -variable onoff -value 1 -highlightthickness 0
label .at -text at
label .atval -textvariable old_setpoint
label .deg -text " degrees C."
scale .vtscale -label "" -orient horizontal \
      -from $mintemp -to $maxtemp -tickinterval $tempinterval \
      -length 12c -variable old_setpoint -resolution 1
pack .vt .at .atval .deg -in .vtframe -side left -fill x -anchor w
pack .vttitle -in .set -side top -anchor n
pack .off -in .set -side top -anchor w
pack .vtframe -in .set -side top -anchor w
pack .vtscale -in .set -side top -anchor w

bind .off <Button-1> {
    vnmrsend "sethw('vt','off'):\$e vnmrinfo('set','tempOnOff',0) temp='n'"
}
bind .vt <Button-1> {
    set setpoint [.vtscale get]
    vnmrsend "sethw('temp',$setpoint):\$e temp=$setpoint"
}
bind .vtscale <Button-1> {
   set settempOff 1
   }

bind .vtscale <ButtonRelease-1> {
   setTemp
   }
bind .vtscale <Button-3> {
    set pos [.vtscale identify %x %y]
    if {$pos == "trough1"} {
       set old_setpoint [expr $old_setpoint - 0.1]
       vnmrsend "sethw('temp',$old_setpoint):\$e temp=$old_setpoint"
    } elseif {$pos == "trough2"} {
       set old_setpoint [expr $old_setpoint + 0.1]
       vnmrsend "sethw('temp',$old_setpoint):\$e temp=$old_setpoint"
    }
}

frame .interlock -relief groove -borderwidth 4
label .interlock.title -text "Experiment Control"
checkbutton .interlock.cntl -text "Allow temperature control in an experiment with go"\
             -relief flat \
            -variable expcontrol -anchor w -highlightthickness 0
radiobutton .interlock.warn -text "Issue a warning after a temperature error"  \
             -relief flat \
             -variable errorcontrol -value 2 -anchor w -highlightthickness 0
radiobutton .interlock.on -text "Stop acquisition after a temperature error" \
             -relief flat \
             -variable errorcontrol -value 1 -anchor w -highlightthickness 0
pack .interlock.title -anchor n
pack .interlock.cntl -side top -anchor w -fill x
pack .interlock.warn .interlock.on -side top -padx 10 -anchor w -fill x
bind .interlock.cntl <Button-1> {toggleExpControl}
bind .interlock.warn <Button-1> \
      {vnmrsend vnmrinfo('set','tempErrorControl',2)}
bind .interlock.on <Button-1> \
      {vnmrsend vnmrinfo('set','tempErrorControl',1)}

frame .reset -relief groove -borderwidth 4
frame .resetframe
button .vtreset -text "Reset VT"  -command resetflash
label  .resetlabel -text "Press \"Reset VT\" when VT cable is reconnected to a probe"
pack .vtreset .resetlabel -in .reset -side left -fill x -anchor w

vnmrInitVars 500
pack .set .interlock .reset -side top -anchor w -fill x
