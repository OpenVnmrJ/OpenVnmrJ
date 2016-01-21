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
# dg Configurator


set elem(row) -1
set elem(col) -1
set elem(index) ""
catch {destroy .examp}

proc endConf {} {
   global dgConf dgInfo dgTemp
   catch {after cancel $dgConf(id)}
   catch {exec rm -f $dgConf(tmpfile)}
   unset dgConf
   unset dgInfo
   catch {unset dgTemp}
   destroy .examp
   destroy .dgC  
   bind .dgmain <Leave> {}
   rename getColors {}
   rename colorOk {}
   rename updateGeometry {}
   rename saveelem {}
   rename savedg {}
   rename flashbutton {}
   rename selectbutton {}
   rename setbutton {}
   rename elemconfig {}
   rename initButton {}
   rename selectIt {}
   rename showexp {}
   rename newexp {}
   rename globalSave {}
   rename setTemplate {}
   rename startConf {}
}

proc getColors {} {
   global dgConf

   set dgConf(colors) {}
   set file [file join /usr openwin lib X11 rgb.txt]
   if {[file exists $file] == 0} {
      set file [file join /usr lib X11 rgb.txt]
   }
   set f [open $file]
   while {[gets $f line] >= 0} {
      lappend dgConf(colors) [string tolower "[lrange $line 3 end]"]
   }
   close $f
}

proc colorOk {color} {
   global dgConf
   set clr [string tolower $color]
   return [lsearch $dgConf(colors) $clr]
}

proc saveDisplay {} {
   global dgConf dgInfo env seqfil

   if {$dgConf(save) == "sys"} {
     if {$dgConf(exp) == 1} {
       catch {exec mkdir \
         [file join $env(vnmrsystem) user_templates dg $seqfil]}
       exec cp $dgConf(tmpfile) \
         [file join $env(vnmrsystem) user_templates dg $seqfil $dgInfo(file)]
     } else {
       exec cp $dgConf(tmpfile) \
         [file join $env(vnmrsystem) user_templates dg default $dgInfo(file)]
     }
   } else {
     if {$dgConf(exp) == 1} {
       catch {exec mkdir -p [file join $env(vnmruser) templates dg $seqfil]}
       exec cp $dgConf(tmpfile) \
         [file join $env(vnmruser) templates dg $seqfil $dgInfo(file)]
     } else {
       exec cp $dgConf(tmpfile) \
         [file join $env(vnmruser) templates dg default $dgInfo(file)]
     }
   }
}

proc setTemplate {} {
   global dgTemplate dgTemp

   foreach el [array names dgTemplate] {
      set el2 [split $el ,]
      set type [lindex $el2 0]
      if {$type == "sticky"} {
        switch $dgTemplate($el) {
            nws     { set justify left   }
            news    { set justify center }
            nes     { set justify right  }
            default { set justify center }
        }
        set dgTemp(justify,[lindex $el2 1],[lindex $el2 2]) $justify
      } else {
        set dgTemp($el) $dgTemplate($el)
      }
   }
   set testElements {vnmrSet vnmrSet2 tclSet}
   for {set j 0} {$j < $dgTemplate(cols)} {incr j} {
      for {set i 0} {$i < $dgTemplate(rows)} {incr i} {
         foreach el $testElements {
            if {[info exists dgTemp($el,$i,$j)] == 1} then {
               if {[regsub -all value,$i,$j [set dgTemp($el,$i,$j)] \
                  VALUE dgTemp($el,$i,$j)] >= 1} {
                   regsub -all {dgTemplate\(VALUE\)} [set dgTemp($el,$i,$j)] \
                      VALUE dgTemp($el,$i,$j)
               }
            }
         }
      }
   }
}

proc updateGeometry {} {
   global dgInfo
   set dgInfo(geom) [wm geometry .]
}

proc saveelem {index FD} {
  global dgTemp dgConf

  puts $FD ""
  set type $dgTemp(type,$index)
  puts $FD "set dgIJ $index"
  foreach element $dgConf(elements) {
    if {[lsearch -exact $dgConf($type,elements) $element] != -1} then {
      if {[info exists dgTemp($element,$index)] == 1} then {
        if {($element == "color") && \
            ([colorOk $dgTemp($element,$index)] == -1)} {
           set dgTemp($element,$index) "black"
        }
        if {($element == "width") && ($type == "entry")} {
           set tst [catch {expr 1+$dgTemp(width,$index)}]
           if {($tst == 1) || ($dgTemp(width,$index) <= 0) } {
             set dgTemp($element,$index) 8
           }
        }
        if {$element == "value"} {
          puts $FD "set dgTemplate($element,\$dgIJ)  \{\}"
        } elseif {$element == "justify"} {
          switch $dgTemp(justify,$index) {
            left    { set sticky "nws"  }
            center  { set sticky "news" }
            right   { set sticky "nes"  }
            default { set sticky "news" }
          }
          if {($type == "button") && ($sticky == "news")} {
            set tst [catch {expr 1+$dgTemp(width,$index)}]
            if {($tst == 0) && ($dgTemp(width,$index) >= 0) } {
              set sticky "ns"
            }
          }
          puts $FD "set dgTemplate(sticky,\$dgIJ)  \{$sticky\}"
        } else {
          puts $FD "set dgTemplate($element,\$dgIJ)  \{$dgTemp($element,$index)\}"
        }
      }
    }
  }
}

proc savedg {} {
   global dgTemp dgConf dgInfo env

   set dgConf(tmpfile) [file join $env(vnmrsystem) tmp dg.[pid]]
   set outFD [open $dgConf(tmpfile) w]
   puts $outFD "set dgTemplate(rows)       $dgInfo(rows)"
   puts $outFD "set dgTemplate(cols)       $dgInfo(cols)"
   for {set j 0} {$j < $dgInfo(cols)} {incr j} {
      for {set i 0} {$i < $dgInfo(rows)} {incr i} {
         if {[info exists dgTemp(type,$i,$j)] == 1} then {
            if {($dgTemp(type,$i,$j) != "none") && \
                ($dgTemp(type,$i,$j) != "")} then {
               saveelem $i,$j $outFD
            }
         }
      }
   }
   close $outFD
   resetDgConf [file tail $dgConf(tmpfile)]
}

proc flashbutton {row col} {
   global elem dgConf
   if {$dgConf(flash) == "$row,$col"} then {
      set fg [.examp.box($row,$col) cget -foreground]
      .examp.box($row,$col) configure -foreground pink
      update idletasks
      after 250
      .examp.box($row,$col) configure -foreground $fg
      update idletasks
      set dgConf(id) [after 750 "catch {flashbutton $row $col}"]
   }
}

proc selectbutton {row col} {
   global elem dgTemp dgConf

   set elem(row) $row
   set elem(col) $col
   set elem(index) "[expr $row +1],[expr $col +1]"
   if {[info exists dgTemp(label,$row,$col)] == 1} then {
      set index 0
      foreach element $dgConf(elements) {
         if {[info exists dgTemp($element,$row,$col)] == 1} then {
            set elem($element) $dgTemp($element,$row,$col)
         } else {
            set elem($element) [lindex $dgConf(elemvals) $index]
#            set dgTemp($element,$row,$col) $elem($element)
         }
         incr index
      }
   } else {
      set index 0
      foreach element $dgConf(elements) {
         set elem($element) [lindex $dgConf(elemvals) $index]
         incr index
      }
   }
   if {($elem(type) == "") || ($elem(type) == "none")} then {
      .examp.box($row,$col) configure -background $dgConf(background) \
          -text $elem(index)
   } else {
      set lab [string range $elem(label) 0 3]
      .examp.box($row,$col) configure -background $dgConf(selectcolor) \
          -text $lab
   }
   elemconfig
   set dgConf(flash) $row,$col
   
}

proc setbutton {} {
   global elem dgTemp dgConf

   foreach element $dgConf(elements) {
      set dgTemp($element,$elem(row),$elem(col)) $elem($element)
    }
   if {($elem(type) == "") || ($elem(type) == "none")} then {
      .examp.box($elem(row),$elem(col)) configure -background $dgConf(background)
   } else {
      .examp.box($elem(row),$elem(col)) configure -background $dgConf(selectcolor)
   }
   savedg
}

proc elemconfig {} {
   global elem dgConf

   set bindex 0
   foreach butindex $dgConf(elements) {
      if {[lsearch -exact $dgConf($elem(type),elements) $butindex] != -1} then {
         .dgC.conf3Label($bindex) configure -foreground $dgConf(foreground)
         .dgC.conf3Entry($bindex) configure -state normal
      } else {
         .dgC.conf3Label($bindex) configure -foreground $dgConf(stipple)
         .dgC.conf3Entry($bindex) configure -state disabled
      }
      incr bindex
   }
}

proc initButton {row col} {
   global elem

   set elem(newRow) $row
   set elem(newCol) $col
}
proc selectIt {win x y} {
  global elem dgTemp dgConf

  set relwin [winfo containing $x $y]
  if {$win == $relwin} then {
      selectbutton $elem(newRow) $elem(newCol)
      flashbutton $elem(newRow) $elem(newCol)
  } elseif {($relwin != "") && (".examp" == [winfo parent $relwin])} then {
      scan $relwin ".examp.box(%d,%d)" row col
      set savetype $dgTemp(type,$elem(newRow),$elem(newCol))
      set dgTemp(type,$elem(newRow),$elem(newCol)) none
      selectbutton $elem(newRow) $elem(newCol)
      foreach element $dgConf(elements) {
         set dgTemp($element,$row,$col) $elem($element)
      }
      set dgTemp(type,$row,$col) $savetype
      selectbutton $row $col
      setbutton
  }
}

proc showexp {} {
   global dgConf dgInfo dgTemp

   wm deiconify .examp
   wm title .examp "$dgInfo(file)"
   grid rowconfigure .examp $dgInfo(rows) -minsize 1 -weight 1
   grid columnconfigure .examp $dgInfo(cols) -minsize 1 -weight 1
   for {set j 0} {$j < 16} {incr j} {
     for {set i 0} {$i < 20} {incr i} {
        catch {destroy .examp.box($i,$j)}
     }
   }

   for {set j 0} {$j < $dgInfo(cols)} {incr j} {
     for {set i 0} {$i < $dgInfo(rows)} {incr i} {
        catch {destroy .examp.box($i,$j)}
        button .examp.box($i,$j) -text "$dgConf(id,$i,$j)" -width 6 -padx 0 \
              -highlightthickness 0 \
              -height 1 -relief raised
        bind .examp.box($i,$j) <1> "initButton $i $j"
#             -height 1 -relief raised \
#             -command "selectbutton $i $j; flashbutton $i $j"
#        frame .examp.box($i,$j) -width ${elemWidth} \
#              -height ${elemHeight} -relief raised
        grid .examp.box($i,$j) -row $i -column $j -sticky news
        selectbutton $i $j
     }
   }
   bind .examp <ButtonRelease-1> {selectIt %W %X %Y}
   setbutton
}

proc newexp {} {
   global dgInfo dgTemp dgConf

   bind .dg <Configure> {}
   catch {unset dgTemp}
   if {[info exists dgConf(panlabel,$dgConf(panel))] != 1} {
      set dgConf(panlabel,$dgConf(panel)) $dgConf(panlabel)
   }
   set dgConf(panlabel) $dgConf(panlabel,$dgConf(panel))
   if {[info exists dgConf(panfile,$dgConf(panel))] != 1} {
      set dgConf(panfile,$dgConf(panel)) $dgInfo(file)
   }
   set dgInfo(file) $dgConf(panfile,$dgConf(panel))
   showexp
}

proc globalSave {} {
   global env
   catch {exec touch [file join $env(vnmrsystem) user_templates dg touchtest]}
   if {[file exists [file join $env(vnmrsystem) user_templates dg touchtest]] == 1} {
      exec rm [file join $env(vnmrsystem) user_templates dg touchtest]
      return 1
   } else {
      return 0
   }
}

proc startConf {} {
   global dgInfo dgConf dgTemplate dgLocal
   catch {destroy .dgC}
   toplevel .dgC
   wm title .dgC "Display Groups"
   wm iconname .dgC "Display"
   wm minsize .dgC 1 1
   frame .dgC.f0
   frame .dgC.f1
   frame .dgC.f2
   frame .dgC.f3
   frame .dgC.f4
   toplevel .examp -class Grid
   wm geometry .examp +0+0
   wm withdraw .examp
   updateGeometry
   set dgInfo(rows) $dgTemplate(rows)
   set dgInfo(cols) $dgTemplate(cols)
   set dgInfo(file) $dgLocal(file,$dgLocal(index))
   set dgInfo(label) $dgLocal(title,$dgLocal(index))
   for {set j 0} {$j < 16} {incr j} {
      for {set i 0} {$i < 20} {incr i} {
         set dgConf(id,$i,$j) "[expr $i+1],[expr $j+1]"
      }
   }

   label .dgC.panLabel(0) -text "Panel File: "
   entry .dgC.panEntry(0) -textvariable dgInfo(file) \
              -highlightthickness 0
   label .dgC.panLabel(1) -text "Panel Title: "
   label .dgC.panEntry(1) -text "$dgInfo(label)"
   label .dgC.panLabel(2) -text "Geometry of DG window: " \
              -highlightthickness 0
   entry .dgC.panEntry(2) -textvariable dgInfo(geom) \
              -highlightthickness 0
   for {set index 0} {$index <= 2 } {incr index} {
      grid .dgC.panLabel($index) -row $index -column 0 -in .dgC.f0 -sticky w
      grid .dgC.panEntry($index) -row $index -column 1 -in .dgC.f0 -sticky w
   }

   label .dgC.rowlabel -text "Rows:"
   set rID [tk_optionMenu .dgC.rowval dgInfo(rows) 10 11 12 13 14 15 16 17 18 19 20]
   pack .dgC.rowlabel .dgC.rowval -side left -in .dgC.f1
   bind $rID <ButtonRelease-1> {+after 1 showexp}
   label .dgC.collabel -text "      Columns:"
   set cID [tk_optionMenu .dgC.colval dgInfo(cols)  2 3 4 5 6 7 8 9 10 11 12 13 14 15 16]
   bind $cID <ButtonRelease-1> {+after 1 showexp}
   pack .dgC.collabel .dgC.colval -side left -in .dgC.f1
   set dgConf(background) [$cID cget -background]
   set dgConf(selectcolor) "pink"
   set dgConf(flash) off

   label .dgC.contrlLabel(3) -text "Current element:    " \
              -highlightthickness 0
   label .dgC.contrlEntry(3) -textvariable  elem(index) \
              -highlightthickness 0
   pack .dgC.contrlLabel(3) .dgC.contrlEntry(3) -side left -in .dgC.f2

   set dgConf(elements) {type rows cols justify label choices values \
     value units color width scale vnmrVar vnmrSet vnmrSet2 tclSet showVal}
   set dgConf(elemvals) {"none" 1  1 center ""      "" "" "" "" "black" "" ""  ""   "" "" "" ""}
   set dgConf(widgets)  {menu menu menu menu entry entry entry entry entry entry entry entry entry entry entry entry entry}
   set dgConf(menuLists) {typeList rowsList colsList posList "" "" "" "" "" ""  "" "" "" ""}
   set dgConf(widgetLabel)  {                     \
                          {Type of element: }  \
                          {Row extent: }       \
                          {Column extent: }    \
                          {Justify: }	       \
                          {Label of element: } \
                          {Choices: }		\
                          {Value of choices: } \
                          {Value: }            \
                          {Units: }            \
                          {Color of label: }   \
                          {Width of element: } \
                          {Scale Tcl pars: }       \
                          {Vnmr Variables: }   \
                          {Vnmr Cmd: }         \
                          {Vnmr Cmd2: }         \
                          {Tcl Cmd: }         \
                          {Show Condition: }  \
                         }

   set dgConf(typeList) {none title label button entry check radio menu2 scale scroll}
   set dgConf(rowsList) {1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20}
   set dgConf(colsList) {1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16}
   set dgConf(posList) {left center right}
   set dgConf(none,elements)   {type rows cols}
   set dgConf(title,elements)  {type rows cols label justify color}
   set dgConf(label,elements)  {type rows cols label justify value units color \
                                width vnmrVar tclSet}
   set dgConf(button,elements) {type rows cols label justify color width vnmrSet}
   set dgConf(entry,elements)  {type rows cols label justify value units color \
                                width vnmrVar vnmrSet tclSet showVal}
   set dgConf(check,elements)  {type rows cols label justify value color \
                                vnmrVar vnmrSet vnmrSet2 tclSet}
   set dgConf(radio,elements)  {type rows cols label justify value values color \
                                vnmrVar vnmrSet tclSet}
   set dgConf(menu2,elements)  {type rows cols label justify choices values \
                                value color vnmrVar vnmrSet tclSet}
   set dgConf(scale,elements)  {type rows cols label justify value color width \
                                scale vnmrVar vnmrSet tclSet}
   set dgConf(scroll,elements) {type rows cols label justify choices value color \
                                width vnmrVar vnmrSet tclSet}

   set bindex 0
   set rindex 0
   foreach butindex $dgConf(elements) {
      set typeWidget [lindex $dgConf(widgets) $bindex]
      text .dgC.conf3Label($bindex) -height 1 -relief flat -width 20 \
          -highlightthickness 0
      .dgC.conf3Label($bindex) insert 1.0 [lindex $dgConf(widgetLabel) $bindex]
      .dgC.conf3Label($bindex) configure -state disabled
      if {$typeWidget == "menu"} then {
         set menuID($bindex) [eval tk_optionMenu .dgC.conf3Entry($bindex) \
           elem($butindex) \
           $dgConf([lindex $dgConf(menuLists) $bindex])]
      } elseif {$typeWidget == "entry"} then {
         entry .dgC.conf3Entry($bindex) -width 80 \
          -highlightthickness 0 \
          -textvariable elem($butindex)
      }
      if {$butindex != "value"} {
         grid .dgC.conf3Label($bindex) -row $rindex -column 0 -in .dgC.f3
         grid .dgC.conf3Entry($bindex) -row $rindex -column 1 -sticky w -in .dgC.f3
         incr rindex
      }
      incr bindex
   }
   bind $menuID(0) <ButtonRelease-1> {+after 1 elemconfig}
   set dgConf(foreground) [.dgC.conf3Label(0) cget -foreground]
   set dgConf(stipple) gray50

   button .dgC.exit -text "Exit" -command "endDgConf 1"
   button .dgC.setdg -text "Refresh Display" -command setbutton
   label .dgC.spaceHolder -text "   "
   pack .dgC.setdg .dgC.exit .dgC.spaceHolder -side left -anchor w -in .dgC.f4
   frame .dgC.us
   radiobutton .dgC.us.sys -text "System  " -variable dgConf(save) -value sys \
          -highlightthickness 0
   radiobutton .dgC.us.usr -text "User    " -variable dgConf(save) -value usr \
          -highlightthickness 0
   checkbutton .dgC.exp -textvariable seqfil -variable dgConf(exp) \
          -highlightthickness 0
   label .dgC.explab -text " Specific"
   button .dgC.saveDg -text "Save Display" -command saveDisplay
   set dgConf(save) "usr"
   set dgConf(exp) 1
   if {[globalSave] == 1} {
      pack .dgC.us.usr .dgC.us.sys -side bottom -anchor w -in .dgC.us
      pack .dgC.us -side left -anchor w -in .dgC.f4
   }
   pack .dgC.exp .dgC.explab .dgC.saveDg -side left -anchor w -in .dgC.f4
   pack .dgC.f0 .dgC.f1 .dgC.f2 .dgC.f3 .dgC.f4 -side top -expand 1 \
         -fill x -in .dgC
   wm geometry .dgC -0+0
   setTemplate
   bind .dgmain <Leave> updateGeometry
   getColors
   showexp
}
