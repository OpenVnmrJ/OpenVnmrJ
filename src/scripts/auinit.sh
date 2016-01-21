: '@(#)auinit.sh 22.1 03/24/08 1999-2002 '
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
#!/bin/sh
#

cd /etc/security
case "$1" in
    'start')

	echo 'Starting Unix auditing.'
	/sbin/init 1 >/dev/msglog 2>&1 
	(./bsmconv 2>&1 >/dev/null << +++
y	
+++
)
	/sbin/init 6 >/dev/msglog 2>&1 
	;;

    'stop')

	/sbin/init 1 >/dev/msglog 2>&1 
	(./bsmunconv 2>&1 >/dev/null << +++
y	
y
+++
)
	
	/sbin/init 6 >/dev/msglog 2>&1 
	;;

    *)
	echo "Usage: $0 { start | stop }"
	exit 1
	;;
esac
