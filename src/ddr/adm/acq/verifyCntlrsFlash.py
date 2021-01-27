# -*- coding: utf-8 -*-
from __future__ import print_function
"""

   This script Verifies the VNMRS Digital Controllers Flash Contents 
   It Compares the files on the controller's flash to those in /Vnmr/acq/download 
   If it finds mismatches , it downdloads the files from /vnmr/acq/download to
   the controller's flash. Unless the -y option is given, permission to update the files is requested.
   This script use the Linux rsh facility to communicate with the controllers.

   Notes: 

      Before running this script.

           Besure the Acquisition processes are stopped
           i.e. acqcomm stop

           Close all remote logins to any controllers
   
      Reflashing can take considerable amount of time, especially for nddslib, be patient!

      The VxWorks image can be reflashed (if one deletes it) as long as the controller is still running
      and has not been reset/rebooted.

      If for any reason the script throws an error, please rerun the script with the additional '-d' option
      the debug log file created can be used to determine the cause of problem.


   Usage: verifyCntlrsFlash.py [options]  [specific controller(s) to check ]

      e.g. verifyCntlrsFlash.py       - this will check all discovered controllers in the Console

           verifyCntlrsFlash.py ddr1  - this will check just the ddr1 controller

           verifyCntlrsFlash.py rf1,rf2,ddr1  - this will check just the rf1 rf2 ddr1  controllers
        or
           verifyCntlrsFlash.py rf1 rf2 ddr1  - this will check just the rf1 rf2 ddr1  controllers
   
"""
__author__ = "Greg Brissey"
__version__ = "$Revision: 1.0 $"
__date__ = "$Date: 2012/05/22 $"
__license__ = "Python"



import sys, os, glob, datetime, subprocess 
import signal
import traceback
import datetime
import collections

import logging
from optparse import OptionParser

# for Progress Bar
import time, threading

#
# ====================================================================================
# terminal control Module 
# Copyright: 2009 Nadia Alramli
# License: BSD
# Original Source can be found @ http://nadiana.com/animated-terminal-progress-bar-in-python
# ====================================================================================
#

#print(__name__)
MODULE = sys.modules[__name__]

#print(MODULE)

COLORS = "BLUE GREEN CYAN RED MAGENTA YELLOW WHITE BLACK".split()
# List of terminal controls, you can add more to the list.
CONTROLS = {
    'BOL':'cr', 'UP':'cuu1', 'DOWN':'cud1', 'LEFT':'cub1', 'RIGHT':'cuf1',
    'CLEAR_SCREEN':'clear', 'CLEAR_EOL':'el', 'CLEAR_BOL':'el1',
    'CLEAR_EOS':'ed', 'BOLD':'bold', 'BLINK':'blink', 'DIM':'dim',
    'REVERSE':'rev', 'UNDERLINE':'smul', 'NORMAL':'sgr0',
    'HIDE_CURSOR':'cinvis', 'SHOW_CURSOR':'cnorm'
}

# List of numeric capabilities
VALUES = {
    'COLUMNS':'cols', # Width of the terminal (None for unknown)
    'LINES':'lines',  # Height of the terminal (None for unknown)
    'MAX_COLORS': 'colors',
}

def default():
    """Set the default attribute values"""
    for color in COLORS:
        setattr(MODULE, color, '')
        setattr(MODULE, 'BG_%s' % color, '')
    for control in CONTROLS:
        setattr(MODULE, control, '')
    for value in VALUES:
        setattr(MODULE, value, None)

def setup():
    """Set the terminal control strings"""
    # Initializing the terminal
    curses.setupterm()
    # Get the color escape sequence template or '' if not supported
    # setab and setaf are for ANSI escape sequences
    bgColorSeq = curses.tigetstr('setab') or curses.tigetstr('setb') or ''
    fgColorSeq = curses.tigetstr('setaf') or curses.tigetstr('setf') or ''

    for color in COLORS:
        # Get the color index from curses
        colorIndex = getattr(curses, 'COLOR_%s' % color)
        # Set the color escape sequence after filling the template with index
        setattr(MODULE, color, curses.tparm(fgColorSeq, colorIndex))
        # Set background escape sequence
        setattr(
            MODULE, 'BG_%s' % color, curses.tparm(bgColorSeq, colorIndex)
        )
    for control in CONTROLS:
        # Set the control escape sequence
        setattr(MODULE, control, curses.tigetstr(CONTROLS[control]) or '')
    for value in VALUES:
        # Set terminal related values
        setattr(MODULE, value, curses.tigetnum(VALUES[value]))

def render(text):
    """Helper function to apply controls easily
    Example:
    apply("%(GREEN)s%(BOLD)stext%(NORMAL)s") -> a bold green text
    """
    return text % MODULE.__dict__

try:
    import curses
    setup()
except Exception as e:
    # There is a failure; set all attributes to default
    print('Warning: %s' % e)
    default()



#
# ====================================================================================
# Animated Progress bar
# Copyright: 2009 Nadia Alramli
# License: BSD
# Original Source can be found @ http://nadiana.com/animated-terminal-progress-bar-in-python
# ====================================================================================
#

"""Draws an animated terminal progress bar
Usage:
    p = ProgressBar("blue")
    p.render(percentage, message)
"""

class ProgressBar(object):
    """Terminal progress bar class"""
    TEMPLATE = (
     '%(percent)-2s%% %(color)s%(progress)s%(normal)s%(empty)s %(message)s\n'
    )
    PADDING = 7

    def __init__(self, color=None, width=None, block='█', empty=' '):
        """
        color -- color name (BLUE GREEN CYAN RED MAGENTA YELLOW WHITE BLACK)
        width -- bar width (optinal)
        block -- progress display character (default '█')
        empty -- bar display character (default ' ')
        """
        if color:
            # self.color = getattr(terminal, color.upper())
            self.color = getattr(__main__, color.upper())
        else:
            self.color = ''
        # if width and width < terminal.COLUMNS - self.PADDING:
        if width and width < COLUMNS - self.PADDING:
            self.width = width
        else:
            # Adjust to the width of the terminal
            # self.width = terminal.COLUMNS - self.PADDING
            self.width = COLUMNS - self.PADDING
        self.block = block
        self.empty = empty
        self.progress = None
        self.lines = 0

    def render(self, percent, message = ''):
        """Print the progress bar
        percent -- the progress percentage %
        message -- message string (optional)
        """
        inline_msg_len = 0
        if message:
            # The length of the first line in the message
            inline_msg_len = len(message.splitlines()[0])
        #if inline_msg_len + self.width + self.PADDING > terminal.COLUMNS:
        if inline_msg_len + self.width + self.PADDING > COLUMNS:
            # The message is too long to fit in one line.
            # Adjust the bar width to fit.
            # bar_width = terminal.COLUMNS - inline_msg_len -self.PADDING
            bar_width = COLUMNS - inline_msg_len -self.PADDING
        else:
            bar_width = self.width

        # Check if render is called for the first time
        if self.progress != None:
            self.clear()
        self.progress = (bar_width * percent) / 100
        data = self.TEMPLATE % {
            'percent': percent,
            'color': self.color,
            'progress': self.block * self.progress,
            'normal': NORMAL,
            'empty': self.empty * (bar_width - self.progress),
            'message': message
        }
        sys.stdout.write(data)
        sys.stdout.flush()
        # The number of lines printed
        self.lines = len(data.splitlines())

    def clear(self):
        """Clear all printed lines"""
        #sys.stdout.write(
        #    self.lines * (terminal.UP + terminal.BOL + terminal.CLEAR_EOL)
        #)
        sys.stdout.write(
            self.lines * (UP + BOL + CLEAR_EOL)
        )


#
# ====================================================================================
# Thread to handle the animated progress Bar
# ====================================================================================
#
class ProgressThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    def __init__(self, time):
        super(ProgressThread, self).__init__()
        self.pos = 0
        self.time= float(time)
        self._stop = threading.Event()
        threading.Thread.__init__(self)

    def run(self):
        self.p = ProgressBar( width=50, block='▣', empty='□')
        # calc duration interval for 100%
        self.timeincr = self.time / 100.0;
        #print self.time
        #print self.timeincr
        self.cntdwn = self.time
        for i in range(101):
          self.p.render(i, '\nWriting File to Flash, Remaining Time: %4.1f sec' % (self.cntdwn))
          time.sleep(self.timeincr)
          self.cntdwn = self.cntdwn - self.timeincr
          if self.stopped(): 
             self.p.render(100, '\nWriting File to Flash, Remaining Time: %4.1f sec' % (0.0))
             break

    def stop(self):
        self._stop.set()

    def stopped(self):
        return self._stop.isSet()





#
# ====================================================================================
# ====================================================================================
#
# return the index of the 1st occurence of the substring within a list of strings
def first_substring(strings, substring):
    try:
       result = (i for i, string in enumerate(strings) if substring in string).next()
    except Exception as e:    # catch StopIteration when it doesn't find the substring, return -1
        result = -1
    return result


#
# ====================================================================================
# ====================================================================================
#

class BreakHandler:
    """
    Trap CTRL-C, set a flag, and keep going.  This is very useful for
    gracefully exiting database loops while simulating transactions.
 
    To use this, make an instance and then enable it.  You can check
    whether a break was trapped using the trapped property.
 
    # Create and enable a break handler.
    ih = BreakHandler()
    ih.enable()
    for x in big_set:
        complex_operation_1()
        complex_operation_2()
        complex_operation_3()
        # Check whether there was a break.
        if ih.trapped:
            # Stop the loop.
            break
    ih.disable()
    # Back to usual operation...
    """
 
    def __init__(self, emphatic=9):
        '''
        Create a new break handler.
 
        @param emphatic: This is the number of times that the user must
                    press break to *disable* the handler.  If you press
                    break this number of times, the handler is automagically
                    disabled, and one more break will trigger an old
                    style keyboard interrupt.  The default is nine.  This
                    is a Good Idea, since if you happen to lose your
                    connection to the handler you can *still* disable it.
        '''
        self._count = 0
        self._enabled = False
        self._emphatic = emphatic
        self._oldhandler = None
        return
 
    def _reset(self):
        '''
        Reset the trapped status and count.  You should not need to use this
        directly; instead you can disable the handler and then re-enable it.
        This is better, in case someone presses CTRL-C during this operation.
        '''
        self._count = 0
        return
 
    def enable(self):
        '''
        Enable trapping of the break.  This action also resets the
        handler count and trapped properties.
        '''
        if not self._enabled:
            self._reset()
            self._enabled = True
            self._oldhandler = signal.signal(signal.SIGINT, self)
        return
 
    def disable(self):
        '''
        Disable trapping the break.  You can check whether a break
        was trapped using the count and trapped properties.
        '''
        if self._enabled:
            self._enabled = False
            signal.signal(signal.SIGINT, self._oldhandler)
            self._oldhandler = None
        return
 
    def __call__(self, signame, sf):
        '''
        An break just occurred.  Save information about it and keep
        going.
        '''
        self._count += 1
        # If we've exceeded the "emphatic" count disable this handler.
        if self._count >= self._emphatic:
            self.disable()
        return
 
    def __del__(self):
        '''
        Python is reclaiming this object, so make sure we are disabled.
        '''
        self.disable()
        return
 
    @property
    def count(self):
        '''
        The number of breaks trapped.
        '''
        return self._count
 
    @property
    def trapped(self):
        '''
        Whether a break was trapped.
        '''
        return self._count > 0

#
# ====================================================================================
# ====================================================================================
#

#
#   rshCmd Execption class
#
class rshCmdError(Exception):
    "rsh command exception class"
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

#
#    rsh commands class for VNMR Digital Controllers
#
class rshCmd:
    "issue commands via rsh"
    def __init__(self,hostname):
        self.dirdic = {}
        self.dirinfodic = {}
        self.cntlrPrompt='.*> '
        self.nddstype = 'None'
        self.trap = BreakHandler()  # Use this Class to prevent Cntlr-C from interrupting rsh commands
        logger.debug(hostname);
        cmd = 'rsh -l %s %s' % ('vnmr1',hostname)
        logger.debug(cmd)
        self.child = pexpect.spawn('rsh -l %s %s'%('vnmr1',hostname))
        i = self.child.expect([self.cntlrPrompt, pexpect.EOF, pexpect.TIMEOUT])
        logger.debug(self.child.after)
        # print i
        if i == 0: # Login good
            pass
        if i == 1: # EOF
            raise rshCmdError('rsh Login Failed')
        if i == 2: # Timeout
            raise rshCmdError('rsh Login Timeout')

    def __del__(self):
        try: 
           self.outputlinelist = self.sendCmd('logout',1)
        except Exception as e:
           pass

    def sendCmd(self,cmd,cmdtimeout=10):
        self.trap.enable() # prevent Cntlr-C from interrupting rsh commands
        self.child.sendline(cmd);
        # self.child.expect('.*> ',timeout=cmdtimeout)
        result = self.child.expect([self.cntlrPrompt, pexpect.EOF, pexpect.TIMEOUT],timeout=cmdtimeout)
        if result == 0:
           linelist = self.child.after.split('\r\n');
           logger.debug(linelist)
           # There maybe extranious strings in the result, so find the echo of the command given
           # and make that the beginning string in the list
           index = first_substring(linelist,cmd)
           logger.debug(index)
           linelist = linelist[index:]
        else:
           logger.debug("EOF or TIMEOUT occurred.")
           linelist=[]

        self.trap.disable()    # disable the Cntlr-C trap
        return linelist

    @property
    def trapped(self):
        """
        Whether a break was trapped.
        """
        return self.trap._count > 0

    def getCpuId(self):
        """
         Obtain the Unique PPC ID
        """
        outputlinelist = self.sendCmd('get405ECID');
        logger.debug(outputlinelist)
        index = first_substring(outputlinelist, 'value =')
        # print outputlinelist
        logger.debug(outputlinelist[index])
        d1,d2,cpuId = outputlinelist[index].split('=',2)
        return cpuId.strip(' ')

    def isIcatAttached(self):
        """
         Obtain the Icat ID, 0=no icat, 0xa Icat present 
        """
        outputlinelist = self.sendCmd('getIcatId');
        # if ( 'undefined symbol:' not in outputlinelist[1]):
        index = first_substring(outputlinelist, 'undefined symbol:')
        logger.debug('undefined symbol: index ' + str(index))
        if ( index == -1 ):
           # print outputlinelist
           # find the answer, use that index to access value
           index = first_substring(outputlinelist, 'value =')
           d1,d2,icatID = outputlinelist[index].split('=',2)
        else:
           icatID = '0x0'

        if ( icatID.strip(' ') == '0x0'):
           answer =  False
        else:
           answer = True
        return answer

    def getIcatDNA(self):
        """
         Obtain the Icat Unique ID
        """
        outputlinelist = self.sendCmd('prtIcatDNA');
        logger.debug(outputlinelist)
        index = first_substring(outputlinelist, 'DNA:')
        logger.debug('DNA indexed to: ' + str(index))
        # print outputlinelist
        if (index != -1):
           d1,icatDNA = outputlinelist[index].split(':',1)
        else:
           icatDNA="Unknown"
        return icatDNA.strip(' ')


    def getFPGAInfo(self):   # doesn't work
        outputlinelist = self.sendCmd('checkFpgaVersion');
        logger.debug(outputlinelist)
        # print outputlinelist
        return outputlinelist[1]

    def getmd5(self,file):
        """
        Obtain the md5 signature of the flash file given
        """
        outputlinelist = self.sendCmd('ffmd5 "' + file + '"');
        logger.debug(outputlinelist)
        # if ( 'undefined symbol:' not in outputlinelist[1]):
        index = first_substring(outputlinelist, 'undefined symbol:')
        logger.debug('undefined symbol: index ' + str(index))
        if ( index == -1 ):
           # if ('not found' not in outputlinelist[1]):
           index = first_substring(outputlinelist, 'not found')
           logger.debug('not found: index ' + str(index))
           if ( index == -1 ): 
              md5index = first_substring(outputlinelist, 'MD5:')
              logger.debug('MD5: index ' +  str(md5index))
              logger.debug(outputlinelist[md5index])
              title,ffmd5sig = outputlinelist[md5index].split(' ',1)
           else:
              logger.debug(outputlinelist[index])
              ffmd5sig = outputlinelist[index].strip(' ')
        else:
           ffmd5sig = 'unknown: Controller not fully booted'
        return ffmd5sig


    def getNDDSType(self):
        # 1st try the command for NDDS 3.x 
        outputlinelist = self.sendCmd('prtnddsver')
        logger.debug(outputlinelist)
        # if ( 'undefined symbol:' not in outputlinelist[1]):
        index = first_substring(outputlinelist, 'undefined symbol:')
        if ( index == -1 ):
           result = 'NDDS 4.x'
           self.nddstype='4x'
        else:
           # 2nd try the command for NDDS 4.x 
           outputlinelist = self.sendCmd('NddsVersionGet')
           logger.debug(outputlinelist)
           # if ('undefined symbol:' not in outputlinelist[1]):
           index = first_substring(outputlinelist, 'undefined symbol:')
           if ( index == -1 ):
              result = 'NDDS 3.x'
              self.nddstype='3x'
           else:
              # "NDDS libraries do not appear to be loaded"
              result = "Indeterminate"
              self.nddstype='None'

        return result

    def copy2cntlr(self,file,filesize,cmdtimeout=300):
        # example output of a cp2ffw command
        #cp2ffw "/home/vnmr1/test.txt"
        #Coping file '/home/vnmr1/test.txt' of 44 bytes to Flash file 'test.txt'
        #Copy Successful, CRC match: 0x42eca34a
        #Complete.
        #value = 0 = 0x0

        # approx writting speed of controller flash,  Bytes / Second
        bytesPerSec = 24352.0
        time2write = float(filesize) / bytesPerSec

        #if self.isFileOnCntlr(file) == False:
        #   time2write / 2.0
        self.t = ProgressThread(time2write)
        self.t.start() #  Progress Bar in action while we wait on command to finish 

        outputlinelist = self.sendCmd('cp2ffw "' + file + '"',cmdtimeout);

        self.t.stop()   # stop progress bar if not already completed
        self.t.join()   # wait for thread to terminate
        logger.debug(outputlinelist)

        # confirm successful copy
        # found = outputlinelist[2].find('Successful')
        found = first_substring(outputlinelist,'Successful')
        logger.debug(found)
        if (found != -1):
          status = True
        else:
          status = False
        return status

    def reboot(self):
        outputlinelist = self.sendCmd('reboot', 3);

    def dir(self):
        outputlinelist = self.sendCmd('ffdir');
        logger.debug(outputlinelist)
        # parse list into a dictionary key filename, tuple bytes,md5
        sys.stdout.write("\r\nObtaining Directory Info: ")
        sys.stdout.flush()
        for lval in outputlinelist[1:-6]:
           # print "lval: " + lval
           (  name, size ) = lval.split('\t ',1)
           name = name.strip(' \t')
           size = size.strip(' \t')
           logger.debug('name: "'+name+'"')
           logger.debug('size: "'+size+'"')
           self.dirdic[name] = ( size,'0' )
        #print self.dirdic

        #
        # store the directory info as well, number of filesm size, and free space
        #
        logger.debug(outputlinelist[-5])
        ( name, size ) = outputlinelist[-5].split('\t',1)
        self.dirinfodic['nfiles'] = ( name.strip(' '), size.strip(' \t') )
        logger.debug(outputlinelist[-4])
        ( name, size ) = outputlinelist[-4].split('\t',1)
        self.dirinfodic['freespace'] = ( name.strip(' \t'), size.strip(' \t') )

        #
        # get the md5 signitures for each file
        #
        for file, ( size, md5)  in self.dirdic.items():
           sys.stdout.write(".")
           sys.stdout.flush()
           ffmd5 = self.getmd5(file)
           self.dirdic[file] = ( size, ffmd5.strip("'") )
        sys.stdout.write("\r\n")
        sys.stdout.flush()

        logger.debug(self.dirdic)
        logger.debug(self.dirinfodic)
        return outputlinelist

    def icatdir(self):
        outputlinelist = self.sendCmd('isfdir');
        logger.debug(outputlinelist)
        return outputlinelist[1:-2]


    def getNumberUsedAndFreeSpace(self):
        """Returns a tuple of three, number of files, total bytesused, free space"""
        ( number, tsize ) = self.dirinfodic['nfiles']
        ( dum, fsize ) = self.dirinfodic['freespace']
        return ( number, tsize, fsize )

    def cmpmd5(self,filename,md5):
        """Compares the filename and md5, against the controllers ffdir listing dictionary"""
        try:
           ( size, ffmd5 ) = self.dirdic[filename]
           if ( md5 == ffmd5):
              result = True
           else:
              result = False
        except Exception as e:
           result = False;

        return result

    def isFileOnHost(self,files):
       """The list of files given is compared against the file on the controller's flash
          a list of files on the flash but not on the host is returned."""
       notfoundlist=[]
       for filename in self.dirdic.keys():
         if filename not in files:
            notfoundlist.append(filename)
       return notfoundlist

    def isFileOnCntlr(self,filename):
       """The list of files given is compared against the file on the controller's flash
          a list of files on the flash but not on the host is returned."""
       filelist = self.dirdic.keys()
       return filename in filelist


      # The fail files pri_imagecp.fail and/or sec_imagecp.fail are created if the copy operation fails 
      #  on the primary and/or secondary iCAT FPGA image respectively.
      #
      # The failure file sec_imageld.fail is created on the RF FFS if the reboot of the secondary image fails.
      #
      # The fail file icat_config.fail is created if the FORTH interpreter returns an error code.
      #

    #def getFreeSpace(self):
    #    ( number, size ) = self.dirinfodic['freespace']
    #    return size
    #    return ( number, size )
#
# ====================================================================================
#
 
def exit_with_usage():

    # print globals()['__doc__']
    parser.print_help()
    os._exit(1)

#
#  Calc the MD5 signiture for file given
#  Return the calc MD5 Sig.
#
def md5sig(filepath):
    md5cmd = 'md5sum ' + filepath
    md5proc = subprocess.Popen(md5cmd, shell=True, stdout=subprocess.PIPE)
    status = os.waitpid(md5proc.pid, 0)
    md5sig,file = md5proc.stdout.read().split(' ',1)
    #logger.debug(md5sig)
    return md5sig

#
#  Calc the MD5 signiture for file given
#  Return the calc MD5 Sig.
#
def getfilesize(filepath):
    """-rwxrwxrwx 1 greg greg 1302 2012-08-30 14:44 tst.py"""
    ducmd = 'ls -l ' + filepath
    md5proc = subprocess.Popen(ducmd, shell=True, stdout=subprocess.PIPE)
    status = os.waitpid(md5proc.pid, 0)
    # line = md5proc.stdout.read();
    perms,lnks,user,grp,size,stuff = md5proc.stdout.read().split(' ',5)
    return size

#    ducmd = 'du -b ' + filepath
 #   md5proc = subprocess.Popen(ducmd, shell=True, stdout=subprocess.PIPE)
  #  status = os.waitpid(md5proc.pid, 0)
   # size = md5proc.stdout.read()
    #size,file = md5proc.stdout.read().split('\t',1)
    # print size
    #return size

#
#
# pingIP: ping the hostname or IP given, returns True if response received otherwise False
#
def pingIP(ip):
        """ pingIP: ping the hostname or IP given, returns True if response received otherwise False"""
        # ping once , timeout in one second if no responce
        pingTest = "ping -c 1 -W 1 " + ip
        # print pingTest
        process = subprocess.Popen(pingTest, shell=True, stdout=subprocess.PIPE)
        process.wait()
        returnCodeTotal = process.returncode
        # print returnCodeTotal
        return (returnCodeTotal == 0)

def getEthersList():
    """getEthersList - Obtain the Controllers Names from the /etc/ethers files"""
    hostlist = []
    file=os.path.join(os.path.sep,'etc','ethers')
    # print file
    if ( os.path.exists(file) == False ) :
       logger.debug("Ethers file: " + file + " - Not present.")
       return hostlist
   
    inFile = open(file, 'rb')
    for line in inFile:
           # print line
           # logger.debug(line)
           # skip blank or commented '#' lines
           if ((line[0] != "#") and (line != "\n")):
              #
              # grab the MAC filepath from line
              #
              (mac, cntlrname) = line.split(' ',1)
              mac = mac.strip()
              # logger.debug(mac)
              cntlrname = cntlrname.strip(' \n')
              #print cntlrname
              # logger.debug(cntlrname)
              hostlist.append(cntlrname);
    return hostlist

def md5DownloadList():
   """md5DownloadList: create a listing of the downloadable content in the VnmrJ /vnmr/acq/download directory
                       This list contain the filename and md5 signature"""
   #dwnldlist = ( glob.glob('/vnmr/acq/download/*.o') + glob.glob('/vnmr/acq/download/*.bit') + 
   #              glob.glob('/vnmr/acq/download/*.4th') + glob.glob('/vnmr/acq/download/*.bdx') + 
   #              ['/vnmr/acq/download/nvScript'] )
   dwnldlist = ( glob.glob(options.dwnldpath+'/*.o') + glob.glob(options.dwnldpath+'/*.bit') + 
                 glob.glob(options.dwnldpath+'/*.4th') + glob.glob(options.dwnldpath+'/*.bdx') + 
                 [options.dwnldpath+'/nvScript'] )
   # print dwnldlist
   logger.debug(dwnldlist);
   dwnlddic = { }
   for file in dwnldlist:
       # md5 = md5sig("/vnmr/acq/download/ddrexec.o")
       md5 = md5sig(file)
       size = getfilesize(file)
       #
       # filename stripped of directory path
       #
       (path , filename) = os.path.split(file);
       #path = path.strip()
       #print path
       filename = filename.strip(' \n')
       dwnlddic[filename] = ( size, md5 )
       # print file + " : " + filename + ': ' + md5
       # logger.debug(file + " : " + filename + ': ' + md5);
       logger.debug(file + " :\t" + md5);
   return dwnlddic


#
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
#----------------------------------------------------------------------
#
def main():
    hostname = []
    #
    # Obatin the ist of possible VNMRS Digital Controllers in this syste,
    #
    hostname = getEthersList()
    if len(hostname) == 0:
       logger.critical("No Ethers file on this system, Script Aborted")
       os._exit(1)

    logger.debug('number of hostnames ' + str(len(hostname)) + ' : '  + ', '.join(hostname))

    md5sums = {}
    md5sums = md5DownloadList()

    #
    # if not specific controllers given to test, then
    # ping the list of controllers obtain from the ether file, and determine
    # which are present via ping
    #
    if ( len(targets) == 0):
       activelist = []
       sys.stdout.write("Determining Active Controllers, Working: ")
       sys.stdout.flush()
       for host in hostname:
           if ( pingIP(host) == True ):
              activelist.append(host)
           sys.stdout.write(".")
           sys.stdout.flush()

       sys.stdout.write("\r\n")
       sys.stdout.flush()
       logger.info(' ')
       logger.info('Controllers Found: ' + ', '.join(activelist))
    else:
       activelist = targets 

    
    # unfortantely the collection method is not present for python 2.4 which is on RHEL 5.3 which we must support.
    #   nddstypedic = collections.defaultdict(list) # uses an initializer function for missing keys, use list() for our case
    #   rftypedic = collections.defaultdict(list) # uses an initializer function for missing keys, use list() for our case
    nddstypedic = {}
    rftypedic = {}
    for cntlr in activelist:
       try:
          # attempt to rlogin into the controller
          cntlrRsh = rshCmd(cntlr)
       except rshCmdError as e:
          # print 'rshCmd exception occurred, value:', e.value
          logger.info("rsh to Controller %s failed,  Check for an open rsh session to controller." % (cntlr))
          logger.critical(e.value)
          os._exit(1)

       nddstype = cntlrRsh.getNDDSType()
       # nddstypedic[nddstype].append(cntlr)   # see above initialazation that make this work properly
       nddstypedic.setdefault(nddstype,[]).append(cntlr)

       logger.debug(nddstypedic)

       cpuId = cntlrRsh.getCpuId()
       #fpgaInfo = cntlrRsh.getFPGAInfo()
       #print fpgaInfo
       #os._exit(1)
       
       logger.info(' ')
       logger.info('------------------------------------------------------------------ ')
       logger.info('------------------------------------------------------------------ ')
       logger.info(' ')
       logger.info('Digital Controller: %s, Unique ID: %s,    NDDS Ver: %s' %(cntlr,cpuId, nddstype))

       icatPresent = False
       if ( 'rf' in cntlr ):
          icatPresent = cntlrRsh.isIcatAttached()
          if ( icatPresent ):
             # rftypedic['ICAT RF'].append(cntlr)    not in python 2.4  that's on RHEL 5.3 or earlier 
             rftypedic.setdefault('ICAT RF',[]).append(cntlr)
             icatDNA = cntlrRsh.getIcatDNA()
             # print icatDNA
             isflist = cntlrRsh.icatdir()
             #print isflist
          else:
             #rftypedic['VNMRS RF'].append(cntlr) not in python 2.4  that's on RHEL 5.3 or earlier
             rftypedic.setdefault('VNMRS RF',[]).append(cntlr)


       #
       # obtain the FFS listing of the controller with md5, this takes a bit of time
       #
       dirlist = cntlrRsh.dir()
       # logger.info('\r\n'.join(dirlist[1:-2]))

       if ( icatPresent ):
          logger.info(" ")
          logger.info("     iCAT RF attached to this RF Controller,  iCAT DNA: %s" % icatDNA)
          logger.info(" ")
          logger.info(" ")
          logger.info("      " + isflist[0])
          for line in isflist[1:-2]:
                ( file, size , md5 ) = line.split('\t',2)
                logger.info("%25s  %20s    %24s" %(file.strip('\t '),size.strip('\t '),md5.strip('\t ')))
          logger.info("      " + isflist[-1])
          logger.info(' ')
          logger.info(' ')
  
       #
       # print directory listing
       #
       ( nfiles, fsize, tsize ) = cntlrRsh.getNumberUsedAndFreeSpace()
       logger.info(" ")
       logger.info("Controller Directory listing: %11s  %20s,   %20s" % (nfiles, fsize, tsize))
       logger.info(" ")
       for file, ( size, ffmd5) in cntlrRsh.dirdic.items():
         logger.info("%25s  %15s    md5: %24s" %(file,size,ffmd5))
       logger.info(' ')
       logger.info(' ')

       
       notfoundlist = [ ]
       notfoundlist = cntlrRsh.isFileOnHost(md5sums.keys())
       notfoundlist.remove('boot.ini')   # remove the Visionware bootloader ini file, it's always present
       if ( len(notfoundlist) > 0 ):
          logger.info("Non-Release files found on Controller: %s" % ', '.join(notfoundlist))
          logger.info(' ')

       #
       # compare md5s if mismatch then download files from host to controller's flash
       #
       files2update= []
       filesup2date= []
       for file, (size,md5) in md5sums.items():
           logger.debug('Cntlr: %s, file: %s,  md5: %s' % (cntlr,file,md5))
           result = cntlrRsh.cmpmd5(file,md5)
           if (result == False):
              files2update.append(file)
           else:
              filesup2date.append(file)
              #logger.info('File: ' + file + ' is up to date')

       if ( len(filesup2date) > 0):
          logger.info('Files Not Requiring Updating: ')
          logger.info(" ")
          for file  in filesup2date:
              logger.info("%30s" % (file))
       else:
          logger.info('Files Not Requiring Updating:   None')
          logger.info(" ")
      

       if ( len(files2update) > 0):
          logger.info(" ")
          logger.info('Files Requiring Updating: ')
          logger.info(" ")
          for file  in files2update:
              logger.info("%30s" % (file))
       else:
          logger.info(" ")
          logger.info('Files Requiring Updating:   None')
          logger.info(" ")

       # only ask to update if there are files to update
       if ( (len(files2update) > 0) and (options.noupdateflag == False) ):
          if (options.autoupdateflag == True):
             update = True
          else:
             answer = raw_input('\r\nUpdate the files? (y/n): ')
             # print answer
             if ( answer in [ 'y', 'yes', 'Y', 'Yes','YES' ] ):
                update = True
             else:
                update = False
       else:
          update = False


       #
       ########################################################################################
       #    File Transfer Section
       ########################################################################################
       #
       # Update or Not?
       #
       if (update == True):
          failures=False
          #
          # copy the files from the host to the controller for the files that need to be updated
          #
          for file in files2update:
              filepath = os.path.join(options.dwnldpath,file)
              logger.debug("download host file: " + filepath)
              logger.info("\r\nUpdate: " + file)
              (filesize,md5s) = md5sums[file]
              result = cntlrRsh.copy2cntlr(filepath,filesize)
              if ( result == False ):
                  failures = True
                  logger.critical("        Failed to copy '%s' to Controller" % (filepath))

              # Cntlr-C pressed during transfer?
              if cntlrRsh.trapped:
                 logger.info("Control-C was pressed during transfer, do you wish to Abort?")
                 answer = raw_input('\r\nAbort? (y/n): ')
                 # print answer
                 if ( answer not in [ 'n', 'N', 'No', 'NO' ] ):
                    logger.info("\r\nAborting further transfers.")
                    return 


          if ( failures == False):
              logger.info("\r\nRebooting Controller: " + cntlr)
              cntlrRsh.reboot()    # reboot controller ?
           # Or wait till complete and reboot entire console  reboot 1 to master1

    if ( len(nddstypedic.keys()) > 1):
       logger.info(" ")
       logger.info("W A R N I N G !, Incompatible NDDS Versions detected.")
       logger.info(" ")
       for key in nddstypedic:
          logger.info("%27s: %s" % (key, ', '.join(nddstypedic[key])))

    if ( len(rftypedic.keys()) > 1):
       logger.info(" ")
       logger.info("W A R N I N G !, Multiple RFs detected.")
       logger.info(" ")
       for key in rftypedic:
          logger.info("%27s: %s" % (key, ', '.join(rftypedic[key])))


#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
#
# traverse a list of lists, etc. 
#
def traverse(o, tree_types=(list, tuple)):
    if isinstance(o, tree_types):
        for value in o:
            for subvalue in traverse(value):
                yield subvalue
    else:
        yield o

import pexpect


#
#  Entry point of Script
#

if __name__ == "__main__":

    # print "\n".join(sys.argv)

    dateTime = datetime.datetime.today()
    datetimestr = dateTime.strftime("%Y-%m-%d:%H:%M:%S")

    parser = OptionParser(usage=__doc__)
    parser.add_option("-f", "--file", dest="loggingFilename",
                  help="Logging base file name,  a timestamp.log with be appended to name e.g. '_2012-05-30:21:34:13.log'", metavar="FileName")
    parser.add_option("-l", "--location", action="store", dest="dwnldpath", default="/vnmr/acq/download", metavar="DirectoryPath",
                  help="location/directory path of controller's files on host (default: /vnmr/acq/download)") 
    parser.add_option("-d", "--debug",
                  action="store_true", dest="debugOutput", default=False,
                  help="Write debugging output to debug log file, e.g. 'verifyCntlrsFlash_Debug_2012-05-30:21:34:13.log'")
    parser.add_option("-y", "--autoupdate",
                  action="store_true", dest="autoupdateflag", default=False,
                  help="Automaticly update files that need to be, without confirmation")
    parser.add_option("-n", "--noupdates",
                  action="store_true", dest="noupdateflag", default=False,
                  help="Do not update flash file and do not ask to update files. Useful if you want just a log of the present state of the controllers.")



    (options, args) = parser.parse_args()

    # print(args)

    # arglist is going to be a lists of lists
    arglist = []
    for argitem in args:
        arglist.append(argitem.split(','))


    # travarse the list of lists creating a simple list of all targeted controller
    targets = []
    for cntlr in traverse(arglist):
       targets.append(cntlr)
    #print(targets)

    #print options    # acces via the options.optionname
    #print args       # accces as an array index, args[0] - 1st arg not name of script
    #print options.loggingFilename
    #print args[0]

    #formatter = logging.Formatter('%(asctime)-6s: %(name)s - %(levelname)s - %(message)s')
    #formatter = logging.Formatter(
    #   '[%(asctime)s] p%(process)s {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s','%m-%d %H:%M:%S')
    # console_formatter = logging.Formatter('%(asctime)-6s: %(message)s','%m-%d %H:%M:%S')

    console_formatter = logging.Formatter('%(message)s')
    consolelog_formatter = logging.Formatter('%(asctime)-6s: %(message)s','%m-%d %H:%M:%S')
    debug_formatter = logging.Formatter(
       '[%(asctime)s] {%(filename)s:%(lineno)d:%(funcName)s} %(levelname)s - %(message)s','%m-%d %H:%M:%S')

    consoleLogger = logging.StreamHandler()
    consoleLogger.setLevel(logging.INFO)
    consoleLogger.setFormatter(console_formatter)
    logging.getLogger('').addHandler(consoleLogger)

    if ( options.loggingFilename == None):
        loggingFilename = 'verifyCntlrsFlash_' + datetimestr + '.log'
    else:
        loggingFilename = options.loggingFilename + '_' + datetimestr + '.log'

    fileLogger = logging.FileHandler(filename=loggingFilename)
    fileLogger.setLevel(logging.INFO)
    fileLogger.setFormatter(consolelog_formatter)
    logging.getLogger('').addHandler(fileLogger)

    if (options.debugOutput):
       debugLogger = logging.FileHandler(filename='verifyCntlrsFlash_Debug_' + datetimestr + '.log')
       debugLogger.setLevel(logging.DEBUG)
       debugLogger.setFormatter(debug_formatter)
       logging.getLogger('').addHandler(debugLogger)

    logger = logging.getLogger('verifyCntlrsFlash logger')
    logger.setLevel(logging.DEBUG)
  
    logger.info(" ")
    logger.info(" ------ " + datetimestr + " ------- ")
    logger.info(" ")
    logger.info('Log file: "%s"' % (loggingFilename))
    logger.info(" ")
    if (options.debugOutput):
       logger.info(" ")
       logger.info('Debug Log file: "%s"' % ('verifyCntlrsFlash_Debug-' + datetimestr + '.log'))
       logger.info(" ")

    try:
        main()
    except Exception as e:
        print(str(e))
        traceback.print_exc()
        os._exit(1)

    logger.info(" ")
    logger.info(" ------ Completed: " + datetimestr + " ------- ")
    logger.info(" ")


