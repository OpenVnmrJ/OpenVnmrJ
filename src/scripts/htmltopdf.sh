#! /bin/sh
#  '@(#)htmltopdf.sh  1991-2014 '
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
#-----------------------------------------------

if [  -r /etc/SuSE-release ]
then
   distrover=5.0
elif [  -r /etc/debian_version ]
then
   distrover=`lsb_release -rs` # 8.04, 9.04, etc.
else
   distrover=`cat /etc/redhat-release | sed -r 's/[^0-9]+//' | sed -r 's/[^0-9.]+$//'`    # yield 5.1, 5.3 , 6.1, etc..
fi
if [[ $distrover < 6.0 ]];
then
  wkhtmltopdf --page-size Letter $1 $2
else
  wkhtmltopdf --no-footer-line --footer-font-size 8 --footer-right "[page]/[topage]" --page-size Letter $1 $2
fi
