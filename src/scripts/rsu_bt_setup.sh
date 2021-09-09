#!/bin/bash
#
# Add Bluetooth Remote Status Unit components
#
#-----------------------------------------------------------------
# make sure no VnmrJ is running
#
# set -x

#if /vnmr/web does not exist, exit
if [[ ! -d /vnmr/web ]]; then
   exit 0
fi

findacqproc="ps -e  | grep Vnmr | awk '{ printf(\"%d \",\$1) }'"
npids=`eval $findacqproc`
if (test x"$npids" != "x" )
then 
   echo ""
   echo "You must exit all 'VnmrJ'-s to run $0"
   echo "Please type 'exit' in the VnmrJ command line, or close VnmrJ by "
   echo "using the 'X' button or the Menu 'File -> Exit VnmrJ'" 
   echo "Then restart $0"
   echo ""
   exit 0
fi

exit_code=0
consolenic=eth0
iam=`whoami`
this_script=$0
quiet=0
dont_install_bt=0
dont_prompt=0
dont_restart_network=0

# bluetooth adapter device - should match `hcitool dev | grep hci`
hci_device="hci0"
hci_device_addr=""

# bluetooth network pseudo-device
bnep_device="bnep0"

# bluetooth personal-area-network pseudo-device
pan_device="pan1"
pan_subnet="172.16.1"
pan_subnet_start=${pan_subnet}.0
pan_ip_addr=${pan_subnet}.1
pan_netmask="255.255.255.0"

# DHCP and bluetooth networking configuration files
dhcpd_conf="/etc/dhcp/dhcpd.conf"
sysconf_dir="/etc/sysconfig"
dhcpd_sysconf="${sysconf_dir}/dhcpd"
bnep_script="${sysconf_dir}/network-scripts/ifcfg-${bnep_device}"
pan_script="${sysconf_dir}/network-scripts/ifcfg-${pan_device}"

# nmrweb version
vnmrsystem="/vnmr"
version_file="${vnmrsystem}/web/.version"
install_log="${vnmrsystem}/web/run/.install_log"

# enable network restart
restart_network=1
network_restart_required=0

msg()
{
  if [[ $quiet == 0 ]]; then echo -e "$*" | tee -a ${install_log}; fi
}

msg_()
{
  if [[ $quiet == 0 ]]; then echo -ne "$*" | tee -a ${install_log}; fi
}

error()
{
  if [[ $quiet == 0 ]]; then echo -e "\nERROR: $*"; fi
  if [[ -w ${install_log} ]]; then echo -e "\n$*" >> ${install_log}; fi
  exit 2
}

warn()
{
  echo -e "\nWARNING: $*" | tee -a ${install_log}
}

check_bt_dongle()
{
  msg_ "checking bluetooth dongle.. "
  dongled=$(lsusb | grep -i "Bluetooth Dongle" | wc -l)
  if [ $dongled -eq 0 ]; then
    echo ""
    echo ""
    echo "no bluetooth dongle detected."
    echo "If you want bluetooth enabled, attach the dongle and re-run"
    echo "$this_script"
    return
  fi
  msg "OK"
  return
}

check_hci_device()
{
  msg_ "checking hci device"
  hci_device=`hcitool dev | grep hci | awk '{print $1}'`
  hci_device_addr=`hcitool dev | grep hci | awk '{print $2}'`
  if [[ "${hci_device_addr}" == "" ]]; then
    error ".. no bluetooth adapter has been found - please insert and re-run"
  else
    msg ".. hci device ${hci_device} is ${hci_device_addr}"
  fi
}

check_required_rpms()
{
  msg_ "checking for required RPM packages"
  have_bluez=`rpm -q blue | grep -v "not installed" | wc -l`
  if [[ $have_bluez -lt 1 ]]; then 
    error "required RHEL package bluez is missing - install RHEL installation disk and restart the $0";
  fi
}

install_tornado()
{
  msg_ "installing web server"
  if [[ -x /usr/bin/dpkg ]]; then
      dpkg-query -l python3-tornado >& /dev/null
      if [[ $? -eq 1 ]]; then
      vers=$(python3 --version | cut -d" " -f2 | cut -d. -f2)
      if [[ $vers -ge 8 ]]; then
         apt-get -y install ${vnmrsystem}/web/dist/python3-tornado_6.0.3+really5.1.1-3_amd64.deb  &>> ${install_log}
      else
         apt-get -y install ${vnmrsystem}/web/dist/python3-tornado_4.5.3-1ubuntu0.2_amd64.deb  &>> ${install_log}
      fi
    fi
    
  elif [[ -z $(type -t python) ]]; then
    if [[ "$(rpm -q python3-tornado |
         grep 'not installed' > /dev/null;echo $?)" == "0" ]]; then
      yum -y install ${vnmrsystem}/web/dist/python3-tornado-6.0.2-1.el8.x86_64.rpm  &>> ${install_log}
    fi
  else
    if [[ "$(rpm -q python-tornado |
         grep 'not installed' > /dev/null;echo $?)" == "0" ]]; then
      yum -y install ${vnmrsystem}/web/dist/python-tornado-4.2.1-1.el7.x86_64.rpm &>> ${install_log}
    fi
  fi
  msg ".. OK"
}

sysconfig_dhcpd()
{
  msg_ "setting system configuration for DHCP for bluetooth network interface"
  if [[ -e $dhcpd_sysconf ]]; then
    timestamp=`date +"%y-%m-%d.%H:%M:%S"`
    mv ${dhcpd_sysconf} ${dhcpd_sysconf}.${timestamp}
  fi
  cat > $dhcpd_sysconf <<EOF
# only listen to DHCP requests on the bluetooth network
DHCPDARGS=${pan_device};
EOF
  msg ".. OK"
}

setup_bt_dhcpd()
{
  # look for lines without just white space and whitespace followed by a comment
  if [[ -e ${dhcpd_conf} ]]; then
    dhcp_seems_configured=`egrep -v '(^[[:space:]]*$)|(^[[:space:]]*#)' ${dhcpd_conf} | wc -l`
    timestamp=`date +"%y-%m-%d.%H:%M:%S"`
    saved_dhcpd_conf=${dhcpd_conf}.${timestamp}
    if [[ $dhcp_seems_configured -gt 0 ]]; then
      msg "${dhcpd_conf} seems to be configured, saving to ${saved_dhcpd_conf}"
    fi
    mv ${dhcpd_conf} ${saved_dhcpd_conf}
  fi
  msg_ "configuring DHCP server"
  cat > ${dhcpd_conf} <<EOF
#
# DHCP Server Configuration File
#   Configured to support Agilent NMR Bluetooth PAN on ${pan_subnet} subnet.
#
ddns-update-style none;
authoritative;

subnet ${pan_subnet}.0 netmask 255.255.255.0 {
        option routers  ${pan_ip_addr};
        option domain-name "bluetooth";
        range ${pan_subnet}.101 ${pan_subnet}.150;
	default-lease-time 864000;
	max-lease-time 864000;
        
	host btrsu {
                # change hardware address to match RSU BT Tablet's
	     	hardware ethernet ${hci_device_addr};
		fixed-address 172.16.1.100;
	}
}
EOF
  msg ".. OK"
  msg_ "restarting DHCP server"
  dhcpd_hiccuped=`/sbin/service dhcpd restart 2>&1 | tee -a ${install_log} | grep FAILED | wc -l`
  if [[ $dhcpd_hiccuped -lt 1 ]]; then 
    msg ".. OK" 
  else
    msg ".. FAILED - check /var/log/messages"
  fi

  msg_ "enabling DHCP server to restart at reboot"
  /sbin/chkconfig dhcpd on
  msg ".. OK"
}

install_bt()
{
  modules="bluetooth rfcomm btusb llc sco bridge bnep stp l2cap"
  # /sbin/lsmod | egrep '(bluetooth)|(rfcomm)|(btusb)|(llc)|(sco)|(bridge)|(bnep)|(stp)|(l2cap)'
  msg_ "adding kernel modules"
  for m in $modules ; do
    /sbin/modprobe $m
    msg_ " $m"
  done
  msg ".. OK"
  #/sbin/modprobe -v hidp

  # enable bluetooth page and inquiry scan
  msg_ "configuring ${hci_device} for page and inquiry scan"
  /usr/sbin/hciconfig ${hci_device} piscan

  have_pscan=`/usr/sbin/hciconfig $hci_device | grep -iw pscan | wc -l`
  if [[ $have_pscan -lt 1 ]]; then
    msg ".. could not configure $hci_device for page scan"
    exit 2
  fi

  have_iscan=`/usr/sbin/hciconfig $hci_device | grep -iw iscan | wc -l`
  if [[ $have_iscan -lt 1 ]]; then
    msg ".. could not configure $hci_device for inquiry scan"
    exit 2
  fi

  msg ".. OK"
}

install_bt_pan()
{
  #/sbin/iptables -V
  #/sbin/lsmod | grep ip_tables
  msg_ "configuring network interfaces"
  cat > $bnep_script <<EOF
DEVICE=${bnep_device}
BOOTPROTO=dhcp
NM_CONTROLLED=no
ONBOOT=no
EOF
  chmod 644 $bnep_script

  cat > $pan_script <<EOF
DEVICE=${pan_device}
ONBOOT=yes
TYPE=Bridge
STP=off
DELAY=0
BOOTPROTO=static
IPADDR=${pan_ip_addr}
IPV6INIT=no
NM_CONTROLLED=no
EOF
  chmod 644 $pan_script
  network_restart_required=1
  msg ".. OK"

  bluetooth_conf='/etc/bluetooth/main.conf'
  if [[ -e ${bluetooth_conf} ]]; then
    /bin/sed -i.bkup 's/Class = 0x[0-9]*/Class = 0x020300/' ${bluetooth_conf}
  else
    msg "bluetooth does not seem to be configured" 
  fi

  bluetooth_network_svc='/etc/bluetooth/network.service'
  cat > $bluetooth_network_svc <<EOF
[Bluetooth Service]
Identifier=network
Name=Network service
Description=Bluetooth Personal Area Network service
Autostart=true
EOF
  chmod 644 $bluetooth_network_svc

  bluetooth_network_conf='/etc/bluetooth/network.conf'
  cat > $bluetooth_network_conf <<EOF
[NAP Role]
Interface=${pan_device}
EOF

  msg_ "restarting bluetooth services"
  /sbin/service bluetooth stop > /dev/null
  /sbin/service bluetooth start > /dev/null
  msg ".. OK"

  # enable forwarding so that the tablet can access the internet
  echo "1" > /proc/sys/net/ipv4/ip_forward

  pan_configured=`/usr/sbin/brctl show | grep ${pan_device} | wc -l`
  if [[ $pan_configured -lt 1 ]]; then
    /usr/sbin/brctl addbr ${pan_device}
  fi
  /sbin/ifconfig ${pan_device} ${pan_ip_addr}
  /usr/sbin/brctl setfd ${pan_device} 0  # set forward-delay to none
  /usr/sbin/brctl stp ${pan_device} off  # disable spanning tree protocol for bt

  #/usr/sbin/brctl addif ${pan_device} ${bnep_device}
  msg_ "configuring iptables"
  /sbin/iptables -t nat -A POSTROUTING -j MASQUERADE >> ${install_log}
  /sbin/iptables-save >> ${install_log}
  msg ".. OK"
}

setup_install_log()
{
  logdir=$(dirname ${install_log})
  mkdir -p $logdir || error "could not create ${logdir}"
  touch ${install_log} || error "could not write to ${install_log}"
}

stop_nmrwebd()
{
  if [[ -f $vnmrsystem/web/scripts/vnmrweb.service ]]; then
    if [[ ! -z $(type -t systemctl) ]] ; then
       sysdDir=$(pkg-config systemd --variable=systemdsystemunitdir)
       rm -f $sysdDir/vnmrweb.service
       cp $vnmrsystem/web/scripts/vnmrweb.service $sysdDir/.
       chmod 644 $sysdDir/vnmrweb.service
       systemctl enable --quiet vnmrweb.service
    fi
  fi
  if [[ -e /vnmr/web/run/nmrwebd.pid ]]; then
    msg_ "NMR web services are already running - stopping nmrwebd"
    if [[ ! -z $(type -t systemctl) ]] ; then
       systemctl stop --quiet vnmrweb.service
    else
       /vnmr/web/scripts/ovjWeb stop
    fi
    msg ".. OK"
  fi
}

start_nmrwebd()
{
  msg_ "starting NMR web service"
  if [[ ! -z $(type -t systemctl) ]] ; then
     systemctl start --quiet vnmrweb.service
  else
     /vnmr/web/scripts/ovjWeb start
  fi
  sleep 2
  if [[ -e /vnmr/web/run/nmrwebd.pid ]]; then
    nmrwebd_pid=`cat /vnmr/web/run/nmrwebd.pid`
    nmrwebd_started=`ps -p ${nmrwebd_pid} | wc -l`
  else
    nmrwebd_started=0
  fi
  if [[ $nmrwebd_started -lt 1 ]]; then
    error "nmrwebd failed to start"
  else
    msg ".. OK"
  fi
}

install_nmrwebd_components()
{
  stop_nmrwebd
  install_tornado
  start_nmrwebd
}

install_bt_components()
{
  check_bt_dongle
  if [[ dongled -ne 0 ]]; then
     install_bt
     check_hci_device
     install_bt_pan
     sysconfig_dhcpd
     setup_bt_dhcpd
  fi
}

as_root()
{
  s=1
  t=3
  while [ $s == 1 -a ! $t == 0 ]; do
     if [ -x /usr/bin/dpkg ]; then
        echo "If requested, enter the admin (sudo) password"
        sudo $this_script $* ;
     else
        echo "Please enter this system's root user password"
        su root -c "$this_script $*";
     fi
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t -eq 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $this_script to start the installation program again"
      echo
  fi
  exit 0
}

if [[ "$iam" != "root" ]]; then
  as_root $*
fi

for arg in $*; do
  # check if the script should be run silently (except errors)
  if [[ "$arg" = "-q" ]]; then quiet=1; fi
  if [[ "$arg" = "-nobt" ]]; then dont_install_bt=1; fi
  if [[ "$arg" = "-noprompt" ]]; then dont_prompt=1; fi
  if [[ "$arg" = "-nonet" ]]; then dont_restart_network=1; fi
done

setup_install_log

install_nmrwebd_components
if [[ $dont_install_bt -eq 0 ]]; then
  # install the bluetooth dongle related softwared if we have a dongle
  # or it is ok to prompt the user to install it
  if [[ $dont_prompt == 0 ]]; then
    install_bt_components
  fi
fi

if [[ $network_restart_required -eq 1 ]]; then
  if [[ $restart_network -eq 1 ]]; then
    if [[ $dont_restart_network -eq 0 ]]; then
      /sbin/service network restart
    else
      exit_code=4
    fi
  fi
fi

if [[ $quiet -eq 0 ]]; then
  echo
  echo "Remote Status Unit host setup complete"
  echo
fi

admin=$(/vnmr/bin/fileowner /vnmr/vnmrrev)
grp=$(/vnmr/bin/fileowner -g /vnmr/vnmrrev)
chown -R $admin:$grp /vnmr/web/run

exit $exit_code
