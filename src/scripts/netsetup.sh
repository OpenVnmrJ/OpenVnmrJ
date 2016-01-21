: '@(#)netsetup.sh 22.1 03/24/08 2003-2004 '
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
#netsetup
#
#for a new "RedHat WS 3  Update 2" system
#run this script right after pkgsetup
#then reboot the system
# initialize the main network configuration files
# "/etc/hosts"
# "/etc/sysconfig/network"
# "/etc/sysconfig/network-scripts/ifcfg-eth0"
# "/etc/yp.conf"
# "/etc/nsswitch.conf"
# other files not changes yet but maybe should be....  GMB
# "/etc/sysconfig/networking/profiles/default/ifcfg-eth0"
# "/etc/sysconfig/networking/profiles/default/hosts"
# "/etc/sysconfig/networking/devices/ifcfg-eth0"
#

host_name=
host_ip=
net_mask=255.255.255.0
net_ip=
brdc_ip=
gateway=
nis_serv=
nis_serv_ip=
nis_domain=
done=

get_sys_val() { 

   echo
   echo -n "Enter system host name [$host_name]:"
   read answ
   if [ x$answ != "x" ]
   then 
       host_name=$answ
   fi
   echo -n "Enter system host IP address [$host_ip]:"
   read answ
   if [ x$answ != "x" ]
   then 
       host_ip=$answ
   fi
   echo -n "Enter system Netmask address [$net_mask]:"
   read answ
   if [ x$answ != "x" ]
   then 
       net_mask=$answ
   fi
   sub_net=`echo $host_ip | cut -d. -f1,2,3`
   if [ x$net_ip = "x" ]
   then
      net_ip=${sub_net}.0 
   fi
   echo -n "Enter system network IP address [$net_ip]:"
   read answ
   if [ x$answ != "x" ]
   then 
       net_ip=$answ
   fi
   if [ x$brdc_ip = "x" ]
   then
      brdc_ip=${sub_net}.255 
   fi
   echo -n "Enter system broadcast IP address [$brdc_ip]:"
   read answ
   if [ x$answ != "x" ]
   then 
       brdc_ip=$answ
   fi
   if [ x$gateway = "x" ]
   then
      gateway=${sub_net}.1 
   fi
   echo -n "Enter system gateway IP address [$gateway]:"
   read answ
   if [ x$answ != "x" ]
   then 
       gateway=$answ
   fi

   echo
   echo -n "Does your network using NIS ? [y/n]:"
   read nis_answ
   if [ x$nis_answ = "xy" -o x$nis_answ = "xyes" ]
   then 
       echo 
       echo -n "Enter system NIS server name [$nis_serv]:"
       read answ
       if [ x$answ != "x" ]
       then 
           nis_serv=$answ
       fi
       if [ x$nis_serv_ip = "x" ]
       then
          nis_serv_ip=${sub_net}.2
       fi
       echo -n "Enter system NIS server IP address [$nis_serv_ip]:"
       read answ
       if [ x$answ != "x" ]
       then 
           nis_serv_ip=$answ
       fi
       echo -n "Enter system NIS domain name [$nis_domain]:"
       read answ
       if [ x$answ != "x" ]
       then 
           nis_domain=$answ
       fi
   fi

   echo
   echo
   echo
   echo "The system setting values are:"
   echo
   echo "	Host name     =  $host_name"
   echo "	Host IP       =  $host_ip"
   echo "	Netmask       =  $net_mask"
   echo "	Network IP    =  $net_ip"
   echo "	Broadcast IP  =  $brdc_ip"
   echo "	Gateway       =  $gateway"
   echo
   if [ x$nis_answ = "xy" -o x$nis_answ = "xyes" ]
   then 
       echo "	NIS server    =  $nis_serv"
       echo "	NIS server IP =  $nis_serv_ip"
       echo "	NIS domain    =  $nis_domain"
   fi
   echo -n "Are these correct values? y/n [n]:"
   read anws
   if [ x$anws != "xy" ]
   then
      get_sys_val
   fi
   if [ x$done != "xy" ]
   then
      re_confirm
   fi
}

re_confirm () {
   echo
   echo
   echo
   echo "***** Please RECONFIRM ! *****"
   echo
   echo "Press Ctrl c to terminate this script"
   echo "Press any key plus Enter to modify the setting values"
   echo -n "Press Enter to accept the values []:"
   read answ
   echo
   if [ x$answ != "x" ]
   then
       done="n"
       get_sys_val
   fi
   done="y"
}

get_sys_val
                                                                              
network_file="/etc/sysconfig/network"
ifcfg_eth0_file="/etc/sysconfig/network-scripts/ifcfg-eth0"
hosts_file="/etc/hosts"
yp_conf="/etc/yp.conf"
nsswitch_conf_file="/etc/nsswitch.conf"
profiles_default_ifcfg_eth0_file="/etc/sysconfig/networking/profiles/default/ifcfg-eth0"
profiles_default_hosts="/etc/sysconfig/networking/profiles/default/hosts"
devices_ifcfg_eth0_file="/etc/sysconfig/networking/devices/ifcfg-eth0"
tmpfile="/tmp/tt1"

#---------------------------------------------------------------------
#saving the originals
cp $network_file ${network_file}.orig
cp $ifcfg_eth0_file /etc/sysconfig/ifcfg-eth0.orig
cp $hosts_file ${hosts_file}.orig
cp $yp_conf ${yp_conf}.orig
cp $nsswitch_conf_file ${nsswitch_conf_file}.orig

#---------------------------------------------------------------------
sed '/NETWORKING/d' $network_file	| \
sed '/HOSTNAME/d'		| \
sed '/GATEWAY/d'		| \
sed '/NISDOMAIN/d'		> $tmpfile

echo "NETWORKING=yes" >> $tmpfile
echo "HOSTNAME=$host_name" >> $tmpfile
echo "GATEWAY=$gateway" >> $tmpfile
if [ x$nis_answ = "xy" -o x$nis_answ = "xyes" ]
then
   echo "NISDOMAIN=$nis_domain" >> $tmpfile
fi

mv $tmpfile $network_file

#---------------------------------------------------------------------
sed '/TYPE/d' $ifcfg_eth0_file	| \
sed '/BROADCAST/d'	| \
sed '/IPADDR/d'		| \
sed '/NETMASK/d'	| \
sed '/NETWORK/d'	| \
sed '/BOOTPROTO/d'	> $tmpfile

echo  "TYPE=Ethernet" >> $tmpfile
echo  "IPADDR=$host_ip" >> $tmpfile
echo  "NETMASK=$net_mask" >> $tmpfile
echo  "BROADCAST=$brdc_ip" >> $tmpfile
echo  "NETWORK=$net_ip" >> $tmpfile
echo  "BOOTPROTO=none" >> $tmpfile

mv $tmpfile $ifcfg_eth0_file

#---------------------------------------------------------------------
echo "$host_ip       $host_name loghost" >> /etc/hosts

if [ x$nis_answ = "xy" -o x$nis_answ = "xyes" ]
then
   echo "$nis_serv_ip       $nis_serv" >> /etc/hosts
   echo "domain $nis_domain server $nis_serv_ip" >> /etc/yp.conf
   if [ ! -L /etc/rc5.d/S27ypbind ]
   then
       cd /etc/rc5.d
       ln -s ../init.d/ypbind S27ypbind
   fi
fi

sed 's/passwd: * files/passwd:      files nis/' /etc/nsswitch.conf | \
sed 's/shadow: *  files/shadow:      files nis/' | \
sed 's/group: * files/group:      files nis/'  | \
sed 's/hosts: * files/hosts:      files nis/'  | \
sed 's/protocols: * files/protocols:  files nis/'  | \
sed 's/services: * files/services:   files nis/'  | \
sed 's/netgroup: * files/netgroup:   files nis/'  | \
sed 's/automount: * files/automount:  files nis/' > $tmpfile
mv $tmpfile /etc/nsswitch.conf

echo
echo "Reboot the system for the changes taking effect"
echo
