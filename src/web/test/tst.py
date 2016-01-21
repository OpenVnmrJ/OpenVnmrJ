#!python
import unittest
#import sys
import threading
import signal
import os
import sys
import tornado.escape
#import argparse
import logging
import pdb

import vato
import vjifc
from poll import *

class Subscriber(threading.Thread):
    
    def reply(self, msg):
        #print '%s <=== %s'% (str(self), str(msg),)
        decoded_data = self.decode(msg)
        if hasattr(decoded_data, 'isalpha'): # it's just a string
            self._key, self._value = None, decoded_data
        else:
            self._key, self._value = decoded_data[0], decoded_data[1]
             
        self._times_called += 1
        self._sync.set()
        
    def wait(self, timeout=2.0):
        self._timeout = False
        if timeout > 0: 
            self._sync.wait(timeout=timeout)
        else:
            self._sync.wait()
        self._timeout = not self._sync.isSet()
        self._sync.clear()
        
    def run(self):
        self._lock.acquire()

    def stop(self):
        try:
            self._timeout = False
            self._lock.release()
        except:
            pass

    def reset(self):
        self._key = None
        self._value = None
        self._times_called = 0
        
    def __init__(self, name=None, decoder=lambda noop: noop):
        # by default decoder is a no-op, i.e. out <= in
        threading.Thread.__init__(self)
        self.decode = decoder
        self._id = name
        self._lock = threading.Lock()
        self._run = True
        self._sync = threading.Event()
        self.reset()
        self.start()

class MockVnmrJ(vjifc.SocketServer.BaseRequestHandler):
    # Just acts as a sink for messages
    def reply(self, msg):
        self.request.send(msg)
        
    def handle(self):
        while 1:
            msg = self.request.recv(1024)
            if not msg:
                break
            files = 'set files 0 spec1.dat spec2.dat'
            if msg == 'net.init':
                print "MockVJ net.init: '%s'"% (files,)
                #send('localhost', self._port, files)
            elif msg == 'net.request':
                print "MockVJ net.request: '%s'"% (files,)
                #send('localhost', self._port, files)

class FakeSocketHandler(vjifc.VnmrJInterface):
    def send(self, host, port, msg):
        self._sent = {'host':host, 'port':port, 'msg':msg}
    
    def reply(self, msg):
        self._value = msg
    
    def close(self):
        self.disconnect(self.client_address)
        
    def __init__(self):
        vjifc.VnmrJInterface(self)
        self.client_address=('127.0.0.1', id(self))
        
class SocketListener(threading.Thread):
    def __init__(self, port):
        threading.Thread.__init__(self)
        self._port = port
        self._lock = threading.Event()
        self.start()
        self._lock.wait() # wait for port to be known

    def run(self):
        try:
            self._server = vjifc.SocketServer.ThreadingTCPServer(('localhost',self._port), MockVnmrJ)
            self._port = self._server.server_address[1] # in case 0 was used to find any open port
            self._lock.set() # port is now known
            self._server.allow_reuse_addr = True
            self._server.serve_forever() 
        except SystemExit:
            # :TODO: kill all pending threads
            self._server.socket.shutdown(vjifc.socket.SHUT_WR)
            raise Exception("keyboard interrupt")

    def shutdown(self):
        self._server.shutdown()

class TestPublishSubscribe(unittest.TestCase):

    def setUp(self):
        self.s1 = Subscriber('s1')
        self.s2 = Subscriber('s2')
        self.vj = vjifc.VnmrJInterface()

    def tearDown(self):
        self.s1.stop()
        self.s2.stop()
        vjifc.db.clear()

    def test_pubsub_one(self):
        self.vj.process('subscribe a', self.s1)
        self.vj.process('set b 2')

        # setting b shouldn't invoke either s1 or s2
        self.assertTrue(self.s1._times_called == 0)
        self.assertTrue(self.s2._times_called == 0)

        # subscribe s2 to value that has already been set
        self.vj.process('subscribe b', self.s2)
        self.s2.wait()
        self.assertTrue(self.s1._times_called == 0)
        self.assertTrue(self.s2._times_called == 1)
        self.assertEqual(self.s2._value, str(2))
        
        # trigger publication of value of 'a'
        self.vj.process('set a 1')
        self.s1.wait()
        self.assertEqual(self.s1._value, str(1))
        self.assertTrue(self.s1._times_called == 1)
        self.assertTrue(self.s2._times_called == 1)
        self.vj.process('subscribe a', self.s2)

        # call a 2nd time with same value
        self.vj.process('set a 1')
        self.s1.wait()
        self.assertEqual(self.s1._value, str(1))
        self.assertTrue(self.s1._times_called == 2)
        
    def test_two_subscribers(self):
        self.assertTrue(self.s1._times_called == 0)
        self.assertTrue(self.s2._times_called == 0)
        self.vj.process('subscribe a', self.s1) # s1 subscribes to variable 'a'
        self.vj.process('subscribe a', self.s2) # s2 subscribes to variable 'a'
        self.vj.process('set a "foo"')
        self.s1.wait()
        self.s2.wait()
        self.assertEqual(self.s1._value, '"foo"')
        self.assertTrue(self.s1._times_called == 1)
        self.assertTrue(self.s2._times_called == 1)
        self.vj.process('set a "foo"')
        self.s1.wait()
        self.s2.wait()
        self.assertEqual(self.s1._value, '"foo"')
        self.assertTrue(self.s1._times_called == 2)
        self.assertTrue(self.s2._times_called == 2)

    def test_subscribe_twice(self):
        self.vj.process('subscribe b', self.s1)
        self.vj.process('subscribe b', self.s1)
        self.vj.process('set b "bar"')
        self.s1.wait()
        self.assertTrue(self.s1._times_called == 1)

    def test_unsubscribe(self):
        self.vj.process('subscribe a', self.s1)
        self.vj.process('subscribe a', self.s2)
        self.vj.process('set a 1')
        self.s1.wait(timeout=0.1)
        self.assertFalse(self.s1._timeout)
        self.vj.process('unsubscribe a', self.s1)
        self.vj.process('set a 2')
        self.s1.wait(timeout=0.1) # should timeout
        self.s2.wait(timeout=0.1) # shouldn't timeout
        self.assertTrue(self.s1._timeout)
        self.assertFalse(self.s2._timeout)
        self.assertTrue(self.s1._times_called == 1)
        self.assertTrue(self.s2._times_called == 2)

class TestTrTune(unittest.TestCase):
    # note - only methods beginning with the substring 'test' are run as tests
    def setUp(self):
        if hasattr(self, 'tuner'):
            self.tuner.stop()
        self.vj = FakeSocketHandler()# vjifc.VnmrJInterface()
        self.s1 = Subscriber(decoder=tornado.escape.json_decode) # @UndefinedVariable
        self.vj_mock = SocketListener(0)
        self.tuner = vjifc.TrTune(start=True)

    def tearDown(self):
        self.s1.stop()
        vjifc.db.clear()
        self.vj_mock.shutdown()
        self.tuner.stop()
        #self.tuner = None

    def startTrTune(self):
        port = self.vj_mock._port
        
        # Our mock VnmrJ just acts as a sink for send() requests, so fake it:
        self.nuclei = ['H1', 'C13', 'F19', 'N15', 'P31', 'H2']
        for i in range(len(self.tuner._channels)):
            self.vj.process('set '+self.tuner._channels[i]+' '+self.nuclei[i])
        self.vj.process('set trtune vnmr1')
        self.vj.process('set vnmr1.vnmrj.trtune '+str(port))
        self.vj.process('set tunesw 10000000')
        self.vj.process('set nf 2')
        # needs to match VnmrJ trtune's format, currently set files <counter> filename1 filename2 ... filenameN
        self._set_files_msg = 'set files 1 spec1.dat spec2.dat'
        self.vj.process(self._set_files_msg)

    def checkReceivedData(self):
        self.assertTrue(len(self.s1._value['data']['tn']) > 1)
        self.assertTrue(len(self.s1._value['data']['dn']) > 1)
        self.assertTrue('dn2' not in self.s1._value['data'] or len(self.s1._value['data']['dn2']) == 0)
        self.assertTrue('dn3' not in self.s1._value['data'] or len(self.s1._value['data']['dn3']) == 0)
        self.assertTrue('dn4' not in self.s1._value['data'] or len(self.s1._value['data']['dn4']) == 0)
        self.assertTrue(self.s1._value['tn'] == self.nuclei[0])
        self.assertTrue('dn4' not in self.s1._value or self.s1._value['dn4'] == self.nuclei[4])

    def test_trtune_vnmrj_first(self):
        self.vj.process('trtune:json', self.s1)  # web client sends this
        self.startTrTune()                       # VnmrJ sends this

        # wait for the client to get the data (or not)
        self.s1.wait(timeout=0)#timeout=1)
        self.assertFalse(self.s1._timeout)
        self.checkReceivedData()

    def test_trtune_multiple_calls(self):        # test order independence between VnmrJ and client
        self.startTrTune()                       # VnmrJ TrTune does this
        self.vj.process('trtune:json', self.s1)  # web client sends this
        self.s1.wait(timeout=1)#timeout=0)       # wait for the client to get the data
        self.assertFalse(self.s1._timeout)
        self.checkReceivedData()                 

        self.s1.reset()
        self.vj.process('read:json vnmr1.vnmrj.trtune', self.s1)  # signal readiness for more data
        self.vj.process(self._set_files_msg)     # VnmrJ sends more files to read
        self.s1.wait(timeout=1)                  # wait for client to get the additional data
        self.assertFalse(self.s1._timeout)
        self.checkReceivedData()

    def test_trtune_disconnect(self):
        global db
        db = vjifc.db
        c1 = FakeSocketHandler()
        c2 = FakeSocketHandler()
        c1.process('set a 1')
        c2.process('set b 2')
        c1.process('get a')
        a = c1._value
        c1.process('get b')
        b = c1._value
        self.assertEquals(a, '1')
        self.assertEquals(b, '2')
        c1.close()
        c2.process('get a')
        a2nd = c2._value
        c2.process('get b')
        b2nd = c2._value
        self.assertEquals(a2nd, '')
        self.assertEquals(b2nd, '2')
                
    @unittest.skip("not implemented yet")
    def test_web_client_refresh(self):           # simulate client performing web page reload operation
        pass

    @unittest.skip("not implemented yet")
    def test_multiple_clients(self):
        pass

class TestStartupLockFile(unittest.TestCase):
    def setUp(self):
        paths = vato.Paths(vnmr='.')
        paths.add('http_path', None, "web/static")
        paths.add('log_path', None, "web/run")
        paths.logfile = paths.log_path + "/nmrwebd.log"
        paths.pidfile = paths.log_path + "/nmrwebd.pid"
        signal.signal(signal.SIGALRM, self.handler)
        self._timeout = False
        self._paths = paths

    def handler(self, signum, frame):
        self._timeout = True

    def set_timeout(self, secs):
        self._timeout = False
        signal.alarm(secs)
    
    def clear_timeout(self, timeout_was_expected=False):
        self.assertEqual(self._timeout, timeout_was_expected)
        signal.alarm(0)
 
    def startServices(self, in_child=True):
        # pass in_child=False to step through what the child is actually doing
        # child sends 'services started' message back to parent via a pipe
        r, w = os.pipe()
        child = os.fork() if in_child else 0
        if child == 0: # we're in the child process
            os.close(r)
            w = os.fdopen(w, 'w')
            vato.Svcs(paths=self._paths, port=0, ws_port=0, http_port=0).start()
            w.write('services started')
            w.close()
            if in_child:
                signal.pause()
                time.sleep(10) # exit after 10 secs in case something goes wrong
                sys.exit(0)
        os.close(w)
        r = os.fdopen(r, 'r')
        return child, r
            
    def test_svcs_lock(self):
        child1, pipe1 = self.startServices()
        self.set_timeout(4)
        try:
            msg1 = pipe1.read()
        except Exception as e:
            pass
        self.clear_timeout()
        self.assertEqual(msg1, 'services started'.strip())
        child2, pipe2 = self.startServices()
        self.set_timeout(1)
        try:
            status2 = os.waitpid(child2, 0)
        except Exception as e:
            pass
        self.clear_timeout(timeout_was_expected=True)
        os.kill(child1, signal.SIGKILL)
        status1 = os.waitpid(child1, 0)
        child3, pipe3 = self.startServices(in_child=True)
        self.set_timeout(4)
        msg3 = pipe3.read()
        self.clear_timeout()
        os.kill(child3, signal.SIGKILL)
        self.assertEqual(msg3, 'services started'.strip())
        
        
# to turn on debug-level output in unit under test: 
#logging.basicConfig(loglevel=logging.DEBUG)

# to run with a finer granularity:
ps_suite = unittest.TestLoader().loadTestsFromTestCase(TestPublishSubscribe)
unittest.TextTestRunner(verbosity=1).run(ps_suite)

tr_suite = unittest.TestLoader().loadTestsFromTestCase(TestTrTune)
unittest.TextTestRunner(verbosity=1).run(tr_suite)

lk_suite = unittest.TestLoader().loadTestsFromTestCase(TestStartupLockFile)
unittest.TextTestRunner(verbosity=1).run(lk_suite)

#if __name__ == '__main__':
#    unittest.main()

#ws= SocketListener(5888)
#ws.start()
