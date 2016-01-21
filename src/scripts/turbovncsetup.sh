#! /bin/sh
#
# run the Turbo VNC configuration script giving it
# the basic defaults know to work for Varian purposes
#
# set -x
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  echo
  echo "This script must be run with root privileges"
  echo "e.g. su or sudo to root."
  echo
  exit 1
fi

if [ -x /opt/VirtualGL/bin/vglserver_config ]
then
/opt/VirtualGL/bin/vglserver_config 1>/dev/null 2>/dev/null << +++
1
n
n
y
x
+++
else
   echo "Turbo VNC is not installed"
   exit 2
fi
exit 0
