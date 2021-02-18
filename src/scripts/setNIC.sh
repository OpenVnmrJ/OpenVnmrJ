#!/bin/bash
#
# Copyright (C) 2017  Dan Iverson
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#
#Uncomment next line for debugging output or add a -d to the command line
# set -x

SCRIPT=$(basename "$0")
vnmrsystem=/vnmr

: ${OVJ_VECHO="echo"}
: ${OVJ_SHOW=0}
: ${OVJ_VERIFY=0}
: ${OVJ_NIC=0}
: ${OVJ_IP=0}

ovj_usage() {
    cat <<EOF

usage:
    $SCRIPT will allow one to inspect and configure a
    Network Interface Card (NIC) for use with a spectometer.

    $SCRIPT help 
        Display this help information

    $SCRIPT show 
        Show the current NIC configuration

    $SCRIPT verify 
        Test the current NIC configuration for spectrometer usage

    $SCRIPT <-nic NIC name>  <-ip IP address>
        Configure the network for spectrometer usage. If a "-nic" argument
        is given, it will be used as the NIC to the spectrometer. If a "-ip"
        argument is given, it will be used as the IP address for the spectrometer.
        The IP address must be either 172.16.0.1 (preferred) or 10.0.0.1
        At a minimum, either 172 or 10 may be entered.


EOF
    exit 1
}

# process args
while [ $# -gt 0 ]; do
    key="$1"
    case $key in
        help)              ovj_usage;    exit 0   ;;
        show)              OVJ_SHOW="1";          ;;
        verify)            OVJ_VERIFY="1";        ;;
        -nic)              OVJ_NIC="$2"; shift    ;;
        -ip)               OVJ_IP="$2";  shift    ;;
        -d)                set -x;                ;;
        *)
            # unknown option
            echo "unrecognized argument: $key"
            ovj_usage
            ;;
    esac
    shift
done

getEthernetInfo() {
  allNicNames=$(ls /sys/class/net)
  nic_cnt=$(echo "$allNicNames" | wc -l)
  nicNames=''
  totalNics=0
  availableNics=0
  for name in $allNicNames; do
     if [[ "$name" != "lo" ]]; then
        if [[ "$name" != "virbr"* ]]; then
           nicName[$totalNics]=$name
           addr=$(/sbin/ip -f inet address show $name |
                  grep inet | awk '{print $2}'|cut -d/ -f1)
           if [[ -z "$addr" ]]; then
              nicIP[$totalNics]="Not configured"
              nicStatus[$totalNics]="down"
              availableNics=$((availableNics+1))
           else
              nicIP[$totalNics]=$addr
              nicStatus[$totalNics]=$(cat /sys/class/net/${name}/operstate)
           fi
           totalNics=$((totalNics+1))
        fi
     fi 
  done
}

showEthernetInfo() {
   echo " "
   echo "Available Network Interface Cards (NIC)"
   echo "Index   Name         IP             Status"
   for ((i=0;i<$totalNics;i+=1))
   do
      j=$((i+1))
      printf "%d       %-12s %-14s %-6s\n" \
         $j "${nicName[$i]}" "${nicIP[$i]}" "${nicStatus[$i]}"
   done
}

wormholeMessage() {
    cat <<EOF

The hostname is set to $1, which is probably incorrect.
To change the hostname, as root, enter the hostname into
the file /etc/hostname

Often, a Fully Qualified Domain Name (FQDN) is required and created
as an alias of the hostname in the /etc/hosts file. For example,
if your hostname is ursula, one might have a line in /etc/hosts
that reads

   127.0.1.1    ursula.example.com ursula

The file /etc/hostname would just have the single word

   ursula

Do
   man hostname
for more details.

EOF
}

wormholeCountMessage() {
   cat <<EOF

There are $1 occurances of wormhole in the /etc/hosts file.
This is probably not correct. Edit the /etc/hosts file so that
there is only one wormhole entry. It should be

${OVJ_IP}     wormhole

EOF
}

addToNetplan() {
   file=$(ls /etc/netplan/*yaml)
   grep OpenVnmrJ $file > /dev/null
   if [[ $? -eq 0 ]]; then
      sed --in-place '/# OpenVnmrJ Start/,/# OpenVnmrJ End/d' $file
   fi
   cat <<EOF >> ${file}
# OpenVnmrJ Start
  ethernets:
    ${OVJ_NIC}:
      addresses:
      - $OVJ_IP/24
      renderer: networkd
EOF
   if [[ -f /sys/class/net/${OVJ_NIC}/address ]]; then
      hwaddr=$(awk '{ print tolower($0) }' < /sys/class/net/${OVJ_NIC}/address)
      echo "      macaddress: ${hwaddr}" >> ${file}
   fi
   echo "# OpenVnmrJ End" >> ${file}
   netplan apply
}

addToInterfaces() {
   file=/etc/network/interfaces
   grep OpenVnmrJ $file
   if [[ $? -eq 0 ]]; then
      sed --in-place '/# OpenVnmrJ Start/,/# OpenVnmrJ End/d' $file
   fi
   cat <<EOF >> ${file}
# OpenVnmrJ Start
auto ${OVJ_NIC}
iface ${OVJ_NIC} inet static
   address $OVJ_IP
EOF
   if [[ -f /sys/class/net/${OVJ_NIC}/address ]]; then
      hwaddr=$(awk '{ print toupper($0) }' < /sys/class/net/${OVJ_NIC}/address)
      echo "   hwaddress ${hwaddr}" >> ${file}
   fi
   echo "# OpenVnmrJ End" >> ${file}
}

addIfcfgFile() {
   file=/etc/sysconfig/network-scripts/ifcfg-${OVJ_NIC}
#   file=/tmp/ifcfg-${OVJ_NIC}
   cat <<EOF > ${file}
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=none
IPADDR=$OVJ_IP
PREFIX=24
DEFROUTE=no
PEERDNS=no
PEERROUTES=no
IPV4_FAILURE_FATAL=no
IPV6INIT=no
NAME=Spectrometer
ONBOOT=yes
AUTOCONNECT_PRIORITY=-999
NM_CONTROLLED=yes
NO_DHCP_HOSTNAME=yes
ZONE=trusted
EOF
   if [[ -f /sys/class/net/${OVJ_NIC}/address ]]; then
      hwaddr=$(awk '{ print toupper($0) }' < /sys/class/net/${OVJ_NIC}/address)
      echo "HWADDR=${hwaddr}" >> ${file}
   fi
}

updateProfile() {
   # update the profile default, otherwise if a user uses the UI network config
   # tools, and saves changes the hosts file gets trashed.
   # i.e. console IP & names  are lost
   hostFile=/etc/hosts
   profileHosts="/etc/sysconfig/networking/profiles/default/hosts"
   if [[ -f $profileHosts ]] ; then
     mv $profileHosts $profileHosts.orig
     cp -p $hostFile $profileHosts
   fi
}

cntlrList="     \
        master1 \
        rf1     \
        rf2     \
        rf3     \
        rf4     \
        rf5     \
        rf6     \
        rf7     \
        rf8     \
        rf9     \
        rf10    \
        rf11    \
        rf12    \
        rf13    \
        rf14    \
        rf15    \
        rf16    \
        pfg1    \
        grad1   \
        lock1   \
        lpfg1   \
        ddr1    \
        ddr2    \
        ddr3    \
        ddr4    \
        ddr5    \
        ddr6    \
        ddr7    \
        ddr8    \
        ddr9    \
        ddr10   \
        ddr11   \
        ddr12   \
        ddr13   \
        ddr14   \
        ddr15   \
        ddr16   \
        "

peripheralList="        \
        V-CryogenMonitor\
        V-AS768Robot\
        V-Trap\
        V-AS768\
        V-Protune\
        V-Pda\
        V-LcWorkstation\
        V-Slim\
        V-Autosampler\
        V-Cryobay\
        "

addToHosts() {
   hostFile=/etc/hosts

   if [[ ! -f $hostFile.orig ]] ; then
      cp $hostFile $hostFile.orig
   fi
   chmod +w $hostFile
   i=0
   allHosts="wormhole $cntlrList $peripheralList"
   for host in $allHosts
   do

      case $host in
         wormhole )
           i=1 ;
           ;;
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
         *)
           i=$((i+1))
           ;;
      esac

      line=$(grep -w $host $hostFile)
      if [[ -z "$line" ]] ; then
         printf "%-16s  %-16s\n" "${OVJ_IP_BASE}.$i" "$host" >> $hostFile
      else
         grep -w $host $hostFile | grep -w ${OVJ_IP_BASE}.$i > /dev/null
         if [[ $? -ne 0 ]] ; then
            sed -i s/"$line"/"${OVJ_IP_BASE}.$i        $host"/ $hostFile
         fi
      fi
   done

   chmod -w $hostFile
   updateProfile
}

checkHosts() {
   hostFile=/etc/hosts
   ret=0
   i=0
   allHosts="wormhole $cntlrList $peripheralList"
   for host in $allHosts
   do
      case $host in
         wormhole )
           i=1 ;
           ;;
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
         *)
           i=$((i+1))
           ;;
      esac

      line=$(grep -w $host $hostFile)
      if [[ -z "$line" ]] ; then
         echo "$host missing from $hostFile"
         ret=1
      else
         grep -w $host $hostFile | grep -w ${OVJ_IP_BASE}.$i > /dev/null
         if [[ $? -ne 0 ]] ; then
            echo "$host has incorrect IP address in $hostFile"
            echo "It should be ${OVJ_IP_BASE}.$i"
            ret=1
         fi
      fi
   done

   profileHosts="/etc/sysconfig/networking/profiles/default/hosts"
   if [[ -f $profileHosts ]] ; then
     diff=$(diff -w --brief $hostFile $profileHosts)
     if [[ ! -z "$line" ]] ; then
        echo "$hostFile and $profileHosts differ. They should be the same."
        ret=1
     fi
   fi
   return $ret
}

addToHostsMI() {
   hostFile="/etc/hosts"
   if [[ ! -f $hostFile.orig ]] ; then
      cp $hostFile $hostFile.orig
   fi
   chmod +w $hostFile
   hostName=$(uname -n)
   peripheralListMI="   \
      V-Slim\
      V-Cryobay\
      V-Protune\
      V-Trap\
      V-AS768\
      V-AS768Robot"

   allHosts="inova inovaauto ${hostname} $peripheralListMI"
   for host in $allHosts
   do

      case $host in
         inova )
           i=2 ;
           ;;
         inovaauto )
           i=4 ;
           ;;
         V-Slim )
           i=43
           ;;
         V-Cryobay )
           i=51
           ;;
         V-Protune )
           i=70
           ;;
         V-Trap )
           i=71
           ;;
         V-AS768 )
           i=101
           ;;
         V-AS768Robot )
           i=100
           ;;
         *)    #  hostname case
           i=11
           ;;
      esac

      line=$(grep -w $host $hostFile)
      if [[ -z "$line" ]] ; then
         printf "%-16s  %-16s\n" "${OVJ_IP_BASE}.$i" "$host" >> $hostFile
      elif [[ $i -ne 11 ]] ; then
         grep -w $host $hostFile | grep -w ${OVJ_IP_BASE}.$i > /dev/null
         if [[ $? -ne 0 ]] ; then
            sed -i s/"$line"/"${OVJ_IP_BASE}.$i        $host"/ $hostFile
         fi
      fi
   done

   chmod -w $hostFile
   updateProfile
}

checkHostsMI() {
   hostFile="/etc/hosts"
   ret=0
   hostName=$(uname -n)
   peripheralListMI="   \
      V-Slim\
      V-Cryobay\
      V-Protune\
      V-Trap\
      V-AS768\
      V-AS768Robot"

   allHosts="${hostName} inova inovaauto $peripheralListMI"
   for host in $allHosts
   do

      case $host in
         inova )
           i=2 ;
           ;;
         inovaauto )
           i=4 ;
           ;;
         V-Slim )
           i=43
           ;;
         V-Cryobay )
           i=51
           ;;
         V-Protune )
           i=70
           ;;
         V-Trap )
           i=71
           ;;
         V-AS768 )
           i=101
           ;;
         V-AS768Robot )
           i=100
           ;;
         *)    #  hostname case
           i=11
           ;;
      esac

      line=$(grep -w $host $hostFile)
      if [[ -z "$line" ]] ; then
         echo "$host missing from $hostFile"
         ret=1
      elif [[ $i -ne 11 ]] ; then
         grep -w $host $hostFile | grep -w ${OVJ_IP_BASE}.$i > /dev/null
         if [[ $? -ne 0 ]] ; then
            echo "$host has incorrect IP address in $hostFile"
            echo "It should be ${OVJ_IP_BASE}.$i"
            ret=1
         fi
      fi
   done

   profileHosts="/etc/sysconfig/networking/profiles/default/hosts"
   if [[ -f $profileHosts ]] ; then
     diff=$(diff -w --brief $hostFile $profileHosts)
     if [[ ! -z "$line" ]] ; then
        echo "$hostFile and $profileHosts differ. They should be the same."
        ret=1
     fi
   fi
   return $ret
}

addToHostsEquiv()
{
   file=/etc/hosts.equiv
   if [[ ! -f $file ]] ; then
      touch $file
   fi
   for cntlr in $cntlrList
   do
      grep $cntlr $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         echo "$cntlr" >> $file
      fi
   done
}

checkHostsEquiv()
{
   file=/etc/hosts.equiv
   ret=0
   if [[ ! -f $file ]] ; then
      echo "$file does not exist"
      return 1
   fi
   for cntlr in $cntlrList
   do
      grep $cntlr $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         echo "Controller $cntlr does not exist in $file"
         ret=1
      fi
   done
   return $ret
}

addToHostsEquivMI()
{
   file=/etc/hosts.equiv
   cntlrListMI="inova inovaauto"
   if [[ ! -f $file ]] ; then
      touch $file
   fi
   for cntlr in $cntlrListMI
   do
      grep $cntlr $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         echo "$cntlr" >> $file
      fi
   done
}

checkHostsEquivMI()
{
   file=/etc/hosts.equiv
   ret=0
   if [[ ! -f $file ]] ; then
      echo "$file does not exist"
      return 1
   fi
   cntlrListMI="inova inovaauto"
   for cntlr in $cntlrListMI
   do
      grep $cntlr $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         echo "Controller $cntlr does not exist in $file"
         ret=1
      fi
   done
   return $ret
}

addToEthers()
{
   file=/etc/ethers
   if [[ ! -f $file ]] ; then
      touch $file
   fi
   # ether_base is hard coded
   base_ethers="00:60:93:03"
   #initialize
   prev_cntlr="master0"
   #lpfg must come after ddr
   newList=""
   for cntlr in $cntlrList
   do
      if [[ "$cntlr" != "lpfg1" ]] ; then
         newList="$newList $cntlr"
      fi
   done
   newList="$newList lpfg1"
   
   i=0
   j=0
   for cntlr in $newList
   do
      a=$(echo $prev_cntlr | cut -c 1,1 -)
      b=$(echo $cntlr | cut -c 1,1 -)
      if [[ x$a = x$b ]] ; then
         j=$((j+1))
      else
         j=1
         i=$((i+10))
      fi
      grep -w "$cntlr" $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         printf "%-20s   %s\n" "$base_ethers:$i:$j" "$cntlr" >> $file
      fi
      prev_cntlr=$cntlr
   done
}

checkEthers()
{
   file=/etc/ethers
   ret=0
   if [[ ! -f $file ]] ; then
      echo "$file does not exist"
      return 1
   fi
   # ether_base is hard coded
   base_ethers="00:60:93:03"
   #initialize
   prev_cntlr="master0"
   #lpfg must come after ddr
   newList=""
   for cntlr in $cntlrList
   do
      if [[ "$cntlr" != "lpfg1" ]] ; then
         newList="$newList $cntlr"
      fi
   done
   newList="$newList lpfg1"
   
   i=0
   j=0
   for cntlr in $newList
   do
      a=$(echo $prev_cntlr | cut -c 1,1 -)
      b=$(echo $cntlr | cut -c 1,1 -)
      if [[ x$a = x$b ]] ; then
         j=$((j+1))
      else
         j=1
         i=$((i+10))
      fi
      grep -w "$cntlr" $file > /dev/null
      if [[ $? -ne 0 ]] ; then
         echo "Controller $cntlr does not exist in $file"
         ret=1
      else
         grep -w "$cntlr" $file | grep "$base_ethers:$i:$j" > /dev/null
         if [[ $? -ne 0 ]] ; then
            echo "Controller $cntlr has wrong MAC address in $file"
            ret=1
         fi
      fi
      prev_cntlr=$cntlr
   done
}

addTrusted() {
   if [[ -x /usr/bin/firewall-cmd ]] ; then
      file=/etc/sysconfig/network-scripts/ifcfg-${OVJ_NIC}
      if [[ -e $file ]] ; then
         line=$(grep ZONE $file)
         # if not there then add line to file
         if [[ -z "$line" ]] ; then
            echo "ZONE=trusted" >> ${file}
         elif [[ "$line" != *"trusted" ]] ; then
            sed -i s/"$line"/"ZONE=trusted"/ $file
         fi
      else
         zone=$(/usr/bin/firewall-cmd --get-zone-of-interface=${OVJ_NIC})
         if [[ $zone -ne "trusted" ]] ; then
            /usr/bin/firewall-cmd --permanent --zone=trusted --add-interface=${OVJ_NIC}
         fi
      fi
   else
      file=/etc/sysconfig/iptables
      if [[ -f $file ]] ; then
         line=$(grep ${OVJ_NIC} $file | grep INPUT | grep -c ACCEPT)
         if [ -z $line ] ; then
            iptables -A INPUT -i ${OVJ_NIC} -j ACCEPT
            iptables -A OUTPUT -o ${OVJ_NIC} -j ACCEPT
         fi
      fi
      file=/etc/sysconfig/system-config-firewall
      if [[ -f $file ]] ; then
         line=$(grep trust=${OVJ_NIC} $file )
         if [ -z $line ] ; then
            echo "--trust=${OVJ_NIC}" >> ${file}
         fi
      fi
   fi
}

checkTrusted() {
   ret=0
   if [[ -x /usr/bin/firewall-cmd ]] ; then
      file=/etc/sysconfig/network-scripts/ifcfg-${OVJ_NIC}
      if [[ -e $file ]] ; then
         line=$(grep ZONE $file)
         # if not there then add line to file
         if [[ -z "$line" ]] || [[ "$line" != *"trusted" ]] ; then
            echo "ZONE not set to "trusted" in $file"
            ret=1
         fi
      else
         zone=$(/usr/bin/firewall-cmd --get-zone-of-interface=${OVJ_NIC})
         if [[ $zone -ne "trusted" ]] ; then
            echo "ZONE not set to "trusted" according to firewall-cmd"
            ret=1
         fi
      fi
   else
      file=/etc/sysconfig/iptables
      if [[ -f $file ]] ; then
         num=$(grep ${OVJ_NIC} $file | grep INPUT | grep -c ACCEPT)
         if [ $num -lt 1 ] ; then
            echo "Firewall IP tables not set to "trusted""
            ret=1
         fi
      fi
   fi
   return $ret
}

checkWormhole() {
   # Check if hostname is wormhole. This is a common error
   ret=0
   hname=$(hostname | grep wormhole)
   if [[ ! -z "$hname" ]]; then
      wormholeMessage ${hname}
      ret=1
   fi
   # Another common error is multiple occurances of wormhole in /etc/hosts
   hname=$(getent hosts | grep wormhole | wc -l)
   if [[ $hname -gt 1 ]] ; then
      wormholeCountMessage ${hname}
      echo "This is not correct and must be fixed before"
      echo "$0 will work correctly."
      ret=1
   elif [[ $hname -eq 1 ]] ; then
      hostFile="/etc/hosts"
      mas_cnt=$(grep master1 $hostFile | wc -l)
      if [[ $mas_cnt -eq 0 ]] ; then
         echo "wormhole is already present in $hostFile"
         echo "but other console information is not."
         echo "This is not correct and must be fixed before"
         echo "$0 will work correctly."
         ret=1
      fi
   fi
   return $ret
}

saveNICinfo() {
   userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
   if [[ $userId = "uid=0(root)" ]]; then
      echo "${OVJ_NIC}" > ${vnmrsystem}/adm/log/CONSOLE_NIC
      echo "${OVJ_IP_BASE}" > ${vnmrsystem}/adm/log/CONSOLE_IP
      owner=$(ls -l ${vnmrsystem}/vnmrrev | awk '{ printf($3) }')
      group=$(ls -l ${vnmrsystem}/vnmrrev | awk '{ printf($4) }')
      chown $owner:$group ${vnmrsystem}/adm/log/CONSOLE_* 2> /dev/null
      chmod 644 ${vnmrsystem}/adm/log/CONSOLE_* 2> /dev/null
   else
      file=${vnmrsystem}/adm/log/.try
      touch ${file}
      if [[ -e  ${file} ]] ; then
         rm -f ${file}
         echo "${OVJ_NIC}" > ${vnmrsystem}/adm/log/CONSOLE_NIC
         echo "${OVJ_IP_BASE}" > ${vnmrsystem}/adm/log/CONSOLE_IP
         chmod 644 ${vnmrsystem}/adm/log/CONSOLE_* 2> /dev/null
      fi
   fi
}

#
# Main program starts here
#
if [[ ! x$(uname -s) = "xLinux" ]] ; then
   echo " "
   echo "$SCRIPT suitable for Linux-based systems only"
   echo " "
   exit 0
fi

getEthernetInfo
if [[ ${OVJ_SHOW} -eq 1 ]]; then
   showEthernetInfo
   echo " "
   if [[ -e ${vnmrsystem}/adm/log/CONSOLE_NIC ]] &&
      [[ -e ${vnmrsystem}/adm/log/CONSOLE_IP ]] ; then
      nic=$(cat ${vnmrsystem}/adm/log/CONSOLE_NIC)
      ip=$(cat ${vnmrsystem}/adm/log/CONSOLE_IP)
      echo "Spectrometer configured on $nic at ${ip}.1"
      echo " "
   else
      echo "Spectrometer network has not been configured"
      echo " "
   fi
   exit 0
fi

if [[ $totalNics -eq 0 ]]; then
   echo "This PC does not have any Network Interface Cards."
   echo "It can not be used as a spectrometer host"
   exit 0
fi

if [[ ${OVJ_VERIFY} -eq 0 ]] ; then
#  Need to be root to configure a NIC
   userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
   if [[ $userId != "uid=0(root)" ]]; then
      echo
      echo "To run $0 you will need to be the system's root user,"
      echo "or type cntrl-C to exit."
      echo
      s=1
      t=3
      while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
         echo "Please enter this system's root user password"
         echo
         if [ -f /etc/debian_version ]; then
            sudo $0 $* ;
         else
            su root -c "$0 $*";
         fi
         s=$?
         t=$((t-1))
         echo " "
      done
      if [[ $t = 0 ]]; then
         echo "Access denied. Type cntrl-C to exit this window."
         echo
      fi
   exit 0
   fi
fi

# Check input options
if [[ "x${OVJ_IP}" != "x0" ]] ; then
   if [[ "$OVJ_IP" = "172"* ]] ; then
      OVJ_IP=172.16.0.1
      OVJ_IP_BASE=172.16.0
   elif [[ "$OVJ_IP" = "10"* ]] ; then
      OVJ_IP=10.0.0.1
      OVJ_IP_BASE=10.0.0
   else
      echo " "
      echo "-ip option neither 172 (172.16.0.1)"
      echo "nor 10 (10.0.0.1).  Ignoring it."
      echo " "
      OVJ_IP=0
   fi
fi
if [[ "x${OVJ_NIC}" != "x0" ]] ; then
   ok=0
   for ((i=0;i<$totalNics;i+=1)) ; do
      if [[ "x${OVJ_NIC}" = "x${nicName[$i]}" ]] ; then
         ok=1
      fi
   done
   if [[ $ok -eq 0 ]] ; then
      echo " "
      echo "-nic option does not match available NIC names. Ignoring it."
      echo " "
      OVJ_NIC=0
   fi
fi

if [[ ${OVJ_VERIFY} -eq 0 ]] && [[ $totalNics -eq 1 ]]; then
   echo "This system has only one Network Interface Card (NIC). "
   echo "Do you wish configure this NIC for the"
   echo "spectromenter (y or n)? [y] "
   read ans
   if [[ "x$ans" != "xn" ]]  && [[ "x$ans" != "xN" ]] ; then
      OVJ_NIC=${nicName[0]}
      if [[ ${OVJ_IP} -eq 0 ]] ; then
         echo "Do you want to configure the IP address as"
         echo "1.  172.16.0.1  (preferred)"
         echo "2.  10.0.0.1"
         echo "Enter 1, 2, or q to quit (1,2,q): [1] "
         read ans
         if [[ "x$ans" = "x" ]] || [[ "x$ans" = "x1" ]] ; then
            OVJ_IP=172.16.0.1
            OVJ_IP_BASE=172.16.0
         elif [[ "x$ans" = "x2" ]] ; then
            OVJ_IP=10.0.0.1
            OVJ_IP_BASE=10.0.0
         else
            if [[ "x$ans" != "xq" ]] ; then
               echo "Unknown response: ${ans}"
            fi
            exit 0
         fi
      fi
   else
      exit 0
   fi

elif [[ ${OVJ_VERIFY} -eq 0 ]] ; then
   while [[ "x${OVJ_NIC}" = "x0" ]] ; do
      showEthernetInfo
      echo " "
      echo "Select the index of the NIC you would like to use."
      echo "Alternatively, you may enter a b to sequentially blink"
      echo "each NIC and select the proper one."
      echo "You may also enter a q to quit"
      echo "Enter index, b, or q to quit (1-$totalNics,b,q): "
      read ans
      if [[ "x$ans" = "xq" ]] || [[ "x$ans" = "xQ" ]] ; then
         exit 1
      elif [[ "x$ans" = "xb" ]] || [[ "x$ans" = "xB" ]] ; then
         for ((i=0;i<$totalNics;i+=1))
         do
            j=$((i+1))
            echo " "
            echo "Blinking NIC $j ( ${nicName[$i]} ) for 10 secs"
            /usr/sbin/ethtool -p ${nicName[$i]} 10
            echo "Was that the correct NIC (y or n)? [y] "
            read ans
            if [[ "x$ans" != "xn" ]] ; then
               OVJ_NIC=${nicName[$i]}
               break
            fi
         done
      else
         for ((i=0;i<$totalNics;i+=1))
         do
            j=$((i+1))
            if [[ "x$ans" = "x$j" ]] ; then
               OVJ_NIC=${nicName[$i]}
            fi
         done
         if [[ "x${OVJ_NIC}" = "x0" ]] ; then
            echo "One must enter 1-$totalNics, b, or q"
         fi
      fi
   done
   while [[ "x${OVJ_IP}" = "x0" ]] ; do
      echo " "
      echo "Do you want to configure ${OVJ_NIC} as"
      echo "1.  IP 172.16.0.1  (preferred)"
      echo "2.  IP 10.0.0.1"
         echo "Enter 1, 2, or q to quit (1,2,q): [1] "
         read ans
         if [[ "x$ans" = "x" ]] || [[ "x$ans" = "x1" ]] ; then
            OVJ_IP=172.16.0.1
            OVJ_IP_BASE=172.16.0
         elif [[ "x$ans" = "x2" ]] ; then
            OVJ_IP=10.0.0.1
            OVJ_IP_BASE=10.0.0
         else
            if [[ "x$ans" != "xq" ]] ; then
               echo "Unknown response: ${ans}"
            fi
            exit 0
         fi

   done
fi

if [[ ${OVJ_VERIFY} -eq 0 ]] ; then
   echo "Selected ethernet device is ${OVJ_NIC}"
   echo "Selected IP address is ${OVJ_IP}"
   echo "Do you wish to continue (y or n)? [y] "
   read ans
   if [[ "x$ans" = "xn" ]] || [[ "x$ans" = "xN" ]] ; then
      echo "Aborting NIC configuration"
      exit 1
   fi
   checkWormhole
   if [[ $? -ne 0 ]] ; then
      exit 1
   fi
   if [[ -d /etc/sysconfig ]]; then
      addIfcfgFile
   elif [[ -f /etc/network/interfaces ]];  then
      addToInterfaces
   elif [[ -d /etc/netplan ]];  then
      addToNetplan
   fi
   if [[ -e ${vnmrsystem}/adm/log/CONSOLE ]] ; then
      cons=$(cat ${vnmrsystem}/adm/log/CONSOLE)
   else
      cons=ddr
   fi
   if [[ x${cons} = "xinova" ]] || [[ x${cons} = "xmerc"* ]] ; then
      addToHostsMI
      addToHostsEquivMI
   else
      addToHosts
      addToHostsEquiv
      addToEthers
   fi
   addTrusted
   # This just updates the gui. In CentoS, the reload option
   # does not exist. Just ignore the error
   if [[ -x /usr/bin/nmcli ]] ; then
      nmcli con reload &> /dev/null
   fi
   saveNICinfo
   echo "Restarting network"
   /etc/init.d/network restart &> /dev/null
   exit 0
else
   echo "Verify NIC setup"
   exitCode=0
   if [[ -e ${vnmrsystem}/adm/log/CONSOLE ]] ; then
      cons=$(cat ${vnmrsystem}/adm/log/CONSOLE)
   else
      cons=ddr
   fi
   if [[ x${cons} = "xinova" ]] || [[ x${cons} = "xmerc"* ]] ; then
      doingMI=1
      controller=inova
   else
      doingMI=0
      controller=ddr1
      checkWormhole
      if [[ $? -ne 0 ]] ; then
         exitCode=2
      fi
   fi
   line=$(grep -w $controller /etc/hosts)
   if [[ -z "$line" ]]; then
      exitCode=$((exitCode+1))
      echo "$controller does not exist in /etc/hosts"
      exit $exitCode
   fi
   OVJ_IP=$(echo "$line" | awk '{print $1}')
   if [[ "$OVJ_IP" = "172.16.0."* ]] ; then
      OVJ_IP=172.16.0.1
      OVJ_IP_BASE=172.16.0
   elif [[ "$OVJ_IP" = "10.0.0."* ]] ; then
      OVJ_IP=10.0.0.1
      OVJ_IP_BASE=10.0.0
   else
      echo "Base IP address of $controller is nether 172.16.0 nor 10.0.0"
      exitCode=$((exitCode+1))
   fi
   OVJ_NIC=""
   match=0
   for ((i=0;i<$totalNics;i+=1))
   do
      if [[ "$OVJ_IP" = "${nicIP[$i]}" ]] && 
         [[ "${nicStatus[$i]}" = "up" ]] ; then
         OVJ_NIC=${nicName[$i]}
         match=$((match+1))
      fi
   done
   echo "Verify NIC ${OVJ_NIC} with IP address ${OVJ_IP}"
   if [[ $match -gt 1 ]] ; then
      exitCode=$((exitCode+1))
      echo "Multiple NIC are assigned to ${OVJ_IP}"
   fi
   if [[ ${doingMI} -eq 1 ]] ; then
      checkHostsMI
      if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
      fi
      checkHostsEquivMI
      if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
      fi
   else
      checkHosts
      if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
      fi
      checkHostsEquiv
      if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
      fi
      checkEthers
      if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
      fi
   fi
   checkTrusted
   if [[ $? -ne 0 ]] ; then exitCode=$((exitCode+1))
   fi
   if [[ $exitCode -eq 0 ]] ; then
      saveNICinfo
   fi
   exit $exitCode
fi

