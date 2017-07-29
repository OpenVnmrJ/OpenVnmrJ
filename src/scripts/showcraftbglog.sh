#!/bin/sh 

cmd='craftbg(`showlog`)'

if [ $# -gt 0 ]; then
    if [ "x$1" = "xon" ]; then
	cmd='craftbg(`showProgress`)'
    fi
    if [ "x$1" = "xoff" ]; then
	cmd='craftbg(`hideProgress`)'
    fi
fi

Vnmrbg -mback -n0 "$cmd"
