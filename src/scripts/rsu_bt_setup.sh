#!/bin/sh
#
# Add Bluetooth Remote Status Unit components
#
#-----------------------------------------------------------------
# make sure no VnmrJ is running
#
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

# tornado
tornado_version="3.1"
tornado=tornado-${tornado_version}
tornado_dist=${vnmrsystem}/web/dist/${tornado}.tar.gz

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

have_bt_dongle()
{
  local dongled=`lsusb | grep -i "Bluetooth Dongle" | wc -l`
  if [[ $# -gt 0 ]]; then
    eval "$1=$dongled"
  else
    echo $dongled
  fi
}

check_bt_dongle()
{
  local congled=0
  msg_ "checking bluetooth dongle.. "
  have_bt_dongle dongled  #=`lsusb | grep -i "Bluetooth Dongle" | wc -l`
  while [ $dongled == 0 ]; do
    echo "no bluetooth dongle detected."
    read -n1 -p "Please attach and press enter to continue or Ctrl-C to quit"
    have_bt_dongle dongled  #=`lsusb | grep -i "Bluetooth Dongle" | wc -l`
  done
  msg "OK"
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

check_rhel_version()
{
  msg_ "checking Redhat release"
  rhel_rel=`cat /etc/redhat-release | sed -r 's/[^0-9]+//' | sed -r 's/[^0-9.]+$//'`
  if [[ $rhel_rel < 6.1 ]]; then
    error ".. older than release 6.1 - vjweb is not supported for Redhat version $rhel_rel"
  fi
  msg ".. $rhel_rel - OK"
}

check_required_rpms()
{
  msg_ "checking for required RPM packages"
  have_bluez=`rpm -q blue | grep -v "not installed" | wc -l`
  if [[ $have_bluez < 1 ]]; then 
    error "required RHEL package bluez is missing - install RHEL installation disk and restart the $0";
  fi
}

check_vjweb_version()
{
  msg_ "checking vjweb version"
  if [[ -e ${version_file} ]]; then
    installed_version=`cat ${version_file}`
    if [[ $installed_version < $version ]]; then
      error "installed version $installed_version is newer than this this version $version"
    fi
  fi
  msg ".. OK"
}

check_python_version()
{
  msg_ "checking python version"
  version=`python --version |& cut -d' ' -f2`
  if [[ ${version} < 2.6 ]]; then
     error ".. $version - python must be at version 2.6 or above"
  fi
  msg ".. ${version} - OK"
}

check_tornado_version()
{
  #msg_ "checking tornado version"
  tornado_installed=`printf "import tornado\nprint tornado.version" | python`
  if [[ $tornado_installed < 3.1 ]]; then
    warn "${program} has only been tested with tornado version 3.1 or later"
    return 1
  fi
  #msg ".. ${tornado_installed} - OK"
  return 0
}

install_tornado()
{
  msg_ "installing web server"
  pushd /tmp > /dev/null
  tar xf ${tornado_dist}
  cd ${tornado}
  python setup.py build >> ${install_log}
  /usr/bin/sudo python setup.py install >> ${install_log}
  popd > /dev/null
  check_tornado_version || error "tornado installation failed"
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
    if [[ $dhcp_seems_configured > 0 ]]; then
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
  if [[ $dhcpd_hiccuped < 1 ]]; then 
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
  if [[ $have_pscan < 1 ]]; then
    msg ".. could not configure $hci_device for page scan"
    exit 2
  fi

  have_iscan=`/usr/sbin/hciconfig $hci_device | grep -iw iscan | wc -l`
  if [[ $have_iscan < 1 ]]; then
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
  if [[ $pan_configured < 1 ]]; then
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

check_ip_config()
{
  # check the most likely candidate
  wormhole=wormhole
  wormholeip=`gethostip $wormhole | awk '{print $2}'`
  if [[ "$wormholeip" != "172.16.0.1" && "$wormholeip" != "10.0.0.1" ]]; then
    echo "console port has not been configured correctly - please run setacq first"
    exit 2
  fi

  msg_ "determining console network interface"
  consolenic=""
  for nic in `/sbin/ifconfig -a | awk '/^eth/ { print $1}' | grep -v :`; do
    addr=`/sbin/ifconfig $nic | awk '/inet addr/ { print $2}' | sed 's/addr://'`
    if [[ "$addr" == "$wormholeip" ]]; then
      msg ".. selected $nic for tablet network"
      consolenic=$nic
    fi
  done
  if [[ "$consolenic" == "" ]]; then
    error "console NIC ($wormhole) could not be determined - has it been configured?"
  fi
}

setup_install_log()
{
  logdir=`dirname ${install_log}`
  mkdir -p $logdir || error "could not create ${logdir}"
  touch ${install_log} || error "could not write to ${install_log}"
}

stop_nmrwebd()
{
  pgrep -f nmrwebd > /dev/null && \
    msg_ "NMR web services are already running - stopping nmrwebd" && \
    pkill -f nmrwebd && msg ".. OK"
}

start_nmrwebd()
{
  msg_ "starting NMR web service"
  /bin/sh /vnmr/acqbin/rc.vnmr start nmrwebd
  sleep 2
  if [[ -e /vnmr/web/run/nmrwebd.pid ]]; then
    nmrwebd_pid=`cat /vnmr/web/run/nmrwebd.pid`
    nmrwebd_started=`ps -p ${nmrwebd_pid} | grep nmrwebd | wc -l`
  else
    nmrwebd_started=0
  fi
  if [[ $nmrwebd_started < 1 ]]; then
    error "nmrwebd failed to start"
  else
    msg ".. OK"
  fi
}

install_nmrwebd_components()
{
  check_rhel_version
  check_python_version
  check_vjweb_version
  stop_nmrwebd
  install_tornado
  start_nmrwebd
}

install_bt_components()
{
  check_bt_dongle
  install_bt
  check_hci_device
  install_bt_pan
  sysconfig_dhcpd
  setup_bt_dhcpd
}

as_root()
{
  s=1
  t=3
  while [ $s == 1 -a ! $t == 0 ]; do
     echo
     echo "Please enter this system's root user password"
     echo
     su root -c "$this_script $*";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t == 0 ]; then
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
  if [[ "$arg" == "-q" ]]; then quiet=1; fi
  if [[ "$arg" == "-nobt" ]]; then dont_install_bt=1; fi
  if [[ "$arg" == "-noprompt" ]]; then dont_prompt=1; fi
  if [[ "$arg" == "-nonet" ]]; then dont_restart_network=1; fi
done

setup_install_log

if [[ ! -e /etc/redhat-release ]]; then
  REPLY="N"
  if [[ $dont_prompt == 0 ]]; then
    read -n1 -p "warning - this script is only known to work on Redhat Enterprise Linux systems - continue? [yN] "
  else
    error "RSU configuration is only known to work on Redhat Enterprise Linux systems - exiting"
    exit 2;
  fi
  if [[ "$REPLY" != "y" && "$REPLY" != "Y" ]]; then 
    exit 2; 
  fi
fi

install_nmrwebd_components
if [[ $dont_install_bt == 0 ]]; then
  dongled=$(have_bt_dongle)
  # install the bluetooth dongle related softwared if we have a dongle
  # or it is ok to prompt the user to install it
  if [[ $dongled == 1 || $dont_prompt == 0 ]]; then
    install_bt_components
  fi
fi

if [[ $network_restart_required == 1 ]]; then
  if [[ $restart_network == 1 ]]; then
    if [[ $dont_restart_network == 0 ]]; then
      /sbin/service network restart
    else
      exit_code=4
    fi
  fi
fi

if [[ $quiet == 0 ]]; then
  echo
  echo "Remote Status Unit host setup complete"
  echo
fi

exit $exit_code
