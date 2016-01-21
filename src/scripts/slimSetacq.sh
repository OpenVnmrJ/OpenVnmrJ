: '@(#)slimSetacq.sh 22.1 03/24/08 1999-2002 '
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
: '@(#)rsetacq.sh 16.1 10/25/2 1991-1997 '
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
cons_eadd="080088888888"
etc=/etc

 
# Main main MAIN
#
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
     su root -c "/vnmr/bin/slimSetacq ${ARGS}";
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
#-----------------------------------------------------------------
n_eri=`"$vnmrsystem"/bin/findedevices | grep "eri[ ,0-9]" | wc -l` 
n_hme=`"$vnmrsystem"/bin/findedevices | grep "hme[ ,0-9]" | wc -l` 
case $n_eri in
   *2) driver1="eri0"
       driver2="eri1"
       ;;
   *1) driver1="eri0"
       if [ $n_hme -gt 0 ]
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

#-----------------------------------------------------------------
# Ask the user to reboot the console now,
# then select le0 or le1 as interface
#-----------------------------------------------------------------
echo " "
echo "Please reboot the Slim."
echo "Enter a return after Slim reboots: \c"
read ans
echo " "
#echo "Select from the options below:"
#echo "1. Your SUN is attached to the console via the standard ethernet"
#echo "   port"
#echo "2. Your SUN is attached to the console via the second ethernet"
#echo "   port."
#echo " "
#echo "What is your configuration? (1 or 2) [2]: \c"
#read enet_type
#if ( test x$enet_type != "x1" )
#then
   enet_type=2
#fi

cons_name="slim"
cons_ip="10.0.0"
enet2_name="Not_Used"
reboot=0

#-----------------------------------------------------------------
# check that the default IP is not the main net IP 
# Because VxWorks does not use 'bootptab' parameters 
# right away, for now we use 120.0.0
#-----------------------------------------------------------------
host_net=`grep -v '^#' $etc/hosts | grep -w \`uname -n\` | awk 'BEGIN { FS = "." } \
                { printf("%d.%d.%d",$1,$2,$3) }'`
first_net_byte=`echo $host_net | awk 'BEGIN { FS = "." } \
                { printf("%d",$1) }'`
fix_bootptab="n"
if [ x$enet_type = "x2" ] 
then
   if [ x$first_net_byte = "x10" ]
   then
      cons_ip=172.16.0
      fix_bootptab="y"
   fi
else
   cons_ip=$host_net
fi

# catch the ethernet address
if [ x$enet_type = "x1" ]
then
   cons_eadd=`"$vnmrsystem"/bin/catcheaddr $driver1 INOVA`
else
   cons_eadd=`"$vnmrsystem"/bin/catcheaddr $driver2 INOVA`
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
if [ x$enet_type = "x2" ]
then
   tmp=`grep -v '^#' $etc/hosts | grep -w slim |  awk 'BEGIN { FS = "." } \
                { printf("%d.%d.%d",$1,$2,$3) }'`
   if [ x$tmp != "x" ]
   then
      if [ $tmp != $cons_ip ]
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
grep -v '^#' $etc/hosts | grep -w $cons_name > /dev/null
if [ $? -ne 0 ]
then
   echo "$cons_ip.43\t$cons_name" >> $etc/hosts
   reboot=1
else
   #remove the line with slim and replace
   mv $etc/hosts $etc/hosts.orig
   grep -v $cons_name $etc/hosts.orig > $etc/hosts
   echo "$cons_ip.43\t$cons_name" >> $etc/hosts
   rm $etc/hosts.orig
   reboot=1
fi
chmod -w $etc/hosts

#-----------------------------------------------------------------
# add slim line to bootptab and substitute the received ethernet address
# Done no matter what, in case CPU (=ethernet address) was changed
#-----------------------------------------------------------------
#cp $vnmrsystem/acq/bootptab $etc

chmod +w $etc/bootptab
grep -v '^#' $etc/bootptab | grep -w $cons_name > /dev/null
if [ $? -ne 0 ]
then
   echo "$cons_name:ht=ethernet:ha=080088888888:ip=$cons_ip.43" >> $etc/bootptab
   reboot=1
else
   #remove the line with slim and replace
   mv $etc/bootptab $etc/bootptab.orig
   grep -v $cons_name $etc/bootptab.orig > $etc/bootptab
   echo "$cons_name:ht=ethernet:ha=080088888888:ip=$cons_ip.43" >> $etc/bootptab
   rm $etc/bootptab.orig
   reboot=1
fi
chmod -w $etc/bootptab

cat $etc/bootptab | sed -e \
	"s/080088888888/${cons_eadd}/" > $etc/bootptab.tmp
cp $etc/bootptab.tmp $etc/bootptab
rm $etc/bootptab.tmp
if [ x$fix_bootptab = "xy" ]
then
cat $etc/bootptab |  \
                    sed -e 's/ip=10.0.0.43/ip='$cons_ip'.43/'  > $etc/bootptab.tmp
cp $etc/bootptab.tmp $etc/bootptab
rm $etc/bootptab.tmp
fi

if [ -f "$vnmrsystem"/acqbin/bootpd ]
then
    ("$vnmrsystem"/acqbin/bootpd -s > /dev/console &)
fi

echo ""
echo "SLIM software installation complete"


