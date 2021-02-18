#!/bin/sh
# chkconfig: 5 99 99
# description: Saves and restores
### BEGIN INIT INFO
# Required-Start: $network
# Default-Start: 5
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

# set -x 

if [[ -d /etc/rc.d ]]; then
   . /etc/rc.d/init.d/functions
fi

vnmrsystem=/vnmr
($vnmrsystem/acqbin/ovjProcs start >/dev/console &)

bootp_d=/usr/sbin/bootpd
if [[ -f $bootp_d ]]; then
   ($bootp_d -s > /dev/console &)
fi

if [[ -x $vnmrsystem/web/scripts/ovjWeb ]]; then
   ($vnmrsystem/web/scripts/ovjWeb start >/dev/console &)
fi
