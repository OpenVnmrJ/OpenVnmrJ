#!/usr/bin/python
'''
@author: coldewey
'''
#
# Note: on *nix systems you may invoke 'lsof -i :5555' from the command line 
# to determine which process is using port 5555
#
# Requires tornado libraries, which can be downloaded from tornadoweb.org 
# (links to github).  Make sure you use the right python version, i.e. 
# python2.6, python3.1, etc. (generically referred to as python2.x below):
#     Get the tornado tarball (available on http://pypi.python.org/pypi/tornado)
#     tar xvzf tornado-$VERSION.tar.gz
#     cd tornado-$VERSION
#     python2.x setup.py build
#     sudo python2.x setup.py install
#
# To debug using pdb and emacs you need a pdb shell script that calls 
#     #!/bin/sh 
#     exec python -m pdb "$@"
#
#     to get help: help
#     to start: M-x pdb vato.py
#           or: M-x pdb nmrwebd
#     to set a breakpoint go to place in code and: C-x SPC 
#     to run with arguments: run -arg1 --longarg2
#     to run to breakpoint: c
#     to step: s           to go to next line: n    
#     to print stack: w    to go up and down in stack: u and d
#     to print expression: p <expr>     i.e. p self
#
# To see the definition of an object from the Python shell: dir(object)
#
# To find out which process is using a particular port on a Linux or MacOS 
# systems: lsof -i @host:port, i.e.
#     lsof -i @localhost:5558 or lsof -i:5558
# 
# To print readable dictionary:
#     vato.db.pprint()
#
# To pretty-print the monitor cache:
#     from pprint import *
#     pprint(vato.svcs.monitor._cache)
#
# To access the cryogen monitor direct read interface runtime object from the:
# python prompt after starting nmrwebd with debug():
#     d = vato.svcs.monitor.cryomonmon.cryodirect
#     d._measurement # last measurement made
#     d._he          # last Helium level parsed
#     d._n2          # last N2 level parsed
#     d._timeouts    # number of times that the readline timed out
#
import os
import errno
import sys
import fcntl
import subprocess
import threading
import struct
import time
import tornado.ioloop
import tornado.options
import tornado.web
import tornado.netutil
import tornado.httpserver
import tornado.escape     # for json_encode
import tornado.gen
try:
  import http.client as httplib
except ImportError:
   import httplib

import hashlib
import base64
import traceback
import logging

# don't overwhelm logging with frequently changing parameters or heartbeat
DontLog = ['heartbeat', 
           'tv_sec', 'tv_usec', 'AcqLockLevel', 'AcqOpsComplCnt', 'DataTime', 
           'AcqCtCnt', 'CT'
           ]

import pdb

import cfg
from vjifc import *
from poll import *

class SocketListener(threading.Thread):
    def __init__(self, port):
        threading.Thread.__init__(self)
        self.name = "SocketListener"
        self._port = port

    def run(self):
        try:
            self._server = SocketServer.ThreadingTCPServer(('',self._port), SockHandler)
            self._server.allow_reuse_addr = True
            self._server.serve_forever()
        except SystemExit:
            # :TODO: kill all pending threads
            self._server.socket.shutdown(socket.SHUT_WR)
            raise Exception("keyboard interrupt")

# :TODO: integrate websocketlistener with HttpListener on the same port (but different URL)
class WebSocketListener(threading.Thread):
    def __init__(self, ws_port):
        threading.Thread.__init__(self)
        self.ws_port = ws_port
        self.name = "WebSocketListener"
        self.ioloop = tornado.ioloop.IOLoop()
        
    def run(self):
        app = tornado.web.Application([
                (r"/", WebSockHandler),
                ])
        app.listen(self.ws_port, io_loop=self.ioloop)
        self.ioloop.start()
        #tornado.ioloop.IOLoop.instance().start()

class HttpListener(threading.Thread):
    def __init__(self, http_port, http_path, monitor, timeout):
        threading.Thread.__init__(self)
        self.http_path = http_path
        self.http_port = http_port
        self.dir = os.path.dirname(__file__)
        self.name = "HttpSocketListener"
        self.monitor = monitor
        self.ioloop = tornado.ioloop.IOLoop()
        self._timeout = timeout

    def run(self):
        app = tornado.web.Application([
                (r"/", HttpHandler),
                (r"/ws", WebSockHandler),
                (r"/monitor/(.*)/(.*)", EventSourceHandler, dict(monitor=self.monitor, event_class = SSE, keepalive = 60)),
                (r"/query", QueryHandler, dict(monitor=self.monitor, timeout=self._timeout)),
                (r"/static", tornado.web.StaticFileHandler, dict(path=self.http_path+'/static'))
                ], template_path=self.http_path, static_path=self.http_path+'/static')

        app.listen(self.http_port, io_loop=self.ioloop)
        cfg.tornado = app
        self.ioloop.start()

class HttpHandler(tornado.web.RequestHandler):
    def get(self):
        self.redirect("/static/index.html", permanent=True)

class Query(VnmrJInterface):
    def __init__(self, connection, request=None, timeout=None):
        self._id = connection # handle by which Subscription checks for membership
        self._request = request
        self._sync = Queue.Queue()
        self._timeout = timeout

    @classmethod
    def subscription(cls, connection, request=None, timeout=None):
        cls._timeout = timeout
        return VnmrJInterface.subscription(connection) or cls(connection, request, timeout)

    @classmethod
    def cleanup(self, connection):
        return VnmrJInterface.disconnect(connection)

    @tornado.gen.coroutine
    def get(self, callback=None):
        logging.debug("Query.get.sync")
        try:
          response = self._sync.get(True, int(self._timeout))
          #response = self._sync.get(True, None)
        except:
          response = 'timedout'
        logging.debug("Query.get.sync=%s"% (str(response),))
        return (response, None)

    #def reply(self, msg): # VnmrJInterface base class requires a reply method
    def __call__(self, msg):
        logging.debug('reply(%s): %s%s'% (str(self._request.remote_ip),
                                        str(msg)[0:40],'' if len(str(msg)) < 40 else '...'))
        self._sync.put(msg)

class QueryHandler(tornado.web.RequestHandler, VnmrJInterface):
    def initialize(self, monitor, timeout=None):
        self.monitor = monitor
        self._timeout = timeout
        cfg.queryHandler = self
        #self._sync = Queue.Queue()

    def jsonify(self, data):
        return tornado.escape.json_encode(dict(data)) +'\r\n\r\n' # @UndefinedVariable

    @tornado.web.asynchronous
    @tornado.gen.coroutine
    def post(self):
        cfg.query = self  # to facilitate debug
        target = self.request.query
        logging.debug("post(%s)%s/%s" % (target, id(self), self.request))

        # send a json msg with the state of the entire acqusition system
        if target == "systemState":
            # older versions of Chrome on the HP Slate tablet crash when parsing JSON with empty strings,
            # 'cryomon':'' crashes older versions of Chrome on the HP Slate tablet.  Before that issue arose,
            # it was reply = self.jsonify(self.monitor.status()).  This is computationally expensive and should
            # be boofed at some point, perhaps by not putting empty strings into the self.monitor._cache in
            # the first place.  
            nonempty = dict((k,v) for k, v in self.monitor.status().items() if str(v)!='')
            reply = self.jsonify(nonempty)
            logging.debug("post(%s) reply = %s%s" % (target, str(reply)[0:40],'' if len(str(reply)) < 40 else '...'))
            self.write(reply)
            self.finish()
            return

        # send a json msg with updates to the last state sent
        if target == "systemUpdate":
            reply = self.jsonify(self.monitor.status())
            logging.debug("post(%s) reply = %s%s" % (target, str(reply)[0:40],'' if len(str(reply)) < 40 else '...'))
            status = self.monitor.update()
            self.write(self.jsonify(self.monitor.update()))
            self.finish()
            return

        # cryogen monitor
        if target == "cryomon":
            reply = self.jsonify({'cryomon':self.monitor.cryomon.check().strip('\n')})
            logging.debug("post(%s) reply = %s%s" % (target, str(reply)[0:40],'' if len(str(reply)) < 40 else '...'))
            self.write(reply)
            self.finish()
            return

        # cryogen monitor status monitor
        if target == "cryomonstatus":
            #reply = self.jsonify({'cryomonstatus':self.monitor.cryomonmon.check()})
            reply = self.jsonify(self.monitor.cryomonmon.check())
            logging.debug("post(%s) reply = %s%s" % (target, str(reply)[0:40],'' if len(str(reply)) < 40 else '...'))
            self.write(reply)
            self.finish()
            return

        # send json msg with trtune data
        args = target.split('+')
        if args[0] == "trtune:json":
            response = Query.subscription(self.request.connection, self.request, 20)
            self.process(args[0], response)
            logging.debug("post(%s) waiting for trtune %s" % (target,str(response)))
            #reply = response.get(timeout=self._timeout)
            (reply, error) = yield tornado.gen.Task(response.get)
            if error:
                raise tornado.web.HTTPError(500, error)
            else:
                logging.debug("post(%s) trtune completed" % (target,))
                self.write(reply) #self.write('OK')
                self.finish()
            return

        # :TODO: may want this to be a blocking query on the tuner and loop over all clients...
        if args[0] == "read:json":
            response = Query.subscription(self.request.connection, self.request, self._timeout)
            self.process(" ".join(args), response)
            logging.debug("post(%s) subscribers is %s" 
                          % (args[0],str(map(lambda x: x._id, db['trtune.data']['subscribers']))))
            logging.debug("post(%s) connection %s waiting for data %s" 
                          % (args[0],str(self.request.connection),str(response)))
            try:
                (reply, error) = yield tornado.gen.Task(response.get)
                logging.debug("post(%s) writing %s" % (args[0], reply))
                self.write(reply)
            except Exception as te:
                logging.debug("post(%s) timed out after %ds %s" % (args[0], self._timeout or -1, te))
                self.send_error(504, mesg="start trtune from VnmrJ timed out")
                reply = self.jsonify({'timeout' : 1})
            finally:
                self.finish()
            return

        if len(args) > 2 and args[0] == 'raise':
            logging.info("post(%s) raising HTTPError %s %s"% (args[0], args[1], args[2],))
            raise tornado.web.HTTPError(int(args[1]), str(args[2]))
            self.finish()
            return

        if len(args) > 2 and args[0] == 'error':
            logging.info("post(%s) sending error %s %s"% (args[0], args[1], args[2],))
            self.send_error(int(args[1]), mesg=str(args[2]))
            self.finish()
            return

        if target not in self.monitor._cache:
            logging.info("post(%s) no status" % (target,))
            self.send_error(404, mesg="No status available")
            self.finish()
            return

        try:
            reply = unicode(str(self.monitor._cache[target]))
            logging.debug("post(%s) = %s" % (target, reply,))
            self.write(reply)
        except ValueError as ve:
            logging.error("post(%s) not formatted properly (%s)" % (target, ve))
            self.send_error(400, mesg="Data is not properly formatted: <br />%s" % (ve,))
        finally:
            self.finish()

    def on_connection_close(self):
        logging.debug("cleaning up %s "% (str(self.request.connection),))
        response = Query.cleanup(self.request.connection)

class Monitor(object):
    """ monitor NMR subsystems """
    def __init__(self,path=None):   # call with m=Monitor(os.getcwd()) for debugging
        self.cryomon = CryoChecker(path)
        self.cryomonmon = CryoMonMonitor(self.cryomon)
        self.acqmsg  = AcqMsgChecker(path+"/acqqueue")
        self.expstat = ExpStatChecker('/vnmr/acqqueue')
        self.heartbeat = Heartbeat(5.0)
        self.shutdown = threading.Event()
        self._cache = {}
        self._lock = {}
        self._listener = {}
        self.checker = {
            'acqmsg':  AcqMsgChecker,
            'cryomon': CryoChecker,
            'cryomonmon': CryoMonMonitor,
            'default': ExpStatChecker
        }

    def lock(self, key, value):
        if key in self._lock:
            self._lock[key].append(value)
            logging.debug('lock:appending %s = %s\n', key, value)
        else:
            self._lock[key] = [value]  
            logging.debug('lock:setting %s = %s\n', key, value)
            
    def register(self, target, key, callback=None, event=None):
        if not callback and not event:
            raise Exception("need either a callback or an event")
        what = target
        who = (callback, target, key, event)
        logging.debug('registering %s for %s'% (who, what))
        self.lock(what, who)
        self._listener[what] = who

    def unregister(self, target, key, callback=None, event=None):
        # there are only a small number of (key,detail) pairs, so don't bother
        # removing empty entries - we'll just need them again for future clients
        who = (callback, target, key, event)
        key = self._listener[who]
        if key in self._lock and who in self._lock[key]:
            logging.debug("removing listener (%s)"% str(who))
            self._lock[key].remove(who)
            self._listener.pop(key,'None')
        
    def monitor(self,checker,cache=True):
        while not self.shutdown.isSet():
            changes = checker.follow()
            for change in changes:
                self.notify(change,cache)

    def status(self):
        return self._cache
            
    def update(self):
        # brute force it for now, eventually delta between last and current states
        return self._cache
            
    def notify(self, change, store=True):
        if type(change) == tuple:
            if change[0] not in DontLog:
                logging.debug("change = %s", change)
            i, value = change
            if store: self._cache[i] = value
            if i in self._lock:
                for who in self._lock[i]:
                    callback, target, key, event = who
                    if callback: 
                        if target not in DontLog:
                            logging.debug("callback %s/%s => %s\n"% (key, target, str(value)))
                        callback(target, value)
                    if event:
                        logging.debug("setting %s => %s\n"% (str(event), str(value)))
                        event.set(value)
        else:
            for i in change: self.notify(i,store)
                
    def start(self):
        self.heartbeat_thread = threading.Thread(name="Heartbeat",target=self.monitor, args = (self.heartbeat,))
        self.cryo_thread = threading.Thread(name='CryogenListener',target=self.monitor, args = (self.cryomon,))
        self.cryomonmon_thread = threading.Thread(name='CryoMonMonitor',target=self.monitor, args = (self.cryomonmon,False,))
        self.acqmsg_thread = threading.Thread(name='AcqMsgListener',target=self.monitor, args = (self.acqmsg,))
        self.expstat_thread = threading.Thread(name='ExpStatListener',target=self.monitor, args = (self.expstat,))
        self.heartbeat_thread.start()
        self.cryo_thread.start()
        self.cryomonmon_thread.start()
        self.acqmsg_thread.start()
        self.expstat_thread.start()
        
    def stop(self): self.shutdown = True

class SSE(object):
    # a Server-Sent Event Object
    cnt = 0
    content_type = "text/plain"
    LISTEN  = "poll"
    FINISH  = "close"
    RETRY   = "retry"
    ACTIONS = [FINISH]

    def get_id(self):
        if self.cnt == SSE.cnt:
            self.cnt = SSE.cnt
            SSE.cnt+=1
        return self.cnt

    def set_value(self, value):
        self._value = value

    def get_value(self):
        return [line for line in self._value.split("\n")]

    def __init__(self, target, action, value=None):
        self.target = target
        self.action = action
        self.set_value(value)

    id = property(get_id)
    value = property(get_value, set_value)

class EventSourceHandler(tornado.web.RequestHandler):
    _connected = {}
    _lock = {}
    def initialize(self, monitor, event_class = SSE, keepalive = 0):
        self._event_class = event_class
        self._retry = None
        logging.debug("initializing %s keepalive = %d"% (str(self),int(keepalive)))
        if keepalive != 0:
            self._keepalive = tornado.ioloop.PeriodicCallback(self.push_keepalive, int(keepalive))
        else:
            self._keepalive = None
        self.monitor = monitor
        
    @tornado.web.asynchronous
    def push_keepalive(self):
        # called by `tornado.ioloop.PeriodicCallback` 
        #? does it need to be protected by a semaphore or does tornado do that for us?
        logging.debug("push_keepalive()")
        self.write(": keepalive %s\r\n\r\n" % (unicode(time.time())))
        self.flush()

    def push(self, event):
        # write event-source outputs on current handler for given event
        if event.target not in DontLog:
            logging.debug("push(%s@%s/%s,%s,%s,%s)" % (self.request.remote_ip, id(self), event.target, event.id, event.action, event._value))
        if hasattr(event, "id"):
            self.write("id: %s\r\n" % (unicode(event.id)))
        if self._retry is not None:
            self.write("retry: %s\r\n" % (unicode(self._retry)))
            self._retry = None

        target = event.target
        if type(event._value) == list:
            for line in event._value:
                nonempty = str(line).strip('\n')
                self.write("data: %s\r\n" % (tornado.escape.json_encode({target: str(line).strip('\n')})))  # @UndefinedVariable
                if target not in DontLog:
                    logging.debug("data+@%s: %s\r\n" % (self.request.remote_ip, tornado.escape.json_encode({target: str(line).strip('\n')}))) # @UndefinedVariable
        elif type(event._value) == dict: # workaround for Chrome that cannot handle nested JSON data structures
            nonempty = dict((k,v) for k, v in event._value.items() if str(v)!='')
            self.write("data: %s\r\n"% (tornado.escape.json_encode(nonempty),))  # @UndefinedVariable
            #self.write("data: %s\r\n"% (tornado.escape.json_encode(dict(event._value.items()))))  # @UndefinedVariable
        else:
            value = event._value.strip('\n') if type(event._value) is str else event._value
            self.write("data: %s\r\n"% (tornado.escape.json_encode({target: value}))) # @UndefinedVariable
            #self.write("data: %s\r\n"% (tornado.escape.json_encode({target: str(event._value).strip('\n')}))) # @UndefinedVariable
            if target not in DontLog:
                logging.debug("data@%s/%s: %s\r\n" % (self.request.remote_ip, id(self), tornado.escape.json_encode({target: value}))) # @UndefinedVariable
        self.write("\r\n")
        self.flush()

    def buffer_event(self, target, value = None):
        # create and store an event for the target
        event = self._event_class(target, "poll", value)
        self.push(event)
        
    def is_connected(self, target):
        return target in self._connected.values()

    def set_connected(self, target):
        logging.debug("set_connected(%s)" % (target,))
        if not self in self._connected: 
            self._connected[self] = set()
        self._connected[self].add(target)
        self._lock[target] = self # toro.AsyncResult(self._event_loop) # was ()
        logging.debug("set_connected(%s) locked" % (target,))

    def set_disconnected(self):
        target = None
        try:
            targets = self._connected[self]
            logging.debug("set_disconnected(%s)" % (targets,))
            if self._keepalive:
                self._keepalive.stop()
            for target in targets:
                self.monitor.unregister(target, id(self), callback=self.buffer_event)
                del(self._lock[target])
            self.finish()
            del(self._connected[self])
        except Exception as err:
            logging.error("set_disconnected(%s,%s): %s", str(self), target, err)
            #raise

    def write_error(self, status_code, **kwargs):
        # Overloads the write_error() method of RequestHandler from httplib
        # This will end the current eventsource channel.
        if self.settings.get("debug") and "exc_info" in kwargs:
            # in debug mode, try to send a traceback
            self.set_header("Content-Type", "text/plain")
            for line in traceback.format_exception(*kwargs["exc_info"]):
                self.write(line)
            self.finish()
        else:
            if "mesg" in kwargs:
                self.finish("<html><title>%(code)d: %(message)s</title>"
                            "<body>%(code)d: %(mesg)s</body></html>\n" % {
                        "code": status_code,
                        "message": httplib.responses[status_code],
                        "mesg": kwargs["mesg"],
                        })
            else:
                self.finish("<html><title>%(code)d: %(message)s</title>"
                            "<body>%(code)d: %(message)s</body></html>\n" % {
                        "code": status_code,
                        "message": httplib.responses[status_code],
                        })

    # Synchronous actions
    @tornado.web.asynchronous
    def post(self, action, target):
        logging.debug("post(%s,%s)%s/%s" % (target, action, id(self), self.request))
        self.response.headers['Content-Type'] = "text/plain"
        self.set_header("Accept", self._event_class.content_type)
        if target not in self.monitor._cache:
            self.send_error(404, mesg="Target is not connected")

        try:
            self.write(self.monitor._cache[target])
        except ValueError as ve:
            self.send_error(400, mesg="Data is not properly formatted: <br />%s" % (ve,))

    # Asynchronous actions
    def _event_loop(self, event):
        # gets and forwards all events for target matching current handler
        # until Event.FINISH is reached, and then closes the channel.
        logging.debug("_event_loop(%s)" % (event.target,))
        if self._event_class.RETRY in self._event_class.ACTIONS:
            if event.action == self._event_class.RETRY:
                try:
                    self._retry = int(event.value[0])
                    return
                except ValueError:
                    logging.error("incorrect retry value: %s" % (event.value,))
        if event.action == self._event_class.FINISH:
            self.set_disconnected()
            self.finish()
            return
        self.push(event)
        self._lock[event.target].get() # self._event_loop)

    @tornado.web.asynchronous
    def get(self, action=None, targets=None):
        """
        Opens a new event_source connection and wait for events to come

        :returns: error 423 if the target token already exists
        Redirects to / if action is not matching Event.LISTEN.
        """
        logging.debug("get(from=%s,target=%s,action=%s) thread %d" % (self.request.remote_ip, targets, action, threading.currentThread().ident))
        if action == "poll":
            self.set_header("Content-Type", "text/event-stream")
            self.set_header("Cache-Control", "no-cache")
            # request time to make sure basic communication is working
            fields = []
            fields = targets.split('!')
            target = fields[0]
            details = fields[1:]
            if target == 'time':
                cfg.timer = self    # to aid in debugging from the python command-line
                msg = tornado.escape.json_encode({target: time.asctime(time.localtime())}) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                self.write(reply)
                self.flush()
            elif target == 'hostname':
                msg = tornado.escape.json_encode({target: socket.gethostname()}) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                logging.debug(reply)
                self.write(reply)
                self.flush()
                return

            elif target == 'expstat' and not details: # expstat without an expstat!detail
                # send the whole enchilada the 1st time through, and shouldn't 
                # generate any more events - subscribe to sub-topics like 
                # LockLevel for change-updates
                cfg.expstat = self  # python command-line debug aid
                raw = self.monitor.expstat.expstat.as_list() # :TODO: move translation back down into ExpStat.py
                filtered = [self.monitor.expstat.expstat.acqstat.translate(i) for i in raw]
                reply = 'data: '+  tornado.escape.json_encode(dict(filtered)) +'\r\n\r\n' # @UndefinedVariable
                logging.debug(reply)
                self.write(reply)
                self.flush()
                return

            elif target == 'cryomon':
                cfg.cryo = self     # to aid in debugging from the python command-line
                last = self.monitor.cryomon.last(1)
                if last:
                    cryodata = last[0]
                else:
                    cryodata = self.monitor.cryomon.default()
                msg = tornado.escape.json_encode({target: cryodata}) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                logging.debug('%s: %s'% (id(self), reply))
                self.write(reply)
                self.flush()
            
            elif target == 'cryomonstatus':
                cfg.cryomonmon = self
                data = self.monitor.cryomonmon.check()
                #msg = tornado.escape.json_encode({target: data}) # @UndefinedVariable
                msg = tornado.escape.json_encode(data) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                #logging.debug(reply)
                self.write(reply)
                self.flush()
                
            elif target == 'heartbeat':
                cfg.heartbeat = self
                data = self.monitor.heartbeat.check()
                msg = tornado.escape.json_encode({target: data}) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                #logging.debug(reply)
                self.write(reply)
                self.flush()
                
            elif target == 'acqmsg':
                cfg.acqmsg = self   # to aid in debugging from the python command-line
                status = self.monitor.acqmsg.last
                if not status: status = ''
                msg = tornado.escape.json_encode({target: status}) # @UndefinedVariable
                reply = 'data: ' + msg + '\r\n\r\n'
                logging.debug(reply)
                self.write(reply)
                self.flush()

            # the real work is done by the monitor
            if not details:
                details = [target]
            logging.debug("details = %s"% (str(details)))
            for detail in details:
                if not self.is_connected(detail):
                    self.set_connected(detail)
                    self.monitor.register(detail, id(self), callback=self.buffer_event)
        else:
            self.redirect("/", permanent=True)
                    
    def on_connection_close(self):
        """
        overloads RequestHandler's on_connection_close to disconnect
        current handler on client's socket disconnection.
        """
        logging.debug("on_connection_close()")
        self.set_disconnected()


class Svcs(object):
    # (:TODO: merge websocket server, sse server, web host into a single server instance?
    def __init__(self, paths, port=5555, ws_port=5678, http_port=5558, timeout=None):
        self._paths = paths
        self._sock_port = port
        self._ws_port = ws_port
        self._http_port = http_port
        self._started = False
        self._lock = threading.Lock()
        self._lock.acquire()
        self._pidfile = PidFile(paths.log_path)
        self._timeout = timeout
        # monitor needs to be started from here to declare signal handlers in main thread
        self.monitor = Monitor(path=self._paths.vnmr)
        self.tuner = TrTune()  # tuner should really be a dict, i.e. tuner[args[1]] = TrTune()
        self.ws_server = WebSocketListener(self._ws_port)
        self.sock_server = SocketListener(self._sock_port)
        self.http_server = HttpListener(self._http_port, self._paths.http_path, self.monitor, self._timeout)

    def start_listeners(self):
        # start web server
        logging.info('starting http socket server for %s on port %s'
                     %(self._paths.http_path, str(self._http_port)))
        self.http_server.start()

        # start websocket server
        logging.info('starting web socket server')
        self.ws_server.start()

        # start the console status monitor
        logging.info('starting NMR system monitor')
        self.monitor.start()
    
        # start generic socket server
        logging.info('starting TCP socket server')
        self.sock_server.start()

        # start tuner
        logging.info('starting Probe Tuning interface')
        self.tuner.start()
        
    def run(self):
        # block until someone unlocks us
        try:
            #with self._pidfile as lock:
            logging.info('starting nmrwebd services')
            self._started = True
            self.start_listeners()
            self._lock.acquire()
            logging.info('stopped nmrwebd services')
        except Exception as exc:
            logging.critical(exc)
            sys.exit()
            
        logging.info('svcs exiting')
        # TODO: cleanup work here

    def start(self): 
        self.svcs_thread = threading.Thread(name='ServicesMonitor',target=self.run)
        self.svcs_thread.start()
  
    def stop(self):
        self._lock.release()

class PidFile(object):
    def __init__(self, path='/vnmr/web/run', name='nmrwebd.pid'):
        self._name = name
        self._dirpath = path
        self._path = self._dirpath +'/'+self._name
        mkdir(self._dirpath)
        self._file = open(self._path, "a+")

    def __enter__(self): # called when "with PidFile() as lock: is encountered
        try:
            self.lock()
        except:
            logging.error('could not create pid file %s'% (self._path))
            raise
        return self._file

    def __exit__(self, typ, value, traceback): # called when "with PidFile() as lock: completes
        try:
            self._file.close() # also releases flock()
        except IOError as e:
            if e != 9:
                raise
            os.remove(self._path)

    def lock(self, record_pid=False):
        try:
            fcntl.flock(self._file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            if record_pid:
                self._file.seek(0)
                self._file.truncate()
                self._file.write(str(os.getpid()))
                self._file.flush()
                self._file.seek(0)

        except IOError:
            raise SystemExit("nmrwebd is already running")
                
def mkdir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            logging.error('could not create directory %s'% (path))
            raise

cfg.cryo = ''
cfg.expstat = ''
cfg.acqmsg = ''
cfg.timer = ''

class Paths(object):
    def __init__(self, **kwargs): 
        self.__setattr__('vnmr',kwargs['vnmr'] if 'vnmr' in kwargs else '/vnmr')
        if self.vnmr.startswith('.'): 
            # replace leading . with current working directory as required by logging.filename
            self.vnmr = os.getcwd() + self.vnmr[1:]
        for k,v in kwargs.items():
            if k != 'vnmr': 
                self.__setattr__(k,self.vnmr+'/'+v)
    def add(self, attr, absolute, relative=None):
        self.__setattr__(attr, absolute if absolute else self.vnmr + '/' + relative)

if __name__ == "__main__":
    if 'svcs' not in locals():
        svcs = Svcs(paths=Paths(http_path='web/static', log_path='web/run'))

    if not svcs._started:
        logging.debug("starting web services")
        svcs.start()
