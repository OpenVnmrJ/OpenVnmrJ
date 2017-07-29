#!/bin/sh 

if [ $# -lt 1 ]; then
    cmd='submit2craft'
elif [ $# -lt 2 ]; then
    cmd='submit2craft(`'$1'`)'
elif [ $# -lt 3 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`)'
elif [ $# -lt 4 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`)'
elif [ $# -lt 5 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`)'
elif [ $# -lt 6 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`)'
elif [ $# -lt 7 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`,`'$6'`)'
elif [ $# -lt 8 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`,`'$6'`,`'$7'`)'
elif [ $# -lt 9 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`,`'$6'`,`'$7'`,`'$8'`)'
elif [ $# -lt 10 ]; then
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`,`'$6'`,`'$7'`,`'$8'`,`'$9'`)'
else
    cmd='submit2craft(`'$1'`,`'$2'`,`'$3'`,`'$4'`,`'$5'`,`'$6'`,`'$7'`,`'$8'`,`'$9'`,`'$10'`)'
fi

#echo $cmd

Vnmrbg -mback -n0 "$cmd"
