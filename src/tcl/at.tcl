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

proc vnmrinit {arg dir} {
  global vsock osType vhost vport
  scan $arg {"%s %d %d"} vhost vport vpid
  if { ($osType == "Interix") || ($osType == "Linux") } {
    binary scan [binary format s $vport] S vport
  }
  if { ($osType != "Interix") } {
    set vsock [socket $vhost $vport]
  }
}

proc vnmrsend {arg} {
  global vsock osType vhost vport
  if { ($osType == "Interix") } {
    set vsock [socket $vhost $vport]
    puts $vsock $arg
    close $vsock
  } else {
    puts $vsock $arg
  }
}

proc winname {path} {
   global env
   set ret [scan $path {/dev/fs/%c/%s} drive name]
   if {$ret == 2} {
      set drive [string index $path 8]
      set newpath "$drive:/$name"
   } else {
      if { (-1 == [string first : $path]) } {
         set newpath $env(SFUDIR)/$path
      } else {
         set newpath $path
      }
   }
   return $newpath
}

set osType [exec uname -s]

set SELECTED_TESTS_FILE_CP  $env(vnmruser)/autotest/atdb/at_selected_tests
set CYCLED_TESTS_FILE       $env(vnmruser)/autotest/atdb/at_cycled_tests
if { $osType == "Interix"} {
   set env(vnmruser) [winname $env(vnmruser)]
   set env(vnmrsystem) [winname $env(vnmrsystem)]
}
vnmrinit \"[lindex $argv 0]\" $env(vnmrsystem)

set AT_DIR               $env(vnmruser)/autotest
set HISTORY_DIR          $AT_DIR/history
set DATA_DIR             $AT_DIR/data
set TEXTS_DIR            $AT_DIR/texts

set TESTS_FILE           $AT_DIR/atdb/at_tests_file      ;#location to add more tests
set GROUPS_FILE          $AT_DIR/atdb/at_groups_file     ;#location to add more groups
set SELECTED_TESTS_FILE  $AT_DIR/atdb/at_selected_tests
set GRAPH_PS_FILE        $AT_DIR/atdb/graph.ps
set TEMP_FILE            $AT_DIR/atdb/at_temp_file
set LOCK_FILE            $AT_DIR/atdb/at_lock_file
set PREV_SET_FILE        $AT_DIR/atdb/at_prev_setting
set REPORT_FILE          $AT_DIR/atrecord_report
set NMR_INIT_FILE        $env(vnmruser)/maclib/ATinit

if { ($osType == "Linux") } {
   set bitmapdir $env(vnmrsystem)/tcl/tklibrary/vnmr
   option readfile $env(vnmrsystem)/app-defaults/Enter
} elseif { ($osType == "Interix") } {
   set bitmapdir $env(SFUDIR)usr/X11R6/include/X11/bitmaps
   option readfile ../../app-defaults/Enter
} else {
   set bitmapdir /usr/openwin/share/include/X11/bitmaps
   option readfile $env(vnmrsystem)/app-defaults/Enter
}

option add *Graph.x.title             "Number of Trials"
#option add *Graph.x.MajorTicks        "1 2 3 4 5 6 7 8 9"
#option add *Graph.y.title             "YY Axis Label"
option add *y.title                   "YY Axis Label"
option add *Graph.y.MinorTicks        ".1 .2 .3 .4 .5 .6 .7 .8 .9" 

#option add *Graph.Legend.symbol       cross
#option add *Graph.Legend.mapped      1
#option add *Graph.Legend.hide        1

#option add *Grid.color               grey50
#option add *Grid.dashes              "" ;#{ 2 2 }
#option add *Grid.hide                no

#option add *active.Color          green ;#active element only
#option add *activeLine.Color          black ;#active element only
#option add *activeLine.Fill          black;# yellow
#option add *activeLine.symbol         none
#option add *activeLine.Pixels        .075i
#option add *activeLine.Pixels        2i

option add *Graph.plotBackground     black

option add *Graph.Element.Symbol      ""      ;#turn off the circle symbols in legend box
#option add *Graph.Element.color       black   ;#color of the trace, all elements
option add *Graph.Element.fill        ""      ;#in the legend box
#option add *Graph.Element.Pixels     .075i
#option add *Graph.Axis.MinorTicks    ".1 .2 .3 .4 .5 .6 .7 .8 .9" 

eval destroy [winfo child .]
wm title . "Auto    Test"
wm iconname . "ATtest"
wm geometry . 750x920+400+40
source $env(vnmrsystem)/tcl/tklibrary/vnmr/deck.tk
source $env(vnmrsystem)/tcl/tklibrary/vnmr/menu2.tk

if { ($osType != "Interix") } {
   if { ![info exists env(BLT_LIBRARY)] } {
      set env(BLT_LIBRARY) $env(vnmrsystem)/tcl/bltlibrary
   }

   load "$env(vnmrsystem)/lib/libBLT.so"
   catch {
       namespace import blt::*
   }
   lappend auto_path $env(BLT_LIBRARY)
}
 
set confpan(clabel,1) "Operator Name:"
set confpan(clabel,2) "Console type:"
set confpan(clabel,3) "Console S/N:"
set confpan(clabel,4) "Data Directory:"
set confpan(clabel,5) "Probe:"
set confpan(clabel,6) "Coil size(mm):"
set confpan(clabel,7) "Max 13C Pulse Power:"
set confpan(clabel,8) "Max 13C Dec.Power:"
set confpan(clabel,9) "Max 15N Pulse Power:"
set confpan(clabel,10) "Max 15N Dec.Power:"
set confpan(clabel,11) "1H Pulse Power:"
set confpan(clabel,12) "Nominal 1H pw90:"

set confpan(nmrvar,1) "at_user"
set confpan(nmrvar,2) "at_consoletype"
set confpan(nmrvar,3) "at_consolesn"
set confpan(nmrvar,4) "at_datadir"
set confpan(nmrvar,5) "at_probetype"
set confpan(nmrvar,6) "at_coilsize"
set confpan(nmrvar,7) "at_max_pwxlvl"
set confpan(nmrvar,8) "at_max_dpwr"
set confpan(nmrvar,9) "at_max_pwx2lvl"
set confpan(nmrvar,10) "at_max_dpwr2"
set confpan(nmrvar,11) "at_tpwr"
set confpan(nmrvar,12) "at_pw90_estimate"

set confpan(tclvar,1) $env(USER)            ;#op name
set confpan(tclvar,2) ""                    ;#con type	
set confpan(tclvar,3) 0000                  ;#sys sn
set confpan(tclvar,4) $DATA_DIR             ;#data result dir
set confpan(tclvar,5) xxxx                  ;#probe id
set confpan(tclvar,6) 0                     ;#coilsize
set confpan(tclvar,7) 0                     ;#max 13C pulse power
set confpan(tclvar,8) 0                     ;#max 13C dec power
set confpan(tclvar,9) 0                     ;#max 15N pulse power
set confpan(tclvar,10) 0                    ;#max 15N dec power
set confpan(tclvar,11) 0                    ;#tpwr
set confpan(tclvar,12) 1                    ;#pw90_estimate

set copan(clabel,1) "Enable VT tests"
set copan(clabel,2) "Enable C13 tests"
set copan(clabel,3) "Enable Lock tests"
set copan(clabel,4) "Enable Gradient tests"

set copan(nmrvar,1) "at_vttest"
set copan(nmrvar,2) "at_c13tests"
set copan(nmrvar,3) "at_locktests"
set copan(nmrvar,4) "at_gradtests"

set copan(nmrval,1) "n"
set copan(nmrval,2) "n"
set copan(nmrval,3) "n"
set copan(nmrval,4) "n"

set copan(tclval,1) 0
set copan(tclval,2) 0
set copan(tclval,3) 0
set copan(tclval,4) 0


set copn(clabel,1) "Auto Plotting"
set copn(clabel,2) "Print Parameters"
set copn(clabel,3) "Graph History"
set copn(clabel,4) "Process During Acq."
set copn(clabel,5) "Plot to file (PostScript printer only)"
set copn(clabel,6) "Repeat Until Stopped"
 
set copn(nmrvar,1) "at_plotauto"
set copn(nmrvar,2) "at_printparams"
set copn(nmrvar,3) "at_graphhist"
set copn(nmrvar,4) "at_wntproc"
set copn(nmrvar,5) "at_plot_to_file"
set copn(nmrvar,6) "at_cycletest"

set copn(nmrval,1) "n"
set copn(nmrval,2) "n"
set copn(nmrval,3) "n"
set copn(nmrval,4) "n"
set copn(nmrval,5) "n"
set copn(nmrval,6) "n"
 
set copn(tclval,1) 0
set copn(tclval,2) 0
set copn(tclval,3) 0
set copn(tclval,4) 0
set copn(tclval,5) 0
set copn(tclval,6) 0

proc readAutotestDb {} {
  global TESTS_FILE testselec \
         GROUPS_FILE grtest validgrp_list \
         tmac_list Label

    set cnt 1
    set fd1 [open $TESTS_FILE r]
    while {[gets $fd1 Line] >= 0} {
         if {[ llength $Line ] == 0} {
            continue
         }
         set tmac  [lindex $Line 0]
         set firstchar [string index $tmac 0]
         if { ($firstchar != "#") } {    ;# # --> comment

            if { $tmac == "Label" } {    ;# Label as a divider
               set tmac ${tmac}${cnt}
               set testselec($tmac,smac) [lindex $Line 0]
               set testselec($tmac,label) [lrange $Line 1 end]
               set testselec($tmac,type) "label"
               set Label($cnt)  $tmac
               incr cnt

            } else {
                set testselec($tmac,smac) [lindex $Line 0]
                set testselec($tmac,label) [lrange $Line 1 end]
                set testselec($tmac,type) "checkbutton"
                lappend Label([expr $cnt-1]) $tmac
              }

            lappend tmac_list $tmac
         }
    }
    close $fd1

    set gcnt 1
    set grpfd [open $GROUPS_FILE r]
    while {[gets $grpfd Line] >= 0} {
       if { [lindex $Line 0] != "#" } {
          if {[lsearch $Line "*:"] == 0} {
             set grtest($gcnt,grlabel)  [string trim [lindex $Line 0] :]
             set grtest($gcnt,grmacs) [lrange $Line 1 end]
             incr gcnt

          } else { 
               foreach bb $Line {
                  lappend grtest([expr $gcnt-1],grmacs) [string trim $bb]
               }
            }
       }
    }    
    close $grpfd
    #Validate macro in available groups
    for {set ii 1} {$ii <= [expr [array size grtest]/2]} {incr ii} {
       foreach aa $grtest($ii,grmacs) {
          set m_exist 0
          foreach bb $tmac_list {
             if { $aa == $bb } {
                set m_exist 1
                break
             }
          }
          if { $m_exist == 0 } { break }
       }   
       # .fr8.mes configure -text "test macro for \"$grtest($ii,grlabel)\" group does not match"
       if { $m_exist == 1 } { lappend validgrp_list $ii }  
    }
}

proc setupAutotest {at_dir} {
   global NMR_INIT_FILE confpan copan copn
     
     vnmrsend "ATglobal" ;#must be the first thing to do

     if ![file isdirectory $at_dir] {
          #dialog 
     }

     if ![ file isdirectory $at_dir/history ] {
         catch { exec mkdir -p $at_dir/history }
     }
     if ![ file isdirectory $at_dir/data ] {
         catch { exec mkdir -p $at_dir/data }
     }
     #if ![ file isdirectory $at_dir/data/params ] {
     #    catch { exec mkdir -p $at_dir/data/params }
     #}
     #if ![ file isdirectory $at_dir/data/texts ] {
     #    catch { exec mkdir -p $at_dir/data/texts }
     #}

     set initFD [open $NMR_INIT_FILE w]

     puts $initFD "cd(autotestdir)"

     puts $initFD "at_user='$confpan(tclvar,1)'"
     puts $initFD "at_consoletype='$confpan(tclvar,2)'"
     puts $initFD "at_consolesn='$confpan(tclvar,3)'"
     puts $initFD "at_datadir='$confpan(tclvar,4)'"
     puts $initFD "at_probetype='$confpan(tclvar,5)'"
     puts $initFD "at_coilsize=$confpan(tclvar,6)"

     puts $initFD "at_max_pwxlvl=$confpan(tclvar,7)"
     puts $initFD "at_max_dpwr=$confpan(tclvar,8)"
     puts $initFD "at_max_pwx2lvl=$confpan(tclvar,9)"
     puts $initFD "at_max_dpwr2=$confpan(tclvar,10)"
     puts $initFD "at_tpwr=$confpan(tclvar,11)"
     puts $initFD "at_pw90_estimate=$confpan(tclvar,12)"

     puts $initFD "at_vttest='$copan(nmrval,1)'"
     puts $initFD "at_c13tests='$copan(nmrval,2)'"
     puts $initFD "at_locktests='$copan(nmrval,3)'"
     puts $initFD "at_gradtests='$copan(nmrval,4)'"

     puts $initFD "at_plotauto='$copn(nmrval,1)'"
     puts $initFD "at_printparams='$copn(nmrval,2)'"
     puts $initFD "at_graphhist='$copn(nmrval,3)'"
     puts $initFD "at_wntproc='$copn(nmrval,4)'"
     puts $initFD "at_plot_to_file='$copn(nmrval,5)'"
     puts $initFD "at_cycletest='$copn(nmrval,6)'"

     close $initFD
     vnmrsend "ATinit"
     return 0
}

proc reConfigButtons {} {
  global atpage

  switch $atpage {
     1 { catch {.fr8.runtest configure -state disabled -disabledforeground  gray70}
         #catch {.fr8.reset configure -state normal}
         catch {.fr8.reset configure -state normal -text "Clear Selection"}
         catch {.fr8.printhist configure -state disabled -disabledforeground  gray70}
         catch {.fr8.runtest configure -state normal}
       }
     2 { catch {.fr8.runtest configure -state disabled -disabledforeground  gray70}
         #catch {.fr8.runtest configure -state normal}
         #catch {.fr8.reset configure -state normal}
         #catch {.fr8.printhist configure -state normal}
         catch {.fr8.reset configure -text "Clear Selection"}
         catch {.fr8.printhist configure -state disabled -disabledforeground  gray70}
       }
     3 { catch {.fr8.runtest configure -state disabled -disabledforeground  gray70}
         catch {.fr8.reset configure -state normal}
         #catch {.fr8.reset configure -state disabled -disabledforeground  gray70}
         #catch {.fr8.printhist configure -state disabled -disabledforeground  gray70}
         catch {.fr8.printhist configure -state normal}
       }
     4 { catch {.fr8.runtest configure -state disabled -disabledforeground  gray70}
         catch {.fr8.reset configure -state disabled -disabledforeground  gray70}
         #catch {.fr8.printhist configure -state disabled -disabledforeground  gray70}
         catch {.fr8.printhist configure -state normal -text "Print Report"}
       }
  }
}

proc getConsoleTypeFreq {} {
  global env

  set conFD [open $env(vnmrsystem)/conpar r] 
  while {[gets $conFD Line] >= 0} {
    switch [lindex $Line 0] {
        "Console" { gets $conFD Line
                    set c_type [lindex [split $Line \"] 1]
                  }
         "h1freq" { gets $conFD Line
                    set c_freq [lindex $Line 1]
                    break
                  }
    }  
  }  
  close $conFD
  return ${c_type}${c_freq}
}

proc runAtTest {} {
  global LOCK_FILE SELECTED_TESTS_FILE CYCLED_TESTS_FILE copn \
         NMR_INIT_FILE confpan SELECTED_TESTS_FILE_CP

    if {![file exists $SELECTED_TESTS_FILE] || [file size $SELECTED_TESTS_FILE] == 0} {
       return
    }
    set num [expr [array size confpan]/3]
    for {set i 1} {$i <= $num} {incr i} {
       sendEntryToNmr $i "confpan"
    }
    if { $copn(tclval,6) == 1 } {
       exec cp -p $SELECTED_TESTS_FILE_CP $CYCLED_TESTS_FILE
    }
    vnmrsend "ATstart"
    after 1000
    atExit
    return 0
}

proc updateTestFile {mc} {
   global SELECTED_TESTS_FILE TEMP_FILE singletest testselec alltest atpage

   if {$alltest == 1 &&  $atpage == 2} {
      return
   }
   if {$singletest($mc) == 1} {
      ;#add this test to selected tests file
      set atFD [open $SELECTED_TESTS_FILE a+]
      puts $atFD "[format "%-8s" $testselec($mc,smac)]"
      close $atFD
   } else { #remove this test from selected tests file
        set atFD [open $SELECTED_TESTS_FILE r]
        set tmpFD [open $TEMP_FILE w]

        while {[gets $atFD Line] >= 0} {
            if {[lindex $Line 0] != $mc} {
               puts $tmpFD $Line
            }
        }
        close $tmpFD
        close $atFD
        catch {exec mv $TEMP_FILE $SELECTED_TESTS_FILE}
     }
   return 0
}

proc doGrpUpdate {grnum} {
  global grtest singletest thisgrp

   foreach mac $grtest($grnum,grmacs) {
      set singletest($mac) $thisgrp($grnum)
             ;#updateTestFile check singletest determining add or remove
      updateTestFile $mac
   } 
}

#ccccc
proc updateGrpFile {grnum grpage} {
  global SELECTED_TESTS_FILE TEMP_FILE thisgrp grtest \
         origfgcolor singletest gpagelist alltest \
         ch1_grp ch2_grp

   #group (SELECT)
   if {($thisgrp($grnum) == 1)} {
      if {($grtest($grnum,grlabel) == "All Tests")} {
         set alltest 1
         foreach i $gpagelist($grpage) {
            if {$i > 1} {    ;# index 1 is All Tests
               .fr1.fc.b.c${i} deselect
               .fr1.fc.b.c${i} configure -state disabled -disabledforeground gray70
               #.fr1.fc.t.prev configure -state disabled -disabledforeground  gray60
               .fr1.fc.t.next configure -state disabled -disabledforeground  gray60
            }
         }
         catch {exec cat /dev/null > $SELECTED_TESTS_FILE}
         after 50 doGrpUpdate $grnum 
         return 0
      }
 
      if {[lrange $grtest($grnum,grlabel) 0 2] == "All Channel 1"} {
         foreach i $ch1_grp {
            if {$thisgrp($i) == 1} {
                 set thisgrp($i) 0
                 doGrpUpdate $i
            }
            .fr1.fc.b.c${i} deselect
            .fr1.fc.b.c${i} configure -state disabled -disabledforeground gray70
         } 
      }
 
      if {[lrange $grtest($grnum,grlabel) 0 2] == "All Channel 2"} {
         foreach i $ch2_grp {
            if {$thisgrp($i) == 1} {
                 set thisgrp($i) 0
                 doGrpUpdate $i
            }
            .fr1.fc.b.c${i} deselect
            .fr1.fc.b.c${i} configure -state disabled -disabledforeground gray70
         } 
      }

      #other groups
      after 500 doGrpUpdate $grnum
      return 0
   }

   #group (DESELECT)
   if {($thisgrp($grnum) == 0)} {
      if {($grtest($grnum,grlabel) == "All Tests")} {
         set alltest 0
         foreach i $gpagelist($grpage) {
            .fr1.fc.b.c${i} configure -state normal
         }
         #.fr1.fc.t.next configure -state normal
         clearSelection "all"
         return 0
      }
      if {[lrange $grtest($grnum,grlabel) 0 2] == "All Channel 1"} {
         foreach i $ch1_grp {
            .fr1.fc.b.c${i} configure -state normal
         } 
      }  
 
      if {[lrange $grtest($grnum,grlabel) 0 2] == "All Channel 2"} {
         foreach i $ch2_grp {
            .fr1.fc.b.c${i} configure -state normal
         }
      }    

      #other groups
      after 500 doGrpUpdate $grnum 
      return 0
   }
}

proc clearSelection {page} {
   global  SELECTED_TESTS_FILE singletest origfgcolor tmac_list \
           validgrp_list thisgrp atpage gpagelist alltest

   if {$page == 0} { set page $atpage }

   if {$alltest == 1 && $atpage == 2} {
      set alltest 0
      dsplConfPage
      .fr0.conf invoke ;#display config page as default
   }

   if {$alltest == 1 && $atpage == 1} {
     #updateGrpFile 1 1
     .fr1.fc.t.next configure -state normal
   }

   if {$page == 3} {  ;#use this button to plot regression 'all'
      vnmrsend "ATregplot('all')"
      return
   }

   foreach aa $validgrp_list { set thisgrp($aa) 0 }
   foreach i $validgrp_list {
      catch {.fr1.fc.b.c${i} configure -state normal}
   }
   foreach i $tmac_list { set singletest($i) 0 }
   catch {exec cat /dev/null > $SELECTED_TESTS_FILE}
}

proc dsplReportPage {} {
  global REPORT_FILE rooth rootw scrollw atpage repmode repbox

    #clearSelection $atpage
    set atpage 4
    eval destroy [winfo child .fr1]

    set repmode "all"
    set repbox ".fr1.ft.l1"
    frame .fr1.ftop -width $rootw
    pack .fr1.ftop -side top -expand 1 -fill x
    frame .fr1.ft -width $rootw
    pack .fr1.ft -side top -expand 1 -fill both

    label .fr1.ftop.dum  -text "                               "
    radiobutton .fr1.ftop.repall -highlightthickness 0 -indicatoron 1 -selectcolor pink \
      -variable repmode -value "all" -text "All " \
              -command {insertReport}
    radiobutton .fr1.ftop.repfail -highlightthickness 0 -indicatoron 1 -selectcolor pink \
      -variable repmode -value "fail" -text "Fail Only " \
              -command {insertReport}
    grid .fr1.ftop.dum -in .fr1.ftop -row 1 -column 0 -sticky w
    grid .fr1.ftop.repall -in .fr1.ftop -row 1 -column 1 -sticky w
    grid .fr1.ftop.repfail -in .fr1.ftop -row 2 -column 1 -sticky w -pady 5


    text .fr1.ft.l1 -relief flat -bg gray60 -yscrollcommand ".fr1.ft.s1 set"
    scrollbar .fr1.ft.s1  -command ".fr1.ft.l1 yview"
    pack .fr1.ft.l1 -side left -fill x -pady 10
    pack .fr1.ft.s1 -side left -fill y -pady 10
  
    insertReport
    reConfigButtons
    return 0
}
proc insertReport {} {
  global REPORT_FILE repmode repbox


    if {![file exists $REPORT_FILE]} {
       return
    }
    ${repbox} delete 1.0 end 
    set f [ open $REPORT_FILE r ]
    if { $repmode == "all" } {
        while { [gets $f Line] >= 0 } {  
            $repbox insert end "$Line \n"
        }
        #catch {.fr8.printhist configure -text "Print Text "}
    } else {
         set lastline ""
         while { [gets $f Line] >= 0 } {  
             if {[lindex $Line 0] == "Fail"} {
                $repbox insert end "$lastline\n"
                $repbox insert end "$Line\n\n"
             }
             set lastline $Line
         }
      }

    close $f 
}

proc reportPrevNext { cat direc } {
   global previndex hist_dspl_box lbox1 histlinecnt
 
    #frame .fr1.ftop -width $rootw
    #pack .fr1.ftop -side top -expand 1 -fill x
    #label .fr1.ftop.dum -text "                                                "
    #button .fr1.ftop.gprev -text "<-" -highlightthickness 0 -pady 0 -command {reportPrevNext gr -1}
    #button .fr1.ftop.gnext -text "->" -highlightthickness 0 -pady 0 -command {reportPrevNext gr 1}
    #pack .fr1.ftop.dum .fr1.ftop.gprev .fr1.ftop.gnext -side left  -padx 4 -pady 10
 
   #set index [expr $previndex+$direc]
   #set hisfile [$lbox1 get $index]
   #$lbox1 selection clear 0 $histlinecnt
   #$lbox1 see $index
   #$lbox1 selection set $index
   #histTextGraph $hisfile $index
}



proc dsplGenPage {} {
  global atpage

    #clearSelection $atpage
    set atpage 2
    eval destroy [winfo child .fr1]
    frame .fr1.ftop
    frame .fr1.ft
    frame .fr1.bt
    grid .fr1.ftop -in .fr1 -row 0 -column 1 -columnspan 5 -sticky news
    grid .fr1.ft -in .fr1 -row 1 -column 1 -columnspan 2 -sticky news
    grid .fr1.bt -in .fr1 -row 3 -column 1 -columnspan 2 -sticky news
    makeGenPanelTop
    #makeGenPanelBot
    reConfigButtons
    return 0
}

proc makeGenPanelBot {} {
   frame .fr1.bt.vardum  -height 419
   grid .fr1.bt.vardum -row 0 -column 0 -sticky news
}

proc makeGenPanelTop {} {
  global testselec singletest tmac_list bitmapdir mlist \
         prev_index Label

  button .fr1.ftop.next -bitmap @$bitmapdir/Right -command {makeGenPage 1}
  button .fr1.ftop.prev -bitmap @$bitmapdir/Left -command {makeGenPage -1}

  pack .fr1.ftop.prev -in .fr1.ftop -side left -padx 5 -pady 15 -anchor e
  pack .fr1.ftop.next -in .fr1.ftop -side left -padx 5 -pady 15 -anchor e


  if {[array size Label] == 1} {
     set mlist(1) $Label(1)
  } else {

      set asize [array size Label]
      set nextlabel 1
      set i 1
      set mlistcnt 1

      while {$i <= $asize} {

        set mlist($mlistcnt) ""

        if {$nextlabel < $asize} {
          incr nextlabel
          set nextlen [llength $Label($nextlabel)]
        } else {
            set nextlen 9999
          }

        if {[llength $Label($i)] >= 16} {
          set mlist($mlistcnt) [lindex $Label($i) 0 15]
          incr i
          incr mlistcnt
          # one Label cannot have more than 16 tests, the rest will be ignored 
        } else {
   
            set mlist($mlistcnt) $Label($i)
            incr i

            while {[set x 1] == 1} {
               if {[expr [llength $mlist($mlistcnt)]+$nextlen] <= 16} {
                  append mlist($mlistcnt) " $Label($nextlabel)"
                  incr i
                  incr nextlabel

                  if {$i > $asize || $nextlabel > $asize} { break }
                  set nextlen [llength $Label($nextlabel)]

               } else {
                    break
                 }
            }
            incr mlistcnt
          }
      }
   } 
  set prev_index 0
  makeGenPage 1   ;#$tmac_list
}

proc makeGenPage {direction} {
  global testselec singletest mlist prev_index alltest origfgcolor

  set a_index [expr $prev_index+$direction]
  if {$alltest == 1} {
     set astate "disabled"
  } else { set astate "normal" }

  if { $a_index <= 0 } {
     .fr1.ftop.prev configure -state disabled -disabledforeground  gray60
     return
  } else {
       if { $a_index <= [array size mlist] } {
          .fr1.ftop.next configure -state normal
       }
    }
  if {$a_index > [array size mlist]} {
     .fr1.ftop.next configure -state disabled -disabledforeground  gray60
     return
  }  else { 
       if { $a_index > 0 } {
          .fr1.ftop.prev configure -state normal
       }  
    }

  eval destroy [winfo child .fr1.ft]
  set i 1
  foreach mc $mlist($a_index) {
     if {[string range $mc 0 4] == "Label"} {
        label .fr1.ft.duml${i}  -text "\n$testselec($mc,label)" -highlightthickness 0 -fg blue
        grid .fr1.ft.duml${i} -in .fr1.ft -row [expr $i+1] -column 0 -sticky nw -padx 9 -pady 3

     } else {
         checkbutton .fr1.ft.c${i} -text [format "%-42s"  " $testselec($mc,label) "] \
           -variable singletest($mc) -highlightthickness 0 -state $astate \
           -disabledforeground $origfgcolor -command "updateTestFile $mc"
         grid .fr1.ft.c${i} -in .fr1.ft -row [expr $i+1] -column 0 -sticky nw -padx 9 -pady 3
     }
     incr i
  }
  if { $prev_index == 0} { 
     .fr1.ftop.prev configure -state disabled -disabledforeground  gray60
  }
  if { $a_index == 1 && $direction == -1 } {
     .fr1.ftop.prev configure -state disabled -disabledforeground  gray60
  }
  if { $a_index == [array size mlist] && $direction == 1 } {
     .fr1.ftop.next configure -state disabled -disabledforeground  gray60
  }

  set prev_index $a_index
}

proc paneGeometry {master} {
  global paa masterwidth

    set masterwidth [winfo width .]
    place $paa(1) -relheight $paa(-percent)
    place $paa(2) -relheight [expr 1.0 - $paa(-percent)]
    place $paa(3) -x $masterwidth -relheight $paa(-percent)
    place $paa(4) -x $masterwidth -relheight [expr 1.0 - $paa(-percent)]
    place $paa(grip) -rely $paa(-percent)
    set paa(size) [winfo height $master]
}
 
proc paneDrag {master D} {
  global paa

    set delta [expr double(-$D) / $paa(size)]
    set paa(-percent) [expr $paa(-percent) - $delta]
    if {$paa(-percent) < 0.0} {
       set paa(-percent) 0.0
    } elseif {$paa(-percent) > 1.0} {
         set paa(-percent) 1.0
      }
    paneGeometry $master
}

proc pane_Create {wid1 wid2 wid3 wid4 args} {
   global paa masterwidth
 
    array set paa $args
    set master $paa(-in)
 
    #destroy [winfo child $master]
    set paa(1) $wid1
    set paa(2) $wid2
    set paa(3) $wid3
    set paa(4) $wid4
 
    set paa(grip) [frame $master.grip -background gray40 -width 10 -height 10 \
                                   -bd 1 -relief raised -cursor crosshair]
    set masterwidth [winfo reqwidth .fr1.ft]

    place $paa(1) -in $master -x 0 -rely 0.0 -anchor nw -relwidth 1.0 \
                                   -height -1 -width -[winfo reqwidth $paa(3)]
    place $paa(2) -in $master -x 0 -rely 1.0 -anchor sw -relwidth 1.0 \
                                   -height -1 -width -[winfo reqwidth $paa(4)]
    place $paa(3) -in $master -x ${masterwidth} -rely 0.0 -anchor ne -height -1
    place $paa(4) -in $master -x ${masterwidth} -rely 1.0 -anchor se -height -1
    place $paa(grip) -in $master -anchor c -relx 0.8
 
    $master configure -background black
 
    bind $master <Configure> [list paneGeometry $master] 
    bind $paa(grip) <ButtonRelease-1> [list paneDrag $master %y] 
}

proc dsplHistPage {} {
  global atpage histmode prevname previndex bitmapdir
  global osType

    set atpage 3
    set prevname ""
    set previndex 0
    set histmode "graph"

    eval destroy [winfo child .fr1]

    frame .fr1.fu
    frame .fr1.ft
    pack .fr1.fu -side top
    pack .fr1.ft -side bottom

    label .fr1.fu.dum -text "           "
    if { ($osType != "Interix") } {
       radiobutton .fr1.fu.bgraph -highlightthickness 0 -indicatoron 1 -selectcolor pink \
      -variable histmode -value "graph" -text "Graph " \
              -command {makeHistPanelTop .fr1.ft}
       radiobutton .fr1.fu.btext -highlightthickness 0 -indicatoron 1 -selectcolor pink \
      -variable histmode -value "text" -text "Text " \
              -command {makeHistPanelTop .fr1.ft}
    #update idletasks
       grid .fr1.fu.dum -in .fr1.fu -row 1 -column 1 -sticky w
       grid .fr1.fu.bgraph -in .fr1.fu -row 1 -column 0 -sticky w
       grid .fr1.fu.btext -in .fr1.fu -row 2 -column 0 -sticky w -pady 5
    } else {
       set histmode "text"
       grid .fr1.fu.dum -in .fr1.fu -row 1 -column 1 -sticky w
    }

    button .fr1.fu.gprev -text "<-" -highlightthickness 0 -pady 0 -command {histPrevNext gr -1}
    button .fr1.fu.gnext -text "->" -highlightthickness 0 -pady 0 -command {histPrevNext gr 1}
    grid .fr1.fu.gprev -in .fr1.fu -row 1 -column 1 -sticky e
    grid .fr1.fu.gnext -in .fr1.fu -row 1 -column 2 -sticky w

    button .fr1.fu.tprev -text "<-" -highlightthickness 0 -pady 0 -command {histPrevNext tx -1}
    button .fr1.fu.tnext -text "->" -highlightthickness 0 -pady 0 -command {histPrevNext tx 1}
    grid .fr1.fu.tprev -in .fr1.fu -row 2 -column 1 -sticky e
    grid .fr1.fu.tnext -in .fr1.fu -row 2 -column 2 -sticky w

    update idletasks
    makeHistPanelTop .fr1.ft
    reConfigButtons
}

proc makeHistPanelTop {f} {
   global HISTORY_DIR histmode hist_dspl_box atpage prevname \
          rooth rootw fr0h fr8h lbox1 previndex histlinecnt scrollw

    set htt [expr $fr0h+$fr8h+[winfo height .fr1.fu]]

    set orient "vertical"
    catch {destroy $f}
    frame $f -width [winfo width .] -height [expr $rooth-$htt]
    pack $f -in [winfo parent $f] -expand true -fill both
    pack propagate $f off
    
    set lbox1 [listbox $f.l1 -highlightthickness 0 -yscrollcommand "$f.s1 set"]
    set sbar1 [scrollbar $f.s1 -orient $orient -command "$f.l1 yview"]
    set lbox2 [text $f.l2 -bg gray60 -yscrollcommand "$f.s2 set"]
    set sbar2 [scrollbar $f.s2 -orient $orient -command "$f.l2 yview"]
    set gfram [frame $f.f2 -bg gray60]
    set dfram [frame $f.dum -bg gray60]

    if { $histmode == "graph" } {
       pane_Create $lbox1 $gfram $sbar1 $dfram -in [winfo parent $lbox1] \
                                       -orient "vertical" -percent 0.2
       set hist_dspl_box $gfram
       .fr1.fu.tnext configure -state disabled -relief flat -text "  "
       .fr1.fu.tprev configure -state disabled -relief flat -text "  "
       .fr1.fu.gnext configure -state normal -relief raised -text "->"
       .fr1.fu.gprev configure -state normal -relief raised -text "<-"
    } else {
         pane_Create $lbox1 $lbox2 $sbar1 $sbar2 -in [winfo parent $lbox1] \
                                       -orient "vertical" -percent 0.2
         set hist_dspl_box $lbox2
         .fr1.fu.tnext configure -state normal -relief raised -text "->"
         .fr1.fu.tprev configure -state normal -relief raised -text "<-"
         .fr1.fu.gnext configure -state disabled -relief flat -text "  "
         .fr1.fu.gprev configure -state disabled -relief flat -text "  "
      }
 
    set histlinecnt 0
    catch {exec ls $HISTORY_DIR} temp_list

    if {[llength $temp_list] != 0} {
       foreach i $temp_list {
          if ![file readable $i] {
             $lbox1 insert end $i
             set hist_map($i) $histlinecnt
             incr histlinecnt
          }
       }     
       bind $lbox1 <ButtonRelease-1> {histTextGraph [selection get] [$lbox1 curselection]}

       if {$atpage == 3 && ($prevname != "")} {  
          set fname $prevname
       } else {
            set fname [lindex $temp_list 0] 
         }

       histTextGraph $fname $previndex
       $lbox1  selection set $hist_map($fname)  ;#defaults
    } else {
       .fr1.fu.tnext configure -state disabled -relief flat -text "  "
       .fr1.fu.tprev configure -state disabled -relief flat -text "  "
       .fr1.fu.gnext configure -state disabled -relief flat -text "  "
       .fr1.fu.gprev configure -state disabled -relief flat -text "  "
      }
      set scrollw [winfo width $sbar1]
}

proc readHistData {hisfile} {
   global HISTORY_DIR totaltrialist graphlabel grcols ttcol pen_styles pen_noink
  
    catch {unset grcols}
    catch {unset pen_styles}
    catch {unset pen_noink}
    set totaltrialist ""
    set graphlabel ""

    set linecnt 0
    set cnt1 1
    set fd1 [ open $HISTORY_DIR/${hisfile} r ]
    while {[gets $fd1 Line] >= 0} {
         if {[llength $Line] == 0} {
            continue
         } 

         set sindex [lindex $Line 0]
         if { ($sindex == "#Test:") } {
            set graphlabel  [lrange $Line 1 end]
            continue
         }

         if { $sindex == "#Average:" } {
            set ttcol [expr [llength $Line]-1]
            for {set i 1} {$i <= $ttcol} {incr i} {
                set grcols($i,avrg) [lindex $Line $i]
            }
            continue
         }

         if { [lindex $sindex 0] == "#Std" } {
            for {set i 1} {$i <= $ttcol} {incr i} {
                set grcols($i,stdev) [lindex $Line [expr $i+1]]
            }
            continue
         }

         if { ($sindex == "#Date") } {
            for {set i 1} {$i <= $ttcol} {incr i} {
                set grcols($i,grlabel) [lindex $Line $i]
            }

         } else {
              set fail_line 0
              set last_index [expr [llength $Line]-1]

              if {[lindex $Line $last_index] == "Fail"} {
                 set fail_line 1 
              }

              set lo_end 0,0
              set hi_end 0,0
              for {set i 1} {$i <= $ttcol} {incr i} {
                  set thisval [lindex $Line [expr $i+3]]
                  lappend grcols($i,valist) $thisval

                  set lo_end [expr $cnt1-abs(($cnt1*2.00)/100)]
                  set hi_end [expr $cnt1+abs(($cnt1*2.00)/100)]


                  #mark the index of the graph element to pass or fail
                  #use vX to weight
                  if { $fail_line == 1 } {
                     lappend pen_styles($i) "penred $lo_end $hi_end"
                  }  else {
                        lappend pen_styles($i) "penyel $lo_end $hi_end"
                     }
                  lappend pen_noink($i) "pen0ink $lo_end $hi_end"
              }

              lappend totaltrialist $cnt1
              incr cnt1
           }
         incr linecnt
    }
    close $fd1
}

proc printHist {} {
  global HISTORY_DIR GRAPH_PS_FILE histfile histmode grbox lege_acti_index atpage \
         REPORT_FILE 

   if {$atpage == 4} {
      vnmrsend "printon cat('$REPORT_FILE') printoff"
      return
   }
 
   if {$atpage == 3} {
      if {$histmode == "graph"} {
         #$gb postscript output $GRAPH_PS_FILE
         #set execstring "lp -o nobanner $GRAPH_PS_FILE"
         vnmrsend "ATregplot('$histfile',$lege_acti_index)"
      } else {
           #set execstring "cat $HISTORY_DIR/${histfile} | /usr/bin/unix2dos | lp -o nobanner "
           #catch { exec sh -c $execstring }
           vnmrsend "printon cat('$HISTORY_DIR/${histfile}') printoff"
        }
      return
   }
}

proc histPrevNext { cat direc } {
   global previndex hist_dspl_box lbox1 histlinecnt

   set index [expr $previndex+$direc]
   set hisfile [$lbox1 get $index]
   $lbox1 selection clear 0 $histlinecnt
   $lbox1 see $index
   $lbox1 selection set $index
   histTextGraph $hisfile $index
}

proc histTextGraph {hfile index} {
  global HISTORY_DIR histfile histmode totaltrialist graphlabel \
         grbox prevname hist_dspl_box prevmode lege_acti_index \
         lbox1 previndex grcols ttcol elemlist histlinecnt pen_styles pen_noink

   if {$hfile == $prevname && $prevmode == $histmode} {
      return
   }
   set histfile $hfile
   set grbox ${hist_dspl_box}.g
   set graphlabel ""
   set active_ele ""
   readHistData $histfile

   $lbox1 see $index

   if { $histmode == "graph" } {
      destroy [winfo child $hist_dspl_box]
      graph $grbox -bg gray60 -title $graphlabel
      vector  vX
      vX set $totaltrialist
      set xticklist ""
      set trialist_len [llength $totaltrialist]
      if {$trialist_len <= 20} {
         set xticklist $totaltrialist
      } else {
           set xincr [expr ((($trialist_len/20)+4)/5)*5]           
           set sum 0
           while {$sum <= $trialist_len} {
              lappend xticklist $sum
              incr sum $xincr
           }
        }
      frame $grbox.f
      label $grbox.f.avrg -text "Average: " -bg black -fg snow
      label $grbox.f.avrgval -text "$grcols(1,avrg)" -bg black -fg snow
      label $grbox.f.stdev -text "        Std Dev: " -bg black -fg snow
      label $grbox.f.stdeval -text "$grcols(1,stdev)" -bg black -fg snow
      table $grbox.f $grbox.f.avrg  0,0 -anchor nw \
                     $grbox.f.avrgval  0,1 -anchor nw \
                     $grbox.f.stdev  0,2 -anchor nw \
                     $grbox.f.stdeval  0,3 -anchor nw

      #create avrg stdev displayed box
      $grbox marker create window -name avgstd -coords " $trialist_len 0.93" \
                                  -window $grbox.f -under true -anchor se -hide no
      #.g element configure "line1" -styles { myPen 2.0 3.0 }
      #to mark fail data points
      $grbox pen create penred -color red  -symbol cross
      $grbox pen create penyel -color yellow  -symbol cross
      $grbox pen create pen0ink -pixels 0  -symbol "" ;#-color black  -symbol cross
      $grbox axis configure x -majorticks $xticklist
      set elemlist ""

      for {set i 1} {$i <= $ttcol} {incr i} {
         catch {unset vY$i}
         catch {unset y_axis$i}
         vector  vY${i}
         vY${i} set  $grcols($i,valist)

         $grbox axis create y_axis$i

         $grbox element create elem$i -xdata vX -ydata vY${i} -mapy y_axis$i -linewidth 0 \
                             -weights vX -label $grcols($i,grlabel) ;# -styles $pen_styles($i)
         set low_up_limits [$grbox axis limits y_axis$i]
         set lol [lindex $low_up_limits 0]
         set upl [lindex $low_up_limits 1]
         set ttt [expr (($upl-$lol)*20)/100]
         set lower_l [expr $lol-$ttt] 
         set upper_l [expr $upl+$ttt] 
         $grbox axis configure y_axis$i -min $lower_l -max $upper_l
         lappend elemlist elem$i
      }
      set active_ele elem1
      set lege_acti_index 1
      $grbox element configure $active_ele -styles $pen_styles(1)
      eval $grbox legend activate $active_ele
      $grbox yaxis use y_axis1
      table ${hist_dspl_box} $grbox 0,0  -fill both

      bind $grbox <ButtonRelease-1>  {
         global lege_acti_index
         set xax %x
         set yax %y
         set lname [$grbox legend get @$xax,$yax]

         if { [lsearch $elemlist $lname] != "-1" } { ;#inside legend box
            for {set i 1} {$i <= $ttcol} {incr i} {
               if { "elem$i" == $lname } {
                 set active_elem $lname
                 set lege_acti_index $i

               } else {
                   $grbox element configure elem$i -pixels 0
                   $grbox element configure elem$i -styles $pen_noink($i)
                   $grbox element configure elem$i -symbol "" 
                   eval $grbox legend deactivate elem$i
                 }
            }
            $grbox element configure $active_elem -styles $pen_styles($lege_acti_index)
            eval $grbox legend activate $active_elem
            $grbox yaxis use y_axis$lege_acti_index
            $grbox.f.avrgval configure -text "$grcols($lege_acti_index,avrg)"
            $grbox.f.stdeval configure -text "$grcols($lege_acti_index,stdev)"
         }
      }
      catch {.fr8.printhist configure -text "Plot Graph"}
      catch {.fr8.reset configure -text "Plot All Graphs"}
      set hprevbut .fr1.fu.gprev
      set hnextbut .fr1.fu.gnext

   } else { ;# Display Text
        ${hist_dspl_box} delete 1.0 end
        set f [ open $HISTORY_DIR/${histfile} r ]
        set insertcnt 1
        while { [gets $f Line] >= 0 } {
            set last_index [expr [llength $Line]-1]
            if {[lindex $Line $last_index] == "Fail"} {
               $hist_dspl_box insert end "[string trim $Line PassFail] \n"
               $hist_dspl_box tag add failtest $insertcnt.0 $insertcnt.end
               $hist_dspl_box tag configure failtest  -foreground  orange
            } else {
                  $hist_dspl_box insert end "[string trim $Line PassFail] \n"
              }
            incr insertcnt
        }
        close $f
        catch {.fr8.printhist configure -text "Print Text "}
        set hprevbut .fr1.fu.tprev
        set hnextbut .fr1.fu.tnext
     }

   set previndex $index
   set prevname $histfile
   set prevmode $histmode

   if { $index <= 0 } {
      $hprevbut configure -state disabled -disabledforeground  gray60
   } else {
       if { $index <= $histlinecnt} {
         $hnextbut configure -state normal
       }
     }  

   if {$index == [expr $histlinecnt-1]} {
      $hnextbut configure -state disabled -disabledforeground  gray60
   }  else {
        if { $index > 0 } {
           $hprevbut configure -state normal
        }
     }  
}

proc sendEntryToNmr {index name} {
   upvar $name aa
    
   if {$aa(nmrvar,$index) == "at_tpwr" || \
       $aa(nmrvar,$index) == "at_pw90_estimate" || \
       $aa(nmrvar,$index) == "at_coilsize" || \
       $aa(nmrvar,$index) == "at_max_pwxlvl" || \
       $aa(nmrvar,$index) == "at_max_dpwr" || \
       $aa(nmrvar,$index) == "at_max_pwx2lvl" || \
       $aa(nmrvar,$index) == "at_max_dpwr2"} { 

      vnmrsend "$aa(nmrvar,$index)=$aa(tclvar,$index)"
   } else {
      vnmrsend "$aa(nmrvar,$index)='$aa(tclvar,$index)'"
   }
   return 0
}

proc sendYesNoToNmr {index name} {
   upvar $name aa

   if { $aa(tclval,$index) == 1 } {
      vnmrsend "$aa(nmrvar,$index)='y'"
      set aa(nmrval,$index) "y"
   } else {
        vnmrsend "$aa(nmrvar,$index)='n'"
        set aa(nmrval,$index) "n"
     }

   return 0
}

proc makeConfPanelTop {} {
  global confpan copan copn atpmode

   frame .fr1.fl.tl
   frame .fr1.fl.tr
   frame .fr1.fl.bl
   frame .fr1.fl.br

   grid .fr1.fl.tl -in .fr1.fl -row 1 -column 1 -sticky nw
   grid .fr1.fl.tr -in .fr1.fl -row 1 -column 3
   grid .fr1.fl.bl -in .fr1.fl -row 3 -column 1 -sticky nw
   grid .fr1.fl.br -in .fr1.fl -row 3 -column 3 -sticky nw

   frame .fr1.fl.duma -height 20 -width 40
   grid .fr1.fl.duma -row 0 -column 2 -sticky news
   frame .fr1.fl.dumb -height 20
   grid .fr1.fl.dumb -row 2 -column 2 -sticky news

   set num [expr [array size confpan]/3]
   set rr 1
   set master .fr1.fl.tl
   set xval 20
   for {set i 1} {$i <= $num} {incr i} {

     label ${master}.l${i} -text [format "%-12s"  "$confpan(clabel,$i) "]
     entry ${master}.e${i} -highlightthickness 0 -width 14 \
                           -relief sunken -textvariable confpan(tclvar,$i)
          bind ${master}.e${i} <Return> "sendEntryToNmr $i confpan" 
          bind ${master}.e${i} <FocusOut> "sendEntryToNmr $i confpan" 
     grid ${master}.l${i} -in ${master} -row $rr -column 0 -sticky nw -padx $xval -pady 3
     grid ${master}.e${i} -in ${master} -row $rr -column 1 -sticky nw -pady 3

     incr rr
     if { $rr == 7 } {
        set master .fr1.fl.tr
        set xval 10
        set rr  1
     }
   }

   set nm1 [expr [array size copn]/4]
   set rrr 1
   for {set i 1} {$i <= $nm1} {incr i} {

      checkbutton .fr1.fl.bl.ca${i} -text "$copn(clabel,$i) " -highlightthickness 0 \
                       -variable copn(tclval,$i) -command "sendYesNoToNmr $i copn"

       grid .fr1.fl.bl.ca${i} -in .fr1.fl.bl -row $rrr -sticky nw -padx 50 -pady 3
       incr rrr
   }
   .fr1.fl.bl.ca[expr $i-1] configure -pady 15

   set num1 [expr [array size copan]/4]

   set rr1 1
   for {set i 1} {$i <= $num1} {incr i} {
      checkbutton .fr1.fl.br.ca${i} -text "$copan(clabel,$i) " -highlightthickness 0  \
                        -variable copan(tclval,$i) -command "sendYesNoToNmr $i copan"
      grid .fr1.fl.br.ca${i} -in .fr1.fl.br -row $rr1 -sticky nw -padx 40 -pady 3
      incr rr1
   }

   #label .fr1.fl.a -text [format "%-22s"  "  A. T. P.  mode: "]
   #radiobutton .fr1.fl.ayes -highlightthickness 0 -width 6 -indicatoron 0 \
   #           -selectcolor pink -variable atpmode -value y -text " yes " \
   #           -command { }
   #radiobutton .fr1.fl.ano -highlightthickness 0 -width 6 -indicatoron 0 \
   #           -selectcolor pink -variable atpmode -value n -text "no" \
   #           -command { }
   #grid .fr1.fl.a -in .fr1.fl -row 6 -column 2 -sticky e ;# -pady 8
   #grid .fr1.fl.ayes -in .fr1.fl -row 6 -column 3 -sticky w
   #grid .fr1.fl.ano -in .fr1.fl -row 7 -column 3 -sticky w
   #set atpmode n
   return 0
}

proc makeConfPanelMid {} {
  global grtest validgrp_list bitmapdir prv_index \
         gpagelist
 
  frame .fr1.fc.t
  frame .fr1.fc.b
  pack .fr1.fc.t -in .fr1.fc -side top -anchor w
  pack .fr1.fc.b -in .fr1.fc -side top -anchor w


  button .fr1.fc.t.next -bitmap @$bitmapdir/Right -command {makeGrpPage 1}
  button .fr1.fc.t.prev -bitmap @$bitmapdir/Left -command {makeGrpPage -1}

  pack .fr1.fc.t.prev -in .fr1.fc.t -side left -padx 20 -pady 15 -anchor w
  pack .fr1.fc.t.next -in .fr1.fc.t -side left  -pady 15 -anchor w

  if {[llength $validgrp_list] <= 11} {
    .fr1.fc.t.prev configure -state disabled -disabledforeground  gray60
    .fr1.fc.t.next configure -state disabled -disabledforeground  gray60
    set gpagelist(1) $validgrp_list
  } else {
       set tmplist $validgrp_list
       set cnt 1
       while {[llength $tmplist] > 11} {
          set gpagelist($cnt) [lrange $tmplist 0 11]
          set tmplist [lrange $tmplist 12 end]
          incr cnt
       } 
       if {$tmplist > 0} {
          set gpagelist($cnt) $tmplist
       }
    }   
  set prv_index 0
  makeGrpPage 1
}

proc makeGrpPage {direc} {
  global grtest thisgrp validgrp_list prv_index gpagelist \
         ch1_grp ch2_grp
   
  set gpage_num [expr $prv_index+$direc]

  if { $gpage_num <= 0 } {
     .fr1.fc.t.prev configure -state disabled -disabledforeground  gray60
     return
  } else {
       if { $gpage_num <= [array size gpagelist] } {
          .fr1.fc.t.next configure -state normal
       } 
    }   
  if {$gpage_num > [array size gpagelist]} {
     .fr1.fc.t.next configure -state disabled -disabledforeground  gray60
     return
  }  else {
       if { $gpage_num > 0 } {
          .fr1.fc.t.prev configure -state normal
       } 
    }   

  eval destroy [winfo child .fr1.fc.b]
  foreach ii $gpagelist($gpage_num) {
     checkbutton .fr1.fc.b.c${ii} -text [format "%-28s"  " $grtest($ii,grlabel) "] \
         -variable thisgrp($ii) -highlightthickness 0 -command "updateGrpFile $ii $gpage_num"
     grid .fr1.fc.b.c${ii} -in .fr1.fc.b -row [expr $ii+1] -column 0 -sticky nw -padx 20 -pady 3
      
     ################################################################
     #This section is hardcoded for George Gray on September 18 1999.
     #will figure out the systematic way entering second level of "All Test"
     if { [lrange $grtest($ii,grlabel) 0 2] == "All Channel 1" } {
           set ch1_grp ""
        } else {
             if { [lrange $grtest($ii,grlabel) 0 1] == "Channel 1" } {
                append ch1_grp "$ii "
             } 
          }

        if { [lrange $grtest($ii,grlabel) 0 2] == "All Channel 2" } {
           set ch2_grp ""
        } else {
             if { [lrange $grtest($ii,grlabel) 0 1] == "Channel 2" } {
                append ch2_grp "$ii "
             } 
          }
     ################################################################
  }   

  if { $prv_index == 0} {
     .fr1.fc.t.prev configure -state disabled -disabledforeground  gray60
  }
  if { $gpage_num == 1 && $direc == -1 } {
     .fr1.fc.t.prev configure -state disabled -disabledforeground  gray60
  }
  if { $gpage_num == [array size gpagelist] && $direc == 1 } {
     .fr1.fc.t.next configure -state disabled -disabledforeground  gray60
  }
 
  set prv_index $gpage_num
  return 0
}

proc makeConfPanelBot {} {
  frame .fr1.fb.vardum  -height 123
  grid .fr1.fb.vardum -row 0 -column 0 -sticky news
}

proc dsplConfPage {} {
  global atpage rooth rootw fr0h fr8h thisgrp alltest
 
  #clearSelection $atpage
  set atpage 1

  eval destroy [winfo child .fr1]
  frame .fr1.fl ;#  -highlightthickness 1
  frame .fr1.fc
  frame .fr1.fb

  grid .fr1.fl -in .fr1 -row 0 -column 0  -sticky nw
  grid .fr1.fc -in .fr1 -row 10 -column 0 -sticky nw 
  grid .fr1.fb -in .fr1 -row 31 -column 0 -columnspan 2 -sticky nw 
 
  makeConfPanelTop
  makeConfPanelMid
  #makeConfPanelBot

  if { $alltest == 1 } {
    set thisgrp(1) 1
    updateGrpFile  1 1
  }

  reConfigButtons
  update idletasks
  set rooth [winfo height .]
  set rootw [winfo width .]
  set fr0h [winfo height .fr0]
  set fr8h [winfo height .fr8]

  return 0
}

proc updatePrevSetFile {} {
  global PREV_SET_FILE confpan copan copn 
   set fd [open $PREV_SET_FILE w]
     puts  $fd "set confpan(tclvar,1) $confpan(tclvar,1)"
     puts  $fd "set confpan(tclvar,2) $confpan(tclvar,2)"
     puts  $fd "set confpan(tclvar,3) $confpan(tclvar,3)"
     puts  $fd "set confpan(tclvar,4) $confpan(tclvar,4)"
     puts  $fd "set confpan(tclvar,5) $confpan(tclvar,5)"
     puts  $fd "set confpan(tclvar,6) $confpan(tclvar,6)"
     puts  $fd "set confpan(tclvar,7) $confpan(tclvar,7)"
     puts  $fd "set confpan(tclvar,8) $confpan(tclvar,8)"
     puts  $fd "set confpan(tclvar,9) $confpan(tclvar,9)"
     puts  $fd "set confpan(tclvar,10) $confpan(tclvar,10)"
     puts  $fd "set confpan(tclvar,11) $confpan(tclvar,11)"
     puts  $fd "set confpan(tclvar,12) $confpan(tclvar,12)"

     puts  $fd "set copan(tclval,1) $copan(tclval,1)"
     puts  $fd "set copan(tclval,2) $copan(tclval,2)"
     puts  $fd "set copan(tclval,3) $copan(tclval,3)"
     puts  $fd "set copan(tclval,4) $copan(tclval,4)"
     puts  $fd "set copan(nmrval,1) $copan(nmrval,1)"
     puts  $fd "set copan(nmrval,2) $copan(nmrval,2)"
     puts  $fd "set copan(nmrval,3) $copan(nmrval,3)"
     puts  $fd "set copan(nmrval,4) $copan(nmrval,4)"

     puts  $fd "set copn(tclval,1) $copn(tclval,1)"
     puts  $fd "set copn(tclval,2) $copn(tclval,2)"
     puts  $fd "set copn(tclval,3) $copn(tclval,3)"
     puts  $fd "set copn(tclval,4) $copn(tclval,4)"
     puts  $fd "set copn(tclval,5) $copn(tclval,5)"
     puts  $fd "set copn(tclval,6) $copn(tclval,6)"
     puts  $fd "set copn(nmrval,1) $copn(nmrval,1)"
     puts  $fd "set copn(nmrval,2) $copn(nmrval,2)"
     puts  $fd "set copn(nmrval,3) $copn(nmrval,3)"
     puts  $fd "set copn(nmrval,4) $copn(nmrval,4)"
     puts  $fd "set copn(nmrval,5) $copn(nmrval,5)"
     puts  $fd "set copn(nmrval,6) $copn(nmrval,6)"

     puts  $fd "wm geometry . [wm geometry .]"
   close $fd
} 

proc atExit {} {
  global LOCK_FILE confpan copan env NMR_INIT_FILE

   catch {exec rm $LOCK_FILE}
   catch {exec chmod 666 $NMR_INIT_FILE}
   updatePrevSetFile
   destroy .
}

proc dsplMainWindow {} {
  global atpage

  frame .fr0                        ;#decks
  frame .fr1 -relief raised         ;#-highlightthickness 1
  frame .fr8 -highlightthickness 0  ;#dialog and Exit
  pack  .fr0 -anchor w -side top
  pack  .fr1 -anchor w -side top
  pack  .fr8 -anchor w -side bottom

  deck .fr0.conf 1 -text "Configuration"  -highlightthickness 0 \
       -command {selectdeck .fr0.conf {dsplConfPage}}
  deck .fr0.gereral 1 -text "Test Library"  -highlightthickness 0 \
       -command {selectdeck .fr0.gereral {dsplGenPage}}
  deck .fr0.hist 1 -text "  History  "  -highlightthickness 0 \
       -command {selectdeck .fr0.hist {dsplHistPage}}
  deck .fr0.report 1 -text " Test Report "  -highlightthickness 0 \
       -command {selectdeck .fr0.report {dsplReportPage}}
  deck .fr0.dum 1 -text "                                "  -highlightthickness 0 \
       -command {selectdeck .fr0.dum {}} -state disabled
  #label .fr0.dum -text "                     " -relief flat -highlightthickness 0

  grid .fr0.conf -row 0 -column 0 -sticky nw
  grid .fr0.gereral -row 0 -column 1 -sticky nw
  grid .fr0.hist -row 0 -column 2 -sticky nw
  grid .fr0.report -row 0 -column 3 -sticky nw
  grid .fr0.dum -row 0 -column 4 -sticky nw

  frame .fr0.ldum  -height 5
  grid .fr0.ldum -in .fr0 -row 1 -column 0 -sticky nw

  #label .fr8.mes -text "  test dialog      " -fg blue
  #label .fr8.mes -text "                   " -fg blue
  button .fr8.runtest -text "Begin Test" -relief raised \
                      -highlightthickness 1 -command {runAtTest}
  #button .fr8.viewrep -text "View Report" -relief raised \
  #                    -highlightthickness 1 -command {viewHistory}
  button .fr8.reset -text "Clear Selections" -relief raised \
                    -highlightthickness 1 -command {clearSelection 0}
  button .fr8.printhist -text " Print Graph " -relief raised \
                       -highlightthickness 1 -command {printHist}
  button .fr8.exit -text " Exit " -relief raised \
                   -highlightthickness 1 -command {atExit}
  #grid .fr8.mes -in .fr8 -row 0 -column 1 -sticky sw -padx 20 -pady 10
  grid .fr8.runtest -in .fr8 -row 1 -column 1 -sticky sw -padx 5
  #grid .fr8.viewrep -in .fr8 -row 1 -column 2 -sticky sw -padx 5
  grid .fr8.reset -in .fr8 -row 1 -column 3 -sticky sw -padx 5
  grid .fr8.printhist -in .fr8 -row 1 -column 5 -sticky sw -padx 5
  grid .fr8.exit -in .fr8 -row 1 -column 6 -sticky sw -padx 5
  return 0
}

#-----------------------Main------------------------------
set alltest 0
dsplMainWindow
if {![file exists $PREV_SET_FILE]} {
   update idletask
   updatePrevSetFile
}
catch {source $PREV_SET_FILE}
readAutotestDb
catch {exec touch $LOCK_FILE}
#set confpan(tclvar,2) [getConsoleTypeFreq]
set atpage 3 ;#only playing trick here, clearSelection will do nothing
.fr0.conf invoke ;#display config page as default
catch {exec cat /dev/null > $SELECTED_TESTS_FILE}
setupAutotest $AT_DIR ;# pass autotest dir
set origfgcolor [.fr8.exit cget -fg]
