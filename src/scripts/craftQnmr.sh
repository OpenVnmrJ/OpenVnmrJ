#!/bin/bash 

if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi
cmd='craftQnmr'
if [ $# -ge 1 ]; then
  args=''
  while [[ $# > 0 ]] ; do
    args=${args}"'"$1"'"
    shift
    if [[ $# > 0 ]] ; then
       args=${args}","
    fi
  done
  cmd=${cmd}"("${args}")"
fi
# echo $cmd

Vnmrbg -mback -n0 "$cmd"
