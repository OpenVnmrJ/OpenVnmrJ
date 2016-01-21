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
#atregbuilt hist_full_filename column# nmrvar1 nmrvar2 reg_file_dest vnmraddr
#atregbuilt "$AT_DIR/history/H1RFHOMO" 2 at_avrg at_stdev "/export/home/vnmr1/vnmrsys/exp1" vnmraddr

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


if { $argc != 6 } {
  return
}

set hist_fullname [lindex $argv 0]
set hcoln [lindex $argv 1]
set destdir [lindex $argv 4]
set AT_DIR        $env(vnmruser)/autotest

set osType [exec uname -s]
if { $osType == "Interix"} {
   set AT_DIR [winname $AT_DIR]
   set destdir [winname $destdir]
}

set TABLE_FILE    $AT_DIR/atdb/at_spec_table

exec rm -f ${destdir}/analyze.inp ${destdir}/analyze.out ${destdir}/regression.inp \
        ${destdir}/analyze.list  ${destdir}/expl.out ${destdir}/fp.out ${destdir}/avrgstdev.inp

set REG_FILE ${destdir}/regression.inp
set TEMP_FILE ${destdir}/avrgstdev.inp

proc getLabel {tfile tcol} {
  global TABLE_FILE

   set ret_val 0
   set tblFD [open $TABLE_FILE r]
   while {[gets $tblFD Line] >= 0} {
     if {[lindex $Line 1] == "$tfile" && [lindex $Line 2] == "$tcol"} {
        set ret_val [string range [lrange $Line 5 end] 0 18]
        break
     }   
   }    
   close $tblFD
   return $ret_val 
}

set linecnt 0
set h_glabel [getLabel [file tail $hist_fullname] $hcoln]
set regFD [open $REG_FILE w]
puts $regFD "Trial #"
puts $regFD $h_glabel

set histFD [open $hist_fullname r]
while {[gets $histFD Line] >= 0} {

   if {[lindex $Line 0] == "#Average:"} {
      set h_avrg [lindex $Line $hcoln]
      continue
   }

   if {[lindex $Line 0] == "#Std"} {
      set h_stdev [lindex $Line [expr $hcoln+1]]
      continue
   }

   if {([string index [lindex $Line 0] 0] != "#") && ([llength $Line] != 0)} {
      incr linecnt
      set tmp($linecnt) [lindex $Line [expr $hcoln+3]]
   }
}

set tmpFD [open $TEMP_FILE w]
   puts $tmpFD "average  $h_avrg"
   puts $tmpFD "stdev $h_stdev"
   puts $tmpFD "glabel $h_glabel"
close $tmpFD

puts $regFD "1     $linecnt"

for {set i 1} {$i<=$linecnt} {incr i} {
   puts $regFD "       $i  $tmp($i)"
}

close $regFD
close $histFD
