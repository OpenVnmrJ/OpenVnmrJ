#!/bin/sh
# script for execution of deployed applications
#
# Sets up the MCR environment for the current $ARCH and executes 
# the specified command.
#
exe_name=$0
exe_dir=`dirname $0`

#echo $exe_name
  MCRROOT="/opt/MATLAB/matlab"
  # without the following line matlab seems to run as is if the "-nojvm"
  # compile option was used (even though it wasn't)
  # this causes fiqures to sometimes fail to show any data and causes
  # messages such as: "line xxx function not supported with the -nojvm option"
  # etc. to show up in the evocation shell
  MATLAB_JAVA=''

  MWE_ARCH="glnxa64" ;

  LD_LIBRARY_PATH=.:${MCRROOT}/runtime/glnxa64 ;

  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRROOT}/bin/glnxa64 ;
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRROOT}/sys/os/glnxa64;

  MCRJRE=${MCRROOT}/sys/java/jre/glnxa64/jre/lib/amd64 ;
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRJRE}/native_threads ; 
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRJRE}/server ;
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRJRE}/client ;
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${MCRJRE} ;  
 
  XAPPLRESDIR=${MCRROOT}/X11/app-defaults ;
  export XAPPLRESDIR;
  export LD_LIBRARY_PATH;
  function=$1
  shift 1;

${exe_dir}/${function} "$*"

exit
