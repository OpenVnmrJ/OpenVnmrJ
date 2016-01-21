#!/usr/bin/python
#
# publish/subscribe interface to NMR Status Dictionary
#
# - limited issues subscriptions - option to automatically unsubscribe after 
#   a specified number of events
#
# - multiple subscribers automatically notified when an entry is written
#
# - Dictionary entries automatically removed when connection is broken
#
import threading
import socket
import SocketServer
import tornado.websocket
import logging
import struct
import subprocess
import Queue
import pprint
import tornado.escape     # for json_encode

class Subscription:
    def __init__(self, subscriber, encoding= lambda noop: noop):
        self._subscriber = subscriber
        self._count = 0
        self._seq = 0
        self._id = getattr(subscriber, '_id', subscriber)
        self.encode = encoding
        
    def __hash__(self): # needed for set membership
        return hash(self._id) # hash(self._subscriber)
    
    def __cmp__(self, other): # needed for set membership
        #return 0 if (self._subscriber == other._subscriber) else -1
        return 0 if (self._id == other._id) else -1
    
    def reply(self, data):
        msg = self.encode(data)
        #print '%s ===> gets %s'% (str(subscriber), str(msg))
        self._subscriber(msg)
    
class VnmrJDictionary(dict):
    def __init__(self):
        dict.__init__(self)
        self._lock = threading.RLock() # reentrant lock allows multiple calls per thread
        self._set_times = 0

    # :TODO: move to TrTune class?  Cleanup if port is no longer valid
    def send(self, server, port, msg):
        logging.debug('db(send): sending <%s:%d:%s>'%(str(server), int(port), str(msg)))
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((server, int(port)))
            sock.send(msg)
            sock.close()
        except:
            pass
            
    def set(self, key, data, client_address=None):
        self._lock.acquire()
        if key in db:
            self[key]['value'] = data
            awake = '->'
        else:
            self[key] = {'value':data, 'event':threading.Event()}
            awake = '=>'
        logging.debug('set %s %s %s%s'% 
                      (str(key),awake, str(data)[0:40],'' if len(str(data)) < 40 else '...'))
        
        if client_address:
            self[key]['client'] = client_address
        if 'event' in self[key]:
            self[key]['event'].set()  # wake up any waiting threads
        self._set_times += 1
        self.publish(key)
        self._lock.release()

    def delete(self, client):    
        for key in self.keys():
            if 'client' in self[key]:
                if client == self[key]['client']:
                    self._lock.acquire()
                    logging.debug("disconnect client %s key %s"% (client,key))
                    del self[key]
                    self._lock.release()

    def push(self, subscription, key):
        if 'value' in self[key]:
            value = self[key]['value']
            data = (key, value)
            subscription.reply(data)

    def publish(self, key):
        self._lock.acquire()
        if 'seq' not in self[key]: 
            self[key]['seq'] = 1
        else: 
            self[key]['seq'] += 1
        if 'subscribers' in self[key]:
            for subscription in self[key]['subscribers']:
                subscriber = subscription._id # subscription._subscriber
                logging.debug('db[%s] delivering %s%s to %s if seq %d < %d'% 
                              (key, str(db[key]['value'])[0:40],
                               '' if len(str(db[key]['value'])) < 40 else '...', subscriber,
                               subscription._seq, self[key]['seq']))
                
                if subscription._seq < self[key]['seq']:
                    self.push(subscription, key)
                    subscription._seq = self[key]['seq']
                if subscription._count > 0: 
                    subscription._count -= 1
                    if subscription._count == 0:
                        logging.debug('auto-unsubscribing[%s] %s'% (key, subscriber,) )
                        self.unsubscribe(subscription, key)
        self._lock.release()

    def subscribe(self, key, subscriber, encoding=lambda noop: noop, count=0, immediately=True): 
        # subscribe with a subsequent encoding, which is a no-op by default
        self._lock.acquire()
        if key not in self:
            self[key] = {}
        if 'subscribers' not in self[key]:
            self[key]['subscribers'] = set()
        subscription = Subscription(subscriber, encoding)
        if subscription not in db[key]['subscribers']:
            self[key]['subscribers'].add(subscription)
        subscription._count = count
        self._lock.release()
        if immediately:
            self.push(subscription, key)
        logging.debug('db[%s] subscribers = %s'% (key, map(lambda x: x._id, self[key]['subscribers'])))

    def subscribed(self, subscriber, keys=None):
        if not keys: keys = self.keys()
        if not type(keys) == list: keys = [keys]
        for key in keys:
            if key in self and 'subscribers' in self[key]:
                for subscription in self[key]['subscribers']:
                    if Subscription(subscriber) == subscription:
                        return subscription._subscriber
        return None

    def unsubscribe(self, subscription, keys=None):
        if not keys: keys = self.keys()
        for key in keys:
            if 'subscribers' in self[key]:
                self._lock.acquire()
                self[key]['subscribers'].discard(subscription)
                self._lock.release()

    def pprint(self):
        pprint.PrettyPrinter(indent=3).pprint(self)

#=======
class Base(object):
    def __init__(self, **args):
        pass

class VnmrJInterface(Base):
    def __init__(self, addr=None):
        self.client_address = addr

    def reply(self, msg):
        print 'reply %s'% (str(msg),)
        raise NotImplementedError("reply is abstract")

    def format(self, query, keys, values):
        if query.endswith(':json'):
            data = {}
            map(lambda k,v: data.update({k:v}), keys, values)
            return tornado.escape.json_encode(data)
        return ' '.join(values)

    @classmethod
    def subscription(cls, subscriber):
        return db.subscribed(subscriber)

    def disconnect(self, publisher):
        db.unsubscribe(Subscription(self))  # cancel all subscriptions for caller
        db.delete(publisher)  # delete the caller's publications

    def sync(self, key, pop=False):
        while 1:
            if key in db:
                record = db[key]
                if 'value' in record:
                    value = record['value']
                    logging.debug("%s: %s"% (str(self.client_address), str(value)))
                    return value
                if not 'event' in record:
                    db[key]['event'] = threading.Event()
            else:
                db[key] = {'event':threading.Event()}
            logging.debug("%s: waiting for %s"% (str(self.client_address), str(key)))
            db[key]['event'].wait()
            if key in db:
                if (pop):
                    return db.pop(key)['value']
                else:
                    return db[key]['value']
            else:
                return ''

    def process(self, msg, requestor=None):
        if not msg:
            logging.debug('empty message from %s'% (str(self.client_address[0])))
            return

        args = msg.split()
        
        if (msg.strip() == 'bye'):
            logging.info("%s formally disconnected"% (str(self.client_address),))
            self.disconnect(self.client_address)

        elif msg.strip() == 'vnmrj':
            tstmgr_env = {"TSTMGR_HOST": str(self.client_address[0]), "TSTMGR_PORT":"5555"}
            subprocess.call("/bin/echo calling "+str(tstmgr_env)+" /vnmr/bin/vnmrj",shell=1,env=tstmgr_env)
            subprocess.call("/vnmr/bin/vnmrj",shell=1,env=tstmgr_env)

        elif args[0] == 'get' or args[0] == 'get:json':  
            # non-blocking read on a dictionary entry
            values = []
            keys = args[1:]
            logging.debug('%s %s\n'% (args[0], keys))
            for key in keys:      #key = str(args[1])
                value = ''
                if key in db:
                    record = db[key]
                    if 'value' in record:
                        value = record['value']
                values.append(value)
            #else:
            #    raise Exception("usage: "+str(args[0])+" key")
            response = self.format(args[0], keys, values)
            logging.debug("%s: get %s => %s"% (str(self.client_address), keys, str(response)))
            self.reply(response)

        elif args[0] == 'sync' or args[0] == 'sync:json': 
            # blocking read on a dictionary entry
            logging.debug("%s: sync %s"% (args[0], str(args[1:])))
            if len(args) > 1:
                value = self.sync(args[1])
            else:
                value = ''
            msg = self.format(args[0], key, value)
            self.reply(msg)

        elif args[0] == 'once' or args[0] == 'sync:once': 
            # blocking read on a dictionary entry, then destroy it
            value = self.sync(args[1], True)
            msg = self.format(args[0], key, value)
            self.reply(msg)

        elif args[0] == 'set':  # set a dictionary key, value pair, wake up blocked readers
            if len(args) > 2:
                data = msg.split(None,2)[2].strip()
                key = str(args[1])
                db.set(key, data, self.client_address)
            else:
                raise Exception("usage: "+str(args[0])+" key value")

        elif args[0] == 'subscribe':
            db.subscribe(args[1], requestor)

        elif args[0] == 'subscribe:json':
            db.subscribe(args[1], requestor, tornado.escape.json_encode)

        elif args[0] == 'unsubscribe':
            db.unsubscribe(Subscription(requestor), args[1:])

        elif args[0] == 'send': # send a command to VnmrJ
            if len(args) > 2:
                key = str(args[1])
                data = msg.split(None,2)[2]
                if key in db:
                    port = db[key]['value']
                    logging.debug('sending <%s> to port %s', data, port)
                    self.send('localhost', port, data)
                else:
                    logging.error('no port has been registered for "%s"' % (args[1]))

        elif args[0] == 'tune' or args[0] == 'tune:json':
            """
             protocol for sending and receiving fid files:
               1. a) reader 'set' db['listening'] with counter value 
                     (and server-side timestamp) 
                  b) increment the counter (on the client side), corresponds to
                     an "ACK". The server may need to set 'nettune_count' in 
                     VnmrJ to incremented value, since VnmrJ can't read the 
                     counter, or "shell('nc '):$cnt", but that's rather
                     expensive with process creation and all.
            
               2. a) reader 'read' file names synchronously (i.e. a la 'sync').
                     This is a blocking call.
                  b) clear the db['files']
                  c) read each file 'readfile', terminated by EOT
                  d) read and send each file (with timeout) with 0x1C (file
                     separator between each file and EOT after all files.
            
               3. a) writer 'get' db['listening'] counter (and timestamp).
                     This is a non-blocking call.
                  b) If set and greater than last count, 
                     write out new files and 'set' db['files'] to list of 
                     filenames.  There is no need to clear db['listening'], 
                     since it is a timestamp.
                  c) on disconnect clear db['listening']
            
               4. synchronously read files set in db
            """
            # :TODO: add a semaphore here?
            if len(args)>1:
                logging.debug("synchronizing on %s"% args[1])
                port = self.sync(args[1])        # wait for VnmrJ trtune to connect (${user}.vnmrj.trtune)
                if len(args)==2 or len(args)>2 and str(args[2])=='0':
                    vjcmd = "trtune('net.init')"
                    logging.debug("sending <%s> to VnmrJ port %s"% (vjcmd,str(port)))
                    self.send('localhost',port,vjcmd+'\n')
                if len(args)>2:
                    vjcmd = ''.join(["trtune('net.request',",args[2],")"])
                    logging.debug("sending <%s> to VnmrJ port %s"% (vjcmd,str(port)))
                    self.send('localhost',port,vjcmd+'\n')
                    logging.debug('waiting on file list')
                    files = self.sync('files', True) # wait for VnmrJ trtune to provide a list of files to read
                    logging.debug('returning file list: ',(str(files),))
                    self.reply(files)         # send the list of files back to client and destroy the record

        elif args[0] == 'trtune:json':
            # let trtune know that we are interested
            #-subscriber = requestor if requestor else self
            subscriber = requestor if requestor else self.reply
            db.subscribe('trtune.data', subscriber, tornado.escape.json_encode)
            
        elif args[0] == 'read:json':
            # let trtune know we're ready for more data
            subscriber = requestor if requestor else self.reply
            logging.debug(str(args))
            if len(args) > 1:
                trtune_instance =  args[1]
                seq_id = '#'+args[2] if len(args) > 2 else ''
                db.subscribe('trtune.data', subscriber, tornado.escape.json_encode,immediately=False)
                if trtune_instance in db and 'value' in db[trtune_instance]:
                    vjcmd = "trtune('net.request') "
                    port = db[trtune_instance]['value']
                    logging.debug("%s%s: sending <%s> to VnmrJ port %s"% (args[0], seq_id, vjcmd,str(port)))
                    db.send('localhost',port,vjcmd+'\n')
        
        elif args[0] == 'read':
            # called by remote client to dump file contents (in ascii) for each channel independently
            path = args[1]
            fileobj = open(path,'rb')
            try:
                # read a buffer - for now the whole file, but could specify a buffer size
                for data in iter(lambda: fileobj.read(),""):
                    s = struct.calcsize('f')
                    #print 'read %s'% (str(s))
                    n = len(data)/s
                    x = struct.unpack('f'*n,data)    # extract floats into ascii format
                    y = '\n'.join(map(str,x))        # make a list of floats separated by end-of-line
                    #logging.debug("sending %f float"%(len(x),))
                    #msg = self.format(args[0], path, y)
                    self.reply(y)
            finally:
                fileobj.close()

        elif args[0] == 'tune_cmd':
            if len(args) > 1:
                port = self.sync(args[1])
                logging.debug("tune_cmd %s on port %s"% (port,args[2]))
                vjcmd = None
                if args[2] == 'start':
                    vjcmd = "trtune('start')"
                elif args[2] == 'stop':
                    vjcmd = "aa('Tuning Complete') acqmode=''"
                if vjcmd:
                    logging.debug("sending tune command <%s> to VnmrJ port %s"% (vjcmd,str(port)))
                    self.send('localhost', port, vjcmd+'\n')
            
        else:
            logging.debug('%s received "%s"' % (str(self.client_address), msg))

class SockHandler(SocketServer.BaseRequestHandler, VnmrJInterface):
    def reply(self, msg):
        self.request.send(msg)

    def handle(self):
        while 1:
            msg=self.request.recv(1024)
            if not msg: break
            #print 'received: "',msg,'"'
            self.process(msg, self.reply)

    # VnmrJ does not keep the socket open, so we have to rely on erroring out
    #def finish(self):
        #self.disconnect(self.client_address)
        #print self.client_address,'disconnected!'

class WebSockHandler(tornado.websocket.WebSocketHandler, VnmrJInterface):
    def allow_draft76(self): return True
    
    def on_message(self, msg):
        self.process(msg)

    def on_close(self):
        self.client_address = (self.request.remote_ip, id(self))
        logging.info("%s %s"% (str(self.client_address), 'websocket disconnected!'))
        self.disconnect(self.client_address[0])

    def open(self):
        self.client_address = self.request.remote_ip, id(self)  # create unique instance for each connection
        logging.info("%s %s"% (str(self.client_address), 'websocket connected!'))
        self.write_message('hello')

    def reply(self, msg):
        if self.ws_connection != None:
            logging.debug("%s(reply-to): %s%s"% ("WebSockHandler", str(msg)[0:40],'' if len(str(msg)) < 40 else '...'))
            self.write_message(msg)
        else:
            junk=self
            logging.debug("%s(reply-to): socket closed"% ("WebSockHandler",))

class TrTune(threading.Thread):
    def read(self, path):
        # called by remote client to dump file contents (ascii) for each channel independently
        x = []
        fileobj = open(path,'rb')
        try:
            for data in iter(lambda: fileobj.read(),""):
                s = struct.calcsize('f')
                n = len(data)/s
                x = struct.unpack('f'*n,data)    # extract floats into ascii format
        finally:
            fileobj.close()
        return x

    def publish(self, data):
        db.set('trtune.data', data)

    def complete(self):
        # take an inventory of all items reqired to display tuning info
        result = {}

        for param in self._params + self._channels:
            if param in db and 'value' in db[param]:
                result.update({param: db[param]['value']})
            else:
                logging.debug("waiting for '%s',..."% param)
                #return None

        if 'files' in db and 'value' in db['files']:
            data = {}
            contents = db['files']['value'].split()
            count = contents[0]
            files = contents[1:]
            logging.debug("TrTune[%s]: reading %s"% (count, str(files),))
            for i in range(len(files)):
                raw = self.read(files[i])
                data.update({self._channels[i]: raw})
                logging.debug("TrTune[%s]: reading %s"% (self._channels[i], str(files[i]),))
            result['data'] = data
            return result
        logging.debug("waiting for 'files'")
        return None

    def _files_callback(self, msg):
        # trtune files are ready to be sent
        key, value = msg
        contents = value.split()
        count = contents[0]
        files = contents[1:]
        logging.debug("TrTune[%s]: reading %s"% (count, str(files),))
        data = self.complete()
        logging.debug("TrTune[%s]: data %s"% (count, str(data),))
        if data:
            self.publish(data)
        logging.debug("TrTune[%s]: reading %s complete"% (count, str(files),))
        
    def _params_callback(self, msg):
        # update parameters associated with trtune data
        key, value = msg
        logging.debug('%s event received %s: %s'% (self._name, key, str(value)))
        self._data.update({key: value})
        
    def _port_callback(self, msg):
        # change in port number of the VnmrJ instance doing the tuning
        key, value = msg
        user, port = value.split()
        cmds = [
            {'host':'localhost', 'port':port, 'data':"trtune('net.init')"+'\n'},
            {'host':'localhost', 'port':port, 'data':"trtune('net.request',0)"+'\n'},
        ]
        logging.debug("queuing <%s>"% (str(cmds),))
        self._sync.put(cmds)
        #db.send('localhost',port,vjcmd+'\n')
        #db.send('localhost',port,"trtune('net.request',0)"+'\n')
        
    def run(self):
        logging.debug("entering TrTune main loop")
        while not self._stop:
            cmds = self._sync.get()
            logging.debug("TrTune(run): received <%s>"% (str(cmds),))
            for msg in cmds:
                logging.debug("TrTune(run): sending <%s>"% (str(msg),))
                db.send(msg['host'], msg['port'], msg['data'])
        log.info('TrTune interface exiting')

    def stop(self):
        if self.isAlive():
            self._stop = True
            self._state = 2
            #self._sync.put('wakeup') #.release() #.set()
            self.join(3)

    def __init__(self, state=0, start=False):
        threading.Thread.__init__(self)
        self.name = 'TrtuneInterface'
        self._name = 'trtune interface'
        self._state = state
        self._data = {}                  # build up a tuning packet to send
        self._sync = Queue.Queue() #threading.Semaphore(0) #threading.Event()
        self._channels = ['tn', 'dn', 'dn2', 'dn3', 'dn4']
        self._stop = False
        self._set_times = 0
        self._params = ['trtune', 'nf', 'tunesw']
        for param in self._channels + self._params:
            db.subscribe(param, self._params_callback)
        db.subscribe('files',self._files_callback)
        db.subscribe('trtune.port',self._port_callback)
        if start: 
            self.start()
        #    #self._params = ['trtune', 'nf', 'tunesw', 'files']

db = VnmrJDictionary()    # initialize an empty dictionary
junk= None
