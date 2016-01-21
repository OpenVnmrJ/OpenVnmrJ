#!/bin/sh
# '@(#)S99pgsql.sh 22.1 03/24/08 1991-2004 '
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

# Make sure that /usr is mounted
if [ ! -d /usr/bin ]; then
   exit 1
fi

if [ x`uname -s` = "xLinux" ]
then
    /bin/mount -a
    #determine if we should use su (redhat) or sudo (debian)
    if [ -r /etc/debian_version ]
    then
        lflvr="debian"
    else
        lflvr="rhat"
    fi
fi
if [ x`uname -s` = "xDarwin" ]
then
    lflvr="darwin"
fi

LOGNAME=root
vnmrsystem=/vnmr
PGLIB="$vnmrsystem/pgsql/lib"
PGDATA="$vnmrsystem/pgsql/data"
PGDATABASE=vnmr
login_user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
vnmradm=` ls -l "$vnmrsystem/vnmrrev" | awk '{ print $3 }' `
# Remove any whitespace
vnmradm=`echo $vnmradm`
PATH='/etc:/usr/etc:/usr/bin:/bin:"$vnmrsystem/bin":"$vnmrsystem/pgsql/bin"'
export LOGNAME vnmrsystem vnmruser login_user vnmradm PGLIB PGDATA PGDATABASE PATH

# if $vnmrsystem/pgsql/bin/pg_ctl, use that path, else use no path and
# let it use the system one

if [ x$lflvr = "xdebian" ]
then
    # Ubuntu needs a path.  Unfortunately, the path will be different
    # if a newer version is installed.  Postgres is normally installed
    # in /usr/lib/postgresql/ in a directory name which is the version number.
    # Look in /usr/lib/postgresql/ and find the dir with the largest number.
    dirlist=`ls -1 /usr/lib/postgresql`

    version="0.0"
    for dir in $dirlist
    do
        if [ $( echo "$version < $dir" | bc ) = 1 ]; then
            version=$dir
        fi
    done

    pgpath="/usr/lib/postgresql/"$version/bin/

fi
if [ x$lflvr = "xdarwin" ]
then
    # Mac needs a fullpath much like Ubuntu above
    # First, see if pg_ctl is found in the PATH
    whichout=`which pg_ctl`
    if [ $? -eq 0 ]
    then
        # "which" found a path, use the directory portion with "/" added
         pgpath=`dirname $whichout`/
    else
        dirlist=`ls -1 /Library/PostgreSQL`
        version="0.0"
        for dir in $dirlist
        do
            if [ $( echo "$version < $dir" | bc ) = 1 ]; then
                version=$dir
            fi
        done
        pgpath="/Library/PostgreSQL/"$version/bin/
    fi
fi
if [ x$lflvr = "xrhat" ]
then
    # Redhat does not seem to need a path to find the cmds
    pgpath=""
fi

file="$vnmrsystem/pgsql/bin/pg_ctl"
if [ -f "$file" ]
then
    pgpath="$vnmrsystem/pgsql/bin/"
fi
echo "Using postgres path: $pgpath"

case "$1" in
'start')
        # Ubuntu needs to have a writable directory "/var/run/postgresql"
        # or it fails to start the server.
        if [ x$lflvr = "xdebian" ]
        then
            if [ ! -d /var/run/postgresql ]
            then
                sudo mkdir /var/run/postgresql
            fi
            sudo chown "$vnmradm" /var/run/postgresql
        fi

        if [ -d "$vnmrsystem/pgsql/data" ]
        then
             if [ "x$login_user" != "x$vnmradm" ]
             then
                 echo "starting postmaster, owner=$vnmradm"
                 if [ -f "$vnmrsystem/pgsql/data/postmaster.pid" ]
                 then
                    rm -f "$vnmrsystem/pgsql/data/postmaster.pid"
                 fi
                 if [ x$lflvr = "xrhat" ]; then
                     su "$vnmradm" -c $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D /vnmr/pgsql/data -o '-N 10 -B 45 -i -c sort_mem=100000'"
                 fi
                 if [ x$lflvr = "xdarwin" ]; then
                     echo Executing: sudo -u "$vnmradm" -i $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D $vnmrsystem/pgsql/data"
                     sudo -u "$vnmradm" -i $pgpath"pg_ctl start -l $vnmrsystem/pgsql/pgsql.log -D $vnmrsystem/pgsql/data"
                     # If this script is allowed to complete and terminate, launchd
                     # will teminate the postgres server.  Keep this script alive.
                     while [ true ]; do
                         sleep 5000
                     done
                 fi
                 if [ x$lflvr = "xdebian" ]; then
                     sudo -u "$vnmradm" -i ${pgpath}pg_ctl start -l "$vnmrsystem"/pgsql/pgsql.log -o '-N 10 -B 45 -i -c sort_mem=100000'
                 fi
             else
                 echo must have been login user
                 $pgpath"pg_ctl" start -l "$vnmrsystem/pgsql/pgsql.log"  -o "-N 10 -B 45 -i -c sort_mem=100000"
             fi
        fi
        ;;

'stop')
        if [ "x$login_user" != "x$vnmradm" ]
        then
            echo "switching user to database owner=$vnmradm"
            if [ x$lflvr = "xdebian" ]; then
               sudo -u "$vnmradm" -i $pgpath"pg_ctl" stop -m fast
            fi
            if [ x$lflvr = "xrhat" ]; then
               su "$vnmradm" -c "$vnmrsystem/bin/S99pgsql stop"
            fi
            if [ x$lflvr = "xdarwin" ]; then
               sudo -u "$vnmradm" -c "$vnmrsystem/bin/S99pgsql stop"       
            fi
        else
            if [ -f "$vnmrsystem/pgsql/data/postmaster.pid" ]
            then
                echo "killing postmaster"
                $pgpath"pg_ctl" stop -m fast
            else
                echo "postmaster already killed"
            fi
        fi
        ;;

*)
        echo "Usage: $0 { start | stop }"
        exit 1
        ;;
esac
exit 0
