# @(#)vwScript.sh 22.1 03/24/08 - MVME-162 Bootup Script
putenv("TIMEZONE=PDT::420:040102:100102")
_SERVER = &sysBootParams + 20
#nfsMount("bootHost","/home/vnmr","/vnmr")
cd "/vnmr/acq/vxBoot.big"
ld < vwlibs.o
ld < vwcom.o
ld < vwhdobj.o
ld < vwtasks.o
rdate ; date
logMsgCreate(200);
systemInit
