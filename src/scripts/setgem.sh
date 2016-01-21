: '@(#)setgem.sh 22.1 03/24/08 1991-1996 '
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
:/bin/sh

# First the one on /vnmr
cd $vnmrsystem
# Remove the old links and establish new ones for parameter files
rm -f par200 par300 par400
ln -s gpar200 par200
ln -s gpar300 par300
ln -s gpar400 par400

# Next are the psg psglib and seqlib directories
rm -f psg psglib seqlib
ln -s gpsg psg
ln -s gsglb gpsglib
ln -s gseqlib seqlib

# go to the bin directory and fix up acqi, config and setacq
cd bin
rm -f iadisplay vconfig setacq
ln -s giadisplay iadisplay
ln -s gconfig vconfig
ln -s gsetacq setacq

# Finally in the acqbin directory and fix up Acqproc
cd ../acqbin
rm -f Acqproc
ln -s gAcqproc Acqproc

