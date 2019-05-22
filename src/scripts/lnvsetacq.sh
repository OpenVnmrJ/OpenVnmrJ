#!/bin/sh
: 'lnvsetacq.sh 2003-2009 '
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
#lnvsetacq.sh

# set -x

etc="/etc"
vnmrsystem="/vnmr"
tmpDir="/tmp"
netDir="/etc/sysconfig/network-scripts"
networkdir="/etc/sysconfig/networking/devices/"
consolePort="eth0"

# For reference two common locations for the tftpboot directory
#  1.  /tftpboot
#  2.  /var/lib/fttpboot
tftpdir="/tftpboot"

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
	lpfg1	\
	"

peripheralList="	\
	V-Autosampler\
	V-Cryobay\
	V-AS768\
	V-AS768Robot\
	V-CryogenMonitor\
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
 
   # echo "$main_IP        wormhole" >> $tmpHost
   echo "$consoleIP      wormhole" >> $tmpHost
   
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
         lpfg1 )
           i=46 ;
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
       V-CryogenMonitor )
	 i=241
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

   # update the profile default, otherwise if a user uses the UI network config
   # tools, and saves changes the hosts file gets trashed. 
   # i.e. console IP & names  are lost
   # RHEL 5.X location
   profileHosts="/etc/sysconfig/networking/profiles/default/hosts"
   if [ -f $profileHosts ] 
   then
     mv $profileHosts $profileHosts.orig
     cp -p $etc/hosts $profileHosts
   fi
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
   for cntlr in $cntlrList
   do
      a=`echo $prev_cntlr | cut -c 1,1 -`
      b=`echo $cntlr | cut -c 1,1 -`
      if [ x$a = x$b ]
      then
         j=`expr $j + 1`
      else
         j=1
         i=`expr $i + 10`
      fi
      grep "$cntlr" $etc/ethers > /dev/null #all or none, the addresses are hardcoded
      if [ $? -ne 0 ]
      then
         echo "$base_ethers:$i:$j        $cntlr" >> $etc/ethers
      fi
      prev_cntlr=$cntlr
   done
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
    if (test x$1 = "nddsManager" )
    then
      sleep 10
    fi
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
# add: auth    required        pam_permit.so
# comment out the line containing pam_rhosts_auth.so, 
#     pam_unix_acct.so ,pam_unix_session.so
# otherwise the hosts.equiv does not work with VxWorks rsh
#
modEtcPamdRsh() {
  pamlist="pam_rhosts_auth.so pam_unix_acct.so pam_unix_session.so"
  if [ -r /etc/pam.d/rsh ] 
  then
     rshline=`grep pam_permit.so /etc/pam.d/rsh`
     # if not there then add line to file
     if [ -z "$rshline" ] 
     then
      sudo /bin/ed /etc/pam.d/rsh  << THEEND
/pam_env.so/
a
auth    required        pam_permit.so
.
w
q
THEEND
     fi

     # check for each lib to be commented out
     for rmlibs in $pamlist ; do
         rshline=`grep $rmlibs /etc/pam.d/rsh` 
         firstfield=`echo $rshline | awk '{print $1}'`
         # if not already commented then comment line out
         if [ x$firstfield != "x#" ]; then
           sudo /bin/ed /etc/pam.d/rsh  << THEEND
,s/$rshline/# $rshline/g
w
q
THEEND
          fi
     done
  fi
}


# future idea to implement
# check for setsebool and getsebool rather than debian
# use these programs rather than checking/modifing files directly.
#  /usr/sbin/getsebool -a
# modify settings via setsebool
# /usr/sbin/setsebool SELINUX false ??
#
disableSelinux() {

#  if [ -x /usr/sbin/setsebool ] ; then
 if [ -f /etc/selinux/config ] ; then
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
       exit 1
   fi
  fi
}

disablefirewall() {

   echo "Disabling firewall. Interface $consolePort to the console must be trusted."
   if [ -x /sbin/service ] ; then
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
    num=`grep $consolePort /etc/sysconfig/iptables | grep INPUT | grep -c ACCEPT`
    if [ $num -lt 1 ] ; then
       disablefirewall
    fi
 fi
}

# For reference two common locations for the tftpboot directory
#  1.  /tftpboot
#  2.  /var/lib/fttpboot
getTFTPBootDir() {
   if [ -f /etc/xinetd.d/tftp ]; then
      # skip line with # eg comments
      tftpdir=`grep  -v \# /etc/xinetd.d/tftp | grep server_args | awk '{print $4 }'`
      # besure the path make sense i.e. has tftpboot in the string
      if [ "$(echo "$tftpdir" | grep 'tftpboot' > /dev/null;echo $?)" != "0" ]; then
         tftpdir="/tftpboot"
      fi
   else
      tftpdir="/tftpboot"
   fi
   # lets be sure the directory is present
   if [ ! -d "$tftpdir" ] ; then
       echo "tftpboot directory:  $tftpdir is not present "
       echo "Creating directory: $tftpboot"
       mkdir "$tftpdir"
   fi
   # echo "TFTP Boot Directory: $tftpdir "
}

rmTFTPBootFiles() {
   rm -f "$tftpdir"/*.o   "$tftpdir"/*.bdx  "$tftpdir"/nvScript
   #rm -f "$tftpdir"/ddrexec.o   "$tftpdir"/lockexec.o  "$tftpdir"/gradientexec.o
   #rm -f "$tftpdir"/pfgexec.o   "$tftpdir"/rfexec.o    "$tftpdir"/masterexec.o
   #rm -f "$tftpdir"/nvlib.o "$tftpdir"/vxWorks405gpr.bdx "$tftpdir"/nddslib.o
   #rm -f "$tftpdir"/nvScript
}


getEthernetInfoIfconf() {
    ethlist=`/sbin/ifconfig -a | awk 'BEGIN { FS = " " } /^eth/ { print $1 }'`
    for ethif in $ethlist; do
      eval ${ethif}_IPBASE=\"\"
      if [ -x /sbin/ifconfig ] ; then
         # line: "inet addr:10.190.213.93  Bcast:10.190.255.255  Mask:255.255.0.0"
         # eth0IP="addr:10.190.213.93"
         ethIp=`/sbin/ifconfig $ethif | awk 'BEGIN { FS = " " } /inet addr:/ { print $2 }'`
         # eth0Ip="10.190.213.93"
         eth_IP=`echo "$ethIp" | awk 'BEGIN { FS = ":" } { print $2 }'`
         eval ${ethif}_IP=\"$eth_IP\"
         if [ ! -z "$eth_IP" ] ; then
            eth_IPBASE=`echo "$eth_IP" | awk  '{ split($0, fields, ".") 
                    printf("%s.%s.%s", fields[1],fields[2],fields[3]) }'`
            eval ${ethif}_IPBASE=\"$eth_IPBASE\"
         fi
      fi
   done
}

#
# rmQuotes removes the surrounding double quotes
# e.g. "172.16.0.1" -> 172.16.0.1
rmQuotes() {
    noquotes=$(echo "$1" | tr -d '"')
}

getEthernetInfo() {
  if [ -x /sbin/lspci ]; then
     nic_cnt=`/sbin/lspci | grep -i ethernet | wc -l`
  elif [ -x /usr/bin/lspci ] ; then
     nic_cnt=`/usr/bin/lspci | grep -i ethernet | wc -l`
  else
     nic_cnt=0
  fi
  # if there are less than 2 NIC card then return, the setacq script will abort
  if [ $nic_cnt -lt 2 ] ; then
     return 0
  fi
  # debain /etc/network file interfaces ,  iface eth0  (line)
  # debian using NetworkManager  /etc/NetworkManager/system-connections  files : 'Auto eht0', 'Auto eht1'
  #   lines following [ipv4]  have addresses, etc.
  # rhel  /etc/sysconfig/networking/devices  files ifcfg-eth0 ifcfg-eth1
  # rhel the above may on occassion not contian the ifcfg-eth script so also check in the network-scripts directory as well
  if [ -d $networkdir ] ; then
     ifs=`ls ${networkdir} | grep ifcfg-eth`
  fi
  if [ -z "$ifs" ]; then
     if [ -d $netDir ] ; then
         ifs=`ls ${netDir}/ifcfg-eth*`
     fi
  else
     ifs=`ls ${networkdir}/ifcfg-eth*`
  fi

  eth0_IP=""
  eth0_IPBAE=""
  eth1_IP=""
  eth1_IPBAE=""
  if [ ! -z "$ifs" ] ; then
     for iffile in $ifs ; do

         # determine if last 4 char corresponds to eth# , to avoid backup files, etc.
         # ending='eth0 or eth1, etc.  for eth10 or above this does not work.
         ending=`echo -n $iffile | tail -c -4`
         case $ending in
           eth[0-9])
             # echo "valid file" ;
             ;;    # do nothing and let it continue on
          *) # echo "invalid file"; 
            continue ;;   # skip this file
         esac ;

         DEVSTR=`awk '/^DEVICE=/ { print substr($0, 8) }' $iffile | sed 's/:/_/'`
         IPSTR=`awk '/^IPADDR=/ { print substr($0, 8) }' $iffile`
         rmQuotes $IPSTR
         IPSTR=$noquotes
         # echo "DEVSTR=$DEVSTR"
         # echo "IPSTR=$IPSTR"
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
  else
     # /etc/sysconfig/networking/.. dir not there then use ifconfig 
     getEthernetInfoIfconf
  fi
   # echo "eth0 IP: $eth0_IP"
   # echo "eth1 IP: $eth1_IP"
}


chkNICSetup() {

#
# does the NIC exist ?
#
#nic_exist=`ls ${networkdir} | grep $consolePort | wc -l`
# if [ $nic_exist -eq 0 ]
# then
# eval ethIP=\$${consolePort}_IP    RHEL 6.1 busted the eval syntax
if [ x$consolePort = "xeth0" ] ; then
    ethIP=$eth0_IP
    consoleIP=$eth0_IP
    consoleIPBase=$eth0_IPBASE
elif [ x$consolePort = "xeth1" ] ; then
    ethIP=$eth1_IP
    consoleIP=$eth1_IP
    consoleIPBase=$eth1_IPBASE
else
    echo "Console Port, not eth0 or eth1"
    exit 0
fi

if [ -z "$ethIP" ]; then
    echo
    echo "  Ethernet port '$consolePort' is either not present or not active."
    echo "  Abort! "
    exit 0
fi

#
# set consoleIP and IPBASE for console based on select console port  (eth0, eth1)

#eval 'consoleIP'=\"\$${consolePort}_IP\"
#eval 'consoleIPBase'=\"\$${consolePort}_IPBASE\"

# base_IP is used in the addToHosts() function above...
base_IP="$consoleIPBase"



#echo $consolePort
#echo $consoleIP
#echo $consoleIPBase

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
else    # determine if port is active
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
echo "and confirm proper connection of the ethernet cable from the console."
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
   echo "Please type 'exit' in the VnmrJ command line, or close VnmrJ by "
   echo "using the 'X' button or the Menu 'File -> Exit VnmrJ'" 
   echo "or execute 'pkill -9 -f vnmrj.jar' and 'pkill -9 -f Vnmr' as root."
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
     if [ -f /etc/debian_version ]; then
        sudo $0 $* ;
     else
        su root -c "$0 $*";
     fi
     s=$?
     t=`expr $t - 1`
     echo " "
     if [ -f /tmp/reboot ] ; then
        exit 1
     fi
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again"
      echo
  fi
  exit 0
fi

#----------------------------------------------------------------
# remove reboot flag for SELinux disabling
rm -f /tmp/reboot

#----------------------------------------------------------------

#
#  First make sure the SELinux is OFF.
#


# if SELinux was enabled, then disable, but a system reboot is required.
# Stop installation.
disableSelinux

#----------------------------------------------------------------
# Assumption here is that the NIC(s) have been setup to be static and
# are up and running.
# Note: using /sbin/ifconfig to bring up the NIC is not enough, since it 
# does not change the /etc/sysconfig/network-scripts/ifcfg-eth[0,1] files
# next bootup the ifconfig changes would be lost. 
#                    GMB 3/27/09
#----------------------------------------------------------------


#-----------------------------------------------------------------
# Get Info on hte NIC configurations 
#
getEthernetInfo

#-----------------------------------------------------------------
#if [ $nic_cnt -eq 1 ]
#then
#       echo
#       echo "  This is a single Network Interface Card system"
#       echo "  Make sure the NMR console is attached to the "
#       echo "  host's only ethernet port (eth0) using a null cable"
#       echo
#       if [ "x" = "x$eth0_IP" ] ; then
#          echo ""
#          echo " eth0 has not been configured,  Please configure eth0"
#          echo ' Menu: System->Administration->Network (RHEL: 5.X)'
#          echo ' reboot system after configuring network.'
#          echo ""
#          exit 0
#      fi
if [ $nic_cnt -lt 2 ] ; then
   echo
   echo "  This is a single Network Interface Card system"
   echo "  This configuration is not supported "
   echo "  The must have at least two Network Interface Cards"
   echo
   exit 0
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

#
#  chk nic setup base on selected nic  consolePort
#

chkNICSetup

# check firewall
checkfirewall

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
# if [ x$arg1 != "xndds4" ]
# then
#   echo " "
#   echo "${esc}48;34mPlease make sure both console and PC are connected.${esc}0m"
#   echo " "
# fi


addToHosts
addToHostsEquiv
addToEthers
addToSysctlConf

#----------------------------------------------------------
## Notes for debian Ubuntu changes for future reference
## inetd is used rather than xinetd, so don't install xinetd
## chkconfig equiv on Debian (Ubuntu) is  update-rc.d
## These control the process started by init, in /etc/init.d
## Temporary stop,  e.g. /etc/init.d/<$SERVICE_NAME> stop
## Permanent
## e.g. RHEL chkconfig $SERVICE_NAME off
##      DEB  update-rc.d -f $SERVICE_NAME remove
##
## if rarpd is installed then no changes from it's default
## configuration is required   (already used -e option)
## if tftpd, rsh-server were install thee should be already enabled in /etc/inetd.conf
## use update-inetd to enable or disable these
## e.g. sudo update-inetd --enable tftp
##      sudo update-inetd --disable tftp
#
#
# check /etc/inetd.conf for line to determine if tftpd is installed
#  'tftp dgram udp wait nobody  /usr/sbin/tcpd  /usr/sbin/in.tftpd /srv/tftp'
# Note the above default line must be changed for proper operation of tftp to:
#  'tftp dgram udp wait nobody  /usr/sbin/tcpd  /usr/sbin/in.tftpd -s /tftpboot'
#  use 'sudo update-inetd --enable tftp' to enable
# check  /etc/inetd.conf for line to determine if rsh server installed
#  'shell  stream  tcp  nowait  root  /usr/sbin/tcpd  /usr/sbin/in.rshd'
#  use 'sudo update-inetd --enable shell' to enable
# check  /etc/inetd.conf for line to determine if time server installed
# #time            stream  tcp     nowait  root    internal'
# use 'sudo update-inetd --comment-chars '#' --enable time'
# use 'sudo kill -s SIGHUP `cat /var/run/inetd.pid` to force inetd to
# reread the inetd.conf file
#----------------------------------------------------------
#
# check if inetd configuration rather than xinetd 
if [ -x /usr/sbin/update-inetd ]; then
   useupdateinetd="y"
else
   useupdateinetd="n"
fi

#-----------------------------------------------------------------
# determine if the utilities chkconfig and service are available
# RHEL
#-----------------------------------------------------------------
if [ -x /sbin/chkconfig -a -x /sbin/service ]; then
   usechkconfig="y"
else
   usechkconfig="n"
fi

#-----------------------------------------------------------------
# enable rsh, tftp, time via inetd (Ubuntu)
#-----------------------------------------------------------------
if [ x$useupdateinetd = "xy" ] ; then
   ## enable rsh server
   sudo update-inetd --enable shell

   ## remove incorrect tftp entry
   sudo update-inetd  --remove tftp
   ## add corrected entry for tftp
   sudo update-inetd --add 'tftp\t\tdgram\tudp\twait\tnobody\t/usr/sbin/tcpd\t/usr/sbin/in.tftpd -s /tftpboot'

   ## enable time for rdate of console
   ## sudo update-inetd --comment-chars '#' --enable time
   # remove any unwanted entry for time, so that add works below
   sudo update-inetd --remove  time 
   # add the time entry into the inetd.conf file
   sudo update-inetd --add  'time\t\tstream\ttcp\tnowait\troot\tinternal'
   # using update-inetd  insures the inetd daemon re-reads the inetd.conf file

   # for Ubuntu we have to modify the /etc/pam.d/rsh file
   # add: auth    required        pam_permit.so
   # comment out the line containing pam_rhosts_auth.so,pam_unix_acct.so ,pam_unix_session.so
   # otherwise the hosts.equiv does not work with VxWorks rsh
   if [ -r /etc/init.d/openbsd-inetd ] ; then
      modEtcPamdRsh
      sudo /etc/init.d/openbsd-inetd reload
   fi

else  # RHEL xinetd config

   #-----------------------------------------------------------------
   # fix /etc/xinetd.d/tftp  so tftpd gets started at boot time
   #-----------------------------------------------------------------

   if [ x$usechkconfig = "xy" ] ; then
      /sbin/chkconfig --list | grep -q tftp ;
      if [ $? -eq 0 ]; then
         # have tftp service start on level 3 (single user) and on level 5 (multiuser)
         /sbin/chkconfig --level 35 tftp on ;
         /sbin/service xinetd restart > /dev/null 2>&1 ;
      fi
   else
      tftpconf="/etc/xinetd.d/tftp"
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
         exit 0
      fi
   fi

   #-----------------------------------------------------------------
   # fix /etc/xinetd.d/time  so time service gets started at boot time
   #  this allows rdate from console to work properly  GMB 
   #-----------------------------------------------------------------
   if [ x$usechkconfig = "xy" ] ; then
      # try the rhel 5.x version of time (time-stream) 1st, 
      # then fall back to RHEL 4.X time (time)
      /sbin/chkconfig --list | grep -q time-stream ;
      if [ $? -eq 0 ]; then
         /sbin/chkconfig --level 35 time-stream on ;
         /sbin/service xinetd restart > /dev/null 2>&1 ;
      else
        /sbin/chkconfig --list | grep -q time ;
        if [ $? -eq 0 ]; then
           /sbin/chkconfig --level 35 time on ;
           /sbin/service xinetd restart > /dev/null 2>&1 ;
        fi
      fi
   else
      if [ -f /etc/xinetd.d/time-stream ]
      then
         timeconf="/etc/xinetd.d/time-stream"
      else
         timeconf="/etc/xinetd.d/time"
      fi 
      if [ -r $timeconf ]
      then
         grep -w disable $timeconf | grep -w no 2>&1 > /dev/null
         if [ $? -ne 0 ]
         then
             chmod +w $timeconf
             cat $timeconf | sed '/disable/s/= yes/= no/' > $tmpfile
             mv $tmpfile $timeconf
             chmod -w $timeconf
             pkill -HUP xinetd
         fi
      else
         echo "${timeconf} file is not available."
      fi
   fi

   #-----------------------------------------------------------------
   # fix /etc/xinetd.d/rsh  so rsh service gets started at boot time
   #-----------------------------------------------------------------
   if [ x$usechkconfig = "xy" ] ; then
      /sbin/chkconfig --list | grep -q rsh ;
      if [ $? -eq 0 ]; then
         /sbin/chkconfig --level 35 rsh on ;
         /sbin/service xinetd restart > /dev/null 2>&1 ;
      fi
   else
     rshconf="/etc/xinetd.d/rsh"
     if [ -r $rshconf ]
     then
        grep -w disable $rshconf | grep -w no 2>&1 > /dev/null
        if [ $? -ne 0 ]
        then
            chmod +w $rshconf
            cat $rshconf | sed '/disable/s/= yes/= no/' > $tmpfile
            mv $tmpfile $rshconf
            chmod -w $rshconf
            pkill -HUP xinetd
        fi
     else
        echo "rsh package is not available."
     fi
   fi
   
fi   # end if of useupdateinetd

#-----------------------------------------------------------------
# setup rarpd so rarpd service gets started at boot time
#-----------------------------------------------------------------
# this didn't work out for rarpd, so back to the original way
# if [ x$usechkconfig = "xy" ] ; then
#   /sbin/chkconfig --list | grep -q rarpd ;
#   if [ $? -eq 0 ]; then
#      /sbin/chkconfig --level 35 rarpd on ;
#      /sbin/service xinetd restart > /dev/null 2>&1 ;
#   fi
# fi

#
#  works equally well for RHEL and Ubuntu rarpd
#
rarpd="/etc/init.d/rarpd"
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
   # is there a S rarpd link already present?
   ls -l /etc/rc5.d/S*a* | grep -q ../init.d/rarpd ;
   if [ $? -ne 0 ]; then
      (cd /etc/rc5.d; if [ ! -f S98rarpd ]; then \
              ln -s ../init.d/rarpd S98rarpd; fi)
   fi
   # is there a K rapd link already present?
   ls -l /etc/rc0.d/K*a* | grep -q ../init.d/rarpd ;
   if [ $? -ne 0 ]; then
      (cd /etc/rc0.d; if [ ! -f K98rarpd ]; then \
           ln -s ../init.d/rarpd K98rarpd; fi)
   fi

else
   echo "rarp package is not available. Exit"
   exit 0
fi

#-----------------------------------------------------------------
# set write permission on serial port
#-----------------------------------------------------------------
if [ -c /dev/ttyS0 ]
then
    chmod 666 /dev/ttyS0
fi

#for Linux this had been done at software installation time
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
# determine location of the tftpboot directory which the tftpd is configured for
# usually /tftpboot or /var/lib/tftpboot
#-----------------------------------------------------------------
getTFTPBootDir

#if [ ! -d /tftpboot ]
#then
#   if [ -L /tftpboot  -o -f /tftpboot ]
#   then
#      echo "/tftpboot exists but is not a directory"
#      echo "This needs to be corrected."
#      echo "Then restart $0"
#      echo ""
#      exit 0
#   fi
#   mkdir -p /tftpboot
#fi

#-----------------------------------------------------------------
# Arrange for Acqproc to start at system bootup
#-----------------------------------------------------------------
rm -f $etc/init.d/rc.vnmr
cp -p $vnmrsystem/acqbin/rc.vnmr $etc/init.d
chmod +x $etc/init.d/rc.vnmr
(cd $etc/rc5.d; if [ ! -h S99rc.vnmr ]; then 
    ln -s ../init.d/rc.vnmr S99rc.vnmr; fi)
(cd $etc/rc0.d; if [ ! -h K99rc.vnmr ]; then
    ln -s ../init.d/rc.vnmr K99rc.vnmr; fi)
touch $vnmrsystem/acqbin/acqpresent

#-----------------------------------------------------------------
# Connection to FTS chiller if present
#-----------------------------------------------------------------
if [ ! -f /etc/udev/rules.d/99-CP210x.rules ]
then
    cp $vnmrsystem/acqbin/99-CP210x.rules /etc/udev/rules.d/99-CP210x.rules
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
NIRVANA_CONSOLE="wormhole"
export NIRVANA_CONSOLE
chmod 775 "$tftpdir"
rmTFTPBootFiles 
rm -f /vnmr/acq/download/load* /vnmr/acq/download3x/load*
nvLen=`cat /vnmr/acq/download/nvScript | wc -c`
nvLenStd=`cat /vnmr/acq/download/nvScript.std | wc -c`
nvLenLs=`cat /vnmr/acq/download/nvScript.ls | wc -c`


echo " "
echo "    ${esc}47;31mThe download step may take four to eight minutes ${esc}0m"
echo "    ${esc}47;31mDo not reboot the console during this process    ${esc}0m"
echo " "

if [ $nvLen -eq $nvLenStd ]
then
echo "    Download files for the VNMRS / DD2 systems"
echo " "
fi

if [ $nvLen -eq $nvLenLs ]
then
echo "    Download files for the ProPulse / 400-MR systems"
echo " "
fi

#ping master1 (once [-c1]), if no answer ask to check and rerun $0
ping -c1 -q master1 > /dev/null
if [ $? -ne 0 ] 
then
     echo ""
     echo Please check that the console and host are connected
     echo Then rerun $0
     echo ""
     exit 0
fi

echo "Testing console software version, will try up to 20 seconds"
#   $vnmrsystem/acqbin/testconf42x
#   query the master for ndds version
   stop_program nddsManager
   $vnmrsystem/acqbin/testconf42x -querynddsver
   if [ $? -eq 1 ] 
   then    
      $vnmrsystem/acqbin/testconf3x
      if [ $? -eq 1 ]
      then
         $vnmrsystem/acqbin/consoledownload3x -dwnld3x
         nofile=1
         count=0
         if [ -f $vnmrsystem/acq/download3x/loadhistory ]
         then
            owner=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }'`
            group=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }'`
            chown $owner:$group $vnmrsystem/acq/download/load* 2> /dev/null
            count=`grep successful $vnmrsystem/acq/download3x/loadhistory | wc -l`
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
         rmTFTPBootFiles 
         rm -f /vnmr/acq/download3x/load*
         echo pausing for 25 seconds
         sleep 25
         $vnmrsystem/acqbin/consoledownload3x -tftpdir "$tftpdir"
      else
         $vnmrsystem/acqbin/consoledownload3x -tftpdir "$tftpdir"
      fi
      stop_program nddsManager
      sleep 25
   else    
      $vnmrsystem/acqbin/consoledownload42x -tftpdir "$tftpdir"
   fi      
    
nofile=1
count=0
if [ -f $vnmrsystem/acq/download/loadhistory ]
then
   owner=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }'`
   group=`ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }'`
   chown $owner:$group $vnmrsystem/acq/download/load* 2> /dev/null
   count=`grep successful $vnmrsystem/acq/download/loadhistory | wc -l`
fi
if [ $count -lt 10 ]
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
# Activate the NMR status web service
#-----------------------------------------------------------------
/vnmr/bin/rsu_bt_setup -noprompt -nonet -q
if [ $? -eq 4 ]; then
  echo ""
  echo "NMR Status Web service is now configured but requires network restart."
  echo "As root type '/sbin/service network restart' to restart networking"
  echo ""
fi

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
