#!/bin/bash
# 
# Script to call Bayes programs

export PATH=/vnmr/craft/Bayes3/bin:${PATH}
export LD_LIBRARY_PATH=/vnmr/craft/Bayes3/lib:/vnmr/lib

if [[ $# = 1 ]] ; then
   $1
else
   cmd=$1; shift
   $cmd $*
fi
