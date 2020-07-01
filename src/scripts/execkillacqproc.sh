#!/bin/bash
# 
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
#
#  execkillacqproc - run startStopProcs with sudo
#                    On newer systems with systemd in use, unless the
#                    procs are started by the systemd system, they will
#                    be terminated when the user that starts them logs out.
#

sudo /vnmr/acqbin/startStopProcs
