: '@(#)write_a_tape.sh 22.1 03/24/08 1991-1996 '
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
: /bin/sh

#  At various times it is necessary to modify a tape without disturbing certain
#  archives on the tape.  This script writes the modified tape.  The archives
#  are expected to be in subdirectories file1, file2, file3 etc., up to the
#  number you want on the tape.  You specify that with the argument to this
#  script.

#  NOT THOROUGHLY TESTED
#  It is only know to work on SunOS systems locally, although it is designed
#  for arbitrary systems and to work remotely

#  NOT TO BE GIVEN TO CUSTOMERS
#  NOT A PART OF ANY RELEASED PRODUCT


define_tape() {
    tapedrive="/dev/rst8"
    ntapedrive="/dev/nrst8"
    echo " "
    echo -n "Which tape drive for writing? [rst8]: "
    read a
    if ( test ! x$a = "x")
    then
        tapedrive="/dev/$a"
        ntapedrive="/dev/n$a"
    fi
}

define_host() {
    TAPE_LOC=""
    REMOTE_HOST="NA"
    REMOTE=0
    while [ "$TAPE_LOC" =  "" ]
    do
        echo
        echo -n "Enter Drive Location (local or remote) [local]: "
        read TAPE_LOC
        case "$TAPE_LOC" in
            r*) 
                echo -n "Enter hostname of remote tape drive: "
                read REMOTE_HOST
                echo ""
                echo -n "Login name for $REMOTE_HOST [vnmr1]: "
                read REMOTE_LOG
	        if [ x$REMOTE_LOG = "x" ]
 	        then
		    REMOTE_LOG=vnmr1
	        fi
                REMOTE=1
                PARAMS="$PARAMS -r$REMOTE_HOST"
                rsh $REMOTE_HOST -l $REMOTE_LOG "echo 0 > /dev/null"
                if [ "$?" -ne 0 ]
                then
                   echo "$cmdname : Problem with reaching remote host $REMOTE_HOST"
                   exit 1
                fi;;
            l*) ;;
            *)
               TAPE_LOC="local" ;;
        esac
    done
}

tape_rew() {
    if [ $REMOTE -eq 0 ]
    then
        mt -f $ntapedrive rewind
    else
        rsh $REMOTE_HOST -l $REMOTE_LOG mt -f $ntapedrive rewind
    fi
}

tape_putf() {
    if [ $REMOTE -eq 0 ]
    then
        tar cvf $ntapedrive *
    else
        rsh $REMOTE_HOST -l $REMOTE_LOG tar cBvfb - 20 * | dd of=$ntapedrive bs=20b
    fi
}

tape_bputf() {
    if [ $REMOTE -eq 0 ]
    then
        tar cvfb $ntapedrive 2000 *
    else
        rsh $REMOTE_HOST -l $REMOTE_LOG tar cBvfb - 2000 * | dd of=$ntapedrive bs=2000b
    fi
}

# main script starts here

if [ $# -lt 1 ]
then
    echo $0 requires count of archives to write to the tape
    exit 1
fi

iter=$1
count=1

echo    "This program will make a tape from subdirectories of `pwd`"
echo -n "OK to continue: "
read a
if ( test x$a = "xn" )
then
    exit 1;
fi

define_tape
define_host

tape_rew

while [ $iter -gt 0 ]
do
    subdir="file$count"
    if test ! -d $subdir
    then
        echo $subdir does not exist
        exit 1
    fi

    cd $subdir
    echo "File $count"
    if test $count -eq 1
    then
        tape_putf
    else
        tape_bputf
    fi
    cd ..

    iter=`expr $iter - 1`
    count=`expr $count + 1`
done

tape_rew
