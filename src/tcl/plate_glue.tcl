#! /vnmr/tcl/bin/vnmrWish -f
# 

proc butdown {x y} {
 .plate addtag StWell closest $x $y
}

proc identify {tag} {
 set stNo ""
 set start [.plate gettags $tag]
 scan [lindex $start [lsearch $start X*]] "X%d" stNo
 return $stNo
}
 
proc butup {x y butn} {
 global order cell
 .plate addtag EndWell closest $x $y
 set stNo [identify StWell]
 set endNo [identify EndWell]
 if {$stNo == $endNo} {
  switch $butn {
   1 {set order [concat $order $stNo]}
   2 -
   3 {
      set stx [lsearch $order $stNo]
      set order [lreplace $order $stx $stx]
     }
  }
  showit
 } else {
  if {$butn > 1} {
   set stx [lsearch $order $stNo]
   set endx [lsearch $order $endNo]
   if {$endx >= $stx} {
    set order [lreplace $order $stx $endx]
   } else {
    set order [lreplace $order $endx $stx]
   }    
   showit
   .plate dtag StWell
   .plate dtag EndWell
   .plate dtag Olap
   return
  }   
  set stx $cell(x,$stNo)
  set sty $cell(y,$stNo)
  set endx $cell(x,$endNo)
  set endy $cell(y,$endNo)
  if {$stx > $endx} {
   set incx $stx
   set stx $endx
   set endx $incx
  }
  if {$sty > $endy} {
   set incy $sty
   set sty $endy
   set endy $incy
  }
# build a list running from stNo to endNo

  .plate addtag Olap overlapping $stx $sty $endx $endy
  set ord1 ""
  while {[set next [identify Olap]] != ""} {
   set ord1 [concat $ord1 $next]
   .plate dtag X$next Olap
  }

# is it vert, horiz, or diag

  set ox [expr $cell(x,$stNo) - $cell(x,$endNo)]
  set oy [expr $cell(y,$stNo) - $cell(y,$endNo)]
  set oz [expr $ox + $oy]
  set sw 0
  if {$oz == $ox} {set sw 1}
  if {$oz == $oy} {set sw 1}
  
# calc dist for each from stNo

  set size [llength $ord1]
  for {set i 0} {$i < $size} {incr i} {
   set ord3($i) [lindex $ord1 $i]
   set ox [expr $cell(x,$stNo) - $cell(x,$ord3($i))]
   set oy [expr $cell(y,$stNo) - $cell(y,$ord3($i))]
   if { ($sw == 1) || ([expr abs($ox) - abs($oy)] == 0)} {
    set ord2($i) [expr $ox * $ox + $oy * $oy]
   } else {
    set ord2($i) [expr - ($ox * $ox + $oy * $oy)]
   }
  }
  
  incr size -1
            
# now we can sort
          
  for {set jnd 1} {$jnd <= $size} {incr jnd} {
    set a $ord2($jnd)
    set b $ord3($jnd)
    set ind [expr $jnd - 1]
    while {($ind >= 0) && ($ord2($ind) > $a)} {
      set ord2([expr $ind + 1]) $ord2($ind)
      set ord3([expr $ind + 1]) $ord3($ind)
      incr ind -1
    }
    set ord2([expr $ind + 1]) $a
    set ord3([expr $ind + 1]) $b
  }
# finally stick the +ve wells onto the list
  
  for {set i 0} {$i <= $size} {incr i} {
   if {$ord2($i) >= 0} {
    set order [concat $order $ord3($i)]
   }
  }
 }

 .plate dtag StWell
 .plate dtag EndWell
 .plate dtag Olap
 showit
}

proc showit {} {
 global order cellback cell
 set orderLength [llength $order]
 set cellback white
 .plate itemconfigure Well -fill $cellback
 catch {.plate delete SeqLine}
 set cellback indianred
 if {$orderLength != 0} {
  for {set i 0} {$i < $orderLength} {incr i} {
   set cno [lindex $order $i]
   .plate itemconfigure Spot$cno -fill $cellback
   if {$i == 0} {
    set stx $cell(x,$cno)
    set sty $cell(y,$cno)
   } else {
    set x $cell(x,$cno)
    set y $cell(y,$cno)
    .plate create line \
     $stx $sty $x $y \
     -fill red -width 5 -tags SeqLine
    set stx $x
    set sty $y
   }
  .plate lower SeqLine Spot1
  }
 }
 update
}

proc loadplate {} {
  global order
  set fn [dirview]
  catch {destroy .fdv}
  if { [file readable $fn] } {
    .msg configure -text "                               "
    set file [open $fn "r"]
    gets $file dummy
    gets $file order 
    catch {close $fn}
    showit
  } else {
    .msg configure -text "No such File - $fn" -fg red
  }

}

proc saveplate {} {
  global order char
  toplevel .svscreen
  wm title .svscreen "Save File"
  wm geometry .svscreen -20-190

  label .svscreen.title -text "SAVE DATA"
  frame .svscreen.c
  label .svscreen.c.c1 -text "Current directory: "
  label .svscreen.c.cur -text [pwd]
  pack .svscreen.c.c1 .svscreen.c.cur -side left
  frame .svscreen.d
  label .svscreen.d.d1 -text "New Directory:"
  entry .svscreen.d.dir -width 20 -relief sunken -bd 2 -textvariable newdir
  pack .svscreen.d.d1 .svscreen.d.dir -side left -padx 1m
  pack .svscreen.title .svscreen.c .svscreen.d -side top
  bind .svscreen.d.dir <Key-Return> {
   cd $newdir
   .svscreen.c.cur configure -text $newdir
  }
  frame .svscreen.f -relief groove -borderwidth 4
  frame .svscreen.f.fa
  label .svscreen.f.fa.f1 -text "Enter Filename:"
  entry .svscreen.f.fa.file -width 20 -relief sunken -bd 2 -textvariable fn
  label .svscreen.f.err -text " " -fg red
  pack .svscreen.f.fa.f1 .svscreen.f.fa.file -side left -padx 1m
  pack .svscreen.f.fa -side top
	
  pack .svscreen.f -side top
  bind .svscreen.f.fa.file <Key-Return> {	
   if { [file writable $fn] || ![file exists $fn] } {
    .svscreen.f.err configure -text " "
   } else {
    .svscreen.f.err configure -text "Can't write File"
   }
  }
  frame .svscreen.bu
  button .svscreen.bu.ok -text OK -command {
   if { [file writable $fn] || ![file exists $fn] } {
    .svscreen.f.err configure -text " "
    set file [open $fn "w"]
    set orderLength [llength $order]
    puts $file $orderLength
    puts $file $order 
    set ord1 ""
    for {set i 0} {$i < $orderLength} {incr i} {
     set cu [lindex $order $i]
     set num [expr int(($cu - 1) / 8) + 1]
     set alf [expr ($cu - 1) % 8]
     set cu [lindex $char $alf]$num
     set ord1 [concat $ord1 $cu]
    }
    puts $file $ord1
    flush $file
    catch {close $file}
    showit
    destroy .svscreen
   } else {
    .svscreen.f.err configure -text "Can't write File"
   }
  }	
  button .svscreen.bu.can -text Cancel -command "destroy .svscreen"
  pack .svscreen.bu.ok -side left
  pack .svscreen.bu.can -side right -fill x
  pack .svscreen.bu -side bottom
  set help_tips(.svscreen.bu.ok) {Click to save data}
  set help_tips(.svscreen.bu.can) {Click to cancel save}

}

proc clrplate {} {
 global order
 set order ""
 showit
}


#--------------------------------------------------------
# Start here

option add *Balloon*delay 750
option add *Balloon*font courb10
option add *Balloon*background yellow
catch {option readfile $env(vnmrsystem)/app-defaults/Combiplate}
catch {option readfile $env(HOME)/app-defaults/Combiplate}

set VNMR_TK_DIR $env(TK_LIBRARY)/vnmr
source $VNMR_TK_DIR/dirview.tk

set order ""

if { [catch init_ballons] } {
   source $env(vnmrsystem)/tcl/tklibrary/balloon.tcl
   source $env(vnmrsystem)/tcl/tklibrary/getopt.tcl
   init_balloons
}

file mkdir $env(HOME)/vnmrsys/templates/glue
cd $env(HOME)/vnmrsys/templates/glue

canvas .plate -width 400 -height 600 -bg gray90

#each well is 50 x 50

set char {H G F E D C B A}
for {set i 0} {$i < 8} {incr i} {
  for {set j 0} {$j < 12} {incr j} {
    set cellabel [lindex $char $i][expr $j + 1]
    set x1 [expr $i * 50]
    set y1 [expr $j * 50]
    set x2 [expr $x1 + 49]
    set y2 [expr $y1 + 49]
    set cellNo [expr ($j * 8) + $i + 1]
    .plate create rectangle \
      $x1 $y1 \
      $x2 $y2 \
      -tags "Cell$cellNo AllWells X$cellNo"
    set xx1 [expr $x1 + 10]
    set yy1 [expr $y1 + 10]
    set xx2 [expr $x2 - 10]
    set yy2 [expr $y2 - 10]
    set cenx [expr $x1 + 25]
    set ceny [expr $y1 + 25]
    set cell(x,$cellNo) $cenx
    set cell(y,$cellNo) $ceny
    set cellback white
    .plate create oval \
      $xx1 $yy1 $xx2 $yy2\
      -fill $cellback -tags "Spot$cellNo Well W X$cellNo"
    .plate create text \
      $cenx $ceny  \
      -text $cellabel -anchor center -tags "SpotText$cellNo W X$cellNo" \
       -fill blue
  }
}

.plate bind W <ButtonPress-1> {butdown %x %y}
.plate bind W <ButtonPress-3> {butdown %x %y}
.plate bind W <ButtonRelease-1> {butup %x %y 1}
.plate bind W <ButtonRelease-3> {butup %x %y 3}
update

#Do the action buttons
frame .buts -relief groove -borderwidth 2
button .buts.load -text "Load..." -command loadplate
button .buts.save -text "Save..." -command saveplate
button .buts.show -text "Show" -command showit
button .buts.clear -text Clear -command {clrplate}
button .buts.quit -text Exit -command {
 catch {close $fn}
 exit
}
pack .buts.load .buts.save .buts.show .buts.clear \
 -side left -padx 2
pack .buts.quit -side right -padx 2
label .msg -text "  "

set help_tips(.buts.load) {Load existing template}
set help_tips(.buts.show) {Refresh display}
set help_tips(.buts.quit) {Exit}
set help_tips(.buts.save) {Save current as new template}
set help_tips(.buts.clear) {Clear plate}

pack .plate -side top
pack .msg -side top
pack .buts -side bottom -fill x


