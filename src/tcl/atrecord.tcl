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

# args to atrecord are filename, label, userdir, and owner
# these are followed by pairs of label and values
#atrecord filename test_text /export/home/vnmr1 vnmr1 pair_of_values ...
#atrecord H1RFHOMO "test_text" /export/home/vnmr1 vnmr1 pw360 60 pw360/0 0.87 pw720 123 pw720/0 0.73

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

set hist_name [lindex $argv 0]
set filelabel [lindex $argv 1]
set AT_DIR    [lindex $argv 2]/autotest

set osType [exec uname -s]
if { $osType == "Interix"} {
   set AT_DIR [winname $AT_DIR]
}

set FAIL_FILE     $AT_DIR/atfail
set HISTORY_DIR   $AT_DIR/history
set REPORT_FILE   $AT_DIR/atrecord_report
set TABLE_FILE    $AT_DIR/atdb/at_spec_table
set TEMP_FILE     $AT_DIR/atdb/temp
set REPORT_HEADER $AT_DIR/texts/reportform
set HIST_FILE $HISTORY_DIR/$hist_name

if { [expr $argc%2] == 1 } {
  #Must be a filename plus "test label" plus one or more pairs of "label and value"
  return
}

if ![ file isdirectory $AT_DIR/history ] {
    catch { exec mkdir -p $AT_DIR/history }
}   

catch { exec date "+%b %d %H:%M %Y"} goodjunk
set udate [lrange $goodjunk 0 3]

set lblstr ""
set index_list ""
set hisdate "#Date"
 
proc calAvgStd {} {
   global  HIST_FILE TEMP_FILE hist_name index_list lblstr

   foreach i $index_list {
     set sum($i) 0.0
     set sumsq($i) 0.0
     set std_tmp($i) 0.0
     set averg($i) 0.0
     set std_tmp($i) 0.0
   }
   set linecnt 0
   set histFD [open $HIST_FILE r]
   while {[gets $histFD Line] >= 0} {
     if {([string index [lindex $Line 0] 0] != "#") && ([llength $Line] != 0)} {
       foreach i $index_list {
          set val [lindex $Line $i] 
          if {$val != "No_spec"} {
            set ival [format "%.4f" [lindex $Line $i]]
            set sum($i) [expr $sum($i)+$ival]
            set sumsq($i) [expr $sumsq($i)+($ival*$ival)]  
          }
       }
       incr linecnt
     }
   }
   close $histFD

   if {$linecnt == 1} {
      foreach i $index_list {
        set averg($i) $sum($i)
        set stdev($i) 0
      }
   } else {
        foreach i $index_list {
          set averg($i) [expr $sum($i)/$linecnt]
          set std_tmp($i)  [expr $sumsq($i)-(($sum($i)*$sum($i))/$linecnt)] 
          if {$std_tmp($i) < 0} {
             set std_tmp($i) [expr $std_tmp($i)*(-1)]
          }
          set stdev($i) [expr  sqrt($std_tmp($i)/($linecnt-1))]
        }
     }

   append avgstr [format "%-17s" "#Average:"]
   append stdstr [format "%-17s" "#Std Deviation:"]

   foreach i $index_list {
      append avgstr [format "%12.3f" $averg($i)]
      append stdstr [format "%12.3f" $stdev($i)]
   }

   set tempFD [open $TEMP_FILE w+]
   set histFD [open $HIST_FILE r]

   gets $histFD Line
   puts $tempFD "$Line\n"
   puts $tempFD "$avgstr"
   puts $tempFD "$stdstr\n"
   puts $tempFD "$lblstr"

   while {[gets $histFD Line] >= 0} {
      if {[lindex $Line 0] != "#Average:" && \
          [lindex $Line 0] != "#Std" && \
          [lindex $Line 0] != "#Date" && \
          [llength $Line] != 0} {
         puts $tempFD "$Line"
      }
   }

   close $tempFD
   close $histFD
   exec mv $TEMP_FILE $HIST_FILE
   return 0
}

proc getTableVal {t_file} {
  global TABLE_FILE  tab_test

   set i 1
   set tab_test(num) 0
   set tblFD [open $TABLE_FILE r]
   while {[gets $tblFD Line] >= 0} {
     if {[string index [lindex $Line 0] 0 ]!= "#"} {
        if {[lindex $Line 1] ==  "$t_file"} {
           set tab_test($i,tmacro) [lindex $Line 0]
           set tab_test($i,tname) [lindex $Line 1]
           set tab_test($i,colun) [lindex $Line 2]
           set tab_test($i,lo_limit) [lindex $Line 3]
           set tab_test($i,up_limit) [lindex $Line 4]
           set tab_test($i,tlabel) [lrange $Line 5 end]
           set tab_test(num) $i 
           incr i
        }
     }   
   }  
   close $tblFD
   return 0
}

#----- Main ------------

if { ![file exists $HIST_FILE] } {
   set histFD [open $HIST_FILE a+]
   puts $histFD "#Test: $filelabel\n\n"
} else {
     set histFD [open $HIST_FILE a+] 
}

for {set i 4} {$i <= [expr $argc-2]} {incr i 2} {
   append lblstr [format "%10s " [lindex $argv $i]]
}
set lblstr "[format "%-17s %s" $hisdate $lblstr]"


set t_status "No_spec"
if [ file exists $TABLE_FILE ] { 
   getTableVal $hist_name
   set colcnt 0
   set t_status "Pass"
   set star_cnt 0

   for {set i 5} {$i <= $argc} {incr i 2} {
      incr colcnt
      set hdata [lindex $argv $i]
      append hdatastr [format "%10s" $hdata]
      for {set j 1} {$j <= $tab_test(num)} {incr j} {
         if {$tab_test($j,tname) == [lindex $argv 0] && $tab_test($j,colun) ==  $colcnt} {
            if {$tab_test($j,lo_limit) == "*" || $tab_test($j,up_limit) == "*"} {
               incr star_cnt
               continue
            }
            if {($hdata >= $tab_test($j,lo_limit)) && ($hdata <= $tab_test($j,up_limit))} {
               #do not set "Pass" here, one column failed, the rest of the test failed
               continue
            } else {
                set t_status "Fail"
              }
         }
      }
   }
   if {$star_cnt == $tab_test(num)} {
      set t_status "No_spec"
   }

   append hdatastr [format "%10s" $t_status]

} else {
     for {set i 5} {$i <= $argc} {incr i 2} {
        set hdata [lindex $argv $i]
        append hdatastr [format "%10s" $hdata]
     }
  }

set aa 4 ;#nmr data alway starts at column 4
for {set i 5} {$i <= $argc} {incr i 2} {
   lappend index_list $aa
   incr aa
}
puts $histFD [format "%-17s %s" $udate $hdatastr]
close $histFD
calAvgStd

if {$t_status == "Fail"} {
   set stat "Fail"
   set resFD [open $FAIL_FILE w]
   puts $resFD "Fail"
   close $resFD
} else {  set stat "" }

set limit_name [string range $hist_name 0 14]
set repstr1 [format "%-16s" "${limit_name}:"]
set repstr2 [format "%-16s" " $stat"]

if { ![file exists $REPORT_FILE] } {
   exec cat $REPORT_HEADER > $REPORT_FILE
   set repFD [open $REPORT_FILE a+]
   puts $repFD $udate
   puts $repFD "Run by [lindex $argv 3] on [exec hostname]\n"
} else {
    set repFD [open $REPORT_FILE a+]
}  
     
for {set i 4} {$i <= [expr $argc-2]} {incr i 2} {
  append repstr1 [format "%-11s" "[string range [lindex $argv $i] 0 10]"]
  append repstr2 [format "%-11s" "[lindex $argv [expr $i+1]]"]
}    
puts $repFD "$repstr1"
puts $repFD "$repstr2\n"
close $repFD

