#! /bin/sh
: '@(#)vnmrj.sh 22.1 03/24/08 1991-2006 '
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

# -debug info
#    When multiple debug flags are to be set, include them as a comma
#    separated list enclosed in double quotes such as:
#       vnmrj -debug "SqlCmd, showmem, panels"
#    Some of the possible arguments for debug are:
#       managedb, fillATable, sqlCmd, addFiletoDB, panels, SQ, showmem,
#       accessfile, ShufflerItem, userListfile, RFMonData, fullCommandHistory,
#       locatorHistory, debugCategoryList, StackTrace, traceXML, timeXML
#    squish can be from 0.4 to 1.0, default is off
#    laf can be metal, motif or gtk
#    theme can be default, ocean, aqua or emerald
#

if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi

debug_on="n"
itype="exp"
squish=1.0
#laf="metal"
showmem="no"
theme="ocean"

for arg in $*; do
    arg02=`echo $arg | cut -c1-2`
    if [ "x$debug_on" = "xyy" ]; then
        debug_on="y"
        debugargs=$arg
    elif test ${arg02} = "-D"; then
        dargs="$dargs $arg";
    elif test $arg = "-debug"; then
        debug_on="yy"
    else
        itype=$arg
    fi;
done
if [ "x$debug_on" = "xyy" ]; then
    debug_on="y"
    debugargs="traceXML"
fi
# echo dargs=\"$dargs\"
# echo debugargs=\"$debugargs\"
# echo itype=\"$itype\"

if [ $itype = "adm" -o $itype = "admin" ]
then
    admin=`$vnmrsystem/bin/fileowner $vnmrsystem/vnmrrev`
    itype="adm"
    if [ $USER != $admin ]
    then
       echo "Switching to administrator $admin and starting vnmrj adm"
       su - $admin -c "vnmrj adm"
       exit
    fi
    laf="metal"
else
    id=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
    # echo "USER: $USER"
    # echo "id: $id"
    if [ "$USER" != "$id" ]
    then
       echo "Switching to user $id and starting vnmrj"
       su - $id -c "$vnmrsystem/bin/vnmrj"
       exit
    fi
    if [ -d $vnmrsystem/p11 ]
    then
       session=`ls $vnmruser/lock_*.primary >& /dev/null`
       if [ $? -eq 0 ]
       then
          cvnmr=`ps -ef | grep "/java/vnmrj.jar" | grep -v grep | awk '{print $1}'`
          if [ x$cvnmr = x$id ]
          then
             sel=`/usr/bin/zenity --info --text="A VnmrJ session is already active"`
             exit
          fi
       fi
    fi
    if [ -f $vnmruser/persistence/singleSession ]
    then
       session=`ls $vnmruser/lock_*.primary >& /dev/null`
       if [ $? -eq 0 ]
       then
          cvnmr=`pgrep -u $id Vnmrbg`
          for pid in $cvnmr
          do
             grep -w $pid $vnmruser/lock_*.primary >& /dev/null
             if [ $? -eq 0 ]
             then
                sel=`/usr/bin/zenity --info --text="A VnmrJ session is already active"`
                exit
             fi
          done
       fi
    fi
fi


if [ x$LD_LIBRARY_PATH = "x" ]
then
    LD_LIBRARY_PATH=$vnmrsystem/java:$vnmruser/psg:$vnmrsystem/lib
    export LD_LIBRARY_PATH
fi


# Set the name of the host where the Database is located.
# Default to "localhost".
if [ x$PGHOST = "x" ]
then
    dbhost="localhost"
else
    dbhost=$PGHOST
fi

# Get the port number for the database if available, else set to default
# If the env variable PGPOST is set, use that, else default to 5432
if [ x$PGPORT = "x" ]
then
    dbport="5432"
else
    dbport=$PGPORT
fi

# If the env variable PGNETWORK_SERVER is set, use its value, default to 'no'
if [ x$PGNETWORK_SERVER = "x" ]
then
    dbnet_server="no"
else
    dbnet_server=$PGNETWORK_SERVER
fi

vjclasspath=$vnmrsystem/java/vnmrj.jar
if [ -f $vnmrsystem/java/vjmol.jar ]
then
    vjclasspath=$vjclasspath:$vnmrsystem/java/vjmol.jar:$vnmrsystem/java/jmol.jar
fi

if [ x$VGLRUN = "x" ]
then
    vjshell=""
else
    vjshell="$VGLRUN"
fi
# Set java memory usage
if [ x$VJMEM = "x" ]
then
    vjmem=-mx1500m
else
    vjmem=-mx"$VJMEM"m
fi

ostype=`uname -s`
sysdir=$vnmrsystem
userdir=$vnmruser
shtoolcmd="/bin/sh"
shtooloption="-c"
javabin="$vnmrsystem/jre/bin/java"

if [ x$ostype = "xDarwin" ]
then
   javabin="java"
fi

# Set the "MultiClickTime"
if [ -e $HOME/.vxresource ]; then
   xrdb -merge $HOME/.vxresource
fi

# Remember which X DISPLAY to use
if [ -w $vnmruser ]; then
   rm -f $vnmruser/.DISPLAY
   echo $DISPLAY > $vnmruser/.DISPLAY
fi


if [ x$debug_on = "xy" ]
then

     $vjshell "$javabin" "$vjmem" \
        -classpath $vjclasspath $dargs \
        -Ddbhost=$dbhost -Ddbport=$dbport -Dsysdir=$sysdir -Duserdir=$userdir \
        -Duser=$USER -Dfont="Dialog plain 14" -Dlookandfeel=$laf -Dtheme=$theme -DqueueArea=yes \
        -Dsfudirwindows="$SFUDIR" -Dsfudirinterix="$SFUDIR_INTERIX" \
        -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" -Dvjerrfile=custom \
        -Dbatchupdates=no -Dpersona=$itype -Dsavepanels=10 -DSQAllowNesting=false \
        -Djava.library.path="/vnmr/lib" \
        -Djogamp.gluegen.UseTempJarCache="false" \
        -Dsquish=$squish -DshowMem=$showmem -Ddebug="savedatasetup,$debugargs" -Ddbnet_server=$dbnet_server \
        vnmr.ui.VNMRFrame &

else

        $vjshell  "$javabin" "$vjmem" \
          -classpath $vjclasspath $dargs \
          -Ddbhost=$dbhost -Ddbport=$dbport -Dsysdir=$sysdir -Duserdir=$userdir \
          -Duser=$USER -Dfont="Dialog plain 14" -Dlookandfeel=$laf -Dtheme=$theme -DqueueArea=yes \
          -DcanvasFont="Dialog plain 12" \
          -Dsfudirwindows="$SFUDIR" -Dsfudirinterix="$SFUDIR_INTERIX" \
          -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption" -Dvjerrfile=custom \
          -Dbatchupdates=no -Dpersona=$itype -Dsavepanels=10 -DSQAllowNesting=false \
          -Dsquish=$squish -Ddbnet_server=$dbnet_server -Ddebug="savedatasetup" \
          -Djava.library.path="/vnmr/lib" \
          -Djogamp.gluegen.UseTempJarCache="false" \
           vnmr.ui.VNMRFrame >> /dev/null &
fi

