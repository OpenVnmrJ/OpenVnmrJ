: '@(#)cmd_w_count.sh 22.1 03/24/08 1991-1996 '
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
#
#  Execute 1st argument (command) multiple times
#  2nd argument gives the Count.
#
#  Use quotes if command has arguments:
#    cmd_w_count "wc *.c" 5

n=$2
echo "Counting to $2"
i=0
while [ $i -lt $n ]
do
    i=`expr $i + 1`
    $1
done
echo
