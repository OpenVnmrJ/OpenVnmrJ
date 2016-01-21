: '@(#)read_a_tape.sh 22.1 03/24/08 1991-1996 '
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

# At various times it is necessary to modify a released tape, altering some
# of the archives while making no changes to other archives.  This script
# assists in this process, reading the initial tape.  Each archive is stored
# in a subdirectory, file1, file2, file3 etc., up to the count of files
# called out when the script is started.

define_tape() {
    tapedrive="/dev/rst8"
    ntapedrive="/dev/nrst8"
    echo " "
    echo -n "Which tape drive for loading? [rst8]: "
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
                rsh -l $REMOTE_LOG -n  $REMOTE_HOST "echo 0 > /dev/null"
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
        rsh -l $REMOTE_LOG -n $REMOTE_HOST mt -f $ntapedrive rewind
    fi
}

tape_nextf() {
    if [ $REMOTE -eq 0 ]
    then
        mt -f $ntapedrive fsf 1
    fi
}

tape_getf() {
    if [ $REMOTE -eq 0 ]
    then
        tar xvf $ntapedrive $an
    else
        rsh -l $REMOTE_LOG -n $REMOTE_HOST dd if=$ntapedrive bs=20b | tar xBvfb - 20 $an
    fi
}

tape_bgetf() {
    if [ $REMOTE -eq 0 ]
    then
        tar xvfb $ntapedrive 2000 $an
    else
        rsh -l $REMOTE_LOG -n $REMOTE_HOST dd if=$ntapedrive bs=2000b | tar xBvfb - 2000 $an
    fi
}

# main script starts here

if [ $# -lt 1 ]
then
    echo $0 requires count of archives on the tape to read
    exit 1
fi

iter=$1
count=1

echo    "This program will put a bunch of files in subdirectories of `pwd`"
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
        mkdir $subdir
        if test $? -ne 0
        then
            echo "can't make $subdir subdirectory"
            exit 1
        fi
    else

# subdirectory to store files from tape expected to be empty
# if you done care about this, remove this else block

        filelist=`cd $subdir; ls`
        if test x$filelist != "x"
        then
            echo "$subdir subdirectory is not empty"
            exit 1
        fi
    fi

    cd $subdir
    echo "File $count"
    if test $count -eq 1
    then
        tape_getf
    else
        tape_bgetf
    fi
    cd ..

    iter=`expr $iter - 1`
    count=`expr $count + 1`

    if [ $iter -gt 0 ]
    then
        tape_nextf
    fi
done

tape_rew
