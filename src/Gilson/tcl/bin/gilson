# 

set showInjector 0

# If you want the Inject/Load Injector switch to appear in the
# "Main Control" window, uncomment (remove the # symbol) from the next line
# set showInjector 1

vnmrinit \"[lindex $argv 1]\" $env(vnmrsystem)
set SMS_DEV_FILE  $env(vnmrsystem)/smsport
set GIL_INFO_FILE $env(vnmrsystem)/asm/racksetup
set GIL_TEMP_FILE $env(vnmrsystem)/asm/giltemp
set GILALIGN_FILE $env(vnmrsystem)/bin/gilalign

set itemlist "injector rack"
set gilDef(injector,fields) 5
set gilDef(rack,fields)     3
set gilDef(samp,fields)     19

set calList [list rinse inject rack]
set samp_list [list samp,10 samp,1 push,1 samp,4 samp,16 samp,15 \
               samp,12 samp,13 samp,11 samp,3 samp,14 next,1 ]
set samp2_list [list samp,2 samp,5 samp,6 samp,17 samp,18 \
               samp,7 samp,8 samp,9 samp,19]

set labels(samp,1) "SampleVolume"
set labels(samp,2) "SampleWellRate"
set labels(samp,3) "SampleExtraVol"
set labels(samp,4) "SampleKeepFlag"
set labels(samp,5) "SampleHeight"
set labels(samp,6) "SampleDepth"
set labels(samp,7) "MixVolume"
set labels(samp,8) "MixTimes"
set labels(samp,9) "MixFlowRate"
set labels(samp,10) "ProbeVolume"
set labels(samp,11) "ProbeFastRate"
set labels(samp,12) "ProbeSlowVol"
set labels(samp,13) "ProbeSlowRate"
set labels(samp,14) "RinseExtraVol"
set labels(samp,15) "RinseDeltaVol"
set labels(samp,16) "NumRinses"
set labels(samp,17) "NeedleRinseVolume"
set labels(samp,18) "NeedleRinseRate"
set labels(samp,19) "MixHeight"


set labelText(samp,1) "Sample Volume"
set labelText(samp,2) "Sample Well Rate"
set labelText(samp,3) "Sample Extra Vol"
set labelText(samp,4) "Keep Sample"
set labelText(samp,5) "Sample Height"
set labelText(samp,6) "Sample Depth"
set labelText(samp,7) "Mix Volume"
set labelText(samp,8) "Mix Time"
set labelText(samp,9) "Mix Flow Rate"
set labelText(samp,10) "Probe Volume"
set labelText(samp,11) "Probe Fast Rate"
set labelText(samp,12) "Probe Slow Vol"
set labelText(samp,13) "Probe Slow Rate"
set labelText(samp,14) "Rinse Extra Vol"
set labelText(samp,15) "Rinse Delta Vol"
set labelText(samp,16) "Number of Rinses"
set labelText(samp,17) "Needle Rinse Volume"
set labelText(samp,18) "Needle Rinse Rate"
set labelText(samp,19) "Mix Height"

set labels(push,1) "Push Volume"

set labelText(push,1) "Push Volume"

set labels(rack,2) "X center"
set labels(rack,3) "Y center"

set labelText(rack,2) "X center"
set labelText(rack,3) "Y center"


eval destroy [winfo child .]
wm title . "Gilson   Liquid   Handler"
wm iconname . "Gilson"
wm geometry . +500+100
wm minsize . 720 380
set axis(0) ""
set cur_coor_dspl ""
set gil_prompt ""
set cur_dspl ""
set refresh ""
set valEntry(syr_vol) 0
set samps(num) 10

source $tk_library/vnmr/deck.tk
source $tk_library/vnmr/menu2.tk

proc setPushVol {} {
   global pushVol valEntry
   set pushVol [expr $valEntry(samp,10) - $valEntry(samp,1)]
}

proc getGilInfos {} {
  global GIL_INFO_FILE num current gilInfo gilDef rackList \
         rack_select argc argv gilsonFD gilOK pushVol gilPanel

  foreach i {injector rack} {
    set num($i) 0
  }
  if ![file exists $GIL_INFO_FILE] {
    puts "$GIL_INFO_FILE not exist"
    return 1
  }

  set gilFD [open $GIL_INFO_FILE r]
  while {[gets $gilFD Line] >= 0} {
    set arr_name [lindex $Line 0]
    if {($arr_name != "#") && ($arr_name != "")} {
      set count $gilDef($arr_name,fields)
      incr num($arr_name)
      if {$count > [expr [llength $Line]-2]} {
        set count [expr [llength $Line]-2]
      }
      for {set i 0} {$i <= $count} {incr i} {
        set gilInfo($num($arr_name),$arr_name,$i) [lindex $Line [expr $i+1]]
      }
    }
  }
  close $gilFD
  foreach i {injector rack} {
    set current($i) $gilInfo(1,$i,0)
    set current($i,num) 1
  }
  set rackList {}
  for {set i 1} {$i <= $num(rack)} {incr i} {
    lappend rackList " [lindex [split $gilInfo($i,rack,1) {_.}] 1]"
  }
  set rack_select [lindex $rackList 0]
#  if {$argc >= 1} {
#    set arg [lindex $argv 0]
#    for {set i 1} {$i <= $num(rack)} {incr i} {
#       if {$arg == $gilInfo($i,rack,0)} {
#         set current(rack) $gilInfo($i,rack,0)
#         set current(rack,num) $i
#         set rack_select [lindex $rackList [expr $i-1]]
#       }
#    }
#  }
  set gilOK 1
  set gilPanel 1
  set gilsonFD -1
  if {$argc >= 1} {
    set arg [lindex $argv 0]
    if {$arg == "panel1"} {
      set gilOK 0
    }
    if {$arg == "panel2"} {
      set gilOK 0
      set gilPanel 2
    }
  }
  return 0
}

proc isANumber {index} {
     global valEntry labels
     upvar #0 labelText labelText
    
     if {$valEntry(syr_vol) == 0} {
        return
     }

     set x $valEntry($index)
     catch {.fr8.status configure -text ""}
     if [regexp -nocase \
         {^[-+]?([0-9]+\.?[0-9]*|[0-9]*\.?[0-9]+)(e[-+]?[0-9]+)?$} $x] {
      
# Total volume can not be greater than syringe volume
        if {($index == "samp,3") || ($index == "samp,10") || ($index == "samp,14") || \
            ($index == "samp,15") || ($index == "samp,16") } {
           set max $valEntry(samp,3)
           if {$max < $valEntry(samp,14)} {
              set max $valEntry(samp,14)
           }
           set rvol 0
           for {set num 1} {$num <= $valEntry(samp,16)} {incr num} {
              set rvol [expr $rvol + $valEntry(samp,15)]
#             set rvol [expr $rvol + $valEntry(samp,15) * $num]
           }
           set vol [expr $valEntry(samp,10) + $max + $rvol]
           if {$vol > $valEntry(syr_vol)} {
              set sv $valEntry(syr_vol)
              catch {.fr8.status configure -text \
     "Total volume ($vol) exceeds syringe volume ($sv)"}
              if {$index == "samp,3"} {
                 set valEntry(samp,3) [expr $sv - $rvol - $valEntry(samp,10)]
              } elseif {$index == "samp,10"} {
                 set valEntry(samp,10) [expr $sv - $rvol - $max]
              } elseif {$index == "samp,14"} {
                 set valEntry(samp,14) [expr $sv - $rvol - $valEntry(samp,10)]
              } elseif {$index == "samp,15"} {
                 set valEntry(samp,15)  \
                   [expr ($sv - $max - $valEntry(samp,10))/$valEntry(samp,16)]
                 set valEntry(samp,15) [expr int($valEntry(samp,15)*10.0) / 10.0]
              } elseif {$index == "samp,16"} {
                 set tmp [expr $valEntry(samp,10) + $max]
                 set valEntry(samp,16) 0
                 while {[expr $tmp + ($valEntry(samp,16)+1)*$valEntry(samp,15)]\
                        <= $sv} {
                    incr valEntry(samp,16)
                 }
              }
           }
        }

# ProbeValue can not be greater than syringe volume
        if {($index == "samp,10") && \
            ($valEntry(samp,10) > $valEntry(syr_vol))} {
           catch {.fr8.status configure -text \
  "$labelText(samp,10) cannot be greater than syringe volume ($valEntry(syr_vol))"} 
           set valEntry(samp,10) $valEntry(syr_vol)
           set valEntry(samp,10,orig) $valEntry(syr_vol)
        }

# ProbeSlowVol can not be greater than syringe volume
        if {($index == "samp,12") && \
            ($valEntry(samp,12) > $valEntry(syr_vol))} {
           catch {.fr8.status configure -text \
  "$labelText(samp,12) cannot be greater than syringe volume ($valEntry(syr_vol))"} 
           set valEntry(samp,12) $valEntry(syr_vol)
           set valEntry(samp,12,orig) $valEntry(syr_vol)
        }

# NeedleRinseVolume can not be greater than syringe volume
        if {($index == "samp,17") && \
            ($valEntry(samp,17) > $valEntry(syr_vol))} {
           catch {.fr8.status configure -text \
  "$labelText(samp,17) cannot be greater than syringe volume ($valEntry(syr_vol))"} 
           set valEntry(samp,17) $valEntry(syr_vol)
           set valEntry(samp,17,orig) $valEntry(syr_vol)
        }

# ProbeValue can not be less than SampleVolume.  Reduce Sample Volume is necessary
        if {($index == "samp,1") || ($index == "samp,10")} {
           if {$valEntry(samp,10) < $valEntry(samp,1) } {
              catch {.fr8.status configure -text \
                 "$labelText(samp,1) cannot be greater than $labelText(samp,10)"} 
              set valEntry(samp,1) $valEntry(samp,10)
              set valEntry(samp,1,orig) $valEntry(samp,10)
           }
           setPushVol
        }
        if {($index == "samp,5") && ($valEntry(samp,5) < 0)} {
           set valEntry(samp,5) 0
        }
        if {($index == "samp,6") && ($valEntry(samp,6) < 0)} {
           set valEntry(samp,6) 0
        }
        set valEntry($index,orig) $valEntry($index)
        return 0
     } elseif {$index == "samp,6"} {
        set valEntry($index) NOSEEK
     } else {
        if {[info exists valEntry($index,orig)] == 1} {
           set valEntry($index) $valEntry($index,orig);#restore the old value
        }
     }
}

proc getPort {} {
   global SMS_DEV_FILE

   if ![file exists $SMS_DEV_FILE] {
      puts "Vnmr is not configured for sample changers. Run config."
      return -1
   }   

   set portFD [open $SMS_DEV_FILE r]
   set aa [gets $portFD]
   close $portFD
   if {[lindex $aa 1] == "GIL"} {
      return [lindex $aa 0]
   } else {
        puts "Gilson sample changer is not connecting to this console"
        return -1
   } 
   return 0
}

proc connectToGilalign {} {
   global gilsonFD GILALIGN_FILE

   set serial_port [getPort]
   if {[string match $serial_port "a"] || [string match $serial_port "b"]} {
      catch {.fr8.status configure -text "Connecting to Gilson"}
      update idletasks
      set gilsonFD [open "| $GILALIGN_FILE /dev/term/${serial_port} noprint" r+]
      catch {.fr8.status configure -text ""}
      return 0
   } elseif {[string match $serial_port "c1"]} {
      catch {.fr8.status configure -text "Connecting to Gilson"}
      update idletasks
      set gilsonFD [open "| $GILALIGN_FILE /dev/ttyS0 noprint" r+]
      catch {.fr8.status configure -text ""}
      return 0
   } else {
      catch {.fr8.status configure -text "Can't connect to Gilson"}
      set gilsonFD -1
      if {[winfo exists .fr0.syringe] == 1} {
        .fr0.syringe configure -state disabled
        .fr0.calibrate configure -state disabled
      }
      return -1
   }  
   return 0
}

proc sendToGilalign {cmd} {
   global gilsonFD
 
   if {$gilsonFD != -1} {
     puts $gilsonFD "$cmd"
     flush $gilsonFD
     return 0
   } else {
     catch {.fr8.status configure -text "sendToGilalign: Problem sending \"${cmd}\" "}
     return -1
   }  
}

proc readValues {valcnt} { 
   global gilsonFD 
 
   if {$gilsonFD == -1} {
     return -1
   }
   set count 0 
   while { 1 == 1 } { 
      if {$count >= 30} { 
         catch {.fr8.status configure -text "Problem reading from Gilson"}
         return -1 
      }   
 
      set Line "" 
      set stat [gets $gilsonFD Line] 
      if { $stat >= 1 } { 
         if {[llength $Line ] == $valcnt} { 
            return $Line 
         } else {return -1} 
      }   
      incr count 
      after 50 
   } 
} 

proc readPrompt {prompt} {
   global gilsonFD

   if {$gilsonFD == -1} {
     return -1
   }
   set count 0
   while { 1 == 1 } {
      if {$count >= 1800} {               ;#prime pump will take about 90 secs
         catch {.fr8.status configure -text "Problem reading from Gilson"}
         return -1
      }  

      set Line ""
      set stat [gets $gilsonFD Line]
      if { $stat >= 1 } {
         if {[lindex $Line 0] == "$prompt"} {
            return 0
         } else {return -1}
      }  
      incr count
      after 50
   }
}
 
proc quitGilalign {} {
   global gilsonFD gilOK

   if {$gilOK == 1} {
     sendToGilalign "q"
     if {$gilsonFD != -1} {
        catch {close $gilsonFD}
     }   
   }
   destroy .
   return 0
}

# setValue -   value "v"  for Air valve
#              value "j"  for Injector valve
proc setValve {valve state} {
   global gil_prompt gilsonFD
 
    if {$gil_prompt == "Cmds:"} {
      sendToGilalign "$valve" 
    } else {
         catch {.fr8.status configure -text "Wrong prompt, Cannot set valve"}
         return -1
      }  
 
      if {[readPrompt "Cmd:"] == 0} {
         set $gil_prompt "Cmd:"
      } else {
           catch {.fr8.status configure -text "Problem1 setting valve"}
           return -1
        }
 
    sendToGilalign "$state"
    readPrompt "Cmd:"         ;#discard
    sendToGilalign "q" 

    if {[readPrompt "Cmds:"] == 0} {
       set $gil_prompt "Cmds:" 
       return 0
    } else {
         catch {.fr8.status configure -text "Problem2 setting valve"}
         return -1
      }
}

proc resetArm {} {
   global gil_prompt

      sendToGilalign "k"

      if {[readPrompt "Cmds:"] == 0} {
         set $gil_prompt "Cmds:"
         return 0
      } else {
           catch {.fr8.status configure -text "Problem Reseting arm"}
           return -1
        }
}

proc returnHome {} {
   global gil_prompt
 
      sendToGilalign "x"
 
      if {[readPrompt "Cmds:"] == 0} {
         set $gil_prompt "Cmds:"
         return 0
      } else {
           catch {.fr8.status configure -text "Problem going home"}
           return -1
        }
}

proc primePump {} {
   global gil_prompt

      sendToGilalign "p"

      if {[readPrompt "Cmds:"] == 0} {
         set $gil_prompt "Cmds:"
         return 0
      } else {
           catch {.fr8.status configure -text "Problem Priming pump"}
           return -1
        }
}

# moveIt -  cat       - x_rinse, y_rinse, z_rinse, x_inject ...
#           val       - 1, 5, 10, 50, 100
#           direction - "+" or "-" 
proc moveIt {cat val direction} {
   global gil_prompt x_min  x_max  y_min  y_max  z_min  z_max

   upvar $cat aa

    set xtemp [expr $aa + [expr   $val*$direction]]

    switch [string range $cat 0 0] {

        "x" { if {$xtemp < $x_min || $xtemp > $x_max} {return -1}
              if { $direction <= 0 } {set cmd "k"} else {set cmd "l"}
              set axis 0
            }

        "y" { if {$xtemp < $y_min || $xtemp > $y_max} {return -1}
              if { $direction <= 0 } {set cmd "b"} else {set cmd "f"}
              set axis 1
            }

        "z" { if {$xtemp < $z_min || $xtemp > $z_max} {return -1}
              if { $direction <= 0 } {set cmd "d"} else {set cmd "u"}
              set axis 2
            }
    }

   if {$gil_prompt == "Cmd:"} {
      sendToGilalign "${cmd}${val}"
      
      set temp [readValues 3]
      if {$temp == -1} {
         catch {.fr8.status configure -text "Problem sending \"${cmd}${val}\" to gilalign"}
         return -1
      } else {   ;#updating matching entry box
           set aa [format "%7d" [lindex $temp $axis]]
        }

      if {[readPrompt "Cmd:"] == 0} {
         set gil_prompt "Cmd:"
         return 0
      } else {
           catch {.fr8.status configure -text "Problem sending \"${cmd}${val}\" to gilalign"}
           return -1
        }
   } else {
        puts "moveIt: Wrong prompt"
        return -1
     }
}

proc quitCalibration {} {
   global gil_prompt gilsonFD ;# cal_select
      
   if {$gilsonFD != -1} {
     if {$gil_prompt == "Cmd:"} {
        sendToGilalign "q"
     
        readValues 3
        if {[readPrompt "Cmd:"] == 0} {
           set gil_prompt "Cmd:" 
        } else {  
             catch {.fr8.status configure -text "Problem quiting calibration"}
             return -1
          }
     }

     if {$gil_prompt == "Cmd:"} {
        sendToGilalign "q"
     }
     if {$gil_prompt != "Cmds:"} {
       if {[readPrompt "Cmds:"] == 0} {
          set gil_prompt "Cmds:"  
          return 0
       } else {   
          catch {.fr8.status configure -text "Problem1 quiting calibration"} 
          return -1 
       }
     }
   }
   return 0
}
 
proc dsplCoordinates {category} {
   global x_rinse y_rinse z_rinse  x_inject y_inject z_inject \
          x_rack y_rack z_rack rInfo rack1 env \
          cur_coor_dspl gil_prompt current refresh

   if { $refresh == 1 } {

      sendToGilalign "q"

      if {[readPrompt "Cmds:"] == 0} {
         set gil_prompt "Cmds:" 
      } else {
           catch {.fr8.status configure -text "Problem displaying $category Cal. , Wrong prompt"}
           return -1
        }
      set refresh 0   
   } else { 

        if {"$cur_coor_dspl" == "$category"} {
           return 1
        }
        catch {.fr8.status configure -text "Moving to $category position"}
        .fr5.coor.clabel configure -text "" 
        update idletasks

        if {$gil_prompt == "Cmd:"} {
           sendToGilalign "q"
           if {[readValues 3] == -1} {  ;# no need for updating these values 
              catch {.fr8.status configure -text "Problem displaying $category Cal. , Wrong values returned"}
              return -1
           }
           if {[readPrompt "Cmd:"] == 0} {
              set gil_prompt "Cmds:" 
              sendToGilalign "q"
           }

           if {[readPrompt "Cmds:"] == 0} {
              set gil_prompt "Cmds:" 
           } else {
                catch {.fr8.status configure -text "Problem displaying $category Cal. , Wrong prompt"}
                return -1
             }
        }
     }

   set cal_label [string toupper $category]
   .fr5.coor.clabel configure -text "    $cal_label" 

   .fr5.coor.e_x configure -textvariable x_${category}
   .fr5.coor.e_y configure -textvariable y_${category}
   .fr5.coor.e_z configure -textvariable z_${category}

   set cnt 2
   foreach j "1 5 10 50 100" {

      .fr5.coor.s${j}_x configure -command "moveIt x_${category} $j"
      .fr5.coor.s${j}_y configure -command "moveIt y_${category} $j"
      .fr5.coor.s${j}_z configure -command "moveIt z_${category} $j"
  
      incr cnt
   } 

   switch $category {
       "rinse" { set cmd "r" ; set cur_coor_dspl "rinse"}
      "inject" { set cmd "i" ; set cur_coor_dspl "inject"}
        "rack" { set cmd "t" ; set cur_coor_dspl "rack"}
   }

   if {$category == "rack"} {
      if {([info exists rInfo(rack,1)] == 1) && ($rInfo(rack,1) != $rack1)} {
         set rack1 $rInfo(rack,1)
         sendToGilalign "d"
         sendToGilalign "$env(vnmrsystem)/asm/racks/code_$rack1.grk"
         if {[readPrompt "Cmds:"] == 0} {
            set gil_prompt "Cmds:"
         } else {
           catch {.fr8.status configure \
                    -text "Problem sending \"${cmd}\" to gilalign"}
           return -1
         }
      }
   }

   sendToGilalign "$cmd"     ;# r , i , t
      set tmp [readValues 3]
      if {$tmp == -1} {
         catch {.fr8.status configure -text "Problem sending \"${cmd}\" to gilalign"}
         return -1
      } else {      ;# updating x, y, z values on entry box
           set x_${category} [format "%7d" [lindex $tmp 0]]
           set y_${category} [format "%7d" [lindex $tmp 1]]
           set z_${category} [format "%7d" [lindex $tmp 2]]
        }
 
      if {[readPrompt "Cmd:"] == 0} {  ;#ready for sending k#,l#,u#,d#,b#,f# by moveIt{}
         set gil_prompt "Cmd:"
      } else {
           catch {.fr8.status configure -text "Problem sending \"${cmd}\" to gilalign"}
           return -1
        }
    catch {.fr8.status configure -text ""}
    return 0
} 

proc setCenters {} {
   global  gilInfo gil_prompt valEntry

    set rack_path $gilInfo(1,rack,1)

#rack
    set x_center $gilInfo(1,rack,2)
    set y_center $gilInfo(1,rack,3)

#injector
    set xcenter $gilInfo(1,injector,2)
    set ycenter $gilInfo(1,injector,3)
    set zcenter $gilInfo(1,injector,4)


   sendToGilalign "d"  ;#prepare to send rack dir.
   sendToGilalign "$rack_path"

   if {[readPrompt "Cmds:"] != 0} {
      catch {.fr8.status configure -text "Problem setting Center"}
      return -1
   }
   set $gil_prompt "Cmds:"
 
   sendToGilalign "c"
   sendToGilalign " r $x_center $y_center 1013 "
 
   if {[readPrompt "Cmds:"] != 0} {
      catch {.fr8.status configure -text "Problem1 setting Center"}
      return -1
   }
   set $gil_prompt "Cmds:"
 
   sendToGilalign "c"
   sendToGilalign " i $xcenter $ycenter $zcenter "
 
   if {[readPrompt "Cmds:"] != 0} {
      catch {.fr8.status configure -text "Problem2 setting Center"}
      return -1
   }
   set $gil_prompt "Cmds:"
 
   return 0
}
 
proc dsplRackDef {} {
   global cur_dspl cur_coor_dspl
   if {$cur_dspl == 6} {quitCalibration}
   if {$cur_dspl == 3} {return 1}
   set cur_coor_dspl ""
   catch {.fr8.status configure -text ""} 
   removeCurrentDisplay
   set cur_dspl 3
   pack .racks .info -side top -fill x -expand 1 -in .fr3
   pack .fr3 -in .mn -fill both -expand 1
#   returnCurrentRack 1
   selectRack 1
   return 0
}

proc dsplSampleDef {} {
   global cur_dspl cur_coor_dspl

   if {$cur_dspl == 6} {quitCalibration}
   if {$cur_dspl == 2} {return 1}
   set cur_coor_dspl ""
   catch {.fr8.status configure -text ""} 
   removeCurrentDisplay
   set cur_dspl 2
   pack .fr1 -in .mn -fill both -expand 1
   setPushVol
   return 0
}

proc removeCurrentDisplay {} {
   global cur_dspl
   switch $cur_dspl {
      2 {pack forget .fr1}
      3 {pack forget .fr3}
      5 {pack forget .fr4}
      6 {pack forget .fr5}
      default {}
   }
}

# Rack panel definition

proc initRInfo {} {
   global rInfo
   set rInfo(first) {1 3 7 9}
   set rInfo(1,1) {1 2 3 4 5 6 7 8 9}
   set rInfo(1,2) {3 2 1 6 5 4 9 8 7}
   set rInfo(1,3) {7 8 9 4 5 6 1 2 3}
   set rInfo(1,4) {9 8 7 6 5 4 3 2 1}

   set rInfo(2,1) {1 2 3 6 5 4 7 8 9}
   set rInfo(2,2) {3 2 1 4 5 6 9 8 7}
   set rInfo(2,3) {7 8 9 6 5 4 1 2 3}
   set rInfo(2,4) {9 8 7 4 5 6 3 2 1}

   set rInfo(3,1) {1 4 7 2 5 8 3 6 9}
   set rInfo(3,2) {7 4 1 8 5 2 9 6 3}
   set rInfo(3,3) {3 6 9 2 5 8 1 4 7}
   set rInfo(3,4) {9 6 3 8 5 2 7 4 1}

   set rInfo(4,1) {1 6 7 2 5 8 3 4 9}
   set rInfo(4,2) {7 6 1 8 5 2 9 4 3}
   set rInfo(4,3) {3 4 9 2 5 8 1 6 7}
   set rInfo(4,4) {9 4 3 8 5 2 7 6 1}
   set rInfo(pattern) 1
   set rInfo(number) 0
   set rInfo(start) 1
}

proc selectPattern {index} {
   global rInfo
   if {$rInfo(pattern) != 0} {
      .p$rInfo(pattern) configure -relief raised
   }
   set rInfo(pattern) $index
   .p$rInfo(pattern) configure -relief sunken
}

proc numberCircles {} {
   global rInfo
   set st $rInfo(start)
   set first [lindex $rInfo(first) [expr $st-1]]
   for {set i 1} {$i <= 4} {incr i} {
      .p$i delete lines
      for {set j 1} {$j <= 9} {incr j} {
         set num [expr $j-1]
         if {$j == $first} {
            set color red
         } else {
            set color black
         }
         .p$i itemconfigure t$j -text [lindex $rInfo($i,$st) $num] \
            -fill $color
      }
   }
   if {($st == 1) || ($st == 4)} {
      .p1 create line  20m  4m  4m 12m -tags lines
      .p1 create line  20m 12m  4m 20m -tags lines
      .p2 create line  20m  4m 20m 12m -tags lines
      .p2 create line   4m 12m  4m 20m -tags lines
      .p3 create line   4m 20m 12m  4m -tags lines
      .p3 create line  12m 20m 20m  4m -tags lines
      .p4 create line   4m 20m 12m 20m -tags lines
      .p4 create line  12m  4m 20m  4m -tags lines
   } else {
      .p1 create line   4m  4m 20m 12m -tags lines
      .p1 create line   4m 12m 20m 20m -tags lines
      .p2 create line   4m  4m  4m 12m -tags lines
      .p2 create line  20m 12m 20m 20m -tags lines
      .p3 create line  20m 20m 12m  4m -tags lines
      .p3 create line  12m 20m  4m  4m -tags lines
      .p4 create line  20m 20m 12m 20m -tags lines
      .p4 create line  12m  4m  4m  4m -tags lines
   }
   .p1 lower lines
   .p2 lower lines
   .p3 lower lines
   .p4 lower lines
}

proc addcircles {win dir} {
   global rInfo
   set size 24
   set gap 2
   set radius [expr ($size - (3*$gap))/ 3.0 / 2.0]
   $win create oval 1m 1m 7m 7m -outline black -fill $rInfo(bg)
   $win create oval 9m 1m 15m 7m -outline black -fill $rInfo(bg)
   $win create oval 17m 1m 23m 7m -outline black -fill $rInfo(bg)
   $win create text 4m 4m -tags t1
   $win create text 12m 4m -tags t2
   $win create text 20m 4m -tags t3

   $win create oval 1m 9m 7m 15m -outline black -fill $rInfo(bg)
   $win create oval 9m 9m 15m 15m -outline black -fill $rInfo(bg)
   $win create oval 17m 9m 23m 15m -outline black -fill $rInfo(bg)
   $win create text 4m 12m -tags t4
   $win create text 12m 12m -tags t5
   $win create text 20m 12m -tags t6

   $win create oval 1m 17m 7m 23m -outline black -fill $rInfo(bg)
   $win create oval 9m 17m 15m 23m -outline black -fill $rInfo(bg)
   $win create oval 17m 17m 23m 23m -outline black -fill $rInfo(bg)
   $win create text 4m 20m -tags t7
   $win create text 12m 20m -tags t8
   $win create text 20m 20m -tags t9

   if {$dir == "h"} {
      $win create line  4m   4m  20m   4m -tags perm
      $win create line  4m  12m  20m  12m -tags perm
      $win create line  4m  20m  20m  20m -tags perm
   } else {
      $win create line  4m  4m  4m  20m -tags perm
      $win create line  12m  4m  12m  20m -tags perm
      $win create line  20m  4m  20m  20m -tags perm
   }
   $win lower perm

}

proc saveCurrentRack {} {
   global rInfo env rackInfo
   set index $rInfo(rackIndex)
   set fd2 [open $env(vnmrsystem)/asm/info/currentRacks w]
   puts $fd2 "set rackInfo(num) 5"
   for {set i 1} {$i <= 5} {incr i} {
      if {$rInfo(rack,$i) != "none"} {
         set num $rackInfo($rInfo(rack,$i),zones)
         puts $fd2 "set rackInfo($i,zones) $num"
         set el $rInfo(rack,$i)
         set numbers ""
         for {set j 1} {$j <= $num} {incr j} {
            set tot [expr $rackInfo($el,$j,row) * $rackInfo($el,$j,col)]
            puts $fd2 "set rackInfo($i,$j,row) $rackInfo($el,$j,row)"
            puts $fd2 "set rackInfo($i,$j,col) $rackInfo($el,$j,col)"
            puts $fd2 "set rackInfo($i,$j,num) $tot"
            if {($tot == 96) && ($numbers == "")} {
               if {$rInfo(number) == 1} {
                  set numbers ""
               } else {
                  set numbers "gilsonNumber"
               }
            }
         }
         if {$i == $index} {
            set rackInfo($i,file) $numbers
            set rackInfo($i,start) $rInfo(start)
            set rackInfo($i,pattern) $rInfo(pattern)
         }
         puts $fd2 "set rackInfo($i,file) \"$rackInfo($i,file)\""
         puts $fd2 "set rackInfo($i,start) $rackInfo($i,start)"
         puts $fd2 "set rackInfo($i,pattern) $rackInfo($i,pattern)"
      } else {
         puts $fd2 "set rackInfo($i,zones) 0"
         puts $fd2 "set rackInfo($i,file) \"\""
         puts $fd2 "set rackInfo($i,start) 1"
         puts $fd2 "set rackInfo($i,pattern) 1"
      }
   }
   close $fd2
   catch {[exec chmod 666 $env(vnmrsystem)/asm/info/currentRacks]}
}

proc returnCurrentRack {index} {
   global rInfo env
   if [catch {source $env(vnmrsystem)/asm/info/currentRacks} result] {
   } else {
      set rInfo(start) $rackInfo($index,start)
      numberCircles
      set rInfo(number) 0
      if {$rackInfo($index,file) == ""} {
         set rInfo(number) 1
      } elseif {$rackInfo($index,file) == "gilsonNumber"} {
         set rInfo(number) 2
      }
      selectPattern $rackInfo($index,pattern)
   }
}

proc selectRack {index} {
   global rInfo env rackInfo
   if {$rInfo(rackIndex) != 0} {
      .rframe$rInfo(rackIndex) configure -relief raised
   }
   set rInfo(rackIndex) $index
   .rframe$rInfo(rackIndex) configure -relief sunken
   source $env(vnmrsystem)/asm/racks/rackInfo
   set rInfo($index,label) $rackInfo($rInfo(rack,$index),label)
   returnCurrentRack $index
   set fd [open $env(vnmrsystem)/asm/info/racks w]
   for {set i 1} {$i <= 5} {incr i} {
      if {$rInfo(rack,$i) != "none"} {
         puts $fd "gRackLocTypeMap $i $rInfo(rack,$i)"
      }
   }
   close $fd
   catch {[exec chmod 666 $env(vnmrsystem)/asm/info/racks]}
   saveCurrentRack
}

proc drawRacks {win} {
   global env rInfo
   source $env(vnmrsystem)/asm/racks/rackInfo
   foreach el $rackInfo(types) {
      set els {}
      for {set i 1} {$i <= $rackInfo($el,zones)} {incr i} {
         set tot [expr $rackInfo($el,$i,row) * $rackInfo($el,$i,row)]
         if {[lsearch $els tol] == -1} {
            lappend els $tot
         }
      }
      set rInfo($el,numZones) [llength $els]
      
   }
   for {set i 1} {$i <= 5} {incr i} {
      set rInfo(rack,$i) none 
   }
   set fd [file exists $env(vnmrsystem)/asm/info/racks]
   if {$fd == 1} {
      set fd [open $env(vnmrsystem)/asm/info/racks r]
      while {[gets $fd Line] >= 0} {
         if {[lindex $Line 0] == "gRackLocTypeMap"} {
            set rInfo(rack,[lindex $Line 1]) [lindex $Line 2]
            set rInfo([lindex $Line 1],label) $rackInfo([lindex $Line 2],label)
         }
      }
      close $fd
   }
   frame .rmenu
   for {set i 1} {$i <= 5} {incr i} {
      frame .rframe$i -relief raised -borderwidth 2
      label .rlab$i -text "Rack $i" -anchor w
      eval {tk_optionMenu .rk$i rInfo(rack,$i) none} $rackInfo(types)
      .rk$i configure -width 5
      bind .rk$i.menu <ButtonRelease-1> "+after 1 selectRack $i"
      pack .rlab$i .rk$i -in .rframe$i -side top -padx 5 -anchor w
      pack .rframe$i -in .rmenu -side left -padx 5 -fill y
      bind .rframe$i <Button-1> "selectRack $i"
      frame .rlabs$i
      frame .val$i
      label .rlab3$i -text "Zones: "
      label .rvalz0$i -textvariable rInfo($i,label) \
            -justify left -height 3 -anchor nw
      bind .rlab3$i <ButtonRelease-1> "+after 1 selectRack $i"
      bind .rvalz0$i <ButtonRelease-1> "+after 1 selectRack $i"
      pack .rlab3$i .rvalz0$i -in .rlabs$i -side top -anchor nw
#      pack .rvalz1$i .rvalz2$i .rvalz3$i -in .val$i -side top -anchor w
      pack .rlabs$i .val$i -in .rframe$i -side left -anchor nw
   }
   pack .rmenu -in $win -side top
   set rInfo(rackIndex) 0
}

proc rackPanel {} {
   global rInfo gilPanel
   frame .racks
   drawRacks .racks
   frame .info
   frame .start
   radiobutton .st1 -text "Left & Back" \
          -command "numberCircles; saveCurrentRack" \
          -variable rInfo(start) -value 1 -highlightthickness 0
   radiobutton .st2 -text "Right & Back" \
          -command "numberCircles; saveCurrentRack" \
          -variable rInfo(start) -value 2 -highlightthickness 0
   radiobutton .st3 -text "Left & Front" \
          -command "numberCircles; saveCurrentRack" \
          -variable rInfo(start) -value 3 -highlightthickness 0
   radiobutton .st4 -text "Right & Front" \
          -command "numberCircles; saveCurrentRack" \
          -variable rInfo(start) -value 4 -highlightthickness 0
   set rInfo(fg) [.st1 cget -foreground]
   set rInfo(bg) [.st1 cget -background]
   grid .st1 -row 0 -column 0 -in .start -sticky w
   grid .st2 -row 0 -column 1 -in .start -sticky w
   grid .st3 -row 1 -column 0 -in .start -sticky w
   grid .st4 -row 1 -column 1 -in .start -sticky w
   pack .start -in .info -side top -padx 10 -pady 10
   frame .pattern
   canvas .p1 -width 24m -height 24m -bd 3 -relief raised
   canvas .p2 -width 24m -height 24m -bd 3 -relief raised
   canvas .p3 -width 24m -height 24m -bd 3 -relief raised
   canvas .p4 -width 24m -height 24m -bd 3 -relief raised
   bind .p1 <Button-1> "selectPattern 1; saveCurrentRack"
   bind .p2 <Button-1> "selectPattern 2; saveCurrentRack"
   bind .p3 <Button-1> "selectPattern 3; saveCurrentRack"
   bind .p4 <Button-1> "selectPattern 4; saveCurrentRack"
   addcircles .p1 h
   addcircles .p2 h
   addcircles .p3 v
   addcircles .p4 v
   pack .p1 .p2 .p3 .p4 -in .pattern -side left -padx 5 -pady 5
   pack .pattern -in .info -side top
   frame .numb
   label .nb -text "Number locations with: "
   radiobutton .nb1 -text "Numerals (1-96)" \
          -command saveCurrentRack \
          -variable rInfo(number) -value 1 -highlightthickness 0
   radiobutton .nb2 -text "Names (A1-H12)" \
          -command saveCurrentRack \
          -variable rInfo(number) -value 2 -highlightthickness 0
   pack .nb .nb1 .nb2 -in .numb -side left
   pack .numb -in .info -side top
   if {$gilPanel == 2} {
      button .p2exit -text "    Exit   " -relief raised \
                    -highlightthickness 1 -command {quitGilalign}
      pack .p2exit -in .info -pady 10 -side top
   }

}

proc initRackInfo {} {
   global rackInfo env
   for {set i 1} {$i <= 5} {incr i} {
      set rackInfo($i,start) 1
      set rackInfo($i,file) ""
      set rackInfo($i,pattern) 1
   }
   set rackInfo(none,label) ""
   catch {source $env(vnmrsystem)/asm/info/currentRacks}
}

proc makeRackPanel {} {
   global rInfo
   rackPanel
   initRInfo
   initRackInfo
   numberCircles
}

proc makeExtraPanel {} {
   global valEntry labels samp2_list
   upvar #0 labelText labelText

   frame .fr1ab -bd 2 -relief groove
   frame .fr1ab.fl0
   frame .fr1ab.fl1
   frame .fr1ab.fl2
   grid  .fr1ab.fl0 -in .fr1ab -row 0 -column 0 -columnspan 4 -rowspan 1\
                   -sticky nw -pady 5
   grid  .fr1ab.fl1 -in .fr1ab -row 1 -column 0 -columnspan 4 -rowspan 2\
                   -sticky nw -pady 5
   grid  .fr1ab.fl2 -in .fr1ab -row 3 -column 0 -columnspan 4 -rowspan 2\
                   -sticky nw -pady 5
   frame .fr1ab.fr0
   grid  .fr1ab.fr0 -in .fr1ab -row 0 -column 4 -columnspan 4 -rowspan 3\
                   -sticky nw -pady 5
   for {set i 0} {$i < 8} {incr i} {
      grid columnconfigure .fr1ab $i -weight 1
   }
 
set samp2_list [list samp,2 samp,5 samp,6 samp,17 samp,18 \
               samp,7 samp,8 samp,9 samp,19]
   set col0  0
   set col1  1
   set num [llength $samp2_list]
   set side l
   set subs {0 1 1 2 2 0 0 0 0 }
   set rows {0 0 1 0 1 0 1 2 3 }
   for {set i 0} {$i < $num} {incr i} {
       if {$i > 4} {
          set side r
       }
       set j [lindex $samp2_list $i]
 
       set sub [lindex $subs $i]
       set row [lindex $rows $i]
       if {[info exists labels($j)] == 1} {
         label .fr1ab.l${i} -text "[format "%-18s"   $labelText($j):]"
         grid .fr1ab.l${i} -in .fr1ab.f${side}${sub} -row $row \
              -column $col0 -sticky w
       }
 
       entry .fr1ab.e${i} -highlightthickness 0 -width 10  -relief sunken \
                        -textvariable valEntry($j) -state normal
       grid .fr1ab.e${i} -in .fr1ab.f${side}${sub} -row $row \
              -column $col1 -padx 10 -pady 4 -sticky w
       bind .fr1ab.e${i} <Return> "isANumber $j"
       bind .fr1ab.e${i} <FocusOut> "isANumber $j"
   }
}

proc toggleExtra {} {
   global valEntry
   if {$valEntry(next,1) == 1} {
      if {[winfo exists .fr1ab] == 0} {
        makeExtraPanel
      }
      pack .fr1ab -in .fr1hold -fill both -expand 1
   } else {
      pack forget .fr1ab
   }
}

proc makePanel1 {} {
   global valEntry labels \
          samp_list pushVol samps
   upvar #0 labelText labelText

;#making frames for entries
   frame .fr1select
   frame .fr1a -bd 2 -relief groove
   frame .fr1a.fl0
   frame .fr1a.fl1
   frame .fr1a.fl2
   grid  .fr1a.fl0 -in .fr1a -row 1 -column 0 -columnspan 4 -rowspan 3\
                   -sticky nw -pady 5
   grid  .fr1a.fl1 -in .fr1a -row 4 -column 0 -columnspan 4 -rowspan 1\
                   -sticky nw -pady 5
   grid  .fr1a.fl2 -in .fr1a -row 5 -column 0 -columnspan 4 -rowspan 2\
                   -sticky nw -pady 5
   frame .fr1a.fr0
   frame .fr1a.fr1
   frame .fr1a.fr2
   grid  .fr1a.fr0 -in .fr1a -row 1 -column 4 -columnspan 4 -rowspan 3\
                   -sticky nw -pady 5
   grid  .fr1a.fr1 -in .fr1a -row 4 -column 4 -columnspan 4 -rowspan 2\
                   -sticky nw -pady 5
   grid  .fr1a.fr2 -in .fr1a -row 6 -column 4 -columnspan 4 -rowspan 2\
                   -sticky nw -pady 5
   for {set i 0} {$i < 8} {incr i} {
      grid columnconfigure .fr1a $i -weight 1
   }
 
   set col0  0
   set col1  1
   set num [llength $samp_list]
   set side l
   set subs {0 0 0 1 2 2 0 0 0 1 1 2}
   set rows {0 1 2 0 0 1 0 1 2 0 1 2}
   for {set i 0} {$i < $num} {incr i} {
       if {$i > 5} {
          set side r
       }
       set j [lindex $samp_list $i]
 
       set sub [lindex $subs $i]
       set row [lindex $rows $i]
       if {[info exists labels($j)] == 1} {
         label .fr1a.l${i} -text "[format "%-18s"   $labelText($j):]"
         grid .fr1a.l${i} -in .fr1a.f${side}${sub} -row $row \
              -column $col0 -sticky w
       }
 
       if {$j == "samp,4"} {
# this is the keep flag entry
          frame .fr1a.e${i} -width 10
          radiobutton .fr1a.e${i}.y -highlightthickness 0 -width 5 \
                        -indicatoron 0 -selectcolor pink \
                        -variable valEntry($j) -value 1 -text " yes "
          radiobutton .fr1a.e${i}.n -highlightthickness 0 -width 5 \
                        -indicatoron 0 -selectcolor pink \
                        -variable valEntry($j) -value 0 -text " no  "
          grid .fr1a.e${i}.y -in .fr1a.e${i} -row 0 -column 0 -sticky w
          grid .fr1a.e${i}.n -in .fr1a.e${i} -row 0 -column 1 -sticky w
       } elseif {$j == "next,1"} {
          checkbutton .fr1a.e${i} -highlightthickness 0 \
                        -indicatoron 0 -selectcolor pink \
                        -variable valEntry($j) -command toggleExtra \
                        -text "Additional Parameters ..."
          grid .fr1a.e${i} -in .fr1a.f${side}${sub} -row $row \
              -column $col0 -pady 4 -sticky w
       } elseif {[info exists valEntry($j)] == 1} {
          entry .fr1a.e${i} -highlightthickness 0 -width 10  -relief sunken \
                        -textvariable valEntry($j) -state normal
       } else {
# this is the push volume entry
          entry .fr1a.e${i} -highlightthickness 0 -width 10  -relief flat \
                        -selectbackground [.fr1a.l${i} cget -background] \
                        -textvariable pushVol -state disabled
       }
       if {$j != "samp,4"} {
          bind .fr1a.e${i} <Return> "isANumber $j"
          bind .fr1a.e${i} <FocusOut> "isANumber $j"
       }
       if {$j != "next,1"} {
          grid .fr1a.e${i} -in .fr1a.f${side}${sub} -row $row \
              -column $col1 -padx 10 -pady 4 -sticky w
       }
   }
   frame .fr1b
#   frame .fr1b.sampdef
   frame .fr1b.sampdef2
   label .rsamp -text "Select parameters " -foreground darkblue
   menubutton .readsamp -text "configuration" -indicatoron 1 \
            -menu .readsamp.menu \
            -relief raised -bd 2 -highlightthickness 2 -anchor c
   menu .readsamp.menu -tearoff 0
   for {set i 0} {$i < $samps(num)} {incr i} {
      .readsamp.menu add radiobutton -variable samps(current) -value $i
   }
   .readsamp.menu add cascade -label "Save to " -menu .readsamp.menu.copy
   menu .readsamp.menu.copy -tearoff 0
   for {set i 0} {$i < $samps(num)} {incr i} {
      .readsamp.menu.copy add command -label $samps($i) \
           -command "copySampInfo $i"
   }
   setSampList .readsamp.menu 1
   bind .readsamp <Double-1> {+after 1 setSampLabel}
   bind .readsamp.menu <ButtonRelease-1> {+after 1 readSampInfo}
   pack .rsamp .readsamp -in .fr1select -side left -pady 10

   entry .rsamp5 -textvariable samps(newLabel) \
                 -highlightthickness 0 -width 20  -relief sunken
   bind .rsamp5 <Return> "newLabel"
   bind .rsamp5 <FocusOut> "newLabel"
   label .rsamp6 -text " as label for "
   button .readsampbut -text "Save Now" -command {writeSampInfo}
   pack .readsampbut -padx 5m -in .fr1b.sampdef2 -side left
#   pack .fr1b.sampdef .fr1b.sampdef2 -in .fr1b -side top -anchor w
   pack .fr1b.sampdef2 -in .fr1b -side top -anchor w
   frame .fr1hold
   frame .fr1bb -height 20
   pack .fr1bb -in .fr1hold -side bottom
   pack .fr1select .fr1a .fr1hold .fr1b -in .fr1 -side top -fill both -expand 1
   return 0
}

proc calSelect {id index} {
   global cal_select calList

   set cal_select [lindex $calList $index]
   dsplCoordinates $cal_select
   .fr5.calsave configure -state normal
   return 0
}

proc dsplCalPanel {category} {
   global x_rinse y_rinse z_rinse  x_inject y_inject z_inject \
          x_rack y_rack z_rack rack1 cal_select cur_dspl calList

   if {$cur_dspl == 6} {return 1}
   removeCurrentDisplay
   pack .fr5 -in .mn -fill both -expand 1
   catch {.fr8.status configure -text ""} 

  if {[winfo exists .fr5.dum1] == 0} {
   frame .fr5.dum1 -width 330 -height 28 -relief flat
   grid .fr5.dum1 -row 1 -column 0 -sticky w
   
   label .fr5.slabel -text "Cal. Selections:"
   grid .fr5.slabel -row 1 -column 1 -sticky w

   menu2 .fr5.select $calList calSelect 1 \
       -width 10 -anchor w
   grid .fr5.select -row 1 -column 2  -sticky e

   frame .fr5.dum2 -relief sunken -height 10 ;# -bg blue
   grid .fr5.dum2 -row 2 -column 0 -in .fr5 -sticky w
   frame .fr5.coor
   grid .fr5.coor -row 3 -column 0 -columnspan 6 -sticky w

   label .fr5.coor.clabel -text ""
   label .fr5.coor.l_1x -text "    x1: "
   label .fr5.coor.l_5x -text "    x5: "
   label .fr5.coor.l_10x -text "   x10: "
   label .fr5.coor.l_50x -text "   x50: "
   label .fr5.coor.l_100x -text "  x100: "
   grid .fr5.coor.clabel -in .fr5.coor -row 3 -column 0 -sticky w
   grid .fr5.coor.l_1x -in .fr5.coor -row 3 -column 2 -sticky w
   grid .fr5.coor.l_5x -in .fr5.coor -row 3 -column 3 -sticky w
   grid .fr5.coor.l_10x -in .fr5.coor -row 3 -column 4 -sticky w
   grid .fr5.coor.l_50x -in .fr5.coor -row 3 -column 5 -sticky w
   grid .fr5.coor.l_100x -in .fr5.coor -row 3 -column 6 -sticky w
   label .fr5.coor.l_x -text " X Position : "
   label .fr5.coor.l_y -text " Y Position : "
   label .fr5.coor.l_z -text " Z Position : "
   grid .fr5.coor.l_x -in .fr5.coor -row 4 -column 0 -sticky w
   grid .fr5.coor.l_y -in .fr5.coor -row 5 -column 0 -sticky w
   grid .fr5.coor.l_z -in .fr5.coor -row 6 -column 0 -sticky w
   entry .fr5.coor.e_x -highlightthickness 2 -width 8 -relief flat -state disabled
   entry .fr5.coor.e_y -highlightthickness 2 -width 8 -relief flat -state disabled
   entry .fr5.coor.e_z -highlightthickness 2 -width 8 -relief flat -state disabled
   grid .fr5.coor.e_x -in .fr5.coor -row 4 -column 1 -sticky w
   grid .fr5.coor.e_y -in .fr5.coor -row 5 -column 1 -sticky w
   grid .fr5.coor.e_z -in .fr5.coor -row 6 -column 1 -sticky w

   set cnt 2
   foreach j "1 5 10 50 100" {
      scrollbar .fr5.coor.s${j}_x -orient horizontal -width 20
      scrollbar .fr5.coor.s${j}_y -orient horizontal -width 20
      scrollbar .fr5.coor.s${j}_z -orient horizontal -width 20
      grid .fr5.coor.s${j}_x -in .fr5.coor -row 4 -column ${cnt} -sticky w -padx 8 -pady 4
      grid .fr5.coor.s${j}_y -in .fr5.coor -row 5 -column ${cnt} -sticky w -padx 8 -pady 4
      grid .fr5.coor.s${j}_z -in .fr5.coor -row 6 -column ${cnt} -sticky w -padx 8 -pady 4
      incr cnt
   } 

   button .fr5.calsave -text " Save " -relief raised -highlightthickness 1 \
                                      -command {saveCoorValues $cal_select}
   grid .fr5.calsave -in .fr5.coor -row 7 -column 6 -pady 4 -sticky se
   set rack1 "none"
  }
   set cal_select ""
   set x_rinse ""
   set y_rinse ""
   set z_rinse ""
   set x_inject ""
   set y_inject ""
   set z_inject ""
   set x_rack ""
   set y_rack ""
   set z_rack ""
   foreach j "1 5 10 50 100" {
      .fr5.coor.s${j}_x configure -command ""
      .fr5.coor.s${j}_y configure -command ""
      .fr5.coor.s${j}_z configure -command ""
   } 
   .fr5.select configure -text ""
   .fr5.coor.clabel configure -text "" 
   .fr5.calsave configure -state disabled
   set cur_dspl 6
   return 0
}
 
proc setSampList {win doSource} {
   global env samps
   if {$doSource == 1} {
     source $env(vnmrsystem)/asm/info/samps
   }
   for {set i 0} {$i < $samps(num)} {incr i} {
      $win entryconfigure $i -label $samps($i)
      $win.copy entryconfigure $i -label $samps($i)
   }
   set samps(curLabel) $samps($samps(current))
   catch {.readsamp configure -text $samps(curLabel)}
}

proc newLabel {} {
   global samps
   place forget .rsamp5
   set samps($samps(current)) $samps(newLabel)
   setSampList .readsamp.menu 0
}

proc setSampLabel {} {
   place .rsamp5 -in .readsamp -relheight 1 -relwidth 1 -x 0 -y 0
}

proc readSampInfo {} {
   global samps env valEntry gilDef labels
   source $env(vnmrsystem)/asm/info/samp$samps(current)
   set samps(curLabel) $samps($samps(current))
   set samps(newLabel) $samps($samps(current))
   set samps(new) $samps(current)
   for {set i 1} {$i <= $gilDef(samp,fields)} {incr i} {
      if {[info exists $labels(samp,$i)] == 1} {
         set valEntry(samp,$i) [set $labels(samp,$i)]
      }
   }
   catch {.readsamp configure -text $samps(curLabel)}
   isANumber samp,10
   catch {checkLabel}
   set cwd [pwd]
   cd $env(vnmrsystem)/asm/info
   catch {exec rm -f sampInfo}
   exec ln -s samp$samps(current) sampInfo
   cd $cwd
}

proc checkLabel {} {
   global samps
   set id $samps(new)
   for {set i 0} {$i < $samps(num)} {incr i} {
      if {($samps(newLabel) == $samps($i)) && ($i != $samps(new))} {
         set id $i
      }
   }
}

proc setSampSelection {} {
   global samps
   checkLabel
}

proc writeSampInfo {} {
   global valEntry labels gilDef env samps

   set fd [open $env(vnmrsystem)/asm/info/samp$samps(new) w]
   for {set i 0} {$i <= $gilDef(samp,fields)} {incr i} {
      if {[info exists valEntry(samp,$i)] == 1} {
         puts $fd "set $labels(samp,$i) \"$valEntry(samp,$i)\""
      }
   }
   close $fd
   catch {[exec chmod 666 $env(vnmrsystem)/asm/info/samp$samps(new)]}
   set samps($samps(new)) "$samps(newLabel)"
   set fd [open $env(vnmrsystem)/asm/info/samps w]
   puts $fd "set samps(num) $samps(num)"
   for {set i 0} {$i < $samps(num)} {incr i} {
      puts $fd "set samps($i) \{$samps($i)\}"
   }
   puts $fd "set samps(current) $samps(new)"
   close $fd
   catch {[exec chmod 666 $env(vnmrsystem)/asm/info/samps]}
   setSampList .readsamp.menu 1
}

proc copySampInfo {index} {
   global samps
   if {$index == $samps(new)} {
      return
   }
   set samps(new) $index
   set samps(newLabel) "$samps($index)"
   writeSampInfo
}

proc writeToFile {cat} {
  global GIL_INFO_FILE GIL_TEMP_FILE gilInfo gilDef current valEntry
 
   if ![file exists $GIL_INFO_FILE] {
      puts "$GIL_INFO_FILE not exist"
      return 1
   }
   set srcFD [open $GIL_INFO_FILE r]
   set tmpFD [open $GIL_TEMP_FILE w]
 
   while {[gets $srcFD Line] >= 0} {
      set el [lindex $Line 0]
      if {$el  != "$cat"} {
         puts $tmpFD $Line
      } else {
        switch  $el {
        "injector" {
                       set Line [lreplace $Line 3 5 $gilInfo(1,$cat,2) \
                                                    $gilInfo(1,$cat,3) \
                                                    $gilInfo(1,$cat,4)]
                       puts $tmpFD $Line
                       set cat ""
                   }
 
        "rack" {
                  set Line [lreplace $Line 3 4 $gilInfo(1,$cat,2) \
                                               $gilInfo(1,$cat,3)]
                  puts $tmpFD $Line
                  set cat ""
               }
        }
     }
   }
 
   close $srcFD
   close $tmpFD
 
   exec mv $GIL_TEMP_FILE $GIL_INFO_FILE
   catch {[exec chmod 666 $GIL_INFO_FILE]}
   return 0
}

proc saveCoorValues {cat} {
   global gil_prompt refresh gilInfo current

     if {$gil_prompt == "Cmd:"} {
        sendToGilalign "q"
     } else {
        catch {.fr8.status configure -text "Problem saving \"${cat}\""} 
        return -1
     }

     readValues 3
     if {[readPrompt "Cmd:"] == 0} {
        set gil_prompt "Cmds:" 
        sendToGilalign "n"
     } else {
          catch {.fr8.status configure -text "Problem1 saving \"${cat}\""}
          return -1
       } 

     switch $cat {
  
         "inject" { set tmp [readValues 3]
                    if {$tmp == -1} {
                       catch {.fr8.status configure -text "Problem2 saving \"${cat}\""}
                       return -1
                    } else {
                         set gilInfo(1,injector,2)  [lindex $tmp 0]
                         set gilInfo(1,injector,3)  [lindex $tmp 1]
                         set gilInfo(1,injector,4)  [lindex $tmp 2]
                      }
                    writeToFile injector
                  }

           "rack" { set tmp [readValues 2] 
                    if {$tmp == -1} {
                       catch {.fr8.status configure -text "Problem3 saving \"${cat}\""}
                       return -1
                    } else {
                         set gilInfo(1,$cat,2)  [lindex "$tmp" 0]
                         set gilInfo(1,$cat,3)  [lindex "$tmp" 1]
                      }
                    writeToFile $cat
                  }

          "rinse" {}
     }
     
     if {[readPrompt "Cmd:"] == 0} {
        set gil_prompt "Cmd:"
     } else {
          catch {.fr8.status configure -text "Problem saving \"${cat}\""}
          return -1
       }
   set refresh 1    ;#dsplCoordinates will check this variable
   dsplCoordinates $cat
   return 0
}

proc dsplReplaceSyringe {category} {
   global cur_dspl cur_coor_dspl syr_type syr_plunger z_height \
          gil_prompt plunger_loc valEntry showInjector
 
   if {$cur_dspl == 5} {return 1}
   if {$cur_dspl == 6} {quitCalibration}
   set cur_coor_dspl ""
   catch {.fr8.status configure -text ""} 
 
   removeCurrentDisplay
   pack .fr4 -in .mn -fill both -expand 1
 
  if {[winfo exists .fr4.air] == 0} {

   label .fr4.air -text "Air valve: "
   radiobutton .fr4.airon -text "  ON " -variable air_valve -value on \
     -bd 2 -highlightthickness 0 -indicatoron 0 \
     -selectcolor pink -command {setValve "v" "1"}
   radiobutton .fr4.airoff -text " OFF " -variable air_valve -value off \
     -bd 2 -highlightthickness 0 -indicatoron 0 \
     -selectcolor pink -command {setValve "v" "0"}

   label .fr4.lctubeless -text "  Injector: "
   radiobutton .fr4.lc -text " Inject " -variable lc_tubeless -value lc \
     -bd 2 -highlightthickness 0 -indicatoron 0 \
     -selectcolor pink -command {setValve "j" 1}
   radiobutton .fr4.tubeless -text "  Load  " \
     -variable lc_tubeless -value tubeless \
     -bd 2 -highlightthickness 0 -indicatoron 0 \
     -selectcolor pink -command {setValve "j" 0}
   label .fr4.plun -text "  Plunger: "
   radiobutton .fr4.plunup -text "  UP  " -variable syr_plunger -value up\
        -bd 2 -highlightthickness 0 -indicatoron 0 \
        -selectcolor pink -command {moveSyrPlunger}
   radiobutton .fr4.plundown -text " DOWN " -variable syr_plunger -value down\
        -bd 2 -highlightthickness 0 -indicatoron 0 \
        -selectcolor pink -command {moveSyrPlunger}

   grid .fr4.air   -row 3 -column 0 -sticky w
   grid .fr4.airon -row 3 -column 1 -sticky w
   grid .fr4.airoff -row 4 -column 1 -sticky w 
   grid .fr4.plun   -row 3 -column 2 -sticky w
   grid .fr4.plunup -row 3 -column 3 -sticky w
   grid .fr4.plundown -row 4 -column 3 -sticky w
   if {$showInjector == 1} {
     grid .fr4.lctubeless -row 3 -column 4 -sticky w
     grid .fr4.lc       -row 3 -column 5 -sticky w
     grid .fr4.tubeless -row 4 -column 5 -sticky w 
   }
   frame .fr4.butts
   button .resetarm -text "Reset arm " -relief raised \
                        -highlightthickness 1 -command { resetArm }
   button .primepump -text "Prime pump" -relief raised \
                         -highlightthickness 1 -command { primePump }
   button .calibrate -text "Return Home" -relief raised \
                         -highlightthickness 1 -command {returnHome}
#   grid .fr4.resetarm   -row 5 -column 0 -padx 10 -pady 10 -sticky w
#   grid .fr4.primepump  -row 5 -column 1 -padx 10 -pady 10 -sticky w
#   grid .fr4.calibrate  -row 5 -column 2 -padx 10 -pady 10 -sticky w
   pack .resetarm .primepump .calibrate -in .fr4.butts -side left \
         -pady 10
   grid .fr4.butts  -row 5 -column 0 -columnspan 4 -sticky w

   label .fr4.lsyrvol -text "Syringe Volume (ul): " ;#-bg blue
   set syrMenu [tk_optionMenu .fr4.esyrvol valEntry(syr_vol) \
                100 250 500 1000 5000 10000 25000]
   bind $syrMenu <ButtonRelease-1> {+after 1 saveSyrVolume}
   grid .fr4.lsyrvol -in .fr4 -row 6 -column 0 -sticky w
   grid .fr4.esyrvol -in .fr4 -row 6 -column 1 -padx 1 -pady 15 -sticky w

   label .fr4.zlabel -text "Arm Z scale (mm): " ;#-bg blue
   grid .fr4.zlabel -row 7 -column 1 -sticky w
   set zhtMenu [tk_optionMenu .fr4.zht z_height \
                120 121 122 123 124 125 126 127 128 129 130]
   bind $zhtMenu <ButtonRelease-1> {+after 1 setInstall}
   grid .fr4.zlabel -in .fr4 -row 7 -column 0 -sticky w
   grid .fr4.zht -in .fr4 -row 7 -column 1 -padx 1 -pady 15 -sticky w

#   scale .fr4.s.zheight -from 5 -to 220 -length 8c -orient horizontal \
                      -variable z_height -highlightthickness 0
#   grid  .fr4.s.zheight -row 7 -column 2 -sticky w                
#   button .fr4.s.savedef -text " Save " -command setInstall
#   grid .fr4.s.savedef -row 7 -column 7 -padx 100 -pady 6 -sticky e


   frame .fr4.dum11 -height 15 ;#-highlightthickness 4
   grid .fr4.dum11 -row 8 -columnspan 9 -sticky news
  }
   set cur_dspl 5

   sendToGilalign "n"
   set tmp [readValues 2]
   if {$tmp == -1} {
      catch {.fr8.status configure -text "Problem1 getting syringe volume"}
      return -1
   } else {
      set valEntry(syr_vol) [lindex $tmp 1]
   }

   if {[readPrompt "Cmd:"] == 0} {
      set gil_prompt "Cmd:"
      sendToGilalign "q"
   } else {
        catch {.fr8.status configure -text "Problem getting syringe volume"}
        return -1
     }  

   if {[readPrompt "Cmds:"] == 0} {
      set gil_prompt "Cmds:"
   } else {
        catch {.fr8.status configure -text "Problem getting syringe volume"}
        return -1
     }  

   #assume that gilalign sets the plunger to up position
   set syr_plunger "up"
   set plunger_loc "up"

   return 0
}

proc setInstall {} {
   global gil_prompt z_height valEntry

    #commented out for the release, Dan or Greg will decide on this matter later
    #sendToGilalign "a"
    #sendToGilalign "[expr $z_height*10]"

    #if {[readPrompt "Cmds:"] == 0} {
    #   set gil_prompt "Cmds:"
    #} else {
    #     catch {.fr8.status configure -text "Problem setting  Zscale"}
    #     return -1
    #  }
    return 0
}

proc saveSyrVolume {} {
   global max_flowrate valEntry

   sendToGilalign "n"

      set tmp [readValues 2]
      if {$tmp == -1} {
         catch {.fr8.status configure -text "Problem1 changing syringe"}
         return -1
      } else {
         set syr_type [lindex $tmp 0]
      }

   if {[readPrompt "Cmd:"] == 0} {
      set gil_prompt "Cmd:"
   } else {
        catch {.fr8.status configure -text "Problem changing syringe"}
        return -1
     }   

   sendToGilalign "n $syr_type $valEntry(syr_vol)"
      set tmp [readValues 2]
      if {$tmp == -1} {
         catch {.fr8.status configure -text "Problem1 changing syringe"}
         return -1
      } else {
         set valEntry(syr_vol) [lindex $tmp 0]
         set max_flowrate [lindex $tmp 1]
      }

   if {[readPrompt "Cmd:"] == 0} {
      set gil_prompt "Cmds:"
      sendToGilalign "q"   ;# quit
   } else {
        catch {.fr8.status configure -text "Problem2 changing syringe"}
        return -1
     }

   if {[readPrompt "Cmds:"] == 0} {
      set gil_prompt "Cmds:"
   } else {
        catch {.fr8.status configure -text "Problem changing syringe"}
        return -1
     }   

   isANumber samp,10
   return 0 
}

proc moveSyrPlunger {} {
   global gil_prompt syr_type syr_plunger plunger_loc valEntry

   if {$plunger_loc == $syr_plunger} {return 1}

   sendToGilalign "n"

      set tmp [readValues 2]
      if {$tmp == -1} {
         catch {.fr8.status configure -text "Problem1 moving plunger"}
         return -1
      } else {
         set valEntry(syr_vol) [lindex $tmp 1]
      }

   if {[readPrompt "Cmd:"] == 0} {
      set gil_prompt "Cmd:"
   } else {
        catch {.fr8.status configure -text "Problem moving plunger"}
        return -1
     }   

   sendToGilalign [string index "$syr_plunger" 0] ;# send either "u" or "d" 
   after 3000

   if {[readPrompt "Cmd:"] == 0} {
      set gil_prompt "Cmds:"
      sendToGilalign "q"   ;# quit
   } else {
        catch {.fr8.status configure -text "Problem2 moving plunger"}
        return -1
     }

   if {[readPrompt "Cmds:"] == 0} {
      set gil_prompt "Cmds:"
   } else {
        catch {.fr8.status configure -text "Problem moving plunger"}
        return -1
     }   

   set plunger_loc $syr_plunger
   return 0 
}

proc rackSelect {id index} {
   global gil_prompt current gilInfo gilOK
   
    set i $index
    incr i
    set current(rack,num) $i
    set current(rack) $gilInfo($i,rack,0)
    set rack_path $gilInfo($i,rack,1)
    if {$gilOK == 1} {
      sendToGilalign "d"           ;#prepare to send rack's  full pathname.
      sendToGilalign "$rack_path"
      if {[readPrompt "Cmds:"] == 0} {
        set $gil_prompt "Cmds:"
      } else {
        catch {.fr8.status configure -text "Problem setting rack"}
        return -1
      }
    }
    return 0
}

proc dsplMainWindow {} {
   global cur_dspl gilOK gilPanel
   
  frame .fr0
  frame .mn -relief raised
  frame .fr1 -relief raised
  frame .fr3 -relief raised
  frame .fr8 -highlightthickness 0
  makePanel1
  makeRackPanel
  if {$gilOK == 1} {
   frame .fr4 -relief raised ;#-highlightthickness 1
   frame .fr5 -relief raised ;#-highlightthickness 1

   deck .fr0.samp 1 -text "SAMPLE Def."  -highlightthickness 0 \
        -command {selectdeck .fr0.samp {dsplSampleDef}}
   deck .fr0.rack 1 -text "Rack Def."  -highlightthickness 0 \
        -command {selectdeck .fr0.rack {dsplRackDef}}
   deck .fr0.syringe 1 -text "Main Control"  -highlightthickness 0 \
        -command {selectdeck .fr0.syringe {dsplReplaceSyringe syringe}}
   deck .fr0.calibrate 1 -text "Calibrations"  -highlightthickness 0 \
        -command {selectdeck .fr0.calibrate {dsplCalPanel calibrate}}
   pack .fr0.samp .fr0.rack .fr0.syringe .fr0.calibrate -in .fr0 -side left -anchor w
  }

  label .fr8.status -text "   " -fg indianred
  pack .fr8.status  -in .fr8 -side left -pady 15 -anchor nw

  button .fr8.exit -text "    Exit   " -relief raised \
                    -highlightthickness 1 -command {quitGilalign}
#  pack .fr8.exit -in .fr8 -side right -anchor ne -padx 40 -pady 4

  pack .fr1 -in .mn -fill both -expand 1
  pack .fr0 -side top -fill x -anchor w
  pack .mn -side top -fill both -anchor w -expand 1
  pack .fr8 -side top -fill x -anchor w

  if {$gilOK == 1} {
    .fr0.samp invoke
  } elseif {$gilPanel == 1} {
    after idle "dsplSampleDef"
  } elseif {$gilPanel == 2} {
    set cur_dspl 2
    after idle "dsplRackDef"
  }
  return 0
}

proc initGilalign {} {
   global num syr_plunger gil_prompt \
          x_min x_max y_min y_max z_min z_max max_flowrate \
          air_valve lc_tubeless z_height valEntry

     set valEntry(syr_vol)  500
     set z_height 121
     set tmp0 [readValues 14]
     if {$tmp0 == -1} {
        catch {.fr8.status configure -text "Problem initializing Gilson"}
        return -1
      } else {
           set x_min        [lindex $tmp0 0]
           set x_max        [lindex $tmp0 1]
           set y_min        [lindex $tmp0 2]
           set y_max        [lindex $tmp0 3]
           set z_min        [lindex $tmp0 4]
           set z_max        [lindex $tmp0 5]
           set valEntry(syr_vol)   [lindex $tmp0 6]
           set max_flowrate [lindex $tmp0 7]
           set valveAir	    [lindex $tmp0 8]
           set valveEject   [lindex $tmp0 9]
           set current_x    [lindex $tmp0 10]
           set current_y    [lindex $tmp0 11]
           set current_z    [lindex $tmp0 12]
           set current_vol  [lindex $tmp0 13]
       }
 
     if {[readPrompt "Cmds:"] == 0} {
        set gil_prompt "Cmds:"
     } else {
          catch {.fr8.status configure -text "Problem initializing Gilson"}
          return -1
      }
 
     set syr_plunger ""
     set z_height [expr $z_max / 10 ]
     if {$valveAir == 0} {
        set air_valve off
     } else {
        set air_valve on
     }
     if {$valveEject == "I"} {
        set lc_tubeless lc
     } else {
        set lc_tubeless tubeless
     }
     setCenters
     return 0
}

getGilInfos
source $env(vnmrsystem)/asm/info/samps
readSampInfo
dsplMainWindow
if {$gilOK == 1} {
  update idletasks
  connectToGilalign
  initGilalign
  isANumber samp,10
}
readSampInfo
