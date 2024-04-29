#!/bin/csh
# '@(#)managedb.sh 22.1 03/24/08 1991-2005 '
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

set id = `id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`


if ( $id == "root" ) then
   echo "Cannot execute $0 as root"
   exit
endif

# USER is not correct sometimes, so fix it
set USER = $id

# HOME, vnmruser and other env variables are wrong when executed
# from exec_asuser in Windows, so we need to fix them here.

if ( ! "$?vnmrsystem" ) then
    set vnmrsystem = /vnmr
endif

set tmpdir = "$vnmrsystem/tmp"
if ( $id == $USER ) then
  if ( "$?vnmruser" ) then
    set tmpdir = "$vnmruser"
  endif
endif


set debugargs=""

# The debug flag and its arg must be the last two things on the line.
# Check next to last arg to see if it is 'debug'
@ debugflag = $#argv
if ( $debugflag > 0 ) then
    @ debugflag = $debugflag - 1
    if($argv[$debugflag] == debug) then
            set debugargs="$argv[$#argv]"
    endif
endif

# Set the name of the host where the Database is located.
# Default to "localhost".
if ( $?PGHOST ) then
    set dbhost=$PGHOST
else
    set dbhost="localhost"
endif


# Get or default the port number for the database
# If the env variable PGPOST is set, use that, else default to 5432
if ( $?PGPORT ) then
    set dbport=$PGPORT
else
    set dbport="5432"
endif

# If the env variable PGNETWORK_SERVER is set, use its value, default to 'no'
if ( $?PGNETWORK_SERVER ) then
    set dbnet_server=$PGNETWORK_SERVER
else
    set dbnet_server="no"
endif

set shtoolcmd="/bin/sh"
set shtooloption="-c"
set sfudir=""
set sfudir_interix=""
set javacmd="$vnmrsystem/jre/bin/java"
if ( ! -f $javacmd ) then
   set javacmd="java"
endif
set vjclasspath="$vnmrsystem/java/managedb.jar"
set sysdir="$vnmrsystem"

$javacmd -mx256m -classpath $vjclasspath -Dsysdir="$sysdir" -Duserdir="$tmpdir" -Ddbhost=$dbhost -Ddbport=$dbport -Ddbnet_server=$dbnet_server  -Dsfudirwindows="$sfudir" -Dsfudirinterix="$sfudir_interix" -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" -Ddebug="$debugargs" -Duser.name=$id vnmr.ui.shuf.FillDBManager $argv[*]

if ( "$tmpdir" == "$vnmrsystem/tmp" ) then
   chmod 666 "$tmpdir"/ManagedbMsgLog
endif
