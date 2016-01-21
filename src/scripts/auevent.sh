: '@(#)auevent.sh 22.1 03/24/08 1999-2002 '
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
#!/usr/bin/sh 
#
# auevent.sh
#
# This script extracts all occurrents of a selected event from nmr relate
# condensed audit file (.cond)
# Usage: auevent .cond_file event
# Eg: auevent 20020326212342.20020329210051.stone.cond unlink

#header:
# creat
# unlink
# rename

#return:
# failure:
# success,

if [ $# -ne 2 ]
then
	echo "\n $0: Must called with 2 arguments"
	echo " Usage: auevent .cond_file event \n"
	exit
fi

dest_dir=`pwd`
searchstr=$2
of_ext="."${searchstr}


fname=`basename $1 | cut -d. -f1,2,3`
outfile=${fname}${of_ext}
outpath=${dest_dir}/${outfile}

#Don't do it if it had been done once, outfile exists
#maybe user did extra clicks

for i in `ls $dest_dir`
do
        if [ x$i = x$outfile ]
        then
		#echo "File EXISTS"
                exit
        fi
done

cat $1 | 
nawk '
BEGIN { FS=","; y = 0; prev_line_cnt = 0 }

#/^header,/{
/^header,/&&/'$searchstr'/{

	pseudo_line = 0
	fail_action = 0

	a_action = $4
	
	if ( NF == 7 ) {
		a_date = $6
	} else if ( NF == 8 ) {
		a_date = $7
	}


	while ( $0 !~ /^return,/) {

	    getline

	    if ( $0 ~ /^path,/) {
	        if ( $0 ~ /pseudo/)
		    pseudo_line = 1

	        a_path = $2

	    } else if ( $0 ~ /^subject,/) {

			a_user = $2

	    }
	}

	#get to return line
	a_result = $2

	if ( $0 ~ /failure:/)
		fail_action = 1

	if ( (pseudo_line == 0) && (fail_action == 1) )
	   print a_date "  " a_user "  " a_result "  " a_action "  " a_path

} ' > $outpath
