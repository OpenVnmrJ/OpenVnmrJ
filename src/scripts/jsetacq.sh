: '@(#)jsetacq.sh 22.1 03/24/08 1991-1994 '
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

vnmrsystem=/vnmr
etc=/etc
nic_name=$1

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

get_nic() {

   nic_strg=`  awk ' BEGIN  {  FS="\""
                               numCard=0
                            }
   
                            {  if ($4=="le" || $4=="hme")
                               {
                                  for (k=1; k<=length($3); k++)
                                  {
                                     refchar = substr($3,k,1)
                                     if ( refchar ~ /[0-9]/ )
                                     {
                                        startChar = k;
                                        break
                                     }
                                  }   
                                  s = sprintf("%s%s",$4, substr($3,startChar))
                                  nicList= nicList s " "
                                  numCard ++
                               }
                            }   
   
                      END   { print   numCard " " nicList }
   
                    ' < /etc/path_to_inst
            `
   #echo "nic_strg= $nic_strg"
}

do_admin() {

   case `uname -r` in
       4*) user=`whoami`
           ;;
       5*) user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
           ;;
   esac
 
   if [ ! x$user = "xroot" ]
   then
      echo "Please login as root"
      echo "then restart $0"
      exit 1
   fi

   OS_NAME=`uname -s`               
   if [ ! $OS_NAME = "SunOS" ]   
   then                             
      echo "$0 suitable for Sun-based systems only"
      echo "$0 exits"               
      exit 1                        
   fi                               

   ps -e | grep Expproc > /dev/null
   if [ x$? = "x0" ]
   then 
      echo ""
      /vnmr/bin/execkillacqproc
   fi

   ps -e | grep Vnmr > /dev/null
   if [ x$? = "x0" ]
   then
      echo "\nExiting Vnmr..............\n"
      stop_program Vnmr
   fi


   #kill bootpd, otherwhise otherwhise the console will reboot
   #with the old files (can't delete /tftpboot/vxWorks!)
   stop_program bootpd
}

#-----------------------------------------------------------------
# Main main MAIN
#-----------------------------------------------------------------

#echo "\nNIC name= $1 --------"
#
#echo "NIC num= $2 --------"
#
#echo "\nTEST1 from isetacq"
#sleep 5
#
#echo TEST2 from isetacq
#sleep 5
#
#echo TEST3 from isetacq
#
#echo ""
#echo "NMR Console setting up complete"
#
#exit


#-----------------------------------------------------------------
# Ask the user to reboot the console now,
# then select the interface
#-----------------------------------------------------------------
nic_strg=""
e_driver=""

if [ $# -eq 0 ]
then
   do_admin
   get_nic #place nic's infos to nic_string

   if [ -n $nic_strg ]
   then
      num_nic=` echo $nic_strg | awk ' {print $1} ' `
      nic_list=` echo $nic_strg | sed 's/'$num_nic'//' `

      if [ x$num_nic = "x" ]
      then
         echo "No network card found, Please check your system"
         echo "Exiting this program .........................."
         exit
      else
         if [ x$num_nic = "x1" ]
         then
            
            echo "This host has only 1 network card\n"
            echo "The NMR console is attached to the Host computer via $nic_list ? (n,y) [y]: \c"
            read input
            if [ x$input != "xn" ]
            then 
               e_driver=$nic_list
            else 
               echo "Exiting this program ..........."
               exit
            fi
         else
             input_stat=0
             echo " "
             echo "\n>>>>>>>>>> Please reboot the NMR console. <<<<<<<<<<<"
             echo " "
             echo "Then select from the options below,"
             echo "the NMR console is attached to the Host computer via: $nic_list"
             echo " "

             while [ $input_stat -eq 0 ]
             do
                echo "What is your configuration? : \c"
                read e_driver
                echo " "
   
                for driver in $nic_list
                do 
                   if [ x$e_driver = x$driver ]
                   then
                      input_stat=1
                   fi
                done
             done
         fi
      fi
   fi
else
   if [ $# -eq 2 ]  ###called from Java interface
   then
      do_admin
      #echo "\n>>>>>>>>>> Please reboot the NMR console. <<<<<<<<<<<"
      e_driver=$1
      num_nic=$2
   else
      exit
   fi
fi

#-----------------------------------------------------------------
# If only 1 NIC in the host, we use host name and host ip for NIC
# If multiple NIC, We use wormhole for NIC's name and either 10.0.0.1
#                                  or 172.16.0.1 for NIC's ip address
# Specified by RFC1597 the following IP numbers are reserved for private networks
# 10.0.0.0     -  10.255.255.255
# 172.16.0.0   -  172.31.255.255
# 192.168.0.0  -  192.168.255.255
#-----------------------------------------------------------------

cons_net_name="inova"         #fixed name
cons_auto_name="inovaauto"    #fixed name
 
sun_reboot=0
fix_bootptab="n"

host_ip=`grep -w \`uname -n\` $etc/hosts | awk ' { print $1 } '`
host_ip_firstbyte=`echo $host_ip | awk 'BEGIN { FS = "." } { print $1 }'`
host_ip_lastbyte=`echo $host_ip | awk 'BEGIN { FS = "." } { print $4 }'`
partial_host_ip=`echo $host_ip | awk 'BEGIN { FS = "." } { printf("%d.%d.%d",$1,$2,$3) }'`

if [ x$num_nic = "x1" ]
then 

   partial_ip=$partial_host_ip

   nmr_netcard_name=`uname -n`
   nmr_netcard_ip=$host_ip
   cons_net_ip=${partial_ip}.`expr $host_ip_lastbyte + 1`
   cons_auto_ip=${partial_ip}.`expr $host_ip_lastbyte + 3`
   fix_bootptab="y"

else 
   if [ x$host_ip_firstbyte = "x10" ]   #10.0... already used for main net
   then
      partial_ip=172.16.0
      fix_bootptab="y"
   else
      partial_ip="10.0.0"
   fi

   nmr_netcard_ip=${partial_ip}.1   
   cons_net_ip=${partial_ip}.2
   cons_auto_ip=${partial_ip}.4

   if [ -f $etc/hostname.$e_driver ]
   then
      nmr_netcard_name=`cat $etc/hostname.$e_driver`
   else
      nmr_netcard_name="wormhole"
      echo $nmr_netcard_name > $etc/hostname.$e_driver

      #next two commands "turn on" the ethernet interface,
      #so catcheaddr can receive the broadcast RARP packet
      #ifconfig commands taken from /etc/init.d/rootusr
      /sbin/ifconfig $e_driver plumb
      /sbin/ifconfig $e_driver inet $nmr_netcard_ip netmask + broadcast + -trailers up 2>&1 > /dev/null

      sun_reboot=1
   fi

   #-----------------------------------------------------------------
   # Cleaning up the legacy stuff
   # Because we used to have an install base with 120.0.0.0 as the console IP
   # address, we must delete those entries in /etc/hosts
   #-----------------------------------------------------------------
   tmp=`grep -w $cons_net_name $etc/hosts |  awk 'BEGIN { FS = "." } \
                                                    { printf("%d.%d.%d",$1,$2,$3) }'`
   #if [ $# -eq 0 -o $# -gt 4 ]
   if [ x$tmp != "x" -a $tmp != $partial_ip ]
   then  
      grep -v $tmp $etc/hosts > /tmp/hosts
      mv -f /tmp/hosts $etc/hosts
      sun_reboot=1
   fi

fi

cons_ether_add=`$vnmrsystem/bin/catcheaddr $e_driver INOVA`

#-----------------------------------------------------------------
# Updating /etc/hosts 
#-----------------------------------------------------------------
chmod +w $etc/hosts

grep -w $nmr_netcard_name  $etc/hosts > /dev/null
if [ $? -ne 0 ]
then
   echo "$nmr_netcard_ip \t $nmr_netcard_name" >> $etc/hosts
   sun_reboot=1
fi

grep -w $cons_net_name $etc/hosts > /dev/null
if [ $? -ne 0 ]
then
   echo "$cons_net_ip \t $cons_net_name" >> $etc/hosts
   sun_reboot=1
fi

grep -w $cons_auto_name $etc/hosts > /dev/null
if [ $? -ne 0 ]
then
   echo "$cons_auto_ip \t $cons_auto_name" >> $etc/hosts
   sun_reboot=1
fi

chmod -w $etc/hosts

#-----------------------------------------------------------------
# add the hosts.equiv to give vxWorks acces to files
# hosts.equiv additions do not need a reboot
#-----------------------------------------------------------------
if [ ! -f $etc/hosts.equiv ]
then
   echo $cons_net_name > $etc/hosts.equiv
   echo $cons_auto_name >> $etc/hosts.equiv

else
   grep -w $cons_net_name $etc/hosts.equiv > /dev/null
   if [ $? -ne 0 ]
   then
      echo $cons_net_name >> $etc/hosts.equiv
   fi

   grep -w $cons_auto_name $etc/hosts.equiv > /dev/null
   if [ $? -ne 0 ]
   then
      echo $cons_auto_name >> $etc/hosts.equiv
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
   sun_reboot=1
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
cp $vnmrsystem/acq/bootptab $etc    #a default copy supplied by vnmr
cat $etc/bootptab | sed -e  "s/08003E236BC4/${cons_ether_add}/" > $etc/bootptab.tmp
mv -f $etc/bootptab.tmp $etc/bootptab

if [ x$fix_bootptab = "xy" ]
then
   cat $etc/bootptab | sed -e 's/ip=10.0.0.2/ip='$cons_net_ip'/' \
                     | sed -e 's/ip=10.0.0.4/ip='$cons_auto_ip'/' > $etc/bootptab.tmp


   mv -f  $etc/bootptab.tmp $etc/bootptab
fi

#-----------------------------------------------------------------
# create /tftpboot if needed, copy vxBoot from /vnmr/acq
#-----------------------------------------------------------------
if [ ! -d /tftpboot/vxBoot ]
then
   mkdir -p /tftpboot/vxBoot
   sun_reboot=1
fi
if [ ! -h /tftpboot/tftpboot ]
then
   cd /tftpboot
   ln -s . tftpboot
fi

rm -f /tftpboot/vxBoot/*
cp -p $vnmrsystem/acq/vxBoot/vxWorks*           /tftpboot/vxBoot
cp -p $vnmrsystem/acq/vxBootPPC/vxWorks         /tftpboot/vxBoot/vxWorksPPC
cp -p $vnmrsystem/acq/vxBootPPC/vxWorks.sym     /tftpboot/vxBoot/vxWorksPPC.sym

cp -p $vnmrsystem/acq/vxBoot.auto/vxWorks.auto /tftpboot/vxBoot

#-----------------------------------------------------------------
# add two lines to the services file if needed
# these two entries are used by the bootpd deamon
# bootps and bootpc are the official Internet service names
#-----------------------------------------------------------------
grep -w bootps $etc/services > /dev/null
if [ $? -ne 0 ]
then
   chmod +w $etc/services
   head -26 $etc/services > $etc/services.tmp
   echo "bootps		67/udp		#bootpd server" >>  $etc/services.tmp
   echo "bootpc		68/udp		#bootpd client" >>  $etc/services.tmp
   tail +27 $etc/services >> $etc/services.tmp
   mv -f $etc/services.tmp $etc/services
   chmod -w $etc/services
   sun_reboot=1
fi
   
#-----------------------------------------------------------------
# Arrange for Acqproc to start at system bootup
#-----------------------------------------------------------------
    cp -p $vnmrsystem/rc.vnmr $etc/init.d

    if [ ! -h $etc/rc0.d/K19rc.vnmr ]
    then
       ln -s $etc/init.d/rc.vnmr $etc/rc0.d/K19rc.vnmr
    fi

    if [ ! -h $etc/rc3.d/S19rc.vnmr ]
    then 
       ln -s $etc/init.d/rc.vnmr $etc/rc3.d/S19rc.vnmr
    fi

    touch /etc/acqpresent
    rm -f /etc/statpresent

#-----------------------------------------------------------------
# By default, TCP/IP considers any machine with multiple network interfaces to be a router.
# With the presence of /etc/norouter , the machine is now a multihomed host
# so it does not turn on IP forwarding on all interfaces
#-----------------------------------------------------------------
if [ ! -f /etc/notrouter ]
then
   touch $etc/notrouter
fi

#-----------------------------------------------------------------
# Remove some files (Queues) NOT IPC_V_SEM_DBM
#-----------------------------------------------------------------
(cd /tmp; rm -f ExpQs		\
                ExpActiveQ	\
                ExpStatus	\
                msgQKeyDbm	\
                ProcQs		\
                ActiveQ		)

echo ""
#There is a check in  ProgressMonitor.java for below message
echo "NMR Console setting up completed\n"

#-----------------------------------------------------------------
# Either reboot the host or restart bootpd
#-----------------------------------------------------------------
if [ $sun_reboot -eq 1 ]
then
   echo " "
   echo "You must reboot Solaris for these changes to take effect"
   echo "As root type 'reboot' to reboot Solaris\n"
else
   if [ -f $vnmrsystem/acqbin/bootpd ]
   then
      ($vnmrsystem/acqbin/bootpd -s > /dev/console &)
   fi
fi
