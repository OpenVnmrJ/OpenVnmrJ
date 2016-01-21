: '@(#)setupscanlog.sh 20.1 07/09/07 1999-2002 '
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
#!/usr/bin/sh
#
# setupscanlog.sh
# Setting up facilities for syslog, run only once
# This file will be removed from Vnmr system files at the end
# of the software installation

syslog_conf_file="/etc/syslog.conf"
authlog_name="authlogp11"
auth_log=/var/log/$authlog_name
login_log="/var/log/loginlog"

syslog_conf1="#P11_monitoring"
syslog_conf2="auth.notice                        $auth_log"


if [ ! -r $auth_log ]
then
    touch $auth_log
    chmod 600 $auth_log
    chown root:sys $auth_log
fi

if [ ! -r $login_log ]
then
    touch $login_log
    chmod 600 $login_log
    chown root:sys $login_log
fi

if [ ! -r $tmp_path ]
then
    touch $tmp_path
    chmod 600 $tmp_path
    chown root:sys $tmp_path
fi


if [ -r $syslog_conf_file ]
then
   grep $authlog_name $syslog_conf_file
   if [ $? -eq 1 ]
   then
       echo $syslog_conf1 >> $syslog_conf_file
       echo $syslog_conf2 >> $syslog_conf_file
   fi
else
   echo $syslog_conf1 >> $syslog_conf_file
   echo $syslog_conf2 >> $syslog_conf_file
fi

