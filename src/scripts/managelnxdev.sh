: '@(#)managelnxdev.sh 22.1 03/24/08 2003-2006 '
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
#managelnxdev
#manage Vnmr printers and plotters on Linux machine

#DeviceURI lpd://print.nmr.varianinc.com/shakespear

#Linux serial port name ttyS0 ttyS1 ...
v_devicenames="${vnmrsystem}"/devicenames
LPADMIN="/usr/sbin/lpadmin"

delete_printer() {

    prname=$1
    $LPADMIN -x wakeup-daemon > /dev/null  2>&1
    $LPADMIN -x $prname
    if [ $? -eq 0 ]
    then
        totallines=`/usr/bin/wc -l $v_devicenames | awk '{print $1}'`
        cat $v_devicenames | awk '
        BEGIN {PRname="'$prname'";Tlines="'"$totallines"'";printlast=1} {
           prline=prev_line
           prev_line=$0
           if ($1=="Name" && $2==PRname){
                 while ($0 !~ /^#############################/ && NR!=Tlines) {
                    getline
                 }
                 prev_line=$0
                 if(NR==Tlines) printlast=0
            } else print prline
        }
        END {if(printlast)print $0}' > /tmp/dev_names
        mv /tmp/dev_names  $v_devicenames
    fi
}

if [ $# -eq 2 -a x$1 = "xdelete" ]
then
    delete_printer $2
    exit
fi

p_name=$1	#name of printer
p_type=$2	#type of printer in devicetable
p_usedas=$3	#Printer or Plotter
p_port=$4  	#serial_A/B , parallel or remote
p_host=$5	#just host name only
r_os=$6		#S5 or bsd
p_baud=$7	#38400 , 19200 , 9600 , 2400
p_default=$8	# 1 for default , 0 for not
p_driver="postscript.ppd"   # default for most Linux printers

case $p_port in
    parallel) l_port="parallel:/dev/lp0" ;;
     network) l_port=" " ;;
esac

case $p_type in
                       PS_*) p_driver="postscript.ppd" ;;
                    HP2800*) p_driver="postscript.ppd" ;;
                PostScript*) p_driver="postscript.ppd" ;;
                  DeskJet_*) p_driver="deskjet.ppd " ;;
                 LaserJet_*) p_driver="laserjet.ppd " ;;
                       LJ_*) p_driver="laserjet.ppd " ;;
esac

add_vnmr_pr() {

     echo "##############################" >> $v_devicenames
     echo "Name    $p_name" >> $v_devicenames
     echo "Use     $p_usedas"  >> $v_devicenames
     echo "Type    $p_type" >> $v_devicenames
     echo "Host    $p_host" >> $v_devicenames
     echo "Port    $p_port" >> $v_devicenames
     echo "Baud" >> $v_devicenames
     echo "Shared  No" >> $v_devicenames
}

ok_2_add_vnmr=

#Linux remote printer
if [ x$p_port = "xremote" ]
then
     case $r_os in
         bsd)  proto="lpd" ;;
           *)  proto=      ;;
     esac

     # test if the remote host can be reached
     # rhost_ip=`/sbin/arp $p_host` 2> /dev/null  
     # The use of arp was a bad choice, it can return with MAC or IP addr
     # and if you entered an IP it's sure to fail
     # the result of this test and getting the IP would be flaky. GMB 7/31/09

     # using ping should solve all this.   GMB 7/31/2009
     # ping once, wait upto 3 sec for a responce
     # test if the remote host can be reached
     /bin/ping -c 1 -w 3 -q $p_host > dirlist 2>&1
     if [ $? -eq 0 ]
     then
         rhost_ip=`/bin/ping -c 1 -q $p_host | awk 'BEGIN{FS="("} {print $2}' | awk 'BEGIN{FS=")"} {print $1}'`
     else
         echo "Add printer: Problem communicating with print server $p_host"
         exit
     fi
     
     $LPADMIN -x wakeup-daemon > /dev/null 2>&1

     # This was always was using the IP
     # $LPADMIN -p $p_name -v ${proto}://${rhost_ip}/${p_name} -E

     # use the host or IP which ever is given, ping has confirmed access
     $LPADMIN -p $p_name -v ${proto}://${p_host}/${p_name} -E
     if [ $? -eq 0 ]
     then
        ok_2_add_vnmr=1
     else
        echo "Add printer: Problem adding printer $p_name"
        exit
     fi
     $LPADMIN -x wakeup-daemon > /dev/null 2>&1
     $LPADMIN -p $p_name -P /usr/share/cups/model/postscript.ppd
     /usr/bin/lpoptions -p $p_name -l

else   #Linux local printer

    p_host=`uname -n`
    def_drv_dir="/usr/share/cups/model"
    def_drv_path=${def_drv_dir}/${p_driver}

    if [ ! -r ${def_drv_path} ]
    then
        if [ -r ${def_drv_path}.gz ]
        then
             gunzip ${def_drv_path}.gz 
             if [ $? -ne 0 ]
             then
                  echo "Add printer: Problem locating driver for this printer"
                  exit
             fi
        else
             echo "Add printer: Problem locating driver $p_driver"
             exit
        fi
     fi
     $LPADMIN -x wakeup-daemon 2>&1 > /dev/null
     $LPADMIN -p $p_name -E -v $l_port -m $p_driver
     if [ $? -eq 0 ]
     then
         ok_2_add_vnmr=1
     else
         echo "Add printer: Problem locating driver for this printer"
         exit
     fi

fi

if [ $ok_2_add_vnmr ]
then
    add_vnmr_pr
else
     echo "Add printer: Problem locating driver for this printer"
fi
