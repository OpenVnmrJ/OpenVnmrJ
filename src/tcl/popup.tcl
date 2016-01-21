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
source [file join $tk_library vnmr menu2.tk]

proc initChoicePopup {} {
   global tk_library choiceInfo
   image create bitmap choiceInfo(flagdown) \
        -file [file join $tk_library vnmr arrow2.bmp] \
        -maskfile [file join $tk_library vnmr arrow2.bmp]
   menu .dumm
   set choiceInfo(ag) [.dumm cget -activebackground]
   set choiceInfo(bg) [.dumm cget -background]
   destroy .dumm
}

proc choiceEntry {win entrys cmd id args} {
   global choiceInfo menu2Index
   set menu2Index -1
   if {[llength $args] != 0} then {
      eval "label $win -height 8 -width 8 \
           -highlightthickness 0 \
           -image choiceInfo(flagdown) \
           -relief raised -borderwidth 2 $args"
   } else {
      label $win -height 8 -width 8 \
           -highlightthickness 0 \
           -image choiceInfo(flagdown) \
           -relief raised -borderwidth 2
   }
   bind $win <Enter> "$win configure -background $choiceInfo(ag)"
   bind $win <Leave> "$win configure -background $choiceInfo(bg)"
   bind $win <1> "menu2_popup $win {$entrys}"
   bind $win <ButtonRelease-1> "exit_menu2 $win $cmd $id"
   set select selectMenu2Entry
   lappend select {winfo containing %X %Y}
   lappend select $choiceInfo(ag)
   lappend select $choiceInfo(bg)
   bind $win <Motion> "$select"
# puts "bind $win with $select"
}

initChoicePopup
