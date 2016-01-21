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

proc dock_sound {} {
   global pInfo
   if {($pInfo(volume) > 0) && ($pInfo(audio) == 1)} {
      catch {exec audioplay -iv $pInfo(volume) $pInfo(dockSound)}
   }
}

proc setDisplayStyle {id} {
   global psgInfo stipple pInfo
# puts "add el = $index"
   set style normal
   set parallel no
   set stipple ""
   set psgInfo($id,polar) 1
   set psgInfo($id,shape) rect
   set psgInfo($id,stipple) ""
   set psgInfo($id,mpdisp) 0.5
# puts "check style for element $id $psgInfo($id,elemLabel)"
   for {set i 1} {$i <= $psgInfo($id,numAttr)} {incr i} {
     switch $psgInfo($id,N$i) {
       style    {
                  set style $psgInfo($id,V$i)
                }
       parallel {
                  set parallel $psgInfo($id,V$i)
                }
       len      {
                  set psgInfo($id,len) $psgInfo($id,V$i)
                }
       ht       {
                  set psgInfo($id,ht) $psgInfo($id,V$i)
                }
       shape    {
                  set psgInfo($id,shape) $psgInfo($id,V$i)
                }
       stipple  {
                  set psgInfo($id,stipple) $psgInfo($id,V$i)
                  if {$psgInfo($id,stipple) != ""} {
                   set stipple \
                     @[file join $pInfo(homeDir) bitmaps $psgInfo($id,stipple)]
                  }
                }
       polar    {
                  set psgInfo($id,polar) $psgInfo($id,V$i)
                  set psgInfo($id,polaraddr) $id,V$i
                }
       mpdisp    {
                  set psgInfo($id,mpdisp) $psgInfo($id,V$i)
                }
       default  {  }
     }
   }
   set psgInfo($id,multi) 0
   set psgInfo($id,base) 0
   if {$psgInfo($id,type) == "ERROR"} {
      set psgInfo($id,style) error
      set style error
   } else {
      set psgInfo($id,style) $style
   }
# puts "style for $id is $style"
   set psgInfo($id,parallel) $parallel
   switch $style {
    normal   {
                set psgInfo($id,defHt) 6
                set psgInfo($id,defLen) 50
             }
    error   {
                set psgInfo($id,defHt) 100
                set psgInfo($id,defLen) 100
             }
    delay    {
                set psgInfo($id,defHt) 2
                set psgInfo($id,defLen) 50
             }
    shortdelay {
                set psgInfo($id,defHt) 2
                set psgInfo($id,defLen) 10
             }
    pulse    {
                set psgInfo($id,defHt) 40
                set psgInfo($id,defLen) 10
             }
    acquire     {
                set psgInfo($id,defHt) 20
                set psgInfo($id,defLen) 20
             }
    gate     {
                set psgInfo($id,defHt) 20
                set psgInfo($id,defLen) 6
             }
    if   {
                set psgInfo($id,multi) 1
                set psgInfo($id,base) 15
                set psgInfo($id,defHt) 55
                set psgInfo($id,defLen) 5
             }
    while   {
                set psgInfo($id,defHt) 1
                set psgInfo($id,defLen) 80
                set psgInfo($id,multi) 1
             }
    box   {
                set psgInfo($id,defHt) 15
                set psgInfo($id,defLen) 15
             }
    marker   {
                set psgInfo($id,defHt) 15
                set psgInfo($id,defLen) 15
             }
    trap     {
                set psgInfo($id,defHt) 15
                set psgInfo($id,defLen) 50
             }

    default  {
                set psgInfo($id,defHt) 10
                set psgInfo($id,defLen) 50
             }
   }
   if {[info exists psgInfo($id,ht)] != 1} {
#puts "set ht to default ht $psgInfo($id,defHt) for $id"
#puts "set psgInfo($id,ht) to default ht $psgInfo($id,defHt) for $id"
#      set psgInfo($id,ht) [expr $psgInfo($id,defHt) / $psgInfo(fract)]
      set psgInfo($id,ht) $psgInfo($id,defHt)
   }
   if {[info exists psgInfo($id,len)] != 1} {
      set psgInfo($id,len) [expr $psgInfo($id,defLen) / $psgInfo(fract)]
   }
}

proc resetCollectionInfo {id index i} {
   global psgInfo pElem
   for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
# puts "check psgInfo($id,S$j) $psgInfo($id,S$j) == user ($psgInfo($id,N$j))"
      if {$psgInfo($id,S$j) == "user"} {
         if {[regexp {attr([0-9]+)} $psgInfo($id,N$j) match ii] == 1}  {
# puts "pElem(N$ii,$index,$i) $pElem(N$ii,$index,$i) = $psgInfo($id,N$j)"
            if {$pElem(N$ii,$index,$i) == $psgInfo($id,N$j)} {
               set psgInfo($id,V$j) $pElem(V$ii,$index,$i)
# puts "set psgInfo($id,V$j) to pElem(V$ii,$index,$i) $pElem(V$ii,$index,$i)"
            }
         }
      } elseif {$psgInfo($id,S$j) == "primitive"} {
# puts "set psgInfo($id,V$j) to pElem($psgInfo($id,N$j),$index,$i) ($pElem($psgInfo($id,N$j),$index,$i))"
# puts "current psgInfo($id,V$j) is $psgInfo($id,V$j)"
         if {[info exists pElem($psgInfo($id,N$j),$index,$i)] == 1} {
            set psgInfo($id,V$j) $pElem($psgInfo($id,N$j),$index,$i)
         } else {
            set psgInfo($id,V$j) ""
         }
      }
   }
# puts "collection elem $id"
# foreach el [array names psgInfo $id,*] {
#  puts "psgInfo($el): $psgInfo($el)"
# }
}

proc changePsgInfo {id} {
   global psgInfo pElem
   set index $pElem(loadId)
# parray pElem
# puts "adding element $index"
   set psgInfo($id,name) $pElem(elemName,$index)
   set psgInfo($id,pIndex) $index
   set psgInfo($id,elemLabel) $pElem(elemLabel,$index)
   set psgInfo($id,type) $pElem(type,$index)
   set psgInfo($id,collection) ""
   set psgInfo($id,label) $pElem(label,$index)
   set psgInfo($id,numAttr) $pElem(numAttr,$index)
   set psgInfo($id,numWins) $pElem(numWins,$index)
# puts "wins for id $id is $psgInfo($id,numWins)"
   set psgInfo($id,labelId) 0
   for {set i 1} {$i <= $pElem(numAttr,$index)} {incr i} {
         set psgInfo($id,N$i) $pElem(N$i,$index)
         set tmp($pElem(N$i,$index)) $i
         if {($pElem(N$i,$index) == "attr1")} {
            set psgInfo($id,labelId) $i
# puts "psgInfo($id,labelId) $psgInfo($id,labelId)"
         } elseif {($pElem(N$i,$index) == "labelId")} {
            set psgInfo($id,labelId) $tmp($pElem(V$i,$index))
# puts "psgInfo($id,labelId) $psgInfo($id,labelId)"
         }
         set psgInfo($id,B$i) $pElem(B$i,$index)
         set psgInfo($id,L$i) $pElem(L$i,$index)
         set psgInfo($id,L$i,type) entry
         if {[info exists pElem(LE$i,$index)] == 1} {
# puts "pElem(LE$i,$index)= $pElem(LE$i,$index)"
            if {[lindex $pElem(LE$i,$index) 0] == "MENU"} {
               set psgInfo($id,L$i,type) menu
               set psgInfo($id,L$i,vals)  \
                 [lrange $pElem(LE$i,$index) 1 [llength $pElem(LE$i,$index)]]
# puts "set vals for $psgInfo($id,L$i) to $psgInfo($id,L$i,vals)"
            } elseif {[lindex $pElem(LE$i,$index) 0] == "EXECMENU"} {
               set psgInfo($id,L$i,type) execmenu
               set psgInfo($id,L$i,vals)  \
                 [lrange $pElem(LE$i,$index) 1 [llength $pElem(LE$i,$index)]]
# puts "set vals for $psgInfo($id,L$i) to $psgInfo($id,L$i,vals)"
               if {$psgInfo($id,L$i,vals) == "updateIfStyle"} {
                  set psgInfo($id,ifStyleAddr) $id,V$i
# puts "set psgInfo($id,ifStyleAddr)= $psgInfo($id,ifStyleAddr)"
               }
            } elseif {[lindex $pElem(LE$i,$index) 0] == "EXECWAVE"} {
               set psgInfo($id,L$i,type) execwave
               set psgInfo($id,L$i,vals)  \
                 [lrange $pElem(LE$i,$index) 1 [llength $pElem(LE$i,$index)]]
            } elseif {[lindex $pElem(LE$i,$index) 0] == "CHOICE"} {
               set psgInfo($id,L$i,type) choice
               set psgInfo($id,L$i,vals)  \
                 [lrange $pElem(LE$i,$index) 1 [llength $pElem(LE$i,$index)]]
# puts "set vals for $psgInfo($id,L$i) to $psgInfo($id,L$i,vals)"
            } elseif {[lindex $pElem(LE$i,$index) 0] == "EXECCHOICE"} {
               set psgInfo($id,L$i,type) execchoice
               set psgInfo($id,L$i,vals)  \
                 [lrange $pElem(LE$i,$index) 1 [llength $pElem(LE$i,$index)]]
# puts "set vals for $psgInfo($id,L$i) to $psgInfo($id,L$i,vals)"
            }
            set psgInfo($id,LE$i) $pElem(LE$i,$index)
         }
         set psgInfo($id,V$i) $pElem(V$i,$index)
         set psgInfo($id,S$i) $pElem(S$i,$index)
   }
   setDisplayStyle $id
   set psgInfo($id,phTable) 0
# puts "pElem(phTablesInherit,$index) $pElem(phTablesInherit,$index)"
   if {[llength $pElem(phTablesInherit,$index)]  > 0} {
      foreach el $pElem(phTablesInherit,$index) {
         set val $el
         for {set i 1} {$i <= $pElem(numAttr,$index)} {incr i} {
            if {$el == $psgInfo($id,N$i)} {
               set val V$i
               break
            }
         }
         lappend psgInfo(phTablesInherit) $id $val
         set psgInfo($id,phTable) $val
         set psgInfo($id,phType) inherit
      }
# puts "new inherit ($index) phase table list is $psgInfo(phTablesInherit)"
   }
# puts "pElem(phTablesFixed,$index) $pElem(phTablesFixed,$index)"
   if {[llength $pElem(phTablesFixed,$index)]  > 0} {
      foreach el $pElem(phTablesFixed,$index) {
         lappend psgInfo(phTablesFixed) $id $el
         set psgInfo($id,phTable) $el
         set psgInfo($id,phType) fixed
# puts "new fixed ($index) phase table list is $psgInfo(phTablesFixed)"
      }
   }
}

proc addElemToPSG {} {
   global psgInfo
   incr psgInfo(elem)
   set id $psgInfo(elem)
   set psgInfo(cur) $id
   set psgInfo($id,id) $id
   lappend psgInfo(elems) $id
   changePsgInfo $id
   set psgInfo($id,inuse) 1
   set psgInfo($id,attrOk) 1
   set psgInfo($id,dock) "front"
   set psgInfo($id,idAt) start
#   set psgInfo($id,nameAt) 0
   set psgInfo($id,dockAt) end
   set psgInfo($id,chanId) 1
   set psgInfo($id,tags) L$id
#puts "init tags for $id to $psgInfo($id,tags)"
}

proc dashLine {w x} {
   global psgInfo pInfo lineFill
   set y1 $psgInfo(ytop)
   set y2 $psgInfo(ybot)
   $w create line $x $y1 $x $y2 -tag dockLines \
      -fill $pInfo(dockColor) -stipple $lineFill
}

proc showDockList {} {
   global dockList psgInfo linesOK pInfo
   if {$pInfo(composite) == 1} {
      updateCompDock
   }
   .draw delete dockLines
   if {$linesOK == 0} {
      return
   }
   set list {}
   for {set i 1} {$i <= $dockList(num)} {incr i} {
      if {$dockList($i) == 0} continue
      if {[lsearch $list $dockList($i)] != -1} continue
      lappend list $dockList($i)
      dashLine .draw $dockList($i)
   }
   foreach id $psgInfo(drawList) {
      if {$psgInfo($id,idAt) != "start"} {
         set i $dockList($psgInfo($id,idAt),$psgInfo($id,dockAt))
         set x $dockList($i)
         .draw create line $x $psgInfo(ytop) $x $psgInfo(ybot) \
            -tag dockLines -fill $pInfo(dockColor)
      }
   }
   .draw lower dockLines
}

proc showTmpDockList {} {
   global dockList psgInfo linesOK pInfo
   .draw delete dockLines
   if {$linesOK == 0} {
      return
   }
   set list {}
   for {set i 1} {$i <= $dockList(num)} {incr i} {
      if {$dockList($i) == 0} continue
      if {[lsearch $list $dockList($i)] != -1} continue
      if {[lsearch $psgInfo(detach) $dockList($i,id)] != -1} continue
      lappend list $dockList($i)
      dashLine .draw $dockList($i)
   }
   foreach id $psgInfo(drawList) {
      if {($psgInfo($id,idAt) != "start") && \
          ([lsearch $psgInfo(detach) $id] == -1)} {
         set i $dockList($psgInfo($id,idAt),$psgInfo($id,dockAt))
         set x $dockList($i)
         .draw create line $x $psgInfo(ytop) $x $psgInfo(ybot) \
            -tag dockLines -fill $pInfo(dockColor)
      }
   }
   .draw lower dockLines
}

proc setDockList {id d1 d2 d3} {
   global dockList psgInfo
   set d12 [expr $d1 + 2]
   set d22 [expr $d2 + 2]
   set d32 [expr $d3 + 2]
   incr dockList(num)
   set inum $dockList(num)
   set dockList($inum) $d12
   set dockList($inum,id) $id
   set dockList($inum,pos) front
   set dockList($id,front) $inum
   set dockList($id,true) $inum
   set dockList($inum,trueOk) 1
   lappend dockList(sort) "$d12 $inum"
   if {($psgInfo($id,parallel) != "no") || \
       ($psgInfo($id,style) == "if")} {
      incr dockList(num)
      set inum $dockList(num)
      if {$psgInfo($id,style) == "if"} {
         set dockList($inum) $d12
      } else {
         set dockList($inum) $d22
      }
      set dockList($inum,id) $id
      set dockList($inum,pos) mid
      set dockList($id,mid) $inum
   set dockList($id,false) $inum
   set dockList($inum,falseOk) 1
      lappend dockList(sort) "$d22 $inum"
   }
   incr dockList(num)
   set inum $dockList(num)
   set dockList($inum) $d32
   set dockList($inum,id) $id
   set dockList($inum,pos) end
   set dockList($id,end) $inum
   set dockList($inum,endOk) 1
#puts "set dockList($inum,endOk) 1"
   lappend dockList(sort) "$d32 $inum"
}

proc redrawItem {id} {
   global psgInfo
   set d1 [expr $psgInfo($id,x) + 2]
   set len $psgInfo($id,len)
# puts "redraw $id at $d1, $psgInfo($id,y) with len $len and  ht $psgInfo($id,ht)"
   if {[info exists psgInfo($id,polaraddr)] == 1} {
      set psgInfo($id,polar) $psgInfo($psgInfo($id,polaraddr))
   }
   if {$psgInfo($id,multi) != 0} {
# puts "psgInfo($id,multi)= $psgInfo($id,multi)"
      set num $psgInfo(numChan)
      if {$psgInfo($id,polar) > 0} {
       set ytop [expr $psgInfo($id,y)]
       set ybot [expr $ytop + $psgInfo($id,ht)]
      } else {
       set ytop [expr $psgInfo($id,y) + $psgInfo($id,ht)]
       set ybot [expr $ytop + $psgInfo($id,ht) * 2]
      }
# This may potentially be a problem.
#   If negative elements are floating above the line, readjust
# puts "redraw - for item $id, multi is $psgInfo($id,multi)"

      for {set i 1} {$i <= $num} {incr i} {
         set diff [expr ($i - $psgInfo($id,chanId)) * $psgInfo(ydelta)]
         alterSize $id T${id}CH$i $d1 [expr $ytop + $diff] \
          [expr $d1+$len] [expr $ybot + $diff]
         if {($psgInfo($id,style) == "if")} {
           alterSize $id TIF${id}CH$i $d1 [expr $ytop + $diff] \
            [expr $d1+3] [expr $ybot + $diff]
         }
#         .draw raise TIF${id}CH$i channel
      }
   } else {
      if {$psgInfo($id,polar) > 0} {
       alterSize $id T$id $d1 $psgInfo($id,y) \
        [expr $d1+$len] [expr $psgInfo($id,y) + $psgInfo($id,ht)]
      } else {
       alterSize $id T$id \
         $d1 [expr $psgInfo($id,y) + $psgInfo($id,ht) * 2]\
         [expr $d1+$len] [expr $psgInfo($id,y) + $psgInfo($id,ht)]
      }       
   }
}

proc alterSize {id tag x1 y1 x2 y2} {
 global psgInfo
# puts "$id is $psgInfo($id,shape)"
 switch $psgInfo($id,shape) {
  rect		{
  		 .draw coords $tag $x1 $y1 $x2 $y2
#  		 puts "$id rect - $x1 $y1, $x2 $y2"
  		}
  arc		{
  		 .draw coords $tag $x1 $y1 \
  		   $x2 [expr $y2 + abs($y1 -$y2)]
#  		 puts "$id arc - $x1 $y1, $x2 $y2"
  		}
  diamond	{
  		  .draw coords $tag $x1 $y2 \
  		   [expr ($x1 + $x2)/2.0] $y1 \
  		   $x2 $y2\
  		   [expr ($x1 + $x2)/2.0] [expr $y2 + abs($y1 -$y2)]
  		}
  ellipse	{
  		 .draw coords $tag $x1 $y1 \
  		   $x2 [expr $y2 + abs($y1 -$y2)]
#  		 puts "$id ellipse"
  		}
  flagOn	{
  		 .draw coords $tag $x1 [expr $y2 - 5] $x2 [expr $y2 - 5] \
                $x2 $y2 $x1 $y2 $x1 $y1 \
                $x2 [expr $y1 + 5] $x1 [expr $y1 + 10]
#  		 puts "$id flag on"
  		}
  flagOff	{
  		 .draw coords $tag $x2 [expr $y2 - 5] $x1 [expr $y2 - 5] \
                $x1 $y2 $x2 $y2 $x2 $y1 \
                $x1 [expr $y1 + 5] $x2 [expr $y1 + 10]
#  		 puts "$id flag off"
  		}
  triangle	{
#  		 puts [.draw coords $tag]
  		 .draw coords $tag $x1 $y1 \
  		   $x2 $y2 \
  		   $x1 [expr $y2 + abs($y1 -$y2)]
#  		 puts "$id triangle"
  		}
     loop     {
               set y [expr ($y1 + $y2)/2.0]
               .draw coords $tag $x1 $y $x1 [expr $y - 45] \
                [expr $x1 + 5] [expr $y - 50] $x1 [expr $y - 45] \
                $x1 [expr $y + 10] [expr $x1 + 5] [expr $y + 15] \
                $x1 [expr $y + 10] \
                $x1 $y \
                $x2 $y $x2 [expr $y - 45] [expr $x2 - 5] [expr $y - 50]  \
                $x2 [expr $y - 45] \
                $x2 [expr $y + 10] [expr $x2 - 5] [expr $y + 15] \
                $x2 [expr $y + 10] \
                $x2 $y
              }
  marker	{
#  		 puts [.draw coords $tag]
# puts "x1= $x1 y1= $y1 x2= $x2 y2= $y2"
  		 .draw coords $tag $x1 $y2 \
  		   $x2 $y2 \
  		   [expr $x1 + abs($x1 -$x2)/2.0] $y1
#   		 puts "$id marker"
  		}
  trap		{
  		 .draw coords $tag [expr $x1 + 5] $y1 \
  		  $x1 $y2 $x2 $y2 \
  		  [expr $x2 - 5] $y1
#  		 puts "$id trap - $x1 $y1, $x2 $y2"
  		}
  doubletrap   {
              .draw coords $tag [expr $x1 + 5]  [expr ($y1 + $y2)/2.0 - 15] \
               $x1 [expr ($y1 + $y2)/2.0] \
               [expr $x1 + 5]  [expr ($y1 + $y2)/2.0 + 15] \
               [expr $x2 - 5]  [expr ($y1 + $y2)/2.0 + 15] \
               $x2 [expr ($y1 + $y2)/2.0] \
               [expr $x2 - 5]  [expr ($y1 + $y2)/2.0 - 15]
                }
 }
}

proc generateElem {win id fillcol tag xa ya xb yb} {
    global psgInfo pInfo
# puts "generateElem: $win $id $fillcol $tag $xa $ya $xb $yb"
    set stipple ""
    if {$fillcol == "bg"} {
     if {$psgInfo($id,stipple) != ""} {
      set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($id,stipple)]
     }
     if {$stipple == ""} {
      set fcol $psgInfo(bg)
     } else {
      set fcol $pInfo(elemColor)
     }
    } elseif {$fillcol != ""} {
     set fcol $pInfo($fillcol)
    } else {
     set fcol ""
    }
# puts "generateElem $id with $psgInfo($id,shape)"
    switch $psgInfo($id,shape) {
     rect     {
               $win create rect $xa $ya $xb $yb -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple
              }
     ellipse  {
               $win create oval $xa $ya $xb $yb -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     arc      {
               $win create arc $xa $ya $xb $yb -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol \
                -extent -180 -start 180 -style chord -stipple $stipple              
              }
     diamond  {
               $win create polygon $xa $yb \
                [expr ($xa + $xb)/2.0] $ya \
                $xb $ya \
                [expr ($xa + $xb)/2.0] [expr $yb + abs($yb -$ya)] \
                -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple
              }
     triangle {
               $win create polygon $xa $ya $xb $yb \
                $xa [expr $yb + abs($yb -$ya)] -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     loop     {
               set y [expr ($ya + $yb)/2.0]
               $win create polygon $xa $y $xa [expr $y - 50] \
                [expr $xa + 5] [expr $y - 50] $xa [expr $y - 45] \
                $xa [expr $y + 10] [expr $xa + 5] [expr $y + 15] \
                $xa [expr $y + 10] \
                $xa $y \
                $xb $y $xb [expr $y - 45] [expr $xb - 5] [expr $y - 50]  \
                $xb [expr $y - 45] \
                $xb [expr $y + 10] [expr $xb - 5] [expr $y + 15] \
                $xb [expr $y + 10] \
                $xb $y  \
                -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     flagOn   {
               $win create polygon $xa [expr $yb - 5] $xb [expr $yb - 5] \
                $xb $yb $xa $yb $xa $ya \
                $xb [expr $ya + 5] $xa [expr $ya + 10] -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     flagOff  {
               $win create polygon $xb [expr $ya - 5] $xa [expr $ya - 5] \
                $xa $ya $xb $ya $xb $yb \
                $xa [expr $yb + 5] $xb [expr $yb + 10] -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     marker   {
               $win create polygon $xa $ya $xb $ya \
                [expr $xa + abs($xb -$xa)/2.0] [expr -1 * $yb ] -tag $tag \
                -outline $pInfo(elemColor) -fill $fcol -stipple $stipple               
              }
     trap     {
               $win create polygon [expr $xa + 5] $ya \
                $xa $yb \
                $xb $yb \
                [expr $xb - 5] $ya \
                -outline $pInfo(elemColor) -tag $tag -fill $fcol \
                -stipple $stipple              
              }
   doubletrap {
               $win create polygon [expr $xa + 5]  [expr ($ya + $yb)/2.0 - 15] \
               $xa [expr ($ya + $yb)/2.0] \
               [expr $xa + 5]  [expr ($ya + $yb)/2.0 + 15] \
               [expr $xb - 5]  [expr ($ya + $yb)/2.0 + 15] \
               $xb [expr ($ya + $yb)/2.0] \
               [expr $xb - 5]  [expr ($ya + $yb)/2.0 - 15] \
                -outline $pInfo(elemColor) -tag $tag -fill $fcol \
                -stipple $stipple
              }
    }
}

proc drawItem {id adjust} {
   global psgInfo pInfo numChan
#   puts "drawItem - $id is $psgInfo($id,shape)"
   if {[.draw find withtag T$id] == ""} {
# nothing with this tag - make new tiny box ------
      generateElem .draw $id "bg" "T$id element scl" \
       0 20 0 20
      if {$psgInfo($id,multi) != 0} {
# now for multi-channel
      .draw addtag T${id}CH1 withtag T$id
        if {($psgInfo($id,style) == "if")} {
         generateElem .draw $id "elemColor" \
          "T$id TIF$id L$id TIF${id}CH1 element scl" \
          0 0 3 20
        }
        for {set i 2} {$i <= $numChan} {incr i} {
           generateElem .draw $id "" "T$id L$id T${id}CH$i element scl" \
            0 0 10 20
           if {($psgInfo($id,style) == "if")} {
             generateElem .draw $id "elemColor" \
              "T$id TIF$id L$id TIF${id}CH$i element scl" \
              0 0 3 20
           }
         }
      }
# have this tag
   } elseif {$psgInfo($id,multi) != 0} {
# multichannel
      for {set i 1} {$i <= $numChan} {incr i} {
         if {[.draw find withtag T${id}CH$i] == ""} {
           generateElem .draw $id "" "T$id L$id T${id}CH$i element scl" \
              0 0 10 20
           if {($psgInfo($id,style) == "if")} {
             generateElem .draw $id "elemColor" \
              "T$id TIF$id L$id TIF${id}CH$i element scl"\
                0 0 3 20
           }
         }
      }
      while {$i <= 8} {
         if {[.draw find withtag T${id}CH$i] != ""} {
            .draw delete T${id}CH$i
            .draw delete TIF${id}CH$i
         }
         incr i
      }
   }
   redrawItem $id
   .draw bind T$id <Button-1> "startAdjustmentC .draw $id %x %y"
   .draw bind T$id <Button-3> "selectAnItemC .draw $id %x %y"
   if {$pInfo(doComp) == 1} {
      .draw bind T$id <Shift-Control-Button-1> "replaceItemC .draw $id %x %y"
   }
   .draw bind T$id <Shift-Button-1> "startCopyItemC .draw $id %x %y"
}

set pInfo(dropsite) 0
set pInfo(dropsite,on) 0

proc setDropSite {} {
  global pInfo psgInfo
  if {$pInfo(dropsite,on) == 1} {
    set pInfo(dropsite) $psgInfo(cur)
    .draw itemconfigure T$pInfo(dropsite) -width 3
  } else {
    .draw itemconfigure T$pInfo(dropsite) -width 1
    set pInfo(dropsite) 0
  }
}

proc turnOffDropSite {} {
  global pInfo
  if {$pInfo(dropsite) > 0} {
     .draw itemconfigure T$pInfo(dropsite) -width 1
  }
  set pInfo(dropsite) 0
  set pInfo(dropsite,on) 0
}

proc deleteElem {id} {
   global psgInfo

   set newidAt $psgInfo($id,idAt)
   set psgInfo($id,inuse) -1
   set psgInfo($id,idAt) 0
   for {set i 1} {$i <= $psgInfo(elem)} {incr i} {
      if {$psgInfo($i,idAt) == $id} {
         set psgInfo($i,idAt) $newidAt
      }
   }
}

proc deleteElems {id} {
   global psgInfo pInfo
   set psgInfo($id,inuse) -1
   set psgInfo($id,phTable) 0
# puts "delete id $id"
   .draw delete T$id
   set tids [.draw find withtag L$id]
   set usedIds $id
   delElem $id
   delPhElem $id
#puts "additional elem to delete ($tids)"
   foreach i $tids {
      set tid [.draw gettags $i]
#puts "elem $i with tags $tid"
      regexp {[^T]*T([0-9]+).*} $tid match newId
      set psgInfo($newId,inuse) -1
      set psgInfo($newId,phTable) 0
      .draw delete T$newId
#puts "elem $i with tags $tid and id $newId"
      delElem $newId
      delPhElem $newId
      lappend usedIds $newId
   }
   for {set i 1} {$i <= $psgInfo(elem)} {incr i} {
      if {$psgInfo($i,inuse) != -1} {
         if {([lsearch $usedIds $i] == -1) && \
              ([lsearch $usedIds $psgInfo($i,idAt)] != -1)} {
            set psgInfo($i,inuse) -1
            set psgInfo($i,phTable) 0
            lappend usedIds $i
            delElem $i
            delPhElem $i
         }
      }
   }
   clearWindows
}

proc resortIds {oldList} {
   global psgInfo
   set newList start
# puts "sort oldList= $oldList"
   while {[llength $oldList] > 0} {
      set id [lindex $oldList 0]
      set idAt $psgInfo($id,idAt)
      set index [lsearch $newList $idAt]
      if {$index == -1} {
         lappend oldList $id
      } else {
         lappend newList $id
      }
      set oldList [lreplace $oldList 0 0]
   }
# puts "sort newList= $newList"
   set newList [lreplace $newList 0 0]
   return $newList
}

proc sortIds {oldList} {
   global psgInfo
   set newList [lindex $oldList 0]
# puts "sort oldList= $oldList"
   set oldList [lreplace $oldList 0 0]
   while {[llength $oldList] > 0} {
      set id [lindex $oldList 0]
      set idAt $psgInfo($id,idAt)
      set index [lsearch $newList $idAt]
      if {$index == -1} {
         lappend oldList $id
      } else {
         lappend newList $id
      }
      set oldList [lreplace $oldList 0 0]
   }
# puts "sort newList= $newList"
   return $newList
}

proc reattachElems {id chanDiff prog} {
   global psgInfo numChan
#  puts "reattach $id with a channel difference of $chanDiff and prog $prog"
# puts "elems= $psgInfo(outList)"
   set tids [.draw find withtag L$id]
#  puts "elems to reattach ($tids) orig tags $psgInfo($id,tags)"
   set tags [ldelete $psgInfo($id,tags) L$id]
   set usedIds $id
   set psgInfo($id,chanId) [expr $psgInfo($id,chanId) - $chanDiff]
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
#  puts "old elem $i with tags $tid and id $newId"
      if {[lsearch $usedIds $newId] == -1} {
         lappend usedIds $newId
      }
   }
# puts "reattach usedIds= $usedIds"
   for {set i 1} {$i <= $psgInfo(elem)} {incr i} {
      if {$psgInfo($i,inuse) == 0} {
         if {([lsearch $usedIds $i] == -1) && \
              ([lsearch $usedIds $psgInfo($i,idAt)] != -1)} {
            lappend usedIds $i
         }
      }
   }
# puts "reattach usedIds after unused search= $usedIds"
   set usedIds [sortIds $usedIds]
   foreach newId $usedIds {
      set psgInfo($newId,inuse) 1
#  puts "new tags are sum of L$newId aNd $psgInfo($psgInfo($newId,idAt),tags)"
         set psgInfo($newId,tags) \
             [concat L$newId $psgInfo($psgInfo($newId,idAt),tags)]
#  puts "reattached tags for $newId are $psgInfo($newId,tags)"
         set psgInfo($newId,chanId) [expr $psgInfo($newId,chanId) + $chanDiff]
# puts "set psgInfo($newId,chanId) to $psgInfo($newId,chanId) + $chanDiff [expr $psgInfo($newId,chanId) + $chanDiff]"
         if {$psgInfo($newId,chanId) > $numChan} {
            set psgInfo($newId,chanId) $numChan
         } elseif {$psgInfo($newId,chanId) < 1} {
            set psgInfo($newId,chanId) 1
         }
         $prog $newId
   }
# puts "final elems= $psgInfo(outList)"
}

proc detachElems2 {id} {
   global psgInfo
#  puts "call detachElem"

   set tids [.draw find withtag L$id]
#  puts "$id elems to detach ($tids) orig tags $psgInfo($id,tags)"
   set newIds {}
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
      if {[lsearch $newIds $newId] == -1} {
         lappend newIds $newId
      }
# puts "elem $i with tags $tid and id $newId"
   }
# puts "psgInfo($id,tags) $psgInfo($id,tags)"
# puts "newIds $newIds"
   foreach i $psgInfo($id,tags) {
      if {$i != "L$id"} {
         foreach newId $newIds {
            set psgInfo($newId,tags) [ldelete $psgInfo($newId,tags) $i]
# puts "tag $i removed from $newId, current tags are $psgInfo($newId,tags)"
            set oldTags [.draw gettags T$newId]
# puts "$newId delete tags $oldTags"
            foreach t $oldTags {
               if {[regexp {L[0-9]+} $t] == 1} {
                  .draw dtag T$newId $t
               }
            }
# puts "$newId add tags $psgInfo($newId,tags)"
            foreach t $psgInfo($newId,tags) {
               .draw addtag $t withtag T$newId
            }
         }
      }
   }
# puts "detach done"
#    foreach i $newIds {
#       set tid [.draw gettags T$i]
# puts "detached elem $i with tags $tid and parameter tags $psgInfo($i,tags)"
#    }
}

proc detachElems {id} {
   global psgInfo
#   puts "call detachElem $id"

   set tids [.draw find withtag L$id]
#   puts "$id elems to detach ($tids) orig tags $psgInfo($id,tags)"
   set newIds {}
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
      if {[lsearch $newIds $newId] == -1} {
         lappend newIds $newId
         set psgInfo($newId,inuse) 0
         delElem $newId
         delPhElem $newId
      }
# puts "elem $i with tags $tid and id $newId"
   }
# puts "psgInfo($id,tags) $psgInfo($id,tags)"
   set dList [ldelete $psgInfo($id,tags) L$id]
#puts "delete tags $dList"
#  puts "detach $newIds"
#  puts "hideList $psgInfo(hideList)"
   foreach newId $psgInfo(hideList) {
# puts "test hideList elem $newId"
      if {($newId != "") && ([lsearch $newIds $psgInfo($newId,idAt)] != -1)} {
         lappend newIds $newId
         set psgInfo($newId,inuse) 0
         delElem $newId
         delPhElem $newId
      }
   }
# puts "detach w/ hideList $newIds"
   foreach i $dList {
      foreach newId $newIds {
         set psgInfo($newId,tags) [ldelete $psgInfo($newId,tags) $i]
      }
      .draw dtag L$id $i
   }
#      foreach newId $newIds {
#         puts "psgInfo($newId,tags) $psgInfo($newId,tags)"
#      }
}

proc reallowDocks {id} {
   global psgInfo dockList
#puts "called reallow docks"
   if {([lsearch $psgInfo(phaseList) $psgInfo($id,idAt)] != -1) || \
       ($psgInfo($id,idAt) == "phase")} {
      return
   }
      if {$psgInfo($id,dockAt) == "end"} {
         set inum $dockList($psgInfo($id,idAt),end)
         set dockList($inum,endOk) 1
#puts "dockList($inum,endOk) $dockList($inum,endOk)"
      } elseif {$psgInfo($id,dockAt) == "true"} {
         set inum $dockList($psgInfo($id,idAt),true)
         set dockList($inum,trueOk) 1
      } elseif {$psgInfo($id,dockAt) == "false"} {
         set inum $dockList($psgInfo($id,idAt),false)
         set dockList($inum,falseOk) 1
      }
}

proc resetElem {win x y tag} {
   global dockList psgInfo numChan pInfo
   bind $win <B1-Motion> {}
   bind $win <ButtonRelease-1> {}
#  puts "resetElem x= $x y= $y tag= $tag"
   set id $psgInfo(cur)
#  puts "id $id inuse $psgInfo($id,inuse)"
   if {$y >= $psgInfo(ybot)} {
     if {$x >= $pInfo(trashX)} {
      if {[info exists pInfo(factory)] == 1} {
         set curSelect $pInfo(factory)
      }
      $win delete $tag
      deleteElems $psgInfo(cur)
      updateMeta
      drawPsg
      if {[info exists curSelect] == 1} {
         enableFactory $curSelect
      }
#      showDockList
      unsetDocker
      if {$pInfo(labels) == "on"} {
         destroyCanvasLabels
         makeCanvasLabels
      }
      return
     } elseif {$psgInfo($id,phTable) != 0} {
       set psgInfo($id,dock) front
       if {[lsearch $psgInfo(phaseList) $id] == -1} {
          if {$psgInfo(phaseList) == ""} {
             set psgInfo($id,idAt) phase
             set psgInfo($id,tags) L$id
          } else {
             set psgInfo($id,idAt) [lindex $psgInfo(phaseList) end]
             set psgInfo($id,tags) \
                 [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
          }
          set psgInfo(lastPhase) $id
       }
       set psgInfo($id,dockAt) end
#  puts "phase table inuse= $psgInfo($id,inuse)"
#puts "phaseList 2 $psgInfo(phaseList)"
        reattachElems $id 0 addPhElem
#puts "phaseList 3 $psgInfo(phaseList)"
#  puts "call unsetDocker"
       unsetDocker
#  puts "call ph resetDisplay"
       resetDisplay
#  puts "call dockSound"
       dock_sound
#  puts "done"
       return
     }
   }
#  puts "resetElem continue"
#   if {$pInfo(dropsite) > 0} {
#     .draw itemconfigure T$pInfo(dropsite) -width 1
#     set pInfo(dropsite) 0
#   }
   set ymin 10000
   for {set i 1} {$i <= $numChan} {incr i} {
      set dely [expr abs($dockList(ch$i) - $y)]
      if {$dely < $ymin} {
         set yval $i
         set ymin $dely
         set bestY $dockList(ch$i)
      }
   }
# puts "yval= $yval ymin= $ymin bestY= $bestY"
#   if {$psgInfo($id,phTable) != 0} {
#      set psgInfo($id,inuse)  0
#   }
   if {[lsearch $psgInfo(phaseList) $id] != -1} {
      set psgInfo($id,inuse) 0
   }
   if {($psgInfo($id,inuse) == 1) && ($psgInfo($id,dock) == "front")} {
      reallowDocks $id
   }
   set min $dockList(maxx)
   set len $psgInfo($id,len)
   set dist 300
   set coords [$win coords T${id}CH$yval]
#puts "coords for channel T${id}CH$yval are $coords"
   if {$coords == ""} {
      set coords [$win coords T$id]
#puts "coords for item T$id are $coords"
   }
   set itemX [lindex $coords 0]
   set itemY [expr [lindex $coords 1] + $psgInfo($id,ht)]
#puts "detach list is $psgInfo(detach)"
# puts "itemX= $itemX itemY= $itemY (y= $y)"
   set xid 1
#  puts "resetElem 2"
   for {set i 1} {$i <= $dockList(num)} {incr i} {
      if {($dockList($i,pos) == "front") && \
          ($dockList($i,trueOk) == 0)} {
# puts "skip dockList($i) $dockList($i) trueOK == 0"
         continue
      } elseif {($dockList($i,pos) == "mid") && \
          ($dockList($i,falseOk) == 0)} {
# puts "skip dockList($i) $dockList($i) falseOK == 0"
         continue
      }
      if {[lsearch $psgInfo(detach) $dockList($i,id)] == -1} {
         if {($dockList($i,pos) != "end") || ($dockList($i,endOk) == 1)} {
         set delx [expr abs($dockList($i) - $itemX)]
         set dely $ymin
         set newY $bestY
# puts "delx= $delx dely= $dely newY= $newY"
         if {[info exists psgInfo($dockList($i,id),y)] == 1} {
            set diff [expr \
               ($yval - $psgInfo($dockList($i,id),chanId)) * $psgInfo(ydelta)]
            set newY [expr $psgInfo($dockList($i,id),y) + $diff]
            set dely [expr $newY - $itemY]
            set adely [expr abs($dely)]
# puts "channel adjusted dely= $dely newY= $newY"
            if {abs($dely + $psgInfo($dockList($i,id),ht)) < $adely} {
               set dely [expr $dely + $psgInfo($dockList($i,id),ht)]
               set adely [expr abs($dely)]
            }
            if {abs($dely + $psgInfo($dockList($i,id),ht) \
                      - $psgInfo($dockList($i,id),base)) < $adely} {
               set dely [expr abs($dely + $psgInfo($dockList($i,id),ht) \
                      - $psgInfo($dockList($i,id),base))]
            }
         }
         set dxy [expr abs($delx) + abs($dely)]
#  puts "check dock delx=$delx dely=$dely"
#  puts "dist=  [expr $delx + $dely] old dist = $dist"
         if {($dxy < $dist) && ($dxy < 50)} {
#  puts "dock to $dockList($i,pos) of $i"
            set xid $i
            set d1 $dockList($i)
            set d2 [expr $dockList($i) + $len/2.0]
            set d3 [expr $dockList($i) + $len]
            set min $delx
            set ymin $dely
            set dist $dxy
            set pos "front"
            set bestY $newY
         }
         }
         set delx [expr abs($dockList($i) - $itemX - $len/2.0)]
         set dxy [expr abs($delx) + abs($dely)]
#  puts "delx=  $delx min= $min dockList($i,pos) = $dockList($i,pos)"
         if {($delx < $min) && ($dxy < $dist) && ($dockList($i,pos) == "mid")} {
            set xid $i
            set d1 [expr $dockList($i) - $len/2.0]
            set d2 $dockList($i)
            set d3 [expr $dockList($i) + $len/2.0]
            set min $delx
            set pos "mid"
         }
         if {($dockList($i,pos) != "end") || \
             ($psgInfo($dockList($i,id),style) != "if")} {
         set delx [expr abs($dockList($i) - $itemX - $len)]
         set dxy [expr abs($delx) + abs($dely)]
#  puts "delx=  $delx min= $min"
         if {($delx < $min) && ($dxy < $dist)} {
            set xid $i
            set d1 [expr $dockList($i) - $len]
            set d2 [expr $dockList($i) - $len/2.0]
            set d3 $dockList($i)
            set min $delx
            set pos "end"
         }
         }
      }
   }
   if {$dockList($xid,id) == "start"} {
      set d1 0
      set d2 [expr $len/2.0]
      set d3 $len
      set pos "front"
   }
# puts "ymin= $ymin"
#  puts "resetElem 3"
   if {$ymin > 50} {
      if {[info exists psgInfo($dockList($xid,id),y)] == 1} {
         set del [expr $psgInfo($dockList($xid,id),y) - $y]
         if {(abs($del) < 50) || \
             (abs($del + $psgInfo($dockList($xid,id),ht)) < 50)} {
            set ymin 1
         }
      }
   }
   if {(($ymin > 50) || ($min > 50)) && ($pInfo(dropsite) > 0)} {
     set tmpIds [.draw find withtag L$id]
     foreach i $tmpIds {
       set tid [.draw gettags $i]
       regexp {[^T]*T([0-9]+).*} $tid match newId
       if {$newId == $pInfo(dropsite)} {
         turnOffDropSite
         .dockBut configure -state disabled
         break
       }
     }
   }
   if {(($ymin > 50) || ($min > 50)) && ($pInfo(dropsite) == 0)} {
# puts "set idAt for $id to screen"
     $win itemconfigure T$psgInfo($id,idAt) -outline $pInfo(elemColor)
     if {$psgInfo($id,dock) == "front"} {
        reallowDocks $id
     }
     set psgInfo($id,idAt) screen
     set psgInfo($id,dock) $x
     set psgInfo($id,dockAt) $y
#  puts "resetElem 3b"
     detachElems $id
#  puts "resetElem 3c"
     resetDisplay
#  puts "resetElem 3d"
   } else {
      set oldIdAt $psgInfo($id,idAt)
      if {$pInfo(dropsite) > 0} {
       set psgInfo($id,idAt) $psgInfo($pInfo(dropsite),id)
       set psgInfo($id,dockAt) "end"
       set psgInfo($id,dock) "front"
      } else {
      set psgInfo($id,dock) $pos
      set psgInfo($id,idAt) $dockList($xid,id)
      set psgInfo($id,dockAt) $dockList($xid,pos)
      }
#puts "reset $id idAt to $psgInfo($id,idAt)"
#      set psgInfo($id,nameAt) $psgInfo($psgInfo($id,idAt),name)
#puts "dock $psgInfo($id,dock) to $psgInfo($id,dockAt) to $psgInfo($id,idAt)"
     if {($psgInfo($id,dockAt) != "end") && \
         ($psgInfo($psgInfo($id,idAt),style) == "if")} {
        set type $psgInfo($psgInfo($psgInfo($id,idAt),ifStyleAddr))
        if {$type == "True"} {
           set psgInfo($id,dockAt) true
        } elseif {$type == "False"} {
           set psgInfo($id,dockAt) false
        } elseif {abs($bestY - $itemY) < \
             abs($bestY + $psgInfo($psgInfo($id,idAt),ht) - $itemY)} {
           set psgInfo($id,dockAt) true
        } else {
           set psgInfo($id,dockAt) false
        }
     }
     if {($psgInfo($id,dockAt) != "end") && \
         ($psgInfo($psgInfo($id,idAt),style) == "while")} {
        set psgInfo($id,dockAt) true
     }
     if {$psgInfo($id,multi) == 0} {
      set chanDiff [expr $yval - $psgInfo($id,chanId)]
# puts "channels new= $yval, old= $psgInfo($id,chanId) diff= $chanDiff"
      set psgInfo($id,chanId) $yval
     } else {
      set chanDiff 0
     }
      if {($oldIdAt != $psgInfo($id,idAt)) && ($psgInfo($id,inuse) == 1)} {
# puts "old tags for $id are $psgInfo($id,tags)"
# puts "reset tags are sum of L$id and idAt tags $psgInfo($psgInfo($id,idAt),tags)"
#         set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
         detachElems $id
      }
# puts "new reset tags for $id are $psgInfo($id,tags)"
     if {$psgInfo($id,parallel) == "mid"} {
#
#         ($psgInfo($psgInfo($id,idAt),parallel) == "mid") && 
#              ($psgInfo($id,parallel) == "mid")
#        set psgInfo($id,dock) mid
#parray dockList
     }
#  puts "resetElem 4"
      if {($psgInfo($id,inuse) == 0) || ($chanDiff != 0)} {
        reattachElems $id $chanDiff addElem
      }
#  puts "resetElem 5"
      setDocker $id
      resetDisplay
      dock_sound
   }
   if {$pInfo(labels) == "on"} {
      update idletasks
      destroyCanvasLabels
      makeCanvasLabels
   }
}

proc resetElemC {win x y tag} {
  global psgInfo
  resetElem $win [$win canvasx $x] [$win canvasy $y] $tag
  turnOffDropSite
  if {$psgInfo($psgInfo(cur),inuse) <= 0} {
    .dockBut configure -state disabled
  } else {
    .dockBut configure -state normal
  }
}

proc loadComposite {win cid x y} {
   global psgInfo pElem
#   puts "called loadCompositie for id $cid ($psgInfo($cid,name))"
   set userList(name) {}
   set userList(val) {}
   for {set i 1} {$i <= $psgInfo($cid,numAttr)} {incr i} {
      if {$psgInfo($cid,S$i) == "user"} {
         lappend userList(name) $psgInfo($cid,N$i)
         lappend userList(val) $psgInfo($cid,V$i)
      }
   }
   set index $psgInfo($cid,pIndex)
# puts "add collection of $pElem(comps,$index) pulse elements"
   set oldLen $psgInfo($cid,len)
   set oldidAt $psgInfo($cid,idAt)
   set olddock $psgInfo($cid,dock)
   set olddockAt $psgInfo($cid,dockAt)
   set saveDockChan $psgInfo($cid,chanId)
   unset psgInfo($cid,ht)
   unset psgInfo($cid,len)
   delElem $cid
   set relFrontDock $pElem(frontDock,$index)
   set relFrontId $pElem(frontId,$index)
   set relMidDock $pElem(midDock,$index)
   set relMidId $pElem(midId,$index)
   set relEndDock $pElem(endDock,$index)
   set relEndId $pElem(endId,$index)
   set origCid $cid
   set midId $cid
   if {$olddock == "front"} {
      set aId $relFrontId
      set aDock $relFrontDock
   } elseif {$olddock == "mid"} {
      set aId $relMidId
      set aDock $relMidDock
   } else {
      set aId $relEndId
      set aDock $relEndDock
   }
   for {set i 1} {$i <= $pElem(comps,$index)} {incr i} {
      lappend ids $i
   }
   set ids [ldelete $ids $aId]
   set i $aId
# puts "load element [getElemIndex $pElem(comp$i,$index)]"
         loadElem [getElemIndex $pElem(comp$i,$index)]
# puts "aId is $aId"
          set psgInfo($cid,inuse) 0
          .draw delete T$cid
          addElemToPSG
          set id $psgInfo(cur)
#puts "psgInfo($id,ht) $psgInfo($id,ht)"
          set psgInfo($id,idAt) $psgInfo($cid,idAt)
          set psgInfo($id,dock) $aDock
          set psgInfo($id,dockAt) $psgInfo($cid,dockAt)
          set psgInfo($id,chanId) $saveDockChan
#   puts "changed name for id $id is $psgInfo($id,name)"
          resetCollectionInfo $id $index $i
          addElem $id
          set relId($i) $id
# puts "first idAt for psgInfo($id,idAt) is $psgInfo($id,idAt)"
          set newLen $psgInfo($id,len)
         for {set k 1} {$k <= $pElem(numInherit,$index,$i)} {incr k} {
             set ii [lsearch $userList(name) $pElem(iB$k,$index,$i)]
             if {$ii != -1} {
                for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
                   if {$pElem(iN$k,$index,$i) == $psgInfo($id,N$j)} {
                      set psgInfo($id,V$j) [lindex $userList(val) $ii]
                   }
                }
             }
         }
      set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
   set newId $pElem(relIdAt,$index,$i)
   set newIdAt $i
   set newDock $pElem(dock,$index,$i)
   set newDockAt $pElem(dockAt,$index,$i)
   while {$newId >= 1} {
         set i $newId
# puts "load element [getElemIndex $pElem(comp$i,$index)]"
         loadElem [getElemIndex $pElem(comp$i,$index)]
       
# puts "add next element"
          addElemToPSG
          resetCollectionInfo $psgInfo(cur) $index $i
          set id $psgInfo(cur)
          set relId($i) $id
          addElem $id
          set psgInfo($id,dock) $newDockAt
          set psgInfo($id,idAt) $relId($newIdAt)
# puts "idAt for psgInfo($id,idAt) is $psgInfo($id,idAt)"
          set psgInfo($id,dockAt) $newDock
          set psgInfo($id,chanId) $saveDockChan
#puts "dock $id to $psgInfo($id,idAt) - $psgInfo($id,dock) to $psgInfo($id,dockAt)"
          set newLen [expr $newLen + $psgInfo($id,len)]
         for {set k 1} {$k <= $pElem(numInherit,$index,$i)} {incr k} {
             set ii [lsearch $userList(name) $pElem(iB$k,$index,$i)]
             if {$ii != -1} {
                for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
                   if {$pElem(iN$k,$index,$i) == $psgInfo($id,N$j)} {
                      set psgInfo($id,V$j) [lindex $userList(val) $ii]
                   }
                }
             }
         }
      set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
         set newId $pElem(relIdAt,$index,$i)
         set newIdAt $i
#puts "end: set newId $newId; newIdAt= $newIdAt"
         set newDock $pElem(dock,$index,$i)
         set newDockAt $pElem(dockAt,$index,$i)
         set ids [ldelete $ids $i]
   }
   foreach i $ids {
# puts "load element [getElemIndex $pElem(comp$i,$index)]"
         loadElem [getElemIndex $pElem(comp$i,$index)]
       
# puts "add next element"
          addElemToPSG
          resetCollectionInfo $psgInfo(cur) $index $i
          set id $psgInfo(cur)
          set relId($i) $id
          addElem $id
          set psgInfo($id,dock) $pElem(dock,$index,$i)
          set psgInfo($id,idAt) $relId($pElem(relIdAt,$index,$i))
# puts "idAt for psgInfo($id,idAt) is $psgInfo($id,idAt)"
          set psgInfo($id,dockAt) $pElem(dockAt,$index,$i)
          set psgInfo($id,chanId) $saveDockChan
          set newLen [expr $newLen + $psgInfo($id,len)]
         for {set k 1} {$k <= $pElem(numInherit,$index,$i)} {incr k} {
             set ii [lsearch $userList(name) $pElem(iB$k,$index,$i)]
             if {$ii != -1} {
                for {set j 1} {$j <= $psgInfo($id,numAttr)} {incr j} {
                   if {$pElem(iN$k,$index,$i) == $psgInfo($id,N$j)} {
                      set psgInfo($id,V$j) [lindex $userList(val) $ii]
                   }
                }
             }
         }
      set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
   }
   set lenRatio [expr double($oldLen)/double($newLen)]
   foreach i [array names relId] {
      set id $relId($i)
#puts "adjust len for $id"
      set psgInfo($id,len) [expr $psgInfo($id,len) * $lenRatio]
   }
#puts "fix docks"
   foreach el $psgInfo(elems) {
      if {$psgInfo($el,idAt) == $origCid} {
        switch $psgInfo($el,dockAt) {
          front {
                   set psgInfo($el,idAt) $relId($relFrontId)
                   set psgInfo($el,dockAt) $relFrontDock
                }
          mid   {
                   set psgInfo($el,idAt) $relId($relMidId)
                   set psgInfo($el,dockAt) $relMidDock
                }
          end   {
                   set psgInfo($el,idAt) $relId($relEndId)
                   set psgInfo($el,dockAt) $relEndDock
                }
         }
      }
   }
#puts "sortIds $psgInfo(outList)"
   set ids [resortIds $psgInfo(outList)]
#puts "ids $ids"
   set psgInfo(outList) {}
   foreach id $ids {
      addElem $id
   }
}

proc replaceItem {win id x y} {
   global psgInfo pInfo
   if {($pInfo(composite) == 1) || ($pInfo(collection) == 1)} {
      return
   }
   if {$psgInfo($id,inuse) == 0} {
      return
   }
   if {($psgInfo($id,type) == "composite")} {
      foreach el $psgInfo(elems) {
         if {$psgInfo($el,idAt) == $id} {
            return
         }
      }
      loadComposite $win $id $x $y
      resetDisplay
   }
}

proc replaceItemC {win id x y} {
   replaceItem $win $id [$win canvasx $x] [$win canvasy $y]
}

proc startDetachAdjust {win id x y} {
   global pInfo
   if {$pInfo(composite) == 1} {
      return
   }
   selectDetachItem $win $id $x $y
   bind $win <B1-Motion> {drag %W [%W canvasx %x] [%W canvasy %y] L$psgInfo(cur)}
   bind $win <ButtonRelease-1> "resetElemC %W %x %y T$id"
}

proc continueAdjustment {win id} {
   global psgInfo
   set tids [.draw find withtag L$id]
# puts "elems $id to detach ($tids) orig tags $psgInfo($id,tags)"
   set newIds $id
   foreach i $tids {
      set tid [.draw gettags $i]
      regexp {[^T]*T([0-9]+).*} $tid match newId
      if {[lsearch $newIds $newId] == -1} {
         lappend newIds $newId
      }
# puts "elem $i with tags $tid and id $newId"
   }
   set psgInfo(detach) $newIds
#puts "set psgInfo(detach) $psgInfo(detach)"
   showTmpDockList
#    showDockList 1
   .draw delete Scale
#puts "set2 psgInfo(detach) $psgInfo(detach)"
   bind $win <B1-Motion> {drag %W [%W canvasx %x] [%W canvasy %y] L$psgInfo(cur)}
   bind $win <ButtonRelease-1> "resetElemC %W %x %y T$id"
}

proc monitorAdjustment {win id x y} {
   global pInfo
   if {(abs($x - $pInfo(curX)) > 4) || 
       (abs($y - $pInfo(curY)) > 4)} {
      continueAdjustment $win $id
   }
}

proc monitorAdjustmentC {win id x y} {
   monitorAdjustment $win $id [$win canvasx $x] [$win canvasy $y]
}

proc startAdjustment {win id x y} {
   global pInfo psgInfo
   if {$pInfo(composite) == 1} {
      return
   }
   if {$psgInfo($id,inuse) == 0} {
      selectDetachItem $win $id $x $y
   } else {
      selectItem $win $id $x $y
   }
   set pInfo(curX) $x
   set pInfo(curY) $y
   bind $win <B1-Motion> "monitorAdjustmentC $win $id %x %y"
   bind $win <ButtonRelease-1> "bind $win <B1-Motion> {}"
}

proc startAdjustmentC {win id x y} {
   startAdjustment $win $id [$win canvasx $x] [$win canvasy $y]
}

proc copyItem {win oldId x y} {
   global pInfo psgInfo
   if {$pInfo(composite) == 1} {
      return
   }
# puts "oldId tags are [.draw gettags T$oldId]"
   set id $psgInfo(cur)
   $win itemconfigure T$id -fill $psgInfo(bg) \
           -outline $pInfo(elemColor)
   incr psgInfo(elem)
   set id $psgInfo(elem)
   set psgInfo(cur) $id
   set psgInfo($id,id) $id
   lappend psgInfo(elems) $id
   foreach elem [array names psgInfo $oldId,*] {
      scan $elem "$oldId,%s" arr
      set psgInfo($id,$arr) $psgInfo($elem)
# puts "set psgInfo($id,$arr)= psgInfo($elem) ($psgInfo($elem))"
   }
   if {[info exists psgInfo($id,ifStyleAddr)] == 1} {
      scan $psgInfo($id,ifStyleAddr) {%d,%s} dum val
      set psgInfo($id,ifStyleAddr) $id,$val
   }
   set psgInfo($id,id) $id
   set psgInfo($id,tags) L$id
   set psgInfo($id,inuse) 0
   set coords [.draw coords T$oldId]
   set psgInfo($id,x) [lindex $coords 0]
   set psgInfo($id,y) [lindex $coords 1]
   set stipple ""
   if {$psgInfo($id,stipple) != ""} {
      set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($id,stipple)]
   }
   set bg $psgInfo(bg)
   drawItem $id 0
   $win addtag L$id withtag T$id
   selectItem $win $id $pInfo(curX) $pInfo(curY)
   select $win $pInfo(curX) $pInfo(curY) T$id
   set psgInfo(detach) $id
# puts "newId tags are [.draw gettags T$id]"
   bind $win <B1-Motion> "dragC %W %x %y T$id"
   bind $win <ButtonRelease-1> "resetElemC %W %x %y T$id"
}

proc monitorCopy {win id x y} {
   global pInfo
   if {(abs($x - $pInfo(curX)) > 4) || 
       (abs($y - $pInfo(curY)) > 4)} {
      copyItem $win $id $x $y
   }
}

proc monitorCopyC {win id x y} {
  monitorCopy $win $id [$win canvasx $x] [$win canvasy $y]
}

proc startCopyItem {win id x y} {
   global pInfo psgInfo
   if {$pInfo(composite) == 1} {
      return
   }
   if {$psgInfo($id,inuse) == 0} {
      selectDetachItem $win $id $x $y
   } else {
      selectItem $win $id $x $y
   }
   set pInfo(curX) $x
   set pInfo(curY) $y
   bind $win <B1-Motion> "monitorCopyC $win $id %x %y"
   bind $win <ButtonRelease-1> "bind $win <B1-Motion> {}"
}

proc startCopyItemC {win id x y} {
  startCopyItem $win $id [$win canvasx $x] [$win canvasy $y]
}

proc selectAnItem {win id x y} {
   global pInfo psgInfo
   if {$pInfo(composite) == 1} {
      return
   }
   if {$psgInfo($id,multi) == 0} {
      $win raise T$id
   }
   if {$psgInfo($id,inuse) == 0} {
      selectDetachItem $win $id $x $y
   } else {
      selectItem $win $id $x $y
   }
}

proc selectAnItemC {win id x y} {
  selectAnItem $win $id [$win canvasx $x] [$win canvasy $y]
}

proc selectDetachItem {win id x y} {
   global psgInfo pInfo
   if {$pInfo(composite) == 1} {
      return
   }
   if {$psgInfo(replace) == 1} {
      set psgInfo(replace) 0
      resetDisplay
   }
#puts "called detach select with id $id"
    set fcol $psgInfo(bg)
    set stipple ""
    set i $psgInfo(cur)
    if {$psgInfo($i,stipple) != ""} {
     set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($i,stipple)]
     set fcol $pInfo(elemColor)
    }
    $win itemconfigure T$i -fill $fcol -outline $pInfo(elemColor) \
       -stipple $stipple
#   $win itemconfigure T$psgInfo(cur) -fill $psgInfo(bg) -outline $pInfo(elemColor)
   if {$psgInfo($psgInfo(cur),idAt) != "start"} {
      $win itemconfigure T$psgInfo($psgInfo(cur),idAt) -outline $pInfo(elemColor)
   }
   set tids [$win gettags [$win find closest $x $y]]
   regexp {[^T]*T([0-9]+).*} $tids match id
#   scan $tid {T%d} id
   if {$psgInfo($id,style) == "if"} {
     $win itemconfigure TIF$id -fill $pInfo(selectColor) \
       -outline $pInfo(selectColor)
   } else {
     $win itemconfigure T$id -fill $pInfo(selectColor) \
       -outline $pInfo(selectColor)
   }
   set psgInfo(cur) $id
   if {$psgInfo($psgInfo(cur),idAt) != "screen"} {
      $win itemconfigure T$psgInfo($psgInfo(cur),idAt) -outline $pInfo(connectColor)
   }
   selectPsgElem $id
   select $win $x $y L$psgInfo(cur)
   setDocker $psgInfo(cur)
   if {$psgInfo(curChan) != 0} {
      .draw bind ChanLabel$psgInfo(curChan) <Leave> \
         {.draw itemconfigure current -fill black}
      .draw itemconfigure ChanLabel$psgInfo(curChan) -fill black
      set psgInfo(curChan) 0
   }
}

proc selectItem {win id x y} {
   global psgInfo pInfo
   set fcol $psgInfo(bg)
   if {$pInfo(composite) == 1} {
      return
   }
   if {$psgInfo(replace) == 1} {
      set psgInfo(replace) 0
      resetDisplay
   }
# puts "called select with id $id"
   foreach i $psgInfo(drawList) {
    set fcol $psgInfo(bg)
    set stipple ""
    if {$psgInfo($i,stipple) != ""} {
     set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($i,stipple)]
     set fcol $pInfo(elemColor)
    }
    $win itemconfigure T$i -fill $fcol -outline $pInfo(elemColor) \
       -stipple $stipple
   }
   foreach i $psgInfo(phaseList) {
    set fcol $psgInfo(bg)
    set stipple ""
    if {$psgInfo($i,stipple) != ""} {
     set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($i,stipple)]
     set fcol $pInfo(elemColor)
    }
    $win itemconfigure T$i -fill $fcol -outline $pInfo(elemColor) \
       -stipple $stipple
   }
   if {$psgInfo(cur) > 0} {
    set i $psgInfo(cur)
    set fcol $psgInfo(bg)
    set stipple ""
    if {$psgInfo($i,stipple) != ""} {
     set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($i,stipple)]
     set fcol $pInfo(elemColor)
    }
    $win itemconfigure T$i -fill $fcol -outline $pInfo(elemColor) \
       -stipple $stipple
   }
   if {$psgInfo($id,style) == "if"} {
     drawItem $id 1
     $win itemconfigure TIF$id -fill $pInfo(selectColor) \
       -outline $pInfo(selectColor) 
   } else {
     $win itemconfigure T$id -fill $pInfo(selectColor) \
       -outline $pInfo(selectColor) -stipple ""
   }
   set psgInfo(cur) $id
   if {($psgInfo($id,idAt) != "start") && ($psgInfo($id,idAt) != "screen")} {
      $win itemconfigure T$psgInfo($id,idAt) -outline $pInfo(connectColor)
   }
   .draw delete Scale
   if {([lsearch $psgInfo(phaseList) $id] == -1) &&  \
       ($psgInfo($id,idAt) != "screen")} {
      if {$psgInfo($id,dock) == "end"} {
         set endX $psgInfo($id,x)
      } else {
         set endX [expr $psgInfo($id,x) + $psgInfo($id,len)]
      }
      .draw create line $endX [expr $psgInfo(ybot)+10] $endX $psgInfo(ybot) \
            -tag "S$id Scale" -fill red -arrow last
      .draw bind S$id <Button-1> "adjustLen %W $id"
      .draw bind S$id <Control-Button-1> "adjustLen2 %W $id"
   }
   selectPsgElem $id
   select $win $x $y L$psgInfo(cur)
   setDocker $psgInfo(cur)
   if {[info exists psgInfo($id,phaseId)] == 1} {
      .draw itemconfigure T$psgInfo(lastPhaseId) \
            -outline $pInfo(elemColor)
      .draw itemconfigure T$psgInfo($id,phaseId) \
            -outline $pInfo(connectColor)
      set psgInfo(lastPhaseId) $psgInfo($id,phaseId)
   }
   if {$psgInfo(curChan) != 0} {
      .draw bind ChanLabel$psgInfo(curChan) <Leave> \
         {.draw itemconfigure current -fill black}
      .draw itemconfigure ChanLabel$psgInfo(curChan) -fill black
      set psgInfo(curChan) 0
   }
}

proc setElem2 {win x y tag index} {
   global dockList pElem psgInfo numChan pInfo
   if {($x >=$pInfo(trashX)) && ($y >= $psgInfo(ybot))} {
      $win delete $tag
      return
   }
   set xpos $x
#puts "load element $index"
   loadElem $index
   setElem $win $xpos $y $tag
   set relId {0}
   set collId $psgInfo(cur)
   if {$psgInfo($collId,idAt) == $psgInfo(lastPhase)} {
      set phaseAdd 1
      addPhElem $collId
      set psgInfo($collId,chanId) 0
   } else {
      set phaseAdd 0
      addElem $collId
   }
   if {$pElem(type,$index) == "collection"} {
# puts "add collection $pElem(elemName,$index) of $pElem(comps,$index) pulse elements"
      set collName $pElem(elemName,$index)
# puts "current psgInfo(cur) is $psgInfo(cur)"
      unset psgInfo($psgInfo(cur),ht)
      for {set i 1} {$i <= $pElem(comps,$index)} {incr i} {
# puts "load element [getElemIndex $pElem(comp$i,$index)]"
         loadElem [getElemIndex $pElem(comp$i,$index)]
         if {$i == 1} {
          changePsgInfo $psgInfo(cur)
          set psgInfo($psgInfo(cur),collection) $collName
          resetCollectionInfo $psgInfo(cur) $index $i
          if {$psgInfo($psgInfo(cur),multi) == 1} {
             set id $psgInfo(cur)
             .draw addtag T${id}CH1 withtag T$id
             if {($psgInfo($id,style) == "if")} {
               generateElem .draw $id "elemColor" \
                "T$id TIF$id TIF${id}CH1 element scl" \
                0 0 3 20
             }
          } else {
             .draw delete T$psgInfo(cur)
          }
          lappend relId $psgInfo(cur)
# puts "idAt is $psgInfo($psgInfo(cur),idAt)"
          set saveDockChan $psgInfo($psgInfo(cur),chanId)
         } else {
       
# puts "add next element"
          addElemToPSG
          set psgInfo($psgInfo(cur),collection) $collName
          resetCollectionInfo $psgInfo(cur) $index $i
          set id $psgInfo(cur)
          lappend relId $id
         set psgInfo($id,dock) $pElem(dock,$index,$i)
          set psgInfo($id,idAt) [lindex $relId $pElem(relIdAt,$index,$i)]
#          set psgInfo($id,nameAt) $pElem(nameAt,$index,$i)
          set psgInfo($id,dockAt) $pElem(dockAt,$index,$i)
          set psgInfo($id,chanId) \
              [expr $saveDockChan + $pElem(chanDiff,$index,$i)]
          if {$psgInfo($id,chanId) > $numChan} {
            set psgInfo($id,chanId) $numChan
          } elseif {$psgInfo($id,chanId) < 1} {
            set psgInfo($id,chanId) 1
          }
# puts "idAt is $psgInfo($id,idAt)"
          set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
          if {$phaseAdd == 1} {
             addPhElem $id
          } else {
             addElem $id
          }
         }
         set psgInfo($psgInfo(cur),len) $pElem(len,$index,$i)
      }
   }
   if {$psgInfo($collId,inuse) == 0} {
     if {$pElem(type,$index) == "collection"} {
     set xSave $psgInfo($collId,dock)
     set ySave $psgInfo($collId,dockAt)
     set psgInfo($collId,inuse) 1
     set psgInfo($collId,idAt) $dockList($dockList(num),id)
     set psgInfo($collId,dock) front
     set psgInfo($collId,dockAt) end
     resetDisplay
#     $win itemconfigure T$psgInfo($id,idAt) -outline $pInfo(elemColor)
     startAdjustment .draw $collId $psgInfo($collId,x) $psgInfo($collId,y)
     drag .draw $xSave $ySave L$collId
     set psgInfo($collId,idAt) screen
     set psgInfo($collId,dock) $xSave
     set psgInfo($collId,dockAt) $ySave
#  puts "setElem2 3b"
     detachElems $collId
#  puts "setElem2 3c"
     if {($y >= $psgInfo(ybot)) && \
         ($psgInfo($collId,phTable) !=  0)} {
        set psgInfo(cur) $collId
        resetElem .draw $x $y T$collId
        return
     }
     resetDisplay
     } else {
      delElem $collId
      delPhElem $collId
#puts "resetDisplay"
      resetDisplay
#puts "done resetDisplay"
     }
#   puts "collections detached"
   } else {
# puts "call resetDisplay"
      resetDisplay
      dock_sound
# puts "done resetDisplay"
   }
   if {$pInfo(labels) == "on"} {
      update idletasks
      destroyCanvasLabels
      makeCanvasLabels
   }
}

proc setElem {win x y tag} {
   global dockList psgInfo numChan pInfo
# puts "setelem x=$x y=$y"
   $win itemconfigure T$psgInfo(cur) -fill $psgInfo(bg)
   if {($psgInfo(cur) > 0) && ($psgInfo($psgInfo(cur),idAt) != "start")} {
      $win itemconfigure T$psgInfo($psgInfo(cur),idAt) -outline $pInfo(elemColor)
   }
   addElemToPSG
   set id $psgInfo(cur)
   set stipple ""
   if {$psgInfo($id,stipple) != ""} {
      set stipple @[file join $pInfo(homeDir) bitmaps $psgInfo($id,stipple)]
   }
   set bg $psgInfo(bg)
#   $win addtag T$id withtag $tag
#   $win dtag $tag
   generateElem $win $id "" "T$id element scl" \
    [expr $x-10] [expr $y-20] [expr $x+10] $y
   $win delete $tag
   bind $win <B1-Motion> {}
   bind $win <ButtonRelease-1> {}
   $win itemconfigure T$id -fill $bg
   set ymin 10000
   for {set i 1} {$i <= $numChan} {incr i} {
      set dely [expr abs($dockList(ch$i) - $y)]
      if {$dely < $ymin} {
         set yval $i
         set ymin $dely
      }
   }
   set min $dockList(maxx)
   set xid 1
# puts "min x is $min"
   for {set i 1} {$i <= $dockList(num)} {incr i} {
      if {($dockList($i,pos) == "front") && \
          ($dockList($i,trueOk) == 0)} {
         continue
      } elseif {($dockList($i,pos) == "mid") && \
          ($dockList($i,falseOk) == 0)} {
         continue
      }
      if {($dockList($i,pos) != "end") || ($dockList($i,endOk) == 1)} {
         set delx [expr abs($dockList($i) - $x + 10)]
         if {$delx < $min} {
            set xid $i
            set min $delx
         }
      }
   }
#puts "dock xmin= $min ymin= $ymin"
# puts "psgInfo($id,multi)= $psgInfo($id,multi)"
   if {$psgInfo($id,multi) != 0} {
# puts "draw multi for id $id"
      $win addtag T${id}CH$yval withtag T$id
      if {($psgInfo($id,style) == "if")} {
        generateElem .draw $id "elemColor" \
         "T$id TIF$id TIF${id}CH$yval element scl" \
         0 0 3 20
      }
      for {set i 1} {$i <= $numChan} {incr i} {
         if {$i != $yval} {
           generateElem $win $id "" "T$id T${id}CH$i element scl" \
            0 0 10 20
           if {($psgInfo($id,style) == "if")} {
             generateElem .draw $id "elemColor" \
              "T$id TIF$id TIF${id}CH$i element scl" \
              0 0 3 20
           }
         }
      }
   }
   if {($y >= $psgInfo(ybot)) && ($psgInfo($id,phTable) !=  0)} {
     set psgInfo($id,dock) front
     set psgInfo($id,dockAt) end
     set psgInfo($id,chanId) 1
     set psgInfo($id,inuse) 1
     set psgInfo($id,idAt) $psgInfo(lastPhase)
     set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
     foreach t $psgInfo($id,tags) {
        $win addtag $t withtag T$id
     }
   } elseif {(($ymin > 50) || ($min > 50)) && ($pInfo(dropsite) == 0)} {
     set psgInfo($id,dock) $x
     set psgInfo($id,dockAt) $y
     set psgInfo($id,chanId) 1
     set psgInfo($id,inuse) 0
     set psgInfo($id,idAt) screen
     set psgInfo($id,tags) L$id
     $win addtag L$id withtag T$id
#puts "setElem dettach tags for $id are $psgInfo($id,tags)"
     alterSize $id T$id \
       $x [expr $y-$psgInfo($id,ht) + $psgInfo($id,base)] \
       [expr $x+$psgInfo($id,len)] [expr $y + $psgInfo($id,base)]
     if {($psgInfo($id,style) == "if")} {
       alterSize $id TIF$id \
         $x [expr $y-$psgInfo($id,ht) + $psgInfo($id,base)] \
         [expr $x+3] [expr $y + $psgInfo($id,base)]
     }
     .draw bind T$id <Button-1> "startAdjustmentC $win $id %x %y"
     .draw bind T$id <Button-3> "selectAnItemC $win $id %x %y"
     .draw bind T$id <Shift-Button-1> "startCopyItemC .draw $id %x %y"
   } else {
     if {$pInfo(dropsite) == 0} {
       set psgInfo($id,idAt) $dockList($xid,id)
       set psgInfo($id,dockAt) $dockList($xid,pos)
     } else {
       set psgInfo($id,idAt) $psgInfo($pInfo(dropsite),id)
       set psgInfo($id,dockAt) "end"
       turnOffDropSite
       set xid -1
     }
     if {($psgInfo($id,dockAt) != "end") && \
         ($psgInfo($psgInfo($id,idAt),style) == "if")} {
        set type $psgInfo($psgInfo($psgInfo($id,idAt),ifStyleAddr))
        if {$type == "True"} {
           set psgInfo($id,dockAt) true
        } elseif {$type == "False"} {
           set psgInfo($id,dockAt) false
        } else {
           set del [expr $psgInfo($psgInfo($id,idAt),y) - $y]
           if {abs($del) < abs($del + $psgInfo($psgInfo($id,idAt),ht))} {
              set psgInfo($id,dockAt) true
           } else {
              set psgInfo($id,dockAt) false
           }
        }
     }
     if {($psgInfo($id,dockAt) != "end") && \
         ($psgInfo($psgInfo($id,idAt),style) == "while")} {
        set psgInfo($id,dockAt) true
     }
     set psgInfo($id,dock) front
     set psgInfo($id,chanId) $yval
     set psgInfo($id,tags) [concat L$id $psgInfo($psgInfo($id,idAt),tags)]
     foreach t $psgInfo($id,tags) {
        $win addtag $t withtag T$id
     }
     if {($xid >= 0) && ($dockList($xid,pos) == "mid")} {
        set psgInfo($id,dock) mid
     }
   }
}

proc ldelete {list val} {
   set ix [lsearch -exact $list $val]
   if {$ix >= 0} {
      return [lreplace $list $ix $ix]
   } else {
      return $list
   }
}

proc setDockLists {} {
   global psgInfo dockList
   set dockList(num) 1
   set dockList(1) 0
   set dockList(1,id) start
   set dockList(1,pos) end
   set dockList(start,front) 1
   set dockList(start,mid) 1
   set dockList(start,end) 1
   set dockList(1,endOk) 1
   set dockList(1,trueOk) 1
   set dockList(1,falseOk) 1
   set dockList(sort) {}
   foreach id $psgInfo(drawList) {
      set len $psgInfo($id,len)
      set curX $psgInfo($id,x)
#      if {($psgInfo($psgInfo($id,idAt),parallel) == "mid") && \
#              ($psgInfo($id,parallel) == "mid")} {
#         if {$len > $psgInfo($psgInfo($id,idAt),len)} {
#            set inum $dockList($psgInfo($id,idAt),front)
#            set dockList($inum) [expr $curX + 2.0]
#            incr inum
#            set dockList($inum) [expr $curX + $len/2.0 + 2.0]
#            incr inum
#            set dockList($inum) [expr $curX + $len + 2.0]
#         }
#         continue
#      }
      setDockList $id $curX [expr $curX + $len * $psgInfo($id,mpdisp)] [expr $curX + $len]
      if {$psgInfo($id,dock) == "front"} {
         if {$psgInfo($id,dockAt) == "end"} {
            set idAt $psgInfo($id,idAt)
            set dockList($dockList($idAt,end),endOk) 0
         } elseif {$psgInfo($id,dockAt) == "true"} {
            set idAt $psgInfo($id,idAt)
            set dockList($dockList($idAt,true),trueOk) 0
         } elseif {$psgInfo($id,dockAt) == "false"} {
            set idAt $psgInfo($id,idAt)
            set dockList($dockList($idAt,false),falseOk) 0
         }
      }
   }
}

proc setIfLens {} {
   global psgInfo numChan ifShow
# puts "setIfLens with ratio $ratio"
   set ifIds {}
   set ifTIds {}
   set ifFIds {}
   catch {unset ifShow}
   foreach id $psgInfo(drawList) {
      if {($psgInfo($id,style) == "if") || ($psgInfo($id,style) == "while")} {
         set ifTrueLen($id) 0
         set ifFalseLen($id) 0
         set ifTrueElems($id) {}
         set ifFalseElems($id) {}
         set ifIds [linsert $ifIds 0 $id]
      }
      if {$psgInfo($id,dockAt) == "true"} {
         lappend ifTIds $id
         set ifTId($id) $psgInfo($id,idAt)
# puts "set ifTId($id) $psgInfo($id,idAt)"
         set ifTrueElems($psgInfo($id,idAt)) $id
      } elseif {$psgInfo($id,dockAt) == "false"} {
         lappend ifFIds $id
         set ifFId($id) $psgInfo($id,idAt)
# puts "set ifFId($id) $psgInfo($id,idAt)"
         set ifFalseElems($psgInfo($id,idAt)) $id
      } elseif {[lsearch $ifTIds $psgInfo($id,idAt)] != -1} {
         lappend ifTIds $id
         set ifId $ifTId($psgInfo($id,idAt))
         set ifTId($id) $ifId
         lappend ifTrueElems($ifId) $id
      } elseif {[lsearch $ifFIds $psgInfo($id,idAt)] != -1} {
         lappend ifFIds $id
         set ifId $ifFId($psgInfo($id,idAt))
         set ifFId($id) $ifId
         lappend ifFalseElems($ifId) $id
      }
   }
   set psgInfo(ifList) $ifIds
# puts "ifIds   $ifIds"
# puts "ifTIds  $ifTIds"
# puts "ifFIds  $ifFIds"
# parray ifTId
#parray ifFId
# parray ifTrueElems
#parray ifFalseElems
  set ifShow(ids) {}
  set ifShow(chans) $numChan
  foreach el $ifIds {
     if {($ifTrueElems($el) != "") || ($ifFalseElems($el) != "")} {
        lappend ifShow(ids) $el
        for {set i 1} {$i <= $numChan} {incr i} {
           set ifShow($el,$i) 0
        }
        foreach i $ifTrueElems($el) {
           set ifShow($el,$psgInfo($i,chanId)) 1
        }
        foreach i $ifFalseElems($el) {
           set ifShow($el,$psgInfo($i,chanId)) 1
        }
     }
     set tLen 0
     set psgInfo($el,tId) $el
     set psgInfo($el,fId) $el
     set psgInfo($el,ifId) $el
     if {[info exists psgInfo($el,x)] == 0} continue
# puts "$el true $ifTrueElems($el)"
     foreach id $ifTrueElems($el) {
        if {[info exists psgInfo($id,x)] == 0} continue
        set testX [expr $psgInfo($id,x) + $psgInfo($id,len)]
        if {$testX > $tLen} {
           set tLen $testX
# puts "$el: maxTLen $tLen id $id"
           set psgInfo($el,tId) $id
        }
     }
     set fLen 0
# puts "$el false $ifFalseElems($el)"
     foreach id $ifFalseElems($el) {
        if {[info exists psgInfo($id,x)] == 0} continue
        set testX [expr $psgInfo($id,x) + $psgInfo($id,len)]
        if {$testX > $fLen} {
           set fLen $testX
# puts "maxFLen $fLen"
           set psgInfo($el,fId) $id
        }
     }
     if {$tLen >= $fLen} {
        set len [expr double($tLen) - $psgInfo($el,x)]
        set psgInfo($el,ifId) $psgInfo($el,tId)
#puts "if $el true length ID is $psgInfo($el,tId)"
     } else {
        set len [expr double($fLen) - $psgInfo($el,x)]
        set psgInfo($el,ifId) $psgInfo($el,fId)
#puts "if $el false length ID is $psgInfo($el,tId)"
     }
  }
}

proc findSims {} {
   global psgInfo dockPts
   set dId 0
   catch {unset dockPts}
   foreach id $psgInfo(midList) {
      set idAt $psgInfo($id,idAt)
      if {[info exists dockPts($idAt)] == 1} {
         set oldId $dockPts($idAt)
         set dockPts($id) $oldId
         lappend dockPts($oldId) $id
      } elseif {$psgInfo($idAt,parallel) == "mid"} {
         incr dId
         set dockPts($id) $dId
         set dockPts($idAt) $dId
         lappend dockPts($dId) $idAt $id
      }
   }
# parray dockPts
}

proc getSimIdAt {idAt} {
   global psgInfo dockPts
puts "getSimIdAt $idAt"
   if {[info exists dockPts($idAt)] == 1} {
      set dId $dockPts($idAt)
      set len $psgInfo($idAt,len)
      foreach el $dockPts($dId) {
         if {$psgInfo($el,len) > $len} {
puts "el $el has larger len ($psgInfo($el,len) > $len) than $idAt"
            set len $psgInfo($el,len)
            set idAt $el
         }
      }
   }
   return $idAt
}

proc setItemX {} {
   global psgInfo dPts dockList
   set curX 0
   set oldList $psgInfo(drawList)
   set coords [.draw bbox L[lindex $oldList 0]]
   set ratio $psgInfo(currentRatio)
   if {$coords != ""} {
      set max [lindex $coords 2]
# puts "coords= $coords"
      if {$max > $dockList(maxx)-5} {
#         set ratio [expr double($dockList(maxx) -6) / double($max)]
         set ratio [expr (double($dockList(maxx)) - \
              double($dockList(maxx)/10.0)) / double($max)]
      } else {
         set ratio 1.0
      }
   }
   set max 1
#  puts "ratio= $ratio currentRatio= $psgInfo(currentRatio)"
   setIfLens
   foreach id $psgInfo(ifList) {
      set idAt $psgInfo($id,ifId)
      if {$idAt == $id} continue
# puts "if $id: x+len [expr $psgInfo($id,x) + $psgInfo($id,len)]"
# puts "id $idAt: x+len [expr $psgInfo($idAt,x) + $psgInfo($idAt,len)]"
      set len [expr $psgInfo($idAt,x) + $psgInfo($idAt,len) + $psgInfo($id,defLen)]
      if {$psgInfo($id,x) + $psgInfo($id,len) < $len} {
        set len [expr $len - $psgInfo($id,x)]
        if {$psgInfo($id,len) < $len} {
           set psgInfo($id,len) $len
        }
     }
   }
   if {$ratio == $psgInfo(currentRatio)} {
   foreach id $oldList {
# puts "start $id $psgInfo($id,len)"
      set idAt $psgInfo($id,idAt)
#      if {$psgInfo($idAt,parallel) == "mid"} {
#         set idAt [getSimIdAt $idAt]
#      }
      set curX $psgInfo($idAt,x)
      if {$psgInfo($id,dockAt) == "mid"} {
         set curX [expr $curX + $psgInfo($idAt,len) * $psgInfo($idAt,mpdisp)]
      } elseif {$psgInfo($id,dockAt) == "end"} {
         set curX [expr $curX + $psgInfo($idAt,len)]
      }
# puts "id= $id len= $psgInfo($id,len) curX= $curX"
      set len $psgInfo($id,len)
      if {$psgInfo($id,dock) == "mid"} {
         set curX [expr $curX - $len * $psgInfo($id,mpdisp)]
      } elseif {$psgInfo($id,dock) == "end"} {
         set curX [expr $curX - $len]
      }
      set psgInfo($id,x) $curX
      set i $dPts($id,id)
      set dPts($i,x) [expr $curX + $psgInfo($id,len)]
      if {$dPts($i,x) > $max} {set max $dPts($i,x)}
   }
   } else {
   set psgInfo(currentRatio) [expr $psgInfo(currentRatio) * $ratio]
   foreach id $oldList {
      set idAt $psgInfo($id,idAt)
#      if {$psgInfo($idAt,parallel) == "mid"} {
#         set idAt [getSimIdAt $idAt]
#      }
      set curX $psgInfo($idAt,x)
      set psgInfo($id,len) [expr double($psgInfo($id,len)) * double($ratio)]
      if {$psgInfo($id,dockAt) == "mid"} {
         set curX [expr $curX + $psgInfo($idAt,len) * $psgInfo($idAt,mpdisp)]
      } elseif {$psgInfo($id,dockAt) == "end"} {
         set curX [expr $curX + $psgInfo($idAt,len)]
      }
# puts "id= $id len= $psgInfo($id,len) curX= $curX"
      set len $psgInfo($id,len)
      if {$psgInfo($id,dock) == "mid"} {
         set curX [expr $curX - $len * $psgInfo($id,mpdisp)]
      } elseif {$psgInfo($id,dock) == "end"} {
         set curX [expr $curX - $len]
      }
      set psgInfo($id,x) $curX
      set i $dPts($id,id)
      set dPts($i,x) [expr $curX + $psgInfo($id,len)]
      if {$dPts($i,x) > $max} {set max $dPts($i,x)}
   }
   }
   set $ratio 1.0
   if {$max > $dockList(maxx)-5} {
      set ratio [expr (double($dockList(maxx)) - \
                 double($dockList(maxx)/10.0)) / double($max)]
      set psgInfo(currentRatio) [expr $psgInfo(currentRatio) * $ratio]
   }
   foreach id $psgInfo(ifList) {
      set idAt $psgInfo($id,ifId)
      if {$idAt == $id} continue
# puts "if $id: x+len [expr $psgInfo($id,x) + $psgInfo($id,len)]"
# puts "id $idAt: x+len [expr $psgInfo($idAt,x) + $psgInfo($idAt,len)]"
      set len [expr $psgInfo($idAt,x) + $psgInfo($idAt,len) + $psgInfo($id,defLen)]
      if {$psgInfo($id,x) + $psgInfo($id,len) < $len} {
        set len [expr $len - $psgInfo($id,x)]
        if {$psgInfo($id,len) < $len} {
           set psgInfo($id,len) $len
        }
     }
   }
   foreach id $oldList {
      set idAt $psgInfo($id,idAt)
      set curX $psgInfo($idAt,x)
      set psgInfo($id,len) [expr double($psgInfo($id,len)) * double($ratio)]
      if {$psgInfo($id,dockAt) == "mid"} {
         set curX [expr $curX + $psgInfo($idAt,len) * $psgInfo($idAt,mpdisp)]
      } elseif {$psgInfo($id,dockAt) == "end"} {
         set curX [expr $curX + $psgInfo($idAt,len)]
      } elseif {$psgInfo($idAt,shape) == "loop"} {
        set curX [expr $curX + 5]
      }
# puts "id= $id len= $psgInfo($id,len) curX= $curX"
      set len $psgInfo($id,len)
      if {$psgInfo($id,dock) == "mid"} {
         set curX [expr $curX - $len * $psgInfo($id,mpdisp)]
      } elseif {$psgInfo($id,dock) == "end"} {
         set curX [expr $curX - $len]
      }
      set psgInfo($id,x) $curX
      set i $dPts($id,id)
      set dPts($i,x) [expr $curX + $psgInfo($id,len)]
   }
   set sortX {}
   foreach id $psgInfo(drawList) {
      lappend sortX "$id $psgInfo($id,x)"
   }
   set sortX [lsort -index end -real $sortX]
# puts $sortX
   set psgInfo(sortX) {}
   foreach el $sortX {
      lappend psgInfo(sortX) [lindex $el 0]
   }
   foreach id $psgInfo(phaseList) {
      lappend psgInfo(sortX) $id
   }
# puts $psgInfo(sortX)
}

proc setAnchors {} {
   global psgInfo dockList dPts
   lappend mainList "start"
   set index 0
   catch {unset dPts}
   set dPts(num) 0
   set dPts(start,id) 0
   set dPts(0,x) 0
   set firstList {}
   set nextList {}
# puts "start setAnchors2  $psgInfo(outList)"
   set psgInfo(midList) {}
   set psgInfo(hideList) {}
   set psgInfo(drawList) {}
   foreach id $psgInfo(outList) {
      set idAt $psgInfo($id,idAt)
#puts "id $id idAt $idAt dock $psgInfo($id,dock) dockAt $psgInfo($id,dockAt)"
      if {$psgInfo($id,style) == "if"} {
# puts "if style with type= $psgInfo($id,ifStyleAddr) $psgInfo($psgInfo($id,ifStyleAddr))"
         set type $psgInfo($psgInfo($id,ifStyleAddr))
         if {$type == "True"} {
            set psgInfo($id,base) 5
            set psgInfo($id,ht) 5
            if {$psgInfo($id,falseId) != ""} {
               lappend psgInfo(hideList) $psgInfo($id,falseId)
            }
         } elseif {$type == "False"} {
            set psgInfo($id,base) 5
            set psgInfo($id,ht) 5
            if {$psgInfo($id,trueId) != ""} {
               lappend psgInfo(hideList) $psgInfo($id,trueId)
            }
         } else {
            set psgInfo($id,base) 15
            set psgInfo($id,ht) 60
         }
# puts "hideList = $psgInfo(hideList)"
      }
      if {($psgInfo($id,dockAt) == "end") && \
          ($psgInfo($id,dock) == "front") && \
           ([lsearch $mainList $idAt] != -1)} {
         incr index
# puts "prime $index  id= $psgInfo($id,id) idAt= $psgInfo($id,idAt)"
         set chanId $psgInfo($id,chanId)
         set idAtIndex $dPts($idAt,id)
         set dPts($index) $id  
         set dPts($id,id) $index
         set dPts($id,front) $idAtIndex
         set dPts($id,end) $index
         lappend mainList $id
         set dPts($index,$chanId,y) \
               [expr $dockList(ch$chanId) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
         set pInfo($id,y) $dPts($index,$chanId,y)
         set psgInfo($id,y) $dPts($index,$chanId,y)
         lappend psgInfo(drawList) $id
         if {$psgInfo($id,parallel) == "mid"} {
            lappend psgInfo(midList) $id
         }
      } elseif {[lsearch $psgInfo(hideList) $idAt] == -1} {
         if {[lsearch $psgInfo(hideList) $id] == -1} {
            if {([lsearch $mainList $idAt] != -1) || \
                ([lsearch $firstList $idAt] != -1)} {
               lappend firstList $id
            } else {
               lappend nextList $id
            }
            lappend psgInfo(drawList) $id
            if {$psgInfo($id,parallel) == "mid"} {
               lappend psgInfo(midList) $id
            }
         }
      } else {
         if {[lsearch $psgInfo(hideList) $id] == -1} {
            lappend psgInfo(hideList) $id
         }
      }
   }
#   set psgInfo(hideList) [lreplace $psgInfo(hideList) 0 0]
#puts "psgInfo(hideList) $psgInfo(hideList)"
   set dPts(prime) $index
# puts "dPts(prime) $dPts(prime)"
# puts "firstList $firstList"
# puts "nextList $nextList"
   set newList [concat $firstList $nextList]
   while {[llength $nextList] > 0} {
      set id [lindex $nextList 0]
      set idAt $psgInfo($id,idAt)
      set index [lsearch $firstList $idAt]
# puts "id= $id idAt= $idAt index= $index"
      if {$index == -1} {
         lappend nextList $id
      } else {
         lappend firstList $id
      }
      set nextList [lreplace $nextList 0 0]
   }
#if {$newList != $firstList} {
#puts "probably error"
#puts "sorted newList $newList"
# puts "firstList $firstList"
#}
   foreach id $firstList {
      set idAt $psgInfo($id,idAt)
      set chanId $psgInfo($id,chanId)
      set idAtIndex $dPts($idAt,id)
      incr index
      set dPts($index) $id  
      set dPts($id,id) $index
      if {(($psgInfo($id,dockAt) == "end") && ($psgInfo($id,dock) == "front")) \
        || (($psgInfo($id,dock) == "end") && ($psgInfo($id,dockAt) == "front"))} {
# puts "f/e"
         set dPts($id,front) $idAtIndex
         set dPts($id,end) $index
            if {[info exists dPts($idAtIndex,$chanId,y)] == 1} {
# puts "dPts($index,$chanId,y)= $dPts($idAtIndex,$chanId,y) + $psgInfo($idAt,ht) - $psgInfo($idAt,base) - $psgInfo($id,ht) + $psgInfo($id,base)"
               set dPts($index,$chanId,y) \
                  [expr $dPts($idAtIndex,$chanId,y) \
                     + $psgInfo($idAt,ht) - $psgInfo($idAt,base) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
            } else {
               set atChan $psgInfo($idAt,chanId)
               set off [expr $dPts($idAtIndex,$atChan,y) - $dockList(ch$atChan)]
# puts "atChan= $atChan off= $dPts($idAtIndex,$atChan,y) - $dockList(ch$atChan) = $off"
               set dPts($index,$chanId,y) \
               [expr $dockList(ch$chanId) + $off \
                     + $psgInfo($idAt,ht) - $psgInfo($idAt,base) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
            }
         set pInfo($id,y) $dPts($index,$chanId,y)
      } elseif {$psgInfo($id,dock) == "front"} {
# puts "f"
            if {[info exists dPts($idAtIndex,$chanId,y)] == 1} {
# puts "f exists dPts($idAtIndex,$chanId,y) $dPts($idAtIndex,$chanId,y)"
               set dPts($index,$chanId,y) \
                  [expr $dPts($idAtIndex,$chanId,y) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
               if {$psgInfo($id,dockAt) == "false"} {
# puts "ff"
                  set dPts($index,$chanId,y) \
                     [expr $dPts($index,$chanId,y) + $psgInfo($idAt,ht) - 3]
               }
            } else {
# puts "f not exists dPts($idAtIndex,$chanId,y)"
               set atChan $psgInfo($idAt,chanId)
               set off [expr $dPts($idAtIndex,$atChan,y) - $dockList(ch$atChan)]
               set dPts($index,$chanId,y) \
               [expr $dockList(ch$chanId) + $off \
                     + $psgInfo($idAt,ht) - $psgInfo($idAt,base) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
          if {$psgInfo($id,dockAt) == "true"} {
# puts "ft"
               set dPts($index,$chanId,y) \
                  [expr $dPts($index,$chanId,y) \
                     - $psgInfo($idAt,ht) + $psgInfo($idAt,base)]
          } elseif {$psgInfo($id,dockAt) == "false"} {
# puts "ff"
               set dPts($index,$chanId,y) \
                  [expr $dPts($index,$chanId,y) \
                      + $psgInfo($idAt,base) - 3]
          }
            }
      } else {
# puts "e"
            if {[info exists dPts($idAtIndex,$chanId,y)] == 1} {
# puts "f exists dPts($idAtIndex,$chanId,y) $dPts($idAtIndex,$chanId,y)"
               set dPts($index,$chanId,y) \
                  [expr $dPts($idAtIndex,$chanId,y) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
            } else {
# puts "f not exists dPts($idAtIndex,$chanId,y)"
               set atChan $psgInfo($idAt,chanId)
               set off [expr $dPts($idAtIndex,$atChan,y) - $dockList(ch$atChan)]
               set dPts($index,$chanId,y) \
               [expr $dockList(ch$chanId) + $off \
                     + $psgInfo($idAt,ht) - $psgInfo($idAt,base) \
                     - $psgInfo($id,ht) + $psgInfo($id,base)]
            }
      }
         set pInfo($id,y) $dPts($index,$chanId,y)
         set psgInfo($id,y) $dPts($index,$chanId,y)
   }
   set dPts(num) $index
#   findSims
#  parray dPts
#  parray pInfo
}

proc unHide {id} {
   global psgInfo
   while {($psgInfo($id,dockAt) != "true") && \
          ($psgInfo($id,dockAt) != "false")} {
      set id $psgInfo($id,idAt)
  }
  set id $psgInfo($id,idAt)
  set psgInfo($psgInfo($id,ifStyleAddr)) Both
  if {[lsearch $psgInfo(hideList) $id] != -1} {
     unHide $id
  }
}

proc addPhElem {id} {
   global psgInfo
# puts "addPhElem $id"
   set i [lsearch $psgInfo(phaseList) $id]
   if {$i == -1} {
      lappend psgInfo(phaseList) $id
   }
   delElem $id
   set psgInfo($id,chanId) 0
   .draw dtag T$id scl
}

proc delPhElem {id} {
   global psgInfo pInfo
# puts "delPhElem $id"
   if {$pInfo(dropsite) == $id} {
     turnOffDropSite
   }
   set i [lsearch $psgInfo(phaseList) $id]
   if {$i == -1} return
   set psgInfo(phaseList) [lreplace $psgInfo(phaseList) $i $i]
   .draw addtag scl withtag T$id
}

proc addElem {id} {
   global psgInfo
# puts "addElem $id"
   if {[lsearch $psgInfo(outList) $id] == -1} {
      lappend psgInfo(outList) $id
   }
   delPhElem $id
}

proc delElem {id} {
   global psgInfo pInfo
# puts "delElem $id"
   if {$pInfo(dropsite) == $id} {
     turnOffDropSite
     .dockBut configure -state disabled
   }
   set i [lsearch $psgInfo(outList) $id]
   if {$i == -1} return
   set psgInfo(outList) [lreplace $psgInfo(outList) $i $i]
}

proc drawPsg {} {
   global psgInfo ifShow
   foreach id $psgInfo(outList) {
      if {$psgInfo($id,style) == "if"} {
         set psgInfo($id,trueId) {}
         set psgInfo($id,falseId) {}
      }
      if {$psgInfo($id,dockAt) == "true"} {
         set psgInfo($psgInfo($id,idAt),trueId) $id
      } elseif {$psgInfo($id,dockAt) == "false"} {
         set psgInfo($psgInfo($id,idAt),falseId) $id
      }
   }
#  puts "call setAnchors"
   setAnchors
# puts "call set XY "
   setItemX
   setItemX
# puts "done setting XY "
   setDockLists
#puts "3"
   foreach id $psgInfo(drawList) {
     drawItem $id 1
# puts "$id tags parameter is $psgInfo($id,tags)"
     if {$psgInfo($id,labelId) != 0} {
        set psgInfo($id,labelVal) $psgInfo($id,V$psgInfo($id,labelId))
     }
     set oldTags [.draw gettags T$id]
     set newTags {}
     foreach t $psgInfo($id,tags) {
        set ix [lsearch -exact $oldTags $t]
        if {$ix >= 0} {
           set oldTags [lreplace $oldTags $ix $ix]
        } else {
           lappend newTags $t
        }
     }
     foreach t $oldTags {
        if {[regexp {L[0-9]+} $t] == 1} {
           .draw dtag T$id $t
        }
     }
     foreach t $newTags {
        .draw addtag $t withtag T$id
     }
#  puts "$id tags on element are [.draw gettags T$id]"
   }
   foreach id $psgInfo(hideList) {
      .draw delete T$id
   }
   foreach id $ifShow(ids) {
     if {$id != $psgInfo(cur)} {
      for {set i 1} {$i <= $ifShow(chans)} {incr i} {
        if {$ifShow($id,$i) == 0} {
           .draw delete TIF${id}CH$i
           .draw delete T${id}CH$i
        }
      }
     }
   }
#puts "4"
   showDockList
   set x $psgInfo(xPhase)
   set y $psgInfo(yPhase)
   set psgInfo(lastPhase) phase
# puts "psgInfo(phaseList) $psgInfo(phaseList)"
   foreach id $psgInfo(phaseList) {
     set psgInfo(lastPhase) $id
     set psgInfo($id,x) $x
     set psgInfo($id,y) $y
     set x [expr $x + $psgInfo($id,len)]
#puts "draw $id at $x, $y"
     drawItem $id 1
# puts "$id tags parameter is $psgInfo($id,tags)"
     .draw dtag T$id scl
     set oldTags [.draw gettags T$id]
     foreach t $oldTags {
        if {[regexp {L[0-9]+} $t] == 1} {
           .draw dtag T$id $t
        }
     }
     foreach t $psgInfo($id,tags) {
        .draw addtag $t withtag T$id
     }
#  puts "$id tags on element are [.draw gettags T$id]"
   }
   foreach id $psgInfo(ifList) {
      .draw raise T$id channel
   }
#puts "done drawPSG"
# puts "elements [.draw find all]"
}

proc redrawPsg {id} {
}

proc dragAnchor3 {win x id} {
   global dPts psgInfo dockList pInfo
# puts "dragAnchor3 x= $x id= $id"
# puts "psgInfo($id,x)= $psgInfo($id,x) len= $psgInfo($id,len)"
   if {$psgInfo($id,dock) == "end"} {
      set oldX $psgInfo($id,x)
   } else {
      set oldX [expr $psgInfo($id,x) + $psgInfo($id,len)]
   }
# puts "dragAnchor3 x= $x oldX= $oldX"
   set diff [expr $x - $oldX]
   if {abs($diff) < 2} {
      return
   }
   if {$psgInfo($id,dock) == "end"} {
      set psgInfo($id,len) [expr $psgInfo($id,x) + $psgInfo($id,len) - $x]
   } else {
      set psgInfo($id,len) [expr $x - $psgInfo($id,x)]
   }
   if {$psgInfo($id,len) < 3} {
      set psgInfo($id,len) 3
   }
# puts "new len= $psgInfo($id,len)"
   setItemX
   setDockLists
   foreach i $psgInfo(drawList) {
     redrawItem $i
   }
   showDockList
# puts "adjusted x= $psgInfo($id,x) len= $psgInfo($id,len)"
# puts "diff=  $diff new diff= [expr $x - ($psgInfo($id,x) + $psgInfo($id,len))]"
   if {$psgInfo($id,dock) == "end"} {
      .draw move S$id [expr $diff - $x + $psgInfo($id,x)] 0
   } else {
      .draw move S$id [expr $diff - $x + ($psgInfo($id,x)+$psgInfo($id,len))] 0
   }
}

proc dragAnchor3C {win x id} {
   dragAnchor3 $win [$win canvasx $x] $id
}

proc adjustLen {win index} {
   $win bind S$index <B1-Motion> "dragAnchor3C %W %x $index"
   $win bind S$index <ButtonRelease-1> {
      if {$pInfo(labels) == "on"} {
         destroyCanvasLabels
         makeCanvasLabels
      }
   }
}

proc adjustLen2 {win index} {
   $win bind S$index <B1-Motion> "dragAnchor3C %W %x $index"
   $win bind S$index <ButtonRelease-1> "drawPsg"
}

proc stdDisplay {} {
   global psgInfo
   foreach id $psgInfo(outList) {
      set psgInfo($id,len) [expr $psgInfo($id,defLen) * $psgInfo(currentRatio)]
   }
   set psgInfo(currentRatio) 1
   resetDisplay
   resetDisplay
   if {[.mbar.labels.menu entrycget 1 -label] == "Update"} {
      destroyCanvasLabels
      makeCanvasLabels
   }
}

proc prevElem {} {
   global psgInfo
# puts "prevElem psgInfo(cur) is $psgInfo(cur)"
   set index [lsearch $psgInfo(sortX) $psgInfo(cur)]
# puts "index= $index"
   if {$index > 0} {
      set id [lindex $psgInfo(sortX) [expr $index-1]]
      set coords [.draw coords T$id]
      selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1] 
   }
}

proc nextElem {} {
  global psgInfo
# puts "nextElem psgInfo(cur) is $psgInfo(cur)"
   set index [lsearch $psgInfo(sortX) $psgInfo(cur)]
   set num [llength $psgInfo(sortX)]
# puts "index= $index num= $num"
# puts "nextElem cur= $psgInfo(cur) index= $index num= $num"
   if {($index+1) < $num} {
      set id [lindex $psgInfo(sortX) [expr $index+1]]
      set coords [.draw coords T$id]
      selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1] 
   }
}

proc prevDock {} {
   global psgInfo pInfo
   if {$psgInfo($psgInfo(cur),idAt) != "start"} {
      set xid $psgInfo($psgInfo(cur),idAt)
      set xid $psgInfo($xid,idAt)
      set psgInfo($psgInfo(cur),idAt) $xid
      resetDisplay
   }
}

proc nextDock {} {
   global psgInfo pInfo dockList

   set num [llength $dockList(sort)]
   set id $psgInfo(cur)
   set idAt $psgInfo($id,idAt)
   set inum $dockList($idAt,$psgInfo($id,dockAt))
   set i [lsearch $$dockList "$dockList($inum) $inum"]
   incr i
   while {$i < $num} {
      set val [lindex $dockList $i]
      set newInum [lindex $val 1]
   }

#   dockList($inum) $d1
#   dockList($inum,id) $id
#   dockList($inum,pos) front
#   dockList($id,front) $inum
#   dockList($id,true) $inum
#   dockList($id,false) $inum

#   if {$psgInfo($psgInfo(cur),idAt) == "start"} {
#      set xid 1
#   } else {
#      set xid [expr $psgInfo($psgInfo(cur),idAt) + 1]
#   }
#   if {$xid == $psgInfo(cur)} {
#      incr xid
#   }
#      if {$xid > $psgInfo(elem)} {
#         return
#      }
#      
#      if {$psgInfo($xid,idAt) == $psgInfo(cur)} {
#         set psgInfo($xid,idAt) $psgInfo($psgInfo(cur),idAt)
#      }
#      set psgInfo($psgInfo(cur),idAt) $xid
#      resetDisplay
}

proc updateDisplay {} {
  global psgInfo pInfo
  if {[info exists pInfo(factory)] == 1} {
     set curSelect $pInfo(factory)
  }
  set id $psgInfo(cur)
  lappend psgInfo(outList) $id
  lappend psgInfo(drawList) $id
  drawPsg
  if {[info exists psgInfo($id,id)] == 1} {
     set coords [.draw coords T$id]
     if {$coords != {}} {
        selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1] 
     }
  }
  if {[info exists curSelect] == 1} {
     enableFactory $curSelect
  }
}

proc resetDisplay {} {
  global psgInfo pInfo
  if {[info exists pInfo(factory)] == 1} {
     set curSelect $pInfo(factory)
  }
# puts "cur= $psgInfo(cur)"
  set id $psgInfo(cur)
  updateMeta
# puts "new cur= $id"
  drawPsg
# puts "done drawPsg"
  if {[info exists psgInfo($id,id)] == 1} {
     set coords [.draw coords T$id]
     if {$coords != {}} {
        selectAnItem .draw $id [lindex $coords 0] [lindex $coords 1] 
     }
  }
  if {[info exists curSelect] == 1} {
     enableFactory $curSelect
  }
#   foreach el $psgInfo(elems) {
#      puts "RD $el id= $psgInfo($el,id)  idAt= $psgInfo($el,idAt)"
#   }
}

proc setElemDock {} {
  global dElem psgInfo
  set id $psgInfo(cur)
  set psgInfo($id,dock) $dElem(dock)
  if {($psgInfo($psgInfo($id,idAt),parallel) == "mid") && \
              ($psgInfo($id,parallel) == "mid")} {
     if {$psgInfo($id,dock) == "mid"} {
        set psgInfo($id,dockAt) mid
     } elseif {$psgInfo($id,dockAt) == "mid"} {
        set psgInfo($id,dockAt) $psgInfo($id,dock)
     }
  }
  resetDisplay
}

proc setDock {} {
  global dElem psgInfo pInfo
  set psgInfo($psgInfo(cur),dockAt) $dElem(dockAt)
#puts "setDock $dElem(dockAt)"
  if {$dElem(dockAt) != "mid"} {
    if {$psgInfo($psgInfo(cur),dock) == "mid"} {
#       set psgInfo($psgInfo(cur),dock) front
    }
  }
  resetDisplay
}

proc setChanDock {} {
  global dElem psgInfo
  set psgInfo($psgInfo(cur),chanId) $dElem(chanId)
}

proc unsetDocker {} {
   global dElem
   set dElem(name) ""
   set dElem(dock) ""
   set dElem(dockAt) ""
   set dElem(nameAt) ""
   .d1 configure -state disabled
   .d2 configure -state disabled
   .d3 configure -state disabled
   .t1 configure -state disabled
   .t2 configure -state disabled
   .t3 configure -state disabled
}

proc setDocker {id} {
   global dElem psgInfo dockList
   if {($psgInfo($id,idAt) == "screen") || \
        ($psgInfo($id,idAt) == "phase")} {
      unsetDocker
      return
   }

   set d1state normal
   set t3state normal
   if {($psgInfo($id,dock) == "front") && \
       ($psgInfo($id,dockAt) != "end") && \
       ($dockList($dockList($psgInfo($id,idAt),end),endOk) == 0)} {
# puts "setDocker dockList($dockList($psgInfo($id,idAt),end),endOk) $dockList($dockList($psgInfo($id,idAt),end),endOk) "
      set t3state disabled
   }
   if {($psgInfo($id,dock) == "end") && \
       ($psgInfo($id,dockAt) == "end") && \
       ($dockList($dockList($psgInfo($id,idAt),end),endOk) == 0)} {
      set d1state disabled
   }
   .d1 configure -state $d1state
   if {$psgInfo($id,parallel) != "no"} {
      .d2 configure -state normal
   } else {
      .d2 configure -state disabled
   }
   .d3 configure -state normal
   if {$psgInfo($id,idAt) == "start"} {
      .d2 configure -state disabled
      .d3 configure -state disabled
      .t1 configure -state disabled
      .t2 configure -state disabled
      .t3 configure -state disabled
   } elseif {$psgInfo($psgInfo($id,idAt),style) == "if"} {
      .d2 configure -state disabled
      if {($dockList($dockList($psgInfo($id,idAt),true),trueOk) == 0) && \
          ($psgInfo($id,dockAt) != "true")} {
         .t1 configure -state disabled
      } else {
         .t1 configure -state normal -text True -value true
      }
      if {($dockList($dockList($psgInfo($id,idAt),false),falseOk) == 0) && \
          ($psgInfo($id,dockAt) != "false")} {
         .t2 configure -state disabled
      } else {
         .t2 configure -state normal -text False -value false
      }
      if {$t3state == "disabled"} {
         .t3 configure -state $t3state
      } elseif {$psgInfo($id,dock) == "end"} {
         .t1 configure -state normal -text Front -value front
         if {$dElem(dockAt) != "front"} {
            set dElem(dockAt) front
            after idle {.t1 invoke}
         }
         .t2 configure -state disabled
         .t3 configure -state disabled
      } else {
         .t3 configure -state normal -text End -value end
      }
      if {($psgInfo($id,dock) == "front") && \
          ($psgInfo($id,dockAt) == "front")} {
         after idle {.t3 invoke; .t2 invoke; .t1 invoke}
      }
   } elseif {$psgInfo($psgInfo($id,idAt),style) == "while"} {
      .d2 configure -state disabled
      if {($dockList($dockList($psgInfo($id,idAt),true),trueOk) == 0) && \
          ($psgInfo($id,dockAt) != "true")} {
         .t1 configure -state disabled
      } else {
         .t1 configure -state normal -text True -value true
      }
      .t2 configure -state disabled
      if {$t3state == "disabled"} {
         .t3 configure -state $t3state
      } elseif {$psgInfo($id,dock) == "end"} {
         .t1 configure -state normal -text Front -value front
         if {$dElem(dockAt) != "front"} {
            set dElem(dockAt) front
            after idle {.t1 invoke}
         }
         .t2 configure -state disabled
         .t3 configure -state disabled
      } else {
         .t3 configure -state normal -text End -value end
      }
      if {($psgInfo($id,dock) == "front") && \
          ($psgInfo($id,dockAt) == "front")} {
         after idle {.t3 invoke; .t2 invoke; .t1 invoke}
      }
   } elseif {($psgInfo($psgInfo($id,idAt),parallel) == "mid") && \
              ($psgInfo($id,parallel) == "mid")} {
      if {$psgInfo($id,dock) == "mid"} {
         .t1 configure -state disabled
         .t2 configure -state normal -text Middle -value mid
         .t3 configure -state disabled
      } else {
         .t1 configure -state normal -text Front -value front
         .t2 configure -state disabled
         .t3 configure -state $t3state
     }
   } elseif {[lsearch $psgInfo(phaseList) $id] != -1} {
      .d2 configure -state disabled
      .d3 configure -state disabled
      .t1 configure -state disabled
      .t2 configure -state disabled
      .t3 configure -state normal
   } else {
      .t1 configure -state normal -text Front -value front
      if {$psgInfo($psgInfo($id,idAt),parallel) != "no"} {
         .t2 configure -state normal -text Middle -value mid
      } else {
         .t2 configure -state disabled -text Middle
      }
      .t3 configure -state $t3state
   }
   set dElem(id) $id
   set dElem(name) $psgInfo($id,label)
   set dElem(dock) $psgInfo($id,dock)
   set dElem(idAt) $psgInfo($id,idAt)
   set dElem(dockAt) $psgInfo($id,dockAt)
   set dElem(nameAt) $psgInfo($psgInfo($id,idAt),label)
   set dElem(chanId) $psgInfo($id,chanId)
   if {$psgInfo($id,inuse) == 1} {
      .dockBut configure -state normal
   } else {
      turnOffDropSite
      .dockBut configure -state disabled
   }
}

proc enableFactory {label} {
   global pInfo
#   if {[info exists pInfo(factory,disabled)] == 1} {
#      return
#   }
#   set pInfo(factory) $label
   set pInfo(elName) $label
   set pInfo(elIndex) [lsearch [.list2 get 0 end] $label]
}

proc reenableFactory {label} {
#   global pInfo psgInfo trashcan
#   catch {unset pInfo(factory,disabled)}
#   .draw create bitmap 500 [expr $psgInfo(ybot) + 8] \
#      -bitmap $trashcan -anchor nw -foreground red4 \
#      -background "" -tag {deleter factoryItem}
#   enableFactory $label
}

proc disableFactory {} {
#   global pInfo
#   set pInfo(factory,disabled) {}
#   .draw delete factoryItem
}

proc docker {win} {
   global dElem psgInfo trashcan pInfo
   frame $win.fr1
   frame $win.fr2
   frame $win.fr3
   frame $win.fr4
   label .lab1 -text "Dock " -width 6 -anchor w
   radiobutton .d1  -text Front -variable dElem(dock) -value front \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setElemDock -width 6
   radiobutton .d2  -text Middle -variable dElem(dock) -value mid \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setElemDock -width 6
   radiobutton .d3  -text End -variable dElem(dock) -value end \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setElemDock -width 6
   label .lab2 -text "of   " -width 6 -anchor w
   label .lab3 -textvariable dElem(name)
   label .lab4 -text "to   " -width 6 -anchor w
   button .dPrev -text "<=" -command "prevElem" -padx 0 -pady 0
   button .dNext -text "=>" -command "nextElem" -padx 0 -pady 0
   label .lab5 -text "of    " -width 6 -anchor w
   label .lab6 -textvariable dElem(nameAt)
   button .dPrevDock -text "<=" -command "prevDock" -padx 0 -pady 0
   button .dNextDock -text "=>" -command "nextDock" -padx 0 -pady 0
   radiobutton .t1  -text Front -variable dElem(dockAt) -value front \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDock -width 6
   radiobutton .t2  -text Middle -variable dElem(dockAt) -value mid \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDock -width 6
   radiobutton .t3  -text End -variable dElem(dockAt) -value end \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command setDock -width 6
   checkbutton .dockBut -text "Dock to this" -command setDropSite\
            -variable pInfo(dropsite,on) \
            -onvalue 1 -offvalue 0
   pack .lab1 .d1 .d2 .d3 -in $win.fr1 -side left -anchor w -pady 3
   pack .lab2 .lab3 -in $win.fr2 -side left -anchor w
   pack .dNext .dPrev -in $win.fr2 -side right -anchor e
   pack .lab4 .t1 .t2 .t3 -in $win.fr3 -side left -anchor w -fill x -pady 3
   pack .dockBut -in $win.fr3 -side right -anchor e
   pack .lab5 .lab6 -in $win.fr4 -side left -anchor w
#   pack .dNextDock .dPrevDock -in $win.fr4 -side right -anchor e
   pack $win.fr1 $win.fr2 $win.fr3 $win.fr4 -side top -anchor w -fill x
   frame .factoryLabel
   label .factLabel -text "Select Component" -width 50 -anchor w
   label .factLabel2 -text "Remove Component"
   pack .factLabel -side left -in .factoryLabel -anchor w
   pack .factLabel2 -side right -in .factoryLabel -anchor e
   set psgInfo(bg) [.draw cget -bg]
   unsetDocker
   .dockBut configure -state disabled
   bind . <Key-Down>  {prevElem}
   bind . <Key-Up>  {nextElem}
}
