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

ostype=$(uname -s)
set_system_stuff() {
    case x$ostype in
          "xLinux")
                           sysV="n"
                           default_dir="/home"
                           ;;
	 
	  "xDarwin")
                           sysV="n"
                           default_dir="/Users"
                           ;;

	   *)
                           sysV="n"
                           default_dir="/home"
			   ;;
esac

    if test ! -d "$default_dir"
    then
        default_dir="/"
    fi
}

####################################################################
#  echo without newline - needs to be different for BSD and System V
####################################################################
nnl_echo() {
    if test x$sysV = "x"
    then
        echo "error in echo-no-new-line: sysV not defined"
        exit 1
    fi

    if test $sysV = "y"
    then
        if test $# -lt 1
        then
            echo
        else
            echo "$*\c"
        fi
    else
        if test $# -lt 1
        then
            echo
        else
            echo -n $*
        fi
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

    if test x"$vnmrsystem" = "x"
    then
        # with 5.3 the vnmrsystem env is not set when VnmrJ adm sudo makeuser
        # thus test if this has been envoke as a sudo command
        # could test SUDO_USER, SUDO_GID, or SUDO_COMMAND
        # if SUDO_USER has a value then don't ask for vnmrsystem just default 
        # to /vnmr     GMB 5/4/2009
        if [ "x" = "x$SUDO_USER" ]; then
           nnl_echo  "Please enter location of VNMR system directory [/vnmr]: "
           read vnmrsystem
           if test x"$vnmrsystem" = "x"
           then
               vnmrsystem="/vnmr"
           fi
        else
         vnmrsystem="/vnmr"
        fi
    fi

    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, cannot proceed:"
        exit 1
    fi
}

####################################################################
#  Script function to get thr groupname to use
####################################################################
get_group()
{
   if [ "x$1" != "x" ]
   then 
      nmr_group=$1
   elif [ x$ostype = "xDarwin" ]
   then
      nmr_group=`id -gn`
   else
      cd "$vnmrsystem"
      nmr_group=`ls -ld . | awk '{ print $4 }'`
   fi
}

####################################################################
#  Script function to extract home directory 
#  if nonexistant echo empty string.
####################################################################

get_homedir() {
    if [ -d "$(eval echo "~$1")" ]; then
        echo "$(eval echo "~$1")"
    else
        echo ""
    fi
}

####################################################################
#  Script function to make the home directory
####################################################################

make_homedir() {
    if mkdir "$cur_homedir"
    then
        chmod 775 "$cur_homedir"
	chown -R "$name_add" "$cur_homedir"
	chgrp -R "$nmr_group" "$cur_homedir"
        return 0
    else
        echo "Cannot create homedir '$cur_homedir'"
        return 1
    fi
}

####################################################################
#  Script function to verify the user name has no invalid characters
####################################################################

verify_user_name() {
    namelen=`echo "$1" | awk '{ print length( $0 ) }'`
    if [ $namelen -lt 1 ]
    then
        echo "No user name was given"
        return 1
    fi

    echo "$1" | awk '
    /^[0-9A-Za-z_][0-9A-Za-z_-]*$/ {
	exit 0
    }

    #  if the input does not match the above pattern, then the name is rejected
    #  locate the offending character and point it out

    {
        namelen = length( $0 )
	for (iter = 1; iter <= namelen; iter++) {
            onechar = substr( $0, iter, 1 )
            if (onechar == "-" && iter == 1) {
                print "Leading '-' not allowed in a user name"
                break
            }
            if ((onechar < "A" || onechar > "Z") && (onechar < "a" || onechar > "z") && \
		(onechar < "0" || onechar > "z") && (onechar != "_") && (onechar != "-")) {
                print "User name " $0 " not permitted, invalid character"
                  for (jter = 1; jter < iter+10; jter++)
                    printf( " " )
                print( "^" )
                if (iter > 1)
                  for (jter = 1; jter < iter+10; jter++)
                    printf( " " )
                print( "|" )
                break
            }
	}
        exit 1
    }
    '
}

run_updaterev () {

   if [ x$ostype = "xLinux" ]
   then
       (export PATH; PATH="$PATH":"$vnmrsystem"/bin; \
        cd "$vnmrsystem"/bin; Vnmrbg -mback -n1 "updaterev" 2> /dev/null)
   else
       (export PATH; PATH="$PATH":"$vnmrsystem"/bin; cd "$vnmrsystem"/bin; Vnmrbg -mback -n1 "updaterev")
   fi

}

###########################################################################
#  Script function to install a file and make a backup if it exists already
###########################################################################

cp_backup() {
    srcfile="$1"
    dstfile="$2"
    base="$(basename "${dstfile}")"
    if [ -s "$dstfile" ]; then
	mv -f "$dstfile" "${dstfile}.bkup.$date"
	echo "  ${base} backed up in ${base}.bkup.$date";
    fi
    /bin/cp "$srcfile" "$dstfile"
    chown "$name_add":"$nmr_group" "$dstfile"
    echo "  $base updated from templates."
}

setWallPaper() {
  if [[ $as_root = "y" ]]; then
    if [[ x$lflvr != "xdebian" ]]; then
      su - ${user_name} -c "/vnmr/bin/ovjWallPaper"
    else
      sudo su - ${user_name} -c "/vnmr/bin/ovjWallPaper"
    fi
  else
    /vnmr/bin/ovjWallPaper
  fi
}

setDeskTop() {
   cp "$vnmrsystem"/user_templates/$1 Desktop/.
   if [[ $as_root = "y" ]]; then
      chown "$name_add":"$nmr_group" $1
   fi
   if [[ -d ./.local/share/applications ]]; then
      cp "$vnmrsystem"/user_templates/$1 .local/share/applications/.
      if [[ $as_root = "y" ]]; then
         chown "$name_add":"$nmr_group" .local/share/applications/$1
      fi
   fi
}

##########################
#  start of main script
##########################
if [ "x$vnmrsystem" = "x" ]
then
   vnmrsystem="/vnmr"
   . /vnmr/user_templates/.vnmrenvsh
fi
set_system_stuff
curr_user=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
new_account="n"
user_name=$1
user_dir="$2"
user_group=$3
user_update=$4  #"y" or "n", or "appmode"
date=`date +%y%m%d.%H:%M`

if [ $# -ge 4 ] && [ x$ostype = "xDarwin" ]  #Called from Java
then
#  exit if called from java and system is a Mac
   exit 0
fi

# determine if this is a debian linux flavor, if so must use sudo rather than su
#  e.g. sudo -u User2ExecuteCmdAs Cmd_To_Execute
# When it becomes needed the script can be moded to use lflvr, but is not implement now.  GMB 7/22/08
# See inv_vnmr2.sh for example of howto.
if [ -r /etc/debian_version ]
then
   lflvr="debian"
else
   lflvr="rhat"
fi

# cmd /usr/local/bin/sudo /usr/varian/sbin/makeuser $user_name $HOME nmr appmode
# run by vnmrj adm (when saving user) to set vnmrbg appmode
if [ x$user_update = "xappmode" ]
then
  if [ x$lflvr != "xdebian" ]
  then
      # the carrage return is for the remote display question 
      su - "$user_name" -c "/vnmr/bin/Vnmrbg -mback -n1 'setappmode'" <<+

+
  else
      sudo su - "$user_name" -s /bin/bash -c "/vnmr/bin/Vnmrbg -mback -n1 setappmode"
  fi
  exit 0
fi

if [ $# -ge 4 ] && [ "x$user_update" = "xy" ]  #Called from Java
then
   intera_unix="n"
   intera_vnmr="n"
fi

if [ "x$curr_user" = "xroot" ]
then
    as_root="y"
fi

if [ x$as_root != "xy" ]
then
    if [ x"$HOME" = "x/" ]
    then
        echo "no home directory, cannot proceed with $0"
        exit 1
    else
        name_add=$curr_user
        if [ $# -ge 4 ] && [ "x$user_update" = "xy" ]
        then
          cur_homedir="$2"
          if [ $# -eq 4 ]
	  then
	    get_vnmrsystem
	  elif [ $# -gt 4 ] 
	  then
	    vnmrsystem="$5"
	  fi
          get_group "$3"
        else
          cur_homedir="$HOME"
          get_vnmrsystem
          get_group "$3"
        fi
        as_root="n"
    fi

    if [ $# -ne 0 ]
    then
        if [ "x$1" != "x$name_add" ]
        then
           echo "Only $1 or root can change or add $1's account"
           exit 1
        fi
    fi

else  #current user is root

    #  get group number for `nmr' group; exit if none
    #  use backquotes to get nmr group number into script
    #  variable `nmrgnum'.  If no `nmr' group, abort

    if [ x$ostype = "xDarwin" ]  #Cannot run as root on a Mac
    then
       if test $# -eq 2
       then
          su - "$1" -c "$vnmrsystem/bin/makeuser \"$1\" \"$2\""
          exit
       else
          echo "Cannot run makeuser as root"
          exit 1
       fi
    fi
    as_root="y"

    get_vnmrsystem

    get_group "$3"
    my_file=/tmp/my.file.$date
    touch "$my_file"
    chgrp "$nmr_group" "$my_file" 2>/dev/null
    if [ $? -ne 0 ]
    then
        rm -f "$my_file"
        echo "the '$nmr_group' group must be present to configure accounts for VNMR use"
        exit 1
    else
        nmrgnum=`ls -ln $my_file | awk '{ printf $4}'`
    fi
    rm -f "$my_file"

    #  with the presence of the `nmr' group verified and /vnmr present,
    #  get the name of the user if not entered on the command line

    if test $# -eq 0
    then
        nnl_echo  "Please enter user name [vnmr1]: "
        read name_add
        if test "x$name_add" = "x"
        then
            name_add="vnmr1"
        fi
    else
        name_add=$1
	if test $# -gt 1
	then
	    default_dir="$2"
	fi
    fi

    # if verify user name rejects the name, it tells you about it
    # the Bourne shells if-construct does not allow for a non-zero
    # status to take the first branch.  It also insists on something
    # between the then and the else

    if ! verify_user_name "$name_add"
    then
        exit 1
    fi
    cur_homedir=$(get_homedir "$name_add")
 
    #  lookup username in the password file
    #  if not present, /tmp/newuser will have password file
    #  entry and a size larger than 0
    #  keep user-id number within bounds of positive 16-bit numbers,
    #    that is, less than 32768

    rm -f /tmp/newuser
    if [ x"$cur_homedir" = "x" ]
    then
	    awk '
	    BEGIN {N=1000
		AlreadyExists=0
		NewUser="'"$name_add"'"
		FS=":"
	    }
	    {
		if ($3>N && $3<32768) N=$3
		if ($1==NewUser) AlreadyExists=1
	    }
	    END {  if (AlreadyExists==0)
		printf "%d\n",N+1
	    }
	    ' < /etc/passwd > /tmp/newuser
    else
        if [ ! -d "$cur_homedir" ]
        then
            if ! make_homedir
            then
                exit 1
            fi
        else
            # The home directory already exists.  Be sure it is owned by $name_add
            username=$(stat -c %U "$cur_homedir")
            if [ "x$username" != "x$name_add" ]
            then  
                chown -R "$name_add" "$cur_homedir"
            fi
        fi

        if touch "${cur_homedir}/makeuser.${date}" 2>/dev/null
        then
            rm -f "${cur_homedir}/makeuser.${date}"
        else
	   if [ ! $# -ge 4 ] 
	   then 
		echo "Cannot create files in the home directory of '$name_add' ($cur_homedir)"
		echo "Is this directory exported from a remote system using NFS?"
		exit 1
	    else # called from java, network user
		user_update="n"
	    fi
        fi
    fi

    #  If new entry required, insert it on
    #  the next to the last line in the file

    if test -s /tmp/newuser
    then
        new_account="y"

        if [ $# -ge 4 ]  #Called from java, No question be asked 
	then
	    dir_name="$user_dir"
	else
           nnl_echo  "Please enter home directory for '$name_add' [$default_dir]: "
           read dir_name
           if test x"$dir_name" = "x"
           then
              dir_name="$default_dir"
           fi
	fi

	read num </tmp/newuser
	rm /tmp/newuser
	if [ x$ostype = "xLinux" ]
        then
            if [ x$lflvr != "xdebian" ]
            then
                /usr/sbin/useradd -d"$dir_name/$name_add" -s/bin/bash -u$num -g$nmrgnum "$name_add"
                /usr/bin/passwd -f -u "$name_add"
	        chmod 755 "$dir_name/$name_add"
                echo ""
                echo "User $name_add has no initial password"
                echo ""
            else
                # --home-create == -m, --home == -d, --shell == -s, --gid == -g
                # must give the account a temp password 'abcd1234' to get the account active 
                # since passwd does not have the -f to force activation as does RHEL
                # passwd abcd1234 = $1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1
                # add user to appropriate groups, the special group admin and/or adm  allows user
                # to use 'root' sudo 
                # if vnmr1 then give it root access via sudo by adding the admin group to vnmr1
                # other users should not have root access
                if [ "x$name_add" = "xvnmr1" ] ; then
                    sudo /usr/sbin/useradd --create-home --home-dir "$dir_name/$name_add" \
                         --shell /bin/bash --uid $num --gid "$nmr_group" \
                         --groups cdrom,floppy,audio,video,plugdev,lpadmin \
                         --password '$1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1' "$name_add"
                else
                    sudo /usr/sbin/useradd --create-home --home-dir "$dir_name/$name_add" \
                         --shell /bin/bash --uid $num --gid "$nmr_group" \
                         --groups cdrom,floppy,audio,video,plugdev,lpadmin \
                         --password '$1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1' "$name_add"
                fi
                sudo chmod 755 "$dir_name/$name_add"
                # we give a temp default password, use the --expire
                # option to force to user to change password on login
                # sudo /usr/bin/passwd --expire "$name_add" 2>/dev/null
                sudo /vnmr/bin/setupbashenv "$dir_name/$name_add"
                echo ""
                echo "User $name_add has an initial password of abcd1234"
                echo ""
            fi
        else
            /bin/cp /etc/passwd /etc/passwd.bk
            
            stuff="$name_add::$num:$nmrgnum:$name_add:$dir_name/$name_add:/bin/bash"

            (sed '$i\
'"$stuff"'' /etc/passwd >/tmp/newpasswd)
            mv -f /tmp/newpasswd /etc/passwd
            chmod 644 /etc/passwd
	    chown -R "$name_add" "$dir_name/$name_add"
        fi

	# from Java only for now
	if [ $# -ge 4 ] && [ x$new_account = "xy" ]
	then
		recfile="$vnmrsystem/adm/users/uexist"

		if [ ! -r $recfile ]
		then 
		    touch $recfile
		fi

		today=`/bin/date`
		echo "${name_add}* ${today}* active" >> $recfile
	fi
    fi

	#  Now lookup username in the nmr group
	#  If not present, add this user to the group
	awk '
	BEGIN { FS=":" }
	{
	    if ($1=="'"$nmr_group"'")
	    print $0
	}' </etc/group | grep "$name_add" >/dev/null

	#  Anchor the search string to the start of the line in /etc/group
	if [ $? -ne 0 ]
	then
	    sed -e '/^'"$nmr_group"'/ s/$/,'"$name_add"'/' < /etc/group > /tmp/newusergroup
	    mv -f /tmp/newusergroup /etc/group
	    if [ x$new_account = "xn" ]
	    then
		echo "'$name_add' added to the '$nmr_group' group"
	    fi
	fi

    #  Username now in password, group file
    cur_homedir=$(get_homedir "$name_add")
    #  Make home directory if not present
    if [ ! -d "$cur_homedir" ]
    then
        if ! make_homedir
        then
            exit 1
        fi
    else

    #  the following test for exported file systems may well be redundant,
    #  it does not hurt to check this out again though ...

        if touch "$cur_homedir"/makeuser.$date 2>/dev/null
        then
            rm -f "$cur_homedir"/makeuser.$date
        else
	    if [ ! $# -ge 4 ]
	    then
		echo "Cannot create files in the home directory of '$name_add' ($cur_homedir)"
		echo "Is this directory exported from a remote system using NFS?"
		exit 1
	    else # called from java, network user
		user_update="n"
	    fi
        fi

        # The home directory already exists.  Be sure it is owned by $name_add
        username=$(stat -c %U "$cur_homedir")
        if [ "x$username" !=  "x$name_add" ]
        then  
            chown -R "$name_add" "$cur_homedir"
        fi
    fi

    #  Ask if OK to update account

    if [ x$new_account = "xy" ]
    then
        echo "'$name_add' is now an authorized user in the '$nmr_group' group"
    else
        echo "'$name_add' is already a defined user"
    fi

    if [ $# -lt 4 ] #if called from Java, do not ask this
    then
       nnl_echo  "Do you wish to update files for '$name_add' (y or n) [n]: "
       read yesno
       if [ "x$yesno" != "xy" ]
       then
           exit
       fi
    fi
    if [ $# -ge 4 ]  #Called from Java
    then
	if [ x$user_update = "xy" ]
	then
            cp_backup "$vnmrsystem/user_templates/.login" "$cur_homedir/.login"
            if [ x$ostype = "xLinux" ]
            then
                cp_backup "$vnmrsystem/user_templates/.vnmrenv" "$cur_homedir/.vnmrenv"
                cp_backup "$vnmrsystem/user_templates/.vnmrenvbash" "$cur_homedir/.vnmrenvbash"
                cp_backup "$vnmrsystem/user_templates/.vnmrenvsh" "$cur_homedir/.vnmrenvsh"
                cp_backup "$vnmrsystem/user_templates/.vxresource" "$cur_homedir/.vxresource"
            fi
	fi
        if [[ x$lflvr != "xdebian" ]]; then
	  su - "$name_add" -c "/vnmr/bin/makeuser '$1' '$cur_homedir' '$nmr_group' $user_update '$vnmrsystem'" 2> /dev/null
	  exit
        else
	  sudo su - "$name_add" -c "/vnmr/bin/makeuser '$1' '$cur_homedir' '$nmr_group' $user_update '$vnmrsystem'"
	  exit
        fi
    fi

fi # end of root-specific commands

#-----------------------------------
# remainder of script is identical for root or non-root account
# except if root the ownership and group membership of these
# files is set at the very end

sys_user=`ls -l "$vnmrsystem"/vnmrrev | awk '{ printf $3}'`

intera_unix="n"
intera_vnmr="n"

if [ "x$ostype" = "xDarwin" ] && [ $# -eq 2 ] && [ "$2" = "y" ]
then
  intera_vnmr="n"
else
if [ $# -lt 4 ]
then
  
  nnl_echo  "Automatically configure user '$name_add' account (y or n) [y]: "
  read yesno
  if test "x$yesno" = "xn" -o "x$yesno" = "xno"
  then
     nnl_echo  "Automatically configure UNIX environment (.files) (y or n) [y]: "
     read yesno
     if test "x$yesno" = "xn" -o "x$yesno" = "xno"
     then
        intera_unix="y"
     fi
     nnl_echo  "Automatically configure VNMR directories (y or n) [y]: "
     read yesno
     if test "x$yesno" = "xn" -o "x$yesno" = "xno"
     then
        intera_vnmr="y"
     fi
  fi
fi
fi

cd "$cur_homedir"
if [ x$ostype = "xDarwin" ]
then
  vim_files=
else
  vim_files="vim_magical"
fi
for file in $vim_files
do
   if test x$file = "xvim_magical"
   then
      local_file=".vim"
   else
      local_file=$file
   fi
   if test $intera_unix = "y"
   then
      nnl_echo  "OK to update $local_file (y or n) [y]: "
      read yesno
       if test "x$yesno" = "xn" -o "x$yesno" = "xno"
       then
           continue
       fi
   fi
   if test -d  $local_file
   then
       mv -f "$local_file"  "$local_file.bkup.$date"
       echo "  $local_file backed up in $file.bkup.$date";
   fi
   /bin/cp -R "$vnmrsystem/user_templates/$file" "$local_file"
   if test $as_root = "y"
   then
        chown -R "$name_add" "$local_file"
        chgrp -R "$nmr_group" "$local_file"
   else
        chgrp -R "$nmr_group" "$local_file"
   fi
   echo "  $file updated from templates."
done


if [ "x$ostype" = "xDarwin" ] && [ $# -eq 2 ] && [ "$2" = "y"  ]
then
   dotfiles=
else
   dotfiles=`(cd "$vnmrsystem"/user_templates && ls .??*)`
fi
for file in $dotfiles
do
    if [ $# -ge 4 ]  #Called from Java
    then
	if [ x$user_update = "xy" ]
	then
	    if [ "x$file" = "x.login" ]
 	    then	
		continue
	    fi
	elif [ x$user_update = "xn" ]
	then
	    continue
	fi
    fi
    if test $intera_unix = "y"
    then
        nnl_echo  "OK to update $file (y or n) [y]: "
        read yesno
        if test "x$yesno" = "xn" -o "x$yesno" = "xno"
        then
            continue
        fi
    fi
    if test -f "$file"
    then
        if [ $intera_unix = "n" ] && [ "x$file" = "x.cshrc" ]
        then
          continue
        fi
        mv -f "$file" "$file.bkup.$date"
        echo "  $file backed up in $file.bkup.$date";
    fi
    /bin/cp "$vnmrsystem/user_templates/$file" .
    chmod 644 "$file"
    if test "$file" = ".openwin-init"
    then
       chmod +x .openwin-init
    fi
    if [ "$file" = ".xinitrc" ] || [ "$file" = ".mwmrc" ]
    then
       chmod +x "$file"
    fi
    if test $as_root = "y"
    then
        chown "$name_add" "$file"
        chgrp "$nmr_group" "$file"
    else
        chgrp "$nmr_group" "$file"
    fi
    echo "  $file updated from templates."
done

if test -f .bash_profile
then
   envFound=`grep vnmrenvbash .bash_profile`
   if [ -z "$envFound" ] ; then
      cat "$vnmrsystem"/user_templates/profile >> .bash_profile
   fi
elif test -f .profile
then
   envFound=`grep vnmrenvbash .profile`
   if [ -z "$envFound" ] ; then
      cat "$vnmrsystem"/user_templates/profile >> .profile
   fi
fi

if test ! -d vnmrsys
then
    mkdir vnmrsys
    chmod 755 vnmrsys
    if test $as_root = "y"
    then
        chown "$name_add" vnmrsys
        chgrp "$nmr_group" vnmrsys
    else
        chgrp "$nmr_group" vnmrsys
    fi
fi

# Imaging Files and Directories
imgfiles="ib_initdir csi_initdir"
for file in $imgfiles
do
    if [ "$file" = "ib_initdir" ] || [ "$file" = "csi_initdir" ]
    then
	if test -d "$vnmrsystem/user_templates/$file"
	then
	   if [ -d "$cur_homedir/vnmrsys/$file" ] && [ $intera_unix = "y" ]
    	   then
        	nnl_echo  "OK to update $file (y or n) [y]: "
        	read yesno
                if test "x$yesno" = "xn" -o "x$yesno" = "xno"
        	then
        	    continue
        	fi
	   fi
	   if test -d "$cur_homedir/vnmrsys/$file"
	   then
		mv -f "$cur_homedir/vnmrsys/$file" "$cur_homedir/vnmrsys/$file.bkup.$date"
		echo "  $file backed up in $file.bkup.$date";
	   fi
	   # Copy with tar to preserve symbolic links:
	   (cd "$vnmrsystem"/user_templates && tar cf - "$file" | (cd "$cur_homedir"/vnmrsys && tar xfBp -))
    	   if test $as_root = "y"
    	   then
               chown -R "$name_add" "$cur_homedir/vnmrsys/$file"
               chgrp -R "$nmr_group" "$cur_homedir/vnmrsys/$file"
    	   else
               chgrp -R "$nmr_group" "$cur_homedir/vnmrsys/$file"
    	   fi
	   if [ "$file" = "ib_initdir" ]
	   then
	        rm -f "$cur_homedir"/vnmrsys/aip_initdir
	        ln -s ib_initdir "$cur_homedir"/vnmrsys/aip_initdir
    	        if test $as_root = "y"
    	        then
        	    chown -h "$name_add" "$cur_homedir"/vnmrsys/aip_initdir
        	    chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/aip_initdir
    	        else
        	    chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/aip_initdir
    	        fi
	   fi
           echo "  $file updated from templates."
	fi
    elif [ "$file" = "pulsecal" ]
    then
	if test -d "$vnmrsystem"/imaging
	then
	   if test ! -f "$cur_homedir/vnmrsys/$file"
	   then
    	        if test $intera_unix = "y"
    	        then
        	    nnl_echo  "OK to create $file (y or n) [y]: "
        	    read yesno
                    if test "x$yesno" = "xn" -o "x$yesno" = "xno"
        	    then
        	        continue
        	    fi
	        fi
		pcaldate=`date +%m%d%y`
		echo "     PULSE CALIBRATION VALUES\n" > "$cur_homedir/vnmrsys/$file"
		echo "     rfcoil      length        flip       power      date\n" >> "$cur_homedir/vnmrsys/$file"
		echo "     main             1         180          30     $pcaldate" >> "$cur_homedir/vnmrsys/$file"
		echo "  pulsecal file created."
    	        if test $as_root = "y"
    	        then
        	     chown -h "$name_add" "$cur_homedir"/vnmrsys/pulsecal
        	     chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/pulsecal
    	        else
        	     chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/pulsecal
    	        fi
	   fi
	fi
    fi
done
if test -d "$vnmrsystem"/user_templates/ib_initdir
then
    if test ! -d "$cur_homedir"/vnmrsys/prescan
    then
       mkdir "$cur_homedir"/vnmrsys/prescan
       if test $as_root = "y"
       then
          chown -h "$name_add" "$cur_homedir"/vnmrsys/prescan
          chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/prescan
       else
          chgrp -h "$nmr_group" "$cur_homedir"/vnmrsys/prescan
       fi
    fi
fi
# End Imaging Files

# some files are to be backed up (moved and deactivated) and not replaced

bkupnoreplace="Acqi $cur_homedir/vnmrsys/templates/vnmrj/choicefiles/planParams"
for file in $bkupnoreplace
do
    if [ -d "$file" ] || [ -f "$file" ]
    then
	mv -f "$file" "$file.bkup.$date"
	echo "  $file backed up as $file.bkup.$date";
    fi
done

cd vnmrsys

#  remove contents of seqlib
#  use word count program (wc) so script variable will have a value
#  that `test' sees as a single argument

if test -d seqlib
then
    tmpval=`(cd seqlib; ls) | wc -c`
    if test $tmpval != "0"
    then
	echo "  removing old pulse sequences for $name_add"
	(cd seqlib; rm -f *)
    fi
fi

#  remove make file and binary files in psg (*.ln, *.a, *.so.* *.o)
#  One level of evaluation ($filespec => *.a) - Use double quotes
#  Expand implicit wildcard ($filespec => libpsglib.a) - no quote characters
#  redirect error output to /dev/null, to avoid messages if no such files exist

if test -d psg
then
    for filespec in "*.a" "*.so.*" "*.ln" "*.o" "makeuserpsg"
    do
	tmpval=`(cd psg; ls $filespec 2>/dev/null) | wc -c`
	if test $tmpval != "0"
	then
	    echo "  removing '$filespec' from psg subdirectory"
	    (cd psg; rm -f $filespec)
	fi
    done
fi

#  make some subdirectories of the user's VNMR directory

dirlist="help maclib manual menujlib parlib persistence probes psglib seqlib shapelib shims \
	tablib imaging templates templates/layout mollib"
for subdir in $dirlist
do
    if test ! -d $subdir
    then
        if test $intera_vnmr = "y"
        then
            nnl_echo  "Create $subdir in your VNMR user directory (y or n) [y]: "
            read yesno
            if test "x$yesno" = "xn" -o "x$yesno" = "xno"
            then
                continue
            fi
        fi
        mkdir $subdir
        chmod 775 $subdir
        if test $as_root = "y"
        then
            chown "$name_add" $subdir
            chgrp "$nmr_group" $subdir
        else
            chgrp "$nmr_group" $subdir
        fi
        if test $subdir = "imaging"
        then
            mkdir $subdir/eddylib
            chmod 775 $subdir/eddylib
            if test $as_root = "y"
            then
                chown "$name_add" $subdir/eddylib
                chgrp "$nmr_group" $subdir/eddylib
            else
                chgrp "$nmr_group" $subdir/eddylib
            fi
        fi
        echo "  VNMR $subdir directory created."
    fi
done

# Remove old gxyz shimming parameters
if test -f parlib/gmapxyz.par/procpar
then
   cat parlib/gmapxyz.par/procpar | grep gxyzshimPDF >/dev/null
   if [ $? -ne 0 ]
   then
      rm -rf parlib/gmapxyz.par
   fi
fi
# Starting with 1.1D we no longer backup gshimlib
#if test -d gshimlib
#then
#  if test $intera_vnmr = "y"
#  then
#    nnl_echo  "Backup gshimlib in your VNMR user directory (y or n) [y]: "
#    read yesno
#    if test x$yesno = "xn" -o x$yesno = "xno"
#    then
#        continue
#    else
#      mv gshimlib gshimlib.bkup.$date
#      echo "  gshimlib backed up in gshimlib.bkup.$date"
#    fi
#  else
#    mv gshimlib gshimlib.bkup.$date
#    echo "  gshimlib backed up in gshimlib.bkup.$date"
#  fi
#fi

export vnmrsystem
if test -d persistence
then
    rm -f persistence/LocatorHistory_*
    rm -f persistence/TagList
    rm -f persistence/session
    rm -f persistence/Graphics
    rm -f persistence/Interface
    rm -f persistence/Plot
    echo "  persistence directory cleaned."
fi

if test ! -d exp1
then
    mkdir exp1
    chmod 775 exp1
    /bin/cp "$vnmrsystem"/fidlib/fid1d.fid/text    exp1/.
    /bin/cat "$vnmrsystem"/fidlib/fid1d.fid/procpar | sed 's"/vnmr/fidlib/Ethylindanone/Ethylindanone_PROTON_01"exp"' > exp1/procpar
    /bin/cp exp1/procpar exp1/curpar
    chmod 664 exp1/*
    mkdir exp1/acqfil
    chmod 775 exp1/acqfil
#    ln -s "$vnmrsystem"/fidlib/fid1d.fid/fid     exp1/acqfil/fid
    mkdir exp1/datdir
    chmod 775 exp1/datdir
    if test $as_root = "y"
    then
        chown -R "$name_add" exp1
        chgrp -R "$nmr_group" exp1
    else
        chgrp -R "$nmr_group" exp1 2> /dev/null
    fi
    echo "  VNMR experiment 1 created."
fi

if test ! -d data
then
    mkdir data
    chmod 775 data
    if test $as_root = "y"
    then
        chown "$name_add" data
        chgrp "$nmr_group" data
    else
        chgrp "$nmr_group" data
    fi
fi

if [ x$ostype != "xLinux" ]
then
    vnmruser=`pwd`
    export vnmruser
fi

if test ! -s global
then
    /bin/cp "$vnmrsystem"/user_templates/global .
    chmod 644 global
    if test $as_root = "y"
    then
	chown "$name_add" global
        chgrp "$nmr_group" global
    else
        chgrp "$nmr_group" global
        run_updaterev
    fi
    echo "  global updated from templates;"

elif test $as_root = "y"
then
    mv -f global global.bkup.$date
    echo "  global backed up in global.bkup.$date"
    /bin/cp "$vnmrsystem"/user_templates/global .
    chmod 644 global
    chown "$name_add" global
    chgrp "$nmr_group" global
else
    /bin/cp global global.bkup.$date
    echo "  global backed up in global.bkup.$date"
    rm -rf global[0-9]
    run_updaterev
    if test ! -s global
    then
	 /bin/cp "$vnmrsystem"/user_templates/global .
	 chmod 644 global
	 chgrp "$nmr_group" global
         echo "  global updated from templates."
    else
         echo "  global updated with current values."
    fi
fi

#  The pbox defaults file goes in the shapelib subdirectory of the
#  user's VNMR directory.  If none present in that subdirectory, a
#  copy is present in the VNMR system directory and a shapelib exists
#  in the user's VNMR directory, then copy in the current version.

if test ! -f shapelib/.Pbox_defaults
then
  if test -f "$vnmrsystem"/shapelib/.Pbox_defaults
  then
    if test -d shapelib
    then
      /bin/cp "$vnmrsystem"/shapelib/.Pbox_defaults shapelib
      chmod 644 shapelib/.Pbox_defaults
      if test $as_root = "y"
      then
        chown "$name_add" shapelib/.Pbox_defaults
        chgrp "$nmr_group" shapelib/.Pbox_defaults
      else
        chgrp "$nmr_group" shapelib/.Pbox_defaults
      fi
      echo "  .Pbox_defaults updated from shapelib;"
    fi
  fi
fi

if [ x$ostype = "xLinux" ]
then
   cd "$cur_homedir"
   if [ ! -d Desktop ]
   then
      mkdir Desktop
      if test $as_root = "y"
      then
         chown "$name_add" Desktop
         chgrp "$nmr_group" Desktop
      fi
   fi
   setDeskTop vnmrj.desktop
   if [ "x$name_add" = x$sys_user ]; then
      setDeskTop vnmrjadmin.desktop
   fi
   if [ -f "$vnmrsystem"/templates/vnmrj/properties/labelResources_ja.properties ]; then
      setDeskTop vnmrjja.desktop
   fi
   if [ -f "$vnmrsystem"/templates/vnmrj/properties/labelResources_zh_CN.properties ]; then
      setDeskTop vnmrjzh.desktop
   fi
   setWallPaper
fi

if test -d templates/vnmrj/properties
then
    rm -rf templates/vnmrj/properties 
    echo "  templates/vnmrj/properties directory removed."
fi

echo ""
echo "Updating for '$name_add' complete"
echo ""
