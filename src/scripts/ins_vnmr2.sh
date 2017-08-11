#! /bin/sh
# 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 

#-----------------------------------------------

# for logging use logmsg
# or append to the log file, i.e.    >> $logfile 2>&1

logdir="/tmp/vnmrjinstall"
logfile="/tmp/vnmrjinstall/VnmrjInstallationScript.log"

initlog() {
     mkdir -p $logdir
     echo "========================================================" >> $logfile
     echo " " >> $logfile
     echo " ins_vnmr script log " >> $logfile
     date >> $logfile
     echo " " >> $logfile
     echo "========================================================" >> $logfile
     echo " " >> $logfile
     echo " " >> $logfile
}

logdone() {
     echo " " >> $logfile
     echo "========================================================" >> $logfile
     echo " " >> $logfile
     cmplttime=`date`
     echo "Installation script complete $cmplttime " >> $logfile
     echo " " >> $logfile
     echo "========================================================" >> $logfile
     echo " " >> $logfile
     echo " " >> $logfile
     # copy the log file into the vnmr log directory
     cp $logfile /vnmr/adm/log
     rm -rf $logdir

}

logmsg() {
      echo $1 >> $logfile
      # echo $1 >> $logfile 2>&1
}


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

#-----------------------------------------------
update_user_group() {

   test_file="/tmp/testfile_willberemoved"
   touch $test_file
   chgrp $nmr_group $test_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       if [ x$os_version = "xwin" ]
       then
          $src_code_dir/win/bin/groupadd $nmr_group
       else
          logmsg "groupadd -g  $nmr_group_no $nmr_group"
          if [ x$lflvr != "xdebian" ]
          then
             /usr/sbin/groupadd  -g $nmr_group_no $nmr_group
          else
             sudo /usr/sbin/groupadd -g $nmr_group_no $nmr_group
          fi
       fi
   elif [ x$os_version != "xwin" ]
   then
       nmr_group_no=`grep "^${nmr_group}:" /etc/group | awk 'BEGIN { FS = ":" } { print $3}'`
   fi

   #Only copy the passwd file if there is no nmr_adm account

   chown $nmr_adm $test_file 2>/dev/null
   if [ $? -ne 0 ]
   then
       if [ x$os_version = "xrht" ]
       then
          if [ x$lflvr != "xdebian" ]
          then
             logmsg "/usr/sbin/useradd -d$nmr_home/$nmr_adm -s/bin/csh -g$nmr_group $nmr_adm"
             /usr/sbin/useradd -d$nmr_home/$nmr_adm -s/bin/csh -g$nmr_group $nmr_adm
             logmsg "chmod 755 $nmr_home/$nmr_adm"
             chmod 755 "$nmr_home/$nmr_adm"
             logmsg "/usr/bin/passwd -f -u $nmr_adm"
             /usr/bin/passwd -f -u $nmr_adm >> $logfile 2>&1
             # for a new user copy these environment parameter files into place
             # so that the search paths and other VnmrJ env parameters are setup on login
             logmsg "cp $src_code_dir/.login $nmr_home/$nmr_adm"
             cp $src_code_dir/.login $nmr_home/$nmr_adm
             logmsg "cp $src_code_dir/.cshrc $nmr_home/$nmr_adm"
             cp $src_code_dir/.cshrc $nmr_home/$nmr_adm
             logmsg "cp $src_code_dir/.vnmrenv $nmr_home/$nmr_adm"
             cp $src_code_dir/.vnmrenv $nmr_home/$nmr_adm
             logmsg "cp $src_code_dir/.vxresource $nmr_home/$nmr_adm"
             cp $src_code_dir/.vxresource $nmr_home/$nmr_adm
          else
             # --home-create == -m, --home == -d, --shell == -s, --gid == -g
             # must give the account a temp password 'abcd1234' to get the account active 
             # since passwd does not have the -f to force activation as does RHEL
             # passwd abcd1234 = $1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1
             # Prior to 12.04 we used useradd to set all the groups, however with 12.04 the admin group did not exist and the useradd completely failed.
             # Thus we no longer do that part in the useradd command, but perform that operation below.
             logmsg "sudo /usr/sbin/useradd --create-home --home-dir $nmr_home/$nmr_adm --shell /bin/bash --gid $nmr_group --password '$1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1' $nmr_adm"
             sudo /usr/sbin/useradd --create-home --home-dir $nmr_home/$nmr_adm --shell /bin/bash --gid $nmr_group --password '$1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1' $nmr_adm

             # add vnmr1 to appropriate groups as well the most important, admin or sudo (12.04) so it is permitted to sudo.
             # this method adds the group one by one, if one doesn't exist it fails but no harm done.
             possblegrps="admin adm sudo dialout cdrom floppy audio video plugdev users fuse lpadmin"
             logmsg "Add Vnmr1 to these groups"
             for grp in $possblegrps
             do
                 logmsg "/usr/sbin/adduser $nmr_adm $grp"
                 /usr/sbin/adduser $nmr_adm $grp >> $logfile 2>&1
             done
             logmsg "sudo chmod 755 $nmr_home/$nmr_adm"
             sudo chmod 755 "$nmr_home/$nmr_adm"
             # we give a temp default password,  use the --expire option to force to user to change password on login
             logmsg "sudo /usr/bin/passwd --expire $nmr_adm 2>/dev/null"
             sudo /usr/bin/passwd --expire $nmr_adm >> $logfile 2>&1
             # setup the bash env for VnmrJ
             logmsg "sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm"
             sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm
             logmsg "sudo $src_code_dir/setupbashenv $nmr_home/$nmr_adm"
             sudo $src_code_dir/setupbashenv "$nmr_home/$nmr_adm"
          fi
       elif [ x$os_version = "xwin" ]
       then
          if [ ! -d "$nmr_home" ]
          then
            mkdir $nmr_home
          fi
          chmod 775 $nmr_home
          "$src_code_dir"/win/bin/useradd -d"$nmr_home"/$nmr_adm -s/bin/csh -g$nmr_group $nmr_adm
       else
           add_to_passwd
       fi

       #special stuff for Solaris

       if [ x$ostype = "xSOLARIS" ]
       then
           if touch /etc/shadow
           then
               if grep -s $nmr_adm /etc/shadow >/dev/null
               then
                   :
               else
                   echo "$nmr_adm:::0:::::" >>/etc/shadow
               fi
           else
               echo "Cannot add $name to the shadow file"
           fi
       fi
   elif [ x$os_version = "xwin" ]
   then
      nmrhomedir=`"$src_code_dir"/win/bin/getuserinfo "$nmr_adm" | awk 'BEGIN {FS=";"} {print $2}'`
      echo "$nmrhomedir"
      if [ x"$nmrhomedir" = "x" -o x"$nmrhomedir" = "x " ]
      then
         if [ ! -d "$nmr_home" ]
         then
             mkdir "$nmr_home"
         fi
         mkdir "$nmr_home/$nmr_adm"
         homedir=`unixpath2win "$nmr_home/$nmr_adm"` 
         net user "$nmr_adm" /homedir:"$homedir"
      fi
   else  # user is already present, let's be sure the user is configured correctly
       if [ x$os_version = "xrht" ]
       then
          if [ x$lflvr != "xdebian" ]
          then
             logmsg "User already present, be sure user is configured."
             logmsg "chmod 755 $nmr_home/$nmr_adm"
             chmod 755 "$nmr_home/$nmr_adm"
             if [ ! -f $nmr_home/$nmr_adm/.login ] ; then
               logmsg "cp $src_code_dir/.login $nmr_home/$nmr_adm"
               cp $src_code_dir/.login $nmr_home/$nmr_adm
             fi
             if [ ! -f $nmr_home/$nmr_adm/.cshrc ] ; then
               logmsg "cp $src_code_dir/.cshrc $nmr_home/$nmr_adm"
               cp $src_code_dir/.cshrc $nmr_home/$nmr_adm
             fi
             if [ ! -f $nmr_home/$nmr_adm/.vnmrenv ] ; then
               logmsg "cp $src_code_dir/.vnmrenv $nmr_home/$nmr_adm"
               cp $src_code_dir/.vnmrenv $nmr_home/$nmr_adm
             fi
             if [ ! -f $nmr_home/$nmr_adm/.vxresource ] ; then
               logmsg "cp $src_code_dir/.vxresource $nmr_home/$nmr_adm"
               cp $src_code_dir/.vxresource $nmr_home/$nmr_adm
             fi
          else
             # sudo /usr/sbin/useradd --create-home --home-dir $nmr_home/$nmr_adm --shell /bin/bash --gid $nmr_group --groups admin,cdrom,floppy,audio,video,plugdev,fuse,lpadmin,adm --password '$1$LEdmx.Cm$zKS4GXyvUzjNLucQBNgwR1' $nmr_adm
             # make sure the user belongs the the nmr group
             grpval=`grep ${nmr_group}: /etc/group | grep $nmr_adm`
             if [ -z "$grpval" ] ; then
                 logmsg "/usr/sbin/adduser $nmr_adm $nmr_group "
                 /usr/sbin/adduser $nmr_adm $nmr_group >> $logfile 2>&1
             fi
             logmsg "sudo chmod 755 $nmr_home/$nmr_adm"
             sudo chmod 755 "$nmr_home/$nmr_adm"
             # setup the bash env for VnmrJ
             if [ ! -f $nmr_home/$nmr_adm/.vnmrenvbash ] ; then
               logmsg "sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm"
               sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm
               logmsg "sudo $src_code_dir/setupbashenv $nmr_home/$nmr_adm"
               sudo $src_code_dir/setupbashenv "$nmr_home/$nmr_adm"
             fi
          fi
      fi
   fi
   rm -f $test_file

   # update the env where we created the account or it already existed.
   if [ x$lflvr = "xdebian" ]
   then
      # setup the bash env for VnmrJ
      logmsg "sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm"
      sudo cp $src_code_dir/.vnmrenvbash $nmr_home/$nmr_adm
      logmsg "sudo $src_code_dir/setupbashenv $nmr_home/$nmr_adm"
      sudo $src_code_dir/setupbashenv "$nmr_home/$nmr_adm"
   fi

   #if windows, then add the vnmrj adminstrator to Administrators group
   if [ x$os_version = "xwin" ]
   then
      net localgroup Administrators | grep $nmr_adm  2>/dev/null
      if [ $? -ne 0 ]
      then
       "$src_code_dir"/win/bin/groupadd Administrators $nmr_adm
      fi
   fi
}

#-----------------------------------------------
add_to_passwd() {

# make backup copy of password file
# scan password file for largest user-id
# add one to that user-id to obtain id for vnmr1
# insert before last line in password file
# keep user-id number within bounds of positive 16-bit numbers,
#  that is, less than 32768

   echo "add_to_passwd() ---"
   logmsg "add_to_passwd() ---"

   awk '
      BEGIN { N=0
              AlreadyExists=0
              NewUser="'$nmr_adm'"
              FS=":"
      }

      {
        if ($3>N && $3<32768) N=$3
        if ($1==NewUser) AlreadyExists=1
      }

      END { if (AlreadyExists==0)
            printf "%s::%d:'$nmr_group_no':%s:'$nmr_home'/%s:/bin/csh\n",NewUser,N+1,NewUser,NewUser
      }
   ' < /etc/passwd >/tmp/newuser

   #Insert new entry before the last line in the password file

   if [ -s /tmp/newuser ]
   then
      echo "add_to_passwd():add new entry ----"
      logmsg "add_to_passwd():add new entry ----"
      if [ x$lflvr != "xdebian" ]
      then
         cp /etc/passwd /etc/passwd.bk
         read stuff </tmp/newuser
         (sed '$i\
'"$stuff"'' /etc/passwd >/tmp/newpasswd)
         mv /tmp/newpasswd /etc/passwd
         chmod 644 /etc/passwd
         rm /tmp/newuser
      else
         sudo cp /etc/passwd /etc/passwd.bk
         read stuff </tmp/newuser
         (sed '$i\
'"$stuff"'' /etc/passwd >/tmp/newpasswd)
         sudo mv /tmp/newpasswd /etc/passwd
         sudo chmod 644 /etc/passwd
         rm /tmp/newuser
      fi
   fi
}

#-----------------------------------------------
change_procpar() {
  for proc in $nproc
  do
    cp $proc /tmp/proc_tmp
    cat /tmp/proc_tmp | sed 's/4 "a" "n" "s" "y"/2 "n" "y"/' | \
    sed 's/9 "c" "f" "g" "m" "p" "r" "u" "w" "x"/5 "c" "f" "n" "p" "w"/' > $proc
  done
  rm /tmp/proc_tmp 
}
 
#-----------------------------------------------
update_if_dosy()
{
  if [ "x$1" = "xDOSY" ]
  then
     if [ -f "$dest_dir"/pgsql/data/postmaster.pid ]
     then
        echo "  Updating protocols in database."
   if [ x$os_version != "xwin" ]
   then
      if [ x$lflvr != "xdebian" ]
      then
         su $nmr_adm -c "$dest_dir/bin/managedb updatetable protocol"
      else
         sudo -u $nmr_adm $dest_dir/bin/managedb updatetable protocol
      fi
   else
       $dest_dir/bin/managedb updatetable protocol
   fi
     fi
  fi
}

#-----------------------------------------------
load_pw_option()
{
   i=0
   n_pass=`expr $# / 2`
   load_type=${cons_type}.opt

   #console=`basename $console $opt_dir`opt
   tmp_size=0
   while [ $i -lt $n_pass ]
   do
       cat "$src_code_dir"/$os_version/$load_type | (while read line
      do
         b=`echo $line | awk 'BEGIN { FS = " " } { print $1 }'`
         if [ x$b = x$1 ]
         then
            size=`echo $line | awk 'BEGIN { FS = " " } { print $2 }'`
            c=`echo $line | awk 'BEGIN { FS = " " } { print $3 }'`
            #echo  $c $tmp_size
            #tmp_size=$size
            if [ "x$nmr_adm" != "x" ] # actually nmr_adm always set
            then
               echo "  Extracting:  \"$1\"  $c"
               (cd "$dest_dir"; \
                cp "$source_dir"/"$c" .; \
                "$src_code_dir"/decode.$os_version $2 `basename "$c"` tmp.tar;\
                rm `basename "$c"` )
               i=`ls -l "$dest_dir"/tmp.tar | awk 'BEGIN { FS=" " } {print $5}'`
               # if the wrong password was used the file size will be zero
               if [ $i -gt 2 ]
               then
                  # decode was OK so now untar the file
                  if [ x$os_version != "xwin" ]
                  then
                     if [ x$lflvr != "xdebian" ]
                     then
                        (cd "$dest_dir";su $nmr_adm -fc "tar $taroption tmp.tar" )
                     else
                        (cd "$dest_dir";sudo -u $nmr_adm tar $taroption tmp.tar )
                     fi
                  else
                     (cd "$dest_dir";tar $taroption tmp.tar )
                  fi
                  update_if_dosy $1
                  printf "`basename $c`\n" >> "$dest_dir"/adm/log/options
                  ${chown_cmd} $nmr_adm "$dest_dir"/adm/log/options
                  ${chgrp_cmd} $nmr_group "$dest_dir"/adm/log/options
                  echo "  DONE:  $size KB."
               else
                  echo "    PASSWORD for $1 incorrect. "
                  echo "      If you have the correct password"
                  echo "      you can load the option separately"
                  echo "      when this install is complete."
                  echo "      Run load.nmr again, and only select $1"
                  echo "  SKIPPED: $size"
                  grep $1  "$dest_dir"/pw_fault
                  if [ $? -ne 0 ]
                  then
                      touch  "$dest_dir"/pw_fault
                      echo "  $1" >> "$dest_dir"/pw_fault
                  fi
               fi
               rm -f "$dest_dir"/tmp.tar
            else
               (cd "$dest_dir"; \
                cp "$source_dir"/"$c" .; \
                "$src_code_dir"/decode.$os_version $2  `basename "$c"` tmp.tar; \
                rm `basename "$c"` )
               i=`ls -l "$dest_dir"/tmp.tar | awk 'BEGIN { FS=" " } {print $5}'`
               if [ $i -gt 2 ]
               then
                  (cd "$dest_dir";tar $taroption tmp.tar )
                  printf "`basename $c`\n" >> "$dest_dir"/adm/log/options
                  ${chown_cmd} $nmr_adm "$dest_dir"/adm/log/options
                  ${chgrp_cmd} $nmr_group "$dest_dir"/adm/log/options
                  echo "  DONE:  $size KB."
               else
                  echo "    Password for $1 incorrect "
                  echo "      If you have the correct password"
                  echo "      you can load the option separately"
                  echo "      when this install is complete."
                  echo "      Run load.nmr again, and only select $1"
                  echo "  SKIPPED: $size"
                  grep $1  "$dest_dir"/pw_fault
                  if [ $? -ne 0 ]
                  then
                      touch  "$dest_dir"/pw_fault
                      echo "  $1" >> "$dest_dir"/pw_fault
                  fi
               fi
               rm -f "$dest_dir"/tmp.tar
            fi
         fi   
      done
      #echo " " $tmp_size
      )

      i=`expr $i + 1`
      shift; shift
   done
   echo " "

}


#
#  save the complete asm directory, for recovery of added and updated VAST files
#  tar out the present asm directory excluding the backup directories.
#  No need to keep propagating backup directories.
#
savevast()
{
#  echo  " "
#  echo  "  Backing up previous release VAST files."
#  echo " "
   logmsg "savevast"
  /bin/rm -f /tmp/vastexcludelist /tmp/vast.tar
#
# create the exclude list
#
  /bin/echo "./info.new" > /tmp/vastexcludelist
  /bin/echo "./asm.this_release.bkup" >> /tmp/vastexcludelist
  /bin/echo "./asm.previous_release.bkup" >> /tmp/vastexcludelist
#
# tar present asm directory
#
  if [ x$os_version = "xrht" ]
  then
      (cd /vnmr/asm; /bin/tar -X /tmp/vastexcludelist -cf /tmp/vast.tar . )
  else
      (cd /vnmr/asm; /bin/tar -cXf /tmp/vastexcludelist /tmp/vast.tar . )
  fi
}


#
# restore the previous release VAST files
#
restorevast()
{
#  remove the temp files about to be created and used
   logmsg "restorevast"
  /bin/rm -f /tmp/nvast.tar /tmp/vastlist.prev /tmp/vastlist.latest /tmp/vastcombolist /tmp/vastuniqlist /tmp/vastextractlist

  # just to be sure it's there
  if [  ! -d "$dest_dir"/asm/info ]
  then
      /bin/mkdir -p "$dest_dir"/asm/info
  fi

# just being sure that the backup directories are brand new
  /bin/rm -rf "$dest_dir"/asm/asm.this_release.bkup
  /bin/rm -rf "$dest_dir"/asm/asm.previous_release.bkup
  /bin/rm -rf "$dest_dir"/asm/info.new
  /bin/mkdir -p "$dest_dir"/asm/asm.this_release.bkup
  /bin/mkdir -p "$dest_dir"/asm/asm.previous_release.bkup
#
# tar new release asm directory
#
#ccccc
  if [ x$os_version = "xrht" ]
  then
      (cd "$dest_dir"/asm; tar -X /tmp/vastexcludelist -cf /tmp/nvast.tar . )
  else
      (cd "$dest_dir"/asm; tar -cXf /tmp/vastexcludelist /tmp/nvast.tar . )
  fi

#
# create an untouched backup copy of this release asm directory
# just incase the restore does damage
#
   echo "   The Original VAST files for this release are backed up in: "
   echo "        $dest_dir/asm/asm.this_release.bkup"
   (cd "$dest_dir"/asm/asm.this_release.bkup; /bin/tar -xf /tmp/nvast.tar )
 
# generate two list of whats in the present and new asm directories
   /bin/tar -tf /tmp/vast.tar > /tmp/vastlist.prev
   /bin/tar -tf /tmp/nvast.tar > /tmp/vastlist.latest
   /bin/cat /tmp/vastlist.prev /tmp/vastlist.latest | /bin/sort > /tmp/vastcombolist

# create a unique list of files present in one but not the other
# these we will copy over to the new release asm
#  --- uniqlist maybe unique to the new or the old
   # uniqlist=`/usr/bin/uniq -u /tmp/combolist`
   /usr/bin/uniq -u /tmp/vastcombolist > /tmp/vastuniqlist

# ---  extractlist are the unique ones in the previous release
   extractlist=
   #Determine those unique items from previous release 
   if [ x$os_version != "xrht" ]
   then
       extractlist=`/bin/tar -tf /tmp/vast.tar $uniqlist`
   else 
       cat /tmp/vastlist.prev /tmp/vastuniqlist | /bin/sort | uniq -d > /tmp/vastextractlist
       extractlist=`cat /tmp/vastextractlist`
   fi

# restore (i.e. copy over the new release) racksetup and info files
   echo " "
   echo "   Restoring previous release VAST racksetup and sample files"
   (cd "$dest_dir"/asm; /bin/tar xpf /tmp/vast.tar ./racksetup ./info );

   # cat $extractlist > /tmp/extractlist
# if there some addition files to be copied do so now
   if [ x"$extractlist" != "x" ]
   then
      echo " "
      echo "    Restoring user/system added VAST files from previous release"
      # out to null since even though the items maybe present for some reason tar output error about them not
      # being in the archive.. GMB  11/17/09,, could use tar's option '-T /tmp/vastextractlist'
      ( cd "$dest_dir"/asm; /bin/tar xpf /tmp/vast.tar $extractlist  >/dev/null 2>/dev/null )
      flist=`echo $extractlist | /usr/bin/tr -s '\012' " " `
      # echo "      $flist"
      echo " "
      echo "     For user-added protocols and/or racks, you will need"
      echo "     manually to update the protocol.vast and racks/rackInfo files. "
      echo " "
   fi
#
# create a complete backup of the previous release files, various *.conf
# and other files may be needed if they were customized
#
   ( cd "$dest_dir"/asm/asm.previous_release.bkup; /bin/tar -xf /tmp/vast.tar )
   echo " "
   echo "    If you change any of the '.conf' or other files you will need manually"
   echo "    to reapply your changes."
   echo "    A complete backup of the previous VAST files can be found in: "
   echo "         $dest_dir/asm/asm.previous_release.bkup"
 
   /bin/rm -f /tmp/vastexcludelist /tmp/vast.tar
   /bin/rm -f /tmp/nvast.tar /tmp/vastlist.prev /tmp/vastlist.latest /tmp/vastcombolist
   /bin/rm -f /tmp/vastuniqlist /tmp/vastextractlist
}

probeid_finish()
{
   echo "Configuring Probe ID"
   PROBEID_DIR="${dest_dir}/probeid"
   PROBEID_CACHE="${PROBEID_DIR}/cache"
   PROBEID_MOUNT_LINK="${PROBEID_DIR}/mnt"
   PROBE_MNT="/mnt/probe"

   logmsg "Configuring Probe ID"
   logmsg "PROBEID_DIR: $PROBEID_DIR"
   logmsg "PROBEID_CACHE: $PROBEID_CACHE"
   logmsg "PROBEID_MOUNT_LINK: $PROBEID_MOUNT_LINK"
   logmsg "PROBE_MNT: $PROBE_MNT"

   # move existing probeid mount point and cache
   if [ -e /tmp/probeid.tbz2 ]
   then
         echo "Moving probe connection"
         logmsg "Restoring probe connection"
         logmsg "(cd $dest_dir ; tar xjf /tmp/probeid.tbz2)"
         (cd $dest_dir ; tar xjf /tmp/probeid.tbz2)
         logmsg "rm /tmp/probeid.tbz2"
         rm /tmp/probeid.tbz2
   fi

   # set up a mount point in case there wasn't already one 
   if [ ! -e "$PROBE_MNT" ]
   then
         echo "Creating probe mount point"
         logmsg "Creating probe mount point"
         logmsg "mkdir -p $PROBE_MNT"
         mkdir -p "$PROBE_MNT"
         logmsg "chmod 775 $PROBE_MNT"
         chmod 775 "$PROBE_MNT"
         logmsg "${chown_cmd} $nmr_adm $PROBE_MNT"
         ${chown_cmd} $nmr_adm "$PROBE_MNT"
         logmsg "${chgrp_cmd} $nmr_group $PROBE_MNT"
         ${chgrp_cmd} $nmr_group "$PROBE_MNT"
   fi

   # initialize the cache directory in case there wasn't already one
   if [ ! -e "$PROBEID_CACHE" ]
   then
         echo "Creating probe cache"
         logmsg "Creating probe cache"
         logmsg "mkdir -p $PROBEID_CACHE"
         mkdir -p "$PROBEID_CACHE"
         logmsg "chmod 775 $PROBEID_CACHE"
         chmod 775 "$PROBEID_CACHE"
         logmsg "${chown_cmd} -R $nmr_adm $PROBEID_DIR"
         ${chown_cmd} -R $nmr_adm "$PROBEID_DIR"
         logmsg "${chgrp_cmd} -R $nmr_group $PROBEID_DIR"
         ${chgrp_cmd} -R $nmr_group "$PROBEID_DIR"
   fi

   # initialize the probe mount link in case there wasn't already one
   if [ ! -e "$PROBEID_MOUNT_LINK" ]
   then
         echo "Linking to probe mount point"
         logmsg "Linking to probe mount point"
         logmsg "ln -s $PROBE_MNT $PROBEID_MOUNT_LINK"
         ln -s "$PROBE_MNT" "$PROBEID_MOUNT_LINK"
   fi
}

probeid_prep()
{
   local exclude=""
   if [ ! -h ./probeid/mnt ]; then exclude="--exclude ./probeid/mnt"; fi
   if [ -e ./probeid/cache ]
   then
     logmsg "(cd /vnmr; tar -cjf /tmp/probeid.tbz2 $exclude ./probeid/mnt ./probeid/cache)"
     (cd /vnmr; tar -cjf /tmp/probeid.tbz2 $exclude ./probeid/mnt ./probeid/cache)
   fi
}

#-----------------------------------------------
#  Main MAIN main program starts here
#-----------------------------------------------
initlog

os_version=$1    #sol ibm sgi rht
shift            #because sh does not use $10 and up
cons_type=$1     #inova ... g2000
shift
src_code_dir=$1  #/cdom/cdrom0/code
shift
dest_dir=$1      #/export/home/vnmr
shift
nmr_adm=$1       #vnmr1
shift
nmr_group=$1     #nmr
shift
nmr_home=$1      #/space or /export/home or /
shift
vnmr_link=$1     #yes
shift
man_link=$1      #no
shift
gen_list=`echo $1 | sed 's/+/ /g'`  #agr8 or 9 is a list, items separated by a "*"
shift
opt_list=`echo $1 | sed 's/+/ /g'`

logmsg "OS Version: $os_version"
logmsg "console type: $cons_type"
logmsg "Dest Directory: $dest_dir"
logmsg "Admin: $nmr_adm"
logmsg "Group: $nmr_group"
logmsg "Admin Home DIr: $nmr_home"
logmsg "Install List: $gen_list"
logmsg "Install Options List: $opt_list"
logmsg " "
logmsg "================================================="
logmsg " "
logmsg " "

NAWK="nawk"

rootuser="root"
taroption="jxpf"
sbindir="/usr/varian/sbin"
lflvr=" "

nmr_group_no=30
case x$os_version in
    xsol) 
        ostype="SOLARIS"
        file_ext="sol"
   ;;

    xwin)

        file_ext=$os_version
        ostype="Interix" 
   NAWK="awk"
   rootuser="Administrator"
   src_code_dir=`/bin/ntpath2posix "${src_code_dir}"`
   dest_dir=`/bin/ntpath2posix "$dest_dir"`
   nmr_home=`/bin/ntpath2posix "$nmr_home"`
   taroption="xf"
   sbindir="$dest_dir"/.sbin
   ;;

    xrht)

        #RedHat does not have nawk, but other flavors may
        NAWK="gawk"

        if [  -r /etc/SuSE-release ]
        then
            lflvr="suse"
        elif [  -r /etc/debian_version ]
        then
            distro=`lsb_release -is`    # Ubuntu
            distrover=`lsb_release -rs` # 8.04, 9.04, etc.
            distmajor=`echo $distrover | cut -f1 -d.`
            lflvr="debian"
                 # Ubuntu has awk
                 NAWK="awk"
        else
            lflvr="rhat"
            distrover=`cat /etc/redhat-release | sed -r 's/[^0-9]+//' | sed -r 's/[^0-9.]+$//'`    # yield 5.1, 5.3 , 6.1, etc..
            # VersionNoDot=`cat /etc/redhat-release | sed -e 's#[^0-9]##g' -e 's#7[0-2]#73#'`    # yields 51, 53, 61, etc.
            # MajorVersionNum=`cat /etc/redhat-release | sed -e 's#[^0-9]##g' | cut -c1`            # yields  5, 5,  6,  etc.
            distmajor=`echo $distrover | cut -f1 -d.`

        fi

        logmsg "Distro:  $lflvr"
        logmsg "Distro Version:  $distrover"

        file_ext=$os_version

        #group 30 in RedHat system taken by gopher
        # also 30 is group 'dip' (Dialup IP) on RHEL and Ubuntu
        while [ x`grep ":${nmr_group_no}:" /etc/group` != "x" ]
        do
           nmr_group_no=`expr $nmr_group_no + 1`
        done
        ;;
esac

real_console=$cons_type

if [ x$os_version = "xwin" ]
then
   domainname=`/bin/pdomain`
else
   # nis may not be installed
   if [ -x /bin/domainname ]; then
      domainname=`/bin/domainname`
   else
      domainname=""
   fi
fi

echo "NMR Owner = $nmr_adm"
echo "NMR Group = $nmr_group"
echo "NMR Destination directory= $dest_dir"
echo "NMR host='`/bin/hostname`' domain='$domainname'"

source_dir=`dirname "$src_code_dir"`
acq_pid=-1
 
if [ x$ostype = "xSOLARIS" ]
then
    chown_cmd="/bin/chown -h "
    chgrp_cmd="/bin/chgrp -h "
else
    chown_cmd="chown "
    chgrp_cmd="chgrp "
fi

if [ $acq_pid -ne -1 ]
then
   echo "Stopping acquisition."
   kill -2 $acq_pid
   sleep 5     # give time for kill message to show up.
fi

# stop nmrwebd web status daemon
nmrwebd_root=/vnmr/web
nmrwebd_home=${nmrwebd_root}/scripts
nmrwebd_exe=nmrwebd
nmrwebd_path=${nmrwebd_home}/${nmrwebd_exe}
nmrwebd_pidfile=${nmrwebd_root}/run/${nmrwebd_exe}.pid

if [ -e $nmrwebd_pidfile ]; then
   echo "Stopping NMR Web Service $nmrwebd_exe"
   pid=`cat $nmrwebd_pidfile`
   kill -TERM $pid >/dev/null
   rm -f $nmrwebd_pidfile
fi

#and if we want to load vnmr we create entry in passwd/shadow

echo $gen_list | grep -s VNMR > /dev/null
if [ $? -eq 0 ]
then
   #echo "Checking for $nmr_adm   in password file(s)"
   #echo "Checking for $nmr_group in group file"
   if [ x$ostype = "xIRIX" -o x$ostype = "xIRIX64" ]
   then
      $src_code_dir/i_vnmr.4j $nmr_adm $nmr_group /usr/people
   else
      if [ x$ostype = "xSOLARIS" ]
      then
          echo "Updating User group and password files"
          update_user_group
          if [ x$nmr_adm != "xvnmr1" ]
          then
             tmp_admin=$nmr_adm
             nmr_adm="vnmr1"
             update_user_group
             nmr_adm=$tmp_admin
          fi
      else
          nmr_home="/home"
          update_user_group
      fi
   fi
fi

if [ ! -d "$dest_dir" ]
then
    echo "Creating '$dest_dir' directory"
    mkdir -p "$dest_dir"
else
    echo "'$dest_dir' already exists"
    if [ -d "$dest_dir"/jre ]
    then
        chmod -R u+w "$dest_dir"/jre
    fi
fi

rm -f $dest_dir/pw_fault   #just in case

cd "$dest_dir"
chmod 755 .
if [ x$os_version != "xwin" ]
then
    ${chown_cmd} $nmr_adm .
    ${chgrp_cmd} $nmr_group .
fi

if [ ! -d tmp ]
then
   mkdir tmp
   ${chown_cmd} $nmr_adm tmp
   ${chgrp_cmd} $nmr_group tmp
fi

save_p11_users="n"

echo $gen_list | grep -s Secure_Environments > /dev/null
if [ $? -eq 0 ]
then
    configP11="yes"
else
    configP11="no"
fi

cp_files="n"
load_main="n"
echo $gen_list | grep -s VNMR > /dev/null
if [ $? -eq 0 ]
then
   load_main="y"
   if [ -r /vnmr/conpar ]
   then
         echo "Backing up files from previous installation (this may take a few minutes)"
         if [ -r /vnmr/conpar.prev ]
         then
            cp /vnmr/conpar.prev /tmp/conpar
         else
            cp /vnmr/conpar /tmp/conpar
         fi
         cp /vnmr/devicenames /tmp/devicenames
         cp /vnmr/devicetable /tmp/devicetable
         rm -rf /tmp/probes /tmp/shims
         cp -r /vnmr/probes /tmp/probes
         cp -r /vnmr/shims /tmp/shims
         rm /tmp/probes/probe.tmplt
         cp_files="y"

         if [ -r /vnmr/p11/part11Config -a x$configP11="xyes" ]
         then
             save_p11_users="y"
         fi
   fi

   if [ -r /vnmr/gshimdir ]
   then
         rm -rf /tmp/gshimdir
         cp -r /vnmr/gshimdir /tmp/gshimdir
   fi

   if [ -r /vnmr/probeid ]
   then
	 probeid_prep
   fi

fi



# if cp_files='y' then we are loading VNMR,
# so save gradtables and decclib directories

cp_gradtables='n'
cp_decclib='n'
cp_fastmap='n'
cp_mollib='n'
cp_corba='n'
cp_admusers='n'
cp_dicom='n'
cp_p11='n'
cp_gshimlib='n'
#cp_database='n' # don't copy the database since it is rebuilt from scratch later anyway
cp_protune='n'
cp_accounting='n'
cp_cryomon='n'
cp_amptables='n'
cp_vast='n'
mv_probeid='n'
cp_overlay='n'

rm -f "$dest_dir"/pw_fault

if [ x$cp_files = "xy" ]
then
      if [ -d /vnmr/imaging/gradtables ]
      then
         if [ -f imaging/coilIDs ]
         then
            (cd /vnmr; tar cjf /tmp/gradtables.tar imaging/gradtables \
            imaging/coilIDs imaging/grad.tmplt)
         else
            (cd /vnmr; tar cjf /tmp/gradtables.tar imaging/gradtables)
         fi
         cp_gradtables='y'
      fi
      if [ -d /vnmr/imaging/decclib ]
      then
         (cd /vnmr; tar cjf /tmp/decclib.tar imaging/decclib)
         cp_decclib='y'
      fi
      if [ -d /vnmr/fastmap ]
      then
         (cd /vnmr; tar cjf /tmp/fastmap.tar fastmap)
         cp_fastmap='y'
      fi
      if [ -d /vnmr/mollib ]
      then
       (cd /vnmr; tar cjf /tmp/mollib.tar mollib)
       cp_mollib='y'
      fi

      if [ -d /vnmr/cryo/cryomon ]
      then
       (cd /vnmr; tar cjf /tmp/cryomon.tar cryo/cryomon cryo/probecal)
       cp_cryomon='y'
      fi

      if [ -d /vnmr/amptables ]
      then
       (cd /vnmr; tar cjf /tmp/amptables.tar amptables)
       cp_amptables='y'
      fi

      # even if the vast is not selected to be installed we still want 
      # to transfer the previous vast installation forward.
      if [ -d /vnmr/asm/info ]
      then
         savevast
         cp_vast='y'
      fi

      if [ -d /vnmr/lc ]
      then
         (cd /vnmr; tar cjf /tmp/corba.tar acqqueue/*.CORBAref lc/FlowCal >/dev/null 2>/dev/null )
         if [ "x`tar -tjf /tmp/corba.tar`" = x ] ; then
             rm /tmp/corba.tar
         else
             cp_corba='y'
         fi
      fi

      if [ -f "$src_code_dir"/.nv ]
      then
         mv_probeid='y'
      fi

      if [ -d /vnmr/adm/users ]
      then
         if [ x$save_p11_users = "xy" ]
         then
             (cd /vnmr/adm;
              if [ -d /vnmr/adm/users/userProfiles ]
              then
               # Do not transfer the All* nor Basic* profiles.  The user should not modify these.
               # We need to be able to install new ones at install time.  Also don't transfer
               # the appdirXXX files.  Old ones can cause problems.
               tar -cj --exclude='All*' --exclude='BasicLiquids*' --exclude='CommonLiquids*' --exclude='appdir*' -f /tmp/admusers.tar users/group users/profiles users/userlist users/properties users/operators users/administrators users/userProfiles
              else
               tar cjf /tmp/admusers.tar users/group users/profiles users/userlist users/properties users/operators users/administrators
              fi
            )
         else
        if [ x$configP11 = "xno" ]
             then
                (cd /vnmr/adm; 
       if [ -d /vnmr/adm/users/userProfiles ]
       then
          tar -cj --exclude='All*' --exclude='BasicLiquids*' --exclude='CommonLiquids*' --exclude='appdir*' -f /tmp/admusers.tar users/group users/profiles users/userlist users/operators users/userProfiles
       else
          tar cjf /tmp/admusers.tar users/group users/profiles users/userlist users/operators
       fi 
                 )
        fi
         fi
         cp_admusers='y'
      fi
      if [ -d /vnmr/dicom/conf ]
      then
    (cd /vnmr/dicom; tar cjf /tmp/dicom.tar conf)
    cp_dicom='y'
      fi
      if [ -d /vnmr/p11 ]
      then
         (cd /vnmr; tar cjf /tmp/p11.tar p11)
         cp_p11='y'
      fi
      if ( test -d /vnmr/gshimlib)
      then
         (cd /vnmr; tar cjf /tmp/gshimlib.tar gshimlib)
         cp_gshimlib='y'
      fi
#       if [ -d /vnmr/pgsql/data ] && [ -d /vnmr/pgsql/persistence ]
#       then
#          (cd /vnmr/pgsql; tar cjf /tmp/database.tar data persistence)
#          cp_database='y'
#       fi

      if [ -d /vnmr/tune/methods ]
      then
         # Don't copy over the standard methods or Qtune stuff,
    # but include the probe files (which can have pretty much any name).
         cat >/tmp/tune.exclude <<EOF
tune/methods
tune/manifest
tune/manual
tune/OptimaFirmware/Optima.bin
EOF
         (cd /vnmr; tar cjfX /tmp/tune.tar /tmp/tune.exclude tune)
         cp_protune='y'
      fi

#
# accounting records
#
      if [ -d /vnmr/adm/accounting/accounts ]
      then
         (cd /vnmr; tar cf /tmp/jaccount.tar adm/accounting/accounts)
         cp_accounting='y'
      fi
      if [ -f /vnmr/adm/accounting/gorecords.xml ]
      then
         (cd /vnmr; cp adm/accounting/gorecords.xml /tmp)
         cp_accounting='y'
      fi
      if [ -f /vnmr/adm/accounting/acctLog.xml ]
      then
         (cd /vnmr; cp adm/accounting/acctLog.xml /tmp)
         cp_accounting='y'
      fi
      if [ -f /vnmr/adm/accounting/acctLog_onHold.xml ]
      then
         (cd /vnmr; cp adm/accounting/acctLog_onHold.xml /tmp)
         cp_accounting='y'
      fi
      if [ -f /vnmr/adm/accounting/loggingParamList ]
      then
         (cd /vnmr; cp adm/accounting/loggingParamList /tmp)
         cp_accounting='y'
      fi
# If the new file exists, save it, else if the old file exists, save it to the new name.
      if [ -f /vnmr/adm/accounting/loginrecords.txt ]
      then
          (cd /vnmr; cp adm/accounting/loginrecords.txt /tmp)
          cp_accounting='y'
      elif [ -f /vnmr/adm/tmp/macrorecords.txt ]
      then
         (cd /vnmr; cp adm/tmp/macrorecords.txt /tmp/loginrecords.txt)
         cp_accounting='y'
      fi
#      if [ -f /vnmr/adm/accounting/gorecords_onHold.xml ]
#      then
#         (cd /vnmr; cp adm/accounting/gorecords_onHold.xml /tmp)
#         cp_accounting='y'
#      fi
#      if [ -f /vnmr/adm/accounting/loginrecords_onHold.txt ]
#      then
#         (cd /vnmr; cp adm/accounting/loginrecords_onHold.txt /tmp)
#         cp_accounting='y'
#      elif [ -f /vnmr/adm/tmp/macrorecords_onHold.txt ]
#      then
#         (cd /vnmr; cp adm/tmp/macrorecords_onHold.txt /tmp/loginrecords_onHold.txt)
#         cp_accounting='y'
#      fi
      if [ -f /vnmr/msg/noHelpOverlay ]
      then
          cp_overlay='y'
      fi
fi

# if the VAST option was selected we 
# still want to attempt restore any previous vast files.
echo $opt_list | grep -s VAST > /dev/null
if [ $? -eq 0 ]
then
   cp_vast='y'
fi

# cp_vast='n'
#echo $opt_list | grep -s VAST > /dev/null
#if [ $? -eq 0 ]
#then
#   if [ -d /vnmr/asm/info ]
#   then
#set -x
#         savevast
#set +x
#         cp_vast='y'
#   fi
#fi

##############################################
#########load the generic files 
echo "-------------------"
gen_testlist=`echo "$gen_list" | tr -d " "`

# if installed by root, some of the initial files might be owned by root,
# change the permissions so that the directories created are writable 
# by the vnmr administrator
if [ x$os_version != "xwin" ]
then
   ${chown_cmd} -R $nmr_adm "$dest_dir"
   ${chgrp_cmd} -R $nmr_group "$dest_dir"
   cd "$dest_dir"
   ${chown_cmd} $nmr_adm .
   ${chgrp_cmd} $nmr_group .
   
fi

#if [ -z gen_testlist ]
if [ x"$gen_testlist" = "x" ]
then
   echo "Skipping NMR's GENERIC files"
else
   echo "Installing NMR's GENERIC files"
   temp_size=0
   tar_size=0
   did_vnmr="n"

   load_type=${cons_type}.$file_ext
   for Item in $gen_list
   do
      if [ x$Item = "x" ]
      then
         continue
      fi
      cat "$src_code_dir"/$os_version/$load_type | \
      ( while read line
        do
           tar_cat=`echo $line | awk 'BEGIN { FS = " " } { print $1 }'`
  
           if [ x$tar_cat = x$Item ]
           then
            tar_size=`echo $line | awk 'BEGIN { FS = " " } { print $2 }'`
         tar_name=`echo $line | awk 'BEGIN { FS = " " } { print $3 }'`
              temp_size=$tar_size
         
         if [ x$tar_name = "x" ]
         then
            continue
         fi

         if (test $tar_name = "code/tarfiles/jre.tar")
              then
                    cd "$source_dir"
                    echo "  Extracting  \"$Item\"  $source_dir/$tar_name"
          if [ x$os_version != "xwin" ]
          then
             if [ x$lflvr != "xdebian" ]
             then
                (su $nmr_adm -fc "cp -r $source_dir/code/jre.linux $dest_dir"/jre)
             else
                (sudo -u $nmr_adm cp -r $source_dir/code/jre.linux $dest_dir/jre )
             fi
          else
             (cp -r "$source_dir"/"$tar_name" "$dest_dir"/jre)
          fi
          if [ $? -ne 0 ]
          then
             echo "Installation of $Item failed"
          fi
          echo "  DONE:  $tar_size KB."
       else
          echo "  Extracting  \"$Item\"  $source_dir/$tar_name"
          if [ x$os_version != "xwin" ]
          then
             if [ x$lflvr != "xdebian" ]
             then
                (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption $source_dir/$tar_name")
             else
                (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption $source_dir/$tar_name )
             fi
          else
             (cd "$dest_dir"; tar $taroption "$source_dir"/"$tar_name")
          fi
               if [ $? -ne 0 ]
                    then
             if [ x$os_version != "xwin" ]
             then
                if [ x$lflvr != "xdebian" ]
                then
                   (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption $source_dir/$tar_name")
                else
                   (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption $source_dir/$tar_name )
                fi
             else
                (cd "$dest_dir"; tar $taroption "$source_dir"/"$tar_name")
             fi
                  if [ $? -ne 0 ]
                       then
                           echo "Installation of $Item failed"
                       fi
                    fi
                    echo "  DONE:  $tar_size KB."
              fi
           fi
        done
      )

      if [ x$Item = "xVNMR" -o x$Item = "xvnmr" ]
      then
         did_vnmr="y"
      fi
   done
fi

##############################################
######### load the passworded options
echo "-------------------------" 

# if [ x$did_vnmr = "xy" ]
# then
#    mkdir "$dest_dir"/adm/options
#    cp "$src_code_dir"/rht/${cons_type}.options "$dest_dir"/adm/options/options
#    cat "$src_code_dir"/rht/${cons_type}.options | (while read line
#      do
#         filename=`echo $line | awk 'BEGIN { FS = " " } { print $1 }'`
#         cp "$src_code_dir"/tarfiles/$filename "$dest_dir"/adm/options/.
#      done
#    )
#    if [ -f "$src_code_dir"/tarfiles/servicetools.pwd ]
#    then
#       cp "$src_code_dir"/tarfiles/servicetools.pwd "$dest_dir"/adm/options/.
#    fi
#    ${chown_cmd} -R $nmr_adm "$dest_dir"/adm/options
#    ${chgrp_cmd} -R $nmr_group "$dest_dir"/adm/options
# fi
# 
# opt_testlist=`echo $opt_list | tr -d " "` 
# 
# if [ x$opt_testlist = "x" ]
# then
#    echo "Skipping PASSWORDED OPTION files"
# else
#    echo "Installing PASSWORDED OPTION files"
# 
#    load_pw_option $opt_list
#    #echo "Passworded Options Completed."
# fi

echo "ALL REQUESTED SOFTWARE EXTRACTED"

################################################################
#fix some things, depending on what system we are
if [ x$did_vnmr = "xy" ]
then

   #Check if jre exists and is newer than 1.1.6, if not, load it

   if [ -x /usr/bin/jre ]
   then
      version=`/usr/bin/jre -version 2>&1 | grep Version`
      minor=`echo $version | awk 'BEGIN { FS = "." } { print $2 }'`
      sub=`echo $version | awk 'BEGIN { FS = "." } { print $3 }'`
      if [ x$minor = "x" ]
      then
         minor=1
         sub=3
      fi 
      if [ $minor -lt 2 ]
      then 
         if [ $sub -lt 6 ]
         then
            load=1
         else
            load=0
         fi
      else
         load=0
      fi
   else
      version=""
      load=1
   fi

   load_type=${cons_type}.opt

   ##########################
   echo "Reconfiguring files... "
   echo " "
   
   if ( test x$load_type = "xmr400.opt" -o x$load_type = "xmr400dd2.opt" -o x$load_type = "xpropulse.opt" )
   then
       here=`pwd`
#       cd $dest_dir/acq/download
#       cp nvScript.ls nvScript
#       cp nvScript.ls.md5 nvScript.md5
#       cd $here
       if [ -f $dest_dir/conpar.400mr ]
       then
          mv $dest_dir/conpar.400mr $dest_dir/conpar
       fi
   else
       rm -f $dest_dir/conpar.400mr
   fi

   # if ( test x$load_type = "xmercury.opt" -o x$load_type = "xmercvx.opt" -o x$load_type = "xmercplus.opt" )
   # then
   #    file="$dest_dir/user_templates"
   #    cp "$file"/global "$file"/tmp
   #    cat "$file"/tmp | sed 's/lockgain 1 1 48 0 1/lockgain 1 1 39 0 1/' | \
   #        sed 's/lockpower 1 1 68 0 1/lockpower 1 1 48 0 1/' > $file/global
   #    rm "$file"/tmp
   # fi

# if still there, delete the following.
   rm -rf "$dest_dir"/user_templates/dg/default.g2000
   rm -rf "$dest_dir"/user_templates/dg/default/dg.unity
   rm -rf "$dest_dir"/user_templates/dg/default/dg.uplus
   rm -rf "$dest_dir"/asm/auto.g2000
   rm -rf "$dest_dir"/asm/auto.unity
   rm -rf "$dest_dir"/asm/auto.uplus
   rm -rf "$dest_dir"/asm/enter.g2000
   rm -rf "$dest_dir"/asm/enter.unity
   rm -rf "$dest_dir"/asm/enter.uplus
   rm -rf "$dest_dir"/asm/experiments.g2000
   rm -rf "$dest_dir"/asm/experiments.unity
   rm -rf "$dest_dir"/asm/experiments.uplus


   if [ ! -d "$dest_dir"/tmp ]
   then
      mkdir "$dest_dir"/tmp
   fi
   chmod 777   "$dest_dir"/tmp
   chmod 666   "$dest_dir"/acq/info
   chmod 775   "$dest_dir"/tune
   if [ ! -d "$dest_dir"/acqqueue ]
   then
      mkdir "$dest_dir"/acqqueue
      mkdir "$dest_dir"/acqqueue/acq
      chmod 777 "$dest_dir"/acqqueue
      chmod 777 "$dest_dir"/acqqueue/acq
      if [ x$os_version != "xwin" ]
      then
   ${chown_cmd} $nmr_adm "$dest_dir"/acqqueue
   ${chgrp_cmd} $nmr_group "$dest_dir"/acqqueue
      fi
   fi
   if [ ! -d "$dest_dir"/tune/OptimaFirmware ]
   then
         mkdir "$dest_dir"/tune/OptimaFirmware
   fi


   if [ ! -d "$dest_dir"/status/logs ]
   then        
        mkdir -p "$dest_dir"/status/logs
         chmod 777 "$dest_dir"/status
         chmod 777 "$dest_dir"/status/logs
   fi

   # Make sure we have cryobay data directory w/ correct permissions
   if [ ! -d "$dest_dir"/cryo/data ]
   then
      mkdir -p "$dest_dir"/cryo/data
      ${chown_cmd} $nmr_adm "$dest_dir"/cryo
      ${chgrp_cmd} $nmr_group "$dest_dir"/cryo
      ${chown_cmd} $nmr_adm "$dest_dir"/cryo/data
      ${chgrp_cmd} $nmr_group "$dest_dir"/cryo/data
      chmod 775 "$dest_dir"/cryo/data
   fi

   old_link=""
   if [ x$vnmr_link = "xyes" ]
   then
      old_link=`readlink /vnmr`
      cd /
      rm -f /vnmr
      ln -s "$dest_dir" /vnmr
      echo "Link '/vnmr' to OpenVnmrJ Software"
   fi

   if [ x$cp_files = "xy" ]
   then
      echo "Restoring conpar, devicenames and devicetable."
      mv /tmp/conpar "$dest_dir"/conpar.prev
      mv /tmp/devicenames "$dest_dir"/devicenames
      mv /tmp/devicetable "$dest_dir"/devicetable
      ${chown_cmd} $nmr_adm   "$dest_dir"/devicenames "$dest_dir"/devicetable "$dest_dir"/conpar.prev
      ${chgrp_cmd} $nmr_group "$dest_dir"/devicenames "$dest_dir"/devicetable "$dest_dir"/conpar.prev
      echo "Restoring shim and probe-calibration files"
      mv /tmp/shims/* "$dest_dir"/shims
      # if probelist is zero length string the for loop doesn't execute.
      probelist=`ls /tmp/probes/`
      for probename in $probelist
      do
         if [ -d /tmp/probes/$probename ]; then
            mv /tmp/probes/$probename "$dest_dir"/probes
         fi
      done
      ${chown_cmd} -R $nmr_adm   "$dest_dir"/shims "$dest_dir"/probes
      ${chgrp_cmd} -R $nmr_group "$dest_dir"/shims "$dest_dir"/probes
      rm -rf /tmp/shims /tmp/probes

      if [ -d /tmp/gshimdir ]
      then 
         echo "Restoring gshimdir"
         mv /tmp/gshimdir "$dest_dir"/gshimdir
         ${chown_cmd} -R $nmr_adm   "$dest_dir"/gshimdir
         ${chgrp_cmd} -R $nmr_group "$dest_dir"/gshimdir
      fi
   fi

   if [ x$mv_probeid = "xy" ]
   then
     probeid_finish
   fi

   # Make gradtables and decclib dirs writable only by owner,
   # regardless of how they are on the CD.
   # If we have old ones, ignore new ones but set permission anyway.

   if [ x$cp_gradtables = "xy" ]
   then
      echo "Restoring gradtables."
#      rm -rf "$dest_dir"/imaging/gradtables
      if [ x$os_version != "xwin" ]
      then
        if [ x$lflvr != "xdebian" ]
        then
      (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/gradtables.tar")
        else
      (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/gradtables.tar )
        fi
      else
   (cd "$dest_dir"; tar $taroption /tmp/gradtables.tar)
      fi
      rm -f /tmp/gradtables.tar
      # replace 156_100S with 156_100_HD in coilIDs
      (  cd "$dest_dir"/imaging;
         if [ -f coilIDs ]
         then
            grep 156_100S coilIDs >  /dev/null;
            if [ $? -eq 0 ] 
            then
               sed 's/156_100S/156_100_HD/' coilIDs > coilIDs.new
               mv coilIDs.new coilIDs
               chown $nmr_adm:$nmr_group coilIDs
               rm -f gradtables/156_100S
            fi
         fi
      )

   fi
   if [ -d "$dest_dir"/imaging/gradtables ]
   then
      chmod 755 "$dest_dir"/imaging/gradtables
   fi
   if [ x$cp_decclib = "xy" ]
   then
      echo "Restoring decclib."
      rm -rf "$dest_dir"/imaging/decclib
      if [ x$os_version != "xwin" ]
      then
        if [ x$lflvr != "xdebian" ]
        then
         (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/decclib.tar")
        else
         (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/decclib.tar )
        fi
      else
        (cd "$dest_dir"; tar $taroption /tmp/decclib.tar)
      fi
      rm -f /tmp/decclib.tar
   fi
   if [ -d "$dest_dir"/imaging/decclib ]
   then
      chmod 755 "$dest_dir"/imaging/decclib
   fi
   if [ x$cp_fastmap = "xy" ]
   then
      echo "Restoring fastmap."
      rm -rf "$dest_dir"/fastmap
      if [ x$os_version != "xwin" ]
      then
        if [ x$lflvr != "xdebian" ]
        then
      (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/fastmap.tar")
        else
      (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/fastmap.tar )
        fi
      else
   (cd "$dest_dir"; tar $taroption /tmp/fastmap.tar)
      fi
   fi
   if [ x$cp_mollib = "xy" ]
   then
   echo "Restoring MOL files."
   if [ x$os_version != "xwin" ]
   then
           if [ x$lflvr != "xdebian" ]
           then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/mollib.tar")
           else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/mollib.tar )
           fi
   else
       (cd "$dest_dir"; tar $taroption /tmp/mollib.tar)
   fi
      rm -f /tmp/mollib.tar
   fi
   
   if [ x$cp_cryomon = "xy" ]
   then
   echo "Restoring Cryomon files."
   if [ x$os_version != "xwin" ]
   then
           if [ x$lflvr != "xdebian" ]
           then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/cryomon.tar")
           else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/cryomon.tar )
           fi
   else
       (cd "$dest_dir"; tar $taroption /tmp/cryomon.tar)
   fi
      rm -f /tmp/cryomon.tar
   fi

   if [ x$cp_amptables = "xy" ]
   then
   echo "Restoring amptables files."
   if [ x$os_version != "xwin" ]
   then
           if [ x$lflvr != "xdebian" ]
           then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/amptables.tar")
           else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/amptables.tar )
           fi
   else
       (cd "$dest_dir"; tar $taroption /tmp/amptables.tar)
   fi
   fi

   
   if [ "x$cp_corba" = "xy" ]
   then
   echo "Restoring LC files."
   if [ x$os_version != "xwin" ]
   then
           if [ x$lflvr != "xdebian" ]
           then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/corba.tar")
           else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/corba.tar )
           fi
   else
       (cd "$dest_dir"; tar $taroption /tmp/corba.tar)
   fi
   rm -f /tmp/corba.tar
   fi
   # besure of correct permissions n the FlowCal file
   if [ -f /vnmr/lc/FlowCal ]; then
      chmod 664 /vnmr/lc/FlowCal
   fi

   if [ x$cp_admusers = "xy" ]
   then
   if [ -f /tmp/admusers.tar ]
        then
           echo "Restoring user files."
           rm -rf "$dest_dir"/adm/users/profile
           rm -rf "$dest_dir"/adm/users/group
           rm -rf "$dest_dir"/adm/users/userlist
      if [ x$os_version != "xwin" ]
      then
              if [ x$lflvr != "xdebian" ]
              then
            (cd "$dest_dir"/adm; su $nmr_adm -fc "tar $taroption /tmp/admusers.tar")
         else
            (cd "$dest_dir"/adm; sudo -u $nmr_adm tar $taroption /tmp/admusers.tar )
              fi
      else
        (cd "$dest_dir"/adm; tar $taroption /tmp/admusers.tar)
      fi
      rm -f /tmp/admusers.tar
           ( cd "$dest_dir"/adm/users/operators; 
             file="automation.conf"
             if [ -f $file ]
             then
                grep ChangeTime $file > /dev/null;
                if [ $? -ne 0 ]
                then
                    cat $file | sed 's/SampleReuse/ChangeTime     180\
SampleReuse/' > auto.conf2
                    mv auto.conf2 $file
                fi
             fi
             file="automation.en.conf"
             if [ -f $file ]
             then
                grep ChangeTime $file > /dev/null;
                if [ $? -ne 0 ]
                then
                    cat $file | sed 's/SampleReuse/ChangeTime     180\
SampleReuse/' > auto.conf2
                    mv auto.conf2 $file
                fi
             fi
             file="automation.ja.conf"
             if [ -f $file ]
             then
                grep ChangeTime $file > /dev/null;
                if [ $? -ne 0 ]
                then
                    cat $file | sed 's/SampleReuse/ChangeTime     180\
SampleReuse/' > auto.conf2
                    mv auto.conf2 $file
                fi
              fi
           )

       fi
   fi
   userDefaults="$dest_dir/adm/users/userDefaults"
   if [ -f "$userDefaults" ]
   then
      if [ x$os_version != "xwin" ]
      then
   cat "$dest_dir"/adm/users/userDefaults | sed '/^home/c\
home yes no '${nmr_home}'/$accname' > "$dest_dir"/adm/users/userDefaults.bak
    
   mv "$dest_dir"/adm/users/userDefaults.bak "$dest_dir"/adm/users/userDefaults
      fi
      if [ x$os_version != "xwin" ]
      then
   chown "$nmr_adm" "$dest_dir"/adm/users/userDefaults
      fi
   fi
   if [ x$cp_dicom = "xy" ]
   then
      rm -rf "$dest_dir"/dicom/conf
      if [ -d "$dest_dir"/dicom ]
      then
         echo "Restoring dicom."
         if [ x$lflvr != "xdebian" ]
         then
           (cd "$dest_dir"/dicom; su $nmr_adm -fc "tar $taroption /tmp/dicom.tar")
         else
           (cd "$dest_dir"/dicom; sudo -u $nmr_adm tar $taroption /tmp/dicom.tar )
         fi
      fi
      rm -f /tmp/dicom.tar
   fi
   if [ x$cp_p11 = "xy" ]
   then
      echo "Restoring part11 files."

      p11Config="$dest_dir"/p11/part11Config

      lines=`grep ":checksum:" $p11Config | wc -l`

      if [ $lines != 0 ]
      then

         echo "Update part11Config file"

         cp $p11Config /tmp/part11Config

         rm -rf "$dest_dir"/p11
    if [ x$os_version != "xwin" ]
    then
            if [ x$lflvr != "xdebian" ]
            then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/p11.tar")
       else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/p11.tar )
            fi
    else
       (cd "$dest_dir"; tar $taroption /tmp/p11.tar)
    fi

         lines=`grep ":checksum:" $p11Config | wc -l`

         if [ $lines = 0 ]
         then
           awk 'BEGIN {FS=":"} {if( $2=="auto" || $2=="study" || $2=="checksum") print $0}' /tmp/part11Config >> $p11Config
         fi
         rm /tmp/part11Config

      else

         rm -rf "$dest_dir"/p11
    if [ x$os_version != "xwin" ]
    then
       if [ x$lflvr != "xdebian" ]
       then
          (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/p11.tar")
       else
          (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/p11.tar )
       fi
    else
       (cd "$dest_dir"; tar $taroption /tmp/p11.tar)
    fi

      fi

   fi

   if [ x$cp_gshimlib = "xy" ]
   then
      echo "Restoring gradient shim map files."
      if [ x$os_version != "xwin" ]
      then
         if [ x$lflvr != "xdebian" ]
         then
            (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/gshimlib.tar")
         else
            (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/gshimlib.tar )
         fi
      else
         (cd "$dest_dir"; tar $taroption /tmp/gshimlib.tar)
      fi
      rm -f /tmp/gshimlib.tar
   fi
#    if [ x$cp_database = "xy" ]
#    then
#       echo "Restoring database files."
#       rm -rf "$dest_dir"/pgsql/persistence
#       rm -rf "$dest_dir"/pgsql/data
#       if [ x$os_version != "xwin" ]
#       then
#          if [ x$lflvr != "xdebian" ]
#          then
#             (cd "$dest_dir"/pgsql; su $nmr_adm -fc "tar $taroption /tmp/database.tar")
#          else
#             (cd "$dest_dir"/pgsql; sudo -u $nmr_adm tar $taroption /tmp/database.tar )
#          fi
#       else
#    (cd "$dest_dir"/pgsql; tar $taroption /tmp/database.tar)
#       fi
#    fi

   if [ x$cp_protune = "xy" ]
   then
      echo "Restoring tune files."
      if [ x$os_version != "xwin" ]
      then
         if [ x$lflvr != "xdebian" ]
         then
            (cd "$dest_dir"; su $nmr_adm -fc "tar $taroption /tmp/tune.tar")
         else
            (cd "$dest_dir"; sudo -u $nmr_adm tar $taroption /tmp/tune.tar )
         fi
      else
        (cd "$dest_dir"; tar $taroption /tmp/tune.tar)
	(cd "$dest_dir"; chmod g+w tune tune/tunecal_* tune/tunecal_*/bak)
      fi
      rm -f /tmp/tune.tar
      rm -f /tmp/tune.exclude
   fi

   cp_merge='n'
   if [ x$cp_accounting = "xy" ]
   then
      echo "Restoring accounting files."
      if [ -f /tmp/jaccount.tar ]
      then
         (cd "$dest_dir"; tar xf /tmp/jaccount.tar)
         rm /tmp/jaccount.tar
      fi
      if [ -f /tmp/acctLog.xml ]
      then
         (cd "$dest_dir"; cp /tmp/acctLog.xml adm/accounting)
         rm -f /tmp/acctLog.xml
      fi
      if [ -f /tmp/acctLog_onHold.xml ]
      then
         (cd "$dest_dir"; cp /tmp/acctLog_onHold.xml adm/accounting)
         rm -f /tmp/acctLog_onHold.xml
      fi
      if [ -f /tmp/loggingParamList ]
      then
         (cd "$dest_dir"; cp /tmp/loggingParamList adm/accounting)
         rm -f /tmp/loggingParamList
      fi
      if [ -f /tmp/gorecords.xml ]
      then
         (cd "$dest_dir"; cp /tmp/gorecords.xml adm/accounting)
         rm -f /tmp/gorecords.xml
         cp_merge='y'
      fi
      if [ -f /tmp/loginrecords.txt ]
      then
         (cd "$dest_dir"; cp /tmp/loginrecords.txt adm/accounting; chmod a+w adm/accounting/loginrecords.txt)
         rm -f /tmp/loginrecords.txt
         cp_merge='y'
      fi
#      if [ -f /tmp/gorecords_onHold.xml ]
#      then
#         (cd "$dest_dir"; cp /tmp/gorecords_onHold.xml adm/accounting)
#         rm -f /tmp/gorecords_onHold.xml
#      fi
#      if [ -f /tmp/loginrecords_onHold.txt ]
#      then
#         (cd "$dest_dir"; cp /tmp/loginrecords_onHold.txt adm/accounting; chmod a+w adm/accounting/loginrecords_onHold.txt)
#         rm -f /tmp/loginrecords_onHold.txt
#      fi
   fi
   chmod 777 "$dest_dir"/adm/accounting
   if [ -f "$dest_dir"/adm/accounting/acctLog.xml ]
   then
      chmod 666 "$dest_dir"/adm/accounting/acctLog.xml
   fi
   if [ x$cp_merge = "xy" ]
   then
      if [ x$os_version != "xwin" ]
      then
         if [ x$lflvr != "xdebian" ]
         then
            (cd "$dest_dir"/bin; su $nmr_adm -fc "./Vnmrbg -mback -n0 mergeAcct")
         else
            (cd "$dest_dir"/bin; sudo -u $nmr_adm ./Vnmrbg -mback -n0 mergeAcct)
         fi
      fi
   fi
   rm -f "$dest_dir"/maclib/mergeAcct
   if [ x$cp_overlay = "xy" ]
   then
      touch  "$dest_dir"/msg/noHelpOverlay
   fi

   # restore vast files is the vast.tar files exists in /tmp
   if [ -f /tmp/vast.tar ]
   then
      echo "Restoring VAST files."
          restorevast
   fi 

   # Clean up 
#   rm -f /tmp/database.tar

   if [ ! -d "$dest_dir"/mollib ]
   then
   mkdir "$dest_dir"/mollib
   if [ x$os_version != "xwin" ]
   then
      if [ x$lflvr != "xdebian" ]
      then
         chown ${nmr_adm}:${nmr_group} "$dest_dir"/mollib
         chmod 755 "$dest_dir"/mollib
      else
         sudo chown ${nmr_adm}:${nmr_group} "$dest_dir"/mollib
         sudo chmod 755 "$dest_dir"/mollib
      fi
   fi
   fi

   case $real_console in
        inova ) explist_file="explist.inova" ;;
        g2000 ) explist_file="explist.g2000" ;;
      mercury ) explist_file="explist.mercury" ;;
            * ) explist_file="explist.inova";;
   esac
   if [ -f "$dest_dir"/asm/${explist_file} ]
   then
      (cd "$dest_dir"/asm; mv -f $explist_file explist; rm -f explist.*)
   fi

   #######################################################################
   #bottom part was move in from outside of this if, need to be rechecked

   #Load individual packages only, do not need the rest
   if [ x$load_main = "xn" ]
   then
      if [ x$os_version = "xwin" ]
      then
   prim_group=`/usr/bin/id -Gn $nmr_adm | $NAWK '{print $1}'`
      else
   prim_group=`groups $nmr_adm | $NAWK '{print $1}'`
      fi
      if [ x$prim_group !=  x$nmr_group ]
      then
         echo Updating group name to ${nmr_group}...
         cd $dest_dir/..
         chgrp -hR $nmr_group "$dest_dir"
      fi

      echo "Software Installation Completed."
      exit
   fi

   if [ x$os_version != "xrht" -a x$os_version != "xwin" ]
   then
      #Use Vnmr Motif library if one not already present
      if (test ! -f /usr/dt/lib/libXm.so)
      then  
         ( cd "$dest_dir"/lib; mv libXmVnmr.so.3 libXm.so.3; ln -s libXm.so.3 libXm.so )
      fi
   fi
 
   #For Solaris 2.6 the types.h file of the GNU compiler can not be used
   #ver=`uname -r`
   #if [ $ver="5.6" ]
   #then  
   #   ( cd $dest_dir/gnu/lib/gcc-lib/sparc-sun-solaris2.3/2.6.3/include/sys;
   #     mv types.h types.h.bk
   #   )  
   #fi 
 
   #finally copy the revision file info to $dest_dir and add $load_type
   rm -rf "$dest_dir"/vnmrrev
   rm -f "$dest_dir"/adm/log/CONSOLE
   cp "$source_dir"/vnmrrev "$dest_dir"/vnmrrev
   #A hack, for now
   if [ x${cons_type} = "xrht" ]
   then
        cons_type="mercplus"
   fi
   echo ${cons_type}  >> "$dest_dir"/vnmrrev
   echo ${cons_type}  > "$dest_dir"/adm/log/CONSOLE
   chown $nmr_adm   "$dest_dir"/vnmrrev "$dest_dir"/adm/log/CONSOLE
   chgrp $nmr_group "$dest_dir"/vnmrrev "$dest_dir"/adm/log/CONSOLE
   chmod 644  "$dest_dir"/vnmrrev "$dest_dir"/adm/log/CONSOLE
   if [ x$lflvr != "xdebian" ]
   then
      chown $nmr_adm   "$dest_dir"/vnmrrev
      chgrp $nmr_group "$dest_dir"/vnmrrev
      chmod 644  "$dest_dir"/vnmrrev
   else
      sudo -u $nmr_adm chown $nmr_adm   "$dest_dir"/vnmrrev
      sudo -u $nmr_adm chgrp $nmr_group "$dest_dir"/vnmrrev
      sudo -u $nmr_adm chmod 644  "$dest_dir"/vnmrrev
   fi

   if [ x$os_version = "xwin" ]
   then
   prim_group=`/usr/bin/id -Gn $nmr_adm | $NAWK '{print $1}'`
   else
   prim_group=`groups $nmr_adm | $NAWK '{print $1}'`
   fi
   if [ x$prim_group !=  x$nmr_group ]
   then
      echo Updating group name to ${nmr_group}...
      cd "$dest_dir"/..
      chgrp -hR $nmr_group "$dest_dir"
   fi

   # finally make sure that execkill is owned by root, executable by root
   if [ x$lflvr != "xdebian" ]
   then
      chown $rootuser "$dest_dir"/bin/execkillacqproc
      chmod 500  "$dest_dir"/bin/execkillacqproc
   else
      sudo chown $rootuser "$dest_dir"/bin/execkillacqproc
      sudo chmod 500  "$dest_dir"/bin/execkillacqproc
   fi

   if [ -f "$dest_dir"/bin/killroboproc ]
   then
     # make sure killroboproc is owned by root, executed as root
     if [ x$lflvr != "xdebian" ]
     then
        chown $rooruser:$rootuser "$dest_dir"/bin/killroboproc
        chmod 4755 "$dest_dir"/bin/killroboproc
     else
        sudo chown $rooruser:$rootuser "$dest_dir"/bin/killroboproc
        sudo chmod 4755 "$dest_dir"/bin/killroboproc
     fi
   fi

   # for VnmrJ set user ID on execution for /vnmr/bin/loginvjpassword
   # this way everyone can change password file which is owned by the admin
   if [ -f "$dest_dir"/bin/loginvjpassword ]
   then
       chmod 4755 "$dest_dir"/bin/loginvjpassword
   fi
   # for VnmrJ set user ID on execution for /vnmr/bin/cptoconpar
   # this way everyone can update sysgcoil in conpar
   if [ -f "$dest_dir"/bin/cptoconpar ]
   then
       chmod 4755 "$dest_dir"/bin/cptoconpar
   fi

   if [ x$os_version = "xwin" ] 
   then
       chmod 755 "$dest_dir"/adm/log
       chmod 755 "$dest_dir"/bin/vnmrj.exe
       chmod 755 "$dest_dir"/bin/vnmrj_debug.exe
       chmod 700 "$dest_dir"/bin/vnmrj_adm.exe
       chmod 777 "$dest_dir"/unins*.dat       
       
       if [ ! -f "$dest_dir"/uninstallvj.bat ]
       then
    cp "$src_code_dir"/code/uninstallvj.bat "$dest_dir"
       fi
       chmod 755 "$dest_dir"/uninstallvj.bat

       chmod g+w "$dest_dir"/pgsql

       # make softlinks of exe in pgsql/bin
       pgsqlfiles=`ls /vnmr/pgsql/bin/*.exe`
       for pgsqlfile in $pgsqlfiles
       do
     pgsqlfile2=`echo $pgsqlfile | awk '{FS="."} {print $1}'`
     ln -s $pgsqlfile $pgsqlfile2
       done

       chmod a+w "$dest_dir"/pgsql
   fi
  

   #To have the Postgres postmaster started at system boot up
   if [ x$os_version != "xwin" ] 
   then
     if [ x$lflvr != "xdebian" ]
     then
        cp -p "$dest_dir"/bin/S99pgsql /etc/init.d/pgsql
     else
        sudo cp -p "$dest_dir"/bin/S99pgsql /etc/init.d/pgsql
     fi
     
     r_levl="rc3.d"
     r_levl0="rc0.d"
   fi

   if [ x$os_version = "xrht" ]
   then
     if [ x$lflvr = "xsuse" ]
     then
       r_levl="init.d/rc5.d"
       r_levl0="init.d/rc0.d"
     else
       r_levl="rc5.d"
       r_levl0="rc0.d"
     fi
     if [ x$lflvr = "xdebian" ]
     then
       # cd /vnmr/lib 
       cd "$dest_dir"/lib
       # for autotest and spincad
       rm -f libBLT.so
       ln -s libBLT24.so.8.4 libBLT.so
       # for autotest and spincad
       # Ubuntu (10.04) has a slightly different path than RHEL
       rm -f /usr/share/tcltk/tk8.4/vnmr
       ln -s /vnmr/tcl/tklibrary/vnmr /usr/share/tcltk/tk8.4/vnmr
     else   # xrht
       # perform the following only if it's NOT RHEL 6.X
       # appears some version of RHEL 5.X don't have lsb_release
       # if [ "$(lsb_release -rs  | grep '6' > /dev/null;echo $?)" != "0" ]; then
       # this would fail for version 5.6
       # if [ "$(echo $distrover | grep '6' > /dev/null;echo $?)" != "0" ]; then
       if [ $distmajor -lt 6 ]
       then
         if [ -f /usr/lib/libtcl8.4.so ]
         then
           cd "$dest_dir"/lib
           rm -f libtcl8.3.so libtk8.3.so
           ln -s /usr/lib/libtcl8.4.so libtcl8.3.so
           ln -s /usr/lib/libtk8.4.so libtk8.3.so
           if  [ -f libBLT24.so ]
           then
             mv libBLT24.so libBLT24.so.8.3
           fi
           rm -f libBLT.so
           ln -s libBLT24.so.8.4 libBLT.so
           rm -f /usr/share/tk8.4/vnmr
         fi
         # rm -f /vnmr/bin/wkhtmltopdf
         # mv /vnmr/bin/wkhtmltopdf-i386 /vnmr/bin/wkhtmltopdf
       else  # RHEL 6.X there will be no /usr/lib/libtcl8.4.so, that's installed further down
         cd "$dest_dir"/lib
         # for autotest and spincad
         rm -f libBLT.so
         ln -s libBLT24.so.8.4 libBLT.so
         # for autotest and spincad
         rm -f /usr/share/tk8.4/vnmr
         # rm -f /vnmr/bin/wkhtmltopdf-i386
       fi
     fi
   fi

   if [ x$os_version != "xwin" ] 
   then
     if [ x$lflvr != "xdebian" ]
     then
        (cd /etc/${r_levl}; if [ ! -f S99pgsql ]; then \
        ln -s ../init.d/pgsql S99pgsql ; fi)
        (cd /etc/${r_levl0}; if [ ! -f K99pgsql ]; then \
        ln -s ../init.d/pgsql K99pgsql ; fi)
        #Remove old versions
        rm -f /etc/init.d/S99pgsql
        rm -f /etc/rc2.d/S99pgsql

        # if a non Vnmr postgres install disable it from start at boot,
        # most likely they use the same port number
        if [ -f /etc/init.d/postgresql ];then
           /etc/init.d/postgresql stop
           mv /etc/init.d/postgresql /etc/init.d/postgresql.moveAside
        fi

     else
        ( cd /etc/init.d; sudo /usr/sbin/update-rc.d -f pgsql remove > /dev/null 2>&1; sudo /usr/sbin/update-rc.d pgsql defaults > /dev/null 2>&1; )
        # if a non Vnmr postgres install disable it from start at boot, most likely they use the same port number
        # Ubuntu installs with a version number, making it bit more difficult, must find the correct postgres-version# script
        postgrescript=`cd /etc/init.d; ls postgresql*`
        if [ ! -z "$postgrescript" ]; then
           sudo /etc/init.d/$postgrescript stop
           ( cd /etc/init.d; postgrscript=`ls postgresql*` ; sudo /usr/sbin/update-rc.d -f $postgrscript remove > /dev/null 2>&1; )
        fi

        #(cd /etc/${r_levl}; if [ ! -f S99pgsql ]; then \
        #sudo ln -s ../init.d/pgsql S99pgsql ; fi)
        #(cd /etc/${r_levl0}; if [ ! -f K99pgsql ]; then \
        #sudo ln -s ../init.d/pgsql K99pgsql ; fi)
        ##Remove old versions
        #sudo rm -f /etc/init.d/S99pgsql
        #sudo rm -f /etc/rc2.d/S99pgsql
     fi
   fi

#   if [ x$os_version = "xsol" ]
#   then
#     ln -s /cdrom/cdrom0/.jhelp /vnmr/jhelp
#   elif [ x$os_version = "xrht" ]
#   then
#     if [ -d /media/cdrom ]
#     then
#        ln -s /media/cdrom/.jhelp /vnmr/jhelp
#     elif [  -d /media/cdrecorder ]
#     then
#        ln -s /media/cdrecorder/.jhelp /vnmr/jhelp
#     else
#        ln -s /mnt/cdrom/.jhelp /vnmr/jhelp
#     fi
#   fi

   #Open Source Sudo package
   sudoers="/etc/sudoers"
   sudofile="/usr/local/bin/sudo"

   if [ x$os_version = "xrht" ]
   then
      #special precaution for Linux
#     /usr/bin/file /usr/local/bin/sudo | grep Intel 2>&1 >/dev/null
#     if [ $? -ne 0 ]
#     then
#        rm -f /usr/local/bin/sudo
#     fi

#     if [ -f /etc/redhat-release ]
#     then
#        lv=`/bin/cat /etc/redhat-release | grep 4` 
#        if [ x"$lv" != "x" ]
#        then
#           rm -f /usr/local/bin/sudo
#        fi
#     fi

      # for debian these packages would need to be already installed (tftp, rarpd)
      if [ x$lflvr != "xsuse" -a x$lflvr != "xdebian" ]
      then
         if [ -f $src_code_dir/linux/dkms-2.0.19-1.noarch.rpm ]
         then
            echo "Installing/Updating dkms package"
            logmsg "Installing/Updating dkms package"
            rpm -U $src_code_dir/linux/dkms-2.0.19-1.noarch.rpm  >> $logfile 2>&1 
         fi

         # install kdiff3 for maintanance/diagnostic usage,etc...
         # For RHEL 6.1 this kidff uses a an older libkonq shared library /usr/lib64/libkonq.so.4
         #  so use --nodpes to avoid the dependency failure on 6.1
         # logmsg "Install kdiff3"
         # rpm -U --nodeps $src_code_dir/linux/kdiff3-0.9.92-1.el5.rf.x86_64.rpm >> $logfile  2>&1
         #  in the 6.1  case just make a symlink between the older lib and the newer one.
         # if [ -h /usr/lib64/libkonq.so.5  -a ! -h /usr/lib64/libkonq.so.4 ] 
         # then 
         #  logmsg "Create symlink between libkonq.so.5 libkonq.so.4"
         #    ( cd /usr/lib64 ; ln -s libkonq.so.5 libkonq.so.4 >/dev/null >> $logfile 2>&1 )
         # fi

         # check for installed dkms package
         #if [ "$(rpm -q dkms | grep 'not installed' > /dev/null;echo $?)" == "0" ]
         #then
         #  echo "Installing dkms package"
         #  # rpm -i $src_code_dir/linux/dkms-2.0.17-1.el5.noarch.rpm >/dev/null  2>&1
         #  #rpm -i $src_code_dir/linux/dkms-2.0.17-1.el5.noarch.rpm  &>/dev/null
         #  rpm -U $src_code_dir/linux/dkms-2.0.19-1.noarch.rpm  &>/dev/null
         #fi


         #if [ ! -x /usr/sbin/rarpd -a -f $src_code_dir/.nv -a x`uname -p` = "xi686" ]         
         #then
         #   echo "Installing rarp server package"
         #   rpm -i $src_code_dir/linux/rarpd-ss981107-14.i386.rpm 2>&1 >/dev/null
         #fi
         #if [ ! -x /usr/sbin/rarpd -a -f $src_code_dir/.nv -a x`uname -p` = "xx86_64" ]
         #then
         #   echo "Installing rarp server package"
         #   # rpm -i $src_code_dir/linux/rarpd-ss981107-18.x86_64.rpm 2>&1 >/dev/null
         #   rpm -U $src_code_dir/linux/rarpd-ss981107-22.2.2.x86_64.rpm 2>&1 >/dev/null
         #fi
 
         if [ -f $src_code_dir/linux/rarpd-ss981107-22.2.2.x86_64.rpm ]
         then
            if [ "$(rpm -q rarpd | grep 'not installed' > /dev/null;echo $?)" == "0" ]; then
                logmsg "Install rarpd"
                rpm -U $src_code_dir/linux/rarpd-ss981107-22.2.2.x86_64.rpm >> $logfile 2>&1
            fi 
         fi 

         # for RHEL 6.1 
         # if [ "$(lsb_release -rs  | grep '6' > /dev/null;echo $?)" == "0" ]; then   # appears some version of RHEL 5.X don't have lsb_release
         # if [ "$(echo $distrover  | grep '6' > /dev/null;echo $?)" == "0" ]; then
         if [ $distmajor -ge 6 ]
         then
            # shareutils package nolonger resides on the RHEL 6.X Installation DVD
           if [ -f $src_code_dir/linux/sharutils-4.7-6.1.el6.x86_64.rpm ]
           then
            if [ "$(rpm -q sharutils  | grep 'not installed' > /dev/null;echo $?)" == "0" ]; then
                logmsg "Install sharutils"
                rpm -U $src_code_dir/linux/sharutils-4.7-6.1.el6.x86_64.rpm >> $logfile 2>&1
            fi 
           fi 
            # Need to install a tcl 8.4 version of the library to support vnmrWish and autotest 
           if [ -f $src_code_dir/linux/tcl-8.4.13-4.el5.i386.rpm ]
           then
            if [ "$(rpm -q tcl-8.4.13-4.el5.i386.rpm | grep 'not installed' > /dev/null;echo $?)" == "0" ]; then
                logmsg "Install tcl 8.4"
                rpm -i --force $src_code_dir/linux/tcl-8.4.13-4.el5.i386.rpm >> $logfile 2>&1
            fi 
           fi
            # Need to install a tk 8.4 version of the library to support vnmrWish and autotest 
           if [ -f $src_code_dir/linux/tk-8.4.13-5.el5_1.1.i386.rpm ]
           then
            if [ "$(rpm -q tk-8.4.13-5.el5_1.1.i386.rpm  | grep 'not installed' > /dev/null;echo $?)" == "0" ]; then
                logmsg "Install tk 8.4"
                rpm -i --force $src_code_dir/linux/tk-8.4.13-5.el5_1.1.i386.rpm >> $logfile  2>&1
            fi 
           fi 
         fi
         if [ ! -d /usr/share/tk8.4/vnmr ]
         then
            logmsg "symlink /vnmr/tcl/tklibrary/vnmr /usr/share/tk8.4/vnmr "
            ln -s /vnmr/tcl/tklibrary/vnmr /usr/share/tk8.4/vnmr
         fi

         if [ -f $src_code_dir/linux/tftp-server-0.32-4.i386.rpm ]
         then
           if [ ! -r /etc/xinetd.d/tftp -a -f $src_code_dir/.nv -a x`uname -p` = "xx86_64" ]
           then
              echo "Installing tftp server package"
              logmsg "Installing tftp server package"
              rpm -i --nodeps $src_code_dir/linux/tftp-server-0.32-4.i386.rpm >> $logfile  2>&1
           fi
           if [ ! -r /etc/xinetd.d/tftp -a ! -f $src_code_dir/.nv ]
           then
              echo "Installing tftp server package"
              logmsg "Installing tftp server package"
              rpm -i $src_code_dir/linux/tftp-server-0.32-4.i386.rpm >> $logfile 2>&1
           fi
         fi
#         if [ ! -x /usr/sbin/bootpd -a ! -f $src_code_dir/.nv ]
#         then
#            echo "Installing bootp package"
#            rpm -i $src_code_dir/linux/bootp-2.4.3-7.i386.rpm 2>&1 >/dev/null
#         fi
         rm -f /vnmr/pgsql/lib/libtermcap.so.2.0.8
      elif [ x$lflvr = "xsuse" ]; then
         (cd /vnmr/pgsql/lib; ln -s libtermcap.so.2.0.8 libtermcap.so.2)
      fi
      # a reminder to check this on the Ubuntu releases, my guess it's OK  GMB
      #if [ x$lflvr = "xdebian" ]
      #then
      #   echo "check on termcap link  (cd /vnmr/pgsql/lib; ln -s libtermcap.so.2.0.8 libtermcap.so.2) "
      #fi

      # now done in the varian_preinstall scripts
      #if [ x$lflvr = "xdebian" ]
      #then
      #    echo "Installation of rarpd (/usr/sbin/rarpd) and tftpd must be check.."
      #    echo "     Use apt-get to install rarpd & tftpd packages."
      #fi

      # decide to go ahead and upgrade it even if there is one already present   GMB  7/22/09
      # if [ ! -x /usr/bin/acroread  -a  ! -x /usr/X11R6/bin/acroread ]
      # then
         if [ x$lflvr = "xrhat" ]
         then
           if [ -f $src_code_dir/linux/AdbeRdr9.3.2-1_i486linux_enu.rpm ]
           then
             echo "Installing/Updating Acrobat package"
             logmsg "Installing/Updating Acrobat package"
#            rpm -i $src_code_dir/linux/acroread-5.08-2.i386.rpm 2>&1 >/dev/null
#            rpm -i $src_code_dir/linux/AdobeReader_enu-8.1.3-1.i486.rpm >/dev/null 2>&1
            rpm -U $src_code_dir/linux/AdbeRdr9.3.2-1_i486linux_enu.rpm >> $logfile 2>&1
#         else
#            echo "     Use apt-get to install debian acroread package..."
#            echo "     Use repository from www.medibuntu.org to obtain acroread for Ubuntu, etc."
            # dpkg --install 
            # aptitude ...
           fi
         fi
         if [ x$lflvr = "xdebian" ] ; then
             # getlibs was install previous by the preinstall for Ubuntu
             #echo "Installing getlibs "
             #logmsg "Installing getlibs "
             #dpkg -i $src_code_dir/linux/getlibs.deb >> $logfile 2>&1
             # for Ubuntu 12.04 xinetd replaced inetd, the rsh-server doesn't add the required file into /etc/inetd.d 
             # so that the rsh server is active, we copy shell,exec,login files to correct this if missing
             # e.g. 12.04
             if [ $distmajor -ge 12 ] ; then
                if [ ! -r /etc/xinetd.d/shell ]
                then
                   echo "Installing xinetd files for rsh server"
                   logmsg "Installing xinetd files for rsh server"
                   (cd /etc/xinetd.d ; sudo tar -xzvf $src_code_dir/linux/xinetd.d.tgz >> $logfile 2>&1 )
                fi
            fi
         fi
         ## if [ x$lflvr = "xrhat" ]
         ## then
             ## Removed for VnmrJ 3.2 release
             ## echo "Installing/Updating Adobe AIR package"
             ## rpm -U $src_code_dir/linux/adobeair.i386.rpm >/dev/null 2>&1
         ## elif [ x$lflvr = "xdebian" ]; then
             ## Removed for VnmrJ 3.2 release
             ## echo "Installing/Updating Adobe AIR package"
              # use --force-acrhitecture so that the 32-bit deb package is installed
              # use the getlibs to do this, it does it more cleanly on 64-bit machines
              # getlibs wasn't getting the jobs done, thus converted the 32 to an all type package
              # allowing a standard isntallation via dpkg
              # getlibs -i $src_code_dir/linux/adobeair202.i386.deb >/dev/null 2>&1
              ## dpkg -i  $src_code_dir/linux/adobeair_all.deb >/dev/null 2>&1
         ## fi
         ## Removed for VnmrJ 3.2 release
         ## if [ -x "/usr/bin/Adobe AIR Application Installer" ]
         ## then
         ##     echo "Installing/Updating Adobe AIR application"
         ##      # usr rpm or dpkg ?
         ##      if [ x$lflvr = "xrhat" ]; then
         ##         # determine air app pakcage name.
         ##         airpkgname=`rpm -qa | grep vj3help`
         ##         # uninstall air app as to allow silnet installtion to work.
         ##         if [ ! -z "$airpkgname" ] ; then
         ##            rpm -e "$airpkgname"  > /dev/null 2>&1
         ##         fi
         ##      elif [ x$lflvr = "xdebian" ] ; then
         ##            airpkgname=`dpkg --get-selections | grep vj3help`
         ##            if [ ! -z "$airpkgname" ] ; then
         ##               dpkg --remove "$airpkgname" > /dev/null 2>&1;
         ##            fi
         ##      fi
         ##      # perform air app silent installation 
         ##      "/usr/bin/Adobe AIR Application Installer" -silent -eulaAccepted -desktopShortcut -programMenu -location $dest_dir/help $src_code_dir/vj3help.air
         ##      # set -x
         ##      # back to using silent installation
         ##      # had to give up on silent installations, now a pop-up will be displayed and the user will have 
         ##      # to click the appropriate button
         ##      # background this user interactive air installation, so as to not stop the progress of the rest of the VnmrJ install
         ##      # ( "/usr/bin/Adobe AIR Application Installer" $src_code_dir/vj3help.air >/dev/null 2>&1 ) &
         ##      # set +x
         ## fi
      # fi
      if [ x$lflvr = "xrhat" ]
      then
       if [ -d $src_code_dir/linux/TurboVNC ]
       then
         echo "Installing/Updating Turbo VNC package"
         logmsg "Installing/Updating Turbo VNC package"
         rpm -U $src_code_dir/linux/TurboVNC/xorg-x11-libs-6.8.2-31.i386.rpm >> $logfile 2>&1
         rpm -U $src_code_dir/linux/TurboVNC/turbojpeg-1.11.x86_64.rpm >> $logfile 2>&1
         rpm -U $src_code_dir/linux/TurboVNC/VirtualGL-2.1.2.x86_64.rpm >> $logfile 2>&1
         rpm -U $src_code_dir/linux/TurboVNC/turbovnc-0.5.1.i386.rpm >> $logfile 2>&1
      #elif [ x$lflvr = "xdebian" ]; then
      #   echo "Installing/Updating Turbo VNC package"
      #   logmsg "Installing/Updating Turbo VNC package"
      #   dpkg -i $src_code_dir/linux/TurboVNC/turbojpeg_1.11_amd64.deb >> $logfile 2>&1
      #   dpkg -i $src_code_dir/linux/TurboVNC/VirtualGL_2.1.2_amd64.deb >> $logfile 2>&1
      #   dpkg -i $src_code_dir/linux/TurboVNC/turbovnc_0.5.1_i386.deb >> $logfile 2>&1
       fi
      fi

      if [ -f $src_code_dir/linux/matlabv714.tbz2 ]
      then
        echo "Installing/Updating MATLAB RUNTIME package"
        logmsg "Installing/Updating MATLAB RUNTIME package"
      #   note this test must change with version of matlab being stalled
        if [ ! -d /opt/MATLAB/v714 ]
        then
            mkdir -p /opt/MATLAB
            ( cd /opt/MATLAB; tar -xjf $src_code_dir/linux/matlabv714.tbz2 >> $logfile 2>&1 ; rm -fr matlab v713; ln -s v714 matlab ; )
            logmsg "symlink v714 matlab"
        fi
      fi
      
      (cd $dest_dir/bin; ln Vnmrbg Vnmr)

      # use the systems convert program, by creating a symlink in /vnmr/bin
      # check if convert is present prior to creating the symbolic link to it
      if [ -x /usr/bin/convert ] ; then
          logmsg "symlink /usr/bin/convert /vnmr/bin/convert"
          ln -s /usr/bin/convert /vnmr/bin/convert
      fi

      # Debian (Ubuntu) does not have the following directory structure for CUPS
      if [ ! -r /usr/share/cups/model/laserjet.ppd ] 
      then
         if [  x$lflvr = "xdebian" ]; then
            # echo "   Are the proper CUPS filters already in place on Ubuntu???"
            echo "CUPS filters already in place on Ubuntu"  # ??
         else
            echo "Unpacking cups filters"
            logmsg "Unpacking cups filters"
            /bin/gunzip /usr/share/cups/model/*.gz >> $logfile 2>&1
         fi 
         logmsg "chmod 666 /dev/console"
         chmod 666 /dev/console
      fi
   fi
#  debain releases depend on its own sudo for many operations, so don't add another one for VnmrJ
#  VnmrJ admin invokes sudo from /usr/local/bin  so if it does not exist we need to copy it
#  otherwise VnmrJ admin will not be able to function properly    GMB 5/1/2009
#  if [ ! -f "$sudofile" ]
#  then
#     case x$os_version in
#        "xrht") 
#                if [ x$lflvr != "xdebian" ]
#                then
#                  logmsg "cp -rf /vnmr/sudo.lnx/usr/local /usr"
#                  cp -rf /vnmr/sudo.lnx/usr/local /usr
#              # else
#                  # copy sudo over to /usr/local/bin for VNmrJ admin to use, should be fixed in VnmrJ
#                  # echo "sudo mkdir -p /usr/local/bin"
#                  # echo "sudo cp -p /usr/bin/sudo /usr/local/bin"
#                fi
#                ;;

#        "xsol") cp -rf /vnmr/sudo/usr/local /usr ;;
#             *) ;;
#     esac
#  fi
#  debain release will always have /etc/sudoers file, so this is skipped
#  if [ ! -f "$sudoers" -a x$os_version != "xwin" ]
#  then
#      logmsg "cp -f $dest_dir/sudo/etc/sudoers /etc"
#      cp -f "$dest_dir"/sudo/etc/sudoers /etc
#  fi

#  debain releases depend on its own sudo for many operations, so don't mess with it!
#  VnmrJ admin invokes sudo from /usr/local/bin  so if it does not exist we need to copy it
#  otherwise VnmrJ admin will not be able to function properly    GMB 5/1/2009
#  if [ x$os_version = "xrht" -a x$lflvr != "xdebian" ]
#  then
#      if [ ! -x /usr/local/bin/sudo ]
#      then
#          logmsg "cp /usr/bin/sudo /usr/local/bin/sudo"
#          cp /usr/bin/sudo /usr/local/bin/sudo
#      fi

#      if [ ! -x /usr/local/sbin/visudo ]
#      then
#         logmsg "cp /usr/sbin/visudo /usr/local/sbin/visudo"
#         cp /usr/sbin/visudo /usr/local/sbin/visudo
#      fi
#  fi

   if [ x$os_version != "xwin" ]
   then
      # execute our sudoins script to  add to the /etc/sudoers file
      # sudoins script has been modified for proper Debian (Ubuntu operation)
      # add, admin username and the programs/scripts to run without passwords
      logmsg "Running sudoers script"
      "$dest_dir"/bin/sudoins $NAWK $configP11 $os_version $nmr_adm $nmr_group >> $logfile 2>&1
      chown $rootuser "$dest_dir"/bin/sudoins
      chmod 500 "$dest_dir"/bin/sudoins
  
#     if [ x$lflvr != "xdebian" ]
#     then
#        chmod 4111 /usr/local/bin/sudo
#        chmod 111 /usr/local/sbin/visudo
#  
#        chmod 440 "$sudoers"
#        chown $rootuser "$sudoers"
#        chgrp $rootuser "$sudoers"
#        rm -rf "$dest_dir"/sudo
#     fi
      logmsg "sudoers script complete"
   fi

## Sudo cmds go to /usr/varian/sbin
## might not need the following
##
##
## since these Vnmr commands will be executed as root, 
## root better own them so others # can't edit them and
## run their own scripts as root, i.e.  *security hole"   GMB
##
#   for cmd in $sudo_cmds
#   do
#       bdir=`echo $cmd | cut -d '/' -f1-2`
#       # just change the /vnmr commands not the /usr system commands
#       if [ "x/vnmr" = "x$bdir" ]
#       then
#    # get rid of the comma
#          ccmd=`echo $cmd | cut -d ',' -f1`
#          chown root $ccmd
#          chmod 555 $ccmd
#      fi
#  done
#

   if [ ! -d /usr/varian/config -a  x$os_version != "xwin" ]
   then
       mkdir -p /usr/varian/config
       chmod 777 /usr/varian/config
   fi
   if [ ! -d "$sbindir" -a x$os_version != "xwin" ]
   then
       mkdir "$sbindir"
   fi
 
   /bin/echo "#!/bin/sh
   if [ \$# -eq 3 -a x\$3 = \"xadMin\" ]
   then
   /bin/mv \$1 \$2
   fi"> "$dest_dir"/bin/vcmdm
   chmod 700 "$dest_dir"/bin/vcmdm
 
   /bin/echo "#!/bin/sh
   if [ \$# -eq 2 -a x\$2 = \"xadMin\" ]
   then
   /bin/rm -rf \$1
   fi"> "$dest_dir"/bin/vcmdr
   chmod 700 "$dest_dir"/bin/vcmdr

   if [ x$os_version != "xwin" ]
   then
      cd "$dest_dir"/bin
      cp adddevices makeuser vnmr_accounting dtsharcntrl "$sbindir"
      if [ -f "$src_code_dir"/.nv ]
      then
        cp probe_mount probe_unmount "$sbindir"
      fi
      mv jtestgroup jtestuser vcmdm vcmdr "$sbindir"

      cp create_pgsql_user "$sbindir"

   fi

   if [ x$os_version = "xrht" ]
   then
      chown root:root "$sbindir"/jtest*
      if [ x$lflvr != "xdebian" ]
      then
         chown root:root "$sbindir"/jtest*
      else
         sudo chown root:root "$sbindir"/jtest*
      fi
   elif [ x$os_version != "xwin" ]
   then
      chown root:other "$sbindir"/jtest*
   fi
   if [ x$os_version != "xwin" ]
   then
      chmod 755 /usr/varian
      chmod 755 "$sbindir"
      chmod 700 "$sbindir"/*
   fi
# Special macros for new system.

   if [ -f "$src_code_dir"/.nv ]
   then
     cd "$dest_dir"/maclib
     mv _sw _sw_orig
     ln -s _sw_ddr _sw
     mv mtune mtune_orig
     ln -s mtune_ddr mtune
   fi

fi

# alter the kudzu configuration file in /etc/init.d to place kudzu into "safe mode" i.e. do ONT probe the
# serial ports, causes havoc with VAST (gilson-215), etc.    GMB 3/2/2007  bugzilla # 5146
if [ x$os_version = "xrht" ]
then
#   if [ x$lflvr != "xsuse" -a x$lflvr != "xdebian" ]
#  kudzu is a RedHat hardware checker, Debian (Ubuntu) does not use Kudzu
   if [ x$lflvr = "xrhat" ]
   then
      cd /etc/init.d
      if [ -f ./kudzu ]
      then
         if [ ! -f ./Varian_kudzu.safe-mode ]
         then
           logmsg "modify RHEL Kudzu to not interogate serial ports"
           cat kudzu | sed s/KUDZU_ARGS=$/KUDZU_ARGS=\"-s\"/ > ./Varian_kudzu.safe-mode
           mv kudzu kudzu.orig
           cp Varian_kudzu.safe-mode kudzu
         else
           cp Varian_kudzu.safe-mode kudzu
         fi
      fi
   fi
fi


# no doubt for some of these command under debian will require sudo  GMB
# just incase the wrong password was given to P11 and the files weren't installed
# test to see if one of them exists, if not skipp the whole p11    GMB  5/01/2009
if [ x$configP11 = "xyes" -a -e ${dest_dir}/bin/vnmrMD5 ]
then
   ( cd ${dest_dir}/bin

      cp chchsums "$sbindir"
      cp /vnmr/bin/vnmrMD5 /tmp
      mv auconvert auevent auinit aupurge aureduce auredt aupw \
         makeP11checksums vnmrMD5 killau killch scanlog "$sbindir"

     mv -f ${dest_dir}/bin/S99scanlog "$sbindir"
     mv -f ${dest_dir}/bin/setupscanlog "$sbindir"
     "$sbindir"/setupscanlog

     # Have the scanlog started at system boot up.  We want this to run
     # at level 1 (single user), level 3 (text login) and level 5 (graphical)
     # Thus, put a link in all three places

     cp -p ${sbindir}/S99scanlog /etc/init.d
     ( cd /etc/rc1.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
     ( cd /etc/rc3.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
     ( cd /etc/rc5.d
       if [ -f S99scanlog ]
       then
         rm -f S99scanlog
       fi
       ln -s /etc/init.d/S99scanlog
     )
   )
   #Need to reboot the computer for this to work
    
    "$sbindir"/makeP11checksums /vnmr $nmr_adm $nmr_group

    #for cmd in $sudo_p11_cmds
    #do
    #    chmod 500 $cmd
    #done
    
    #moved over from the patch
    chmod 644 "$dest_dir"/adm/users/profiles/accPolicy

    p11config="$dest_dir"/p11/part11Config

    # Get parent directory of $dest_dir in case we need it 
    parent=`dirname $dest_dir`

   # Check the part11Config file for a part11Dir entry, if found, use it
   # else default to $parent/vnmrp11 and write this to the config file
   p11dir=`grep part11Dir: /vnmr/p11/part11Config | awk 'BEGIN {FS=":"} {print $2}'`
   if [ x$p11dir = "x" ]
   then
      # If no path given in config file, default to $parent/vnmrp11
      p11dir="$parent/vnmrp11"
      echo "Setting part11Dir to default ($p11dir)"
      /bin/ed -s /vnmr/p11/part11Config > /dev/null << THEEND
/part11Dir:/
d
i
part11Dir:$p11dir
.
w
q
THEEND
    fi

    # Make the directory if necessary
    if [ ! -d "$p11dir" ]
    then
        mkdir -p "$p11dir"
    fi

   # Check the part11Config file for a auditDir entry, if found, use it
   # else default to $parent/vnmrp11/auditTrails and write this to the config file
   auditdir=`grep auditDir: /vnmr/p11/part11Config | awk 'BEGIN {FS=":"} {print $2}'`
   if [ x$auditdir = "x" ]
   then
      # If no path given in config file, default to $parent/vnmrp11/auditTrails
      auditdir="$parent/vnmrp11/auditTrails"
      echo "Setting auditDir to default ($auditdir)"
      /bin/ed -s /vnmr/p11/part11Config  > /dev/null << THEEND
/auditDir:/
d
i
auditDir:$auditdir
.
w
q
THEEND
    fi

    # Make the directory if necessary
    if [ ! -d "$auditdir" ]
    then
        mkdir -p "$auditdir"
    fi

    chown ${nmr_adm}:${nmr_group} "$auditdir" "$p11dir"
    chmod 755 "$auditdir" "$p11dir"

    cd "$sbindir"
    chown $rootuser:$rootuser auconvert aureduce auevent auinit aupurge aupw \
       auredt makeP11checksums vnmrMD5 killau killch scanlog chchsums
    chmod 700 auconvert aureduce auevent auinit aupurge aupw auredt \
              makeP11checksums vnmrMD5 killau killch scanlog chchsums
    if [ -f "$src_code_dir"/.nv ]
    then
       chown $rootuser:$rootuser probe_mount probe_unmount
       chmod 700 probe_mount probe_unmount
    fi

    # Set sticky bit for files in /vnmr/p11/bin
    chmod ug+sw "$dest_dir/p11/bin/safecp"
    chmod ug+sw "$dest_dir/p11/bin/writeAaudit"
    chmod ug+sw "$dest_dir/p11/bin/writeTrash"

else
    if [ -d "$dest_dir"/p11 ]
    then
        rm -rf "$dest_dir"/p11
    fi
fi

# gilalign needs to be able to access /dev/ttyS0
if [ -f "$dest_dir"/bin/gilalign ]
then
  chgrp uucp "$dest_dir"/bin/gilalign
  chmod g+s "$dest_dir"/bin/gilalign
fi

if [ -f "$dest_dir"/pw_fault ]
then
    echo ""
    echo "One or more passwords for the following options were incorrect "
    cat $dest_dir/pw_fault
    echo "     If you have the correct password, you can "
    echo "     (re)load the option separately."
    echo "     Run load.nmr again, and only select the option(s)"
    echo ""
fi
#pw_fault is deleted in ProgressMonitor.java

if [ x$old_link != "x" ]
then
    su ${nmr_adm} -fc "/vnmr/bin/update_OpenVnmrJ /vnmr $old_link fromInstall"
fi

#
#There is a check in ProgressMonitor.java for the below message
echo "Software Installation Completed."

logmsg "Software Installation Completed."
logdone
