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
# status

eval destroy [winfo child .]
wm title . "Sample Status"
wm iconname . "status"
# wm iconbitmap . @/vnmr/varian.xicon
wm minsize . 1 1
catch {option readfile $env(vnmrsystem)/app-defaults/Status}
catch {option readfile $env(HOME)/app-defaults/Status}
source $tk_library/vnmr/menu2.tk

proc addCntrl indexName {
   upvar #0 $indexName a
   global rtOK

   button .cntrls.$a(id) -text $a(label) -command $a(id)
   if {$a(id) == "rt"} {
      if {$rtOK == "yes"} {
         .cntrls.$a(id) configure -state disabled
         pack .cntrls.$a(id) -in .cntrls -side left -expand 1 -pady 2m
      }
   } else {
      pack .cntrls.$a(id) -in .cntrls -side left -expand 1 -pady 2m
   }
}

proc initCounters {} {
   global ctr outfields

   set ctr(rtfile) ""
   set ctr(queued) 0
   set ctr(active) 0
   set ctr(complete) 0
   set ctr(error) 0
   set ctr(line) 1
   set ctr(tag) 1
   .text configure -state normal
   .logtext configure -state normal
   .text delete 1.0 end
   .logtext delete 1.0 end
}

proc selectEntry {num index} {
   global ctr results outfields statusDir info

   if {[info exists info([lindex $outfields 0],$index)] == 0} {
      return
   }
   if {($ctr(currentEntry) != -1) && ($ctr(currentEntry) != $num)} {
      .text tag configure entry($ctr(currentEntry)) -background $ctr(back)
   }
   set ctr(currentEntry) $num
   .text tag configure entry($num) -background $results(selectcolor)
   .logtext configure -state normal
   .logtext delete 1.0 end
   set logfile ""
   set ctr(currentInfo) $index
   foreach el $outfields {
      .logtext insert end "$info($el,$index)\n"
      if {$el == "data"} {
         set pathname ""
         scan $info($el,$index) "%11s %s" label pathname
         set firstchar [string range $pathname 0 0]
         if {$firstchar != "/"} {
            set pathname $statusDir/$pathname
         }
         if {[file readable $pathname.fid/log] == 1} {
            set logfile $pathname.fid/log
            set ctr(rtfile) $pathname
            catch {.cntrls.rt configure -state normal}
         } else {
            set ctr(rtfile) ""
            catch {.cntrls.rt configure -state disabled}
         }
      }
   }
   if {$logfile != ""} {
      set f [open $logfile r]
      while { [gets $f line] >= 0} {
         .logtext insert end "$line\n"
      }
      close $f
   }
   .logtext configure -state disabled
}

proc readQfile {filename} {
   global ctr outfields info

   if [catch {open $filename r} f ] {
     return
   }
   set index 0
   while { [gets $f line] >= 0} {
      set el [lindex $outfields $index]
      set info($el,$ctr(total)) $line
      incr index
      if {$index == $ctr(linesPerEntry)} {
         set index 0
         incr ctr(total)
      }
   }
   close $f
}

proc qsort2 {type n} {
   global sortlist sK targets tIndex

   set nk [lsort $type $sK]
   catch {unset targets}
   set targets [lindex $nk 0]
   set tIndex 1
   set last $targets
   for {set index 0} {$index < $n} {incr index} {
      set next [lindex $nk $index]
      set sortlist($index) [lsearch $sK $next]
      set sK [lreplace $sK $sortlist($index) $sortlist($index) ""]
      if {$last != $next} {
         lappend targets $next
         lappend tIndex [expr $index+1]
         set last $next
      }
   }
}

proc sortQfile {} {
   global ctr results info sortlist sK

   catch {unset sK}
   for {set i 0} {$i < $ctr(total)} {incr i} {
      set sortlist($i) $i
   }
   if {$results(sort) == "user"} {
      for {set num 0} {$num < $ctr(total)} {incr num} {
         scan $info(user,$num) "%11s %s" label val
         lappend sK $val
      }
      qsort2 -ascii $ctr(total)
   } elseif {$results(sort) == "loc"} {
      for {set num 0} {$num < $ctr(total)} {incr num} {
         scan $info(loc,$num) "%11s %s" label val
         lappend sK $val
      }
      qsort2 -integer $ctr(total)
   } elseif {$results(sort) == "stat"} {
      for {set num 0} {$num < $ctr(total)} {incr num} {
         scan $info(stat,$num) "%11s %s" label val
         lappend sK $val
      }
      qsort2 -ascii $ctr(total)
   }
}

proc showQfile {} {
   global ctr results showfields info sortlist

   sortQfile
   .text configure -state normal
   for {set num 0} {$num < $ctr(total)} {incr num} {
      set index $sortlist($num)
      foreach el $showfields {
         set line $info($el,$index)
         .text insert $ctr(line).0 "$line\n"
         incr ctr(line)
      }
      set line $info(stat,$index)
      scan $line "%11s %s" label curstatus
      .text tag delete entry($num)
      .text tag add entry($num) $ctr(tag).0 $ctr(line).0
      .text tag bind entry($num) <1> "selectEntry $num $index"
      if {$curstatus == "Error"} {
         incr ctr(error)
         .text tag configure entry($num) -foreground $results(errorcolor)
      } elseif {$curstatus == "Queued"} {
         incr ctr(queued)
         .text tag configure entry($num) -foreground $results(queuecolor)
      } elseif {$curstatus == "Complete"} {
         incr ctr(complete)
         .text tag configure entry($num) -foreground $results(completecolor)
      } elseif {$curstatus == "Active"} {
         incr ctr(active)
         .text tag configure entry($num) -foreground $results(activecolor)
      } elseif {$curstatus == "Shimming"} {
         incr ctr(active)
         .text tag configure entry($num) -foreground $results(shimcolor)
      }
      set ctr(tag) $ctr(line)
      .text insert $ctr(line).0 "\n"
      incr ctr(line)
   }
   .text configure -state disabled
}

proc updateQfile {} {
   global ctr statusDir info sortlist

   if {[file exists $statusDir/enterQ] == 0} {
      after 1000 updateQfile
      return
   }
   set enterQsize [file size $statusDir/enterQ]
   if {[file exists $statusDir/doneQ] == 0} {
      set doneQstat -1
   } else {
      set doneQstat [file mtime $statusDir/doneQ]
   }
   if {($enterQsize > $ctr(enterQsize)) || ($doneQstat != $ctr(doneQmstat)) } {
      if {$enterQsize == -1} {
         set fract {0 0}
      } else {
         set fract [.text yview]
      }
      set ctr(total) 0
      catch {unset info}
      initCounters
      readQfile $statusDir/doneQ
      readQfile $statusDir/enterQ
      showQfile
      .text yview moveto [lindex $fract 0]
      if {$ctr(currentEntry) != -1} {
         set lastEntry $ctr(currentEntry)
         set ctr(currentEntry) -1
         set lastInfo $ctr(currentInfo)
         for {set num 0} {$num < $ctr(total)} {incr num} {
            if {$sortlist($num) == $lastInfo} {
               set lastInfo $num
            }
         }
         selectEntry $lastEntry $lastInfo
      }
   }
   set ctr(enterQsize) $enterQsize
   set ctr(doneQmstat) $doneQstat
   after 1000 updateQfile
}

proc setmenu2 {id index} {
   global tIndex ctr
   .text yview  [expr 1+ ([lindex $tIndex $index]-1) * $ctr(showLines)].0
}
proc pickTarget {} {
   global targets

   catch {destroy .locate.menu2}
   menu2 .locate.menu2 $targets setmenu2 0 -text "[lindex $targets 0]" \
         -width 10 -anchor w
   grid .locate.label2 -row 5 -column 0 -in .locate -sticky w
   grid .locate.menu2 -row 5 -column 1 -in .locate -sticky w
}

proc locate {} {
   global results listvar
   if {[winfo exists .locate] != 1} {
      toplevel .locate
      label .locate.label -text "Sort entries "
      radiobutton .locate.rad(1) -text "Chronologically" \
          -variable results(sort) -value std \
          -command "initCounters; showQfile; \
              catch {grid forget .locate.label2 .locate.menu2}"
      radiobutton .locate.rad(2) -text "by User"  \
          -variable results(sort) -value user \
          -command "initCounters; showQfile; pickTarget"
      radiobutton .locate.rad(3) -text "by Location"  \
          -variable results(sort) -value loc \
          -command "initCounters; showQfile; pickTarget"
      radiobutton .locate.rad(4) -text "by Status" \
          -variable results(sort) -value stat \
          -command "initCounters; showQfile; pickTarget"
      grid .locate.label -row 0 -column 0 -in .locate -sticky w
      grid .locate.rad(1) -row 0 -column 1 -in .locate -sticky w
      grid .locate.rad(2) -row 1 -column 1 -in .locate -sticky w
      grid .locate.rad(3) -row 2 -column 1 -in .locate -sticky w
      grid .locate.rad(4) -row 3 -column 1 -in .locate -sticky w
      label .locate.label2 -text "and look for "
      if {$results(sort) != "std"} pickTarget
   
#     button .locate.quit -text "Quit" -command {destroy .locate}
#     grid .locate.quit -row 7 -column 0 -in .locate -sticky w
   }
   set x [expr [winfo rootx .scroll] + [winfo width .scroll]]
   set y [winfo rooty .scroll]
   set width [winfo screenwidth .]
   wm geometry .locate +$x+$y
   update idletasks
   set popWidth [winfo width .locate]
   if {[expr $x + $popWidth + 10] > $width } then {
      set x [expr $width - $popWidth - 10]
   }
   wm geometry .locate +$x+$y
}

proc rt {} {
   global ctr statusDir info

   set pathname ""
   set filename ""
   scan $info(data,$ctr(currentInfo)) "%11s %s" label pathname
   set firstchar [string range $pathname 0 0]
   if {$firstchar != "/"} {
      set pathname $statusDir/$pathname
   }
   if {[file readable $pathname.fid/procpar] == 1} {
      set filename $pathname
   }
   if {$filename != ""} {
      vnmrsend "rt('$pathname') process"
   }
}

proc quit {} {
   exit 0
}

if {$argc < 1} {
   puts "A directory must be supplied to $argv0"
   exit 1
}
set statusDir [lindex $argv 0]
set dirOK [file isdirectory $statusDir]
if {$dirOK == 0} {
   puts "A directory must be supplied to $argv0"
   exit 1
}
if {$argc >= 2} {
   set conffile [lindex $argv 1]
} else {
   set conffile status.conf
}
if {[file readable $env(vnmruser)/asm/$conffile ] != 0} {
   set sourcedir $env(vnmruser)/asm
} else {
   set sourcedir $env(vnmrsystem)/asm
}
source $sourcedir/$conffile
set rtOK no
if {$argc >= 3} {
   vnmrinit \"[lindex $argv 2]\" $env(vnmrsystem)
   set rtOK yes
}

frame .summary
set summ {Queued  Active  Complete  Error  Total}
set summcolor {queuecolor activecolor completecolor errorcolor totalcolor}
set summvar {queued active complete error total}
set num [llength $summ]
label .summarylabel(head) -text "Samples: "
grid columnconfigure .summary 0 -minsize 80 -weight 1
grid .summarylabel(head) -row 0 -column 0 -in .summary
for {set i 0} {$i < $num} {incr i} {
   set j [expr $i + 1]
   label .summarylabel($i) -text [lindex $summ $i] \
       -foreground $results([lindex $summcolor $i])
   label .summaryval($i) -textvariable ctr([lindex $summvar $i])
   grid columnconfigure .summary $j -minsize 80 -weight 1
   grid .summarylabel($i) -row 0 -column $j -in .summary
   grid .summaryval($i) -row 1 -column $j -in .summary
}

set ctr(back) [.summary cget -background]
set ctr(currentEntry) -1
set ctr(currentInfo) -1
set ctr(linesPerEntry) [llength $outfields]
set ctr(showLines) [expr (1 + [llength $showfields])]
set ctr(enterQsize) -1
set ctr(doneQmstat) -1
frame .outtext
text .text -yscrollcommand ".scroll set" \
           -selectborderwidth 0 -selectbackground $ctr(back) \
           -height [expr  $ctr(showLines)  * $results(num)] \
           -width $results(col)
scrollbar .scroll -command ".text yview"
pack .scroll -in .outtext -side right -fill y
pack .text -in .outtext -side left
bind .text <Double-Button-1> {break}

frame .log
label .log.label -text $results(logtitle)
text .logtext -yscrollcommand ".logscroll set" \
           -selectborderwidth 0 -selectbackground $ctr(back) \
           -height $results(rows) \
           -width $results(col)
scrollbar .logscroll -command ".logtext yview"
pack .log.label
pack .logscroll -in .log -side right -fill y
pack .logtext -in .log -side left -fill both -expand 1

frame .cntrls
foreach index $cntrls {
   addCntrl $index
}

pack .summary -side top -anchor w
pack .outtext -side top
pack .log -side top
pack .cntrls -side top -expand y -fill x
updateQfile
