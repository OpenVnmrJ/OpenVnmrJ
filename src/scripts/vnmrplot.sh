: '@(#)vnmrplot.sh 22.1 03/24/08 1991-2004 '
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
#  vnmrplot
#
#  General purpose plot shell for the VNMR system
#
#  Usage  vnmrplot filename [plotterType]
#
#  If no plotterType is enter, the laser will be the default
#

# SFU/SUA Win native lp command (ours)
WinNativeLP="/bin/Lp.exe"

if [ $# -eq 0 -o $# -gt 3 ]
then
   echo " vnmrplot error: "
   echo "  Usage --  vnmrplot filename [plotterType [clear|filename]] "
   exit 1
fi
ostype=`vnmr_uname`

prcmd="lpr -P "

if [ x$ostype = "xSOLARIS" ]
then
     prcmd="lp -s -c -o nobanner -d "
fi

status=0
if [ $# = 1 ]   #  No plotterType given, use default
then
   if [ x$ostype = "xDarwin" ]
   then
      lp "$1"
   elif [ x$ostype = "xInterix" ]
   then
      # lp -d "default" "$1"
      /usr/contrib/win32/bin/cmd /c $(unixpath2win $WinNativeLP) -d "default" "$(unixpath2win $1)"
      status=$?
   else
      if [ x$ostype = "xSOLARIS" ]
      then
         $prcmd"lp" "$1" > /dev/null
      else
         lpr "$1" > /dev/null
      fi
   fi
   if [ $status -ne 0 ]; then
       echo "Plotting Failed."
   else
       echo "Plotting started."
  fi
else
   if [ $# = 2 ]  # if plotterType given, find out which one
   then
      if [ x$ostype = "xDarwin" ]
      then
         lp -s "$1"
      elif [ x$ostype = "xInterix" ]
      then
         if [ x"$nmrplotter" = "x" ]
         then
             plotter=$2
         else
             plotter=$nmrplotter
         fi
         # lp -d "$plotter" "$1"
         /usr/contrib/win32/bin/cmd /c $(unixpath2win $WinNativeLP) -d "$plotter" "$(unixpath2win $1)"
         status=$?
      else
         $prcmd"$2" "$1" > /dev/null
      fi
      if [ $status -ne 0 ]; then
         echo "Plotting Failed."
      else
         echo "Plotting started."
     fi
      rm -f "$1"
   else
      if [ $# = 3 ]  # if a third argument is give then either clear
                     # or write to the filename
      then
         if [ x"$3" = "xclear" ]
         then
             rm -f "$1"
             if [ x$2 = "xnone" ] 
             then
                echo "Plotter not defined. Plotting turned off."
             else
                echo "Plot file removed."
             fi
         else
             mv "$1" "$3"
             echo "Plot file saved to file $3."
         fi
      fi
   fi
fi
