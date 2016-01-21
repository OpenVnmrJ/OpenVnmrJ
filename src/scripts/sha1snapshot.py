#! /usr/bin/env python

"""Script for checking VnmrJ installation file's sha1 against the sha1 list 
   1st arg is taken to the the root path of directory tree to check, e.g. '/vnmr'
   '/vnmr' is the default if no argument given
"""

__author__ = "Greg Brissey"
__version__ = "$Revision: 1.0 $"
__date__ = "$Date: 2009/20/04 21:57:19 $"
__license__ = "Python"

import sys 
import os
import glob
import datetime
import subprocess

root_path = "/vnmr"
if len(sys.argv) > 1 :
   root_path = sys.argv[1]
   print 'root_path ', root_path


#
# path to where created files are logged
# /vnmr/adm/log
#
log_path = os.path.join(root_path,'adm','log')

#
# path to the sha1 files for this vnmrj
# /vnmr/adm/sha1
#
sha1_path = os.path.join(root_path,'adm','sha1')

#
# create a timestamp file name
# e.g. filename_2009-06-15:14:54:23.txt
#
dateTime = datetime.datetime.today()
datetimestr = dateTime.strftime("%Y-%m-%d:%H:%M:%S")

sha1snapshot_file_name = 'sha1snapshot_' + datetimestr + '.txt'
sha1snapshot_filepath = os.path.join(sha1_path,sha1snapshot_file_name)

#
#list of directories, etc. for the find to skip, since these directories
# contained modified files
#
skiplisting = """
         */pgsql/[dp]*
         */adm/*
         */imaging/templates/*
         */bin/convert
         */lib/libtk8.3.so
         */vnmrrev
         */probes/*
         */shims/*
         */gshimdir/*
         */imaging/coilIDs/*
         */imaging/decclib/*
         */fastmap/*
         */mollib/*
         */asm/*
         */p11/*
         */tune/*
         */tmp/*
         */acqqueue/*
"""
skiplist = skiplisting.split()
findtest = ""
#
# create the find option string, end up with extra -o which is removed below
#
for path in skiplist:
    findtest += "-wholename '" + path + "' -o " 

#
# create a list, last entry will be that pesty last -o
#
findlist = findtest.split()
#
# now convert the list back into a string but without the last -o
#
findoptstr = " ".join(findlist[0:-1])

findcmdpre = "find " + root_path + " -follow \( -type f -o -type l \) ! \( "
findcmdpost = " \) -exec sha1sum {} > " + sha1snapshot_filepath + " \;"
# build up the entire find string command
findcmdstr = findcmdpre + findoptstr + findcmdpost

#
# create the file with all sha1sum with the /vnmr dierctory tree
#
print "\nCreating snapshot of current /vnmr files"
# findoutput = subprocess.Popen( findopt, shell=True)
findoutput = subprocess.Popen( findcmdstr, shell=True)
status = os.waitpid(findoutput.pid, 0)
print "Snapshot Complete.... "

print "\nShanpshot Output log to: %s" % sha1snapshot_filepath

