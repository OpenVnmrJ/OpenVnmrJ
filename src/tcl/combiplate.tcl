#! /bin/sh
# 
# Re execute this file under Tcl.
# The next line is executed by "sh" but it is a comment to Tcl. \
        exec $vnmrsystem/tcl/bin/vnmrwish "$0" ${1+"$@"}

# plate analysis and display

proc subWindowsOf {win} {
    set rtnList {}
    set n [llength $win]
    for {set ix 0} {$ix < $n} {incr ix} {
	set rtnList [concat $rtnList [winfo children [lindex $win $ix]]]
    }
    return $rtnList
}

proc flashWindow {window {color black} {ntimes 2} {delay 200}} {
    # Make a list of all subwindows
    set winList $window
    set subwinList [subWindowsOf $winList]
    while {[llength $subwinList]} {
	set winList [concat $winList $subwinList]
	set subwinList [subWindowsOf $subwinList]
    }

    # Remember the original background colors
    set nwins [llength $winList]
    for {set iw 0} {$iw < $nwins} {incr iw} {
	set win [lindex $winList $iw]
	set oldColor($iw) [$win cget -bg]
    }

    # Flash the window and all subwindows
    for {set ix 1} {$ix <= $ntimes} {incr ix} {
	for {set iw 0} {$iw < $nwins} {incr iw} {
	    set win [lindex $winList $iw]
	    $win config -bg $color
	}
	update idletasks
	after $delay
	for {set iw 0} {$iw < $nwins} {incr iw} {
	    set win [lindex $winList $iw]
	    $win config -bg $oldColor($iw)
	}
	update idletasks
	if {$ix < $ntimes} { after $delay }
    }
}

proc saveplate {} {

    upvar #0 rval rval
    upvar #0 gval gval
    upvar #0 bval bval
    global redsav grnsav blusav fn

    if { [winfo exists .svscreen] } {
	wm deiconify .svscreen
	raise .svscreen
	flashWindow .svscreen
    } else {
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
	frame .svscreen.f.fb
	label .svscreen.f.fa.f1 -text "Enter Filename:"
	entry .svscreen.f.fa.file -width 20 -relief sunken -bd 2 -textvariable fn
	checkbutton .svscreen.f.fb.c1 -text "Save red" -variable redsav -fg red
	checkbutton .svscreen.f.fb.c2 -text "Save green" -variable grnsav -fg green
	checkbutton .svscreen.f.fb.c3 -text "Save blue" -variable blusav -fg blue
	label .svscreen.f.err -text " " -fg red
	pack .svscreen.f.fa.f1 .svscreen.f.fa.file -side left -padx 1m
	pack .svscreen.f.fb.c1 .svscreen.f.fb.c2 \
		.svscreen.f.fb.c3 -side left -padx 1m
	pack .svscreen.f.fa .svscreen.f.fb .svscreen.f.err -side top
	
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
		if { ! $redsav && ! $grnsav && ! $blusav} {
		    .svscreen.f.err configure -text "No colors selected "
		} else {
		    set fd [open $fn "w"]
		    if {$redsav == 1} {
			puts $fd rval
			puts $fd [array get rval]
		    }
		    if {$grnsav == 1} {
			puts $fd gval
			puts $fd [array get gval]
		    }
		    if {$blusav == 1} {
			puts $fd bval
			puts $fd [array get bval]
		    }
		    close $fd
		    destroy .svscreen
		}
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
}

proc clrplate {} {

    upvar #0 rval rval
    upvar #0 gval gval
    upvar #0 bval bval

    for {set i 0} {$i < 8} {incr i} {
	for {set j 0} {$j < 12} {incr j} {
	    set rval($i,$j) 0
	    set gval($i,$j) 0
	    set bval($i,$j) 0
	}
    }
    colrmap
    repaint
}

proc repaint {} {
    upvar #0 color color
    global char cfail
    set cfail 0
    for {set i 0} {$i < 8} {incr i} {
	for {set j 0} {$j < 12} {incr j} {
	    set cell [lindex $char $i]$j
	    set cellabel [lindex $char $i][expr $j + 1]
	    set cellback [format "#%2.2x%2.2x%2.2x" \
		    $color($i,$j,r) $color($i,$j,g) $color($i,$j,b)]
	    .plate.cell$cell configure -bg $cellback
	    set cfail [expr [winfo colormapfull .] + $cfail]

	}
    }
    if {$cfail > 0} {
	.results.raw.text insert end \
		"Expect some color losses!" redtag "\n"
    }
}

proc loadfile {} {
    global sets type units file fn opos
    upvar #0 range range

    .results.raw.text insert end "--------------------------\n--------------------------\n"
    if { [file readable $fn] } {
	.lds.ldscreen.f.err configure -text "                               "
	set file [open $fn "r"]
	gets $file header
	set sets [lindex $header 0]
	set type [lindex $header 1]
	set units [lindex $header end]
	for {set i 1} {$i <= $sets} {incr i} {
	    gets $file header
	    set range($i,st) [lindex $header 1]
	    set range($i,end) [lindex $header end]
	    .results.raw.text insert end \
		    "$type region $i - $range($i,st) to $range($i,end) $units\n"
	}
	gets $file header  ;# dummy read
	set opos [tell $file]
	.results.raw.text insert end "--------------------------\n"
	.results.raw.text see end
    } else {
	.lds.ldscreen.f.err configure -text "No such File - $fn" -fg red
    }
}

proc contents {i j} {
    upvar #0 color color
    upvar #0 rval rval
    upvar #0 gval gval
    upvar #0 bval bval
    global curi curj
    global rfield gfield bfield
    set char {h g f e d c b a}
    set red [format "%4.0f" [expr $color($i,$j,r) / 255.0 * 100.0]]
    set green [format "%4.0f" [expr $color($i,$j,g) / 255.0 * 100.0]]
    set blue [format "%4.0f" [expr $color($i,$j,b) / 255.0 * 100.0]]
    set curi $i
    set curj $j
    set cellabel [lindex $char $i][expr $j + 1]
    
    .results.wt.red delete 0 end
    .results.wt.green delete 0 end
    .results.wt.blue delete 0 end

    .results.wt.red insert end $red
    .results.wt.green insert end $green
    .results.wt.blue insert end $blue
    .results.title1 configure -text "Current cell - $cellabel"
    .results.raw.text insert end \
	    "Cell $cellabel\; $rval($i,$j), $gval($i,$j), $bval($i,$j)\n"
    .results.raw.text see end

    #send details to vnmr

    set spect [expr ($j * 8) + $i + 1]
    vnmrsend "combds($spect,$rfield,$gfield,$bfield)"
}


proc loadplate {} {
    if { [winfo exists .lds] } {
	wm deiconify .lds
	raise .lds
	flashWindow .lds
    } else {
	toplevel .lds
	wm title .lds "Input screen"
	wm geometry .lds -20-190
	
	global source
	label .lds.title -text "FILE INPUT"
	label .lds.t1 -text "Choose Input Type:"
	frame .lds.a
	radiobutton .lds.a.r1 -text "From VNMR" \
		-variable source -value 0 -anchor w -command {
	    		catch [destroy .relscreen]
	   		vnmrplate
			}
	radiobutton .lds.a.r2 -text "Reload" \
		-variable source -value 1 -anchor w -command {
	    		catch [destroy .lds.ldscreen]
	    		reloadp
			}
	pack .lds.a.r1 .lds.a.r2 -side left -padx 1m
	pack .lds.title .lds.t1 .lds.a -side top
    }
}

proc okfile { fn} {
    if { [file readable $fn] } {
   	set v 1
	.lds.rlscreen.f.err configure -text "                               "
	.results.raw.text insert end \
		"--------------------------\n------ Reload data -------\n"
    } else {
	.lds.rlscreen.f.err configure -text "No such File" -fg red
	set v 0
    }
    return $v
}

proc reloadp {} {
    global fn source
    set fn [dirview]
    catch {destroy .fdv}
    catch {destroy .lds.ldscreen}
    catch {destroy .lds.rlscreen}
    if {[string length $fn] == 0} return
    frame .lds.rlscreen
    label .lds.rlscreen.title -text "INPUT FILE "
    frame .lds.rlscreen.c
    label .lds.rlscreen.c.c1 -text "Current directory:  "
    label .lds.rlscreen.c.cur -text [pwd] -width 40 -anchor w
    pack .lds.rlscreen.c.c1 .lds.rlscreen.c.cur -side left -padx 1m
    pack .lds.rlscreen.title .lds.rlscreen.c -side top
    frame .lds.rlscreen.f
    label .lds.rlscreen.f.err -text "                               "
    frame .lds.rlscreen.f.fa
    label .lds.rlscreen.f.fa.f1 -text "Filename: $fn"
    pack .lds.rlscreen.f.fa.f1 -side left
    pack .lds.rlscreen.f.fa .lds.rlscreen.f.err -side top -anchor w
    pack .lds.rlscreen.f -side top -anchor w
    pack .lds.rlscreen -side top
    frame .lds.rlscreen.bu
    button .lds.rlscreen.bu.ok -text OK -command {
	if {[okfile $fn]} {
	    set file [open $fn "r"]
	    while {[gets $file arr] >= 0} {
		gets $file val
		array set $arr $val
		if {$arr == "rval"} {
		    .r.crit.d3 configure -text "  Data from file $fn"
		}
		if {$arr == "gval"} {
		    .g.crit.d3 configure -text "  Data from file $fn"
		}
		if {$arr == "bval"} {
		    .b.crit.d3 configure -text "  Data from file $fn"
		}
	    }
	    close $file
	    destroy .lds
	}
    }
    button .lds.rlscreen.bu.can -text cancel -command {destroy .lds.rlscreen}
    pack .lds.rlscreen.bu.ok -side left
    pack .lds.rlscreen.bu.can -side right -fill x
    pack .lds.rlscreen.bu -side bottom
    set help_tips(.lds.rlscreen.bu.ok) {Click to load data}
    set help_tips(.lds.rlscreen.bu.can) {Click to cancel load}
}

proc vnmrplate {} {
    global sets type units file fn startcol endcol rows source
    upvar #0 range range
    set fn [dirview]
    catch {destroy .fdv}
    catch {destroy .lds.rlscreen}
    catch {destroy .lds.ldscreen}
    if {[string length $fn] == 0} return
    frame .lds.ldscreen
    label .lds.ldscreen.title -text "INPUT FILE "
    frame .lds.ldscreen.c
    label .lds.ldscreen.c.c1 -text "Current directory: "
    label .lds.ldscreen.c.cur -text [pwd] -width 40 -anchor w
    pack .lds.ldscreen.c.c1 .lds.ldscreen.c.cur -side left -padx 1m
    pack .lds.ldscreen.title .lds.ldscreen.c -side top

    frame .lds.ldscreen.f -relief groove -borderwidth 2
    frame .lds.ldscreen.f.fb
    frame .lds.ldscreen.f.fc
    label .lds.ldscreen.f1 -text "Filename:$fn"
    label .lds.ldscreen.f.fb.c1 -text "Start column "
    entry .lds.ldscreen.f.fb.c1v -width 2 -relief sunken -bd 2 \
	    -textvariable startcol
    label .lds.ldscreen.f.fb.c2 -text "  End column "
    entry .lds.ldscreen.f.fb.c2v -width 2 -relief sunken -bd 2 \
	    -textvariable endcol
    label .lds.ldscreen.f.fc.r2 -text "Rows: "
    entry .lds.ldscreen.f.fc.r2v -width 2 -relief sunken -bd 2 \
	    -textvariable rows
    label .lds.ldscreen.f.err -text "                               "
    pack .lds.ldscreen.f1 -side left -padx 1m
    pack .lds.ldscreen.f.fb.c1 .lds.ldscreen.f.fb.c1v \
	    .lds.ldscreen.f.fb.c2 .lds.ldscreen.f.fb.c2v \
	    -side left -padx 1m
    pack .lds.ldscreen.f.fc.r2 .lds.ldscreen.f.fc.r2v \
	    -side left -padx 1m
    pack .lds.ldscreen.f.fb .lds.ldscreen.f.fc .lds.ldscreen.f.err \
	    -anchor w -side top
    pack .lds.ldscreen.f -side top
    pack .lds.ldscreen -side top

    loadfile
    frame .lds.ldscreen.bu
    button .lds.ldscreen.bu.ok -text OK -command "destroy .lds"
    button .lds.ldscreen.bu.can -text cancel \
	    -command ".results.raw.text delete 1.0 end
    			catch {close $fn}
    			destroy .lds.ldscreen"
    pack .lds.ldscreen.bu.ok -side left
    pack .lds.ldscreen.bu.can -side right -fill x
    pack .lds.ldscreen.bu -side bottom
    set help_tips(.lds.ldscreen.bu.ok) {Click to load data}
    set help_tips(.lds.ldscreen.bu.can) {Click to cancel load}
}

proc doit {} {
    upvar #0 rval rval
    upvar #0 gval gval
    upvar #0 bval bval
    global file rfield gfield bfield colst colend rows
    global sets char startcol endcol opos

    seek $file $opos start
    set colst [lsearch $char $startcol]
    set colend [lsearch $char $endcol]
    for {set j 0} {$j < $rows} {incr j} {
	for {set i $colst} {$i <= $colend} {incr i} {
	    gets $file dat
	    for {set k 1} {$k <= $sets} {incr k} {
		gets $file dat
		set item [lindex $dat 0]
		set value [lindex $dat 1]
		if {$value < 0.0} {set value 0.0}
		if {$item == $rfield} {set rval($i,$j) $value}
		if {$item == $gfield} {set gval($i,$j) $value}
		if {$item == $bfield} {set bval($i,$j) $value}
	    }
	}

}   }

proc colrmap {} {
    global redMax greenMax blueMax
    upvar #0 rval rval
    upvar #0 gval gval
    upvar #0 bval bval
    upvar #0 color color

    for {set i 0} {$i < 8} {incr i} {
	for {set j 0} {$j < 12} {incr j} {
	    if {$redMax == 1} {
		set rcol [expr $rval($i,$j) * 256.0 / pow(10.0,[.r.slide.scale get]) ]
	    } else {
		set rcol $redMax
	    }
	    if {$rcol > 255} {set rcol 255}
	    
	    if {$greenMax == 1} {
		set gcol [expr $gval($i,$j) * 256.0 / pow(10.0,[.g.slide.scale get]) ]
	    } else {
		set gcol $greenMax
	    }
	    if {$gcol > 255} {set gcol 255}

	    if {$blueMax == 1} {
		set bcol [expr $bval($i,$j) * 256.0 / pow(10.0,[.b.slide.scale get]) ]
	    } else {
		set bcol $blueMax
	    }
	    if {$bcol > 255} {set bcol 255}

	    set color($i,$j,r) [expr int($rcol)]
	    set color($i,$j,g) [expr int($gcol)]
	    set color($i,$j,b) [expr int($bcol)]
	}
    }
}

proc showHelp {} {
    global VNMR_TK_DIR Line

    if { [winfo exists .start] } {
	wm deiconify .start
	raise .start
	flashWindow .start
    } else {
	toplevel .start
	wm title .start "Welcome!"
	wm geometry .start -20-190
	frame .start.txt
	frame .start.but
	text .start.txt.t -yscrollcommand ".start.txt.s set" \
		-height 20 -width 45 -bd 2 -relief sunken -bg gray90
	scrollbar .start.txt.s -command ".start.txt.t yview"
	pack .start.txt.s -side right -fill y
	pack .start.txt.t -side left -fill y
	button .start.but.o -text "OK" -command "unset Line ; destroy .start"
	pack .start.but.o -side top -anchor n
	pack .start.txt -side top -fill y -expand 1
	pack .start.but -side top
	set fn1 "$VNMR_TK_DIR/combi.help"
	set file [open $fn1 "r"]
	while {[gets $file Line] >= 0} {
	    .start.txt.t insert end "$Line\n"
	}
	close $file
    }
}

#we start here ----------------------------------------------------------------

option add *Balloon*delay 750
option add *Balloon*font courb10
option add *Balloon*background lightyellow
catch {option readfile $env(vnmrsystem)/app-defaults/Combiplate}
catch {option readfile $env(HOME)/app-defaults/Combiplate}

set VNMR_TK_DIR $env(TK_LIBRARY)/vnmr
source $VNMR_TK_DIR/dirview.tk

set i [lindex $argv 0]
set j [lindex $argv 1]
set vnmraddr "\"$i $j\""
vnmrinit $vnmraddr $env(vnmrsystem)

if { [catch init_ballons] } {
   source $env(vnmrsystem)/tcl/tklibrary/balloon.tcl
   source $env(vnmrsystem)/tcl/tklibrary/getopt.tcl
   init_balloons
}
enable_balloon Radiobutton

set redMax 1
set greenMax 1
set blueMax 1
set sets 1
set type Integral
set units (ppm)
set file ""
set fn "integ.out"
set range(1,st) 11.0
set range(1,end) 0.0
set color(0,0,r) 255
set char {h g f e d c b a}
set red "    "
set green "    "
set blue "    "
set rv 1.2
set gv 1.2
set bv 1.2
set rfield 0
set gfield 0
set bfield 0
set curi 0
set curj 0
set font courb10
set startcol h
set endcol a
set rows 12
set colst 0
set colend 7
set source 0
set opos 0


#manage main window
wm title . "CombiPlate"
wm geometry . -10-150
wm iconbitmap . @$VNMR_TK_DIR/combi.m.xbm

#dummy up some results
for {set i 0} {$i < 8} {incr i} {
    for {set j 0} {$j < 12} {incr j} {
	set rco [expr (($i + 1) * 2) ]
	set gco 255
	set bco [expr round((($j + 1) * 16.0/12.0) )]
	set rval($i,$j) $rco
	set gval($i,$j) $gco
	set bval($i,$j) $bco
    }
}

# Lets do the control panes
frame .r -relief groove -borderwidth 2
frame .g -relief groove -borderwidth 2

frame .b -relief groove -borderwidth 2

# Red 1st
frame .r.left -relief groove -borderwidth 4 -bg red
radiobutton .r.left.zero -text "0%" -variable redMax -value 0 -anchor w
radiobutton .r.left.hundred -text "100%" -variable redMax -value 255 -anchor w
radiobutton .r.left.var -text "Variable" -variable redMax -value 1 -anchor w
pack .r.left.zero .r.left.hundred .r.left.var -side top -fill x
set help_tips(.r.left.zero) "Red off"
set help_tips(.r.left.hundred) "Red full on"
set help_tips(.r.left.var) "Red variable"


frame .r.slide
scale .r.slide.scale -label "log(100%) has a value of ..." \
	-from -2 -to 6 -length 8c -orient horizontal \
	-resolution 0.001 -tickinterval 2 -digits 4 -variable rv
pack .r.slide.scale

frame .r.crit
frame .r.crit.d
label .r.crit.d.l1 -text "Field selected for Red: "
entry .r.crit.d.e1 -width 4 -relief sunken -bd 2 -textvariable rfield
pack .r.crit.d.l1 .r.crit.d.e1 -side left -padx 2m
label .r.crit.d3 -text "                    "
pack .r.crit.d  .r.crit.d3 -side top -anchor w
bind .r.crit.d.e1 <Key-Return> {
    .r.crit.d3 configure \
	    -text "$type $range($rfield,st) to $range($rfield,end) $units"
    doit
}

pack .r.left -side left -anchor n
pack .r.crit .r.slide -side top -anchor w

# next green
frame .g.left -relief groove -borderwidth 4 -bg green
radiobutton .g.left.zero -text "0%" -variable greenMax -value 0 -anchor w
radiobutton .g.left.hundred -text "100%" -variable greenMax -value 255 -anchor w
radiobutton .g.left.var -text "Variable" -variable greenMax -value 1 -anchor w
pack .g.left.zero .g.left.hundred .g.left.var -side top -fill x
set help_tips(.g.left.zero) "Green off"
set help_tips(.g.left.hundred) "Green full on"
set help_tips(.g.left.var) "Green variable"


frame .g.slide
scale .g.slide.scale -label "log(100%) has a value of ..." \
	-from -2 -to 6 -length 8c -orient horizontal \
	-resolution 0.001 -tickinterval 2 -digits 4 -variable gv

pack .g.slide.scale

frame .g.crit
frame .g.crit.d
label .g.crit.d.l1 -text "Field selected for Green: "
entry .g.crit.d.e1 -width 4 -relief sunken -bd 2 -textvariable gfield
pack .g.crit.d.l1 .g.crit.d.e1 -side left -padx 2m
label .g.crit.d3 -text "                    "
pack .g.crit.d  .g.crit.d3 -side top -anchor w
bind .g.crit.d.e1 <Key-Return> {
    .g.crit.d3 configure \
	    -text "$type $range($gfield,st) to $range($gfield,end) $units"
    doit
}

pack .g.left -side left -anchor n
pack .g.crit .g.slide -side top -anchor w

#finally blue
frame .b.left -relief groove -borderwidth 4 -bg blue
radiobutton .b.left.zero -text "0%" -variable blueMax -value 0 -anchor w
radiobutton .b.left.hundred -text "100%" -variable blueMax -value 255 -anchor w
radiobutton .b.left.var -text "Variable" -variable blueMax -value 1 -anchor w
pack .b.left.zero .b.left.hundred .b.left.var -side top -fill x
set help_tips(.b.left.zero) "Blue off"
set help_tips(.b.left.hundred) "Blue full on"
set help_tips(.b.left.var) "Blue variable"


frame .b.slide
scale .b.slide.scale -label "log(100%) has a value of ..." \
	-from -2 -to 6 -length 8c -orient horizontal \
	-resolution 0.001 -tickinterval 2 -digits 4 -variable bv
pack .b.slide.scale

frame .b.crit
frame .b.crit.d
label .b.crit.d.l1 -text "Field selected for Blue: "
entry .b.crit.d.e1 -width 4 -relief sunken -bd 2 -textvariable bfield
pack .b.crit.d.l1 .b.crit.d.e1 -side left -padx 2m
label .b.crit.d3 -text "                    "
pack .b.crit.d  .b.crit.d3 -side top -anchor w
bind .b.crit.d.e1 <Key-Return> {
    .b.crit.d3 configure \
	    -text "$type $range($bfield,st) to $range($bfield,end) $units"
    doit
}
pack .b.left -side left -anchor n
pack .b.crit .b.slide -side top -anchor w

#Next the results

frame .results
label .results.title1 -text "Current cell     "
frame .results.wt
label .results.wt.title -text "Weight - Red "
label .results.wt.b -text "Blue"
label .results.wt.g -text "Green"
entry .results.wt.red -textvariable red -width 4 -relief sunken -borderwidth 2
entry .results.wt.green -textvariable green -width 4 -relief sunken -borderwidth 2
entry .results.wt.blue -textvariable blue -width 4 -relief sunken -borderwidth 2
pack .results.wt.title .results.wt.red .results.wt.g .results.wt.green .results.wt.b \
	.results.wt.blue -side left
bind .results.wt.red <Key-Return> {
    set rv [expr log10({$rval($curi,$curj)} * 100.0 / [.results.wt.red get])]
    colrmap ; repaint
}
bind .results.wt.green <Key-Return> {
    set gv [expr log10({$gval($curi,$curj)} * 100.0 / [.results.wt.green get])]
    colrmap ; repaint
}
bind .results.wt.blue <Key-Return> {
    set bv [expr log10({$bval($curi,$curj)} * 100.0 / [.results.wt.blue get])]
    colrmap ; repaint
}

frame .results.raw
label .results.raw.title -text "Raw Data"
text .results.raw.text -yscrollcommand ".results.raw.scrl set" -height 5 -width 40 \
	-borderwidth 2 -relief sunken
scrollbar .results.raw.scrl -command ".results.raw.text yview"
pack .results.raw.title -side top
pack .results.raw.scrl -side right -fill y
pack .results.raw.text -side left

pack .results.title1 .results.wt .results.raw -side top

#Do the action buttons
frame .buttons -relief groove -borderwidth 2
button .buttons.load -text "Load..." -command loadplate
button .buttons.save -text "Save..." -command saveplate
button .buttons.display -text Display -command {colrmap ; repaint}
button .buttons.clear -text Clear -command {clrplate}
button .buttons.help -text Help -command {showHelp}
button .buttons.quit -text Exit -command {
    catch {close $fn}
    exit
}
pack .buttons.load .buttons.save .buttons.display .buttons.clear .buttons.help -side left -padx 2
pack .buttons.quit -side right -padx 2

set help_tips(.buttons.load) {Load new data}
set help_tips(.buttons.display) {Refresh}
set help_tips(.buttons.quit) {Exit}
set help_tips(.buttons.save) {Save current}
set help_tips(.buttons.help) {Help!}
set help_tips(.buttons.clear) {Clear all wells}

# 96 well plate display

colrmap

set cfail 0
# set up the grid
frame .plate -relief raised -borderwidth 4
set char {h g f e d c b a}
for {set i 0} {$i < 8} {incr i} {
    grid columnconfigure .plate $i -minsize 0 -weight 0
    for {set j 0} {$j < 12} {incr j} {
	set cell [lindex $char $i]$j
	set cellabel [lindex $char $i][expr $j + 1]
	set cellback [format "#%2.2x%2.2x%2.2x" \
		$color($i,$j,r) $color($i,$j,g) $color($i,$j,b)]
	button .plate.cell$cell -text $cellabel -height 1 -width 1 -bg $cellback \
		-command "contents $i $j"
	set cfail [expr [winfo colormapfull .] + $cfail]
	grid .plate.cell$cell -row $j -column $i -ipady 4
	set help_tips(.plate.cell$cell) "Show values"
    }
}
pack .buttons -side bottom -fill x
pack .plate -side left
pack .r .g .b -side top
pack .results -side top

#colors ok?
.results.raw.text tag configure redtag -foreg red
if {$cfail > 0} {
    .results.raw.text insert end \
	    "Expect some color losses!" redtag "\n"
}

#vnmr stuff - get the file

if {$argc > 2} {
    toplevel .lds
    frame .lds.ldscreen
    frame .lds.ldscreen.f
    label .lds.ldscreen.f.err -text "                               "
    pack .lds.ldscreen.f.err -side left
    pack .lds.ldscreen.f
    pack .lds.ldscreen

    set fn [lindex $argv 3]
    set file [open $fn "r"]
    if {$argc > 4} {
	set rfield [lindex $argv 4]
	if {$argc > 5} {
	    set gfield [lindex $argv 5]
	    if {$argc > 6} {
		set bfield [lindex $argv 6]
	    }
	}
    }
    loadfile
    .r.crit.d3 configure -text "$type $range($rfield,st) to $range($rfield,end) $units"
    .g.crit.d3 configure -text "$type $range($gfield,st) to $range($gfield,end) $units"
    .b.crit.d3 configure -text "$type $range($bfield,st) to $range($bfield,end) $units"
    doit
    destroy .lds
}

update
showHelp
