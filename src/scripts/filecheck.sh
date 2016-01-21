# '@(#)filecheck.sh 22.1 03/24/08 2003-2007 '
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
#/bin/csh
#****************************************************************************"
# filecheck - Checks local and system directories for duplicate VNMRJ files
#
# Details: The macro checks if the system directory and the 
#	   local directory for conflicting files or programs.
#	   It is important to check for file conflicts after a software installation.
#	
#	   If you need to check additional directories, include the system directory name 
#	   in the list, systemdir, and the corresponding local directory name, 
#	   in the localdir list. 
#
# Usage:   filecheck
#	   
#
#****************************************************************************"
#system directories
set systemdir = \
(maclib \
imaging/maclib \
autotest/maclib \
imaging_atp/maclib \
psglib \
imaging/psglib \
parlib \
imaging/parlib \
shapelib \
imaging/shapelib \
tablib \
imaging/tablib \
bin \
imaging/templates/layout \
seqlib \
psg ) 
#local directories; must match with system directories above 
set localdir  = \
(maclib \
maclib \
maclib \
maclib \
psglib \
psglib \
parlib \
parlib \
shapelib \
shapelib \
tablib \
tablib \
bin \
templates/layout \
seqlib \
psg)

@ i = 0
@ j = 0
foreach ld ($localdir)
  @ i++
  if(-d $vnmruser/$localdir[$i]) then
    cd $vnmruser/$localdir[$i]
    echo "****************************************************************"
    foreach f (*)         
      if(-e $vnmrsystem/$systemdir[$i]/$f) then
        @ j++
        echo "$j $vnmruser/$ld/$f <==> $vnmrsystem/$systemdir[$i]/$f"
      endif
    end
  endif
end    
        
  
