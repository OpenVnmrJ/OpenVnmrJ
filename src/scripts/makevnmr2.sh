: '@(#)makevnmr2.sh 22.1 03/24/08 1991-1996 '
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
: /bin/sh
:
:  By prefacing these lines with colons, we insure that
:  the sh shell is used.
#
#

#  a system-independent echo w/no-new-line
#  Note:  insists on an environmental variable sysV being defined.

nnl_echo() {
    if test x$sysV = "x"
    then
        echo "error in echo-no-new-line: sysV not defined"
        exit 1
    fi

    if test $sysV = "y"
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

# IRIX does not support recursive chmod

ch_mod() {
    if [ x$ostype = "xIRIX" ]
    then
	find $* -exec chmod 644 {} \;
    else
	chmod -R 644 $*
    fi
    chmod 755 $*
}

ch_xmod() {
    if [ x$ostype = "xIRIX" ]
    then
	find $* -exec chmod 755 {} \;
    else
	chmod -R 755 $*
    fi
}

######################################################################
#  Start of main script
######################################################################

#  Figure out System V vs. SunOS
#  Only works on Sun systems ...  not IBM
#  required for nnl_echo program

if [ x$ostype = "xAIX" -o x$ostype = "xIRIX" -o x$ostype = "xSOLARIS" ]
then
    sysV="y"
else
    sysV="n"
fi

nnl_echo "   8"

chmod 777   ./tmp
chmod 644   ./bootup_message
chmod 644   ./conpar
chmod 644   ./devicenames
chmod 644   ./devicetable
if (test -f ./rc.vnmr)
then
   chmod 644   ./rc.vnmr
fi
chmod 644   ./solvents
chmod 666   ./acqqueue/acqinfo
chmod 777   ./acqqueue
if (test -f ./varian.icon)
then
   chmod 644   ./varian.icon
fi
if (test -f ./varian.xicon)
then
   chmod 644   ./varian.xicon
fi
if [ x$ostype = "xIRIX" ]
then
   find ./user_templates -exec chmod 644 {} \;
   chmod    755   ./user_templates
else
   chmod -Rf 644  ./user_templates
   chmod    755   ./user_templates
fi
if (test -f ./user_templates/setow2)
then
   chmod 755   ./user_templates/setow2
fi
if (test -f ./user_templates/setow3)
then
   chmod 755   ./user_templates/setow3
fi
#
nnl_echo "   7"
ch_mod "./menulib"
nnl_echo "   6"
ch_mod "./maclib"
nnl_echo "   5"
ch_mod "./manual"
nnl_echo "   4"
#
#  Following directories do not have many files
#
ch_mod "./nuctables"
if (test -d ./tablib)
then
   ch_mod  "./tablib"
fi
if (test -d ./shapelib)
then
   ch_mod  "./shapelib"
fi
nnl_echo "   3"
ch_mod "./shimmethods"
ch_mod "./shims"
nnl_echo "   2"
ch_mod "./help"
ch_xmod "./bin"
nnl_echo "   1"
if (test -d ./acq)
then
   ch_xmod  "./acq"
fi
ch_xmod  "./acqbin"
if (test -d ./binx)
then
   ch_xmod  "./binx"
fi
if (test -d ./xvfonts)
then
   ch_mod "./xvfonts"
fi
ch_mod  "./fidlib"
chmod 755   ./fidlib/*
#
# Set modes for imaging subdirectories.
#
if (test -d ./maclib/maclib.imaging)
then
   chmod 755  ./maclib/maclib.imaging
fi
if (test -d ./menulib/menulib.imaging)
then
   chmod 755  ./menulib/menulib.imaging
fi
if (test -d ./help/help.imaging)
then
   chmod 755  ./help/help.imaging
fi
if (test -d ./user_templates/ib_initdir)
then
   chmod 755  ./user_templates/ib_initdir
   chmod 755  ./user_templates/ib_initdir/filter
   chmod 755  ./user_templates/ib_initdir/gframe
   chmod 755  ./user_templates/ib_initdir/roi
fi
#
# Allow group access for gradtables.
#
if (test -d ./imaging/gradtables)
then
   chmod 775  ./imaging/gradtables
fi
#
# Set uid to vnmr1 for cptoconpar
#
if (test -f ./bin/cptoconpar)
then
   chmod u+s  ./bin/cptoconpar
fi

echo    "   0"
