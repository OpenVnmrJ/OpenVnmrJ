#!/bin/bash
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#

# set -x

SCRIPT=$(basename "$0")
vnmrsystem="/vnmr"

stop_program () {
  npids=$(ps -e  | grep $1 | awk '{ printf("%d ",$1) }')
  if [[ ! -z "$npids" ]] ; then
     echo "Stopping $1 program..."
  fi
  for prog_pid in $npids
  do
    kill -2 $prog_pid
    sleep 5             # give time for kill message to show up.
    if [[ x$1 = "nddsManager" ]] ; then
      sleep 10
    fi
  done
  # test to be sure the program died, if still running try kill -9
  npids=$(ps -e  | grep $1 | awk '{ printf("%d ",$1) }')
  if (test x"$npids" != "x" )
  then   
    for prog_pid in $npids
    do   
       kill -9 $prog_pid
       sleep 5          # give time for kill message to show up.
    done
    # Once again double check to see if any are still running.
    npids=$(ps -e  | grep $1 | awk '{ printf("%d ",$1) }')
    if [[ x"$npids" != "x" ]] ; then
      for prog_pid in $npids
      do 
        echo "Unable to kill '$1' (pid=$prog_pid)"
      done
    fi
  fi
}

# make backup copy of bootpd, but only if cksum is different
backup_bootpd()
{
   vnmr_bootpd="${vnmrsystem}/acqbin/bootpd.51"
   system_bootpd="/usr/sbin/bootpd"

   if [[ -f $system_bootpd ]] ; then
      do_cp="n"
      v1sum=$(cksum $vnmr_bootpd | awk '{printf $1}')
      v2sum=$(cksum $vnmr_bootpd | awk '{printf $2}')
      s1sum=$(cksum $system_bootpd | awk '{printf $1}')
      s2sum=$(cksum $system_bootpd | awk '{printf $2}')
      if [[ x"$v1sum" != x"$s1sum" ]] ||
         [[ x"$v2sum" = x"$s2sum" ]] ; then
         do_cp="y"
         files=$(ls /usr/sbin/bootpd*)
         for file in $files
         do
            f1sum=$(cksum $file | awk '{printf $1}')
            f2sum=$(cksum $file | awk '{printf $2}')
            if [[ x"$f1sum" = x"$s1sum" ]] &&
               [[ x"$f2sum" = x"$s2sum" ]] ; then
               if [ x"$file" != x"$system_bootpd" ] ; then
                  do_cp="n"
               fi
            fi
         done
      fi

      if [[ x$do_cp = "xy" ]] ; then
         date=$(date +%y%m%d.%H:%M)
         cp -p $system_bootpd $system_bootpd.bkup.$date
      fi
   fi
}

#
modEtcPamdRsh() {
   file=/etc/pam.d/rsh
   pamlist="pam_rhosts_auth.so pam_unix_acct.so pam_unix_session.so"
   if [[ -r $file ]] ; then
      rm -f ${file}.tmp
      rshline=$(grep pam_permit.so $file)
      # if not there then add line to file
      if [[ -z "$rshline" ]] ; then
         while IFS='' read -r line || [[ -n "$line" ]]; do
            echo $line >> ${file}.tmp
            if [[ "$line" = *"pam_env.so"* ]]; then
               echo "auth       required     pam_permit.so" >> ${file}.tmp
            fi
         done < $file
      fi
      if [[ -e ${file}.tmp ]] ; then
         rm -f ${file}
         mv ${file}.tmp ${file} 
         chmod 644 ${file}
      fi

      # check for each lib to be commented out
      for rmlibs in $pamlist ; do
         rshline=$(grep $rmlibs $file)
         if [[ ! -z "$rshline" ]] ; then
         while IFS='' read -r line || [[ -n "$line" ]]; do
            if [[ "$line" = *"$rmlibs"* ]] ; then
               if [[ "$line" != "#"* ]]; then
                  echo "# $line" >> ${file}.tmp
               else
                  echo $line >> ${file}.tmp
               fi
            else
               echo $line >> ${file}.tmp
            fi
         done < $file
         fi
         if [[ -e ${file}.tmp ]] ; then
            rm -f ${file}
            mv ${file}.tmp ${file} 
            chmod 644 ${file}
         fi
      done
   fi
}


# future idea to implement
# check for setsebool and getsebool rather than debian
# use these programs rather than checking/modifing files directly.
#  /usr/sbin/getsebool -a
# modify settings via setsebool
# /usr/sbin/setsebool SELINUX false ??
#
#   SELINUX=disabled   (possible values: enforcing, permissive, disabled)
disableSelinux() {
#  if [ -x /usr/sbin/setsebool ] ; then
   if [ -f /etc/selinux/config ] ; then
      selinux=$(grep SELINUX /etc/selinux/config | grep -i disabled)
      # echo "str: $str"
      if [ -z "$selinux" ] ; then
         echo ""
         echo ""
         echo "Disabling SELinux, System Reboot Required."
         echo "You must reboot the system prior to continuing setacq."
         echo ""
         echo ""
         # replace the two possibilites enforcing or permissive, to be disabled
         cat /etc/selinux/config | sed s/SELINUX=[eE][nN][fF][oO][rR][cC][iI][nN][gG]/SELINUX=disabled/ | sed s/SELINUX=[pP][eE][rR][mM][iI][sS][sS][iI][vV][eE]/SELINUX=disabled/ > config.mod
         cp config.mod /etc/selinux/config
         rm -f config.mod
         # SELinux reboot flag file, to break out of the enter root password 
         # loop in main.
         reboot=1
      fi
   fi
}

# For reference two common locations for the tftpboot directory
#  1.  /tftpboot
#  2.  /var/lib/fttpboot
getTFTPBootDir() {
   if [[ -f /etc/xinetd.d/tftp ]] ; then
      # skip line with # eg comments
      tftpdir=$(grep  -v \# /etc/xinetd.d/tftp |
                grep server_args | awk '{print $4 }')
      # be sure the path make sense i.e. has tftpboot in the string
      if [[ $(echo "$tftpdir" |
              grep tftpboot > /dev/null;echo $?) -ne 0 ]] ; then
         tftpdir="/tftpboot"
      fi
   elif [[ -f $sysdDir/tftp.service ]] ; then
      tftpdir=$(grep  tftpboot $sysdDir/tftp.service |
                awk '{print $3 }')
   elif [[ -f /etc/default/tftpd-hpa ]] ; then
      . /etc/default/tftpd-hpa
      tftpdir=$TFTP_DIRECTORY
   else
      tftpdir="/tftpboot"
   fi
   # lets be sure the directory is present
   if [[ ! -d "$tftpdir" ]] ; then
       echo "tftpboot directory:  $tftpdir is not present "
       echo "Creating directory: $tftpboot"
       mkdir "$tftpdir"
   fi
   # echo "TFTP Boot Directory: $tftpdir "
}

rmTFTPBootFiles() {
   rm -f "$tftpdir"/*.o   "$tftpdir"/*.bdx  "$tftpdir"/nvScript
}

installTftp() {
# Install tftp if needed
 if [ -f /etc/debian_version ]; then
   if [ "$(dpkg --get-selections tftp-hpa 2>&1 | grep -w 'install' > /dev/null;echo $?)" != "0" ] 
   then
      echo "Installing console communication tool (tftp)..."
      apt-get -y install tftp-hpa &>> ${vnmrsystem}/adm/log/tftp.log
   fi
   if [ "$(dpkg --get-selections tftpd-hpa 2>&1 | grep -w 'install' > /dev/null;echo $?)" != "0" ] 
   then
      apt-get -y install tftpd-hpa &>> ${vnmrsystem}/adm/log/tftp.log
   fi
 fi
}

installRarp() {
# Install rarp if needed
 if [ ! -f /etc/debian_version ]; then
   rarp=$(rpm -qa | grep rarpd)
   if [[ -z $rarp ]] ; then
      file=${vnmrsystem}/adm/acq/rarpd-ss981107-22.2.2.x86_64.rpm
      if [[ -e ${file} ]] ; then
         echo "Installing console network identification tool (rarp)..."
         yum -y --disablerepo=* install ${file} &> ${vnmrsystem}/adm/log/rarp.log
      fi
   else
      stop_program rarpd
   fi
 else
   if [ "$(dpkg --get-selections rarpd 2>&1 | grep -w 'install' > /dev/null;echo $?)" != "0" ] 
   then
      apt-get -y install rarpd &> ${vnmrsystem}/adm/log/rarp.log
   fi
 fi
}

configureRarp() {
#-----------------------------------------------------------------
# setup rarpd so rarpd service gets started at boot time
#-----------------------------------------------------------------
# this didn't work out for rarpd, so back to the original way
# if [ x$usechkconfig = "xy" ] ; then
#   /sbin/chkconfig --list 2> /dev/null | grep -q rarpd ;
#   if [ $? -eq 0 ]; then
#      /sbin/chkconfig --level 35 rarpd on ;
#      /sbin/service xinetd restart > /dev/null 2>&1 ;
#   fi
# fi

#
#  works equally well for RHEL and Ubuntu rarpd
#
 if [ ! -f /etc/debian_version ]; then
   rarpd="/etc/init.d/rarpd"
   if [[ -r $rarpd ]] ; then
      grep 'daemon /usr/sbin/rarpd -e' $rarpd 2>&1 > /dev/null
      if [[ $? -ne 0 ]] ; then
#        echo "edit $rarpd ---------"
         chmod +w $rarpd
         sed -i -e 's/daemon \/usr\/sbin\/rarpd/daemon \/usr\/sbin\/rarpd -e/' $rarpd
         chmod 755 $rarpd
         pkill -HUP rarpd
      fi

      #Arrange for rarp server running at boot time
      # is there a S rarpd link already present?
      Arp=$(ls -l /etc/rc5.d/S*rarp* &> /dev/null)
      if [ -z "${Arp}" ] ; then
         (cd /etc/rc5.d; if [ ! -f S98rarpd ]; then \
             ln -s ../init.d/rarpd S98rarpd; fi)
      else
         ls -l /etc/rc5.d/S*a* | grep -q ../init.d/rarpd ;
         if [ $? -ne 0 ]; then
            (cd /etc/rc5.d; if [ ! -f S98rarpd ]; then \
                 ln -s ../init.d/rarpd S98rarpd; fi)
         fi
      fi
      # is there a K rapd link already present?
      Arp=$(ls -l /etc/rc0.d/K*rarp* &> /dev/null)
      if [ -z "${Arp}" ] ; then
         (cd /etc/rc0.d; if [ ! -f K98rarpd ]; then \
             ln -s ../init.d/rarpd K98rarpd; fi)
      else
         ls -l /etc/rc0.d/K*a* | grep -q ../init.d/rarpd ;
         if [ $? -ne 0 ]; then
            (cd /etc/rc0.d; if [ ! -f K98rarpd ]; then \
                ln -s ../init.d/rarpd K98rarpd; fi)
         fi
      fi
   else
      echo "rarp package is not available. Exit"
      exit 0
   fi
 fi
}

enableTftp() {
 if [ ! -f /etc/debian_version ]; then
   tftpconf="/etc/xinetd.d/tftp"
   if [[ -r $tftpconf ]] ; then
      grep -w disable $tftpconf | grep -w no 2>&1 > /dev/null
      if [[ $? -ne 0 ]] ; then
         chmod +w $tftpconf
         sed -i '/disable/s/= yes/= no/' $tftpconf
         chmod -w $tftpconf
         pkill -HUP xinetd
      fi
   elif [[ $1 -eq 0 ]] ; then
      if [[ ! -f $sysdDir/tftp.service ]]; then
         echo "tftp package is not available. Exit"
         echo " "
         exit 0
      fi
   fi
 fi
}

setupServices() {
#----------------------------------------------------------
## Notes for debian Ubuntu changes for future reference
## inetd is used rather than xinetd, so don't install xinetd
## chkconfig equiv on Debian (Ubuntu) is  update-rc.d
## These control the process started by init, in /etc/init.d
## Temporary stop,  e.g. /etc/init.d/<$SERVICE_NAME> stop
## Permanent
## e.g. RHEL chkconfig $SERVICE_NAME off
##      DEB  update-rc.d -f $SERVICE_NAME remove
##
## if rarpd is installed then no changes from it's default
## configuration is required   (already used -e option)
## if tftpd, rsh-server were install thee should be already enabled in /etc/inetd.conf
## use update-inetd to enable or disable these
## e.g. sudo update-inetd --enable tftp
##      sudo update-inetd --disable tftp
#
#
# check /etc/inetd.conf for line to determine if tftpd is installed
#  'tftp dgram udp wait nobody  /usr/sbin/tcpd  /usr/sbin/in.tftpd /srv/tftp'
# Note the above default line must be changed for proper operation of tftp to:
#  'tftp dgram udp wait nobody  /usr/sbin/tcpd  /usr/sbin/in.tftpd -s /tftpboot'
#  use 'sudo update-inetd --enable tftp' to enable
# check  /etc/inetd.conf for line to determine if rsh server installed
#  'shell  stream  tcp  nowait  root  /usr/sbin/tcpd  /usr/sbin/in.rshd'
#  use 'sudo update-inetd --enable shell' to enable
# check  /etc/inetd.conf for line to determine if time server installed
# #time            stream  tcp     nowait  root    internal'
# use 'sudo update-inetd --comment-chars '#' --enable time'
#----------------------------------------------------------
#
# check if inetd configuration rather than xinetd 
   if [[ -x /usr/sbin/update-inetd ]] ; then
      useupdateinetd="y"
   else
      useupdateinetd="n"
   fi

#-----------------------------------------------------------------
# determine if the utilities chkconfig and service are available
# RHEL
#-----------------------------------------------------------------
   if [[ -x /sbin/chkconfig ]] && [[ -x /sbin/service ]] ; then
      usechkconfig="y"
   else
      usechkconfig="n"
   fi

#-----------------------------------------------------------------
# enable rsh, tftp, time via inetd (Ubuntu)
#-----------------------------------------------------------------
   if [ x$useupdateinetd = "xy" ] ; then
      ## enable rsh server
      sudo update-inetd --enable shell

      ## remove incorrect tftp entry
      # sudo update-inetd  --remove tftp
      ## add corrected entry for tftp
      # sudo update-inetd --add 'tftp\t\tdgram\tudp\twait\tnobody\t/usr/sbin/tcpd\t/usr/sbin/in.tftpd -s /tftpboot'

      ## enable time for rdate of console
      ## sudo update-inetd --comment-chars '#' --enable time
      # remove any unwanted entry for time, so that add works below
      sudo update-inetd --remove  time 
      # add the time entry into the inetd.conf file
      sudo update-inetd --add  'time\t\tstream\ttcp\tnowait\troot\tinternal'
      # using update-inetd  insures the inetd daemon re-reads the inetd.conf file
      update-inetd --enable tftpd-hpa
      systemctl start tftpd-hpa.service

      # for Ubuntu we have to modify the /etc/pam.d/rsh file
      # add: auth    required        pam_permit.so
      # comment out the line containing
      # pam_rhosts_auth.so,pam_unix_acct.so ,pam_unix_session.so
      # otherwise the hosts.equiv does not work with VxWorks rsh
      if [ -r /etc/init.d/openbsd-inetd ] ; then
         modEtcPamdRsh
         sudo /etc/init.d/openbsd-inetd reload > /dev/null
      fi

   else  # RHEL CentOS xinetd config

      #-----------------------------------------------------------------
      # fix /etc/xinetd.d/tftp  so tftpd gets started at boot time
      #-----------------------------------------------------------------

      if [ x$usechkconfig = "xy" ] ; then
         /sbin/chkconfig --list 2> /dev/null | grep -q tftp ;
         if [ $? -eq 0 ]; then
            # have tftp service start on level 3 (single user)
            # and on level 5 (multiuser)
            /sbin/chkconfig --level 35 tftp on 2> /dev/null ;
            /sbin/service xinetd restart 2> /dev/null ;
         fi
      fi
      enableTftp 1

      #-----------------------------------------------------------------
      # fix /etc/xinetd.d/time  so time service gets started at boot time
      #  this allows rdate from console to work properly  GMB 
      #-----------------------------------------------------------------
      if [ x$usechkconfig = "xy" ] ; then
         # try the rhel 5.x version of time (time-stream) 1st, 
         # then fall back to RHEL 4.X time (time)
         /sbin/chkconfig --list 2> /dev/null | grep -q time-stream ;
         if [ $? -eq 0 ]; then
            /sbin/chkconfig --level 35 time-stream on ;
            /sbin/service xinetd restart > /dev/null 2>&1 ;
         else
           /sbin/chkconfig --list 2> /dev/null | grep -q time ;
           if [ $? -eq 0 ]; then
              /sbin/chkconfig --level 35 time on ;
              /sbin/service xinetd restart > /dev/null 2>&1 ;
           fi
         fi
      else
         if [ -f /etc/xinetd.d/time-stream ] ; then
            timeconf="/etc/xinetd.d/time-stream"
         else
            timeconf="/etc/xinetd.d/time"
         fi 
         if [ -r $timeconf ] ; then
            grep -w disable $timeconf | grep -w no 2>&1 > /dev/null
            if [ $? -ne 0 ] ; then
               chmod +w $timeconf
               sed -i '/disable/s/= yes/= no/'  $timeconf
               chmod -w $timeconf
               pkill -HUP xinetd
            fi
         else
            echo "${timeconf} file is not available."
         fi
      fi

      #-----------------------------------------------------------------
      # fix /etc/xinetd.d/rsh  so rsh service gets started at boot time
      #-----------------------------------------------------------------
      if [ x$usechkconfig = "xy" ] ; then
         /sbin/chkconfig --list 2> /dev/null | grep -q rsh ;
         if [ $? -eq 0 ]; then
            /sbin/chkconfig --level 35 rsh on ;
            /sbin/service xinetd restart > /dev/null 2>&1 ;
         fi
      else
         rshconf="/etc/xinetd.d/rsh"
         if [ -r $rshconf ] ; then
            grep -w disable $rshconf | grep -w no 2>&1 > /dev/null
            if [ $? -ne 0 ] ; then
               chmod +w $rshconf
               sed -i '/disable/s/= yes/= no/' $rshconf
               chmod -w $rshconf
               pkill -HUP xinetd
            fi
         else
            echo "rsh package is not available."
         fi
      fi
   
   fi   # end if of useupdateinetd
}

installConsole() {
   /usr/sbin/rarpd -e

   PATH=${vnmrsystem}/bin:$PATH
   export PATH
   NIRVANA_CONSOLE="wormhole"
   export NIRVANA_CONSOLE
   chmod 775 "$tftpdir"
   rmTFTPBootFiles 
   rm -f ${vnmrsystem}/acq/download/load* ${vnmrsystem}/acq/download3x/load*
   if [[ -e ${vnmrsystem}/acq/download/nvScript ]] ; then
      nvLen=$(cat ${vnmrsystem}/acq/download/nvScript | wc -c)
      nvLenStd=$(cat ${vnmrsystem}/acq/download/nvScript.std | wc -c)
      nvLenLs=$(cat ${vnmrsystem}/acq/download/nvScript.ls | wc -c)
   else
      echo "Spectrometer software is not available in ${vnmrsystem}"
      echo " "
      exit 1
   fi


   esc="["
   echo " "
   echo "    ${esc}47;31mThe download step may take four to eight minutes ${esc}0m"
   echo "    ${esc}47;31mDo not reboot the console during this process    ${esc}0m"
   echo " "

   if [[ $nvLen -eq $nvLenStd ]] ; then
      echo "    Download files for the VNMRS / DD2 systems"
      echo " "
   fi

   if [[ $nvLen -eq $nvLenLs ]] ; then
      echo "    Download files for the ProPulse / 400-MR systems"
      echo " "
   fi

#   ping master1 (once [-c1]), if no answer ask to check and rerun $0
   ping -c1 -q master1 > /dev/null
   if [[ $? -ne 0 ]] ; then
     echo ""
     echo Please check that the console and host are connected
     echo Then rerun $0
     echo ""
     exit 0
   fi

   echo "Testing console software version, will try up to 20 seconds"
#   $vnmrsystem/acqbin/testconf42x
#   query the master for ndds version
   stop_program nddsManager
   $vnmrsystem/acqbin/testconf42x -querynddsver
   if [[ $? -eq 1 ]] ; then    
      $vnmrsystem/acqbin/testconf3x
      if [[ $? -eq 1 ]] ; then
         $vnmrsystem/acqbin/consoledownload3x -dwnld3x
         nofile=1
         count=0
         if [[ -f $vnmrsystem/acq/download3x/loadhistory ]] ; then
            owner=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }')
            group=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }')
            chown $owner:$group $vnmrsystem/acq/download/load* 2> /dev/null
            count=$(grep successful $vnmrsystem/acq/download3x/loadhistory | wc -l)
         fi
         if [[ $count -lt 9 ]] ; then
            echo ""
            echo "Console software download failed"
            echo "Reboot the NMR console"
            echo "Then rerun setacq"
            echo ""
            exit 0
         fi
         rmTFTPBootFiles 
         rm -f ${vnmrsystem}/acq/download3x/load*
         echo pausing for 25 seconds
         sleep 25
         $vnmrsystem/acqbin/consoledownload3x -tftpdir "$tftpdir"
      else
         $vnmrsystem/acqbin/consoledownload3x -tftpdir "$tftpdir"
      fi
      stop_program nddsManager
      sleep 25
   else    
      $vnmrsystem/acqbin/consoledownload42x -tftpdir "$tftpdir"
   fi      
    
   nofile=1
   count=0
   if [[ -f $vnmrsystem/acq/download/loadhistory ]] ; then
      owner=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }')
      group=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }')
      chown $owner:$group $vnmrsystem/acq/download/load* 2> /dev/null
      count=$(grep successful $vnmrsystem/acq/download/loadhistory | wc -l)
   fi
   if [[ $count -lt 10 ]] ; then
      echo ""
      echo "Console software download failed"
      echo "Reboot the NMR console"
      echo "Then rerun setacq"
      echo ""
      exit 0
   fi
}

installConsoleMI() {
#-----------------------------------------------------------------
# create /tftpboot if needed, copy vxBoot from /vnmr/acq
#-----------------------------------------------------------------
   tftpboot=$tftpdir
   if [[ ! -d $tftpboot/vxBoot ]] ; then
      mkdir -p $tftpboot/vxBoot
      reboot=1
   fi
   if [[ ! -f /etc/debian_version ]]; then
      if [[ ! -h $tftpboot/tftpboot ]] ; then
         cd $tftpboot
         ln -s . tftpboot
      fi
      if [[ $tftpboot = "/var/lib/tftpboot" ]] ; then
         if [[ ! -h $tftpboot/var ]] ; then
            cd $tftpboot
            ln -s . var
         fi
         if [[ ! -h $tftpboot/lib ]] ; then
            cd $tftpboot
            ln -s . lib
         fi
      fi
   fi
   #  Ubuntu 20 case
   if [[ $tftpboot = "/srv/tftp" ]] ; then
      if [[ ! -h $tftpboot/srv ]] ; then
         cd $tftpboot
         ln -s . srv
      fi
      if [[ ! -h $tftpboot/tftp ]] ; then
         cd $tftpboot
         ln -s . tftp
      fi
   fi
   rm -f $tftpboot/vxBoot/*
   cp -p $vnmrsystem/acq/vxBoot/vxWorks* $tftpboot/vxBoot
   if [[ x${cons} = "xinova" ]] ; then
      cp -p $vnmrsystem/acq/vxBootPPC/vxWorks  $tftpboot/vxBoot/vxWorksPPC
      if [[ -f $vnmrsystem/acq/vxBootPPC/vxWorks.sym ]] ; then
         cp -p $vnmrsystem/acq/vxBootPPC/vxWorks.sym $tftpboot/vxBoot/vxWorksPPC.sym
      fi
      cp -p $vnmrsystem/acq/vxBoot.auto/vxWorks.auto $tftpboot/vxBoot
   fi
}


#----------------------------
#  MAIN Main main
#----------------------------
if [[ ! x$(uname -s) = "xLinux" ]] ; then
   echo " "
   echo "$SCRIPT suitable for Linux-based systems only"
   echo " "
   exit 0
fi

#-----------------------------------------------------------------
# make sure no OpenVnmrJ is running
#
npids=$(ps -e  | grep Vnmr | awk '{ printf("%d ",$1) }')
if [[ x"$npids" != "x" ]] ; then 
   echo ""
   echo "You must exit all 'OpenVnmrJ'-s before running $SCRIPT"
   echo "Please exit from OpenVnmrJ then restart $SCRIPT"
   echo ""
   exit 0
fi

#-----------------------------------------------------------------
#  Need to be root to configure an acquisition system

userId=$(/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }')
if [[ $userId != "uid=0(root)" ]]; then
   echo
   echo "To run $0 you will need to be the system's root user,"
   echo "or type cntrl-C to exit."
   echo
   s=1
   t=3
   while [[ $s = 1 ]] && [[ ! $t = 0 ]]; do
      echo "Please enter this system's root user password"
      echo
      if [ -f /etc/debian_version ]; then
         sudo $0 $* ;
      else
         su root -c "$0 $*";
      fi
      s=$?
      t=$((t-1))
      echo " "
   done
   if [[ $t = 0 ]]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo
   fi
exit 0
fi

reboot=0
${vnmrsystem}/bin/setNIC verify
if [ $? -ne 0 ] ; then
   echo "Network for the spectrometer is not configured correctly"
   echo "Would you like to configure it now (y/n) [y]? "
   read ans
   if [[ "x$ans" = "xn" ]] ; then
      echo "Would you like to continue with $0 anyway (y/n) [y]? "
      read ans
      if [[ "x$ans" = "xn" ]] ; then
         exit 0
      fi
   else
      ${vnmrsystem}/bin/setNIC
      if [ $? -ne 0 ] ; then
         echo "Network for the spectrometer is not configured correctly"
         echo "Would you like to continue with $0 anyway (y/n) [y]? "
         read ans
         if [[ "x$ans" = "xn" ]] ; then
            exit 0
         fi
         echo "Use the command /vnmr/bin/setNIC to configure the network"
         echo "or run setacq again"
         echo " "
      fi
   fi
   reboot=1
else
   echo "Network for the spectrometer is configured"
   ${vnmrsystem}/bin/setNIC show
   echo " "
   echo "Would you like to re-configure it (y/n) [n]? "
   read ans
   if [[ "x$ans" = "xy" ]] ; then
      ${vnmrsystem}/bin/setNIC
   fi
fi

#-----------------------------------------------------------------
# initialize some variables
#
if [[ -e ${vnmrsystem}/adm/log/CONSOLE ]] ; then
   cons=$(cat ${vnmrsystem}/adm/log/CONSOLE)
else
   cons=ddr
fi

#
#  First make sure the SELinux is OFF.
#


# if SELinux was enabled, then disable, but a system reboot is required.
# Stop installation.
disableSelinux

#-----------------------------------------------------------------
# Check if the Expproc is still running. If so, terminate it
#-----------------------------------------------------------------
npids=$(pgrep Expproc)
if [[ ! -z $npids ]]; then
    ${vnmrsystem}/acqbin/startStopProcs
fi

sysdDir=""
if [[ ! -z $(type -t systemctl) ]] ; then
   sysdDir=$(pkg-config systemd --variable=systemdsystemunitdir)
fi
#-----------------------------------------------------------------
# determine location of the tftpboot directory which the tftpd is configured for
# usually /tftpboot or /var/lib/tftpboot or /srv/tftp
#-----------------------------------------------------------------
installTftp
getTFTPBootDir

if [[ x${cons} = "xinova" ]] || [[ x${cons} = "xmerc"* ]] ; then
   doingMI=1
   if [[ -f $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto ]] ; then
    #Ask if the mts version of MSR is needed
       rm -f $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
       cp -p $vnmrsystem/acq/vxBoot.auto/std.vxWorks.auto $vnmrsystem/acq/vxBoot.auto/vxWorks.auto
   fi
#-----------------------------------------------------------------
# kill bootpd, otherwhise otherwise the console will reboot
# with the old files (can't delete /tftpboot/vxWorks!)
#-----------------------------------------------------------------
   stop_program bootpd

   file=${vnmrsystem}/adm/log/CONSOLE_NIC
   if [[ ! -e $file ]] ; then
      echo "Network has not been configured for a spectrometer"
      exit 1
   fi
   nic=$(cat $file)
# obtain the MAC address of the console NIC
   echo " "
   echo "Please reboot the console."
   echo "Press the Enter key after pressing the console reset button: "
   read ans
   echo 
   echo "One moment please..."
   echo 
   macaddr=$(/usr/sbin/tcpdump -i $nic host 255.255.255.255 -t -n  -X -c 1 2>/dev/null |
         grep 0x0030 | awk '{print $6 $7 $8}')


#-----------------------------------------------------------------
# copy bootptab and substitute the received ethernet address
# Done no matter what, in case CPU (=ethernet address) was changed
#-----------------------------------------------------------------

   enableTftp 0
   backup_bootpd
   cp "${vnmrsystem}/acqbin/bootpd.51" /usr/sbin/bootpd
   vnmr_bootptab="${vnmrsystem}/acq/bootptab.51"
   etc_bootptab="/etc/bootptab"

   base_IP=$(cat ${vnmrsystem}/adm/log/CONSOLE_IP)
   cp $vnmr_bootptab $etc_bootptab
   sed -i -e "s/08003E236BC4/${macaddr}/" $etc_bootptab
   sed -i -e 's/10.0.0./'${base_IP}'./'   $etc_bootptab
   if [ "$(echo $tftpdir | grep '/tftpboot' > /dev/null;echo $?)" != "0" ]; then
      sed -i -e "s:/tftpboot:${tftpdir}:"   $etc_bootptab
   fi

# This program prevents bootpd from working
   if [[ -f /usr/sbin/dnsmasq ]] ; then
     mv -f /usr/sbin/dnsmasq /usr/sbin/dnsmasq_off
     reboot=1
   fi
else
   doingMI=0
   installRarp
   setupServices
   configureRarp
   if [[ ! -z $sysdDir ]] ; then
     if [[ -f $sysdDir/rsh.socket ]]; then
      systemctl is-active --quiet rsh.socket
      if [[ $? -ne 0 ]]; then
         systemctl start rsh.socket
      fi
      systemctl is-enabled --quiet rsh.socket
      if [[ $? -ne 0 ]]; then
         systemctl enable --quiet rsh.socket
      fi
     fi

     if [[ -f $sysdDir/rlogin.socket ]]; then
      systemctl is-enabled --quiet rlogin.socket
      if [[ $? -ne 0 ]]; then
         systemctl enable --quiet rlogin.socket
      fi
      systemctl is-active --quiet rlogin.socket
      if [[ $? -ne 0 ]]; then
         systemctl start rlogin.socket
      fi
     fi

     if [[ -f $sysdDir/tftp.socket ]]; then
      systemctl is-enabled --quiet tftp.socket
      if [[ $? -ne 0 ]]; then
         systemctl enable --quiet tftp.socket
      fi
      systemctl is-active --quiet tftp.socket
      if [[ $? -ne 0 ]]; then
         systemctl start tftp.socket
      fi
     fi

     if [[ -f $sysdDir/tftp.service ]]; then
      systemctl is-enabled --quiet tftp.service
      if [[ $? -ne 0 ]]; then
         systemctl enable --quiet tftp.service
      fi
      systemctl is-active --quiet tftp.service
      if [[ $? -ne 0 ]]; then
         systemctl start tftp.service
      fi
     fi

   fi
fi

# On CentOS 7 systems, this is required for Infoproc to be able to register its socket
if [[ ! -z $sysdDir ]] ; then
   systemctl is-active --quiet rpcbind.service
   if [[ $? -ne 0 ]]; then
      systemctl add-wants multi-user rpcbind.service 2> /dev/null
      reboot=1
   fi
fi

#-----------------------------------------------------------------
# set write permission on serial port
#-----------------------------------------------------------------
if [[ -c /dev/ttyS0 ]] ; then
   chmod 666 /dev/ttyS0
fi

#-----------------------------------------------------------------
# Arrange for procs to start at system bootup
#-----------------------------------------------------------------
rm -f /etc/init.d/rc.vnmr
if [[ ! -z $sysdDir ]] ; then
   rm -f $sysdDir/vnmr.service $sysdDir/vnmrweb.service $sysdDir/bootpd.service
   cp $vnmrsystem/acqbin/vnmr.service $sysdDir/.
   chmod 644 $sysdDir/vnmr.service
   systemctl enable --quiet vnmr.service
   if [[ -f $vnmrsystem/acqbin/bootpd.service ]]; then
       cp $vnmrsystem/acqbin/bootpd.service $sysdDir/.
       chmod 644 $sysdDir/bootpd.service
       systemctl enable --quiet bootpd.service
   fi
else
   cp -p $vnmrsystem/acqbin/rc.vnmr /etc/init.d
   chmod +x /etc/init.d/rc.vnmr
   (cd /etc/rc5.d; if [ ! -h S99rc.vnmr ]; then 
    ln -s ../init.d/rc.vnmr S99rc.vnmr; fi)
   (cd /etc/rc0.d; if [ ! -h K99rc.vnmr ]; then
    ln -s ../init.d/rc.vnmr K99rc.vnmr; fi)
fi
touch $vnmrsystem/acqbin/acqpresent
owner=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($3) }')
group=$(ls -l $vnmrsystem/vnmrrev | awk '{ printf($4) }')
chown $owner:$group $vnmrsystem/acqbin/acqpresent
chmod 644 $vnmrsystem/acqbin/acqpresent

#-----------------------------------------------------------------
# Connection to FTS chiller if present
#-----------------------------------------------------------------
if [[ ! -f /etc/udev/rules.d/99-CP210x.rules ]] ; then
   cp $vnmrsystem/acqbin/99-CP210x.rules /etc/udev/rules.d/99-CP210x.rules
fi

# this deletes the statpresent file in /etc. This is not needed for
# INOVA. Because we created it at one point, we make extra sure it isn't
# there anymore
if [[ -f /etc/statpresent ]] ; then
   rm -f /etc/statpresent
fi

#-----------------------------------------------------------------
# Also, create /etc/norouter, so that no matter how the install
# is done (e.g. incomplete nets) the system does not become a router
#-----------------------------------------------------------------
if [[ ! -f /etc/notrouter ]] ; then
   touch /etc/notrouter
fi

#-----------------------------------------------------------------
# Remove some files (Queues) NOT IPC_V_SEM_DBM
# Because cleanliness is next to ... you know.
#-----------------------------------------------------------------
rm -f /tmp/ExpQs
rm -f /tmp/ExpActiveQ
rm -f /tmp/msgQKeyDbm
rm -f /tmp/ProcQs
rm -f /tmp/ActiveQ
rm -f /vnmr/acqqueue/ExpStatus

if [[ ${doingMI} -eq 0 ]] ; then
   installConsole
else
   installConsoleMI
fi
echo ""

echo ""
echo "NMR Console software installation complete"
echo ""

#-----------------------------------------------------------------
# Activate the NMR status web service
#-----------------------------------------------------------------
if [[ -d $vnmrsystem/web ]] ; then
   echo "Would you like to setup the Remote Status Unit"
   echo "web server (y/n) [y]? "
   read ans
   if [[ "x$ans" != "xn" ]] ; then
      ${vnmrsystem}/bin/rsu_bt_setup
   fi
   echo ""
fi

export vnmrsystem

#-----------------------------------------------------------------
if [ $reboot -eq 1 ]
then
   echo ""
   ${vnmrsystem}/bin/acqcomm help
   echo ""
   echo " "
   echo "You must reboot Linux for these changes to take effect"
   if [ -f /etc/debian_version ]; then
      echo "Enter 'sudo reboot' to reboot Linux"
   else
      echo "As root type 'reboot' to reboot Linux"
   fi
else
   if [[ ${doingMI} -eq 1 ]] ; then
      if [[ -z $sysdDir ]]; then
         (/usr/sbin/bootpd -s > /dev/console &)
      else
         systemctl start bootpd.service
      fi
   fi
   ${vnmrsystem}/acqbin/startStopProcs
   echo ""
   ${vnmrsystem}/bin/acqcomm help
   echo ""
fi

