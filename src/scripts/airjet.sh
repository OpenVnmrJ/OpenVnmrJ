#!/bin/bash
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

#############################################################
# airjet script                                             #
#                                                           #
# TCLTK script to control SOLO temperature controler found  #
# on new AIRJET XR refrigeration unit                       #
#                                                           #
# Version 1.1                                               #
#                                                           #
# June 2013                                                 #
# last mod:  April 2016                                     #
# written by Pierre Audet, Universite Laval, Qc, Canada     #
# pierre.audet@chm.ulaval.ca                                #
#                                                           #
#############################################################

#############################################################
# Configuration parameters                                  #
#############################################################

# Usually the USB-485M will get the default /dev/ttyUSB0 port
set port /dev/ttyUSB0

# When debugflg is true, extra messages are printed in the terminal window
# this is for debugging purpose only.  For normal operation this flag should
# be set false
set debugflg false

# set commflg true to enable communication
# or false for testing the GUI and/or development
set commflg true

# setpointflg sets to false
set setpointflg false
set gettempflg true

############################################################
# some local procedures                                    #
############################################################
proc lshift {inputlist} {
   upvar $inputlist argv
   set arg  [lindex $argv 0]
   set argv [lreplace $argv 0 0]
   return $arg
}

proc debugmsg { message } {
    global debugflg
    if {$debugflg} { puts $message }
}

proc usage {} {
  puts {\
Usage:  airjet [options]
 -h                : print this message
 -debug            : print more info on terminal for debugging purpose
 -setpoint value   : change temperature setpoint to value then exit script
 -gettemp          : get current temperature from unit then exit script
        
  August 2013, Pierre Audet, Universite Laval, Qc, Canada.
  Contact: pierre.audet@chm.ulaval.ca }
}

#############################################################
# Reads in argument lines                                   #
# if no argument provided, then read the temperature        #
# if -setpoint is present, issue the setpoit to the         #
# controller and exit from the script                       #
#############################################################

if { [catch {exec stty -F $port clocal}]} {
  if { $commflg != false } {
    # No answer from the USB dongle
    # check for the correct rules.d file
    if {![file exists "/etc/udev/rules.d/99-CP210x.rules"]} {
    	puts "Airjet Fault:Cannot open $port, exiting"
	puts "File /etc/udev/rules.d/99-CP210x.rules not found"
	exit
    }
    set rc [catch { exec /sbin/udevadm info --query=path --name=$port } msg ]
    if { [string compare $msg "device node not found"] == 0 } {
    	puts "Airjet Fault: cannot open $port, exiting"
	puts "The AutomationDirect USB-485M dongle not detected or not working"
        exit
    }
    puts "Airjet Fault: unknow error, exiting"
    exit
  }
}
 
if {[llength $argv] > 0} {
  while {[llength $argv] > 0} {
    set arg [lshift argv]
    switch -exact -- $arg {
      -h        { usage }
      -setpoint { set setpointflg true; set temp [lshift argv]; set gettempflg false }
      -gettemp  { set gettempflg true }
      -debug    { set debugflg true }
       default  { set gettempflg true }
    }
  }
}

##############################################################
# Conversion routines                                        #
##############################################################

proc dec2hex {var} { return [format %02x $var] }
proc hex2dec {var} { return [expr 0x$var] }
proc asc2hex {var} { return [dec2hex [scan $var %c]] }
proc hex2asc {var} { return [format %c [hex2dec $var]] }

###############################################################
# Linear Redundance Check routine                             #
###############################################################

proc LRCcalc {str} {
    set len [string length $str]
    set result 0
    for {set i 0} {$i < $len} {incr i 2} {
      set sub [string range $str $i [expr $i+1] ]
      incr result [expr 0x$sub]
    }
    return [format %02X [expr (($result ^ 0xFF) + 1) & 0xFF] ]
}

###############################################################
# Modbus ASCII communication routines                         #
###############################################################

proc modbussendasciimsg {message device timeoutms} {
    global debugflg
    set fh [open $device RDWR]
    fconfigure $fh -translation binary -mode 9600,e,7,1 -buffering none -blocking 0
    flush $fh
    set junk [read $fh]
    puts -nonewline $fh [string toupper $message]
    flush $fh
    set timeoutctr 0
    set reply ""
#   attend le premier caractere de la chaine
    while {[string length $reply] <= 0 && $timeoutms >= $timeoutctr }  {
        binary scan [read $fh] a* ascii
        if {$ascii != ""} {
            append reply $ascii
        }
        after 5
        incr timeoutctr 5
    }
    if {$timeoutms < $timeoutctr} {
	close $fh
	return "timeout"
    }
#   Lit le reste jusqu'au LF
    while { [ asc2hex [string index $reply end]] != "0a" && $timeoutms >= $timeoutctr }  {
        binary scan [read $fh] a* ascii
        if {$ascii != ""} {
            append reply $ascii
        }
        after 5
        incr timeoutctr 5
    }
    if {$timeoutms < $timeoutctr} {
        set reply "timeout"
    }
    close $fh
    return $reply
}

proc modbusasciicommand {str device timeoutms} {
    global commflg
    append message ":" $str [LRCcalc $str] \u000D \u000A
    if { $commflg } {
        set result [modbussendasciimsg $message $device $timeoutms]
    } else {
        set result 0000000000000
    }
    if {$result == "timeout"} {return $result}
    set result [string range $result 7 end-4]
    return $result
}

###############################################################
# Solo Temperature controler routines                         #
###############################################################

proc getTemp {port} {
    global debugflg
    set readPV 010310000001
    set reply [ modbusasciicommand $readPV $port 1000]
    CheckTimeoutError $reply
    if {$debugflg} {puts -nonewline "  > Register 1000: $reply  "}
    scan $reply %x ttemp
    if { $ttemp < 1000 } {
      set temp [expr (($ttemp ) / 10.0)]
    } else {
      set temp [expr (($ttemp - 65536) / 10.0)]
    }
    return $temp
}
proc getTempSetPoint {port} {
    global debugflg
    set readPV 010310010001
    set reply [ modbusasciicommand $readPV $port 1000]
    CheckTimeoutError $reply
    if {$debugflg} {puts "Register 1001: $reply"}
    scan $reply %x ttemp
    if { $ttemp < 1000 } {
      set temp [expr (($ttemp ) / 10.0)]
    } else {
      set temp [expr (($ttemp - 65536) / 10.0)]
    }
    return $temp
}
proc setTempSetPoint { T port} {
    global afterpid debugflg
    set setSV 01061001
    if {$T >= 0} {
        append setSV [format %04x [expr int($T * 10)]]
    } else {
        set setSVn [format 0x%.8x [expr int($T * 10) & 0xFFFFFFFF]]
        append setSV [string range $setSVn 6 end  ]
    }
    set setSV [string toupper $setSV]
    debugmsg "  > Writing to Register 1001: $setSV"
    modbusasciicommand $setSV $port 1000

}

proc CheckTimeoutError { str } {
   if { [string compare $str "timeout"]  == 0 } {
      puts "Airjet Fault:  Timeout Communication Error"
      puts "Please check the cable between the Airjet unit and the host computer"
      puts "or make sure the Airjet unit has a communication port (older unit doesn't have one)"
      exit
   }
}

if { $setpointflg } {
   setTempSetPoint $temp $port
}

if { $gettempflg } {
   puts [expr int([getTemp $port] * 10)]
}

