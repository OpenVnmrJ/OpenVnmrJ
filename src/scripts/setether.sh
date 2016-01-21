: '@(#)setether.sh 22.1 03/24/08 1991-1997 '
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
: /bin/sh

#***********************************************************
#*                                                         *
#*   setether  -  connects the SUN system to the ethernet  *
#*                network                                  *
#*                                                         *
#*	Version for V4.1 of SunOS only		   	   *
#*      Extended to work with Solaris  09/1994             *
#***********************************************************

common_env() {

#  ostype:  IBM: AIX  Sun: SunOS or SOLARIS  SGI: IRIX

    ostype=`uname -s`

    if [ x$ostype = "xAIX" ]
    then
        osver=`uname -v`.`uname -r`
        svr4="y"
    else if [ x$ostype = "xIRIX" ]
	 then
             osver=`uname -r`
             svr4="y"
         else
             osver=`uname -r`
             osmajor=`echo $osver | awk 'BEGIN { FS = "." } { print $1 }'`
             if test $osmajor = "5"
             then
           	svr4="y"
                ostype="SOLARIS"
             else
          	svr4="n"
             fi
	 fi
    fi
}

#  All O/S's use "whoami" to get the name of the user ...  except Solaris

env_to_whoami() {
    if test x$ostype = "xSOLARIS"
    then
	id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'
    else
        whoami
    fi
}

nnl_echo() {
    if test x$svr4 = "x"
    then
        echo "error in echo-no-new-line: svr4 not defined"
        exit 1
    fi

    if test $svr4 = "y"
    then
        if test $# -lt 1
        then
            echo
        else
            echo "$*\c"
        fi
    else
        if test $# -lt 1
        then
            echo
        else
            echo -n $*
        fi
    fi
}

#  Main script starts here

common_env

if test `env_to_whoami` != "root"
then
    echo " "
    echo "Only Superuser (root) can alter ethernet status."
    echo " "
    exit 1
fi

if test x$ostype != "xSunOS" -a x$ostype != "xSOLARIS"
then
    echo " "
    echo "The `basename $0` command only works on Sun systems"
    echo " "
    exit 1
fi


if test x$ostype = "xSOLARIS"
then

#  Use -s switch with vnmr.ether_device to guard against a 0-length file

    cd /etc
    if test ! -s vnmr.ether_device
    then
        echo " "
        echo "You have not run the setnoether command yet"
        echo "You must run it first"
        echo " "
        exit 1
    fi

    if test ! -f hostname.lo0 -a ! -f hostname.xx0
    then
        echo " "
        echo "Error:  Neither /etc/hostname.lo0 or /etc/hostname.xx0 are present"
        echo "Correct before retrying" `basename $0`
        echo " "
        exit 1
    fi

# Preliminaries complete

    net_iface=`cat vnmr.ether_device`
    if test -f hostname.lo0
    then
        mv -f hostname.lo0 hostname.${net_iface}
    else
        mv -f hostname.xx0 hostname.${net_iface}
    fi
    if test -f defaultdomain.${net_iface}
    then
        mv -f defaultdomain.${net_iface} defaultdomain
    fi
    if test -f defaultrouter.${net_iface}
    then
        mv -f defaultrouter.${net_iface} defaultrouter
    fi

    echo " "
    echo "Host computer is back on the local area network"
    echo "UNIX must be rebooted to effect the change"
    echo " "
    exit 0
fi

#  Rest of the script is the original SunOS stuff...

if test -f /etc/vnmr.ether_device
then
   ether_device=`cat /etc/vnmr.ether_device`
else
   echo -n "Please enter name of ethernet interface (ie0, le0, etc.): "
   read ether_device
   if test x$ether_device = "x"
   then
      echo "No ethernet deice entered, $0 aborts"
      exit 1
   fi
   echo -n "Please confirm $ether_device is the ethernet interface [y/n]: "
   read ans
   if test x$ans != "x" -a x$ans != "xy"
   then
      echo "Ethernet interface not confirmed, $0 aborts"
      exit 1
   fi
fi

hostname=`/bin/hostname`

if test -f /etc/hostname.lo0
then
   mv /etc/hostname.lo0 /etc/hostname.$ether_device
else
   echo $hostname >/etc/hostname.$ether_device
fi

# Check for existing hosts file for ethernet

if test -f /etc/hosts.ether
then
   cp /etc/hosts.ether /etc/hosts
   echo " "
   echo "Host computer has been placed on the ethernet network."
   echo "UNIX must be re-booted to effect this change."
   echo " "
   exit
fi


# Make a basic /etc/hosts file for ethernet

rm /etc/hosts
cat >/etc/hosts << ++
# This is the most basic hosts file for machines that do have
# ethernet attached.
192.9.200.1	$hostname loghost

127.0.0.1	localhost
# End of file
++

echo " "
echo "Host computer has been placed on the ethernet network."
echo "The InterNet Address is 192.9.200.1"
echo "UNIX must be re-booted to effect the change."
echo " "
