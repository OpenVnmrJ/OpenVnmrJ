#!/bin/sh 

source /vnmr/user_templates/.vnmrenvsh
cmd='autocraftQ(`showlog`)'

if [ $# -gt 0 ]; then
    if [ "x$1" = "xon" ]; then
	cmd='autocraftQ(`monitorlog`)'
    fi
    if [ "x$1" = "xoff" ]; then
	cmd='autocraftQ(`hideProgress`)'
    fi
fi

Vnmrbg -mback -n0 "$cmd"
