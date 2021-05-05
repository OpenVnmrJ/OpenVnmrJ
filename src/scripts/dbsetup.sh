#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
 
#-----------------------------------------
#  dbsetup script - setup PGSQL database
#-----------------------------------------
#  see show_usage() for explanation.

# This needs to be replaced by a dynamic way to know which postgres we
# are running.

# set -x

newpostgres="false"
#set -x
kill_procs()
{
    # Take care of difference in Mac ps command options
    if [ x`uname -s` = "xDarwin" ]
    then
        pg_pids=`ps -ax | grep $1 | grep -v grep | awk '{ print $1 }'`
    else
        pg_pids=`ps -e -o comm,pid | grep $1 | grep -v grep | awk '{ print $2 }'`
    fi

    pid_teststrg=`echo $pg_pids | tr -d " "`

    if [ x$pid_teststrg != "x" ]
    then
        for pg_proc in $pg_pids
        do
           kill $pg_proc  #don't use kill -9
        done
    fi
}

remove_db()
{
    if [ x$newpostgres = 'xtrue' ]
    then
        dropdb vnmr
    else

        echo "remove_db: Removing any existing DB"
        # Try killing the postmaster with pg_ctl then go after any remaining
        # processes found.
        # If pg_ctl exists in $vnmrsystem/pgsql/bin/, use it, 
        # else use system version.
        file="$vnmrsystem/pgsql/bin/pg_ctl"
            
        if [ -f "$vnmrsystem/pgsql/data/postmaster.pid" ]
        then
            if [ -f "$file" ]
            then
                $vnmrsystem/pgsql/bin/pg_ctl stop -m immediate -s -D "$vnmrsystem"/pgsql/data
            else
                $pgpath"pg_ctl" stop -m immediate -s -D "$vnmrsystem"/pgsql/data
            fi
        fi


        # kill all occurences of the postmaster processes
        echo "Killing any current local postmaster daemons"
        kill_procs "postmaster"
        kill_procs "postgres"


           rm -rf $vnmrsystem/pgsql/data

#        rm -rf $vnmrsystem/pgsql/persistence

        # Temproary in case this file is left behind
        rm -f /tmp/PGSQL_PORT

        # Killing Vnmrbg will also cause vnmrj to be killed
        kill_procs "Vnmrbg"
        echo DONE removing the database system owned by "$vnmr_adm".
    fi
}

show_usage()
{
# if no vnmr_adm is given, current user us used
# If one arg, default is to rebuild the DB which is what users are used
# to when they simply run dbsetup.
# If 4 args, preserve an existing DB.  This is to speed
# up installation by avoiding unnecessary rebuilding.
# The command 'dbsetup preserveDB' can be used after changing symbolic links
# to change versions of vnmrj.  It will check the validity and version 
# compatibility and start a new daemon running on the DB now being pointed to.
   echo ""
   echo "Usage:   $0 <remove|saveuser|preserveDB|standard|imaging|walkup> "
   echo "         $0 <vnmr_adm>"
   echo "         $0  vnmr_adm <remove|erase_users|standard|imaging|walkup>"
   echo "As Root: $0  vnmr_adm $vnmrsystem <standard|imaging|walkup>"
   echo "As Root: $0  vnmr_adm $vnmrsystem preserveDB prevVnmrPath"
   echo " "
   echo "Exiting $0 ..."
   echo ""
}

godoit() 
{
   osname=`uname -s`
   if [ $# -eq 2 ]
   then
      if [ x$2 = "xremove" ]
      then
          # Get owner of DB and compare with vnmr_adm or postgres
          if [ x$newpostgres = 'xtrue' ]
          then
              owner="postgres"
          else
              owner=`"$vnmrsystem"/bin/fileowner $PGDATA`
          fi
          # The new postgres also uses postgres as a user
          if [ x$newpostgres = 'xtrue']
          then
              dbadm="postgres"
          else
              dbadm=$vnmr_adm
          fi

          if [ x$owner != x$dbadm ]
          then
             echo ""
             echo "A database system owned by $vnmr_adm does NOT exist"
             echo "Exiting $0 ............"
             echo ""
             exit 1
          else
             echo "Previous Database being removed"
             remove_db           
             exit 1
          fi
      elif [ x$2 = "xerase_users" ]
      then
         rm -rf "$vnmrsystem"/adm/users/profiles
         rm -rf "$vnmrsystem"/adm/users/group
         rm -rf "$vnmrsystem"/adm/users/userlist
         exit 1
      elif [ x$2 = "xstandard" -o x$2 = "ximaging" -o x$2 = "xwalkup" ]
      then
          APP_MODE=$2
      elif [ x$2 = "xpreserveDB" ]
      then
          owner=`"$vnmrsystem"/bin/fileowner $PGDATA`
      elif [ x$2 = "xrebuildDB" ]
      then
          # If PGHOST is set and is not the local host, nor 'localhost',
          # then do not try to remove it
          localhost=`hostname`

          if test x$PGHOST = "x"
          then
              dbhost=$localhost
          else
              dbhost=$PGHOST
          fi

          if test $dbhost = "localhost"
          then
              dbhost=$localhost
          fi
          if test $dbhost = $localhost
          then
              owner=`"$vnmrsystem"/bin/fileowner $PGDATA`
				  dbadm=$vnmr_adm
              if [ x$owner = x$dbadm ]
              then
                  echo "Previous Database being removed"
                  remove_db           
              fi
          fi
      else
         show_usage
         exit 1
      fi 
   fi

   if [ "x$APP_MODE" = "x" ]
   then
      file="$vnmradm_home_dir/vnmrsys/global"
      if [ -f "$file" ]
      then
         APP_MODE=`cat "$file" | awk '/appmode/ {getline; print $NF}' | sed 's/"//g'`
      fi
   fi
   # If PGHOST is set and is not the local host, nor 'localhost',
   # then do not kill nor start a DB.
   # Get the local host name
   localhost=`hostname`

   if test x$PGHOST = "x"
   then
       dbhost=$localhost
   else
       dbhost=$PGHOST
   fi
   if test $dbhost = "localhost"
   then
       dbhost=$localhost
   fi
   # If we are using a remote DB server, then do not start one here
   if test $dbhost = $localhost
   then
      dbStatus="bad"
      # If a DB exists, see if its version is correct.
      owner=`"$vnmrsystem"/bin/fileowner $PGDATA`

      if [ x$newpostgres = 'xtrue' ]
      then
          # New Postgres, don't kill the server
          #echo "Previous Database being removed"
          #remove_db  
          echo "What to do here?"
      else
          if [ x$owner = x$vnmr_adm ]
          then
              echo "Found a database in $PGDATA owned by $vnmr_adm, checking it"
              # If pg_ctl exists in $vnmrsystem/pgsql/bin/, use it, 
              # else use system version.
              file="$vnmrsystem/pgsql/bin/pg_ctl"
              if [ -f "$file" ]
              then
                  # Stop it
                  $vnmrsystem/pgsql/bin/pg_ctl stop -m immediate -s -D "$vnmrsystem"/pgsql/data
                  # Restart it
                  if [ x$lflvr = "xdarwin" ]
                  then
                      $vnmrsystem/pgsql/bin/pg_ctl start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log
                  else
                      $vnmrsystem/pgsql/bin/pg_ctl start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log  -o "-k /tmp -N 20 -B 45 -i -c sort_mem=100000"
                  fi
              else
                  # Stop it
                  $pgpath"pg_ctl" stop -m immediate -s -D "$vnmrsystem"/pgsql/data
                  # Restart it
                  if [ x$lflvr = "xdarwin" ]
                  then
                      $pgpath"pg_ctl" start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log
                  else
                      $pgpath"pg_ctl" start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log  -o "-k /tmp -N 20 -B 45 -i -c sort_mem=100000"
                  fi
              fi
          else
              echo "No database found, we will create one"
              # Print the version of postgres we are using
              # If pg_ctl exists in $vnmrsystem/pgsql/bin/, use it, 
              # else use system version.
              # file="$vnmrsystem/pgsql/bin/pg_ctl"
              # if [ -f "$file" ]
              # then
              #     $vnmrsystem/pgsql/bin/pg_ctl --version
              # else
              #     $pgpath"pg_ctl" --version
              # fi

              dbStatus="none"
          fi
      fi

      # If we found something, ask it for its status
      if [ $dbStatus != "none" ]
      then
          sleep 4
          chmod a+r "$vnmrsystem"/pgsql/pgsql.log
          dbStatus=`"$vnmrsystem"/bin/managedb checkDBversion`
      fi
      if [ x$dbStatus != "xokay" ]
      then
          if [ x$dbStatus = "xnone" ]
          then
             dbStatus="bad"
          else
             echo "No database available or incompatible version"
             echo "Removing any local current database system owned by "
             echo "$vnmr_adm if necessary".
          fi
          remove_db

          rm -f /tmp/.s.PGSQL*

          echo "Initializing Postgres Database"
             # If initdb exists in $vnmrsystem/pgsql/bin/, use it, 
             # else use system version.
              file="$vnmrsystem/pgsql/bin/initdb"
              if [ -f "$file" ]
              then
                  $vnmrsystem/pgsql/bin/initdb --pgdata="$vnmrsystem"/pgsql/data &> "$vnmrsystem"/pgsql/init.log
              else
                  $pgpath"initdb" --pgdata="$vnmrsystem"/pgsql/data &> "$vnmrsystem"/pgsql/init.log
              fi

          if test ! -d "$vnmrsystem/pgsql/data"
          then
              echo "Database initialization failed!"
              exit 2
          fi

          # Allow any PC to connect to the database
          mv "$vnmrsystem/pgsql/data/pg_hba.conf" "$vnmrsystem/pgsql/data/pg_hba.conf.orig"
          cat "$vnmrsystem/pgsql/data/pg_hba.conf.orig" | sed 's/127.0.0.1\/32/0.0.0.0\/0/' > "$vnmrsystem/pgsql/data/pg_hba.conf"
          rm -f "$vnmrsystem/pgsql/data/pg_hba.conf.orig"

                  # If pg_ctl exists in $vnmrsystem/pgsql/bin/, use it, 
                  # else use system version.
              file="$vnmrsystem/pgsql/bin/pg_ctl"
              if [ -f "$file" ]
              then
                  if [ x$lflvr = "xdarwin" ]
                  then
                      $vnmrsystem/pgsql/bin/pg_ctl start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log
                  else
                      $vnmrsystem/pgsql/bin/pg_ctl start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log  -o "-k /tmp -N 20 -B 45 -i -c sort_mem=100000"
                  fi
              else
                  if [ x$lflvr = "xdarwin" ]
                  then
                      $pgpath"pg_ctl" start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log
                  else
                      $pgpath"pg_ctl" start -D "$vnmrsystem"/pgsql/data -l "$vnmrsystem"/pgsql/pgsql.log  -o "-k /tmp -N 20 -B 45 -i -c sort_mem=100000"
                  fi
              fi
          sleep 2
          echo " "

          chmod a+r "$vnmrsystem"/pgsql/pgsql.log

	      # Take care of difference in Mac ps command options
              # grep for "post" to catch the old postmaster and the new postgres processes
	  if [ x`uname -s` = "xDarwin" ]
	  then
	      post_pid=`ps -ax | grep post | grep -v grep | awk '{ printf $1 }'`
	  else
	      post_pid=`ps -e -o comm,pid | grep post | grep -v grep | awk '{ printf $2 }'`
	  fi

          if [ x$post_pid = "x" ]
          then
              echo "Database postmaster daemon failed to start!"
              exit 4
          fi
          

             $vnmrsystem/bin/managedb createdb

          # The newpostgres will not be in /vnmr.  May want to figure out
          # a way to tell if the DB has been created for newpostgres
          if [ x$newpostgres != 'xtrue' ]
          then
              if test ! -d "$vnmrsystem/pgsql/data"
              then
                  echo "Create vnmr database failed!"
                  exit 5
              fi
          fi
      else
          echo "Valid local database found"

	  # If we were called with a path for a previous vnmr install, then we
	  # want to remove the varian files from the DB that were in that install.
	  # We only do this if we found and are using a previous database.
          if [ x$prevVnmrDir != "x" ]
          then
	      echo "Removing DB files from previous install, $prevVnmrDir"
              "$vnmrsystem"/bin/managedb removeentrydir $prevVnmrDir
	      # we also want to put the files from the new install into the DB
	      echo "Add appdir files in new install to DB, $vnmrsystem"
	      "$vnmrsystem"/bin/managedb updateappdirs
          fi

      fi

   else
       # We must be set to point to a remote DB server.  Check our
       # connection ability and the DB version.
       dbStatus=`managedb checkDBversion`
       if test x$dbStatus = "xbad"
       then
          echo "Connection to DB is good, but the version is incorrect."
      echo "Using Postgres Database on $dbhost."
      echo "Run dbsetup again after resolving this problem."
       elif test x$dbStatus != "xokay"
       then
          echo "Problem connecting to Postgres Database server on $dbhost."
      echo "Run dbsetup again after resolving this problem."
       elif test x$dbStatus = "xokay"
       then
          echo "Valid remote database server found"
       fi
   fi
   # End of creation of empty DB or establishing one already exist

   if test ! -d "$vnmrsystem/pgsql/persistence"
   then
       mkdir "$vnmrsystem"/pgsql/persistence
       chmod a+w "$vnmrsystem"/pgsql/persistence
   fi

   if [ ! -d "$vnmrsystem/adm/users" ]
   then
      mkdir -p "$vnmrsystem"/adm/users
   fi
   
# pis
      if [ x$APP_MODE = "ximaging" ]
      then
         file=$vnmrsystem/imaging/templates/vnmrj/choicefiles
      else
         file="$vnmrsystem"/templates/vnmrj/choicefiles
      fi

      if [ ! -f "$file" ]
      then
         # echo "Creating $file/pis"
         if test ! -d "$file"
         then
             mkdir -p "$file"
         fi
         echo "\"\" \"\"" > "$file"/pis
         echo "\"$vnmr_adm\" \"$vnmr_adm\"" >> "$file"/pis
      fi
   

   # If we already had a database that we kept, then do not update it
   if [ x$dbStatus = "xokay" ]
   then
       echo  "Using previous database, skipping update"
   else
       user_count=`wc -w "$vnmrsystem"/adm/users/userlist | awk '{ print $1 }'`
       echo ""
       echo "Recreating $user_count user(s)..."
       userlist=`cat "$vnmrsystem"/adm/users/userlist`

       for user in $userlist
       do
       echo "     Adding $user"

          dbadm=$vnmr_adm

       if [ x$user != x$dbadm ]
       then
             "$vnmrsystem"/bin/create_pgsql_user $user
       fi
       done


    #   echo "Loading data..."
    #   "$vnmrsystem"/bin/managedb filldb $vnmrsystem/fidlib $vnmr_adm vnmr_data
    #   $vnmrsystem/bin/managedb filldb $vnmrsystem/parlib $vnmr_adm vnmr_par
    #   $vnmrsystem/bin/managedb filldb $vnmrsystem/shims $vnmr_adm shims
    #   $vnmrsystem/bin/managedb filldb ${vnmruser_dir}/data $vnmr_adm vnmr_data
    #   $vnmrsystem/bin/managedb filldb ${vnmruser_dir}/parlib $vnmr_adm vnmr_par
    #   $vnmrsystem/bin/managedb filldb ${vnmruser_dir}/shims $vnmr_adm shims
    #

    if [[ -f "$vnmrsystem"/pgsql/persistence/LocatorOff ]]
    then
       echo ""
       echo "Locator is turned off"
       echo "To re-activate it, turn it on from vnmrj admin"
       echo "and then re-run dbsetup"
       echo ""
    else
       echo "Loading data for all users (this may take a moment)..."
       "$vnmrsystem"/bin/managedb update
       echo " "

       echo "Loading protocols..."
       "$vnmrsystem"/bin/managedb filldb "$vnmrsystem"/templates/vnmrj/protocols $vnmr_adm protocol
       "$vnmrsystem"/bin/managedb filldb "$vnmrsystem"/imaging/templates/vnmrj/protocols $vnmr_adm protocol
    fi


#       echo "Loading workspaces..."
#       "$vnmrsystem"/bin/managedb filldb_nonrecursive "${vnmruser_dir}" $vnmr_adm workspace

       # If DB server is remote, go ahead and have it vacuum and analyze now
       # If not remote, it will have been vacuumed by the 'update' above.
       if test $dbhost != $localhost
       then
      echo Vacuuming and analyzing DB
      vacuumdb
       fi
   fi
}


#-----------------------------------------
#  MAIN  Main  main
#-----------------------------------------
sudocmd=
revm_save=
prevVnmrDir=
login_user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
if [ "x$vnmrsystem" = "x" ]
then
   vnmrsystem="/vnmr"
fi
if [ ! -d "$vnmrsystem"/pgsql ]
then
   exit
fi
vnmr_adm=`"$vnmrsystem"/bin/fileowner /vnmr/vnmrrev`
nmrnetdb="/vnmr/pgsql/config/NMR_NETWORK_DB"
osname=`uname -s`
rootuser="root"

if [ x`uname -s` = "xLinux" ]
then
    #determine if we should use su (redhat) or sudo (debian)
    if [ -r /etc/debian_version ]
    then
        lflvr="debian"
        sudocmd="sudo -u $vnmr_adm "
    else
        lflvr="rhat"
        sudocmd=""
    fi
fi
if [ x`uname -s` = "xDarwin" ]
then
    lflvr="darwin"
    sudocmd="sudo -u $vnmr_adm "
fi

# echo OS: $lflvr

if [ x$lflvr = "xdebian" ]
then
    # Ubuntu needs a path.  Unfortunately, the path will be different
    # if a newer version is installed.  Postgres is normally installed
    # in /usr/lib/postgresql/ in a directory name which is the version number.
    # Look in /usr/lib/postgresql/ and find the dir with the largest number.
    # echo finding pgpath for Ubuntu
    # See if Postgres is installed
    if [ -d "/usr/lib/postgresql" ]
    then

       dirlist=`ls -1 /usr/lib/postgresql`

#    echo "dirlist: $dirlist"
       version="0.0"
       for dir in $dirlist
       do
           if [ $( echo "$version < $dir" | bc ) = 1 ]; then
               version=$dir
           fi
       done

       pgpath="/usr/lib/postgresql/"$version/bin/
    elif [ -d "$vnmrsystem/pgsql/bin" ]
    then
       pgpath="$vnmrsystem/pgsql/bin/"
    else
        echo "Postgres Not Installed.  Aborting dbsetup"
        exit
    fi
fi
if [ x$lflvr = "xdarwin" ]
then
    # echo finding pgpath for Mac
    # Mac sometimes needs a fullpath much like Ubuntu above
    # First, see if pg_ctl is found in the PATH
    pgpath=""
    whichout=`which pg_ctl`
    if [ $? -eq 0 ]
    then
        # "which" found a path, use the directory portion with "/" added
         pgpath=`dirname $whichout`/
    else
        if [ -d "/Library/PostgreSQL" ]
        then
            
            dirlist=`ls -1 /Library/PostgreSQL`
#            echo $dirlist
            version="0.0"
            for dir in $dirlist
            do
                if [ $( echo "$version < $dir" | bc ) = 1 ]; then
                    version=$dir
                fi
            done
            pgpath="/Library/PostgreSQL/"$version/bin/
        fi
        if [ -f "/usr/local/bin/pg_ctl" ]
        then
            pgpath="/usr/local/bin/"
        fi
        if [ x$pgpath = "x" ]
        then
            echo "Postgres Not Installed.  Aborting dbsetup"
            exit
        fi
    fi
fi
if [ x$lflvr = "xrhat" ]
then
    # Redhat does not seem to need a path to find the cmds.
    pgpath=""

    # On RH 5.3 and perhaps others, they may not have postgresql installed
    # on the system.  On 6.1 and 6.3 we do this during the install of the OS
    # If this system does not have the system pg_ctl, then move the vnmrj
    # release of the older postgresql into place and continue on with 
    # that version.
    if test ! -f "/usr/bin/pg_ctl"
    then
        # The system pg_ctl must not exist
        if test -d "$vnmrsystem/pgsql/bin_ver9"
        then
            # bin_ver9 is still there, rename it to bin
            mv "$vnmrsystem/pgsql/bin_ver9" "$vnmrsystem/pgsql/bin"
            echo "Postgres not installed, switching to OpenVnmrJ's version 9.x"
            echo "    mv $vnmrsystem/pgsql/bin_ver9 $vnmrsystem/pgsql/bin"
        fi
    fi
fi

# echo "Using postgres path: $pgpath"

# Setup the Mac for using Postgres
if [ x$lflvr = "xdarwin" ]
then
    # Remove launch file from postgres installation
    # TEB: should we really blow away another postgres?
    oldfiles=`ls -1 /Library/LaunchDaemons/*postgresql-*`
    for file in $oldfiles; do 
        sudo rm -f $file 
    done 
    # Use our launch file
    sudo cp /vnmr/bin/com.edb.launchd.postgresql.plist /Library/LaunchDaemons 

    if [ ! -d /vnmr/pgsql ]; then 
        mkdir /vnmr/pgsql
        chown $vnmr_adm /vnmr/pgsql 
    fi
    if [ x$pgpath != x"/vnmr/pgsql/bin/" ]; then
        cd /vnmr/pgsql
        if [ -d bin ]; then 
            if [ ! -d bin.sav ]; then
                mv bin bin.sav 
            else
                rm -rf bin
            fi
        fi
        ln -s $pgpath bin
    fi
    echo "login_user:.${login_user}. vnmr_adm:.${vnmr_adm}.\n"
fi

case $# in

       0) if [ x$login_user = "xroot" ]
          then
             echo ""
             echo "User root can't run $0 without appropriate arguments."
             echo "Exiting $0 ......."
             echo ""
             exit
          fi
          if test $login_user != $vnmr_adm
          then
             echo "Switching to administrator $vnmr_adm and running $0"
             if [ x$lflvr != "xdebian" ]
             then
                su - $vnmr_adm -c "$0"
             else
                sudo -u $vnmr_adm $0
             fi
             exit
          fi
          # Default dbAction to rebuildDB if no args
          dbAction="rebuildDB"
          ;;

       1) if [ x$login_user = "xroot" ]
          then
             echo ""
             echo "User root can't run $0 without appropriate arguments." 
             echo "Exiting $0 ......." 
             echo ""
             exit 
          fi
          if test $login_user != $vnmr_adm
          then
             echo "Switching to administrator $vnmr_adm and running $0 $1"
             if [ x$lflvr != "xdebian" ]
             then
                su - $vnmr_adm -c "$0 $1"
             else
                sudo -u $vnmr_adm $0 $1
             fi
             exit
          fi
          if [ x$1 = "xremove" -o x$1 = "xsaveusers" -o x$1 = "xstandard" -o x$1 = "ximaging" -o x$1 = "xwalkup" -o x$1 = "xpreserveDB" ]
          then
             revm_save=$1
          elif [ x$1 = "xlocaldb" ]
          then
              rm -f $nmrnetdb
          elif [ x$1 = "xnetdb" ]
          then
              if [ ! -f $nmrnetdb ]
              then
                  touch -f $nmrnetdb
              fi
          else
             echo "dbsetup options are: remove saveusers standard, imaging, walkup, preserveDB, localdb or netdb"
             exit
          fi
          # If preserveDB, set dbAction to preserveDB
          if [ x$revm_save = "xpreserveDB" ]
          then
              dbAction="preserveDB"
          else
              dbAction="rebuildDB"
          fi
          ;;

       2) if test $1 != $vnmr_adm
          then
             echo "Switching to administrator $vnmr_adm and running $0 $vnmr_adm $2"
             if [ x$lflvr != "xdebian" ]
             then
                su - $vnmr_adm -c "$0 $vnmr_adm $2"
             else
                sudo -u $vnmr_adm $0 $vnmr_adm $2
             fi
             exit
          fi
          # Default to rebuildDB if not 4 args
          dbAction="rebuildDB"

          vnmr_adm=$1
          if [ x$2 = "xremove" -o x$2 = "xsaveusers" -o x$2 = "xstandard" -o x$2 = "ximaging" -o x$2 = "xwalkup" ]
          then
             revm_save=$2
          else
             vnmrsystem=$2   #This value passed by java program
             export vnmrsystem
          fi
          ;;

       3) if test $1 != $vnmr_adm
          then
             echo "Switching to administrator $vnmr_adm and running $0 $vnmr_adm $2 $3"

             if [ x$lflvr != "xdebian" ]
             then
                su - $vnmr_adm -c "$0 $vnmr_adm $2 $3"
             else
                sudo -u $vnmr_adm $0 $vnmr_adm $2 $3
             fi
             exit
          fi
          vnmr_adm=$1
          vnmrsystem=$2   #This value passed by java program
          export vnmrsystem
          if [ x$3 = "xstandard" -o x$3 = "ximaging" -o x$3 = "xwalkup" ]
          then
             revm_save=$3
          fi

          # Default to rebuildDB if not 4 args
          dbAction="rebuildDB"
          ;;

       4) if [ "$1" != "$vnmr_adm" ]
          then
             echo "Switching to administrator $vnmr_adm and running $0 $vnmr_adm $1 $2  $3"

             if [ x$lflvr != "xdebian" ]
             then
                su - $vnmr_adm -c "$0 $vnmr_adm $2 $3 $4"
             else
                sudo -u $vnmr_adm -c $0 $vnmr_adm $2 $3 $4
             fi
             exit
          fi
          vnmr_adm=$1
          vnmrsystem=$2   #This value passed by java program
          export vnmrsystem
          if [ x$3 = "xstandard" -o x$3 = "ximaging" -o x$3 = "xwalkup" -o x$3 = "xpreserveDB" ]
          then
             revm_save=$3
          fi

	  # The forth arg is the path to the prev install so that we can
	  # remove /vnmr items from that install from the DB
	  prevVnmrDir=$4
          ;;

       *) show_usage
          exit 1
          ;;
esac

if [ "x$login_user" = "x$rootuser" ]
then
    # Some files may have been created in /tmp by another user.
    # We need to get rid of them if they exist.
    /bin/rm -f /tmp/sed_cmd /tmp/sed_out

    # Kill existing postmaster before starting new database if not preserveDB
    if [ x$revm_save != "xpreserveDB" ]
    then
	echo "Killing any current postmaster"
	kill_procs "postmaster"
 	kill_procs "postgres"
   fi

#for setting up $vnmrsystem/adm/users/...files only
#removed, when VnmrAdm mature enough.
    if [ x$osname = "xDarwin" ]
    then
       vnmradm_home_dir=$HOME
    else
       vnmradm_home_dir=`/usr/bin/getent passwd ${vnmr_adm} | awk 'BEGIN { FS = ":" } { print $6 }'`
    fi

        vnmruser=${vnmradm_home_dir}/vnmrsys
    export vnmruser
 
    if [ x$lflvr != "xdebian" ]
    then
       su - $vnmr_adm -c "$0 $vnmr_adm $vnmrsystem $revm_save $prevVnmrDir"
    else
       sudo -u $vnmr_adm $0 $vnmr_adm "$vnmrsystem" $revm_save $prevVnmrDir
    fi
    exit
else
    if [ x$newpostgres != 'xtrue' ]
    then
        #check for writing permission
        my_file="$vnmrsystem"/pgsql/testfile_please_remove

        touch $my_file 1>/dev/null 2>/dev/null
        if [ $? -ne 0 ]
        then 
            rm -f $my_file
            echo ""
            echo "User \"$login_user\" doesn't have permission to write to $vnmrsystem/pgsql directory."
            echo ""
            echo "Exiting $0 ............."
            echo ""
            exit 1
        else 
            rm -f $my_file
        fi

        USER=$vnmr_adm
        PATH=$PATH:"$vnmrsystem"/pgsql/bin
        PGLIB="$vnmrsystem"/pgsql/lib
        PGDATA="$vnmrsystem"/pgsql/data
        PGDATABASE=vnmr
        export USER PATH PGLIB PGDATA PGDATABASE
    fi
#for setting up $vnmrsystem/adm/users/...files only
#removed, when VnmrAdm mature enough.
    osname=`uname -s`
    if [ x$osname = "xDarwin" ]
    then
        vnmradm_home_dir=$HOME
    else
        vnmradm_home_dir=`/usr/bin/getent passwd ${vnmr_adm} | awk 'BEGIN { FS = ":" } { print $6 }'`
    fi

        vnmruser_dir=${vnmradm_home_dir}/vnmrsys
   
    if [ "x$vnmruser" = "x" ]
    then
        vnmruser="${vnmruser_dir}"
        export vnmruser
    fi
    export vnmradm_home_dir
    export vnmruser_dir

    # If revm_save was not set, then substitute the cur val of dbAction
    # and pass it on
    if [ "x$revm_save" = "x" ]
    then
        revm_save=$dbAction
    fi
    godoit $vnmr_adm $revm_save
fi

# Print the version of postgres we are using
# If pg_ctl exists in $vnmrsystem/pgsql/bin/, use it, 
# else use system version.
# file="$vnmrsystem/pgsql/bin/pg_ctl"
# if [ -f "$file" ]
# then
#     $vnmrsystem/pgsql/bin/pg_ctl --version
# else
#     $pgpath"pg_ctl" --version
# fi

echo ""
echo "End of Database Setup."

