import mmap
import os
#import struct
import time
import logging

from ctypes import *
#from pprint import pprint

# PPC is bigendian, but expstat(BigEndianStructure) is not required
class expstat(Structure):
    EXPSTAT_STR_SIZE = 128
    MAX_SHIMS_CONFIGURED = 48

    _fields_ = [
        # sub-structures not supported on the python2.6 that comes stock
        # on RHEL6.x.  When it does, then we may want to move to 
        # sub-structures.
        #
        # class timeval(BigEndianStructure):
        #    _fields_ = [("tv_sec", c_uint32), ("tv_usec", c_uint32)]
        # ("TIMESTAMP",      timeval),
        ("tv_sec", c_uint32), ("tv_usec", c_uint32),
        # ----
        ("CompletionTime", c_int32),
        ("RemainingTime",  c_int32),
        ("DataTime",       c_int32),
        ("ExpTime",        c_int32),
        ("StartTime",      c_int32),
        ("ExpInQue",       c_int32),
        ("CT",             c_uint32),
        ("FidElem",        c_uint32),
        ("Sample",         c_int32),
        ("GoFlag",         c_int),
        ("SystemVerId",    c_int),
        ("InterpVerId",    c_int),
        ("ExpIdStr",       c_char * EXPSTAT_STR_SIZE),
        ("UserID",         c_char * EXPSTAT_STR_SIZE),
        ("ExpID",          c_char * EXPSTAT_STR_SIZE),
        ("ProcExpIdStr",   c_char * EXPSTAT_STR_SIZE),
        ("ProcUserID",     c_char * EXPSTAT_STR_SIZE),
        ("ProcExpID",      c_char * EXPSTAT_STR_SIZE),
        ("ReportAcqState", c_char * EXPSTAT_STR_SIZE),
        ## ("csb",            CONSOLE_STATUS),
        # class CONSOLE_STATUS(Structure):
        #   MAX_SHIMS_CONFIGURED = 48 
        #   _fields_ = [
        ("AcqCtCnt",                c_int),
        ("AcqFidCnt",               c_int), 
        ("AcqSample",               c_int), 
        ("AcqLockFreqAP",           c_int), 
        ("AcqLockFreq1",            c_int), 
        ("AcqLockFreq2",            c_int), 
        ("AcqNpErr",                c_int), 
        ("AcqSpinSet",              c_int), 
        ("AcqSpinAct",              c_int), 
        ("AcqSpinSpeedLimit",       c_int), 
        ("AcqPneuBearing",          c_int), 
        ("AcqPneuStatus",           c_int), 
        ("AcqPneuVtAir",            c_int), 
        ("AcqTickCountError",       c_int),    # enable for VJ4.x systems
        ("AcqChannelBitsConfig1",   c_int),    # whether the channel exists or not # enable for VJ4.x systems
        ("AcqChannelBitsConfig2",   c_int),    # enable for VJ4.x systems
        ("AcqChannelBitsActive1",   c_int),    # whether channel is active or not # enable for VJ4.x systems
        ("AcqChannelBitsActive2",   c_int),    # enable for VJ4.x systems
        ("AcqRcvrNpErr",            c_short),   
        ("AcqState",                c_short),   
        ("AcqOpsComplCnt",          c_short),   
        ("AcqLSDVbits",             c_short),   
        ("AcqLockLevel",            c_short),  # display ! (may be divided by 10)
        ("AcqRecvGain",             c_short),   
        ("AcqSpinSpan",             c_short),   
        ("AcqSpinAdj",              c_short),   
        ("AcqSpinMax",              c_short),   
        ("AcqSpinActSp",            c_short),   
        ("AcqSpinProfile",          c_short),   
        ("AcqVTSet",                c_short),   
        ("AcqVTAct",                c_short),  # display * 10, so divide by 10, i.e. 200 => 20 degrees
        ("AcqVTC",                  c_short),   
        ("AcqPneuVTAirLimits",      c_short),   
        ("AcqPneuSpinner",          c_short),   
        ("AcqLockGain",             c_short),   
        ("AcqLockPower",            c_short),   
        ("AcqLockPhase",            c_short),   
        ("AcqShimValues",           c_short * MAX_SHIMS_CONFIGURED),
        ("AcqShimSet",              c_short),   
        ("AcqOpsComplFlags",        c_short),   
        ("rfMonError",              c_short),   
        ("rfMonitor",               c_short * 8),   
        ("statblockRate",           c_short),   
        ("gpaTuning",               c_short * 9),   
        ("gpaError",                c_short),   
        ("probeId1",                c_char * 20),   
        ("gradCoilId",              c_char * 12),   
        ("consoleID",               c_short),
        #   ] ## END-OF-CONSOLE_STATUS STRUCT
        ]

class AcqStat():
    AcqStates = { # from statdispfuncs.c
          0:'Inactive'       ,
          5:'Rebooted'       ,
         10:'Idle' 	     ,
         15:'Active'         ,
         16:'Working'        ,
         17:'Ready'          ,
         20:'Acquiring'      ,
         25:'Pre-Acquisition',  
         30:'VT Regulating'  ,
         40:'Spin Regulating',
         50:'Auto Set Gain'  ,
         55:'AutoGain: -'    ,
         60:'AutoLock'       ,
         61:'LockFindRes'    ,
         62:'LockPower'      ,
         63:'LockPhase'      ,
         65:'Find Z0'        ,
         70:'Shimming'       ,
         75:'Host Shim'      ,  
         80:'Change Sample'  ,
         81:'Retrieve Sample',
         82:'Load Sample'    ,
         90:'Interactive'    ,
        100:'Tuning'         ,
        105:'Probe Tuning'   ,
    }

    def __init__(self):
        self._damp = 0.4
        self._LockPercent = 0.0

    def acqState(self, state):
        if self.AcqStates.has_key(state):
            return self.AcqStates[state]
        else: 
            # should be either "Active" or "Inactive" depending on acq_ok
            # as defined in statdispfuncs.c:showInfoStatus. 
            # This seems to be what VnmrJ sets non-represented values to 
            # when a sethw('acqstate') is used to set it to something 
            # invalid, like 4).  I'm assuming that "Inactive" would be
            # overwritten by an error message (but I could be wrong).
            return 'Active' 

    def lockLevel(self, Locklevel):
        if Locklevel > 1300.0:
            Locklevel = 1300.0 / 16.0 + (Locklevel - 1300) / 15.0
            if Locklevel > 100.0:
                Locklevel = 100.0
        else:
            Locklevel = Locklevel / 16.0

        dampenedLocklevel = Locklevel
        # Damp change in Locklevel
        if self._LockPercent <= 100.0 and self._LockPercent >= 0.0:
            damplevel = self._damp * self._LockPercent + (1 - self._damp) * dampenedLocklevel
            dampenedLocklevel = round(damplevel * 16) / 16
            self._LockPercent = damplevel
    
        if round(self._LockPercent * 16.0) / 16.0 != dampenedLocklevel:
            if not dampenedLocklevel > 0.0: dampenedLocklevel = 0.0
    
        lklvl = '%.1f'% Locklevel
        damp_lklvl = '%.1f'% dampenedLocklevel
        return lklvl, damp_lklvl, self._LockPercent

    def translate(self, attr):
        a = attr
        key = a[0]
        val = a[1]
        if (key == 'AcqLockLevel'):
            a = (attr[0], self.lockLevel(val)[1])
        elif key == 'AcqState':
            a = (attr[0], self.acqState(val))
        elif (key == 'ProcExpIdStr') or (key == 'ProcExpID') or (key == 'ExpIdStr') or (key == 'UserID') or (key == 'ProcUserID') or (key == 'ExpID'):
            a = (attr[0], ''.join(val))
        return a

class InfoProcMonitor(object):
    def __init__(self):
        self._infopid = None

    def active(self):
        try:
            with open('/vnmr/acqqueue/acqinfo','r') as acqinfo:
                line = acqinfo.readline()
        except IOError:
            infopid = None
        else:
            infopid = line.split()
        if infopid:
            try: # N.B. this only works with superuser privileges
                os.kill(int(infopid[0]), 0)
                self._infopid = infopid[0]
            except:
                self._infopid = None
        else:
            self._infopid = None
        return self._infopid != None

class ExpStat(expstat):
    """
    Track experiment status and whether it has changed since the last time it
    was visited. It maintains the stat when asked to as a list that has been
    constructed for the sake of easy comparisons.
    """
    DefaultPath = '/tmp'
    ValueErrorSeenAlready = False

    def __init__(self, path=None):
        if not path: path = ExpStat.DefaultPath
        self.expstat_path = path + '/ExpStatus'
        self.last = None
        self.stat = None
        self.acqstat = AcqStat()
        self._infoproc = InfoProcMonitor()

    def __repr__(self): # useful so that the class can print itself
        res = []
        for field in self._fields_:
            #res.append((field[0], repr(getattr(self,field[0]))))
            res.append('%s=%s' % (field[0], repr(getattr(self,field[0]))))
        return self.__class__.__name__ + '(' + ', '.join(res) + ')'

    def changes(self, then=None, now=None, remember=False, finalize=None):
        """ 
        returns a list of changed fields as a list of (field, value) tuples,
        which can be cast to a dict for simple iteration over fields.
        e = ExpStat()         
        was = e.status()                # or e.status(remember=True)
        changes = dict(e.changes(was))  # or just e.changes() if remember=True above
        """
        if not finalize: finalize = self.as_list
        if not then and self.stat: then = self.stat
        if not now: now = self.status(remember=remember)
        if not then: then=[]
        if type(then) != type([]): then = finalize(then)
        if type(now) != type([]): now = finalize(now)
        return [self.acqstat.translate(i) for i in now if i not in then]

    def fixAcqState(self, status):
        if not self._infoproc.active():
            status.AcqState = 0
        return status

    def as_list(self, struct=None, remember=False):
        if not struct: struct = self.status(remember=remember)
        stat = []
        for field in self._fields_:
            # we don't iterate over the non-simple c_struct elements, but at this point we don't
            # need to.  Doing so would require figuring out a way to distinguish between the strings
            # (arrays of characters) and the arrays of shorts that are part of the ExpStat
            # shared memory structure.
            try:
                si = iter(getattr(struct,field[0]))
                stat.append(self.acqstat.translate((field[0],[s for s in si])))
            except TypeError:
                stat.append((field[0],getattr(struct,field[0])))
            except AttributeError:
                pass
        return stat

    @classmethod
    def mmap_init(cls, path='/tmp/ExpStatus', length=1236):
        """ Initialize a memory-mapped file (for testing purposes on other than NMR hosts) """
        f = os.open(path, os.O_RDWR|os.O_CREAT|os.O_TRUNC)
        os.write(f, '\x00'*len)  # zero out the shared memory segment
        mmap.mmap(f, length, mmap.MAP_SHARED, mmap.PROT_WRITE)
        os.close(f)

    @classmethod
    def change(cls, key, value, path='/tmp/ExpStatus', length=1236):
        """ 
        Change a field (for testing purposes only!  The map is usually 
        read-only).  Unfortunately calcsize doesn't work on ctype Structures, 
        so you have to divine the length some other way 
        """
        f = os.open(path, os.O_RDWR)
        m = mmap.mmap(f, length, mmap.MAP_SHARED, mmap.PROT_WRITE)
        writeable_expstat = expstat.from_buffer(m)
        setattr(writeable_expstat,key,value)

    def status(self, remember=False):
        try:
            expstat_file = open(self.expstat_path,'r+b')
            fd = expstat_file.fileno()
            f = open(self.expstat_path)
            f.seek(0,2)
            length = f.tell()
            buf = mmap.mmap(fd, length, mmap.MAP_SHARED, mmap.PROT_READ)
            status = expstat.from_buffer_copy(buf)
            self.fixAcqState(status)
            if remember:
                self.last = self.as_list(self.stat)
                self.stat = status
                self.buf = buf
            return status
        except ValueError:
            if not ExpStat.ValueErrorSeenAlready:
                if length > 0:
                    logging.error("invalid mmap %d byte file %s - version mismatch?\n"% (length, self.expstat_path))
                    ExpStat.ValueErrorSeenAlready = True
                else:
                    logging.error("zero-length mmap file %s - procs not running?\n"% (self.expstat_path))
                return None
        except IOError:
            return None

def tstExpStat():
    ExpStat.mmap_init(len=1236)
    e = ExpStat()
    s1 = e.status()
    l1 = e.as_list(s1)
    l1cpy = e.as_list(s1)
    assert(l1 == l1cpy)

    # make sure the base case works for non-matching
    modified = False
    for i in l1cpy:
        print "checking %s" % (i[1])
        if hasattr(i[1],'__iter__') and len(i[1]) > 1:
            i[1][-1] = 'no way is it going to have this value'
            modified = True
            break

    assert(modified)
    assert(l1 != l1cpy)

    # make sure that at the very least we catch the modified timestamp
    time.sleep(4)
    s2 = e.status()
    l2 = e.as_list(s2)
    assert(s1.tv_sec != s2.tv_sec)
    assert(l1 != l2)

