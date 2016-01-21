: '@(#)vnmrprint.sh 22.1 03/24/08 1991-2004 '
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
#  vnmrprint
#
#  General purpose print shell for the VNMR system
#
#  Usage  vnmrprint filename [printcap]
#
#  If no printcap is entered, lp will be the default
#

if [ $# -eq 0 -o $# -gt 4 ]
then
   echo vnmrprint: Usage -  vnmrprint filename \[printerName \[printType \[clear \| filename \]\]\]
   exit 1
fi

ostype=`vnmr_uname`

propt="lpr -P"
EXPAND="expand"

if [ x$ostype = "xSOLARIS" ]
then
    propt="lp -s -o nobanner -d"

elif [ x$ostype = "xLinux" ]
then
    propt="lp -d"
fi

if [ $# = 1 ]   #  No printerName given, use default
then
   if [ x$ostype = "xInterix" -o x$ostype = "xLinux" ]
   then
        prcmd="lp"
   else
        prcmd=$propt"lp"
   fi
else
   if [ x"$nmrprinter" = "x" ]
   then
        printer=$2
   else
        printer="$nmrprinter"
   fi
   if [ x$ostype = "xInterix" ]
   then
        prcmd="lp -d $printer"
   else
        prcmd=$propt"$printer"
   fi
fi

if [ x$ostype = "xDarwin" ]
then
   prcmd="lp"
fi
	
if [ $# -lt 3 ]
then
#  cpi is the number of characters per inch
   $EXPAND "$1" | sed -e 's/^//' | fold | $prcmd -o cpi=12
else
   if [ $# = 3 ]  # printerType is given, remove file after printing
   then
       echo "Printing started."
       $EXPAND "$1" | sed -e 's/^//' | fold | $prcmd -o cpi=12
       rm "$1"
   else
       if [ $# = 4 ]  # if a fourth argument is give then either clear
                     # or write to the filename
       then
            if [ x$4 = "xclear" ]
            then
                 rm -f "$1"
                 if [ x$2 = "xnone" -a x$3 = "xnone" ] 
                 then
                    echo "Printer not defined. Printing turned off."
                 else
                    echo "Print file removed."
                 fi
            else
                 mv "$1" "$4"
                 echo "Print file saved to file $4."
            fi
       fi
   fi
fi
