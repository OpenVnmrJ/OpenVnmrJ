#! /bin/sh
#
# simple convenience wrapper for
# running the Turbo VNC server 
#
# set -x

if [ -x /opt/TurboVNC/bin/vncserver ]
then
  if [ $# -lt 1 ]; then
    echo ""
    echo "Usage $0 [-kill] :displaynumber"
    echo "e.g. turbovncserver :10"
    echo "e.g. turbovncserver -kill :10"
    echo ""
    exit
  fi

  /opt/TurboVNC/bin/vncserver $*
else
   echo "Turbo VNC is not installed"
fi
