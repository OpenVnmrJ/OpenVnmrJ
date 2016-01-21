#!/vnmr/tcl/bin/tclsh
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

proc ldelete {list val} {
   set ix [lsearch -exact $list $val]
   if {$ix >= 0} {
      return [lreplace $list $ix $ix]
   } else {
      return $list
   }
}

proc setError {id quit msg} {
   global errorFd outName seqInfo env seqName
   if {$errorFd == -1} {
      set errorFd [open [file join $env(vnmruser) seqlib $outName.error] w]
   }
   puts $errorFd "Error occurred at line 0 column 0 for PSG Element \"$seqInfo($id,name)\" Id \"$seqInfo($id,errorId)\" in file \"$seqName\""
   puts $errorFd $msg
   if {$quit != 0} {
      exit $quit
   }
}

proc copyOutToOut {fromIndex toIndex} {
   global outInfo
# puts "copyOutToOut from $fromIndex to $toIndex"
   set outInfo($toIndex,names) {}
   set outInfo($toIndex,name) $outInfo($fromIndex,name)
   set outInfo($toIndex,type) $outInfo($fromIndex,type)
   set outInfo($toIndex,errorId) $outInfo($fromIndex,errorId)
   set outInfo($toIndex,number) $outInfo($fromIndex,number)
   set outInfo($toIndex,origIndex) $outInfo($fromIndex,origIndex)
   set outInfo($toIndex,attr) $outInfo($fromIndex,attr)
   set outInfo($toIndex,drt,index) $outInfo($fromIndex,drt,index) 
   set outInfo($toIndex,chanId,index) $outInfo($fromIndex,chanId,index) 
   set outInfo($toIndex,drt) $outInfo($fromIndex,drt)
   set outInfo($toIndex,midpoint) $outInfo($fromIndex,midpoint)
   set outInfo($toIndex,mpoff) $outInfo($fromIndex,mpoff)
   if {[info exists outInfo($fromIndex,drtFront)] == 1} {
      set outInfo($toIndex,drtFront) $outInfo($fromIndex,drtFront)
      set outInfo($toIndex,drtEnd) $outInfo($fromIndex,drtEnd)
   }
   for {set j 1} {$j <= $outInfo($toIndex,attr)} {incr j} {
      set outInfo($toIndex,a$j,attr) $outInfo($fromIndex,a$j,attr)
      set outInfo($toIndex,a$j,val) $outInfo($fromIndex,a$j,val)
   }
}

proc copySeqToOut {seqIndex outIndex} {
   global seqInfo outInfo
# puts "copySeqToOut from $seqIndex to $outIndex"
   set outInfo($outIndex,names) {}
   set outInfo($outIndex,name) $seqInfo($seqIndex,1,name)
   set outInfo($outIndex,type) $seqInfo($seqIndex,1,name)
   set outInfo($outIndex,errorId) $seqInfo($seqIndex,errorId)
   set outInfo($outIndex,number) 0
   set outInfo($outIndex,parallel) 0
   set outInfo($outIndex,parallelE) 0
   set outInfo($outIndex,parallelF) 0
   set outInfo($outIndex,origIndex) $outIndex
   set outInfo($outIndex,attr) $seqInfo($seqIndex,attr)
   set j $seqInfo($seqIndex,drt,index)
   set outInfo($outIndex,drt,index) $j
   set outInfo($outIndex,chanId,index) $seqInfo($seqIndex,chanId,index)
   set outInfo($outIndex,drt) $seqInfo($seqIndex,a$j,val) 
   if {[info exists seqInfo($seqIndex,midpoint)] == 1} {
      set outInfo($outIndex,midpoint) $seqInfo($seqIndex,midpoint)
      if {[info exists seqInfo($seqIndex,mpoff)] == 1} {
         set outInfo($outIndex,mpoff) $seqInfo($seqIndex,mpoff)
      } else {
         set outInfo($outIndex,mpoff) 0
      }
   } else {
      set outInfo($outIndex,midpoint) "MID"
      set outInfo($outIndex,mpoff) 0
   }
   if {[info exists seqInfo($seqIndex,drtFront)] == 1} {
      set outInfo($outIndex,drtFront) $seqInfo($seqIndex,drtFront)
      set outInfo($outIndex,drtEnd) $seqInfo($seqIndex,drtEnd)
   }
   for {set j 1} {$j <= $outInfo($outIndex,attr)} {incr j} {
      set outInfo($outIndex,a$j,attr) $seqInfo($seqIndex,a$j,attr)
      set outInfo($outIndex,a$j,val) $seqInfo($seqIndex,a$j,val)
   }
}

proc copySeqToSeq {fromIndex toIndex} {
   global seqInfo
# puts "copySeqToSeq $fromIndex $toIndex"
   set seqInfo($toIndex,name) $seqInfo($fromIndex,name)
   set seqInfo($toIndex,origId) $seqInfo($fromIndex,origId)
   set seqInfo($toIndex,errorId) $seqInfo($fromIndex,errorId)
   set seqInfo($toIndex,elemLabel) $seqInfo($fromIndex,elemLabel)
   set seqInfo($toIndex,comps) $seqInfo($fromIndex,comps)
   set seqInfo($toIndex,name) $seqInfo($fromIndex,1,name)
   set seqInfo($toIndex,attr) $seqInfo($fromIndex,attr)
   set seqInfo($toIndex,pElem) $seqInfo($fromIndex,pElem)
   set seqInfo($toIndex,parent) $seqInfo($fromIndex,parent)
   set seqInfo($toIndex,1,name) $seqInfo($fromIndex,1,name)
#   set seqInfo($toIndex,1,attr) $seqInfo($fromIndex,1,attr)
#   set seqInfo($toIndex,1,pElem) $seqInfo($fromIndex,1,pElem)
#   set seqInfo($toIndex,1,parent) $seqInfo($fromIndex,1,parent)
   for {set j 1} {$j <= $seqInfo($fromIndex,attr)} {incr j} {
#      set seqInfo($toIndex,1,a$j,val) $seqInfo($fromIndex,1,a$j,val)
#      set seqInfo($toIndex,1,a$j,attr) $seqInfo($fromIndex,1,a$j,attr)
      set pEl $seqInfo($fromIndex,a$j,attr)
#      set seqInfo($toIndex,1,$pEl) $seqInfo($fromIndex,1,$pEl)
#      set seqInfo($toIndex,1,$pEl,index) $seqInfo($fromIndex,1,$pEl,index)

      set seqInfo($toIndex,a$j,val) $seqInfo($fromIndex,a$j,val)
      set seqInfo($toIndex,a$j,attr) $seqInfo($fromIndex,a$j,attr)
      set seqInfo($toIndex,$pEl) $seqInfo($fromIndex,$pEl)
      set seqInfo($toIndex,$pEl,index) $seqInfo($fromIndex,$pEl,index)
   }
}

proc addToOutList {index indexAt offset} {
   global outInfo
# puts "addToOutList $index to $indexAt with offset $offset"
   set outInfo(elems) [linsert $outInfo(elems) \
      [expr [lsearch $outInfo(elems) $indexAt] + $offset] $index]
}

proc addToOut2 {index indexAt offset corrOffset} {
   global outInfo
# puts "addToOut2 $index to $indexAt with offset $offset and corr $corrOffset"
   set outInfo(elems) [linsert $outInfo(elems) \
      [expr [lsearch $outInfo(elems) $indexAt] + $offset] $index]
   if {$outInfo($index,drt) == "SIMMAX"} {
      set corrId [lindex $outInfo(elems) \
         [expr [lsearch $outInfo(elems) $indexAt] + $corrOffset]]
      lappend outInfo($corrId,simmaxVals) $index
# puts "tag SIMMAX from $index for $corrId"
   } elseif {$outInfo($index,drt) != 0.0} {
      set idAt [lindex $outInfo(elems) \
         [expr [lsearch $outInfo(elems) $indexAt] + $corrOffset]]
      set atJ $outInfo($idAt,drt,index) 
# puts "subtract $outInfo($index,drt) from $idAt"
      if {$outInfo($idAt,drt) == $outInfo($index,drt)} {
# puts "$outInfo($idAt,drt) equals $outInfo($index,drt) set val to 0.0"
         set outInfo($idAt,drt) 0.0
      } else {
         if {[regexp {[\+\-\/\*]+} $outInfo($index,drt)] == 1} {
            set outInfo($idAt,drt) \
             "$outInfo($idAt,drt) - ($outInfo($index,drt))"
         } else {
            set outInfo($idAt,drt) \
             "$outInfo($idAt,drt) - $outInfo($index,drt)"
         }
      }
      set outInfo($idAt,a$atJ,val) $outInfo($idAt,drt)
   }
}

proc addToOut3 {index indexAt offset corrId} {
   global outInfo
# puts "addToOut3 $index to $indexAt with offset $offset and corr ID $corrId"
   set outInfo(elems) [linsert $outInfo(elems) \
      [expr [lsearch $outInfo(elems) $indexAt] + $offset] $index]
   if {$outInfo($index,drt) == "SIMMAX"} {
      lappend outInfo($corrId,simmaxVals) $index
# puts "tag SIMMAX from $index for $corrId"
   } elseif {$outInfo($index,drt) != 0.0} {
#      set idAt [lindex $outInfo(elems) \
#         [expr [lsearch $outInfo(elems) $indexAt] + $corrOffset]]
      set atJ $outInfo($corrId,drt,index) 
# puts "subtract $outInfo($index,drt) from $corrId"
      if {[regexp {[\+\-\/\*]+} $outInfo($index,drt)] == 1} {
         set outInfo($corrId,drt) \
             "$outInfo($corrId,drt) - ($outInfo($index,drt))"
      } else {
         set outInfo($corrId,drt) \
             "$outInfo($corrId,drt) - $outInfo($index,drt)"
      }
      set outInfo($corrId,a$atJ,val) $outInfo($corrId,drt)
   }
}

proc splitVal {index index2} {
   global outInfo
   if {$outInfo($index,drt) != 0.0} {
    if {[info exists outInfo($index,drtFront)] == 1} {
       set outInfo($index,drt) $outInfo($index,drtFront)
       set outInfo($index2,drt) $outInfo($index,drtEnd)
    } else {
      if {[regexp {[\+\-\/\*]+} $outInfo($index,drt)] == 1} {
         set val "($outInfo($index,drt))"
      } else {
         set val "$outInfo($index,drt)"
      }
# puts "regexp of $outInfo($index,midpoint)"
      set type $outInfo($index,midpoint)
      if {$type == "FRONT"} {
         set value $outInfo($index,mpoff)
         set outInfo($index,drt) $value
         if {[regexp {[\+\-]+} $value] == 1} {
            set outInfo($index2,drt) "$val - ($value)"
         } else {
            set outInfo($index2,drt) "$val - $value"
         }
      } elseif {$type == "END"} {
         set value $outInfo($index,mpoff)
         if {[regexp {[\+\-]+} $value] == 1} {
            set outInfo($index,drt) "$val - ($value)"
         } else {
            set outInfo($index,drt) "$val - $value"
         }
         set outInfo($index2,drt) $value
      } else {
         set outInfo($index,drt) $val/2.0
         set outInfo($index2,drt) $outInfo($index,drt)
         if {$outInfo($index,name) == "psgAcquire"} {
            global seqInfo
            set j $seqInfo($index,pts,index) 
            set val $outInfo($index,a$j,val)
            set outInfo($index,a$j,val) ($val)/2
            set outInfo($index2,a$j,val) ($val)/2
         }
      }
    }
      set j $outInfo($index,drt,index) 
      set outInfo($index,a$j,val) $outInfo($index,drt)
      set outInfo($index2,a$j,val) $outInfo($index2,drt)
   }
}

proc splitAtMid {index} {
   global outInfo
#  test sequences which use this section include
#  p4, p5, p6, p17, p18, p19
# puts "splitAtMid $index"
   set mId $index,M
   copyOutToOut $index $mId
   splitVal $index $mId
   set outInfo(elems) [linsert $outInfo(elems) \
                       [expr [lsearch $outInfo(elems) $index] + 1] $mId]
}

proc addMidToOut2 {index indexAt} {
   global outInfo
# puts "addMidToOut2 $index at $indexAt"
   set outInfo(elems) [linsert $outInfo(elems) \
                       [expr [lsearch $outInfo(elems) $indexAt] + 1] $index]
# puts "outInfo($index,drt)= $outInfo($index,drt)"
   if {$outInfo($index,drt) != 0.0} {
      set atJ $outInfo($indexAt,drt,index) 
      set mId $indexAt,M
      if {[info exists outInfo($indexAt,drtFront)] == 1} {
         set type $outInfo($index,midpoint)
         if {[regexp {[\+\-\/\*]+} $outInfo($index,drt)] == 1} {
            set val "($outInfo($index,drt))"
         } else {
            set val "$outInfo($index,drt)"
         }
         if {$type == "FRONT"} {
            set outInfo($indexAt,a$atJ,val) \
             "$outInfo($indexAt,drtFront) - ($outInfo($index,mpoff))"
            set outInfo($mId,a$atJ,val) \
             "$outInfo($mId,drtEnd) - ($val-($outInfo($index,mpoff)))"
         } elseif {$type == "END"} {
            set outInfo($indexAt,a$atJ,val) \
             "$outInfo($indexAt,drtFront) - ($val-($outInfo($index,mpoff)))"
            set outInfo($mId,a$atJ,val) \
             "$outInfo($mId,drtEnd) - ($outInfo($index,mpoff))"
         } else {
            set outInfo($indexAt,a$atJ,val) \
             "$outInfo($indexAt,drtFront) - $val/2.0"
            set outInfo($mId,a$atJ,val) \
             "$outInfo($mId,drtEnd) - $val/2.0"
         }
      } else {
         if {[regexp {[\+\-\/\*]+} $outInfo($index,drt)] == 1} {
            set outInfo($indexAt,a$atJ,val) \
                "$outInfo($indexAt,a$atJ,val) - ($outInfo($index,drt))/2.0"
            set outInfo($mId,a$atJ,val) \
                "$outInfo($mId,a$atJ,val) - ($outInfo($index,drt))/2.0"
         } else {
            set outInfo($indexAt,a$atJ,val) \
                "$outInfo($indexAt,a$atJ,val) - $outInfo($index,drt)/2.0"
            set outInfo($mId,a$atJ,val) \
                "$outInfo($mId,a$atJ,val) - $outInfo($index,drt)/2.0"
         }
      }
      set outInfo($indexAt,drt) $outInfo($indexAt,a$atJ,val)
      set outInfo($mId,drt) $outInfo($mId,a$atJ,val)
# puts "outInfo($indexAt,a$atJ,val)= $outInfo($indexAt,a$atJ,val)"
# puts "outInfo($mId,a$atJ,val)= $outInfo($mId,a$atJ,val)"
   }
}

proc parallelize {index} {
   global outInfo
# puts "call parallelize $index"
   if {[info exists outInfo($index,F,number)] == 0} {
      copyOutToOut $index $index,F
      set outInfo($index,F,type) psgStartMarker
      set outInfo($index,F,parallelIndex) $index
      copyOutToOut $index,F $index,B
      copyOutToOut $index,F $index,P
      set outInfo($index,P,type) parallelEnd
      set outInfo($index,P,count) 1
      set outInfo($index,P,countId) $index,F,number
      set outInfo($index,B,type) psgEndMarker
      set outInfo($index,B,countId) $index,F,number
      set outInfo(elems) [linsert $outInfo(elems) \
         [lsearch $outInfo(elems) $index] $index,F]
      set outInfo(elems) [linsert $outInfo(elems) \
         [expr [lsearch $outInfo(elems) $index] + 1] $index,B]
      set outInfo(elems) [ldelete $outInfo(elems) $index]
      set outInfo($index,F,number) 1
   } else {
      incr outInfo($index,F,number)
   }
   set newId $index,P$outInfo($index,F,number)
   copyOutToOut $index $newId
   set outInfo($newId,pParent) $index
   set outInfo(elems) [linsert $outInfo(elems) \
              [expr 1 + [lsearch $outInfo(elems) $index,F]] $newId]
   set outInfo(elems) [linsert $outInfo(elems) \
              [expr 1 + [lsearch $outInfo(elems) $index,F]] $index,P]
   return $newId
}

proc specialMidMId {index indexAt} {
   global seqInfo outInfo
#  test sequences which use this section include
#  p8p1, p8p2, p8p3, p9p1, p9p2, and p9p3
   copySeqToOut $index $index
   copyOutToOut $index $index,S
   splitVal $index $index,S

   parallelize $indexAt
# end to end style
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) end
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelF) $pIndexAt
      set outInfo($index,parallelE) NONE
# puts "outInfo($indexAt,parallelId) $outInfo($indexAt,parallelId)"
# puts " outInfo($index,parallel) $outInfo($index,parallel)"
# puts " outInfo($index,parallelF) $outInfo($index,parallelF)"
# puts " outInfo($index,parallelE) $outInfo($index,parallelE)"
# puts " outInfo($indexAt,F,number) $outInfo($indexAt,F,number)"
      addToOut2 $index $pIndexAt 1 0

   set index $index,S
   set indexAt $indexAt,S
   parallelize $indexAt
# front to front style
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) front
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelE) $pIndexAt
      set outInfo($index,parallelF) NONE
      addToOut2 $index $pIndexAt 0 0
}

proc addPrimitive {index indexAt dock dockAt} {
   global seqInfo outInfo
# puts "addPrimitive $index to $indexAt $dock to $dockAt"
   if {$seqInfo($index,drt) == ""} {
      if {[info exists seqInfo($index,parent)] == 1} {
         setError $index 3 "$seqInfo($seqInfo($index,parent),elemLabel) has no duration specified"
      } else {
         setError $index 3 "$seqInfo($index,elemLabel) has no duration specified"
      }
   }
   set ifListSub 0
   set checkId2 1
   set checkpId 1
   set adjustIdAt {}
   if {($dock == "front") && ($dockAt == "end")} {
      copySeqToOut $index $index
      if {[info exists outInfo($indexAt,S,drt)] == 1} {
         set indexAt $indexAt,S
      }
      if {$indexAt == $outInfo(last)} {
# puts "append front to end standard"
         set outInfo(last) $index
         lappend outInfo(elems) $index
      } elseif {[info exists outInfo($indexAt,iflist)] == 1} {
# puts "append front to end of IF"
         if {($outInfo($indexAt,name) == "psgIf") || \
             ($outInfo($indexAt,name) == "psgFor") || \
             ($outInfo($indexAt,name) == "psgWhile")} {
# puts "IF 2"
           set checkId2 0
           if {[info exists outInfo($indexAt,parallelE)] == 1} {
# puts "IF 3T"
               if {$outInfo($indexAt,parallelE) != 0} {
                  addToOut3 $index $indexAt,endif 1 $outInfo($indexAt,parallelE)
               } else {
                  addToOutList $index $indexAt,endif 1
               }
               set outInfo($index,parallel) $outInfo($indexAt,parallelE)
               set outInfo($index,parallelF) $outInfo($indexAt,parallelF)
               set outInfo($index,parallelE) $outInfo($indexAt,parallelE)
# puts "outInfo($index,parallelE) $outInfo($index,parallelE)"
           } else {
               addToOut2 $index $indexAt,endif 1 2
            }
            set ifListSub 1
         } else {
# puts "IF 3F"
           if {([info exists outInfo($indexAt,parallelE)] == 1) && \
               ($outInfo($indexAt,parallelE) != 0)} {
               if {[lsearch $outInfo(elems) $indexAt,B] != -1} {
                  addToOut3 $index $indexAt,B 1 $outInfo($indexAt,parallelE)
                  set adjustIdAt $indexAt,B
               } else {
                  addToOut3 $index $indexAt 1 $outInfo($indexAt,parallelE)
               }
               set outInfo($index,parallel) $outInfo($indexAt,parallelE)
               set outInfo($index,parallelF) $outInfo($indexAt,parallelF)
               set outInfo($index,parallelE) $outInfo($indexAt,parallelE)
            } elseif {[lsearch $outInfo(elems) $indexAt,B] != -1} {
               addToOutList $index $indexAt,B 1
               set adjustIdAt $indexAt,B
            } else {
               addToOutList $index $indexAt 1
            }
         }
      } elseif {([info exists outInfo($indexAt,parallelE)] == 1) && \
               ($outInfo($indexAt,parallelE) != 0)} {
# puts "front to end standard"
          set pId $outInfo($indexAt,parallelE)
          if {[lsearch $outInfo(elems) $indexAt,endif] != -1} {
             addToOut3 $index $indexAt,endif 1 $pId
             set checkId2 0
          } elseif {[lsearch $outInfo(elems) $indexAt,B] != -1} {
             addToOut3 $index $indexAt,B 1 $pId
             set adjustIdAt $indexAt,B
          } else {
             addToOut3 $index $indexAt 1 $pId
          }
          set outInfo($index,parallel) $pId
          set outInfo($index,parallelF) $outInfo($indexAt,parallelF)
          set outInfo($index,parallelE) $pId
# puts "outInfo($index,parallelE) $outInfo($index,parallelE)"
          set outInfo($index,pId) $pId
# puts "set outInfo($index,pId) $outInfo($index,pId)"
          lappend outInfo($pId,pIds) $index
# puts "outInfo($pId,pIds) $outInfo($pId,pIds)"
          set checkpId 0
      } else {
# puts "outInfo($indexAt,parallelE) does not exist"
         addToOutList $index $indexAt 1
      }
   } elseif {($dock == "front") && ($dockAt == "front")} {
      if {$seqInfo($indexAt,style) != "if"} {
      parallelize $indexAt
#      if {[info exists outInfo($indexAt,S,drt)] == 1} {
#         set outInfo($indexAt,S,drt) 0
#         set atJ $outInfo($indexAt,drt,index) 
#         set outInfo($indexAt,S,a$atJ,val) 0
#      }
      set checkpId 0
      copySeqToOut $index $index
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      if {[info exists seqInfo($indexAt,nsd)] == 1} {
         set outInfo($pIndexAt,a$seqInfo($indexAt,nsd),val) \
            $seqInfo($index,chanId)
      }
      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) front
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelE) $pIndexAt
      set outInfo($index,parallelF) NONE
      addToOut2 $index $pIndexAt 0 0
      } else {
         copySeqToOut $index $index
         addToOutList $index $indexAt 0
         set checkId2 0
      }
   } elseif {($dock == "end") && ($dockAt == "end")} {
      if {($seqInfo($indexAt,name) == "psgIf") || \
          ($seqInfo($indexAt,style) == "while")} {
         setError $index 1 "unsupported docking. Try front to end"
      }
      if {[info exists outInfo($indexAt,S,drt)] == 1} {
#         set outInfo($indexAt,drt) 0
#         set atJ $outInfo($indexAt,drt,index) 
#         set outInfo($indexAt,a$atJ,val) 0
         set indexAt $indexAt,S
      }
      parallelize $indexAt
      set checkpId 0
      copySeqToOut $index $index
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      if {[info exists seqInfo($indexAt,nsd)] == 1} {
         set outInfo($pIndexAt,a$seqInfo($indexAt,nsd),val) \
            $seqInfo($index,chanId)
      }
      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) end
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelF) $pIndexAt
      set outInfo($index,parallelE) NONE
# puts "outInfo($indexAt,parallelId) $outInfo($indexAt,parallelId)"
# puts " outInfo($index,parallel) $outInfo($index,parallel)"
# puts " outInfo($index,parallelF) $outInfo($index,parallelF)"
# puts " outInfo($index,parallelE) $outInfo($index,parallelE)"
# puts " outInfo($indexAt,F,number) $outInfo($indexAt,F,number)"
      addToOut2 $index $pIndexAt 1 0
   } elseif {($dock == "end") && ($dockAt == "front")} {
      copySeqToOut $index $index
      if {[info exists seqInfo($index,simId)] == 1} {
# puts "EF1: outInfo($indexAt,parallelF) $outInfo($indexAt,parallelF)"
      set indexAt $outInfo($indexAt,parallelF)
      parallelize $indexAt
      set checkpId 0
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) end
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelF) $pIndexAt
      set outInfo($index,parallelE) NONE
      addToOut2 $index $pIndexAt 1 0
      } else {
# puts "EF2: outInfo($indexAt,parallelF) $outInfo($indexAt,parallelF)"
      if {[lsearch $outInfo(elems) $indexAt,F] != -1} {
         addToOut3 $index $indexAt,F 0 $outInfo($indexAt,parallelF)
      } else {
         addToOut3 $index $indexAt 0 $outInfo($indexAt,parallelF)
      }
      set outInfo($index,parallel) $outInfo($indexAt,parallel)
      set outInfo($index,parallelF) $outInfo($indexAt,parallelF)
      set outInfo($index,parallelE) $outInfo($indexAt,parallelE)
      set checkId2 0
      }
   } elseif {$dockAt == "mid"} {
      if {$dock == "mid"} {
         set checkpId 0
         if {[info exists outInfo($indexAt,S,drt)] == 1} {
            specialMidMId $index $indexAt
            set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
            if {[info exists seqInfo($indexAt,nsd)] == 1} {
               set outInfo($pIndexAt,a$seqInfo($indexAt,nsd),val) \
                  $seqInfo($index,chanId)
               set sId $indexAt,S,P$outInfo($indexAt,F,number)
               set outInfo($sId,a$seqInfo($indexAt,nsd),val) \
                  $seqInfo($index,chanId)
               set seqInfo($indexAt,S,nsd) $seqInfo($indexAt,nsd)
            }
         } else {
            parallelize $indexAt
            set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
            splitAtMid $pIndexAt
            set outInfo($indexAt,parallelId) $index
            set outInfo($pIndexAt,parallelAt) mid

            copySeqToOut $index $index
            addMidToOut2 $index $pIndexAt
            set outInfo($index,parallel) $pIndexAt
            set outInfo($index,parallelF) $pIndexAt
            set outInfo($index,parallelE) $pIndexAt,M
# puts "MM: outInfo($index,parallel) $outInfo($index,parallel)"
# puts "MM: outInfo($index,parallelE) $outInfo($index,parallelE)"
# puts "MM: outInfo($index,parallelF) $outInfo($index,parallelF)"
            set outInfo($index,pId) $pIndexAt
            set outInfo($pIndexAt,pIds) $index
            set outInfo($index,B,pId) $pIndexAt,M
            set outInfo($pIndexAt,M,pIds) $index,B
            if {[info exists seqInfo($indexAt,nsd)] == 1} {
               set outInfo($pIndexAt,a$seqInfo($indexAt,nsd),val) \
                  $seqInfo($index,chanId)
               set outInfo($pIndexAt,M,a$seqInfo($indexAt,nsd),val) \
                  $seqInfo($index,chanId)
            }
         }
      } elseif {$dock == "front"} {
      parallelize $indexAt
      set checkpId 0
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      splitAtMid $pIndexAt
      copySeqToOut $index $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) mid
         addToOut3 $index $pIndexAt,M 0 $pIndexAt,M
      set outInfo($index,parallel) $pIndexAt,M
      set outInfo($index,parallelF) NONE
      set outInfo($index,parallelE) $pIndexAt,M

      set outInfo($index,pId) $pIndexAt,M
      set outInfo($pIndexAt,M,pIds) $index

      } elseif {$dock == "end"} {
      parallelize $indexAt
      set checkpId 0
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      splitAtMid $pIndexAt
      copySeqToOut $index $index
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) mid
         addToOut3 $index $pIndexAt 1 $pIndexAt
      set outInfo($index,parallel) $pIndexAt
      set outInfo($index,parallelE) NONE
      set outInfo($index,parallelF) $pIndexAt

      set outInfo($index,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index

      }
   } elseif {($dock == "mid") && ($dockAt == "front")} {
#  test sequences which use this section include
#  p8, p17, p8p*
      if {[lsearch $outInfo(elems) $indexAt,F] == -1} {
         set idAt [lindex $outInfo(elems)  \
           [expr [lsearch $outInfo(elems) $indexAt] - 1]]
      } else {
         set idAt [lindex $outInfo(elems)  \
           [expr [lsearch $outInfo(elems) $indexAt,F] - 1]]
      }
      parallelize $indexAt
      copySeqToOut $index $index
      copyOutToOut $index $index,S
      set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
      set outInfo($index,S,pId) $pIndexAt
      set outInfo($pIndexAt,pIds) $index,S
      set outInfo($indexAt,parallelId) $index
      set outInfo($pIndexAt,parallelAt) front
      set outInfo($index,parallel) $indexAt
      if {$outInfo($idAt,type) == "psgEndMarker"} {
         set idAt $outInfo($idAt,origIndex)
      }
      if {$idAt == $outInfo($idAt,origIndex)} {
         set checkpId 0
         parallelize $idAt
         set outInfo($idAt,parallelId) $index
         set idAt $idAt,P$outInfo($idAt,F,number)
         set outInfo($index,pId) $idAt
         set outInfo($idAt,pIds) $index
         set outInfo($idAt,parallelAt) end
      }
      splitVal $index $index,S
      addToOut2 $index,S $pIndexAt 0 0
      addToOut2 $index $idAt 1 0
      set outInfo($index,parallel) $idAt
      set outInfo($index,parallelF) $idAt
      set outInfo($index,parallelE) NONE
      set outInfo($index,S,parallel) $pIndexAt
      set outInfo($index,S,parallelE) $pIndexAt
      set outInfo($index,S,parallelF) NONE
   } elseif {($dock == "mid") && ($dockAt == "end")} {
# puts "addPrim mid to end: $index and $indexAt"
      if {[lsearch $outInfo(elems) $indexAt,B] == -1} {
         set idAt [lindex $outInfo(elems)  \
           [expr [lsearch $outInfo(elems) $indexAt] + 1]]
      } else {
         set idAt [lindex $outInfo(elems)  \
           [expr [lsearch $outInfo(elems) $indexAt,B] + 1]]
      }
# puts "addPrim mid to end: idAt= $idAt"
      if {$idAt == ""} {
# No test sequences use this section
         parallelize $indexAt
         set checkpId 0
         copySeqToOut $index $index
         copyOutToOut $index $index,S
         set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
         set outInfo($index,pId) $pIndexAt
         set outInfo($pIndexAt,pIds) $index
         set outInfo($index,S,pId) $pIndexAt
         lappend outInfo($pIndexAt,pIds) $index,S
         set outInfo($indexAt,parallelId) $index
         set outInfo($pIndexAt,parallelAt) end
         set outInfo($index,parallel) $index
         splitVal $index $index,S
         addToOut2 $index $pIndexAt 1 0
         set outInfo(last) $index,S
         lappend outInfo(elems) $index,S
         set outInfo($index,parallel) $pIndexAt
         set outInfo($index,parallelF) $pIndexAt
         set outInfo($index,parallelE) NONE
      } else {
#  test sequences which use this section include
#  p9, p9p*
         parallelize $indexAt
         set checkpId 0
         copySeqToOut $index $index
         copyOutToOut $index $index,S
         set pIndexAt $indexAt,P$outInfo($indexAt,F,number)
         set outInfo($index,pId) $pIndexAt
         set outInfo($pIndexAt,pIds) $index
         set outInfo($indexAt,parallelId) $index
         set outInfo($pIndexAt,parallelAt) end
       if {$outInfo($idAt,type) == "psgStartMarker"} {
         set idAt $outInfo($idAt,origIndex)
       }
       if {$idAt == $outInfo($idAt,origIndex)} {
         parallelize $idAt
         set outInfo($idAt,parallelId) $index
         set outInfo($index,parallel) $idAt
         set idAt $idAt,P$outInfo($idAt,F,number)
         set outInfo($index,S,pId) $idAt
         set outInfo($idAt,pIds) $index,S
         set outInfo($idAt,parallelAt) front
         splitVal $index $index,S
         addToOut2 $index $pIndexAt 1 0
         addToOut2 $index,S $idAt 0 0
         set outInfo($index,S,parallel) $idAt
         set outInfo($index,S,parallelE) $idAt
         set outInfo($index,S,parallelF) NONE
        } else {
         splitVal $index $index,S
         addToOut2 $index $pIndexAt 1 0
         if {$idAt == "$outInfo($idAt,origIndex),endif"} {
            addToOutList $index,S $idAt 0
         } else {
            addToOut2 $index,S $idAt 0 0
         }
        }
        set outInfo($index,parallel) $pIndexAt
        set outInfo($index,parallelF) $pIndexAt
        set outInfo($index,parallelE) NONE
      }
   } elseif {($dock == "front") && ($dockAt == "true")} {
      copySeqToOut $index $index
      addToOutList $index $indexAt 1
      set outInfo($indexAt,iflist) $indexAt,trueTime
      set outInfo($index,iflist) $indexAt,trueTime
      if {($seqInfo($index,drt) != 0.0) || ($seqInfo($index,style) == "if") || \
          ($seqInfo($index,style) == "while")} {
         set outInfo($indexAt,id2val) 1
# puts "outInfo($indexAt,id2val) turned on"
      } else {
         set outInfo($index,id2val) $indexAt
# puts "outInfo($index,id2val) chained to $outInfo($index,id2val)"
      }
      set checkId2 0
      set checkpId 0
   } elseif {($dock == "front") && ($dockAt == "false")} {
      if {[info exists outInfo($indexAt,falseMade)] != 1} {
         addToOutList $indexAt,elseif $indexAt,endif 0
         set outInfo($indexAt,falseMade) 0
      }
      copySeqToOut $index $index
      addToOutList $index $indexAt,elseif 1
      set outInfo($indexAt,iflist) $indexAt,falseTime
      set outInfo($index,iflist) $indexAt,falseTime
      if {($seqInfo($index,drt) != 0.0) || ($seqInfo($index,style) == "if") || \
          ($seqInfo($index,style) == "while")} {
         set outInfo($indexAt,id2val) 1
      } else {
         set outInfo($index,id2val) $indexAt
      }
      set checkId2 0
      set checkpId 0
   }
   if {$seqInfo($index,name) == "psgIf"} {
# puts "element $index is an If with parallel value $outInfo($index,parallel)"
      copyOutToOut $index $index,elseif
      set outInfo($index,elseif,name) psgElseif
      copyOutToOut $index $index,endif
      set outInfo($index,endif,name) psgEndif
      addToOutList $index,endif $index 1
      set outInfo($index,id2val) 0
# puts "outInfo($index,a$id2Index,val) $outInfo($index,a$id2Index,val)"
# puts "outInfo($index,id2val) initialized to 0"
      set outInfo($index,trueTime) 0
      set outInfo($index,falseTime) 0
      set id2Index $seqInfo($index,id2,index)
      incr seqInfo(id2)
      set outInfo($index,a$id2Index,val) cond_drt_$seqInfo(id2)
      set outInfo($index,id2) cond_drt_$seqInfo(id2)
      if {$outInfo($index,parallel) != 0} {
         lappend outInfo($outInfo($index,parallel),ifList) $index
# puts "add to ifList outInfo($outInfo($index,parallel),ifList)= $outInfo($outInfo($index,parallel),ifList)"
         set outInfo($index,ifListParallel) $outInfo($index,parallel)
#         set indexAt $outInfo($index,parallel)
      } elseif {[info exists outInfo($indexAt,ifListParallel)] == 1} {
         lappend outInfo($outInfo($indexAt,ifListParallel),ifList) $index
# puts "adding to ifList outInfo($outInfo($indexAt,ifListParallel),ifList)= $outInfo($outInfo($indexAt,ifListParallel),ifList)"
         set outInfo($index,ifListParallel) $outInfo($indexAt,ifListParallel)
      }
   }
   if {$seqInfo($index,style) == "while"} {
# puts "element $index is a While style with parallel value $outInfo($index,parallel)"
      copyOutToOut $index $index,endif
      if {$seqInfo($index,name) == "psgFor"} {
         set outInfo($index,endif,name) psgEndFor
      } else {
         set outInfo($index,endif,name) psgEndWhile
      }
      set outInfo($index,trueTime) 0
      addToOutList $index,endif $index 1
      set id2Index $seqInfo($index,id2,index)
      incr seqInfo(id2)
      set outInfo($index,a$id2Index,val) cond_drt_$seqInfo(id2)
      set outInfo($index,id2) cond_drt_$seqInfo(id2)
      if {$outInfo($index,parallel) != 0} {
         lappend outInfo($outInfo($index,parallel),ifList) $index
# puts "add to ifList outInfo($outInfo($index,parallel),ifList)= $outInfo($outInfo($index,parallel),ifList)"
         set outInfo($index,ifListParallel) $outInfo($index,parallel)
      } elseif {[info exists outInfo($indexAt,ifListParallel)] == 1} {
         lappend outInfo($outInfo($indexAt,ifListParallel),ifList) $index
# puts "adding to ifList outInfo($outInfo($indexAt,ifListParallel),ifList)= $outInfo($outInfo($indexAt,ifListParallel),ifList)"
         set outInfo($index,ifListParallel) $outInfo($indexAt,ifListParallel)
      }
   }
   if {([info exists outInfo($indexAt,iflist)] == 1) && ($dock == "front") && \
        (($dockAt == "true") || ($dockAt == "false") || ($dockAt == "end"))} {
# puts "$index connected to if chain outInfo($indexAt,iflist)"
      set outInfo($index,iflist) $outInfo($indexAt,iflist)
      if {[info exists outInfo($indexAt,ifListParallel)] == 1} {
         set outInfo($index,ifListParallel) $outInfo($indexAt,ifListParallel)
      }
      if {($outInfo($index,drt) != 0) && ($ifListSub == 0)} {
         set ifIndex $outInfo($indexAt,iflist)
         if {$outInfo($ifIndex) == 0} {
            set outInfo($ifIndex) "$outInfo($index,drt)"
         } else {
            set outInfo($ifIndex) "$outInfo($ifIndex) - ($outInfo($index,drt))"
         }
#  puts "outInfo($ifIndex) $outInfo($ifIndex)"
      }
# puts "outInfo($indexAt,iflist)= $outInfo($indexAt,iflist)"
# puts "outInfo($indexAt,iflist)= $outInfo($outInfo($indexAt,iflist))"
   }
   if {($checkId2 == 1) && ([info exists outInfo($indexAt,id2val)] == 1)} {
      if {($seqInfo($index,drt) != 0.0) || ($seqInfo($index,style) == "if") || \
          ($seqInfo($index,style) == "while")} {
         set outInfo($outInfo($indexAt,id2val),id2val) 1
# puts "id2 set on outInfo($outInfo($indexAt,id2val),id2val) 1"
      } else {
         set outInfo($index,id2val) $outInfo($indexAt,id2val)
# puts "set outInfo($index,id2val) $outInfo($index,id2val)"
      }
   }
   if {$adjustIdAt != ""} {
     if {([info exists outInfo($adjustIdAt,pId)] == 1) || \
         ([info exists outInfo($indexAt,pId)] == 0)} {
         set indexAt $adjustIdAt
     }
   }
#puts "checkpId $checkpId info exists outInfo($indexAt,pId) [info exists outInfo($indexAt,pId)]"
   if {($checkpId == 1) && ([info exists outInfo($indexAt,pId)] == 1)} {
# foreach ppid [array names outInfo *,pId] {
# puts "outInfo($ppid) $outInfo($ppid)"
# }
# foreach ppid [array names outInfo *,pIds] {
# puts "outInfo($ppid) $outInfo($ppid)"
# }
         set pId $outInfo($indexAt,pId)
         set outInfo($index,pId) $pId
#puts "set outInfo($index,pId) $outInfo($index,pId)"
         lappend outInfo($pId,pIds) $index
   }
#  puts "$outInfo(elems)"
}

proc serial {index list1 list2} {
   global seqInfo dockList dockPts simPts
   set id $seqInfo($index,id)
#  puts "serialize $seqInfo($index,name) index=$index with id= $id"
   set idAt $seqInfo($index,idAt)
   if {$idAt == $seqInfo(lastPhase)} {
      set dock front
      set dockAt end
   } else {
      set dock $seqInfo($index,dock)
      set dockAt $seqInfo($index,dockAt)
   }
   set fromIndex $dockList($id,$dock,index)
   set fromIndexOrig $fromIndex
# puts "dock $dock of $id to $dockAt of $idAt"
if {$idAt == "phase"} {
# puts "dock $dock of $seqInfo($fromIndex,name) to phase list"
} else {
# puts "dock $dock of $seqInfo($fromIndex,name) to $dockAt of $seqInfo($dockList($idAt,$dockAt,index),name)"
}
# puts "id= $id index=$index from=$id fromIndex=$fromIndex toId=$idAt"
# puts "seqInfo($fromIndex,name) $seqInfo($fromIndex,name)"
   if {$seqInfo($fromIndex,name) == "psgPhaseTable"} {
      set match {}
      regexp {[0-9 \n\.(),^]+} $seqInfo($fromIndex,phaseBase) match
      if {$match != $seqInfo($fromIndex,phaseBase)} {
        setError $fromIndex 0 "phase table $seqInfo($fromIndex,phaseTableName) has illegal values $seqInfo($fromIndex,phaseBase)"
      }
      setPhaseName $fromIndex 1
      set seqInfo($fromIndex,tableUsed) 0
# puts "seqInfo($fromIndex,phaseBase) $seqInfo($fromIndex,phaseBase)"
   }
   if {$idAt == $seqInfo(lastPhase)} {
      set seqInfo(lastPhase) $id
      lappend seqInfo(phase) $fromIndex
      return
   }
   set atIndex $dockList($idAt,$dockAt,index)
   set atIndexOrig $atIndex
   set fromDock $dockList($id,$dock,dock)
   set atDock $dockList($idAt,$dockAt,dock)
# puts "dock $fromIndex to $atIndex $fromDock to $atDock"
   if {([lsearch $seqInfo($list1) $atIndex] != -1) \
       || ($atIndex == "start")} {
#  puts "A: $list1: $fromIndex $atIndex $dockList($id,$dock,dock) $atDock"
#  puts "A: seqInfo($fromIndex,name) $seqInfo($fromIndex,name)"
#  puts "A: seqInfo($atIndex,name) $seqInfo($atIndex,name)"
#  puts "A: seqInfo($fromIndex,dock) $seqInfo($fromIndex,dock)"
#  puts "A: seqInfo($fromIndex,dockAt) $seqInfo($fromIndex,dockAt)"
      lappend seqInfo($list1) $fromIndex $atIndex $fromDock $atDock
      set addList $list1
   } else {
# puts "B: $list2: $fromIndex $atIndex $dockList($id,$dock,dock) $atDock"
      lappend seqInfo($list2) $fromIndex $atIndex $fromDock $atDock
      set addList $list2
      set list1 $list2
   }
   if {($addList == $list2) && \
       ((($fromDock == "front") && ($atDock == "end")) || \
        (($fromDock == "end") && ($atDock == "front")))} {
      set fLoc [expr [llength $seqInfo($list2)] - 4]
      set aLoc [lsearch $seqInfo($list2) $atIndex]
      if {($aLoc % 4 == 0) && ($fLoc > $aLoc) && ($fLoc != $aLoc + 4)} {
            set seqInfo($list2) [lreplace $seqInfo($list2) $fLoc [expr $fLoc + 3]]
            set seqInfo($list2) [linsert $seqInfo($list2) [expr $aLoc + 4] \
               $fromIndex $atIndex $fromDock $atDock]
      }
   }
   
   while {$fromIndex != ""} {

      if {[regexp {([0-9,]+),([0-9]+)} $fromIndex match parent child] == 1} {
# puts "fromIndex= $fromIndex parent= $parent child= $child"
      if {$seqInfo($parent,comps) > 1} {
# puts "add comps seqInfo($parent,comps) $seqInfo($parent,comps)"
        set compList {}
        set secondList {}
        for {set i 1} {$i <= $seqInfo($parent,comps)} {incr i} {
           lappend compList $i
        }
        set compList [ldelete $compList $child]
        set conn $seqInfo($parent,$child,relIdAt)
        while {[lsearch $compList $conn] != -1} {
# puts "connect conn=$conn"
           if {$seqInfo($parent,$conn,comps) == 1} {
              set saveIdAt $seqInfo($parent,$conn,relIdAt)
              set seqInfo($parent,$conn,relIdAt) $child
              lappend secondList $conn
              lappend seqInfo($list2) $parent,$conn $parent,$child \
             $seqInfo($parent,$child,dockAt) $seqInfo($parent,$child,dock)
              set seqInfo($parent,$conn,relIdAt) $saveIdAt
           } else {
#  puts "add comp $parent,$child"
           }
           set compList [ldelete $compList $conn]
           set child $conn
           set conn $seqInfo($parent,$conn,relIdAt)
           if {$child == $conn} break
        }
# puts "remaining connections $compList"
# puts "end index is $seqInfo($index,endId)"
        set lastId $seqInfo($index,endId)
        foreach child $compList {
           set conn $seqInfo($parent,$child,relIdAt)
           if {$seqInfo($parent,$child,comps) == 1} {
# puts "addPrimitive $parent,$child  $parent,$conn $seqInfo($parent,$child,dock) $seqInfo($parent,$child,dockAt)"
# puts "secondList= $secondList child= $child conn= $conn"
              if {($child <= $lastId) && \
                  ([lsearch $secondList $conn] == -1) && \
                  ($seqInfo($parent,$child,dock) == "front") && \
                  ($seqInfo($parent,$child,dockAt) == "end")} {
# puts "child $list1: $parent,$child  $parent,$conn $seqInfo($parent,$child,dock) $seqInfo($parent,$child,dockAt)"
                lappend seqInfo($list1) $parent,$child  $parent,$conn \
             $seqInfo($parent,$child,dock) $seqInfo($parent,$child,dockAt)
              } else {
                lappend secondList $child
# puts "child $list2: $parent,$child  $parent,$conn $seqInfo($parent,$child,dock) $seqInfo($parent,$child,dockAt)"
                lappend seqInfo($list2) $parent,$child  $parent,$conn \
             $seqInfo($parent,$child,dock) $seqInfo($parent,$child,dockAt)
              }
           } else {
#  puts "add comp $parent,$child"
           }
        }
      }
         set fromIndex $parent
      } else {
         set fromIndex ""
      }
# puts "new fromIndex= $fromIndex"
   }
}

proc putElem {id} {
   global outInfo
#  puts "putElem $outInfo($id,name) (id=$id)"
   if {(($outInfo($id,name) == "psgDelay") || \
        ($outInfo($id,name) == "psgSimDelay")) && \
       ($outInfo($id,drt) == 0.0)} {
#  puts "putElem $outInfo($id,name) drt= 0.0; skip it"
      return
   }
   savePsgData "<$outInfo($id,name)>"
   if {[info exists outInfo($id,errorId)] == 1} {
#  puts "   outInfo($id,errorId) $outInfo($id,errorId)"
      savePsgDataAttr id $outInfo($id,errorId)
   }
   set j $outInfo($id,chanId,index)
   savePsgDataAttr $outInfo($id,a$j,attr) $outInfo($id,a$j,val)
   for {set j 1} {$j <= $outInfo($id,attr)} {incr j} {
      if {[string tolower $outInfo($id,a$j,val)] != "not used"} {
         if {[lsearch $outInfo(skipList) $outInfo($id,a$j,attr)] == -1} {
            if {([lsearch $outInfo(skipElems) $outInfo($id,name)] == -1) || \
                ([lsearch $outInfo(skipList2) $outInfo($id,a$j,attr)] == -1)} {
               savePsgDataAttr $outInfo($id,a$j,attr) $outInfo($id,a$j,val)
            }
         }
      }
   }
   savePsgData "</$outInfo($id,name)>"
}

proc putPreacq {} {
   savePsgData "<psgInput>"
   savePsgDataAttr id 0
   savePsgDataAttr chanId 1
   savePsgDataAttr drt 0.0
   savePsgDataAttr calc {double acqtime; \
if (preacqactive == "n") \
{preacq = rof2 + alfa + 1.0/(beta*fb); \
 output ("vnmr","setvalue('preacq',",preacq*1e6,")\n");\
 acqtime = preacq + np/2.0/sw + OVERFLOW_DELAY;\
}}
   savePsgData "</psgInput>"
}

proc testIfList {id} {
   global outInfo
   if {[info exists outInfo($id,ifList)] == 0} {
      return 0
   }
   foreach ifId $outInfo($id,ifList) {
      if {($outInfo($ifId,trueTime) != 0) || ($outInfo($ifId,falseTime) != 0)} {
         return 1
      }
   }
   return 0
}

proc fixIfDelays {id} {
   global outInfo
   set corr ""
# puts "fixIf $id outInfo($id,pIds) $outInfo($id,pIds)"
   foreach el $outInfo($id,pIds) {
      if {([info exists outInfo($el,id2val)] == 1) && \
          ($outInfo($el,id2val) == 1)} {
         if {[info exists outInfo($el,id2)] == 0} {
            setError $el 1 "unsupported docking. Try front to end"
         }
         if {$corr == ""} {
            set corr $outInfo($el,id2)
         } else {
            set corr "$corr + $outInfo($el,id2)"
         }
      }
   }
   set drtI $outInfo($id,drt,index)
#  puts "fix if Delay $id corr= $corr"
   if {($outInfo($id,a$drtI,val) == "SIMMAX")} {
# puts "found simMax for $id ($outInfo($id,pParent)) corr= $corr"
      set outInfo($id,simmaxId) $outInfo($id,pParent),F,simmax
# if {[info exists outInfo($outInfo($id,pParent),F,simmax)] == 0} {
#   puts "outInfo($outInfo($id,pParent),F,simmax) does not exist"
# } else {
#   puts "initial outInfo($outInfo($id,pParent),F,simmax) $outInfo($outInfo($id,pParent),F,simmax)"
# }
      if {$corr != ""} {
         lappend outInfo($outInfo($id,pParent),F,simmax) $corr
# puts "outInfo($outInfo($id,pParent),F,simmax) $outInfo($outInfo($id,pParent),F,simmax)"
      }
   }
   if {$corr != ""} {
      if {[regexp {[\+\-\/\*]+} $corr] == 1} {
         set outInfo($id,a$drtI,val) "$outInfo($id,a$drtI,val) - ($corr)"
      } else {
         set outInfo($id,a$drtI,val) "$outInfo($id,a$drtI,val) - $corr"
      }
# puts "outInfo($id,a$drtI,val) $outInfo($id,a$drtI,val)"
   }
}

proc checkDpsParallel {} {
   global outInfo seqInfo
   set list $outInfo(elems)
   foreach id $list {
      if {$outInfo($id,type) == "psgStartMarker"} {
         set origId $outInfo($id,origIndex)
         if {[info exists seqInfo($origId,dpsName)] == 1} {
            addPrimitive $origId $origId front front
# The following line is a trick to prevent checkParallel from removing
# this parallel chain.
            set outInfo($origId,drt) "$outInfo($origId,drt) "
         }
      }
   }
}

proc checkParallel {} {
   global outInfo seqInfo
# puts "checkParallel: elems= $outInfo(elems)"
   set num 0
# parray outInfo
   set changeList {}
   set list $outInfo(elems)
   set skipNext 0
   foreach id $list {
      if {$num > 1} {
         for {set inum 1} {$inum < $num} {incr inum} {
            lappend idList($inum) $id
         }
      }
      if {$skipNext == 1} {
         set skipNext 0
#  puts "skip element $outInfo($id,name) ($id)"
         continue
      }
#  puts "check element $outInfo($id,name) ($id)"
      if {$outInfo($id,type) == "psgStartMarker"} {
#  puts "write parallel start $id $outInfo($id,number) chains"
         if {$num >= 1} {
            lappend idList($num) $id
         }
         incr num
#  puts "num incremented to $num"
         set drt($num) $outInfo($id,drt)
         set parallelId($num) $outInfo($id,parallelIndex)
         set first($num) yes
         set idList($num) {}
         set last($num) -1
         set skipNext 1
#  puts "parallel drt= $drt($num) for id $parallelId($num) num= $num"
      } elseif {($outInfo($id,type) == "parallelEnd") || \
                ($outInfo($id,type) == "psgEndMarker")} {
#  puts "write parallel end $id $outInfo($id,number) chains $outInfo($id,type)"
#  puts "orig drt= $drt($num) first= $first($num) last= $last($num) num= $num"
#  puts "outInfo($parallelId($num),F,number): $outInfo($parallelId($num),F,number)"
         set newId [lindex $idList($num) 0]
         if {$outInfo($parallelId($num),F,number) > 1} {
#  puts "drt($num) $drt($num) first($num) $first($num) last($num) $last($num)"
            if {($drt($num) == $first($num)) && \
                ($outInfo($parallelId($num),F,origIndex) == $outInfo($newId,origIndex))} {
             set numEl [llength $idList($num)]
#  puts "id= $id numEl= $numEl"
#  puts "outInfo($parallelId($num),F,origIndex)= $outInfo($parallelId($num),F,origIndex)"
#  puts "outInfo($newId,origIndex)= $outInfo($newId,origIndex)"
#  puts "outInfo($newId,ifList)= [info exists outInfo($newId,ifList)]"
#  puts "ifList test= [testIfList $newId]"
             if {([testIfList $newId] == 0) && ($numEl > 1)} {
#  puts "remove parallel place $idList($num) after $parallelId($num),B"
               set id1 [lsearch $outInfo(elems) $newId]
               incr numEl -1
               set id2 [expr $id1 + $numEl]
               incr id1 -1
# puts "removes elems $id1 to $id2 (numEl= $numEl)"
# puts "oldlist= $outInfo(elems)"
# puts "remove= [lrange $outInfo(elems) $id1 $id2]"
               set outInfo(elems) [lreplace $outInfo(elems) $id1 $id2]
# puts "removed= $outInfo(elems)"
               set idList($num) [lreplace $idList($num) 0 0]
               eval {set outInfo(elems) [linsert $outInfo(elems) \
                [expr [lsearch $outInfo(elems) $parallelId($num),B] + 1] } $idList($num)]
# puts "newlist= $outInfo(elems)"
               incr outInfo($parallelId($num),F,number) -1
              }
            } elseif {$drt($num) == $last($num)} {
               set newId [lindex $idList($num) [expr [llength $idList($num)] - 1]]
#  puts "outInfo($parallelId($num),F,origIndex)= $outInfo($parallelId($num),F,origIndex)"
#  puts "outInfo($newId,origIndex)= $outInfo($newId,origIndex)"
#  puts "outInfo($newId,ifList)= [info exists outInfo($newId,ifList)]"
#  puts "ifList test= [testIfList $newId]"
             if {($outInfo($parallelId($num),F,origIndex) == $outInfo($newId,origIndex))\
                 && ([testIfList $newId] == 0)} {
#  puts "remove parallel place $idList($num) before $parallelId($num),F"
               set id1 [lsearch $outInfo(elems) [lindex $idList($num) 0]]
               set numEl [llength $idList($num)]
               incr numEl -1
               set id2 [expr $id1 + $numEl]
               incr id1 -1
# puts "oldlist= $outInfo(elems)"
# puts "remove= [lrange $outInfo(elems) $id1 $id2]"
               set outInfo(elems) [lreplace $outInfo(elems) $id1 $id2]
# puts "removed= $outInfo(elems)"
               set idList($num) [lreplace $idList($num) $numEl $numEl]
               eval {set outInfo(elems) [linsert $outInfo(elems) \
                    [lsearch $outInfo(elems) $parallelId($num),F] } $idList($num)]
# puts "newlist= $outInfo(elems)"
               incr outInfo($parallelId($num),F,number) -1
              }
            }
      }
            set first($num) "yes"
            set idList($num) {}  
            set last($num) -1
          if {$outInfo($id,type) == "psgEndMarker"} {
            unset first($num)
            unset idList($num)
            unset last($num)
            unset parallelId($num)
            incr num -1
# puts "num decremented to $num"
           }
      } elseif {($outInfo($id,type) != "psgMarker") && ($num > 0)} {
         if {$first($num) == "yes"} {
            set first($num) $outInfo($id,drt)
            set last($num) $first($num)
            lappend idList($num) $id
         } elseif {$last($num) != -1} {
            set last($num) $outInfo($id,drt)
            lappend idList($num) $id
         }
         if {[info exists seqInfo($id,corrDrt)] == 1} {
# puts "really found corrDrt for id $id num=$num"
            if {$num > 1} {
              set inum [expr $num -1]
              if {($inum > 1) && \
                  ($outInfo($parallelId($inum),F,drt) == "SIMMAX")} {
                 incr inum -1
              }
              lappend changeList $id $parallelId($inum),F
# puts "parallelId($inum)= $parallelId($inum)"
            } else {
              lappend changeList $id $parallelId($num),F
# puts "parallelId($num)= $parallelId($num)"
            }
         }
      }
   }
   foreach {id para} $changeList {
#puts "1: outInfo(elems) $outInfo(elems)"
      set id2 [lsearch $outInfo(elems) $id]
      set outInfo(elems) [lreplace $outInfo(elems) $id2 $id2]
#puts "2: outInfo(elems) $outInfo(elems)"
      set id2 [lsearch $outInfo(elems) $para]
      set outInfo(elems) [linsert $outInfo(elems) $id2 $id]
#puts "3: outInfo(elems) $outInfo(elems)"
   }
}

proc checkSim3 {id} {
   global outInfo
   if {[info exists outInfo($id,simmax)] == 1} {
# puts "1: outInfo($id,simmax) $outInfo($id,simmax) number: $outInfo($id,number)"
      set len [llength $outInfo($id,simmax)]
      set temp $outInfo($id,simmax)
      set outInfo($id,simmax) "simMax([lindex $temp 0]"
      for {set i 1} {$i < $len} {incr i} {
# puts "outInfo($id,simmax) $outInfo($id,simmax),[lindex $temp $i]"
         set outInfo($id,simmax) "$outInfo($id,simmax),[lindex $temp $i]"
# puts "i: $i  outInfo($id,simmax) $outInfo($id,simmax)"
      }
      set outInfo($id,simmax) "$outInfo($id,simmax))"
      set outInfo($outInfo($id,origIndex),simmaxVal) $outInfo($id,simmax)
      if {$len <=1} {
         set outInfo($outInfo($id,origIndex),simmaxVal) "[lindex $temp 0]"
      }
      if {$outInfo($id,number) <= 1} {
         set outInfo($id,simmax) DELETE
      } elseif {$len <=1} {
         set outInfo($id,simmax) DELETE
      }
# puts "outInfo($id,simmax) $outInfo($id,simmax)"
# puts "outInfo($outInfo($id,origIndex),simmaxVal) $outInfo($outInfo($id,origIndex),simmaxVal)"
   }
}

proc addElem3 {id} {
   global outInfo
# puts "addElem3: id= $id"
   if {[info exists outInfo($id,simmaxId)] == 1} {
# puts "outInfo($id,simmaxId) $outInfo($id,simmaxId)"
      set idrt $outInfo($id,drt,index) 
# puts "drt= $outInfo($id,a$idrt,val)"
      if {[info exists outInfo($outInfo($id,simmaxId))] == 1} {
         set rep $outInfo($outInfo($id,simmaxId))
      } else {
         set rep DELETE
      }
      if {($outInfo($id,a$idrt,val) == "SIMMAX") || \
          ($rep == "DELETE")} {
# puts "skip elem $id with drt $outInfo($id,a$idrt,val) and rep= $rep"
         return 0
      } else {
         regsub -all "SIMMAX" $outInfo($id,a$idrt,val) $rep outInfo($id,a$idrt,val)
# puts "new drt= $outInfo($id,a$idrt,val)"
         set outInfo($id,drt) $outInfo($id,a$idrt,val)
      }
   }
   if {[info exists outInfo($id,simmaxVals)] == 1} {
# puts "outInfo($id,simmaxVals) $outInfo($id,simmaxVals)"
      set idrt $outInfo($id,drt,index) 
# puts "drt= $outInfo($id,a$idrt,val)"
      foreach el $outInfo($id,simmaxVals) {
         if {[info exists outInfo($el,simmaxVal)] == 1} {
# puts "subtract outInfo($el,simmaxVal) = $outInfo($el,simmaxVal)"
         set outInfo($id,a$idrt,val) "$outInfo($id,a$idrt,val) - $outInfo($el,simmaxVal)"
         }
      }
# puts "new drt= $outInfo($id,a$idrt,val)"
   }
   if {$outInfo($id,number) > 1} {
#  puts "parallel start element $outInfo($id,name) ($id)"
         savePsgData "<parallel start>"
         savePsgDataAttr number $outInfo($id,number)
         if {$outInfo($id,drt) == "SIMMAX"} {
            savePsgDataAttr drt $outInfo($id,simmax)
         } else {
            savePsgDataAttr drt $outInfo($id,drt)
         }
         savePsgData "</parallel start>"
#         savePsgData "<parallel 1>"
#         savePsgData "</parallel 1>"
   } elseif {$outInfo($id,type) == "parallelEnd"} {
#  puts "parallel next element $outInfo($id,name) ($id) $outInfo($id,count)"
      if {$outInfo($outInfo($id,countId)) > 1} {
         if {$outInfo($id,count) <= $outInfo($outInfo($id,countId))} {
            savePsgData "<parallel $outInfo($id,count)>"
            set pCount $outInfo($id,count)
         }
         incr outInfo($id,count)
         return $pCount
      }
   } elseif {$outInfo($id,type) == "psgEndMarker"} {
#  puts "parallel end element $outInfo($id,name) ($id) parallel end"
      if {$outInfo($outInfo($id,countId)) > 1} {
         savePsgData "<parallel end>"
         savePsgData "</parallel end>"
      }
   } elseif {($outInfo($id,type) != "psgMarker") && \
             ($outInfo($id,type) != "psgStartMarker")} {

#  puts "$id,drt $outInfo($id,drt)"
      if {$outInfo($id,drt) != "SIMMAX"} {
         putElem $id
      }
   }
   return 0
}

proc getElemIndex {name id} {
   global pElem
   for {set i 0} {$i < $pElem(numGroups)} {incr i} {
      for {set j 0} {$j < $pElem(numElems,$i)} {incr j} {
#puts "test $name == $pElem(elemName,$i,$j)"
         if {$name == $pElem(elemName,$i,$j)} {
            return "$i,$j"
         }
      }
   }
puts "no element named $name"
   set pElem(errname) $name
   setError $id 1 "Element $name does not exist"
   return -1,-1
}

proc loadElem {index} {
   global pElem
   
# puts "loadElem $index ($pElem(elemName,$index))"
   set fd [open \
     $pElem(groupPath,$pElem(elemGroup,$index))/$pElem(elemName,$index) r]
# puts "loadpath $pElem(groupPath,$pElem(elemGroup,$index))/$pElem(elemName,$index)"
   set indexSave $index
   set pElem(type,$index) "primitive"
   set pElem(name,$index) $index
   set pElem(label,$index) $pElem(elemLabel,$index)
   set pElem(comps,$index) 0
   set pElem(numAttr,$index) 1
   set pElem(infos,$index) ""
           set pElem(chanId,$index) chanId
           set pElem(N1,$index) chanId
           set pElem(B1,$index) ""
           set pElem(L1,$index) chanId
           set pElem(V1,$index) ""
           set pElem(S1,$index) primitive
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
             set value "$value $line"
           }
        }
#  puts "check attribute $attr with value '$value'"
        if {[regexp {\[inheritString ([\.A-Za-z0-9]*)\]([\.A-Za-z0-9]*)} \
              $value match id label] == 1} {
           set pElem($attr,$index) $label
           incr pElem(numAttr,$index)
           set i $pElem(numAttr,$index)
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) $id
           set pElem(L$i,$index) $label
           set pElem(V$i,$index) ""
           set pElem(S$i,$index) inheritString
#  puts "inheritString attribute $attr,$index with $id and ext '$label'"
#  puts "pElem($attr,$index) is $pElem($attr,$index)"
        } elseif {[regexp {\[inherit ([\.A-Za-z0-9]*)\]([\._A-Za-z0-9]*)} \
              $value match id label] == 1} {
           set pElem($attr,$index) $label
           incr pElem(numAttr,$index)
           set i $pElem(numAttr,$index)
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) $id
           set pElem(L$i,$index) $label
           set pElem(V$i,$index) ""
           set pElem(S$i,$index) inherit
#  puts "inherit attribute $attr,$index with $id and ext '$label'"
#  puts "pElem($attr,$index) is $pElem($attr,$index)"

        } elseif {[regexp {\[declare ([\.A-Za-z0-9]*) ([\.A-Za-z0-9]*) ([\.A-Za-z0-9]*)\]([\.A-Za-z0-9]*)} \
              $value match elemType id label] == 1} {
          set pElem($attr,$index) $label
          incr pElem(numAttr,$index)
          set i $pElem(numAttr,$index)
          set pElem(N$i,$index) $attr
          set pElem(T$i,$index) $elemType
          set pElem(B$i,$index) $id
          set pElem(L$i,$index) $label
          set pElem(V$i,$index) ""
          set pElem(S$i,$index) declare

        } elseif {[regexp {primitive[^"]+"([^"]*)"} \
             $value match label] == 1} {
           incr pElem(numAttr,$index)
           set i $pElem(numAttr,$index)
           set pElem($attr,$index) $label
           set pElem(N$i,$index) $attr
           set pElem(B$i,$index) 0
           set pElem(V$i,$index) $label
           set pElem(S$i,$index) primitive
#  puts "primitive attribute $attr with default '$label'"
        } elseif {[regexp {label([0-9]+)} $attr match id] == 1}  {
           set compLabel($id) $value
#  puts "label attribute $id '$value'"
        } elseif {[regexp {attr([0-9]+)} $attr match id] == 1}  {
           if {$composite == 1} {
              incr pElem(numAttr,$index)
              set i $pElem(numAttr,$index)
              set pElem(N$i,$index) $attr
              set pElem(B$i,$index) 0
              set pElem(L$i,$index) $compLabel($id)
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
           if {[regexp {info([0-9]+)} $attr match id] == 1}  {
             lappend pElem(infos,$index) attr$id V$i
           }
#  puts "set pElem($attr,$index) $value"
        }
     } elseif {[regexp {<([a-zA-Z0-9_]+)>} $line match el]  == 1} {
        lappend stack $el
        set getAttr 1
        if {$el == "composite"} {
           set composite 1
           catch {unset compLabel}
           set pElem(type,$index) "composite"
#  puts "composite entry"
        } elseif {$composite == 0} {
           set pElem(label,$index) $el
        } else {
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
#   if {$pElem(elemName,$index) == "psgSetPhase"} {
#foreach id [array names pElem *,$index] {
#puts "pElem($id) $pElem($id)"
#}
#   }
#  puts "attr for $index is $pElem(numAttr,$index)"
#   printPelem
}
proc loadPhase {id} {
   global seqInfo pElem curElem
   set seqInfo($id,name) psgPhaseTable
   set seqInfo($id,id) $id
   set index [getElemIndex $seqInfo($id,name) $id]
   if {$pElem(elemLoaded,$index) != 1} {
      loadElem $index
   }
   set seqInfo($id,elemLabel) $pElem(elemLabel,$index)
   set seqInfo($id,elemIndex) $index
   set parent $id
   set seqInfo($id,origId) $parent
      set seqInfo($id,comps) 1
      set seqInfo($id,1,name) $pElem(elemName,$index)
      set seqInfo($id,attr) $pElem(numAttr,$index)
      set seqInfo($id,1,attr) $pElem(numAttr,$index)
      set seqInfo($id,1,comps) 0
      set seqInfo($id,1,pElem) $index
      set seqInfo($id,1,parent) $id
      set seqInfo($id,parent) $parent
      for {set j 1} {$j <= $pElem(numAttr,$index)} {incr j} {
         set val $pElem(V$j,$index)
         set seqInfo($id,1,a$j,attr) $pElem(N$j,$index)
         set seqInfo($id,1,a$j,val) $val
         set seqInfo($id,1,$pElem(N$j,$index)) $val
         set seqInfo($id,1,$pElem(N$j,$index),index) $j

         set seqInfo($id,a$j,attr) $pElem(N$j,$index)
         set seqInfo($id,a$j,val) $val
         set seqInfo($id,$pElem(N$j,$index)) $val
         set seqInfo($id,$pElem(N$j,$index),index) $j
      }
}

proc makePsgSimDelay {} {
   global seqInfo
#   incr seqInfo(elems)
   set i extraPsgSimDelay
   set seqInfo($i,name) psgSimDelay
   set seqInfo($i,1,name) psgSimDelay
   set seqInfo($i,elemLabel) psgSimDelay
   set seqInfo($i,errorId) 0
   set seqInfo($i,pElem) "inline"
   set seqInfo($i,parent) "inline"
   set seqInfo($i,comps) 0
   set seqInfo($i,origId) ""
   set seqInfo($i,attr) 3
   set seqInfo($i,a1,val) SIMMAX
   set seqInfo($i,a1,attr) drt
   set seqInfo($i,drt) SIMMAX
   set seqInfo($i,drt,index) 1
   set seqInfo($i,a2,val) delay
   set seqInfo($i,a2,attr) style
   set seqInfo($i,style) delay
   set seqInfo($i,style,index) 2
   set seqInfo($i,a3,val) ""
   set seqInfo($i,a3,attr) chanId
   set seqInfo($i,chanId) delay
   set seqInfo($i,chanId,index) 3
   set seqInfo($i,id) $i
   set seqInfo($seqInfo($i,id),idIndex) $i
}

proc makeNewSimDelay {parent} {
   global seqInfo dockList
   incr seqInfo(elems)
   set id $seqInfo(elems)
   copySeqToSeq extraPsgSimDelay $id
   set seqInfo($id,id) $id
#   set seqInfo($id,origId) $parent
#   set seqInfo($id,parent) $parent
   set seqInfo($id,parentId) 0
   set seqInfo($id,origId) $id
   set seqInfo($id,parent) $id
   set seqInfo($id,chanId) $seqInfo($parent,chanId)
   set seqInfo($id,a3,val) $seqInfo($parent,chanId)
   set seqInfo($id,frontId) 1
   set dockList($id,front,dock) front
   set dockList($id,front,index) $id
   set dockList($id,mid,dock) mid
   set dockList($id,mid,index) $id
   set dockList($id,end,dock) end
   set dockList($id,end,index) $id
   set seqInfo($id,lastId) 0
   return $id
}

proc loadPulseElem {id parent} {
   global seqInfo pElem curElem dpsFlag
#   puts "called loadComposite for id= $id ($seqInfo($id,name))"
   set index [getElemIndex $seqInfo($id,name) $id]
   if {$pElem(elemLoaded,$index) != 1} {
      loadElem $index
   }
   set seqInfo($id,elemLabel) $pElem(elemLabel,$index)
   set seqInfo($id,elemIndex) $index
   set seqInfo($id,origId) $parent
   set seqInfo($id,errorId) $seqInfo($parent,errorId)
   if {$pElem(comps,$index) == 0} {
# puts "add primitive element $seqInfo($id,name))"
      set seqInfo($id,comps) 1
      set seqInfo($id,frontId) 1
     set seqInfo($id,1,name) $pElem(elemName,$index)
     set seqInfo($id,1,attr) $pElem(numAttr,$index)
# puts "load prim $id,1 named $seqInfo($id,1,name) with $seqInfo($id,1,attr) attrs."
#     set seqInfo($id,1,comps) 0
     set seqInfo($id,pElem) $index
     set seqInfo($id,1,pElem) $index
#     set seqInfo($id,1,parent) $id
      set seqInfo($id,parent) $parent
      for {set j 1} {$j <= $pElem(numAttr,$index)} {incr j} {
         if {($pElem(S$j,$index) != "primitive") && \
             ([info exists seqInfo($id,$pElem(N$j,$index))] == 0)} {
            set val $pElem(V$j,$index)
# puts "add attr $pElem(N$j,$index) ($val) to $seqInfo($id,name)"
#        regsub -all "COMPVALUE" $val $seqInfo(compvalue) val
# puts "prim attr: $pElem(N$j,$index): $val"
            incr seqInfo($id,attr)
            set jj $seqInfo($id,attr)
            set seqInfo($id,a$jj,attr) $pElem(N$j,$index)
            set seqInfo($id,a$jj,val) $val
            set seqInfo($id,$pElem(N$j,$index)) $val
            set seqInfo($id,$pElem(N$j,$index),index) $jj
         } else {
# puts "prim attr seqInfo($id,$pElem(N$j,$index)) $seqInfo($id,$pElem(N$j,$index))"
         if {[info exists seqInfo($id,$pElem(N$j,$index))] == 1} {
            set val $seqInfo($id,$pElem(N$j,$index))
         } else {
            incr seqInfo($id,attr)
            set val $pElem(V$j,$index)
            set seqInfo($id,$pElem(N$j,$index),index) $seqInfo($id,attr)
            set seqInfo($id,a$seqInfo($id,attr),attr) $pElem(N$j,$index)
         }
         regsub -all "COMPVALUE" $val $seqInfo(compvalue) val
         while {[regexp {VALUEOF\(([a-zA-Z0-9]+)\)} $val match newval] == 1} {
# puts "VALUEOF found; set $newval to $seqInfo($id,$newval)"
            set newval $seqInfo($id,$newval)
            regsub {VALUEOF\(([a-zA-Z0-9]+)\)} $val $newval val
# puts "VALUEOF found; new value $val"
         }
# puts "new value $val"
         set seqInfo($id,$pElem(N$j,$index)) $val
         set jj $seqInfo($id,$pElem(N$j,$index),index)
# puts "old value seqInfo($id,a$jj,val) $seqInfo($id,a$jj,val)"
         set seqInfo($id,a$jj,val) $val
# puts "new value seqInfo($id,a$jj,val) $seqInfo($id,a$jj,val)"
         }
      }
      if {[info exists seqInfo($id,minTest)] == 1} {
#puts "mintest is $seqInfo($id,minTest) (index= $seqInfo($id,minTest,index))"
#puts "drt is $seqInfo($id,drt) (index= $seqInfo($id,drt,index))"
         if {[regexp {[^a-zA-Z0-9]*t1|t2|t3|t4|t5[^a-zA-Z0-9]*} \
               $seqInfo($id,drt) match ] == 1} {
#puts "found nD"
            set jj $seqInfo($id,minTest,index)
            if {[regexp {[^a-zA-Z0-9]*t1[^a-zA-Z0-9]*} \
               $seqInfo($id,drt) match ] == 1} {
               if {$seqInfo(nD) < 2} {
                  set seqInfo(nD) 2
               }
#               set seqInfo($id,a$jj,val) -1.0/sw1
#               set seqInfo($id,minTest) -1.0/sw1
            }
            if {[regexp {[^a-zA-Z0-9]*t2[^a-zA-Z0-9]*} \
               $seqInfo($id,drt) match ] == 1} {
               if {$seqInfo(nD) < 3} {
                  set seqInfo(nD) 3
               }
#               set seqInfo($id,a$jj,val) -1.0/sw2
#               set seqInfo($id,minTest) -1.0/sw2
            }
            if {[regexp {[^a-zA-Z0-9]*t3[^a-zA-Z0-9]*} \
               $seqInfo($id,drt) match ] == 1} {
               if {$seqInfo(nD) < 4} {
                  set seqInfo(nD) 4
               }
#               set seqInfo($id,a$jj,val) -1.0/sw3
#               set seqInfo($id,minTest) -1.0/sw3
            }
            if {[regexp {[^a-zA-Z0-9]*t4[^a-zA-Z0-9]*} \
               $seqInfo($id,drt) match ] == 1} {
               if {$seqInfo(nD) < 5} {
                  set seqInfo(nD) 5
               }
#               set seqInfo($id,a$jj,val) -1.0/sw4
#               set seqInfo($id,minTest) -1.0/sw4
            }
         }
      }
      if {($dpsFlag == 1) && ([info exists seqInfo($id,dockType)] == 1)} {
         set seqInfo($id,dpsName) $seqInfo($id,1,name)
         set seqInfo($id,1,name) $seqInfo($id,dockType)
         set seqInfo($id,name) $seqInfo($id,dockType)
      }
      if {[info exists seqInfo($id,chanType)] == 1} {
         set chan $seqInfo($id,chanId)
         if {$seqInfo($id,chanType) != $seqInfo($chan,type)} {
            setError $id 0 "$seqInfo($id,chanType) element is on a $seqInfo($chan,type) channel"
         }
      }
      return
   }
   set relId {0}
   set seqInfo($id,comps) $pElem(comps,$index)
   set seqInfo($id,frontDock) $pElem(frontDock,$index)
   set seqInfo($id,frontId) $pElem(frontId,$index)
   set seqInfo($id,midDock) $pElem(midDock,$index)
   set seqInfo($id,midId) $pElem(midId,$index)
   set seqInfo($id,endDock) $pElem(endDock,$index)
   set seqInfo($id,endId) $pElem(endId,$index)
   if {[info exists pElem(lastId,$index)] == 1} {
      set seqInfo($id,lastId) $pElem(lastId,$index)
   } else {
      set seqInfo($id,lastId) $pElem(endId,$index)
   }
# puts "set seqInfo($id,comps) $seqInfo($id,comps)"
   for {set i 1} {$i <= $pElem(comps,$index)} {incr i} {
      set seqInfo($id,$i,name)  $pElem(elemName,$index,$i)
      set seqInfo($id,$i,attr)  $pElem(numAttr,$index,$i)
# puts "load elem $id,$i named $seqInfo($id,$i,name) with $seqInfo($id,$i,attr) attrs."
      set seqInfo($id,$i,parent) $id
      set seqInfo($id,$i,pElem) $index,$i
      set valueofs {}
      for {set j 1} {$j <= $pElem(numAttr,$index,$i)} {incr j} {
         if {$pElem(S$j,$index,$i) == "inheritString"} {
            set seqIndex $seqInfo($id,$pElem(B$j,$index,$i),index)
            set val \"$seqInfo($id,a$seqIndex,val)\"
# puts "inheritString: val: $val"
         } elseif {($pElem(S$j,$index,$i) == "inherit") && \
                   ([info exists seqInfo($id,$pElem(B$j,$index,$i),index)] == 1)} {
            set seqIndex $seqInfo($id,$pElem(B$j,$index,$i),index)
            set val "$seqInfo($id,a$seqIndex,val)$pElem(L$j,$index,$i)"
# puts "inherit: val: $val"

         } elseif {($pElem(S$j,$index,$i) == "declare") && \
                   ([info exists seqInfo($id,$pElem(B$j,$index,$i),index)] == 1)} {
            set seqIndex $seqInfo($id,$pElem(B$j,$index,$i),index)
            if { $seqInfo($id,a$seqIndex,val) == "NOT USED" } {
                set seqInfo($id,a$seqIndex,val) $pElem(L$j,$index,$i)

             if {$pElem(T$j,$index,$i) == "OblGrad"} {
               set val "double $pElem(L$j,$index,$i)_trise ; double $pElem(L$j,$index,$i)_int ;"

             } elseif {$pElem(T$j,$index,$i) == "SliceSelectRF"} {
               set val "double $pElem(L$j,$index,$i)_BWSS ; double $pElem(L$j,$index,$i)_GSS ;"

             } elseif {$pElem(T$j,$index,$i) == "Variable" } {
               set val "double $pElem(L$j,$index,$i) ; "

             }

            } else {

             if {$pElem(T$j,$index,$i) == "OblGrad"} {
               set val "double $seqInfo($id,a$seqIndex,val)_trise ; double $seqInfo($id,a$seqIndex,val)_int ; "

             } elseif {$pElem(T$j,$index,$i) == "SliceSelectRF"} {
               set val " double $seqInfo($id,a$seqIndex,val)_BWSS ; \
                         double $seqInfo($id,a$seqIndex,val)_GSS ;"

             } elseif {$pElem(T$j,$index,$i) == "Variable" } {
               set val " double $seqInfo($id,a$seqIndex,val) ; "

             }

            }

         } else {
            set val $pElem(V$j,$index,$i)
         }
# puts "elem attr: val: $val"
         regsub -all "COMPVALUE" $val $seqInfo(compvalue) val
         while {[regexp {VALUEOF\(([a-zA-Z0-9]+)\)} $val match newval] == 1} {
# puts "VALUEOF found; set $newval to $seqInfo($id,$i,$newval)"
            if {[regexp {input[0-9]} $newval] == 1} {
               if {[lsearch $valueofs $newval] == -1} {
                  lappend valueofs $newval
               }
            }
            set newval $seqInfo($id,$i,$newval)
#            regsub {VALUEOF\((input[0-9])\)} $val $newval val
            regsub {VALUEOF\(([a-zA-Z0-9]+)\)} $val $newval val
# puts "VALUEOF found; new value $val"
         }
         set seqInfo($id,$i,a$j,attr) $pElem(N$j,$index,$i)
         set seqInfo($id,$i,a$j,val) $val
# puts "seqInfo($id,$i,a$j,val): $seqInfo($id,$i,a$j,val)"
         set seqInfo($id,$i,$pElem(N$j,$index,$i)) $val
         set seqInfo($id,$i,$pElem(N$j,$index,$i),index) $j
      }
      foreach inp $valueofs {
         set j $seqInfo($id,$i,$inp,index)
         set seqInfo($id,$i,a$j,val) "NOT USED"
         set seqInfo($id,$i,$inp) "NOT USED"
      }
      loadPulseElem $id,$i $parent
   }
}

proc setPsgInfo {} {
   global dockList
   catch {unset dockList}
   set dockList(num) 1
#   set dockList(1,id) start
   set dockList(start,front) start
   set dockList(start,mid) start
   set dockList(start,end) start
   set dockList(start,end,index) start
   set dockList(start,end,dock) end
}

proc loadPsg {seqName dir} {
   global seqInfo
   set stack {}
   set getAttr 0
   set index ""
   set seqInfo(name) $seqName
   set seqInfo(start,name) start
   set seqInfo(start,parent) 0
   set seqInfo(start,origId) 0
   set seqInfo(start,lastId) 0
   set seqInfo(start,drt) 0
   set seqInfo(0,endId) 0
   set seqInfo(0,comps) 0
   set seqInfo(elems) 0
   set seqInfo(nD) 1
   if {($seqName != "") && ([file exists [file join $dir psglib $seqName]] == 1)} {
      set fd [open [file join $dir psglib $seqName] r]
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
            ([regexp {[^<]*<([a-zA-Z0-9_]+)>} $line match attr]  == 1)} {
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
                 if {$line != ""} {
                   set value "$value\n$line"
                 }
              }
           }
# puts "got attribute $attr with value $value"
           incr seqInfo($index,attr)
           set seqInfo($index,a$seqInfo($index,attr),attr) $attr
           set seqInfo($index,a$seqInfo($index,attr),val) $value
           set seqInfo($index,$attr) $value
           set seqInfo($index,$attr,index) $seqInfo($index,attr)
        } elseif {[regexp {<([a-zA-Z0-9_]+)>} $line match el]  == 1} {
#puts "new component $el for PS $elName"
           lappend stack $el
           incr seqInfo(elems)
           set index $seqInfo(elems)
           set seqInfo($index,name) $el
           set seqInfo($index,attr) 0
           set getAttr 1
        } elseif {[regexp {</([a-zA-Z0-9_]+)>} $line match el]  == 1} {
           set stackVal [lindex $stack end]
           if {$stackVal == $el} {
              set stack [lrange $stack 0 [expr [llength $stack]-2]]
           } else {
              puts "Syntax error.  Expected $stackVal terminater but got $el"
           }
           if {[info exists seqInfo($index,id)] == 0} {
              set seqInfo($index,errorId) 0
           } else {
              set seqInfo($index,errorId) $seqInfo($index,id)
           }
           set getAttr 0
        } else {
           puts "UNKNOWN line $line"
        }
      }
      close $fd
   }
}

proc savePsgData {line} {
   global outFd
  puts $outFd $line
}

proc savePsgDataAttr {header val} {
   savePsgData " <$header> $val </$header>"
}

proc readElemFiles {} {
   global pElem
   for {set i 0} {$i < $pElem(numGroups)} {incr i} {
#puts "group $i"
      set pElem(numElems,$i) 0
      set j 0
      if {[file exists $pElem(groupPath,$i)/elements] == 1} {
         catch {source $pElem(groupPath,$i)/labels}
         set fd [open $pElem(groupPath,$i)/elements r]
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

proc makeElemMenu {fileName} {
   global pElem env
   set fd [open $fileName r]
   set pElem(numGroups) 0
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
        incr pElem(numGroups)
     }
   }
   close $fd
   readElemFiles
}

proc findTrueAttached {id} {
   global seqInfo
# puts "findTrueAttached $id"
# puts "findTrueAttached $seqInfo(second2)"
   foreach {index indexAt dock dockAt} $seqInfo(second2) {
      if {($indexAt == $id) && ($dockAt == "true")} {
# puts "findTrueAttached $seqInfo($index,name) "
         return $index
      }
   }
   return 0
}

proc removeAttached {id} {
   global seqInfo
# puts "removeAttached $id"
   set elems $seqInfo(second)
   set seqInfo(second) {}
   set newId {}
   lappend idList $id
   foreach {index indexAt dock dockAt} $elems {
      if {[lsearch $idList $indexAt] == -1} {
         lappend seqInfo(second) $index $indexAt $dock $dockAt
      } else {
         lappend idList $index
         if {$newId == ""} {
# puts "removeAttached $seqInfo($index,name) "
            set newId $index
         }
      }
   }
   return $newId
}

proc fixSpecialIfs {} {
   global seqInfo
# puts "removeIfs 0: prime= $seqInfo(prime)"
# puts "removeIfs 0: second= $seqInfo(second)"
   set elems $seqInfo(prime)
   set seqInfo(prime) {}
   set replace NO
   foreach {index indexAt dock dockAt} $elems {
      if {$replace == "indexAt"} {
         set replace NO
# puts "reattached $index from $indexAt to $replaceIndexAt"
         set indexAt $replaceIndexAt
         set seqInfo($index,idAt) $indexAt
      }
      if {$seqInfo($index,name) == "psgIf"} {
# puts "prime $seqInfo($index,name) condition= $seqInfo($index,condition)"
         set res [scan $seqInfo($index,condition) "NOVALUE %s" test]
         if {$res == 1} {
            set tName [string tolower \
                       [string trim $seqInfo($index,$test) { \t"}]]
# puts "test=$test tName=$tName seqInfo($index,$test)= $seqInfo($index,$test)"
            if {[string compare $tName unaltered] == 0} {
# puts "remove IF"
               set replaceIndexAt $indexAt
               set replace indexAt
               removeAttached $index
            } else {
# puts "substitute IF"
               set newIndex [removeAttached $index]
               lappend seqInfo(prime) $newIndex $indexAt $dock $dockAt
               set seqInfo($newIndex,idAt) $indexAt
               set seqInfo($newIndex,dockAt) $dockAt
               set replaceIndexAt $newIndex
               set replace indexAt
            }
         } else {
# puts "keep IF"
            lappend seqInfo(prime) $index $indexAt $dock $dockAt
         }
      } else {
         lappend seqInfo(prime) $index $indexAt $dock $dockAt
      }
   }
# puts "removeIfs 1: prime= $seqInfo(prime)"
# puts "removeIfs 1: second= $seqInfo(second)"
   set elems $seqInfo(second)
   set seqInfo(second2) $seqInfo(second)
   set seqInfo(second) {}
   set removeId {}
   set delId {}
   set replaceId {}
   catch {unset replace}
   foreach {index indexAt dock dockAt} $elems {
# puts "test: $index $indexAt $dock $dockAt"
      if {[lsearch $delId $indexAt] != -1} {
         lappend delId $index
         continue
      }
      if {[lsearch $removeId $indexAt] != -1} {
         if {$dockAt == "true"} {
            if {$replace($indexAt) != $index} {
               lappend delId $index
            }
            continue
         }
         if {[info exists replace($indexAt,dockAt)] == 1} {
            set dockAt $replace($indexAt,dockAt)
         }
         set indexAt $replace($indexAt)
      }
      if {$seqInfo($index,name) == "psgIf"} {
# puts "second $seqInfo($index,name) condition= $seqInfo($index,condition)"
         set res [scan $seqInfo($index,condition) "NOVALUE %s" test]
         if {$res == 1} {
            set tName [string tolower \
                       [string trim $seqInfo($index,$test) { \t"}]]
# puts "test=$test tName=$tName seqInfo($index,$test)= $seqInfo($index,$test)"
            if {[string compare $tName unaltered] == 0} {
# puts "remove IF $index"
               lappend removeId $index
               set replace($index) $indexAt
               set replace($index,dockAt) $dockAt
            } else {
# puts "substitute IF $index"
               set newIndex [findTrueAttached $index]
# puts "$newIndex $indexAt $dock $dockAt"
               lappend seqInfo(second) $newIndex $indexAt $dock $dockAt
               set seqInfo($newIndex,idAt) $indexAt
               lappend removeId $index
               set replace($index) $newIndex
            }
         } else {
# puts "keep IF"
            lappend seqInfo(second) $index $indexAt $dock $dockAt
         }
      } else {
         lappend seqInfo(second) $index $indexAt $dock $dockAt
      }
# puts "current: $seqInfo(second)"
   }
# puts "removeIfs 2: prime= $seqInfo(prime)"
# puts "removeIfs 2: second= $seqInfo(second)"
   set seqInfo(second2) {}
}

proc removeMarkers {} {
   global seqInfo
# puts "marker seqInfo(prime) $seqInfo(prime)"
   foreach {index indexAt dock dockAt} $seqInfo(prime) {
      if {$seqInfo($index,name) == "psgMarker"} {
# puts "removing marker $index connected to $indexAt at $dockAt"
         set marker($index) $indexAt
         set marker($index,dockAt) $dockAt
      } elseif {$seqInfo($indexAt,name) == "psgMarker"} {
# puts "reconnect $index to $marker($indexAt) at $marker($indexAt,dockAt)"
         lappend seqInfo(prime2) $index $marker($indexAt) \
                $dock $marker($indexAt,dockAt)
      } else {
         lappend seqInfo(prime2) $index $indexAt $dock $dockAt
      }
   }
   set seqInfo(second2) {}
# puts "marker seqInfo(second) $seqInfo(second)"
   foreach {index indexAt dock dockAt} $seqInfo(second) {
      if {$seqInfo($index,name) == "psgMarker"} {
# puts "removing marker $index connected to $indexAt at $dockAt"
         set marker($index) $indexAt
         set marker($index,dockAt) $dockAt
      } elseif {$seqInfo($indexAt,name) == "psgMarker"} {
# puts "reconnect $index to $marker($indexAt) at $marker($indexAt,dockAt)"
         lappend seqInfo(second2) $index $marker($indexAt) \
                $dock $marker($indexAt,dockAt)
      } else {
         lappend seqInfo(second2) $index $indexAt $dock $dockAt
      }
   }
}

proc replaceQuotes {val1} {
   global seqInfo
# puts "replaceQuotes $val1"
   set val [split $val1 \"]
   set len [llength $val]
# puts "$len elements: $val"
   if {$len % 2 == 0} {
#      puts "Phase $val1 have unmatched quotes (\")"
      setError $seqInfo(curPhaseElem) 1 "Phase value $val1 has unmatched quotes (\")"
   }
   set val2 [lindex $val 0]
   for {set i 1} {$i < $len} {incr i 2} {
      set id [lsearch $seqInfo(phaseLabels) [lindex $val $i]]
      if {$id == -1} {
#         puts "Phase table [lindex $val $i] not defined"
         setError $seqInfo(curPhaseElem) 1 "Phase table [lindex $val $i] not defined"
      }
      set newName $seqInfo([lindex $seqInfo(phaseIds) $id],phaseTableName)
      set val2 [concat $val2 $newName [lindex $val [expr $i + 1]]]
      lappend seqInfo(phaseLabels) $newName
      lappend seqInfo(phaseIds) [lindex $seqInfo(phaseIds) $id]
# puts "val2: $val2"
   }
   return $val2
}

proc checkPhaseVal {val} {
   global seqInfo
# call bobans program here.
# puts "check phase value $val"
   set fd [open "| expandphase \"$val\"" r+]
   gets $fd num
   gets $fd value
   close $fd
   if {$num < 0} {
#      puts "Phase table $val has an error"
#      puts "$value"
      setError $seqInfo(curPhaseElem) 1 "Phase table $val has an error"
   }
   set seqInfo(phaseTableLen) $num
   return $value
}

proc checkPhase {val} {
   global seqInfo
   set match {}
   regexp {[a-zA-Z0-9+-. "]+} $val match
   if {$val != $match} {
      setError $seqInfo(curPhaseElem) 1 "illegal character in '$val'; okay up to '$match'"
   }
   set match {}
# puts "checkPhase $val"
   if {[string trim [string tolower $val] { \t"}] == "unaltered"} {
      set val Unaltered
   }
   if {[regexp {["]+} $val match] >= 1} {
      set val [replaceQuotes $val]
   }
# puts "checkPhase after quotes $val"
   set valPlus [split $val +]
   set final {}
   foreach plus $valPlus {
      set valMinus [split $plus -]
      set finalMinus {}
      foreach minus $valMinus {
         set minus [string trim $minus]
# puts "check $minus"
         set id [lsearch $seqInfo(phaseLabels) $minus]
         if {$id == -1} {
            set match {}
            regexp {^[0-9]+\.?[0-9]*} $minus match
            if {$match == $minus} {
               set name $minus
#               puts "use value '$minus'"
            } else {
               regexp {^[a-zA-Z]+[a-zA-Z_0-9]*} $minus match
               if {$match == $minus} {
                  set name $minus
#                  puts "use parameter '$minus'"
               } else {
#                  puts "build new table with '$minus'"
                  set minus [checkPhaseVal $minus]
                  set id [makeNewPhase "" $minus ctss $seqInfo(phaseTableLen)],1
                  lappend seqInfo(phaseLabels) $seqInfo($id,phaseTableName)
                  lappend seqInfo(phaseIds) $id
                  set i $seqInfo($id,phaseTableName,index)
                  set seqInfo($id,a$i,val) $seqInfo($id,phaseTableName)
# puts "phaseLabels $seqInfo(phaseLabels)"
                  set name $seqInfo($id,phaseTableName)
                  set seqInfo($id,tableUsed) 1
               }
            }
         } else {
            set id [lindex $seqInfo(phaseIds) $id]
            set name $seqInfo($id,phaseTableName)
            set seqInfo($id,tableUsed) 1
         }
         if {$finalMinus == ""} {
            set finalMinus $name
         } else {
            set finalMinus [concat $finalMinus - $name]
         }
      }
      if {$final == ""} {
         set final $finalMinus
      } else {
         set final [concat $final + $finalMinus]
      }
   }
# puts "checkPhase final $final"
   return $final
}

proc setPhaseName {el testIt} {
   global seqInfo
# puts "setPhaseName for $el and $seqInfo($el,phaseTableName)"
   set phName [string trim $seqInfo($el,phaseTableName)]
   set seqInfo($el,phaseTableName) $phName
   if {($testIt == 1) && ([lsearch $seqInfo(phaseLabels) $phName] != -1)} {
#      puts "WARNING: duplicate phase tables $phName"
      setError $el 0 "duplicate phase tables $phName"
      set el [lsearch $seqInfo(phaseLabels) $phName]
      setError [lindex $seqInfo(phaseIds) $el] 1 "duplicate phase tables $phName"
   }
   lappend seqInfo(phaseLabels) $phName
   lappend seqInfo(phaseIds) $el
   if {$phName == "oph"} {
      set seqInfo(ophIndex) $el
   } else {
      set match ""
      regexp {[a-zA-Z]+[a-zA-Z0-9_]*} $phName match
      if {$match != $phName} {
         incr seqInfo(phaseIndex)
         set seqInfo($el,phaseTableName) phtab$seqInfo(phaseIndex)
# puts "new name for $el is $seqInfo($el,phaseTableName)"
         set i $seqInfo($el,phaseTableName,index)
         set seqInfo($el,a$i,val) $seqInfo($el,phaseTableName)
      } else {
# puts "name for $el is $seqInfo($el,phaseTableName)"
      }
   }
}

proc makeNewPhase {name val index len} {
   global seqInfo
   incr seqInfo(elems)
   set id $seqInfo(elems)
   loadPhase $id
   set seqInfo($seqInfo($id,id),idIndex) $id
   if {$name == ""} {
      incr seqInfo(phaseIndex)
      set name phtab$seqInfo(phaseIndex)
   }
   set i $seqInfo($id,1,phaseTableName,index)
   set seqInfo($id,errorId) 0
   set seqInfo($id,1,errorId) 0
   set seqInfo($id,1,phaseTableName) $name
   set seqInfo($id,1,a$i,val) $name
   set seqInfo($id,phaseTableName) $name
   set seqInfo($id,a$i,val) $name

   set i $seqInfo($id,1,phaseBase,index)
   set seqInfo($id,1,phaseBase) $val
   set seqInfo($id,1,a$i,val) $val
   set seqInfo($id,phaseBase) $val
   set seqInfo($id,a$i,val) $val

   set i $seqInfo($id,1,phaseIndex,index)
   set seqInfo($id,1,phaseIndex) $index
   set seqInfo($id,1,a$i,val) $index
   set seqInfo($id,phaseIndex) $index
   set seqInfo($id,a$i,val) $index

   set i $seqInfo($id,1,phaseLen,index)
   set seqInfo($id,1,phaseLen) $len
   set seqInfo($id,1,a$i,val) $len
   set seqInfo($id,phaseLen) $len
   set seqInfo($id,a$i,val) $len

   set i $seqInfo($id,1,chanId,index)
   set seqInfo($id,1,chanId) 1
   set seqInfo($id,1,a$i,val) 1
   set seqInfo($id,chanId) 1
   set seqInfo($id,a$i,val) 1
   set seqInfo($id,tableUsed) 0
   set seqInfo($id,1,tableUsed) 0

# foreach e [array names seqInfo $id,*] {
# puts "seqInfo($e) $seqInfo($e)"
# }
   lappend seqInfo(phase) $id
   return $id
}

proc setCyclops {el} {
   global seqInfo outInfo
   set el2 $outInfo($el,origIndex)
   if {($seqInfo($el2,phaseCycle) != "Cyclops") && \
          ($seqInfo($el2,phaseCycle) != "None")} {
         set chan $seqInfo($el2,chanId)
         if {$chan == 0} {
            set chan 1
         }
         set i $seqInfo($el2,phaseCycle,index)
# puts "reset outInfo($el,a$i,val) from $outInfo($el,a$i,val) to $seqInfo($chan,cyclops)"
         set outInfo($el,a$i,val) $seqInfo($chan,cyclops)
   }
}

proc sortPhaseList {} {
   global seqInfo
# puts "phase list $seqInfo(phase)"
   set pList $seqInfo(phase)
   set seqInfo(phase) ""
   foreach el $pList {
# puts "seqInfo($el,1,name) $seqInfo($el,1,name)"
      if {$seqInfo($el,1,name) == "psgPhaseTable"} {
         set seqInfo(phase) [linsert $seqInfo(phase) 0 $el]
      } else {
         lappend seqInfo(phase) $el
      }
   }
# puts "phase list $seqInfo(phase)"
}

proc getFirstCalc {} {
   global outInfo
   set elems $outInfo(elems)
   foreach el $elems {
      if {$outInfo($el,type) == "psgStartMarker"} {
         return
      } elseif {$outInfo($el,name) == "psgInput"} {
         putElem $el
         set outInfo(elems) [lreplace $outInfo(elems) 0 0]
      } else {
         return
      }
   }
}

proc setPhaseInfo {} {
   global seqInfo outInfo dpsFlag
   foreach el {1 2 3 4 5} {
      set id [makeNewPhase fad$el "0.0 180.0" t${el}Index 2]
      lappend seqInfo(phaseLabels) fad$el
      lappend seqInfo(phaseIds) $id,1
      set seqInfo(fad$el,id) $id,1
   }
   foreach el $outInfo(elems) {
      set seqInfo(curPhaseElem) $el
      if {$outInfo($el,type) == "psgPhaseTable"} {
# puts "psgPhaseTable $el is docked, remove it"
         set el2 $outInfo($el,origIndex)
# puts "orig phases $seqInfo(phase)"
         set seqInfo(phase) [ldelete $seqInfo(phase) $el2]
         set seqInfo($el2,tableUsed) 1
         set i $seqInfo($el2,phaseBase,index)
#  puts "1: $outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
         set outInfo($el,a$i,val) [checkPhaseVal $outInfo($el,a$i,val)]
# puts "new phases $seqInfo(phase)"
      } elseif {$outInfo($el,type) == "psgSetPhaseTable"} {
# puts "found psgSetPhaseTable"
        setCyclops $el
        set el2 $outInfo($el,origIndex)
         setPhaseName $el2 0
         set seqInfo($el2,tableUsed) 1
        set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
        set outInfo($el,a$i,val) [checkPhase $outInfo($el,a$i,val)]
# puts "new value $outInfo($el,a$i,val)"
        if {$seqInfo($el2,phaseTableName) == "oph"} {
           foreach el3 {1 2 3 4 5} {
              set j $seqInfo(fad$el3,id)
              if {$seqInfo($j,tableUsed) == 1} {
# puts "add fad$el3 to oph"
                 set outInfo($el,a$i,val) \
                    [concat $outInfo($el,a$i,val) + fad$el3]
              }
            }
         }
      } elseif {$outInfo($el,type) == "psgSetPhase"} {
# puts "found psgSetPhase"
        setCyclops $el
        set el2 $outInfo($el,origIndex)
        set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
        set outInfo($el,a$i,val) [checkPhase $outInfo($el,a$i,val)]
        set seqInfo($el2,tableUsed) 1
# puts "new value $outInfo($el,a$i,val)"
      } elseif {($dpsFlag == 1) && \
                ([info exists seqInfo($el,phaseBase)] == 1)} {
        setCyclops $el
        set el2 $outInfo($el,origIndex)
        set i $seqInfo($el2,phaseBase,index)
        set outInfo($el,a$i,val) [checkPhase $outInfo($el,a$i,val)]
        set seqInfo($el2,tableUsed) 1
      }
   }
   sortPhaseList
   foreach el $seqInfo(phase) {
      set seqInfo(curPhaseElem) $el
      copySeqToOut $el $el
      set el2 $outInfo($el,origIndex)
      set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
# puts "outInfo($el,type) $outInfo($el,type)"
      if {$outInfo($el,type) == "psgSetPhaseTable"} {
         setCyclops $el
         setPhaseName $el2 0
         set seqInfo($el2,tableUsed) 1
         set outInfo($el,a$i,val) [checkPhase $outInfo($el,a$i,val)]
         if {$seqInfo($el2,phaseTableName) == "oph"} {
           foreach el3 {1 2 3 4 5} {
              set j $seqInfo(fad$el3,id)
              if {$seqInfo($j,tableUsed) == 1} {
                 set outInfo($el,a$i,val) \
                    [concat $outInfo($el,a$i,val) + fad$el3]
              }
            }
         }
      } else {
# puts "2: $outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
         set outInfo($el,a$i,val) [checkPhaseVal $outInfo($el,a$i,val)]
      }
      putElem $el
   }
}

proc setSimDelays {id idDock idList} {
   global seqInfo dockPts dpsFlag

# puts "setSimDelays id: $id idDock: $idDock idList: $idList"
   set index [lindex $idList 0]
   incr seqInfo(elems)
   set id $seqInfo(elems)
# puts "prime0= $seqInfo(prime2)"
# puts "second0= $seqInfo(second2)"
# puts "new id is $id"
# puts "old index is $index"
# puts "idList: $idList"
   set simIds {}
   if {($dpsFlag == 1) && ([info exists seqInfo($index,dockType)] == 1)} {
      copySeqToSeq extraPsgSimDelay $id
      set seqInfo($id,origId) $id
      set seqInfo($id,nsd) $seqInfo(extraPsgSimDelay,chanId,index)
      set seqInfo($id,chanId) $seqInfo($index,chanId)
   } else {
      copySeqToSeq $index $id
      set seqInfo($id,nsd) $seqInfo($index,chanId,index)
   }
   foreach el $idList {
      lappend simIds $seqInfo($el,origId)
   }
   foreach el $idList {
      set seqInfo($el,simList) [concat $id $simIds]
# puts "seqInfo($el,simList) $seqInfo($el,simList)"
   }
   set seqInfo($id,simList) [concat $id $simIds]
# puts "seqInfo($id,simList) $seqInfo($id,simList)"
# puts "simIds: $simIds"
   set altIndex {}
   if {[info exists seqInfo($index,parent)] == 1} {
      set seqInfo($id,parent) $seqInfo($index,parent)
   }
   set redo 0
   if {[lsearch $seqInfo(prime2) $index] != -1} {
      set cnt 0
      set oldprime $seqInfo(prime2)
      set seqInfo(prime2) {}
      foreach el $oldprime {
         if {$el == $index} {
            lappend seqInfo(prime2) $id
            incr cnt
         } else {
            lappend seqInfo(prime2) $el
         }
      }
# puts "cnt: $cnt"
      if {$cnt == 1} {
         set second2 $seqInfo(second2)
         set seqInfo(second2) {}
         foreach {index2 indexAt2 dock2 dockAt2} $second2 {
            if {($dock2 == "front") && ($dockAt2 == "end") && \
                ([lsearch $idList $indexAt2] != -1) && \
                ([lsearch $simIds $seqInfo($index2,origId)] == -1)} {
               lappend seqInfo(prime2) $index2 $id $dock2 $dockAt2 
# puts "add $index2 $id $dock2 $dockAt2 to prime list"
               set redo 1
            } else {
               lappend seqInfo(second2) $index2 $indexAt2 $dock2 $dockAt2 
            }
         }
      }
      set seqInfo(second2) [linsert $seqInfo(second2) 0 $index $id \
             $idDock $idDock]
   } else {
# puts "prime3= $seqInfo(prime2)"
# puts "second3= $seqInfo(second2)"
      set second2 $seqInfo(second2)
      set seqInfo(second2) {}
      foreach {index2 indexAt2 dock2 dockAt2} $second2 {
         if {$index2 == $index} {
            lappend seqInfo(second2) $id $indexAt2 $dock2 $dockAt2 
            lappend seqInfo(second2) $index2 $id $idDock $idDock 
            lappend altIndex $indexAt2
         } else {
            lappend seqInfo(second2) $index2 $indexAt2 $dock2 $dockAt2 
         }
      }
   }
# puts "prime= $seqInfo(prime2)"
# puts "second= $seqInfo(second2)"
   set altIndex [concat $altIndex $idList]
# puts "altIndex $altIndex"
   set num [llength $idList]
   set type MID
   for {set i 0} {$i < $num} {incr i} {
      set indx [lindex $idList $i]
      if {[info exists seqInfo($indx,midpoint)] == 0} {
         set seqInfo($indx,midpoint) MID
      }
      if {($type == "MID") && ($seqInfo($indx,midpoint) != "MID")} {
        set type $seqInfo($indx,midpoint)
      }
   }
# puts "type = $type"
   if {$type == "MID"} {
      set drt "simMax($seqInfo($index,drt)"
      for {set i 1} {$i < $num} {incr i} {
         set indx [lindex $idList $i]
         set drt "$drt,$seqInfo($indx,drt)"
      }
      set drt $drt)
#  puts "drt= $drt"
   } else {
      set drt "simMax($seqInfo($index,drt)"
      set seqInfo($id,drtFront) "simMax("
      set seqInfo($id,drtEnd) "simMax("
      for {set i 0} {$i < $num} {incr i} {
         set indx [lindex $idList $i]
         if {$seqInfo($indx,midpoint) == "MID"} {
            set seqInfo($id,drtFront) \
                "$seqInfo($id,drtFront)($seqInfo($indx,drt))/2.0,"
            set seqInfo($id,drtEnd) \
                "$seqInfo($id,drtEnd)($seqInfo($indx,drt))/2.0,"
         } elseif {$seqInfo($indx,midpoint) == "FRONT"} {
            set seqInfo($id,drtFront)  \
                "$seqInfo($id,drtFront)$seqInfo($indx,mpoff),"
            set seqInfo($id,drtEnd)  \
                "$seqInfo($id,drtEnd)$seqInfo($indx,drt)-($seqInfo($indx,mpoff)),"
         } else {
            set seqInfo($id,drtFront)  \
                "$seqInfo($id,drtFront)$seqInfo($indx,drt)-($seqInfo($indx,mpoff)),"
            set seqInfo($id,drtEnd)  \
                "$seqInfo($id,drtEnd)$seqInfo($indx,mpoff),"
         }
      }
      set seqInfo($id,drtFront) [string trimright $seqInfo($id,drtFront) ,]
      set seqInfo($id,drtEnd) [string trimright $seqInfo($id,drtEnd) ,]
      set seqInfo($id,drtFront) "$seqInfo($id,drtFront))"
      set seqInfo($id,drtEnd) "$seqInfo($id,drtEnd))"
      set drt "$seqInfo($id,drtFront)+$seqInfo($id,drtEnd)"
      set seqInfo($id,midpoint) $type
   }
   set j $seqInfo($id,drt,index) 
   set seqInfo($id,a$j,val) $drt
   set seqInfo($id,drt) $drt
   lappend indexList $index
   for {set i 1} {$i < $num} {incr i} {
            set indx [lindex $idList $i]
            set second2 $seqInfo(second2)
            set seqInfo(second2) {}
#puts "check $indx in $altIndex"
            foreach {index2 indexAt2 dock2 dockAt2} $second2 {
               if {($index2 == $indx) && \
                   ([lsearch $altIndex $indexAt2] != -1)} {
# puts "reset $index2 $indexAt2 $dock2 $dockAt2 to $index2 $id $dock2 $dockAt2"
                  lappend seqInfo(second2) $index2 $id $dock2 $dock2 
                  set seqInfo($index2,simId) $id
                  lappend indexList $index2
                  lappend altIndex $index2
               } elseif {([lsearch $indexList $indexAt2] != -1) && 
                   ((($dock2 == "front") && ($dockAt2 == "end")) ||
                    (($dock2 == "end") && ($dockAt2 == "front")))} {
                  if {[lsearch $simIds $seqInfo($index2,origId)] == -1} {
                     set indx2 [lsearch $seqInfo(second2) $id]
                     set indx2 [expr $indx2 + 4]
                     set seqInfo(second2) [linsert $seqInfo(second2) $indx2 \
                         $index2 $id $dock2 $dockAt2]
# puts "front attach $index2 $indexAt2 $dock2 $dockAt2 to $index2 $id $dock2 $dockAt2"
                  } else {
                  lappend seqInfo(second2) $index2 $id $dock2 $dockAt2 
# puts "reattach $index2 $indexAt2 $dock2 $dockAt2 to $index2 $id $dock2 $dockAt2"
                  }
                  set seqInfo($index2,simId) $id
               } elseif {($index2 == $indx)} {
                  lappend seqInfo(second2) $index2 $id $dock2 $dock2 
                  set seqInfo($index2,simId) $id
                  lappend indexList $index2
               } else {
                  lappend seqInfo(second2) $index2 $indexAt2 $dock2 $dockAt2 
               }
            }
   }
   if {$redo == 1} {
      checkFrontEnd
   }
# puts "prime2= $seqInfo(prime2)"
# puts "second2= $seqInfo(second2)"
}

proc calcDockPts {} {
   global dockPts seqInfo
   set dId 0
   catch {unset dockPts}
   foreach {index indexAt dock dockAt} $seqInfo(prime2) {
      if {[info exists dockPts($indexAt,$dockAt)] == 1} {
         set oldId $dockPts($indexAt,$dockAt)
         set dockPts($index,$dock) $oldId
         lappend dockPts($oldId) $index $dock
      } else {
         incr dId
         set dockPts($index,$dock) $dId
         set dockPts($indexAt,$dockAt) $dId
         lappend dockPts($dId) $indexAt $dockAt $index $dock
      }
   }
   foreach {index indexAt dock dockAt} $seqInfo(second2) {
      if {[info exists dockPts($indexAt,$dockAt)] == 1} {
         set oldId $dockPts($indexAt,$dockAt)
         set dockPts($index,$dock) $oldId
         lappend dockPts($oldId) $index $dock
      } else {
         incr dId
         set dockPts($index,$dock) $dId
         set dockPts($indexAt,$dockAt) $dId
         lappend dockPts($dId) $indexAt $dockAt $index $dock
      }
   }
   set dockPts(num) $dId
#  parray dockPts
}

proc sortElems {} {
  global seqInfo
  foreach {index2 indexAt2 dock2 dockAt2} $seqInfo(prime2) {
    lappend idList $index2
  }
# puts "sort: $seqInfo(second2)"
  set badList $seqInfo(second2)
  set seqInfo(second2) {}
  set ok 1
  while {([llength $badList] > 1) && ($ok == 1)} {
# puts "idList= $idList"
# puts "badList= $badList"
    set second $badList
    set badList {}
    set ok 0
    foreach {index2 indexAt2 dock2 dockAt2} $second {
      if {[lsearch $idList $indexAt2] != -1} {
        lappend seqInfo(second2) $index2 $indexAt2 $dock2 $dockAt2
        lappend idList $index2
        set ok 1
      } else {
        lappend badList $index2 $indexAt2 $dock2 $dockAt2
      }
    }
    if {$ok == 0} {
      set id [lindex $badList 0]
      setError $seqInfo($id,parent) 1 "Programming error.  Most likely related to docking arrangement"
    }
  }
# puts "sort done: $seqInfo(second2)"
}

proc checkSim2 {} {
   global seqInfo dockPts
   calcDockPts
# parray dockPts
   set dId $dockPts(num)
   for {set i 1} {$i <= $dId} {incr i} {
      set frontElem 0
      set midElem 0
      set endElem 0
      set frontList {}
      set endList {}
      set midList {}
      foreach {el dock} $dockPts($i) {
         if {$seqInfo($el,name) == "psgSimDelay"} {
            if {$dock == "front"} {
               lappend frontList $el
               set frontElem $el
            } elseif {$dock == "end"} {
               lappend endList $el
               set endElem $el
            } elseif {$dock == "mid"} {
               lappend midList $el
               set midElem $el
            }
         }
      }
      if {$midElem != 0} {
         if {$frontElem != 0} {
            set lab1 $seqInfo($seqInfo($midElem,parent),elemLabel)
            set lab2 $seqInfo($seqInfo($frontElem,parent),elemLabel)
            setError $seqInfo($midElem,parent) 0  \
               "middle of $lab1 incorrectly docked to front of $lab2."
            setError $seqInfo($frontElem,parent) 3 \
               "middle of $lab1 incorrectly docked to front of $lab2."
         } elseif {$endElem != 0} {
            set lab1 $seqInfo($seqInfo($midElem,parent),elemLabel)
            set lab2 $seqInfo($seqInfo($endElem,parent),elemLabel)
            setError $seqInfo($midElem,parent) 0  \
               "middle of $lab1 incorrectly docked to end of $lab2."
            setError $seqInfo($endElem,parent) 3 \
               "middle of $lab1 incorrectly docked to end of $lab2."
         }
      }
      if {[llength $frontList] > 1} {
# puts "FF sim: $frontList"
         setSimDelays $i front $frontList
      }
      if {[llength $midList] > 1} {
# puts "MM sim: $midList"
         setSimDelays $i mid $midList
      }
      if {[llength $endList] > 1} {
# puts "EE sim: $endList"
         setSimDelays $i end $endList
      }
   }
}

proc checkSim4 {} {
   global seqInfo dockList simPts
   set elems $seqInfo(elems)
   set dId 0
# parray seqInfo
   catch {unset simPts}
   set lastPhase phase
   set idMap(start) start
   set idMap(phase) phase
   set seqInfo(start,frontId) 1
   set seqInfo(start,style) if
   set seqInfo(start,chanId) 1
   set simPts(prime,start) start
   for {set i 1} {$i <= $elems} {incr i} {
# puts "element $i name is $seqInfo($i,name)"
      if {($seqInfo($i,name) != "channel") && \
          ($seqInfo($i,name) != "pulseSequence")} {
      set idMap($seqInfo($i,id)) $i
# puts "idMap($seqInfo($i,id)) $idMap($seqInfo($i,id))"
      }
   }
   for {set i 1} {$i <= $elems} {incr i} {
# puts "element $i name is $seqInfo($i,name)"
      if {($seqInfo($i,name) != "channel") && \
          ($seqInfo($i,name) != "pulseSequence")} {
   set id $seqInfo($i,id)
   set idAt $seqInfo($i,idAt)
   if {$idAt == $lastPhase} {
      set lastPhase $id
# puts "last phase now $id"
      continue
   }
   set mId $idMap($id)
   set mIdAt $idMap($idAt)
# puts "elem $i: id= $id idAt= $idAt mId= $mId  mIdAt= $mIdAt"
   set dock $seqInfo($i,dock)
   set dockAt $seqInfo($i,dockAt)
# puts "dock $dock of $id to $dockAt of $idAt"
   set id2 $dockList($id,$dock,index)
   set idAt2 $dockList($idAt,$dockAt,index)
   set dock2 $dockList($id,$dock,dock)
   set dockAt2 $dockList($idAt,$dockAt,dock)
# puts "dock2 $dock2 of $id2 to $dockAt2 of $idAt2"
   set simPts(prime,$i) $id2
# puts "seqInfo($idAt2,style) $seqInfo($idAt2,style)"
         if {[info exists simPts($dockAt,$mIdAt)] == 1} {
            set oldId $simPts($dockAt,$mIdAt)
            set simPts($dock,$mId) $oldId
            lappend simPts($oldId) $mId $dock
            incr simPts($oldId,$dock)
# puts "2: dock $dock seqInfo($id2,name) $seqInfo($id2,name)"
            if {$seqInfo($mId,frontId) != 1} {
               incr simPts($oldId,sims)
               set simPts($oldId,sim,$simPts($oldId,sims),id) $mId
               lappend simPts($oldId,sim,ids) $mId
               set simPts($oldId,sim,$simPts($oldId,sims),dock) $dock
               if {$dock == "front"} {
                  set simPts($oldId,eOk) 0
               } elseif {$dock == "end"} {
                  set simPts($oldId,fOk) 0
               }
               lappend simPts($oldId,sim,$dock) $mId
            } elseif {$seqInfo($id2,name) == "psgDelay"} {
               lappend simPts($oldId,delay,$dock) $mId
            } elseif {$seqInfo($id2,name) == "psgSimDelay"} {
               if {$dock == "front"} {
                  set simPts($oldId,eOk) 0
               } elseif {$dock == "end"} {
                  set simPts($oldId,fOk) 0
               }
            }
         } else {
            incr dId
            set simPts($dock,$mId) $dId
            set simPts($dockAt,$mIdAt) $dId
            lappend simPts($dId) $mIdAt $dockAt $mId $dock
            set simPts($dId,front) 0
            set simPts($dId,mid) 0
            set simPts($dId,end) 0
            set simPts($dId,true) 0
            set simPts($dId,false) 0
            incr simPts($dId,$dockAt)
            incr simPts($dId,$dock)
            set simPts($dId,sims) 0
            set simPts($dId,simFirst) $mIdAt
            set simPts($dId,fOk) 1
            set simPts($dId,eOk) 1
            set simPts($dId,sim,front) {}
            set simPts($dId,sim,end) {}
            set simPts($dId,sim,mid) {}
            set simPts($dId,delay,front) {}
            set simPts($dId,delay,mid) {}
            set simPts($dId,delay,end) {}
# puts "1: dockAt $dockAt seqInfo($idAt2,name) $seqInfo($idAt2,name)"
            if {$seqInfo($mIdAt,frontId) != 1} {
               incr simPts($dId,sims)
               set simPts($dId,sim,$simPts($dId,sims),id) $mIdAt
               lappend simPts($dId,sim,ids) $mIdAt
               set simPts($dId,sim,$simPts($dId,sims),dock) $dockAt
               if {$dockAt == "front"} {
                  set simPts($dId,eOk) 0
               } elseif {$dockAt == "end"} {
                  set simPts($dId,fOk) 0
               }
               lappend simPts($dId,sim,$dockAt) $mIdAt
            } elseif {(($seqInfo($idAt2,style) == "if") || \
                   ($seqInfo($idAt2,style) == "while")) && \
                   ($dockAt != "front")} {
              set simPts($dId,fOk) 0
              set simPts($dId,fOkId) $idAt2
            } elseif {$seqInfo($idAt2,name) == "psgDelay"} {
               lappend simPts($dId,delay,$dockAt) $mIdAt
            } elseif {$seqInfo($idAt2,name) != "psgSimDelay"} {
               if {$dockAt == "front"} {
                  set simPts($dId,eOk) 0
               } elseif {$dockAt == "end"} {
                  set simPts($dId,fOk) 0
               }
            }
# puts "1: dock $dock seqInfo($id2,name) $seqInfo($id2,name)"
            if {$seqInfo($mId,frontId) != 1} {
               incr simPts($dId,sims)
               set simPts($dId,sim,$simPts($dId,sims),id) $mId
               lappend simPts($dId,sim,ids) $mId
               set simPts($dId,sim,$simPts($dId,sims),dock) $dock
               if {$dock == "front"} {
                  set simPts($dId,eOk) 0
               } elseif {$dock == "end"} {
                  set simPts($dId,fOk) 0
               }
               lappend simPts($dId,sim,$dock) $mId
            } elseif {$seqInfo($id2,name) == "psgDelay"} {
               lappend simPts($dId,delay,$dock) $mId
# puts "simPts($dId,delay,$dock) $simPts($dId,delay,$dock)"
            } elseif {$seqInfo($id2,name) != "psgSimDelay"} {
               if {$dock == "front"} {
                  set simPts($dId,eOk) 0
               } elseif {$dock == "end"} {
                  set simPts($dId,fOk) 0
               }
            }
         }
      }
   }
   set simPts(num) $dId
#  parray simPts
}

proc checkSim4B {} {
   global seqInfo simPts

   for {set i 1} {$i <= $simPts(num)} {incr i} {
      if {$simPts($i,sims) > 0} {
         if {($simPts($i,eOk) == 0) && ($simPts($i,fOk) == 0)} {
            if {$simPts($i,delay,end) != ""} {
               set idAt [lindex $simPts($i,delay,end) 0]
# puts "idAt $idAt seqInfo($idAt,idAt) $seqInfo($idAt,id)"
               set idAt $seqInfo($idAt,id)
               set newId [makeNewSimDelay $idAt]
               set seqInfo($newId,dock) end
               set seqInfo($newId,dockAt) end
               set firstFront 0
               set firstEnd 0
               for {set j 1} {$j <= $simPts($i,sims)} {incr j} {
                  if {($simPts($i,sim,$j,dock) == "front") && \
                      ($firstFront == 0)} {
                     set firstFront $simPts($i,sim,$j,id)
# puts "1: seqInfo($firstFront,idAt) from $seqInfo($firstFront,idAt) to $idAt"
                     set seqInfo($firstFront,preDelay) $newId
                     if {$simPts($i,delay,front) != ""} {
                        set id2 [lindex $simPts($i,delay,front) 0]
                        set seqInfo($firstFront,idAt) $seqInfo($id2,id)
                        set seqInfo($firstFront,dockAt) front
                     } else {
                        set seqInfo($firstFront,idAt) $idAt
                        set seqInfo($firstFront,dockAt) end
                     }
                     set firstFront $seqInfo($firstFront,id)
                     set seqInfo($newId,idAt) $idAt
                     set seqInfo($newId,parentId) $firstFront
# puts "1a: newElem seqInfo($newId,idAt) $seqInfo($newId,idAt)"
                  } elseif {($simPts($i,sim,$j,dock) == "end") && \
                      ($firstEnd == 0)} {
                     set firstEnd $simPts($i,sim,$j,id)
# puts "2: seqInfo($firstEnd,idAt) from $seqInfo($firstEnd,idAt) to $newId"
                     set seqInfo($firstEnd,idAt) $newId
                     set seqInfo($firstEnd,dockAt) front
                     set firstEnd $seqInfo($firstEnd,id)
                  } elseif {$simPts($i,sim,$j,dock) == "front"} {
                     set id $simPts($i,sim,$j,id)
# puts "3: seqInfo($if,idAt) from $seqInfo($if,idAt) to $firstFront"
                     set seqInfo($id,idAt) $firstFront
                     set seqInfo($id,dockAt) front
                  } elseif {$simPts($i,sim,$j,dock) == "end"} {
                     set id $simPts($i,sim,$j,id)
# puts "4: seqInfo($if,idAt) from $seqInfo($if,idAt) to $firstEnd"
                     set seqInfo($id,idAt) $firstEnd
                     set seqInfo($id,dockAt) end
                  }
               }
            } elseif {$simPts($i,delay,front) != ""} {
               set idAt [lindex $simPts($i,delay,front) 0]
# puts "idAt $idAt seqInfo($idAt,id) $seqInfo($idAt,id)"
               set saveIdAt $idAt
               set idAt $seqInfo($idAt,id)
               set newId [makeNewSimDelay $idAt]
               set seqInfo($newId,dock) front
               set seqInfo($newId,dockAt) front
               set firstFront 0
               set firstEnd 0
               for {set j 1} {$j <= $simPts($i,sims)} {incr j} {
                  if {($simPts($i,sim,$j,dock) == "front") && \
                      ($firstFront == 0)} {
                     set firstFront $simPts($i,sim,$j,id)
# puts "f: seqInfo($firstFront,idAt) from $seqInfo($firstFront,idAt) to $newId (idAt= $idAt)"
                     if {$seqInfo($firstFront,idAt) != $idAt} {
# puts "set seqInfo($saveIdAt,idAt) from $seqInfo($saveIdAt,idAt) to $seqInfo($firstFront,idAt)"
                        set seqInfo($saveIdAt,idAt) $seqInfo($firstFront,idAt)
                        set seqInfo($saveIdAt,dockAt) end
                     }
                     set seqInfo($firstFront,idAt) $newId
                     set seqInfo($firstFront,dockAt) end
                     set firstFront $seqInfo($firstFront,id)
                     set seqInfo($newId,idAt) $idAt
                     set seqInfo($newId,parentId) $firstFront
# puts "f: seqInfo($newId,idAt) $seqInfo($newId,idAt)"
                  } elseif {($simPts($i,sim,$j,dock) == "end") && \
                      ($firstEnd == 0)} {
                     set firstEnd $seqInfo($simPts($i,sim,$j,id),id)
                  } elseif {$simPts($i,sim,$j,dock) == "front"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstFront
                     set seqInfo($id,dockAt) front
                  } elseif {$simPts($i,sim,$j,dock) == "end"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstEnd
                     set seqInfo($id,dockAt) end
                  }
               }
            } elseif {$simPts($i,delay,mid) != ""} {
               set idAt [lindex $simPts($i,delay,mid) 0]
# puts "idAt $idAt seqInfo($idAt,id) $seqInfo($idAt,id)"
               set saveIdAt $idAt
               set idAt $seqInfo($idAt,id)
               catch {unset newId}
               set firstFront 0
               set firstEnd 0
               for {set j 1} {$j <= $simPts($i,sims)} {incr j} {
                  if {($simPts($i,sim,$j,dock) == "front") && \
                      ($firstFront == 0)} {
                     set firstFront $simPts($i,sim,$j,id)
                     if {[info exists newId] != 1} {
                        set newId [makeNewSimDelay $seqInfo($firstFront,id)]
                     }
# puts "m2: seqInfo($firstFront,idAt) from $seqInfo($firstFront,idAt) to $idAt"

                     set seqInfo($firstFront,idAt) $idAt
                     set seqInfo($firstFront,dock) front
                     set seqInfo($firstFront,dockAt) mid
                     set firstFront $seqInfo($firstFront,id)
                     set seqInfo($newId,idAt) $idAt
                     set seqInfo($newId,dock) end
                     set seqInfo($newId,dockAt) mid
                     set seqInfo($newId,parentId) $firstFront
# puts "m: seqInfo($newId,idAt) $seqInfo($newId,idAt)"
                  } elseif {($simPts($i,sim,$j,dock) == "end") && \
                      ($firstEnd == 0)} {
                     set firstEnd $simPts($i,sim,$j,id)
                     if {[info exists newId] != 1} {
                        set newId [makeNewSimDelay $seqInfo($firstEnd,id)]
                     }
                     set seqInfo($newId,idAt) $idAt
                     set seqInfo($newId,dock) end
                     set seqInfo($newId,dockAt) mid
                     set seqInfo($newId,parentId) $firstEnd
# puts "m3: seqInfo($firstEnd,idAt) from $seqInfo($firstEnd,idAt) to $newId"
                     set seqInfo($firstEnd,idAt) $newId
                     set seqInfo($firstEnd,dockAt) front
                     set firstEnd $seqInfo($simPts($i,sim,$j,id),id)
                  } elseif {$simPts($i,sim,$j,dock) == "front"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstFront
                     set seqInfo($id,dockAt) front
                  } elseif {$simPts($i,sim,$j,dock) == "end"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstEnd
                     set seqInfo($id,dockAt) end
                  }
               }
            } else {
               set firstFront 0
               set firstEnd 0
# puts "3a0: "
               catch {unset newId}
               for {set j 1} {$j <= $simPts($i,sims)} {incr j} {
                  if {($simPts($i,sim,$j,dock) == "front") && \
                      ($firstFront == 0)} {
                     set firstFront $simPts($i,sim,$j,id)
                     if {[info exists newId] != 1} {
                        set newId [makeNewSimDelay $seqInfo($firstFront,id)]
                     }
                     if {$seqInfo($firstFront,dock) == "front"} {
                       set seqInfo($newId,dock) front
                       set seqInfo($newId,dockAt) $seqInfo($firstFront,dockAt)
                       set seqInfo($newId,idAt) $seqInfo($firstFront,idAt)
                       set seqInfo($newId,parentId) $firstFront
# puts "3a1: newElem seqInfo($newId,idAt) $seqInfo($newId,idAt)"
                       set seqInfo($firstFront,dockAt) end
# puts "3a2: seqInfo($firstFront,idAt) from $seqInfo($firstFront,idAt) to $newId"
                       set seqInfo($firstFront,idAt) $newId
                       set firstFront $seqInfo($firstFront,id)
                     } else {
                       set seqInfo($newId,dock) end
                       set seqInfo($newId,dockAt) front
                       set seqInfo($newId,idAt) $seqInfo($firstFront,id)
                       set seqInfo($newId,parentId) $firstFront
# puts "3b: newElem seqInfo($newId,idAt) $seqInfo($newId,idAt) firstFront $firstFront"
#                       set seqInfo($firstFront,dockAt) end
                       set firstFront $seqInfo($firstFront,id)
                       foreach {id2 dock2} $simPts($i) {
# puts "3b3: seqInfo($id2,idAt) $seqInfo($id2,idAt)"
                          if {$seqInfo($id2,idAt) == $firstFront} {
# puts "3b4: set seqInfo($id2,idAt) $newId"
                             set seqInfo($id2,idAt) $newId
                          }
                       }
                     }
                  } elseif {($simPts($i,sim,$j,dock) == "end") && \
                      ($firstEnd == 0)} {
                     set firstEnd $simPts($i,sim,$j,id)
# puts "3c1: seqInfo($firstEnd,dock) $seqInfo($firstEnd,dock)"
                     if {$seqInfo($firstEnd,dock) == "end"} {
                        if {[info exists newId] != 1} {
                           set newId [makeNewSimDelay $seqInfo($firstEnd,id)]
                        }
                        set seqInfo($firstEnd,idAt) $newId
# puts "3c: seqInfo($firstEnd,idAt) from $seqInfo($firstEnd,idAt) to $newId"
                     }
                     set firstEnd $seqInfo($firstEnd,id)
                  } elseif {$simPts($i,sim,$j,dock) == "front"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstFront
                     set seqInfo($id,dockAt) front
                  } elseif {$simPts($i,sim,$j,dock) == "end"} {
                     set id $simPts($i,sim,$j,id)
                     set seqInfo($id,idAt) $firstEnd
                     set seqInfo($id,dockAt) end
                  }
               }
            }
         }
      } elseif {($simPts($i,front) == 0) && ($simPts($i,mid) > 1) && ($simPts($i,end) > 0)} {
#special case of multiple mid dockings to the end of something.
         foreach {index dock } $simPts($i) {
            if {$dock == "end"} {
               break
            }
         }
         if {[info exists seqInfo($index,parent)] == 1} {
            set index $seqInfo($index,parent)
         }
         setError $index 3 "attach the front of a delay to the end of $seqInfo($index,elemLabel)"
# this is the start of a fix for this problem.  Test p25
         set newId [makeNewSimDelay $index]
         set seqInfo($newId,dock) front
         set seqInfo($newId,dockAt) end
         set seqInfo($newId,idAt) $seqInfo($index,id)
         set seqInfo($newId,parentId) $seqInfo($index,id)
      }
   }
   checkSim4
}

proc checkFrontEnd2 {} {
   global seqInfo dockPts
   set idList {}
   foreach {index indexAt dock dockAt} $seqInfo(prime2) {
      lappend idList $index
   }
   set second2 $seqInfo(second2)
   set seqInfo(second2) {}
   set matchId {}
   set matchDock {}
   set resetId {}
   set resetDock {}
   set idLen [llength $idList]
# puts "checkFrontEnd2 second2 $second2"
   foreach {index indexAt dock dockAt} $second2 {
      set id [lsearch $idList $indexAt]
      if {$id != -1} {
         if {($dock == "front") && ($dockAt == "end")} {
            if {$id == [expr $idLen - 1]} {
               lappend seqInfo(prime2) $index $indexAt $dock $dockAt
               lappend idList $index
               incr idLen
# puts "prime dock: append $index $indexAt $dock $dockAt to prime2"
            } else {
               set idAt [lindex $idList [expr $id + 1]]
# puts "prime ifdock $index $indexAt $dock $dockAt to $index $idAt $dock $dock"
#foreach em [array names seqInfo $idAt,*] {
#puts "seqInfo($em): $seqInfo($em)"
#}
               if {$seqInfo($idAt,style) == "if"} {
# puts "prime ifdock $index $indexAt $dock $dockAt to $index $idAt $dock $dock"
# puts "prime $seqInfo(prime2)"
#puts "idList: $idList"
                  set id2 [expr (($id+1)*4) + 1]
                  set seqInfo(prime2) [lreplace $seqInfo(prime2) \
                      $id2 $id2 $index]
# puts "prime1: $seqInfo(prime2)"
                  set seqInfo(prime2) [linsert $seqInfo(prime2) \
                      [expr ($id + 1) * 4] $index $indexAt $dock $dockAt]
                  set idList [linsert $idList [expr $id + 1] $index]
                  incr idLen
# puts "prime2: $seqInfo(prime2)"
#puts "idList2: $idList"

               } else {
                  lappend seqInfo(second2) $index $idAt $dock $dock
# puts "prime dock1 $index $indexAt $dock $dockAt to $index $idAt $dock $dock"
               }
            }
         } elseif {($dockAt == "front") && ($dock == "end")} {
            if {$id == 0} {
               set seqInfo(prime2) [lreplace $seqInfo(prime2) 1 1 $index]
               set seqInfo(prime2) [linsert $seqInfo(prime2) 0 $index start front end]
               set idList [linsert $idList 0 $index]
               incr idLen
# puts "prime dock: prepend $index $indexAt $dock $dockAt to prime2"
# puts "prime $seqInfo(prime2)"
            } else {
               set idAt [lindex $idList [expr $id - 1]]
               if {$seqInfo($idAt,style) == "if"} {
# puts "prime $seqInfo(prime2)"
#puts "idList: $idList"
                  set id2 [expr (($id-1)*4) + 1]
                  set idSave [lindex $seqInfo(prime2) $id2]
                  set seqInfo(prime2) [lreplace $seqInfo(prime2) \
                      $id2 $id2 $index]
# puts "prime1: $seqInfo(prime2)"
                  set seqInfo(prime2) [linsert $seqInfo(prime2) \
                      [expr ($id - 1) * 4] $index $idSave $dockAt $dock]
                  set idList [linsert $idList [expr $id - 1] $index]
                  incr idLen
# puts "prime2: $seqInfo(prime2)"
#puts "idList2: $idList"

               } else {
                  if {[info exists seqInfo($seqInfo($index,parent),preDelay)] == 1} {
# puts "id reset from $id to $seqInfo($seqInfo($index,parent),preDelay)"
                     set id $seqInfo($seqInfo($index,parent),preDelay)
                     lappend seqInfo(second2) $index $id $dock $dock
                  } else {
                     lappend seqInfo(second2) $index \
                       [lindex $idList [expr $id - 1]] $dock $dock
                  }
# puts "prime dock2 $index $indexAt $dock $dockAt to $index [lindex $idList [expr $id - 1]] $dock $dock"
               }
            }
         } else {
            lappend seqInfo(second2) $index $indexAt $dock $dockAt
# puts "prime dock: $index $indexAt $dock $dockAt"
         }
      } else {
         lappend seqInfo(second2) $index $indexAt $dock $dockAt
      }
   }
   sortElems
   calcDockPts
   set dId $dockPts(num)
# parray dockPts
# puts "prime2:3 $seqInfo(prime2)"
# puts "second2:3 $seqInfo(second2)"
   set second2 $seqInfo(second2)
#foreach ell [array names seqInfo *,simId] {
#  puts "seqInfo($ell) $seqInfo($ell)"
#}
   set seqInfo(second2) {}
   foreach {index indexAt dock dockAt} $second2 {
#puts "seqInfo($index,origId) $seqInfo($index,origId)"
#puts "seqInfo($indexAt,origId) $seqInfo($indexAt,origId)"
      if {($dock == "front") && ($dockAt == "end")} {
         set indx $dockPts($index,front)
         if {[llength $dockPts($indx)] > 4} {
            set newIdAt [lindex $dockPts($indx) 0]
            set newDockAt [lindex $dockPts($indx) 1]
            if {[info exists seqInfo($indexAt,simList)] == 1} {
              set testList $seqInfo($indexAt,simList)
            } else {
              set testList $seqInfo($indexAt,origId)
            }
# puts "testList = $testList"
            if {[lsearch $testList $seqInfo($index,origId)] != -1} {
               foreach {tIdAt tDockAt} $dockPts($indx) {
                  if {([lsearch $testList $seqInfo($tIdAt,origId)] == -1) && \
                      ($tDockAt == "front")} {
                     set newIdAt $tIdAt
                     set newDockAt $tDockAt
                  }
               }
            }
            lappend seqInfo(second2) $index $newIdAt $dock $newDockAt
# puts "$index $dock: from $indexAt $dockAt to $newIdAt $newDockAt"
         } else {
            lappend seqInfo(second2) $index $indexAt $dock $dockAt
         }
# puts "$index $dock: dockPts($index,front)= $dockPts($dockPts($index,front))"
      } elseif {($dockAt == "front") && ($dock == "end")} {
         set indx $dockPts($index,end)
         if {[llength $dockPts($indx)] > 4} {
            set newIndex [lindex $dockPts($indx) 0]
            set newDockAt [lindex $dockPts($indx) 1]
            if {$seqInfo($index,drt) != "SIMMAX"} {
            if {[info exists seqInfo($indexAt,simList)] == 1} {
              set testList $seqInfo($indexAt,simList)
            } else {
              set testList $seqInfo($indexAt,origId)
            }
# puts "testList $testList"
            set foundEnd 0
            foreach {tIdAt tDockAt} $dockPts($indx) {
# puts "$tIdAt $seqInfo($tIdAt,origId) <> $index $seqInfo($index,origId) drt= $seqInfo($tIdAt,drt)"
# puts "lsearch testList $seqInfo($tIdAt,origId) [lsearch $testList $seqInfo($tIdAt,origId)]"
# puts "lsearch seqInfo(second2) $tIdAt [lsearch $seqInfo(second2) $tIdAt]"
#puts "tIdAt $tIdAt index $index"
# puts "seqInfo($seqInfo($tIdAt,origId),comps): $seqInfo($seqInfo($tIdAt,origId),comps)"
              if {$seqInfo($seqInfo($tIdAt,origId),comps) <= 1} {
               if {([lsearch $testList $seqInfo($tIdAt,origId)] == -1) && \
                   ($seqInfo($tIdAt,drt) != 0.0) && ($tIdAt != $index) && \
                   (($seqInfo($tIdAt,name) == "psgSimDelay") || \
                    ($seqInfo($tIdAt,name) == "psgDelay"))} {
                  if {($foundEnd == 0) || ($seqInfo($tIdAt,drt) == "SIMMAX")} {
                    if {$tDockAt != "mid"} {
                     set newIndex $tIdAt
                     set newDockAt $tDockAt
# puts "resetting newIndex: $newIndex newDockAt: $newDockAt"
# if {$seqInfo($tIdAt,drt) == "SIMMAX"} {
#    puts "seqInfo($tIdAt,parentId) $seqInfo($tIdAt,parentId)"
# }
                    }
                  }
                  if {$tDockAt == "end"} {
                     set foundEnd 1
                  }
                  if {$seqInfo($tIdAt,drt) == "SIMMAX"} {
                     set foundEnd 1
                     if {$seqInfo($tIdAt,parentId) == $seqInfo($index,origId)} {
                        set foundEnd 2
# puts "found seqInfo($tIdAt,parentId) == seqInfo($index,origId)"
                     }
                  }
               }
               if {$foundEnd == 2} {
                  break
               }
            }
            }
            }
            if {($newDockAt == "true") || ($newDockAt == "false")} {
               lappend seqInfo(second2) $indexAt $index front end
               set indx [lsearch $seqInfo(second2) $indexAt]
               set seqInfo(second2) [lreplace $seqInfo(second2) \
                  $indx $indx $index]
            } else {
               lappend seqInfo(second2) $index $newIndex $dock $newDockAt
# puts "$index $dock: from $indexAt $dockAt to $newIndex $newDockAt"
            }
         } else {
            lappend seqInfo(second2) $index $indexAt $dock $dockAt
         }
# puts "$index $dock: dockPts($index,end)= $dockPts($dockPts($index,end))"
# puts "second2a $seqInfo(second2)"
      } else {
         lappend seqInfo(second2) $index $indexAt $dock $dockAt
      }
   }
   sortElems
}

proc checkFrontEnd {} {
   global seqInfo dockPts
#check front dock
   set idCheck [lindex $seqInfo(prime2) 0]
# puts "check front for $idCheck"
   set check 1
   set newCheck -1
   while {$check >= 0} {
      set check -1
      set id -1
      foreach {index indexAt dock dockAt} $seqInfo(second2) {
         incr id
         if {($indexAt == $idCheck) && ($dock == "end") && ($dockAt == "front")} {
            set newCheck $index
            set check $id
         }
      }
      if {$check != -1} {
# puts "prepend to front $newCheck start front end"
         set seqInfo(prime2) [lreplace $seqInfo(prime2) 1 1 $newCheck]
         set seqInfo(prime2) \
             [linsert $seqInfo(prime2) 0 $newCheck start front end]
         set check [expr $check * 4]
         set seqInfo(second2) \
             [lreplace $seqInfo(second2) $check [expr $check + 3]]
         set idCheck $newCheck
# puts "prime2: $seqInfo(prime2)"
# puts "seqInfo(second2): $seqInfo(second2)"
# puts "new check front for $idCheck"
      }
   }
   set idCheck [lindex $seqInfo(prime2) [expr [llength $seqInfo(prime2)] - 4]]
# puts "check end for $idCheck"
   set check 1
   while {$check >= 0} {
      set check -1
      set mCheck -1
      set id -1
      foreach {index indexAt dock dockAt} $seqInfo(second2) {
         incr id
         if {($indexAt == $idCheck) && ($dockAt == "end")} {
            if {$dock == "front"} {
               set newCheck $index
               set check $id
            } elseif {$dock == "mid"} {
               set newMCheck $index
               set mCheck $id
            }
         }
      }
      set dock "front"
      if {($check == -1) && ($mCheck != -1)} {
         set check $mCheck
         set newCheck $newMCheck
         set dock "mid"
# puts "no front to end, but mid to end found"
      }
      if {$check != -1} {
# puts "append to end $newCheck $idCheck $dock end"
         lappend seqInfo(prime2) $newCheck $idCheck $dock end
         set check [expr $check * 4]
# puts "remove $check to [expr $check + 4] elements"
# puts "[lrange $seqInfo(second2) $check [expr $check + 3]]"
         set seqInfo(second2) \
             [lreplace $seqInfo(second2) $check [expr $check + 3]]
         set idCheck $newCheck
#  puts "prime2: $seqInfo(prime2)"
# puts "seqInfo(second2): $seqInfo(second2)"
# puts "new check end for $idCheck"
      }
   }
   checkFrontEnd2
}

if {$argc == 2} {
   set dpsFlag 1
   set seqName [lindex $argv 1]
   set seqFileName [file tail $seqName]
   set outName ${seqFileName}_dps
} else {
   set dpsFlag 0
   set seqName [lindex $argv 0]
   set seqFileName [file tail $seqName]
   set outName $seqFileName
}
set uD [file join $env(vnmruser) spincad]
if {$seqName == ""} {
   puts "Usage: spingen fileName"
   exit 1
} elseif {[file exists [file join $uD psglib $seqName]] != 1} {
   puts "spingen: Pulse sequence $seqName was not found"
   exit 2
}

set infoSeqName [file tail $seqName]
puts "seqName= $seqName"
catch {file delete [file join $env(vnmruser) seqlib $outName.psg]}
catch {file delete [file join $env(vnmruser) seqlib $outName.error]}

set errorFd -1
if {$dpsFlag == 1} {
   makeElemMenu [file join $env(vnmrsystem) spincad dpsFiles]
} else {
   makeElemMenu [file join $env(vnmrsystem) spincad psgFiles]
   catch {file delete [file join $env(vnmruser) seqlib ${outName}_dps.psg]}
   catch {file delete [file join $env(vnmruser) seqlib ${outName}_dps.error]}
}
# destroy .
setPsgInfo
loadPsg $seqName $uD
# for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
#    puts "element $i name is $seqInfo($i,name) with $seqInfo($i,attr) attributes"
#    for {set j 1} {$j <= $seqInfo($i,attr)} {incr j} {
#       puts "   attribute $j $seqInfo($i,a$j,attr): $seqInfo($i,a$j,val)"
#    }
# }
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
 puts "elem $seqInfo($i,name)"
   if {($seqInfo($i,name) != "channel") && \
       ($seqInfo($i,name) != "pulseSequence")} {
      set seqInfo(compvalue) compvalue_${i}_
      loadPulseElem $i $i
      set seqInfo($seqInfo($i,id),idIndex) $i
   }
}
# parray seqInfo
makePsgSimDelay
# for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
#    puts "element $i name is $seqInfo($i,name) with $seqInfo($i,attr) attributes"
#    if {$seqInfo($i,name) != "channel"} {
#       for {set j 1} {$j <= $seqInfo($i,attr)} {incr j} {
#          puts "   attribute $j $seqInfo($i,a$j,attr): $seqInfo($i,a$j,val)"
#       }
#       puts "$seqInfo($i,name) has $seqInfo($i,comps) components"
#       for {set k 1} {$k <= $seqInfo($i,comps)} {incr k} {
#       puts "component $seqInfo($i,$k,name) has $seqInfo($i,$k,attr) attributes"
#         if {$seqInfo($i,$k,comps) > 0} {
#          for {set j 1} {$j <= $seqInfo($i,$k,attr)} {incr j} {
#          puts "   attribute $j $seqInfo($i,$k,a$j,attr): $seqInfo($i,$k,a$j,val)"
#             }
#          }
#       }
#    }
# }
set seqInfo(0,style) ""
set seqInfo(start,idIndex) 0
set seqInfo(phase,idIndex) 0

# foreach nm [array names seqInfo *,id] {
#   puts "seqInfo($nm): $seqInfo($nm)"
# }
# foreach nm [array names seqInfo *,idAt] {
#   puts "seqInfo($nm): $seqInfo($nm)"
# }
# foreach nm [array names seqInfo *,idIndex] {
#   puts "seqInfo($nm): $seqInfo($nm)"
# }
# foreach nm [array names seqInfo *,style] {
#   puts "seqInfo($nm): $seqInfo($nm)"
# }
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {($seqInfo($i,name) != "channel") && \
       ($seqInfo($i,name) != "pulseSequence")} {
      set id $seqInfo($i,id)
      if {[info exists seqInfo($i,composite)] == 1} {
         set dockList($id,front,dock) $seqInfo($i,frontDock)
# puts "check $i with id $id and idAt $seqInfo($i,idAt) ($seqInfo($seqInfo($i,idAt),idIndex))"
         set iAt $seqInfo($seqInfo($i,idAt),idIndex)
#         if {($seqInfo($iAt,style) == "if") || \
#              ($seqInfo($iAt,style) == "while")} {
#puts "multi idAt dockList($id,front,index) to $i,1"
#            set dockList($id,front,index) $i,1
#         } else {
#            set dockList($id,front,index) $i,$seqInfo($i,frontId)
#puts "current dockList($id,front,index) to $i,$seqInfo($i,frontId)"
#         }
         set dockList($id,front,index) $i,$seqInfo($i,frontId)
         set dockList($id,mid,dock) $seqInfo($i,midDock)
         set dockList($id,mid,index) $i,$seqInfo($i,midId)
         set dockList($id,end,dock) $seqInfo($i,endDock)
         set dockList($id,end,index) $i,$seqInfo($i,endId)
# puts "check seqInfo($i,style)= $seqInfo($i,style)"
         if {$seqInfo($i,style) == "if"} {
            set dockList($id,true,dock) true
            set dockList($id,true,index) $i,$seqInfo($i,midId)
            set dockList($id,false,dock) false
            set dockList($id,false,index) $i,$seqInfo($i,midId)
         }
         if {$seqInfo($i,style) == "while"} {
            set dockList($id,true,dock) true
            set dockList($id,true,index) $i,$seqInfo($i,midId)
         }
      } else {
         set dockList($id,front,dock) front
         set dockList($id,front,index) $i
         set dockList($id,mid,dock) mid
         set dockList($id,mid,index) $i
         set dockList($id,end,dock) end
         set dockList($id,end,index) $i
         set seqInfo($i,lastId) 0
         if {$seqInfo($i,style) == "if"} {
            set dockList($id,true,dock) true
            set dockList($id,true,index) $i
            set dockList($id,false,dock) false
            set dockList($id,false,index) $i
         }
         if {$seqInfo($i,style) == "while"} {
            set dockList($id,true,dock) true
            set dockList($id,true,index) $i
         }
      }
   }
}
# parray seqInfo
# parray dockList
set seqInfo(prime) {}
set seqInfo(second) {}
set seqInfo(phase) {}
set seqInfo(prime2) {}
set seqInfo(second2) {}
set seqInfo(lastPhase) "phase"
set seqInfo(phaseLabels) {}
set seqInfo(phaseIds) {}
set seqInfo(phaseIndex) 0
# parray seqInfo
# parray dockList
if {$dpsFlag == 1} {
#  a trick so that pulses docked to the fromt of simultaneous dpsPulses 
#  work correctly
   set elems $seqInfo(elems)
   for {set i 1} {$i <= $elems} {incr i} {
      if {[info exists seqInfo($i,1,dpsName)] == 1} {
         set seqInfo($i,frontId) 2
      }
   }
}
checkSim4
checkSim4B
set elems $seqInfo(elems)
for {set i 1} {$i <= $elems} {incr i} {
# puts "element $i name is $seqInfo($i,name) with $seqInfo($i,attr) attributes"
   if {($seqInfo($i,name) != "channel") && \
       ($seqInfo($i,name) != "pulseSequence")} {
      if {($seqInfo($i,dock) == "front") && ($seqInfo($i,dockAt) == "end")} {
         serial $i prime second
      } else {
         serial $i second second
      }
   }
}
if {$errorFd != -1} {
   close $errorFd
   exit 3
}
#puts "seqInfo(phase)= $seqInfo(phase)"
# puts "second= $seqInfo(second)"
set outInfo(last) start
set outInfo(elems) {}
fixSpecialIfs
removeMarkers
# puts "second= $seqInfo(second)"
checkSim2
checkFrontEnd
# puts "2: prime2= $seqInfo(prime2)"
# puts "2: second2= $seqInfo(second2)"
set seqInfo(id2) 0
foreach {index indexAt dock dockAt} $seqInfo(prime2) {
   addPrimitive $index $indexAt $dock $dockAt
}
foreach {index indexAt dock dockAt} $seqInfo(second2) {
   addPrimitive $index $indexAt $dock $dockAt
}

foreach el $outInfo(elems) {
   if {[info exists outInfo($el,pIds)] == 1} {
      fixIfDelays $el
   }
}
# puts "before parallel $outInfo(elems)"
if {$dpsFlag == 1} {
   checkDpsParallel
}
checkParallel
# puts "after parallel $outInfo(elems)"

set outFd [open [file join $env(vnmruser) seqlib $outName.psg] w]
set infoNumCh 0
set obsCh 1
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {$seqInfo($i,name) == "channel"} {
      savePsgData "<channel>"
      for {set j 1} {$j <= $seqInfo($i,attr)} {incr j} {
         savePsgDataAttr $seqInfo($i,a$j,attr) $seqInfo($i,a$j,val)
      }
      savePsgData "</channel>"
      incr infoNumCh
   }
}
set outInfo(skipList) {relId parallel dock dockAt relIdAt style midpoint \
     mpoff number id idAt len ifstyle polar corrDrt dockType chanType chanId}
set outInfo(skipElems) {psgElseif psgEndif psgEndFor psgEndWhile}
set outInfo(skipList2) {input1 input2 input3 input4 input5 input6 \
     calc condition x y}
getFirstCalc
setPhaseInfo
putPreacq
# puts "put Info $outInfo(elems)"
foreach el $outInfo(elems) {
   checkSim3 $el
}
set pCount 0
if {$dpsFlag == 1} {
   foreach el $outInfo(elems) {
      set origId $outInfo($el,origIndex)
      if {[info exists seqInfo($origId,dpsName)] == 1} {
         set outInfo($el,name) $seqInfo($origId,dpsName)
         for {set j 1} {$j <= $outInfo($el,attr)} {incr j} {
            set tName [string tolower \
                       [string trim $outInfo($el,a$j,val) { \t"}]]
            if {[string compare $tName unaltered] == 0} {
               set outInfo($el,a$j,val) "not used"
            }
         }
# recombine split dps elements
         if {"$seqInfo($origId,drt)/2.0" == "$outInfo($el,drt)"} {
            set ix [lsearch -exact $outInfo(elems) $el,S]
            if {$ix >= 0} {
              set outInfo($el,S,drt) 0.0
              set atJ $outInfo($el,drt,index) 
              set outInfo($el,a$atJ,val) $seqInfo($origId,drt)
              set outInfo($el,drt) $seqInfo($origId,drt)
            }
         }
         if {$outInfo($el,drt) == 0.0} {
            set outInfo($el,name) psgDelay
         }
      } else {
         set origId $outInfo($el,origIndex)
         if {[info exists seqInfo($origId,dpsName)] == 1} {
            set outInfo($el,drt) 0.0
         }
      }
   }
}
foreach el $outInfo(elems) {
   if {$pCount > 0} {
      if {[info exists outInfo($el,parallelAt)] == 1} {
         savePsgDataAttr parallelAt $outInfo($el,parallelAt)
      } else {
         savePsgDataAttr parallelAt front
      }
      savePsgData "</parallel $pCount>"
   }
   set pCount [addElem3 $el]
   if {$outInfo($el,type) == "psgAcquire"} {
      set obsCh $outInfo($el,a$outInfo($el,chanId,index),val)
   }
}
close $outFd
if {$dpsFlag == 0} {
   set outFd [open [file join $env(vnmruser) spincad info $infoSeqName] w]
   puts $outFd "$seqInfo(nD) $infoNumCh $obsCh"
   for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
      if {$seqInfo($i,name) == "channel"} {
         puts $outFd "$seqInfo($i,type) $seqInfo($i,label)"
      }
   }
   close $outFd
   set outFd [open [file join $env(vnmruser) spincad info $infoSeqName.info] w]
   for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
      if {[info exists seqInfo($i,elemIndex)] == 1} {
         foreach {attr index} $pElem(infos,$seqInfo($i,elemIndex)) {
            puts $outFd "<info>"
            puts $outFd " name = $seqInfo($i,$attr)"
            puts $outFd " info = $pElem($index,$seqInfo($i,elemIndex))"
            if {[info exists seqInfo($i,errorId)] == 1} {
               puts $outFd " id = $seqInfo($i,errorId)"
            }
            puts $outFd "</info>"
         }
      }
   }
   close $outFd
}
exit 0
