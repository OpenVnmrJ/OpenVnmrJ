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
# enter

eval destroy [winfo child .]
wm title . "Sample Entry Form"
wm iconname . "enter"
wm minsize . 1 1
catch {option readfile $env(vnmrsystem)/app-defaults/Enter}
catch {option readfile $env(HOME)/app-defaults/Enter}

set maxNameLen 0
set itemIndex -1
set itemVal(0) 0
set itemType(0) user
set itemStyle(0) check
set itemRequired(0) 1
set vrack 1
set vzone 1
set usedLoc($vrack,$vzone,0) 0
set nextLoc 1
set cntExps 0
set newExps 0
set prevExps 0
set saveResultInfo 0
set fInfo(names) {}
set fInfo(num) 0

proc readInfo {fileName PlistName PlistValue} {
   global env fInfo
   upvar $PlistName listName
   upvar $PlistValue listValue

   if {[file readable $env(vnmruser)/asm/$fileName ] != 0} {
      set f [open $env(vnmruser)/asm/$fileName]
#      lappend fInfo(names) $env(vnmruser)/asm/$fileName
   } else {
      set f [open $env(vnmrsystem)/asm/$fileName]
#      lappend fInfo(names) $env(vnmrsystem)/asm/$fileName
   }
#   incr fInfo(num)
#   set i $fInfo(num)
   set NameLength 0
   while {[gets $f line] >= 0} {
      if {$NameLength < [string length $line] } {
         set NameLength [string length $line]
      }
      lappend listName "$line"
#      set fInfo($i,n) "$line"
      gets $f line
      lappend listValue "$line"
#      set fInfo($i,v) "$line"
   }
   close $f
   set NameLength
}

proc getInfo nm {
   upvar $nm a
   global maxNameLen eInfo

   foreach i [array names a] {
      set eInfo($nm,$i) $a($i)
   }
   if {(($a(style) != "textentry") && ($a(style) != "info")) || \
       ($a(id) == "loc")} {
     if {$a(file) != ""} {
        set NameLen [readInfo $a(file) List Value]
        set num [llength $List]
        if {([info exists a(max)] == 1) && ($a(max) != "")} {
           if {$num > [expr $a(max) - $a(min) + 1]} {
              unset List
              unset Value
              for {set i $a(min)} {$i <= $a(max)} {incr i} {
                 lappend List $i
                 lappend Value $i
              }
              set NameLen [string length $a(max)]
              set num [llength $List]
           }
        }
     } else {
       for {set i $a(min)} {$i <= $a(max)} {incr i} {
          lappend List $i
          lappend Value $i
       }
       set NameLen [string length $a(max)]
       set num [llength $List]
     }
     set eInfo($nm,num) $num
     catch {unset eInfo($nm,list)}
     catch {unset eInfo($nm,value)}
     for {set i 0} {$i < $num} {incr i} {
        lappend eInfo($nm,list) [lindex $List $i]
        lappend eInfo($nm,value) [lindex $Value $i]
     }
     set eInfo($nm,index) 0
     set eInfo($nm,width) $NameLen
     if {$NameLen > $maxNameLen} {
        set maxNameLen $NameLen
     }
   }
}

proc buttonThreeDoubleLoc {nm index} {
   global eInfo
   set startIndex [lsearch $eInfo(locOrder) [expr $index+1]]
   for {} {$startIndex >= 0} {incr startIndex -1} {
      set butIndex [expr [lindex $eInfo(locOrder) $startIndex] -1]
      set t [set [.but_${nm}($butIndex) cget -variable]]
      if {1 ==  $t} {
         .but_${nm}($butIndex) invoke
      } else {
         set t [.but_${nm}($butIndex) cget -state]
         if {$t == "normal"} {
            return
         }
      }
   }
}

proc buttonThreeLoc {nm index} {
   global eInfo
   set startIndex [lsearch $eInfo(locOrder) [expr $index+1]]
   for {} {$startIndex >= 0} {incr startIndex -1} {
      set butIndex [expr [lindex $eInfo(locOrder) $startIndex] -1]
      set t [set [.but_${nm}($butIndex) cget -variable]]
      if {1 ==  $t} {
         return
      } else {
         .but_${nm}($butIndex) invoke
      }
   }
}

proc callVnmr {nm i onoff} {
   global eInfo vnmrsendOK filename
   if {$vnmrsendOK == 0} {
      return
   }
   set var $eInfo($nm,id)
   global $var
   set index [expr $i+1]
   set label [.but_${nm}($i) cget -text]
   set value [lindex $eInfo($nm,value) $i]
   if {$onoff == 1} {
      if {$eInfo($nm,$i) == 1} {
         if {[info exists eInfo($nm,set_rtoutput)] == 1} {
            vnmrsend [subst $eInfo($nm,set_rtoutput)]
         }
      } else {
         if {[info exists eInfo($nm,unset_rtoutput)] == 1} {
            vnmrsend [subst $eInfo($nm,unset_rtoutput)]
         }
      }
   } else {
      vnmrsend [subst $eInfo($nm,rtoutput)]
   }
}

proc fillLocList {} {
   global eInfo
   set rowLen $eInfo(col)
   catch {unset eInfo(locOrder)}
   if {$eInfo(pattern) == 1} {
      if {$eInfo(start) == 1} {
         for {set r 1} {$r <= $eInfo(row)} {incr r} {
            for {set c 1} {$c <= $eInfo(col)} {incr c} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } elseif {$eInfo(start) == 2} {
         for {set r 1} {$r <= $eInfo(row)} {incr r} {
            for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } elseif {$eInfo(start) == 3} {
         for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
            for {set c 1} {$c <= $eInfo(col)} {incr c} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } else {
         for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
            for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      }
   } elseif {$eInfo(pattern) == 2} {
      if {$eInfo(start) == 1} {
         for {set r 1} {$r <= $eInfo(row)} {incr r} {
            for {set c 1} {$c <= $eInfo(col)} {incr c} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr r
            if {$r <= $eInfo(row)} {
               for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } elseif {$eInfo(start) == 2} {
         for {set r 1} {$r <= $eInfo(row)} {incr r} {
            for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr r
            if {$r <= $eInfo(row)} {
               for {set c 1} {$c <= $eInfo(col)} {incr c} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } elseif {$eInfo(start) == 3} {
         for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
            for {set c 1} {$c <= $eInfo(col)} {incr c} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr r -1
            if {$r >= 1} {
               for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } else {
         for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
            for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr r -1
            if {$r >= 1} {
               for {set c 1} {$c <= $eInfo(col)} {incr c} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      }
   } elseif {$eInfo(pattern) == 3} {
      if {$eInfo(start) == 1} {
         for {set c 1} {$c <= $eInfo(col)} {incr c} {
            for {set r 1} {$r <= $eInfo(row)} {incr r} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } elseif {$eInfo(start) == 2} {
         for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
            for {set r 1} {$r <= $eInfo(row)} {incr r} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } elseif {$eInfo(start) == 3} {
         for {set c 1} {$c <= $eInfo(col)} {incr c} {
            for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      } else {
         for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
            for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
         }
      }
   } else {
      if {$eInfo(start) == 1} {
         for {set c 1} {$c <= $eInfo(col)} {incr c} {
            for {set r 1} {$r <= $eInfo(row)} {incr r} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr c
            if {$c <= $eInfo(col)} {
               for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } elseif {$eInfo(start) == 2} {
         for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
            for {set r 1} {$r <= $eInfo(row)} {incr r} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr c -1
            if {$c >= 1} {
               for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } elseif {$eInfo(start) == 3} {
         for {set c 1} {$c <= $eInfo(col)} {incr c} {
            for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr c
            if {$c <= $eInfo(col)} {
               for {set r 1} {$r <= $eInfo(row)} {incr r} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      } else {
         for {set c $eInfo(col)} {$c >= 1} {incr c -1} {
            for {set r $eInfo(row)} {$r >= 1} {incr r -1} {
               lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
            }
            incr c -1
            if {$c >= 1} {
               for {set r 1} {$r <= $eInfo(row)} {incr r} {
                  lappend eInfo(locOrder) [expr ($r-1) * $rowLen + $c]
               }
            }
         }
      }
   }
}

proc setLocs {Rack Zone} {
   global eInfo vrack vzone usedLoc rackInfo loc
   for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
      set val [lindex $eInfo(loc,value) $i]
      set rackInfo($vrack,$vzone,[expr $i+1]) $usedLoc($vrack,$vzone,$val) 
   }
   set vrack $Rack
   set vzone $Zone
   set loc(max) $rackInfo($vrack,$vzone,num)
   set loc(file) $rackInfo($vrack,file)
   getInfo loc
   set eInfo(pattern) $rackInfo($vrack,pattern)
   set eInfo(start) $rackInfo($vrack,start)
   set eInfo(row) $rackInfo($vrack,$vzone,row)
   set eInfo(col) $rackInfo($vrack,$vzone,col)
   fillLocList
   for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
      set val [lindex $eInfo(loc,value) $i]
      set usedLoc($vrack,$vzone,$val) $rackInfo($vrack,$vzone,[expr $i+1])
      if {$usedLoc($vrack,$vzone,$val) == 1} {
         .but_loc($i) configure -state disabled
      } else {
         .but_loc($i) configure -state normal
      }
      .but_loc($i) configure -text [lindex $eInfo(loc,list) $i]
      set eInfo(loc,$i) 0
   }  
   for {} {$i < $rackInfo(locmax)} {incr i} {
      .but_loc($i) configure -state disabled
      set eInfo(loc,$i) 0
   }
}

proc setZones {index} {
   global eInfo vrack vzone usedLoc rack zone rackInfo
   for {set i 1} {$i <= $rackInfo($index,zones)} {incr i} {
      .but_zone([expr $i-1]) configure -state normal
      .but_zone([expr $i-1]) configure -command "setLocs $index $i"
   }
   .but_zone(0) invoke
   for {} {$i <= $zone(max)} {incr i} {
      .but_zone([expr $i-1]) configure -state disabled
   }
}

proc saveRackInfo {index} {
   global eInfo vrack vzone usedLoc rack zone rackInfo
}

proc getRackInfo {index} {
   global eInfo vrack vzone usedLoc rack zone rackInfo

   for {set i 1} {$i <= $rackInfo(num)} {incr i} {
      for {set j 1} {$j <= $rackInfo($i,zones)} {incr j} {
         for {set k 0} {$k < $rackInfo($i,$j,num)} {incr k} {
            set usedLoc($i,$j,[lindex $eInfo(loc,value) $k]) 0
         }
      }
   }
}

proc checkMultiRack {} {
   global env eInfo vrack vzone rack zone rackInfo sms

   if {[info exists rack(output)] == 0} {
      return
   }
   set rackInfo(default) "file"
   if {[file readable $env(vnmruser)/asm/info/${sms}currentRacks ] != 0} {
      source $env(vnmruser)/asm/info/${sms}currentRacks
   } elseif { \
       [file readable $env(vnmrsystem)/asm/info/${sms}currentRacks ] != 0} {
      source $env(vnmrsystem)/asm/info/${sms}currentRacks
   } else {
      set rackInfo(num) 1
      set rackInfo(1,zones) 0
      set rackInfo(1,1,num) 96
      set rackInfo(default) "conf"
      .errmess config -text "Racks have not been configured. Use gilson" -fg red
      return
   }
   for {set i $rackInfo(num)} {$i >= 1} {incr i -1} {
      if {$rackInfo($i,zones) == 0} {
         incr rackInfo(num) -1
      } else {
         break
      }
   }
   if {$rackInfo(num) == 0} {
      set rackInfo(num) 1
      set rackInfo(1,zones) 0
      set rackInfo(1,1,num) 96
      set rackInfo(default) "conf"
      .errmess config -text "Racks have not been configured. Use gilson" \
                      -fg red
      return
   }

   set rack(max) $rackInfo(num)
   set maxZone 1
   for {set i 1} {$i <= $rackInfo(num)} {incr i} {
      if {$rackInfo($i,zones) > $maxZone} {
         set maxZone $rackInfo($i,zones)
      }
   }
   set zone(max) $maxZone
}

proc setMultiRack {} {
   global eInfo vrack vzone usedLoc rack zone rackInfo itemVal
   catch {unset eInfo(locOrder)}
   for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
      set usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) 0
      lappend eInfo(locOrder) [expr $i+1]
   }
   if {[info exists rack(output)] == 0} {
      return
   }
   if {$rackInfo(default) == "conf"} {
      return
   }
   for {set i 1} {$i <= $rackInfo(num)} {incr i} {
      if {$rackInfo($i,zones) == 0} {
         .but_rack([expr $i-1]) configure -state disabled
      } else {
         .but_rack([expr $i-1]) configure -state normal
         .but_rack([expr $i-1]) configure -command "setZones $i"
      }
      for {set j 1} {$j <= $rackInfo($i,zones)} {incr j} {
         for {set k 1} {$k <= $rackInfo($i,$j,num)} {incr k} {
            set rackInfo($i,$j,$k) 0
         }
      }
   }
   for {} {$i <= $rack(max)} {incr i} {
      .but_rack([expr $i-1]) configure -state disabled
   }
   set rackInfo(locmax) $eInfo(loc,num)
   getRackInfo 1
   .but_rack(0) invoke
}

proc addCheck {nm conf} {
   global maxNameLen eInfo

   set num $eInfo($nm,num)
   set numOnLine $eInfo($nm,numPerLine)
   set rows [expr int((($num+$numOnLine)-1) / $numOnLine)]
   frame .fr_$nm -relief groove -borderwidth 4
   label .lab1_$nm -text "$eInfo($nm,label)"
   pack .lab1_$nm -in .fr_$nm -side top -anchor n
   for {set i 0} {$i < $rows} {incr i} {
      frame .fr1_${nm}($i)
      pack .fr1_${nm}($i) -side top -in .fr_$nm -anchor w
   }
   set rowm1 -1
   if {$conf == 0} {
      set scolor pink
   } else {
      set scolor red
   }
   for {set i 0} {$i < $num} {incr i} {
      set eInfo($nm,$i) 0
      checkbutton .but_${nm}($i) -text [lindex $eInfo($nm,list) $i] \
               -indicatoron $conf -selectcolor $scolor \
               -variable eInfo($nm,$i) \
               -width $eInfo($nm,width) -anchor w
      if {$eInfo($nm,id) == "loc"} {
         bind .but_${nm}($i) <Button-3> "buttonThreeLoc $nm $i"
         bind .but_${nm}($i) <Double-Button-3> \
               "buttonThreeDoubleLoc $nm $i"
      }
      if {[info exists eInfo($nm,set_rtoutput)] == 1} {
         .but_${nm}($i) configure -command "callVnmr $nm $i 1"
      }
      if {$i%$numOnLine == 0} {
         incr rowm1
      }
      pack .but_${nm}($i) -in .fr1_${nm}($rowm1) -side left
   }
   pack .fr_$nm -side top -anchor w -fill x
}

proc addRadio {nm conf Custom} {
   global maxNameLen eInfo
   global itemVal itemIndex

   set num $eInfo($nm,num)

   set numOnLine $eInfo($nm,numPerLine)
   set rows [expr int((($num+$numOnLine)-1) / $numOnLine)]
   frame .fr_$nm -relief groove -borderwidth 4
   label .lab1_$nm -text "$eInfo($nm,label)"
   pack .lab1_$nm -in .fr_$nm -side top -anchor n
   for {set i 0} {$i < $rows} {incr i} {
      frame .fr1_${nm}($i)
      pack .fr1_${nm}($i) -side top -in .fr_$nm -anchor w
   }
   set rowm1 -1
   if {$conf == 0} {
      set scolor pink
   } else {
      set scolor red
   }
   for {set i 0} {$i < $num} {incr i} {
      if {$Custom == 1} {
         radiobutton .but_${nm}($i) -text [lindex $eInfo($nm,list) $i] \
               -variable itemVal($itemIndex) \
                -value [lindex $eInfo($nm,value) $i] \
               -indicatoron $conf -selectcolor $scolor \
               -width $eInfo($nm,width) -anchor w
      } else {
         radiobutton .but_${nm}($i) -text [lindex $eInfo($nm,list) $i] \
               -variable itemVal($itemIndex) -value $i \
               -indicatoron $conf -selectcolor $scolor \
               -width $eInfo($nm,width) -anchor w
      }
      if {($Custom == 0) && ([info exists eInfo($nm,rtoutput)] == 1)} {
         .but_${nm}($i) configure -command "callVnmr $nm $i 0"
      }
      if {$i%$numOnLine == 0} {
         incr rowm1
      }
      pack .but_${nm}($i) -in .fr1_${nm}($rowm1) -side left
   }
   pack .fr_$nm -side top -anchor w -fill x
   if {$Custom == 1} {
      label .lab2_$nm -text "$eInfo($nm,label2)"
      pack .lab2_$nm -in .fr_$nm -side left -anchor n
      entry .text_$nm -width $eInfo($nm,col) \
                -textvariable itemVal($itemIndex) \
               -relief sunken -bd 2
      pack .text_$nm -in .fr_$nm -side left -anchor n
      set itemVal($itemIndex) ""
   }
}

proc addEntry {nm} {
   global itemVal itemIndex eInfo

   frame .fr_$nm -relief groove
   label .lab1_$nm -text "$eInfo($nm,label)"
   pack .lab1_$nm -in .fr_$nm -side left -anchor n
   entry .text_$nm -width $eInfo($nm,col)  -textvariable itemVal($itemIndex) \
               -relief sunken -bd 2
   pack .text_$nm -in .fr_$nm -side left -anchor n
   pack .fr_$nm -side top -anchor w -fill x
}

proc addInfo {nm} {
   global eInfo nextLoc

   frame .fr_$nm -relief groove
   label .lab1_$nm -text "$eInfo($nm,label)"
   if {($nm == "loc") && \
       ([info exists eInfo($nm,usenextloc)] == 1)} {
      label .lab2_$nm -textvariable nextLoc \
            -foreground red -font courb100 -relief groove -padx 4
      pack .lab1_$nm .lab2_$nm -in .fr_$nm -side left -anchor n
   } else {
      label .lab2_$nm -text "$eInfo($nm,value)"
      pack .lab1_$nm .lab2_$nm -in .fr_$nm -side left -anchor n
   }
   pack .fr_$nm -side top -anchor w -fill x
}

proc addFrame {nm} {
   global itemIndex itemVal itemType itemStyle itemRequired
   global eInfo

   if {$eInfo($nm,style) == "info"} {
      addInfo $nm
   } else {
   switch $eInfo($nm,style) {
      check  {
                incr itemIndex
                set itemVal($itemIndex) $eInfo($nm,num)
                set itemType($itemIndex) $nm
                addCheck $nm 1
             }
      radio  {
                incr itemIndex
                set itemVal($itemIndex) $eInfo($nm,num)
                set itemType($itemIndex) $nm
                addCheck $nm 0
             }
      xcheck {
                incr itemIndex
                set itemVal($itemIndex) -1
                set itemType($itemIndex) $nm
                addRadio $nm 1 0
             }
      xcheckcustom {
                incr itemIndex
                set itemVal($itemIndex) -1
                set itemType($itemIndex) $nm
                addRadio $nm 1 1
             }
      xradio {
                incr itemIndex
                set itemVal($itemIndex) -1
                set itemType($itemIndex) $nm
                addRadio $nm 0 0
             }
      xradiocustom {
                incr itemIndex
                set itemVal($itemIndex) -1
                set itemType($itemIndex) $nm
                addRadio $nm 0 1
             }
      textentry {
                incr itemIndex
                set itemVal($itemIndex) ""
                set itemType($itemIndex) $nm
                addEntry $nm
             }
   }
   set itemStyle($itemIndex) $eInfo($nm,style)
   set itemRequired($itemIndex) $eInfo($nm,required)
   }
}

proc addCntrl indexName {
   upvar #0 $indexName a
   global vnmrsendOK filename results
   if {$a(id) == "vnmrexec"} {
      set a(id) $indexName
      button .cntrls.$a(id) -text $a(label) -command "vnmrsend \"$a(rtoutput)\""
      if {$vnmrsendOK == 0} {
         .cntrls.$a(id) configure -state disabled
      }
   } else {
      button .cntrls.$a(id) -text $a(label) -command $a(id)
   }
   if {$a(id) == "saveAndExit"} {
      if {($results(edit) == "no") || ($results(show) == "no")} {
         .cntrls.$a(id) configure -state disabled
      }
   }
   pack .cntrls.$a(id) -in .cntrls -side left -expand 1 -pady 2m
}

proc countLocs {} {
   global eInfo loc
  
   set usedloc 1
   if {($loc(style) == "check") || ($loc(style) == "radio") } {
      set maxloc $eInfo(loc,num)
      if {$maxloc >= 1} {
        set usedloc 0
        for {set i 0} {$i < $maxloc} {incr i} {
           set t [set [ .but_loc($i) cget -variable]]
           if {1 ==  $t} {
              set t [.but_loc($i) cget -state]
              if {$t == "normal"} {
                 incr usedloc
              }
           }
        }
     }
   }
   return $usedloc
}

proc countExps {} {
   global eInfo exp

   set usedexp 1
   if {($exp(style) == "check") || ($exp(style) == "radio") } {
      set maxexp $eInfo(exp,num)
      set usedexp 0
      for {set i 0} {$i < $maxexp} {incr i} {
         set t [set [ .but_exp($i) cget -variable]]
         if {1 ==  $t} {
            incr usedexp
         }
      }
   }
   return $usedexp
}

proc getLocValue {max num} {
   global eInfo
   set ret -1
   set count 0
   for {set j 0} {$j < $max} {incr j} {
      set butIndex [expr [lindex $eInfo(locOrder) $j] -1]
      set t [set [ .but_loc($butIndex) cget -variable]]
      if {1 ==  $t} {
         set t [.but_loc($butIndex) cget -state]
         if {$t == "normal"} {
            incr count
            if {$count == $num} {
               return $butIndex
            }
         }
      }
   }
   return $ret
}

proc getAllValue {index num} {
   global itemVal itemStyle itemType eInfo

   set ret {}
   set nm $itemType($index)
   for {set j 0} {$j < $itemVal($index)} {incr j} {
      set t [set [ .but_${nm}($j) cget -variable]]
      if {1 ==  $t} {
        set t [.but_${nm}($j) cget -state]
           if {$t == "normal"} {
              lappend ret $j
           }
      }
   }
   return $ret
}

proc getValue {index num} {
   global itemVal itemStyle itemType eInfo

   set ret -1
   switch $itemStyle($index) {
      check  -
      radio  {
                set count 0
                set nm $itemType($index)
                if {$nm == "loc"} {
                   set ret [getLocValue $itemVal($index) $num]
                } else {
                   for {set j 0} {$j < $itemVal($index)} {incr j} {
                      set t [set [ .but_${nm}($j) cget -variable]]
                      if {1 ==  $t} {
                         set t [.but_${nm}($j) cget -state]
                         if {$t == "normal"} {
                            incr count
                            if {$count == $num} {
                               set ret $j
                            }
                         }
                      }
                   }
                }
             }
      xcheck -
      xradio {
               set ret $itemVal($index)
             }
      xcheckcustom -
      xradiocustom -
      textentry {
                if {$itemVal($index) == ""} {
                   set ret -1
                } else {
                   set ret [string length $itemVal($index)]
                }
             }
   }
   return $ret
}

proc setDoneLoc {fname} {
   global argv usedLoc loc prevExps results eInfo rackInfo vrack vzone rack zone
 
   set pattern $loc(output)
   if {[file readable $fname ] == 0} {
      return
   } 
   if {[info exists rack(output)] == 1} {
      set RandZ 1
   } else {
      set RandZ 0
      set rackVal $vrack
      set zoneVal $vzone
   }
   set f [open $fname r]
   while { [gets $f line] >= 0} {
      if {$RandZ == 1} {
         scan $line "%11s %s %d %d" label value rackVal zoneVal
      } else {
         scan $line "%11s %s" label value
      }
      if {$label == $pattern} {
         set usedLoc($rackVal,$zoneVal,$value) 1
         if {$RandZ == 1} {
            set i [lsearch $eInfo(loc,value) $value]
            incr i
            set rackInfo($rackVal,$zoneVal,$i) 1
         }
      }  
   } 
   close $f
   if {(($loc(duplicates) == 0) && (($loc(style) == "xradio") || \
        ($loc(style) == "xcheck") || ($loc(style) == "radio") || \
        ($loc(style) == "check"))) } {
      for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
         if {$usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) == 1} {
            .but_loc($i) configure -state disabled
         }
      }  
   }
}

proc setDoneLocs {} {
   global exList argv
   catch { foreach ex $exList {
              set firstChar [string index $ex 0]
              if {$firstChar == "/"} {
                 setDoneLoc $ex
              } else {
                 setDoneLoc [format "%s/$ex" [file dirname [lindex $argv 0]]]
              }
           }
   }
}

proc setUsedLoc {fname} {
   global argv usedLoc loc prevExps results eInfo rackInfo vrack vzone rack zone

   set prevExps 0
   set pattern $loc(output)
   if {[info exists rack(output)] == 1} {
      set RandZ 1
      set rPattern $rack(output)
      set zPattern $zone(output)
   } else {
      set RandZ 0
      set rPattern ""
      set zPattern ""
   }
   set f [open $fname r]
   if {$results(edit) == "no"} {
      .text configure -state normal
   }
   .text delete 1.0 end
   set outValue ""
   set rackValue ""
   set zoneValue ""
   while { [gets $f line] >= 0} {
      scan $line "%11s %s" label value
      if {$results(content) == "all"} {
         .text insert end "$line\n"
      }
      if {$label == $pattern} {
         if {$RandZ == 1} {
            set outvalue $value
         } else {
            set usedLoc($vrack,$vzone,$value) 1
         }
         incr prevExps
      } elseif {$label == $rPattern} {
         set rackValue $value
      } elseif {$label == $zPattern} {
         set zoneValue $value
      }
      if {($RandZ == 1) && ($outvalue != "") && ($rackValue != "") && \
          ($zoneValue != "")} {
         set usedLoc($rackValue,$zoneValue,$outvalue) 1
         set i [lsearch $eInfo(loc,value) $outvalue]
         incr i
         set rackInfo($rackValue,$zoneValue,$i) 1
         set outvalue ""
         set rackValue ""
         set zoneValue ""
      }
   }
   if {$results(edit) == "no"} {
      .text configure -state disabled
   }
   if {(($loc(duplicates) == 0) && (($loc(style) == "xradio") || \
        ($loc(style) == "xcheck") || ($loc(style) == "radio") || \
        ($loc(style) == "check"))) } {
      for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
         if {$usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) == 1} {
            .but_loc($i) configure -state disabled
         }
      }
   }
}

proc setNextLoc {recurse} {
   global nextLoc usedLoc loc argv locList eInfo vrack vzone rack
   if {[info exists rack(output)] == 1} {
      return
   }
   for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
      if {$usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) == 0} {
         set nextLoc [lindex $eInfo(loc,value) $i]
         if {[catch {set $loc(usenextloc) 1}] == 0} {
            set loc(value) $nextLoc
         }
         return
      }
   }
   if {($loc(errormess) != "") || ($recurse == 0)} {
      set children [pack slaves .fr_loc]
      foreach child $children {
         pack forget $child
      }
      if {$loc(errormess) != ""} {
         label .limit -text $loc(errormess) -foreground red -relief groove
      } else {
         label .limit -text $loc(errormess2) -foreground red -relief groove
      }
      pack .limit -in .fr_loc -fill both
      set children [pack slaves .]
      foreach child $children {
         set res [string first _frame $child]
         if {($res != -1) && ($child != ".fr_loc")} {
            pack forget $child
         }
      }
   } elseif {[llength locList] == 1} {
      catch {exec rm "[format "%s/$locList" [file dirname [lindex $argv 0]]]"}
      for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
         set usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) 0
      }
      setDoneLoc [lindex $argv 0]
      setDoneLocs
      setNextLoc 0
   }
}

proc flash {mess} {
   .errmess config -text $mess
   for {set i 0} {$i < 4} {incr i} {
      .errmess config -fg red
      update idletasks
      after 500
      .errmess config -fg black
      update idletasks
      after 500
   }
   .errmess config -text "   "
}

proc checkExp {} {
   global infields eInfo
   global itemIndex itemVal itemType itemRequired

   for {set index 0} {$index <= $itemIndex} {incr index} {
      if {$itemRequired($index) == 1} {
         set nm $itemType($index)
         set ret [getValue $index 1]
         if {$ret == -1} {
            flash "[string trim $eInfo($nm,label)] must be set"
            return -1
         }
      }
   }
   return 0
}

proc getIndex indexName {
   upvar #0 $indexName a
   global itemIndex itemType

   for {set i 0} {$i <= $itemIndex} {incr i} {
      if {$itemType($i) == $a(id)} {
         return $i
      }
   }
   return -1
}

proc copyFile {infile outfile} {
   set inFD [open $infile r]
   set outFD [open $outfile a+]
   while {[gets $inFD line] >= 0} {
      puts $outFD $line
   }
   close $outFD
   close $inFD
}

proc addexp {update cmd} {
   global itemIndex itemVal itemType itemStyle cntExps newExps prevExps
   global outfields infields argv filename nextLoc
   global usedLoc loc results eInfo vnmrsendOK vrack vzone
   global $cmd
   set j "global [set outfields]"
   eval $j

   set expok [checkExp]
   if {$expok == -1} {
      return -1
   }
   set pattern $loc(output)
   set numExps [countExps]
   set numLocs [countLocs]
   if {$results(edit) == "no"} {
      .text configure -state normal
   }
   for {set locnum 1} {$locnum <= $numLocs} {incr locnum} {
     for {set expnum 1} {$expnum <= $numExps} {incr expnum} {
       foreach index $outfields {
         set iindex [getIndex $index]
         if {$iindex == -1} {
            set ival ""
            if {$eInfo($index,id) == "loc"} {
               set ival $nextLoc
            } else {
               catch {set ival $eInfo($index,value)}
            }
         } elseif {($itemStyle($iindex) == "textentry") || \
                   ($itemStyle($iindex) == "xcheckcustom") || \
                   ($itemStyle($iindex) == "xradiocustom")} {
            set ival $itemVal($iindex)
         } elseif {$eInfo($index,id) == "loc"} {
            set ival [lindex $eInfo($index,value) [getValue $iindex $locnum]]
         } elseif {$eInfo($index,id) == "exp"} {
            set ival [lindex $eInfo($index,value) [getValue $iindex $expnum]]
         } elseif {$eInfo($index,id) == "list"} {
            set ival ""
            set expI [getAllValue $iindex $expnum]
            foreach el $expI {
                set ival "$ival [lindex $eInfo($index,value) $el]"
            }
         } else {
            set ival [lindex $eInfo($index,value) [getValue $iindex 1]]
         }
         set ${index}(val) $ival
         .text insert end "[format "%11s %s" $eInfo($index,output) $ival]\n"
         if {$eInfo($index,output) == $pattern} {
            set usedLoc($vrack,$vzone,$ival) 1
         }
       }
       if {[info exists ${cmd}(rtoutput)] == 1} {
          if {$vnmrsendOK == 1} {
             vnmrsend [subst [set ${cmd}(rtoutput)]]
          }
       }
       incr newExps
       incr cntExps
     }
   }
   if {[info exists ${cmd}(rtoutput2)] == 1} {
      if {$vnmrsendOK == 1} {
         vnmrsend [subst [set ${cmd}(rtoutput2)]]
      }
   }
   if {(($loc(duplicates) == 0) && (($loc(style) == "xradio") || \
        ($loc(style) == "xcheck") || ($loc(style) == "radio") || \
        ($loc(style) == "check"))) } {
      for {set i 0} {$i < $eInfo(loc,num)} {incr i} {
         if {$usedLoc($vrack,$vzone,[lindex $eInfo(loc,value) $i]) == 1} {
            .but_loc($i) configure -state disabled
         }
      }
   }
   if {$update == 1} {
      setNextLoc 1
   }
   .text see [expr ($prevExps + $newExps) * [llength $outfields]].1
   if {$results(edit) == "no"} {
      .text configure -state disabled
   }
   return 0
}

proc addExp {} {
   global newExps .cntrls.saveAndExit
   addexp 1 addExp
   if {$newExps >= 1} {
      catch {.cntrls.saveAndExit configure -state normal}
   }
}

proc saveTextToFile {cmd} {
   global argv results prevExps newExps outfields vnmrsendOK filename
   global $cmd

   if {$results(content) == "all"} {
      set outFD [open [lindex $argv 0] w]
      set start 1
      set last [expr ($prevExps + $newExps) * [llength $outfields]]
   } elseif {$results(content) == "new"} {
      set outFD [open [lindex $argv 0] a+]
      set start 1
      set last [expr $newExps * [llength $outfields]]
   } else {
      set outFD [open [lindex $argv 0] a+]
      set start [expr 1 + $prevExps * [llength $outfields]]
      set last [expr ($prevExps + $newExps) * [llength $outfields]]
   }
   for {set i $start} {$i <= $last} {incr i} {
      set outLine [.text get $i.0 "$i.0 lineend"]
      if {[string length $outLine] > 0} {
         puts $outFD $outLine
      }
   }
   close $outFD
       if {[info exists ${cmd}(rtoutput3)] == 1} {
          if {$vnmrsendOK == 1} {
             vnmrsend [subst [set ${cmd}(rtoutput3)]]
          }
       }
}

proc saveAndExit {} {
   saveTextToFile saveAndExit
   exit 0
}

proc saveAndExit2 {} {
   saveTextToFile saveAndExit2
   exit 0
}

proc saveExp {} {
   global argv saveResultInfo
   set res [addexp 1 saveExp]
   if {$res == 0} {
      set saveResultInfo 1
   }
}

proc addSaveAndExit {} {
   global argv
   set res [addexp 0 addSaveAndExit]
   if {$res == 0} {
      saveTextToFile addSaveAndExit
      exit 0
   }
}
proc quit {} {
   global saveResultInfo
   if {$saveResultInfo == 1} {
      saveTextToFile quit
   }
   exit 0
}

proc saveAutoSample {} {
   global results prevExps newExps vnmrsendOK
   if {$vnmrsendOK == 1} {
      vnmrsend autosa
   }
   if {$results(content) == "new"} {
      set prevExps 0
   } 
   set results(content) "diff"
   saveTextToFile saveAutoSample
   set prevExps [expr $prevExps + $newExps]
   set newExps 0
   if {$vnmrsendOK == 1} {
      vnmrsend autora
   }
}

proc autoSampleNoExit {} {
   set res [addexp 1 autoSampleNoExit]
   if {$res == 0} {
      saveAutoSample
   }
}

proc autoSample {} {
   set res [addexp 0 autoSample]
   if {$res == 0} {
      saveAutoSample
      exit 0
   }
}

proc priortySample {} {
   global argv results prevExps newExps
   global priortySample

#   if {priortySample(passwd) != ""} {
#   } else {
#      set res [checkPasswd priortySample(passwd)]
#   }
#   if {$res != 0} {
#     return
#   }
   if {($results(content) == "new") || ($results(content) == "diff")} {
     set res [addexp 0 priortySample]
     if {$res != 0} {
       return
     }
     vnmrsend autosa
     catch {exec mv "[lindex $argv 0]" "[lindex $argv 0].orig" }
     saveTextToFile priortySample
     copyFile "[lindex $argv 0].orig" "[lindex $argv 0]"
     catch {exec rm "[lindex $argv 0].orig"}
     vnmrsend autora
   } elseif {$results(content) == "all"} {
     set expok [checkExp]
     if {$expok == -1} {
        return
     }
     vnmrsend autosa
     saveTextToFile priortySample
     set prevExps [expr $prevExps + $newExps]
     set newExps 0
     set results(content)  "diff"
     addexp 0 priortySample
     catch {exec mv "[lindex $argv 0]" "[lindex $argv 0].orig" }
     saveTextToFile priortySample
     copyFile "[lindex $argv 0].orig" "[lindex $argv 0]"
     catch {exec rm "[lindex $argv 0].orig"}
     vnmrsend autora
   }
   exit 0
}

if {$argc < 1} {
   puts "A filename must be supplied to $argv0"
   exit 1
}
set fileexists [file exists [lindex $argv 0]]
if {[catch {set outFD [open [lindex $argv 0] a+]} ]} {
   puts "read and write access to [lindex $argv 0] is not allowed"
   exit 1
}
close $outFD
set filename [lindex $argv 0]

set sms ""

if {$argc >= 2} {
   set conffile [lindex $argv 1]
} else {
   set conffile enter.conf
}
if {[file readable $env(vnmruser)/asm/$conffile ] != 0} {
   source $env(vnmruser)/asm/$conffile
} else {
   source $env(vnmrsystem)/asm/$conffile
}
set vnmrsendOK 0
if {$argc >= 3} {
   vnmrinit \"[lindex $argv 2]\" $env(vnmrsystem)
   set vnmrsendOK 1
}

frame .outtext
text .text -yscrollcommand ".scroll set" \
           -height [expr [llength $outfields]  * $results(num)] \
           -width $results(col)
scrollbar .scroll -command ".text yview"
pack .scroll -in .outtext -side right -fill y
pack .text -in .outtext -side left
if {$results(edit) == "no"} {
   .text configure -state disabled
}

label .errmess -text "   "
checkMultiRack
foreach index $infields {
   getInfo $index
   if {[info exists eInfo($index,rtoutput)] == 1} {
      foreach i [array names $index] {
         lappend eInfo($index,rtvals) $i
      }
   }
}
foreach index $outfields {
   if {[info exists eInfo($index,id)] == 0} {
      foreach i [array names $index] {
         set eInfo($index,$i) [set ${index}($i)]
      }
   }
}
foreach index $infields {
   addFrame $index
}
setMultiRack
if {$fileexists == 0} {
   catch {exec rm "[lindex $argv 0]"}
} else {
   setUsedLoc [lindex $argv 0]
}
setDoneLocs
frame .cntrls
foreach index $cntrls {
   addCntrl $index
}
pack .cntrls -side top -expand y -fill x
pack .errmess -side top -expand y -fill x
if {$results(show) == "yes"} {
  pack .outtext -side top
} else {
   label .outtext2 -text "Number of samples submitted: "
   label .outtext3 -textvariable cntExps
   pack .outtext2 .outtext3 -side left -anchor w -fill x
}
setNextLoc 1
