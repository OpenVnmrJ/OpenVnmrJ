: '@(#)setGgrp.sh 22.1 03/24/08 1991-1994 '
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
#!/bin/sh 

usernow=`id | tr '()' '  ' | cut -f2 -d' '`

if (test "$usernow" != "vnmr1")
 then
  echo "You must be vnmr1 to make this change. aborted"
  exit 0
fi

if [ $# != 2 ] 
then
 echo "two arguments - groupname username - must be supplied to $0"
 exit 0
fi
#******************************************************************
grep $2 /vnmr/glide/adm/group > /dev/null
if [ $? -ne 0 ]
then

  if (test "$1" = "public")
   then 
    echo "User "$2" now in "$1" group for GLIDE use"
    cat /vnmr/glide/adm/group
    exit 0
  fi

  awk ' ! /'$1'/ { print $0 } ' /vnmr/glide/adm/group > /vnmr/glide/adm/group1
  awk ' /'$1'/ { print $0 } ' /vnmr/glide/adm/group > /vnmr/glide/adm/group2
  a=`cat /vnmr/glide/adm/group2`' '$2
  if (test "$a" = " $2") 
    then 
        a=$1': '$2 
  fi
  echo $a >> /vnmr/glide/adm/group1
  mv /vnmr/glide/adm/group1 /vnmr/glide/adm/group
  rm /vnmr/glide/adm/group2

    echo "User "$2" now in "$1" group for GLIDE use"
    cat /vnmr/glide/adm/group
  exit 0
else

  cat /vnmr/glide/adm/group | grep $1 | grep $2 > /dev/null
  if [ $? -ne 0 ]
  then
    cat /vnmr/glide/adm/group | sed -e 's/ '$2'//g' > /vnmr/glide/adm/group4
    mv /vnmr/glide/adm/group4 /vnmr/glide/adm/group
    if (test "$1" = "public")
     then 
      echo "User "$2" now in "$1" group for GLIDE use"
      cat /vnmr/glide/adm/group
      exit 0
    fi

    awk ' ! /'$1'/ { print $0 } ' /vnmr/glide/adm/group > /vnmr/glide/adm/group1
    awk ' /'$1'/ { print $0 } ' /vnmr/glide/adm/group > /vnmr/glide/adm/group2
    a=`cat /vnmr/glide/adm/group2`' '$2
    if (test "$a" = " $2") 
    then 
          a=$1': '$2 
    fi
    echo $a >> /vnmr/glide/adm/group1
    mv /vnmr/glide/adm/group1 /vnmr/glide/adm/group
    rm /vnmr/glide/adm/group2

    echo "User "$2" now in "$1" group for GLIDE use"
    cat /vnmr/glide/adm/group
    exit 0
   fi
    echo "User "$2" already in "$1" group for GLIDE use"
    cat /vnmr/glide/adm/group
   exit 0
fi

