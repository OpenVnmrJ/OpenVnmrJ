: '@(#)aupw.sh 22.1 03/24/08 1999-2002 '
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
#aupw.sh
#This script only deals with local users
#Usage: aupw option username
#
#               where: option =  -a (for activate)
#                                -s (for status)
#                      user   =  "all"  (for all users)
#                                user_name  (for individual user)
#
# possible status are: PS for passworded or locked
#                     LK for locked
#                     NP for no password                

if [ $# -eq 2 ]
then
    opt=$1
    usr=$2
    case $opt in

        "-a" )

              if [ $usr = "xall" ]
              then 
                  echo "Can not unlock all users"
              else
                  /usr/bin/passwd  -uf $usr
              fi
              ;;

        "-s" )

            if [ x$usr = "xall" ]
            then
                # Get just normal user entries from passwd file, not system specific users
                for i in `cat /etc/passwd | awk ' BEGIN {FS=":"}  \
		    { if ( $6 != "" && $7 != "" && $7 != "/sbin/nologin" && $6 != "/"  \
                        && substr($6,0,5) != "/sbin" && substr($6,0,4) != "/var" \
                        && $1 != "root" && $1 != "acqproc" && $1 != "pvm" ) print $1}'`
                do 
                    # now we have the name of a user, we need to determine if the user
                    # account is PS (password set and active), LK (Locked) or NP (no password)
                    # Solaris used the passwd -s command.  Linux only has a -S option and
                    # it is not sufficient.  Thus, we will just parse the shadow file
                    # to determine the info we need to return.  In the shadow file,
                    # the second location is the coded password.  If this starts with
                    # "!!", then the account has been locked with passwd -l command (LK).
                    # If the password entry is empty, there is no password (NP).
                    # Else it must be an active password (PS).

                    # Get the appropriate line from the shadow file for this user
                    line=`/bin/grep -w $i /etc/shadow`

                    status=`echo $line | awk ' BEGIN {FS=":"}  { \
                        if ( substr($2,0,2) == "!!" ) print "LK"
                        else if ( $2 == "" ) print "NP"
                        else print "PS"}'`
                    echo $i $status 
                done
    
            else
                echo `passwd -S $usr`
            fi
            ;;
    esac

else
    echo "Usage: aupw option username"
fi
