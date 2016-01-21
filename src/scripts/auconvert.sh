: '@(#)auconvert.sh 22.1 03/24/08 1999-2002 '
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
# auconvert.sh
#
# This script converts the original BSM audit file format into 
# a human-readable file format and stores new file(s) at 
# /tmp/uaudit with .prau prefix then renames the original
# audit file to .orig prefix
#
# Usage: auconvert
#        auconvert  <audit_directory> <dest_directory> <save_directory
#
#	if none of argument given: audit_directory = current directory
#				   dest_directory  = /tmp/uaudit
#				   save_directory  = current directory
#
#	if one arguments given:    dest_directory  = /tmp/uaudit
#				   save_directory  = current directory
#
#	if two arguments given:    save_directory  = current directory
#
#       where:  audit_directory is where the original unix audit files are at
#		dest_directory is where the readable coverted files will be placed
#						 with .praud extension
#		save_directory is where the original unix audit files are to be saved
#				        with the same name and .orig extension appended
#

audit_dir=`pwd`
dest_dir=
#save_dir=`pwd`

if [ $# -gt 0 ]
then
	audit_dir=$1
fi

if [ ! -d $audit_dir ]
then 
	echo "\nAudit record directory does not exists\n"
	exit
fi

save_dir=$audit_dir

if [ $# -gt 1 -a x$2 != "x" ]
then
	dest_dir=$2
#else
#    if [ ! -d /tmp/uaudit ]
#    then
#	mkdir -p /tmp/uaudit
#    fi
#    dest_dir=/tmp/uaudit
fi

if [ ! -d $dest_dir ]
then 
	echo "\nDestination directory does not exists\n"
	exit
fi

if [ x$3 != "x" ]
then
	save_dir=$3
fi

if [ ! -d $save_dir ]
then 
	echo "\nSave directory does not exists\n"
	exit
fi

#close off the *.not_terminated.*
#ps -e | grep auditd 2>&1 > /dev/null
#if [ $? -eq 0 ]
#then
#	audit -n
#fi

ls $audit_dir |
nawk '
BEGIN { tmpfile="/tmp/please_remove_me" }

{
	FileName=$0

	if ( FileName ~ /^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]./ &&
	     FileName !~ /not_terminated/ &&
	     FileName !~ /.orig$/ ) {

		fullpath="'$audit_dir'""/"FileName
		system( "/usr/sbin/auditreduce " fullpath " | /usr/sbin/praudit > " tmpfile )

		destpath="'$dest_dir'" "/" FileName ".praud"
		system( "mv " tmpfile " " destpath)

		if ( "'$save_dir'" == "'$audit_dir'" )
		    system( "mv " fullpath " " fullpath ".orig" )

		else {
		    savepath="'$save_dir'" "/" FileName ".orig"
		    system( "mv " fullpath " " savepath )
		}
	}

	#leave the not_terminated file intact for auditd to close it 
	if ( FileName ~ /^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]./ && 
	     FileName ~ /not_terminated/ && 
	     FileName !~ /.praud$/ && 
	     FileName !~ /.cond$/ && 
	     FileName !~ /.orig$/ ) {

		fullpath="'$audit_dir'""/"FileName
		system( "/usr/sbin/auditreduce " fullpath " | /usr/sbin/praudit > " tmpfile )

		destpath="'$dest_dir'" "/" FileName ".praud"
		system( "mv " tmpfile " " destpath)
	}
}'
