#! /usr/bin/expect -f
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

# Checks unix password by doing su username password

if {$argc == 0} {
	set password ""
	send_user "Please provide username, password\n"
	return -1
} elseif {$argc == 1} {
	set user [lindex $argv 0]
	set password ""
} elseif {$argc  == 2} {
	set user [lindex $argv 0]
	set passfile [lindex $argv 1]
        if [catch {open $passfile r} fileid] {
           set password $passfile
        } else {
          set password [read -nonewline $fileid]
          close $fileid
          file delete $passfile
        }
} else {
	send_user "Invalid Password\n"
	return -1
}

set keyword "magic_word"

spawn su $user -c "echo $keyword"

expect_before eof {
	send_user "Invalid password\n"
	return -1
}

expect -re $keyword {
	send_user "Password correct\n"
	exit
} "Password:" {
	after 10
	send "$password\n"
}

# expect "\r\n"
#log_user 1

expect "Choose*" {
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
}  -re $keyword {
	send_user "Password correct\n"
	exit
} 

expect "su:*" {
	send_user "user id not known Invalid password"
	exit
}



# expect eof
# interact +

