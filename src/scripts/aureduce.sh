: '@(#)aureduce.sh 22.1 03/24/08 1999-2002 '
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

# aureduce.sh
#
# This script will extract anything that relates to .REC directory
# from .praud file
# the output file name is same as input file but ended with .cond instead of .praud
# if the destination directory is not given, output file will be placed 
# at execute directory
#
# Usage: aureduce input_file.praud_OR_.praud_directory <destination_dir>
#

arg1_str=$1
dest_dir=`pwd`
searchstr=".REC"
str1=".aaudit"
str2=".vaudit"
str3=".uat"
str4=".cond"
str5=".trash"
of_ext=".cond"
currnmr=`ls -l /vnmr | awk '{print $11}'`
str6=`basename $currnmr`

if [ x$2 != "x" ]
then
        dest_dir=$2
fi

if [ ! -d $dest_dir ]
then
        echo "\n $0: Destination directory does not exists\n"
        exit
fi


if [ -f $arg1_str ]
then

    #user can either uses just a filename or fullpath name
    fname=`basename $arg1_str`
    echo "fname=$fname"
    if [ x$fname = x$arg1_str ]
    then
	filelist=`pwd`/$arg1_str
    else
	filelist=$arg1_str
    fi

#arg1 is the directory name, get a list of .praud files in that directory
else if [ -d $arg1_str ]
     then 
	flist=`ls $arg1_str | grep .praud`

	for i in $flist
	do
	   filelist="$filelist $arg1_str/$i"
	done
     fi
fi

for ii in $filelist
do

    fname=`basename $ii | cut -d. -f1,2,3`
    outfile=${fname}${of_ext}
    outpath=${dest_dir}/${outfile}
    had_done="no"


    #Don't do it if it had been done once, outfile exists
    #maybe user did extra clicks

    for i in `ls $dest_dir`
    do
	if [ x$i = x$outfile ]
	then
	    had_done="yes"
	fi
    done 

    if [ x$had_done = xno ]
    then

        cat $ii | 
        nawk '
        BEGIN {  y = 0; prev_line_cnt = 0 }

        /^path,/&&/'$searchstr'|'$str1'|'$str2'|'$str3'|'$str4'|'$str5'|'$str6'/{

	    #for now, wish that from header to path will not have 
	    #more than 4 lines, until find way how to do length of an array

	    for (n = 1; n <= prev_line_cnt; n++) {

	       if ( length( prev_line_arr[n]) > 3) {  #some number greater than ""
		    y++
                    arr1[y] = prev_line_arr[n]
	       }
	    }

	    y++
	    arr1[y] = $0

	    while ( $0 !~ /^return,/) {

	         getline
	         y++
	         arr1[y] = $0
	    }

            prev_line_cnt = 0
	    for (i in prev_line_arr)
		delete prev_line_arr[i]

        }

        {
	    prev_line_cnt++
	    prev_line_arr[prev_line_cnt]= $0

	    if ( substr($0,1,7) == "return,") {
		#reset everything here ready to read the next block "Header --> Return"

		prev_line_cnt = 0
		for (i in prev_line_arr)
			delete prev_line_arr[i]
	    }

        }
    
        END {
	    for (i = 1; i <= y; i++)
	       if ( length( arr1[i]) > 0) 
                  print  arr1[i]
        }' > $outpath

    fi

done
