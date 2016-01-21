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

proc initCollectionInfo {} {
   global compInfo compositeOK
   if {$compositeOK == 1} {
      set compositeOK 0
      switchWins
   }
   set compInfo(elems) {}
   set compInfo(elem) 0
   set compInfo(hide) 0
   set compInfo(compName) ""
   set compInfo(compLabel) ""
   set compInfo(errorMsg) "\n"
   set compInfo(collMsg) ""
}

proc putCompAttr {fd attr value} {
   puts $fd "  <$attr>"
   puts $fd "    $value"
   puts $fd "  </$attr>"
}

proc currentCollections {} {
   global compInfo pElem
   set compInfo(collectionNames) {}
   set compInfo(collectionLabels) {}
   set compInfo(collectionNum) 0
   if {[file exists $pElem(groupPath,$pElem(user))] != 1} {
      file mkdir $pElem(groupPath,$pElem(user))
   }
# puts "get current collections from $pElem(groupPath,$pElem(user))/elements"
   if {[file exists $pElem(groupPath,$pElem(user))/elements] == 1} {
      set names {}
      set labels {}
      set fd [open $pElem(groupPath,$pElem(user))/elements r]
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
        if {[regexp {CUSTOM([a-zA-Z0-9_]+) (.*)} $line match id label]  == 1} {
# puts "id:    $id"
# puts "label: $label"
           lappend names $id
           lappend labels $label
           incr compInfo(collectionNum)
         }
      }
      close $fd
      set compInfo(collectionNames) $names
      set compInfo(collectionLabels) $labels
   }
#   set compInfo(compName) "userComp$i"
# puts "done current collections from $pElem(groupPath,$pElem(user))/elements"
}

proc makeCollectionName {} {
   global pElem compInfo
   set index [lsearch $compInfo(collectionLabels) $compInfo(compLabel)]
   set compInfo(nameExists) 0
   if {$index != -1} {
      set compInfo(compName) [lindex $compInfo(collectionNames) $index]
      set compInfo(nameExists) 1
# puts "reusing name $compInfo(compName)"
      return
   }
   if {[regexp {([a-zA-Z0-9_]+).*} $compInfo(compLabel) match name] == 1} {
      set tstName $name
      set rev 1
      while {[lsearch $compInfo(collectionNames) $tstName] != -1} {
         incr rev
              set tstName $name$rev
      }
      set compInfo(compName) $tstName
     
# puts "name is $compInfo(compName)"
   } else {
      set compInfo(compName) ""
   }
}

proc resetCollections {} {
   global pElem compInfo psgInfo
# puts "resetCollection"
   set dir $pElem(groupPath,$pElem(user))
   set fd [open $dir/elements w 0666]
   for {set i 0} {$i < $compInfo(collectionNum)} {incr i} {
      set lab [string trim [lindex $compInfo(collectionLabels) $i]]
      if {$lab !=  ""} {
         puts $fd "CUSTOM[lindex $compInfo(collectionNames) $i] $lab"
#         puts "[lindex $compInfo(collectionNames) $i] $lab"
      } else {
#         puts "DELETE [lindex $compInfo(collectionNames) $i]"
         exec rm -f $dir/CUSTOM[lindex $compInfo(collectionNames) $i]
      }
#      puts $fd "CUSTOM$compInfo(compName) $compInfo(compLabel)"
   }
# puts "resetCollection done"
   return $fd
}

proc saveCollection {} {
   global pElem compInfo psgInfo
# puts "check file $pElem(groupPath,$pElem(user))/elements"
   set fd [resetCollections]
   if {($compInfo(compLabel) == "") || ($compInfo(elem) == 0)} {
      close $fd
      readElemFiles
      fillList
      return
   }
   makeCollectionName
   if {$compInfo(compName) == ""} {
      close $fd
      readElemFiles
      fillList
      return
   }
   if {$compInfo(nameExists) == 0} {
      puts $fd "CUSTOM$compInfo(compName) $compInfo(compLabel)"
   }
   close $fd
   set fd [open $pElem(groupPath,$pElem(user))/CUSTOM$compInfo(compName) w 0666]
      puts $fd <collection>
      putCompAttr $fd label "$compInfo(compLabel)"
      putCompAttr $fd name "CUSTOM$compInfo(compName)"
      puts $fd </collection>
      set num [llength $compInfo(elems)]
      for {set id 1} {$id <= $num} {incr id} {
         set cid [lindex $compInfo(elems) [expr $id-1]]
         puts $fd "<$compInfo($cid,name)>"
# puts "num of attrs is $psgInfo($cid,numAttr)"
         for {set j 1} {$j <= $psgInfo($cid,numAttr)} {incr j} {
# puts "attrs $j is $psgInfo($cid,B$j) $psgInfo($cid,V$j)"
            if {$psgInfo($cid,S$j) != "fixed"} {
               putCompAttr $fd "$psgInfo($cid,N$j)" "$psgInfo($cid,V$j)"
            }
         }
         putCompAttr $fd len $psgInfo($cid,len)
         if {$id == 1} {
            putCompAttr $fd relId 1
            putCompAttr $fd dock {[inherit dock]}
            putCompAttr $fd relIdAt {[inherit idAt]}
            putCompAttr $fd dockAt {[inherit dockAt]}
            putCompAttr $fd chanId {[inherit chanId]}
            set chan $psgInfo($cid,chanId)
            putCompAttr $fd chanDiff 0
         } else {
            set idAt [lindex $compInfo(goodIdAt) [expr $id-1]]
            putCompAttr $fd relId $id
            putCompAttr $fd dock $compInfo($cid,dock)
            putCompAttr $fd relIdAt $idAt
            putCompAttr $fd dockAt $compInfo($cid,dockAt)
            putCompAttr $fd chanId {[inherit chanId]}
            putCompAttr $fd chanDiff [expr $psgInfo($cid,chanId) - $chan]
         }
         puts $fd "</$compInfo($cid,name)>"
      }
   close $fd
   readElemFiles
   fillList
}

proc addToCollection {cid} {
   global compInfo psgInfo
# puts "called addToCollection with find= $cid"
   if {$psgInfo($cid,inuse) == 1} {
        set pos [lsearch $compInfo(elems) $psgInfo($cid,idAt)]
        if {$pos == -1} {
           lappend compInfo(elems) $cid
        } else {
           incr pos
           set compInfo(elems) [linsert $compInfo(elems) $pos $cid]
        }
        incr compInfo(elem)
# puts "elem $compInfo(elem) id:$psgInfo($cid,id) idAt:$psgInfo($cid,idAt)"
# puts "$compInfo(elems)"
        set compInfo($cid,name) $psgInfo($cid,name)
        set compInfo($cid,id) $psgInfo($cid,id)
        set compInfo($cid,idAt) $psgInfo($cid,idAt)
        set compInfo($cid,chanId) $psgInfo($cid,chanId)
        set compInfo($cid,dock) $psgInfo($cid,dock)
        set compInfo($cid,dockAt) $psgInfo($cid,dockAt)
        .draw itemconfigure T$cid -width 3
   }
}

proc addAllToCollection {win x y} {
   global compInfo psgInfo
# puts "call addAllToCollection "
   set tids [$win gettags [$win find closest $x $y]]
   regexp {[^T]*T([0-9]+).*} $tids match id
   if {[info exists id] != 1} {
      return
   }
   set tids [.draw find withtag L$id]
# puts "$id elems to attach ($tids)"
   set newIds $id
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
      if {[lsearch $newIds $newId] == -1} {
         lappend newIds $newId
      }
# puts "elem $i with tags $tid and id $newId"
   }
# puts "hideList = $psgInfo(hideList)"
   set compInfo(hide) 0
   foreach i $psgInfo(hideList) {
      if {[lsearch $newIds $psgInfo($i,idAt)] != -1} {
         lappend newIds $i
         incr compInfo(hide)
# puts "add id $i from hideList"
      }
   }
   set compInfo(elems) {}
   set compInfo(elem) 0
   .draw itemconfigure element -width 1
   foreach i $newIds {
      addToCollection $i
   }
   catch {unset compInfo(goodIdAt)}
   foreach cid $compInfo(elems) {
      set idAt [lsearch $compInfo(elems) $compInfo($cid,idAt)]
      lappend compInfo(goodIdAt) [expr $idAt+1]
   }
# puts "elems= $compInfo(elems)"
# puts "idAt= $compInfo(goodIdAt)"
   if {$compInfo(elem) > 0} {
      set plural "s"
      if {$compInfo(elem) == 1} {
         set plural ""
      }
      if {$compInfo(hide) == 0} {
         set compInfo(collMsg) "$compInfo(elem) element$plural in collection."
      } else {
         set compInfo(collMsg) \
           "$compInfo(elem) element$plural in collection ($compInfo(hide) hidden by if statements)."
      }
   } else {
      set compInfo(collMsg) ""
   }
}

proc removeAllFromCollection {win x y} {
   global compInfo psgInfo
# puts "call removeAllFromCollection "
   set tids [$win gettags [$win find closest $x $y]]
   regexp {[^T]*T([0-9]+).*} $tids match id
   if {[info exists id] != 1} {
      return
   }
   set tids [.draw find withtag L$id]
# puts "$id elems to detach ($tids)"
   set newIds {}
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
      if {[lsearch $newIds $newId] == -1} {
         lappend newIds $newId
      }
# puts "elem $i with tags $tid and id $newId"
   }
   foreach i $psgInfo(hideList) {
      if {[lsearch $newIds $psgInfo($i,idAt)] != -1} {
         lappend newIds $i
         incr compInfo(hide) -1
# puts "add id $i from hideList"
      }
   }
   foreach cid $newIds {
      set found [lsearch $compInfo(elems) $cid]
      if {$found != -1} {
         set compInfo(elems) [lreplace $compInfo(elems) $found $found]
         incr compInfo(elem) -1
         .draw itemconfigure T$cid -width 1.0
      }
   }
   catch {unset compInfo(goodIdAt)}
   foreach cid $compInfo(elems) {
      set idAt [lsearch $compInfo(elems) $compInfo($cid,idAt)]
      lappend compInfo(goodIdAt) [expr $idAt+1]
   }
   if {$compInfo(elem) > 0} {
      set plural "s"
      if {$compInfo(elem) == 1} {
         set plural ""
      }
      if {$compInfo(hide) == 0} {
         set compInfo(collMsg) "$compInfo(elem) element$plural in collection."
      } else {
         set compInfo(collMsg) \
           "$compInfo(elem) element$plural in collection ($compInfo(hide) hidden by if statements)."
      }
   } else {
      set compInfo(collMsg) ""
   }
}

proc exitCollection {} {
   global pInfo compInfo
   set pInfo(collection) 0
   if {[winfo exists .coll] == 1} {
      regexp {[^+]*(.*)} [wm geometry .coll] match val
      set pInfo(collectionGeom) $val
      foreach cid $compInfo(elems) {
         .draw itemconfigure T$cid -width 1.0
      }
      .draw bind element <Button-2>  {}
      .draw bind element <Shift-Button-2>  {}
      destroy .coll
      unset compInfo
   }
}

proc startCollecting {} {
   global compInfo

# puts "startCollecting label: $compInfo(compLabel)"
   if {$compInfo(compLabel) == ""} {
      set compInfo(errorMsg) "\nA name for the collection must be entered."
   } else {
      set index [lsearch $compInfo(collectionLabels) $compInfo(compLabel)]
      if {$index != -1} {
         set compInfo(errorMsg) \
         " This label is in use. The old collection will be\n\
          over-written if and when the OK button is pressed."
      } else {
         set compInfo(errorMsg) "\n"
# puts "$compInfo(collectionLabels)"
      }
      .coll.frCol3.collOkay configure -state normal
      .draw bind element <Button-2>  {addAllToCollection %W \
            [%W canvasx %x] [%W canvasy %y]}
      .draw bind element <Shift-Button-2> {removeAllFromCollection %W \
            [%W canvasx %x] [%W canvasy %y]}
   }
}

proc updateCollection {i} {
   global compInfo
   set tst [lindex $compInfo(collectionLabels) $i]
# puts "updateCollection old= $tst new= $compInfo(coll$i)"
   if {$tst != $compInfo(coll$i)} {
      set compInfo(collectionLabels) \
       [lreplace $compInfo(collectionLabels) $i $i $compInfo(coll$i)]
      .coll.frCol3.collOkay configure -state normal
   }
}

proc fillCollectionList {win} {
   global compInfo
   for {set i 0} {$i < $compInfo(collectionNum)} {incr i} {
      entry $win.b$i -relief sunken -highlightthickness 0 \
         -textvariable compInfo(coll$i) -width 40 -relief flat
      $win window create end -window $win.b$i
      set compInfo(coll$i) [lindex $compInfo(collectionLabels) $i]
      bind $win.b$i <Return> "updateCollection $i"
      bind $win.b$i <FocusOut> "updateCollection $i"
   }
   $win configure -state disabled
}

proc showHideInst {} {
   global pInfo compInfo
# puts "showHideInst"
   if {$pInfo(collectionInst) == 1} {
     regexp {[^+]*(.*)} [wm geometry .coll] match val
     set pInfo(collectionGeom) $val
# puts "save geometry $pInfo(collectionGeom)"
     destroy .coll
     makeCollectionWins
     update idletasks
     wm geometry .coll $pInfo(collectionGeom)
   } else {
     pack forget .coll.collInfo
     pack forget .coll.collInfo2
     update idletasks
     regexp {[^+]*(.*)} [wm geometry .coll] match val
     set pInfo(collectionGeom) $val
# puts "save2 geometry $pInfo(collectionGeom)"
   }
}

proc makeCollection {} {
   global psgInfo pInfo compInfo
   exitCollection
   initCollectionInfo
   set pInfo(collection) 1
   currentCollections
   makeCollectionWins
   showHideInst
}

proc makeCollectionWins {} {
   global psgInfo pInfo compInfo
   toplevel .coll
   set w .coll
   wm title $w "SpinCAD Collection Manager"
   wm minsize $w 40 20
   frame $w.frCol1 
   frame $w.frCol2 
   frame $w.frCol3 
   frame $w.frCol4 
   frame $w.frCol5 
   set w1 $w.frCol1
   set w2 $w.frCol2
   set w3 $w.frCol3
   set w4 $w.frCol4
   set w5 $w.frCol5
   text $w.collInfo -wrap word -setgrid true -relief flat -height 16 \
        -highlightthickness 0
   set compInfo(info) \
         "\nThis panel is used to add, rename, or delete collections\
         of pulse elements.  To add a collection, first enter the name\
         of the pulse collection in the entry field below. Press the\
         Return key or start selecting elements to confirm the name.  Note that the OK button at the\
         bottom of this panel remains inactive until a name is confirmed.\
         After the name is entered, the pulse\
         elements are selected with the Middle Mouse Button.  Selected\
         items appear with a large border. The element under the mouse cursor\
         and all elements connected to the right of that element will be\
         selected. A Middle Mouse Button with the Shift key held down\
         will de-select the item below the mouse cursor and all elements\
         connected to the right of that item. The default attributes for each\
         element should be filled in. The display size of the collection can\
         also be adjusted with the anchor points at the bottom of the display.\
         Once the collection is correct, press the OK button to save it.  The\
         Cancel button may be pressed at any time to prevent any changes.\n"
   $w.collInfo insert end $compInfo(info)

   label $w.collError -textvariable compInfo(errorMsg) -foreground red -justify left
   label $w1.collName -text "Enter name of pulse collection "
   entry $w1.collNameEntry -width 18 -relief sunken -highlightthickness 0 \
         -textvariable compInfo(compLabel)
   pack $w1.collName $w1.collNameEntry -side left -in $w1 -anchor w
   label $w.collMsg -textvariable compInfo(collMsg)

   set compInfo(connError) ""
   label $w2.collError2 -textvariable compInfo(connError)
#   pack $w.collInfo $w.collError $w1 $w.collMsg $w2 -side top -anchor w -padx 5


   text $w.collInfo2 -wrap word -setgrid true -relief flat -height 6 \
        -highlightthickness 0
   set compInfo(info2) \
         "\nThe scrolling list below shows the current collections of\
         pulse elements.  The collections can be relabeled by highlighting\
         the label and entering a new label.  Finish by typing a Return key.\
         Collections may be deleted by deleting a label.  The renaming and\
         deletion of collections will occur when the OK button is pressed.\n"
   $w.collInfo2 insert end $compInfo(info2)

   pack $w.collInfo -side top -anchor w -padx 5 -expand 1 -fill both
   pack $w.collError $w1 $w.collMsg -side top -anchor w -padx 5
   pack $w.collInfo2 -side top -anchor w -padx 5 -expand true -fill both

   checkbutton $w.showInst -text "Show Instructions" -command showHideInst \
          -variable pInfo(collectionInst) -indicatoron 1 -highlightthickness 0
   pack $w.showInst -side top -anchor w -padx 5 -pady 5
   label $w4.listLabel -text "Current Collections"
   text $w4.list -relief raised -borderwidth 2 \
         -yscrollcommand "$w4.scroll set" -width 40 -height 10
   scrollbar $w4.scroll -command "$w4.list yview"
   pack $w4.listLabel -side top -in $w4 -padx 5
   pack $w4.scroll -side right -anchor w -in $w4 -fill y -expand 1
   pack $w4.list -in $w4 -side right -anchor w -expand 1 -fill both
   fillCollectionList $w4.list


   button $w3.collOkay -text OK -command "saveCollection; exitCollection" \
        -state disabled
   bind $w1.collNameEntry <Return> "startCollecting"
   .draw bind element <Button-2>  "startCollecting"
   button $w3.collCancel -text Cancel -command exitCollection
   pack $w3.collOkay $w3.collCancel -side left -padx 5 -pady 5 -in $w3
   pack $w3 -side bottom -fill x
   pack $w4 -side top -anchor w -fill both -expand 1 -padx 5 -pady 5
   if {[info exists pInfo(collectionGeom)] == 1} {
# puts "set geometry $pInfo(collectionGeom)"
      wm geometry .coll $pInfo(collectionGeom)
   }
   wm protocol $w WM_DELETE_WINDOW {
      exitCollection
   }
}
