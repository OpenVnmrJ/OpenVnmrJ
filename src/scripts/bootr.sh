: '@(#)bootr.sh 22.1 03/24/08 2003-2004 '
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
#
# bootr.sh
# For setting up router for Linux system
#
# Usages: bootr 
# 	  bootr -noyp

if [  x`uname -s` != "xLinux" ]
then
    echo " "
    echo "Linux base system only."
    echo " "
    exit 1
fi

if [ `id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'` != "root" ]
then
    echo " "
    echo "Only Superuser (root) can alter ethernet status."
    echo " "
    exit 1
fi

boot_no_yp () {

    mv -f /var/yp /var/yp.none

    netfile=/etc/sysconfig/network
    nicfile=/etc/sysconfig/network-scripts/ifcfg-eth0
    cp $netfile ${netfile}.orig

    #this .orig file causes yp problem
    #cp $nicfile ${nicfile}.orig

    grep  GATEWAY= $netfile 1>&2 >/dev/null
    if [ $? -eq 0 ]
    then
        sed -e 's/^GATEWAY=.*$/GATEWAY=192.168.1.1/' $netfile > ${netfile}tmp
        mv ${netfile}tmp $netfile 
    else
        echo "GATEWAY=192.168.1.1" >> $netfile
    fi


    sed -e 's/^BROADCAST=.*$/BROADCAST=192.168.1.255/' $nicfile | \
    sed -e 's/^IPADDR=.*$/IPADDR=192.168.1.11/' | \
    sed -e 's/^NETWORK=.*$/NETWORK=192.168.1.0/' > ${nicfile}tmp
    mv ${nicfile}tmp $nicfile 

    /usr/bin/reboot
}


boot_yp () {

#commented out for se3 special
#   cd /var
#   if [ -d yp ]
#   then
#      rm -rf yp
#   fi
#
#   mv -f yp.none yp


   netfile=/etc/sysconfig/network
   nicfile=/etc/sysconfig/network-scripts/ifcfg-eth0

   grep  GATEWAY= $netfile 1>&2 >/dev/null
   if [ $? -eq 0 ]
   then
       sed -e 's/^GATEWAY=.*$/GATEWAY=172.16.0.1/' $netfile > ${netfile}tmp
       mv ${netfile}tmp $netfile 
   else
       echo "GATEWAY=172.16.0.1" >> $netfile
   fi


   sed -e 's/^BROADCAST=.*$/BROADCAST=172.16.0.255/' $nicfile | \
   sed -e 's/^IPADDR=.*$/IPADDR=172.16.0.11/' | \
   sed -e 's/^NETWORK=.*$/NETWORK=172.16.0.0/' > ${nicfile}tmp
   mv ${nicfile}tmp $nicfile 

   /usr/bin/reboot
}

################################################################
############ MAIN Main main ####################################

if [  "$#" -eq 0  ]
then
    echo "Boot system with yp"
    boot_yp

elif [ "$#" -eq 1 -a x$1 = "x-noyp" ]
then
    echo "Boot system without yp"
    boot_no_yp

else
    echo "wrong argument(s)"
    exit
fi
