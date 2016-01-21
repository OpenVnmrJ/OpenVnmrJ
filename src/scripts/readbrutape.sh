: '@(#)readbrutape.sh 22.1 03/24/08 1991-1996 '
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
: readbrutape filename [num]
: use with tape contining bruker data, usually 1600 bpi.
: one file on tape is written out to file named in first argument
: num is the number of files skipped. It defaults to 1,skipping the header file.

if [ $# != 1 -a  $# != 2 ]
then
     	echo Usage: readbrutape filename [num]
	echo '       filename is the destination file '
	echo '       num is number of files skipped counting header,'
	echo '       default = 1'	
	echo '       if num = 0, the tape is read without rewinding '
	exit 1
fi
echo ""
TAPE_LOC=""
prefix=""
tdrive="rmt0"
ndrive="nrmt0"
tapecmd="mt -f"
REMOTE=0
while [ "x$TAPE_LOC" =  "x" ]
do
    echo "Enter Drive Location (local or remote) [local]: "
    read TAPE_LOC
    case "$TAPE_LOC" in
        r*)
            echo ""
            echo "Enter hostname of remote tape drive: "
            read REMOTE_HOST
            echo ""
            echo "Login name for $REMOTE_HOST [vnmr1]: "
            read REMOTE_LOG
            if [ x$REMOTE_LOG = "x" ]
            then
                REMOTE_LOG=vnmr1
            fi
            rsh $REMOTE_HOST -l $REMOTE_LOG -n "echo 0 > /dev/null"
            if [ "$?" -ne 0 ]
            then
               echo "readbrutape : Problem with reaching remote host $REMOTE_HOST"
               exit 1
            fi
	    REMOTE=1
            rtype=`rsh $REMOTE_HOST -l $REMOTE_LOG uname -s`
	    prefix="rsh $REMOTE_HOST -n -l $REMOTE_LOG"
            if [ x$rtype = "xIRIX64" ]
            then
               rtype="IRIX"
            fi
            if [ x$rtype = "xAIX" ]
            then
               tapecmd="tctl -f"
               tdrive="rmt0"
               ndrive="rmt0.1"
            else if [ x$rtype = "xIRIX" ]
                 then
                     tapecmd="mt -t"
                     tdrive="tapens"
                     ndrive="nrtapens"
                 else
                     tapecmd="mt -f"
                     tdrive="rmt0"
                     ndrive="nrmt0"
                 fi
            fi;;
        l*) ;;
        *)
           TAPE_LOC="local" ;;
    esac
done

echo " "
echo "Which tape drive for loading? [$tdrive]: "
read a
if ( test ! x$a = "x")
then
        tdrive=$a
fi
echo "Which non-rewind tape drive for loading? [$ndrive]: "
read a
if ( test ! x$a = "x")
then
        ndrive=$a
fi
tapedrive="/dev/"$tdrive
ntapedrive="/dev/"$ndrive

case $# in
      1)	$prefix $tapecmd $tapedrive rewind
		if [ "$?" -ne 0 ]
		then
		  echo "readbrutape : Problem with $tapedrive"
		  exit 1
		fi
		echo skipping header file
		$prefix  $tapecmd $ntapedrive fsf 1
		echo reading file from tape to $1.bru
		if [ $REMOTE -eq 0 ]
		then
		   dd if=$ntapedrive of=$1.bru ibs=1536 obs=512 skip=0 files=1
		else
		   $prefix dd if=$ntapedrive ibs=1536 obs=512 skip=0 files=1 | dd of=$1.bru
		fi ;;
      2)	case x$2 in
			x0) 	echo not rewinding tape ;;
			x*) 	$prefix $tapecmd $tapedrive rewind
				if [ "$?" -ne 0 ]
				then
		  		   echo "readbrutape : Problem with $tapedrive"
		  		   exit 1
				fi
				echo skipping $2 files including header file
				$prefix  $tapecmd $ntapedrive fsf $2 ;;
		esac
		echo reading file from tape to $1.bru
		if [ $REMOTE -eq 0 ]
		then
		   dd if=$ntapedrive of=$1.bru ibs=1536 obs=512 skip=0 files=1
		else
		   $prefix dd if=$ntapedrive ibs=1536 obs=512 skip=0 files=1 | dd of=$1.bru
		fi ;;
esac
