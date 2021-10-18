#!/bin/bash
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
#
#  ovjProcs - if no acquisition processes are running, go ahead and
#		      start them in background. If they are running, then 
#		      kill them with kill -2. If this fails, then use
#		      kill -9. If this fails, list the pid(s) that cannot
#		      killed. For Expproc, we wait 5 seconds before proceeding
#

# set -x

killaproc()
{
	kill -s TERM $1 2> /dev/null
        if [ $? -eq 1 ]
        then
           return 1
        fi
        if [ x$2 = "xExpproc" ]
	then
	    sleep 5
	fi

  # test to be sure the process died, if still running try kill -9
  # redirect error output to /dev/null, in case the process is gone

	if kill -0 $1 2>/dev/null
        then
            if [ x$2 = "xExpproc" ]
	    then
	        echo "'$2' still running, will try kill -9"
	    fi
	    kill -9 $1
	    sleep 1
	    if kill -0 $1 2>/dev/null
	    then
                if [ x$2 = "xExpproc" ]
	        then
		    echo "Unable to kill '$2' (pid=$1)"
		else
		    echo "Unable to kill pid=$1"
	        fi
	    fi
	fi
     return 0
}


startProcs() {
  (cd $vnmrsystem/acqqueue; rm -f exp*)
  (cd $vnmrsystem/acqbin;				\
   TCL_LIBRARY=/vnmr/tcl/tcllibrary;			\
   TERM=sun ;                   			\
   TERMCAP=/etc/termcap ;       			\
   LOGNAME=root ;               			\
   NESSIE_CONSOLE=inova;				\
   NIRVANA_CONSOLE=wormhole;				\
   HOME=$vnmrsystem/bin;                                \
   PATH="$PATH:$vnmrsystem/bin:" ;    			\
   export TERM TERMCAP LOGNAME TCL_LIBRARY NESSIE_CONSOLE NIRVANA_CONSOLE PATH HOME vnmrsystem; \
   ./Expproc )
}

stopProcs() {
  npids=$(pgrep Expproc)
  for acqpid in $npids
  do
    killaproc $acqpid Expproc
    if [ $? -eq 1 ] 
    then
        break
    fi
  done

#  verify all the child processes of Expproc are gone
  proclist="Sendproc Recvproc Procproc Infoproc Autoproc Atproc Masproc Roboproc"
  for procname in $proclist
  do
    npids=$(pgrep $procname)
    if [[ -z $npids ]]; then
      for procpid in $npids
      do
        killaproc $procpid $procname
      done
    fi
  done
}

# MAIN main Main

vnmrsystem=/vnmr
case $# in
    0)
    npids=$(pgrep Expproc)
    if [[ -z $npids ]]; then
        startProcs
    else
        stopProcs
    fi
    ;;

    *)
    case $1 in
        'start')
            startProcs
        ;;

        'stop')
            stopProcs
        ;;

    esac
    ;;
esac

