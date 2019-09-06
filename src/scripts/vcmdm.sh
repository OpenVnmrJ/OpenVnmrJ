#!/bin/bash

if [[ $# -eq 3 ]] && [[ "x$3" = "xadMin" ]] ; then
   /bin/mv $1 $2
fi

