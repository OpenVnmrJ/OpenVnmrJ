#!/bin/sh
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

#
# script to setup,disable, check status for sharing the Desktop via  /etc/Xorg.conf and VNC
# e.g. :0 sharing via VNC
# Usage
#    DtSharCntrl start [passwd] - configure and set vnc password, 
#                                 if passwd not given use the previous store in passwd.bkup
#    DtSharCntrl stop - move the /root/.vnc/passwd to passwd.bkup thus disabling access to Desktop
#    DtSharCntrl status - the various states of shared based on the passwd presents
#

# set -x

#
# For Ubuntu use the system provided Vino interface, rather than the X11 Server for VNC
#
distro=`lsb_release -is`    # RedHatEnterpriseClient or Ubuntu
if [ x$distro = "xUbuntu" ]; then
   echo "On Ubuntu, Use System->Preferences->Remote Desktop, to enable desktop sharing"
   exit 1
fi

#
# For RHEL 6.X  use the system provided Vino interface, rather than the X11 Server for VNC
#
if [ "$(echo $distro  | grep -i 'RedHat' > /dev/null;echo $?)" == "0" ]; then
   if [ "$(lsb_release -rs | grep 6 > /dev/null;echo $?)" == "0" ]; then
      echo "On RHEL 6.X, Use System->Preferences->Remote Desktop, to enable desktop sharing"
      exit 1
   fi
fi

#
# need to be root
#
# userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
userId=`/usr/bin/id -u`
if [ $userId != "0" ]; then
   echo "  `basename $0` Must be root to run.."
   exit 1
fi

if [ $# -lt 1 ]; then
   echo "`basename $0` needs at least one argument"
   # echo "`basename $0` useage: `basename $0` start [password] | stop | status"
   exit 1
fi

#
# check that a native nvidia driver is being used, otherwise the vnc client we get garbage.
#
/bin/grep Driver /etc/X11/xorg.conf | grep nvidia > /dev/null
if [ $? -ne 0 ]
then
   echo "Native Nvidia driver not installed, Install driver before proceeding"
   exit 1
fi

#
# check that the xorg.conf has been initially configured with nvidia-xconfig
#
/bin/grep Load /etc/X11/xorg.conf | grep glx > /dev/null
if [ $? -ne 0 ]
then
   echo "The xorg.conf does not appeared to be configured properly, please execute nvidia-xconfig"
   exit 1
fi

if [  x"$vnmrsystem" = "x" ]
then
    # with 5.3 the vnmrsystem env is not set when VnmrJ adm sudo makeuser
    # thus test if this has been envoke as a sudo command
    # could test SUDO_USER, SUDO_GID, or SUDO_COMMAND
    # if SUDO_USER has a value then don't ask for vnmrsystem just default 
    # to /vnmr     GMB 5/4/2009
    vnmrsystem="/vnmr"
fi

if [ $1 = "start" ]; then
   #
   #  configure the turbo vnc server
   #
   # echo "$vnmrsystem/bin/turbovncsetup"
   $vnmrsystem/bin/turbovncsetup  > /dev/null 2>&1
   if [ $? -gt 0 ]; then
      echo "nvc setup error $?"
      exit 1
   fi

   #
   #  configure the /etc/Xorg.conf file for VNC
   #
   # echo "$vnmrsystem/bin/modxorg.py"
   $vnmrsystem/bin/modxorg.py > /dev/null 2>&1 
   xorgresult=$?  # 0 = no mod needed, 1 = gdm restart needed, > 9 error
   if [ $? -gt 9 ]; then
      echo "xorg error $?"
      exit 1
   fi

   #
   #  set the root VNC server password for the desktop
   #
   if [ $# -gt 1 ]; then   # $2 will be the password
      # echo "/usr/bin/vncpasswd $2"
      # This script is executed via sudo and the $HOME ends up as the users
      # home directory.  We need the passwd to go into root, so hard code the path.
      # make the /root/.vnc directory is present, otherwise the vncpasswd will
      # silently fail.
      if [ ! -d /root/.vnc ]; then
         mkdir -p /root/.vnc
      fi

      /usr/bin/vncpasswd /root/.vnc/passwd > /dev/null <<EOF
$2
$2
EOF
   else  # restore previous password from the passwd.bkup file, i.e. move it back
      if [ -f /root/.vnc/passwd ]; then
         echo "already started"
         exit 0
      elif [ -f /root/.vnc/passwd.bkup ]; then
         mv  /root/.vnc/passwd.bkup /root/.vnc/passwd
      else
         echo "No previous password found, try again using a password"
         exit 1
      fi
   fi
   if [ $xorgresult -eq 0 ]; then
      # First word of successful return MUST be "Sharing", do not change.
      echo "Sharing Enabled"
   elif [ $xorgresult -eq 1 ]; then
      # X server needs to be restarted
      echo "Log out and log back in to restart the X server."
   fi
# Any status returned with the work "active" will be intrepreted as enabled.
# Do not change that without changing the java code using it.
elif [ $1 = "status" ]; then
   if [ ! -d /root/.vnc ] ; then
      echo "no root .vnc dir"
   elif [ -f /root/.vnc/passwd ]; then
      echo "passwd active"
   elif [ -f /root/.vnc/passwd.bkup ]; then
      echo "backup passwd present"  # stopped
   else
      echo "stopped"
   fi
elif [ $1 = "connected" ]; then
     # lsof | grep Xorg | grep ESTABLISHED
     # will be a line for each remote system connected.
     # output:
     #    Xorg 3438 root 45u IPv4 14421  TCP vornak:5900->pantera.nmr.varianinc.com:54156 (ESTABLISHED)
     # awk 8th field would be 'vornack:5900->pantera.nmr.varianinc.com:49646'
     # vornak is the Xserver (Xorg) machine port 5900 (vnc of desktop) connect to (->) pantera
     # awk 2nd field with a field separator '->' would be 'pantera.nmr.varianinc.com:49646'
     # awk 1st field with a field separator ':' would be 'pantera.nmr.varianinc.com'
     # connected=`lsof | grep Xorg | grep ESTABLISHED | awk 'BEGIN { FS = " " } { print $8 }' | awk 'BEGIN { FS = "->" } { printf "%s\n",$2 }'`
     connected=`lsof | grep Xorg | grep ESTABLISHED | awk 'BEGIN { FS = " " } { print $8 }' | 
        awk 'BEGIN { FS = "->" } { print $2 }' | awk 'BEGIN { FS = ":" } { print $1 }'`
     echo $connected
elif [ $1 = "stop" ]; then
   if [ -f /root/.vnc/passwd ]; then
      mv /root/.vnc/passwd /root/.vnc/passwd.bkup
   else
      # no passwd file found
      echo "no passwd file found to disable"
      exit 1
   fi
   # force any VNC Client sessions to disconnect 
   /usr/bin/vncconfig -display :0 -disconnect
   # First word of successful return MUST be "Sharing", do not change.
   echo "Sharing Disabled"
fi
exit 0
