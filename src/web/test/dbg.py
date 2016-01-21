"""
This script can be invoked to observe the bulk of the NMR status web server's
functionality, which is particularly useful when adding features that may cause
one or the other services to fail to start.

python
>>> from dbg import *

At which point the server is running as a foreground process within the python
session, and you can query it.  

An even better option is to start it in Eclipse after installing PyDev and 
specify the foreground option to nmrwebd, i.e. "-f -l DEBUG".

"""
import logging
import vato
logging.basicConfig(level=logging.DEBUG)

paths = vato.Paths(vnmr='/vnmr')
paths.add('http_path', None, "web")
paths.add('log_path', None, "web/run")
paths.logfile = paths.log_path + "/nmrwebd.log"
paths.pidfile = paths.log_path + "/nmrwebd.pid"

svcs = vato.Svcs(paths=paths)
svcs.start()
