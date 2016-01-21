# @(#)vwAutoScript.sh 22.1 03/24/08  - Automation Bootup Script
putenv("TIMEZONE=PDT::420:040102:100102")
_SERVER = &sysBootParams + 20
#nfsMount("bootHost","/home/vnmr","/vnmr")
cd "/vnmr/acq"
ld < vwauto.o
rdate ; date
logMsgCreate(200);
systemInit
