: '@(#)vnmr_textedit.sh 22.1 03/24/08 1991-1996 '
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

common_env() {
    ostype=`uname -s`
    osmajor=`uname -r | awk 'BEGIN { FS = "." } { print $1 }'`

    if [ $osmajor -lt  5 ]
    then
        svr4="n"
    else
        svr4="y"
        ostype="solaris"
    fi
}

nnl_echo() {
    if test x$svr4 = "x"
    then
        echo "error in echo-no-new-line: svr4 not defined"
        exit 1
    fi

    if test $svr4 = "y"
    then
        if test $# -lt 1
        then
            echo
        else
            echo "$*\c"
        fi
    else
        if test $# -lt 1
        then
            echo
        else
            echo -n $*
        fi
    fi
}

common_env
if [ x$graphics = "xsun" ]
then
    textedit $*
else
    echo "textedit is only available in a GUI environment"
    nnl_echo "press Return to continue: "
    read a
fi
