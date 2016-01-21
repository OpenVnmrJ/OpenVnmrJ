#! /bin/sh
#: '@(#)dbinstall.sh 22.1 03/24/08 1991-1994 '
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
##########################################
#  dbinstall script - install database   #
##########################################

# vnmradmuser="vnmr1"
# vnmradmuser=` ls -l /vnmr/pgsql | grep data | grep -v grep | awk '{ print $3 }' `
vnmradmuser=` ls -l /vnmr/bin/Vnmrbg | awk '{ print $3 }' `
if [ $USER != "$vnmradmuser" ]
then
    echo "You must do 'su - $vnmradmuser' or login as $vnmradmuser to execute this command."
    exit 1
fi

#set path=($path $vnmrsystem/pgsql/bin)
#setenv PGLIB      $vnmrsystem/pgsql/lib
#setenv PGDATA     $vnmrsystem/pgsql/data
#setenv PGDATABASE vnmr

PATH=$PATH:$vnmrsystem/pgsql/bin
PGLIB=$vnmrsystem/pgsql/lib
PGDATA=$vnmrsystem/pgsql/data
PGDATABASE=vnmr
export PGLIB 
export PGDATA 
export PGDATABASE 
export PATH

UPDATEUSERS=y
DESTROYDB=n
if [ $# -eq 1 ]
then
  if [ $1 = "remove" ]
  then
    DESTROYDB=y
  else
    if [ $1 = "saveusers" ]
    then
      UPDATEUSERS=n
    else
      echo "dbinstall: illegal argument '$1', usage: dbinstall < remove | saveusers >"
      exit 1
    fi
  fi
fi

if [ $DESTROYDB = "y" ]
then
  $vnmrsystem/bin/managedb destroydb
  destroyuser $vnmradmuser
  rm -rf /tmp/.s.PGSQL*
  rm -rf $vnmrsystem/pgsql/data
# ps -ef | grep postgres_daemon; kill postgres_daemon's id; don't use kill -9
# there may be multiple postgres_daemons running if vnmrj is running!
  DBKILLPID="ps -ef | grep postgres_daemon | grep -v grep | awk '{ printf(\"%d \",\$2) }'"
  npids=`eval $DBKILLPID`
  for prog_pid in $npids
  do
    kill $prog_pid
  done
  echo "$0: database removed!"
  VNMRJ_NUM=` ps -ef | grep vnmrj.jar | grep -v grep | grep -c vnmrj.jar `
  if [ $VNMRJ_NUM -gt 0 ]
  then
    echo "Warning: vnmrj still running!  Exit before rebuilding database."
    VNMRJ_RUN=` ps -ef | grep vnmrj.jar | grep -v grep | awk '{ print $2 }' `
    echo "Process id's = $VNMRJ_RUN"
  fi
  exit 1
fi

VNMRJ_NUM=` ps -ef | grep vnmrj.jar | grep -v grep | grep -c vnmrj.jar `
if [ $VNMRJ_NUM -gt 0 ]
then
  echo "Warning: vnmrj still running!  Exit before rebuilding database."
  VNMRJ_RUN=` ps -ef | grep vnmrj.jar | grep -v grep | awk '{ print $2 }' `
  echo "Process id's = $VNMRJ_RUN"
  exit 1
fi

echo "Run this script only once to install database from scratch."

rm -f /tmp/.s.PGSQL*

#kill postgres_daemon's if running
DBKILLPID="ps -ef | grep postgres_daemon | grep -v grep | awk '{ printf(\"%d \",\$2) }'"
npids=`eval $DBKILLPID`
for prog_pid in $npids
do
  kill $prog_pid
done

initdb --pglib=$vnmrsystem/pgsql/lib --pgdata=$vnmrsystem/pgsql/data

$vnmrsystem/pgsql/bin/postgres_daemon &

sleep 2

$vnmrsystem/bin/managedb createdb

if [ $UPDATEUSERS = "y" ]
then
  if [ ! -d $vnmrsystem/adm/users ]
  then
    mkdir $vnmrsystem/adm/users
  fi
  if [ -d $vnmrsystem/adm/users ]
  then
    admct=`expr 0`
    admfiles='access group userList'
    for file in $admfiles
    do
      if [ -f $vnmrsystem/adm/users/$file ]
      then
        rm -rf $vnmrsystem/adm/users/$file
      fi
      admct=`expr $admct + 1`
      case $admct in
        1) echo "$vnmradmuser:all" > $vnmrsystem/adm/users/$file;;
        2) echo "vnmr:VNMR group:$vnmradmuser" > $vnmrsystem/adm/users/$file;;
        3) echo "$vnmradmuser::VNMR admin:$vnmruser/data:2" > $vnmrsystem/adm/users/$file;;
      esac
      echo "Creating $vnmrsystem/adm/users/$file"
    done
  fi
fi
echo " "

$vnmrsystem/bin/managedb filldb /vnmr/fidlib $vnmradmuser
$vnmrsystem/bin/managedb filldb /vnmr/user_templates/layout/etc $vnmradmuser
# To add other directories to the database, substitute
# $vnmrsystem/fidlib in the above command.  

#########################################################
#  As vnmr1, you must run
# 
#     createuser user
# 
# for each user with access to the database.
# (createuser vnmr1 is done automatically)
# You also need to run vnmr_jadmin and add each user.
#########################################################

# echo " "
# echo "Run 'createuser user' on each user of database (except $vnmradmuser)."
# You also need:
#   set path=($path $vnmrsystem/pgsql/bin)
#   setenv PGLIB      $vnmrsystem/pgsql/lib
#   setenv PGDATA     $vnmrsystem/pgsql/data
#   setenv PGDATABASE vnmr
# echo " "
# echo "Starting vnmr_jadmin pop-up window.  Wait........"
# echo " "
# vnmr_jadmin

# To restart from scratch, run
#   dbinstall remove
# or the following (where $vnmrsystem=/vnmr):
#   set path=($path $vnmrsystem/pgsql/bin)
#   setenv PGLIB      $vnmrsystem/pgsql/lib
#   setenv PGDATA     $vnmrsystem/pgsql/data
#   setenv PGDATABASE vnmr
#   $vnmrsystem/bin/managedb destroydb
#   destroyuser vnmr1; this is optional
#   rm -rf $vnmrsystem/pgsql/data
#   ps -ef | grep postgres_daemon; kill postgres_daemon's id; don't use kill -9 

