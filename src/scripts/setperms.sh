:
: '@(#)setperms.sh 22.1 03/24/08 1996-1997 '
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
if [ $# -lt 4 ]
then
  echo 'Usage - setperms "directory name" "dir permissions" "file permissions" "executable permissions"'
  echo ' E.g. "setperms /sw2/cdimage/code/tmp/wavelib 775 655 755" or "setperms /common/wavelib g+rx g+r g+x" '
  exit 0
fi
dirperm=$2
fileperm=$3
execperm=$4

if [ $# -lt 5 ]
then
  indent=0
else
  indent=$5
fi

pars=`(cd $1; ls)`
for file in $pars
do
#  indent to proper place
   spaces=$indent
   pp=""
   while [ $spaces -gt 0 ]
   do
     pp='-'$pp
     spaces=`expr $spaces - 1`
   done

# test for director, file, executable file
  if [ -d $1/$file ]
  then
    echo "${pp}chmod $dirperm $file/"
    chmod $dirperm $1/$file
    indent=`expr $indent + 4`
    setperms $1/$file $dirperm $fileperm $execperm $indent
    indent=`expr $indent - 4`
  elif [ -f $1/$file ]
  then
    if [ -x $1/$file ]
    then
      echo "${pp}chmod $execperm $file*"
      chmod $execperm $1/$file
    else
      echo "${pp}chmod $fileperm $file"
      chmod $fileperm $1/$file
    fi
  else
   echo file:  $1/$file not modified
  fi
done

