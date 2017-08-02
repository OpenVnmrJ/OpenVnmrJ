#!/bin/bash
#
# This script will add information to OVJ adm files for the specified
# user to enable them to use the OVJ graphical interface. This can
# be used instead of the OVJ admin interface. The three steps are:
#   1. Create the user and user's home directory. This can be done
#      with the /usr/sbin/useradd command and requires root permission.
#   2. Update the new user's HOME directory and create the "vnmrsys"
#      files and directories. This can be done with the /vnmr/bin/makeuser
#      script and should be executed as the new user.
#   3. Update the OVJ administration files that are maintained in
#      /vnmr/adm. This script (ovjUser) will do those tasks. It needs to
#      be invoked by the OVJ system admin account, which is typically vnmr1.
#      The new user name is passed as an argument, as in
#      /vnmr/bin/ovjUser <newUserName>
#
# set -x

name=$1
if [ x$vnmrsystem = "x" ]
then
   vnmrsystem=/vnmr
fi

dir=$vnmrsystem/adm/users/profiles/system
if [ ! -d $dir ]; then
   mkdir $dir
   chmod 755 $dir
fi
file=$dir/$name
if [ ! -f $file ]; then
  cat <<- EOF | sed -e s/USER/$name/ | tee $file
	update	Yes
	home	/home/USER
	name	
	access	all
	itype	Spectroscopy
	owned	/home/USER
	EOF
fi
chmod 644 $file

dir=$vnmrsystem/adm/users/profiles/user
if [ ! -d $dir ]; then
   mkdir $dir
   chmod 755 $dir
fi
file=$dir/$name
if [ ! -f $file ]; then
  cat <<- EOF | sed -e s/USER/$name/ | tee $file
	appdir	Spectroscopy
	accname	USER
	sysdir	/vnmr
	operators	
	userdir	/home/USER/vnmrsys
	datadir	/home/USER/vnmrsys/data /home/USER/vnmrsys/parlib /home/USER/vnmrsys/shims
	EOF
fi
chmod 644 $file

file=$vnmrsystem/adm/users/userlist
if [ ! -f $file ]; then
   echo vnmr1 > $file
fi
grep -w $name $file >& /dev/null
if [ $? -ne 0 ]
then
   mv $file $file.bk
   cat $file.bk | sed s/vnmr1/vnmr1\ $name/ > $file
   rm $file.bk
fi
chmod 644 $file

file=$vnmrsystem/adm/users/group
if [ ! -f $file ]; then
   echo "vnmr:VNMR group:vnmr1" > $file
   echo "agilent and me:Myself and system:me, agilent" >> $file
fi
grep -w $name $file >& /dev/null
if [ $? -ne 0 ]
then
   mv $file $file.bk
   cat $file.bk | sed s/vnmr1/vnmr1,$name/ > $file
   rm $file.bk
fi
chmod 644 $file

file=$vnmrsystem/adm/users/operators/operatorlist
grep -w $name $file >& /dev/null
if [ $? -ne 0 ]
then
   echo "$name  $name;null;30;null;AllLiquids"  >> $file
fi
chmod 644 $file
