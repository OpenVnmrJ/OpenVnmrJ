#! /usr/bin/env python

"""Script for modified the /etc/X11/xorg.conf file for sharing the desktop via 
   VNC server 
"""

__author__ = "Greg Brissey"
__version__ = "$Revision: 1.0 $"
__date__ = "$Date: 2009/08/11 11:57:19 $"
__license__ = "Python"

#
# Read the /etc/X11/xorg.conf and create a new xorg.conf 
# Done in two passes.
# 1st pass is to determine if the changes are already present
# 2nd apply those changes that have not been done.
# We are making changes to or adding the ServerLayout, Monitor, Screen and Module sections
# if we don't find any then we will add them to the end of the file
# The Screen section additions involve the 3 VNC authorization related settings
# which are placed after the DefaultDepth setting
# For the Module section the Load of vnc X module option is added
# The ServerLayout section additions involve the 3 Option to turn off blanking settings
# The Monitor section additions involve the 1 Option to turn off DPMS (Display Power Management System)
# if it blanks, the vnc client losses its access control
#

import sys
import os
import glob
import datetime
import subprocess
# subprocess.call(["foo.sh","args"],shell=True)
from pydoc import help
# from UserDict import UserDict

# help("sys")
# help("subprocess")
# help("string")

#print 'sys.argv[0] =', sys.argv[0]
#pathname = os.path.dirname(sys.argv[0])
#print 'path =', pathname
#print 'full path =', os.path.abspath(pathname)

# addition lines to the xorg.con file
sectionMonitor='Section "Monitor"\n'
sectionServerLayout='Section "ServerLayout"\n'
sectionScreen='Section "Screen"\n'
sectionModule='Section "Module"\n'
endSection='EndSection\n'

screenOption1='\tOption\t\t"SecurityTypes" "VncAuth"\n'
screenOption2='\tOption\t\t"UserPasswdVerifier" "VncAuth"\n'
screenOption3='\tOption\t\t"PasswordFile" "/root/.vnc/passwd"\n'

moduleLoad='\tLoad\t\t"vnc"\n'

MonitorOptionFrom='"DPMS"'
MonitorOption='\tOption\t\t"NODPMS"\n'

ServerLayoutOption1='\tOption\t\t"BlankTime"  "0"\n'
ServerLayoutOption2='\tOption\t\t"StandbyTime"  "0"\n'
ServerLayoutOption3='\tOption\t\t"SuspendTime"  "0"\n'
ServerLayoutOption4='\tOption\t\t"OffTime"   "0"\n'


XORGdir='/etc/X11'
XORGfile='xorg.conf'
XORGnfile='xorg.conf.vnc'

#
# get our uid and make sure we are running as root
#
uid=os.getuid()
if (uid != 0):
   print "Must be run with root privileges."
   sys.exit(11)

#
# create a timestamp for the file name
# e.g. filename_2009-06-15:14:54.txt
#
dateTime = datetime.datetime.today()
datetimestr = dateTime.strftime("%Y-%m-%d:%H:%M")

backupname = XORGfile + '_' + datetimestr
Xorg_filepath = os.path.join(XORGdir,XORGfile)
Xorg_bkup_filepath = os.path.join(XORGdir,backupname)
Xorg_nfilepath = os.path.join(XORGdir,XORGnfile)
# Xorg_nfilepath = os.path.join('/tmp',XORGnfile)

# creat the copy command to creat the backup
cp2bkup = 'cp ' + Xorg_filepath + ' ' + Xorg_bkup_filepath
# creat the copy command to over write xorg.conf with the new one
cp2xorg = 'cp ' + Xorg_nfilepath + ' ' +  Xorg_filepath
rmxorgvnc = 'rm -f ' + Xorg_nfilepath

# invoke the cp command after the test to see if mods are needed

# these flags are inclemented for each option found for the related mod
# 1 for Monitor, 4 for ServerLayout, 3 for Screen and 1 for Module
ServerLayoutModsPresent=0
MonitorModsPresent=0
ScreenModsPresent=0
ModuleModsPresent=0

#
# Simple state machine variables
#
foundServerLayout=0
inServerLayout=0
writtenServerLayoutOption1=0
writtenServerLayoutOption2=0
writtenServerLayoutOption3=0
writtenServerLayoutOption4=0

foundMonitor=0
inMonitor=0
writtenMonitorOption=0

foundScreen=0
inScreen=0
writtenScreenOption1=0
writtenScreenOption2=0
writtenScreenOption3=0

foundModule=0
inModule=0
writtenModuleOption=0

#
# Determine if any of the mods are already present in the xorg.conf file
#

try:
   inFile = open(Xorg_filepath, 'rb')    # open read & binary
   for line in inFile:
      # skip blank or commented '#' lines
      if ((line[0] == "#") and (line == "\n")):
          continue
      # print line

      # convert to lower case for comparisons (lower) and  
      # remove the leading and trailing white characters  (strip)
      lcline=line.lower().strip() 
      # print lcline

      if ( inScreen == 1 ):
         #print 'inScreen'
         #print lcline
         # look for the three addition for VNC in the screen section
         if ((lcline.find('securitytypes') != -1) and (lcline.find('vncauth') != -1)):
             ScreenModsPresent=ScreenModsPresent + 1
         elif (lcline.find('userpasswdverifier') != -1) and (lcline.find('vncauth') != -1):
             ScreenModsPresent=ScreenModsPresent + 1
         elif (lcline.find('passwordfile') != -1) and (lcline.find('/root/.vnc/passwd') != -1):
             ScreenModsPresent=ScreenModsPresent + 1
         #print "ScreenModsPresent: %d" % ScreenModsPresent
            
         if (lcline.startswith('endsection')):
          #  print "ScreenModsPresent: %d" % ScreenModsPresent
          #  print 'Found EndSection'
            # leaving the Screen section
            inScreen=0
      elif ( inModule == 1):
         #print 'inModule'
         #print lcline
         # look for the one addition for VNC in the Module section
         if ((lcline.find('load') != -1) and (lcline.find('vnc') != -1)):
             ModuleModsPresent=1
         if (lcline.startswith('endsection')):
            #print 'Found EndSection'
            # leaving the Module section
            inModule=0
      elif ( inServerLayout == 1):
         # look for the four addition for blanking settings in the serverlayout section
         # print lcline
         if ((lcline.find('blanktime') != -1) and (lcline.find('"0"') != -1) ):
             ServerLayoutModsPresent= ServerLayoutModsPresent + 1
         elif ((lcline.find('standbytime') != -1) and (lcline.find('"0"') != -1) ):
             ServerLayoutModsPresent= ServerLayoutModsPresent + 1
         elif ((lcline.find('suspendtime') != -1) and (lcline.find('"0"') != -1) ):
             ServerLayoutModsPresent= ServerLayoutModsPresent + 1
         elif ((lcline.find('offtime') != -1) and (lcline.find('"0"') != -1) ):
             ServerLayoutModsPresent= ServerLayoutModsPresent + 1
         elif (lcline.startswith('endsection')):
             inServerLayout=0
         #print "ScreenModsPresent: %d" % ScreenModsPresent
      elif ( inMonitor == 1):
         # print lcline
         # look for the change of the DPMS option in the Monitor section
         if ((lcline.find('option') != -1) and (lcline.find('"nodpms"') != -1) ):
             MonitorModsPresent=1
         elif (lcline.startswith('endsection')):
             inMonitor=0
      else:
         # is this a start of a Section?
         if (lcline.startswith('section') ):
            # extract the section name
            (section, title) = lcline.split();
            # print 'Section title: ' + title
            # print "Section is: '%s'" & title
            if ( title.strip('"') == 'screen') :
               #print 'Found Screen'
               # entering the Screen section
               inScreen=1
            elif ( title.strip('"') == 'module'):
               #print 'Found Module'
               # entering the Module section
               inModule=1
            elif ( title.strip('"') == 'serverlayout'):
               # print 'Found ServerLayout'
               # entering the Module section
               inServerLayout=1
            elif ( title.strip('"') == 'monitor'):
               # print 'Found Monitor'
               inMonitor=1

   inFile.close()
   #print "ScreenModsPresent: %d" % ScreenModsPresent
   #print "ModuleModsPresent: %d" % ModuleModsPresent
   #print "ServerLayoutModsPresent: %d" % ServerLayoutModsPresent
   #print "MonitorModsPresent: %d" % MonitorModsPresent

   if ((ScreenModsPresent > 2) and ( ModuleModsPresent > 0) and 
   (ServerLayoutModsPresent > 3) and (MonitorModsPresent > 0)):
      print 'xorg.conf already modified for VNC'
      sys.exit(0)

except IOError:
   print 'file: ', Xorg_filepath, ' not found.'
   sys.exit(12)


# 
# read the xorg.conf and create a new xorg.conf.vnc
#
try:
   inFile = open(Xorg_filepath, 'rb')    # open read & binary
   outFile = open(Xorg_nfilepath, 'wb')  # open write & binary
   for line in inFile:
      # write line to new xorg file   xorg.conf.vnc
      # outFile.write(line)

      # convert to lower case for comparisons (lower) and  
      # remove the leading and trailing characters  (strip)
      lcline=line.lower().strip() 
      # print lcline

      # skip blank or commented '#' lines
      if ((line[0] == "#") and (line == "\n")):
          outFile.write(line)
          continue

      if ( inScreen == 1 ):
         # print 'inScreen'
         # print lcline
         # if the options are found then overwrite them with ours
         if (lcline.find('securitytypes') != -1):
             outFile.write(screenOption1)
             writtenScreenOption1=1
         elif (lcline.find('userpasswdverifier') != -1):
             outFile.write(screenOption2)
             writtenScreenOption2=1
         elif (lcline.find('passwordfile') != -1):
             outFile.write(screenOption3)
             writtenScreenOption3=1
         elif (lcline.startswith('endsection')):
            # if option not written then write them out now prior to section end
            if (writtenScreenOption1 != 1):
               outFile.write(screenOption1)
            if (writtenScreenOption2 != 1):
               outFile.write(screenOption2)
            if (writtenScreenOption3 != 1):
               outFile.write(screenOption3)
            outFile.write(line)
            # print 'Found EndSection'
            # leaving the Screen section
            inScreen=0
         else:
            outFile.write(line)
      elif ( inModule == 1):
         # print 'inModule'
         # if the load vnc is found then mark it there
         if ((lcline.find('load') != -1) and (lcline.find('vnc') != -1)):
             ModuleModsPresent=1
         if (lcline.startswith('endsection')):
            if ( ModuleModsPresent != 1): # if not present write it out
                outFile.write(moduleLoad)
            outFile.write(line)
            # print 'Found EndSection'
            # leaving the Module section
            inModule=0
         else:
            outFile.write(line)
      elif ( inServerLayout == 1):
         # overwrite any entry just to be sure its correct
         if ((lcline.find('blanktime') != -1)):   
             outFile.write(ServerLayoutOption1)
             writtenServerLayoutOption1=1
         elif ((lcline.find('standbytime') != -1)):
             outFile.write(ServerLayoutOption2)
             writtenServerLayoutOption2=1
         elif ((lcline.find('suspendtime') != -1)):
             outFile.write(ServerLayoutOption3)
             writtenServerLayoutOption3=1
         elif ((lcline.find('offtime') != -1)):
             outFile.write(ServerLayoutOption4)
             writtenServerLayoutOption4=1
         elif (lcline.startswith('endsection')):
            # if the option was not written then write them out now, 
            #   prior to section end
            if (writtenServerLayoutOption1 != 1):
               outFile.write(ServerLayoutOption1)
            if (writtenServerLayoutOption2 != 1):
               outFile.write(ServerLayoutOption2)
            if (writtenServerLayoutOption3 != 1):
               outFile.write(ServerLayoutOption3)
            if (writtenServerLayoutOption4 != 1):
               outFile.write(ServerLayoutOption4)
            outFile.write(line)
            inServerLayout=0
         else:
            outFile.write(line)
            
      elif ( inMonitor == 1):
         # if dpms option is present then overwrite it with our nodpms option
         if ((lcline.find('option') != -1) and (lcline.find('"dpms"') != -1) ):
            outFile.write(MonitorOption)
            writtenMonitorOption=1    # note that we already written the option out
         elif ((lcline.find('option') != -1) and (lcline.find('"nodpms"') != -1) ):
            outFile.write(line)       # write it out
            writtenMonitorOption=1    # note that its already there
         elif (lcline.startswith('endsection')):
            # if end of section and haven't written option out, Do so
            if ( writtenMonitorOption != 1):   
               outFile.write(MonitorOption)
            outFile.write(line)
            inMonitor=0
         else:
            outFile.write(line)
      else:
         outFile.write(line)
         # is this a start of a Section?
         if (lcline.startswith('section') ):
            # extract the section name
            (section, title) = lcline.split();
            # print 'Section title: ' + title
            # print "Section is: '%s'" & title
            if ( title.strip('"') == 'screen') :
               # print 'Found Screen'
               # entering the Screen section
               inScreen=1
               foundScreen=1
            elif ( title.strip('"') == 'module'):
               # print 'Found Module'
               # entering the Module section
               inModule=1
               foundModule=1
               # outFile.write(moduleLoad)
            elif ( title.strip('"') == 'serverlayout'):
               # entering the ServerLayout Section
               # print 'Found ServerLayout'
               foundServerLayout=1
               inServerLayout=1
            elif ( title.strip('"') == 'monitor'):
               # print 'Found Monitor'
               foundMonitor=1
               inMonitor=1


   inFile.close()  # close the orginial xorg file

   #
   # If a Screen section was never found then create it now
   #
   if (foundScreen == 0):
      outFile.write(sectionScreen)
      outFile.write(screenOption1)
      outFile.write(screenOption2)
      outFile.write(screenOption3)
      outFile.write(endSection)

   #
   # If a Module section was never found then create it now
   #
   if (foundModule == 0):
      outFile.write(sectionModule)
      outFile.write(moduleLoad)
      outFile.write(endSection)

   #
   # If a ServerLayout section was never found then create it now
   #
   if (foundServerLayout == 0):
      outFile.write(sectionServerLayout)
      outFile.write(ServerLayoutOption1)
      outFile.write(ServerLayoutOption2)
      outFile.write(ServerLayoutOption3)
      outFile.write(ServerLayoutOption4)
      outFile.write(endSection)

   #
   # If a Monitor section was never found then create it now
   #
   if (foundMonitor == 0):
      outFile.write(sectionMonitor)
      outFile.write(MonitorOption)
      outFile.write(endSection)


   outFile.close()    # close the new xrog.conf file

except IOError:
   print 'IOError: ', Xorg_filepath, " or ", Xorg_nfilepath,' ' 
   sys.exit(13)


# make the back up copy now
cpproc = subprocess.Popen(cp2bkup, shell=True)
cppid,status = os.waitpid(cpproc.pid, 0)
#print status
# succesful copy returns 0 as a status
if ( status != 0 ) :
   print 'Cound not copy: ', Xorg_filepath, ' to backup: ' + Xorg_bkup_filepath
   print 'Abort changing xorg.conf'
   sys.exit(14)


# copy modified xorg file on top of the xorg.conf file 
cpproc = subprocess.Popen(cp2xorg, shell=True)
cppid,status = os.waitpid(cpproc.pid, 0)
#print status
# succesful copy returns 0 as a status
if ( status != 0 ) :
   print 'Cound not copy: ', Xorg_nfilepath, ' to ' , Xorg_filepath
   sys.exit(15)

#
# remove the temporary xorg.conf.vnc file
cpproc = subprocess.Popen(rmxorgvnc, shell=True)
cppid,status = os.waitpid(cpproc.pid, 0)
# no need to check status

sys.exit(1)  # indicate that the Xsession will need to be restarted.

#
# for RHEL 5.X execute gdm-restart or gdm-safe-restart to restart the gnome display manager
# and Xsession to enable changes to xorg.conf
# keystroke Cntrl-Alt-Backspace also restart Xsession
#
