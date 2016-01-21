#/bin/sh

# scanloglnx.sh
# Check syslog file for Repeatedly failed login, 
# mail the incident to nmr_adm


vnmrsystem=/vnmr
logfile=/var/log/authlogp11
vnmr_audit_log=/var/log/vnmr_audit_log
sleep_duration=15
nmr_adm=`/bin/ls -l $vnmrsystem/vnmrrev | awk '{print $3}'`
usertype=local

# Echo the contents of the 'notice' file which should contain a list of
# administrator email addresses
get_notice() {
    notice_file=$vnmrsystem/adm/p11/notice
    if [ -s $notice_file ]
    then
        /bin/echo `/bin/cat $vnmrsystem/adm/p11/notice`
    else
        /bin/echo "root"
    fi
}



if [ ! -r $logfile ]
# Create an empty log file if none exist
then
    /bin/touch $logfile
    /bin/chmod 600 $logfile
    /bin/chown root:sys $logfile
fi


if [ ! -r $vnmr_audit_log ]
# Create an empty log file if none exist
then
    /bin/touch $vnmr_audit_log
    /bin/chmod 600 $vnmr_audit_log
    /bin/chown $nmr_adm:nmr $vnmr_audit_log
fi

# Making sure logfile is empty otherwise next grep might go crazy
# save these infos before emptying it
/bin/cat $logfile >> $vnmr_audit_log
/bin/cat /dev/null > $logfile

while :
do
    # Get the date from the last line of the log file
    date=`/bin/grep -i "authentication failure" $logfile | /usr/bin/tail -1 | /usr/bin/cut -d" " -f1,2`

    # Put just failures on this date into tmp file
    /bin/grep -i "authentication failure" $logfile | /bin/grep  "$date" > /tmp/linesThisDate

    # Go through the failures for this date in /tmp/linesThisDate
    while read line
    do

	# get the user account being tried
	str=`/bin/echo $line | /usr/bin/awk '{print $14}'` 
	# avoid errors if like was too short to get a user from
	if [ x$str = "x" ]
	then
	    continue
        fi

	user=`/usr/bin/expr substr $str 6 100`
	# See how many failures this user has in the tmp file
	count=`/bin/grep -cw "user=$user" /tmp/linesThisDate`
	if [ $count -gt 3 ]
	then
	    # found one with too many failures
	    # Create a msg for admin
	    /bin/echo "WARNING: Repeated Failed Login for $user." > /tmp/warningMsg
	    if [ "x$user" != "xroot" ]
	    then
		# Is this user a local account with local password control
		/usr/bin/passwd -S $user > /tmp/passwdout
		/bin/grep "Unknown user"  /tmp/passwdout
		if [ $? -eq 0 ]
		then
                    usertype=network
		    /bin/echo "The user '$user' does not seem to be a locally defined user."  >> /tmp/warningMsg
		    /bin/echo "This user's account cannot be disabled." >> /tmp/warningMsg
		    /bin/echo "If this user is a network user, you will need to contact your" >> /tmp/warningMsg
		    /bin/echo "network administrator for assistance."  >> /tmp/warningMsg
		else
		    /bin/echo "The $user account is being disabled." >> /tmp/warningMsg
		    /bin/echo "This account can be reenabled in 'Vnmrj adm' => 'Show disabled accounts' => 'Activate Account'." >> /tmp/warningMsg
                fi
            else
                /bin/echo "Not disabling the root account."  >> /tmp/warningMsg
            fi
	    /bin/echo "" >> /tmp/warningMsg
	    /bin/echo "Authentication failures on this date are as follows:"  >> /tmp/warningMsg
	    /bin/cat /tmp/linesThisDate  >> /tmp/warningMsg

	    # Send an alert to the admin
	    /bin/cat /tmp/warningMsg | /bin/mailx -s "WARNING: Repeated Failed Login for $user" `get_notice`

	    # Log the alert
	    /bin/echo `/bin/date` "  Mailed Repeated Failed Login to administrator" >> $vnmr_audit_log

	    # Save messages in audit file and clean up the log file
	    /bin/cat $logfile >> $vnmr_audit_log
	    /bin/cat /dev/null > $logfile

	    # Disable the account if not root and not a local account
	    if [ "x$user" != "xroot" -a "x$usertype" = "xlocal" ]
	    then
		# Lock the account
	        /usr/bin/passwd -l $user

		# Log the locking in the audittrail
		cd $vnmrsystem/p11
		auditdir=`/bin/grep auditDir part11Config | /usr/bin/cut -d":" -f2`
		cd $auditdir/user
		# Does any file exist yet?  If not we have no place to write, so just skip it
                flist=`ls *.uat`
                if [ x$flist != "x" ]
                then
                    # a file must exist, get the newest one
                    filelist=`ls -1t *.uat | head -1`
		    if [ -f $filelist ]
                    then
		        echo `/bin/date`"$curdate|$nmr_adm|$user|Locked $user account due to failed login attempts" >> $filelist
                    fi
                fi
            fi

	    break
	fi
    done < /tmp/linesThisDate


    # Cleanup
    rm -f /tmp/linesThisDate
    rm -f /tmp/warningMsg
    rm -f /tmp/passwdout


    # Wait a bit before cycling again
    /bin/sleep $sleep_duration

done



