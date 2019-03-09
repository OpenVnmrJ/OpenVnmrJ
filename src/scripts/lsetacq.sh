#!/bin/sh
: 'lsetacq.sh 2003-2010 '
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
# lsetacq.sh
#
# Linux ONLY

vnmrsystem=/vnmr
cons_macaddr="080023456789"
etc=/etc
export vnmrsystem cons_macaddr etc

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

common_check () {

   findacqproc="ps -e  | grep Vnmr | awk '{ printf(\"%d \",\$1) }'"
   npids=`eval $findacqproc`
   if (test x"$npids" != "x" )
   then
      echo ""
      echo "You must exit all 'Vnmr'-s to run $0"
      echo "Please type 'exit' in the Vnmr input window,"
      echo "or use 'ps -e | grep Vnmr' and 'kill -3 pid' to exit Vnmr-s."
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
       echo "To run $0 you will need to be the system's root user,"
       echo "or type cntrl-C to exit."
       echo
       s=1
       t=3
       while [ $s = 1 -a ! $t = 0 ]; do
          echo "Please enter this system's root user password"
          echo
          su root -c "$0 ${ARGS}";
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
}

# make backup copy of bootpd, but only if cksum is different
backup_bootpd()
{
  vnmr_bootpd="${vnmrsystem}/acqbin/bootpd.51"
  system_bootpd="/usr/sbin/bootpd"

  if [ -f $system_bootpd ]
  then
    do_cp="n"
    v1sum=`cksum $vnmr_bootpd | awk '{printf $1}'`
    v2sum=`cksum $vnmr_bootpd | awk '{printf $2}'`
    s1sum=`cksum $system_bootpd | awk '{printf $1}'`
    s2sum=`cksum $system_bootpd | awk '{printf $2}'`
    if ! [ x"$v1sum" = x"$s1sum" -a x"$v2sum" = x"$s2sum" ]
    then
      do_cp="y"
      files=`ls /usr/sbin/bootpd*`
      for file in $files
      do
        f1sum=`cksum $file | awk '{printf $1}'`
        f2sum=`cksum $file | awk '{printf $2}'`
        if [ x"$f1sum" = x"$s1sum" -a  x"$f2sum" = x"$s2sum" ]
        then
          if [ x"$file" != x"$system_bootpd" ]
          then
            do_cp="n"
          fi
        fi
      done
    fi

    if [ x$do_cp = "xy" ]
    then
      date=`date +%y%m%d.%H:%M`
      cp -p $system_bootpd $system_bootpd.bkup.$date
    fi
  fi
}


disableSelinux() {

 if [ x$distroType  != "xdebian" ] ; then
    #   SELINUX=disabled   (possible values: enforcing, permissive, disabled)
   selinux=`grep SELINUX /etc/selinux/config | grep -i disabled`
   # echo "str: $str"
   if [ -z "$selinux" ] ; then
       echo ""
       echo ""
       echo "Disabling SELinux, System Reboot Required."
       echo "You must reboot the system prior to continuing setacq."
       echo ""
       echo ""
       # replace the two possibilites enforcing or permissive, to be disabled
       cat /etc/selinux/config | sed s/SELINUX=[eE][nN][fF][oO][rR][cC][iI][nN][gG]/SELINUX=disabled/ | sed s/SELINUX=[pP][eE][rR][mM][iI][sS][sS][iI][vV][eE]/SELINUX=disabled/ > config.mod
       cp config.mod /etc/selinux/config
       rm -f config.mod
       # SELinux reboot flag file, to break out of the enter root password 
       # loop in main.
       touch /tmp/reboot
       reboot=1
       exit 1
   fi
  fi
}



disablefirewall() {

   echo "Disabling firewall. Interface $consolePort to the console must be trusted."
   if [ x$distroType  != "xdebian" ] ; then
     ipmod=`/sbin/lsmod | grep _tables`
     # if ip_tables loaded into kernel then we should just turn it off
     # even though it might already be off.
     if [ ! -z "$ipmod" ]; then
        /sbin/service iptables save  > /dev/null  2>&1
        /sbin/service iptables stop  > /dev/null  2>&1
        /sbin/chkconfig iptables off  > /dev/null  2>&1
        # ipv6 may not be active but just turn it off  to be sure
        /sbin/service ip6tables save  > /dev/null  2>&1
        /sbin/service ip6tables stop  > /dev/null  2>&1
        /sbin/chkconfig ip6tables off  > /dev/null  2>&1
     fi
   fi
   rm -f /etc/sysconfig/iptables
   rm -f /etc/sysconfig/ip6tables
}

checkfirewall() {
 if [ -f /etc/sysconfig/iptables ] ; then
    num=`grep $consolePort /etc/sysconfig/iptables | grep -c ACCEPT`
    if [ $num -lt 2 ] ; then
       disablefirewall
    fi
 fi
}

#
# rmQuotes removes the surrounding double quotes
# e.g. "172.16.0.1" -> 172.16.0.1
rmQuotes() {
    noquotes=$(echo "$1" | tr -d '"')
}

getEthernetInfo() {
  nic_cnt=`/sbin/lspci | grep -i ethernet | wc -l`
  # debain /etc/network file interfaces ,  iface eth0  (line)
  # rhel  /etc/sysconfig/networking/devices  files ifcfg-eth0 ifcfg-eth1
    if [ -d $networkdir ] ; then
     ifs=`ls ${networkdir} | grep ifcfg-eth`
  fi
  if [ -z "$ifs" ]; then
     networkdir=$netDir
     if [ -d $netDir ] ; then
         ifs=`ls ${netDir}/ifcfg-eth*`
     fi
  else
     ifs=`ls ${networkdir}/ifcfg-eth*`
  fi

  for iffile in $ifs ; do
      DEVSTR=`awk '/^DEVICE=/ { print substr($0, 8) }' $iffile`
      IPSTR=`awk '/^IPADDR=/ { print substr($0, 8) }' $iffile`
      rmQuotes $IPSTR
      IPSTR=$noquotes

      if [ x$IPSTR != "x" ]
      then
           # create the varable eth0_IP or eth1_IP and set its value to the IP,
           # replacing potential ":" in virtual network interface with "_"
           # ------------------------------------------------------------------
           # ------------------------------------------------------------------
           # eval is busted for RHEL 6.1 don't have time to fix,just use brute force  7/18/2011 GMB
           # ------------------------------------------------------------------
           # ------------------------------------------------------------------
           #eval ${DEVSTR}_IP=\"$IPSTR\"
           IPBASE=`echo "$IPSTR" | awk  '{ split($0, fields, ".") 
                  printf("%s.%s.%s", fields[1],fields[2],fields[3]) }'`
           # echo "IPBASE: $IPBASE"
           #eval ${DEVSTR}_IPBASE=\"$IPBASE\"
           #eval echo "\$${DEVSTR}_IP"
           #eval echo "\$${DEVSTR}_IPBASE"
           if [ $DEVSTR = "eth0" -o $DEVSTR = "\"eth0\"" ] ; then
               eth0_IP=$IPSTR
               eth0_IPBASE=$IPBASE
           elif [ $DEVSTR = "eth1" -o $DEVSTR = "\"eth1\"" ] ; then
               eth1_IP=$IPSTR
               eth1_IPBASE=$IPBASE
           else
               echo "Not eth0 nor eth1"
           fi
     fi
  done
  # echo "eth0 IP: $eth0_IP"
  # echo "eth1 IP: $eth1_IP"
}

chkNICSetup() {

#
# set consoleIP and IPBASE for console based on select console port  (eth0, eth1)
#
eval 'consoleIP'=\"\$${consolePort}_IP\"
eval 'consoleIPBase'=\"\$${consolePort}_IPBASE\"

# base_IP is used in the generating the console IPs
base_IP="$consoleIPBase"

# echo $consoleIP
# echo $consoleIPBase

#garg="^$consolePort"
# nic_exist=`/sbin/ifconfig -a | grep -c $garg`

#
# does the NIC exist ?
#
nic_exist=`ls ${networkdir} | grep $consolePort | wc -l`
if [ $nic_exist -eq 0 ]
then
    echo
    echo "  No ethernet port '$consolePort' found."
    echo "  Abort! "
    exit 0
fi

#
# which ever NIC chosen, be sure it has been configured.
#

# if consoleIP is empty then it has not been configured
if [  "x" = "x$consoleIP" ] ; then 
   echo ""
   echo " \"$consolePort\" has not been configured,  Please configure \"$consolePort\" "
   echo ' Menu: System->Administration->Network (RHEL: 5.X)'
   echo ' reboot system after configuring network.'
   echo ""
   exit 0
# if consoleIP is neither 172.16.0.1 or 10.0.0.1 then the port is not configured properly
# for the console
elif [ x$consoleIP != "x172.16.0.1" -a x$consoleIP != "x10.0.0.1" ] ; then
   echo ""
   echo " \"$consolePort\" has been configured incorrectly for the console."
   echo " IP: $consoleIP , should be either 172.16.0.1 or 10.0.0.1 "
   echo " Please re-configure \"$consolePort\" "
   echo ' Menu: System->Administration->Network (RHEL: 5.X)'
   echo ' reboot system after re-configuring the network.'
   echo ""
   exit 0
else  # test that port is active
   ipaddr=`/sbin/ifconfig -a $consolePort | grep "$consoleIP"`
   if [ -z "$ipaddr" ] ; then
      echo ""
      echo " \"$consolePort\" is not Active."
      echo " Please Activate \"$consolePort\" "
      echo ' Menu: System->Administration->Network (RHEL: 5.X)'
      echo ' A system reboot maybe necessary after Activating \"$consolePort\" .'
      echo ""
      exit 0
   fi
fi

echo " "
echo " "

#
# output some useful info on the network setup
#
if [ $nic_cnt -gt 1 ]
then
    if [ x$consolePort = "xeth0" ]
     then
           etherPort="eth1"
           main_IP="$eth1_IP"
           main_IPBASE="$eth1_IPBASE"
     else
           etherPort="eth0"
           main_IP="$eth0_IP"
           main_IPBASE="$eth0_IPBASE"
    fi
    echo "The Network Port to Main Net = $etherPort, subnet = $main_IPBASE, IP: $main_IP "
fi
echo "The Network Port to Console = $consolePort, subnet = $consoleIPBase, IP = $consoleIP "

#
#  going to blink the LEDs of the console NIC
#
echo " "
echo " "
echo "Console Port: \"$consolePort\" "
echo "For confirmation of correct console network hookup, the NIC LEDs associated with \"$consolePort\" "
echo "will begin to blink in a distinctive manner for 20 seconds. Look at the back of the computer "
echo "and confirm proper hook up the the ethernet cable from the console."
echo "Note: This test does not interfere with the normal operation of the NIC"
echo ""
echo "Do you wish to perform this check (y or n) [n]: "
read ans
if [ "x$ans" = "xy" -o "x$ans" = "xY" ] ; then
   echo "Blinking \"$consolePort\" LEDs."
   /usr/sbin/ethtool -p $consolePort 20
   echo "Blink test done. Do you wish to continue with setacq (y or n) [y]: "
   read ans
   if [ "x$ans" = "xn" -o "x$ans" = "xN" ] ; then
      echo "Exit setacq"
      exit 0
   fi
else
   echo "Skipping Check."
fi
echo ""

}


#
# Main main MAIN
#
                                 
OS_NAME=`uname -s`               
if [ ! x$OS_NAME = "xLinux" ]   
then                             
   echo "$0 suitable for Linux-based systems only"
   echo "$0 exits"               
   exit 0                        
fi                               

system_processor_type=`uname -p`
if [ z"$system_processor_type" != "zx86_64" ]
then
  echo "ERROR: system processor type is incompatible with acquisition software."
  echo "Cannot run $0"
  exit
fi

#---------------------------------------------------------------
# initialize a few things 
#
host_name=`uname -n`
cons_name="inova"
cons_name_auto="inovaauto"
cons_subnet="172.16.0"
reboot=0
Null="/dev/null"
tmpfile=/tmp/tmp_please_remove
netDir="/etc/sysconfig/network-scripts"
networkdir="/etc/sysconfig/networking/devices/"
consolePort="eth0"

export cons_name cons_name_auto cons_subnet reboot
export Null tmpfile consolePort

# become root, etc...
common_check

#----------------------------------------------------------------
# remove reboot flag for SELinux disabling
rm -f /tmp/reboot


#----------------------------------------------------------------
#
#  First make sure SELinux is OFF.
#

# if SELinux was enabled, then disable, but a system reboot is required.
# Stop installation.
disableSelinux

#-----------------------------------------------------------------
# Get Info on the NIC configurations 
#
getEthernetInfo

#-----------------------------------------------------------------
# one, two or no ethernet cards??
#-----------------------------------------------------------------
if [ $nic_cnt -eq 1 ]
then
       echo
       echo "  This is a single Network Interface Card system"
       echo "  Make sure the NMR console is attached to the "
       echo "  host's only ethernet port (eth0) using a null cable"
       echo
       if [ "x" = "x$eth0_IP" ] ; then
          echo ""
          echo " eth0 has not been configured,  Please configure eth0"
          echo ' Menu: System->Administration->Network (RHEL: 5.X)'
          echo ' reboot system after configuring network.'
          echo ""
          exit 0
      fi
elif [ $nic_cnt -eq 2 -o $nic_cnt -gt 2 ]
then
    consolePort="eth1"
else
    echo
    echo "  No ethernet port found."
    echo
    exit 0
fi

#-----------------------------------------------------------------
# If there is an argument (eth0 or eth1) then use that port 
# to the console, check if it really exists.
#
if [ $# -gt 0 ]
then
    consolePort=$1
fi

chkNICSetup

# check firewall
checkfirewall

#-----------------------------------------------------------------
if [ -f $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto ]
then
    #Ask if the mts version of MSR is needed
    rm -f $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
    cp $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
fi

#----------------------------------------------------------------
echo 
echo "One moment please..."

#-----------------------------------------------------------------
# kill bootpd, otherwhise otherwhise the console will reboot
# with the old files (can't delete /tftpboot/vxWorks!)
#-----------------------------------------------------------------
stop_program bootpd


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

# obtain the MAC address of the console NIC
echo " "
echo "Please reboot the console."
echo "Press the Enter key after pressing the console reset button: "
read ans
echo 
echo "One moment please..."
echo 
cons_macaddr=`/usr/sbin/tcpdump -i $consolePort host 255.255.255.255 -t -n  -X -c 1 2>$Null |\
              grep 0x0030 | awk '{print $6 $7 $8}'`

#-----------------------------------------------------------------
# add to /etc/hosts if needed, /etc/hosts should always exist
#-----------------------------------------------------------------
etc_hosts="/etc/hosts"
grep -w $host_name $etc_hosts 2>&1 > $Null
if [ $? -ne 0 ]
then
    chmod +w $etc_hosts
    echo "${base_IP}.11    $host_name loghost" >> $etc_hosts
    reboot=1
    chmod -w $etc_hosts
fi

aa=`grep -w "inova" $etc_hosts | awk '{print $1}'`
if [ x$aa != "x${base_IP}.2" ]
then
    chmod +w $etc_hosts
    grep -vw inova $etc_hosts > /tmp/hosts
    mv /tmp/hosts $etc_hosts

    echo "${base_IP}.2    $cons_name" >> $etc_hosts
    reboot=1
    chmod -w $etc_hosts
fi

bb=`grep -w "inovaauto" $etc_hosts | awk '{print $1}'`
if [ x$bb != "x${base_IP}.4" ]
then
    chmod +w $etc_hosts
    grep -vw inovaauto $etc_hosts > /tmp/hosts
    mv /tmp/hosts $etc_hosts

    echo "${base_IP}.4    $cons_name_auto" >> $etc_hosts
    reboot=1
    chmod -w $etc_hosts
fi

# Insert entries for all known peripheral devices.
# List of subdomain addresses for Inova and hostnames.
peripheral_list="43,V-Slim\
                 51,V-Cryobay\
                 70,V-Protune\
                 71,V-Trap\
                 101,V-AS768\
                 100,V-AS768Robot"
echo "cons_subnet $cons_subnet"
for hostline in $peripheral_list
do
    newip=${cons_subnet}.`echo $hostline | awk -F, '{print $1}'`
    host=`echo $hostline | awk -F, '{print $2}'`
    oldip=`grep -wi $host $etc_hosts | grep -v "#" | awk '{print $1}'`
    if [ x$oldip = "x" ]
    then
        chmod +w $etc_hosts
        grep -wiv $host $etc_hosts > /tmp/hosts
        echo "$newip	$host" >> /tmp/hosts
        mv /tmp/hosts $etc_hosts
        chmod -w $etc_hosts
        # NB: No need to reboot for these
    fi
done

# update the profile default, otherwise if a user uses the UI network config
# tools, and saves changes the hosts file gets trashed. 
# i.e. console IP & names  are lost
# RHEL 5.X location
profileHosts="/etc/sysconfig/networking/profiles/default/hosts"
if [ -f $profileHosts ]
then
  mv $profileHosts $profileHosts.orig
  cp -p $etc_hosts $profileHosts
fi

#-----------------------------------------------------------------
# add the hosts.equiv to give vxWorks acces to files
# hosts.equiv additions do not need a reboot
#-----------------------------------------------------------------
if [ ! -f $etc/hosts.equiv ]
then
   echo $cons_name > $etc/hosts.equiv
   echo $cons_name_auto >> $etc/hosts.equiv
else
   grep -w $cons_name $etc/hosts.equiv > $Null
   if [ $? -ne 0 ]
   then
      echo $cons_name >> $etc/hosts.equiv
   fi
   grep -w $cons_name_auto $etc/hosts.equiv > $Null
   if [ $? -ne 0 ]
   then
      echo $cons_name_auto >> $etc/hosts.equiv
   fi
fi

#-----------------------------------------------------------------
# set write permission on serial port
#-----------------------------------------------------------------
if [ -c /dev/ttyS0 ]
then
    chmod 666 /dev/ttyS0
fi

#-----------------------------------------------------------------
# fix /etc/xinetd.d/tftp  so tftpd gets started at boot time
#-----------------------------------------------------------------
tftpconf="/etc/xinetd.d/tftp"
if [ -r /etc/xinetd.d/tftp ]
then
   grep -w disable /etc/xinetd.d/tftp | grep -w no 2>&1 > /dev/null
   if [ $? -ne 0 ]
   then
       chmod +w $tftpconf
       cat $tftpconf | sed '/disable/s/= yes/= no/' > $tmpfile
       mv $tmpfile $tftpconf
       chmod -w $tftpconf
       reboot=1
   fi
else
   echo "tftp package is not available. Exit"
   exit
fi

#-----------------------------------------------------------------
# copy bootptab and substitute the received ethernet address
# Done no matter what, in case CPU (=ethernet address) was changed
#-----------------------------------------------------------------

backup_bootpd

# see below.....
# RHEL 5.3 typically has bootpd already installed, 
# but may dependent if user installed RHEL 5.3 or DELL's pre-installed 
# so test if we need to copy it, the bootpd.51 is OK on 5.3
#
# Unfortanetly the 2.2D installation copies over any pre-existing 
# bootpd with a RHEL 4u3 version
# Thus we MUST copy the bootpd.51
#
#if [ ! -x /usr/sbin/bootpd ] ; then

cp "${vnmrsystem}/acqbin/bootpd.51" /usr/sbin/bootpd

#fi


vnmr_bootptab="${vnmrsystem}/acq/bootptab.51"
etc_bootptab="/etc/bootptab"

cp $vnmr_bootptab $etc_bootptab
cat $etc_bootptab | sed -e "s/08003E236BC4/${cons_macaddr}/" > $tmpfile
mv $tmpfile $etc_bootptab
cat $etc_bootptab | sed -e 's/10.0.0./'${base_IP}'./' > $tmpfile
mv $tmpfile $etc_bootptab

#-----------------------------------------------------------------
# create /tftpboot if needed, copy vxBoot from /vnmr/acq
#-----------------------------------------------------------------
tftpboot=/tftpboot
if [ -d /var/lib/tftpboot ]
then
  tftpboot=/var/lib/tftpboot
fi
if [ ! -d $tftpboot/vxBoot ]
then
   mkdir -p $tftpboot/vxBoot
   reboot=1
fi
if [ ! -h $tftpboot/tftpboot ]
then
   cd $tftpboot
   ln -s . tftpboot
fi
if [ $tftpboot = "/var/lib/tftpboot" ]
then
   if [ ! -h $tftpboot/var ]
   then
      cd $tftpboot
      ln -s . var
   fi
   if [ ! -h $tftpboot/lib ]
   then
      cd $tftpboot
      ln -s . lib
   fi
fi
rm -f $tftpboot/vxBoot/*
cp -p $vnmrsystem/acq/vxBoot/vxWorks*           $tftpboot/vxBoot
grep -q -E 'mercvx|mercplus' /vnmr/vnmrrev
if [ $? -ne 0 ]
then
   cp -p $vnmrsystem/acq/vxBootPPC/vxWorks         $tftpboot/vxBoot/vxWorksPPC
   if [ -f $vnmrsystem/acq/vxBootPPC/vxWorks.sym ]
   then
      cp -p $vnmrsystem/acq/vxBootPPC/vxWorks.sym     $tftpboot/vxBoot/vxWorksPPC.sym
   fi
   cp -p $vnmrsystem/acq/vxBoot.auto/vxWorks.auto $tftpboot/vxBoot
fi

#-----------------------------------------------------------------
# Arrange for procs to start at system bootup
#-----------------------------------------------------------------
cp -p $vnmrsystem/acqbin/rc.vnmr $etc/init.d
chmod +x $etc/init.d/rc.vnmr
(cd $etc/rc5.d; if [ ! -h S99rc.vnmr ]; then 
    ln -s ../init.d/rc.vnmr S99rc.vnmr; fi)
(cd $etc/rc0.d; if [ ! -h K99rc.vnmr ]; then
    ln -s ../init.d/rc.vnmr K99rc.vnmr; fi)
touch $vnmrsystem/acqbin/acqpresent

# The program prevents bootpd from working
if [ -f /usr/sbin/dnsmasq ]
then
  mv -f /usr/sbin/dnsmasq /usr/sbin/dnsmasq_off
  reboot=1
fi
#-----------------------------------------------------------------
# Also, create /etc/norouter, so that no matter how the install
# is done (e.g. incomplete nets) the system does not become a router
#-----------------------------------------------------------------
if [ ! -f /etc/notrouter ]
then
   touch $etc/notrouter
fi

#-----------------------------------------------------------------
# Remove some files (Queues) NOT IPC_V_SEM_DBM
#-----------------------------------------------------------------
rm -f /tmp/ExpQs
rm -f /tmp/ExpActiveQ
rm -f /tmp/ExpStatus
rm -f /tmp/msgQKeyDbm
rm -f /tmp/ProcQs
rm -f /tmp/ActiveQ

echo ""
echo "NMR Console software installation complete"

#-----------------------------------------------------------------
# Do we need to reboot the PC? If not, restart bootpd
#-----------------------------------------------------------------
if [ $reboot -eq 1 ]
then
   echo " "
   echo "You must reboot Linux for these changes to take effect"
   echo "As root type 'reboot' to reboot Linux"
else
   if [ -x /usr/sbin/bootpd ]
   then
      (/usr/sbin/bootpd -s > /dev/console &)
   else
      echo "bootpd does not exist, be sure to install bootpd package"
      echo "then run \'/usr/sbin/bootpd -s > /dev/console &\' manually"
   fi
   /vnmr/bin/execkillacqproc
fi


exit

#ifcfg-eth1.orig
# Intel Corp.|82540EM Gigabit Ethernet Controller
##DEVICE=eth1
##BOOTPROTO=dhcp
##HWADDR=00:0D:56:98:4A:EE
##ONBOOT=no
##TYPE=Ethernet

#ifcfg-eth1.mod
#DEVICE=eth1
#BOOTPROTO=static
#BROADCAST=172.16.0.255
#HWADDR=00:0D:56:98:4A:EE
#IPADDR=171.16.0.1
#NETMASK=255.255.255.0
#NETWORK=172.16.0.0
#ONBOOT=yes
#TYPE=Ethernet


#ifcfg-eth0.orig
# Linksys|Network Everywhere Fast Ethernet 10/100 model NC100
##DEVICE=eth0
##BOOTPROTO=static
##BROADCAST=10.190.50.255
##HWADDR=00:04:5A:64:85:D6
##IPADDR=10.190.50.83
##NETMASK=255.255.255.0
##NETWORK=10.190.50.0
##ONBOOT=yes
##TYPE=Ethernet


#might have to more GATEWAY and NISDOMAIN from /etc/sysconfig/network
#to /etc/sysconfig/network-scripts/ifcfg-eth0
