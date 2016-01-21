# '@(#)vwScriptPPC.sh 22.1 03/24/08 1991-1994 '
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
# @(#)vwScriptPPC.sh 22.1 03/24/08 - MVME-162 Bootup Script
putenv("TIMEZONE=PDT::420:040102:100102")
_SERVER = &sysBootParams + 20
#nfsMount("bootHost","/home/vnmr","/vnmr")
cd "/vnmr/acq/vxBootPPC.big"
ld < vwlibs.o
ld < vwcom.o
ld < vwhdobj.o
ld < vwtasks.o
rdate ; date
logMsgCreate(200);
systemInit
