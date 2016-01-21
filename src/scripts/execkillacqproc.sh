#! /bin/sh
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
#  execkillacqproc - if no acquisition processes are running, go ahead and
#		      start them in background. If they are running, then 
#		      kill them with kill -2. If this fails, then use
#		      kill -9. If this fails, list the pid(s) that cannot
#		      killed.
#

# kill a proc
# $1 is the process ID
# $2 is the 'name' of the process
# 2nd argument is optional
# it is used to test for Expproc as the name of the proc
# for Expproc, we wait 7 seconds before proceeding

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
	    sleep 7
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


start_procs() {
  npids=`pgrep Expproc`
  if (test x"$npids" != "x")
  then
    echo "Acquisition communications already started."
    return
  fi
  echo "Starting Acquisition communications."
  (cd $vnmrsystem/acqqueue; rm -f exp*)
  (cd $vnmrsystem/acqbin;				\
   TCL_LIBRARY=/vnmr/tcl/tcllibrary;			\
   TERM=sun ;                   			\
   TERMCAP=/etc/termcap ;       			\
   LOGNAME=root ;               			\
   NESSIE_CONSOLE=inova;				\
   NIRVANA_CONSOLE=wormhole;				\
   PATH="/vnmr/bin2:$PATH:$vnmrsystem/bin:" ;    			\
   export TERM TERMCAP LOGNAME TCL_LIBRARY NESSIE_CONSOLE NIRVANA_CONSOLE PATH vnmrsystem; \
   ./Expproc& )
}

stop_procs() {
  echo "Stopping Acquisition communications."
  npids=`pgrep Expproc`
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
    npids=`pgrep $procname`
    if (test x"$npids" != "x")
    then
      for procpid in $npids
      do
        killaproc $procpid $procname
      done
    fi
  done
}

start_stop() {
  npids=`pgrep Expproc`
  if [ x"$npids" = "x" ]
  then
    start_procs
  else
    stop_procs
  fi
}

# MAIN main Main
start_nmrwebd() {
  nmrwebd_root=/vnmr/web
  nmrwebd_home=${nmrwebd_root}/scripts
  nmrwebd_exe=nmrwebd
  nmrwebd_path=${nmrwebd_home}/${nmrwebd_exe}
  nmrwebd_pidfile=${nmrwebd_root}/run/${nmrwebd_exe}.pid
  if [ -x $nmrwebd_path ]; then
     (cd $nmrwebd_home && ./$nmrwebd_exe 2> /dev/null &)
  fi
}

vnmrsystem=/vnmr
if [ $# = 1 ]
then
  if [ $1 = "start" ]
  then
     start_procs
     start_nmrwebd
  else
     stop_procs
  fi
else
   start_stop
fi
