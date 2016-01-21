#!/vnmr/tcl/bin/vnmrwish -f
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

# init_balloons
# source  $env(vnmrsystem)/tcl/tklibrary/vnmr/accnt_helpTip

set ACC_DIR         $env(VNMR_ADM_DIR)/accounting   ;# update and save
set ACC_TMP_DIR     $env(VNMR_ADM_DIR)/tmp          ;# for viewing
set ACC_BIN_DIR     $env(VNMR_ADM_DIR)/bin

set GROUP_FILE      $ACC_DIR/group
set TEMP_FILE       $ACC_DIR/temp
set ACC_FILE        $ACC_TMP_DIR/Nmr.report
set ACC_FILE_TOTAL  $ACC_TMP_DIR/Nmr.report.total
set MULTI_FILE	    $ACC_DIR/multirate
set HOL_FILE 	    $ACC_DIR/holidays

set host_name [exec hostname]

set  group_name " "
set  group_id " "
set  members_list " "
set  hour_cost(1) 0.00
set  cost_day(1,Sun) 0
set  start_hour(1) "0:00"
set  time_array(0) 0
set  rate_array(0) 0
set  Printer " "
set  lpstring "lp -c"
set  proptions ""
set  calc_totals 0


global  members_saved

if { ! [file exists $ACC_TMP_DIR/Nmr.report] } {
   exec touch $ACC_TMP_DIR/Nmr.report
}

if { ! [file exists $ACC_FILE_TOTAL] } {
   set calc_totals 1
}

if { [file exists $HOL_FILE] } {
  set h_file [open $HOL_FILE r]
  while {[gets $h_file line] >= 0} {
    lappend hols $line
  } 
} else {
  set hols " "
}
#if { $h_file != ""} { close $h_file}

exec sh -c $ACC_BIN_DIR/view_acctng

;###############
;# positionWindow - 
;# w: the name of the window to position.
;#
;###########################################################################
proc positionWindow w {
    wm geometry $w +400+200
}


proc saveFile {w  File} {
        set f [open "$File" w]
        puts $f [$w  get 1.0 end]
        close $f
}

proc selectPrinter {} {
  global Printer lpstring lpstring1 proptions help_tips helpTip
  
  set Printer1 $Printer
  set proptions1 $proptions
  set lpstring1 $lpstring
  
  toplevel .prin
  wm title .prin "Printer Selection"
  frame .prin.top
  frame .prin.mid
  frame .prin.bot
  frame .prin.buttons
  label .prin.top.prlabel -text "Printer "
  entry .prin.top.prent -width 20 -relief sunken -bd 2 -textvariable Printer1
  pack  .prin.top.prlabel .prin.top.prent -side left -anchor w -padx 2m
  label .prin.mid.oplabel -text "Options "
  entry .prin.mid.opent -width 20 -relief sunken -bd 2 -textvariable proptions1
  pack  .prin.mid.oplabel .prin.mid.opent -side left -anchor w -padx 2m
  label .prin.bot.stlabel -text "Print string will be "
  label .prin.bot.stl1 -text $lpstring1 -fg blue
  label .prin.bot.stl2 -text " <file>" -fg blue
  pack  .prin.bot.stlabel .prin.bot.stl1 .prin.bot.stl2 -side left -anchor w
  
  bind .prin <Return> {
       set lpstring1 "lp -c $proptions1 -d $Printer1"
       .prin.bot.stl1 configure -text $lpstring1
  }
  
  button .prin.buttons.ok -text "Apply" -command {
    set Printer $Printer1
    set proptions $proptions1
    set lpstring $lpstring1
    destroy .prin
  }
  
  button .prin.buttons.can -text Cancel -command {destroy .prin}
  pack .prin.buttons.ok .prin.buttons.can -side left -padx 2m -pady 2m
  pack .prin.top .prin.mid .prin.bot .prin.buttons -side top

  set help_tips(.prin.top.prlabel) {$helpTip(Printer)}
  set help_tips(.prin.mid.oplabel) {$helpTip(Options)}
  set help_tips(.prin.bot.stlabel) {$helpTip(Print_string_will_be)}
  set help_tips(.prin.buttons.ok)  {$helpTip(Apply)}
  set help_tips(.prin.buttons.can) {$helpTip(Cancel)}
  
}
  
;###############
;# are_you_root -
;#
;###########################################################################
proc are_you_root {} {
    
    set temp [ lindex [ split [exec id] () ] 1 ]
    if { $temp == "root" } {
       return 1
    }
    return 0
}


;###############
;# update_and_save -
;#
;###########################################################################
proc update_and_save {} {
   global ACC_DIR ACC_BIN_DIR .buttons.doit
   set fname wtmpx
   if {[file exists /var/adm/wtmp] == 1 } {
      set fname wtmp
   }
   set k [dialog .d {Update and Save}  " This option deletes the current accounting \
data file (/var/adm/$fname) and replaces it with a new file with each user set to zero hours .
 This is usually done to start a new accounting cycle .
 The $fname.mm.dd.yy file is the saved accounting data file

 Do you want to continue ? " {} -1 OK Cancel]
   if {$k == 0} {
      exec sh -c $ACC_BIN_DIR/update_acctng
      .buttons.doit configure -state disabled
   }
}


;###############
;# printMainWindow -
;#
;###########################################################################
proc printMainWindow {} {
    global GROUP_FILE ACC_FILE_TOTAL TEMP_FILE host_name
    global lpstring
 
   set f [open  $GROUP_FILE  r]
   set TEMP [open $TEMP_FILE w]
      puts $TEMP ""
      puts $TEMP "                      VNMR Console ( Accounted Groups )"
      puts $TEMP "                           Host: $host_name"
      puts $TEMP ""
      puts $TEMP ""
      puts $TEMP ""
      puts $TEMP "         Group      Time(Hours)                    $"

   while {[gets $f Line] >= 0} {

         set group [format "%15s" [lindex $Line 0 ]]
         set members_only [lrange $Line 2 end]
         set group_time 0
         set group_charge 0
         set member_time 0

         foreach i [string trim $members_only] {
            set f1 [open  $ACC_FILE_TOTAL  r]

            while { [gets $f1 one_line ] >= 0 } {
                if { [string match [lindex $one_line  4] $i ]} {
                   set member_time [lindex $one_line 6]
                   catch {set group_time [expr $group_time + $member_time]}
                   set member_charge [lindex $one_line 9]
                   set group_charge [expr $group_charge + $member_charge]

                }
            } 
            close $f1
         }

         append group  [format "%16.2f    "  [expr $group_time / 60.0] ]
         append group [format " %19.2f"  $group_charge]

         puts $TEMP $group
         set members_only ""
         set grp_rate 0
         set group_charge 0
   }
   close $f
   close $TEMP
   after 500
   eval exec $lpstring $TEMP_FILE
   exec rm $TEMP_FILE
   return 0
}


;###############
;# printAllGroup -
;#
;###########################################################################
proc printAllGroup {} {
    global GROUP_FILE
 
   set f [open  $GROUP_FILE  r]
   while {[gets $f Line] >= 0} {
         set group [string trim [lindex $Line 0 ]]
         printGroup $group
   }
   close $f
   return 0
}   


;###############
;# printGroup -
;#
;###########################################################################
proc printGroup group_p {
    global GROUP_FILE ACC_FILE_TOTAL TEMP_FILE host_name
    global lpstring
      set group_and_members [exec grep $group_p $GROUP_FILE]
      set members_only [lreplace $group_and_members 0 1] ;#Take out group and $$ columns

      set TEMP [open $TEMP_FILE w]
      puts $TEMP ""
      puts $TEMP "                      VNMR Accounting Group ( $group_p )"
      puts $TEMP "                           Host: $host_name"
      puts $TEMP ""
      puts $TEMP ""
      puts $TEMP ""
      puts $TEMP "         Member      Time(Hours)                  $"

      set n 1
      foreach i $members_only {
          set member_info  0
          set member [format "%15s" $i]

            set f1 [open  $ACC_FILE_TOTAL  r]
            while { [gets $f1 one_line ] >= 0 } {
                if { [string match [lindex $one_line  4] $i ]} {
                   set member_info  $one_line
                }
            }    
            close $f1


          if { $member_info == 0 } {
              set member_charge  0
              set member_time  0
          } else {
 
               set member_time [lindex $member_info 6]
               set member_charge [lindex $member_info 9]
          }

         append member  [format "%16.2f    "  [expr $member_time / 60.0] ]
         append member [format "%19.2f"  $member_charge]

         puts $TEMP $member
         incr n
      }
   close $TEMP
 
   eval exec $lpstring $TEMP_FILE
   after 500 
   exec rm $TEMP_FILE
   return 0
}

proc multirate_line {w fi} {

upvar #0 cost_day cost_day
upvar #0 start_hour start_hour
upvar #0 hour_cost hour_cost

set fil $w.l$fi
 frame $fil

   checkbutton $fil.sun -text Su -variable cost_day($fi,Sun)
   checkbutton $fil.mon -text Mo -variable cost_day($fi,Mon)
   checkbutton $fil.tue -text Tu -variable cost_day($fi,Tue)
   checkbutton $fil.wed -text We -variable cost_day($fi,Wed)
   checkbutton $fil.thu -text Th -variable cost_day($fi,Thu)
   checkbutton $fil.fri -text Fr -variable cost_day($fi,Fri)
   checkbutton $fil.sat -text Sa -variable cost_day($fi,Sat)
   checkbutton $fil.ho -text Holiday -variable cost_day($fi,ho)
   

   entry $fil.sthr -relief sunken -width 11 -textvariable start_hour($fi)
   entry $fil.entry -relief sunken -width 10 -textvariable hour_cost($fi)
   pack $fil.sun $fil.mon $fil.tue $fil.wed $fil.thu \
        $fil.fri $fil.sat $fil.ho $fil.sthr \
        $fil.entry -side left
        
   pack $fil -side top -anchor w
   
}

proc get_multi_rate {w} {
   global ctr wind multi_flag
   
   set multi_flag -1
   set ctr 1
   set wind $w.f2
   $w.buttons.simple configure -state normal
   $w.buttons.multi configure -state disabled
   catch {destroy $w.f2}
      frame $w.f2 -bd 2
   label $w.f2.label -text "		Days				    Start time  $ per hour"
   button $w.f2.newline -text "Add line" -command {incr ctr ; \
                        multirate_line $wind $ctr}
   pack $w.f2.newline -side top
   pack $w.f2.label -side top -anchor w
   
   multirate_line $w.f2 $ctr
  
   pack $w.msg $w.f2 -side top -fill x
   
} 

proc get_single_rate {w} {

upvar #0 cost_day cost_day
upvar #0 start_hour start_hour
upvar #0 hour_cost hour_cost
global multi_flag

   catch {destroy $w.f2}
   set multi_flag 1  ;#this is a single rate group
   frame $w.f2 -bd 2
   entry $w.f2.entry -relief sunken -width 40 -textvariable hour_cost(1)
   label $w.f2.label
   pack $w.f2.entry -side right
   pack $w.f2.label -side right
   $w.f2.label config -text   "Amount per hour:"
   pack $w.msg $w.f2 -side top -fill x
   $w.buttons.simple configure -state disabled
   $w.buttons.multi configure -state normal
}

;###############
;# addGroup -
;#
;###########################################################################
proc addGroup {} {
   global group_name members_list font host_name

   # the next line is ok for now, next have to make "font" global
   #set font terminal ;#-*-Helvetica-Medium-R-Normal--*-140-*-*-*-*-*-*
   set w .form
   catch {destroy $w}
   toplevel $w
   positionWindow $w

   set main_title "VNMR:  Add Accounting Group to "
   append main_title $host_name
   wm title $w $main_title
   wm iconname $w "Add_Group"

   label $w.msg -wraplength 4i -justify left -text "Enter name of new accounting group
and the rate(s)"
pack $w.msg -side top

   frame $w.buttons
   pack  $w.buttons -side bottom -expand y -fill x -pady 2m
   button $w.buttons.dismiss -text Done -command "destroy $w"
   button $w.buttons.code -text "Apply" -command "applyAddGroup"


   frame $w.f1 -bd 2
   entry $w.f1.entry -relief sunken -width 40 -textvariable group_name
   label $w.f1.label
   pack $w.f1.entry -side right
   pack $w.f1.label -side right
   $w.f1.label config -text   "     Group name:"
   button $w.buttons.simple -text "Single rate" -command "get_single_rate $w"
   
   button $w.buttons.multi -text "Multi rate" -command "get_multi_rate $w"
   
   pack $w.buttons.simple $w.buttons.multi \
        $w.buttons.code $w.buttons.dismiss -side left -expand 1
        
   pack $w.msg $w.f1  -side top -fill x

   focus $w.f1.entry
}

;###############
;# applyAddGroup -
;#  called by apply button of addGroup widget
;#
;###########################################################################
proc applyAddGroup {} {
   upvar #0 cost_day cost_day
   upvar #0 start_hour start_hour
   upvar #0 hour_cost hour_cost
   global group_name GROUP_FILE MULTI_FILE multi_flag ctr
      
   set grp_name [string trim $group_name]     ;# new group name

   if {$grp_name  == "" } {
      dialog .d {Add User} {Missing  group  name} {} -1 OK
      return -1
   }
   set f [open  $GROUP_FILE  r]
   set group [lsort [read $f 1000]]           ;# read from group file
   close $f
   
   foreach j $grp_name {
      set matched 0
      foreach i $group {
         if {[string match $j: $i]} {
            dialog .d {Group Exist}  "\"$j\" group already exists." {} -1 OK
            set matched 1
         }
      }
      if {$matched == 0} {
         set k [dialog .d {Add Group}  " ADD \"$j\"  group ? " {} -1 OK Cancel]
         if {$k == 0} {
            set a $j:
            lappend a [format "%.2f" [expr ($multi_flag < 0) ? -1.0 : $hour_cost(1)]]
            exec echo $a: >> $GROUP_FILE

# a negative number flags multi rate

            if {$multi_flag <= 0} {
               set a $j
               lappend a $ctr
               lappend a [array get cost_day]
               lappend a [array get start_hour]
               lappend a [array get hour_cost]
               exec echo $a >> $MULTI_FILE
            }
            updateMainWindow
            return 0
         }
         return -1
      }
    }
   return 0
}

;###############
;#
;#  Accounting engine starts here
;#
;###############

proc minute {time} {
  set min 0
  set pt [string first ":" $time]
  set min [string range $time [expr $pt + 1] end]
  if {$min != 0} {
    set min [string trimleft $min "0"]
  }
  set hr [string range $time 0 [expr $pt - 1]]
  if {$hr == 0} {
    return $min
  } else {
    set min [expr $min + [string trimleft $hr "0"] * 60]
    return $min
  }
}

proc inc_day {current delta} {
  set listd {Sun Mon Tue Wed Thu Fri Sat}
  set point [lsearch $listd $current]
  set point [expr int(fmod($point + $delta,7))]
  if {$point < 0} {
    incr point 7
  }
  return [lindex $listd $point]
}

proc singlerate_charge {user rate} {
  global ACC_FILE user_time user_charge
  
  set t_file [open $ACC_FILE r]
  
  set user_charge 0
  
  while {[gets $t_file line] >= 0} {
    if {[string match $user [lindex $line 0]]} {
      set utime [lindex $line 8]
      if {$utime == "logged"} {continue}
      ;# jump out if this line is still logged in
      
      set user_time [expr $user_time + $utime]
      set user_charge [expr $user_charge + $rate * $utime / 60.00]
    }
  }
  close $t_file
}

proc holiday {mydate} {
  global hols 
  set date " $mydate"
  set success [lsearch $hols $date]
  set success [expr { ($success >= 0) ? 1 : 0}]
  return $success
}

proc chargeaday {cur_date st_day ctr} {
  global dex
  upvar #0 cost_day cost_day
  upvar #0 start_hour start_hour
  upvar #0 hour_cost hour_cost
  upvar #0 time_array time_array
  upvar #0 rate_array rate_array
  unset time_array rate_array
  
        #check for holiday
        
  if {[holiday $cur_date]} {
    set today ho
  } else {
    set today $st_day
  }
        
  set zero -1
  set dex 0
        
  for {set ind 1} {$ind <= $ctr} {incr ind} {
        
    if {$cost_day($ind,$today)} {		;#get valid rates for today
      set time_array($dex) [minute $start_hour($ind)]
      set rate_array($dex) $hour_cost($ind)
      if {[string compare $start_hour($ind) "0:00"] == 0} {
        set zero $dex
      }
      incr dex
    }
  }	;#made the arrays - check start of day, then sort
 
        
  if {$zero < 0} {	;#no start, we need last of yesterday
    set today [inc_day $st_day -1]	;#never a holiday-1
    set time_array($dex) 0
    set rate_array($dex) 0

;#find yesterdays last rate
          
    for {set ind 1} {$ind <= $ctr} {incr ind} {	
      if {$cost_day($ind,$today)} {
        if {[minute $start_hour($ind)] > $time_array($dex)} {
          set time_array($dex) [minute $start_hour($ind)]
          set rate_array($dex) $hour_cost($ind)
        }
      }
    }
    set time_array($dex) 0	;#clear out the time
    incr dex

  }
  incr dex -1
            
          #now we can sort
          
  for {set jnd 1} {$jnd <= $dex} {incr jnd} {
    set a $time_array($jnd)
    set b $rate_array($jnd)
    set ind [expr $jnd - 1]
    while {($ind >= 0) && ($time_array($ind) > $a)} {
      set time_array([expr $ind + 1]) $time_array($ind)
      set rate_array([expr $ind + 1]) $rate_array($ind)
      incr ind -1
    }
    set time_array([expr $ind + 1]) $a
    set rate_array([expr $ind + 1]) $b
  }
          

  incr dex
  set time_array($dex) [minute 24:00]
  set rate_array($dex) $rate_array([expr $dex - 1])
       
}

proc multirate_charge {user group} {
  global ACC_FILE MULTI_FILE
  global user_time user_charge dex
  
  upvar #0 cost_day cost_day
  upvar #0 start_hour start_hour
  upvar #0 hour_cost hour_cost
  upvar #0 time_array time_array
  upvar #0 rate_array rate_array
  
  set m_file [open $MULTI_FILE r]
  
  while {[gets $m_file line] >= 0} {
    if {[string match $group [lindex $line 0]]} {
      set ctr [lindex $line 1]
      array set cost_day [lindex $line 2]
      array set start_hour [lindex $line 3]
      array set hour_cost [lindex $line 4]
    }
  }
  if {[array exists start_hour] != 1} {
    return -1					#HELP
  }
    
    #got cost structure for this group, now we can build the bill
    #for this user
    
  close $m_file
  set t_file [open $ACC_FILE r]
    
  set user_charge 0
  set user_time 0
    
  while {[gets $t_file line] >= 0} {
    if {[string match $user [lindex $line 0]]} {
      set totime [lindex $line 8]
      if {$totime == "logged"} {continue}
      ;# This line still logged in - discard
      
      set user_time [expr $user_time + $totime]	;#the easy part
        
      set st_day [lindex $line 1]	;#day of week
      set st_date [lindex $line 3]	;#day of month
      set st_mo [lindex $line 2]	;#month
      set st_yr [lindex $line 4]	;#year
      set st_time [lindex $line 5]	;#start time of day
        
      set st_min [minute $st_time]
      set st_secs [clock scan "$st_mo $st_date, $st_yr"]
      
      #may be multiple days
      
      while {$totime > 0} {
        
        set cur_date "$st_mo $st_date, $st_yr"
        chargeaday $cur_date $st_day $ctr	;#rates for 1 day


          ;#finally we can do the money
        
        for {set ind 0} {$ind <= $dex} {incr ind} {
          if {$st_min < $time_array($ind)} {
            set charges [expr $time_array($ind) - $st_min]

            set st_min $time_array($ind)
            if {$charges >= $totime} {
              set charges $totime
            }
            set totime [expr $totime - $charges]
            set user_charge [expr $user_charge + $charges * \
                $rate_array([expr $ind - 1]) / 60.00]
          }
        }		;#built charges for this day
        
        #we got to the end of the day, advance the date and continue
        
        set st_secs [expr $st_secs + (24 * 3600) + 100]
        set days [clock format $st_secs -format "%a %b %d %Y"]
        set st_day [lindex $days 0]
        set st_mo [lindex $days 1]
        set st_date [lindex $days 2]
        set st_yr [lindex $days 3]
        set st_min 0

      }	;#end of this line
      
    }	;#end of the user on the line
  }	;#end of file
  close $t_file
}
        
proc totalize {} {
  global ACC_FILE_TOTAL GROUP_FILE
  global user_time user_charge 		;# time charge
  
  set tot_file [open $ACC_FILE_TOTAL w]
  set g_file [open $GROUP_FILE r]
  
#  set n_group 1
  while {[gets $g_file Line] != -1} {
    set group [string trimright [lindex $Line 0] ":"]
    set rate [lindex $Line 1]
    scan $rate "%f" rate
    set members [lrange $Line 2 end]
#    set time 0
#    set charge 0
    
    foreach user $members {
      .top.listbox insert end "Processing $user in Group $group"
      update
      set user_time 0
      set user_charge 0
      if {$rate >= 0} {
        singlerate_charge $user $rate
      } else {
        multirate_charge $user $group
      }
#      set time [expr $time + $user_time]
#      set charge [expr $charge + $user_charge]
      set save_to_insert "Total time used by "
      append save_to_insert $user
      append save_to_insert " is: "
      append save_to_insert $user_time
      append save_to_insert " minutes, $ "
      append save_to_insert $user_charge
      append save_to_insert " (group "
      append save_to_insert "$group)"
      puts $tot_file $save_to_insert
    }
#    incr n_group
  }
  close $g_file
  close $tot_file
}

;###############
;#
;#  Accounting engine finishes here
;#
;###############

;###############
;# addUsers -
;#
;###########################################################################
proc addUsers {} {
   global group_name1 members_list font host_name

   # the next line is ok for now, next have to make "font" global
   #set font terminal ;#-*-Helvetica-Medium-R-Normal--*-140-*-*-*-*-*-*
   set w .form
   catch {destroy $w}
   toplevel $w
   positionWindow $w
   set main_title "VNMR:  Add  User to "
   append main_title $host_name
   wm title $w $main_title
   wm iconname $w "Add User"

   label $w.msg -wraplength 4i -justify left -text "Enter name of an existing group
 and the user to add to the group"
pack $w.msg -side top

   frame $w.buttons
   pack  $w.buttons -side bottom -expand y -fill x -pady 2m
   button $w.buttons.dismiss -text Cancel -command "destroy $w"
   button $w.buttons.code -text "Apply" -command "applyAddUsers"
   pack $w.buttons.code $w.buttons.dismiss -side left -expand 1

   frame $w.f1 -bd 2
   entry $w.f1.entry -relief sunken -width 40 -textvariable group_name1
   label $w.f1.label
   pack $w.f1.entry -side right
   pack $w.f1.label -side right

   frame $w.f2 -bd 2
   entry $w.f2.entry -relief sunken -width 40 -textvariable members_list
   label $w.f2.label
   pack $w.f2.entry -side right
   pack $w.f2.label -side right


   $w.f1.label config -text   "Group name:"
   $w.f2.label config -text   " User name:"

   pack $w.msg $w.f1 $w.f2 -side top -fill x
   focus $w.f1.entry
   return 0
}


;###############
;# applyAddUsers -
;#
;###########################################################################
proc applyAddUsers {} {
   global group_name1 members_list GROUP_FILE

   set grp_name1 [string trim $group_name1]
   set add_members [string trim $members_list]

   if {$grp_name1  == "" } {
      dialog .d {Add User} {Missing  group  name} {} -1 OK
      return -1
   }

   if {$add_members == ""} {
      dialog .d {Add User} "Missing user name" {} -1 OK
      return -1
   }

   if {[llength $grp_name1 ] != 1 } {
      dialog .d {Add User} {Can only add users of 1 group at a time .} {} -1 OK
      return -1
   }

   set group_matched 0
   set f [open  $GROUP_FILE  r]
   while {[gets $f line] >= 0} {
       set group [lindex $line 0]
 
       if {[string match $grp_name1: $group]} {   ;# YES group exist

          set group_matched 1
          set user_exist [lrange $line 2 end]
          if {[string length $user_exist] == 0} { ;# it is a empty group
             foreach j $add_members {
                 set k [dialog .d {Add User} "Add user $j to  \"$grp_name1\" group ? " \
                                                          {} -1 OK Cancel]
                 if {$k == 0} {
                    doAddUser $grp_name1 $j
                    ;#dialog .d {Add User} " User $j Added to  \"$grp_name1\" group ." {} -1 OK
                 } 
             }
          } else { ;# looking for existing user
               foreach j $add_members {
                  set matched 0
                  foreach i $user_exist {
                     if {[string match $j $i]} {
                        set matched 1
                     }
                  }

                  if {$matched == 1} {
                     dialog .d {Add User} "User \"$j\" Already Exist \
                                                  in \"$grp_name1\" group" {} -1 OK
                  } else {
                       set k [dialog .d {Add User} "Add user $j to  \"$grp_name1\" group ? " \
                                                          {} -1 OK Cancel]
                       if {$k == 0} {
                          doAddUser $grp_name1 $j
                          ;#dialog .d {Add User} " User $j Added to  \"$grp_name1\" group ." {} -1 OK
                       }
                    }

               }
            }
       }
   } ;# end of while

   close $f
   if {$group_matched == 0} {
      dialog .d {Add User} {The requested group not exist,\
                                      use Add group before Add user .} {} -1 OK
      return -1
   }
   return 0
}

;###############
;# doAddUser -
;#  called by apply button of applyDeleteUser widget
;#  the caller responsible for all the necessarily checking
;#
;###########################################################################
proc doAddUser {g_name u_name} {
   global GROUP_FILE TEMP_FILE

   set TEMP [open $TEMP_FILE w]
   set f [open $GROUP_FILE r]
   while {[gets $f Line] >= 0} {
       set grp [lindex $Line 0]
       if {![string match $g_name: $grp]} {   ;# group not matched => save
          puts $TEMP $Line
       } else {                  ;# group matched => add to this group  and save
             set temp_list [lappend Line $u_name]    ;# add user to this group 
             puts $TEMP $temp_list
         }
   }
   close $f
   close $TEMP
   exec cp $TEMP_FILE $GROUP_FILE
   exec rm $TEMP_FILE
   updateMainWindow
   return 0
}


;###############
;# deleteGroup -
;#
;###########################################################################
proc deleteGroup {} {
   global group_name members_list font host_name

   set w .form
   catch {destroy $w}
   toplevel $w
   set main_title "VNMR:  Delete Accounting Group on"
   append main_title $host_name
   wm title $w $main_title
   wm iconname $w "Delete Group"
   positionWindow $w
 
   label $w.msg -wraplength 4i -justify left -text "Enter name of accounting group
to delete"
pack $w.msg -side top
 
   frame $w.buttons
   pack  $w.buttons -side bottom -expand y -fill x -pady 2m
   button $w.buttons.dismiss -text Cancel -command "destroy $w"
   button $w.buttons.code -text "Apply" -command "applyDeleteGroup"
   pack $w.buttons.code $w.buttons.dismiss -side left -expand 1
 
   frame $w.f1 -bd 2
   entry $w.f1.entry -relief sunken -width 40 -textvariable group_name
   label $w.f1.label
   pack $w.f1.entry -side right
   pack $w.f1.label -side right
 
   $w.f1.label config -text   "Group name:"
 
   pack $w.msg $w.f1 -side top -fill x
   #bind $w <Return> "destroy $w"
   focus $w.f1.entry
}


;###############
;# applyDeleteGroup -
;#  called by apply button of deleteGroup widget
;# 
;###########################################################################
proc applyDeleteGroup {} {
   global group_name GROUP_FILE
   set grp_name [string trim $group_name]     ;# new group name
 
   if {[llength $grp_name] != 1} {
      dialog .d { Delete group} {Can only delete one group at a time} {} -1 OK
      return 0
   }
   set f [open  $GROUP_FILE  r]
      while {[gets $f line] >= 0} {
          set group [lindex $line 0]
          if { [string match $grp_name: $group] } {   ;# YES $grp_name exist
             set user_exist [lrange $line 2 end]
             if {[string length $user_exist] == 0} {  ;# users NOT exist, ok to delete
                 set k [dialog .d {Delete Group}  "Delete  \"$grp_name\"  group ?" {} -1 OK Cancel]
                 if {$k == 0} {
                    doDeleteGroup $grp_name
                    close $f
                    return 0
                 }
                 return -1
             } else {
                   close $f
                   dialog .d {Delete group} "Can not delete the \"$grp_name\" group
 Users still exist in this group" {} -1 OK
                   return  0
             }
          } 
      }
      close $f
      dialog .d {Delete group} "\"$grp_name\": group not exist" {} -1 OK
      return 0
}

 
;###############
;# doDeleteGroup -
;#  called by apply button of applyDeleteGroup widget
;#  the caller responsible for all the necessarily checking
;# 
;###########################################################################
proc doDeleteGroup g_name {
   global GROUP_FILE TEMP_FILE
 
   set TEMP [open $TEMP_FILE w]
 
   set f [open $GROUP_FILE r]
   while {[gets $f Line] >= 0} {

       set grp [lindex $Line 0]
       if {![string match $g_name: $grp]} {   ;# group not matched ---> save
          puts $TEMP $Line
       }
   }
   close $f
   close $TEMP
   exec cp $TEMP_FILE $GROUP_FILE
   exec rm $TEMP_FILE
   updateMainWindow
   return 0
}


;###############
;# deleteUser -
;#
;###########################################################################
proc deleteUser {} {
   global group_name1 members_list font host_name

   # the next line is ok for now, next have to make "font" global
   #set font terminal ;#-*-Helvetica-Medium-R-Normal--*-140-*-*-*-*-*-*
   set w .form
   catch {destroy $w}
   toplevel $w
   set main_title "VNMR:  Delete  User on "
   append main_title $host_name
   wm title $w $main_title
   wm iconname $w "Delete User"
   positionWindow $w
 
   label $w.msg -wraplength 4i -justify left -text "Enter name of user to delete"
pack $w.msg -side top
 
   frame $w.buttons
   pack  $w.buttons -side bottom -expand y -fill x -pady 2m
   button $w.buttons.dismiss -text Cancel -command "destroy $w"
   button $w.buttons.code -text "Apply" -command "applyDeleteUser"
   pack $w.buttons.code $w.buttons.dismiss -side left -expand 1
 
   frame $w.f1 -bd 2
   entry $w.f1.entry -relief sunken -width 40 -textvariable group_name1
   label $w.f1.label
   pack $w.f1.entry -side right
   pack $w.f1.label -side right
 
   frame $w.f2 -bd 2
   entry $w.f2.entry -relief sunken -width 40 -textvariable members_list
   label $w.f2.label
   pack $w.f2.entry -side right
   pack $w.f2.label -side right
 
 
   $w.f1.label config -text   "Group name:"
   $w.f2.label config -text   " User name:"
 
   pack $w.msg $w.f1 $w.f2 -side top -fill x
   focus $w.f1.entry
   return 0
}


;###############
;# applyDeleteUser -
;#
;###########################################################################
proc applyDeleteUser {} {
   global group_name1 members_list GROUP_FILE

   set grp_name1 [string trim $group_name1]
   set delete_members [string trim $members_list]

   if {$grp_name1  == "" } {
      dialog .d {Delete User} {Missing  Group  Name} {} -1 OK
      return -1
   }

   if {$delete_members == ""} {
      dialog .d {Delete User} "Missing user name" {} -1 OK
      return -1
   }


   if {[llength $grp_name1 ] != 1 } {
      dialog .d {Delete User} {Can only delete users of 1 group at a time .} {} -1 OK
      return -1
   }

   set f [open  $GROUP_FILE  r]
      while {[gets $f line] >= 0} {
          set group [lindex $line 0]

          if { [string match $grp_name1: $group] } {   ;# YES $grp_name1 exist
             set user_exist [lrange $line 2 end]

             if {[string length $user_exist] == 0} {  ;#  but, it is empty group
 
                 dialog .d {Delete User} "\"$grp_name1\" group has no Users " {} -1 OK
                 close $f
                 return 0
 
             } else {
                   foreach j $delete_members {
                       set matched 0
                       foreach i $user_exist {
                           if {[string match $j $i]} {
                              set k [dialog .d {Delete user} " Delete user \"$j\" ?" {} -1 OK Cancel]
                              if {$k == 0} {
                                 doDeleteUser $grp_name1 $j
                                 set matched 1
                              } else { set matched 1 }
                           }
                       }
                       if {$matched == 0} {
                       dialog .d {Delete User} "User \"$j\" Not exist \
                                                    in \"$grp_name1\" group" {} -1 OK
                       }
                   }
                   close $f
                   return  0
               }
          }
      }    
      close $f
      dialog .d {Delete User} "\"$grp_name1\": group not exist" {} -1 OK
      return 0
}


;###############
;# doDeleteUser -
;#  called by apply button of applyDeleteUser widget
;#  the caller responsible for all the necessarily checking
;#
;###########################################################################
proc doDeleteUser {g_name u_name} {
   global GROUP_FILE TEMP_FILE

   set TEMP [open $TEMP_FILE w]
   set f [open $GROUP_FILE r]
   while {[gets $f Line] >= 0} {
       set grp [lindex $Line 0]
       if {![string match $g_name: $grp]} {   ;# group not matched => save
          puts $TEMP $Line
       } else {                  ;# group matched => delete member  and save

             set temp_list [lreplace $Line 2 end]    ;# save group name and $$$
             set user_exist [lrange $Line 2 end]
             foreach i $user_exist {

                 if {![string match $i $u_name]} {
                    lappend temp_list $i
                 }
             }
             puts $TEMP $temp_list
         }
   }
   close $f
   close $TEMP
   exec cp $TEMP_FILE $GROUP_FILE
   exec rm $TEMP_FILE
   updateMainWindow
   return 0
}


;###############
;# showHelp -
;#
;###########################################################################
proc showHelp {} {
  
   set w .menu
   catch {destroy $w}
   toplevel $w
   wm title $w "HELP.Accounting"
   wm iconname $w "Help"
   positionWindow $w
   set font  terminal

   frame $w.menu -relief raised -bd 2
   pack $w.menu -side top -fill x
 
   label $w.msg -wraplength 4i -justify left -text "This window is reserved \
for HELP only , it is part of showHelp\{\} routine, will add more \
help instructions here as time goes by .
-Add one group name at a time"
   pack $w.msg -side top

   frame $w.buttons
   pack  $w.buttons -side bottom -expand y -fill x -pady 2m
   button $w.buttons.dismiss -text Done -command "destroy $w"
   pack $w.buttons.dismiss
   return 0
}

;###############
;# showConsoleUsers -
;#   called by "See Console Users" button from the main window
;#
;###########################################################################
proc showConsoleUsers {} {
   global ACC_FILE_TOTAL host_name lpstring help_tips helpTip

   if {[winfo exists .users]} {
         destroy .users
      }

      toplevel .users
      wm minsize .users 250 100
      positionWindow  .users
      frame .users.buttons
      frame .users.button
      pack .users.button -side bottom -expand 1 -fill x
      pack .users.buttons -side bottom -expand 1 -fill x
      button .users.buttons.dismiss -text Cancel -command "destroy .users"

      button .users.buttons.printuser -text "Print  user" -command {
           set user_pr ""
           catch {set user_pr [string trim [selection get]]}
           if { $user_pr == "" } {
               dialog .d {Print} "Select  the  user  to  print" {} -1 OK
           } else {
                set k [dialog .d {Print}  "Print  user  [lindex $user_pr 0]  ?" {} -1 OK Cancel]
                if {$k == 0} {
                   catch { [eval exec grep [lindex $user_pr 0] $ACC_FILE | $lpstring 2>/dev/null] }
                }
             }
            set user_pr ""
      }

      button .users.button.printwindow -text "Print user window" -command {
         set k [dialog .d {Print}  {Print  VNMR  Users  ?} {} -1 OK Cancel]
         if {$k == 0} {
            set execstring "cat $ACC_FILE_TOTAL | awk '{print \$5}' | $lpstring 2\>/dev/null"
            catch {exec sh -c $execstring}
         }
      }

      set help_tips(.users.buttons.dismiss)     {$helpTip(Cancel1)}
      set help_tips(.users.buttons.printuser)   {$helpTip(Print_user)}
      set help_tips(.users.button.printwindow) {$helpTip(Print_user_windowx)}

      pack .users.button.printwindow -side bottom -expand 1 -fill x
      pack .users.buttons.printuser .users.buttons.dismiss -side left -expand 1 -fill x

      listbox .users.user -yscrollcommand ".users.scroll set"
      pack .users.user -side left -expand 1 -fill both

      scrollbar .users.scroll -command ".users.user yview"
      pack .users.scroll -side right -fill y
     .users.user delete 1 end

      set f [open $ACC_FILE_TOTAL.tmp r]
      while {[gets $f line] >= 0} {
          .users.user insert end [ format "    %s" [lrange $line 4 4] ]
      }
      close $f

      bind .users.user <Double-Button-1> {
          showUserLog [string trim [selection get]]
      }

   set main_title "VNMR:  "
   append main_title $host_name
   append main_title " Console  Users:"
   wm title .users $main_title
   wm iconname .users "Console Users"
   return 0
}


;###############
;# showConsoleLog -
;#   called by "See Console Log" from the main window
;#
;###########################################################################
proc showConsoleLog {} {
    global ACC_FILE host_name lpstring

    if ![winfo exists .log] {
        toplevel .log
        positionWindow  .log
        frame .log.buttons
        pack .log.buttons -side bottom -expand 1 -fill x
        button .log.buttons.dismiss -text Cancel -command "destroy .log"
        button .log.buttons.print -text "Print" -command {
           set k [dialog .d {Print Log}  {Print  Console  Log  ?} {} -1 OK Cancel ]
           if {$k == 0} {
               catch { [eval exec cat $ACC_FILE.tmp | $lpstring 2>/dev/null ] }
           }
        }
        pack .log.buttons.print .log.buttons.dismiss -side left \
                -expand 1
        text .log.text -height 40 -yscrollcommand ".log.scroll set" -setgrid 1
        pack .log.text -side left -expand 1 -fill both
        scrollbar .log.scroll -command ".log.text yview"
        pack .log.scroll -side right -fill y
    } else {
        wm deiconify .log
        raise .log
    }
   set main_title "VNMR:  "
   append main_title $host_name
   append main_title " Console  Log"
   wm title .log $main_title
    wm iconname .log "Console Log" 
    .log.text delete 1.0 end
    .log.text insert 1.0 [exec cat $ACC_FILE.tmp]
    .log.text mark set insert 1.0
    return 0
}


;###############
;# showUserLog -
;#    show individual user log
;#    called by - showConsoleUsers{}  (bind)
;#              - showGroup{}  (bind)
;#              - and from Unaccounted Users  (bind)
;#
;###########################################################################
proc showUserLog {this_user} {
   global ACC_FILE ACC_FILE_TOTAL host_name lpstring
 
   if {![winfo exists .user]} {
      toplevel .user
      positionWindow  .user
 
      frame .user.buttons
      pack .user.buttons -side bottom -expand 1 -fill x
 
      button .user.buttons.dismiss -text Cancel -command "destroy .user"
      button .user.buttons.print -text "Print" -command {
         set k [dialog .d {Print}  "Print  [lindex [selection get] 0]  ?" {} -1 OK Cancel]
         if {$k == 0} {
            catch { [eval exec grep [lindex [selection get] 0] $ACC_FILE | $lpstring 2>/dev/null] }
         }
      }  
       pack .user.buttons.print .user.buttons.dismiss -side left -expand 1
       text .user.text -height 40 -yscrollcommand ".user.scroll set" -setgrid 1
       pack .user.text -side left -expand 1 -fill both
       scrollbar .user.scroll -command ".user.text yview"
       pack .user.scroll -side right -fill y
    } else {
         wm deiconify .user
         raise .user
    }
   set main_title "VNMR:  "
   append main_title $host_name
   append main_title " Console  User:  $this_user"
   wm title .user $main_title
    wm iconname .user "$this_user"
    .user.text delete 1.0 end
     
    set f [ open $ACC_FILE r ] 
    while { [gets $f Line] >= 0 } {
      if {[string match [lindex $Line 0] $this_user]} {
         .user.text insert 1.0 $Line
         .user.text insert 1.0 "\n" 
      }
    } 
    close $f

    set f [ open $ACC_FILE_TOTAL r ] 
    while { [gets $f Line] >= 0 } {
      if {[string match [lindex $Line 4] $this_user]} {
         .user.text insert end "\n\n" 
         .user.text insert end  [string trim $Line]
      }
    } 
    close $f

    return 0
}


;###############
;# showGroup -
;#    display accounting groups, with total time and $$$$
;#    called from the main window,
;#    the groups are extracted from /vnmr/adm/accounting/group file
;#
;###########################################################################
proc showGroup group_s {
   global GROUP_FILE ACC_FILE_TOTAL host_name
 
   if {![winfo exists .showgroup]} {
 
      toplevel .showgroup
      wm minsize .showgroup 500 100
      positionWindow  .showgroup
      frame .showgroup.buttons
      pack .showgroup.buttons -side bottom -expand 1 -fill x
      label .showgroup.dumlable -text "                                                          " \
                                      -relief raised -bd 1 -font 9x15bold -fg red
      pack .showgroup.dumlable -anchor w -side top  -fill x
 
      button .showgroup.buttons.dismiss -text Done -command "destroy .showgroup"
      button .showgroup.buttons.print -text "Print" -command {
         set k [dialog .d {Print}  "Print  [lindex [selection get] 0]  ?" {} -1 OK Cancel]
         if {$k == 0} {
            printGroup [lindex [selection get] 0]
         }
      }   
      pack .showgroup.buttons.print .showgroup.buttons.dismiss -side left -expand 1 -fill both
 
      listbox .showgroup.listbox -relief flat -highlightthickness 1  -font 9x15bold \
                                           -yscrollcommand ".showgroup.scroll set"
      scrollbar .showgroup.scroll -relief flat -highlightthickness 1 \
                                           -command ".showgroup.listbox yview"
    
    frame .showgroup.tc
    label .showgroup.tc.topcat -justify left \
          -text "             USERS        TIME(hr)              $" \
          -font 7x13bold -fg blue
    pack  .showgroup.tc.topcat -side left
    pack  .showgroup.tc -side top -anchor w -fill x

    pack  .showgroup.listbox -side left -expand 1 -fill both
    pack  .showgroup.scroll -side right -fill y
 
   } else {
       wm deiconify .showgroup
       raise .showgroup
  }   
 
      set group_and_members [exec grep $group_s $GROUP_FILE]
      set members_only [lreplace $group_and_members 0 1] ;#Take out group and $$ columns
      .showgroup.listbox delete 0 end
      set n 1
 
      foreach i $members_only {
         set member_info  0
         set member_to_insert [format "  %12s" $i]
         catch {set member_info [string trim [exec grep -s $i $ACC_FILE_TOTAL]]}
 
         if { $member_info == 0 } {
            set member_charge  0
            set member_time  0
         } else {
               set member_time [lindex $member_info 6]
               set member_charge [lindex $member_info 9]
         }
         append member_to_insert [format "  %10.2f"  [expr $member_time / 60.0] ]
         append member_to_insert [format "  %13.2f"  $member_charge]
         .showgroup.listbox insert end  $member_to_insert
 
         incr n
      }
 
      bind .showgroup.listbox <Double-Button-1> {
          showUserLog [lindex [selection get] 0]
      }  
 
   set main_title "VNMR:  "
   append main_title $host_name
   append main_title " Accounting Group  ( $group_s )"
   wm title .showgroup $main_title
   wm iconname .showgroup "$group_s"
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
        positionWindow $w 
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


;###############
;# updateMainWindow -
;#
;###########################################################################
proc updateMainWindow {} {
   global  ACC_DIR GROUP_FILE ACC_FILE_TOTAL members_saved calc_totals

   if [ winfo exists .top.listbox ] {.top.listbox delete 0 end}
   
   if {$calc_totals} {

     .top configure -cursor watch
     update
   
     totalize			;#do the actual accounting
     set calc_totals 0
   }

   set f [open  $GROUP_FILE  r]
 
   set members_saved ""
   
   if [ winfo exists .top.listbox ] {.top.listbox delete 0 end}

   set n 1
   while {[gets $f Line] != -1} {
 
       set group($n) [lindex $Line 0 ]

       set save_to_insert [format "%15s" $group($n)] ;# save group name
 
       set members_only [lrange $Line 2 end]
 
       set xxx [concat $members_saved $members_only]   ;# save for checking unaccounted users
       set members_saved $xxx
 
       set group_time 0
       set group_charge 0
       foreach i $members_only {
          set f1 [open $ACC_FILE_TOTAL r]
          while { [gets $f1 one_line] >= 0 } {
              if { [string match $i [lindex $one_line 4]] } {
                 set member_time [lindex $one_line 6]
                 set group_time [expr $group_time + $member_time]
                 set member_charge [lindex $one_line 9]
                 set group_charge [expr $group_charge + $member_charge]
              }
          }
          close $f1
       }
 
       append save_to_insert [format "%12.2f"  [expr $group_time / 60.0] ]
       append save_to_insert [format "  %13.2f"  $group_charge]
 
       .top.listbox insert end $save_to_insert     ;# insert group name to left listbox

       incr n
   }
   close $f
   
   .top configure -cursor arrow
   update

   unaccountedUsers
 
}


;###############
;# unaccountedUsers -
;#
;###########################################################################
proc unaccountedUsers {} {
      global members_saved ACC_FILE_TOTAL
 
      if [ winfo exists .bottom.listbox ] {.bottom.listbox delete 0 end}
      set nn 0  
     
      set tmpfile $ACC_FILE_TOTAL.tmp
      set f [open $tmpfile r]
      set unaccounted_display 0

      while {[gets $f line] >= 0} {
        if {[string length $line] == 0} {
            continue
        }
        set matched 0
        set user  [lrange $line 4 4]
        set user_to_insert [format "%15s" $user]
 
        if {[llength $members_saved] != 0 } {
          foreach mem $members_saved {
            if { $user == $mem }  {
              set matched 1  ;#  this user is accounted for
            }   
          }
        }
        if { $matched == 0 } {
          append user_to_insert  [format "%19.2f hours"  [expr [lrange $line 6 6] / 60.00]]
          .bottom.listbox insert end $user_to_insert

          set unaccounted_display 1
          incr nn
        }

      }   

      close $f

      if { $unaccounted_display == 0 } {
        pack forget .label
        pack forget .bottom
        pack .dumlable 
      } else {
        pack forget .dumlable
        pack  .label
        pack .bottom -side top -anchor w -padx 20 -pady 10 -fill x
      }
}

;###############
;#   displayMainWindow       
;#
;###########################################################################
proc displayMainWindow {} {
   
   global host_name help_tips helpTip
   
   eval destroy [winfo child .]

   set main_title "VNMR  ACCOUNTING: "
   append main_title $host_name
   
   wm title . $main_title
   wm iconname . "ACCOUNTING"
   wm geometry . +300+100

    
    frame .mbar -relief raised -width 240 -bd 2  ;#for File and Edit buttons
    
    frame .topcat
    label .topcat.l -justify left \
     -text "            GROUPS    TIME(hr)           $" \
     -font 7x13bold -fg blue
    pack  .topcat.l -side left

    frame .top -width 140m ;#-height 80m ;#for top listbox

    listbox .top.listbox -font 7x13bold  -height 15 -relief flat -highlightthickness 1 \
                                                 -yscrollcommand ".top.scroll set"
    scrollbar .top.scroll -relief flat -highlightthickness 1 -command ".top.listbox yview"
    label .label -text "    Unaccounted Users                                   " \
                                                         -font 9x15bold -fg red
    ;# need dumlabel here to keep main window stayed fixed width
    label .dumlable -text "                                                          " \
                                                         -font 9x15bold -fg red
    frame .bottom -width 140m -height 80m ;# this frame for listing unaccounted user
    frame .buts
    frame .buttons

    pack .mbar -side top -fill x
    pack .topcat -anchor w -side top -fill x
    
    pack .top -side top -anchor w -padx 20 -pady 2 -fill x

    pack .top.listbox -side left -expand 1 -fill both 
    pack .top.scroll -side right -fill y

    ;#the unaccounted frame no need to pack here, wait for checking later
    pack .label -anchor w -side top  -fill x
    pack .dumlable -anchor w -side top  -fill x

    pack .bottom -side top -anchor w -padx 20 -pady 10 -fill x
    pack .buts -side bottom -anchor w -fill x -pady 2m
    pack .buttons -side bottom -anchor w -fill x -pady 2m

    listbox .bottom.listbox -font 9x15bold -height 10 -relief flat -highlightthickness 1 \
                                                 -yscrollcommand ".bottom.scroll set"
    scrollbar .bottom.scroll -relief flat -highlightthickness 1 -command ".bottom.listbox yview"
    pack .bottom.listbox -side left -expand 1 -fill both 
    pack .bottom.scroll -side right -fill y 

   ;#-----------------------------------------------------------

    updateMainWindow

   ;#-----checking for unaccounted users  (done by updateMainWindow)-------

   bind .top.listbox <Double-Button-1> {
         showGroup [lindex [selection get] 0]
   }
   bind .bottom.listbox <Double-Button-1> {
         set this_unaccounted_user [lindex [selection get] 0]
         showUserLog $this_unaccounted_user 
         wm title .user "VNMR  Unaccounted Console  User:  $this_unaccounted_user"
         wm iconname .user "$this_unaccounted_user"
   }

    menubutton .mbar.file -text File  -menu .mbar.file.menu
    menubutton .mbar.edit -text Edit  -menu .mbar.edit.menu
    menubutton .mbar.hols -text Holidays  -menu .mbar.hols.menu

    catch {set help_tips(.mbar.file) {$helpTip(File)} }
    catch {set help_tips(.mbar.edit) {$helpTip(Edit)} }
    catch {set help_tips(.mbar.hols) {$helpTip(Holiday)} }

    menu .mbar.file.menu -tearoff no
      .mbar.file.menu add command -label "Printer..." -command "selectPrinter"
      .mbar.file.menu add command -label "Quit" -command "exit"
    menu .mbar.edit.menu -tearoff no
      .mbar.edit.menu add command -label "Add_Group" -command addGroup
      .mbar.edit.menu add command -label "Add_User" -command addUsers
      .mbar.edit.menu add command -label "Delete_Group" -command deleteGroup
      .mbar.edit.menu add command -label "Delete_User" -command deleteUser

    catch {set help_tips(.mbar.file.menu,0) {$helpTip(Printer...)} }
    catch {set help_tips(.mbar.file.menu,1) {$helpTip(Quit)} }

    catch {set help_tips(.mbar.edit.menu,0) {$helpTip(Add_Group)} }
    catch {set help_tips(.mbar.edit.menu,1) {$helpTip(Add_User)} }
    catch {set help_tips(.mbar.edit.menu,2) {$helpTip(Delete_Group)} }
    catch {set help_tips(.mbar.edit.menu,3) {$helpTip(Delete_User)} }
      
    menu .mbar.hols.menu -tearoff no
      .mbar.hols.menu add command -label "Add ..." \
        -command {source $ACC_BIN_DIR/xcal}

    catch {set help_tips(.mbar.hols.menu,0) {$helpTip(Add)} }

    pack .mbar.file -side left

    button .buttons.dismiss -text Exit -command "destroy ."
    button .buttons.printm -text "Print main window" -command {
             set k [dialog .d {Print} "Print  main  window  ?" {} -1 OK Cancel]
             if {$k == 0} {
                printMainWindow
             }
    }

    button .buttons.printg -text "Print all groups" -command {
                set k [dialog .d {Print} "Print  All  Groups  ?" {} -1 OK Cancel]
                if {$k == 0} {
                   printAllGroup
                }
    }
    set help_tips(.buttons.dismiss) {$helpTip(Exit)}
    set help_tips(.buttons.printm)  {$helpTip(Print_main_window)}
    set help_tips(.buttons.printg)  {$helpTip(Print_all_groups)}

    button .buttons.doit -text "Update & Save" -command "update_and_save"
    button .buts.users -text "Console Users" -command "showConsoleUsers"
    button .buts.log -text "Console Log" -command "showConsoleLog"
    button .buttons.calc -text "Recalc" -command {
            global calc_totals
            set calc_totals 1
            updateMainWindow
    }
    set help_tips(.buttons.doit) {$helpTip(Update_Save)}
    set help_tips(.buts.users)   {$helpTip(Console_Users)}
    set help_tips(.buts.log)     {$helpTip(Console_Log)}
    set help_tips(.buttons.calc) {$helpTip(Recalc)}

   if [ are_you_root ] {
      pack .mbar.edit -side left
      pack .mbar.hols -side left
      pack .buttons.calc .buttons.doit .buttons.printg .buttons.printm \
           .buttons.dismiss -side left -fill x -expand 1 -padx 1m
   } else {
       pack .buttons.printg .buttons.printm .buttons.dismiss -side left -fill x -expand 1
   }

   pack .buts.users .buts.log -side left -fill x -expand 1

}

;###########################################################################
;#                   Main program start here
;#
;###########################################################################
displayMainWindow
updateMainWindow
