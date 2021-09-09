from __future__ import print_function
'''
@author: coldewey

To debug this is handy:
   pkill -f nmrwebd || rm -f /vnmr/web/run/nmrwebd.log && ./nmrwebd --loglevel DEBUG --http 8080 && tail -f /vnmr/web/run/nmrwebd.log

To debug from the python shell (catches python exceptions, for instance):
 - in a terminal window or emacs shell:
   sudo pkill -f nmrwebd || rm -f /vnmr/web/run/nmrwebd.log && touch /vnmr/web/run/nmrwebd.log && tail -f /vnmr/web/run/nmrwebd.log

 - then in another terminal window or emacs shell start nmrwebd in foreground mode:
   cd /vnmr/web/scripts && sudo ln -s nmrwebd nmrwebd.py
   sudo python
   >>> from nmrwebd import *
   >>> dummy('-l DEBUG -f)

 - hit Ctrl-C once or twice to get a python prompt back and print the dictionary
   >>> ^C^C
   >>> vato.db.pprint()
'''

import os
import sys
#import errno
import logging
import optparse
import vato
import signal
#import pdb

def daemonize(pidfile):
    r, w = os.pipe()
    try:
        fpid = os.fork()
        if fpid > 0:
            signal.alarm(2) # don't allow the process to hang
            os.close(w)
            pipe = os.fdopen(r, 'r')
            msg = pipe.read()
            logging.info("successfully spawned daemon reports %s"% (msg,))
            logging.info("spawning process now exiting")
            sys.exit(0)
    except OSError as e:
        logging.error("fork failed %d (%s)"% (e.errno, e.strerror))
        sys.exit(1)

    os.setsid()
    os.umask(0)
    try:
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
    except OSError as e:
        logging.error("decoupling fork failed %d (%s)"% (e.errno, e.strerror))
        sys.exit(1)
    
    sys.stdout.flush()
    sys.stderr.flush()
    try:
        si = open('/dev/null','r')
        so = open('/dev/null','a+')
        se = open('/dev/null','a+')
    except NameError:
        si = file('/dev/null','r')
        so = file('/dev/null','a+')
        se = file('/dev/null','a+')
    os.dup2(si.fileno(), sys.stdin.fileno())
    os.dup2(so.fileno(), sys.stdout.fileno())
    os.dup2(se.fileno(), sys.stderr.fileno())

    os.close(r)
    pipe = os.fdopen(w, 'w')
    pidfile.lock(record_pid=True)
    pipe.write('%s locked'% pidfile._name)
    pipe.close()

def logpath(logdir='/vnmr/web/run'):
    vato.mkdir(logdir)
    logfile='nmrwebd.log'
    logpath=logdir+'/'+logfile
    return logpath
    #logging.basicConfig(level=level, filename=logpath)

parser = optparse.OptionParser()
logvalues = {
    'FATAL': logging.FATAL, 'CRITICAL': logging.CRITICAL, 
    'ERROR': logging.ERROR, 'INFO': logging.INFO, 'DEBUG': logging.DEBUG
    }

def dummy(argv): # to call from python command shell
    main(argv.split())

def debug():
    dummy('-l DEBUG -f')

def main(argv=None):
    parser.add_option("-r","--vnmrdir",dest="vnmrdir",help="vnmr root directory", default="/vnmr")
    parser.add_option("-p","--pidfile",dest="pidfile",help="pid file", default=None)
    parser.add_option("-f","--foreground",action="store_true",dest="foreground",help="run in foreground instead of as a daemon", default=False)
    parser.add_option("--nowait",dest="pause",action="store_false", help="don't wait for sigpause", default=True)
    parser.add_option("-w","--webdir",dest="webdir",help="web root directory",default=None)
    parser.add_option("-L","--logdir",dest="logpath",help="logging directory", default=None)
    parser.add_option("-l","--loglevel",dest="loglevel",help="logging level (FATAL,CRITICAL,ERROR,INFO,DEBUG)", default='INFO')
    parser.add_option("-P","--http",dest="http_port",help="http port", default=5558)
    parser.add_option("-T","--timeout",dest="timeout",help="timeout", default=10)
    parser.add_option("--pdb",dest="pdb", help="enter debugger", default=False)

    (options, args) = parser.parse_args(argv)
    paths = vato.Paths(vnmr=options.vnmrdir)
    paths.add('http_path', options.webdir, "web")
    paths.add('log_path', options.logpath, "web/run")
    paths.logfile = paths.log_path + "/nmrwebd.log"
    paths.pidfile = paths.log_path + "/nmrwebd.pid"
    
    if options.loglevel not in logvalues:
        print('invalid log level %s'% (options.loglevel))
        sys.exit(1)
    loglevel = logvalues[options.loglevel]
    logfmt ='%(levelname)s:%(asctime)s:%(process)d:%(message)s'

    vato.mkdir(os.path.dirname(paths.logfile))
    logging.basicConfig(level=loglevel, filename=paths.logfile, format=logfmt)
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    formatter = logging.Formatter('%(message)s')
    console.setFormatter(formatter)
    logging.getLogger().addHandler(console)
    
    pidfile = vato.PidFile(paths.log_path)
    with pidfile as lock:
        if not options.foreground and not options.pdb:
            logging.debug('daemonizing')
            daemonize(pidfile)
        logging.debug('starting services')
        vato.svcs = vato.Svcs(paths=paths, http_port=options.http_port, timeout=options.timeout)
        vato.svcs.start()
        if options.pause:
            signal.pause()

if __name__ == "__main__":
    main()
