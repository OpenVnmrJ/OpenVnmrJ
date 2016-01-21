: '@(#)nvsetacq.sh 22.1 03/24/08 2003-2004 '
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
#
: /bin/sh

etc="/etc"
vnmrsystem="/vnmr"

cntlrList="	\
	master1	\
	rf1	\
	rf2	\
	rf3	\
	rf4	\
	rf5	\
	rf6	\
	rf7	\
	rf8	\
	rf9	\
	rf10	\
	rf11	\
	rf12	\
	rf13	\
	rf14	\
	rf15	\
	rf16	\
	pfg1	\
	grad1	\
	lock1	\
	ddr1	\
	ddr2	\
	ddr3	\
	ddr4	\
	ddr5	\
	ddr6	\
	ddr7	\
	ddr8	\
	ddr9	\
	ddr10	\
	ddr11	\
	ddr12	\
	ddr13	\
	ddr14	\
	ddr15	\
	ddr16	\
        V-Cryobay\
	V-Autosampler\
	V-Slim\
        V-LcWorkstation\
	V-Pda\
	V-Protune\
	V-AS768\
	V-Trap\
	V-AS768Robot\
	"

addToHosts()
{
chmod +w $etc/hosts
grep -sw "wormhole"  $etc/hosts > /dev/null
if [ $? -ne 0 ]
then 
   echo "$base_IP.1\twormhole" >> $etc/hosts
fi

i=0
for cntlr in $cntlrList
do
  case $cntlr in
    master1 )
      i=10 ;
      ;;
    rf1 )
      i=20 ;
      ;;
    pfg1 )
      i=40 ;
      ;;
    grad1 )
      i=42 ;
      ;;
    lock1 )
      i=44 ;
      ;;
    ddr1 )
      i=50 ;
      ;;
    V-Cryobay )
      i=250
      ;;
    V-Autosampler )
      i=249
      ;;
    V-Slim )
      i=248
      ;;
    V-LcWorkstation )
      i=247
      ;;
    V-Pda )
      i=246
      ;;
    V-Protune )
      i=245
      ;;
    V-AS768 )
      i=244
      ;;
    V-Trap )
      i=243
      ;;
    V-AS768Robot )
      i=242
      ;;
    *)
      i=`expr $i + 1`
      ;;
   esac
   grep -w $cntlr $etc/hosts > /dev/null
   if [ $? -ne 0 ]
   then
      echo "$base_IP.$i\t$cntlr" >> $etc/hosts
   fi
done
chmod -w $etc/hosts
}

addToHostsEquiv()
{
if [ ! -f $etc/hosts.equiv ]
then
   touch $etc/hosts.equiv
fi
for cntlr in $cntlrList
do
   grep -w $cntlr $etc/hosts.equiv > /dev/null
   if [ $? -ne 0 ]
   then
      echo "$cntlr" >> $etc/hosts.equiv
   fi
done
}

addToEthers()
{
if [ ! -f $etc/ethers ]
then
   touch $etc/ethers
fi
# ether_base is hard coded
base_ethers="00:60:93:03"
#initialize
prev_cntlr="master0"
i=0
j=0
grep -w "master1" $etc/ethers > /dev/null #all or none, the addresses are hardcoded
if [ $? -ne 0 ]
   then
   for cntlr in $cntlrList
   do
      a=`echo $prev_cntlr | cut -c 1,1 -`
      b=`echo $cntlr | cut -c 1,1 -`
      prev_cntlr=$cntlr
      if [ x$a = x$b ]
      then
         j=`expr $j + 1`
      else
         j=1
         i=`expr $i + 10`
      fi
      echo "$base_ethers:$i:$j\t$cntlr" >> $etc/ethers
   done
fi
}

stop_program () {
  findproc="ps -e  | grep $1 | awk '{ printf(\"%d \",\$1) }'"
  npids=`eval $findproc`
  for prog_pid in $npids
  do
    kill -2 $prog_pid
    sleep 5             # give time for kill message to show up.
  done
  # test to be sure the program died, if still running try kill -9
  npids=`eval $findproc`
  if (test x"$npids" != "x" )
  then   
    for prog_pid in $npids
    do   
       kill -9 $prog_pid
       sleep 5          # give time for kill message to show up.
    done
    # Once again double check to see if any are still running.
    npids=`eval $findproc`
    if (test x"$npids" != "x" )
    then
      for prog_pid in $npids
      do 
        echo "Unable to kill '$1' (pid=$prog_pid)"
      done
    fi
  fi
}




#
#  MAIN Main main
#

reboot=0
OS_NAME=`uname -s`               
if (test ! $OS_NAME = "SunOS")   
then                             
   echo "$0 suitable for Sun-based systems only"
   echo "$0 exits"               
   exit 0                        
fi                               

findacqproc="ps -e  | grep Vnmr | awk '{ printf(\"%d \",\$1) }'"
npids=`eval $findacqproc`
if (test x"$npids" != "x" )
then 
   echo ""
   echo "You must exit all 'VnmrJ'-s to run $0"
   echo "Please type 'exit' in the VnmrJ input window,"
   echo "or use 'ps -e | grep Vnmr' and 'kill -3 pid' to exit VnmrJ-s."
   echo "Then restart $0"
   echo ""
   exit 0
fi

#-----------------------------------------------------------------
# Login the user as a root user
# Use the "su" command to ask for password and run the installer
#

notroot=0
userId=`/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To install VNMR you will need to be the system's root user."
  echo "Or type cntrl-C to exit.\n"
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password \n"
     su root -c "$vnmrsystem/bin/nvsetacq ${ARGS}";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again \n"
  fi
  exit
fi

#-----------------------------------------------------------------
# kill rarpd, otherwhise otherwhise the console will reboot
# with the files in /tftpboot (if any)
#-----------------------------------------------------------------
echo "One moment please..."
stop_program in.rarpd

#-----------------------------------------------------------------
# Check if the Expproc is still running. If so run 
# /vnmr/execkillacqproc,  afterall, we are already root.
#-----------------------------------------------------------------
findacqproc="ps -e  | grep Expproc | awk '{ printf(\"%d \",\$1) }'"
npids=`eval $findacqproc`
if (test x"$npids" != "x" )
then 
    "$vnmrsystem"/bin/execkillacqproc
fi

#-----------------------------------------------------------------
# Find the number of ethernet cards in the system
#-----------------------------------------------------------------
n_bge=`"$vnmrsystem"/bin/findedevices /dev/bge | wc -l` 
n_ce=`"$vnmrsystem"/bin/findedevices /dev/ce | wc -l` 
# the next line is to work around a bug, where if there is no /dev/ce
# the system claims there are 5; certain Solaris 8 version only
if [ $n_ce -gt 4 ] 
then
    n_ce=0
fi
n_eri=`"$vnmrsystem"/bin/findedevices /dev/eri | wc -l` 
n_hme=`"$vnmrsystem"/bin/findedevices /dev/hme | wc -l` 


#-----------------------------------------------------------------
# select interface
#-----------------------------------------------------------------

echo "Select from the options below:"
echo "1. Your SUN is attached to the console via the standard ethernet"
echo "   port or router."
echo "2. Your SUN is attached to the console via the second ethernet"
echo "   port."
echo " "
echo "What is your configuration? (1 or 2) [2]: \c"
read NICselect
if ( test x$NICselect != "x1" )
then
   NICselect=2
fi

hostname=`hostname`
hostnameFile=`ls $etc/hostname.*[0-9]`
for file in $hostnameFile
do
   name=`cat $file`
   if [ x$name = x$hostname ]
   then
      driver1=`basename $file | cut -d. -f2`
   fi
done

if [ $NICselect -eq 2 ]
then
#start with the slowest (le), if there is a faster on we'll find it
#in order: le, hme, eri, bge, ce
   driver=`echo $driver1 | tr -d 0123456789`
   if [ x$driver1 = "xle0" ]
   then
      driver2="le1"
   else
      driver2="le0"
   fi
   #-----------------------------
   if [ x$driver = "xhme" ]
   then
      n_hme=`expr $n_hme - 1`
   fi
   if [ $n_hme -gt 0 ]
   then
      driver2="hme0"
      if [ x$driver1 = "xhme0" ]
      then
         driver2="hme1"
      fi
   fi
   #-----------------------------
   if [ x$driver = "xeri" ]
   then
      n_eri=`expr $n_eri - 1`
   fi
   if [ $n_eri -gt 0 ]
   then
      driver2="eri0"
      if [ x$driver1 = "xeri0" ]
      then
         driver2="eri1"
      fi
   fi
   #-----------------------------
   if [ x$driver = "xbge" ]
   then
      n_bge=`expr $n_bge - 1`
   fi
   if [ $n_bge -gt 0 ]
   then
      driver2="bge0"
      if [ x$driver1 = "xbge0" ]
      then
         driver2="bge1"
      fi
   fi
   #-----------------------------
   if [ x$driver = "xce" ]
   then
      n_ce=`expr $n_ce - 1`
   fi
   if [ $n_ce -gt 0 ]
   then
      driver2="ce0"
      if [ x$driver1 = "xce0" ]
      then
         driver2="ce1"
      fi
   fi
fi
      
#-----------------------------------------------------------------
# RFC1597 the following net numbers are reserved for private networks
# 10.0.0.0      - 10.255.255.255
# 172.16.0.0    - 172.31.255.255
# 192.168.0.0   - 192.168.255.255
#
# base_IP should be determined from hostname , 
# use 10.0.0 if not used already, else use 172.16.0
#-----------------------------------------------------------------
host_IP=`grep -w $hostname $etc/hosts | cut -d. -f1,2,3 -`
if [ $NICselect -eq 1 ]
then
   base_IP=$host_IP
else
   octet=`grep -w $hostname $etc/hosts | cut -d. -f1`
   if [ x$octet = "x10" ]
   then
      base_IP="172.16.0"
   else
      base_IP="10.0.0"
   fi
fi

if [ $NICselect -eq 1 ]
then
   echo NIC to console = $driver1, subnet=$base_IP
else
   echo NIC to main net is $driver1, subnet=$host_IP
   echo NIC to console  is $driver2, subnet=$base_IP
fi

#----------------------------------------------------------------------------
# check hostname.le1 (etc), create if needed
#----------------------------------------------------------------------------
if [ $NICselect -gt 1 ]
then
   if [ ! -f $etc/hostname.$driver2 ]
   then
       echo  "wormhole" > $etc/hostname.$driver2

      # next two commands "turn on" the ethernet interface,
      # so catcheaddr can receive the broadcast RARP packet
      # ifconfig commands taken from /etc/init.d/rootusr

      /sbin/ifconfig $driver2 plumb
      /sbin/ifconfig $driver2 inet "${base_IP}.1" netmask + \
						broadcast + -trailers up \
						2>&1 > /dev/null
   fi
fi

addToHosts
addToHostsEquiv
addToEthers


#-----------------------------------------------------------------
# fix /etc/inetd.conf so tftpd gets started
# This done only for the convinience of the developer, not needed
#-----------------------------------------------------------------
grep -w "#tftp" $etc/inetd.conf > /dev/null
if [ $? -eq 0 ]
then
   chmod +w $etc/inetd.conf
   cat $etc/inetd.conf | sed -e 's/#tftp/tftp/' > $etc/inetd.conf.tmp
   cp $etc/inetd.conf.tmp $etc/inetd.conf
   rm $etc/inetd.conf.tmp
   chmod -w $etc/inetd.conf
   pkill -HUP inetd
fi

#-----------------------------------------------------------------
# fix /etc/nsswitch.conf to continue looking locally
# this is done whether NIS is enabled or not, if not enabled 'return'
# won't be found. The assumption is that if all other files are OK
# (i.e. roboot=0) that this is ok, reboot is not set here.
#-----------------------------------------------------------------
chmod +w $etc/nsswitch.conf
cat $etc/nsswitch.conf | sed -e                                     \
's/hosts:      nis \[NOTFOUND=return\]/hosts:      nis /' |         \
                         sed -e                                     \
's/hosts:      xfn nis \[NOTFOUND=return\]/hosts:      xfn nis /' | \
                         sed -e                                     \
's/ethers:     nis \[NOTFOUND=return\]/ethers:     nis /' |         \
                         sed -e                                     \
's/bootparams: nis \[NOTFOUND=return\]/bootparams: nis /'           \
                                             > $etc/nsswitch.conf.tmp
cp $etc/nsswitch.conf.tmp $etc/nsswitch.conf
rm $etc/nsswitch.conf.tmp
chmod -w $etc/nsswitch.conf

#-----------------------------------------------------------------
# create /tftpboot if needed, copy vxBoot from /vnmr/acq
# this starts in.rarpd 
#-----------------------------------------------------------------
if [ ! -d /tftpboot ]
then
   if [ -L /tftpboot  -o -f /tftpboot ]
   then
      echo "/tftpboot exists but is not a directory"
      echo "This needs to be corrected."
      echo "Then restart setacq"
      echo ""
      exit 0
   fi
   mkdir -p /tftpboot
fi

#-----------------------------------------------------------------
# Arrange for Acqproc to start at system bootup
#-----------------------------------------------------------------
    cp -p "$vnmrsystem"/rc.vnmr $etc/init.d
    (cd $etc/rc3.d; if [ ! -h S19rc.vnmr ]; then
        ln -s ../init.d/rc.vnmr S19rc.vnmr; fi)
    (cd $etc/rc0.d; if [ ! -h K19rc.vnmr ]; then
        ln -s ../init.d/rc.vnmr K19rc.vnmr; fi)
    # this creates the acqpresent file in /etc
    touch /etc/acqpresent

    # this deletes the statpresent file in /etc. This is not needed for
    # INOVA. Because we created it at one point, we make extra sure it isn't
    # there anymore
    if [ -f /etc/statpresent ]
    then
        rm -f /etc/statpresent
    fi

#-----------------------------------------------------------------
# Also, create /etc/norouter, so that no matter how the install
# is done (e.g. incomplete nets) the system does not become a router
#-----------------------------------------------------------------
if [ ! -f /etc/notrouter ]
then
    touch /etc/notrouter
fi
#-----------------------------------------------------------------
# Check if we need to create a default router
# This would stop in.routed from being started at bootup
#-----------------------------------------------------------------
if [ $NICselect -ge 1 ]
then
   if [ -f $etc/defaultrouter ]
   then
       n=`wc -l $etc/defaultrouter | awk '{ printf("%d",$1) }'`
       if [ $n -ne 0 ]
       then
          echo ""
       else
         reboot=1
         line=`netstat -r -n | grep default`
         echo $line | awk 'BEGIN { FS = " " } { print $2 }' > $etc/defaultrouter
       fi
   else
      reboot=1
      line=`netstat -r -n | grep default`
      echo $line | awk 'BEGIN { FS = " " } { print $2 }' > $etc/defaultrouter
   fi
fi

#-----------------------------------------------------------------
# Remove some files (Queues) NOT IPC_V_SEM_DBM
# Because cleanliness is next to ... you know.
#-----------------------------------------------------------------
rm -f /tmp/ExpQs
rm -f /tmp/ExpActiveQ
rm -f /tmp/ExpStatus
rm -f /tmp/msgQKeyDbm
rm -f /tmp/ProcQs
rm -f /tmp/ActiveQ

PATH=/vnmr/bin:"$PATH"
export PATH
chmod 775 /tftpboot
rm -f /tftpboot/ddrexec.o  /tftpboot/gradientexec.o
rm -f /tftpboot/lockexec.o /tftpboot/masterexec.o
rm -f /tftpboot/pfgexec.o  /tftpboot/rfexec.o
rm -f /tftpboot/nvlib.o    /tftpboot/vxWorks405gpr.bdx
rm -f /tftpboot/nvScript   "$vnmrsystem"/acq/download/load*
if [ $NICselect -eq 1 ]
then
  driver=`echo $driver1 | tr -d 0123456789`
  unit=`echo $driver1 | tr -d $driver`
  /usr/sbin/in.rarpd $driver $unit
else
  driver=`echo $driver2 | tr -d 0123456789`
  unit=`echo $driver2 | tr -d $driver`
  /usr/sbin/in.rarpd $driver $unit
fi
echo "   The download step may take  four to eight minutes"
echo "   Do not reboot the console during this process"
echo ""
"$vnmrsystem"/acqbin/consoledownload
owner=`ls -l "$vnmrsystem"/vnmrrev | awk '{ printf($3) }'`
group=`ls -l "$vnmrsystem"/vnmrrev | awk '{ printf($4) }'`
chown $owner:$group "$vnmrsystem"/acq/download/load* 2> /dev/null

nofile=1
count=0
if [ -f "$vnmrsystem"/acq/download/loadhistory ]
then
   count=`grep successful "$vnmrsystem"/acq/download/loadhistory | wc -l`
fi
if [ $count -lt 9 ]
then
   echo ""
   echo "Console software download failed"
   echo "Reboot the NMR console"
   echo "Then rerun setacq"
   echo ""
   exit 0
fi

sleep 25 

echo ""
echo "NMR Console software installation complete"
#-----------------------------------------------------------------
# Do we need to reboot the SUN? 
#-----------------------------------------------------------------
if [ $reboot -eq 1 ]
then
   echo " "
   echo "You must reboot Solaris for these changes to take effect"
   echo "As root type 'reboot' to reboot Solaris"
else
   "$vnmrsystem"/bin/execkillacqproc
fi

# do we really need to change rarp to work on driver2 only?
# if so, create a file list with 
# fileList=`grep "in.raprd" $etc/rc?.d/*`
