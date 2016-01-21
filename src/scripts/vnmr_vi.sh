: '@(#)vnmr_vi.sh 22.1 03/24/08 1991-1996 '
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
ostype=`uname -s`
if [ x$ostype = "xDarwin" ]
then
  open -a /Applications/TextEdit.app -W $@
elif [ x$ostype = "xInterix" ]
then
  unix2dos $@ $@
  notepad "$(unixpath2win $@)"
else
  if [ x$graphics = "xsun" ]
  then
    vxrTool vi $@
  else
    vi $@
  fi
fi
