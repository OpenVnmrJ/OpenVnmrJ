#!/bin/bash

if [[ $# -eq 2 ]] && [[ "x$2" = "xadMin" ]] ; then
   /bin/rm -rf $1
fi

