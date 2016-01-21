: '@(#)convertgeom.sh 22.1 03/24/08 1999-2002 '
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
#! /bin/sh -f

# Takes an image file and converts the file format and resizes
# First argument is the size, second argument is the layout, third argument is 
# the plotter, fourth argument is the filename, fifth argument is the filename 
# to be converted to, sixth argument if given is type of file to be converted 
# to, otherwise the ending of the file (e.g. gif) is considered to be the type


if test $# -gt 4
then
    size=$1
    layout=$2
    plotter=$3
    file="$4"
    filewrite="$5"
 
    if [ $layout -ne 0 ]
    then
	convert -rotate $layout "$file" "$file"
    fi
    if test $# -gt 5
    then
	type=$6
	# -page -30+770 assumes a page 790 or less (letter 612x792)
	# and it puts the image 30 dots in from the left and for letter
	# size, 22 dots down.  A4 is longer so more margin si left at the top
	convert -geometry $size -page -30+770 "$file" $type:"$filewrite"
	rm "$file"
    else
        convert -geometry $size -page -30+770 "$file" "$filewrite"
	rm "$file"
    fi
    
    vnmr_cdump "$filewrite" $plotter  
else
    echo "Usage: needs 3 arguments"
fi
