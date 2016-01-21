#!/bin/sh
#writeacct.sh

# The acct macro needs to be able to append to the acccounting file
# as root.  acct can call this script using sudo to append to the file.
# Usage: "writeacct  file_path operator owner start/done time"
 
# The line should look something like the following:
#    account="" operator="vnmr1" owner="vnmr1" done=1259097053
echo account=\"\" operator=\"$2\" owner=\"$3\" $4=$5 >> $1
