#!/bin/sh
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
# \
exec $vnmrsystem/tcl/bin/vnmrWish -f "$0" ${1+"$@"}

destroy .
set directory [lindex $argv 1]
set exp [lindex $argv 2]
set file [lindex $argv 3]
set dir [file dirname $file]
set filename [file tail $file]
cd $dir
set mac [lindex $argv 4]
vnmrinit \"[lindex $argv 0]\" $env(vnmrsystem)
after 10000
exec rm -f [lindex $argv 5]
set files [glob -nocomplain -- $filename]
while {$files == ""} {
   after 2000
   set files [glob -nocomplain -- $filename]
}
vnmrsend ${mac}('$directory',$exp,'$files')
