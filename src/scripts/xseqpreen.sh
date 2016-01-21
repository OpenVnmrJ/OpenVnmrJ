: '@(#)xseqpreen.sh 22.1 03/24/08 1991-1996 '
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
#*******************************************************************
#**  xseqpreen:	    					          **
#**                                                               **
#**  Check for syntactically correct but probably unintended C 	  **
#**  constructs.                                                  **
#**                                                               **
#**                                                               **
#**  CAVEAT:                                                      **
#**                                                               **
#**  These checks are performed inside of comments as well as     **
#**  outside!						          **
#**   				   Author: Russ Schmidt  870825   **
#**		     PSG Modifications by: Greg Brissey	 880630   **
#*******************************************************************
set name=`basename $0`
if ($1 == '') then
	echo "Usage: $name filename(s)"
	exit -1
endif
foreach file  ( $argv )
  egrep -n 'if [ 	]*\(.*\)[ 	]*;' $file > /tmp/$name.$$
  egrep -n 'while [ 	]*\(.*\)[ 	]*;' $file >> /tmp/$name.$$
  egrep -n 'for [ 	]*\(.*\)[ 	]*;' $file >> /tmp/$name.$$
  set count=`cat /tmp/$name.$$ | wc -l`
  if ($count == 0) then
      rm -f /tmp/$name.$$
  else
      echo "'$file' the following lines have questionable if's, while's, or for's:"
      sort -n /tmp/$name.$$ | sed -e 's/^/line /' -e 's/:/	/'
      rm -f /tmp/$name.$$
  endif
end
exit 0 
