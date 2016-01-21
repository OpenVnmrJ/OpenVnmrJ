#! /bin/sh
#
# simple convenience wrapper for
# running the Turbo VNC viewer 
#
#set -x

if [ -x /opt/TurboVNC/bin/vncviewer ]
then
  /opt/TurboVNC/bin/vncviewer -losslesswan $* &
else
   echo "Turbo VNC is not installed"
fi
