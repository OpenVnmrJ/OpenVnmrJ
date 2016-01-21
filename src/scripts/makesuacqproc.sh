: '@(#)makesuacqproc.sh 22.1 03/24/08 1991-1996 '
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

#  common_env and common_whoami are currently for SunOS and Solaris only

common_env()
{
    ostype=`uname -s`
    osmajor=`uname -r | awk 'BEGIN { FS = "." } { print $1 }'`

    if [ $osmajor -lt  5 ]
    then
        svr4="n"
    else
        svr4="y"
        ostype="solaris"
    fi
}

####################################################################
#  script function to obtain value for vnmrsystem
#  implemented as a separate function, rather than inline,
#  because it is called at a different place,
#  depending on whether the current account is root or non-root
####################################################################

get_vnmrsystem() {

#  Get value of vnmrsystem
#  If not defined, ask for its value
#  Use /vnmr as the default
#  make sure directory exists

    if test "x$vnmrsystem" = "x"
    then
        echo "Please enter location of VNMR system directory [/vnmr]: \c"
        read vnmrsystem
        if test "x$vnmrsystem" = "x"
        then
            vnmrsystem="/vnmr"
        fi
    fi

    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, cannot proceed:"
        exit
    fi
}

####################################################################
#  Script function to add an entry to /etc/shadow
#  required for Solaris - if no shadow entry is present, then it is
#  not possible to su to the account, even if it is present in passwd
####################################################################

add_shadow_entry() {

    awk '
        BEGIN { FS = ":"; entry_present = 0 }
	{
		print $0
		if ($1 == "'$name'")
		  entry_present = 1
	}
	END {
		if (entry_present == 0)
		  print "'$name':::0:::::"
	}
    ' </etc/shadow >/etc/shadow.new
     
    (cd /etc; mv -f shadow.new shadow; chgrp sys shadow; chmod 400 shadow)
}

# ------------- main ------------------

common_env

notroot=0
userId=`/usr/bin/id | awk 'BEGIN { FS = " " } { print $1 }'`
if [ x$userId != x"uid=0(root)" ]; then
  notroot=1
  echo
  echo "To create the acqproc user you will need to be the system's root user."
  echo "Or type cntrl-C to exit."
  echo
  s=1
  t=3
  while [ $s = 1 -a ! $t = 0 ]; do
     echo "Please enter this system's root user password"
     su root -c "$0 ${ARGS}";
     s=$?
     t=`expr $t - 1`
     echo " "
  done
  if [ $t = 0 ]; then
      echo "Access denied. Type cntrl-C to exit this window."
      echo "Type $0 to start the installation program again"
      echo
  fi
  exit
fi

get_vnmrsystem

if test -x "$vnmrsystem"/bin/execkillacqproc
then
    chown root "$vnmrsystem"/bin/execkillacqproc
    chmod 500  "$vnmrsystem"/bin/execkillacqproc
else
     echo "$vnmrsystem/bin/execkillacqproc  does not exist, cannot proceed:"
     exit
fi 
 
name=acqproc
nmrgnum=65534		#this is the nogroup, at least in Sol 5.6, 5.7, 5.8

#  lookup username in the password file
#  if not present, /tmp/newuser will have password file
#  entry and a size larger than 0
#  keep user-id number within bounds of positive 16-bit numbers,
#    that is, less than 32768

awk '
BEGIN {N=0
       AlreadyExists=0
       NewUser="'$name'"
       FS=":"
      }
{
if ($3>N && $3<32768) N=$3
if ($1==NewUser) AlreadyExists=1
}
END {if (AlreadyExists==0)
          printf "%s::0:'$nmrgnum':I Start or Stop Acqproc:'$vnmrsystem'/bin:'$vnmrsystem'/bin/execkillacqproc\n",NewUser}
' < /etc/passwd > /tmp/newuser

#  If new entry required, insert it on
#  the next to the last line in the file

if test -s /tmp/newuser
then
     new_account="y"
     cp /etc/passwd /etc/passwd.bk
     read stuff </tmp/newuser
     (sed '$i\
'"$stuff"'' /etc/passwd >/tmp/newpasswd)
     mv /tmp/newpasswd /etc/passwd
     rm /tmp/newuser
else
     new_account="n"
fi
(cd /etc; chgrp sys passwd; chmod 444 passwd;)

if [ x$ostype = "xsolaris" -o x$ostype = "xLinux" ]
then
    add_shadow_entry
fi

#  Username now in password, group file
#  use backquotes to move output from script function into script variable

if test $new_account = "y"
then
    echo
    echo $name" is now a login that will start or kill Acqproc"
else
    echo
    echo $name" is already a defined user"
fi

echo "$0 complete."

