#! /bin/sh
: '@(#)isetacq.sh 22.1 03/24/08 1991-1997 '
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

make_etc_files () {
    # this creates the acqpresent file in /etc
    cat $Null > /etc/acqpresent

    # this deletes the statpresent file in /etc. This is not needed for 
    # INOVA. Because we created it at one point, we make extra sure it isn't
    # there anymore
    if [ -f /etc/statpresent ]
    then
        rm -f /etc/statpresent
    fi
}

#common for systems with or without router box
fix_common_things () {
    
    #always fix bootptab for now
    fix_bootptab="y"

    #-----------------------------------------------------------------
    # add the hosts.equiv to give vxWorks acces to files
    # hosts.equiv additions do not need a reboot
    #-----------------------------------------------------------------
    hostsequiv="/etc/hosts.equiv"
    if [ ! -f $hostsequiv ]
    then
       echo $cons_name > $hostsequiv
       echo $cons_name_auto >> $hostsequiv
    else
       grep -w $cons_name $hostsequiv > $Null
       if [ $? -ne 0 ]
       then
          echo $cons_name >> $hostsequiv
       fi
       grep -w $cons_name_auto $hostsequiv > $Null
       if [ $? -ne 0 ]
       then
          echo $cons_name_auto >> $hostsequiv
       fi
    fi

    #-----------------------------------------------------------------
    # fix /etc/inetd.conf so tftpd gets started
    #-----------------------------------------------------------------
    inetd_conf="/etc/inetd.conf"

    grep -w "#tftp" $inetd_conf > $Null
    if [ $? -eq 0 ]
    then
       chmod +w $inetd_conf
       cat $inetd_conf | sed -e 's/#tftp/tftp/' > $tmpfile
       cp $tmpfile $inetd_conf
       rm $tmpfile
       chmod -w $inetd_conf
       reboot=1
    fi

    #-----------------------------------------------------------------
    # fix /etc/nsswitch.conf to continue looking locally
    # this is done whether NIS is enabled or not, if not enabled 'return'
    # won't be found. The assumption is that if all other files are OK
    # (i.e. roboot=0) that this is ok, reboot is not set here.
    #-----------------------------------------------------------------
    nsswitch_conf="/etc/nsswitch.conf"
    chmod +w $nsswitch_conf
    cat $nsswitch_conf | sed -e                                                 \
    's/hosts:      nis \[NOTFOUND=return\]/hosts:      nis \[NOTFOUND=continue\]/' |\
                         sed -e                                                 \
    's/hosts:      xfn nis \[NOTFOUND=return\]/hosts:      xfn nis \[NOTFOUND=continue\]/' |\
                         sed -e                                                 \
    's/ethers:     nis \[NOTFOUND=return\]/ethers:     nis \[NOTFOUND=continue\]/' |\
                        sed -e                                                  \
    's/bootparams: nis \[NOTFOUND=return\]/bootparams: nis \[NOTFOUND=continue\]/'  \
                                                        > $tmpfile
    cp $tmpfile $nsswitch_conf
    rm $tmpfile
    chmod -w $nsswitch_conf

    #-----------------------------------------------------------------
    # copy bootptab (Done no matter what, in case CPU (=ethernet address) was changed)
    # and substitute the received ethernet address
    #-----------------------------------------------------------------
    etc_bootptab="/etc/bootptab"
    vnmr_bootptab="${vnmrsystem}/acq/bootptab"

    cp $vnmr_bootptab $etc
    cat $etc_bootptab | sed -e \
            "s/08003E236BC4/${cons_macaddr}/" > $tmpfile
    cp $tmpfile $etc_bootptab
    rm $tmpfile
    if [ x$fix_bootptab = "xy" ]
    then
    cat $etc_bootptab | sed -e 's/ip=10.0.0.2/ip='${cons_subnet}'.2/' | \
                        sed -e 's/ip=10.0.0.4/ip='${cons_subnet}'.4/'  > $tmpfile
    cp $tmpfile $etc_bootptab
    rm $tmpfile
    fi

    #-----------------------------------------------------------------
    # create /tftpboot if needed, copy vxBoot from /vnmr/acq
    #-----------------------------------------------------------------
    vxboot_dir="/tftpboot/vxBoot"

    if [ ! -d $vxboot_dir ]
    then
        mkdir -p $vxboot_dir
        reboot=1
    fi

    if [ ! -h /tftpboot/tftpboot ]
    then
       cd /tftpboot
       ln -s . tftpboot
    fi
    rm -f ${vxboot_dir}/*
    cp -p $vnmrsystem/acq/vxBoot/vxWorks*  $vxboot_dir

    /usr/xpg4/bin/grep -q -E 'mercvx|mercplus' /vnmr/vnmrrev
    if [ $? -ne 0 ]
    then
       cp -p $vnmrsystem/acq/vxBootPPC/vxWorks           ${vxboot_dir}/vxWorksPPC
       if [ -f $vnmrsystem/acq/vxBootPPC/vxWorks.sym ]
       then
           cp -p $vnmrsystem/acq/vxBootPPC/vxWorks.sym   ${vxboot_dir}/vxWorksPPC.sym
       fi
       cp -p $vnmrsystem/acq/vxBoot.auto/vxWorks.auto    ${vxboot_dir}
    fi

    #-----------------------------------------------------------------
    # add two lines to the services file if needed
    # these two entries are used by the bootpd deamon
    #-----------------------------------------------------------------
    etc_services="/etc/services"
    grep -w bootps $etc_services > $Null
    if [ $? -ne 0 ]
    then
       chmod +w $etc_services
       head -26 $etc_services > $tmpfile
       echo "bootps         67/udp          #bootpd server" >>  $tmpfile
       echo "bootpc         68/udp          #bootpd client" >>  $tmpfile
       tail +27 $etc_services >> $tmpfile
       cp $tmpfile $etc_services
       rm $tmpfile
       chmod -w $etc_services
       reboot=1
    fi

    #-----------------------------------------------------------------
    # Arrange for Acqproc to start at system bootup
    #-----------------------------------------------------------------
    cp -p ${vnmrsystem}/rc.vnmr /etc/init.d
    (cd /etc/rc3.d; if [ ! -h S19rc.vnmr ]; then
        ln -s ../init.d/rc.vnmr S19rc.vnmr; fi)
    (cd /etc/rc0.d; if [ ! -h K19rc.vnmr ]; then
        ln -s ../init.d/rc.vnmr K19rc.vnmr; fi)
    make_etc_files

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
    #-----------------------------------------------------------------
    rm -f /tmp/ExpQs
    rm -f /tmp/ExpActiveQ
    rm -f /tmp/ExpStatus
    rm -f /tmp/msgQKeyDbm
    rm -f /tmp/ProcQs
    rm -f /tmp/ActiveQ
}

do_router () {

    cons_subnet=172.16.0
    host_name=`uname -n`

    #Checking for how many hostname.xxx around
    hndot_cnt=`ls -l /etc/hostname.* | wc -l`
    if [ $hndot_cnt -eq 1 ]
    then
        grep -w $host_name /etc/hostname.* 2>&1 > $Null
        if [ $? -ne 0 ]
        then
            #this is the only hostname.xxx, but somehow it got wrong content, will fix it
            cat $host_name > /etc/hostname.*
        fi
        console_Dev=`ls /etc/hostname.* | awk '{FS="."} {print $2}'`
        eport_used4_console=1

    else      #two or more hostname.xxx
        cnt=0
        aa=`ls /etc/hostname.*`
        for etc_hndot in $aa
        do
              hn_dot=`basename $etc_hndot`
              val=`cat $etc_hndot`
              case x$val in
                  x$host_name )
                                cnt=`expr $cnt + 1`
                                console_Dev=`ls $etc_hndot | awk '{FS="."} {print $2}'`
                                #eport_used4_console=1  might not be relevent
			        ;;

		  "xwormhole")
				#remove this hostname.xxx
			        rm -f  $etc_hndot
			        ;;
              esac
        done

        if [ $cnt -ne 1 ]
        then
            echo "This system is not properly setting up"
            echo "Makinng sure /etc/hostname.xxx has proper setting"
            exit
        fi
    fi

    #All of the below just to get console MAC address for bootptab
    #NMR private subnet special
    rter_mac=
    while [ -z "$rter_mac" ]
    do
       ping 172.16.0.1 2>&1 > $Null
       if [ $? -eq 0 ]
       then
          rter_mac=`/usr/sbin/arp 172.16.0.1 | awk '{
             arrlen=split($4,arr,":")
             for (i=1; i<=arrlen; i++) {
                if (length(arr[i]) == 1)
                   macstr=macstr "0" arr[i]
                else
                   macstr=macstr arr[i]
             }
             print macstr
          }'`
       fi
    done

    while [ x$cons_macaddr = "x080023456789" -o x$cons_macaddr = x$rter_mac ]
    do
       cons_macaddr=`${vnmrsystem}/bin/catcheaddr $console_Dev INOVA`
       #echo "cons_macaddr=$cons_macaddr --------"
       sleep 1
    done

    #-----------------------------------------------------------------
    # add to /etc/hosts if needed, /etc/hosts should always exist
    #-----------------------------------------------------------------
    etc_hosts="/etc/hosts"
    grep -w $host_name $etc_hosts 2>&1 > $Null
    if [ $? -ne 0 ]
    then
        chmod +w $etc_hosts
        echo "172.16.0.11\t$host_name loghost" >> $etc_hosts
        reboot=1
        chmod -w $etc_hosts
    fi

    aa=`grep -w "inova" $etc_hosts | awk '{print $1}'`
    if [ x$aa != "x172.16.0.2" ]
    then
        chmod +w $etc_hosts
        grep -vw inova $etc_hosts > /tmp/hosts
        mv /tmp/hosts $etc_hosts

        echo "${cons_subnet}.2\t$cons_name" >> $etc_hosts
        reboot=1
        chmod -w $etc_hosts
    fi

    bb=`grep -w "inovaauto" $etc_hosts | awk '{print $1}'`
    if [ x$bb != "x172.16.0.4" ]
    then
        chmod +w $etc_hosts
        grep -vw inovaauto $etc_hosts > /tmp/hosts
        mv /tmp/hosts $etc_hosts

        echo "${cons_subnet}.4\t$cons_name_auto" >> $etc_hosts
        reboot=1
        chmod -w $etc_hosts
    fi

    # Insert entries for all known peripheral devices.
    # List of subdomain addresses for Inova and hostnames.
    peripheral_list="249,V-Autosampler\
                     51,V-Cryobay\
	             101,V-AS768\
	             100,V-AS768Robot\
                     50,V-LcWorkstation\
	             246,V-Pda\
	             70,V-Protune\
	             43,V-Slim\
	             71,V-Trap"
    for hostline in $peripheral_list
    do
        newip=${cons_subnet}.`echo $hostline | awk -F, '{print $1}'`
	host=`echo $hostline | awk -F, '{print $2}'`
	oldip=`grep -wi $host $etc_hosts | awk '{print $1}'`
	if [ x$oldip != x$newip ]
	    then
            # If there is a previous entry, it's not what we expect.
	    # The idea here is that if a peripheral is already set up in
	    # a non-standard way and seems to be working, leave it alone.
	    # Of course, the peripheral must be on when setacq is run.
	    if [ x$oldip = "x" ]
		then
		neednewip=1	# No old entry; need new one.
	    else
                ping $oldip 1 2>&1 > $Null
		neednewip=$?	# No response at old address? Replace it.
	    fi
	    if [ $neednewip -ne 0 ]
	    then
		chmod +w $etc_hosts
		grep -wiv $host $etc_hosts > /tmp/hosts
		echo "$newip\t$host" >> /tmp/hosts
		mv /tmp/hosts $etc_hosts
		chmod -w $etc_hosts
		# NB: No need to reboot for these
	    fi
	fi
    done

    grep -w "wormhole" $etc_hosts 2>&1 > $Null
    if [ $? -eq 0 ]
    then
        chmod +w $etc_hosts
        grep -vw wormhole $etc_hosts > /tmp/hosts
        mv /tmp/hosts $etc_hosts
        chmod -w $etc_hosts
    fi

    fix_common_things

    echo ""
    echo "NMR Console software installation complete"

    #-----------------------------------------------------------------
    # Do we need to reboot the SUN? If not, restart bootpd
    #-----------------------------------------------------------------
    if [ $reboot -eq 1 ]
    then
        echo " "
        echo "You must reboot Solaris for these changes to take effect"
        echo "As root type 'reboot' to reboot Solaris"
    else
        if [ -f ${vnmrsystem}/acqbin/bootpd ]
        then
           (${vnmrsystem}/acqbin/bootpd -s > /dev/console &)
        fi
        /vnmr/bin/execkillacqproc
    fi
}
 
# Main main MAIN
#

# Operating System               
# Note that Solaris (on sun hardware) calls itself SunOS
# use OS revision to distinguish between the two
# This sould be Solaris ONLY
                                 
OS_NAME=`uname -s`               
if (test ! $OS_NAME = "SunOS")   
then                             
   echo "$0 suitable for Sun-based systems only"
   echo "$0 exits"               
   exit 0                        
fi                               

cons_name="inova"
cons_name_auto="inovaauto"
cons_subnet="10.0.0"
enet_port2_name="wormhole"
reboot=0
Null="/dev/null"
tmpfile=/tmp/tmp_please_remove
eport_used4_console=
export cons_name cons_name_auto cons_subnet enet_port2_name reboot
export Null tmpfile eport_used4_console

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
userId=`/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ $userId != "uid=0(root)" ]; then
  notroot=1
  echo
  echo "To install VNMR you will need to be the system's root user."
  echo "Or type cntrl-C to exit.\n"
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password \n"
     su root -c "/vnmr/bin/setacq ${ARGS}";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again \n"
  fi
  exit
fi
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

if [ -f $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto ]
then
    #Ask if the mts version of MSR is needed
    rm -f $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
    echo "Is this an imaging console with MTS amplifiers, y/n? [n]: \c"
    read mts
    if [ x$mts = "xy" -o x$mts = "xyes" ]
    then
      cp $vnmrsystem/acq/vxBoot.auto/mts.vxWorks.auto $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
    else
      cp $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
    fi
fi

#-----------------------------------------------------------------
# Ask the user to reboot the console now,
#-----------------------------------------------------------------
echo " "
echo "Please reset the console."
echo "Enter a return after pressing the console reset button: \c"
read ans
echo " "


#user do "setacq router"
if [ $# -eq 1 x$1 = "xrouter" ]
then
      do_router
      exit
else
    #Ask if there is a router box, split from here
    echo "Are the NMR console and Sun computer attached to a firewall-router, y/n? [n]: \c"
    read rter
    if [ x$rter = "xy" -o x$rter = "xyes" ]
    then
          #do the router things then exit 
          do_router
          exit
    fi
fi

#-----------------------------------------------------------------
# To support both the 10 Mbit ethernet and 100 Mbit ethernet
# we must distinguish between the hostname.le? and hostname.hme?
# If /etc/hostname.hme0 exists the primary net is 100/10 Mbit
# autosensing, and the secondary net will be in /etc/hostname.le0
# Otherwhise the names are /etc/hostname.le0 and /etc/hostname.le1
# If the second net ever becomes 100 Mbit, we have to ask.
#
# UltraPCI: the prtconf command failed to report the name of the
# ethernet interface(s).  So we invoked an old rule: when you
# want something done right, do it yourself, and wrote a program,
# findedevices, that probes for possible ethernet interfaces
#
# Next, as of 2/14/01 we must support the eri device. Lots of 
# choices, lots of permutations. The solution does not cover 
# all possibilities (eg, three devices) but is easily adjusted
#
# Blade 1500 introduces the bge Ethernet device
# GigaSwift 10/100/1000 Ethernet introduces the Ethernet ce device
#
#-----------------------------------------------------------------
n_bge=`$vnmrsystem/bin/findedevices /dev/bge | wc -l` 
n_ce=`$vnmrsystem/bin/findedevices /dev/ce | wc -l` 
# the next line is to work around a bug, where if there is no /dev/ce
# the system claims there are 5; certain Solaris 8 version only
if [ $n_ce -gt 4 ] 
then
    n_ce=0
fi
n_eri=`$vnmrsystem/bin/findedevices /dev/eri | wc -l` 
n_hme=`$vnmrsystem/bin/findedevices /dev/hme | wc -l` 
case $n_bge in
  *2) driver1="bge0"
      driver2="bge1"
      ;;
  *1) driver1="bge0"
      if [ $n_eri -gt 0 ]
      then
        driver2="eri0"
      elif [ $n_ce -gt 0 ]
        then
          driver2="ce0"
        elif [ $n_hme -gt 0 ]
          then
            driver2="hme0"
          else
            driver2="le0"
      fi
      ;;
  *)
      case $n_eri in
        *2) driver1="eri0"
            driver2="eri1"
            ;;
        *1) driver1="eri0"
            if [ $n_ce -gt 0 ]
            then
              driver2="ce0"
            elif [ $n_hme -gt 0 ]
              then
                driver2="hme0"
              else
                driver2="le0"
            fi
            ;;
         *)
            case $n_hme in
              *2) driver1="hme0"
                  driver2="hme1"
                  ;;
              *1) driver1="hme0"
                  driver2="le0"
                  ;;
              *)
                  driver1="le0"
                  driver2="le1"
                  ;;
            esac
            ;;
      esac
      ;;
esac

#-----------------------------------------------------------------
# select le0 or le1 as interface
#-----------------------------------------------------------------

echo "Select from the options below:"
echo "1. Your SUN is attached to the console via the standard ethernet"
echo "   port"
echo "2. Your SUN is attached to the console via the second ethernet"
echo "   port."
echo " "
echo "What is your configuration? (1 or 2) [2]: \c"
read eport_used4_console
if ( test x$eport_used4_console != "x1" )
then
   eport_used4_console=2
fi

#-----------------------------------------------------------------
# check that the default IP is not the main net IP 
# Because VxWorks does not use 'bootptab' parameters 
# right away, for now we use 120.0.0
#-----------------------------------------------------------------
host_net=`grep -v '^#' $etc/hosts | grep -w \`uname -n\` | awk 'BEGIN { FS = "." } \
                { printf("%d.%d.%d",$1,$2,$3) }'`
first_net_byte=`echo $host_net | awk 'BEGIN { FS = "." } \
                { printf("%d",$1) }'`
fix_bootptab="y"
if [ x$eport_used4_console = "x2" ] 
then
   if [ x$first_net_byte = "x10" ]
   then
      cons_subnet=172.16.0
      fix_bootptab="y"
   fi
else
   cons_subnet=$host_net
fi

#-----------------------------------------------------------------
# check hostname.le1, create if needed
#-----------------------------------------------------------------
if [ x$eport_used4_console = "x2" ]
then
   if [ -f $etc/hostname.$driver2 ]
   then
      enet_port2_name=`cat $etc/hostname.$driver2`
   else
      echo $enet_port2_name > $etc/hostname.$driver2

      # next two commands "turn on" the ethernet interface,
      # so catcheaddr can receive the broadcast RARP packet
      # ifconfig commands taken from /etc/init.d/rootusr

      /sbin/ifconfig $driver2 plumb
      /sbin/ifconfig $driver2 inet "${cons_subnet}.1" netmask + \
						broadcast + -trailers up \
						2>&1 > /dev/null
      reboot=1
   fi
else
     enet_port2_name=`uname -n`
fi

# catch the ethernet address
if [ x$eport_used4_console = "x1" ]
then
   cons_macaddr=`$vnmrsystem/bin/catcheaddr $driver1 INOVA`
else
   cons_macaddr=`$vnmrsystem/bin/catcheaddr $driver2 INOVA`
fi

#-----------------------------------------------------------------
# Because we have an install base with 120.0.0.0 as the console IP
# address, we must delete those entries in /etc/hosts
# We will then replace the IP net with 10.0.0.0, as specified in
# RFC1597 the following net numbers are reserved for private networks
# 10.0.0.0	- 10.255.255.255
# 172.16.0.0	- 172.31.255.255
# 192.168.0.0	- 192.168.255.255
# If the replacement is done, one needs to reboot the SUN to make
# it effective
#-----------------------------------------------------------------
if [ x$eport_used4_console = "x2" ]
then
   tmp=`grep -v '^#' $etc/hosts | grep -w inova |  awk 'BEGIN { FS = "." } \
                { printf("%d.%d.%d",$1,$2,$3) }'`
   if [ x$tmp != "x" ]
   then
      if [ $tmp != $cons_subnet ]
      then
         grep -v $tmp $etc/hosts > /tmp/hosts
         cp /tmp/hosts $etc/hosts
         rm /tmp/hosts
         reboot=1
      fi
   fi
fi

#-----------------------------------------------------------------
# add to /etc/hosts if needed, /etc/hosts should always exist
#-----------------------------------------------------------------
chmod +w $etc/hosts
grep -v '^#' $etc/hosts | grep -w $enet_port2_name > /dev/null
if [ $? -ne 0 ]
then
   echo "${cons_subnet}.1\t$enet_port2_name" >> $etc/hosts
   reboot=1
fi
grep -v '^#' $etc/hosts | grep -w $cons_name > /dev/null
if [ $? -ne 0 ]
then
   echo "${cons_subnet}.2\t$cons_name" >> $etc/hosts
   reboot=1
fi
grep -v '^#' $etc/hosts | grep -w $cons_name_auto > /dev/null
if [ $? -ne 0 ]
then
   echo "${cons_subnet}.4\t$cons_name_auto" >> $etc/hosts
   reboot=1
fi
chmod -w $etc/hosts

#-----------------------------------------------------------------
# add the hosts.equiv to give vxWorks acces to files
# hosts.equiv additions do not need a reboot
#-----------------------------------------------------------------
if [ ! -f $etc/hosts.equiv ]
then
   echo $cons_name > $etc/hosts.equiv
   echo $cons_name_auto >> $etc/hosts.equiv
else
   grep -w $cons_name $etc/hosts.equiv > /dev/null
   if [ $? -ne 0 ]
   then
      echo $cons_name >> $etc/hosts.equiv
   fi
   grep -w $cons_name_auto $etc/hosts.equiv > /dev/null
   if [ $? -ne 0 ]
   then
      echo $cons_name_auto >> $etc/hosts.equiv
   fi
fi

#-----------------------------------------------------------------
# fix /etc/inetd.conf so tftpd gets started
#-----------------------------------------------------------------
grep -w "#tftp" $etc/inetd.conf > /dev/null
if [ $? -eq 0 ]
then       
   chmod +w $etc/inetd.conf
   cat $etc/inetd.conf | sed -e 's/#tftp/tftp/' > $etc/inetd.conf.tmp
   cp $etc/inetd.conf.tmp $etc/inetd.conf
   rm $etc/inetd.conf.tmp
   chmod -w $etc/inetd.conf
   reboot=1
fi

#-----------------------------------------------------------------
# fix /etc/nsswitch.conf to continue looking locally
# this is done whether NIS is enabled or not, if not enabled 'return'
# won't be found. The assumption is that if all other files are OK
# (i.e. roboot=0) that this is ok, reboot is not set here.
#-----------------------------------------------------------------
chmod +w $etc/nsswitch.conf
cat $etc/nsswitch.conf | sed -e							\
's/hosts:      nis \[NOTFOUND=return\]/hosts:      nis \[NOTFOUND=continue\]/' |\
                         sed -e 						\
's/hosts:      xfn nis \[NOTFOUND=return\]/hosts:      xfn nis \[NOTFOUND=continue\]/' |\
                         sed -e 						\
's/ethers:     nis \[NOTFOUND=return\]/ethers:     nis \[NOTFOUND=continue\]/' |\
			sed -e							\
's/bootparams: nis \[NOTFOUND=return\]/bootparams: nis \[NOTFOUND=continue\]/'  \
							> $etc/nsswitch.conf.tmp
cp $etc/nsswitch.conf.tmp $etc/nsswitch.conf
rm $etc/nsswitch.conf.tmp
chmod -w $etc/nsswitch.conf

#-----------------------------------------------------------------
# copy bootptab and substitute the received ethernet address
# Done no matter what, in case CPU (=ethernet address) was changed
#-----------------------------------------------------------------
cp $vnmrsystem/acq/bootptab $etc
cat $etc/bootptab | sed -e \
	"s/08003E236BC4/${cons_macaddr}/" > $etc/bootptab.tmp
cp $etc/bootptab.tmp $etc/bootptab
rm $etc/bootptab.tmp
if [ x$fix_bootptab = "xy" ]
then
cat $etc/bootptab | sed -e 's/ip=10.0.0.2/ip='${cons_subnet}'.2/' | \
                    sed -e 's/ip=10.0.0.4/ip='${cons_subnet}'.4/'  > $etc/bootptab.tmp
cp $etc/bootptab.tmp $etc/bootptab
rm $etc/bootptab.tmp
fi

#-----------------------------------------------------------------
# create /tftpboot if needed, copy vxBoot from /vnmr/acq
#-----------------------------------------------------------------
if [ ! -d /tftpboot/vxBoot ]
then
   mkdir -p /tftpboot/vxBoot
   reboot=1
fi
if [ ! -h /tftpboot/tftpboot ]
then
   cd /tftpboot
   ln -s . tftpboot
fi
rm -f /tftpboot/vxBoot/*
cp -p $vnmrsystem/acq/vxBoot/vxWorks*           /tftpboot/vxBoot
/usr/xpg4/bin/grep -q -E 'mercvx|mercplus' /vnmr/vnmrrev
if [ $? -ne 0 ]
then 
   cp -p $vnmrsystem/acq/vxBootPPC/vxWorks         /tftpboot/vxBoot/vxWorksPPC
   if [ -f $vnmrsystem/acq/vxBootPPC/vxWorks.sym ]
   then
      cp -p $vnmrsystem/acq/vxBootPPC/vxWorks.sym     /tftpboot/vxBoot/vxWorksPPC.sym
   fi
   cp -p $vnmrsystem/acq/vxBoot.auto/vxWorks.auto /tftpboot/vxBoot
fi

#-----------------------------------------------------------------
# add two lines to the services file if needed
# these two entries are used by the bootpd deamon
#-----------------------------------------------------------------
grep -w bootps $etc/services > /dev/null
if [ $? -ne 0 ]
then
   chmod +w $etc/services
   head -26 $etc/services > $etc/services.tmp
   echo "bootps		67/udp		#bootpd server" >>  $etc/services.tmp
   echo "bootpc		68/udp		#bootpd client" >>  $etc/services.tmp
   tail +27 $etc/services >> $etc/services.tmp
   cp $etc/services.tmp $etc/services
   rm $etc/services.tmp
   chmod -w $etc/services
   reboot=1
fi
   

#-----------------------------------------------------------------
# Arrange for Acqproc to start at system bootup
#-----------------------------------------------------------------
    cp -p $vnmrsystem/rc.vnmr $etc/init.d
    (cd $etc/rc3.d; if [ ! -h S19rc.vnmr ]; then 
        ln -s ../init.d/rc.vnmr S19rc.vnmr; fi)
    (cd $etc/rc0.d; if [ ! -h K19rc.vnmr ]; then
        ln -s ../init.d/rc.vnmr K19rc.vnmr; fi)
    make_etc_files
#-----------------------------------------------------------------
# Check if we need to create a default router
# This would stop in.routed from being started at bootup
#-----------------------------------------------------------------
if [ x$eport_used4_console = "x2" ] 
then
   if [ -f $etc/defaultrouter ]
   then
       n=`wc -l $etc/defaultrouter | awk '{ printf("%d",$1) }'`
       if [ $n -ne 0 ]
       then
          echo ""
       else
         reboot=1
         line=`netstat -r -n | grep default`
         echo $line | awk 'BEGIN { FS = " " } { print $2 }' > $etc/defaultrouter
       fi
   else
      reboot=1
      line=`netstat -r -n | grep default`
      echo $line | awk 'BEGIN { FS = " " } { print $2 }' > $etc/defaultrouter
   fi
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
# Do we need to reboot the SUN? If not, restart bootpd
#-----------------------------------------------------------------
if [ $reboot -eq 1 ]
then
   echo " "
   echo "You must reboot Solaris for these changes to take effect"
   echo "As root type 'reboot' to reboot Solaris"
else
   if [ -f $vnmrsystem/acqbin/bootpd ]
   then
      ($vnmrsystem/acqbin/bootpd -s > /dev/console &)
   fi
   /vnmr/bin/execkillacqproc
fi


