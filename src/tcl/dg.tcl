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
# dg

eval destroy [winfo child .]
wm iconname . "Display"
wm minsize . 1 1
catch {option readfile [file join $env(vnmrsystem) app-defaults Dg]}
catch {option readfile [file join $env(HOME) app-defaults Dg]}
set dispFile(name) [lindex $argv 1]
set curexp [file join $env(vnmruser) exp1]
set seqfil "s2pul"
set dgLocal(getvar) 0
set dgLocal(curvar) 0

source [file join $tk_library vnmr deck.tk]
source [file join $tk_library vnmr menu2.tk]
source [file join $tk_library vnmr scroll2.tk]

proc pointerFocus {} {
  tk_focusFollowsMouse
}

proc dgServer {} {
   global dgLocal
   set port [pid]
   while {$port > 65535} {
      set port [expr $port - 65536]
   }
   if {$port < 3000} {
      set port [expr $port + 24484]
   }
   set dgLocal(server) [socket -server dgAccept $port]
   vnmrsend "initTclDg($port) tcl('set seqfil '+seqfil)"
}

proc dgAccept {sock addr port} {
   global dgLocal
   fconfigure $sock -buffering line
   fileevent $sock readable [list dgCmd $sock]
}

proc dgCmd {sock} {
   if {[eof $sock] || [catch {gets $sock line}]} {
      close $sock
   } else {
      #uplevel [string range $line 0 [expr [string length $line]-2]]
      set xx [string trimright $line "\x00"] 
      set line $xx
      if {$line == "getVarFromVnmr"} {
         after cancel getVarFromVnmr
         after idle getVarFromVnmr
      } else {
         uplevel $line
      }
      close $sock
   }
}

proc resizeDg {} {
   global dgTemplate dgLocal
   set height [winfo height .dgmain]
   set width [winfo width .dgmain]
   if {$dgLocal(side) == "horiz"} {
      set height [expr $height - [winfo height .dgmain.menu]]
   } elseif {$dgLocal(side) == "vert"} {
      set width [expr $width - [winfo width .dgmain.menu]]
   }
   set size [expr $height / $dgTemplate(rows)]
   for {set i 0} {$i < $dgTemplate(rows)} {incr i} {
      grid rowconfigure $dgTemplate(win) $i -weight 1 -minsize $size
   }
   set size [expr $width / $dgTemplate(cols)]
   for {set i 0} {$i < $dgTemplate(cols)} {incr i} {
      grid columnconfigure $dgTemplate(win) $i -weight 1 -minsize $size
   }
}

proc vnmrDgroup {var} {
   global dgTemplate dgVnmr dgExec

   set dgroup 1
   if {[lsearch -exact $dgExec(vnmrVar) $var] != -1} {
      set dgroup $dgVnmr($var,dgroup)
   }
   return $dgroup
}

proc vnmrSize {var} {
   global dgTemplate dgVnmr dgExec

   set size 1
   if {[lsearch -exact $dgExec(vnmrVar) $var] != -1} {
      set size $dgVnmr($var,size)
   }
   return $size
}

proc vnmrMax {var} {
   global dgTemplate dgVnmr dgExec

   set size 1
   if {[lsearch -exact $dgExec(vnmrVar) $var] != -1} {
      set max $dgVnmr($var,max)
   }
   return $max
}

proc vnmrMin {var} {
   global dgTemplate dgVnmr dgExec

   set size 1
   if {[lsearch -exact $dgExec(vnmrVar) $var] != -1} {
      set min $dgVnmr($var,min)
   }
   return $min
}

proc vnmrOn {var} {
   global dgTemplate dgVnmr dgExec

   set onval y
   if {[lsearch -exact $dgExec(vnmrVar) $var] != -1} {
      set onval $dgVnmr($var,on)
   }
   return [expr {($onval == "y") ? 1 : 0}]
}

proc vnmrUnits {var} {
   global dgVnmr

   set index [vnmrDgroup $var]
   switch $index {
      1 {return 1.0}
      2 {return 1e3}
      3 {return 1e6}
      4 {return $dgVnmr(reffrq,val)}
      5 {return $dgVnmr(reffrq1,val)}
      6 {return $dgVnmr(reffrq2,val)}
      default {return 1.0}
   }
}

proc updateVars {index} {
   global dgTemplate dgExec

   set num [llength $dgTemplate(vnmrVar,$index)]
   for {set i 0} {$i < $num} {incr i} {
      set tmp [lindex $dgTemplate(vnmrVar,$index) $i]
      if {[lsearch -exact $dgExec(vnmrVar) $tmp] == -1} {
         lappend dgExec(vnmrVar) $tmp
      }
   }
   lappend dgExec(index) $index
   set dgTemplate(update,$index) 1
}

proc checkSize {index} {
   global dgTemplate dgVnmr dgExec dgLocal
   if {[info exists dgTemplate(vnmrVar,$index)] == 1} {
      set var [lindex $dgTemplate(vnmrVar,$index) 0]
      set size $dgVnmr($var,size)
      if {$size == 0} {
         eval $dgExec(eField,$index) configure -background $dgLocal(nc)
      } elseif {$size == 1} {
         eval $dgExec(eField,$index) configure -background $dgLocal(bg)
      } else {
         eval $dgExec(eField,$index) configure -background $dgLocal(ac)
      }
   }
}

proc restartGetVar {index} {
   global dgLocal
   if {$dgLocal(curvar) == $index} {
      set dgLocal(getvar) 1
      set dgLocal(curvar) 0
   }
}

proc getVarFromVnmr {} {
   global dgTemplate dgVnmr dgExec dgLocal

   if {$dgLocal(getvar) == 0} {
      return
   }
   getmagicvars
   set num [llength $dgExec(vnmrVar)]
   for {set i 0} {$i < $num} {incr i} {
      set name [lindex $dgExec(vnmrVar) $i]
      set $name $dgVnmr($name,val)
   }
   set num [llength $dgExec(index)]
   for {set i 0} {$i < $num} {incr i} {
      set index [lindex $dgExec(index) $i]
      if {$dgTemplate(update,$index) == 1} {
         set tmp {}
         catch {set tmp "[subst $dgTemplate(tclSet,$index)]"}
         set dgTemplate(value,$index) $tmp
      }
      if {[info exists dgExec(eField,$index)] == 1} {
         if {[info exists dgExec(eField2,$index)] == 1} {
            eval $dgExec(eField2,$index)
         } elseif {[info exists dgExec(eField1,$index)] == 1} {
            eval set tmp $dgTemplate(showVal,$index)
            if {$tmp == 1} {
               eval $dgExec(eField,$index) $dgExec(eField1,$index)
               checkSize $index
            } else {
               eval $dgExec(eField,$index) $dgExec(eField0,$index)
            }
         } else {
            checkSize $index
         }
      } elseif {[info exists dgExec(sField,$index)] == 1} {
         eval "$dgExec(sField,$index) configure $dgTemplate(scale,$index)"
      }
   }
   update idletasks
}

proc turnOffUpdate {index} {
   global dgTemplate
   set dgTemplate(update,$index) 0
}

proc sendVarToVnmr {index sendfocus} {
   global dgTemplate dgVnmr

   if {$dgTemplate(key,$index) == 0} {
      return
   }
   set dgTemplate(key,$index) 0
   if {[info exists dgTemplate(vnmrVar,$index)] == 1} {
      set num [llength $dgTemplate(vnmrVar,$index)]
      for {set i 0} {$i < $num} {incr i} {
        set name [lindex $dgTemplate(vnmrVar,$index) $i]
        set $name $dgVnmr($name,val)
      }
      if {[string length $dgTemplate(vnmrSet,$index)] > 0} {
        vnmrsend [subst $dgTemplate(vnmrSet,$index)]
      }
      set dgTemplate(update,$index) 1
   }
}

proc buttonCmd {index} {
   global dgTemplate
#   focus .dgmain
   update
   if {[string length $dgTemplate(vnmrSet,$index)] > 0} {
     eval vnmrsend [list [subst {$dgTemplate(vnmrSet,$index)}]]
   }
}

proc addbutton {index} {
   global dgTemplate

   button $dgTemplate(win).entry($index) -text "$dgTemplate(label,$index)" \
           -highlightthickness 0 \
           -command "buttonCmd $index" \
           -foreground $dgTemplate(color,$index)
   set tst [catch {expr 1+$dgTemplate(width,$index)}]
   if {($tst == 0) && ($dgTemplate(width,$index) >= 1)} {
      $dgTemplate(win).entry($index) configure -width $dgTemplate(width,$index)
   }
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
}

proc addlist {index} {
   global dgTemplate dispFile

   frame $dgTemplate(win).entry($index) -highlightthickness 0
   text $dgTemplate(win).entry($index).list \
           -relief flat \
           -yscrollcommand "$dgTemplate(win).entry($index).scroll set" \
           -font 9x15bold \
           -highlightthickness 0 \
           -padx 10 -pady 10 \
           -foreground $dgTemplate(color,$index)
   scrollbar $dgTemplate(win).entry($index).scroll \
           -command "$dgTemplate(win).entry($index).list yview"
   pack $dgTemplate(win).entry($index).scroll \
           -in $dgTemplate(win).entry($index) \
           -side right -fill y -expand 1 -anchor e -padx 5
   pack $dgTemplate(win).entry($index).list \
           -in $dgTemplate(win).entry($index) \
           -side left -fill both -expand 1
   set dispFile(winID) $dgTemplate(win).entry($index).list
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -rowspan $dgTemplate(rows,$index) \
          -columnspan $dgTemplate(cols,$index)
}

proc radioCmd {index} {
   global dgTemplate
#   focus .dgmain
   update
   set vCmd [subst $dgTemplate(vnmrSet,$index)]
   if {[regexp {\"([^"]*)\"} $vCmd match var] == 1} {
      set vCmd $var
   }
   vnmrsend "$vCmd"
}

proc addradio {index} {
   global dgTemplate

   frame $dgTemplate(win).entry($index)
   set num [llength $dgTemplate(label,$index)]
   for {set i 0} {$i < $num} {incr i} {
      radiobutton $dgTemplate(win).entry($index).$i \
           -text "[lindex $dgTemplate(label,$index) $i]" \
           -variable dgTemplate(value,$index) \
           -value "[lindex $dgTemplate(values,$index) $i]" \
           -command "radioCmd $index" \
           -highlightthickness 0 \
           -foreground $dgTemplate(color,$index)
      pack $dgTemplate(win).entry($index).$i -side top -anchor w \
           -expand 1 -in $dgTemplate(win).entry($index)
   }
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -rowspan $dgTemplate(rows,$index) \
          -padx 10 \
          -columnspan [expr $num * $dgTemplate(cols,$index)]
   updateVars $index
}

proc checkCmd {index} {
   global dgTemplate
#   focus .dgmain
   update
   if {$dgTemplate(value,$index) == 1} {
      vnmrsend [subst $dgTemplate(vnmrSet,$index)]
   } else {
      vnmrsend [subst $dgTemplate(vnmrSet2,$index)]
   }
}

proc addcheck {index} {
   global dgTemplate

   checkbutton $dgTemplate(win).entry($index) \
           -text "$dgTemplate(label,$index)" \
           -variable dgTemplate(value,$index) \
           -command "checkCmd $index" \
           -highlightthickness 0 \
           -foreground $dgTemplate(color,$index)
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   updateVars $index
}

proc addlabel {index} {
   global dgTemplate dgLocal

   frame $dgTemplate(win).entry($index)
   label $dgTemplate(win).entry($index).label1 -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   entry $dgTemplate(win).entry($index).entry -textvariable dgTemplate(value,$index) \
           -width $dgTemplate(width,$index) \
           -relief flat -state disabled \
           -selectbackground $dgLocal(bg) \
           -selectborderwidth 0 \
           -highlightthickness 0 \
           -foreground $dgTemplate(color,$index)
   pack $dgTemplate(win).entry($index).label1 -side left \
           -in $dgTemplate(win).entry($index)
   if {[string length $dgTemplate(units,$index)] >= 1} {
      label $dgTemplate(win).entry($index).label2 \
           -text "$dgTemplate(units,$index)" \
           -foreground $dgTemplate(color,$index)
      pack $dgTemplate(win).entry($index).label2 \
           $dgTemplate(win).entry($index).entry \
           -side right -in $dgTemplate(win).entry($index)
   } else {
      pack $dgTemplate(win).entry($index).entry \
           -side right -in $dgTemplate(win).entry($index)
   }
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   updateVars $index
}

proc scrollCmd {index val} {
   global dgTemplate

#   focus .dgmain
   update
   set dgTemplate(value,$index) $val
   if {[string length $dgTemplate(vnmrSet,$index)] > 0} {
     vnmrsend [subst $dgTemplate(vnmrSet,$index)]
   }
}

proc setscroll3 {index} {
  global dgTemplate

  set num [lsearch -exact $dgTemplate(choices,$index) $dgTemplate(value,$index)]
  setscroll  $dgTemplate(win).entry($index).scroll $num
}

proc addscroll {index} {
   global dgTemplate dgExec

   frame $dgTemplate(win).entry($index)
   label $dgTemplate(win).entry($index).label1 -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   if {$dgTemplate(width,$index) >= 1} {
     label $dgTemplate(win).entry($index).label2 \
           -textvariable dgTemplate(value,$index) \
           -width $dgTemplate(width,$index) \
           -foreground $dgTemplate(color,$index)
   }
   scroll2 $dgTemplate(win).entry($index).scroll  "scrollCmd $index" \
           $dgTemplate(value,$index) $dgTemplate(choices,$index)
   if {$dgTemplate(width,$index) >= 1} {
     pack $dgTemplate(win).entry($index).label1 \
         $dgTemplate(win).entry($index).label2 \
         -side left -in $dgTemplate(win).entry($index)
   } else {
     pack $dgTemplate(win).entry($index).label1 \
         -side left -in $dgTemplate(win).entry($index)
   }
   pack $dgTemplate(win).entry($index).scroll -side right -anchor e \
           -in $dgTemplate(win).entry($index)
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   set dgExec(eField,$index) $index
   set dgExec(eField2,$index) "setscroll3 $index"
   updateVars $index
}

proc checkEntry {index key mod} {
   global dgTemplate
   if {($mod == "") || ($key == "Tab")} {
      return
   }
   set dgTemplate(key,$index) 1
}

proc addentry {index} {
   global dgTemplate dgExec dgLocal

   frame $dgTemplate(win).entry($index)
   label $dgTemplate(win).entry($index).label1 -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   entry $dgTemplate(win).entry($index).entry -textvariable dgTemplate(value,$index) \
           -width $dgTemplate(width,$index) \
           -highlightthickness 0 \
           -foreground $dgTemplate(color,$index)
   pack $dgTemplate(win).entry($index).label1 -side left \
           -in $dgTemplate(win).entry($index)
   if {[string length $dgTemplate(units,$index)] >= 1} {
      label $dgTemplate(win).entry($index).label2 \
           -text "$dgTemplate(units,$index)" \
           -foreground $dgTemplate(color,$index)
      pack $dgTemplate(win).entry($index).label2 \
           $dgTemplate(win).entry($index).entry \
           -side right -in $dgTemplate(win).entry($index)
   } else {
      pack $dgTemplate(win).entry($index).entry \
           -side right -in $dgTemplate(win).entry($index)
   }
   set dgExec(eField,$index) $dgTemplate(win).entry($index).entry
   if {[info exists dgTemplate(showVal,$index)] == 1} {
     if {[string length $dgTemplate(showVal,$index)] >= 1} {
        set dgExec(eField0,$index) "configure -background $dgLocal(off)"
        set dgExec(eField1,$index) "configure -background $dgLocal(bg)"
     }
   }
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   set dgTemplate(key,$index) 0
   bind $dgTemplate(win).entry($index).entry <KeyPress> "checkEntry $index %K %A"
   bind $dgTemplate(win).entry($index).entry <Return> "sendVarToVnmr $index 1"
   bind $dgTemplate(win).entry($index).entry <FocusOut> "sendVarToVnmr $index 0"
   bind $dgTemplate(win).entry($index).entry <FocusIn> "turnOffUpdate $index"
   updateVars $index
}

proc setmenu2 {index cindex} {
   global dgTemplate

#   focus .dgmain
   update
   set dgTemplate(value,$index) [lindex $dgTemplate(values,$index) $cindex]
   set vCmd [subst $dgTemplate(vnmrSet,$index)]
   if {[regexp {\"([^"]*)\"} $vCmd match var] == 1} {
      set vCmd $var
   }
   vnmrsend "$vCmd"
}

proc setmenu3 {name el op} {
  global dgTemplate

  scan $el "value,%s" index
  set num [lsearch -exact $dgTemplate(values,$index) $dgTemplate(value,$index)]
  if {$num == -1} {
     $dgTemplate(win).entry($index).menu2 configure \
        -text ""
  } else {
     $dgTemplate(win).entry($index).menu2 configure \
        -text "[lindex $dgTemplate(choices,$index) $num]"
  }
}

proc addmenu2 {index} {
   global dgTemplate

   frame $dgTemplate(win).entry($index)
   label $dgTemplate(win).entry($index).label -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   menu2 $dgTemplate(win).entry($index).menu2 $dgTemplate(choices,$index) \
          setmenu2 $index \
          -width 10 -anchor w -foreground $dgTemplate(color,$index)
   pack $dgTemplate(win).entry($index).label -side left \
          -in $dgTemplate(win).entry($index)
   pack $dgTemplate(win).entry($index).menu2 -side right \
          -in $dgTemplate(win).entry($index)
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   trace variable dgTemplate(value,$index) w setmenu3
   updateVars $index
}

proc addtitle {index} {
   global dgTemplate

   label $dgTemplate(win).entry($index) -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
}

proc scaleCmd {index} {
   global dgTemplate

#   focus .dgmain
   update
   vnmrsend [subst $dgTemplate(vnmrSet,$index)]
}

proc addscale {index} {
   global dgTemplate dgExec

   frame $dgTemplate(win).entry($index)
   label $dgTemplate(win).entry($index).label1 -text "$dgTemplate(label,$index)" \
           -foreground $dgTemplate(color,$index)
   if {$dgTemplate(width,$index) >= 1} {
     label $dgTemplate(win).entry($index).label2 \
           -textvariable dgTemplate(value,$index) \
           -width $dgTemplate(width,$index) \
           -foreground $dgTemplate(color,$index)
   }
   scale $dgTemplate(win).entry($index).scale -label "" -orient horizontal \
           -showvalue 0 \
           -highlightthickness 0 \
           -foreground $dgTemplate(color,$index) \
           -variable dgTemplate(value,$index) -resolution 1 -width 10
   if {$dgTemplate(scale,$index) != ""} {
     set dgExec(sField,$index) $dgTemplate(win).entry($index).scale
   }
   if {$dgTemplate(width,$index) >= 1} {
     pack $dgTemplate(win).entry($index).label1 \
         $dgTemplate(win).entry($index).label2 \
         -side left -in $dgTemplate(win).entry($index)
   } else {
     pack $dgTemplate(win).entry($index).label1 \
         -side left -in $dgTemplate(win).entry($index)
   }
   pack $dgTemplate(win).entry($index).scale -side right -expand 1 \
          -anchor e -in $dgTemplate(win).entry($index)
   scan $index "%d,%d" row col
   grid $dgTemplate(win).entry($index) -row $row -column $col \
          -sticky $dgTemplate(sticky,$index) \
          -padx 10 \
          -rowspan $dgTemplate(rows,$index) -columnspan $dgTemplate(cols,$index)
   bind $dgTemplate(win).entry($index).scale <ButtonRelease-1> "scaleCmd $index"
   updateVars $index
}

proc addelem {index} {
   global dgTemplate

   switch $dgTemplate(type,$index) {
      title   { addtitle $index }
      label   { addlabel $index }
      button  { addbutton $index }
      entry   { addentry $index }
      list    { addlist $index }
      menu2   { addmenu2 $index }
      scale   { addscale $index }
      check   { addcheck $index }
      radio   { addradio $index }
      scroll  { addscroll $index }
      ""      { }
      default {
                 puts "unknown element type $elemType($index)"
              }
   } 
}

proc readConfFile {file subst} {
   global env dgLocal dgTemplate seqfil
   set default default
   catch {source [file join $env(vnmrsystem) user_templates dg $seqfil DEFAULT]}
   catch {source [file join $env(vnmruser) templates dg $seqfil DEFAULT]}
   if {[file exists [file join $env(vnmrsystem) tmp $file]] == 1} {
      source [file join $env(vnmrsystem) tmp $file]
   } elseif {[file exists \
             [file join $env(vnmruser) templates dg $seqfil $file]] == 1} {
      source [file join $env(vnmruser) templates dg $seqfil $file]
   } elseif {[file exists  \
             [file join $env(vnmruser) templates dg $default $file]] == 1} {
      source [file join $env(vnmruser) templates dg $default $file]
   } elseif {[file exists  \
             [file join $env(vnmruser) templates dg default $file]] == 1} {
      source [file join $env(vnmruser) templates dg default $file]
   } elseif {[file exists  \
            [file join $env(vnmrsystem) user_templates dg $seqfil $file]] == 1} {
      source [file join $env(vnmrsystem) user_templates dg $seqfil $file]
   } elseif {[file exists  \
            [file join $env(vnmrsystem) user_templates dg $default $file]] == 1} {
      source [file join $env(vnmrsystem) user_templates dg $default $file]
   } elseif {[file exists  \
            [file join $env(vnmrsystem) user_templates dg default $file]] == 1} {
      source [file join $env(vnmrsystem) user_templates dg default $file]
   }
   if {$subst == 0} {
      catch {source [file join $env(vnmruser) templates dg geometry]}
      return
   }
   set testElements {vnmrSet vnmrSet2 tclSet}
   for {set j 0} {$j < $dgTemplate(cols)} {incr j} {
      for {set i 0} {$i < $dgTemplate(rows)} {incr i} {
         foreach el $testElements {
            if {[info exists dgTemplate($el,$i,$j)] == 1} then {
               regsub -all VALUE [set dgTemplate($el,$i,$j)] \
                        dgTemplate(value,$i,$j) dgTemplate($el,$i,$j)
            }
         }
      }
   }
}

proc dgExit {} {
   global env
   if {[file isdirectory [file join $env(vnmruser) templates dg]] == 1} {
      set fd [open [file join $env(vnmruser) templates dg geometry] w]
      puts $fd "wm geometry . [wm geometry .]"
      close $fd
      catch {exec chmod 666 [file join $env(vnmruser) templates dg geometry]}
   }
   exit
}

set dgLocal(currentWindow) -1
set dgLocal(forgetWindow) -1
set dgLocal(index) -1
set dgLocal(conf) 0

proc dgstart {win parent fileindex} {
   global dgTemplate dgLocal dgExec dgVnmr
   global env

   if {($dgLocal(conf) == 0)} {
      if {$dgLocal(output,$fileindex) == 1} {
         vnmrsend "textis('reset','tcl') flip('text')"
      } else {
         vnmrsend "textis('set','tcl') flip('text')"
      }
   }
   if {$dgLocal(currentWindow) == $win} then {
      return 1
   }
   if {($dgLocal(conf) == 1) && ($dgLocal(index) != $fileindex)} {
      endDgConf 0
   }
#   focus .dgmain
   catch {unset dgTemplate}
   catch {unset dgExec}
   set dgExec(vnmrVar) ""
   set dgExec(index) ""
   set dgTemplate(cols) 1
   set dgTemplate(rows) 1
   if {$dgLocal(index) != -1} {
      foreach el [winfo children .dgmain.dg($dgLocal(index))] {
            destroy $el
      }
   }
   readConfFile $dgLocal(file,$fileindex) 1
   if {($dgLocal(conf) == 1)} {
      set dgLocal(file,$fileindex) $dgLocal(savefile)
   }
   set dgTemplate(win) $win
   set dgTemplate(vnmrVar,unit) {reffrq reffrq1 reffrq2}
   updateVars unit
   set dgTemplate(update,unit) 0
   for {set j 0} {$j < $dgTemplate(cols)} {incr j} {
      for {set i 0} {$i < $dgTemplate(rows)} {incr i} {
         if {[info exists dgTemplate(type,$i,$j)] == 1} then {
            addelem $i,$j
         }
      }
   }
   if {[llength $dgExec(vnmrVar)] > 0} {
      catch {unset dgVnmr}
      incr dgLocal(curvar)
      set dgLocal(getvar) 0
      magicvars dgVnmr dgExec(vnmrVar) \
                getVarFromVnmr "restartGetVar $dgLocal(curvar); getVarFromVnmr"
   }
   if {$dgLocal(currentWindow) != -1} then {
      pack forget $dgLocal(forgetWindow)
   }
   pack $win -side top -anchor nw -expand 1 -fill both -in $parent
   set dgLocal(currentWindow) $win
   set dgLocal(forgetWindow) $win
   if {$dgLocal(index) != -1} then {
      bind .dg($dgLocal(index)) <Control-Button-1> {}
   }
   set dgLocal(index) $fileindex
   if {$dgLocal(edit) == "yes"} {
      if {$dgLocal(output,$fileindex) == 1} {
         bind .dg($dgLocal(index)) <Control-Button-1> {
             vnmrsend "write('error','cannot reconfigure the text panel')"
         }
      } else {
         bind .dg($dgLocal(index)) <Control-Button-1> {startDgConf}
      }
   }
   set dgLocal(text) $dgLocal(output,$fileindex)
   if {$dgLocal(output,$fileindex) == 1} {
      global dispFile
      $dispFile(winID) mark set seeEnd end
      set dispFile(mtime) 0
   }
   resizeDg
}

proc startDgConf {} {
   global env dgLocal
   set dgLocal(conf) 1
   source [file join $env(vnmrsystem) tcl bin dgconf]
   startConf
}

proc resetDgConf {file} {
   global dgLocal dgExec
   set dgLocal(currentWindow) ""
   set i $dgLocal(index)
   set dgLocal(savefile) $dgLocal(file,$i)
   set dgLocal(file,$i) $file
   .dg($i) configure -state normal
   .dg($i) invoke
}

proc endDgConf {restart} {
   global dgLocal
   endConf
   set dgLocal(conf) 0
   rename endConf {}
   set i $dgLocal(index)
   if {$restart == 1} {
       set dgLocal(currentWindow) ""
      .dg($i) configure -state normal
      .dg($i) invoke
   }
}

proc updateDisp {} {
   global dispFile dgTemplate dgLocal

   if [catch {file stat $dispFile(name) fStat} res] {
      after 500 updateDisp
      return
   }
   set mtime $fStat(mtime)
   set size $fStat(size)
   if {($mtime != $dispFile(mtime)) || ($size != $dispFile(size)) } {
      set dispFile(mtime) $mtime
      set dispFile(size) $size
      if {$dgLocal(text) == 1} {
        $dispFile(winID) configure -state normal
        $dispFile(winID) delete 1.0 end
        set f [open $dispFile(name) r]
        while { [gets $f line] >= 0} {
           [set dispFile(winID)] insert end "$line\n"
        }
        close $f
        $dispFile(winID) configure -state disabled
        $dispFile(winID) see seeEnd
      } elseif {($size > 0) && ($dispFile(autoUpdate) == 1)} {
        if {[info exists dispFile(winID)] == 0} {
          .dg($dgLocal(textout)) invoke
        }
        if {[winfo exists $dispFile(winID)] == 0} {
          .dg($dgLocal(textout)) invoke
        }
        $dispFile(winID) mark set seeEnd end
      }
   }
   after 500 updateDisp
}

proc stopUpdate {} {
   global dispFile
   set dispFile(autoUpdate) 0
}

proc startUpdate {} {
   global dispFile dgLocal
   set dispFile(autoUpdate) 1
   if {$dgLocal(text) != 1} {
      if [catch {file stat $dispFile(name) fStat} res] {
         return
      }
      set dispFile(mtime) $fStat(mtime)
      set dispFile(size) $fStat(size)
   }
}

proc startInfo {} {
   global dispFile

   set dispFile(mtime) 0
   set dispFile(size) -1
   set dispFile(autoUpdate) 1
   updateDisp
}

proc curexpTrace {var el op} {
   global dgLocal dgExec
   set dgLocal(currentWindow) ""
   .dg(1) configure -state normal
   .dg(1) invoke
}

proc invokeTab {label} {
   global dgLocal
   for {set i 1} {$i < $dgLocal(numTabs)} {incr i} {
      if {$label ==  $dgLocal(title,$i)} {
         set dgLocal(currentWindow) ""
         .dg($i) configure -state normal
         .dg($i) invoke
         return
      }
   }
}

set dgLocal(side) "horiz"
set dgLocal(textout) 1
set dgLocal(text) 0
readConfFile dg.conf 0
frame .dgmain -highlightthickness 0
frame .dgmain.menu
set i 1
if {$dgLocal(side) == "horiz"} {
   set deckSide left
} else {
   set deckSide top
}
while {[info exists dgLocal(title,$i)] == 1} {
   deck .dg($i) 0 -text $dgLocal(title,$i) -width 10 \
        -command "[list selectdeck \
          .dg($i) "dgstart .dgmain.dg($i) .dgmain $i"];\
         .dg($i) configure -state normal"
   pack .dg($i) -side $deckSide -in .dgmain.menu
   canvas .dgmain.dg($i) -highlightthickness 0
   if {$dgLocal(output,$i) == 1} {
      set dgLocal(textout) $i
   }
   incr i
}
set dgLocal(numTabs) $i
if {$i > 2} {
   label .dgframe -relief sunken -borderwidth 3 -width 10
   if {$dgLocal(side) == "horiz"} {
      pack .dgframe  -side left -expand 1 -fill both -in .dgmain.menu
      pack .dgmain.menu -side top -anchor nw -expand 1 -fill x -in .dgmain
   } else {
      pack .dgframe  -side top -expand 1 -fill both -in .dgmain.menu
      pack .dgmain.menu -side right -anchor ne -expand 1 -fill y -in .dgmain
   }
   .dgmain configure -relief raised -borderwidth 3
} else {
    set dgLocal(side) "none"
   .dgmain configure -relief flat -borderwidth 3
}
set dgLocal(bg) [.dgmain.menu cget -background]
vnmrinit \"[lindex $argv 0]\" $env(vnmrsystem)
pack .dgmain -fill both -expand 1
startInfo
trace variable curexp w curexpTrace
dgServer
