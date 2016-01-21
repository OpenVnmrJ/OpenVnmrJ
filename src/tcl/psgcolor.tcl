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

set pInfo(elemColor) green
set pInfo(labelColor) blue
set pInfo(leadColor) green
set pInfo(selectColor) red
set pInfo(connectColor) red
set pInfo(dockColor) red
set pInfo(chanColor) red
set pInfo(backColor) gray70
set pInfo(volume) 1
set pInfo(dockSound) [file join $pInfo(homeDir) sounds click]
set bgColor gray70

proc getColor {item index} {
   global pInfo cInfo
   if {$index == -1} {
      set index [lsearch $cInfo(colors) $pInfo($item)]
      if {$index == -1} {
         set index 0
      }
      .color.colors selection set $index
      .color.colors see $index
   }
   set pInfo($item) [lindex $cInfo(colors) $index]
}

proc setElemColor {color} {
   global pInfo cInfo
   getColor elemColor $color
   .draw itemconfigure element -outline $pInfo(elemColor)
}

proc setBackColor {color} {
   global pInfo cInfo backColor
   getColor backColor $color
   set bgColor $pInfo(backColor)
   .draw configure -bg $bgColor
   option add *background $bgColor
}

proc setSelectColor {color} {
   global pInfo cInfo psgInfo
   getColor selectColor $color
   .draw itemconfigure T$psgInfo(cur)  \
         -outline $pInfo(selectColor) -fill $pInfo(selectColor)
}

proc setConnColor {color} {
   global pInfo cInfo psgInfo
   getColor connectColor $color
   if {[info exists psgInfo($psgInfo(cur),idAt)] == 1} {
      .draw itemconfigure T$psgInfo($psgInfo(cur),idAt) \
          -outline $pInfo(connectColor)
   }
}

proc setLineColor {color} {
   global pInfo cInfo
   getColor dockColor $color
   .draw itemconfigure dockLines -fill $pInfo(dockColor)
}

proc setChanColor {color} {
   global pInfo cInfo
   getColor chanColor $color
   .draw itemconfigure channelLine -fill $pInfo(chanColor)
}

proc setColor {color} {
   global cInfo
   switch $cInfo(index) {
      elemColor {setElemColor $color}
      selectColor {setSelectColor $color}
      connectColor {setConnColor $color}
      dockColor {setLineColor $color}
      chanColor {setChanColor $color}
      backColor {setBackColor $color}
      default {}
   }
}

proc editColors {} {
   global pInfo cInfo bgColor
   destroy .color .co1 .co2 .co3 .co4 .co5 .co6 .colorScroll .colors
   toplevel .color -background $bgColor
   if {[info exists pInfo(colorGeom)] == 1} {
      wm geometry .color $pInfo(colorGeom)
   }
   set cInfo(index) ""
   set t .color
   frame .color.top
   frame .color.fLeft
   set w .color.fLeft
   wm title .color "PSG Audio/Visual Control"
   label $w.lab -text "Color Selections" -pady 5
   radiobutton $w.co1  -text "Pulse element    " -variable cInfo(index) -value elemColor \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setElemColor -1" -width 20 -justify left
   radiobutton $w.co2  -text "Selected element " -variable cInfo(index) -value selectColor\
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setSelectColor -1" -width 20 -justify left
   radiobutton $w.co3  -text "Connected element" -variable cInfo(index) -value connectColor \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setConnColor -1" -width 20 -justify left
   radiobutton $w.co4  -text "Connection lines " -variable cInfo(index) -value dockColor \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setLineColor -1" -width 20 -justify left
   radiobutton $w.co5  -text "Channel lines    " -variable cInfo(index) -value chanColor \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setChanColor -1" -width 20 -justify left
   radiobutton $w.co6  -text "Background       " -variable cInfo(index) -value backColor \
       -highlightthickness 0 -indicatoron 0 -selectcolor pink \
       -command "setBackColor -1" -width 20 -justify left
   frame .color.fRight
   listbox $t.colors -relief raised -borderwidth 2 \
       -yscrollcommand "$t.colorScroll set" -selectmode single
   scrollbar $t.colorScroll -command "$t.colors yview"
   pack $t.colorScroll -side right -fill y -in .color.fRight -expand 1
   pack $t.colors -side left -in .color.fRight -expand 1 -fill both
   set file /usr/openwin/lib/X11/rgb.txt
   if {[file exists $file] == 0} {
      set file /usr/lib/X11/rgb.txt
   }
   set cInfo(colors) {}
   set f [open $file]
   while {[gets $f line] >= 0} {
      set aa [lrange $line 3 end]
      if {([llength $aa] == 1) && ![string match "grey*" $aa]} {
         $t.colors insert end $aa
         lappend cInfo(colors) $aa
      }
   }
   close $f
   bind $t.colors <ButtonRelease-1> {
      setColor [.color.colors curselection]
   }
#   pack $w.co1 $w.co2 $w.co3 $w.co4 $w.co5 -in .color
   label $w.lab2 -text "" -pady 5 -justify center
   label $w.lab3 -text "Audio Selection" -pady 5 -justify center
   radiobutton $w.co7  -text "Turn off audio" -variable pInfo(volume) -value 0 \
       -highlightthickness 0 -indicatoron 1 -justify left
   radiobutton $w.co8  -text "Quiet audio" -variable pInfo(volume) -value 1 \
       -highlightthickness 0 -indicatoron 1 -justify left
   radiobutton $w.co9  -text "Medium audio" -variable pInfo(volume) -value 10 \
       -highlightthickness 0 -indicatoron 1 -justify left
   radiobutton $w.co10  -text "Loud audio" -variable pInfo(volume) -value 30 \
       -highlightthickness 0 -indicatoron 1 -justify left
   pack $w.lab $w.co1 $w.co2 $w.co3 $w.co4 $w.co5 $w.co6 \
        $w.lab2 $w.lab3 $w.co7 $w.co8 $w.co9 $w.co10 -in $w -side top -anchor w
   pack $w .color.fRight -in .color -side left -expand 1 -fill both
#   pack $w2.lab $w2.co1 $w2.co2 $w2.co3 -side top -in .color.bot -anchor w
#   pack .color.top .color.bot -side top -anchor w
   wm protocol .color  WM_DELETE_WINDOW {
      set pInfo(colorGeom) [wm geometry .color]
      destroy .color
   }
}
