#! /bin/sh
# 
# . 
#
# Re execute this file under Tcl.
# The next line is executed by "sh" but it is a comment to Tcl. \
	exec $vnmrsystem/tcl/bin/vnmrwish "$0" ${1+"$@"}

load $env(vnmrsystem)/lib/libtix.so
load $env(vnmrsystem)/lib/libBLT.so
# BLT stuff will be needed if the user uses the Graph button.
# Try to import the BLT namespace into the global scope.  If it
# fails, we'll assume BLT was loaded into the global scope.
catch { 
    namespace import blt::*
}
lappend auto_path $env(BLT_LIBRARY)
package require BLT 2.4

set vnmrComm 0
if {$argc == 2} {
    set vnmrComm 1
    # NB: vnmrinit strips off the first and last chars of its first arg.
    vnmrinit x[lindex $argv 0]x [lindex $argv 1]
    set vnmrid [lindex $argv 0]
    scan $vnmrid "%*s %*s %d" vnmrpid
}

set tcl_precision 6
set tk_strictMotif 1

set tauwidth [expr $tcl_precision + 4]
set taumin 0.001
set taumax 3000
set ampmin -100
set ampmax 100
set tauincval 1.05
set allAxes {x y z b0}
set allScales {eccscale shimscale totalscale}
set allLimits {slewlimit dutylimit}
set scalesAndLimits [concat $allScales $allLimits]
set ndirterms 5
set nxterms 3
set boxspace 3

set needAmpcheck 0
set correctingB0Amp 0

set knownModified -1	;# -1 ==> check for modification when data changes
			;#  0 ==> assume unmodified
			;#  1 ==> assume modified

set currentFname ""	;# Not currently used - last saved/loaded file

set modifiedFlag 0

set dflt(enb) 0
set dflt(tau) $taumin
set dflt(amp) 0
set dflt(eccscale) 10
set dflt(shimscale) 5
set dflt(totalscale) 5
set dflt(slewlimit) 1000
set dflt(dutylimit) 5

set slew(offset) 3.17864
set slew(a) 0.646622
set slew(vrail) 5000

# (value displayed) = factor * (value in ECC file)
set factor(tau) 1000.0
set factor(amp) 100.0
set factor(eccscale) 100.0
set factor(shimscale) 100.0
set factor(totalscale) 100.0
set factor(slewlimit) 1.0e6
set factor(dutylimit) 100.0

# For line buffering of the ECC files
set nFileLines 0

# List of all tixControl widgets.
# (We sometimes need to update them manually.)
set updateList {}

# These buttons need to get enabled / disabled
set buttonList {}

#
# Downloadable values are kept in the global array "currentData()".
# Array elements are:
# enb, tau, amp for the time constants and amplitudes, e.g.:
#	(tau,x|y|z,x|y|z|b0,1-5) = (tau,$srcaxis,$dstaxis,$termnumber)
#	(enb=false ==> amp downloaded as 0)
# scale values on the Smart DAC board
#	(eccscale,x|y|z), (shimscale,x|y|z), (totalscale,x|y|z)
# limits for the limit settings
#	(slewlimit,x|y|z), (dutylimit,x|y|z)
#

proc decimal {number} {
    #
    # Make "number" a decimal number by stripping any leading zeros
    # or "0x".
    #
    set rtn [string trimleft $number " 0xX"]
    if { [string length $rtn] == 0} {
	set rtn 0
    }
    return $rtn
}

proc tauInc {value} {
    #global logtau
    global taumax tauincval
    set value [expr $value * $tauincval > $taumax ? \
	    $taumax : $value * $tauincval]
    return $value
}

proc tauDec {value} {
    #global logtau
    global taumin tauincval
    set value [expr $value / $tauincval < $taumin ? \
	    $taumin : $value / $tauincval]
    return $value
}

proc tauCtrl {var value} {
    #global logtau
    global knownModified

    setChanged $knownModified

    #set $var [expr log10($value)]
    return
}

proc tauCallback {src dst iterm value} {
    global currentData

    #tauCtrl logtau($src,$dst,$iterm) $value
    tauCtrl dummy $value

    if { $currentData(amp,$src,$dst,$iterm) != 0} {
	set currentData(enb,$src,$dst,$iterm) 1
    }
    checkAmp $src $dst dummytau
}

proc ampCallback {src dst ix value} {
    global currentData b0Ratio correctingB0Amp coupleB0

    if {! $correctingB0Amp && [string compare $dst b0] == 0} {
	#puts "ampCallback($src, $dst, $ix, $value)"
	#puts "b0Ratio($src,amp,$ix)=$value"
	set b0Ratio($src,amp,$ix) $value
	set b0Ratio($src,scale,$ix) $currentData(totalscale,$src)
    }
    if {$value != 0 && ! $correctingB0Amp} {
	# Turn on the Enable button
	set currentData(enb,$src,$dst,$ix) 1
    }
    checkAmp $src $dst dummyamp
}

proc eccscaleValidation {widget axis newScale} {
    set newScale [decimal $newScale]
    set ival [expr int(0.5 + ($newScale * 256) / 10)]
    if {$ival > 255} {
	set ival 255
    } elseif {$ival < 0} {
	set ival 0
    }
    set val [expr $ival * 10.0 / 256]
    return $val
}

proc totalscaleCallback {axis ifmod value} {
    global needAmpcheck coupleB0 b0Ratio
    #puts "totalscaleCallback $axis $ifmod $value" ;#CMP
    setChanged $ifmod
    set needAmpcheck 1
    if {! $coupleB0} {
	set nt [nTerms $axis b0]
	for {set ix 1} {$ix <= $nt} {incr ix} {
	    #puts "b0Ratio($axis,scale,$ix)=$value" ;#CMP
	    set b0Ratio($axis,scale,$ix) $value
	}
    }
}

proc totalscaleValidation {widget axis newScale} {
    global coupleB0 b0Ratio correctingB0Amp currentData noAmpCheck
    #puts "totalscaleValidation $widget $axis $newScale" ;#CMP
    set oldScale [$widget cget -value]
    set newScale [decimal $newScale]
    if {$newScale < -100} {set newScale -100.0}
    if {$newScale > 100} {set newScale 100.0}
    #puts "oldScale = $oldScale"
    if {$coupleB0} {
	set warned 0
	set nterms [nTerms $axis b0]
	for {set ix 1} {$ix <= $nterms} {incr ix} {
	    set noAmpCheck [expr $ix < $nterms] ;# Only check after last term
	    set refScale $b0Ratio($axis,scale,$ix)
	    #puts "refScale=$refScale" ;#CMP
	    if {$refScale} {
		set oldAmp($ix) $currentData(amp,$axis,b0,$ix)
		set newAmp [expr $b0Ratio($axis,amp,$ix) * $newScale \
			/ $refScale]
		if { ! $warned \
			&& abs($newAmp) > 100.0 \
			&& abs($oldAmp($ix)) < 100.0} {
		    set warned 1
		    set cancel 0
		    #tixControl:StopRepeat $widget
		    #tkButtonUp [$widget subwidget incr]
		    #puts [bind [$widget subwidget incr] <ButtonRelease-1>]
		    #
		    # NB: Popping up a dialog box at this point will
		    # make the up or down button get stuck--like it
		    # never receives the ButtonRelease event.
		    # So forget about warning the user.
		    # (And the stuff below never gets done, i.e.,
		    #  values are left set over 100. Actually, new
		    #  limit should be 50(?).)
		    #
		    #set cancel [tk_dialog .errWin "Warning" \
			#    "Scale change will put B0 correction out of range"\
			#    warning 1 "Continue" "Cancel"]
		    if {$cancel} {
			# CANCEL: Put original amplitudes back
			for {set iy 1} {$iy < $ix} {incr iy} {
			    set correctingB0Amp 1
			    set noAmpCheck [expr $iy < ($ix - 1)]
			    set currentData(amp,$axis,b0,$iy) $oldAmp($iy)
			    set correctingB0Amp 0
			}
			set noAmpCheck 0
			# Return unchanged value
			return $oldScale
		    }
		}
		#puts "refAmp=$b0Ratio($axis,amp,$ix), newScale= $newScale, refScale=$refScale"
		set correctingB0Amp 1
		set currentData(amp,$axis,b0,$ix) $newAmp
		set correctingB0Amp 0
	    }
	}
    }
    return [expr double($newScale)]
}

proc triseToDwnld {trise} {
    global slew
    set dwnld [expr int(floor(0.5 + \
	    (($slew(vrail) - $slew(offset) * $trise) / ($slew(a) * $trise))))]
    return $dwnld
}

proc dwnldToTrise {dwnld} {
    global slew
    set trise [expr $slew(vrail) / ($slew(offset) + $slew(a) * $dwnld)]
    return $trise
}

proc slewInc {trise} {
    if {$trise <= 0} {set trise 1}	;# Avoid divide by 0
    set dwnld [triseToDwnld $trise]
    incr dwnld -1
    set trise [dwnldToTrise $dwnld]
    return $trise
}

proc slewDec {trise} {
    if {$trise <= 0} {set trise 1}	;# Avoid divide by 0
    set dwnld [triseToDwnld $trise]
    incr dwnld
    set trise [dwnldToTrise $dwnld]
    return $trise
}

proc slewValidate {trise} {
    if {$trise <= 0} {set trise 1}	;# Avoid divide by 0
    set dwnld [triseToDwnld $trise]
    if {$dwnld < 0} {set dwnld 0}
    if {$dwnld > 255} {set dwnld 255}
    set trise [dwnldToTrise $dwnld]
    return $trise
}

proc nTerms {srcAxis dstAxis} {
    if {[string compare $srcAxis $dstAxis] == 0} {
	set nterms 6
	#set nterms 5	;# Old value
    } elseif {[string compare $dstAxis b0] == 0} {
	set nterms 4
	#set nterms 3	;# Old value
    } else {
	set nterms 3
    }

    return $nterms
}

proc listDenullify {inlist} {
    set inelem [llength $inlist]
    for {set ix 0} {$ix < $inelem} {incr ix} {
	set str [lindex $inlist $ix]
	if {[string length $str]} {
	    lappend outlist $str
	}
    }
    return $outlist
}

proc copyArray {oldName newName} {
    upvar $oldName old
    upvar $newName new
    #puts "copyArray $oldName $newName" ;#CMP
    array set new [array get old]
}

proc copyArraySkeleton {oldName newName} {
    upvar $oldName old
    upvar $newName new
    set indices [array names old]
    foreach index $indices {
	set new($index) 0
    }
}

proc arraysDiffer {name1 name2} {
    upvar $name1 array1
    upvar $name2 array2
    #puts "arraysDiffer $name1 $name2" ;#CMP

    if {[array size array1] != [array size array2]} {
	return 1
    }
    foreach name [array names array1] {
	if {![info exists array2($name)]} {
	    return 1
	}
	if {[expr $array1($name) != $array2($name)] } {
	    return 1
	}
    }
    return 0
}

proc setChanged {{value -1} {dummy 0}} {
    global statusWidget currentFname savedData currentData modifiedFlag
    #puts "setChanged $value $dummy" ;#CMP

    if {$value == 0 || $value == 1} {
	set diff $value
    } else {
	set diff [arraysDiffer currentData savedData]
    }

    set modifiedFlag $diff
    if {$diff} {
	set text "(Modified)"
    } else {
	set text "(Unmodified)"
    }
    $statusWidget configure -text $text
}

proc setAmplimit {widget axis value} {
    global amplimit amplimittext
    global knownModified showWarning

    setChanged $knownModified

    set label [string toupper "$axis"]
    set amplimittext($axis) "Allowed $label compensation: [expr $value / 2.0]%"
    if { $showWarning($axis) } {
	[set $widget] configure -text $amplimittext($axis)
    }
    checkAmp dummy $axis dummy
}

proc getGainAdjustedAmp {src dst index} {
    # Get amplitude adjusted for difference in overall gains of
    # source and destination axes, but NOT for the eccgain of
    # the destination axis.
    # Also, there is NO correction for to the amplitude for B0 terms.
    global currentData
    set amp $currentData(amp,$src,$dst,$index)
    if {[string compare $dst b0] == 0} {
	return $amp
    }
    set srcgain $currentData(totalscale,$src)
    set dstgain $currentData(totalscale,$dst)
    if {$dstgain != 0} {
	set amp [expr ($amp * $srcgain) / $dstgain]
    }
    return $amp
}

proc absEccEval {srcAxes dstAxis time} {
    # Evaluate the absolute value of the total ECC correction at
    # a given time on a given dst axis due to all the src axes
    # in the given list.
    global currentData
    set y 0
    set t [expr $time * -1]
    foreach srcAxis $srcAxes {
	set nterms [nTerms $srcAxis $dstAxis]
	set yi 0
	for {set ix 1} {$ix <= $nterms} {incr ix} {
	    set amp [getGainAdjustedAmp $srcAxis $dstAxis $ix]
	    if {$amp && $currentData(enb,$srcAxis,$dstAxis,$ix)} {
		set tau $currentData(tau,$srcAxis,$dstAxis,$ix)
		set yi [expr $yi + $amp * [xexp [expr $t / $tau]]]
	    }
	}
	set y [expr $y + abs($yi)]
    }
    return $y
}

proc absEccEval2 {srcAxes dstAxis time} {
    # Like absEccEval except returns additional info in a list.
    # First element, "y", is the same as the value returned by
    # absEccEval.
    # Second element, "maxabs", is the absolute value of the sum of
    # all the individual exponential terms that could have the same
    # sign.  This gives an upper limit to ECC values for all larger
    # times.  For each src axis, separately sums the positive and
    # negative terms and selects the one with the largest absolute
    # value.  These absolute values for all the src axes are added to
    # form the "maxabs" sum.
    global currentData
    set y 0
    set maxabs 0
    set t [expr $time * -1]
    foreach srcAxis $srcAxes {
	set nterms [nTerms $srcAxis $dstAxis]
	set yi 0
	set maxplus 0
	set maxminus 0
	for {set ix 1} {$ix <= $nterms} {incr ix} {
	    set amp [getGainAdjustedAmp $srcAxis $dstAxis $ix]
	    if {$amp && $currentData(enb,$srcAxis,$dstAxis,$ix)} {
		set tau $currentData(tau,$srcAxis,$dstAxis,$ix)
		set term [expr $amp * [xexp [expr $t / $tau]]]
		if {$term > 0} {
		    set maxplus [expr $maxplus + $term]
		} else {
		    set maxminus [expr $maxminus + $term]
		}
	    }
	}
	set y [expr $y + abs($maxplus + $maxminus)]
	if {$maxplus > -$maxminus} {
	    set maxabs [expr $maxabs + $maxplus]
	} else {
	    set maxabs [expr $maxabs - $maxminus]
	}
    }
    return [list $y $maxabs]
}

# Following routine based on Numerical Recipes routine
proc brent {a x b fa fx fb axisList dstAxis} {
    set gold 0.382
    set zeps 1e-4
    set tol 3e-2
    set step2 0
    set v $x; set w $x
    set fv $fx; set fw $fx

    for {set ix 1} {$ix <= 100} {incr ix} {
	set xm [expr ($a + $b) / 2.0]
	set tol1 [expr $tol * abs($x) + $zeps]
	set tol2 [expr 2 * $tol1]
	if {abs($x - $xm) <= $tol2 - ($b - $a) / 2.0} {
	    # Normal return
	    #puts "brent: converged to $fx in $ix iterations"
	    return $fx
	}
	if {abs($step2) > $tol1} {
	    # Parabolic step - cannot happen first time through
	    set r [expr ($x - $w) * ($fv - $fx)]
	    set q [expr ($x - $v) * ($fw - $fx)]
	    set p [expr ($x - $v) * $q - ($x - $w) * $r]
	    set q [expr 2.0 * ($q - $r)]
	    if {$q > 0} {set p [expr -1 * $p]}
	    set q [expr abs($q)]
	    set step2t $step2
	    set step2 $d
	    if {abs($p) >= abs($q * $step2t / 2.0) \
		    || $p <= $q * ($a - $x) \
		    || $p >= $q * ($b - $x)} {
		set step2 [expr $x >= $xm ? $a - $x : $b - $x]
		set d [expr $gold * $step2]
	    } else {
		set d [expr $p / $q]
		set u [expr $x + $d]
		if {$u - $a < $tol2 || $b - $u < $tol2} {
		    set tt [expr $xm - $x]
		    set d [expr $b > 0 ? abs($tol1) : -abs($tol1)]
		}
	    }
	} else {
	    # Bisection step
	    set step2 [expr $x >= $xm ? $a - $x : $b - $x]
	    set d [expr $gold * $step2]
	}
	set tt [expr $d > 0 ? abs($tol1) : -abs($tol1)]
	set u [expr abs($d) >= $tol1 ? $x + $d : $x + $tt]
	set fu [absEccEval $axisList $dstAxis $u]
	if {$fu >= $fx} {
	    if {$u >= $x} {
		set a $x
	    } else {
		set b $x
	    }
	    set v $w; set w $x; set x $u
	    set fv $fw; set fw $fx; set fx $fu
	} else {
	    if {$u < $x} {
		set a $u
	    } else {
		set b $u
	    }
	    if {$fu >= $fw || $w == $x} {
		set v $w; set w $u
		set fv $fw; set fw $fu
	    } elseif {$fu >= $fv || $v == $x || $v == $w} {
		set v $u; set fv $fu
	    }
	}
    }
    puts "brent: did not converge in 100 iterations"
    return $fx
}

proc checkAmps {} {
    global noAmpCheck needAmpcheck
    set noAmpCheck 0
    set axes {{x y z} x {x y z} y {x y z} z {x} b0 {y} b0 {z} b0 }
    foreach {src dst} $axes {
	checkAmp $src $dst dummy
    }
    set needAmpcheck 0
}

proc checkAmp {src dst dummy} {
    global amp enb eccscale ampwarn ampwarntext showWarning

    global currentData
    global updateList
    global knownModified noAmpCheck

    #puts "noAmpCheck=$noAmpCheck"
    updateWidgets $updateList

    setChanged $knownModified
    if {$noAmpCheck} {
	return
    }
    #puts "ampCheck"

    set limit [expr $currentData(eccscale,$dst) / 2.0]
    set showWarn 1
    if {[string compare $dst b0] == 0} {
	# Separate limits for X->B0, Y->B0, Z->B0
	set axislist $src
	set widget $ampwarn($src,$dst)
	set text ampwarntext($src,$dst)
    } else {
	# One limit for X->X + Y->X + Z->X; etc.
	set axislist {x y z}
	set widget $ampwarn($dst)
	set text ampwarntext($dst)
	set showWarn $showWarning($dst)
    }

    set max [absEccEval $axislist $dst 0]
    set step 2.718
    set a 0
    set fa $max
    set b 1e-2
    set f2 [absEccEval2 $axislist $dst $b]
    set fb [lindex $f2 0]
    set ffb [lindex $f2 1]
    while {1} {
	set c [expr $b * $step]
	set f2 [absEccEval2 $axislist $dst $c]
	set fc [lindex $f2 0]
	set ffc [lindex $f2 1]
	if {$fa < $fb && $fb > $fc} {
	    set mx [brent $a $b $c $fa $fb $fc $axislist $dst]
	    if {abs($mx) > $max} {
		set max [expr abs($mx)]
	    }
	}
	if {$ffb <= $max} {
	    # All terms are too small to exceed previous maximum
	    #puts "break at ffb=$ffb, b=$b"
	    break
	}
	set a $b; set fa $fb; set ffb $ffc
	set b $c; set fb $fc
    }

    set msg [format "Worst case excursion %.3g%%" $max]
    if {$max > $limit} {
	set color red3
    } else {
	set color green4
    }
    set $text $msg
    $widget configure -fg $color
    if {$showWarn} {
	$widget configure -text $msg
    }
    #puts "Max $src->$dst excursion: $max%"
}

proc getGcoil {} {
    global env coilLabelWidget

    set conpar [file join $env(vnmrsystem) conpar]
    if [catch {open $conpar r} fd] {
	tk_dialog .errWin "Error" "Error reading Vnmr parameters: $fd" \
		error 0 "Continue"
	return
    }

    # Look for "sysgcoil" parameter
    set name ""
    while {[gets $fd line] >= 0} {
	if {[regexp {^sysgcoil } $line ] == 1} {
	    gets $fd line
	    regexp {" *([^\"]*) *"} $line x name
	}
    }
    close $fd
    if {[string length $name] == 0} {
	tk_dialog .errWin "Error" \
	    "The \"sysgcoil\" parameter is not set\nRun config" \
	    error 0 "Exit"
	exit
    }
    $coilLabelWidget configure -text "Gradient coil: $name"
    return $name
}

proc getMasterFilePath {} {
    global env
    set name [getGcoil]
    set path [file join $env(vnmrsystem) imaging decclib .$name]
    return $path
}

proc getEccDir {} {
    global env
    # NB: Contortions to get the final path separator (Unix "/") included.
    set path [file join $env(vnmrsystem) imaging decclib x]
    set path [string trimright $path x] 
    return $path
}

proc checkEccDir {} {
    set path [getEccDir]
    set ok [file writable $path]
    if {! $ok} {
	tk_dialog .errWin "Error" \
		"Please create the directory \"$path\". Make sure you have write permission in it." \
		error 0 "Continue"
	return 0
    }
    return 1
}

proc varFromGradtable {format} {
    global env
    set name [getGcoil]
    set path [file join $env(vnmrsystem) imaging gradtables $name]
    if [catch {open $path r} fd] {
	tk_dialog .errWin "Error" "Error reading gradtable: $fd" \
		error 0 "Continue"
	return 0
    }

    # Look for the matching line
    set value "0"
    while {[gets $fd line] >= 0} {
	if {[scan $line $format value]} {
	    break
	}
    }
    close $fd
    return $value
}

proc updateWidgets {widgetList} {
    foreach widget $widgetList {
	$widget update
    }
}

proc setStates {list state} {
    foreach widget $list {
	$widget configure -state $state
    }
}

proc deleteFiles {pathlist} {
    foreach file $pathlist {
	file delete $file
    }
}

proc fileEntryCallback {dummy} {
    global buttonList fileSelector

    set name [$fileSelector cget -value]
    set name [string trim $name]
    if {[string length $name]} {
	set state normal
    } else {
	set state disabled
    }
    setStates $buttonList $state
}

proc fileListingPurge {pathlist} {
    global fileSelector

    set filelist {}
    foreach path $pathlist {
	lappend filelist [file tail $path]
    }
    set lbox [$fileSelector subwidget listbox]
    for {set ix [$lbox index end]} {$ix >= 0} {incr ix -1} {
	set entry [lindex [$lbox get $ix $ix] 0]
	foreach file $filelist {
	    if {[string compare $file $entry] == 0} {
		$lbox delete $ix $ix
	    }
	}
    }
}

proc updateFileName {} {
    global fileSelector

    set name [$fileSelector cget -selection]
    set name [string trim $name]
    $fileSelector configure -value $name
}

proc stripFileVersions {fname1} {
    set fname2 ""
    while {[string compare $fname1 $fname2]} {
	set fname2 $fname1
	regsub {\.[1-9][0-9]*$} $fname2 "" fname1
    }
    return $fname1
}

proc versionOf {path} {
    set version 0
    regexp {\.([1-9][0-9]*)$} $path dummy version
    return $version
}

proc setVersionLabel {version} {
    global versionWidget
    if {$version == 0} {
	set vstring "   "
    } else {
	set vstring [format "%-3d" $version]
    }
    $versionWidget configure -text "Version: $vstring"
}

proc listVersionsOf {basepath} {
    set pattern $basepath
    append pattern {.[1-9]*}
    set filelist1 [glob -nocomplain $pattern]
    set filelist2 {}
    foreach file $filelist1 {
	if {[regexp {\.[1-9][0-9]*$} $file]} {
	    lappend filelist2 $file
	}
    }
    set filelist3 [lsort -decreasing -dictionary $filelist2]
    #puts "filelist3=$filelist3"
    return $filelist3
}
    

proc latestVersionOf {path} {
    set filelist3 [listVersionsOf $path]
    if {[llength $filelist3] < 1} {
	return 0
    } else {
	set file [lindex $filelist3 0]
	set version [versionOf $file]
	return $version
    }
}

proc appendVersionNumber {path {vnum 0}} {
    if {[regexp {\.[1-9][0-9]*$} $path]} {
	# path already has a version number
	return $path
    }
    if {$vnum} {
	set version $vnum
    } else {
	set version [latestVersionOf $path]
    }
    if {$version} {
	append path .
	append path $version
    }
    return $path
}

proc makeFilename {filecode {versionFlag keepVersion}} {
    global eccFileName

    if {[string compare $filecode "master"] == 0} {
	set path [getMasterFilePath]
    } elseif {[string compare $filecode "user"] == 0} {
	updateFileName
	if { [string length $eccFileName] } {
	    if {[string compare $versionFlag keepVersion] == 0} {
		set fname $eccFileName
	    } else {
		set fname [stripFileVersions $eccFileName]
	    }
	    set path [file join [getEccDir] $fname]
	} else {
	    set path ""
	}
    } else {
	set path $filecode
    }

    if {[file isdirectory $path]} {
	tk_dialog .errWin "Error" "Error: $path is a directory" \
		error 0 "Continue"
	return ""
    }
    return $path
}

proc readAllDataX {fd} {
    global nFileLines fileLine

    for {set nl 0} {[gets $fd fileLine($nl)] >= 0} {incr nl} {
	# Took this out to avoid lowering the "filename".
	# Could just lower the first word.
	#set fileLine($nl) [string tolower $fileLine($nl)]
    }
    set nFileLines $nl

    return
}

proc readEccFileX {path {mustread 1}} {

    if [catch {open $path r} fd] {
	if {$mustread} {
	    tk_dialog .errWin "Error" "Error opening ECC file: $fd" \
		    error 0 "Continue"
	}
	return 0
    }

    # Read the data into memory
    readAllDataX $fd

    close $fd
    return 1
}

proc updateFileImage {nlines dataArrayName} {
    global eccFileName eccFileVersion
    global scalesAndLimits allAxes factor taumin
    global nFileLines fileLine
    upvar $dataArrayName array

    # Change file name
    # Look for an existing filename line
    for {set iline 0} {$iline < $nlines} {incr iline} {
	if {[scan $fileLine($iline) "%s" fvar] == 1 \
		&& [string compare $fvar "filename"] == 0} {
	    break
	}
    }
    if {$iline == $nlines} {
	incr nlines
    }
    # Put in a new or replacement line
    set fname [appendVersionNumber $eccFileName $eccFileVersion]
    set fileLine($iline) "filename\t$fname"

    # Set the date
    # Look for an existing date line
    for {set iline 0} {$iline < $nlines} {incr iline} {
	if {[scan $fileLine($iline) "%s" fvar] == 1 \
		&& [string compare $fvar "date"] == 0} {
	    break
	}
    }
    if {$iline == $nlines} {
	incr nlines
    }
    # Put in a new or replacement line
    set date [exec date]
    set fileLine($iline) "date\t$date"

    # Changes for ECC values
    foreach axis $allAxes {
	foreach srcAxis { x y z } {
	    if [string match "\[xyz\]\[xyzb\]*" $axis] {
		set srcAxis [string index $axis 0]
		set dstAxis [string range $axis 1 end]
	    } else {
		set dstAxis $axis
	    }

	    # Look for an existing line for this axis
	    for {set iline 0} {$iline < $nlines} {incr iline} {
		if {[scan $fileLine($iline) "%s" ax] == 1 \
			&& [string compare $ax $srcAxis$dstAxis] == 0} {
		    break
		}
	    }
	    if {$iline == $nlines} {
		incr nlines
	    }
	    # Put in a new or replacement line
	    set fileLine($iline) "$srcAxis$dstAxis"
	    set nterms [nTerms $srcAxis $dstAxis]
	    for {set ix 1} {$ix <= $nterms} {incr ix} {
		if {$array(enb,$srcAxis,$dstAxis,$ix)} {
		    set e ""
		} else {
		    set e *
		}
		set t [expr $array(tau,$srcAxis,$dstAxis,$ix) / $factor(tau)]
		set a [expr $array(amp,$srcAxis,$dstAxis,$ix) / $factor(amp)]
		if {$a == 0} {
		    set a 0
		    if {$t == $taumin / $factor(tau)} {
			set t 0
		    }
		}
		append fileLine($iline) "\t$e$t $a"
	    }
	    if [string match "\[xyz\]\[xyzb\]*" $axis] {
		break
	    }
	}
    }

    # Changes for scale and limit values
    foreach var $scalesAndLimits {
	# Check for an existing line
	for {set iline 0} {$iline < $nlines} {incr iline} {
	    if {[scan $fileLine($iline) "%s" fvar] == 1 \
		    && [string compare $fvar $var] == 0} {
		break
	    }
	}
	if {$iline == $nlines} {
	    incr nlines
	}
	# Put in a new or replacement line
	set v(x) [expr $array($var,x) / $factor($var)]
	set v(y) [expr $array($var,y) / $factor($var)]
	set v(z) [expr $array($var,z) / $factor($var)]
	#puts "factor($var)=$factor($var)"
	set fileLine($iline) "$var\t$v(x)\t$v(y)\t$v(z)"
    }
    set nFileLines $nlines
    return $nlines
}

proc updateArray {arrayName {checkamp 0}} {
    upvar $arrayName array
 
    global scalesAndLimits allAxes
    global fileLine nFileLines
    global taumin taumax ampmin ampmax eccFileName
    global dflt factor

    # Load the user's file name
    for {set iline 0} {$iline < $nFileLines} {incr iline} {
	if {[scan $fileLine($iline) "%s %s" fvar fname] == 2 \
		&& [string compare $fvar "filename"] == 0} {
	    # Trim the keyword from the line; the rest is the filename
	    regsub {[ 	]*[^	 ]*[	 ]*} $fileLine($iline) "" fname
	    set fname [string trim $fname]
	    set eccFileName [stripFileVersions $fname]
	    setVersionLabel [versionOf $fname]
	    break
	}
    }

    # Load scaling and limits
    foreach var $scalesAndLimits {
	# Look for a line for this variable
	for {set iline 0} {$iline < $nFileLines} {incr iline} {
	    if {[scan $fileLine($iline) "%s" fvar] == 1 \
		    && [string compare $fvar $var] == 0} {
		break
	    }
	}

	if {$iline < $nFileLines} {
	    # Found it -- load values from file
	    set fieldlist [split $fileLine($iline) " \t"]
	    set fieldlist [listDenullify $fieldlist]
	    set nf [llength $fieldlist]
	    set ix 1
	    foreach axis {x y z} {
		if {$ix < $nf} {
		    set xxx [lindex $fieldlist $ix]
		    set array($var,$axis) [expr $xxx * $factor($var) ]
		} else {
		    set array($var,$axis) [set dflt(${var})]
		}
		#puts "array($var,$axis) = $array($var,$axis)"
		incr ix
	    }
	} else {
	    # No line found -- put in default values
	    foreach axis {x y z} {
		set array($var,$axis) [set dflt(${var})]
		#puts "array($var,$axis) = $array($var,$axis)"
	    }
	}
    }
    # "Load" the B0 scaling limit
    set array(eccscale,b0) 100

    # Load the ECC values
    foreach axis $allAxes {
	foreach srcAxis { x y z } {
	    if [string match "\[xyz\]\[xyzb\]*" $axis] {
		set srcAxis [string index $axis 0]
		set dstAxis [string range $axis 1 end]
	    } else {
		set dstAxis $axis
	    }

	    set nterms [nTerms $srcAxis $dstAxis]
	    # Initialize values in temp array
	    for {set ix 1} {$ix <= $nterms} {incr ix} {
		set e($ix) $dflt(enb)
		set t($ix) $dflt(tau)
		set a($ix) $dflt(amp)
	    }
	    # Look for a line for these axes
	    for {set iline 0} {$iline < $nFileLines} {incr iline} {
		if {[scan $fileLine($iline) "%s" ax] == 1 \
			&& [string compare $ax $srcAxis$dstAxis] == 0} {
		    break
		}
	    }

	    if {$iline < $nFileLines} {
		# Found it -- load values from file
		set fieldlist [split $fileLine($iline) " \t"]
		set fieldlist [listDenullify $fieldlist]
		set nf [llength $fieldlist]
		set nt [expr ($nf-1)/2]
		for {set ix 1} {$ix <= $nt} {incr ix} {
		    set i [expr 1 + ($ix-1)*2]
		    set t($ix) [lindex $fieldlist $i]
		    set e($ix) [string index $t($ix) 0]
		    if {[string compare $e($ix) *]} {
			set e($ix) 1
		    } else {
			set e($ix) 0
			set t($ix) [string range $t($ix) 1 end]
		    }
		    incr i
		    set a($ix) [lindex $fieldlist $i]
		}
	    }

	    # Set only those variables that have actually changed
	    # (Changing them is SLOW)
	    set needCheck 0
	    for {set ix 1} {$ix <= $nterms} {incr ix} {
		set t($ix) [expr $t($ix)*1000]
		if {$t($ix) < $taumin} {
		    set t($ix) $taumin
		}
		if {$t($ix) > $taumax} {
		    set t($ix) $taumax
		}
		if {$array(tau,$srcAxis,$dstAxis,$ix) != $t($ix)} {
		    set array(tau,$srcAxis,$dstAxis,$ix) $t($ix)
		    set needCheck 1
		    #puts "array(tau,$srcAxis,$dstAxis,$ix) $t($ix)"
		}

		set a($ix) [expr $a($ix)*100]
		if {$a($ix) < $ampmin} {
		    set a($ix) $ampmin
		}
		if {$a($ix) > $ampmax} {
		    set a($ix) $ampmax
		}
		if {$array(amp,$srcAxis,$dstAxis,$ix) != $a($ix)} {
		    set array(amp,$srcAxis,$dstAxis,$ix) $a($ix)
		    set needCheck 1
		    #puts "array(amp,$srcAxis,$dstAxis,$ix) $a($ix)"
		}
		if {$array(enb,$srcAxis,$dstAxis,$ix) != $e($ix)} {
		    set array(enb,$srcAxis,$dstAxis,$ix) $e($ix)
		    set needCheck 1
		    #puts "array(enb,$srcAxis,$dstAxis,$ix)=$e($ix)"
		}
	    }

	    if {$needCheck && $checkamp} {
		checkAmp $srcAxis $dstAxis dummy
	    }

	    if [string match "\[xyz\]\[xyzb\]*" $axis] {
		break
	    }
	}
    }
}

proc filePurge {} {
    #puts "filePurge"	;#CMP
    global currentFname fileSelector eccFileName
    global knownModified modifiedFlag

    set basepath [makeFilename user discardVersion]
    set basename [file tail $basepath]
    if { [string length $basepath] } {
	set fullList [listVersionsOf $basepath]
	set nfiles [llength $fullList]
	if {$nfiles == 0} {
	    return
	} elseif {$nfiles == 1} {
	    set cancel [tk_dialog .errWin "Warning" \
		    "This will delete the last and only version of \"$basename\"." \
		    warning 1 "Continue" "Cancel"]
	} elseif {$nfiles > 1} {
	    set version [versionOf [lindex $fullList 0]]
	    set cancel [tk_dialog .errWin "Warning" \
		    "This will delete all versions of \"$basename\" except the latest: version $version." \
		    warning 1 "Continue" "Cancel"]
	    set fullList [lreplace $fullList 0 0]	;# Do not delete latest
	}
	if {! $cancel && $nfiles} {
	    deleteFiles $fullList
	    fileListingPurge $fullList
	}
    }
    set eccFileName $basename
    setVersionLabel 0
}

proc fileLoad {fileCode {mustread 1}} {
    global currentData modifiedFlag fileSelector eccFileName eccFileVersion

    set cancel 0
    if {$modifiedFlag} {
	set cancel [tk_dialog .errWin "Warning" \
		"WARNING:\nThis will wipe out your modified values." \
		warning 1 "Continue" "Cancel"]
    }

    if {! $cancel} {
	set fpath [makeFilename $fileCode keepVersion]
	if { [string length $fpath] } {
	    set fname [file tail $fpath]
	    set fpath [appendVersionNumber $fpath]
	    if {[fileLoadX $fpath currentData $mustread]} {
		set eccFileName [stripFileVersions $fname]
		set eccFileVersion [versionOf $fpath]
		set fname [appendVersionNumber $fname $eccFileVersion]
		$fileSelector addhistory $fname
		setVersionLabel $eccFileVersion
	    }
	}
	fileSaveX master currentData
    }
}

proc fileLoadX {filepath arrayName {mustread 1}} {
    upvar $arrayName array
 
    global taumin taumax ampmin ampmax eccFileName
    global dflt factor

    global scalesAndLimits allAxes
    global fileLine nFileLines
    global currentFname savedData
    global knownModified noAmpCheck coupleB0


    # Check the filepath
    if {[string length $filepath] == 0} {
	return 0
    }

    set knownModified 0
    set noAmpCheck 1

    # Read the lines from file into memory
    if {! [readEccFileX $filepath $mustread]} {
	set knownModified -1
	set noAmpCheck 0
	return 0
    }

    # This is too confusing
    #set coupleB0 1

    # Load values into array
    updateArray array

    # Update the state
    copyArray array savedData
    set currentFname [file tail $filepath]
    #puts "currentFname = $currentFname"	;# CMP
    setChanged 0
    set knownModified -1
    checkAmps

    return 1
}

proc fileCompare {filepath} {
    # Returns 0 if current data in file "path" is identical to the
    # current loaded values.  Otherwise returns 1.
    global currentData

    copyArraySkeleton currentData cmpdat
    if {[fileLoadX $filepath cmpdat 0] == 0} {
	return 1
    }
    set comp [arraysDiffer currentData cmpdat]
    return $comp
}

proc vnmrGo {arrayName} {
    upvar $arrayName array
    global vnmrpid goButton buttonList

    if [catch {exec kill -s 0 $vnmrpid 2> /dev/null} junk] {
	# Vnmr missing; delete the GO button
	$goButton configure -state disabled
	set idx [lsearch -exact $buttonList $goButton]
	set buttonList [lreplace $buttonList $idx $idx]
    } else {
	if {[fileSave user array]} {
	    vnmrsend deccgo
	}
    }
}

proc fileSave {fileCode arrayName} {
    global fileSelector eccFileName eccFileVersion
    upvar $arrayName array

    set basepath [makeFilename $fileCode discardVersion]
    if { [string length $basepath] } {
	set basename [file tail $basepath]
	set eccFileName $basename
	set eccFileVersion [latestVersionOf $basepath]
	set filepath [appendVersionNumber $basepath $eccFileVersion]
	if {[fileCompare $filepath]} {
	    # Current data differs from file contents
	    incr eccFileVersion
	    set filepath [appendVersionNumber $basepath $eccFileVersion]
	    set eccFileName [file tail $basepath]
	    if {! [fileSaveX $filepath array]} {
		return 0
	    }
	    $fileSelector addhistory \
		    [appendVersionNumber $basename $eccFileVersion]
	    setVersionLabel $eccFileVersion
	}
    }

    if { [string compare $fileCode master] != 0} {
	fileSaveX master array
    }

    return 1
}

proc fileSaveX {fileCode arrayName} {
    upvar $arrayName array

    global enb tau amp taumin eccFileName
    global tauWidg ampWidg eccscaleWidg shimscaleWidg totalscaleWidg
    global slewlimitWidg dutylimitWidg
    global factor

    global scalesAndLimits allAxes
    global fileLine nFileLines
    global savedData currentFname
    global updateList

    global knownModified
    set knownModified 0

    updateWidgets $updateList	;# Flush changes to variables

    # Get the filepath
    set filepath [makeFilename $fileCode]
    if {[string length $filepath] == 0} {
	set knownModified -1
	return 0
    }

    # Read the lines for "current" settings into memory
    readEccFileX $filepath 0
    set nlines $nFileLines

    # Modify lines where necessary
    set nlines [updateFileImage $nlines array]

    # Write out the new file
    if [catch {open $filepath w} fd] {
	tk_dialog .errWin "Error" "Error writing ECC file: $fd" \
		error 0 "Continue"
	set knownModified -1
	return 0
    }
    for {set ix 0} {$ix < $nlines} {incr ix} {
	puts $fd $fileLine($ix)
    }
    close $fd
    catch [file attributes $filepath -permissions 0664]

    # Update the state
    copyArray array savedData
    set currentFname [file tail $filepath]
    #puts "currentFname = $currentFname"	;# CMP
    setChanged 0

    set knownModified -1
    return 1
}

proc xexp {power} {
    if {$power < -100} {
	return 0
    } else {
	return [expr exp($power)]
    }
}

proc update_graph {j} {
    global vec1 vec2 vec3
    #puts "update_graph $j"	;#CMP
    set func [.eccGraph.eqns.entry$j get]
    set func "\( $func \)"
    regsub -all {([^0-9a-zA-Z$])t([^0-9a-zA-Z])} $func {\1${i}\2} expr_str
    regsub -all {exp[ \t]*\(([^)]*)\)} $expr_str {[xexp [expr \1]]} expr_str
    #puts $expr_str ;#CMP
    set ix 0
    for { set i .01 } { $i <= 1000 } { set i [expr $i*1.05] } {
	set vec${j}($ix) [expr $expr_str]
	incr ix
    }
}

proc loglin {} {
    global iflog graph

    if { $iflog } {
	set xmin [$graph xaxis cget -min]
	if { [string length $xmin] && $xmin <= 0} {
	    $graph xaxis configure -min ""
	}
    }
    $graph xaxis configure -logscale $iflog
}

proc createGraph {} {
    global graphExpr
    global xvec vec1 vec2 vec3
    global iflog graph
    global taumin

    option add *Graph.LineMarker.Outline white

    toplevel .eccGraph

    set color(1) tan
    set color(2) red
    set color(3) orange
    set color(4) yellow
    set color(5) green
    set color(6) skyblue
    set color(7) magenta
    set color(8) lightgrey
    set color(9) white

    frame .eccGraph.eqns

    for {set i 1} {$i <= 3} {incr i} {
	label .eccGraph.eqns.label$i -text "$i:"
	entry .eccGraph.eqns.entry$i \
		-textvariable graphExpr($i) \
		-selectborderwidth 0 \
		-selectbackground yellow
	set graphExpr($i) "0"
	bind .eccGraph.eqns.entry$i <Return> "update_graph $i"
	grid .eccGraph.eqns.label$i .eccGraph.eqns.entry$i -sticky ew -padx 0 -pady 2
    }

    message .eccGraph.msg -aspect 10000 -justify left -relief groove \
	    -anchor w -text \
	    "Zoom in: Left-click on corners of area.  Zoom out: Right-click."

    frame .eccGraph.ctrls

    radiobutton .eccGraph.ctrls.linear -variable iflog -text linear \
	    -value no -command loglin
    radiobutton .eccGraph.ctrls.log -variable iflog -text log \
	    -value yes -command loglin
    #button .eccGraph.ctrls.quit -text {Close} -command {exit}

    pack .eccGraph.ctrls.linear .eccGraph.ctrls.log -side left
    #pack .eccGraph.ctrls.quit -side right

    graph .eccGraph.graph -title "" -cursor "" -plotbackground black
    set graph .eccGraph.graph

    $graph grid configure -hide no -dashes { 2 2 }

    $graph xaxis configure \
	    -loose yes \
	    -title "Time (ms)"
    set iflog yes

    $graph yaxis configure \
	    -rotate 90.0 \
	    -loose yes \
	    -title "ECC (%)"

    grid .eccGraph.eqns -sticky ew -padx 5 -pady 5
    grid columnconfigure .eccGraph.eqns 1 -weight 1
    grid $graph -sticky news
    grid .eccGraph.ctrls -sticky ew -padx 5 -pady 5
    grid .eccGraph.msg -sticky ew
    grid rowconfigure .eccGraph 1 -weight 1
    grid columnconfigure .eccGraph 0 -weight 1

    wm minsize .eccGraph 200 200

    if {! [array exists xvec] } {
	vector xvec
	set npts 0
	for { set i .01 } { $i <= 1000 } { set i [expr $i*1.05] } {
	    set xvec(++end) $i
	    incr npts
	}
    }

    for {set ix 1} {$ix <= 3} { incr ix} {
	if {! [array exists vec${ix}] } {
	    vector vec${ix}($npts)
	}
	$graph pen create "activeLine$ix"
	$graph element create line$ix \
		-label $ix \
		-color $color($ix) \
		-symbol "" \
		-linewidth 2 \
		-activepen "activeLine$ix" \
		-xdata xvec -ydata vec$ix
	$graph pen configure "activeLine$ix" -color $color($ix) \
		-linewidth 4 -symbol ""
	update_graph $ix
    }


    $graph legend configure \
	    -font {lucida 10 normal} \
	    -position right \
	    -anchor n \
	    -borderwidth 2 \
	    -foreground white \
	    -background black \
	    -activeforeground white \
	    -activebackground grey50 \
	    -activeborderwidth 2 

    loglin

    Blt_ZoomStack $graph
    Blt_ActiveLegend $graph
}

proc showGraph {} {
    if { [winfo exists .eccGraph] } {
	wm deiconify .eccGraph
    } else {
	createGraph
    }
}

proc makeEccExpr {src dst} {
    global currentData
    set nterms [nTerms $src $dst]
    set line ""
    for {set ix 1} {$ix <= $nterms} {incr ix} {
	#puts "currentData: [array names currentData enb*]"
	if {$currentData(enb,$src,$dst,$ix) \
		&& $currentData(amp,$src,$dst,$ix)} {
	    if {$currentData(amp,$src,$dst,$ix) >= 0} {
		if {[string length $line]} {
		    append line "+"
		}
	    }
	    append line "$currentData(amp,$src,$dst,$ix)"
	    append line "*exp(-t/$currentData(tau,$src,$dst,$ix))"
	}
    }
    if {[string length $line] == 0} {
	set line "0"
    }
    return $line
}

proc eccGraph {src dst} {
    global graphLine graphExpr
    global updateList lineButton

    updateWidgets $updateList	;# Flush changes to variables
    $lineButton($src,$dst) update
    set iline $graphLine($src,$dst)
    #puts "graph $src $dst as line $graphLine($src,$dst)"
    showGraph
    set graphExpr($iline) [makeEccExpr $src $dst]
    update_graph $iline
}

proc newPageCallback {src dst updateList} {
    global needAmpcheck amplimit amplimittext ampwarn ampwarntext
    global showWarning

    updateWidgets $updateList
    if {$needAmpcheck} {
	checkAmps
    }
    if { [string compare $dst b0] == 0} {
	$ampwarn($src) configure -text ""
	$amplimit($src) configure -text ""
	set showWarning($src) 0
    } elseif { [string compare $src nosrc] != 0} {
	$ampwarn($dst) configure -text $ampwarntext($dst)
	$amplimit($dst) configure -text $amplimittext($dst)
	set showWarning($src) 1
    }
}


############################# MAIN ##############################

foreach axis {x y z} {
    set nt [nTerms $axis b0]
    for {set ix 1} {$ix <= $nt} {incr ix} {
	set b0Ratio($axis,scale,$ix) 0.0
	set b0Ratio($axis,amp,$ix) 0.0
    }
}

tixNoteBook .book -options {
    nbframe.focusColor black
}

#frame .f0
frame .f1
frame .f2
frame .f3
frame .f4
frame .f5

frame .f1.f11

label .f3.coillabel -text "Gradient coil: "
set coilLabelWidget .f3.coillabel
getGcoil
pack .f3.coillabel -side left

foreach dstAxis { x y z b0 } {

    if {[string compare $dstAxis b0] != 0} {
	.book add $dstAxis \
	    -label [string toupper "$dstAxis"] \
	    -underline 0 \
	    -raisecmd "newPageCallback nosrc $dstAxis {$updateList}"
	set fm [.book subwidget $dstAxis]
	tixNoteBook $fm.book -options {
	    nbframe.focusColor black
	}
	set label [string toupper "$dstAxis"]
	set amplimittext($dstAxis) "Allowed $label compensation: xx%"
	label $fm.lampmax \
		-text $amplimittext($dstAxis)
	set amplimit($dstAxis) $fm.lampmax
	grid $fm.lampmax - - - - -
	label $fm.lampwarn -text "" -fg red3
	set ampwarn($dstAxis) $fm.lampwarn
	set ampwarntext($dstAxis) ""
	grid $fm.lampwarn - - - - -
	frame $fm.pad
	grid $fm.pad - - - - - -pady 2
    }

    foreach srcAxis { x y z } {
	if {[string compare $dstAxis b0] == 0} {
	    # Put this page under previous Dst page according to SrcAxis (!)
	    set fm [.book subwidget $srcAxis]
	    set pgname $dstAxis
	} else {
	    set pgname $srcAxis
	}
	set label [string toupper "${srcAxis}->$dstAxis"]
	$fm.book add $pgname -label $label \
		-raisecmd "newPageCallback $srcAxis $dstAxis {$updateList}"
	if {[string compare $srcAxis $dstAxis] == 0} {
	    $fm.book raise $pgname
	}
	set fn [$fm.book subwidget $pgname]
	set nterms [nTerms $srcAxis $dstAxis]
	#puts "nterms=$nterms"
	set row 1
	
	if {[string compare $dstAxis b0] == 0} {
	    label $fn.lampmax \
		    -text "\nAllowed $label compensation: 50%"
	    set amplimit($srcAxis,$dstAxis) $fn.lampmax
	    grid $fn.lampmax - - - - - -row $row
	    incr row
	    label $fn.lampwarn -text "" -fg red3
	    set ampwarn($srcAxis,$dstAxis) $fn.lampwarn
	    grid $fn.lampwarn - - - - - -row $row
	    incr row
	    frame $fn.pad
	    grid $fn.pad - - - - - -pady 4 -row $row
	    incr row
	}

	label $fn.lenb -text "Enable"
	label $fn.lt -text "Tau (ms)"
	label $fn.lx -text ""
	label $fn.la -text "Amplitude (%)"
	grid $fn.lenb $fn.lt - $fn.lx $fn.la -
	incr row

	for {set ix 1} {$ix <= $nterms} {incr ix} {
	    # Initialize values
	    set currentData(tau,$srcAxis,$dstAxis,$ix) $taumin
	    #set logtau($srcAxis,$dstAxis,$ix) [expr log10($taumin)]
	    set currentData(amp,$srcAxis,$dstAxis,$ix) 0

	    checkbutton $fn.enb$srcAxis$ix \
		    -command "checkAmp $srcAxis $dstAxis dummy" \
		    -variable currentData(enb,$srcAxis,$dstAxis,$ix)

	    tixControl $fn.ct$srcAxis$ix \
		    -variable currentData(tau,$srcAxis,$dstAxis,$ix) \
		    -incrcmd tauInc \
		    -decrcmd tauDec \
		    -command "tauCallback $srcAxis $dstAxis $ix" \
		    -min $taumin \
		    -max $taumax
	    set updateList [concat $updateList $fn.ct$srcAxis$ix]
	    $fn.ct$srcAxis$ix subwidget entry configure -width $tauwidth
	    set tauWidg($srcAxis,$dstAxis,$ix) $fn.ct$srcAxis$ix

	    tixControl $fn.ca$srcAxis$ix \
		    -variable currentData(amp,$srcAxis,$dstAxis,$ix) \
		    -command "ampCallback $srcAxis $dstAxis $ix" \
		    -step .01 -min $ampmin -max $ampmax
	    set updateList [concat $updateList $fn.ca$srcAxis$ix]
	    $fn.ca$srcAxis$ix subwidget entry configure -width 9
	    set ampWidg($srcAxis,$dstAxis,$ix) $fn.ca$srcAxis$ix

	    grid $fn.enb$srcAxis$ix $fn.ct$srcAxis$ix x x \
		    $fn.ca$srcAxis$ix x -row $row -pady $boxspace
	    incr row
	}

	frame $fn.f1
	button $fn.f1.gbut -text "Graph" -command "eccGraph $srcAxis $dstAxis"
	label $fn.f1.glab -text "as line number"
	tixControl $fn.f1.gctrl -min 1 -max 3 -value 1 \
		-variable graphLine($srcAxis,$dstAxis)
	set lineButton($srcAxis,$dstAxis) $fn.f1.gctrl
	pack $fn.f1.gbut $fn.f1.glab $fn.f1.gctrl -side left -padx 2 -pady 3
	grid $fn.f1 - - - - - -row $row -sticky w -pady 2

    }
    grid $fm.book - - - - - -padx 5 -pady 5
}


.book add limit -label Limits -underline 0 -raisecmd {updateWidgets $updateList}
set fn [.book subwidget limit]
set row 1
set trise [varFromGradtable {trise %f}]
set trise [expr $trise * 1e6]
label $fn.ltrise -text "Rise time in grad table: $trise \xb5s\n "
grid $fn.ltrise - - -row $row
incr row
label $fn.lslew -text "Rise time\n(\xb5s)"
label $fn.lduty -text "Duty cycle\n(%)"
grid x $fn.lslew $fn.lduty -row $row
incr row
foreach axis {x y z} {
    label $fn.l$axis -text [string toupper $axis]

    tixControl $fn.slew$axis \
	    -variable currentData(slewlimit,$axis) \
	    -command {setChanged $knownModified} \
	    -validatecmd slewValidate \
	    -incrcmd slewInc \
	    -decrcmd slewDec
    $fn.slew$axis subwidget entry configure -width 7
    set updateList [concat $updateList $fn.slew$axis]
    set slewlimitWidg($axis) $fn.slew$axis

    tixControl $fn.duty$axis \
	    -variable currentData(dutylimit,$axis) \
	    -command {setChanged $knownModified} \
	    -min 0 -max 100
    set updateList [concat $updateList $fn.duty$axis]
    $fn.duty$axis subwidget entry configure -width 6
    set dutylimitWidg($axis) $fn.duty$axis

    grid $fn.l$axis $fn.slew$axis $fn.duty$axis -row $row \
	    -padx 10 -pady $boxspace
    incr row
}


.book add scale -label Scale -underline 0 -raisecmd {updateWidgets $updateList}
set fn [.book subwidget scale]
set row 1
set gmax [varFromGradtable {gmax %f}]
label $fn.lgmax -text "Gmax in grad table: $gmax gauss/cm\n "
grid $fn.lgmax - - - -row $row
incr row
label $fn.lecc -text "ECC (%)"
label $fn.lshim -text "Shims (%)"
label $fn.ltot -text "Overall (%)"
grid x $fn.lecc $fn.lshim $fn.ltot -row $row
incr row
set currentData(eccscale,b0) 100	;# For limiting sum of B0 terms
foreach axis {x y z} {
    label $fn.l$axis -text [string toupper $axis]

    tixControl $fn.ecc$axis \
	    -variable currentData(eccscale,$axis) \
	    -command "setAmplimit amplimit($axis) $axis" \
	    -validatecmd "eccscaleValidation $fn.ecc$axis  $axis" \
	    -min 0 -max 10 -step 1
    set updateList [concat $updateList $fn.ecc$axis]
    $fn.ecc$axis subwidget entry configure -width 7
    set eccscaleWidg($axis) $fn.ecc$axis

    tixControl $fn.shim$axis \
	    -variable currentData(shimscale,$axis) \
	    -command {setChanged $knownModified} \
	    -min 0 -max 10 -step 2
    set updateList [concat $updateList $fn.shim$axis]
    $fn.shim$axis subwidget entry configure -width 6
    set shimscaleWidg($axis) $fn.shim$axis

    # NB: autorepeat must be off for following widget, or it will
    # get stuck on if a warning dialog pops up in response to a
    # button press. [Turned off warning dialogs.]
    tixControl $fn.total$axis \
	    -variable currentData(totalscale,$axis) \
	    -command "totalscaleCallback $axis {$knownModified}" \
	    -validatecmd "totalscaleValidation $fn.total$axis $axis" \
	    -autorepeat true \
	    -min -100 -max 100 -step 10
    set updateList [concat $updateList $fn.total$axis]
    $fn.total$axis subwidget entry configure -width 9
    set totalscaleWidg($axis) $fn.total$axis

    grid $fn.l$axis $fn.ecc$axis $fn.shim$axis $fn.total$axis \
	    -padx 2 -pady $boxspace -row $row
    incr row
}
checkbutton $fn.bcouple \
	-text "B0 amplitudes track Overall scaling" \
	-variable coupleB0
set coupleB0 1
grid $fn.bcouple - - - -padx 2 -pady 6 -row $row
incr row


button .f1.f11.loadButton -text "Load" \
	-command "fileLoad user" -state disabled
set buttonList [concat $buttonList .f1.f11.loadButton]

button .f1.f11.saveButton -text "Save" \
	-command "fileSave user currentData" -state disabled
set buttonList [concat $buttonList .f1.f11.saveButton]

if {$vnmrComm} {
    button .f1.f11.goButton -text "Save & Go" \
	    -command "vnmrGo currentData" -state disabled
}
set goButton .f1.f11.goButton

pack .f1.f11.loadButton .f1.f11.saveButton \
	 -side left -padx 2
if {$vnmrComm} {
    pack .f1.f11.goButton -side left -padx 2
    set buttonList [concat $buttonList .f1.f11.goButton]
}
pack .f1.f11 -side left -expand true -anchor center

label .f5.flabel \
	-text "ECC File: "
tixComboBox .f5.fsel \
	-fancy false \
	-editable true \
	-dropdown true \
	-history false \
	-prunehistory false \
	-labelside left \
	-command fileEntryCallback \
	-selectmode immediate \
	-variable eccFileName
set flist [lsort -dictionary [glob -nocomplain [getEccDir]/*]]
set fileSelector .f5.fsel
foreach path $flist {
    .f5.fsel appendhistory [file tail $path]
}
pack .f5.flabel .f5.fsel -side left -padx 5 -anchor n
pack .f5.fsel -side left -fill x -expand 1

label .f4.lstat -text "(Unmodified)"
set statusWidget .f4.lstat
label .f4.lver -text "Version:    "
set versionWidget .f4.lver
button .f4.purgeButton -text "Purge ..." \
	-command "filePurge" -state disabled
set buttonList [concat $buttonList .f4.purgeButton]
pack .f4.lstat -side left -padx 4
pack .f4.purgeButton -side right -padx 4
pack .f4.lver -side right -padx 4

pack .f3 -pady 5
pack .book -pady 2 -padx 10
pack .f1 -padx 10 -pady 5 -fill x
pack .f5 -pady 5 -fill x
pack .f4 -pady 5 -fill x


checkEccDir
fileLoadX [getMasterFilePath] currentData 0
