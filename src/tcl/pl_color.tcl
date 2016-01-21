#! ./vnmrwish -f
vnmrinit $argv $env(vnmrsystem)
set TEMPLATES_DIR          $env(vnmruser)/templates
set COLOR_DIR              $TEMPLATES_DIR/color
set USER_DEFAULT_FILE      $COLOR_DIR/DEFAULT
set SYSTEM_DEFAULT_FILE    $env(vnmrsystem)/user_templates/color/default

if { ![ file isdirectory $COLOR_DIR ] } {
   if { ![ file isdirectory $TEMPLATES_DIR ] } {
      exec mkdir $TEMPLATES_DIR
   }
   exec mkdir $COLOR_DIR
}

set oneD_rad_list "20 11 12 9 10 14 13  18 15"
set twoDph_rad_list "50 49 48 47 46 45 44 43 42 41 40 39 38 37 36"            
set twoDav_rad_list "35 34 33 32 31 30 29 28 27 26 25 24 23 22 21"
                                  ;# no contour 0 (#20)in av mode for now
set image_rad_list "8 19"
set plot_color_list "black white red green blue cyan magenta yellow"

global spectrum integral fidreal fidimag scale parameter
global pen1 pen2 pen3 pen4 pen5 pen6 pen7 pen8 pl_color
global color_numeric
global jumbo_list

set pl_color ""         ;# raster color,  0 = 1 plane    1 = 4 planes
set current_display ""  ;# to track which window is currently displayed

set name_given [lindex $argv 1]

foreach i "$oneD_rad_list $twoDph_rad_list $twoDav_rad_list $image_rad_list" {
     lappend jumbo_list $i
}

set old_color_file ""
proc forceMacroName {name el op} {
   global old_color_file colored_file
   set match ""
   regexp {[\$a-zA-Z_]+[\$a-zA-Z0-9_\#]*} $colored_file match
   if {$colored_file != $match} {
      set colored_file $old_color_file
   }
   set old_color_file $colored_file
}

########## start of Vnmr-Deck ####################
###from Dan's deck.tk , modified to work with color interface
set deckInfo(active) -1
set deckInfo(foreground) -1
set deckInfo(disabledforeground) -1

proc resetDeck {win} {
   global deckInfo

   if {$deckInfo(active) != -1} then {
      $deckInfo(active) configure -relief sunken
      ;# $deckInfo(active) configure -disabledforeground $deckInfo(disabledforeground)
      $deckInfo(active) configure -state normal
   }
   $win configure -relief flat
   ;# $win configure -disabledforeground $deckInfo(foreground)
   ;# after 200 "$win configure -state disabled"
   set deckInfo(active) $win
}

proc deck {win args} {
   global deckInfo
 
   eval "button $win $args -relief sunken -borderwidth 3"
   ;# bind $win <1> "resetDeck $win"
   set deckInfo(disabledforeground) [$win cget -disabledforeground]
   set deckInfo(foreground) [$win cget -foreground]
}

proc selectdeck {win cmd} {
   resetDeck $win
   eval $cmd
   resetDeck $win
}

########## end of Vnmr-Deck ######################

;###############
;# update2ndListbox -
;###########################################################################
proc update2ndListbox {} {
   global color_category COLOR_DIR

   .fr3.right.bottom.scrlist2.colors delete 0 end
   catch {[set aa [exec ls $COLOR_DIR ]]}
   if [info exists aa] {
       foreach i $aa { .fr3.right.bottom.scrlist2.colors insert end $i }
   }

   ;#default
   ;#.fr3.right.bottom.scrlist2.colors selection set 0
   return 0

}

;###############
;# isValidName -
;###########################################################################
proc isValidName {name} {
   global COLOR_DIR

    catch {[set aa [exec ls $COLOR_DIR ]]}

    if {![info exists aa]} {return 0} ;#for a new user might not have anything here

    foreach i $aa {
       if {[string match "$name" $i]} {
          return 1
       } 
    }   
    return 0

}

;###############
;# deleteColorFile -
;###########################################################################
proc deleteColorFile {del_file} {
   global COLOR_DIR color_category colored_file
   
   if {$del_file == ""} {
      dialog .d {Delete}  "Missing file name" {} -1 OK
      return 0
   } 
   if ![isValidName $del_file] {
       dialog .d {Delete}  " $del_file  does not exist " {} -1 OK
       return 1
   }
   set k [dialog .d {Delete}  " Delete $del_file ? " {} -1 OK Cancel]
   if {$k == 1} {
      return 1
   }

   exec rm -f $COLOR_DIR/$del_file
   set colored_file ""

   ;# leave a mark on the 1st listbox
   .fr3.right.top.scrlist.colors selection set 0

   update2ndListbox
   return 0
}

;###############
;# loadIt -
;###########################################################################
proc loadIt {} { 
   global colored_file current_display bg_default_col 
   global COLOR_DIR
 
    if {$colored_file == ""} { 
       dialog .d {Load}  "Missing a filename, Can not load" {} -1 OK 
       return -1
    }
    if [isValidName $colored_file] {  ;#incase user types in the entry box 
       loadColorFile $COLOR_DIR/$colored_file 
    } else {  
         dialog .d {Load}  "Not a valid filename, select name from the listbox" {} -1 OK 
         return -1
      }  
     
    ;# no background color for plotting 
    #if { $current_display == 1 || $current_display == 2} {
    #                     ;#graphics background
    #   catch {.fr3.disbut.20 configure -state disabled -selectcolor $bg_default_col}
    #   catch {.fr3.pframe.20 configure -background $bg_default_col}
    #                     ;#2D_background
    #   catch {.fr3.disbut.43 configure -state disabled -selectcolor $bg_default_col}
    #   catch {.fr3.pframe.43 configure -background $bg_default_col}
    #}
    return 0
}

;###############
;# loadColorFile -
;#       load_file - full name colored file
;#    
;###########################################################################
proc loadColorFile {load_file} {
   ;# DO NOT remove any of below globals
   global saved_dis_nmr saved_dis_tcl2 dis_bg_col_index \
          saved_pos_nmr saved_pos_tcl2 pos_bg_col_index \
          saved_ras_nmr saved_ras_tcl2 ras_bg_col_index \
          saved_pen_nmr saved_pen_tcl2 pen_bg_col_index \
          COLOR_DIR color_category jumbo_list bg_default_col new_pencolor_list \
          current_display rgbval rgbname rgbindex plot_color_index

   if {$color_category == "plpen"} {.fr3.right.top.scrlist.colors delete 0 end} ;#clear the listbox

   set cfile [file tail $load_file]
   set execstring "macrold(\'$load_file\'):\$dummy $cfile purge('$cfile')"
   set new_pencolor_list ""

   vnmrsend $execstring

  set f [open $load_file]
   while {[gets $f line] >= 0} {
      set cat [string range $line 10 11]
      switch $cat {
           "gr" {set key "dis"
                 set frame "dframe"
                 regexp {[^,]*,([^,]*),([^\)]*)} $line match nmrindex colornumeric
                }
           "ps" {set key "pos"
                 set frame "pframe"
                 regexp {[^,]*,([^,]*),([^\)]*)} $line match nmrindex colornumeric
                }
           "pc" {set key "ras"
                 set frame "pframe"
                 regexp {[^,]*,([^,]*),'([^\']*)} $line match nmrindex colorname
                 set colornumeric $rgbname($colorname)
                 set ras_bg_col_index($nmrindex) $plot_color_index($colorname)
                }
           "hp" {set key "pen"
                 set frame "pframe"
                 regexp {[^,]*,([^,]*),'([^\']*)} $line match nmrindex colorname
                 set colornumeric $rgbname($colorname)
                 set pen_bg_col_index($nmrindex) $plot_color_index($colorname)
                }
           "pe" {  #this is a special case, insert pen to the listbox
                 regexp {[^,]*,([^,]*),'([^\']*)} $line match nmrindex colorname
                 lappend new_pencolor_list [format "%-7s %9s" $colorname "(Pen $nmrindex)"]
                }
      }  
                                  ;#         nmrindex ==> nmr color category index 0 - 50
                                  ;#     colornumeric ==> rgb numeric color  "r,g,b"
 
         set saved_${key}_tcl2($nmrindex) " .fr3.${frame}.${nmrindex} configure \
                                -background [eval format "#%02x%02x%02x" [split $colornumeric ,]]"

         if { $cat == "gr" || $cat == "ps" } {
              set saved_${key}_nmr($nmrindex) "$colornumeric"     ;# saved_dis_nmr(35) "0,0,0"
     
              ;#the colors which selected from the default file or color bars might not exist here
              if { [info exists rgbval($colornumeric)] } {
                 set ${key}_bg_col_index($nmrindex) $rgbindex($rgbval($colornumeric))
              }
         } else {
              set saved_${key}_nmr($nmrindex) "$colorname"        ;# saved_ras_nmr(35) "black"
           }
   }
   close $f
 
   foreach i $jumbo_list {
      catch { eval $saved_dis_tcl2($i) }
   }

   switch $color_category {
   
      "dspl"  { foreach i $jumbo_list {  ;# set all plot color frames to bg default color 
                   catch {.fr3.pframe.$i configure -background $bg_default_col}
                }
              }

      "plps"  { foreach i $jumbo_list { catch { eval $saved_pos_tcl2($i) } } }

      "plras" { foreach i $jumbo_list { catch { eval $saved_ras_tcl2($i) } } }

      "plpen" { foreach i $jumbo_list { catch { eval $saved_pen_tcl2($i) } }
                .fr3.right.top.scrlist.colors delete 0 end
                if [info exists new_pencolor_list] {
                   foreach j $new_pencolor_list {
                       .fr3.right.top.scrlist.colors insert end $j
                   }
                }
              }
   }
}

;###############
;# saveCurrentColors - write to an individual macro file (oneD_color, twoD_color ...)
;# - the argument goes to arrayx_name is just a literal name NOT a variable 
;#   but, it is the name of an array variable
;###########################################################################
proc saveCurrentColors {} {
   global  color_category name_given colored_file \
           saved_dis_nmr saved_pos_nmr saved_ras_nmr saved_pen_nmr

   if {$colored_file == ""} {
      set colored_file $name_given
   }

   if [isValidName $colored_file] {
      set k [dialog .d {Save}  " $colored_file  already exist, over write? " {} -1 OK Cancel]
      if {$k == 1} {
         return 1
      }  
   }   

   saveDisplay saved_dis_nmr  saved_pos_nmr  saved_ras_nmr  saved_pen_nmr

   return 0
}

;###############
;# saveDisplay - write to an individual macro file (oneD_color, twoD_color ...)
;# - the argument goes to arrayx_name is just a literal name NOT a variable 
;#   but, it is the name of an array variable
;###########################################################################
proc saveDisplay {dis_nmr  pos_nmr  ras_nmr  pen_nmr} {

   global COLOR_DIR current_display color_category \
          new_pencolor_list colored_file \
          dis_bg_col_index pos_bg_col_index \
          ras_bg_col_index pen_bg_col_index

   upvar $dis_nmr  disvar
   upvar $pos_nmr  posvar
   upvar $ras_nmr  rasvar
   upvar $pen_nmr  penvar

   set aaa [array names disvar] 
   if { $aaa == "" } { 
      dialog .d {Save}  "No colored informations provided" {} -1 OK
      return 1 
   } 

   set nmr_saved_file [format "%s%s%s" $COLOR_DIR "/" $colored_file]
   exec cat /dev/null > $nmr_saved_file
   set nmrFD [open $nmr_saved_file a+]

   ;#### save display #####################
   foreach name "$aaa" {
      set twoD 0 
      ;#if that is a 2D will add ' 'to the name
      for {set i 20} {$i<=50} {incr i} {
          if {$name == $i} {
             set twoD 1
          } 
      }
      ;#save for updating vnmr color when recalled
      puts $nmrFD "setcolor('graphics',$name,$disvar($name))"
   }

   ;#### savePsPlot #####################
   foreach name [array names posvar] {
      set twoD 0  
      ;#if that is a 2D will add ' 'to the name
      for {set i 20} {$i<=50} {incr i} {
          if {$name == $i} {
             set twoD 1
          }
      }    
      ;#save for updating vnmr color when recalled
      puts $nmrFD "setcolor('ps',$name,$posvar($name))"
   }

   ;#### saveRasPlot #####################
   foreach name [array names rasvar] {
      set twoD 0

      ;#if that is a 2D will add ' 'to the name
      for {set i 20} {$i<=50} {incr i} {
          if {$name == $i} {
             set twoD 1
          }
      }    

      ;#save for updating vnmr color when recalled
      puts $nmrFD "setcolor('pcl',$name,'$rasvar($name)')"
   }

   ;#### savePenPlot #####################
   foreach name [array names penvar] {
      ;#save for updating vnmr color when recalled
      puts $nmrFD "setcolor('hpgl',$name,'$penvar($name)')"
   }

   foreach i $new_pencolor_list {
      puts $nmrFD "setcolor('pen',[string range $i 15 15],'[lindex $i 0]')"
   }

   if { $current_display == 4 } {
      puts $nmrFD "dconi"
   }

   update2ndListbox

   close $nmrFD
   return 0
}

;############### 
;# dsplColorSelect -
;#   - vnmr takes a numerical color name
;#     ex.: 255,0,0 is the red color
;#
;###########################################################################
proc dsplColorSelect {bckgrnd_color_index bckgrnd_color cont_index} {
   global color_numeric current_display
   global saved_dis_nmr saved_dis_tcl1 saved_dis_tcl2 dis_bg_col_index
   global rgbname
          ;#contour in AV mode has 16 levels (20 - 35)
          ;#contour in PH mode has 15 levels (36 - 50)

    ;#arrange color to vnmr format ex. 255,255,255
    set v_color [join $color_numeric($bckgrnd_color_index) ,]
    set cc  [eval format "#%02x%02x%02x" [split $rgbname($bckgrnd_color) ,]] 

    vnmrsend "setcolor('graphics',$cont_index,$v_color)"

    if { $current_display == 4 } {vnmrsend "dconi"}
    .fr3.disbut.$cont_index configure -selectcolor pink
    .fr3.dframe.$cont_index configure -background white  ;#this line is needed here
    .fr3.dframe.$cont_index configure -background "$bckgrnd_color" 

    ;#save the selected colors in the arrays for saveCurrentColors{}

    ;#save for selecting listbox color name from color frame
    set  dis_bg_col_index($cont_index) $bckgrnd_color_index
    set  saved_dis_nmr($cont_index) $v_color
    ;#set  saved_dis_tcl2($cont_index) ".fr3.dframe.$cont_index configure -background $bckgrnd_color"
    set  saved_dis_tcl2($cont_index) ".fr3.dframe.$cont_index configure -background $cc"

    return 0
}

;###############
;# plotColorSelect -
;###########################################################################
proc plotColorSelect {pl_bckgrnd_color_index pl_bckgrnd_color cont_index} {
   global pos_bg_col_index saved_pos_nmr saved_pos_tcl2
   global ras_bg_col_index saved_ras_nmr saved_ras_tcl2
   global pen_bg_col_index saved_pen_nmr saved_pen_tcl2
   global color_category bg_col_index current_display
   global color_numeric rgbname

    .fr3.disbut.$cont_index configure -selectcolor pink
    .fr3.pframe.$cont_index configure -background $pl_bckgrnd_color

    ;#arrange color to vnmr format ex. 255,255,255
    set v_color [join $color_numeric($pl_bckgrnd_color_index) ,]
    set cc  [eval format "#%02x%02x%02x" [split $rgbname($pl_bckgrnd_color) ,]]

    switch $color_category {

      "plps"  { set  pos_bg_col_index($cont_index) $pl_bckgrnd_color_index
                ;#save for plot color frame sees the listbox item

                set  saved_pos_nmr($cont_index) $v_color
                set  saved_pos_tcl2($cont_index) ".fr3.pframe.$cont_index configure -background $cc"

                vnmrsend "setcolor('ps',$cont_index,$v_color)"
              }

      "plras" { set  ras_bg_col_index($cont_index) $pl_bckgrnd_color_index
                ;#save for plot color frame sees the listbox item

                set  saved_ras_nmr($cont_index) $pl_bckgrnd_color
                set  saved_ras_tcl2($cont_index) ".fr3.pframe.$cont_index configure -background $cc"
                if {$cont_index == "background"} {return 1}

                vnmrsend "setcolor('pcl',$cont_index,'$pl_bckgrnd_color')"
              }

      "plpen" { set pen_bg_col_index($cont_index) $pl_bckgrnd_color_index
                ;#save for plot color frame sees the listbox item

                set  saved_pen_nmr($cont_index) $pl_bckgrnd_color
                set  saved_pen_tcl2($cont_index) ".fr3.pframe.$cont_index configure -background $cc"

                vnmrsend "setcolor('hpgl',$cont_index,'$pl_bckgrnd_color')"
              }
    }
    return 0
}

;###############
;# getXColor -
;#
;###########################################################################
proc getXColor {} {
  global color_numeric color_dspl color_dspl_index
  global xcolor_name total_XColor
  global rgbval rgbname rgbindex plot_color_list plot_color_index

   set color_numeric(0) ""
   set i 0     ;#counter associates with the listbox
   set k 0     ;#counter associates with the color pallets

   set file /usr/openwin/lib/X11/rgb.txt
   if {[file exists $file] == 0} {
      set file /usr/lib/X11/rgb.txt
   }
   set f [open $file]
   while {[gets $f line] >= 0} {
         set aa [lrange $line 3 end]          ;#alphabet name only
         if { [llength $aa] == 1 } {          ;#throw out doubled word name
            if { ![string match "grey*" $aa] && ![string match "*Grey" $aa] } {
                                              ;#gray or Gray; neither grey nor Grey
               set color_numeric($i) [lrange $line 0 2]
               set xcolor_name($i) $aa 

               if {($i %10) == 0} {
                 set color_dspl($k) $aa
                 set color_dspl_index($k) $i
                 incr k
               }

               set val [join [lrange $line 0 2] ,]

               set rgbval($val) $aa     ;#rgbval(0,0,0) black
               set rgbname($aa) $val    ;#rgbname(black) 0,0,0
               set rgbindex($aa) $i     ;#rgbindex(black) 1 - 500

               incr i
            }
         }
   }
   set total_XColor $i
   close $f
   
   ;#for "Plot Raster and "Plot Pen" listbox"
   set count 0
   foreach i $plot_color_list {
      set plot_color_index($i) $count
      incr count
   }

   return 0
}

;###############
;# reConfigureFrames -
;###########################################################################
proc reConfigureFrames {fram list relief} {
   global color_category
 
   if [string match "dspl" "$color_category"] { set relief raised } else { set relief flat }
 
   set count 1
   foreach item $list {
       switch $current_display {
          1 {set aa $item}
          2 {set aa [expr 43+$item]}
          3 {set aa [expr 20+$item]}
          4 {set aa $item}
       } 
       ${fram}.$aa configure -relief $relief
   }
   return 0
}

;###############
;# makeRadioButs -
;###########################################################################
proc makeRadioButs {list} {
   global  current_display color_category saved_dis_tcl2

   eval destroy [winfo child .fr3.disbut]
   eval destroy [winfo child .fr3.dframe]
   eval destroy [winfo child .fr3.pframe]
 
   if [string match "dspl" "$color_category"] { set relief raised } else { set relief flat }

   set oneD_text_list "background spectrum integral fid imaginary scale parameter  cursor threshhold"
   set image_text_list "background foreground"

   set count 1
   set count_text 0
   foreach item $list {
       switch $current_display {
          1 {set a_text [lindex $oneD_text_list $count_text]}
          2 {set a_text "Contour [expr $item-43]"}
          3 {set a_text "Contour [expr $item-20]"}
          4 {set a_text [lindex $image_text_list $count_text]}
       }
       incr count_text 1
 
       radiobutton .fr3.disbut.$item -text "$a_text" -variable radi_val -value $item \
                    -highlightthickness 1 -anchor w -width 12 -selectcolor pink -command {bindAll $radi_val}
       #grid .fr3.disbut.$item -row $count -column 0 -sticky w -in .fr3
       pack .fr3.disbut.$item -ipadx 2 -ipady 3 -side top
 
       ;#These frames to display selected color for the screen
       frame .fr3.dframe.$item -width 12m -height 6m -relief $relief -borderwidth 2
       # grid .fr3.dframe.$item -row $count -column 1 -sticky w -in .fr3
       pack .fr3.dframe.$item -padx 1 -ipadx 2 -ipady 3 -side top

       bind .fr3.dframe.$item <ButtonRelease-1> "bindColorFrame [winfo name .fr3.dframe.$item]"
 
       ;#These frames to display selected color for plotting
       frame .fr3.pframe.$item -width 12m -height 6m -relief $relief -borderwidth 2
       #grid .fr3.pframe.$item -row $count -column 2 -sticky w -in .fr3
       pack .fr3.pframe.$item -padx 4 -ipadx 2 -ipady 3 -side top

       bind .fr3.pframe.$item <ButtonRelease-1> "bindPlColorFrame [winfo name .fr3.pframe.$item]"

       incr count 1
   }

   if [info exists saved_dis_tcl2] {
      foreach i [array names saved_dis_tcl2] {
          catch {[eval $saved_dis_tcl2($i)]}
      }
   }
   return 0
}

;###############
;# bindPlColorFrame -
;#      mark labeled radiobutton, listbox , color palette when select color frame
;###########################################################################
proc bindPlColorFrame { pwin_name } {
   global color_category radi_val color_pal total_XColor
   global dis_bg_col_index pos_bg_col_index ras_bg_col_index pen_bg_col_index 
 
    if {$color_category == "dspl" || $pwin_name == "43" || $pwin_name == "20"} {
       return 1
    }
    
    ;#select radiobutton from color frame
    set radi_val $pwin_name
    .fr3.right.top.scrlist.colors selection clear 0 end

    switch $color_category {

       "dspl"   { return 1 }
       "plps"   { ;#select color palet from color frame
                  ;#"dis" and "pos" are using the same listbox index, can remove pos_bg_col_index
                  if { [info exists dis_bg_col_index($pwin_name)] } {
                     if {[expr $dis_bg_col_index($pwin_name)%10] == 0} {
                        set color_pal [expr $dis_bg_col_index($pwin_name)/10]
                     } else { set color_pal ""}
                     .fr3.right.top.scrlist.colors selection clear 0 $total_XColor
                     .fr3.right.top.scrlist.colors see $dis_bg_col_index($pwin_name)
                     .fr3.right.top.scrlist.colors selection set $pos_bg_col_index($pwin_name)
                  } else {
                       .fr3.right.top.scrlist.colors selection clear 0 $total_XColor
                       set color_pal ""
                    }
                }	

       "plras"  { .fr3.right.top.scrlist.colors selection set $ras_bg_col_index($pwin_name) }
       "plpen"  { .fr3.right.top.scrlist.colors selection set $pen_bg_col_index($pwin_name) }
    }

    return 0
}

;############### 
;# bindAll -
;###########################################################################
proc bindAll {index} {
   global color_category
  
   if {$color_category == "dspl"} {
      bindColorFrame $index
   } else {
        bindPlColorFrame $index
     }

}
;############### 
;# bindColorFrame -
;#      tt - spectrum, integra, 11, 50 ...
;#      mark labeled radiobutton, listbox , color palette when select color frame
;###########################################################################
proc bindColorFrame { win_name } {
   global  color_category dis_bg_col_index radi_val total_XColor color_pal

    if ![string match $color_category "dspl"] { return 1 }

    ;#select radiobutton from color frame
    set radi_val $win_name

    if { [info exists dis_bg_col_index($win_name)] } {
       ;#select color palet from color frame
       if {[expr $dis_bg_col_index($win_name)%10] == 0} {
          set color_pal [expr $dis_bg_col_index($win_name)/10]
       } else { set color_pal ""}

       ;#select listbox item from color frame
       .fr3.right.top.scrlist.colors selection clear 0 $total_XColor
       .fr3.right.top.scrlist.colors see $dis_bg_col_index($win_name)
       .fr3.right.top.scrlist.colors selection set $dis_bg_col_index($win_name)
    } else {
         .fr3.right.top.scrlist.colors selection clear 0 $total_XColor
         set color_pal ""
      }

    return 0
}

;############### 
;# bindColorPal -
;# 
;###########################################################################
proc bindColorPal {index} {
   global total_XColor color_dspl_index

    ;#select listbox from color palette
    .fr3.right.top.scrlist.colors selection clear 0 $total_XColor ;\
    .fr3.right.top.scrlist.colors see $color_dspl_index($index) ;\
    .fr3.right.top.scrlist.colors selection set $color_dspl_index($index) ;\
 
    bindListbox [.fr3.right.top.scrlist.colors get $color_dspl_index($index)] ;\
    return 0 
}

;###############
;# bindListbox -
;#      select labled radiobutton and color frame
;###########################################################################
proc bindListbox {col_select} {
   global radi_val color_category plot_bg_color color_pal listbox_select_index
   global rgbname

    if {$color_category == "plbw"} {return 1}  ;#no listbox selection for "plbw"

    set listbox_select_index [.fr3.right.top.scrlist.colors curselection]

    ;#set cc  [eval format "#%02x%02x%02x" [split $rgbname($col_select) ,]]

    if { $radi_val == "" } {
       if { $color_category == "dspl" || $color_category == "plps" } {
          ;#select color pallete only 
          if {[expr $listbox_select_index%10] == 0} {
             set color_pal [expr $listbox_select_index/10]
          }
       }
    } else {
         if { $color_category == "dspl" } { ;#updating displayed color
            dsplColorSelect $listbox_select_index $col_select $radi_val
            ;#dsplColorSelect $listbox_select_index $cc $radi_val

            if {[expr $listbox_select_index%10] == 0} { ;#selecting color palette display
               set color_pal [expr $listbox_select_index/10]
            }

         } else { ;#for plotting donot have background option, 20 ---> graphics background
                                                           ;#  43 ---> 2D_ph display
              if { $radi_val == "20" || $radi_val == "43" } {
                 return 1
              } else {
                  set plot_bg_color $col_select
                  plotColorSelect $listbox_select_index $col_select $radi_val
                  ;#plotColorSelect $listbox_select_index $cc $radi_val

                  if {[expr $listbox_select_index%10] == 0} {
                     set color_pal [expr $listbox_select_index/10]
                  }
               }
           }
      }
    return 0
}

;###############
;# display1dWindow -
;#
;###########################################################################
proc display1dWindow {} {
   global current_display oneD_rad_list color_category 

   if { $current_display == 1 } {return 1}

   dsplGenericWindow

   set current_display 1
   .fr2.display configure -background pink
   changeTextColor .fr1.screen

   makeRadioButs $oneD_rad_list      ;# all display* functions will call this one
 
   return 0
}

;###############
;# display2dPhWindow -
;#
;###########################################################################
proc display2dPhWindow {} {
   global current_display twoDph_rad_list color_category 
 
   if { $current_display == 2 } {return 1}
 
   dsplGenericWindow

   set current_display 2
   .fr2.display configure -background pink
   changeTextColor .fr1.2dph
 
   makeRadioButs $twoDph_rad_list      ;# all display* functions will call this one
 
   return 0
}
;###############
;# display2dAvWindow -
;#
;###########################################################################
proc display2dAvWindow {} {
   global current_display twoDav_rad_list color_category

   if { $current_display == 3 } {return 1}
 
   dsplGenericWindow

   set current_display 3
   .fr2.display configure -background pink
   changeTextColor .fr1.2dav
 
   makeRadioButs $twoDav_rad_list      ;# all display* functions will call this one
   return 0
}

;###############
;# displayImageWindow -
;#
;###########################################################################
proc displayImageWindow {} {
   global current_display image_rad_list color_category
 
   if { $current_display == 4 } {return 1}
 
   dsplGenericWindow

   set current_display 4
   .fr2.display configure -background pink
   changeTextColor .fr1.image

   makeRadioButs $image_rad_list      ;# all display* functions will call this one
   return 0
}

;###############
;# dsplColorPalet -
;###########################################################################
proc dsplColorPalet {} {
   global color_dspl color_pal

   eval destroy [winfo child .fr3.right.top.cpalet]
   pack forget .fr3.right.top.dummyDD

   set m 0
   for {set i 0} {$i < 9} {incr i} {

       frame .fr3.right.top.cpalet.colorBut$i -height 6m
       pack .fr3.right.top.cpalet.colorBut$i -side top
 
       for {set j 0} {$j < 6} {incr j} {
           radiobutton .fr3.right.top.cpalet.colorBut$i.$j -relief flat  -bd 3 \
                          -width 4 -highlightthickness 0 -variable color_pal -value $m \
                          -background $color_dspl($m) -indicator 0 -selectcolor $color_dspl($m) \
                          -activebackground $color_dspl($m) \
           -command "bindColorPal $m"

           pack .fr3.right.top.cpalet.colorBut$i.$j -side left -expand 1 -fill x -fill y
           incr m
       } 
   }
   return 0
}

;###############
;# dsplGenericWindow -
;#      display the color pallete and insert xcolor to the listbox
;###########################################################################
proc dsplGenericWindow {} {
  global radi_val color_category bg_default_col xcolor_name
  global jumbo_list

    set radi_val ""
    if ![string match "dspl" "$color_category"] {
       dsplColorPalet
       .fr2.display configure -background pink
       .fr2.plot configure -text " Plot " -background $bg_default_col

       .fr3.right.top.scrlist.colors delete 0 end
       for {set i 0} {$i<[array size xcolor_name]} {incr i} {
           .fr3.right.top.scrlist.colors insert end $xcolor_name($i)
       }
    
       foreach i $jumbo_list {
          catch {.fr3.pframe.$i configure -background $bg_default_col}
       }

       ;# select the index 0 of the listbox as default
       .fr3.right.top.scrlist.colors selection set 0
    }
    set color_category "dspl"
    return 0
}

;###############
;# makeColorPens -
;###########################################################################
proc makeColorPens {} {
   global bg_default_col radi_val pen_num plot_bg_color col_pen old_aa
   global color_dspl color_dspl_index color_select
   global plot_color_list new_pencolor_list listbox_select_index
   global saved_pen_nmr

   eval destroy [winfo child .fr3.right.top.cpalet]
   pack forget .fr3.right.top.cpalet

   pack .fr3.right.top.dummyDD -side left 
   pack .fr3.right.top.cpalet

   set old_aa ""
   set m 1
   for {set i 0} {$i < 4} {incr i} {
 
       frame .fr3.right.top.cpalet.colorBut$i
       pack .fr3.right.top.cpalet.colorBut$i -side top -anchor n
 
       for {set j 0} {$j < 2} {incr j} {
           radiobutton .fr3.right.top.cpalet.colorBut$i.$j  -text " Pen $m " \
                -variable pen_num -value "Pen_$m" \
                -indicator 0  -selectcolor $bg_default_col\
                -command {
                    if [info exists color_select] {
                          set pen [string range $pen_num 4 end]

                          if [info exists new_pencolor_list] {
                             set list $new_pencolor_list
                          } else {set list $plot_color_list}

                          ;#pen plot format   setcolor('pen',1,'black')
                          ;#set saved_pen_nmr($color_select) $pen

                          vnmrsend "setcolor('pen',$pen,'$color_select')"
                          addPenToListbox $color_select $pen $list
                          .fr3.right.top.scrlist.colors selection set $listbox_select_index
                    }
                }
               ;#have to catch here, user might accidently hit the pen button before selecting color
               ;#need condition checking later

           pack .fr3.right.top.cpalet.colorBut$i.$j -side left -padx 15 -pady 4 \
                           -ipadx 7 -ipady 4 -expand 1 -fill x -fill y
           incr m
       } 
   }
   frame .fr3.right.top.cpalet.colorBut5
   pack .fr3.right.top.cpalet.colorBut5 -side top -anchor n

   return 0
}

;###############
;# addPenToListbox -  assigned pen # to color in the listbox
;#   box_color  -color selected from listbox
;#   pe_num     -pen number from label of the pen button
;#   pe_list    -color and pen list, assembled from modified listbox
;###########################################################################
proc addPenToListbox {box_color pe_num pe_list} {
   global new_pencolor_list save_color_pen
     
    foreach i $pe_list {
       set j [lrange $i 0 0]
       if [string match "$box_color" "$j"] {
                          ;#Clear Pen send "" to Pe_num
          if {$pe_num == ""} {set a ""} else {set a "(Pen $pe_num)"}
          set aa [format "%-7s %9s" $j $a]
          lappend tt $aa
          set save_color_pen($j) ($pe_num)   ;#save an assigned pen to an array
       } else { lappend tt $i }  ;# keep color plus pen#
    }

   .fr3.right.top.scrlist.colors delete 0 end
   foreach k $tt {
        .fr3.right.top.scrlist.colors insert end $k
   }
   set new_pencolor_list $tt
   return 0
}

;###############
;# dsplPlotWindow -
;#     display plot listbox
;###########################################################################
proc dsplPlotWindow {cat text col} {
  global xcolor_name bg_default_col color_category plot_color_list
  global saved_pos_tcl2 saved_pen_nmr
  global saved_pen_tcl2 new_pencolor_list jumbo_list
  global saved_ras_tcl2 current_display

   set color_category "$cat"
   eval destroy [winfo child .fr3.right.top.cpalet]

   if {$col == 1} {vnmrsend "setcolor('plotter',1,3)"}

   switch $cat {

          plps  {  dsplColorPalet
                   .fr3.right.top.scrlist.colors delete 0 end
                   for {set i 0} {$i<[array size xcolor_name]} {incr i} {
                       .fr3.right.top.scrlist.colors insert end $xcolor_name($i)
                   }
                   foreach i [array names saved_pos_tcl2] {
                       catch {[eval $saved_pos_tcl2($i)]}
                   }  
                }

          plras {  foreach i [array names saved_ras_tcl2] {
                          catch {[eval $saved_ras_tcl2($i)]}
                   }  
                   .fr3.right.top.scrlist.colors delete 0 end
                   foreach j $plot_color_list {
                     .fr3.right.top.scrlist.colors insert end $j
                   }
                   .fr3.right.top.scrlist.colors selection set 0  ;#default
                }

          plpen {  if [info exists saved_pen_tcl2] {
                      foreach i [array names saved_pen_tcl2] {
                          catch {[eval $saved_pen_tcl2($i)]}
                      }  
                   }  
                   .fr3.right.top.scrlist.colors delete 0 end
                   foreach j $new_pencolor_list {
                      .fr3.right.top.scrlist.colors insert end $j
                   }
                   ;#select index 0 on the listbox as default
                   .fr3.right.top.scrlist.colors selection set 0
                }

          plbw  {  .fr3.right.top.scrlist.colors delete 0 end
                   .fr3.right.top.scrlist.colors insert end black
                   .fr3.right.top.scrlist.colors selection set 0
                    
                   foreach i $jumbo_list {
                      if {$i == "background" || $i ==  43 || $i == "image_background"} {
                         set bg "white" 
                      } else { set bg "black" }

                      catch {.fr3.pframe.$i configure -background $bg}
                   }
                   vnmrsend "setcolor('plotter',1,0)"
                }
   }
   ;# no background for plotting
   if { $current_display == 1 } {
                        ;#graphics background
      catch {.fr3.disbut.20 configure -state disabled -selectcolor $bg_default_col}
      catch {.fr3.pframe.20 configure -background $bg_default_col}
   }

   if { $current_display == 2 } {
                        ;#2D_background
      catch {.fr3.disbut.43 configure -state disabled -selectcolor $bg_default_col}
      catch {.fr3.pframe.43 configure -background $bg_default_col}
   }

   .fr2.plot configure -text "$text" -background pink
   .fr2.display configure -background $bg_default_col
        
   return 0
} 

;###############
;# changeTextColor -
;#     change the deck text foreground color when selected
;###########################################################################
proc changeTextColor {w} {
   global fg_default_col 

   foreach i {screen 2dph 2dav image} {
      .fr1.$i configure -foreground $fg_default_col
   }

   $w configure -foreground pink
}

;###############
;# displayMainWindow -
;#
;###########################################################################
proc displayMainWindow {} {
   global fg_default_col bg_default_col radi_val colored_file
   global name_given plot_bg_color color_select col_pen
   global xcolor_name color_dspl color_dspl_index current_display
   global dummy color_category ;#plot_color_list new_pencolor_list
   global USER_DEFAULT_FILE COLOR_DIR SYSTEM_DEFAULT_FILE
   eval destroy [winfo child .]
   wm title . "VNMR   Color   Selection"
   wm iconname . "VNMR Color"
   ;#wm geometry . +300+100
   
   set colored_file ""
   set color_category "dspl"
    
   frame .fr1
   frame .fr2
   frame .fr3
   frame .fr4       ;#dummy

   pack .fr1 .fr2 .fr3 .fr4 -side top -anchor w -fill x

   set dummy 11  ;#just to mark the default window
   set pen_plot_color() ""  ;#do not know better way to initialize an array
   set col_pen() "init"
   set dis_bg_col_index() ""

   deck .fr1.screen  -text "  General  " -font courier-boldoblique -highlightthickness 0 \
                     -command {selectdeck .fr1.screen {display1dWindow } }

   deck .fr1.2dph  -text "   2D_PH   " -font courier-boldoblique -highlightthickness 0 \
                   -command {selectdeck .fr1.2dph {display2dPhWindow } }

   deck .fr1.2dav  -text "   2D_AV   " -font courier-boldoblique -highlightthickness 0 \
                   -command {selectdeck .fr1.2dav {display2dAvWindow } }

   deck .fr1.image -text "   Image   " -font courier-boldoblique -highlightthickness 0 \
                   -command {selectdeck .fr1.image {displayImageWindow } }

   ;#deck .fr1.help  -text "Help" -font courier-boldoblique -highlightthickness 0 \
   ;#                -command {selectdeck .fr1.screen {changeTextColor .fr1.help} }

   pack .fr1.screen .fr1.2dph .fr1.2dav .fr1.image -side left -expand 1 -fill x

   set fg_default_col [.fr1.screen cget -foreground]
   set bg_default_col [.fr1 cget -background]

   frame .fr2.dummy -width 23m -height 8m -relief flat ;#to line-up "Display" button

   button .fr2.display -text "Display" -relief raised -highlightthickness 2 \
                       -command {    dsplGenericWindow
                                     ;#reset backgound displays after plotting
                                     if {$current_display == 1 || $current_display == 2} {
                                        catch {.fr3.disbut.20 configure -state normal -selectcolor pink} 
                                        catch {.fr3.disbut.43 configure -state normal -selectcolor pink} 
                                     }
                                     ;#dsplGenericWindow
                                }
   menubutton .fr2.plot -text " Plot " -relief raised -highlightthickness 2 \
                                       -indicator 0 -menu .fr2.plot.menu
   ;#button .fr2.help -text "Help" -relief raised -highlightthickness 2 \
   ;#                                    -command { }

   menu .fr2.plot.menu
     .fr2.plot.menu add command -label "PostScript" -command {dsplPlotWindow "plps" "Plot PS" 1}
     .fr2.plot.menu add command -label "Raster" -command {dsplPlotWindow "plras" "Plot Raster" 1}
     .fr2.plot.menu add command -label "Pen" -command {dsplPlotWindow "plpen" "Plot Pen" 1
                                                        makeColorPens }
     .fr2.plot.menu add command -label "Blk/Wht" -command {dsplPlotWindow "plbw" "Plot B/W" 0}
                   
   pack .fr2.dummy -side left -pady 4
   pack .fr2.display  -side left -padx 2 -pady 6
   pack .fr2.plot -side left -padx 4 -pady 6 -ipadx 2
   ;#pack .fr2.help -side right -padx 20 -pady 6 -ipadx 2

   frame .fr3.disbut -width 150m -relief raised -bd 1  ;# for color radiobuttons
   pack .fr3.disbut -side left -anchor n

   frame .fr3.dframe -relief flat              ;# for displayed color frames
   pack .fr3.dframe -side left -anchor n
 
   frame .fr3.pframe  -relief flat             ;# for plot color frames
   pack .fr3.pframe -side left -anchor n
 
   frame .fr3.dummyD -width 4 -relief raised -bd 1
   pack .fr3.dummyD -side left -anchor n

   frame .fr3.right -relief flat               ;# for Xcolor listbox and color palettes
   pack .fr3.right -side right

   frame .fr4.frame  -height 2m  -width 177m ;#dummy, bottom spacer
   pack  .fr4.frame -side bottom
   display1dWindow  ;# default display

   ;#the name_given is either set to "plottername" or set to "none"
   if [isValidName $name_given] {
      loadColorFile $COLOR_DIR/$name_given
   } else {
        if ![file exists $USER_DEFAULT_FILE] {
           exec cp $SYSTEM_DEFAULT_FILE $USER_DEFAULT_FILE ;#$COLOR_DIR
        }

        loadColorFile $USER_DEFAULT_FILE
   }

   frame .fr3.right.top
   pack .fr3.right.top -side top
 
   ;#between colored listbox and file-name listbox
   frame .fr3.right.dummy1 -height 14m -relief raised -bd 1
   pack .fr3.right.dummy1 -side top
 
   frame .fr3.right.top.scrlist
   pack .fr3.right.top.scrlist -side left
 
   listbox .fr3.right.top.scrlist.colors -height 12 -width 18 -takefocus 1\
                                 -yscrollcommand ".fr3.right.top.scrlist.srollbar set"
   pack .fr3.right.top.scrlist.colors -side left
   scrollbar .fr3.right.top.scrlist.srollbar -command ".fr3.right.top.scrlist.colors yview"
   pack .fr3.right.top.scrlist.srollbar -side left -fill y

   for {set i 0} {$i<[array size xcolor_name]} {incr i} {
         .fr3.right.top.scrlist.colors insert end $xcolor_name($i)
   }
   .fr3.right.top.scrlist.colors selection set 0

   bind .fr3.right.top.scrlist.colors <ButtonRelease-1> {
      ;#need to set color_select here, pen plot will use this value
      set color_select [lrange [selection get] 0 0]
      bindListbox $color_select
   }
 
   frame .fr3.right.top.dummyDD  -width 10 -relief raised -bd 2
   ;#pack .fr3.right.top.dummyDD -side left

   frame .fr3.right.top.cpalet
   pack  .fr3.right.top.cpalet -side left

   dsplColorPalet

   frame .fr3.right.top.dummy -width 1m -relief raised -bd 1
   pack  .fr3.right.top.dummy -side right

   frame .fr3.right.bottom
   pack .fr3.right.bottom -side top -anchor w -fill x -padx 6
 
   frame .fr3.right.bottom.scrlist2
   pack .fr3.right.bottom.scrlist2 -side left -anchor w

   listbox .fr3.right.bottom.scrlist2.colors -height 9 -width 18 -yscrollcommand \
                                            ".fr3.right.bottom.scrlist2.srollbar set"
   bind .fr3.right.bottom.scrlist2.colors <ButtonRelease-1> {
                                               set colored_file [selection get]
                                             }
   update2ndListbox  ;###########

   pack .fr3.right.bottom.scrlist2.colors -side left
   scrollbar .fr3.right.bottom.scrlist2.srollbar -command ".fr3.right.bottom.scrlist2.colors yview"
   pack .fr3.right.bottom.scrlist2.srollbar -side left -fill y

   frame .fr3.right.bottom.right ;# -relief raised -highlightthickness 2
   pack .fr3.right.bottom.right -side right -fill x

   frame .fr3.right.bottom.right.1 ;# -relief raised -highlightthickness 1
   frame .fr3.right.bottom.right.2 ;# -relief raised -highlightthickness 1
   frame .fr3.right.bottom.right.3 ;# -relief raised -highlightthickness 1
   pack .fr3.right.bottom.right.1 -side top -anchor e -fill x -padx 6 -pady 4
   pack .fr3.right.bottom.right.2 .fr3.right.bottom.right.3 -side top -anchor e -fill x -pady 4

   label .fr3.right.bottom.right.1.label -text "Color selection File name:"
   entry .fr3.right.bottom.right.1.entry -highlightthickness 0 -width 25 \
                                         -relief sunken -textvariable colored_file
   trace variable colored_file w forceMacroName
   pack .fr3.right.bottom.right.1.label -side top
   pack .fr3.right.bottom.right.1.entry -side top -pady 3
    bind .fr3.right.bottom.right.1.entry <Return> { }

   button .fr3.right.bottom.right.2.save -text "Save" -command {saveCurrentColors}
   button .fr3.right.bottom.right.2.delete -text "Delete" -command {deleteColorFile $colored_file}
   pack .fr3.right.bottom.right.2.delete .fr3.right.bottom.right.2.save \
                                      -side right -anchor e -padx 15 -pady 14

   button .fr3.right.bottom.right.3.load -text "Load" -command {loadIt}

   button .fr3.right.bottom.right.3.cancel -text " Exit " -command {destroy .}
   pack .fr3.right.bottom.right.3.cancel .fr3.right.bottom.right.3.load \
                                                -side right -anchor e -padx 15
   return 0 
}

;###############
;# dialog -
;#
;###########################################################################
proc dialog {w title text bitmap default args} {
        global button
 
        # 1. Create the top-level window and divide it into top
        # and bottom parts.
 
        toplevel $w -class Dialog

        wm geometry $w -40-420
        wm title $w $title
        wm iconname $w Dialog
        frame $w.top -relief raised -bd 1
        pack $w.top -side top -fill both
        frame $w.bot -relief raised -bd 1
        pack $w.bot -side bottom -fill both
 
        # 2. Fill the top part with the bitmap and message.
 
        message $w.top.msg -width 3i -text $text\
                        -font -Adobe-Times-Medium-R-Normal-*-180-*
        pack $w.top.msg -side right -expand 1 -fill both\
                        -padx 3m -pady 3m
        if {$bitmap != ""} {
                label $w.top.bitmap -bitmap $bitmap
                pack $w.top.bitmap -side left -padx 3m -pady 3m
        }
 
        # 3. Create a row of buttons at the bottom of the dialog.
 
        set i 0
        foreach but $args {
                button $w.bot.button$i -text $but -command\
                                "set button $i"
                if {$i == $default} {
                        frame $w.bot.default -relief sunken -bd 1
                        raise $w.bot.button$i
                        pack $w.bot.default -side left -expand 1\
                                        -padx 3m -pady 2m
                        pack $w.bot.button$i -in $w.bot.default\
                                        -side left -padx 2m -pady 2m\
                                        -ipadx 2m -ipady 1m
                } else {
                        pack $w.bot.button$i -side left -expand 1\
                                        -padx 3m -pady 3m -ipadx 2m -ipady 1m
                }
                incr i
        }
 
        # 4. Set up a binding for <Return>, if there`s a default,
        # set a grab, and claim the focus too.
 
        if {$default >= 0} {
                bind $w <Return> "$w.bot.button$default flash; \
                        set button $default"
        }
        set oldFocus [focus]
        grab set $w
        focus $w

        # 5. Wait for the user to respond, then restore the focus
        # and return the index of the selected button.

        tkwait variable button
        destroy $w
        focus $oldFocus
        return $button
}

;###################################################################
;# 
;#       Main starts here
;#
;###################################################################
wm geometry . -0-250
getXColor
displayMainWindow
.fr1.screen invoke ;#default
