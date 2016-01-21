#!/bin/sh
consolenic=eth0
vnic=${consolenic}:1
vaddr=169.254.0.254
vmask=255.255.0.0
vnet=169.254.0.0
nas=169.254.0.1
vcast=169.254.255.255
iam=`whoami`
quiet=0

msg()
{
  if [[ $quiet == 0 ]]; then
    echo "$*"
  fi
}

msg_()
{
  if [[ $quiet == 0 ]]; then
    echo -n "$*"
  fi
}

release_6_vnic()
{
  addr=`/sbin/ifconfig $vnic | awk '/inet addr/ { print $2 }' | sed 's/addr://'`
  nicfile=ifcfg-${consolenic}
  nicscript=/etc/sysconfig/network-scripts/$nicfile

  backup="/tmp/${nicfile}.orig"
  msg ".. backing up $nicscript to /tmp/"
  cp -p $nicscript $backup

  msg "checking if $vaddr is network manager controlled"
  nminscript=`grep -c NM_CONTROLLED $nicscript`
  msg ".. removing $consolenic from Network Manager Control"
  if [[ $nminscript > 0 ]]; then
    sed /NM_CONTROLLED/d $backup > $nicscript
  fi
  echo "NM_CONTROLLED=\"no\"" >> $nicscript

  nicinscript=`grep -c $vaddr $nicscript`
  msg "checking if $vaddr is already configured in $nicscript"
  if [[ $nicinscript > 0 ]]; then
    msg ".. $vaddr already exists in $nicscript"
  else
    msg ".. adding $vaddr to $nicscript"
    echo "IPADDR2=$vaddr" >> $nicscript
  fi

  msg "restarting network"
  /sbin/service network restart > /dev/null 2>&1

  if [[ -e $netmandir ]]; then
    msg "restarting NetworkManager"
    /sbin/service NetworkManager start > /dev/null 2>&1
  fi

  # bring it up manually in case $consolenic is configured with ONBOOT=no
  msg "bringing up console NIC $consolenic"
  /sbin/ifconfig $consolenic up 

  # check if it's configured properly
  msg_ "verifying that the network is configured for probeid.. "
  routecnt=`/sbin/ip route list | grep -c $vnet`
  if [[ $routecnt > 0 ]]; then
    msg " OK"
  else
    echo " error "
    echo "    network could not be configured for Probe ID"
    exit 1
  fi
}

# install_vnic - installs a virtual NIC, e.g. eth1:1
#      This stopped working with RHEL6.x, where NetworkManager interferes
#      with vnics.  The workaround also seems to work for RHEL 5.x, so
#      this is here for documentation purposes.
#
install_vnic()
{
  msg "checking if $vnic exists and is set to private probe network "
  addr=`/sbin/ifconfig $vnic | awk '/inet addr/ { print $2 }' | sed 's/addr://'`
  if [[ "$addr" == "$vaddr" ]]; then
    msg ".. $vnic already configured to $vaddr"
  else
    if [[ "$addr" != "" && "$addr" != "$vaddr" ]]; then
      REPLY=N
      read -n1 -p "   $vnic is already set to \"$addr\", reset to $vaddr? [yN] "
      if [[ "$REPLY" != "y" && "$REPLY" != "Y" ]]; then
        exit 1
      else
        msg ""
      fi
    fi
  fi

  script=/etc/sysconfig/network-scripts/ifcfg-${vnic}
  msg "checking if $script already exists"
  if [[ -e $script ]]; then
     REPLY=N
     read -n1 -p "   $script already exists, overwrite? [yN] "
     if [[ "$REPLY" != "y" && "$REPLY" != "Y" ]]; then
       exit 1
     fi
  fi

  msg_ "creating $script .. "
  cat > $script <<EOF
DEVICE=$vnic
BOOTPROTO=static
NM_CONTROLLED=no
IPADDR=$vaddr
NETMASK=$vmask
NETWORK=$vnet
BROADCAST=$vcast
ONBOOT=yes
EOF
  chmod 644 $script

  if [[ ! -e $script ]]; then
    echo "failed to create $script"
    exit 1
  else
    msg " OK"
  fi
}

# check if the script should be run silently (except errors)
if [[ $# > 0 && $1 == '-q' ]]; then
  quiet=1
  shift
fi

if [[ $# > 0 ]]; then
  vnic=$1
fi

if [[ $# > 1 ]]; then
  vaddr=$2
fi

if [[ "$iam" != "root" ]]; then
  echo `basename $0` "needs to be run via sudo or as root"
  exit 1
fi

if [[ ! -e /etc/redhat-release ]]; then
  REPLY="N"
  read -n1 -p "warning - this script is only known to work on a Redhat Enterprise Linux system - continue? [yN] "
  if [[ "$REPLY" != "y" && "$REPLY" != "Y" ]]; then 
    exit 1; 
  fi
fi

# check the most likely candidate
wormhole=wormhole
wormholeip=`gethostip $wormhole | awk '{print $2}'`
if [[ "$wormholeip" != "172.16.0.1" && "$wormholeip" != "10.0.0.1" ]]; then
  echo "console port has not been configured correctly - please run setacq first"
  exit 1
fi

msg "determining console network interface"
consolenic=""
for nic in `/sbin/ifconfig -a | awk '/^eth/ { print $1}' | grep -v :`; do
  addr=`/sbin/ifconfig $nic | awk '/inet addr/ { print $2}' | sed 's/addr://'`
  if [[ "$addr" == "$wormholeip" ]]; then
    msg ".. selected $nic for private probe network"
    consolenic=$nic
    vnic=${consolenic}:0
  fi
done
if [[ "$consolenic" == "" ]]; then
  echo "   console NIC ($wormhole) could not be determined - has it been configured?"
  exit 1
fi

msg "checking Redhat release"
rhel_rel=`cat /etc/redhat-release | sed -r 's/[^0-9]+//' | sed -r 's/[^0-9.]+$//'`
if [[ $rhel_rel < 6.0 ]]; then
  msg ".. older than release 6"
  if [[ $rhel_rel == 5.3 ]]; then
    msg ".. Redhat release is $rhel_rel"
    # was: install_vnic 
    install_vnic 
  else
    echo "  Probe ID is not supported for Redhat version $rhel_rel"
    exit 1
  fi
else
  msg ".. Redhat release is 6.0 or newer"
  release_6_vnic
fi

# check if we can reach our own interface
if [[ ! $rhel_rel < 6.0 ]]; then
  msg_ "verifying that $vaddr can be reached.. "
  pings=`/bin/ping -c 1 $vaddr | grep -c " 1 received"`
  if [[ $pings != 1 ]]; then
    echo " Probe ID VNIC is not responding!"
    exit 1
  else
    msg " OK"
  fi
fi

# check if we can reach probeid
msg_ "checking if probe server ($nas) can be reached.. "
pings=`/bin/ping -c 1 $nas | grep -c " 1 received"`
if [[ $pings != 1 ]]; then
  msg "ERROR"
  msg ".. probe server is not responding!"
  msg ".. please make sure it is powered on and attached to the network, then try again"
  exit 1
else
  msg " OK"
fi
