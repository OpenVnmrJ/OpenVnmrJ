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
#      loadPulseElem $id,$i $parent
   }
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
              set seqInfo(indexId,0) $index
           } else {
              set seqInfo($index,errorId) $seqInfo($index,id)
              set seqInfo(indexId,$seqInfo($index,id)) $index
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
#   set el2 $outInfo($el,origIndex)
   if {($seqInfo($el,phaseCycle) != "Cyclops") && \
          ($seqInfo($el,phaseCycle) != "None")} {
         set chan $seqInfo($el,chanId)
         if {$chan == 0} {
            set chan 1
         }
         set i $seqInfo($el,phaseCycle,index)
# puts "reset outInfo($el,a$i,val) from $outInfo($el,a$i,val) to $seqInfo($chan,cyclops)"
         set seqInfo($el,a$i,val) $seqInfo($chan,cyclops)
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

proc setPhaseInfo {} {
   global seqInfo dpsFlag
   foreach el {1 2 3 4 5} {
      set id [makeNewPhase fad$el "0.0 180.0" t${el}Index 2]
      lappend seqInfo(phaseLabels) fad$el
      lappend seqInfo(phaseIds) $id,1
      set seqInfo(fad$el,id) $id,1
      set seqInfo($id,fad) fad$el
   }
   for {set ii 1} {$ii <= $seqInfo(elems)} {incr ii} {
      if {[info exists seqInfo($ii,composite)] == 1} {
         set el $ii,1
         for {set i 1} {$i <= $seqInfo($ii,comps)} {incr i} {
            if {[info exists seqInfo($ii,$i,phaseBase)] == 1} {
               set el $ii,$i
            }
         }
      } else {
         set el $ii
      }
      set seqInfo(curPhaseElem) $el
      if {$seqInfo($el,name) == "psgPhaseTable"} {
# puts "psgPhaseTable $el is docked, remove it"
         set el2 $el
# puts "orig phases $seqInfo(phase)"
         set seqInfo(phase) [ldelete $seqInfo(phase) $el2]
         set seqInfo($el2,tableUsed) 1
         set i $seqInfo($el2,phaseBase,index)
#  puts "1: $outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
         set seqInfo($el,a$i,val) [checkPhaseVal $seqInfo($el,a$i,val)]
# puts "new phases $seqInfo(phase)"
      } elseif {$seqInfo($el,name) == "psgSetPhaseTable"} {
# puts "found psgSetPhaseTable"
        setCyclops $el
         set el2 $el
         setPhaseName $el2 0
         set seqInfo($el2,tableUsed) 1
        set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
        set seqInfo($el,a$i,val) [checkPhase $seqInfo($el,a$i,val)]
# puts "new value $outInfo($el,a$i,val)"
        if {$seqInfo($el2,phaseTableName) == "oph"} {
           foreach el3 {1 2 3 4 5} {
              set j $seqInfo(fad$el3,id)
              if {$seqInfo($j,tableUsed) == 1} {
# puts "add fad$el3 to oph"
                 set seqInfo($el,a$i,val) \
                    [concat $seqInfo($el,a$i,val) + fad$el3]
              }
            }
         }
      } elseif {$seqInfo($el,name) == "psgSetPhase"} {
# puts "found psgSetPhase"
        setCyclops $el
        set el2 $el
        set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
        set seqInfo($el,a$i,val) [checkPhase $seqInfo($el,a$i,val)]
        set seqInfo($el2,tableUsed) 1
# puts "new value $outInfo($el,a$i,val)"
      } elseif {($dpsFlag == 1) && \
                ([info exists seqInfo($el,phaseBase)] == 1)} {
        setCyclops $el
        set el2 $el
        set i $seqInfo($el2,phaseBase,index)
        set seqInfo($el,a$i,val) [checkPhase $seqInfo($el,a$i,val)]
        set seqInfo($el2,tableUsed) 1
      }
   }
   sortPhaseList
   foreach el $seqInfo(phase) {
      set seqInfo(curPhaseElem) $el
      set el2 $el
      set i $seqInfo($el2,phaseBase,index)
# puts "$outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
# puts "outInfo($el,type) $outInfo($el,type)"
      if {$seqInfo($el,name) == "psgSetPhaseTable"} {
         setCyclops $el
         setPhaseName $el2 0
         set seqInfo($el2,tableUsed) 1
         set seqInfo($el,a$i,val) [checkPhase $seqInfo($el,a$i,val)]
         if {$seqInfo($el2,phaseTableName) == "oph"} {
           foreach el3 {1 2 3 4 5} {
              set j $seqInfo(fad$el3,id)
              if {$seqInfo($j,tableUsed) == 1} {
                 set seqInfo($el,a$i,val) \
                    [concat $seqInfo($el,a$i,val) + fad$el3]
              }
            }
         }
      } else {
# puts "2: $outInfo($el,a$i,attr) is $outInfo($el,a$i,val)"
         set seqInfo($el,a$i,val) [checkPhaseVal $seqInfo($el,a$i,val)]
      }
   }
}

proc checkIfConnection {id} {
   global seqInfo numIf

# puts "check element $id, current number of ifs is $numIf"
   for {set i 1} {$i <= $numIf} {incr i} {
      set ifIndex $seqInfo(ifs,$i)
      set ifId $seqInfo($ifIndex,id)
      set newId $seqInfo($id,id)
# puts "check element $id against ifId $ifId"
# puts "seqInfo($id,idAt): $seqInfo($id,idAt)"
# puts "seqInfo($id,dock): $seqInfo($id,dock)"
# puts "seqInfo($id,dockAt): $seqInfo($id,dockAt)"
      if {$seqInfo($id,idAt) == $ifId} {
#         puts "element $id connected to if $ifId"
# puts "seqInfo($id,dockAt): $seqInfo($id,dockAt)"
        if {$seqInfo($id,dockAt) == "true"} {
           set seqInfo($ifIndex,trueId) $newId
           set seqInfo($ifIndex,trueEndId) $newId
        } elseif {$seqInfo($id,dockAt) == "false"} { 
           set seqInfo($ifIndex,falseId) $newId
           set seqInfo($ifIndex,falseEndId) $newId
        } elseif {$seqInfo($id,dockAt) == "end"} { 
           set seqInfo($ifIndex,ifEndId) $newId
        }
      } elseif { ($seqInfo($id,dock) == "front") && \
                 ($seqInfo($id,dockAt) == "end") } {
         if {$seqInfo($id,idAt) == $seqInfo($ifIndex,trueEndId)} {
           set seqInfo($ifIndex,trueEndId) $newId
         } elseif {$seqInfo($id,idAt) == $seqInfo($ifIndex,falseEndId)} {
           set seqInfo($ifIndex,falseEndId) $newId
         }
      }
   }
}

proc makeIfAttr {id} {
   global seqInfo numIf
   set seqInfo($id,trueId) 0
   set seqInfo($id,trueEndId) 0
   set seqInfo($id,falseId) 0
   set seqInfo($id,falseEndId) 0
   set seqInfo($id,ifEndId) 0
   incr numIf
   set seqInfo(ifs,$numIf) $id
}

proc putElem {i} {
    global seqInfo
    set iftype 0
    if {[info exists seqInfo($i,composite)] == 1} {
       for {set j 1} {$j <= $seqInfo($i,comps)} {incr j} {
           savePsgData "<$seqInfo($i,$j,name)>"
           for {set k 1} {$k <= $seqInfo($i,$j,attr)} {incr k} {
             if {($seqInfo($i,$j,a$k,val) != "Unaltered") && \
                 ([string tolower $seqInfo($i,$j,a$k,val)] != "not used")} {
               if {($seqInfo($i,$j,a$k,attr) == "idAt") && \
                   ($seqInfo($i,$j,a$k,val) == "start")} {
                   set seqInfo($i,$j,a$k,val) 0
               }
               if { ($seqInfo($i,$j,a$k,attr) == "dockAt") } {
                 if { ($seqInfo($i,$j,a$k,val) == "true")} {
                   set seqInfo($i,$j,a$k,val) "truebranch"
                 } elseif { ($seqInfo($i,$j,a$k,val) == "false")} {
                   set seqInfo($i,$j,a$k,val) "falsebranch"
                 }
               }
               if {[lsearch $seqInfo(skipList) $seqInfo($i,$j,a$k,attr)] == -1} {
                  savePsgDataAttr $seqInfo($i,$j,a$k,attr) $seqInfo($i,$j,a$k,val)
               }
             }
           }
           if {$seqInfo($i,$j,style) == "if"} {
              savePsgDataAttr trueId $seqInfo($i,trueId)
              savePsgDataAttr trueEndId $seqInfo($i,trueEndId)
              savePsgDataAttr falseId $seqInfo($i,falseId)
              savePsgDataAttr falseEndId $seqInfo($i,falseEndId)
              savePsgDataAttr endId $seqInfo($i,ifEndId)
              set iftype 1
           } elseif {$seqInfo($i,$j,style) == "while"} {
              savePsgDataAttr startLoopId $seqInfo($i,trueId)
              savePsgDataAttr lastLoopId $seqInfo($i,trueEndId)
              savePsgDataAttr postLoopId $seqInfo($i,ifEndId)
              set iftype 1
           }
           savePsgData "</$seqInfo($i,$j,name)>"
       }
    } else {
        savePsgData "<$seqInfo($i,name)>"
        for {set k 1} {$k <= $seqInfo($i,attr)} {incr k} {
          if {($seqInfo($i,a$k,val) != "Unaltered") && \
              ([string tolower $seqInfo($i,a$k,val)] != "not used")} {
             if {($seqInfo($i,a$k,attr) == "idAt") && \
                 ($seqInfo($i,a$k,val) == "start")} {
                 set seqInfo($i,a$k,val) 0
             }
             if {($seqInfo($i,a$k,attr) == "dockAt")} {
               if {($seqInfo($i,a$k,val) == "true")} {
                 set seqInfo($i,a$k,val) "truebranch"
               } elseif {($seqInfo($i,a$k,val) == "false")} {
                 set seqInfo($i,a$k,val) "falsebranch"
               }
             }
             if {[lsearch $seqInfo(skipList) $seqInfo($i,a$k,attr)] == -1} {
                savePsgDataAttr $seqInfo($i,a$k,attr) $seqInfo($i,a$k,val)
             }
          }
        }
        if {$seqInfo($i,style) == "if"} {
           savePsgDataAttr trueId $seqInfo($i,trueId)
           savePsgDataAttr trueEndId $seqInfo($i,trueEndId)
           savePsgDataAttr falseId $seqInfo($i,falseId)
           savePsgDataAttr falseEndId $seqInfo($i,falseEndId)
           savePsgDataAttr endId $seqInfo($i,ifEndId)
           set iftype 1
        } elseif {$seqInfo($i,style) == "while"} {
           savePsgDataAttr startLoopId $seqInfo($i,trueId)
           savePsgDataAttr lastLoopId $seqInfo($i,trueEndId)
           savePsgDataAttr postLoopId $seqInfo($i,ifEndId)
           set iftype 1
        }
        savePsgData "</$seqInfo($i,name)>"
    }
    return $iftype
}

proc putElems {start ifId force} {
   global seqInfo

   set elems $seqInfo(elems)
   for {set i $start} {$i <= $elems} {incr i} {
      if {($seqInfo($i,name) != "channel") && \
          ($seqInfo($i,name) != "pulseSequence")} {
        if {($ifId != 0) &&  ($force == 0) && \
            ([lsearch $seqInfo($ifId,ifDockList) $seqInfo($i,idAt)] == -1)} {
           continue
        } else {
           lappend seqInfo($ifId,ifDockList) $seqInfo($i,id)
        }
        set force 0
        set iftype [putElem $i]
        set seqInfo($i,name) "channel"
        if {$iftype == 1} {
           if {$seqInfo($i,trueId) != 0} {
              set seqInfo($i,ifDockList) {}
              lappend seqInfo($i,ifDockList) $seqInfo($i,trueId)
              putElems $seqInfo(indexId,$seqInfo($i,trueId)) $i 1
           }
           if {$seqInfo($i,falseId) != 0} {
              savePsgData "<psgElseif>"
              savePsgData "</psgElseif>"
              set seqInfo($i,ifDockList) {}
              lappend seqInfo($i,ifDockList) $seqInfo($i,falseId)
              putElems $seqInfo(indexId,$seqInfo($i,falseId)) $i 1
           }
           if {$seqInfo($i,style) == "if"} {
              savePsgData "<psgEndif>"
              savePsgData "</psgEndif>"
           } else {
              savePsgData "<psgEndFor>"
              savePsgData "</psgEndFor>"
           }
        }
      }
   }
}

set dpsFlag 1
set seqName [lindex $argv 0]
set seqFileName [file tail $seqName]
set outName ${seqFileName}_dps
set uD [file join $env(vnmruser) spincad]
if {$seqName == ""} {
   puts "Usage: dpsgen fileName"
   exit 1
} elseif {[file exists [file join $uD psglib $seqName]] != 1} {
   puts "dpsgen: Pulse sequence $seqName was not found"
   exit 2
}

set infoSeqName [file tail $seqName]
# puts "seqName= $seqName"

set errorFd -1
makeElemMenu [file join $env(vnmrsystem) spincad dpsFiles]
catch {file delete [file join $env(vnmruser) seqlib ${outName}_dps.psg]}
catch {file delete [file join $env(vnmruser) seqlib ${outName}_dps.error]}
# destroy .
loadPsg $seqName $uD
set numIf 0
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {[info exists seqInfo($i,style)] == 1} {
      if {($seqInfo($i,style) == "if") || ($seqInfo($i,style) == "while")} {
         makeIfAttr $i
      }
   }
}
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
      if {($seqInfo($i,name) != "channel") && \
          ($seqInfo($i,name) != "pulseSequence")} {
         checkIfConnection $i
      }
}
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
# puts "elem $seqInfo($i,name)"
   if {($seqInfo($i,name) != "channel") && \
       ($seqInfo($i,name) != "pulseSequence")} {
      set seqInfo(compvalue) compvalue_${i}_
      loadPulseElem $i $i
      set seqInfo($seqInfo($i,id),idIndex) $i
   }
}
set seqInfo(0,style) ""
set seqInfo(start,idIndex) 0
set seqInfo(phase,idIndex) 0

set seqInfo(phase) {}
set seqInfo(lastPhase) "phase"
set seqInfo(phaseLabels) {}
set seqInfo(phaseIds) {}
set seqInfo(phaseIndex) 0
set elems $seqInfo(elems)
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
    if {[info exists seqInfo($i,composite)] == 1} {
      set el $i,1
    } else {
      set el $i
    }
    if {$seqInfo($el,name) == "psgPhaseTable"} {
         setPhaseName $el 1
         set seqInfo($el,tableUsed) 0
    }
}

set seqInfo(skipList) {relId id2 parallel relIdAt style midpoint \
     mpoff number len ifstyle polar corrDrt dockType chanType}

set outFd [open [file join $env(vnmruser) seqlib $outName.psg] w]
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {$seqInfo($i,name) == "channel"} {
      savePsgData "<channel>"
      for {set j 1} {$j <= $seqInfo($i,attr)} {incr j} {
         if {[lsearch $seqInfo(skipList) $seqInfo($i,a$j,attr)] == -1} {
            savePsgDataAttr $seqInfo($i,a$j,attr) $seqInfo($i,a$j,val)
         }
      }
      savePsgData "</channel>"
   }
}
# getFirstCalc
setPhaseInfo
putPreacq
set pCount 0
set elems $seqInfo(elems)
# Output fad phases
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {[info exists seqInfo($i,fad)] == 1} {
      if {$seqInfo($i,name) == "psgPhaseTable"} {
         savePsgData "<$seqInfo($i,name)>"
         for {set k 1} {$k <= $seqInfo($i,attr)} {incr k} {
           if {$seqInfo($i,a$k,val) != "Unaltered"} {
             if {[lsearch $seqInfo(skipList) $seqInfo($i,a$k,attr)] == -1} {
                savePsgDataAttr $seqInfo($i,a$k,attr) $seqInfo($i,a$k,val)
             }
           }
         }
         savePsgData "</$seqInfo($i,name)>"
         set seqInfo($i,name) "channel"
      }
   }
}
for {set i 1} {$i <= $seqInfo(elems)} {incr i} {
   if {[info exists seqInfo($i,composite)] == 1} {
      for {set j 1} {$j <= $seqInfo($i,comps)} {incr j} {
         if {($seqInfo($i,$j,name) == "psgPhaseTable") || \
             ($seqInfo($i,$j,name) == "psgSetPhaseTable") } {
            savePsgData "<$seqInfo($i,$j,name)>"
            for {set k 1} {$k <= $seqInfo($i,$j,attr)} {incr k} {
               if {$seqInfo($i,$j,a$k,val) != "Unaltered"} {
                 if {($seqInfo($i,$j,a$k,attr) == "idAt") && \
                     ($seqInfo($i,$j,a$k,val) == "phase")} {
                    set seqInfo($i,$j,a$k,val) 0
                 }
                 if {[lsearch $seqInfo(skipList) $seqInfo($i,$j,a$k,attr)] == -1} {
                    savePsgDataAttr $seqInfo($i,$j,a$k,attr) $seqInfo($i,$j,a$k,val)
                 }
               }
            }
            savePsgData "</$seqInfo($i,$j,name)>"
            set seqInfo($i,name) "channel"
         }
      }
   } else {
      if {($seqInfo($i,name) == "psgPhaseTable") || \
          ($seqInfo($i,name) == "psgSetPhaseTable") } {
         savePsgData "<$seqInfo($i,name)>"
         for {set k 1} {$k <= $seqInfo($i,attr)} {incr k} {
           if {$seqInfo($i,a$k,val) != "Unaltered"} {
             if {($seqInfo($i,a$k,attr) == "idAt") && \
                 ($seqInfo($i,a$k,val) == "phase")} {
                set seqInfo($i,a$k,val) 0
             }
             if {[lsearch $seqInfo(skipList) $seqInfo($i,a$k,attr)] == -1} {
                savePsgDataAttr $seqInfo($i,a$k,attr) $seqInfo($i,a$k,val)
             }
           }
         }
         savePsgData "</$seqInfo($i,name)>"
         set seqInfo($i,name) "channel"
      }
   }
}
putElems 1 0 0
exit 0
