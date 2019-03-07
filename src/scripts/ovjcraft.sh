#!/bin/sh

# Looks for sendMessage macro in systemdir or userdir

if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi
if [ $# -lt 1 ]; then

    if [ -f $HOME/vnmrsys/manual/ovjcraft ]; then
	cat $HOME/vnmrsys/manual/ovjcraft
    elif [ -f $HOME/vnmrsys/CPpatch/CRAFTpatch/manual/ovjcraft ]; then
	cat $HOME/vnmrsys/CPpatch/CRAFTpatch/manual/ovjcraft
    elif [ -f /vnmr/manual/ovjcraft ]; then
	cat /vnmr/vnmrsys/manual/ovjcraft
    else
	echo "Supports 3 arguments -"
	echo "    submit - displays submit2craft popup"
	echo "    admin  - displays craft queue manager"
	echo "    qest   - displays craftQnmr options"
    fi

else
    if [ ! -f /vnmr/maclib/sendMessage -a ! -f $HOME/vnmrsys/maclib/sendMessage ]; then
		# This not new Vnmrbg.  Look for a foreground vnmr
 	rm -f /vnmr/tmp/talk.$$
 	touch /vnmr/tmp/talk.$$
        ps -ef | grep Vnmr | grep mforeground | \
                grep -v 'grep Vnmr' | cut -c50-256 | awk '
        {
          printf("%s %s %s\n",substr($3,3,length($3)-2),$4,$5)
        }' > /vnmr/tmp/talk.$$

	if [ `cat /vnmr/tmp/talk.$$ | wc -l` -gt 0 ]; then
	    if [ "x$1" = "xsubmit" ]; then
	    	send2Vnmr /vnmr/tmp/talk.$$ "submit2craft('window')"
	    elif [ "x$1" = "xadmin" ]; then
		send2Vnmr /vnmr/tmp/talk.$$ "craftQueueManage('window')"
            elif [ "x$1" = "xqest" ]; then
                send2Vnmr /vnmr/tmp/talk.$$ "craftQnmr('window')"
	    fi
	fi
	rm -f /vnmr/tmp/talk.$$
    else
		# Do this with sendMessage utility
        if [ "x$1" = "xsubmit" ]; then
		Vnmrbg -mback -n- -s sendMessage "submit2craft('window') s2cparbg='bg'"
    	elif [ "x$1" = "xexit" ]; then
		Vnmrbg -mback -n0 -s sendMessage "exitComm"
	elif [ "x$1" = "xadmin" ]; then
		Vnmrbg -mback -n- -s sendMessage "craftQueueManage('window') cQMpar2='bg'"
	elif [ "x$1" = "xqest" ]; then
                Vnmrbg -mback -n- -s sendMessage "craftQnmr('window')"
	fi
    fi
fi
