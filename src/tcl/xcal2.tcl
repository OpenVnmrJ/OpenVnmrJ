#!/vnmr/tcl/bin/vnmrWish -f
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

# Adapted from xcal by <m.dsouza@mrc-apu.cam.ac.uk>

# CHANGE THIS LINE BELOW TO POINT TO THE STANDARD X11 BITMAP DIRECTORY

catch {destroy .xcal}
set bitmapdir "/usr/openwin/include/X11/bitmaps/"
if { ([exec uname -s] == "Linux") } {
   set bitmapdir         $env(vnmrsystem)/tcl/tklibrary/vnmr
}

wm withdraw .
toplevel .xcal

   wm title .xcal "Select Date"
   wm iconname .xcal "CALENDAR"


set h_file ""

set hols " "


set date_array [clock format [clock scan today] -format "%Y %m %d"]
set year [lindex $date_array 0]
set this_year $year
set month [lindex $date_array 1]
regexp {0([1-9])} $month m month
set this_month [string trimleft $month 0]
set this_day [lindex $date_array 2]
set day $this_day

set months \
	{{} January February March April May June July August September October \
	November December }

set days \
	{Sun Mon Tue Wed Thu Fri Sat}

set lastDate ""
set retVal ""


proc datesel {d m y pos} {
        global date_moused months lastDate temp retVal
        set date_moused " "
        set retVal "$y $m $d"
        append date_moused [string range [lindex $months $m] 0 2]
        append date_moused " "
        append date_moused $d
        append date_moused ", "
        append date_moused $y
        if {[winfo exists $lastDate]}  {
          $lastDate configure -fg black
        }
        if {[winfo exists $pos.num]}  {
          $pos.num configure -fg red
          set lastDate $pos.num
        }
}

proc hit {day} {
  global hols month year months date_moused
  set date " [string range [lindex $months $month] 0 2] $day, $year"
  if {$date == $date_moused} {
    return 1
  } else {
    return 0
  }

}

proc make_holiday {} {
   global retVal
   puts "$retVal"
   destroy .xcal
   exit
}

proc getcal {} {
	global cal_output
	global month
	global year
	global months

        set month [string trimleft $month 0]
	if {$month > 12} then {set month 1; incr year}
	if {$month < 1 } then {set month 12; incr year -1}
	
	set cal_output [lrange [split [exec cal $month $year] "\n"] 2 7]
	for {set i 0} {$i<5} {incr i} {
		regsub -all {..(.|$)} [lindex $cal_output $i] &: a
		set cal_output [lreplace $cal_output $i $i [split $a ":"]]
	}
	.xcal.header configure  -text "[lindex $months $month] $year"
}

proc drawcal {} {
  global cal_output
  global this_year
  global this_month
  global this_day
  global month
  global year
  global bitmapdir
  global _back

  set _back {gray80 gray90 gray90 gray90 gray90 gray90 gray80}

  for {set y1 0} {$y1<=5} {incr y1} {
    for {set x 0} {$x<7} {incr x} {
      set y [expr $y1%5]
      set day [string trim [lindex [lindex $cal_output [expr $y1]] $x]]
      if { $day == "" && $y1 == 5}  break
      if {![winfo exists .xcal.line$y.col$x]} {
        frame .xcal.line$y.col$x -height 50 -width 50 
        if {$x == 0 || $x == 6} then {
          .xcal.line$y.col$x configure -back [lindex $_back $x]
        } else {
          .xcal.line$y.col$x configure -back [lindex $_back $x]
        }
      }

      if { $day != "" }  {
        if {[hit $day]} {
          set _fontcolor red
        } else {
          set _fontcolor black
        }
        if { $day == $this_day && $month == $this_month && $year == $this_year }  {
          set _font "-adobe-new century schoolbook-*-i-*-*-24-*-*-*-*-*-*-*"
          set _color gray95
        } else {
          set _font "-adobe-new century schoolbook-*-i-*-*-14-*-*-*-*-*-*-*"
          set _color [lindex $_back $x]
        }
        if {[winfo exists .xcal.line$y.col$x.num]}  {
	  .xcal.line$y.col$x.num configure -text $day \
	    -font $_font -bg $_color \
	    -command "datesel $day $month $year .xcal.line$y.col$x" \
	    -fg $_fontcolor				
	} else {
	  button .xcal.line$y.col$x.num -text $day \
	    -font $_font -bg $_color -relief flat \
	    -activebackground pink -highlightthickness 0 \
	    -command "datesel $day $month $year .xcal.line$y.col$x" \
	    -fg $_fontcolor

	  place .xcal.line$y.col$x.num -relx 0 -rely 0 \
				
	}
      } else {
	if {[ winfo exists .xcal.line$y.col$x.num]} {
	  destroy .xcal.line$y.col$x.num
	}
      }
    }
  }
}


for {set y 0} {$y<5} {incr y} {frame .xcal.line$y}
frame .xcal.top
frame .xcal.days
frame .xcal.buttons
for {set x 0} {$x<7} {incr x} {
	message .xcal.days.$x -relief sunken -text [lindex $days $x] -width 50 -relief groove \
	        -bg gray90
	pack append .xcal.days .xcal.days.$x {left padx 20 pady 20}
}

message .xcal.header  -relief sunken -padx 10 -pady 10 \
	-fore blue -width 300  \
	-font "-adobe-new century schoolbook-*-i-*-*-24-*-*-*-*-*-*-*"

set date_moused ""
getcal
drawcal


button .xcal.quit -text "Quit" -command {
        puts ""; destroy .xcal; exit
}
button .xcal.today -text "Today" -command {
		set month $this_month
		set year $this_year
		set day $this_day
		getcal; drawcal}
button .xcal.hol -text "OK" -command {make_holiday}

button .xcal.prev -bitmap @$bitmapdir/Left \
	-command {incr month -1; getcal; drawcal}
button .xcal.next -bitmap @$bitmapdir/Right \
	-command {incr month; getcal; drawcal}

for {set y 0} {$y<5} {incr y} {
        for {set x 0} {$x<7} {incr x} {
             pack append .xcal.line$y .xcal.line$y.col$x {left padx 5 pady 5}
	}
	pack append .xcal .xcal.line$y {top}
}



pack append .xcal.top .xcal.header {top fillx padx 10 pady 10}

place .xcal.prev  -relx 0 -rely 0 -anchor nw
place .xcal.next  -relx 1 -rely 0 -anchor ne

pack before .xcal.line0 .xcal.top {top padx 10 pady 10}
pack before .xcal.line0 .xcal.days {top}
pack append .xcal.buttons .xcal.hol {left fillx expand} \
	.xcal.quit {left fillx expand} .xcal.today {left fillx expand}
pack append .xcal .xcal.buttons {top}
label .xcal.reslabel -textvariable date_moused
pack .xcal.reslabel -side bottom
