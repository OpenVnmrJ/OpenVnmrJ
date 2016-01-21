: '@(#)setnoether.sh 22.1 03/24/08 1991-1997 '
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

#**********************************************************
#*                                                        *
#*   setnoether  -  disconnects the SUN system from the   *
#*                  ethernet network                      *
#*                                                        *
#*	A version for V4.1 of SunOS only		  *
#*      Extended to work with Solaris  09/1994            *
#**********************************************************

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

#  Currently nnl_echo is only used with Solaris ...
#  although any version of O/S can make use of it

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

#  get net interface is currently only used on Solaris.
#  It is expected to work on SunOS too; however some
#  modification of the SunOS script itself would be
#  required
#
#  It is mostly an awk program which expects an input stream like:
#    hostname.le0
#    hostname.le1
#
#  That is what "ls -1 ..." serves to create.  The grep command
#  removes any line with "lo0" from the input.  The awk program
#  employs the "." as a field separator to isolate "le0", "le1",
#  etc.  It assumes the last character represents the unit number
#  and assigns the rest of the chars to the ethernet device name.
#  If it finds two different ethernet device names, it will print
#  an error message on standard error (That is what the cat 1>&2
#  is about) and exits.  This script will not work on a system
#  with (for example) le0 and ie1.  This may seem excessive, but
#  miscues here can render a system unbootable, so we want to be
#  careful.
#
#  The script catches the output from get net interface with
#  backquotes,  If an error occurs, no output is sent to standard
#  output, so the resulting "caught" value is the 0-length string.

get_net_iface() {
    ls -1 /etc/hostname.* | grep -v lo0 | grep -v xx0 | awk '
    BEGIN { FS="."; netdevice=""; error=0 }
    {
        devlen=length( $NF ) - 1
        if (netdevice == "")
          netdevice = substr( $NF, 1, devlen )
        else {
            newdevice = substr( $NF, 1, devlen )
            if (newdevice != netdevice) {
                print "Error:  Found " newdevice " and " netdevice | "cat 1>&2"
                error=1
                exit
            }
        }
    }
    END { if (error == 0) print netdevice }
    '
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
    numifs=`ls -1 /etc/hostname.* | grep -v lo0 | grep -v xx0 | wc -l`
    if test $numifs -le 0
    then
        echo " "
        echo "System not attached to the ethernet"
        echo " "
        exit 1
    fi
    if test $numifs -gt 2
    then
        echo " "
        echo "Too many ethernet interfaces:"
        ls -1 /etc/hostname.* | grep -v lo0 | grep -v xx0
        exit 1
    fi

#  Now we know either 1 or 2 EtherNet interfaces are present

    nnl_echo "Are any file systems mounted remotely on this system (y or n) [y]: "
    read remotefs
    if test x$remotefs != "xn"
    then
        echo " "
        echo "You must remove all file systems mounted remotely from /etc/vfstab"
        echo "before rebooting your system if you continue with" `basename $0`";"
        echo "otherwise your system may fail to boot up."
        echo " "
        nnl_echo "OK to continue? (y or n) [n]: "
        read ans
        if test x$ans != "xy"
        then
            echo `basename $0` "aborts"
            exit 1
        fi
    fi

    nnl_echo "Is this a GEMINI 2000, Mercury or INOVA spectrometer (y or n) [y]: "
    read g2000
    if test x$g2000 != "xn"
    then
        g2000="y"
    fi

    net_iface=`get_net_iface`
    if test x$net_iface = "x"
    then
        exit 1
    fi

    net_iface=${net_iface}0

    if test x$g2000 = "xy"
    then
        echo " "
        echo "Please select from the options below:"
        echo "1. Your SUN is attached to the console via the standard ethernet"
        echo "   port using 10baseT cable (twisted pair). You are not attached"
        echo "   to a local area network. Your SUN is a standalone system and"
        echo "   does not have NIS or NIS+ enabled."
        echo "2. Your SUN is attached to the console via the second BNC ethernet"
        echo "   port using a coax BNC type cable. You are (or will be) attached"
        echo "   to a local area network. Your SUN is a standalone system or may"
        echo "   have NIS or NIS+ enabled."
        echo " "
        nnl_echo "What is your configuration? (1 or 2) [1]: "
        read ans
        if test x$ans != "x2"
        then
            echo " "
            echo `basename $0` "command is not required"
            exit 1
        fi
    elif test $numifs -gt 1
    then
        echo " "
        echo "More than 1 ethernet interface," `basename $0` "cannot proceed"
        echo " "
        exit 1
    fi

    cd /etc
    if test ! -f hostname.${net_iface}
    then
        echo " "
        echo "Error: /etc/hostname.${net_iface} not present"
        echo "Correct before retrying" `basename $0`
        echo " "
        exit 1
    fi

#  Preliminaries complete

    echo ${net_iface} >vnmr.ether_device
    mv hostname.${net_iface} hostname.xx0
    if test -f defaultdomain
    then
        mv -f defaultdomain defaultdomain.${net_iface}
    fi
    if test -f defaultrouter
    then
        mv -f defaultrouter defaultrouter.${net_iface}
    fi

#  Do not modify /etc/hosts...

    echo " "
    echo "Host computer has been removed from the local area network"
    echo "UNIX must be rebooted to effect the change"

    exit 0
fi

#  Rest of the script is the original SunOS stuff...

echo -n "Please confirm that DECnet is NOT installed (enter y to continue): "
read ans
if test x$ans != "xy"
then
    echo "You did not enter y - $0 aborts"
    exit 1
fi

interface_names="`cat /etc/hostname.*                 2>/dev/null`"
if test -n "$interface_names"
then
(
        IFS="$IFS."
        set `echo /etc/hostname\.*`
	if test $# -gt 2
	then
		echo "More than 1 ethernet interface, $0 cannot proceed"
		exit 1
	fi

        shift
        echo $1
	if test $1 = "xx0" -o $1 = "lo0"
	then
		echo "System already disconnected from the ethernet"
		exit 0
	fi
	echo $1 >/etc/vnmr.ether_device
)
else
	echo "System already disconnected from the ethernet"
	exit 0
fi

mv /etc/hostname.* /etc/hostname.xx0
hostname=$interface_names

mv /etc/hosts /etc/hosts.ether
cat >/etc/hosts << ++
# This is the modified hosts file for machines that do not have
# ethernet attached.
127.0.0.1  $hostname localhost loghost
# End of file
++

echo " "
echo "Host computer has been removed from the ethernet network."
echo "UNIX must be re-booted to effect the change."
echo "Before re-booting UNIX, be sure to deactivate yellow pages"
echo "if it has been installed."
echo " "
