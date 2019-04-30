#!/usr/bin/env bash

# Looks for sendMessage macro in systemdir or userdir
if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi
if [ x$vnmruser = "x" ]
then
   source $vnmrsystem/user_templates/.vnmrenvsh
fi
export DISPLAY=:0.0

if [ $# -lt 1 ]; then
    Arg="help"
else
    Arg="$1"
fi

if [ "x$Arg" = "xhelp" ]; then

	#  We should use the display.jar
    if [ -f $vnmruser/manual/ovjcraft ]; then
        cat $vnmruser/manual/ovjcraft
    elif [ -f $vnmruser/CPpatch/CRAFTpatch/manual/ovjcraft ]; then
        cat $vnmruser/CPpatch/CRAFTpatch/manual/ovjcraft
    elif [ -f $vnmrsystem/manual/ovjcraft ]; then
        cat $vnmrsystem/vnmrsys/manual/ovjcraft
    else
        echo "Supports 4 arguments -"
        echo "    submit - displays submit2craft popup"
        echo "    admin  - displays craft queue manager"
        echo "    qest   - displays craftQnmr options"
	echo "    pref   - displays craftPref options"
        echo "    exit   - exits craft application"
    fi

elif [ "x$Arg" = "xfg" ]; then
    vnmrj -exec craftv5 -splash $vnmrsystem/iconlib/Splash_CRAFT.png
else
    if [ "x$Arg" = "xsubmit" ]; then
	Vnmrbg -mback -n- -s sendMessage "submit2craft('window') s2cparbg='bg'" "-splash $vnmrsystem/iconlib/Splash_CRAFT.png"
    elif [ "x$Arg" = "xexit" ]; then
        Vnmrbg -mback -n0 -s sendMessage "exitComm"
    elif [ "x$Arg" = "xadmin" ]; then
        Vnmrbg -mback -n- -s sendMessage "craftQueueManage('window') cQMpar2='bg'" "-splash $vnmrsystem/iconlib/Splash_CRAFT.png"
    elif [ "x$Arg" = "xqest" ]; then
        Vnmrbg -mback -n- -s sendMessage "craftQnmr('window')" "-splash $vnmrsystem/iconlib/Splash_CRAFT.png"
    elif [ "x$Arg" = "xpref" ]; then
        Vnmrbg -mback -n- -s sendMessage "craftPref('window') cpparbg='bg'" "-splash $vnmrsystem/iconlib/Splash_CRAFT.png"
    fi
fi
