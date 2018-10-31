#
# Copyright (C) 2015  University of Oregon
#
# This file is a part of the OpenVnmrJ project.  You may distribute it
# under the terms of either the GNU General Public License or the
# Apache 2.0 License, as specified in the LICENSE file.
#
# For more information, see the OpenVnmrJ LICENSE file.
#
#
# Frits Vosman
#
# do not change this file.
#
# execfile() on this file to import the 'bo' (build options) module
# into the SConstruct environment, ie src/vnmrj/SConstruct does:
#
#  execfile(os.path.join(cwd, os.pardir, os.pardir, 'scripts', 'buildoptions.py'))
#
# this makes bo and boEnv available.  It will also let you run scons
# directly from the directory in question (in this example
# ../src/vnmrj/) and still inherit the build configuration from the
# command-line and/or the top-level 'config.py'.  See bo.py for more
# info on build configuration.
#

from __future__ import print_function
import os
import sys
import inspect

# directory of this file
scriptsdir = os.path.abspath(os.path.dirname(inspect.getfile(inspect.currentframe())))

if scriptsdir not in sys.path:
    # if this file was execfile()'d from above, need to add our path before 'import bo'
    #print('xxxx:', os.getcwd())
    sys.path.insert(0, scriptsdir)

import bo
from bo import env as boEnv
