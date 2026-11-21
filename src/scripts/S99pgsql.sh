#!/bin/bash
#
### BEGIN INIT INFO
# Required-Start: $network
# Default-Start: 5
# description: Starts and stops postgreq database
### END INIT INFO
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#

# Make sure that /usr is mounted
if [ ! -d /usr/bin ]; then
   exit 1
fi

stop_func() {
   if [ "x$login_user" != "x$vnmradm" ]
   then
            # echo "switching user to database owner=$vnmradm"
            if [ x$lflvr = "xdebian" ]; then
               sudo -u "$vnmradm" -i $pgpath"pg_ctl" stop -m fast
            elif [ x$lflvr = "xrhat" ]; then
               su "$vnmradm" -c "$vnmrsystem/bin/S99pgsql stop"
            elif [ x$lflvr = "xdarwin" ]; then
               sudo -u "$vnmradm" -c "$vnmrsystem/bin/S99pgsql stop"       
            fi
   else
            if [ -f "$vnmrsystem/pgsql/data/postmaster.pid" ]
            then
                # echo "killing postmaster"
                $pgpath"pg_ctl" stop -m fast
            fi
   fi
}


if [ x$(uname -s) = "xLinux" ]
then
    #determine if we should use su (redhat) or sudo (debian)
    if [ -r /etc/debian_version ]
    then
        lflvr="debian"
    else
        lflvr="rhat"
    fi
elif [ x$(uname -s) = "xDarwin" ]
then
    lflvr="darwin"
fi

LOGNAME=root
vnmrsystem=/vnmr
PGLIB="$vnmrsystem/pgsql/lib"
PGDATA="$vnmrsystem/pgsql/data"
PGDATABASE=vnmr
login_user=$(id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/')
vnmradm=$(ls -l "$vnmrsystem/vnmrrev" | awk '{ print $3 }')
# Remove any whitespace
vnmradm=$(echo $vnmradm)
PATH='/etc:/usr/etc:/usr/bin:/bin:"$vnmrsystem/bin":"$vnmrsystem/pgsql/bin"'
export LOGNAME vnmrsystem vnmruser login_user vnmradm PGLIB PGDATA PGDATABASE PATH

file="$vnmrsystem/pgsql/bin/pg_ctl"
pgpath="$vnmrsystem/pgsql/bin/"

case "$1" in
'start')
        if [ ! -d "$vnmrsystem/pgsql" ] || [ -f "$vnmrsystem/pgsql/persistence/LocatorOff" ]
        then
           exit 0
        fi

        if [ -f "$vnmrsystem/pgsql/data/postmaster.pid" ]; then
           stop_func
	fi
        if [ -d "$vnmrsystem/pgsql/data" ]
        then
             if [ "x$login_user" != "x$vnmradm" ]
             then
                 # echo "starting postmaster, owner=$vnmradm"
                 if [ x$lflvr = "xrhat" ]; then
                     su "$vnmradm" -c $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D /vnmr/pgsql/data -o '-k /tmp -N 20 -B 45 -i -c sort_mem=100000'"
                 fi
                 if [ x$lflvr = "xdarwin" ]; then
                     # echo Executing: sudo -u "$vnmradm" -i $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D $vnmrsystem/pgsql/data"
                     sudo -u "$vnmradm" -i $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D $vnmrsystem/pgsql/data"
                     # If this script is allowed to complete and terminate, launchd
                     # will teminate the postgres server.  Keep this script alive.
                     while [ true ]; do
                         sleep 5000
                     done
                 fi
                 if [ x$lflvr = "xdebian" ]; then
                     sudo -u "$vnmradm" -i ${pgpath}pg_ctl start -l "$vnmrsystem"/pgsql/pgsql.log -o '-k /tmp -N 20 -B 45 -i -c sort_mem=100000'
                 fi
             else
                 # echo must have been login user
                 $pgpath"pg_ctl" start -l "$vnmrsystem/pgsql/pgsql.log"  -o "-k /tmp -N 20 -B 45 -i -c sort_mem=100000"
             fi
        fi
        ;;

'stop')
	stop_func
        ;;

*)
        echo "Usage: $0 { start | stop }"
        exit 1
        ;;
esac
exit 0
