: '@(#)getXrecon.sh 22.1 03/24/08 2003-2005 '
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
#
# getXrecon copies Xrecon to current directory
#
time=`date +%y%m%d_%R`
if [ -d Xrecon ]; then mv Xrecon Xrecon_$time ; fi
echo "Copying external reconstruction source to directory Xrecon ..."
if [ -d /vnmr/imaging/src/Xrecon ]; then
  cp -r /vnmr/imaging/src/Xrecon . ;
elif [ -f /vnmr/imaging/src/Xrecon.tar.gz ]; then
  tar xzf /vnmr/imaging/src/Xrecon.tar.gz ;
fi
if [ -d Xrecon ]; then
  echo "Contents of Xrecon:"
  ls Xrecon
  echo "Please consult README and INSTALL files" ;
else
  echo "Abort: No Xrecon source found" ;
fi
