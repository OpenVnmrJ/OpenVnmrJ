#!/vnmr/tcl/bin/vnmrWish -f
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
# NMS installer and control

vnmrinit \"[lindex $argv 1]\" $env(vnmrsystem)
set SMS_DEV_FILE  $env(vnmrsystem)/smsport
set NMSALIGN_FILE $env(vnmrsystem)/bin/nmsalign

proc getPort {} {
   global SMS_DEV_FILE

   if ![file exists $SMS_DEV_FILE] {
      puts "Vnmr is not configured for sample changers. Run config."
      return -1
   }

   set portFD [open $SMS_DEV_FILE r]
   set aa [gets $portFD]
   close $portFD
   if {[lindex $aa 1] == "NMS"} {
      return [lindex $aa 0]
   } else {
        .cont.b.scroll.text insert end \
                 "NMS controller is not connecting to this console\n"
        return -1
   }
}

proc connectToNmsalign {} {
   global nmsFD NMSALIGN_FILE nmsProm

   .cont.b.scroll.text insert end "\nConnecting to NMS\n"
   set serial_port [getPort]
   if {[string match $serial_port "a"] || [string match $serial_port "b"]} {
      update idletasks
      set nmsFD [open "| $NMSALIGN_FILE /dev/term/${serial_port} noprint" r+]
      if { $nmsFD >= 3 } {

         set ret [readPrompt "CMDS:"]

         if { $ret == "CMDS:" } {
            .cont.b.scroll.text insert end "Connected\n\n"
            return 0
         } else {
              .cont.b.scroll.text insert end "$ret\n\n"
              readAfterError 
              #.cont.b.scroll.text insert end "NMS Not Responding\n"
           }
      }
   }
   set nmsFD -1
   return -1
}    
 
proc sendToNmsalign {cmd} {
   global nmsFD
 
   .cont.b.scroll.text insert end "$cmd\n"
    update idletasks

   if {$nmsFD != -1} {
      puts $nmsFD "$cmd\n"
      flush $nmsFD
      set s_ret [readPrompt "CMDS:"]
      if { $s_ret == "CMDS:" } {
         .cont.b.scroll.text insert end "$cmd\n\n"
         return 0
      } else {
           .cont.b.scroll.text insert end "$s_ret\n\n"
           readAfterError
           #.cont.b.scroll.text insert end "NMS Not Responding\n"
        }
   } else {
      .cont.b.scroll.text insert end "sendToNmsalign: Problem sending \"${cmd}\"\n"
      return -1
   } 
}

proc readAfterError {} {
   global nmsFD nmsProm
 
   if {$nmsFD == -1} {
     return -1
   }  
      #might send a line feed to NMS here
      #or gets and discard one CMDS: 
 
   set count 0
   while { 1 == 1 } {
 
      if {$count >= 6} {
         #.cont.b.scroll.text insert end "NMS Not Responding\n"
         return -1
      }
         
      set Line ""
      set stat [gets $nmsFD Line]
         
      if { $stat >= 1 } {
         set nmsProm [lindex $Line 0] ;#might not need this
         return
      }  
      incr count
      after 10
   }     
}


proc readPrompt {prompt} {
   global nmsFD nmsProm

   if {$nmsFD == -1} {
     return -1
   }

   set count 0
   while { 1 == 1 } {

      if {$count >= 6} {
         #.cont.b.scroll.text insert end "NMS Not Responding\n"
         return -1
      }

      set Line ""
      set stat [gets $nmsFD Line]

      if { $stat >= 1 } {
         set nmsProm [lindex $Line 0] ;#might not need this
         return [lindex $Line 0] 
      }  
      incr count
      after 10
   }
}
 
proc quitNmsalign {} {
   global nmsFD

     sendToNmsalign "Q"
     if {$nmsFD != -1} {
        catch {close $nmsFD}
     }   
     destroy .
     return 0
}

proc nmsInstall {} {

    if {[winfo exist .install]} {
       return
    }
	frame .install -bd 2 -relief raised
	frame .install.at
	frame .install.at.adjust -bd 2 -relief raised
	frame .install.at.test -bd 2 -relief raised
	frame .install.bye -bd 2 -relief raised
	
	label .install.at.adjust.title -text "Adjust" -fg blue -font courb18
	button .install.at.adjust.ins -text "Insertion" -command "sendToNmsalign F"
	button .install.at.adjust.rac -text "Rack" -command "sendToNmsalign C"
	button .install.at.adjust.car -text "Carousel" -command "sendToNmsalign A"
	pack .install.at.adjust.title .install.at.adjust.ins \
		.install.at.adjust.rac .install.at.adjust.car \
		-fill x
	
	label .install.at.test.title -text "Test" -fg blue -font courb18
	button .install.at.test.ins -text "Insertion" -command "sendToNmsalign P"
	button .install.at.test.rac -text "Rack" -command "sendToNmsalign R"
	button .install.at.test.car -text "System" -command "sendToNmsalign T"
	pack .install.at.test.title .install.at.test.ins \
		.install.at.test.rac .install.at.test.car \
		-fill x
	
	button .install.bye.bye -text "Close" -command "destroy .install"
	pack .install.bye.bye 
	
	pack .install.at.adjust -side left -fill x
	pack .install.at.test -fill x -side right
	pack .install.at -fill x
	pack .install.bye -side bottom -fill x
	
	pack .install -side bottom -fill x
}
	
global loc newloc	
set loc 0
set newloc [expr $loc + 1]

frame .cont
frame .quit
frame .cont.b

frame .cont.current
frame .cont.b.buttons
frame .cont.b.scroll
frame .cont.install

frame .cont.b.buttons.n

label .cont.current.label -text "Current Sample " -font courb18
label .cont.current.numb -textvariable loc -fg blue -font courb18
pack .cont.current.label .cont.current.numb -side left

button .cont.b.buttons.lift -text "Lift Probe" -command "sendToNmsalign U"
button .cont.b.buttons.low  -text "Lower Probe" -command "sendToNmsalign D"
button .cont.b.buttons.stop -text "STOP" -bg red -font courb18 \
	-command "sendToNmsalign "
button .cont.b.buttons.change -text "Change Sample" -command {
	sendToNmsalign S$newloc
	set loc $newloc
	incr newloc
	}
label .cont.b.buttons.n.label -text "Next Sample "
entry .cont.b.buttons.n.new -textvariable newloc -width 3
button .cont.b.buttons.mt -text "Empty Probe" -command {
	sendToNmsalign S0
	set loc 0
	set newloc 1
	}
label .cont.b.buttons.sp
label .cont.b.buttons.sp1
label .cont.b.buttons.sp2

pack .cont.b.buttons.n.label .cont.b.buttons.n.new -side left 
pack .cont.b.buttons.lift .cont.b.buttons.low \
	.cont.b.buttons.sp \
	.cont.b.buttons.change .cont.b.buttons.n \
	.cont.b.buttons.sp1 \
	.cont.b.buttons.mt .cont.b.buttons.sp2 \
	.cont.b.buttons.stop \
	-fill x

text .cont.b.scroll.text -yscrollcommand ".cont.b.scroll.scrl set" \
	-width 20 -bg gray90
scrollbar .cont.b.scroll.scrl -command ".cont.b.scroll.text yview"
pack .cont.b.scroll.text -side left
pack .cont.b.scroll.scrl -side right -fill y

button .cont.install.ins -text "Install" -command "nmsInstall"
button .cont.install.quit -text "Quit" -command "quitNmsalign"
pack .cont.install.ins -side left
pack .cont.install.quit -side right

pack .cont.current -side top
pack .cont.b.buttons -side left
pack .cont.b.scroll -side right
pack .cont.b -side top
pack .cont.install -side top -fill x

pack .cont

.cont.b.scroll.text insert 1.0 "NMS dialog is\nechoed here\n"

connectToNmsalign
