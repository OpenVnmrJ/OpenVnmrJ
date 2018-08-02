#!/usr/bin/env python
#
# Copyright (C) 2018  Michael Tesch
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#

# process a logfile, count the unique compiler warnings, and then sort
# them by most frequent to least frequent
# usage:
#   cat logfile.log > grok_warnings.py
#   grok_warnings.py logfile.log ...
#   grok_warnings.py 20 logfile.log ...

from __future__ import print_function
import re
import sys
import operator

lines = {}
warns = {}
linemsgs = {}
count = 20

# check args
try:
    count = int(sys.argv[1])
    filenames = sys.argv[2:]
except:
    filenames = sys.argv[1:]

if len(filenames) == 0:
    filenames = ['/dev/stdin']

# look for errors/warnings, gcc style
p = re.compile(r'([^:]+:[^:]+:[^:]+:) (warning|error):([^\[]*)(.*)')

# count them up by location
for filename in filenames:
    with open(filename) as f:
        for line in f:
            m = p.match(line)
            if m:
                fileline = m.group(1)
                warnerro = m.group(2)
                linemesg = m.group(3).strip()
                warntype = m.group(4).strip()
                if fileline not in lines:
                    lines[fileline] = 1
                else:
                    lines[fileline] += 1
                # some compilers dont include the warning type with
                # the message, in which case the warning string up to
                # the first ' is unique enough to identify the warning:
                if 0 == len(warntype):
                    warntype = linemesg.split("'")
                    if len(warntype) > 1:
                        warntype = warntype[0] + warntype[2]
                    else:
                        warntype = warntype[0]
                if warntype not in warns:
                    warns[warntype] = 1
                else:
                    warns[warntype] += 1
                linemsgs[fileline] = linemesg

#print(warns)
#exit(0)

# output
print('{} total warnings, {} unique warning lines, top {} follow:'
      .format(sum(lines.values()), len(lines), count))
if len(lines):
    # sort lines by count
    top = sorted(lines.items(), key=operator.itemgetter(1), reverse=True)
    for x in top[0:count]:
        fileline = x[0]
        print('{} {} warning:{}'.format(x[1], x[0], linemsgs[fileline]))

print()
print('{} total warnings, {} unique warning types, top {} follow:'
      .format(sum(warns.values()), len(warns), count))
top = sorted(warns.items(), key=operator.itemgetter(1), reverse=True)
for x in top[0:count]:
    print('{} {}'.format(x[1], x[0]))
