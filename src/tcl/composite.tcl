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

set pInfo(cwins) 0
set pInfo(pwins) 0
set pElem(compDir) $pElem(elem)

proc initCompInfo {} {
   global compInfo pInfo
   set compInfo(elems) {}
   set compInfo(elem) 0
   set compInfo(cur) 0
   set compInfo(cur,id) {}
   set compInfo(compName) ""
   set compInfo(compLabel) ""
   set compInfo(mode) 0
   set compInfo(shape) rect
   set compInfo(stipple) ""
   set compInfo(polar) 1
   set compInfo(dockPt) ""
   set compInfo(dock) 0
   set compInfo(dockId) 0
   set compInfo(dockLabel) ""
   set compInfo(front,dock) front
   set compInfo(front,dockId) 0
   set compInfo(front,label) ""
   set compInfo(mid,dock) mid
   set compInfo(mid,dockId) 0
   set compInfo(mid,label) ""
   set compInfo(end,dock) end
   set compInfo(end,dockId) 0
   set compInfo(end,label) ""
   selectCompMode
   trace variable compInfo(cur) w updateMenu
}

proc exitComp {} {
   global compInfo pInfo
   trace vdelete compInfo(cur) w updateMenu
#   set i 0
#   while {$i < $pInfo(pwins)} {
#      incr i
#      grid remove .cp$i.lab .cp$i.ent
#   }
   .draw delete compDockLines
   while {$compInfo(elem) > 0} {
      addComptoMenu [lindex $compInfo(elem) 0]
   }
}

proc newComp {} {
   global compInfo pElem
   if {$compInfo(compName) != ""} {
      saveComp
   }
   currentComposites
   set compInfo(compLabel) "New pulse element"
}

proc currentComposites {} {
   global compInfo pElem
   set compInfo(compositeNames) {}
   set compInfo(compositeLabels) {}
   set compInfo(compositeNum) 0
# puts "get current composites from $pElem(groupPath,$pElem(compDir))/elements"
   if {[file exists $pElem(groupPath,$pElem(compDir))/elements] == 1} {
      set names {}
      set labels {}
      set fd [open $pElem(groupPath,$pElem(compDir))/elements r]
      while {[gets $fd line] >= 0} {
         if {[regexp {^#} $line] == 1} {
# puts  "A comment"
           continue
         }
         if {[regexp "^\[ \t]*$" $line] == 1} {
# puts  "A blank line"
           continue
         }
# puts "$line"
        if {[regexp {([a-zA-Z0-9_]+) (.*)} $line match id label]  == 1} {
# puts "id:    $id"
# puts "label: $label"
           lappend names $id
           lappend labels $label
           incr compInfo(compositeNum)
         }
      }
      close $fd
      set compInfo(compositeNames) $names
      set compInfo(compositeLabels) $labels
   }
#   set compInfo(compName) "userComp$i"
# puts "done current composites from $pElem(groupPath,$pElem(compDir))/elements"
# puts "$compInfo(compositeNum) labels are $compInfo(compositeLabels)"
}

proc makeCompositeName {} {
   global pElem compInfo
   set index [lsearch $compInfo(compositeLabels) $compInfo(compLabel)]
   set compInfo(nameExists) 0
   if {$index != -1} {
      set compInfo(compName) [lindex $compInfo(compositeNames) $index]
      set compInfo(nameExists) 1
   } elseif {[regexp {([a-zA-Z0-9_]+).*} $compInfo(compLabel) match name] == 1} {
      set tstName $name
      set rev 1
      while {[lsearch $compInfo(compositeNames) $tstName] != -1} {
         incr rev
              set tstName $name$rev
      }
      set compInfo(compName) $tstName
     
# puts "name is $compInfo(compName)"
   } else {
      set compInfo(compName) ""
# puts "nulled name is $compInfo(compName)"
   }
}

proc resetComposites {} {
   global pElem compInfo
#  puts "resetCollection"
#  puts "$compInfo(compositeNum) labels are $compInfo(compositeLabels)"
   set dir $pElem(groupPath,$pElem(compDir))
   set fd [open $dir/elements w 0666]
   for {set i 0} {$i < $compInfo(compositeNum)} {incr i} {
      set lab [string trim [lindex $compInfo(compositeLabels) $i]]
      if {$lab !=  ""} {
         puts $fd "[lindex $compInfo(compositeNames) $i] $lab"
#  puts "[lindex $compInfo(compositeNames) $i] $lab"
      } else {
#  puts "DELETE [lindex $compInfo(compositeNames) $i]"
         exec rm -f $dir/[lindex $compInfo(compositeNames) $i]
      }
   }
#  puts "resetComposite done"
   return $fd
}

proc putDock {fd pt} {
   global compInfo
   set did [lsearch $compInfo(goodList) $compInfo($pt,dockId)]
   if {$did == -1} {
      set did 0
   }
   incr did
   putCompAttr $fd ${pt}Id $did
   if {$compInfo($pt,dock) == 0} {
         set compInfo($pt,dock) $pt
   }
   putCompAttr $fd ${pt}Dock "$compInfo($pt,dock)"
}

proc saveComp {} {
   global pElem compInfo
# puts "check file $pElem(groupPath,$pElem(compDir))/elements"
# parray compInfo
   foreach cid $compInfo(elems) {
      .draw itemconfigure T$cid -width 1.0
      .draw bind T$cid <Button-1>  {}
   }
   if {[info exists compInfo(compositeNum)] != 1} {
      return
   }
   set fd [resetComposites]
   makeCompositeName
   if {$compInfo(compName) == ""} {
      close $fd
      return
   }
   if {[info exists compInfo(goodList)] != 1} {
      close $fd
      return
   }
   if {[llength $compInfo(goodList)] < 1} {
      close $fd
      return
   }
   if {$compInfo(nameExists) == 0} {
      puts $fd "$compInfo(compName) $compInfo(compLabel)"
   }
   close $fd
   set fd [open $pElem(groupPath,$pElem(compDir))/$compInfo(compName) w 0666]
      puts $fd "<composite>"
      putCompAttr $fd label "$compInfo(compLabel)"
      putCompAttr $fd style "$compInfo(style)"
#      putCompAttr $fd name "$compInfo(compName)"
      putCompAttr $fd shape "$compInfo(shape)"
      putCompAttr $fd stipple "$compInfo(stipple)"
      putCompAttr $fd polar "$compInfo(polar)"
      putDock $fd front
      putDock $fd mid
      putDock $fd end
      if {$compInfo(parallel) == 1} {
         putCompAttr $fd parallel yes
      } else {
         putCompAttr $fd parallel no
      }
      for {set i 0} {$i < $compInfo(attrNum)} {incr i} {
         set index [expr $i+1]
         set index2 [lindex $compInfo(attrLabel) $i]
         putCompAttr $fd label$index  "$compInfo($index2)"
      }
      for {set i 0} {$i < $compInfo(attrNum)} {incr i} {
         set index [expr $i+1]
         set index2 [lindex $compInfo(attrEntry) $i]
         putCompAttr $fd entry$index  "$compInfo($index2)"
      }
      for {set i 0} {$i < $compInfo(attrNum)} {incr i} {
         set index [expr $i+1]
         set index2 [lindex $compInfo(attrValue) $i]
# puts "comp attr$index: $compInfo($index2)"
         regsub SHARED $compInfo($index2) "" val
# puts "edited comp attr$index: $val"
         putCompAttr $fd attr$index "$val"
      }
      puts $fd "</composite>"
      set num [llength $compInfo(goodList)]
      for {set id 1} {$id <= $num} {incr id} {
         set cid [lindex $compInfo(goodList) [expr $id-1]]
#         puts "name $compInfo($cid,name)"
#         puts "elemLabel $compInfo($cid,elemLabel)"
#         puts "label $compInfo($cid,label)"
         puts $fd "<$compInfo($cid,name)>"
         for {set i 1} {$i <= $compInfo($cid,numAttr)} {incr i} {
#            puts "N $compInfo($cid,N$i)"
#            puts "B $compInfo($cid,B$i)"
#            puts "L $compInfo($cid,L$i)"
#            puts "V $compInfo($cid,V$i)"
#            puts "S $compInfo($cid,S$i)"
#            puts "cB $compInfo($cid,cB$i)"
#            puts "cS $compInfo($cid,cS$i)"
#            puts "cL $compInfo($cid,cL$i)"
#            puts "cV $compInfo($cid,cV$i)"
            if {$compInfo($cid,cS$i) == "inherit"} {
# puts "inherit attr: compInfo($cid,cV$i) = $compInfo($cid,cV$i)"
               if {[regsub SHARED $compInfo($cid,cV$i) "" ignore] > 0} {
                 putCompAttr $fd \
                  "$compInfo($cid,N$i)" "NOT USED"
               } else {
                 putCompAttr $fd \
                  "$compInfo($cid,N$i)" "\[inherit attr$compInfo($cid,cB$i)\]"
               }
            } else {
               putCompAttr $fd "$compInfo($cid,N$i)" "$compInfo($cid,cV$i)"
            }
         }
         if {$id == 1} {
            putCompAttr $fd relId 1
            putCompAttr $fd relIdAt 0
            putCompAttr $fd id {[inherit id]}
            putCompAttr $fd idAt {[inherit idAt]}
            putCompAttr $fd dock {[inherit dock]}
            putCompAttr $fd dockAt {[inherit dockAt]}
            putCompAttr $fd chanId {[inherit chanId]}
         } else {
            set idAt [lindex $compInfo(goodIdAt) [expr $id-1]]
            putCompAttr $fd relId $id
            putCompAttr $fd dock $compInfo($cid,dock)
            putCompAttr $fd relIdAt $idAt
            putCompAttr $fd dockAt $compInfo($cid,dockAt)
            putCompAttr $fd chanId {[inherit chanId]}
         }
         puts $fd "</$compInfo($cid,name)>"
      }
   close $fd
}

proc updateOneCompDock {dockPt} {
   global dockList psgInfo compInfo
# puts "updateOneCompDock $dockPt"
   if {$compInfo($dockPt,dock) == 0} {
#  puts "compInfo($dockPt,dock) == 0"
      return
   }
   set id $compInfo($dockPt,dockId)

   if {[info exists dockList($id,$compInfo($dockPt,dock))] != 1} {
#  puts "dockList($id,$compInfo($dockPt,dock)) does not exist"
      set compInfo($dockPt,dock) ""
      return
   }
   set i $dockList($id,$compInfo($dockPt,dock))
   set x $dockList($i)
   set ht [expr $dockList(ch$psgInfo($id,chanId)) + $psgInfo($id,base) + 10]
   .draw create line $x $ht $x $psgInfo(ybot) \
            -tag "compDockLines dock$dockPt" -fill red -width 2 -arrow first
}

proc updateCompDock {} {
   .draw delete compDockLines
   updateOneCompDock front
   updateOneCompDock mid
   updateOneCompDock end
}

proc updateMenu {var index op} {
   global compInfo pInfo
   set cid $compInfo(cur)
   set compInfo(label) $compInfo($cid,elemLabel)
   makeCompWindows $cid
   .draw itemconfigure T$compInfo(cur,id) -outline $pInfo(elemColor)
   set compInfo(cur,id) $compInfo($cid,psgInfoId)
   .draw itemconfigure T$compInfo(cur,id) -outline $pInfo(selectColor)
#   if {$compInfo(front,dockId) == $compInfo(cur,id)} {
#      set compInfo(dock) $compInfo(front,dock)
#   } elseif {$compInfo(mid,dockId) == $compInfo(cur,id)} {
#      set compInfo(dock) $compInfo(mid,dock)
#   } elseif {$compInfo(end,dockId) == $compInfo(cur,id)} {
#      set compInfo(dock) $compInfo(end,dock)
#   } else {
#      set compInfo(dock) 0
#   }
#   updateCompDock
}

proc selectCompMode {} {
   global compInfo pInfo
   if {$compInfo(mode) == 1} {
      .composite5 configure -state disabled
      .composite5b configure -state disabled
      .compMidR1 configure -state disabled
      .compMidR2 configure -state disabled
      .compMidR3 configure -state disabled
      .composite5a configure -fg gray
      .composite9 configure -fg gray
      set compInfo(editLabel) "Show component   "
      set i 0
      while {$i < $pInfo(pwins)} {
         incr i
         grid remove .cp$i.lab .cp$i.ent
      }
      set w .composite8
      for {set i 1}  {$i <= $pInfo(cwins)} {incr i} {
         $w.cf$i.ent1 configure -state disabled
         $w.cf$i.ent2 configure -state disabled
      }
   } else {
      .composite5 configure -state normal
      .composite5b configure -state normal
      .compMidR1 configure -state normal
      .compMidR2 configure -state normal
      .compMidR3 configure -state normal
      .composite5a configure -fg $pInfo(fg)
      .composite9 configure -fg $pInfo(fg)
      set compInfo(editLabel) "Edit component   "
      set w .composite8
      for {set i 1}  {$i <= $pInfo(cwins)} {incr i} {
         $w.cf$i.ent1 configure -state normal
         $w.cf$i.ent2 configure -state normal
      }
      makeCompPreviewWindow
   }
}

proc pickSymbol {} {
      .composite1a configure -foreground [.composite5 cget -disabledforeground]
      .composite2a configure -state disabled
      .composite1a configure -foreground [.composite5 cget -foreground]
      .composite2a configure -state normal
}

proc showAttr {item} {
   global prevInfo compInfo
   set pane [lindex $prevInfo($item,pane) $prevInfo($item,cur)]
   set entryNum [lindex $prevInfo($item,pane) $prevInfo($item,cur)]
   if {$pane != $compInfo(cur)} {
      set compInfo(cur) $pane
   }
   incr prevInfo($item,cur)
   if {$prevInfo($item,cur) >= [llength $prevInfo($item,pane)]} {
      set prevInfo($item,cur) 0
   }
}

proc makeCompPreviewWindow {} {
   global pInfo compInfo prevInfo
   if {$compInfo(mode) == 1} {
      return
   }
   set attrList {}
   set compInfo(attrNum) 0
   set compInfo(attrValue) {}
   set compInfo(attrIndex) {}
   set compInfo(attrLabel) {}
   set compInfo(attrEntry) {}
   catch {unset prevInfo}
   foreach cid $compInfo(elems) {
      for {set i 1} {$i <= $compInfo($cid,numAttr)} {incr i} {
         set compInfo($cid,cB$i) 0
         set lab [string trim $compInfo($cid,cL$i)]
         set compInfo($cid,cS$i) inherit
         if {$lab == ""} {
            set compInfo($cid,cS$i) fixed
         } else {
# puts "add user element for label '$lab'"
}
         if {$compInfo($cid,cS$i) == "inherit"} {
            set index [lsearch $attrList $compInfo($cid,cL$i)]
            if {$index == -1} {
# puts "adding $compInfo($cid,cL$i) to attr list $attrList"
               lappend attrList $compInfo($cid,cL$i)
               incr compInfo(attrNum)
               lappend compInfo(attrValue) $cid,cV$i
               lappend compInfo(attrIndex) $cid,cB$i
               lappend compInfo(attrLabel) $cid,cL$i
               lappend compInfo(attrEntry) $cid,cLE$i
               set index [lsearch $attrList $compInfo($cid,cL$i)]
            }
            set compInfo($cid,cB$i) [expr $index+1]
            lappend prevInfo([expr $index+1],pane) $cid
            lappend prevInfo([expr $index+1],entry) $i
            set prevInfo([expr $index+1],cur) 1
         }
      }
   }
   set num $compInfo(attrNum)
   for {set i 1} {$i <= $num} {incr i} {
      set index [lsearch $prevInfo($i,pane) $compInfo(cur)]
      if {$index == -1} {
         set prevInfo($i,cur) 0
      } else {
         set prevInfo($i,cur) $index
      }
   }
   if {$pInfo(pwins) < $num} {
      while {$pInfo(pwins) < $num} {
         incr pInfo(pwins)
         set i $pInfo(pwins)
         frame .cp$i 
         button .cp$i.lab -width 18 -anchor w -padx 0 -pady 0 \
                   -relief flat -highlightthickness 0
         entry .cp$i.ent -width 18 -relief sunken -highlightthickness 0
         grid .cp$i.lab -in .cp$i -row $i -column 0 -sticky nw
         grid .cp$i.ent -in .cp$i -row $i -column 1 -sticky ne
         .composite10 window create end -window .cp$i
      }
   }
   if {$pInfo(pwins) > $num} {
      set i $num
      while {$i < $pInfo(pwins)} {
         incr i
         grid remove .cp$i.lab .cp$i.ent
      }
   }
   for {set i 1} {$i <= $num} {incr i} {
      grid .cp$i.lab
      grid .cp$i.ent
   }
   for {set num 0} {$num < $compInfo(attrNum)} {incr num} {
      set i [expr $num+1]
      .cp$i.lab configure \
           -command "showAttr $i" \
           -textvariable compInfo([lindex $compInfo(attrLabel) $num])
      .cp$i.ent configure \
           -textvariable compInfo([lindex $compInfo(attrValue) $num])
   }
# puts "labels $compInfo(attrValue)"
}


proc makeCompWindows {cid} {
   global pInfo compInfo
   set w .composite8
   $w configure -state normal
   if {$pInfo(cwins) < $compInfo($cid,numWins)} {
      while {$pInfo(cwins) < $compInfo($cid,numWins)} {
         incr pInfo(cwins)
         set i $pInfo(cwins)
         frame $w.cf$i 
#         radiobutton .cf$i.lab -width 18 -anchor w -variable pInfo(curElem)\
               -value $i -command "selectCompAttr $i"
         label $w.cf$i.lab -width 18
         entry $w.cf$i.ent1 -width 18 -relief sunken -highlightthickness 0
         entry $w.cf$i.ent2 -width 18 -relief sunken -highlightthickness 0
          grid $w.cf$i.lab -in $w.cf$i -row 0 -column 0  -columnspan 2 -sticky nw
          grid $w.cf$i.ent1 -in $w.cf$i -row 1 -column 0 -sticky nw
          grid $w.cf$i.ent2 -in $w.cf$i -row 1 -column 1 -sticky ne
         .composite8 window create end -window $w.cf$i -pady 5
      }
   }
# puts "new windows $compInfo($cid,numWins), old $pInfo(cwins)"
   if {$pInfo(cwins) > $compInfo($cid,numWins)} {
      set i $compInfo($cid,numWins)
      while {$i < $pInfo(cwins)} {
         incr i
#         grid remove .cf$i.lab
#         grid remove .cf$i.lab .cf$i.ent
# puts "delete windowID .composite8.cf$i"
          .composite8 delete $w.cf$i
      }
      set pInfo(cwins) $compInfo($cid,numWins)
   }
   if {$compInfo(mode) == 1} {
      set state disabled
   } else {
      set state normal
   }
   set ii 0
   for {set i 1} {$i <= $compInfo($cid,numWins)} {incr i} {
      grid $w.cf$i.lab
      grid $w.cf$i.ent1
      grid $w.cf$i.ent2
#      .cf$i.lab configure -text \
#           [format "%-18s %s" $compInfo($cid,L$i) $compInfo($cid,V$i)]
      incr ii
      while {$compInfo($cid,S$ii) == "fixed"} {
         incr ii
      } 
      $w.cf$i.lab configure -text [format "%-18s" $compInfo($cid,L$ii)]
      $w.cf$i.ent1 configure -textvariable compInfo($cid,cL$ii) \
            -state $state
      $w.cf$i.ent2 configure -textvariable compInfo($cid,cV$ii) \
            -state $state
      bind $w.cf$i.ent1 <Return> "makeCompPreviewWindow"
      bind $w.cf$i.ent1 <FocusIn> "makeCompPreviewWindow"
      bind $w.cf$i.ent2 <Return> "makeCompPreviewWindow"
      bind $w.cf$i.ent2 <FocusIn> "makeCompPreviewWindow"
#      bind .cf$i.ent <Return> "updateMeta"
#      bind .cf$i.ent <FocusOut> "updateMeta"
#      bind .cf$i.ent <ButtonRelease-1> "setListSelection $i"
   }
   set pInfo(curElem) 0
   $w configure -state disabled
#   .composite11 configure -state disabled
#   .composite16 configure -state disabled
}

proc updateGoodList {} {
   global compInfo

   set list(num) 0
   set oldList $compInfo(elems)
   set compInfo(goodIdAt) {}
   set compInfo(goodList) {}
   if {$compInfo(elem) == 0} {
      return
   }
# puts "updateGood $compInfo(elems)"
   foreach cid $compInfo(elems) {
      if {[lsearch $compInfo(elems) $compInfo($cid,idAt)] == -1} {
         incr list(num)
         set list($list(num)) $cid
      }
   }
# puts "found $list(num) lists"
# puts "original list $oldList"
   for {set i 1} {$i <= $list(num)} {incr i} {
      set oldList [ldelete $oldList $list($i)]
   }
# puts "list after removing initiators: $oldList"
   while {[llength $oldList] > 0} {
      for {set i 1} {$i <= $list(num)} {incr i} {
# puts "new iteration: old list $oldList"
# puts "new iteration: new list $newList"
         set newList {}
         foreach cid $list($i) {
            set testId $compInfo($cid,id)
            foreach j $oldList {
              if {$testId == $compInfo($j,idAt)} {
                 lappend newList $j
              }
            }
# puts "test ID is $testIds
         }
         foreach j $newList {
            lappend list($i) $j
# puts "add $j to list $list(num)"
            set oldList [ldelete $oldList $j]
         }
      }
#    for {set i 1} {$i <= $list(num)} {incr i} {
#       puts "list $i is : $list($i)"
#    }
   }
# puts "Final lists ========="
   set num 0
   for {set i 1} {$i <= $list(num)} {incr i} {
      if {[llength $list($i)] > $num} {
         set num [llength $list($i)]
         set compInfo(goodList) $list($i)
      }
#       puts "list $i is : $list($i)"
   }
   foreach cid $compInfo(elems) {
      if {[lsearch $compInfo(goodList) $cid] == -1} {
         set wid 7.0
      } else {
         set wid 3.0
      }
      .draw itemconfigure T$cid -width $wid
   }
#    puts "good list is $compInfo(goodList)"
   foreach cid $compInfo(goodList) {
      set idAt [lsearch $compInfo(goodList) $compInfo($cid,idAt)]
      lappend compInfo(goodIdAt) [expr $idAt+1]
   }
#    puts "idAt list is $compInfo(goodIdAt)"
}

proc addComptoMenu {id} {
   global compInfo psgInfo pInfo
   set found [lsearch $compInfo(elems) $id]
   if {$found == -1} {
      .draw itemconfigure T$compInfo(cur,id) -outline $pInfo(elemColor)
      set compInfo(cur,id) $id

# puts "called addComptoMenu with id $id"
      lappend compInfo(elems) $id
      set found $compInfo(elem)
      incr compInfo(elem)
      set cid $id
      set compInfo($cid,psgInfoId) $id
      .draw bind T$id <Button-1>  "set compInfo(cur) $cid"
      .composite7.menu add radio -variable compInfo(cur) -value $cid
      set compInfo($cid,name) $psgInfo($id,name)
      set compInfo($cid,elemLabel) $psgInfo($id,elemLabel)
      set compInfo($cid,label) $psgInfo($id,label)
      set compInfo($cid,numAttr) $psgInfo($id,numAttr)
      set compInfo($cid,numWins) $psgInfo($id,numWins)
      for {set i 1} {$i <= $psgInfo($id,numAttr)} {incr i} {
         set compInfo($cid,N$i) $psgInfo($id,N$i)
         set compInfo($cid,B$i) $psgInfo($id,B$i)
         set compInfo($cid,L$i) $psgInfo($id,L$i)
         if {[info exists psgInfo($id,LE$i)] == 1} {
            set compInfo($cid,LE$i) $psgInfo($id,LE$i)
         } else {
            set compInfo($cid,LE$i) ENTRY
         }
         set compInfo($cid,V$i) $psgInfo($id,V$i)
         set compInfo($cid,S$i) $psgInfo($id,S$i)
         set compInfo($cid,cS$i) inherit
         set compInfo($cid,cL$i) $compInfo($cid,L$i)
         set compInfo($cid,cLE$i) $compInfo($cid,LE$i)
         set compInfo($cid,cV$i) $compInfo($cid,V$i)
      }
      set compInfo($cid,id) $psgInfo($id,id)
      set compInfo($cid,idAt) $psgInfo($id,idAt)
      set compInfo($cid,chanId) $psgInfo($id,chanId)
      set compInfo($cid,dock) $psgInfo($id,dock)
      set compInfo($cid,dockAt) $psgInfo($id,dockAt)
      set compInfo(cur) $cid
      .composite7.menu entryconfigure $found -label $psgInfo($id,label)
   } else {
      set compInfo(elems) [lreplace $compInfo(elems) $found $found]
      incr compInfo(elem) -1
      set cid $id
      if {$compInfo(elem) == 0} {
         .draw itemconfigure T$compInfo(cur,id) -outline  $pInfo(elemColor)
         set compInfo(label) ""
         set compInfo(dock) 0
         set compInfo(dockLabel) ""
         set compInfo($cid,numAttr) 0
         set compInfo($cid,numWins) 0
         makeCompWindows $cid
      } elseif {$compInfo(cur) == $id} {
         set compInfo(cur) [lindex $compInfo(elems) 0]
      }
      unset compInfo($cid,psgInfoId)
      .draw bind T$id <Button-1>  {}
      .draw itemconfigure T$id -width 1.0
      .composite7.menu delete $found
      unset compInfo($cid,name)
      unset compInfo($cid,elemLabel)
      unset compInfo($cid,label)
      for {set i 1} {$i <= $psgInfo($id,numAttr)} {incr i} {
         unset compInfo($cid,N$i)
         unset compInfo($cid,B$i)
         unset compInfo($cid,L$i)
         catch [unset compInfo($cid,LE$i)]
         unset compInfo($cid,V$i)
         unset compInfo($cid,S$i)
         unset compInfo($cid,cS$i)
         unset compInfo($cid,cL$i)
         unset compInfo($cid,cLE$i)
         unset compInfo($cid,cV$i)
      }
      unset compInfo($cid,id)
      unset compInfo($cid,idAt)
      unset compInfo($cid,chanId)
      unset compInfo($cid,numWins)
      unset compInfo($cid,numAttr)
   }
   makeCompPreviewWindow
   updateGoodList
}

proc setMidPoint {} {
   global compInfo
# puts "call setMidPoint"
   if {$compInfo(cur,id) == ""} {
      set compInfo(dock) 0
   } else {
#  puts "set $compInfo(dockPt) dockId to $compInfo(cur,id)"
      set compInfo($compInfo(dockPt),dockId) $compInfo(cur,id)
      set compInfo($compInfo(dockPt),dock) $compInfo(dock)
      set compInfo($compInfo(dockPt),label) $compInfo(label)
      set compInfo(dockLabel) $compInfo(label)
   }
   updateCompDock
   .draw itemconfigure compDockLines -width 2.0
   .draw itemconfigure dock$compInfo(dockPt) -width 4.0

#  puts "midpoint called with $compInfo(dock) of id $compInfo(cur,id)"
}

proc setDockPoint {} {
   global compInfo
   set compInfo(dock) $compInfo($compInfo(dockPt),dock)
#   set compInfo(dockId) $compInfo($compInfo(dockPt),dockId)
   set compInfo(dockLabel) $compInfo($compInfo(dockPt),label)
   if {$compInfo($compInfo(dockPt),dockId) != 0} {
      set compInfo(cur) $compInfo($compInfo(dockPt),dockId)
   }
   updateCompDock
   .draw itemconfigure compDockLines -width 2.0
   .draw itemconfigure dock$compInfo(dockPt) -width 4.0
}

proc makeCompFrame {win} {
   global compInfo pInfo
   frame $win.fr0
   frame $win.fr1
   frame $win.fr2
   frame $win.fr3
   frame $win.fr2a
   frame $win.fr2b
   frame $win.fr2c
   frame $win.fr2d
   button .composite0 -text "Create New Pulse Element" -command newComp
   label .composite1 -text "Composite name:  "
   entry .composite2 -width 20 -relief sunken -textvariable compInfo(compLabel)
   label .composite5a -text "Symbol "
   menubutton .composite5 -textvariable compInfo(style) \
             -menu .composite5.menu -width 15 \
            -relief raised -bd 2 -highlightthickness 2 -anchor c
   menu .composite5.menu -tearoff 0
   set compInfo(style) normal
   .composite5.menu add radio -variable compInfo(style) \
            -value pulse -label pulse
   .composite5.menu add radio -variable compInfo(style)  \
           -value delay -label delay
   .composite5.menu add radio -variable compInfo(style)  \
           -value gate -label gate
   .composite5.menu add radio -variable compInfo(style)  \
           -value shortdelay -label "short delay"
   .composite5.menu add radio -variable compInfo(style)  \
           -value box -label "calculation"
   .composite5.menu add radio -variable compInfo(style)  \
           -value if -label "if"
   .composite5.menu add radio -variable compInfo(style)  \
           -value normal -label "normal"

   menubutton .composite5s -textvariable compInfo(shape) \
             -menu .composite5s.menu -width 15 \
            -relief raised -bd 2 -highlightthickness 2 -anchor c

   menu .composite5s.menu -tearoff 0
   set compInfo(shape) rect
   .composite5s.menu add radio -variable compInfo(shape) \
            -value rect -label rectangle
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value arc -label "half sine"
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value ellipse -label ellipse
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value triangle -label "acquire"
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value diamond -label "echo"
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value marker -label "marker"
   .composite5s.menu add radio -variable compInfo(shape)  \
           -value trap -label "trapezoid"
#   .composite5s.menu add radio -variable compInfo(shape)  \
           -value if -label "if"
#   .composite5s.menu add radio -variable compInfo(shape)  \
           -value normal -label "normal"

   checkbutton .composite5b -text "Allow Parallel Events" \
          -variable compInfo(parallel) -indicatoron 1 -highlightthickness 0

   radiobutton .compMidS1  -text "Set Front Pt" \
       -variable compInfo(dockPt) -value front \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDockPoint -width 13
   radiobutton .compMidS2  -text "Set Middle Pt" \
       -variable compInfo(dockPt) -value mid \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDockPoint -width 13
   radiobutton .compMidS3  -text "Set End Pt" \
       -variable compInfo(dockPt) -value end \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDockPoint -width 13

   radiobutton .compMidR1  -text Front -variable compInfo(dock) -value front \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setMidPoint -width 6
   radiobutton .compMidR2  -text Middle -variable compInfo(dock) -value mid \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setMidPoint -width 6
   radiobutton .compMidR3  -text End -variable compInfo(dock) -value end \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setMidPoint -width 6

   label .compMidDock2 -text " of " -anchor w
   label .compMidDock3 -textvariable compInfo(dockLabel) -anchor w
   pack .compMidS1 .compMidS2 .compMidS3 -in $win.fr2c -side left -anchor w
   pack .compMidR1 .compMidR2 .compMidR3 .compMidDock2 .compMidDock3 \
       -in $win.fr2d -side left -anchor w
   pack .composite0 -in $win.fr0 -side left -anchor nw -pady 10
   pack .composite1 .composite2 -in $win.fr1 -side left -anchor nw -pady 10
   pack .composite5a .composite5 .composite5s -in $win.fr2b \
          -side left -anchor w
   label .composite6a -text "  "
   pack $win.fr2b .composite5b $win.fr2c $win.fr2d .composite6a -in $win.fr2 \
          -side top -anchor w
   label .composite6 -textvariable compInfo(editLabel)
   set compInfo(editLabel) "Edit component   "
   menubutton .composite7 -textvariable compInfo(label) -indicatoron 1 \
             -menu .composite7.menu -width 15 \
            -relief raised -bd 2 -highlightthickness 2 -anchor c
   menu .composite7.menu -tearoff 0

   pack .composite6 .composite7 -in $win.fr3 -side left -anchor nw
   frame $win.fr4 -relief flat -borderwidth 2
   text .composite8 -relief raised -borderwidth 2 -state disabled \
         -highlightthickness 0 \
         -yscrollcommand ".scroll3 set" -width 40 -height 10
   scrollbar .scroll3 -command ".composite8 yview"
   pack .scroll3 -side right -anchor w -in $win.fr4 -fill y -expand 1
   pack .composite8 -in $win.fr4 -side right -anchor e -fill y -expand 1
   pack $win.fr0 $win.fr1 $win.fr2 $win.fr3 -side top -anchor w
   pack $win.fr4 -side top -anchor w -expand 1 -fill y

   frame $win.fr4a -relief flat -borderwidth 2
   frame $win.fr4b -relief flat -borderwidth 2
   frame $win.fr4c -relief flat -borderwidth 2
   frame $win.fr4d -relief flat -borderwidth 2
   pack $win.fr4a -side top -anchor w

   frame $win.fr5 -relief flat -borderwidth 2
   label .composite9 -text "Composite Preview"
   set pInfo(fg) [.composite9 cget -fg]
   text .composite10 -relief raised -borderwidth 2 -state disabled\
         -yscrollcommand ".scroll4 set" -width 40 -height 5
   scrollbar .scroll4 -command ".composite10 yview"
   pack .composite9 -side top -in $win.fr5
   pack .scroll4 -side right -anchor w -in $win.fr5 -fill y -expand 1
   pack .composite10 -in $win.fr5 -side right -anchor e -fill y  -expand 1
   pack $win.fr5 -side top -anchor w -fill both -expand 1
}
