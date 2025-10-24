#! /bin/bash
#
# Copyright (C) 2018  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 
#
# set -x

if [ x$(uname -s) = "xDarwin" ]; then
   ping -W 1 -c 1 $(hostname) > /dev/null 2>&1
else
   ping -4 -W 1 -c 1 $(hostname) > /dev/null 2>&1
fi
if [[ $? -ne 0 ]]; then
  echo "Unable to ping $(hostname)"
  echo "This hostname should be added to the /etc/hosts file"
else
  echo "Ping $(hostname) test passed"
fi

numHost=$(grep -v ^::1 /etc/hosts | grep -v ^# | grep -w $(hostname) | wc -l)
if [[ $numHost -eq 0 ]]; then
  echo "Hostname $(hostname) should be added to the /etc/hosts file"
elif [[ $numHost -gt 1 ]]; then
  echo "Hostname $(hostname) should only be defined once in the /etc/hosts file"
else
  echo "Hostname $(hostname) test passed"
fi

libTest=0
if [[ ! -z $(type -t ldd) ]] ; then
  ldd /vnmr/bin/Vnmrbg | grep -i "not found" > /dev/null 2>&1
  if [[ $? -eq 0 ]]; then
    echo "Some libraries appear to be missing."
    echo "They appear as \"not found\" in the following output"
    ldd /vnmr/bin/Vnmrbg
    echo ""
    echo "Use /vnmr/bin/installpkgs"
    echo "to install the missing libraries"
    echo ""
    libTest=1
  fi
  ldd /vnmr/bin/Vnmrbg | grep -i "not a dynamic executable" > /dev/null 2>&1
  if [[ $? -eq 0 ]]; then
    echo "Some libraries appear to be missing."
    if [[ ! -z $(type -t objdump) ]] ; then
       echo "They appear as \"NEEDED\" in the following output"
       objdump -p ./Vnmrbg | grep NEEDED
    fi
    echo "Use /vnmr/bin/installpkgs"
    echo "to install the missing libraries"
    echo ""
    libTest=1
  fi
fi
if [[ $libTest -eq 0 ]]; then
   echo "Test for OVJ libraries passed."
fi

echo ""
echo "$0 complete"
