: !/bin/sh
# '@(#)vnmr_accounting.sh 22.1 03/24/08 2005-2007 '
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
# Script for vnmr_accounting <disable|enable>
# No argument starts the java interface
# disable renames record files by appending ".on_hold"
# enable  will rename or create record file(s)
#
if [ $# -eq 0 ] 
then
    #  Get value of vnmrsystem
    #  If not defined, ask for its value
    #  Use /vnmr as the default
    #  make sure directory exists

    if test x"$vnmrsystem" = "x"
    then
        # with 5.3 the vnmrsystem env is not set when VnmrJ adm sudo is executed
        # thus test if this has been envoke as a sudo command
        # could test SUDO_USER, SUDO_GID, or SUDO_COMMAND
        # if SUDO_USER has a value then don't ask for vnmrsystem just default 
        # to /vnmr     GMB/GRS 8/10/2009
        if [ "x" == "x$SUDO_USER" ]; then
           echo -n  "Please enter location of VNMR system directory [/vnmr]: "
           read vnmrsystem
           if test x"$vnmrsystem" = "x"
           then
               vnmrsystem="/vnmr"
           fi
        else
           vnmrsystem="/vnmr"
        fi
        export vnmrsystem
    fi

    if test ! -d "$vnmrsystem"
    then
        echo "$vnmrsystem does not exist, cannot proceed:"
        exit
    fi
    echo Starting accounting...
    $vnmrsystem/jre/bin/java -jar $vnmrsystem/java/account.jar
    exit 0
fi

echo "Usage: $0 <disable|enable>"
