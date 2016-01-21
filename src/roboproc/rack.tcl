# 
#
#!/vnmr/tcl/bin/vnmrWish -f

proc initRInfo {} {
   global rInfo
   set rInfo(first) {1 3 7 9}
   set rInfo(1,1) {1 2 3 4 5 6 7 8 9}
   set rInfo(1,2) {3 2 1 6 5 4 9 8 7}
   set rInfo(1,3) {7 8 9 4 5 6 1 2 3}
   set rInfo(1,4) {9 8 7 6 5 4 3 2 1}

   set rInfo(2,1) {1 2 3 6 5 4 7 8 9}
   set rInfo(2,2) {3 2 1 4 5 6 9 8 7}
   set rInfo(2,3) {7 8 9 6 5 4 1 2 3}
   set rInfo(2,4) {9 8 7 4 5 6 3 2 1}

   set rInfo(3,1) {1 4 7 2 5 8 3 6 9}
   set rInfo(3,2) {7 4 1 8 5 2 9 6 3}
   set rInfo(3,3) {3 6 9 2 5 8 1 4 7}
   set rInfo(3,4) {9 6 3 8 5 2 7 4 1}

   set rInfo(4,1) {1 6 7 2 5 8 3 4 9}
   set rInfo(4,2) {7 6 1 8 5 2 9 4 3}
   set rInfo(4,3) {3 4 9 2 5 8 1 6 7}
   set rInfo(4,4) {9 4 3 8 5 2 7 6 1}
}

proc selectPattern {index} {
   global rInfo
   if {$rInfo(pattern) != 0} {
      .p$rInfo(pattern) configure -relief raised
   }
   set rInfo(pattern) $index
   .p$rInfo(pattern) configure -relief sunken
}

proc setRinfo {index rIndex} {
   global rInfo racks

   set rInfo(rackNum) $index
   set rInfo(rackType) $rIndex
   set rInfo(pattern) 0
   selectPattern 1
}

proc numberCircles {} {
   global rInfo
   set st $rInfo(start)
   set first [lindex $rInfo(first) [expr $st-1]]
   for {set i 1} {$i <= 4} {incr i} {
      .p$i delete lines
      for {set j 1} {$j <= 9} {incr j} {
         set num [expr $j-1]
         if {$j == $first} {
            set color red
         } else {
            set color black
         }
         .p$i itemconfigure t$j -text [lindex $rInfo($i,$st) $num] \
            -fill $color
      }
   }
   if {($st == 1) || ($st == 4)} {
      .p1 create line  20m  4m  4m 12m -tags lines
      .p1 create line  20m 12m  4m 20m -tags lines
      .p2 create line  20m  4m 20m 12m -tags lines
      .p2 create line   4m 12m  4m 20m -tags lines
      .p3 create line   4m 20m 12m  4m -tags lines
      .p3 create line  12m 20m 20m  4m -tags lines
      .p4 create line   4m 20m 12m 20m -tags lines
      .p4 create line  12m  4m 20m  4m -tags lines
   } else {
      .p1 create line   4m  4m 20m 12m -tags lines
      .p1 create line   4m 12m 20m 20m -tags lines
      .p2 create line   4m  4m  4m 12m -tags lines
      .p2 create line  20m 12m 20m 20m -tags lines
      .p3 create line  20m 20m 12m  4m -tags lines
      .p3 create line  12m 20m  4m  4m -tags lines
      .p4 create line  20m 20m 12m 20m -tags lines
      .p4 create line  12m  4m  4m  4m -tags lines
   }
   .p1 lower lines
   .p2 lower lines
   .p3 lower lines
   .p4 lower lines
}

proc addcircles {win dir} {
   global rInfo
   set size 24
   set gap 2
   set radius [expr ($size - (3*$gap))/ 3.0 / 2.0]
   $win create oval 1m 1m 7m 7m -outline black -fill $rInfo(bg)
   $win create oval 9m 1m 15m 7m -outline black -fill $rInfo(bg)
   $win create oval 17m 1m 23m 7m -outline black -fill $rInfo(bg)
   $win create text 4m 4m -tags t1
   $win create text 12m 4m -tags t2
   $win create text 20m 4m -tags t3

   $win create oval 1m 9m 7m 15m -outline black -fill $rInfo(bg)
   $win create oval 9m 9m 15m 15m -outline black -fill $rInfo(bg)
   $win create oval 17m 9m 23m 15m -outline black -fill $rInfo(bg)
   $win create text 4m 12m -tags t4
   $win create text 12m 12m -tags t5
   $win create text 20m 12m -tags t6

   $win create oval 1m 17m 7m 23m -outline black -fill $rInfo(bg)
   $win create oval 9m 17m 15m 23m -outline black -fill $rInfo(bg)
   $win create oval 17m 17m 23m 23m -outline black -fill $rInfo(bg)
   $win create text 4m 20m -tags t7
   $win create text 12m 20m -tags t8
   $win create text 20m 20m -tags t9

   if {$dir == "h"} {
      $win create line  4m   4m  20m   4m -tags perm
      $win create line  4m  12m  20m  12m -tags perm
      $win create line  4m  20m  20m  20m -tags perm
   } else {
      $win create line  4m  4m  4m  20m -tags perm
      $win create line  12m  4m  12m  20m -tags perm
      $win create line  20m  4m  20m  20m -tags perm
   }
   $win lower perm

}

proc selectRack {index} {
   global rInfo env
   if {$rInfo(rackIndex) != 0} {
      .rframe$rInfo(rackIndex) configure -relief raised
   }
   set rInfo(rackIndex) $index
   .rframe$rInfo(rackIndex) configure -relief sunken
   set fd [open $env(vnmrsystem)/asm/info/racks w]
   for {set i 1} {$i <= 5} {incr i} {
      if {$rInfo(rack,$i) != "none"} {
         puts $fd "gRackLocTypeMap $i $rInfo(rack,$i)"
      }
   }
   close $fd
}

proc drawRacks {win} {
   global env rInfo
   source $env(vnmrsystem)/asm/racks/rackInfo
   foreach el $rackInfo(types) {
      set els {}
      for {set i 1} {$i <= $rackInfo($el,zones)} {incr i} {
         set tot [expr $rackInfo($el,$i,row) * $rackInfo($el,$i,row)]
         if {[lsearch $els tol] == -1} {
            lappend els $tot
         }
      }
      set rInfo($el,numZones) [llength $els]
      
   }
   frame .rmenu
   for {set i 1} {$i <= 5} {incr i} {
      frame .rframe$i -relief raised -borderwidth 2
      label .rlab$i -text "Rack $i" -anchor w
      eval {tk_optionMenu .rk$i rInfo(rack,$i) none} $rackInfo(types)
      .rk$i configure -width 5
      bind .rk$i.menu <ButtonRelease-1> "+after 1 selectRack $i"
      pack .rlab$i .rk$i -in .rframe$i -side top -padx 5 -anchor w
      pack .rframe$i -in .rmenu -side left -padx 5 -fill y
      bind .rframe$i <Button-1> "selectRack $i"
      frame .rlabs$i
      frame .val$i
      label .rlab3$i -text "Zones: "
      label .rvalz0$i -textvariable rInfo($i,zones)
      label .rvalz1$i -textvariable rInfo($i,zone,1)
      label .rvalz2$i -textvariable rInfo($i,zone,2)
      label .rvalz3$i -textvariable rInfo($i,zone,3)
      pack .rlab3$i -in .rlabs$i -side top -anchor nw
      pack .rvalz0$i .rvalz1$i .rvalz2$i .rvalz3$i -in .val$i -side top -anchor w
      pack .rlabs$i .val$i -in .rframe$i -side left -anchor nw
   }
   pack .rmenu -in $win -side top
   set rInfo(rackIndex) 0
}

proc rackPanel {} {
   global rInfo
   frame .racks
   drawRacks .racks
   frame .info
   frame .start
   radiobutton .st1 -text "Left & Back" -command numberCircles\
          -variable rInfo(start) -value 1 -highlightthickness 0
   radiobutton .st2 -text "Right & Back" -command numberCircles\
          -variable rInfo(start) -value 2 -highlightthickness 0
   radiobutton .st3 -text "Left & Front" -command numberCircles\
          -variable rInfo(start) -value 3 -highlightthickness 0
   radiobutton .st4 -text "Right & Front" -command numberCircles\
          -variable rInfo(start) -value 4 -highlightthickness 0
   set rInfo(fg) [.st1 cget -foreground]
   set rInfo(bg) [.st1 cget -background]
   grid .st1 -row 0 -column 0 -in .start -sticky w
   grid .st2 -row 0 -column 1 -in .start -sticky w
   grid .st3 -row 1 -column 0 -in .start -sticky w
   grid .st4 -row 1 -column 1 -in .start -sticky w
   pack .start -in .info -side top -padx 10 -pady 10
   frame .pattern
   canvas .p1 -width 24m -height 24m -bd 3 -relief raised
   canvas .p2 -width 24m -height 24m -bd 3 -relief raised
   canvas .p3 -width 24m -height 24m -bd 3 -relief raised
   canvas .p4 -width 24m -height 24m -bd 3 -relief raised
   bind .p1 <Button-1> "selectPattern 1"
   bind .p2 <Button-1> "selectPattern 2"
   bind .p3 <Button-1> "selectPattern 3"
   bind .p4 <Button-1> "selectPattern 4"
   addcircles .p1 h
   addcircles .p2 h
   addcircles .p3 v
   addcircles .p4 v
   pack .p1 .p2 .p3 .p4 -in .pattern -side left -padx 5 -pady 5
   pack .pattern -in .info -side top
   frame .numb
   label .nb -text "Number locations with: "
   radiobutton .nb1 -text "Numerals (1-96)" \
          -variable rInfo(number) -value 1 -highlightthickness 0
   radiobutton .nb2 -text "Names (A1-H12)" \
          -variable rInfo(number) -value 2 -highlightthickness 0
   pack .nb .nb1 .nb2 -in .numb -side left
   pack .numb -in .info -side top

}

rackPanel
initRInfo
set rInfo(start) 1
setRinfo 1 201
numberCircles
pack .racks .info -side top -fill x -expand 1
