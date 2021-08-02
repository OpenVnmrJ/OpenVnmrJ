#!/bin/bash
#
# change 1st line to "#! /bin/sh -x" or pass in -x flag for debug
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

# Get and manage ProbeId values

# Usage examples:
#  enable probeid
#    probeid
#
#  set probeid mounted on /mnt/probe with cache directory at /vnmr/probe/cache 
#    probeid -id -cache /vnmr/probe/cache -mnt /mnt/probe
#
#  run  probeid built-in self-tests
#    probeid -test 
#
#  import a probe file into probeid (Note: <app_directory> is
#                                    relative to ~/vnmrsys and /vnmr)
#    probeid -import filename -opt <app_directory> -probe <probe_name>
#  
#  start a server, then wait on it to complete startup:
#    probeid -start -wait &          - returns process of id of shell
#    probeid -sync [-t timeout]      - waits for probe server startup completion
#
#    This somewhat tortured sequence is a result of the fact that VnmrJ
#    will wait for all spawned processes of a shell to complete before
#    returning.  Otherwise it could be a one-step process.
#         $start='flock '+$lock+' -c "probeid -start -wait" &'
#    	  shell($start):$server               // returns right away
#    	  shell('probeid -sync'):$ok          // waits for server to boot
#
#    Synchronization occurs through the lock file.
#
PROG=$0
SUDO=/usr/bin/sudo

# Defaults:
IP=""

# Set defaults for SYSDIR and USERDIR
# this logic follows that of $vnmrsystem/bin/protune
if [ "$vnmrsystem" == "" ]; then
    SYSDIR=/vnmr
else
    SYSDIR=$vnmrsystem
fi

# this logic follows that of $vnmrsystem/bin/protune
if [ "$vnmruser" == "" ]; then
  # Probably wrong if run from background:
  USERDIR=$HOME/vnmrsys
else
  # This is likely to be wrong if running in background from the Procs:
  USERDIR=$vnmruser
fi

VERBOSE=""                   	      # no debug or info messages by default
ACTION=""               	      # default to "help" if action unspecified
MOUNT_NOCRYPTO_DFLT="/vnmr/probeid/mnt"
MOUNT_DFLT="/vnmr/probeid/mnt"        # default encrypted probe mount point
MOUNT="/mnt/probe"                    # default probe mount point
MOUNT_ONLY=0                          # exit right after mounting probe
PROBE_MOUNT="$SYSDIR/bin/probe_mount"
PROBE_UNMOUNT="$SYSDIR/bin/probe_unmount"
DOMOUNT=""
DOSIM="false"                         # simulate probe

CACHE_DFLT="/vnmr/probeid/cache"      # default encrypted probe cache directory
CACHE_NOCRYPTO_DFLT="/vnmr/probeid/cache" # unencrypted cache
CACHE=""
CHECK="false"

PARAM=""                     	      # VNMR parameter
TREE=""                      	      # VNMR parameter tree
CRYPTO="-nocrypto"

ERR="/dev/null"
TMP="$USERDIR/tmp"
PRINTCACHE="false"
PRINTMNT="false"
FIFO_IN="$TMP/vj.pio.to"
FIFO_OUT="$TMP/vj.pio.from"
MNTDIR="/mnt/probe"
LOCK="$TMP/vj.pio.lock"
NOFLOCK=0                             # defaults to flock if it is available
DOLOCK=1                              # useful for "-quit" option
PIDFILE="$TMP/vj.pio.pid"
IN="$FIFO_IN"
OUT="$FIFO_OUT"
START="false"
BG=""

# error
TALK="$USERDIR/probes/.talk"
SEND2VJ="$SYSDIR/bin/send2Vnmr $TALK"
VJERR=1

# require probe server to be running
REQUIRE_SERVER=1
WAIT=0

# autostart doesn't work from VnmrJ because shell command waits for all 
# children to complete unless it is started in the background
AUTOSTART_SERVER=0  
SERVER_PID=
NOTIFY_PID=0

# read the command flags:
EXEC=""
FLAGSDONE="false"

# by default send output to stdout
REDIRECT=
ERRLOG=

# don't echo the command-line by default
ECHO=0

# filter out the shell vs. probeid and rewriteoptions
OPTS=""

TIMEOUT=10
TIMEINC=0.2
ASYNC=0
WAIT=0
WAITPID=0
MAXTRIES=50
PRE=""                   # pre-command to execute within lock
POST=""                  # post-command to execute within lock
TIMERPID=0

ARGLIST="$0 $*"

timed_wait()
{
  if [ $TIMERPID != 0 ]; then
    if [[ "$BG" != "" ]]; then
          wait $1        # wait for background process to complete
    fi
    kill -QUIT $TIMERPID 2> /dev/null
  fi
}

# serialize access to the probe server named pipes between different
# clients (i.e. vnmrj and protune)
lock()
{
    if [[ $DOLOCK -eq 0 ]]; then # don't really use a lock at all
      eval $PRE
      eval $*
      eval $POST
      timed_wait $!
      return

    else if [[ "`type -tf flock`" != "" && $NOFLOCK -eq 0 ]]; then
      ( 
        flock -w $TIMEOUT -x 200
        if [ $? -eq 0 ]; then
      	    eval $PRE ; eval "$*" ; eval $POST
            timed_wait $!
        else
    	    echo "$0 timed out" >> $ERR
        fi
      ) 200> $LOCK
      return

    else # the operating system doesn't support flock
      attempt=0
      while [ $attempt -lt $MAXTRIES ]; do
        if ( set -o noclobber; echo "$$" > "$LOCK") 2> /dev/null; 
        then
           trap 'rm -f "$LOCK"; exit $?' INT TERM EXIT

           eval $PRE
           eval $*
	   eval $POST
           timed_wait $!
           
           rm -f "$LOCK"
           trap - INT TERM EXIT
           return
        else
           sleep 0.2
           attempt=$(($attempt + 1))
        fi
      done
      echo "Failed to acquire lockfile: ${LOCK} held by $(cat $LOCK)." >> $ERR
      exit 1
    fi
  fi
}

send2vj()
{
   # check for a listener
   if [ -e $TALK ]; then
     pid=`cut -f 3 -d ' ' $TALK`
     match=`ps -p $pid -o pid= | sed -e 's/[ ]*//'`
     if [[ $pid == $match && "$pid" != "" ]]; then
       $SEND2VJ "$1"
     fi
   fi
}

error()
{
  echo "[`date`] $0 ERROR: " $1 >> $ERR
  if [ $VJERR != 0 ]; then
    send2vj "write('error','$1')"
  fi
}

fatal()
{
  error "$1"
  exit 1
}

# Start a probe server.  Ignores $OPTS so that it can be auto-started.
# Make the FIFOs here rather than waiting for the probe server to create
# them so that this script will wait for the server to complete 
# initialization.
start_server()
{
   if [ ! -e $TMP ]; then
     mkdir -p $TMP
   fi
   if [ ! -p $FIFO_IN ]; then
     mkfifo $FIFO_IN
   fi
   if [ ! -p $FIFO_OUT ]; then
     mkfifo $FIFO_OUT
   fi
   if [[ $WAIT != 0 ]]; then
     NOTIFY="-notify $$"                # have server send SIGUSR2 this shell
   fi
   SERVER_OPT="-start -opt $FIFO_IN -opt $FIFO_OUT $NOTIFY"
   REDIRECT="2>> $TMP/vj.pio.err < /dev/null"
   PROBESERVER="$JAVA $JAVAOPTS $JAR $JAVAPROPS $MAIN $SERVER_OPT $REDIRECT > /dev/null"
   
   trap handle_notify USR2              # handle SIGUSR2 from probe server
   echo "starting server" >> $ERR
   eval "$PROBESERVER &"

   if [[ $WAIT != 0 ]]; then            # wait for SIGUSR2 from probe server
   #flock $LOCK -c "sleep 100" & NOTIFY_PID=$!
     sleep 100 & NOTIFY_PID=$!
     wait $NOTIFY_PID                   # "wait" for SIGUSR2 to interrupt
     exit 1
   #else
   #  eval "$PROBESERVER &"
   fi
}

timed_read()
{
   exec 99<> $FIFO_OUT                  # assign file descriptor 99 to $FIFO_OUT
   rc="0" msg="probe server timed out"
   read -t $TIMEOUT rc msg <&99         # timed read on $FIFO_OUT
   printf "${rc}\n${msg}\n"
}

set_timer() {
  trap handle_timeout USR1
  sleep $TIMEOUT && kill -USR1 $$ 2> /dev/null & TIMERPID=$!
}

handle_timeout() 
{
  printf "0\nprobe server timed out\n"
  exit 1
}

handle_notify()
{
  printf "1\nprobe server startup complete\n"
  if [[ $NOTIFY_PID != 0 ]]; then
      echo $$ killing $NOTIFY_PID
      kill $NOTIFY_PID
  fi
  exit 0
}

kill_server()
{
  server_process
  if [ $? -ne 0 ]; then
    echo "killing server with process ID " $1 >> $ERR
    kill -9 $SERVER_PID
  fi
}

server_process()
{
  if [ -e $PIDFILE ]; then
    pid=`sed -e '$! d; s/[ ]*//' $PIDFILE`
    if [ "$pid" != "" ]; then
      match=`ps -p $pid -o pid= | sed -e 's/[ ]*//'`
    else
      match="none"
    fi
    if [ "$match" != "$pid" ]; then
      echo "probe server $pid not active" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\nprobe server $pid not active\n"
      fi
      return 0
    fi
  fi
  SERVER_PID=$pid
  return 1
}

check_for_server() 
{
  echo "checking for server" >> $ERR
  if [ -e $PIDFILE ]; then
    server_process
    if [ $? == 0 ]; then
      return 0
    fi
    if [ ! -p $FIFO_IN ]; then
      echo "server named pipe $FIFO_IN doesn't exist" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\probe server named pipe $FIFO_IN doesn't exist\n"
      fi
      return 0
    fi
    if [ ! -p $FIFO_OUT ]; then
      echo "server named pipe $FIFO_OUT doesn't exist" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\nprobe server named pipe $FIFO_OUT doesn't exist\n"
      fi
      return 0
    fi
    echo "server is active [$pid]" >> $ERR
    if [ "$CHECK" == "true" ]; then
      printf "$pid\nprobe server is active [$pid]\n"
    fi
    SERVER_PID=$pid
    return 1         # probe server lock and both named pipes
  fi
  if [ "$CHECK" == "true" ]; then
    printf "0\nprobe server is not active (lock file $PIDFILE doesn't exist)\n"
  fi
  return 0           # no probe server lock implies no probe server
}

mount_probe()
{
  echo "mounting probe [$MOUNT]" >> $ERR
  if [ "$MOUNT" == "" ]; then
    MOUNT=$MNTDIR
  fi
  if [ -e $MOUNT ]; then
    mkdir -p $MOUNT
  fi
  eval "$SUDO $PROBE_MOUNT $MOUNT" >> $ERR
}

check_for_mount()
{
  if [ "$DOMOUNT" == "true" -a "$DOUNMOUNT" == "true" ]; then
    echo "$0 : error - specify only one of attach or detach" >> $ERR
    exit 1
  fi
  if [ "$DOMOUNT" == "true" -o "$UNMOUNT" == "" -a "$DOUNMOUNT" == "" ]; then
    if [ "$DOSIM" == "false" ]; then # don't mount a simulated probe
      mounted=`mount | grep -c -e " $MOUNT "` # surrounding spaces are required
      if [ "$mounted" == "0" ]; then
	mount_probe
      fi
      mounted=`mount | grep -c -e " $MOUNT "` # surrounding spaces are required
      if [ "$mounted" == "0" ]; then
        fatal "$MOUNT wasn\\'t mounted or isn\\'t a valid mount point"
      fi
      if [ "$DOMOUNT" == "true" ]; then
        exit 0
      fi
    else # nothing to do but check that there isn't really an active probe
      if [ "$MOUNT" != "" ]; then
      	mounted=`mount | grep -c -e " $MOUNT "`
      	if [ "$mounted" != "0" ]; then
      	  fatal "cannot simulate probe attachment on mounted filesystem $MOUNT"
      	fi
      fi
    fi
  fi
  if [ ! check_for_server ]; then
    echo "probe server not running or lock file deleted" >> $ERR
    exit
  fi
}

umount_probe()
{
  echo "unmounting probe [$MOUNT]" >> $ERR
  eval "$SUDO $PROBE_UNMOUNT $MOUNT" >> $ERR
  mounted=`mount | grep -c -e "$MOUNT"`
  if [ "$mounted" != "0" ]; then
      fatal "$MOUNT failed to unmount"
  fi
}

check_for_unmount()
{
  if [ "$DOMOUNT" == "true" -a "$DOUNMOUNT" == "true" ]; then
    echo "$0 : error - specify only one of attach or detach" >> $ERR
    exit 1
  fi
  if [ "$DOUNMOUNT" == "true" ]; then
      mounted=`mount | grep -c -e "$MOUNT"`
      if [ "$DOSIM" == "false" ]; then
      	if [ "$mounted" != "1" ]; then
      	  fatal "$MOUNT wasn\\'t mounted or isn\\'t a valid mount point"
      	else
      	  umount_probe
      	fi
      fi
  fi
  if [ ! check_for_server ]; then
    echo "probe server not running ($*)" >> $ERR
    exit             # unmount probe independently of other probeid commands
  fi
}

while [ "$FLAGSDONE" = "false" ]; do
    case $1 in
	-error)                        # for test purposes
	     error "$2"
	     exit;;

        -noauto)
	     AUTOSTART_SERVER=0
	     shift;;

	-crypto)
	     CRYPTO="-crypto"
	     shift;;

        -read)
	     timed_read                # synchronize on output
	     exit
             shift;;

	-sync)                         # synchronize on process ID
	     WAITPID="$2"
	     shift
	     shift;;

	-timer)
	     BG="&"
             WAIT=1
	     set_timer
	     shift;;

        -mnt)                          # specify an alternate mount point
             MOUNT="$2"
             MOUNT_OPT="-mnt $2"
             shift
             shift;;

	-nocrypto)
	     CRYPTO="-nocrypto"
	     shift;;

        -noflock)                      # disable "flock" based locking
             NOFLOCK=1                 # (for shell-based locks on systems
             shift                     #  that actually do have "flock")
	     shift;;

        -lockfile)                     # set up locking and return lock file
             if [ ! -e $TMP ]; then
               mkdir -p $TMP           # make lock file parent dir
             fi
	     echo $LOCK
	     exit 0;;

        -printmnt)
	     PRINTMNT="true"
	     shift;;

	-printcache)
	     PRINTCACHE="true"
	     shift;;

        -probe | -cfg) 
             OPTS="$OPTS -cfg $2"
             shift
             shift;;

	-sleep)                        # to facilitate tests on lock
             PRE="sleep $2"
	     shift
	     shift;;

        -check)
             CHECK="true"
             shift;;

        -nocheck)
             CHECK="false"
             shift;;

	-kill)
	     kill_server
             exit;;

	-start)                        # start the server
	     REQUIRE_SERVER=0
             START="true"
	     OPTS="$OPTS $1"
	     BG="&"
             shift;;

	-async)                        # read return data asynchronously
	     ASYNC=1                   # use when starting from magical
	     shift;;                   # and sync subsquently with -read

	-nolock)
	     DOLOCK=0
	     shift;;

        -sim | -virtual)               # simulated connect/disconnect
             DOSIM="true"
             shift;;

	-mount)
	     MOUNT_ONLY=1
	     REQUIRE_SERVER=0
             DOMOUNT="true"            # handle this in the script for now
	     OPTS="$OPTS $1"
             shift;;

        -attach)
             DOMOUNT="true"            # handle this in the script for now
	     OPTS="$OPTS $1"
             shift;;

        -detach | -unmount)
             DOUNMOUNT="true"          # handle this in the script for now
	     OPTS="$OPTS $1"
             shift;;

	-cache)                        # specify an alternate cache directory
             CACHE="$2"
             CACHE_OPT="-cache $2"
             shift
             shift;;

        -wait)
             WAIT=1
	     shift
	     shift;;

        -i)  IN="$2"
             shift
             shift;;

	-o)  OUT="$2"                  # redirect stdout
	     if [ $2 == '$$' ]; then   # redirect stdout to a temporary file
		 OUT="$TMP/$$.vj.pio"
	     fi
	     echo $OUT                 # let the caller know tmp file name
	     REDIRECT="> $OUT"
	     shift
	     shift;;

	-t)  TIMEOUT="$2"
             shift
             shift;;

        -v)  OPTS="$OPTS $1"
	     ECHO=1                    # turn on verbose shell script output
	     shift;;

        -log)
             ERR="$2"
	     if [ $2 == '$$' ]; then   # redirect stderr to a temporary file
		 OUT="$TMP/$$.vj.err"  # make unique temporary log file name
		 echo $OUT             # let the caller know tmp file name
	     fi
	     ERRLOG="2>> $ERR"
	     shift
	     shift;;

        -x)  set -x                    # turn on command echoing for debug
	     shift;;

         *)  if [ $# == 0 ]; then
	        FLAGSDONE="t"
	     else
	        OPTS="$OPTS $1"
		shift
	     fi
	     ;;
    esac
done

# check status of probe server process
check_for_server
SERVER=$?

# just report back on the server status
if [ "$CHECK" == "true" ]; then
  exit
fi

# in server mode we don't need to specify a probe mount point or cache
if [ $SERVER -eq 0 ]; then
  # construct the mountpoint option
  if [ "$MOUNT" == "" ]; then
      # come up with a reasonable default probe mount point
      if [ "$CRYPTO" == "-nocrypto" ]; then
  	MOUNT=$MOUNT_NOCRYPTO_DFLT
      else
  	MOUNT=$MOUNT_DFLT
      fi
  fi
  MOUNT_OPT="-mnt $MOUNT"

  # construct the cache directory option
  if [ "$CACHE" == "" ]; then
      # come up with a reasonable default cache directory
      if [ "$CRYPTO" == "-nocrypto" ]; then
  	CACHE=$CACHE_NOCRYPTO_DFLT
      else
  	CACHE=$CACHE_DFLT
      fi
  fi
  CACHE_OPT="-cache $CACHE"
fi

# java runtime
JAVA=$SYSDIR/jre/bin/java
if [[ ! -f $JAVA ]]; then
   JAVA="java"
fi

# one common option: JAVAOPTS="-mx128m"
# java debugger option: 
#    JAVADBG="-agentlib:jdwp=transport=dt_socket,suspend=y,address=localhost:8000"
JADADBG=
JAVAOPTS="$JAVADBG"

# generic java virtual machine system properties
JAVAPROPS="-Dsysdir=$SYSDIR -Duserdir=$USERDIR"

# java jar file path
JAR="-classpath $SYSDIR/java/probeid.jar:$SYSDIR/java/apt.jar:$SYSDIR/java/junit.jar:$SYSDIR/java/vjclient.jar"

# java main
MAIN="vnmr.probeid.ProbeIdIO"

# ProbeId options
OPTS="$OPTS $CRYPTO $MOUNT_OPT $CACHE_OPT -v"

# ProbeId server executable
PROBEID="$JAVA $JAVAOPTS $JAR $JAVAPROPS $MAIN $OPTS"

# wait for the server's parent shell to receive SIGUSR2 or time out
if [[ $WAITPID != 0 ]]; then
    lock "printf '1\nprobe server confirmed\n'"
    exit 0
fi

if [ "$START" == "true" ]; then
  if [ $SERVER == 0 ]; then
    start_server
    if [ $ASYNC != 0 ]; then
      timed_read
    fi
  fi
  exit
fi

# Start a server if -start option was supplied and it isn't already running
# Automatically start a server if $AUTOSTART_SERVER is set.  For the sake of 
# consistency, any server start goes through the same code path in this macro.
if [ $SERVER != 0 -o $REQUIRE_SERVER != 0 ]; then
  echo "using server" >> $ERR
  # either restart the probe server or exit
  if [ $SERVER == 0 -a $REQUIRE_SERVER ]; then
    error "probe server is not running ($ARGLIST)"
    if [ $AUTOSTART_SERVER != 0 ]; then
      # start the server with the same context as the command
      start_server
      #timed_read    # wait for server to sig
    else
      exit
    fi
  fi
  CMD="(echo $OPTS > $FIFO_IN ; cat < $FIFO_OUT)"
else
  #echo "using command-line probe interface " > /dev/stderr
  CMD="$PROBEID"
fi

check_for_mount
check_for_unmount

if [ "$START" == "true" ]; then
    exit 0                 # started above
fi

# print the mount file path and exit
if [ "$PRINTMNT" == "true" ]; then
    echo "$MOUNT"
    exit
fi

# print the cache file path and exit
if [ "$PRINTCACHE" == "true" ]; then
    echo "$CACHE"
    exit
fi

# print the command string to stderr (so that the result is still
# written to the right place in stdout for a calling magical macro)
if [ "$ECHO" != "" ]; then
    echo $CMD > $ERR
fi

# "eval" gets the quotes right in the command string
if [ "$ERRLOG" != "" ]; then
    echo -n "["`date`"] " >> $ERR
    echo "$CMD $ERRLOG $REDIRECT" >> $ERR
fi

# Submit a probe server request.
# Runs with an exclusive file lock to coordinate among several
# potential probe server clients, i.e. protune, vnmrj, or a
# terminal window command line.  Waits for a maximum of 10 secs
# to acquire the lock.
# The server is started above, so is not affected by the lock.
#
lock "$CMD $ERRLOG $REDIRECT $BG" 

exit
