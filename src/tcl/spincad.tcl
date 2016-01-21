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

proc exitError {} {
   global pInfo
   if {[winfo exists .psgError] == 1} {
      regexp {[^+]*(.*)} [wm geometry .psgError] match val
      set pInfo(errorGeom) $val
      destroy .psgError
   }
}

proc loadErrors {file} {
   global errorIds psgInfo
   set fd [open $file r]
   set num 0
   set f .psgError.fr1
   while {[gets $fd line] >= 0} {
      set ret [scan $line \
      {%s occurred at line %d column %d for PSG Element %s Id %s in file %s}\
        errWarn lineNo colNo elem id fileName]
      if {$ret == 6} {
         set id [string trim $id \"]
      } else {
         set id NOT
      }
      gets $fd line
      incr num
      if {[info exists errorIds($id)] == 1} {
         set id $psgInfo($errorIds($id),id)
         set coords [.draw coords T$id]
         selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1] 
         button $f.butt$num -text "$line" -anchor w \
           -command "selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1]"
         pack $f.butt$num -side top -fill x
      } else {
         label $f.butt$num -text "$line" -anchor w
         pack $f.butt$num -side top -fill x
      }
   }
   close $fd
}

proc makeErrorWin {} {
   global psgInfo pInfo compInfo
   toplevel .psgError
   set w .psgError
   wm title $w "SpinCAD Error Manager"
   wm minsize $w 200 100
   if {[info exists pInfo(errorGeom)] == 1} {
      wm geometry .psgError $pInfo(errorGeom)
   }
   frame $w.fr1 
   frame $w.fr2 
   set w1 $w.fr1
   set w2 $w.fr2
   pack $w1 -side top -fill both -expand true
   button $w2.collCancel -text Cancel -command exitError
   pack $w2.collCancel -side left -padx 5 -pady 5
   pack $w2 -side top -fill x
   wm protocol .psgError WM_DELETE_WINDOW {
      exitError
   }
}

proc showError {file} {
   if {[winfo exists .psgError] == 1} {
      exitError
   }
   makeErrorWin
   loadErrors $file
}


proc setError {id quit msg} {
   global errorFd seqName psgInfo env errorIds
   if {$errorFd == -1} {
      set errorFd [open [file join $env(vnmruser) seqlib $seqName.error] w]
   }
   set id $psgInfo($errorIds($id),id)
   puts $errorFd "Error occurred at line 0 column 0 for PSG Element \"$psgInfo($id,name)\" Id \"$psgInfo($id,errorId)\" in file \"$seqName\""
   puts $errorFd $msg
   if {$quit != 0} {
      exit $quit
   }
}

proc zoom2 {scale} {
 global psgInfo dockList numChan pInfo
 . configure -cursor watch
 set fract [expr $psgInfo(scale)/double($scale)]
 set psgInfo(fract) $fract
 set width [expr $dockList(maxx) / $fract]
 set height [expr ($psgInfo(ybot) + 60)/ $fract]
# puts "dimensions $width x $height"
 .draw configure -scrollregion "0 0 $width $height"
 set sc [expr double($scale) / $psgInfo(scale)]
 .draw scale all 0 0 $sc $sc
 set psgInfo(ybot) [varySize $psgInfo(ybot) $fract]
 set psgInfo(ytop) [varySize $psgInfo(ytop) $fract]
 set psgInfo(ydelta) [varySize $psgInfo(ydelta) $fract]
 set dockList(maxx) [varySize $dockList(maxx) $fract]
 for {set i 0} {$i < [llength $psgInfo(elems)]} {incr i} {
  set id [lindex $psgInfo(elems) $i]
  set psgInfo($id,x) [varySize $psgInfo($id,x) $fract]
  set psgInfo($id,y) [varySize $psgInfo($id,y) $fract]
  set psgInfo($id,ht) [varySize $psgInfo($id,ht) $fract]
  set psgInfo($id,len) [varySize $psgInfo($id,len) $fract]
 }
 for {set i 1} {$i <= $numChan} {incr i} {
  set dockList(ch$i) [varySize $dockList(ch$i) $fract]
 }
 if {$fract < 1.0} {
  set id $psgInfo(cur)
#  puts "Selected item is $id, $psgInfo($id,elemLabel)"
  if {$id != 0} {
   set x [expr $psgInfo($id,x) / $width]
   set y [expr $psgInfo($id,y) / $height]
  } else {
   set x 0.5
   set y 0.5
  }
  set x1 [expr $x - ($fract / 2.0)]
  if {$x1 < 0.0} {set x1 0}
  set y1 [expr $y - ($fract / 2.0)]
  if {$y1 < 0.0} {set y1 0}
  .draw xview moveto $x1
  .draw yview moveto $y1
 }
 set psgInfo(scale) $scale
# set pts [expr int( $scale / 10)]
# option add *Balloon*font "courb$pts"
 . configure -cursor $pInfo(cursor)
}

proc zoom {scale} {
 global psgInfo dockList numChan pInfo
 . configure -cursor watch
 set fract [expr $psgInfo(scale)/double($scale)]
 set psgInfo(fract) $fract
 set width [expr $dockList(maxx) / $fract]
 set height [expr ($psgInfo(ybot) + 60)]
# puts "dimensions $width x $height"
 .draw configure -scrollregion "0 0 $width $height"
 set sc [expr double($scale) / $psgInfo(scale)]
 .draw scale scl 0 0 $sc 1
 set dockList(maxx) [varySize $dockList(maxx) $fract]
 foreach id $psgInfo(drawList) {
  set psgInfo($id,x) [varySize $psgInfo($id,x) $fract]
  set psgInfo($id,len) [varySize $psgInfo($id,len) $fract]
 }
 if {$fract < 1.0} {
  set id $psgInfo(cur)
#  puts "Selected item is $id, $psgInfo($id,elemLabel)"
  if {($id > 0) && ([info exists psgInfo($id,x)] == 1)} {
   set x [expr $psgInfo($id,x) / $width]
  } else {
   set x 0.5
  }
  set x1 [expr $x - ($fract / 2.0)]
  if {$x1 < 0.0} {set x1 0}
   scrollDraw moveto $x1
 } else {
   scrollDraw moveto 0
 }
 setItemX
 setDockLists
 foreach i $psgInfo(drawList) {
   redrawItem $i
 }
 showDockList
 if {$pInfo(labels) == "on"} {
    if {[.mbar.labels.menu entrycget 1 -label] == "Update"} {
       destroyCanvasLabels
    }
    makeCanvasLabels
 }
 set psgInfo(scale) $scale
 . configure -cursor $pInfo(cursor)
}

proc scrollDraw {cmd opt1 {opt1 -1}} {
   global pInfo dockList
   if {$cmd == "moveto"} {
      .draw xview moveto $opt1
   } else {
      .draw xview scroll $opt1 $opt2
   }
   set fract [.draw xview]
   set newX [expr $dockList(maxx) * \
          (3.0 * double([lindex $fract 1]) + double([lindex $fract 0])) / 4.0]
   .draw move trash [expr $newX - $pInfo(trashX)] 0
   set pInfo(trashX) [lindex [.draw coords trash] 0]
}

proc scrolls {} {
#  .draw configure -xscrollcommand ".hscr set" \
#    -yscrollcommand ".vscr set"
  .draw configure -xscrollcommand ".hscr set"
  scrollbar .hscr -command "scrollDraw" -orient horizontal
#  scrollbar .vscr -command ".draw yview" -orient vertical
#  pack .vscr -side right -fill y -in .psg
  pack .hscr -side bottom -fill x -in .psg
}

proc varySize {x y} {
 set a [expr $x / $y]
 return $a
}

proc newChanType {index type} {
   global psgInfo
   updateChanInfo Chan$index
   makeWindows Chan$index
   bind .f1.ent <Return> "relabel $index"
   bind .f1.ent <FocusOut> "relabel $index"
   set i 0
   foreach lab $psgInfo(Chan$index,L2,vals) {
      .f2.menu.list entryconfigure $i -command "newChanType $index $lab"
      incr i
   }
   updateMeta
}

proc selectChan {index} {
   global dElem psgInfo pInfo

   if {$pInfo(composite) == 1} {
      return
   }
   if {$index == $psgInfo(curChan)} {
      return
   }
   updateChanInfo Chan$index
   makeWindows Chan$index
   if {$psgInfo(curChan) != 0} {
      .draw bind ChanLabel$psgInfo(curChan) <Leave> \
         {.draw itemconfigure current -fill black}
      .draw itemconfigure ChanLabel$psgInfo(curChan) -fill black
   }
   set id $psgInfo(cur)
   if {($id > 0) && ($psgInfo($id,idAt) != "start") && \
       ($psgInfo($id,idAt) != "screen")} {
      .draw itemconfigure T$psgInfo($id,idAt) -outline $pInfo(elemColor)
   }
   .draw itemconfigure T$id -fill $psgInfo(bg) -outline $pInfo(elemColor)
   .comp1 configure -text $psgInfo(Chan$index,name)
   .draw itemconfigure ChanLabel$index -text "$psgInfo(Chan$index,V1)"
   .draw bind ChanLabel$index <Leave> {}
   bind .f1.ent <Return> "relabel $index"
   bind .f1.ent <FocusOut> "relabel $index"
   set i 0
   foreach lab $psgInfo(Chan$index,L2,vals) {
      .f2.menu.list entryconfigure $i -command "newChanType $index $lab"
      incr i
   }
#   bind .f1.ent <ButtonRelease-1> "setListSelection channel"
   set psgInfo(curChan) $index
   unsetDocker
   bind .draw <B1-Motion> {}
   bind .draw <ButtonRelease-1> {}
   updateMeta
}

proc updateSeqInfo {} {
   global psgInfo scaled pInfo
#   foreach nm [array names psgInfo seq,*] {
#      puts "$nm: $psgInfo($nm)"
#   }
#   set psgInfo(scale) $psgInfo(seq,scale)
   set scaled $psgInfo(seq,scale)
   set psgInfo(seq,V1) $psgInfo(seq,comment)
#   if {$psgInfo(seq,version) != $pInfo(sCVersion)} {
#    puts "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n ***** Wrong version! *****"
#    puts "  Sequence is $psgInfo(seq,version), SpinCAD is $pInfo(sCVersion)"
#   }
}

proc updateChanInfo {index} {
   global psgInfo
   if {$psgInfo($index,V2) == "RF"} {
      if {$psgInfo($index,numAttr) < 4} {
      set psgInfo($index,numAttr) 4
      set psgInfo($index,numWins) 4
      set psgInfo($index,N3) "cyclops"
      set psgInfo($index,B3) ""
      set psgInfo($index,L3) "Acq. Quadrature: "
      set psgInfo($index,L3,type) "MENU"
      set psgInfo($index,L3,vals) "Cyclops None"
      if {[info exists psgInfo($index,cyclops)] == 1} {
         set psgInfo($index,V3) $psgInfo($index,cyclops)
      } else {
         set psgInfo($index,V3) "None"
      }
      set psgInfo($index,S3) inherit
#      set psgInfo($index,N4) "offset"
#      set psgInfo($index,B4) ""
#      set psgInfo($index,L4) "Offset (ppm): "
#      set psgInfo($index,L4,type) "entry"
#      set psgInfo($index,V4) "0.0"
#      set psgInfo($index,S4) inherit
#      set psgInfo($index,N5) "limit"
#      set psgInfo($index,B5) ""
#      set psgInfo($index,L5) "Safety limit: "
#      set psgInfo($index,L5,type) "entry"
#      set psgInfo($index,V5) "MAX"
#      set psgInfo($index,S5) inherit
      set psgInfo($index,N4) "limit"
      set psgInfo($index,B4) ""
      set psgInfo($index,L4) "Safety limit: "
      set psgInfo($index,L4,type) "entry"
      if {[info exists psgInfo($index,limit)] == 1} {
         set psgInfo($index,V4) $psgInfo($index,limit)
      } else {
         set psgInfo($index,V4) "MAX"
      }
      set psgInfo($index,S4) inherit
      }
   } elseif {$psgInfo($index,V2) == "Gradient"} {
      set psgInfo($index,numAttr) 2
      set psgInfo($index,numWins) 2
   } elseif {$psgInfo($index,V2) == "Generic"} {
      set psgInfo($index,numAttr) 2
      set psgInfo($index,numWins) 2
   }
}

proc makeChanInfo {max} {
   global psgInfo seqName
   set psgInfo(seq,name) "Pulse Sequence"
   set psgInfo(seq,numAttr) 1
   set psgInfo(seq,numWins) 1
   set psgInfo(seq,labelId) 0

   set psgInfo(seq,N1) "comment"
   set psgInfo(seq,B1) ""
   set psgInfo(seq,L1) "Comment: "
   set psgInfo(seq,L1,type) "entry"
   set psgInfo(seq,V1) ""
   set psgInfo(seq,S1) inherit

   for {set i 1} {$i <= $max} {incr i} {
      set psgInfo(Chan$i,name) "Channel $i"
      set psgInfo(Chan$i,numAttr) 2
      set psgInfo(Chan$i,numWins) 2
      set psgInfo(Chan$i,labelId) 0

         set psgInfo(Chan$i,N1) "label"
         set psgInfo(Chan$i,B1) ""
         set psgInfo(Chan$i,L1) "Channel Label: "
         set psgInfo(Chan$i,L1,type) "entry"
         set psgInfo(Chan$i,V1) "Channel $i"
         set psgInfo(Chan$i,S1) inherit
         set psgInfo(Chan$i,N2) "type"
         set psgInfo(Chan$i,B2) ""
         set psgInfo(Chan$i,L2) "Channel Type: "
         set psgInfo(Chan$i,L2,type) "MENU"
         set psgInfo(Chan$i,L2,vals) "RF Gradient Generic"
         set psgInfo(Chan$i,V2) "RF"
         set psgInfo(Chan$i,S2) inherit
#         set psgInfo(Chan$i,N3) "offset"
#         set psgInfo(Chan$i,B3) ""
#         set psgInfo(Chan$i,L3) "Offset (ppm): "
#         set psgInfo(Chan$i,L3,type) "entry"
#         set psgInfo(Chan$i,V3) "0.0"
#         set psgInfo(Chan$i,S3) inherit
#         set psgInfo(Chan$i,N4) "limit"
#         set psgInfo(Chan$i,B4) ""
#         set psgInfo(Chan$i,L4) "Safety limit: "
#         set psgInfo(Chan$i,L4,type) "entry"
#         set psgInfo(Chan$i,V4) "MAX"
#         set psgInfo(Chan$i,S4) inherit
   }
}

proc setChans {redo} {
   global numChan dockWin dockList psgInfo pInfo trashcan
   set ht [winfo height .draw]
   set wd [winfo width .draw]
   set tmpScale $psgInfo(scale)
   if {$tmpScale != 100} {zoom 100}
   set dockList(maxx) [expr [winfo width .draw] -4]
   set psgInfo(numChan) $numChan
   set errGap 60
   set y 2
   set delY [expr ($ht-$y-$errGap)/$numChan]
   set psgInfo(ytop) $y
   set psgInfo(ydelta) $delY
   .draw create text 10 $y -text "test" -tags {channel scl}
   set pInfo(font) [.draw itemcget channel -font]
   set linespace [font metrics $pInfo(font) -linespace]
   set pInfo(linespace) $linespace
   set linespace [expr $linespace + 4]
# puts "set top= $psgInfo(ytop)"
   .draw delete channel
#   .draw create line 0 $y $wd $y -width 2 -tags channel 
#   .draw create line 0 [expr $ht-$errGap] $wd [expr $ht-$errGap] \
         -width 2 -tags channel 
   for {set i 1} {$i <= $numChan} {incr i} {
      set id [.draw create text \
               10 [expr $y + $linespace] -text $psgInfo(Chan$i,V1) \
               -tags "channel label ChanLabel$i scl" -anchor sw]
      set y [expr $y + $delY]
      set dockList(ch$i) [expr $y - $delY/3.0]
#      .draw create line 0 $y $wd $y -width 1 -tags "channel ch$i"
#      if {$i < $numChan} {
#         .draw scale ch$i 0 $y .2 1
#      }
      .draw create line 0 $dockList(ch$i) $wd $dockList(ch$i) \
            -tags "channel channelLine scl" -fill $pInfo(chanColor)
     .draw bind ChanLabel$i <Button-1> "selectChan $i"
     .draw bind ChanLabel$i <Enter> {.draw itemconfigure current -fill red}
     .draw bind ChanLabel$i <Leave> {.draw itemconfigure current -fill black}
   }
   .draw create line 0 $y $wd $y -width 2 -tags "channel scl"
   set psgInfo(ybot) $y
   set psgInfo(yPhase) [expr $psgInfo(ybot) + 30]
   set psgInfo(xPhase) 100
   set psgInfo(xPhaseCur) 100
   .draw lower channel
   .draw delete deleter
   .draw create text \
               10 [expr $y + $errGap - 10] -text "Phase Tables" \
               -tags "deleter" -anchor sw
   set pInfo(trashX) [expr 3 * [winfo width .draw] / 4]
   .draw create bitmap $pInfo(trashX) [expr $y + 8] \
      -bitmap $trashcan -anchor nw -foreground red4 \
      -background "" -tag {deleter factoryItem trash}
# puts "set bot= $psgInfo(ybot)"
   if {$tmpScale != 100} {zoom $tmpScale}
   if {$redo == 1} {
      resetDisplay
      if {$pInfo(labels) == "on"} {
         destroyCanvasLabels
         makeCanvasLabels
      }
   }
}

proc makePsgFrame {win} {
   global seqName
   frame $win.psg
   button .psg1 -text "Pulse Sequence: " -command selectPsgSeq
   entry .psg2 -width 20 -relief sunken -textvariable seqName -font {courier 12 bold}
   bind .psg2 <FocusOut> "updateExitMenu"
   label .psg3 -text "Number of Channels: "
   set id [tk_optionMenu .psg4 numChan 1 2 3 4 5 6 7 8]
   bind $id <ButtonRelease-1> {+after 1 "setChans 1"}
   grid .psg1 -in $win.psg -row 0 -column 0 -sticky nw
   grid .psg2 -in $win.psg -row 0 -column 1 -sticky nw
   grid .psg3 -in $win.psg -row 1 -column 0 -sticky nw
   grid .psg4 -in $win.psg -row 1 -column 1 -sticky nw
   pack $win.psg -side top
}

proc makeInfoFrame {win} {
   frame $win.comp -relief flat -borderwidth 2
   label .comp1 -text "Pulse Elements"
   text .comp2 -relief raised -borderwidth 2 -state disabled\
         -yscrollcommand ".scroll2 set" -width 40 -height 5
   scrollbar .scroll2 -command ".comp2 yview"
   pack .comp1 -side top -in $win.comp
   pack .scroll2 -side right -anchor w -in $win.comp -fill y -expand 1
   pack .comp2 -in $win.comp -side right -anchor e -expand 1 -fill both
   pack $win.comp -side top -anchor w -fill both -expand 1
#   pack $win.comp -side top -anchor w -fill both -expand 1
}

proc getElemIndex {name} {
   global pElem
   for {set i 0} {$i < $pElem(numGroups)} {incr i} {
      for {set j 0} {$j < $pElem(numElems,$i)} {incr j} {
#puts "test $name == $pElem(elemName,$i,$j)"
         if {$name == $pElem(elemName,$i,$j)} {
            return $i,$j
         }
      }
   }
   set pElem(errname) $name
   return 0
}

proc makeDefaultElem {} {
   global pElem loadInfo
   set index default,$pElem(nextDefault)
   incr pElem(nextDefault)
   set pElem(loadId) $index
   set pElem(name,$index) $index
   set pElem(elemName,$index) $pElem(errname)
   set pElem(type,$index) ERROR
   set pElem(phTablesInherit,$index) {}
   set pElem(phTablesFixed,$index) {}
   set pElem(elemLabel,$index) $loadInfo(el)
   set pElem(label,$index) "unknown"
   set pElem(comps,$index) 0
   set pElem(numAttr,$index) 0
   set pElem(numWins,$index) 0
   set pElem(elemLoaded,$index) 1
}

proc loadElem {index} {
   global pElem psgLabel pInfo
   
# puts "loadElem $index"
   if {[info exists pElem(elemLoaded,$index)] == 0} {
      makeDefaultElem
puts "loadElem default element $index"
      return
   }
   if {$pElem(elemLoaded,$index) == 1} {
      set pElem(loadId) $index
# puts "loadElem $index already loaded"
      return
   }
   set fd [open [file join \
     $pElem(groupPath,$pElem(elemGroup,$index)) $pElem(elemName,$index)] r]
# puts "loadpath $pElem(groupPath,$pElem(elemGroup,$index))/$pElem(elemName,$index)"
   set indexSave $index
   set pElem(loadId) $index
   set pElem(type,$index) "primitive"
   set pElem(name,$index) $index
   set pElem(label,$index) $pElem(elemLabel,$index)
   set pElem(comps,$index) 0
   set pElem(numAttr,$index) 0
   set pElem(numWins,$index) 0
   set pElem(phTablesFixed,$index) {}
   set pElem(phTablesInherit,$index) {}
   set pElem(phTablesPrimitive,$index) {}
   set composite 0
   set getAttr 0
   set stack {}
   while {[gets $fd line] >= 0} {
#  puts "got line '$line'"
     if {[regexp {^#} $line] == 1} {
# puts  "A comment"
       continue
     }
     if {[regexp "^\[ \t]*$" $line] == 1} {
# puts  "A blank line"
       continue
     }
     if {($getAttr == 1) && \
            ([regexp {[^<]*<([a-zA-Z0-9_]+)>} $line match attr ]  == 1)} {
        lappend stack $attr
        set value ""
        while {[gets $fd line] >= 0} {
# puts "new attr line is $line (cmp to </$attr>)"
           if {"</$attr>" == [string trimleft $line]} {
# puts "</$attr> == $line"
              set stack [lrange $stack 0 [expr [llength $stack]-2]]
              set value [string trimleft $value]
              break
           } else {
# puts "</$attr> [string length </$attr>] != $line [string length $line] "
             set value "$value\n$line"
           }
        }
#  puts "check attribute $attr with value '$value'"
        if {[regexp {\[inherit ([\.A-Za-z0-9]*)\]([\.A-Za-z0-9]*)} \
              $value match id label] == 1} {
           set pElem($attr,$index) $label
           if {$pElem(type,$indexSave) != "composite"} {
              incr pElem(numWins,$index)
           incr pElem(numAttr,$index)
           set i $pElem(numAttr,$index)
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) $id
           set pElem(L$i,$index) $label
           set pElem(V$i,$index) ""
           set pElem(S$i,$index) inherit
           } else {
           incr pElem(numInherit,$index)
           set i $pElem(numInherit,$index)
           set pElem(iN$i,$index) $attr
           set pElem(iB$i,$index) $id
           if {$attr == "phaseTableName"} {
# puts "inherit phaseShow id= $id"
              lappend pElem(phTablesInherit,$indexSave) $id
           }
           }
#  puts "inherit attribute $attr,$index with $id and ext '$label'"
#  puts "pElem($attr,$index) is $pElem($attr,$index)"
        } elseif {[regexp {primitive[^"]+"([^"]*)"} \
             $value match label] == 1} {
           incr pElem(numAttr,$index)
           incr pElem(numWins,$index)
           set i $pElem(numAttr,$index)
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) 0
           set pElem(L$i,$index) $psgLabel($attr)
           set pElem(LE$i,$index) $psgLabel($attr,entry)
           set pElem(V$i,$index) $label
           set pElem(S$i,$index) primitive
           if {$attr == "phaseTableName"} {
puts "primitive phaseShow label= $label"
              lappend pElem(phTablesInherit,$indexSave) V$i
           }
#  puts "primitive attribute $attr with default '$label'"
        } elseif {[regexp {label([0-9]+)} $attr match id] == 1}  {
           set compLabel($id) $value
           if {[regexp \"(.*)\" $value match res] == 1} {
              set compLabel($id) $res
           }
#  puts "label attribute $id '$value'"
        } elseif {[regexp {entry([0-9]+)} $attr match id] == 1}  {
           set compEntry($id) $value
#  puts "entry attribute $id '$value'"
        } elseif {[regexp {attr([0-9]+)} $attr match id] == 1}  {
           if {$composite == 1} {
              incr pElem(numAttr,$index)
              incr pElem(numWins,$index)
              set i $pElem(numAttr,$index)
              set pElem(N$i,$index) $attr
              set pElem(B$i,$index) 0
              set pElem(L$i,$index) $compLabel($id)
              set pElem(LE$i,$index) $compEntry($id)
              set pElem(V$i,$index) $value
              set pElem(S$i,$index) user
#  puts "attr attribute $id with label $compLabel($id)"
           } else {
              incr pElem(numAttr,$index)
              set i $pElem(numAttr,$index)
              set pElem(N$i,$index) $attr
              set pElem(B$i,$index) 0
              set pElem(L$i,$index) {}
              set pElem(V$i,$index) $value
              set pElem(S$i,$index) fixed
# puts "attr attribute $id with value $value"
#  puts "pElem(V$i,$index) set to $value"
           }
        } else {
#  puts "fixed attribute $attr with arguments $value"
           incr pElem(numAttr,$index)
           set i $pElem(numAttr,$index)
           set pElem($attr,$index) $value
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) 0
           set pElem(L$i,$index) ""
           set pElem(V$i,$index) $value
           set pElem(S$i,$index) fixed
           if {$attr == "phaseTableName"} {
# puts "fixed phaseShow value= $value"
              lappend pElem(phTablesFixed,$indexSave) $value
           }
#  puts "set pElem($attr,$index) $value"
        }
     } elseif {[regexp {<([a-zA-Z0-9_]+)>} $line match el]  == 1} {
        lappend stack $el
        set getAttr 1
        if {$el == "composite"} {
           set composite 1
           catch {unset compLabel}
           catch {unset compEntry}
           set pElem(type,$index) "composite"
#  puts "composite entry"
        } elseif {$el == "collection"} {
           set composite 1
           set pElem(type,$index) "collection"
        } elseif {$composite == 0} {
           set pElem(label,$index) $el
        } else {
#           if {($pInfo(doComp) != 1) && \
#               ($pElem(type,$indexSave) == "composite")} {
#              close $fd
#              set pElem(elemLoaded,$indexSave) 1
#              return
#           }
           incr pElem(comps,$indexSave)
#puts "members for pElem(comps,$indexSave) is $pElem(comps,$indexSave)"
           set pElem(comp$pElem(comps,$indexSave),$indexSave) $el
           set index $indexSave,$composite
#  puts "composite member with new index $index"
           incr composite
           set pElem(name,$index) $index
           set pElem(elemName,$index) $el
           set pElem(label,$index) $el
           set pElem(comps,$index) 0
           set pElem(numAttr,$index) 0
           set pElem(numWins,$index) 0
           set pElem(numInherit,$index) 0
        }
#  puts "new component $el for PS $index"
     } elseif {[regexp {</([a-zA-Z0-9_]+)>} $line match el]  == 1} {
        set stackVal [lindex $stack end]
        if {$stackVal == $el} {
#  puts "finish component $el"
           set stack [lrange $stack 0 [expr [llength $stack]-2]]
        } else {
           puts "Syntax error.  Expected $stackVal terminater but got $el"
        }
        set getAttr 0
     } else {
        puts "UNKNOWN line $line"
     }
   }
   close $fd
   set pElem(elemLoaded,$indexSave) 1
}

proc setListSelection {id} {
   global pInfo
   .list2 selection set [lsearch [.list2 get 0 end] $pInfo(factory)]
}

proc exitEntry {} {
   global pInfo
   set pInfo(entryGeom) [wm geometry .entryExpand]
   destroy .entryExpand
}

proc saveEntry {} {
   global psgInfo pInfo
   set index $pInfo(expandId),V$pInfo(expandIndex)
   set psgInfo($index) ""
   .entryExpand.fr1.text mark set insert end
   .entryExpand.fr1.text tag remove sel 1.0 end
   .entryExpand.fr1.text see insert
# puts "get res: [.entryExpand.fr1.text get 1.0 end]"
   set psgInfo($index) [.entryExpand.fr1.text get 1.0 end]
   set psgInfo($index) [string trimright $psgInfo($index)]
}

proc expandEntry {id index} {
   global psgInfo pInfo
   set w .entryExpand
   set w1 $w.fr1
   set w2 $w.fr2
# puts "called expandEntry"
   if {[winfo exists $w] != 1} {
# puts "build window expandEntry"
      toplevel $w
      frame $w1
      frame $w2
      text $w1.text -relief raised -borderwidth 2 \
         -yscrollcommand "$w1.scroll set" -width 40
      scrollbar $w1.scroll -command "$w1.text yview"
      pack $w1.scroll -side right -anchor w -fill y -in $w1
      pack $w1.text -side right -anchor e -expand 1 -fill both -in $w1
      button $w2.collOkay -text OK -command "saveEntry; exitEntry"
      button $w2.collCancel -text Cancel -command "exitEntry"
      pack $w2.collOkay $w2.collCancel -side left -padx 5 -pady 5 -in $w2
      pack $w2 -side bottom -fill x
      pack $w1 -side top -expand 1 -fill both
      wm protocol $w WM_DELETE_WINDOW {
         exitEntry
      }
   } else {
      saveEntry
   }
   set pInfo(expandId) $id
   set pInfo(expandIndex) $index
   $w1.text delete 1.0 end
   $w1.text insert end $psgInfo($id,V$index)
   wm title $w "$psgInfo($id,L$index)"
# puts "test geom pInfo(entryGeom)= [info exists pInfo(entryGeom)]"
   if {[info exists pInfo(entryGeom)] == 1} {
      wm geometry $w $pInfo(entryGeom)
   }
}

proc clearWindows {} {
   global pInfo
   set i 0
   while {$i < $pInfo(wins)} {
      incr i
      grid remove .f$i.lab .f$i.ent .f$i.menu .f$i.pop
   }
   .comp1 configure -text "Pulse Elements"
}

proc setChoice {id index} {
   global psgInfo
# puts "setChoice $id $index"
# puts "setChoice psgInfo($id)= $psgInfo($id)"
   set psgInfo([lindex $psgInfo($id) 0]) \
         \"[lindex $psgInfo([lindex $psgInfo($id) 1]) $index]\"
}

proc setChoice2 {id index} {
   global psgInfo
# puts "setChoice $id $index"
# puts "setChoice psgInfo($id)= $psgInfo($id)"
   set psgInfo([lindex $psgInfo($id) 0]) \
          [lindex $psgInfo([lindex $psgInfo($id) 1]) $index]
   updateMeta2 [lindex $psgInfo($id) 2] [lindex $psgInfo($id) 3]
}

proc relabel2 {el ii} {
   global psgInfo pInfo
   if {[info exists psgInfo($el,labelIdVal)] == 1} {
   set psgInfo($el,labelVal) $psgInfo($el,V$ii)
   if {$psgInfo($el,labelVal) != ""} {
      set txt [string trim $psgInfo($el,V$ii)]
      set clr $pInfo(labelColor)
   } else {
      set txt [string trim $psgInfo($el,label)]
      set clr darkgreen
   }
   if {[regexp "\[^\n\]*\n" $txt match] == 1} {
      set txt $match
   }
   .draw itemconfigure tLv$psgInfo($el,labelIdVal) -text $txt -fill $clr
   }
}

proc makeWindows {el} {
   global pInfo psgInfo
# puts "make windows for el $el with $psgInfo($el,numWins) windows"
   if {$pInfo(wins) < $psgInfo($el,numWins)} {
      while {$pInfo(wins) < $psgInfo($el,numWins)} {
         incr pInfo(wins)
         set i $pInfo(wins)
         frame .f$i 
         label .f$i.lab -width 20 -textvariable psgInfo($el,L$i) -anchor w
         entry .f$i.ent -width 18 -relief sunken -highlightthickness 0 \
               -textvariable psgInfo($el,V$i)
         menubutton .f$i.menu -width 10 -highlightthickness 0 \
               -menu .f$i.menu.list -anchor w -relief raised \
               -indicatoron 1 -pady 0 -direction flush
         menu .f$i.menu.list -tearoff 0
         choiceEntry .f$i.pop "yes no" setChoice $i
#         grid .f$i.lab -in .f$i -row $i -column 0 -sticky nw
#         grid .f$i.ent -in .f$i -row $i -column 1 -sticky ne
         .comp2 window create end -window .f$i
      }
   }
   if {$pInfo(wins) > $psgInfo($el,numWins)} {
      set i $psgInfo($el,numWins)
      while {$i < $pInfo(wins)} {
         incr i
         grid remove .f$i.lab .f$i.ent .f$i.menu .f$i.pop
      }
   }
   set ii 0
   set setFocus 0
   for {set i 1} {$i <= $psgInfo($el,numWins)} {incr i} {
      incr ii
      while {$psgInfo($el,S$ii) == "fixed"} {
         incr ii
      } 
      grid .f$i.lab -in .f$i -row $i -column 0 -sticky nw
      .f$i.lab configure -textvariable psgInfo($el,L$ii)
#      grid .f$i.lab
#      grid .f$i.ent
# puts "psgInfo($el,L$ii,type)= $psgInfo($el,L$ii,type)"
      if {$psgInfo($el,L$ii,type) == "entry"} {
         grid remove .f$i.menu .f$i.pop
         grid .f$i.ent -in .f$i -row $i -column 1 -sticky ne
         .f$i.ent configure -textvariable psgInfo($el,V$ii)
#         bind .f$i.ent <Return> "updateMeta"
#         bind .f$i.ent <FocusOut> "updateMeta"
         bind .f$i.ent <Return> {}
         bind .f$i.ent <FocusOut> {}
         if {$ii == $psgInfo($el,labelId)} {
#            set psgInfo($el,labelVal) $psgInfo($el,V$ii)
            bind .f$i.ent <Return> "relabel2 $el $ii"
            bind .f$i.ent <FocusOut> "relabel2 $el $ii"
#puts "set psgInfo($el,labelVal) $psgInfo($el,labelVal)"
#puts "set psgInfo($el,labelIdVal) $psgInfo($el,labelIdVal)"
         }
         bind .f$i.ent <Control-1> "expandEntry $el $ii"
         if {$setFocus == 0} {
            set setFocus $i
         }
      } elseif {$psgInfo($el,L$ii,type) == "choice"} {
         grid remove .f$i.menu
         grid .f$i.ent -in .f$i -row $i -column 1 -sticky ne
         grid .f$i.pop -in .f$i -row $i -column 2 -sticky ne
         .f$i.ent configure -textvariable psgInfo($el,V$ii)
#         bind .f$i.ent <Return> "updateMeta"
#         bind .f$i.ent <FocusOut> "updateMeta"
         bind .f$i.ent <Return> {}
         bind .f$i.ent <FocusOut> {}
         bind .f$i.ent <Control-1> "expandEntry $el $ii"
         if {$setFocus == 0} {
            set setFocus $i
         }
         bind .f$i.pop <1> "menu2_popup .f$i.pop {$psgInfo($el,L$ii,vals)}"
         set psgInfo($el,choiceId,$i) "$el,V$ii $el,L$ii,vals"
         bind .f$i.pop <ButtonRelease-1> \
             "exit_menu2 .f$i.pop setChoice $el,choiceId,$i"
      } elseif {$psgInfo($el,L$ii,type) == "execchoice"} {
         set psgInfo(phaseTablesUpdated) 0
# puts "command= $psgInfo($el,L$ii,vals)"
         set psgInfo($el,popItems) [$psgInfo($el,L$ii,vals)]
# puts "results= $psgInfo($el,popItems)"
         set psgInfo($el,choiceId) {}
         lappend psgInfo($el,choiceId) $el,V$ii $el,popItems
         if {$psgInfo(phaseTablesUpdated) == 1} {
            if {[info exists psgInfo($el,phaseId)] != 1} {
               set id [lsearch $psgInfo(phaseTables) $psgInfo($el,V$ii)]
               if {$id != -1} {
                  set psgInfo($el,phaseId) [lindex $psgInfo(phaseIds) $id]
               }
            }
            bind .f$i.ent <Return> "updateMeta2 $el $ii"
            bind .f$i.ent <FocusOut> "updateMeta2 $el $ii"
            lappend psgInfo($el,choiceId) $el $ii
            bind .f$i.pop <ButtonRelease-1> \
                "exit_menu2 .f$i.pop setChoice2 $el,choiceId"
         } else {
#            bind .f$i.ent <Return> "updateMeta"
#            bind .f$i.ent <FocusOut> "updateMeta"
            bind .f$i.ent <Return> {}
            bind .f$i.ent <FocusOut> {}
            bind .f$i.pop <ButtonRelease-1> \
                "exit_menu2 .f$i.pop setChoice $el,choiceId"
         }
         bind .f$i.ent <Control-1> "expandEntry $el $ii"
         if {$setFocus == 0} {
            set setFocus $i
         }
         bind .f$i.pop <1> "menu2_popup .f$i.pop {$psgInfo($el,popItems)}"
         grid remove .f$i.menu
         grid .f$i.ent -in .f$i -row $i -column 1 -sticky ne
         grid .f$i.pop -in .f$i -row $i -column 2 -sticky ne
         .f$i.ent configure -textvariable psgInfo($el,V$ii)
      } else {
         .f$i.menu.list delete 0 end
         set psgInfo(phaseTablesUpdated) 0
         set psgInfo(ifStyleUpdated) 0
         if {$psgInfo($el,L$ii,type) == "execmenu"} {
            set menuItems [$psgInfo($el,L$ii,vals)]
         } elseif {$psgInfo($el,L$ii,type) == "execwave"} {
            set menuItems [eval $psgInfo($el,L$ii,vals) $el]
         } else {
            set menuItems $psgInfo($el,L$ii,vals)
         }
         if {$psgInfo(phaseTablesUpdated) == 1} {
            foreach lab $menuItems {
               .f$i.menu.list add radio -label $lab \
                    -variable psgInfo($el,V$ii) \
                    -command "updateMeta2 $el $ii"
# puts "phase name psgInfo($el,PH$ii)= $psgInfo($el,PH$ii)"
            }
            .f$i.menu configure -textvariable psgInfo($el,V$ii)
            if {[info exists psgInfo($el,phaseId)] != 1} {
               set id [lsearch $psgInfo(phaseTables) $psgInfo($el,V$ii)]
               if {$id != -1} {
                  set psgInfo($el,phaseId) [lindex $psgInfo(phaseIds) $id]
               }
            }
         } elseif {$psgInfo(ifStyleUpdated) == 1} {
# puts "call updateMeta3 on radio"
            foreach lab $menuItems {
               .f$i.menu.list add radio -label $lab \
                    -variable psgInfo($el,V$ii) \
                    -command "updateMeta3 $el $ii"
            }
            .f$i.menu configure -textvariable psgInfo($el,V$ii)
         } else {
            if {$psgInfo($el,L$ii,type) == "execwave"} {
              foreach lab $menuItems {
               .f$i.menu.list add radio -label $lab \
                    -variable psgInfo($el,V$ii) \
                    -command "getWaveFiles $el $ii $i; updateMeta"
              }
            } else {
              foreach lab $menuItems {
               .f$i.menu.list add radio -label $lab \
                    -variable psgInfo($el,V$ii) \
                    -command updateMeta
              }
            }
            .f$i.menu configure -textvariable psgInfo($el,V$ii)
         }
         grid remove .f$i.ent .f$i.pop
         grid .f$i.menu -in .f$i -row $i -column 1 -sticky ne
      }
   }
   if {$setFocus != 0} {
      .f$setFocus.ent select range 0 end
      .f$setFocus.ent icur end
      focus .f$setFocus.ent
#puts "entry bindings [bind .f$setFocus.ent]"
   }
}

proc updateMeta2 {index i} {
   global psgInfo pInfo
# puts "psgInfo($index,PH$i)= $psgInfo($index,PH$i)"
# puts "set psgInfo($index,V$i) [makePhaseTableName $psgInfo($index,PH$i)]"
#   set psgInfo($index,V$i) [makePhaseTableName $psgInfo($index,PH$i)]
#    set psgInfo($index,V$i) $psgInfo($index,PH$i)
   .draw itemconfigure T$psgInfo(lastPhaseId) \
            -outline $pInfo(elemColor)
   set id [lsearch $psgInfo(phaseTables) $psgInfo($index,V$i)]
   if {$id != -1} {
      set psgInfo(lastPhaseId) [lindex $psgInfo(phaseIds) $id]
      set psgInfo($index,phaseId) $psgInfo(lastPhaseId)
      .draw itemconfigure T$psgInfo($index,phaseId) \
            -outline $pInfo(connectColor)
   }
#   updateMeta
}

proc updateMeta3 {id i} {
#    global psgInfo pInfo
# puts "updateMeta3 called id=$id i= $i"
# puts "psgInfo($id,ifStyle)= $psgInfo($psgInfo($id,ifStyleAddr))"
   drawPsg
#   updateMeta
}

proc relabel {num} {
   global psgInfo
   .draw itemconfigure ChanLabel$num -text "$psgInfo(Chan$num,V1)"
#   updateMeta
}

proc selectPsgElem {id} {
   global pInfo psgInfo
   set pInfo(elName) $psgInfo($id,name)
   makeWindows $id
   .comp1 configure -text "Pulse Element <$psgInfo($id,elemLabel)>"
}

proc selectPsgSeq {} {
   global pInfo psgInfo seqName
   set pInfo(elName) $psgInfo(seq,name)
   makeWindows seq
   .comp1 configure -text "Pulse Sequence"
}

proc replacePsgElem {elName} {
   global pInfo psgInfo
   set id $psgInfo(cur)
   set pInfo(elName) $elName
   loadElem $elName
   changePsgInfo $id
   makeWindows $id
   .comp1 configure -text "Pulse Element $psgInfo($id,elemLabel)"
}

proc selectElem {index} {
   global pInfo psgInfo
   if {$index != ""} {
   set elIndex $pInfo(elemIndex,$index)
#   set pInfo(elIndex) $elIndex
   set elName [.list2 get $index]
#   loadElem $elIndex
   enableFactory $elName
   if {$psgInfo(replace) == 1} {
      replacePsgElem $elIndex
   }
   }
}

proc fillList {} {
   global pInfo pElem help_tips
   .list2 delete 0 end
   set cur 0
   for {set i 0} {$i < $pElem(numGroups)} {incr i} {
      if {$pElem(groupOnOff,$i) == "on"} {
         for {set j 0} {$j < $pElem(numElems,$i)} {incr j} {
            .list2 insert end $pElem(elemLabel,$i,$j)
#            puts "line $cur is $i,$j"
            catch {unset $help_tips(.list2,$cur)}
            set pInfo(elemIndex,$cur) $i,$j
            set file [file join \
              $pElem(groupPath,$i) help $pElem(elemName,$i,$j)]
             if {[file exists $file] == 1} {
              set fd [open $file r]
              set str [read -nonewline $fd]
#	      puts "Help for <$cur> is==>\n$str"
              set help_tips(.list2,$cur)  $str
              close $fd
             }
            incr cur
         }
      }
   }
   set pInfo(listLength) $cur
}

proc makeListFrame {win} {
   global pInfo
   frame $win.list -relief flat -borderwidth 2
   label .list1 -text "Components"
   listbox .list2 -relief raised -borderwidth 2 -selectmode single \
         -yscrollcommand ".scroll set" -width 40 -height 10
   scrollbar .scroll -command ".list2 yview"
   pack .list1 -side top -in $win.list
   pack .scroll -side right -anchor w -in $win.list -fill y -expand 1
   pack .list2 -in $win.list -side right -anchor e -expand 1 -fill both
   pack $win.list -side top -anchor w -fill both -expand 1
#   fillList
#   bind .list2 <ButtonRelease-1> {
#      selectElem [.list2 curselection]
#   }
#   bind .list2 <Button-1> {
#      after 1 {createElem [.list2 curselection]}
#   }
}

proc select {w x y tag} {
  global wids
  set wids($w,$tag,x) $x
  set wids($w,$tag,y) $y
}

proc drag {w x y tag} {
  global wids
  $w move $tag [expr $x - $wids($w,$tag,x)] [expr $y - $wids($w,$tag,y)]
  set wids($w,$tag,x) $x
  set wids($w,$tag,y) $y
}

proc dragC {w x y tag} {
  drag $w [$w canvasx $x] [$w canvasy $y] $tag
}

proc setPsgInfo {} {
   global numChan psgInfo dockList scaled
   catch {unset psgInfo}
   catch {unset dockList}
   makeChanInfo 8
   set psgInfo(replace) 0
   set psgInfo(elem) 0
   set psgInfo(elems) {}
   set psgInfo(seqOK) {}
   set psgInfo(fract) 1.0
   set psgInfo(detach) {}
   set psgInfo(phTablesInherit) {}
   set psgInfo(phTablesFixed) {}
   set psgInfo(phaseTables) {}
   set psgInfo(phaseIds) {}
   set psgInfo(lastPhaseId) 0
   set psgInfo(lastPhase) "phase"
   set psgInfo(phase,tags) {}
   set psgInfo(outList) {}
   set psgInfo(phaseList) {}
   set psgInfo(idAtPhList) {}
   set psgInfo(cur) 0
   set psgInfo(numChan) $numChan
   set psgInfo(curChan) 0
   set psgInfo(scale) 100.0
   set psgInfo(currentRatio) 1.0
   set psgInfo(start,name) start
   set psgInfo(start,label) start
   set psgInfo(start,len) 1
   set psgInfo(start,defLen) 1
   set psgInfo(start,parallel) "no"
   set psgInfo(start,style) "start"
   set psgInfo(start,shape) "start"
   set psgInfo(start,tags) {}
   set psgInfo(screen,tags) {}
   set psgInfo(start,x) 0
   set psgInfo(0,name) start
   set psgInfo(skipList) {stipple shape labelId parallel name frontId frontDock midId midDock endId endDock lastId}
   set psgInfo(bg) [.draw cget -bg]
   set dockList(maxx) [expr [winfo width .draw] -4]
   set dockList(num) 1
   set dockList(1) 0
   set dockList(1,id) start
   set dockList(1,pos) end
   set dockList(start,front) 1
   set dockList(start,mid) 1
   set dockList(start,end) 1
   set dockList(curList) {}
   set dockList(curStyle) {}
   set numChan 1
   set scaled 100.0
}

proc upDateChannelInfo {id} {
   global psgInfo
#puts "upDateChannelInfo for id $id"
   for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
#puts "check psgInfo($id,S$j) $psgInfo($id,S$j)"
#puts "check psgInfo($id,N$j) $psgInfo($id,N$j)"
#puts "check psgInfo($id,V$j) $psgInfo($id,V$j)"
#puts "check psgInfo($id,$psgInfo($id,N$j)) $psgInfo($id,$psgInfo($id,N$j))"
      set psgInfo($id,V$j) $psgInfo($id,$psgInfo($id,N$j))
   }
}

proc upDatePsgInfo {} {
   global psgInfo
   set id $psgInfo(cur)
# puts "upDatePsgInfo for id $id"
   for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
# puts "check psgInfo($id,S$j) $psgInfo($id,S$j)"
# puts "check psgInfo($id,N$j) $psgInfo($id,N$j)"
# puts "check psgInfo($id,V$j) $psgInfo($id,V$j)"
# puts "check psgInfo($id,$psgInfo($id,N$j)) $psgInfo($id,$psgInfo($id,N$j))"
      if {($psgInfo($id,S$j) == "primitive") || \
          ($psgInfo($id,S$j) == "user")} {
        if {[info exists psgInfo($id,$psgInfo($id,N$j))] == 1} {
           set psgInfo($id,V$j) $psgInfo($id,$psgInfo($id,N$j))
        } else {
           set psgInfo($id,V$j) "Unaltered"
        }
      }
   }
}

proc updateIfStyle {} {
   global psgInfo
   set psgInfo(ifStyleUpdated) 1
# puts "called updateIfStyle"
   lappend list Both True False
}

proc makePhaseList {} {
   global psgInfo
#  puts "psgInfo(phTablesInherit)= $psgInfo(phTablesInherit)"
#  puts "psgInfo(phTablesFixed)= $psgInfo(phTablesFixed)"
# puts "prefix inherit table list is $psgInfo(prefixTablesInherit)"
# puts "prefix fixed table list is $psgInfo(prefixTablesFixed)"
   set list {}
   set idList {}
   foreach {id val} $psgInfo(phTablesFixed) {
      if {$psgInfo($id,inuse) == 1} {
      if {([lsearch $list $val] == -1) && \
          ([string match *COMPVALUE* $val] == 0)} {
# puts "F: val= $val"
         lappend list \"$val\"
         lappend idList $id
      }
      }
   }
   foreach {id el} $psgInfo(phTablesInherit) {
# puts "inuse= $psgInfo($id,inuse)"
      if {$psgInfo($id,inuse) == 1} {
      set val $psgInfo($id,$el)
# puts "I: val= $val"
      if {[lsearch $list $val] == -1} {
         if {$val != ""} {
            lappend list \"$val\"
            lappend idList $id
         }
      }
      }
   }
   lappend list Unaltered
   lappend idList Unaltered
   set psgInfo(phaseTablesUpdated) 1
   set psgInfo(phaseIds) $idList
#  puts "psgInfo(phaseTables)= $list"
   set psgInfo(phaseTables) $list
   set list [ldelete $list \"oph\"]
}



proc makePhaseList2 {} {
   global psgInfo
#  puts "psgInfo(phTablesInherit)= $psgInfo(phTablesInherit)"
#  puts "psgInfo(phTablesFixed)= $psgInfo(phTablesFixed)"
# puts "prefix inherit table list is $psgInfo(prefixTablesInherit)"
# puts "prefix fixed table list is $psgInfo(prefixTablesFixed)"
   set list {}
   set idList {}
   foreach {id val} $psgInfo(phTablesFixed) {
      if {$psgInfo($id,inuse) == 1} {
      if {([lsearch $list $val] == -1) && \
          ([string match *COMPVALUE* $val] == 0)} {
# puts "F: val= $val"
         lappend list \"$val\"
         lappend idList $id
      }
      }
   }
   foreach {id el} $psgInfo(phTablesInherit) {
# puts "inuse= $psgInfo($id,inuse)"
      if {$psgInfo($id,inuse) == 1} {
      set val $psgInfo($id,$el)
# puts "I: val= $val"
      if {[lsearch $list $val] == -1} {
         if {$val != ""} {
            lappend list \"$val\"
            lappend idList $id
         }
      }
      }
   }
   set psgInfo(phaseTablesUpdated) 1
   set psgInfo(phaseIds) $idList
#  puts "psgInfo(phaseTables)= $list"
   set psgInfo(phaseTables) $list
   set list [ldelete $list \"oph\"]
}



proc makePhaseTableName {label} {
puts "makePhaseTableName from '$label'"
   set name [string trim $label]
   regsub -all " " $name _ name
   regsub -all "," $name _ name
puts "makePhaseTableName: step 1:  '$name'"
   if {[regexp {[0-9].*} $name] == 1} {
      set name ph$name
puts "makePhaseTableName: step 2:  '$name'"
   }
   return $name
}

proc loadPhaseTables {} {
   global psgInfo pInfo
   set index [getElemIndex CUSTOMprefix]
   set psgInfo(phTablesInherit) {}
   set psgInfo(phTablesFixed) {}
   if {$index != 0} {
      set psgInfo(xPhaseCur) $psgInfo(xPhase)
      setElem2 .draw $psgInfo(xPhase) $psgInfo(yPhase) currentPS $index
      clearWindows
   }
# parray psgInfo
# global pInfo pElem
# parray pInfo
# parray pElem
    .draw itemconfigure T$psgInfo(cur) -fill $psgInfo(bg) \
                -outline $pInfo(elemColor)
}

proc getWaveFiles {el ii win} {
   global env psgInfo
   set list [glob -nocomplain -- \
      [file join $env(vnmrsystem) wavelib $psgInfo($el,V$ii) *]]
   incr win
   set ii $psgInfo($el,coIndex)
   set list2 {}
   foreach f $list {
      lappend list2 [file tail $f]
   }
   set list [lsort -dictionary $list2]
   set psgInfo($el,L$ii,vals) $list
   if {[llength $list] > 0} {
      grid .f$win.pop -in .f$win -row $win -column 2 -sticky ne
      bind .f$win.pop <1> "menu2_popup .f$win.pop {$psgInfo($el,L$ii,vals)}"
   } else {
      grid remove .f$win.pop
   }
   set psgInfo($el,V$ii) "None"
}

proc getWaveDirs {eId id} {
   global env psgInfo
# puts "getWaveDirs got $eId $id"
   set list [glob -nocomplain -- [file join $env(vnmrsystem) wavelib *]]
   set list2 {}
   foreach f $list {
      lappend list2 [file tail $f]
   }
   set ii 0
   for {set i 1} {$i <= $psgInfo($id,numAttr)} {incr i} {
      if {$psgInfo($id,N$i) == $eId} {
        set ii $i
        break
      }
   }
   set psgInfo($id,coIndex) $ii
   set list [lsort -dictionary $list2]
}

proc loadPsg {hD} {
   global psgInfo seqName numChan pInfo loadInfo
   .output.comp4 configure -state normal
   .output.comp4 delete 1.0 end
   set seq 0
   set channel 0
   set numCh 0
   set chanid Chan0
   set stack {}
   set getAttr 0
   set lastPhase phase
   set elName $seqName
#   wm iconname . $seqName
   if {($seqName != "") && ([file exists [file join \
       $hD psglib $seqName]] == 1)} {
      set fd [open [file join $hD psglib $seqName] r]
      while {[gets $fd line] >= 0} {
        if {[regexp {^#} $line] == 1} {
# puts  "A comment"
          continue
        }
        if {[regexp "^\[ \t]*$" $line] == 1} {
# puts  "A blank line"
          continue
        }

        if {[regexp {</*spincadSequence>} $line] == 1} {
          continue
        }

        if {($getAttr == 1) && \
            ([regexp {[^<]*<([a-zA-Z0-9_]+)>} $line match attr ]  == 1)} {
           lappend stack $attr
           set value ""
           while {[gets $fd line] >= 0} {
# puts "new attr line is $line (cmp to </$attr>)"
              if {"</$attr>" == [string trimleft $line]} {
# puts "</$attr> == $line"
                 set stack [lrange $stack 0 [expr [llength $stack]-2]]
                 set value [string trimleft $value]
                 break
              } else {
# puts "</$attr> [string length </$attr>] != $line [string length $line] "
                 if {$value == ""} {
                    set value $line
                 } else {
                    set value "$value\n$line"
                 }
              }
           }
# puts "got attribute $attr with value $value"
           if {$seq == 1} {
              set psgInfo(seq,$attr) "$value"
           } elseif {$channel == 1} {
              if {$attr == "chanId"} {
                 set chanid Chan$value
              } else {
                 set psgInfo($chanid,$attr) "$value"
              }
           } else {
              set id $psgInfo(cur)
# puts "set psgInfo($id,$attr) to $value"
                 set psgInfo($id,$attr) "$value"
                 if {$attr == "idAt"} {
                    if {(($value != "start") && ($value != "phase"))} {
                       set psgInfo($id,tags) [concat $psgInfo($id,tags) \
                          $psgInfo($value,tags)]
#puts "append tag $psgInfo($value,tags) for $id"
                    }
                    if {($value == $lastPhase) || \
                        ($value == "phase")} {
                       set phaseAdd 1
                       set lastPhase $id
                    }
                 }
           }
        } elseif {[regexp {<([a-zA-Z0-9_]+)>} $line match el]  == 1} {
#puts "new component $el for PS $elName"
           set loadInfo(el) $el
           lappend stack $el
           if {$el == "pulseSequence"} {
             set seq 1
             set channel 0
           } elseif {$el == "channel"} {
             set channel 1
             set seq 0
             incr numCh
           } else {
             set channel 0
             set seq 0
             set phaseAdd 0
# puts "call loadElem for $el with index [getElemIndex $el]"
             loadElem [getElemIndex $el]
             addElemToPSG
           }
           set getAttr 1
        } elseif {[regexp {</([a-zA-Z0-9_]+)>} $line match el]  == 1} {
           set stackVal [lindex $stack end]
           if {$stackVal == $el} {
              set stack [lrange $stack 0 [expr [llength $stack]-2]]
              if {$seq == 1} {
                 updateSeqInfo
              } elseif {$channel == 1} {
                 upDateChannelInfo $chanid
              } else {
                 upDatePsgInfo
                 if {$phaseAdd == 0} {
                    addElem $psgInfo(cur)
                 } else {
                    addPhElem $psgInfo(cur)
                    set  psgInfo($psgInfo(cur),chanId) 0
                 }
              }
           } else {
              puts "Syntax error.  Expected $stackVal terminater but got $el"
           }
           set getAttr 0
        } else {
           puts "UNKNOWN line $line"
        }
      }
      close $fd
   }
# puts "set numChan $numCh"
   if {$numCh == 0} {
      incr numCh
   }
   set numChan $numCh
   setChans 0
   for {set i 1} {$i <= 8} {incr i} {
      updateChanInfo Chan$i
   }
#   updateMeta
   drawPsg
}

proc savePsgData {line} {
   global psgFile
#   .output.comp4 insert end "$line\n"
   incr psgFile(lines)
   set psgFile($psgFile(lines)) $line
#  puts "save line $psgFile(lines) \"$psgFile($psgFile(lines))\""
}

proc savePsgDataAttr {header val} {
   savePsgData "  <$header>"
   savePsgData "   $val"
   savePsgData "  </$header>"
}

proc updateMeta {} {
#   sortElems
}

proc updateOutput {} {
   global psgInfo psgFile errorIds
   .output.comp4 configure -state normal
   .output.comp4 delete 1.0 end
   exitError
   catch {unset errorIds}
   catch {unset psgFile}
   set psgInfo(seqOK) {}
   set psgFile(lines) 0
   for {set i 1} {$i <= $psgInfo(numChan)} {incr i} {
      savePsgData "<channel>"
      savePsgDataAttr chanId $i
# puts "chan $i attr= $psgInfo(Chan$i,numAttr)"
      for {set j 1} {$j <= $psgInfo(Chan$i,numAttr)} {incr j} {
         savePsgDataAttr $psgInfo(Chan$i,N$j) $psgInfo(Chan$i,V$j)
      }
      savePsgData "</channel>"
   }
   catch {unset psgInfo(lastCur)}
#   sortElems
   set idVal 0
# puts "updateMeta  outlist is $psgInfo(outList)"
# puts "updateMeta elemList is $psgInfo(elems)"
   foreach i $psgInfo(outList) {
         incr idVal
         savePsgData "<$psgInfo($i,name)>"
# puts "updateMeta num of attrs is $psgInfo($i,numAttr)"
         if {$psgInfo($i,collection) != ""} {
            savePsgDataAttr collection $psgInfo($i,collection)
         }
         if {$psgInfo($i,type) == "composite"} {
            savePsgDataAttr composite $psgInfo($i,label)
         }
         set errorIds($idVal) $i
         set psgInfo($i,errorId) $idVal
         savePsgDataAttr id $idVal
         for {set j 1} {$j <= $psgInfo($i,numAttr)} {incr j} {
# puts "updateMeta attrs $j is $psgInfo($i,B$j) $psgInfo($i,V$j)"
            if {$psgInfo($i,B$j) == 0} {
               set nm $psgInfo($i,N$j)
            } else {
               set nm attr$psgInfo($i,B$j)
            }
            if {[lsearch $psgInfo(skipList) $nm] == -1} {
               savePsgDataAttr $nm $psgInfo($i,V$j)
               if {($psgInfo($i,V$j) == "") && ($psgInfo($i,S$j) != "fixed")} {
# puts "set seqOK $i psgInfo($i,V$j): $psgInfo($i,V$j) $nm"
                  set psgInfo(seqOK) $i
                  setError $idVal 0 "$psgInfo($i,L$j) is not set"
               }
            }
         }
         set newID($i) $idVal
         savePsgDataAttr dock $psgInfo($i,dock)
         set newIdAt $psgInfo($i,idAt)
         if {$newIdAt != "start"} {
            set newIdAt $newID($newIdAt)
         }
         savePsgDataAttr idAt $newIdAt
         savePsgDataAttr dockAt $psgInfo($i,dockAt)
         if {$psgInfo($i,multi) == 0} {
            savePsgDataAttr chanId $psgInfo($i,chanId)
         } else {
            savePsgDataAttr chanId 1
         }
         savePsgDataAttr len $psgInfo($i,len)
#         savePsgDataAttr ht $psgInfo($i,ht)
      if {$psgInfo(cur) == $psgInfo($i,id)} {
# puts "cur=$psgInfo(cur) is now $idVal"
         set psgInfo(lastCur) $idVal
      }
      savePsgData "</$psgInfo($i,name)>"
   }
   if {[info exists psgInfo(lastCur)] == 0} {
      set psgInfo(lastCur) $idVal
   }
   set psgInfo(saveNum) $idVal
}

proc printps {} {
   global dockList psgInfo seqName
   .draw create text 3.5i [expr $psgInfo(ytop) + 10] -text "Pulse sequence $seqName" \
               -tags "printLabel" -anchor nw -font {courier 12 bold}
   .draw postscript -colormode gray -rotate 1 -file "/vnmr/tmp/prpstmp" \
       -width $dockList(maxx) -height [expr $psgInfo(ybot) - $psgInfo(ytop) - 2]\
       -x 0 -y [expr $psgInfo(ytop) + 2] -pageheight 8i
   .draw delete printLabel
   eval exec {/usr/dt/bin/sdtimage /vnmr/tmp/prpstmp}
}

proc debugger {} {
#   toplevel .output
#   wm title .output "PSG Metafile"
   frame .output
   text .output.comp4 -relief raised -borderwidth 2 -state disabled\
         -yscrollcommand ".output.scroll4 set" -width 40
   scrollbar .output.scroll4 -command ".output.comp4 yview"
#   pack .output.scroll4 -side right -anchor w -fill y -expand 1
#   pack .output.comp4 -side right -anchor e -expand 1 -fill both
}

proc exitOutput {} {
   global pInfo
   set pInfo(printGeom) [wm geometry .fileOut]
   destroy .fileOut
   .draw delete txtLblNum
}

proc showOutput {} {
   global seqName psgInfo psgFile pInfo theText
   set w .fileOut
   set w1 $w.fr1
   set w2 $w.fr2
   if {[winfo exists $w] == 0} {
      toplevel $w
      if {[info exists pInfo(printGeom)] == 1} {
         wm geometry $w $pInfo(printGeom)
      }
      frame $w1
      frame $w2
      text $w1.comp4 -relief raised -borderwidth 2\
         -yscrollcommand "$w1.scroll4 set" -width 40
      scrollbar $w1.scroll4 -command "$w1.comp4 yview"
      pack $w1.scroll4 -side right -anchor w -fill y -in $w1
      pack $w1.comp4 -side right -anchor e -expand 1 -fill both -in $w1
      button $w2.but1 -text Update -command showOutput
      button $w2.but2 -text "Print text" -command {
        set tfile [file join $env(vnmrsystem) tmp prpstmp]
      	set fd [open $tfile w]
      	puts $fd $theText
      	close $fd
      	eval exec {lp -c $tfile}
      }
      button $w2.but3 -text "Show graphic..."  -command "printps"
      button $w2.but4 -text "Cancel"  -command "exitOutput"
      pack $w2.but1 $w2.but2 $w2.but3 $w2.but4 -side left \
           -padx 5 -pady 5 -in $w2
      pack $w2 -side bottom -fill x
      pack $w1 -side top -expand 1 -fill both
   }
   wm title $w "Pulse Sequence $seqName"
   raise $w
   $w1.comp4 configure -state normal
   $w1.comp4 delete 1.0 end
   $w1.comp4 insert end "\nPulse Sequence $seqName\n"
   if {$psgInfo(seq,V1) != ""} {
      $w1.comp4 insert end [set psgInfo(seq,V1)]
   }
   $w1.comp4 insert end "\n\n\n"
   updateMeta
   for {set i 1} {$i <= $psgInfo(numChan)} {incr i} {
      updateChanInfo Chan$i
      $w1.comp4 insert end "Channel $i\n"
      for {set j 1} {$j <= $psgInfo(Chan$i,numWins)} {incr j} {
         $w1.comp4 insert end "   $psgInfo(Chan$i,L$j)\t $psgInfo(Chan$i,V$j)\n"
      }
   }
   $w1.comp4 insert end "\n"
  
   foreach i $psgInfo(phaseList) {
      $w1.comp4 insert end "\n$psgInfo($i,elemLabel) \n"
      set ii 0
      for {set j 1} {$j <= $psgInfo($i,numWins)} {incr j} {
         incr ii
         while {$psgInfo($i,S$ii) == "fixed"} {
            incr ii
         }
         $w1.comp4 insert end "   $psgInfo($i,L$ii)\t $psgInfo($i,V$ii)\n"
      }
   }
   $w1.comp4 insert end "\n"

   set idVal 0
   .draw delete txtLblNum
   foreach i $psgInfo(outList) {
         incr idVal
         set newID($i) $idVal
         set newIdAt $psgInfo($i,idAt)
         if {$newIdAt != "start"} {
            set newIdAt $newID($newIdAt)
         }
         set x [expr $psgInfo($i,x) + ($psgInfo($i,len) / 2.0) + 2.0]
         set y [expr $psgInfo($i,y) + 30.0 + $psgInfo($i,ht)]
         .draw create text $x $y \
           -text "$idVal" -tags "textLabel tLt$idVal txtLblNum scl"
         while {$x > 0} {
          set listx  [.draw bbox tLt$idVal]
          set listL [eval .draw find overlapping $listx]
          set overlaps 0
          foreach idL $listL {
           set tagL [.draw gettags $idL]
           if {[lsearch $tagL textLabel] > -1} {
            set t [lsearch $tagL tLt$idVal]
            if {$t == -1} {
             incr overlaps
            }
           }
          }
          if {$overlaps <= 0} {
           break
          }
          set yinc 5
          .draw move tLt$idVal 0 $yinc
         }
         $w1.comp4 insert end \
             "\n$idVal: $psgInfo($i,label) on channel $psgInfo($i,chanId)\n"
         $w1.comp4 insert end \
             "   $psgInfo($i,dock) attached to $psgInfo($i,dockAt) of $newIdAt\n"
#         if {$psgInfo($i,collection) != ""} {
#            set nm [string range $psgInfo($i,collection) 6 end]
#            $w1.comp4 insert end\
#                 "   member of collection $nm\n"
#         }
         set ii 0
         for {set j 1} {$j <= $psgInfo($i,numWins)} {incr j} {
            incr ii
            while {$psgInfo($i,S$ii) == "fixed"} {
               incr ii
            } 
            $w1.comp4 insert end\
               "   $psgInfo($i,L$ii)\t $psgInfo($i,V$ii)\n"
         }
   }
   set theText [$w1.comp4 get 1.0 end]
   $w1.comp4 configure -state disabled
   wm protocol $w  WM_DELETE_WINDOW {
      exitOutput
#      set pInfo(printGeom) [wm geometry .fileOut]
#      destroy .fileOut
   }
}

proc destroyCanvasLabels {} {
   global pInfo
   .draw delete textLabel
   .mbar.labels.menu entryconfigure 1 \
     -label Show -command "makeCanvasLabels"
   .mbar.labels.menu delete 2
   set pInfo(labels) off
}

proc makeCanvasLabels {} {
   global psgInfo psgFile pInfo dockList numChan ifShow
   set idVal 0
   set pInfo(labels) on
   set newID(start) start
   set maxRows 6
   for {set i 1} {$i <= $numChan} {incr i} {
      for {set j 1} {$j <= $maxRows} {incr j} {
         set lastX($i,$j) -1
      }
   }
   set labList {}
   foreach i $psgInfo(drawList) {
      set x [expr $psgInfo($i,x) + ($psgInfo($i,len) / 2.0)]
      lappend labList "$i $x"
   }
   set labList [join [lsort -real -index 1 $labList] " "]
   foreach {i x} $labList {
         incr idVal
         if {$psgInfo($i,multi) == 1} {
            set chan $numChan
            if {[info exists ifShow($i,1)] == 1} {
               set chan 1
               for {set j 1} {$j <= $ifShow(chans)} {incr j} {
                  if {$ifShow($i,$j) == 1} {
                     set chan $j
                  }
               }
            }
         } else {
            set chan $psgInfo($i,chanId)
         }
         set chId ch$chan
         if {($psgInfo($i,labelId) != 0) && \
             ($psgInfo($i,labelVal) != "")} {
            set labelText [string trim $psgInfo($i,labelVal)]
            set col $pInfo(labelColor)
         } else {
            set labelText [string trim $psgInfo($i,label)]
            set col darkgreen
         }
         if {[regexp "\[^\n\]*\n" $labelText match] == 1} {
            set labelText $match
         }
         set len [font measure $pInfo(font) $labelText]
#         set x [expr $psgInfo($i,x) + ($psgInfo($i,len) / 2.0)]
         set x2 $x
#         set x [expr $x - $len/2]
         set testX [expr $x - $len/2 - 2]
         if {$testX < 1} {
            set testX 1
            set x [expr $len/2 + 1]
         }
         for {set j 1} {$j <= $maxRows} {incr j} {
            if {$testX > $lastX($chan,$j)} {
               set lastX($chan,$j) [expr $x + $len/2 + 2]
# puts "set lastX($chan,$j) $lastX($chan,$j)"
               break
            }
         }
# parray lastX
         set y [expr $dockList($chId) + $j*$pInfo(linespace)]
         if {$psgInfo($i,polar) < 0} { set y [expr $y + $psgInfo($i,ht)]}
#         puts "$i at $x , $y"
         set yref [expr $y - 8.0]
         set psgInfo($i,labelIdVal) $idVal
# puts "$labelText ($x $y chan= $chan j=$j)"
         .draw create text $x $y \
            -text $labelText -justify left \
            -tags "textLabel txtVal tL$idVal tLv$idVal scl" -fill $col
         set y2  [lindex [.draw bbox tL$idVal] 1]
         set y1 $yref
         if {$y1 > $y2} {set y1 [expr $y2 - 5]}
#         puts "$i marker at $x from $y1 to $y2"
         .draw create line $x2 [expr $dockList($chId) + 4] $x2 [expr $y - 4] \
           -fill $pInfo(leadColor) -tags "textLabel textLead tL$idVal scl"
    }
   .draw lower textLead
   .mbar.labels.menu entryconfigure 1 \
      -label Update -command "destroyCanvasLabels; makeCanvasLabels"
   .mbar.labels.menu add command -label Hide -command "destroyCanvasLabels"
#   .draw raise textLabel 
   .draw raise txtVal 
}

proc checkFile {eFile tFile} {
   global pInfo
   if {[file exists $tFile] != 1} {
      after 1000 "checkFile $eFile $tFile"
      return
   }
   exec rm $tFile
   if {[file exists $eFile] == 1} {
      showError $eFile
      .psg1 configure -fg red
   } else {
      .psg1 configure -fg $pInfo(fg)
   }
}

proc saveSelf {{arg check}} {
   global seqName pInfo psgFile linesOK pElem psgInfo env \
          confPath errorFd errorIds vnmrsendOK
   
   set errorFile [file join $env(vnmruser) seqlib $seqName.error]
   catch {exec rm $errorFile}
   updateOutput
   . configure -cursor watch
   set fd [open $confPath w]
#   puts $fd "wm geometry .output [wm geometry .output]"
   puts $fd "set seqName \{$seqName\}"
   puts $fd "set pInfo(seqDir) $pInfo(seqDir)"
   puts $fd "set linesOK $linesOK"
   for {set i 0} {$i < [expr $pElem(numGroups) -1]} {incr i} {
      puts $fd "set pElem(groupOnOff,$i) $pElem(groupOnOff,$i)"
   }
   puts $fd "set pInfo(elemColor) $pInfo(elemColor)"
   puts $fd "set pInfo(labelColor) $pInfo(labelColor)"
   puts $fd "set pInfo(leadColor) $pInfo(leadColor)"
   puts $fd "set pInfo(selectColor) $pInfo(selectColor)"
   puts $fd "set pInfo(connectColor) $pInfo(connectColor)"
   puts $fd "set pInfo(dockColor) $pInfo(dockColor)"
   puts $fd "set pInfo(chanColor) $pInfo(chanColor)"
   puts $fd "set pInfo(backColor) $pInfo(backColor)"
   puts $fd "set pInfo(volume) $pInfo(volume)"
   puts $fd "set pInfo(labels) $pInfo(labels)"
   puts $fd "set pInfo(collectionInst) $pInfo(collectionInst)"
#  On Mac, wm geometry sometimes returns negative values 
   regsub -all -- - [wm geometry .] {} geom
   puts $fd "set pInfo(rootGeom) $geom"
   if {[info exists pInfo(colorGeom)] == 1} {
      puts $fd "set pInfo(colorGeom) $pInfo(colorGeom)"
   }
   if {[info exists pInfo(helpGeom)] == 1} {
      puts $fd "set pInfo(helpGeom) $pInfo(helpGeom)"
   }
   if {[info exists pInfo(collectionGeom)] == 1} {
      puts $fd "set pInfo(collectionGeom) $pInfo(collectionGeom)"
   }
   if {[info exists pInfo(printGeom)] == 1} {
      puts $fd "set pInfo(printGeom) $pInfo(printGeom)"
   }
   if {[info exists pInfo(entryGeom)] == 1} {
      puts $fd "set pInfo(entryGeom) $pInfo(entryGeom)"
   }
   if {[info exists pInfo(errorGeom)] == 1} {
      puts $fd "set pInfo(errorGeom) $pInfo(errorGeom)"
   }
   close $fd
   if {$seqName == ""} {
      set seqName noname
   }
#   makePhaseList
   set fd [open [file join $pInfo(userDir) psglib $seqName] w]
   fconfigure $fd -buffering none
# puts "phaseIDs list= $psgInfo(phaseIds)"
   set idVal $psgInfo(saveNum)
   set lastId "phase"
   puts $fd "<spincadSequence>"
   foreach i $psgInfo(phaseList) {
         savePsgData "<$psgInfo($i,name)>"
# puts "updateMeta num of attrs is $psgInfo($i,numAttr)"
         if {$psgInfo($i,collection) != ""} {
            savePsgDataAttr collection $psgInfo($i,collection)
         }
         if {$psgInfo($i,type) == "composite"} {
            savePsgDataAttr composite $psgInfo($i,label)
         }
         incr idVal
         set errorIds($idVal) $i
         set psgInfo($i,errorId) $idVal
         for {set j 1} {$j <= $psgInfo($i,numAttr)} {incr j} {
# puts "updateMeta attrs $j is $psgInfo($i,B$j) $psgInfo($i,V$j)"
            if {$psgInfo($i,B$j) == 0} {
               set nm $psgInfo($i,N$j)
            } else {
               set nm attr$psgInfo($i,B$j)
            }
            if {[lsearch $psgInfo(skipList) $nm] == -1} {
               savePsgDataAttr $nm $psgInfo($i,V$j)
               if {($psgInfo($i,V$j) == "") && ($psgInfo($i,S$j) != "fixed")} {
# puts "set seqOK $i psgInfo($i,V$j): $psgInfo($i,V$j) $nm"
                  set psgInfo(seqOK) $i
                  setError $idVal 0 "$psgInfo($i,L$j) is not set"
               }
            }
         }
         savePsgDataAttr id $idVal
         savePsgDataAttr dock front
         savePsgDataAttr idAt $lastId
         savePsgDataAttr dockAt end
         savePsgDataAttr chanId $psgInfo($i,chanId)
         savePsgDataAttr x $psgInfo($i,x)
         savePsgDataAttr y $psgInfo($i,y)
         savePsgDataAttr len $psgInfo($i,len)
#         savePsgDataAttr ht $psgInfo($i,ht)
         savePsgData "</$psgInfo($i,name)>"
         set lastId $idVal
   }
   savePsgData "<pulseSequence>"
   savePsgDataAttr scale $psgInfo(scale)
   set date [clock format [clock seconds]]
#   puts "Date is $date"
   savePsgDataAttr date $date
   savePsgDataAttr version $pInfo(sCVersion)
   savePsgDataAttr comment $psgInfo(seq,V1)
   savePsgData "</pulseSequence>"

   for {set i 1} {$i <= $psgFile(lines)} {incr i} {
      puts $fd $psgFile($i)
   }
   puts $fd "</spincadSequence>"
   close $fd
   if {$errorFd != -1} {
      close $errorFd
      set errorFd -1
   }
   catch {exec rm [file join $env(vnmruser) seqlib $seqName.psg]}
   if {$psgInfo(seqOK) == ""} {
      .psg1 configure -fg yellow
      update idletasks
      catch {exec /vnmr/tcl/bin/spingen $seqName >& [file join $env(vnmruser) seqlib seqOut]} ret
#      puts "spingen return value is $ret"
      if {$vnmrsendOK == 1} {
         set vnmrsendOK [vnmractive]
      }
      if {$ret != ""} {
         .psg1 configure -fg red
         if {[file exists $errorFile] == 1} {
            showError $errorFile
         }
      } elseif {$vnmrsendOK == 1} {
         .psg1 configure -fg cyan
         set tstFile [file join $env(vnmruser) seqlib $seqName.check]
         catch {exec rm $tstFile}
         if {$arg == "check"} {
            vnmrsend "sccheck('$seqName','$errorFile','$tstFile')"
         } else {
            vnmrsend "createparamsvnmrj('$seqName','$tstFile')"
         }
         after 1000 "checkFile $errorFile $tstFile"
         catch {exec /vnmr/tcl/bin/spingen -dps $seqName >& [file join \
               $env(vnmruser) seqlib dpsOut]} ret
      } else {
         .psg1 configure -fg $pInfo(fg)
         catch {exec /vnmr/tcl/bin/spingen -dps $seqName >& [file join \
               $env(vnmruser) seqlib dpsOut]} ret
      }
   } else {
# puts "psgInfo(seqOK): $psgInfo(seqOK)"
      .psg1 configure -fg red
      set coords [.draw coords T$psgInfo(seqOK)]
      if {[lsearch $psgInfo(hideList) $psgInfo(seqOK)] != -1} {
         unHide $psgInfo(seqOK)
         drawPsg
      }
      selectAnItem .draw $psgInfo(seqOK) [lindex $coords 0] [lindex $coords 1] 
      if {[file exists $errorFile] == 1} {
         showError $errorFile
      }
   }
   . configure -cursor $pInfo(cursor)
}

proc addComp {win x y} {
   set tags [$win gettags [$win find closest $x $y]]
   set id 0
   foreach val $tags {
      if {[regexp {T([0-9]+)} $val match id] == 1} {
         break
#         set id [string range $val 1 end]
      }
   }
# puts "add compositie with id $id"
   addComptoMenu $id
}

proc switchWins {} {
   global compositeOK pInfo psgInfo

   if {$compositeOK == 1} {
      exitCollection
      initCompInfo
      set pInfo(composite) 1
      pack .comp -in .psg -side left -fill y -expand false -anchor nw
      pack forget .info
      .draw itemconfigure T$psgInfo(cur) -fill $psgInfo(bg)
      if {($psgInfo(cur) != -1) && ($psgInfo($psgInfo(cur),idAt) != "start")} {
         .draw itemconfigure T$psgInfo($psgInfo(cur),idAt) \
            -outline $pInfo(connectColor)
      }
#      disableFactory
      .draw bind element <Button-2>  {addComp %W %x %y}
   } else {
      set pInfo(composite) 0
      saveComp
      exitComp
      readElemFiles
      fillList
      pack .info -in .psg -side left -fill y -expand false -anchor nw
      pack forget .comp
#      reenableFactory $pInfo(factory)
      catch {selectElem [getElemIndex $pInfo(factory)]}
      .draw itemconfigure T$psgInfo(cur) -fill $psgInfo(bg)
      if {($psgInfo(cur) != -1) && ($psgInfo($psgInfo(cur),idAt) != "start")} {
         .draw itemconfigure T$psgInfo($psgInfo(cur),idAt) \
            -outline $pInfo(connectColor)
      }
      .draw bind element <Button-2>  {}
      foreach id $psgInfo(elems) {
         .draw bind T$id <Button-1> "startAdjustment .draw $id %x %y"
         .draw bind T$id <Button-3> "selectAnItem .draw $id %x %y"
         .draw bind T$id <Control-Button-1> "replaceItem .draw $id %x %y"
      }
   }
}

proc readElemFiles {} {
   global pElem psgLabel
   for {set i 0} {$i < $pElem(numGroups)} {incr i} {
#puts "group $i"
      set pElem(numElems,$i) 0
      set j 0
      if {[file exists [file join $pElem(groupPath,$i) elements]] == 1} {
         catch {source [file join $pElem(groupPath,$i) labels]}
         set fd [open [file join $pElem(groupPath,$i) elements] r]
         while {[gets $fd line] >= 0} {
            if {[regexp {^#} $line] == 1} {
               continue
            }
            if {[regexp "^\[ \t]*$" $line] == 1} {
               continue
            }
            if {[scan $line "%s %\[^\n\]\n" elem label]  == 2} {
               set pElem(elemName,$i,$j) $elem
               set pElem(elemGroup,$i,$j) $i
               set pElem(elemLabel,$i,$j) $label
               set pElem(elemLoaded,$i,$j) 0
               incr j
               set pElem(numElems,$i) $j
#puts "elems in $i are $j"
            }
         }
         close $fd
      }
   }
}

proc makeElemMenu {win} {
   global pElem pInfo env
   set fd [open [file join $pInfo(homeDir) psgFiles] r]
   set pElem(numGroups) 0
   set pElem(nextDefault) 0
#puts "makeElemMenu"
   while {[gets $fd line] >= 0} {
#puts "makeElemMenu got $line"
     if {[regexp {^#} $line] == 1} {
# puts  "A comment"
        continue
     }
     if {[regexp "^\[ \t]*$" $line] == 1} {
# puts  "A blank line"
        continue
     }
     if {[scan $line "%s %s %\[^\n\]\n" dir type label]  == 3} {
#puts "add elem menu with label $label"
        set pElem(groupPath,$pElem(numGroups)) [subst $dir]
        set pElem($type) $pElem(numGroups)
        set pElem(groupOnOff,$pElem(numGroups)) off
        $win add checkbutton -label $label -command fillList\
            -variable pElem(groupOnOff,$pElem(numGroups)) \
            -onvalue on -offvalue off
        incr pElem(numGroups)
     }
   }
   close $fd
#   set pElem(groupOnOff,1) on
#   set pElem(groupOnOff,2) on
   readElemFiles
}

proc readNewPsglib {name dir} {
   global seqName psgInfo env pInfo scaled
   set seqName $name
   set pInfo(seqDir) $dir
   .draw delete element
   setPsgInfo
   clearWindows
   loadPsg $dir
   updateExitMenu
   .psg1 configure -fg $pInfo(fg)
   if {$name == ""} {
      loadPhaseTables
      selectPsgSeq
   } else {
      if {[file exists [file join $env(vnmruser) seqlib $seqName.psg]] == 0} {
         .psg1 configure -fg red
      }
   }
   makePhaseList
   set psgInfo(xPhaseCur) $psgInfo(xPhase)
   zoom $scaled
   .draw delete Scale
   set psgInfo(cur) -1
}

proc makePsglibMenu {dir m} {
   global pInfo
   set psglib [lsort -dictionary [glob -nocomplain -- [file join $dir psglib *]]]
   $m delete 0 end
   set num [llength $psglib]
   set colBreak [expr $num + 1]
   if {$num > 30} {
      set cols [expr ($num + 29) / 30]
      set colBreak [expr ($num + $cols - 1) / $cols]
#      set colBreak [expr $colBreak + ($colBreak % 2)]
   }
# puts "num: $num cols: $cols break: $colBreak"
   set i 0
   foreach  elem $psglib {
      set file [file tail $elem]
      if {$i == $colBreak} {
         $m add radio -label "$file" -command "readNewPsglib $file $dir" \
            -columnbreak 1
         set i 0
      } else {
         $m add radio -label "$file" -command "readNewPsglib $file $dir" \
            -columnbreak 0
      }
      incr i
   }
}

proc updateExitMenu {} {
   global seqName pInfo
   if {$seqName == ""} {
      .mbar.seq.menu entryconfigure 4 -label "" -command "" -state disabled
      .mbar.seq.menu entryconfigure 5 -label "" -command "" -state disabled
      .mbar.seq.menu entryconfigure 6 -label "Exit"
   } else {
      .mbar.seq.menu entryconfigure 4 -label "Save $seqName and make parameters" \
          -command "saveSelf par; makePsglibMenu $pInfo(userDir) $pInfo(psglibUMenu)" \
          -state normal
      .mbar.seq.menu entryconfigure 5 -label "Save and check $seqName" \
          -command "saveSelf; makePsglibMenu $pInfo(userDir) $pInfo(psglibUMenu)" -state normal
      .mbar.seq.menu entryconfigure 6 -label "Exit, saving $seqName"
   }
}

proc makemenu {comp} {
   global help_tips pInfo vnmrsendOK
   menubutton .mbar.seq -text Sequence -menu .mbar.seq.menu
   menubutton .mbar.elem -text Elements -menu .mbar.elem.menu
   menubutton .mbar.edit -text Tools -menu .mbar.edit.menu
   menubutton .mbar.help -text Help -menu .mbar.help.menu
   menubutton .mbar.labels -text Labels -menu .mbar.labels.menu
#   bind .mbar.seq <Button-1> clearFocus
#   bind .mbar.elem <Button-1> clearFocus
#   bind .mbar.edit <Button-1> clearFocus
#   bind .mbar.help <Button-1> clearFocus
#   bind .mbar.labels <Button-1> clearFocus
   set help_tips(.mbar.seq) {File related stuff}
   set help_tips(.mbar.elem) {Select components to show}
   set help_tips(.mbar.edit) {Configuration of the display}
   set help_tips(.mbar.help) {Getting started}
   pack .mbar.seq .mbar.elem .mbar.edit .mbar.labels -side left
   pack .mbar.help -side right
   menu .mbar.seq.menu
   menu .mbar.elem.menu
   menu .mbar.edit.menu
   menu .mbar.help.menu
   menu .mbar.labels.menu
   .mbar.seq.menu add command -label "New Sequence" -command {readNewPsglib "" ""}
   .mbar.seq.menu add cascade -label "Read user sequence" -menu .mbar.seq.menu.menu2
   set pInfo(psglibUMenu) [menu .mbar.seq.menu.menu2 -tearoff 0]
   makePsglibMenu $pInfo(userDir) $pInfo(psglibUMenu)
   .mbar.seq.menu add cascade -label "Read system sequence" -menu .mbar.seq.menu.menu3
   set pInfo(psglibSMenu) [menu .mbar.seq.menu.menu3 -tearoff 0]
   makePsglibMenu $pInfo(homeDir) $pInfo(psglibSMenu)
   .mbar.seq.menu add command -label "Save and make parameters"
   .mbar.seq.menu add command -label Save
   .mbar.seq.menu add command -label Exit -command "saveSelf; exit"
   .mbar.seq.menu add command \
        -label "Exit, abandoning unsaved changes" -command "exit"
   set help_tips(.mbar.seq.menu,1) {Clear all}
   set help_tips(.mbar.seq.menu,2) {Get an existing sequence}
   set help_tips(.mbar.seq.menu,3) {Get a system sequence}
   set help_tips(.mbar.seq.menu,4) {Save current work}
   set help_tips(.mbar.seq.menu,5) {Save current work}
   set help_tips(.mbar.seq.menu,6) {Normal exit}
   set help_tips(.mbar.seq.menu,7) {Abort session}
   makeElemMenu .mbar.elem.menu
   .mbar.edit.menu add command -label "Make a pulse sequence collection" \
       -command "makeCollection"
   .mbar.edit.menu add checkbutton -label "Show Docking Lines" \
       -variable linesOK -command showDockList
   .mbar.edit.menu add checkbutton -label "Tool tip help" \
       -variable use_balloons
   .mbar.edit.menu add command -label "Audio/visual preferences" \
       -command "editColors"
   .mbar.edit.menu add command -label "Print sequence" \
       -command "showOutput"
   .mbar.edit.menu add command -label "Re-layout display" \
       -command "stdDisplay"
   .mbar.edit.menu add cascade -label "Zoom to" \
       -menu .mbar.edit.menu.menu2
   set em [menu .mbar.edit.menu.menu2 -tearoff 0]
   $em add radio -label "500%" -command "zoom 500" \
       -variable scaled -value 500
   $em add radio -label "400%" -command "zoom 400" \
       -variable scaled -value 400
   $em add radio -label "200%" -command "zoom 200" \
       -variable scaled -value 200
   $em add radio -label "150%" -command "zoom 150" \
       -variable scaled -value 150
   $em add radio -label "125%" -command "zoom 125" \
       -variable scaled -value 125
   $em add radio -label "100%" -command "zoom 100" \
       -variable scaled -value 100
   if {$comp == 1} {
     .mbar.edit.menu add checkbutton -label "Show Composite Builder" \
       -variable compositeOK -command switchWins
   }

   .mbar.help.menu add command -label "Getting started" \
       -command "helpMouse basic"
   .mbar.help.menu add command -label "Collection Manager" \
       -command "helpMouse collection"
   if {$comp == 1} {
     .mbar.help.menu add command -label "Composite Builder" \
       -command "helpMouse composite"
   }
   set help_tips(.mbar.edit.menu,1) {Start collection window}
   set help_tips(.mbar.edit.menu,2) {Vertical guidance\nSolid lines have elements docked}
   set help_tips(.mbar.edit.menu,3) {Dont show tool tips}
   set help_tips(.mbar.edit.menu,4) {Window configurator}
   set help_tips(.mbar.edit.menu,5) {Print sequence & attributes}
   set help_tips(.mbar.edit.menu,6) {Reorganize time scale for the sequence}
   set help_tips(.mbar.edit.menu,7) {Zoom in on the canvas}
   set i 8
#   if {$vnmrsendOK == 1} {
#    set help_tips(.mbar.edit.menu,8) {seqfil must be set to spincad sequence name}
#    incr i
#   }
   if {$comp == 1} {
    set help_tips(.mbar.edit.menu,$i) {Make a complex element\n Use middle mouse}
   }
   set help_tips(.mbar.help.menu,1) {Basic info}
   .mbar.labels.menu add command -label Show -command "makeCanvasLabels"
}

proc b1motion {x y} {
  global pInfo
  if {$pInfo(made) == 0} {
     set win [winfo containing $x $y]
     if {$win == ".draw"} {
       selectElem [.list2 curselection]
       set pInfo(xroot) [expr [winfo rootx .draw] - [winfo vrootx .draw]]
       set pInfo(yroot) [expr [winfo rooty .draw] - [winfo vrooty .draw]]
       set newX [expr $x - $pInfo(xroot)]
       set newY [expr $y - $pInfo(yroot)]
       set x1 [.draw canvasx $newX]
       set y1 [.draw canvasy $newY]
       set newX $x1
       set newY $y1
       .draw create rect [expr $newX-10] [expr $newY-20] \
         [expr $newX+10] $newY -tag {currentPS element scl} \
          -outline $pInfo(elemColor)
       select .draw $newX $newY currentPS
       set pInfo(made) 1
     }
  } else {
     set newX [expr $x - $pInfo(xroot)]
     set newY [expr $y - $pInfo(yroot)]
       set x1 [.draw canvasx $newX]
       set y1 [.draw canvasy $newY]
       set newX $x1
       set newY $y1
     drag .draw $newX $newY currentPS
  }
}

proc b1release {x y} {
  global pInfo
  set win [winfo containing $x $y]
  if {($win == ".draw") && ($pInfo(made) == 1)} {
       set newX [expr $x - $pInfo(xroot)]
       set newY [expr $y - $pInfo(yroot)]
       set newX [.draw canvasx $newX]
       set newY [.draw canvasy $newY]
       setElem2 .draw $newX $newY currentPS $pInfo(elemIndex,$pInfo(elIndex))
  } elseif {$pInfo(made) == 1} {
      .draw delete currentPS
  }
  set pInfo(made) 0
  set pInfo(sel) 0
  . configure -cursor $pInfo(cursor)
}

#----- main starts here

set pInfo(cursor) "left_ptr"
. configure -cursor watch
set vnmrsendOK 0
if {$argc >= 1} {
   if {[lindex $argv 0] != "-edit"} {
      vnmrinit \"[lindex $argv 0]\" $env(vnmrsystem)
      set vnmrsendOK 1
   }
}
set hD [file join $env(vnmrsystem) spincad]
set uD [file join $env(vnmruser) spincad]
set bD [file join $env(vnmrsystem) tcl tklibrary vnmr]
set confPath [file join $uD .psgconf]
set pInfo(doComp) 0
set compositeOK 0
set errorFd -1
if {[lsearch $argv "-edit"] != -1} {
   set pInfo(doComp) 1
}
set pInfo(homeDir) $hD
set pInfo(userDir) $uD
set pInfo(seqDir) [file join $uD psglib]
set pInfo(sCVersion) "10/21/08"
set seqName ""
set hostname [exec uname -n]
# wm geom . 100x100+1500+1500
# catch {source [file join $bD splash.tcl]}
set pInfo(wins) 0
set pInfo(audio) 0
set pInfo(composite) 0
set pInfo(collection) 0
set pInfo(labels) off
set pInfo(collectionInst) 1
set wids(id) {}
set trashcan @[file join $hD bitmaps trash.l.xbm]
set lineFill @[file join $hD bitmaps dash.xbm]
debugger
source [file join $bD psgcolor.tcl]
if {[file exists $uD] != 1} {
   file mkdir $uD
}
if {[file exists $confPath] != 1} {
   catch {file copy [file join $hD .psgconf] [file join $uD .psgconf]}
}
catch {source $confPath}
#wm geom . 100x100+1500+1500
wm withdraw .
update idletasks
catch {source [file join $bD splash.tcl]}
catch {source [file join $env(vnmrsystem) tcl tklibrary balloon.tcl]}
catch {source [file join $env(vnmrsystem) tcl tklibrary getopt.tcl]}
# parray pInfo
option add *background $pInfo(backColor)
option add *Balloon*delay 750
option add *Balloon*font courb10
option add *Balloon*background yellow

init_balloons			;# For help
enable_balloon Radiobutton


frame .mbar -relief raised -bd 2
makemenu $pInfo(doComp)
frame .psg -borderwidth 10
frame .info -highlightthickness 0 -bd 5
frame .comp -highlightthickness 0 -bd 5
makePsgFrame .info
makeListFrame .info
makeInfoFrame .info
source [file join $bD docker.tbc]
canvas .draw -highlightthickness 0 -relief sunken -borderwidth 3 \
   -takefocus 0
docker .info
pack .info -in .psg -side left -fill y -expand false -anchor nw
scrolls
pack .draw -in .psg -side right -fill both -expand true -anchor ne
pack .mbar -side top -fill x
pack .psg -side top -expand true -fill both
# debugger
wm maxsize . [winfo screenwidth .] [winfo screenheight .]
wm title . "SpinCAD Version $pInfo(sCVersion) for $hostname"
wm iconbitmap . @[file join $hD bitmaps spin.xbm]
wm minsize . 10 10
wm deiconify .
catch {raise .splash}
if {[info exists pInfo(rootGeom)] != 1} {
   set pInfo(rootGeom) [wm geometry .]
}
if {[regexp {(.*):.+} [winfo screen .] match name] == 1} {
   if {($name == "") || ($name == [exec uname -n])} {
      set pInfo(audio) 1
   }
}
wm geometry . $pInfo(rootGeom)
tkwait visibility .draw
bind .draw <Configure> {setChans 1}
wm protocol . WM_DELETE_WINDOW {
   saveSelf
   exit
}
catch {source $confPath}
source [file join $bD popup.tcl]
fillList
update idletasks
set pInfo(fg) [.psg2 cget -fg]
readNewPsglib $seqName $pInfo(seqDir)
update idletasks
if {$pInfo(doComp) == 1} {
   source [file join $bD composite.tcl]
}
source [file join $bD collection.tcl]
source [file join $bD psghelp.tcl]
if {$pInfo(doComp) == 1} {
   makeCompFrame .comp
}
updateExitMenu

set pInfo(sel) 0
set pInfo(made) 0
enable_balloon .list2 "%W index @%x,%y"
bind .list2 <B1-Leave> {break}
bind .list2 <Button-1> {
      set pInfo(sel) 1
      set pInfo(made) 0
      set pInfo(cursor) [. cget -cursor]
      . configure -cursor hand2
}
bind .list2 <ButtonRelease-1> {b1release %X %Y}
bind .list2 <B1-Motion> {b1motion %X %Y}
. configure -cursor $pInfo(cursor)
bind . <Unmap> {wm iconname . $seqName}
bind Entry <<Paste>> {
    global tcl_platform
    catch {
        catch {
           %W delete sel.first sel.last
        }
        %W insert insert [selection get -displayof %W -selection CLIPBOARD]
        tkEntrySeeInsert %W
    }
}
if {[file exists [file join $uD psglib]] != 1} {
   file mkdir [file join $uD psglib]
}
if {[file exists [file join $uD info]] != 1} {
   file mkdir [file join $uD info]
}
