import os
import signal
import threading
import subprocess
import re
import traceback
import datetime

from ExpStat import *

"""
TODO: use pyinotify (a Python interface to the Linux inotify library), available
      from github.com/seb-m/pyinotify to get asynchronous notification rather than
      relying on polling.

"""
from abc import ABCMeta, abstractmethod # abstract base class


class Checker(object):
    __metaclass__ = ABCMeta

    def __init__(self):
        self.shutdown = threading.Event()
        self.items = {'All':[]} # sub-classes expand with self.items.update{'key1':[], 'key2':[]}
        
    @abstractmethod
    def check(self):
        return []

    def listen(self, event):
        """ add a listener for the monitored object """
        self.items['All'].append(event)

    def notify(self, change=None):
        """ notify subscribers of changes """
        for lock in self.items['All']:
            lock.set()
        
    def follow(self,f,t=1):
        f.seek(0,2)  # end-of-file
        try:
            while not self.shutdown.isSet():
                line = f.readline()
                if not line:
                    time.sleep(t)
                    continue
                yield line
            print "shutting down %s follower"% str(f.fileno())
        except GeneratorExit:
            print "terminating %s follower"% str(f.fileno())

class Heartbeat(Checker):
    def __init__(self, secs):
        super(Heartbeat, self).__init__()
        self._timestamp = time.time()
        self._secs = secs
        
    def check(self):
        return self._timestamp
    
    def follow(self):
        while not self.shutdown.isSet():
            time.sleep(self._secs)
            self._timestamp = time.time()
            yield ('heartbeat',self._timestamp)
        
class ExpStatChecker(Checker):
    DefaultPath = '/tmp'
    def __init__(self,path=None):
        if not path: path = ExpStatChecker.DefaultPath
        super(ExpStatChecker,self).__init__()
        self.expstat = ExpStat(path=path)
        
    def check(self,t=1):
        return self.expstat.changes(remember=True)

    def follow(self,t=1):
        while not self.shutdown.isSet():
            try:                
                changes = self.expstat.changes(remember=True)
                if len(changes):
                    yield changes
                time.sleep(t)
            except IOError:
                logging.error("IO Error: memory map file %s not accessible?", (self.expstat.expstat_path))
                
class AcqMsgChecker(Checker):
    DefaultPath = '/vnmr/acqqueue'
    OPEN = "open"
    CLOSED = "closed"
    CHANGED = "changed"
    UNCHANGED = None
    
    def __init__(self,path=None):
        super(AcqMsgChecker,self).__init__()
        if not path: path = AcqMsgChecker.DefaultPath
        self.acqmsg_path = path + '/acqmsg'
        self.state = AcqMsgChecker.CLOSED
        self.transition = None
        self.value = ''
        self.last = None
        logging.debug("watching %s\n" % self.acqmsg_path)

    def follow(self,t=1):
        while not self.shutdown.isSet():
            try:
                f = open(self.acqmsg_path,'r')
                if self.state != AcqMsgChecker.OPEN:
                    logging.debug("%s -> OPEN \n" % self.state)
                    self.transition = AcqMsgChecker.CHANGED
                    self.state = AcqMsgChecker.OPEN
                self.value = f.readlines()
                if self.value != self.last:
                    self.transition = AcqMsgChecker.CHANGED
                    self.last = self.value
                    logging.debug(" => CHANGE %s\n" % self.value)

            except IOError:
                if self.state == AcqMsgChecker.OPEN:
                    self.transition = AcqMsgChecker.CHANGED
                    self.state = AcqMsgChecker.CLOSED
                    self.value = 'clear'
                    self.last = self.value
                    logging.debug(" => CHANGE %s\n" % 'clear')

            if self.transition == AcqMsgChecker.CHANGED:
                self.transition = AcqMsgChecker.UNCHANGED
                yield ('acqmsg',self.value)
            
            time.sleep(t)

    def check(self):
        return self.value
        
class CryoChecker(Checker):
    DefaultPath = '/vnmr'
    def __init__(self,path=None):
        super(CryoChecker,self).__init__()
        self._cache = self.default()
        self._timestamp = time.time()
        if not path: path = CryoChecker.DefaultPath
        self.prefs_path = path + '/cryo/cryomon/cryomon.prefs'
        self.data_path = path + '/cryo/cryomon/cryomonData.txt'
        self.version_path = path + '/cryo/cryomon/cryomon.version'
        self.lock_path = path + '/cryo/cryomon/cryomon.lock'
        self.prefs = {}
        self.get_prefs()
        self.items.update(
            {'LevelN2':{}, 'LevelHe':{}, 'Timestamp':{}, 'Status':{}}
            )
        
    def get_prefs(self):
        try:
            with open(self.prefs_path) as prefs:
                for line in prefs:
                    key, value = line.split()
                    self.prefs[key] = float(value)
        except IOError as e:
            logging.warning("{0}: I/O error({1}): {2}".format(self.prefs_path, e.errno, e.strerror))

    def follow(self):
        try:
            line = self.last(1)
            if len(line):
                self._timestamp = time.time()
                self._cache = line[0].strip('\n')
                yield ('cryomon', self._cache)
            self.data_file = open(self.data_path,'r')
            lines = super(CryoChecker, self).follow(self.data_file)
            for line in lines:
                if self.shutdown.isSet():
                    lines.close()
                self._timestamp = time.time()
                self._cache = line.strip('\n')
                yield ('cryomon', self._cache)
        except IOError:
            self.data_file = None

    @staticmethod
    def default():
        return "00:00:00:00:00,000,000,000000"

    def latest(self):
        return self._cache, self._timestamp

    def last(self,count=1,estimated_line_length=30):
        f = open(self.data_path,'r')
        f.seek(0,0)
        sample = f.readline()
        sample_size = max(len(sample),estimated_line_length)+5 # should be 30 or so, plus some slack for cryomon

        f.seek(0,2)
        file_size = f.tell()
        offset = min(file_size,sample_size * (count+1))
        f.seek(-offset,2)

        lines = f.readlines()
        line_count = len(lines)
        if lines:
            # clean up any raggedy schmutz at the front
            fields=lines[0].split(',')
            if len(fields) > 1 and fields[0].strip() == 'DATE' or len(fields) < 4:
                line_count -= 1
        count = min(count, line_count)
        f.close()
        if not count:
            return []
        return lines[-count:]
        
    def check(self, last=1):
        # get log entry of form YY:MM:DD:HH:mm, % He Level, % N2 Level, status code
        # updated every 8 hours or so
        with open(self.data_path) as data:
            for line in data:
                self.timestamp, self.He_level, self.N2Level, self.status = line.split(',')

        # get the version
        self.version = None
        #try:
        #    with open(self.version_path) as version:
        #        for line in version:
        #            self.version = line.strip()
        #except IOError:
        #    self.version = None
        return line

class CryoDirect:
    # E5025 firmware V15 and below crash when opened too many times, so
    # the connection needs to stay open unless CryoMon is started
    _timeouts = 0

    def __init__(self, cryohost="V-CryogenMonitor", cryoport=23, timer=15):
        self._cryohost = cryohost
        self._cryoport = cryoport
        self._continue = False
        self._timer = timer
        self._process = None
        self._cache = (None, 0)
        self.signals()

    def signals(self):
        # must be called from the main thread!
        logging.info('installing SIGALRM signal handler for cryogen monitor read timeout')
        signal.signal(signal.SIGALRM, self.monitor_timeout_handler)
        logging.info('installing SIGUSR1 signal handler for CryoMon startup notification')
        signal.signal(signal.SIGUSR1, self.cryomon_started_handler)

    def pingable(self):
        return (os.system('/bin/ping -c1 '+self._cryohost+' > /dev/null') == 0)

    def read(self):
        # call to this function means CryoMon isn't running, so access directly
        if not self._continue:
            self.start_monitor()
        if self._cache: 
            # give it up to 5 minutes to smooth out any delays in parsing CryoMon output
            if (time.time() - self._cache[1] < max(self._timer * 2, 300)):
                return self._cache[0]
            else:
                logging.debug("failed to make a measurement within %d seconds"% (max(self._timer*2,300)))
        return None

    def start_monitor(self):
        self._continue = True
        self._thread = threading.Thread(target=self.monitor)
        self._thread.start()

    def stop_monitor(self):
        self._continue = False

    def cryomon_started_handler(self, signum, frame):
        self.stop_monitor()
        signame = tuple((v) for v, k in signal.__dict__.iteritems() if k == signum)[0]
        logging.info('signal %d (%s) caught, stopping direct cryomon communications'% (signum,signame,))
        print 'signal %d (%s) caught, stopping direct cryomon communications'% (signum,signame,)

    @staticmethod
    def monitor_timeout_handler(signum, frame):
        CryoDirect._timeouts += 1
        raise Exception("monitor read timeout")

    def read_monitor(self, process, attempts=1):
        for attempt in range(0, attempts):
            try:
                process.stdin.write('W\n')
                process.stdin.flush()
                signal.alarm(10)
                self._response = response = process.stdout.readline()
                signal.alarm(0)
                self._he = he = re.compile(r',P1.+?(\d{3})%').search(response)
                self._n2 = n2 = re.compile(r',ND(\d{3}),').search(response)
                self._ts = t = re.compile(r't ([^,]+),').findall(response)[0].replace(' ',':') if response else False
                self._when = str(datetime.datetime.now())
                if he and n2 and t:
                    # put the results in the same format as /vnmr/cryo/cryomon/cryomonData.txt
                    dummy = '0' # not used in cryogen display
                    self._measurement = measurement = t+','+he.group(1)+','+n2.group(1)+','+dummy
                    if attempt > 1:
                        logging.debug('measured %s on attempt %d'% (measurement, attempt))
                    return measurement
                else:
                    self._oops=(he,n2,t,m)
                    logging.debug('measured he=%s n2=%s t=%s'% (h3,n2,t,))
            except Exception, e:
                logging.debug('Exception reading cryogen levels directly '+str(e))
                time.sleep(5)
        logging.debug('could not read cryogen levels after %d attempts'% (attempts,))
        return None

    def spawn(self):
        shellcmd = ["nc", self._cryohost,str(self._cryoport)]
        p = self._process = subprocess.Popen(shellcmd, 
                                             stdin=subprocess.PIPE, 
                                             stdout=subprocess.PIPE, 
                                             stderr=subprocess.PIPE)
        return p
        
    def monitor(self, attempts=1):
        while self._continue:
            if CryoMonMonitor.cryomon_is_running():
                if self._process:
                    try:
                        self._process.terminate()
                    except Exception, e:
                        logging.debug('exception terminating process '+str(e))
                        pass
                    finally:
                        self._process = None
                        self._cache = None
            else:
                if not self._process or self._process.poll():
                    self.spawn()
                    logging.debug('spawned subprocess %d'% (self._process.pid))
                self._cache = (self.read_monitor(self._process, attempts), time.time())
            
            time.sleep(self._timer)
        return None

# check if the Cryogen Monitor is responding to pings and the CryoMon server is running
class CryoMonMonitor(Checker):
    def __init__(self, cryochecker=None, cryohost="V-CryogenMonitor", cryoport=23):
        super(CryoMonMonitor, self).__init__()
        self.cryohost = cryohost
        self.cryodirect = CryoDirect(cryohost, cryoport)
        self.description = "CryoMon/CryogenMonitor checker"
        self.cryochecker = cryochecker

    @staticmethod
    def cryomon_is_running():
        shellcmd = "ps aww|grep -E 'java.*vnmr\.cryomon.*-master' -c"
        expected = '1'
        process = subprocess.Popen(shellcmd, shell=True, stdout=subprocess.PIPE)
        return (process.stdout.read().strip() == expected)

    def recent(self):
        if self.cryochecker:
            measurement, timestamp = self.cryochecker.latest()
            if time.time() - timestamp < 300:
                return measurement
            else:
                measurement = self.cryochecker.last(1)
                if measurement:
                    return measurement[0]
        return CryoChecker.default()

    def check(self):
        proc = 1 if self.cryomon_is_running() else 0
        ping = 1 if self.cryodirect.pingable() else 0
        status = {'cryoping': ping, 'cryoproc': proc}
        measurement = None
        if ping: 
            if not proc:
                measurement = self.cryodirect.read()
            else:
                measurement = self.recent()

        if measurement:
            status.update({'cryolevel': measurement})
        return status

    def follow(self,t=10):
        try:
            while not self.shutdown.isSet():
                yield ('cryomonstatus', self.check())
                time.sleep(t)
            logging.debug("shutting down %s"% (self.description,))
        except GeneratorExit:
            logging.debug("generator exiting %s"% (self.description,))
        finally:
            logging.debug("%s terminated"% (self.description,))

def listenToCryoMonMonitor(p):
    status = p.follow()
    i = 0
    for s in status:
        print str(s)
        i = i + 1
        if i > 3:
            p.shutdown.set()

def runCryoMonMonitorTst():
    p = CryoMonMonitor()
    t1 = threading.Thread(target=listenToCryoMonMonitor,args=(p,))
    t1.start()
    return p

def listenToCryoMon(p):
    lines = p.follow()
    for line in lines:
        print line,

def runCryoStatTst(d=None):
    s = threading.Event()
    if d == None: d = os.getcwd() 
    p = CryoChecker(d) # test files need to be in current working directory
    #f = open(p.data_path,'r')
    last2 = p.last(2)
    t1 = threading.Thread(target=listenToCryoMon,args=(p,))
    t1.start()
    # add soemthing to cryomonData.txt (manually for now) and watch output
    print "modify cryomonData.txt manually, i.e. with echo msg >> cryomonData.txt"
    p.shutdown.set()
    t1.join()

def listenAcqMsg(name,checker):
    event = threading.Event()
    checker.listen(event)
    while not checker.shutdown.isSet():
        event.wait()
        print "listener %s was notified" % name
        event.clear()

def runAcqMsgTst():
    a = AcqMsgChecker(os.getcwd()) # put test collateral in current working directory

    t1a = threading.Thread(target=listenAcqMsg, args = ('t1a',a))
    t2a = threading.Thread(target=listenAcqMsg, args = ('t2a',a))
    t2a.start()

    values = a.follow()

    cnt = 0
    for value in values:
        cnt += 1
        print "%d: acqmsg = %s\n" % (cnt, value)
        a.notify()
        if cnt == 2:
            t1a.start()

def runExpStatTst():
    e = ExpStatChecker(os.getcwd())
    t1e = threading.Thread(target=listenAcqMsg, args = ('t1e',e))
    #t2e = threading.Thread(target=listenAcqMsg, args = ('t2e',e))
    values = e.follow()
    cnt = 0
    for value in values:
        cnt += 1
        print "%d: expstat changes = %s\n" % (cnt, value)
        e.notify()
        if cnt == 2:
            t1e.start()

def runHeartbeatTst(secs=1):
    h = Heartbeat(secs)
    i = 0
    last = time.time()
    for t in h.follow(): 
        print t, t-last
        i += 1
        if i > 3:
            h.shutdown.set()

def runTst():
    runAcqMsgTst()
    runExpStatTst()

"""
to test standalone uncomment the following lines and modifiy /tmp/ExpStatus
tst = threading.Thread(target=runExpStatTst)
tst.start()
"""
