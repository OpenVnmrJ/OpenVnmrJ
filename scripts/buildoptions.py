#
# Copyright (C) 2016  Michael Tesch
#
# This file is a part of the OpenVnmrJ project.  You may distribute it
# under the terms of either the GNU General Public License or the
# Apache 2.0 License, as specified in the LICENSE file.
#
# For more information, see the OpenVnmrJ LICENSE file.
#

# execfile() on this file to import 'bo' into a SConstruct environment

from __future__ import print_function
import os
import sys
import inspect

# directory of this file
optdir = os.path.abspath(os.path.dirname(inspect.getfile(inspect.currentframe())))

if optdir not in sys.path:
    # if this file was execfile()'d from above, need to add our path before 'import bo'
    #print('xxxx:', os.getcwd())
    sys.path.insert(0, optdir)

import bo
from bo import env as boEnv
