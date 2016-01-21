#! /bin/sh -x
# @(#)probid.sh 22.1 03/24/08 2009 
#
# change 1st line to "#! /bin/sh -x" for debug
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
#
#
PROG=$0

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
MOUNT=""

DOSIM="false"                         # simulate probe

CACHE_DFLT="/vnmr/probeid/cache"      # default encrypted probe cache directory
CACHE_NOCRYPTO_DFLT="/vnmr/probeid/cache" # unencrypted cache
CACHE=""
CHECK="false"

PARAM=""                     	      # VNMR parameter
TREE=""                      	      # VNMR parameter tree
CRYPTO="-nocrypto"

ERR="/dev/null"
VJERR=""
TMP="/tmp"
PRINTCACHE="false"
PRINTMNT="false"
FIFO_IN="$TMP/vj.pio.to"
FIFO_OUT="$TMP/vj.pio.from"
MNTDIR="/mnt/vnmr"
LOCK="$TMP/vj.pio.lock"
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
AUTOSTART_SERVER=1
SERVER_PID=

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

error()
{
  echo "[`date`] $0 ERROR: " $1 >> $ERR
  if [ $VJERR ]; then
    $SEND2VJ "write('error','$1')"
  fi
}

fatal()
{
  error $1
  exit 1
}

start_server()
{
   echo "starting server" >> $ERR
   eval "$PROBESERVER -start -opt $FIFO_IN -opt $FIFO_OUT $ERRLOG &"
   sleep 1 # TODO: fix this with a more reliable synch mechanism
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
  if [ -e $LOCK ]; then
    pid=`sed -e '$! d; s/[ ]*//' $LOCK`
    match=`ps -p $pid -o pid= | sed -e 's/[ ]*//'`
    if [ "$match" != "$pid" ]; then
      echo "server $match not active" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\nserver $match not active\n"
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
  if [ -e $LOCK ]; then
    pid=`sed -e '$! d; s/[ ]*//' $LOCK`
    match=`ps -p $pid -o pid= | sed -e 's/[ ]*//'`
    if [ "$match" != "$pid" ]; then
      echo "server $match not active" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\nserver $match not active\n"
      fi
      return 0
    fi
    if [ ! -p $FIFO_IN ]; then
      echo "server named pipe $FIFO_IN doesn't exist" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\server named pipe $FIFO_IN doesn't exist\n"
      fi
      return 0
    fi
    if [ ! -p $FIFO_OUT ]; then
      echo "server named pipe $FIFO_OUT doesn't exist" >> $ERR
      if [ "$CHECK" == "true" ]; then
        printf "0\nserver named pipe $FIFO_OUT doesn't exist\n"
      fi
      return 0
    fi
    echo "server is active [$pid]" >> $ERR
    if [ "$CHECK" == "true" ]; then
      printf "$pid\nserver is active [$pid]\n"
    fi
    SERVER_PID=$pid
    return 1         # probe server lock and both named pipes
  fi
  if [ "$CHECK" == "true" ]; then
    printf "0\nserver is not active (lock file $LOCK doesn't exist)\n"
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
  eval "sudo probe_mount $MOUNT" >> $ERR
}

check_for_mount()
{
  if [ "$DOMOUNT" == "true" -a "$DOUNMOUNT" == "true" ]; then
    echo "$0 : error - specify only one of attach or detach" >> $ERR
    exit 1
  fi
  if [ "$DOMOUNT" == "true" ]; then
    if [ "$DOSIM" == "false" ]; then # don't mount a simulated probe
      mount_probe
      mounted=`mount | grep -c -e "$MOUNT"`
      if [ "$mounted" != "1" ]; then
        fatal "$MOUNT wasn't mounted or was isn't a valid mount point"
      fi
    else # nothing to do but check that there isn't really an active probe
      mounted=`mount | grep -c -e "$MOUNT"`
      if [ "$mounted" != "0" ]; then
        fatal "can't simulate probe attachment on mounted filesystem $MOUNT"
      fi
    fi
  fi
  if [ ! check_for_server ]; then
    echo "probe server not running" >> $ERR
    exit             # mount probe independently of other probeid commands
  fi
}

umount_probe()
{
  echo "unmounting probe [$MOUNT]" >> $ERR
  eval "sudo probe_unmount $MOUNT" >> $ERR
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
      	  fatal "$MOUNT wasn't mounted or was isn't a valid mount point"
      	else
      	  unmount_probe
      	fi
      fi
  fi
  if [ ! check_for_server ]; then
    echo "probe server not running" >> $ERR
    exit             # unmount probe independently of other probeid commands
  fi
}

while [ "$FLAGSDONE" = "false" ]; do
    case $1 in
	-error)                        # for test purposes
	     error "$2"
	     exit;;

	-crypto)
	     CRYPTO="-crypto"
	     shift;;

	-nocrypto)
	     CRYPTO="-nocrypto"
	     shift;;

        -probe | -cfg) 
             OPTS="$OPTS -cfg $2"
             shift
             shift;;

        -mnt)
             MOUNT="$2"
             MOUNT_OPT="-mnt $2"
             shift
             shift;;

        -printmnt)
	     PRINTMNT="true"
	     shift;;

	-printcache)
	     PRINTCACHE="true"
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

	-start)
             START="true"
	     OPTS="$OPTS $1"
             if [ $# -gt 1 ]; then
               LOCK="$2"
               shift
             fi
	     BG="&"
             shift;;

        -mount)
             DOMOUNT="true"            # handle this in the script for now
	     OPTS="$OPTS $1"
             shift;;

        -sim)                          # simulated connect/disconnect
             DOSIM="true"
             shift;;

        -detach)
             DOUNMOUNT="true"          # handle this in the script for now
	     OPTS="$OPTS $1"
             shift;;

	-cache)
             CACHE="$2"
             CACHE_OPT="-cache $2"
             shift
             shift;;

        -v)  OPTS="$OPTS $1"
	     ECHO=1                    # turn on verbose shell script output
	     shift;;

        -i)  IN="$2"
             shift
             shift;;

	-o)  OUT="$2"                  # redirect stdout
	     if [ $2 == '$$' ]; then   # redirect stdout to a temporary file
		 OUT="/tmp/$$.vj.pio"
		 echo $OUT             # let the caller know tmp file name
	     fi
	     REDIRECT="> $OUT"
	     shift
	     shift;;

        -log)
             ERR="$2"
	     if [ $2 == '$$' ]; then   # redirect stderr to a temporary file
		 OUT="/tmp/$$.vj.err"  # make unique temporary log file name
		 echo $OUT             # let the caller know tmp file name
	     fi
	     ERRLOG="2>> $ERR"
	     shift
	     shift;;

        *)   if [ $# == 0 ]; then
	        FLAGSDONE="t"
	     else
	        OPTS="$OPTS $1"
	        #echo "processed command-line parameter $1 ($# remaining)"
		shift
	     fi
	     ;;
    esac
done

# just report back on the server status
if [ "$CHECK" == "true" ]; then
  check_for_server
  exit
fi

# command line or server request
check_for_server
SERVER=$?

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

#JAVAOPTS="-mx128m"
#JAVADBG="-agentlib:jdwp=transport=dt_socket,suspend=y,address=localhost:38000"
JADADBG=
JAVAOPTS="$JAVADBG"



# generic java virtual machine system properties
JAVAPROPS="-Dsysdir=$SYSDIR -Duserdir=$USERDIR"

# java jar file
JAR="-classpath $SYSDIR/java/probeid.jar:$SYSDIR/java/apt.jar:$SYSDIR/java/junit.jar:$SYSDIR/java/vnmrutil.jar"

# java main
MAIN="vnmr.probeid.ProbeIdIO"

# ProbeId options
OPTS="$OPTS $CRYPTO $MOUNT_OPT $CACHE_OPT -v"

# ProbeId server executable
PROBEID="$JAVA $JAVAOPTS $JAR $JAVAPROPS $MAIN $OPTS"

if [ $SERVER != 0 -o $REQUIRE_SERVER != 0 ]; then
  echo "using server" >> $ERR
  # either restart the probe server or exit
  if [ $SERVER == 0 -a $REQUIRE_SERVER ]; then
    error "probe server is not running"
    if [ $AUTOSTART_SERVER ]; then
      # start the server with the same context as the command
      PROBESERVER="$JAVA $JAVAOPTS $JAR $JAVAPROPS $MAIN"
      #start_server
      PROBESERVER="/home/vnmrj_3.0_033110/jre/bin/java -classpath /vnmr/java/probeid.jar:/vnmr/java/apt.jar:/vnmr/java/junit.jar:/vnmr/java/vnmrutil.jar -Dsysdir=/vnmr -Duserdir=/home/vnmr1/vnmrsys -Dfile.encoding=UTF-8 -classpath /home/vnmr1/dirk/git-repo/software/probeid/bin:/home/vnmr1/dirk/git-repo/software/vnmrutil/bin:/home/vnmr1/dirk/git-repo/3rdParty/junit/junit.jar vnmr.probeid.ProbeIdIO -start -opt /tmp/vj.pio.to -opt /tmp/vj.pio.from"
      PROBESERVER="/vnmr/jre/bin/java -classpath /vnmr/java/probeid.jar:/vnmr/java/apt.jar:/vnmr/java/junit.jar:/vnmr/java/vnmrutil.jar -Dsysdir=/vnmr -Duserdir=/home/vnmr1/vnmrsys -Dfile.encoding=UTF-8 -classpath /home/vnmr1/dirk/git-repo/software/probeid/bin:/home/vnmr1/dirk/git-repo/software/vnmrutil/bin:/home/vnmr1/dirk/git-repo/3rdParty/junit/junit.jar vnmr.probeid.ProbeIdIO -start -opt /tmp/vj.pio.to -opt /tmp/vj.pio.from 2>> $USERDIR/tmp/vj.pio.err < /dev/null &"
      sleep 1
      eval $PROBESERVER
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

# start a server if one isn't already running
#if [ "$START" == "true" ]; then
#  check_for_server && start_server
#  exit
#fi

# run command, possibly in the background if starting a server
eval "$CMD $ERRLOG $REDIRECT $BG"
