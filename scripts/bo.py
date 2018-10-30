#
# Copyright (C) 2016  Michael Tesch
#
# This file is a part of the OpenVnmrJ project.  You may distribute it
# under the terms of either the GNU General Public License or the
# Apache 2.0 License, as specified in the LICENSE file.
#
# For more information, see the OpenVnmrJ LICENSE file.
#

import sys, os
from SCons.Variables import *
from SCons.Environment import *
from SCons.Script import *

# bo module
if 'optinc_once' not in globals():
    print("******************** ONCE ***********************")

    # the directory of bo.py
    bodir = os.path.dirname(__file__)

    # load the customization file
    cmdline = Variables(os.path.join(bodir, os.path.pardir, 'custom.py'))

    # command line variables
    cmdline.Add(EnumVariable('COLOR', 'Set background color', 'red',
                             allowed_values=('red', 'green', 'blue'),
                             map={'navy':'blue'}))
    cmdline.AddVariables(
        BoolVariable('RELEASE', 'Set to build for release', False),
        BoolVariable('apt_0', '', False),
        BoolVariable('AS768', '', False),
        BoolVariable('BACKPROJ', '', True),
        BoolVariable('BIR', '', False),
        BoolVariable('CHEMPACK', '', False),
        BoolVariable('cryomon_O', '', False),
        BoolVariable('CSI', '', False),
        BoolVariable('dasho', '', False),
        BoolVariable('DOSY', '', False),
        BoolVariable('DIFFUS', '', False),
        BoolVariable('FDM', '', False),
        BoolVariable('FIDDLE', '', False),
        BoolVariable('GMAP', '', False),
        BoolVariable('Gxyz', '', False),
        BoolVariable('IMAGE', '', False),
        BoolVariable('INOVA', '', False),
        BoolVariable('jaccount_O', '', False),
        BoolVariable('JCP', '', False),
        BoolVariable('JMOL', '', False),
        BoolVariable('LC', '', False),
        BoolVariable('managedb_O', '', False),
        BoolVariable('MR400FH', '', False),
        ('NDDS_VERSION', 'ndds version', '4.2e'),
        ('NDDS_LIB_VERSION', 'ndds library version', 'i86Linux2.6gcc3.4.3'),
        BoolVariable('P11', '', False),
        BoolVariable('PATENT', '', False),
        BoolVariable('PFG', '', False),
        BoolVariable('PROTUNE', '', False),
        BoolVariable('protune_0', '', False),
        BoolVariable('probeid_0', '', False),
        BoolVariable('SENSE', '', False),
        BoolVariable('SOLIDS', '', True),
        BoolVariable('stars', '', False),
        BoolVariable('VAST', '', False),
        BoolVariable('vnmrj_O', '', False),
        PathVariable('prefix', 'install prefix', os.path.abspath(os.path.join(bodir, os.pardir)), False),
    )

    #
    # this will be exported to users of 'import bo' as bo.env / boEnv
    #
    global env
    env = Environment(variables = cmdline)
    Help(cmdline.GenerateHelpText(env))
    optinc_once = True

    global PREFIX
    PREFIX=env['prefix']
