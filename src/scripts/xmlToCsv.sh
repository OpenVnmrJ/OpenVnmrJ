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

# This is the entry for a command to convert xml log files to csv files.
# It allows for limiting the conversion to 
#     specific date ranges
#     specific parameters
#     acquisition/login/both entries

# Usage: xmlToCsv inputLogFilePath outputCsvPath 
#         [-type acq/login/all] [-paramlist p1 p2 p3 p4 ...]
#         [-startdate "Month Date Year"] 
#         [-enddate "Month Date Year"]
#         [-debug]
#
#  -type can have values of "acq", "login", or "all"
#  -paramlist should be followed by a space separate list of parameters
#             that are desired in the csv file
#  -startdate specifies the beginning of the date range to be included
#             in the output file.  The dates must be in the form
#                 "Month Date Year" 
#             and surrounded by double quotes
#  -enddate specifies the end of the date range to be included
#             in the output file.  The dates must be in the form
#                 "Month Date Year" 
#             and surrounded by double quotes
#  -debug Causes the command to output all of the values it will be operating with.
#         Basically a verbose output to the command line.

# Only the input and output file paths are mandatory.  If other options
# are not specified, then "all" will be assumed for those.

# Example command line (Note: the actual command is a single line):
#   xmlToCsv /vnmr/adm/accounting/acctLog.xml /home/vnmr1/testout2.csv  
#   -type login -paramlist type start end operator owner
#   -startdate "Jan 01 2014" -enddate "Feb 1 2014"

shtoolcmd="/bin/bash"
shtooloption="-c"
javabin="$vnmrsystem/jre/bin/java"
if [ ! -f $javabin ]
then
   javabin="java"
fi

vjclasspath="$vnmrsystem/java/vnmrj.jar"

$javabin -mx600m -classpath $vjclasspath  -Dshtoolcmd="$shtoolcmd" -Dshtooloption="$shtooloption"  vnmr.ui.XmlToCsvUtil "$@"
