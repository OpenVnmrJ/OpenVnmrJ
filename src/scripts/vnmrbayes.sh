#!/bin/csh
# 

#    theme can be default, ocean, aqua or emerald
#

setenv PATH /vnmr/craft/Bayes3/bin:${PATH}
setenv LD_LIBRARY_PATH /vnmr/craft/Bayes3/lib:/vnmr/lib

if ($# == 1) then
   $1
else
   $1 $argv[2-]
endif

