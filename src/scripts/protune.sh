#! /bin/sh
# @(#)protune.sh 22.1 03/24/08 2005 
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

# Run the ProTune algorithm

# Usage examples:
# (1) protune -probe <probe>
#	Start up with GUI for the given probe
# (2) protune -probe <probe> -tunefreq <freq_MHz>
#	Tune to the given frequency on the given probe

PROG=$0

# Defaults:
PROBE=asw
LOCKPORT=4593
IP=""
AUTOFLAG=""
SYSTUNEDIR=""

# Set defaults for SYSDIR and USERDIR
if [ x"$vnmrsystem" = "x" ]
then
    SYSDIR=/vnmr
else
    SYSDIR=$vnmrsystem
fi

if [ x"$vnmruser" = "x" ]
then
  # Probably wrong if run from background:
  USERDIR=$HOME/vnmrsys
else
  # This is likely to be wrong if running in background from the Procs:
  USERDIR=$vnmruser
fi


# Read the command flags:
FLAGSDONE="false"
EXEC=""
while [ "$FLAGSDONE" = "false" ]; do
    case $1 in
        -probe)  PROBE="\"$2\""
             shift
             shift;;
        -exec)  EXEC="$EXEC -exec \"$2\""
             shift
             shift;;
        -tunefreq)  TUNECMD="-exec \"setTuneFrequency 0 ${2}e6\""
             shift
             shift;;
        -tunefreq2)  TUNECMD2="-exec \"setTuneFrequency 0 ${2}e6\""
             shift
             shift;;
        -tunemode) TUNEMODE="-exec \"motor $2\""
             shift
             shift;;
        -tunemode2) TUNEMODE2="-exec \"motor $2\""
             shift
             shift;;
        -uppercutoff) UCUT="-upperCutoff $2e6"
             shift
             shift;;
        -lowercutoff) LCUT="-lowerCutoff $2e6"
             shift
             shift;;
        -match) MATCH="-exec \"setMatch $2\""
             shift
             shift;;
        -match2) MATCH2="-exec \"setMatch $2\""
             shift
             shift;;
        -sweeptimeout) STO="-sweepTimeout $2"
             shift
             shift;;
        -calsweep) SW="-calSweep $2 $3 $4 $5"
             shift
             shift
             shift
             shift
             shift;;
        -debug) DEB="-debug $2"
             shift
             shift;;
        -ip) IP="-motorIP $2"
             shift
             shift;;
        -ipsuffix) # no-op
             shift
             shift;;
        -probeid) PROBEID="-probeid $2"
             shift
	     shift;;
        -quitgui) # This option is no longer used
	     echo NOTE: protune.sh called with -quitgui argument
	     pfile=$SYSDIR/acqqueue/ptunePort
             if [ -f "$pfile" ]
             then
                ptext=`cat $pfile`
                CMD="$SYSDIR/jre/bin/java -cp $SYSDIR/java/apt.jar "
		CMD=${CMD}"vnmr.apt.ProtuneQuit "
                CMD=${CMD}$ptext
                CMD="$CMD &"
                eval $CMD
		# Give ProtuneQuit a few seconds to finish and then kill it.
		# (It should have already exited.)
		PID=$!
		#echo $PID
		sleep 5
		kill -9 $PID >/dev/null 2>/dev/null
		#echo DONE
             fi
             exit 0;;
	-userdir) USERDIR=$2
             shift
             shift;;
	-sysdir) SYSDIR=$2
             shift
             shift;;
	-systunedir) SYSTUNEDIR=$2
             shift
             shift;;
        -nogui)  GUIFLAG="$GUIFLAG -nogui"
             shift;;
        -simpleGui)  GUIFLAG="$GUIFLAG -simpleGui"
             shift;;
	-dispInfo)  INFOFLAG="-dispInfo"
	     shift;;
        -lockport) LOCKPORT="$2"
             shift
             shift;;
	-auto)  AUTOFLAG="auto"
	     shift;;
        *) FLAGSDONE="t";;
    esac
done

if [ x$SYSTUNEDIR = x ]; then
    SYSTUNEDIR="$SYSDIR/tune/$PROBE"
fi

#echo PROBE=$PROBE
#echo TUNECMD=$TUNECMD

if [ x$AUTOFLAG = "xauto" ]; then
    # DISPLAY on the correct screen
    if [ -r $vnmruser/.DISPLAY ]; then
        export DISPLAY=`cat $vnmruser/.DISPLAY`
    fi
fi

# Note: The "eval" is needed to get the quotes right in the command string
ostype=`uname -s`
if [ x$ostype = "xInterix" ]
then
  eval $SYSDIR/bin/protune.exe $DEB $PROBEID -probe $PROBE $SW $STO $GUIFLAG $INFOFLAG $UCUT $LCUT -sweep mt $IP -lockPort $LOCKPORT $MATCH $TUNEMODE $TUNECMD $MATCH2 $TUNEMODE2 $TUNECMD2 &


else
  #echo -------- vnmruser=$vnmruser
  #echo -------- USERDIR=$USERDIR
  #echo -------- DISPLAY=$DISPLAY
  #JAVADBG="-Xdebug -Xrunjdwp:transport=dt_socket,address=8000,server=n,suspend=n"
  JAVA=$SYSDIR/jre/bin/java
  JAVADBG="-Xdebug"
  JAR="$SYSDIR/java/apt.jar:$SYSDIR/java/probeid.jar:$SYSDIR/java/junit.jar:$SYSDIR/java/vnmrutil.jar"
  MAIN="vnmr.apt.ProbeTune"

  eval $JAVA $DBG -mx128m -classpath $JAR -Dsysdir=$SYSDIR -Duserdir=$USERDIR $MAIN $DEB $PROBEID -probe $PROBE $SW $STO $GUIFLAG $INFOFLAG $UCUT $LCUT -sweep mt $IP -lockPort $LOCKPORT -systunedir $SYSTUNEDIR $MATCH $TUNEMODE $TUNECMD $MATCH2 $TUNEMODE2 $TUNECMD2 $EXEC > /dev/null &

fi
