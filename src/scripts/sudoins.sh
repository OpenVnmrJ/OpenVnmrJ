: /bin/sh
# 'sudoins.sh 2003-2008 '
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

# Writes the /etc/sudoers.

if test $# -lt 5
then 
    ostype=`uname -s`
    os_version="sol"
    configP11="no"
    NAWK="nawk"
    nmr_adm=`ls -l /vnmr/vnmrrev | awk '{print $3}'`
    nmr_group=`ls -l /vnmr/vnmrrev | awk '{print $4}'`

    if [ -f /vnmr/p11/part11Config ]
    then
	configP11="yes"
    fi

    if [ x$ostype = "xLinux" ]
    then
	NAWK="gawk"
	os_version="rht"
    elif [ x$ostype = "xInterix" ]
    then
	NAWK="awk"
	os_version="win"
    fi
    #echo "For Solaris systems type '/vnmr/bin/sudoins nawk no sol vnmr1'"
    #echo "For Linux systems type '/vnmr/bin/sudoins gawk no rht vnmr1'"
    
else
   configP11=$2
   os_version=$3
   nmr_adm=$4
   nmr_group=$5
   NAWK=$1  
fi
     
#default CAT to the command 'cat'
CAT="cat"

#if Linux then determine the Linux Flavor, Redhat, Debain (Ubuntu)
if [ x$os_version = "xrht" ]
then
    if [  -r /etc/debian_version ]
    then
        lflvr="debian"
        # Ubuntu has awk
        NAWK="awk"
        CAT="sudo cat "
    else
        lflvr="rhat"
    fi
fi

#Open Source Sudo package
# check for Redhat's then and Debian (Ubuntu) sudo locations
# reversed check, RHEL 6.X has both /usr/local/bin and /usr/bin sudo, however the /usr/local/bin version is 32-bit
# and the 32-bit libpam.so lib is not installed. With the search reversed we get the proper RHEL 6.X 64-bit sudo version. GMB
if [ -x /usr/bin/sudo ]; then
      SUDO="/usr/bin/sudo"
elif [ -x /usr/local/bin/sudo ]; then
      SUDO="/usr/local/bin/sudo"
else
      SUDO=""
fi

VISUDO="/usr/sbin/visudo"
ED="/bin/ed"
CP="/bin/cp"
CHMOD="/bin/chmod"
sudoers="/etc/sudoers"
sudoerstmp="/tmp/sudoers.vjtmp"
tempfile="/tmp/please_remove_me"
host_name=`uname -n`
# host_name="localhost"
append_to_sudoers="no"
sbindir="/usr/varian/sbin"
if [ x$os_version = "xwin" ]
then
   sbindir="/vnmr/.sbin"
fi
   
   # echo "sudoins: sudoins $NAWK $configP11 $os_version $nmr_adm" 
   # echo "sudoins: sudoins $NAWK $CAT $lflvr $nmr_adm" 

   sudo_cmds="$sbindir/adddevices,\
              $sbindir/dtsharcntrl,\
              $sbindir/makeuser,\
              $sbindir/jtestgroup,\
              $sbindir/jtestuser,\
              $sbindir/vnmr_accounting,\
              $sbindir/vcmdr,\
              $sbindir/vcmdm,\
              $sbindir/create_pgsql_user,\
              /usr/bin/getent,\
              /usr/bin/passwd,\
              /usr/sbin/useradd,\
	      /bin/mkdir,\
	      /bin/chown,\
              /usr/sbin/userdel"

   sudo_p11_cmds=", $sbindir/auconvert,\
                    $sbindir/auevent,\
                    $sbindir/auinit,\
                    $sbindir/aupurge,\
                    $sbindir/aureduce,\
                    $sbindir/auredt,\
                    $sbindir/aupw,\
                    $sbindir/killau,\
                    $sbindir/killch,\
                    $sbindir/chchsums,\
                    $sbindir/makeP11checksums,\
                    $sbindir/vnmrMD5,\
                    $sbindir/scanlog"

   sudo_group_cmds="$sbindir/probe_mount,\
                    $sbindir/probe_unmount"

if [ x$configP11 = "xyes" ]
then
   sudo_cmds=$sudo_cmds" "$sudo_p11_cmds
fi

if [ ! -f $sudoers ]
then
   echo "$sudoers does not exist"
   exit
fi

# for debian Never change the permission of the sudoers file!
# or you will pretty much hose the system!!
#
# for Redhat sudo will work for root without a password
# Thus use sudo for the general case and then both Redhat and Debian
# will be happy

# copy present sudoers to a working file to be modified
# sudo cp -p /etc/sudoers /etc/sudoers.vjtmp
$SUDO  $CP -p "$sudoers" "$sudoerstmp"
Date=`date +%y%m%d.%H:%M`
# sudo /bin/cp -p /etc/sudoers /etc/sudoers.bkup_${Date}
$SUDO $CP -p "$sudoers" "/etc/sudoers.bkup_${Date}"
$SUDO $CHMOD 666 "$sudoerstmp"


# ---------------------------------------------------------------------------------------------
regex="^$nmr_adm\b"
org_sudo_str=`grep "$regex" "$sudoerstmp" | awk 'BEGIN {FS=":"} {print $2}'`
org_no_comma=`echo $org_sudo_str | sed 's/,//g'`
append_to_sudoers="yes"     #echo nmr_adm is not in the list

if [ ! -z "$org_sudo_str" ]
then
   # use ed to find and delete the vnmr admin sudo line
   # so it can just be replaced
   $ED $sudoerstmp << THEEND
/$regex/
d
w
q
THEEND
# ---------------------------------------------------------------------------------------------
#
# alternative  method of ed
#           # use ed to append the new sudo line
#           newline="$nmr_adm $host_name = NOPASSWD: $sudo_cmds"
#           /bin/ed "$sudoerstmp" << THEEND
#a
#$newline
#.
#w
#q
#THEEND
fi

# ---------------------------------------------------------------------------------------------
# add probe_mount and probe_unmount commands for NMR users
regex="^%$nmr_group\b"
grp_sudo_str=`grep "$regex" "$sudoerstmp" | awk 'BEGIN {FS=":"} {print $2}'`
grp_no_comma=`echo $grp_sudo_str | sed 's/,//g'`

if [ ! -z "$grp_sudo_str" ]
then
   # use ed to find and delete the vnmr admin sudo line
   # so it can just be replaced
   $ED $sudoerstmp << GROUPEND
/$regex/
d
w
q
GROUPEND
fi
# ---------------------------------------------------------------------------------------------

if [ "x$append_to_sudoers" = "xyes" ]
then
   # for RHEL 5.X comment out the 'Default    requiretty' 
   # so that desktop started VnmrJ Admin can use sudo, 
   # otherwise it won't be able,  resulting in failure to
   # set parameters, etc. that require root permission

   #
   # obtain the search string to be used in ed
   #
   requiresttyline=`grep requiretty "$sudoerstmp"` 

   firstfield=`echo $requiresttyline | awk '{print $1}'`
   alreadycommented="n"
   if [ "x$firstfield" = "x#" ]; then
      alreadycommented="y"
   fi

   #
   # use ed to find and comment out the line if it exists
   #
   if [ ! -z "$requiresttyline" -a "x$alreadycommented" = "xn" ]; then
      $ED "$sudoerstmp" << THEEND
,s/$requiresttyline/# $requiresttyline/g
w
q
THEEND
   fi

   #
   # addend the sudo cmd for vnmr1
   #
   echo "$nmr_adm $host_name = NOPASSWD: $sudo_cmds" >> "$sudoerstmp"
   echo "%$nmr_group $host_name = NOPASSWD: $sudo_group_cmds" >> "$sudoerstmp"

#
#       newline="$nmr_adm $host_name = NOPASSWD: $sudo_cmds"
#      or     # use ed to append the new sudo line
#           newline="$nmr_adm $host_name = NOPASSWD: $sudo_cmds"
#           /bin/ed "$sudoerstmp" << THEEND
#a
#$newline
#.
#w
#q
#THEEND

       # more general method than the one below would be to create a temp file and modified
       # it a more general way, then test it's syntax with visudo, if it passes then copy over the
       # the real sudoers files, (make a backup of course)   GMB  7/7/08
       #
       # debian (Ubuntu) requires mod to sudoers file to be done
       # via visudo app. 

       # the new line to be added to the sudoers file
#       newline="$nmr_adm $host_name = NOPASSWD: $sudo_cmds"

       # copy present sudoers to a working file to be modified
#       sudo cp -p /etc/sudoers /etc/sudoers.vjtmp
#       Date=`date +%y%m%d.%H:%M`
#       sudo cp -p /etc/sudoers /etc/sudoers.bkup_${Date}
       # use the ex mode of vi to append a line to the end
#       export EDITOR=ex && sudo -E visudo -f /etc/sudoers.vjtmp << THEEND
#a
#$newline
#.
#w
#q
#THEEND
     #
     # check sudoers file for syntax errors
     #
     # changed permission to proper values, 
     # since VISUDO now check for proper file permissions
     # as well as syntax error
     $SUDO $CHMOD 440 "$sudoerstmp"
     #  $SUDO $VISUDO -c -s  -f "$sudoerstmp"
     $SUDO $VISUDO -c -f "$sudoerstmp"
     if [ $? -eq 0 ] 
     then
         # No errors so copy tmp file on top of sudoers file
         $SUDO $CHMOD 440 "$sudoerstmp"
         $SUDO $CP -p "$sudoerstmp" "$sudoers"
         # $SUDO $CHOWN root:root "$sudoers"
         # add the sudo cp /etc/sudoers.vjtmp /etc/sudoers
         # sudo rm -f /etc/sudoers.vjtmp
         #echo " uncomment lines in sudions.sh to enable debian sudoers file change ... "
     else
         echo "sudoer file syntax error, file unchanged."
     fi
   fi

   if [ x$os_version = "xwin" ]
   then
      chmod 400 "$sudoers"
      chown Administrator "$sudoers"
   fi
  
#   echo "sudoins done"
