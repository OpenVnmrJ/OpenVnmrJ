:
: '@(#)setparperm.sh 22.1 03/24/08 1996-1996 '
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
if [ $# -lt 1 ]
then
  echo 'Usage - setparperm "directory name of Vnmr parameters sets"'
  echo ' E.g. "setparperm /vcommon/gpar" or "setparperm /vcommon/upar" '
  exit 0
fi
echo "About to change group and permission of Vnmr parameters in $1"
echo "Note: You need to have Write Access for changes to be done."
echo " Proceed ? (y or n): "
read answr
if [ ! x$answr = "xy" ]
then
   echo User Aborted.
   exit
fi
pars=`(cd $1; ls)`
for file in $pars
do
 echo "chgrp software $file"
 chgrp software $1/$file
if [ -d $1/$file ]
then
  echo "chmod g+rwx $file"
  echo "chmod o+rx $file"
  chmod g+rwx $1/$file
  chmod o+rx $1/$file
  setparperm $1/$file
elif [ -f $1/$file ]
then
  echo "chmod g+rw $file"
  echo "chmod o+r $file"
  chmod g+rw $1/$file
  chmod o+r $1/$file
else
 echo file:  $1/$file not modified
fi
done

