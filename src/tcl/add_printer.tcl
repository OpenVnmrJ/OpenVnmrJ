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

set VNMR_DIR         $env(vnmrsystem)
set DEVICENAMES     $VNMR_DIR/devicenames
set DEVICETABLE     $VNMR_DIR/devicetable
set TEMP_FILE        $VNMR_DIR/tmp/tempfile

set SOL_PRINTER_DIR     /etc/lp/printers
global device_type vnmr_pr_names pname_del sol_rev
global pname_add ptype_add used_as port_name baud_rate remote_host remote_os

set device_type ""

;###############
;# positionWindow -
;# w: the name of the window to position.
;#
;###########################################################################
proc positionWindow w {
    wm geometry $w +400+200
}

;###############
;# you_are_root -
;#
;###########################################################################
proc you_are_root {} {
 
    if { [lindex [ split [exec id] () ] 1] == "root" } {
       return 1
    }
    return 0
}


;############### 
;# resetAll -
;# 
;###########################################################################
proc resetAll {} {
    global pname_add ptype_add port_name baud_rate
    global used_as remote_host remote_os defaults_pr

        set pname_add ""                                        
        set ptype_add ""                                        
        set port_name ""                                        
        set baud_rate ""                                        
        set used_as ""                                        
        set remote_host ""                                        
        set remote_os ""                                        
        set default_pr ""                                        
    return 0
}


;############### 
;# getSolPrinterType -
;# 
;###########################################################################
proc getSolPrinterType {printer_name} {
     global SOL_PRINTER_DIR device_type
     
     set f [open $SOL_PRINTER_DIR/$printer_name/configuration r]

     while {[gets $f Line] >= 0} {
        if {[string match [lindex $Line 0] "Remote:"]} {
           set device_type "remote"
           return 0
        } else {
              if {[string match [lindex $Line 1] "/dev/bpp0"]} {
                 set device_type "parallel"
                 return 0
              }
          }
     }
     set device_type "serial"
     close $f
     return 0
}


;############### 
;# getVnmrPrName -
;# 
;###########################################################################
proc getVnmrPrName {} {
  global DEVICENAMES

  set f [open $DEVICENAMES r]
  set pname "" 
  while {[gets $f Line] >= 0} {
      if {[string match [lindex $Line 0] "Name"]} {
          lappend pname [lindex $Line 1]
      }
  }
  close $f
  if { $pname == "" } {
     return 0
  } else { return $pname }
}


;############### 
;# isValidVnmrName -
;# 
;###########################################################################
proc isValidVnmrName {name} {
   global vnmr_pr_names

   set vnmr_pr_names [getVnmrPrName]
   foreach i $vnmr_pr_names {
      if {[string match $i $name]} {
         return 1
      }
   }
   return 0
}

proc getSolPrNames {} {
   #set sol_rev [string range [exec uname -r ] 0 2]
   #set sol_rev  [exec uname -r]
   #if { $sol_rev == "5.6" } {}

   if { [file exists /etc/printers.conf] } {
      set fd [open /etc/printers.conf r]
      set sol_pr_names {}
      while {[gets $fd Line] >= 0} {
         set aa [lindex $Line 0]
         set p_name [string index $aa 0]
         if { ($p_name != "#") && ($p_name != ":") } {
            set bb [string trimright "$aa" ":\\"]
            if { $bb != "_default" } {
               lappend sol_pr_names $bb
            }
         }
      }
      close $fd
   } else {
        set sol_pr_names [exec ls /etc/lp/printers]
     }
   return $sol_pr_names
}

;############### 
;# isValidSolName -
;# 
;###########################################################################
proc isValidSolName {name} {

    #set cc [getSolPrNames]
    #foreach i $cc {}
    foreach i [getSolPrNames] {
       if {[string match $i $name]} {
          return 1 
       }
    }   
    return 0
}


;###############
;# isPlotter -
;# 
;###########################################################################
proc isPlotter {type} {
    global DEVICETABLE

    set f [open $DEVICETABLE r]
    while {[gets $f Line ] >= 0} {
        if {[string match [lindex $Line 1] $type]} {
           break
        }
    }
    gets $f Line
    if {[string match pl* [lindex $Line 1]]} {
        close $f
        return 1
    }
    
    close $f
    return 0
}


;############### 
;# displayAddPrinter -
;# 
;###########################################################################
proc displayAddPrinter {} {
    global device_type vnmr_pr_names baud_rate default_pr r_label_text
    global ras_num remote_host port_name serial_A serial_B p_port

        pack forget .topD .buttonsD
    wm title . "VNMR :  ADD   Printer/Plotter"

    eval destroy [winfo child .topA]
    eval destroy [winfo child .mid]
    eval destroy [winfo child .bottom]
    eval destroy [winfo child .port]
    eval destroy [winfo child .baud]
    eval destroy [winfo child .remote]
    eval destroy [winfo child .remote1]
    eval destroy [winfo child .buttonsA]

        pack .topA -side top -pady 2m -fill x
        pack .bottom -side top -pady 2m -fill x
        pack .mid -side top -pady 2m -fill x
        pack .port -side top -pady 2m -fill x
        pack .baud -side top -pady 2m -fill x
        pack .remote -side top -pady 2m -fill x
        pack .remote1 -side top -pady 2m -fill x
        pack .buttonsA -side bottom -fill x -pady 2m

;#        if {[file exists "/dev/ecpp0"]} {
;#           set p_port "/dev/ecpp0"
;#        } else { set p_port "/dev/bpp0" }

    label .topA.namelabel -text "Printer name:" -font 8x13bold
    entry .topA.nameentry  -highlightthickness 0 -width 20 -relief sunken -textvariable pname_add
    checkbutton .topA.button -text "Default" -font 8x13bold -variable default_pr \
             -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink -command {  }
       pack .topA.namelabel .topA.nameentry -side left -padx 4m -pady 2m
       pack .topA.button -side right -padx 25m -pady 2m -ipadx 8 -ipady 2

    bind .topA.nameentry <Return> {
       if { [isValidVnmrName $pname_add] } {
          dialog .d {Add devices}  "Printer  \"$pname_add\"  already exist in Vnmr " {} -1 OK 
          return 1
       } else {
            if { [isValidSolName $pname_add] } {
               set k [ dialog .d {Add devices}  "Printer \"$pname_add\"  already exist in Solaris print service . \
This procedure may or may not modify the configuration of this printer \n
Do you want to continue ?" {} -1 CONTINUE Cancel ]
               if { $k == 1 } { return 1 }
            }   
         }
   }

    label .bottom.typelabel -text "Printer type:" -font 8x13bold
    label .bottom.typeentry  -bd 1 -highlightthickness 1 -width 20 -relief flat -textvariable ptype_add
        pack .bottom.typelabel .bottom.typeentry -side left -padx 4m -pady 2m
 
    listbox .bottom.typelistbox -relief raised -borderwidth 1 -yscrollcommand ".bottom.scroll set"
    scrollbar .bottom.scroll -command ".bottom.typelistbox yview"
        pack .bottom.scroll -side right -fill y
        pack .bottom.typelistbox -side right -fill x
 

    label .mid.usedas -text " Used as: " -font 8x13bold
    radiobutton .mid.printing -text "Printer" -font 8x13bold -variable used_as -value Printer \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
    radiobutton .mid.plotting -text "Plotter" -font 8x13bold -variable used_as -value Plotter \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
;#radiobutton .mid.both -text Both -font 8x13bold -variable used_as -value Both \
;#                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
 
        pack .mid.usedas -side left
        pack .mid.printing .mid.plotting -side left -ipadx 4 -padx 4 -pady 2 -ipadx 6 -ipady 2


    label .port.label -text " Port: " -font 8x13bold
    radiobutton .port.terma -text $serial_A -font 8x13bold -variable port_name -value $serial_A \
             -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink -command {
                       pack forget .remote .remote1
                       pack .baud -fill x
                       pack .baud.rate -side left
                       pack .baud.fast .baud.medium .baud.slow .baud.veryslow -side left \
                                             -padx 4 -pady 2 -ipadx 6 -ipady 2
                       set remote_host ""
                       set remote_os ""
             }
    radiobutton .port.termb -text $serial_B -font 8x13bold -variable port_name -value $serial_B \
             -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink -command {
                       pack forget .remote .remote1
                       pack .baud -fill x
                       pack .baud.rate -side left
                       pack .baud.fast .baud.medium .baud.slow .baud.veryslow -side left \
                                             -padx 4 -pady 2 -ipadx 6 -ipady 2
                       set remote_host ""
                       set remote_os ""
             }
    radiobutton .port.parallel -text "Parallel" -font 8x13bold -variable port_name -value $p_port \
             -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink -command {
                       set baud_rate ""
                       pack forget .baud .remote .remote1
                       set remote_host ""
                       set remote_os ""
             }
    radiobutton .port.remote -text "Remote" -font 8x13bold -variable port_name -value "remote" \
             -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink -command {
                       set baud_rate ""
                       pack forget .baud
                       pack .remote -fill x
                       pack .remote1 -fill x
                       pack .remote.namelabel .remote.nameentry -side left -padx 4m -pady 2m
                       pack .remote1.namelabel .remote1.bsd .remote1.s5  -side left -padx 4m -pady 2m
             }
        pack .port.label -side left
        pack .port.terma .port.termb .port.parallel .port.remote -side left \
                                         -padx 4 -pady 2 -ipadx 6 -ipady 2


    label .baud.rate -text "   Baud rate:  " -font 8x13bold
    radiobutton .baud.veryslow -text "2400" -font 8x13bold -variable baud_rate -value 2400 \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
    radiobutton .baud.slow -text "9600" -font 8x13bold -variable baud_rate -value 9600 \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
    radiobutton .baud.medium -text "19200" -font 8x13bold -variable baud_rate -value 19200 \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
    radiobutton .baud.fast -text 38400 -font 8x13bold -variable baud_rate -value 38400 \
                  -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink

;#pack .baud.rate -side left
;#pack .baud.fast .baud.medium .baud.slow .baud.veryslow -side left -padx 4 -pady 2 -ipadx 6 -ipady 2

 
    label .remote.namelabel -text " Remote host name:" -font 8x13bold
    entry .remote.nameentry  -highlightthickness 0 -width 20 -relief sunken -textvariable remote_host
        pack .remote.namelabel .remote.nameentry -side left -padx 4m -pady 2m


    label .remote1.namelabel -text "Remote host OS :" -font 8x13bold
    radiobutton .remote1.bsd -text $r_label_text -font 8x13bold -variable remote_os -value bsd \
                      -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink
    radiobutton .remote1.s5 -text "System V" -font 8x13bold -variable remote_os -value S5 \
                      -bd 1 -highlightthickness 0 -indicatoron 0 -selectcolor pink

        pack .remote1.namelabel .remote1.s5 .remote1.bsd -side left -padx 4m -pady 2m -ipadx 6 -ipady 2

;#    set execstring "grep PrinterType /vnmr/devicetable | awk '{print \$2}' "
;#    set temp [exec sh -c $execstring]

    foreach i [getVnmrPrType] {
     .bottom.typelistbox insert end  $i
    }
 
    bind .bottom.typelistbox <ButtonRelease-1> {
      set ptype_add [selection get]

      if {$ras_num($ptype_add) == 0} {
          set used_as Plotter
      } else { set used_as "" }
 
      ;# remember to remove isPlotter{}
      ;#if {[isPlotter $ptype_add]} {
      ;#    set used_as Plotter 
      ;#} else { set used_as "" }

    }

    button .buttonsA.apply -text "Apply" -command "applyAddPrinter"
    button .buttonsA.reset -text "Reset" -command {
                                      resetAll
                                      pack forget .baud .remote .remote1
                                   }
    button .buttonsA.cancel -text Cancel -command "exit"

        pack .buttonsA.apply .buttonsA.reset .buttonsA.cancel -side left -expand 1 -fill x
        pack forget .baud .remote .remote1
    focus .topA.nameentry
    return 0
}

;#xxxxxxxxx

;###############
;# getVnmrPrType -
;#
;###########################################################################
proc getVnmrPrType {} {
  global DEVICETABLE ras_num

  set f [open $DEVICETABLE r]
  set tname ""
  while {[gets $f Line] >= 0} {
      if {[string match [lindex $Line 0] "PrinterType"]} {
          set aa [lindex $Line 1]
          lappend tname  $aa
      } else {
           if {[string match [lindex $Line 0] "raster"]} { 
              set ras_num($aa) [lindex $Line 1]
           }
        }
  }
  close $f
  return $tname
}


;###############
;# applyAddPrinter -
;#
;###########################################################################
proc applyAddPrinter {} {
  global DEVICENAMES DEVICETABLE device_type pname_add ptype_add used_as port_name baud_rate 
  global VNMR_DIR remote_host remote_os sys_os default_pr

  ;#  set ff [open $DEVICETABLE r]

  ;#ccccc
  ;#if { ($sys_os == "Linux") } {
  ;#   exec $VNMR_DIR/bin/managelnxdev  $pname_add $ptype_add $used_as $port_name $remote_host \
  ;#                                    $remote_os $baud_rate $default_pr
  ;#   return 0
  ;#}
       if { [isValidVnmrName $pname_add] } {
          dialog .d {Add devices}  "Printer  \"$pname_add\"  already exist in Vnmr " {} -1 OK
          return 1
       } else {
           if { ($sys_os != "Linux") } {
            if { [isValidSolName $pname_add] } {
               set k [ dialog .d {Add devices}  "Printer \"$pname_add\"  already exist in Solaris print service . \
This procedure may or may not modify the configuration of this printer \n
Do you want to continue ?" {} -1 CONTINUE Cancel ]
               if { $k == 1 } { return 1 }
            }
           }
         }

  if { ($port_name == "") } {
     dialog .d {Add Printer/Plotter}  " Cannot  Add  Printer / Plotter \n \n Incomplete  Informations" {} -1 OK
     return 1
  }

  if { ($port_name == "/dev/term/a") || ($port_name == "/dev/term/b") } {
     if { ($used_as == "") || ($pname_add == "") || ($ptype_add == "") || ($baud_rate == "") } {
     dialog .d {Add Printer/Plotter}  " Cannot  Add  Printer / Plotter \n \n Incomplete  Informations" {} -1 OK
        return 1
     }
  } else {
       if { ($port_name == "remote") } {
          if { ($used_as == "") || ($pname_add == "") || ($ptype_add == "") \
                                 || ($remote_host == "") || ($remote_os == "") } {
     dialog .d {Add Printer/Plotter}  " Cannot  Add  Printer / Plotter \n \n Incomplete  Informations" {} -1 OK
             return 1
          } else { 
             if { ($sys_os != "Linux") } {
               catch {exec ping $remote_host} tempvar
               after 500

               if { ![string match *live $tempvar] } {
                  dialog .d {Add devices}  " Can not communicate with host \"$remote_host\" " {} -1 OK
                  return 1
               }
             }
            }
       } else {    ;#Parallel printer
            if {($used_as == "") || ($pname_add == "") || ($ptype_add == "")} {
     dialog .d {Add Printer/Plotter}  " Cannot  Add  Printer / Plotter \n \n Incomplete  Informations" {} -1 OK
               return 1
            }
         }
    }

  set f [open $DEVICENAMES r]
  ;########### is printer name already exist in /vnmr/devicenames ? ############
  while {[gets $f Line] >= 0} {
     if {[string match [lindex $Line 1] $pname_add]} {
        dialog .d {Add devices}  "Printer  \"$pname_add\"  already  exist" {} -1 OK
        return 1
     }
  }
  close $f

;#  ;########### is it a right printer type in /vnmr/devicetable ? ###############
;#  set matched 0
;#  while {[gets $ff Line] >= 0} {
;#     if {[string match [lindex $Line 1] $ptype_add]} {
;#        set matched 1
;#        break
;#     } 
;#  }
;#  close $ff
;#
;# if { $matched == 0 } {
;#     dialog .d {Add devices}  " \"$ptype_add\"  is not a valid Printer Type" {} -1 OK
;#     return 1
;#  }

;#  set k [dialog .d {Add devices} "Add  printer  \"$pname_add\" ? " \
;#                                                        {} -1 OK Cancel]
;#  if {$k == 1} {
;#     return 1
;#  }

    if { ($sys_os == "Linux") } {
       exec $VNMR_DIR/bin/managelnxdev  $pname_add $ptype_add $used_as $port_name $remote_host \
                                      $remote_os $baud_rate $default_pr
       #resetAll
       displayAddPrinter
       return 0
    }

    addSolPr
    addVnmrPr
    return 0
}


;############### 
;# addSolPr -
;# 
;###########################################################################
proc addSolPr {} {
    global DEVICENAMES pname_add used_as ptype_add port_name baud_rate
    global remote_host remote_os default_pr ras_num sol_rev


    ;####### remote ######################
    if { $port_name == "remote" } {
       if { $sol_rev >= "5.6" } {
          if { $remote_os == "bsd" } {
             catch {[exec lpset -n system -a bsdaddr=$remote_host,$pname_add $pname_add]} 
          } else {
             catch {[exec lpset -n system -a bsdaddr=$remote_host,$pname_add,Solaris $pname_add]} 
            }
       } else {
            catch {[exec lpsystem -t $remote_os $remote_host]}
            exec lpadmin -p $pname_add -s $remote_host -T unknown -I any
         }

    } else {
    ;######### local #####################

         catch {[exec chown lp $port_name]}
         catch {[exec chmod 600 $port_name]}
         catch {[exec lpadmin -p $pname_add -v $port_name]} 
 
         switch $ras_num($ptype_add) {

                0 { ;#Plotter
                    exec lpadmin -p $pname_add -T unknown
                    exec lpadmin -p $pname_add -I any
                  }
 
                1 -
                2 { ;#Both
                    if {[string match $used_as "Plotter"]} {
                       exec lpadmin -p $pname_add -T unknown
                    } else { exec lpadmin -p $pname_add -T hplaser }

                    exec lpadmin -p $pname_add -I simple
                  }
           
                3 -
                4 { ;#PS
                    exec lpadmin -p $pname_add -T PS
                    exec lpadmin -p $pname_add -I postscript
 
                    cd /etc/lp/fd
                    set aa [exec ls /etc/lp/fd]
                    foreach filter $aa {
                       exec lpfilter -f [file rootname $filter]  -F $filter
                    }
                    exec lpfilter -f postio -F postio.fd
                    exec lpfilter -f postior -F postior.fd
                    exec lpfilter -f postprint -F postprint.fd
                    exec lpfilter -f postreverse -F postreverse.fd
                  }
         }


         if {[string match $port_name "/dev/term/a"] || [string match $port_name "/dev/term/b"]} {
            exec lpadmin -p $pname_add -o "stty='$baud_rate -opost'" 
         }
      }
  
    if {$default_pr == 1} {
       exec lpadmin -d $pname_add
    }

;######common for both remote and local
    catch {[exec lpadmin -p $pname_add -o nobanner]}
    catch {exec accept $pname_add}
    catch {exec enable $pname_add}

    return 0
}


;############### 
;# addVnmrPr -
;# 
;###########################################################################
proc addVnmrPr {} {
    global DEVICENAMES pname_add used_as ptype_add port_name baud_rate
    global remote_host remote_os remote_host

;#     have to check here which printer needs -opost
;#  if { ($device_type == "serial") && ($used_as == "Plotter") } {

;#   set execstring "lpadmin -p $pname_add -o \"stty='$baud_rate -opost'\" "
;#   exec sh -c $execstring
;#  }   

  if {$port_name == "remote"} {
     set p_host $remote_host
  } else { set p_host [exec uname -n] }

  set f [open $DEVICENAMES a+]
  puts $f "##############################"
  puts $f "Name    $pname_add"
  puts $f "Use     $used_as"
  puts $f "Type    $ptype_add"
  puts $f "Host    $p_host"
  puts $f "Port    $port_name"
  puts $f "Baud    $baud_rate"
  puts $f "Shared  No"

  close $f

  return 0
}


;###############
;# showHelp -
;#
;###########################################################################
proc showHelp {} {
#    set file [string range $w 1 end]
    global r_label_text

    if ![winfo exists .help] {
        toplevel .help
        positionWindow  .help
        frame .help.buttons
        pack .help.buttons -side bottom -expand 1 -fill x

        button .help.buttons.cancel -text Cancel -command "destroy .help"
        pack .help.buttons.cancel -side left -expand 1
        text .help.text -width 95 -height 40 -yscrollcommand ".help.scroll set" -setgrid 1
        pack .help.text -side left -expand 1 -fill both
        scrollbar .help.scroll -command ".help.text yview"
        pack .help.scroll -side right -fill y
    } else {
        wm deiconify .help
        raise .help
    }

    set helptxt " \n\
                    INFORMATION  ON   \" ADD  Printer / Plotter \" \n\


    \n\
    \n\
       MENU BAR:\n\
    \n\
    .    File \n\
    .       Quit             Quit exits from the Add Printer/Plotter window. \n\
    .\n\
    .    Edit                Select either Add or Delete printer \n\
    .       Add Printer      Add Printer opens a blank Add Printer/Plotter window \n\
    .                        so you can add a new printer or plotter.\n\
    .\n\
    .       Delete Printer   Delete Printer opens the Delete Printer/Plotter  \n\
    .                        window. This window shows a list of currently    \n\
    .                        installed VNMR printers/plotters . \n\
    .                        Double-clicking on an output device and clicking Apply \n\
    .                        deletes the device from VNMR. Devices are only deleted if\n\
    .                        they were originally added with the Add Printer/Plotter\n\
    .                        window. Devices added with Solaris AdminTool are not deleted. \n\
    .\n\
    .    View                Viewing printer status of both VNMR and SOLARIS \n\
    .       Vnmr Printer     View Vnmr Printers provides a list of output devices, \n\
    .                        along with some setup information, available to VNMR      \n\
    .\n\
    .       Linux Printer    View Linux Printers provides a list of output devices,\n\
    .                        along with setup and availability information available \n\
    .                        to Linux Print Service.      \n\
    .\n\
    .       Solaris Printer  View Solaris Printers provides a list of output devices,\n\
    .                        along with setup and availability information available \n\
    .                        to Solaris Print Service.      \n\
    .\n\
    .    Help \n\
    .       Help             To get you here  \n\
    \n\
    \n\
       WINDOW INPUT AREA: \n\
    \n\
    .    Printer name        Printer name entry box is where you enter the name of \n\
    .                        the printer or plotter which you want to add . \n\
    .                        You can enter any name that is 14 characters or less \n\
    .                        using letters, numbers, dashes, and underscores.\n\
    .\n\
    .    Default             Click on the default button to select the default printer for \n\
    .                        the VNMR system . Each system should  have a default printer.\n\
    .\n\
    .    Printer type        Printer type box is to display the selected printer type which \n\
    .                        you double-click onto the right hand side scrolling list-box \n\
    .                        displayed all currently device types available for your system \n\
    .\n\
    .    Scrolling window    This window is called the list-box which display all the VNMR \n\
    .                        device types. These types are from  /vnmr/devicetable file . \n\
    .\n\
    .    Used as             To select whether the device will be used as a printer or plotter. \n\
    .\n\
    .    Port                To select how the device is connected to the host computer: \n\
    .                            /dev/term/a is serial port A on Sun computer, \n\
    .                            /dev/term/b is serial port B on Sun computer, \n\
    .                        Parallel is the parallel port, and \n\
    .                        Remote is for remote printer set up \n\
    .\n\
    .      Baud rate         To provide the  selections for the device baud rate. The baud- \n\
    .                        rate buttons appear only when a serial port is selected. \n\
    .\n\
    .      Remote host name  This entry box is where you enter the name of the remote computer \n\
    .                        which controls the printer you want to access. The remote host name \n\
    .                        field appears only  when Remote button is selected. \n\
    .\n\
    .      Remote host OS    To seclect the UNIX OS flavor running on the remote host. \n\
    .                        Select System V for Solaris, IRIX, or AIX; select $r_label_text \n\
    .                        for SunOS or Linux. \n\
    .                        The remote host OS selection appears only when Remote  \n\
    .                        button is selected. \n\
    \n\
    \n\
       ACTION BAR:\n\
    \n\
    .    Apply               To set up the printer or plotter in VNMR and Solaris.\n\
    .\n\
    .    Reset               To clear the window entries.\n\
    .\n\
    .    Cancel              To cancel the printer setup and \n\
    .                        reduces the Add Printer/Plotter window.\n\
    \n\
    "

    wm title .help "HELP"
    wm iconname .help "Help"
    .help.text delete 1.0 end
    .help.text insert end  "\n\n $helptxt"
    .help.text mark set insert 1.0
    return 0
}


;###############
;# showUnixPrinters -
;#
;###########################################################################
proc showUnixPrinters {} {
    global SOL_PRINTER_DIR sys_os

    if { ($sys_os == "Linux") } {
         return 0
    }

#    set file [string range $w 1 end]

    if ![winfo exists .solpr] {
        toplevel .solpr
        positionWindow  .solpr
        frame .solpr.buttons
        pack .solpr.buttons -side bottom -expand 1 -fill x

        button .solpr.buttons.cancel -text Cancel -command "destroy .solpr"
        pack .solpr.buttons.cancel -side left -expand 1
        text .solpr.text -height 40 -yscrollcommand ".solpr.scroll set" -setgrid 1
        pack .solpr.text -side left -expand 1 -fill both
        scrollbar .solpr.scroll -command ".solpr.text yview"
        pack .solpr.scroll -side right -fill y
    } else {
        wm deiconify .solpr
        raise .solpr
    }     
    wm title .solpr "SOLARIS    Printers / Status"
    wm iconname .solpr "Sol printers"
    .solpr.text delete 1.0 end
    #.solpr.text insert 1.0 [exec ls $SOL_PRINTER_DIR]
    .solpr.text insert 1.0 [join [getSolPrNames] "\n"]
    update idletasks
    .solpr.text insert end "\n\n [exec lpstat -t]"
    .solpr.text mark set insert 1.0
    return 0
}


;############### 
;# showVnmrPrinters -
;#
;###########################################################################
proc showVnmrPrinters {} {   
  global DEVICENAMES vnmr_pr_names
#    set file [string range $w 1 end]
 
    set vnmr_pr_names [getVnmrPrName]

    if {$vnmr_pr_names == 0} {
       dialog .d {Delete printer}  "NO printer available in vnmrsytem" {} -1 OK
       return 1
    }

    if ![winfo exists .vnmrpr] {
        toplevel .vnmrpr
        positionWindow  .vnmrpr
        frame .vnmrpr.buttons
        pack .vnmrpr.buttons -side bottom -expand 1 -fill x
 
        button .vnmrpr.buttons.cancel -text Cancel -command "destroy .vnmrpr"
        pack .vnmrpr.buttons.cancel -side left -expand 1
        text .vnmrpr.text -height 40 -yscrollcommand ".vnmrpr.scroll set" -setgrid 1
        pack .vnmrpr.text -side left -expand 1 -fill both
        scrollbar .vnmrpr.scroll -command ".vnmrpr.text yview"
        pack .vnmrpr.scroll -side right -fill y
    } else {
        wm deiconify .vnmrpr
        raise .vnmrpr
    }    
    wm title .vnmrpr "VNMR  Printers  Configuration"
    wm iconname .vnmrpr "Vnmr printers"
    .vnmrpr.text delete 1.0 end

    foreach i $vnmr_pr_names {
         .vnmrpr.text insert end  "$i \n"
    }   
 
    .vnmrpr.text insert end  \n\n\n


    set f [open $DEVICENAMES r]
    while {[gets $f Line] >= 0} {
        if {[string match [lindex $Line 0] "Name"]} {
           .vnmrpr.text insert end  "$Line \n"

            for {set i 1} {$i <= 6} {incr i} {
               gets $f Line
               .vnmrpr.text insert end  "$Line \n"
            }

            .vnmrpr.text insert end  \n
        }  
    }
    close $f

    .vnmrpr.text mark set insert 1.0
    return 0
}



;############### 
;# displayDeletePrinter -
;# 
;###########################################################################
proc displayDeletePrinter {} {
    global vnmr_pr_names pname_del
      if {[getVnmrPrName] == 0} {
         resetAll
         displayAddPrinter 
         return 1
      }

      pack forget .topA .mid .bottom .port .baud .remote .remote1 .buttonsA
      pack .topD -side top -pady 2m -fill x
      pack .buttonsD -side bottom -fill x -pady 2m
    wm title . "VNMR :  DELETE  Printer/Plotter"

    eval destroy [winfo child .topD]
    eval destroy [winfo child .buttonsD]
    set vnmr_pr_names [getVnmrPrName]
    set pname_del ""

    label .topD.namelabel -text "Printer name:" -font 8x13bold
    label .topD.nameentry -bd 1 -highlightthickness 1 -width 20 -relief flat -textvariable pname_del
       pack .topD.namelabel .topD.nameentry -side left -padx 4m -pady 2m
 
    bind .topD.nameentry <Return> {
       if { ![isValidVnmrName $pname_del] } {
          dialog .d {Delete devices}  " \"$pname_del\"  is not a valid Vnmr Printer Name" {} -1 OK
          return 1
       }   
    }
 
    listbox .topD.namelistbox -relief raised -borderwidth 1 -yscrollcommand ".topD.scroll set"
    scrollbar .topD.scroll -command ".topD.namelistbox yview"
       pack .topD.scroll -side right -fill y
       pack .topD.namelistbox -side right -fill x
 
    foreach i $vnmr_pr_names {
         .topD.namelistbox insert end  $i
    }
    bind .topD.namelistbox <ButtonRelease-1> {
        set pname_del [selection get]
    }

    button .buttonsD.apply -text "Apply" -command {applyDelete $pname_del}
    button .buttonsD.reset -text "Reset" -command {set pname_del ""}
    button .buttonsD.cancel -text Cancel -command {
                                 pack forget .topD .mid .bottom .buttonsD
                                 wm title . "VNMR :  Configure  Printer/Plotter"
                              }
       pack .buttonsD.apply .buttonsD.reset .buttonsD.cancel -side left -expand 1 -fill x

}


;############### 
;# applyDelete -
;# 
;#       Remove deleted printer informations from /vnmr/devicenames
;#          and Solaris Print Service 
;# 
;# 
;###########################################################################
proc applyDelete {name} {
    global VNMR_DIR DEVICENAMES TEMP_FILE sys_os

    if { $name == "" } {
       dialog .d {Delete printer}  "Double click onto the listbox to select a printer name " {} -1 OK
       return 1
    }

    if { ($sys_os == "Linux") } {
         exec $VNMR_DIR/bin/managelnxdev  delete $name
         displayDeletePrinter
         return 0
    }
                                                                                                                  
    exec touch $TEMP_FILE
    set TEMP [open $TEMP_FILE w]
    set f [open $DEVICENAMES r]
    while {[gets $f Line] >= 0} {
       if {[string match [lindex $Line 0] "Name"] && [string match [lindex $Line 1] $name]} {
          for {set i 1} {$i <= 7} {incr i} {
                gets $f Line
          }
       } else { puts $TEMP $Line }
    }

    close $f
    close $TEMP
    exec cp $TEMP_FILE $DEVICENAMES
    exec rm $TEMP_FILE

;#  this part might be bothered by the messages returned from unix
    exec reject $name
    exec disable $name
    exec lpadmin -x $name

    displayDeletePrinter
    return 0
}


;############### 
;# displayMainWindow -
;# 
;###########################################################################
proc displayMainWindow {} {
     updateMainWindow
}


;############### 
;# updateMainWindow -
;# 
;###########################################################################
proc updateMainWindow {} {
   global device_type vnmr_pr_names view_prlabel

   eval destroy [winfo child .]
   wm title . "VNMR :  Configure  Printer/Plotter"
   wm iconname . "Adddevices"
   positionWindow .

    frame .mbar -width 10c -height 2c -relief raised -bd 2
    frame .topA -relief raised -borderwidth 0 -width 8c -height 2c   ;#printer name
    frame .topD -relief raised -borderwidth 0 -width 8c -height 2c   ;#printer name
    frame .mid -relief raised -borderwidth 0                         ;#used as
    frame .bottom -relief raised -borderwidth 0 -width 8c -height 5c ;#printertype
    frame .port -relief raised -borderwidth 0 -width 8c -height 2c   ;#port name
    frame .baud  -relief raised -borderwidth 0
    frame .remote  -relief raised -borderwidth 0
    frame .remote1  -relief raised -borderwidth 0
    frame .buttonsA
    frame .buttonsD

      pack .mbar -side top -fill x
      pack .topA -side top -pady 2m -fill x
      pack .mid -side top -pady 2m -fill x
      pack .bottom -side top -pady 2m -fill x
      pack .port -side top -pady 2m -fill x
      pack .baud -side top -pady 2m -fill x
      pack .remote -side top -pady 2m -fill x
      pack .remote1 -side top -pady 2m -fill x
      pack .buttonsA -side bottom -fill x -pady 2m
      pack .buttonsD -side bottom -fill x -pady 2m

   menubutton .mbar.file -text File  -menu .mbar.file.menu
   menubutton .mbar.edit -text Edit  -menu .mbar.edit.menu
   menubutton .mbar.view -text View  -menu .mbar.view.menu
   menubutton .mbar.help -text Help  -menu .mbar.help.menu

   menu .mbar.file.menu
      .mbar.file.menu add command -label "Quit" -command "exit"
   menu .mbar.edit.menu
      .mbar.edit.menu add command -label "Add Printer" -command {
                                             resetAll
                                             displayAddPrinter
                                          }

      .mbar.edit.menu add command -label "Delete Printer" -command {
                     if {[getVnmrPrName] == 0} {
                           dialog .d {Delete printer}  "NO printer in vnmrsytem" {} -1 OK
                     } else { displayDeletePrinter }
                  }
   menu .mbar.view.menu
      .mbar.view.menu add command -label "Vnmr Printers" -command "showVnmrPrinters"
      .mbar.view.menu add command -label $view_prlabel   -command "showUnixPrinters"

   label .mbar.dummylabel -text "                  "
   menu .mbar.help.menu
      .mbar.help.menu add command -label "Help" -command "showHelp"

      pack .mbar.file -side left -padx 1m -ipadx 2m -fill x
      pack .mbar.edit -side left -padx 1m -ipadx 2m -fill x
      pack .mbar.view -side left -padx 1m -ipadx 2m -fill x
      pack .mbar.dummylabel -side left
      pack .mbar.help -side right -padx 1m -ipadx 2m -fill x
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


;############ Main ##############################

;#move this root checking section to /vnmr/bin/adddevices
;#if { ![you_are_root] } {
;#     puts ""
;#     puts "      Only Superuser (root) can run this command !"
;#     puts ""
;#     exit 1
;#}

set sol_rev  [exec uname -r]
set sys_os  [exec uname -s]

if { ($sys_os == "Linux") } {
     set r_label_text "BSD/Linux"
     set view_prlabel "Linux Printers"
     set serial_A "serial_A"
     set serial_B "serial_B"
     set p_port "parallel"

} else {
     set r_label_text "BSD"
     set view_prlabel "Solaris Printers"
     set serial_A "/dev/term/a"
     set serial_B "/dev/term/b"

     if {[file exists "/dev/ecpp0"]} {
        set p_port "/dev/ecpp0"
     } else { set p_port "/dev/bpp0" }
}

displayMainWindow
pack forget .topA .mid .bottom .port .baud .remote .remote1 .buttonsA .buttonsD 
displayAddPrinter
