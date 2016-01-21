#! /usr/bin/env python

"""Script for checking VnmrJ installation file's sha1 against the sha1 list 
   1st arg is taken to the the root path of directory tree to check, e.g. '/vnmr'
   '/vnmr' is the default if no argument given
"""

__author__ = "Greg Brissey"
__version__ = "$Revision: 1.0 $"
__date__ = "$Date: 2009/04/08 21:57:19 $"
__license__ = "Python"

import sys 
import os
import glob
import datetime
import subprocess
# subprocess.call(["foo.sh","args"],shell=True)
from pydoc import help
# from UserDict import UserDict

#print 'sys.argv[0] =', sys.argv[0]
#pathname = os.path.dirname(sys.argv[0])
#print 'path =', pathname
#print 'full path =', os.path.abspath(pathname)

comparison_count = 0
file_count = 0
ok_count = 0
issue_count = 0

root_path = "/vnmr"
if len(sys.argv) > 1 :
   root_path = sys.argv[1]
   print 'root_path ', root_path


# print 'root_path: ', root_path

# help("sys.argv")
# print 'sys.argv ' , sys.argv
# print len(sys.argv)

# help("subprocess")
# help("os.path")
# help("dict")
# help("string")

#
# special treatment for these release sha1 snapshot files
# to decode paths properly
#
requireSha1Files = [ 'sha1chklist.txt','sha1chklistFiles.txt' ]

# these files will change so ignore missmatch of sha1 for these files
iglist = """
   adm/sha1/sha1chklistFiles.txt
	conpar
	conpar.prev
	devivenames
	dicom/conf
	lc/FlowCal
	conpar.400mr
	user_templates/dg/default/dg.uplus
	user_templates/dg/default/dg.unity
	user_templates/dg/default.g2000/dg.conf
	user_templates/dg/default.g2000/dg.walkup
	user_templates/dg/default.g2000/dg.qknmr
	user_templates/dg/default.g2000/dg.exp
	bin/jtestuser
	bin/jtestgroup
	lib/libtcl8.3.so
	lib/libBLT24.so
"""

# note the lib/libtcl8.3.so, lib/libBLT24.so are symlinks created during installation, thus ignored here
# otherwise report as missing  (MIA)

#
# create list base on the listing above
#
ignorelist = iglist.split()

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
# obtain directory listing of the sha1 files in /vnmr/adm/sha1
#
filereg = os.path.join(sha1_path,'sha1chklist*.txt')
#
# e.g. find files that match '/vnmr/adm/sha1/sha1chklist*.txt'
# filelist = [ "sha1chklist.txt", "sha1chklistOptionsStd.txt", "sha1chklistOptionsPwd.txt", etc. ]
#
sha1FileList = glob.glob(filereg)

#
# create a timestamp file name
# e.g. filename_2009-06-15:14:54:23.txt
#
dateTime = datetime.datetime.today()
datetimestr = dateTime.strftime("%Y-%m-%d:%H:%M:%S")

match_file_name = 'tmatchedlist_' + datetimestr + '.txt'
nomatch_file_name = 'tnomatchedlist_' + datetimestr + '.txt'
match_filepath = os.path.join(log_path,match_file_name)
nomatch_filepath = os.path.join(log_path,nomatch_file_name)

# sorted finial results go into these files
sortmatch_file_name = 'matchedlist_' + datetimestr + '.txt'
sortnomatch_file_name = 'nomatchedlist_' + datetimestr + '.txt'
sortmatch_filepath = os.path.join(log_path,sortmatch_file_name)
sortnomatch_filepath = os.path.join(log_path,sortnomatch_file_name)

#
# file path for file containing the sha1 of the present directory tree
#
installedSha1FilePath = os.path.join(log_path,'installedSha1ChkList.txt')

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
findcmdpost = " \) -exec sha1sum {} >> " + installedSha1FilePath + " \;"
# build up the entire find string command
findcmdstr = findcmdpre + findoptstr + findcmdpost


# e.g.
##  findopt="find " + root_path + " -follow \( -type f -o -type l \) ! \( -wholename '*/pgsql/[dp]*' -o -wholename '*/adm/*' -o -wholename '*/imaging/templates/*' -o -wholename '*/bin/convert' -o -wholename '*/lib/libtk8.3.so' -o -wholename '*/vnmrrev' \) -exec sha1sum {} > " + installedSha1FilePath + " \;"
#

#
# create the file with all sha1sum with the /vnmr dierctory tree
#
print "\nCreating snapshot of current /vnmr files"
# delete tmp sha1 snapshot file if present
if ( os.path.exists(installedSha1FilePath) == True ) :
    os.unlink(installedSha1FilePath)
# create sha1 entries for the sha1 release files, they are skipped by the find listing
for sha1file in sha1FileList:
    sha1cmd = 'sha1sum ' + sha1file + '>> ' + installedSha1FilePath
    sha1proc = subprocess.Popen(sha1cmd, shell=True)
    status = os.waitpid(sha1proc.pid, 0)

# findoutput = subprocess.Popen( findopt, shell=True)
# create sha1 entries for all files found
findoutput = subprocess.Popen( findcmdstr, shell=True)
#print findoutput

#
# normally we need to wait for this process to complete
# e.g. status = os.waitpid(findoutput.pid, 0)
# first do some work while the 'find' is doing it's thing.
#

## findopt = "-follow \( -type f -o -type l \) ! \( -wholename '*/pgsql/[dp]*' -o -wholename '*/adm/*' -o -wholename '*/imaging/templates/*' -o -wholename '*/bin/convert' -o -wholename '*/lib/libtk8.3.so' -o -wholename '*/vnmrrev' \) -exec sha1sum {}\;"
## print findopt
## findoutput = subprocess.Popen( ["find", findopt], shell=False, stdout=PIPE ).communicate()[0]
## output=`mycmd myarg`
##     ==>
##     output = Popen(["mycmd", "myarg"], stdout=PIPE).communicate()[0]
## sts = os.system("mycmd" + " myarg")
##     ==>
##     p = Popen("mycmd" + " myarg", shell=True)
##     pid, sts = os.waitpid(p.pid, 0)


# dict use the sha1 as key for checking agains a match
sha1map = {}
filemap = {}
infolist = []

# list of all the files within the sha1 lists
distrubfiles = []

# list of files that should be present but are not
missingfiles = []

# 
# main program 
#
try:
   # outFile = open(sys.argv[2], 'wb')
   #
   # read all the sha1 list files, generate a dictionary (hash map) base on the sha1
   # create a list of missing files as well when reading in the sha1chklist.txt
   # This probably won't be completely accurate, but a good first step
   # the result will be sha1map a dictionary using the sha1 as the key value is a 
   # tuple of pkg , optiontype, and a list of relative filepaths (with the same sha1)
   # this are typical links, or option files have differnet names but the same contents
   #
   print "Reading Released File Snapshot"
   for file in sha1FileList:
       print 'Reading: %s' % file
       # inFile = open(sys.argv[1], 'rb')
       inFile = open(file, 'rb')
       for line in inFile:
           # skip blank or commented '#' lines
           if ((line[0] != "#") and (line != "\n")):
              comparison_count = comparison_count + 1
              #
              # grab the sha1 and filepath from line
              #
              (sha1, filepath) = line.split(' ',1)
              sha1 = sha1.strip()
              filepath = filepath.strip(' \n')

              #
              # grab the sha1 and filepath from line
              #
              (path, filename) = os.path.split( filepath )

              #
              # filename stripped of directory path
              #
              (path , sha1filename) = os.path.split(file);
              path = path.strip()
              sha1filename = sha1filename.strip(' \n')

              # 
              # for the master list remove the './' directory 
              # 
              if sha1filename in requireSha1Files :
              # if sha1filename == "sha1chklist.txt" :
                  opttype="Required"
                  pkg="VNMR"
                  # 
                  # remove the ./ from path
                  # 
                  (dot, relativepath) = filepath.split(os.path.sep, 1)
                  # print "dot: '%s', path: '%s'" % (dot, relativepath)
                  
                  # 
                  # if the file is in sha1chklist (master list) and not on disk 
                  # then this file is missing.
                  # 
                  if ( os.path.exists(os.path.join(root_path,relativepath)) != True ) :
                      # print "filepath: '%s' not found" % relativepath
                      missingfiles.append(relativepath)
              else:
                  # 
                  # for standard and password options remove the 1st two directories, 
                  # then they will be relative to /vnmr
                  # 
                  (opttype, pkg, relativepath) = filepath.split(os.path.sep, 2)
                  opttype = opttype.strip()
                  pkg = pkg.strip()

              #
              # if sha1 read in is not in the dictionary yet, then just add it
              # otherwise check if the file is in the filelist, if not add 
              # to the list.  Thus when completed, the list will contain all 
              # files that have the same sha1
              #
              if sha1map.has_key(sha1) :
                 # print "sha1map has key %s" % sha1
                 (dum1, dum2, infolist) = sha1map[sha1]
                 # print "keyed files %s" % infolist
                 # print "relative filepath: %s" % relativepath
                 if (relativepath in infolist) == False :
                    # print "Appending to infolist"
                    infolist.append(relativepath)
                    # print "new list: %s" % infolist
                 #else:
                 #   # print "relative path %s is in %s" % (relativepath,infolist)
                 #   pass    # no-op
              else:   
                 infolist = [ relativepath ]
                 sha1map[sha1] = (opttype, pkg, infolist)
                 # print "Setting sha1map %s %s" % (sha1, infolist)

              filemap[relativepath] = sha1
              distrubfiles.append(relativepath)
              # print 'opt: %s, package: %s, path: %s\n' % (opttype,pkg,relativepath)
              # 2.6.1 print 'We are the {0} who say "{1}!"'.format('knights', 'Ni')
       # print "close file\n"
       inFile.close()

       # print distrubfiles
   print "Complete... %d Files in Snapshot." %  comparison_count

   #
   # now we need to wait for find process to complete...
   #
   print "Waiting for Snapshot of Current Files to Complete.... "
   status = os.waitpid(findoutput.pid, 0)
   #print status
   print "Snapshot Complete.... "

   matchFile = open(match_filepath, 'wb')
   nomatchFile = open(nomatch_filepath, 'wb')
   # print "Reading " + installedSha1FilePath
   # inFile = open("installedSha1ChkList.txt", 'rb')
   inFile = open(installedSha1FilePath, 'rb')
   #
   # read the sha1 listing of present file system and compare sha1 
   #
   print "Reading Current Snapshot of /vnmr Files: %s" % installedSha1FilePath
   print "Creating Comparison Results to Release Snapshot"
   for line in inFile:
           # skip blank or commented '#' lines
           if ((line[0] != "#") and (line != "\n")):
              file_count = file_count + 1
              (sha1, filepath) = line.split(' ',1)
              sha1 = sha1.strip()
              filepath = filepath.strip(' \n')

              if sha1map.has_key(sha1) :

                 ( opttype, pkg, filelst ) = sha1map[sha1]

                 # check for name match, for comparison make sure this is not an absolute path
                 if os.path.isabs(filepath) :
                    (blank, root, filepath) = filepath.split(os.path.sep, 2)

                 if (filepath in filelst) == True:
                     ok_count = ok_count + 1
                     matchFile.write( "%s   %s   %s   OK\n" % ( pkg, opttype, filepath ) )
                 else:
                     issue_count = issue_count + 1
                     matchFile.write( "%s   %s   %s     has equivalent: %s \n" % (pkg, opttype, filelst[0], filepath) )
                     nomatchFile.write( "%s   %s   %s     has equivalent: %s \n" % (pkg, opttype, filelst[0], filepath) )

              else:
                 # no match, for comparison make sure this is not an absolute path
                 #if  filename1 == "convertbru" :
                 #    print "sha1 nomatch: %s %s\n" % (sha1,filepath)
                 if os.path.isabs(filepath) :
                    (blank, root, filepath) = filepath.split(os.path.sep, 2)
                    # print "root: '%s', path: '%s'" % (root, filepath)

                 # if file is in distrubuted files, then this is a missmatch
                 if (filepath in ignorelist) == False:
                     if filepath in distrubfiles :
                         issue_count = issue_count + 1
                         origsha1 = filemap[filepath]
                         nomatchFile.write(  "Sha1 missmatch: %s (r) vs %s (c): %s \n" % (origsha1, sha1, filepath) )
                     else:
                         # print "No match for %s  %s\n" % (sha1,filepath)
                         # if this file is not a distrubed file, then we don't know what's it doing here
                         # print "filename: %s" % filepath
                         issue_count = issue_count + 1
                         nomatchFile.write(  "Unknown file: %s :  %s\n" % (sha1, filepath) )

   inFile.close()
   matchFile.close()
   if  len(missingfiles) > 0 :
       for missing in missingfiles:
            if (missing in ignorelist) == False:
               nomatchFile.write(  "Possible MIA: %s \n" %  missing )
       
   nomatchFile.close()
              

except IOError:
   print 'file: ', file, ' not found.'
   exit

#finally:
print "Finished, Analyzed %d Files" % file_count
print "    %d Files are Consistent with Release Snapshot" %  ok_count
print "    %d Files are Inconsistent with Release Snapshot" %  issue_count

matchsortstr = "sort -r " + match_filepath + " > " + sortmatch_filepath
nomatchsortstr = "sort " + nomatch_filepath + " > " + sortnomatch_filepath
sort1 = subprocess.Popen( matchsortstr, shell=True)
sort2 = subprocess.Popen( nomatchsortstr, shell=True)
status = os.waitpid(sort1.pid, 0)
status = os.waitpid(sort2.pid, 0)
os.unlink(match_filepath)
os.unlink(nomatch_filepath)

print "\nMatching Output log to: %s" % sortmatch_filepath
print "Inconsistencies log to: %s\n" % sortnomatch_filepath
#    print sha1map
# print missingfiles

