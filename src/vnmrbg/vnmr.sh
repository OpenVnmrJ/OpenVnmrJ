# 
#! /bin/csh
#

# Set the port id (and optional host id) for VnmrJ

set port_id = -1
if ($#argv > 1) then
   if ( $1 == "-port" ) then
      set port_id = "$2"
   endif
endif
set host_id = "no_host_id"
if ($#argv > 3) then
   if ( $3 == "-host" ) then
      set host_id = "$4"
   endif
endif
echo "vnmr: vnmrj_port_id $port_id vnmrj_host_id $host_id"


# If we are at the console, do an autostart window

set ostype = `vnmr_uname`
if ( $ostype == "AIX" ) then
   if ( `tty | sed 's/[1-9]/0/'` == "/dev/hft/0" ) then
        echo -n " Motif Windows starting (^C aborts) "
        xinit -bs
   else
	if ( $port_id == "-1" ) then
	    vn&
	else
	    if ( $host_id == "no_host_id" ) then
              vn -port $port_id&
	    else
              vn -port $port_id -host $host_id&
	    endif
	endif
   endif
   exit 1
else if ( $ostype == "IRIX" ) then
	if ( $port_id == "-1" ) then
	    vn&
	else
	    if ( $host_id == "no_host_id" ) then
              vn -port $port_id&
	    else
              vn -port $port_id -host $host_id&
	    endif
	endif
        exit 1
endif
# User is in the SUN environment
if (`tty` == "/dev/console") then
   echo "OpenWindows starting (^C aborts)"
   setenv DISPLAY unix:0.0
   openwin -noauth
else
   if ( $port_id == "-1" ) then
      vn&
   else
      if ( $host_id == "no_host_id" ) then
          vn -port $port_id&
      else
          vn -port $port_id -host $host_id&
      endif
   endif
endif

