: '@(#)lnvsetacq2.sh 22.1 03/24/08 '
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
#lnvsetacq.sh

etc="/etc"
vnmrsystem="/vnmr"
tmpDir="/tmp"
netDir="/etc/sysconfig/network-scripts"
consolePort="eth0"

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
	"

peripheralList="	\
	V-Autosampler\
	V-Cryobay\
	V-AS768\
	V-AS768Robot\
	V-LcWorkstation\
	V-Pda\
	V-Protune\
	V-Slim\
	V-Trap
	"

getIpBase()
{
   IpFile=$netDir/ifcfg-$etherPort
   if [ ! -f $IpFile ]
     then
        return 1
   fi
   IPSTR=`awk '/^IPADDR=/ { print substr($0, 8) }' $IpFile`
   if [ x$IPSTR != "x" ]
     then
        ether_IP=$IPSTR
        ether_BASE=`echo "$IPSTR" | awk  '{ split($0, fields, ".")
             printf("%s.%s.%s", fields[1],fields[2],fields[3]) }'`
   fi
}


addToHosts()
{
   if [ ! -f $etc/hosts.orig ]
     then
        cp $etc/hosts $etc/hosts.orig
   fi

   chmod +w $etc/hosts
   tmpHost=$tmpDir/hosts.tmp
   rm -f $tmpHost
   awk '
        BEGIN {
           hlist="'"$cntlrList"'"
           sizeOfList = split(hlist, hArray, FS)
        }
        {
            vhost=0
            for (i = 1; i <= sizeOfList; i++) {
                if ($2 == hArray[i])
                    vhost = i
            }
            if (vhost == 0) {
                if ($2 != "wormhole")
                    print
            }
        }
        END {
        }
   '  $etc/hosts > $tmpHost

   if [ ! -f $tmpHost ]
     then
        return 1
   fi
 
   echo "$main_IP        wormhole" >> $tmpHost
   
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
         *)
           i=`expr $i + 1`
           ;; 
        esac
      echo "$base_IP.$i        $cntlr" >> $tmpHost
   done

   i=0
   for cntlr in $peripheralList
   do
     case $cntlr in
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
      esac
      grep $cntlr $tmpHost > /dev/null
      if [ $? -ne 0 ]
      then
	 echo "$base_IP.$i        $cntlr" >> $tmpHost
      fi
   done

   mv $tmpHost $etc/hosts
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
      grep $cntlr $etc/hosts.equiv > /dev/null
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
   grep "master1" $etc/ethers > /dev/null #all or none, the addresses are hardcoded
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
         echo "$base_ethers:$i:$j        $cntlr" >> $etc/ethers
      done
   fi
}

addToSysctlConf()
{
   # if file not present then make an empty one
   if [ ! -f $etc/sysctl.conf ]
   then
      touch $etc/sysctl.conf
   fi
   grep "net.ipv4.ipfrag_high_thresh" $etc/sysctl.conf > /dev/null  # is the entry already there?
   if [ $? -ne 0 ]   # if not then do the following and add them
   then
     echo " " >> $etc/sysctl.conf
     echo "# " >> $etc/sysctl.conf
     echo "# Agilent Technologies Addition to increase IP fragment reassemble buffer" >> $etc/sysctl.conf
     echo "# default is 256K raising it to 2 MB"  >> $etc/sysctl.conf
     echo "net.ipv4.ipfrag_high_thresh = 2097152" >> $etc/sysctl.conf
     echo " " >> $etc/sysctl.conf
     reboot=1       # mark that a reboot is needed
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


#----------------------------
#  MAIN Main main
#----------------------------
if [ ! x`uname -s` = "xLinux" ]
then
   echo "$0 suitable for Linux-based systems only"
   echo "$0 exits"
   exit 0
fi

arg1=$1
#-----------------------------------------------------------------
# initialize some variables
#
tmpfile=/tmp/tmp_please_remove
reboot=0
main_IP="172.16.0.1"
base_IP="172.16.0"

#-----------------------------------------------------------------
# make sure no VNmrJ is running
#
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
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To run setacq you will need to be the system's root user,"
  echo "or type cntrl-C to exit."
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password"
     echo
     su root -c "$0 $*";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again"
      echo
  fi
  exit
fi

#-----------------------------------------------------------------
# Check how many NICs are present
#
nic_cnt=`/sbin/ifconfig -a | grep -c '^eth'`
if [ $nic_cnt -eq 1 ]
then
       echo
       echo "  This is a single Network Interface Card system"
       echo "  Make sure the NMR console is attached to the "
       echo "  host's only ethernet port (eth0) using a null cable"
       echo

elif [ $nic_cnt -eq 2 -o $nic_cnt -gt 2 ]
then
    consolePort="eth1"
else
    echo
    echo "  No ethernet port found."
    echo
    exit
fi

#-----------------------------------------------------------------
# If there is an argument (eth0 or eth1) then use that port 
# to the console, check if it really exists.
#
if [ $# -gt 0 ]
then
    consolePort=$1
fi

garg="^$consolePort"
nic_exist=`/sbin/ifconfig -a | grep -c $garg`
if [ $nic_exist -eq 0 ]
then
    echo
    echo "  No ethernet port '$consolePort' found."
    echo "  Abort! "
    exit
fi

#-----------------------------------------------------------------
# retrieve IP addresses
# 

etherPort=$consolePort
ether_IP=""
ether_BASE=""
getIpBase
if [ x$ether_IP != "x" ]
 then
     main_IP=$ether_IP
     base_IP=$ether_BASE
fi

echo " "

if [ $nic_cnt -gt 1 ]
then
    echo "The Network Port to console = $consolePort, subnet = $base_IP"

    if [ x$consolePort = "xeth0" ]
     then
           etherPort="eth1"
     else
           etherPort="eth0"
    fi
    ether_IP=""
    ether_BASE=""
    getIpBase
    echo "The Network Port to main net = $etherPort,  subnet = $ether_BASE"
else
    echo "The Network Port to console = $consolePort, subnet = $base_IP"
fi


#-----------------------------------------------------------------
echo "One moment please..."
stop_program rarpd

#-----------------------------------------------------------------
# Check if the Expproc is still running. If so run 
# /vnmr/execkillacqproc,  afterall, we are already root.
#-----------------------------------------------------------------
findacqproc="ps -e  | grep Expproc | awk '{ printf(\"%d \",\$1) }'"
npids=`eval $findacqproc`
if (test x"$npids" != "x" )
then 
    /vnmr/bin/execkillacqproc
fi

#-----------------------------------------------------------------
# Ask the user to reboot the console now,
#-----------------------------------------------------------------
esc="["
echo " "
echo "${esc}48;34mPlease make sure both console and PC are connected.${esc}0m"
echo " "

addToHosts
addToHostsEquiv
addToEthers
addToSysctlConf

#-----------------------------------------------------------------
# fix /etc/xinetd.d/tftp  so tftpd gets started at boot time
#-----------------------------------------------------------------
tftpconf="/etc/xinetd.d/tftp"
rarpd="/etc/init.d/rarpd"
if [ -r $tftpconf ]
then
   grep -w disable $tftpconf | grep -w no 2>&1 > /dev/null
   if [ $? -ne 0 ]
   then
       chmod +w $tftpconf
       cat $tftpconf | sed '/disable/s/= yes/= no/' > $tmpfile
       mv $tmpfile $tftpconf
       chmod -w $tftpconf
       pkill -HUP xinetd
   fi
else
   echo "tftp package is not available. Exit"
   exit
fi

if [ -r $rarpd ]
then
   grep 'daemon /usr/sbin/rarpd -e' $rarpd 2>&1 > /dev/null
   if [ $? -ne 0 ]
   then
       echo "edit $rarpd ---------"
       chmod +w $rarpd
       cat $rarpd | sed -e 's/daemon \/usr\/sbin\/rarpd/daemon \/usr\/sbin\/rarpd -e/' > $tmpfile
       mv $tmpfile $rarpd
       chmod 755 $rarpd
       pkill -HUP rarpd
   fi

   #Arrange for rarp server running at boot time
   (cd /etc/rc5.d; if [ ! -f S98rarpd ]; then \
           ln -s ../init.d/rarpd S98rarpd; fi)
   (cd /etc/rc0.d; if [ ! -f K98rarpd ]; then \
           ln -s ../init.d/rarpd K98rarpd; fi)

else
   echo "rarp package is not available. Exit"
   exit
fi

#-----------------------------------------------------------------
# set write permission on serial port
#-----------------------------------------------------------------
if [ -c /dev/ttyS0 ]
then
    chmod 666 /dev/ttyS0
fi

#for Linux this had been done at sofware installtion time
##-----------------------------------------------------------------
## fix /etc/nsswitch.conf to continue looking locally
## this is done whether NIS is enabled or not, if not enabled 'return'
## won't be found. The assumption is that if all other files are OK
## (i.e. roboot=0) that this is ok, reboot is not set here.
##-----------------------------------------------------------------
#chmod +w $etc/nsswitch.conf
#cat $etc/nsswitch.conf | sed -e                                     \
#'s/hosts:      nis \[NOTFOUND=return\]/hosts:      nis /' |         \
#                         sed -e                                     \
#'s/hosts:      xfn nis \[NOTFOUND=return\]/hosts:      xfn nis /' | \
#                         sed -e                                     \
#'s/ethers:     nis \[NOTFOUND=return\]/ethers:     nis /' |         \
#                         sed -e                                     \
#'s/bootparams: nis \[NOTFOUND=return\]/bootparams: nis /'           \
#                                             > $etc/nsswitch.conf.tmp
#cp $etc/nsswitch.conf.tmp $etc/nsswitch.conf
#rm $etc/nsswitch.conf.tmp
#chmod -w $etc/nsswitch.conf

#-----------------------------------------------------------------
# create /tftpboot if needed
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
cp -p $vnmrsystem/rc.vnmr $etc/init.d
(cd $etc/rc5.d; if [ ! -h S99rc.vnmr ]; then 
    ln -s ../init.d/rc.vnmr S99rc.vnmr; fi)
(cd $etc/rc0.d; if [ ! -h K99rc.vnmr ]; then
    ln -s ../init.d/rc.vnmr K99rc.vnmr; fi)
if [ -f /etc/acqpresent ]
then
    rm -f /etc/acqpresent
fi

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
# Remove some files (Queues) NOT IPC_V_SEM_DBM
# Because cleanliness is next to ... you know.
#-----------------------------------------------------------------
rm -f /tmp/ExpQs
rm -f /tmp/ExpActiveQ
rm -f /tmp/ExpStatus
rm -f /tmp/msgQKeyDbm
rm -f /tmp/ProcQs
rm -f /tmp/ActiveQ

/usr/sbin/rarpd -e

PATH=/vnmr/bin:$PATH
export PATH
chmod 775 /tftpboot
rm -f /tftpboot/ddrexec.o /tftpboot/gradientexec.o /tftpboot/lockexec.o /tftpboot/masterexec.o
rm -f /tftpboot/pfgexec.o /tftpboot/rfexec.o /tftpboot/nvlib.o /tftpboot/vxWorks405gpr.bdx

sleep 10

rm -f /tftpboot/nvScript /vnmr/acq/download/load*
echo " "
echo "    ${esc}47;31mThe download step may take four to eight minutes ${esc}0m"
echo "    ${esc}47;31mDo not reboot the console during this process    ${esc}0m"
echo " "

#ping master1, if no answr ask to check and rerun $0
ping -c1 -q master1 > /dev/null
if [ $? -ne 0 ] 
then
     echo ""
     echo Please check that the console and host are connected
     echo Then rerun $0
     echo ""
     exit 0
fi

echo "Testing console software version, will try up to 10 seconds"
#   $vnmrsystem/acqbin/testconf42x
#   query the master for ndds version
$vnmrsystem/acqbin/testconf42x -querynddsver
if [ $? -eq 0 ] 
then    
   downloadcmd="$vnmrsystem/acqbin/consoledownload42x"
else    
   ${vnmrsystem}/acqbin/testconf3x
   if [ $? -eq 1 ]
   then
      ${vnmrsystem}/acqbin/consoledownload
   else
      ${vnmrsystem}/acqbin/consoledownload3x
   fi
fi      
    
echo ${downloadcmd}

${downloadcmd} 
sleep 25 

nofile=1
count=0
if [ -f $vnmrsystem/acq/download/loadhistory ]
then
   owner=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }'`
   group=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }'`
   chown $owner:$group $vnmrsystem/acq/download/load* 2> /dev/null
   count=`grep successful $vnmrsystem/acq/download/loadhistory | wc -l`
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

echo ""

#-----------------------------------------------------------------
# Do we need to reboot the SUN? If not, restart bootpd
#-----------------------------------------------------------------
if [ $reboot -eq 1 ]
then
   echo " "
   echo "You must reboot Linux for these changes to take effect"
   echo "As root type 'reboot' to reboot Linux"
else
   /vnmr/bin/execkillacqproc
fi


# copy *exec.o and nvlib.o

# no Sun reboot required.

# /vnmr/acqbin needs new nvexpproc, nvsendproc, nvrecvproc
# /vnmr/bin    needs new nvVnmrbg
# /vnmr/bin    needs new nddsManager, but for now mount /sw and define
#              NDDS_HOME 

# still may need to deal with shmsys and semsys in /etc/system
#
# do we really need to change rarp to work on driver2 only?
# if so, create a file list with 
# fileList=`grep "in.raprd" $etc/rc?.d/*`
