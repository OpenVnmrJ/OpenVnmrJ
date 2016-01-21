#: '@(#)loginpassword.sh 22.1 03/24/08 1999-2002 '
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
# \

# wrapper to make passwd(1) be non-interactive
# username is passed as 1st arg, passwd as 2nd


set cursor "((%|#|\\$) |>|])$"
catch {set cursor $env(EXPECT_PROMPT)}

proc passwd {user password passwordnew cursor} {
    send_user "Password correct\n"
    
    set ostype [exec uname -s]
    if ![string match $ostype "Linux"] {
	send "passwd $user\r"
    } else {
	send "passwd \r"
    }
    expect_after default {
	send_user "Invalid Password\n"
	return -1
    }
    expect "ssword:"
    after 10
    send "$password\r"
    expect "ssword:"
    after 10
    send "$passwordnew\r"
    expect "ssword:"
    after 10
    send "$passwordnew\r"
    expect eof {
	send_user "Password successfully changed\n"
	exit
    } "failure"  {
	send_user "Invalid Password\n"
	exit
    } "error"  {
	send_user "Invalid Password\n"
	exit
    } -re $cursor {
	send_user "Password successfully changed\n"
	exit
    } 
}

if {[llength $argv]>2} {
    set user [lindex $argv 0]
    set password [lindex $argv 1]
    set passwordnew [lindex $argv 2]
	
    spawn su $user

    expect_before eof {
	send_user "Invalid password\n"
	return -1
    }

    expect -re $cursor {
	send_user "Password correct\n"
	passwd $user $password $passwordnew $cursor
    } "Password:" {
	after 10
	send "$password\r"
    }

    expect -re $cursor {
	passwd $user $password $passwordnew $cursor
    } "Choose*" {
 	send "\r"
    } "incorrect" {
	send_user "Invalid password\n"
	return -1
    } timeout {
	send_user "Connection to host timed out Invalid password"
	exit
    } eof {
	send_user "connection to host failed: "
	send_user "$expect_out(buffer) Invalid password"
	exit
    } "Sorry" {
	send_user "invalid password\n"
	exit 
    }  
} else {
    send "Please provide username, password, new password\n"
    send_user "Invalid Password\n"
}
