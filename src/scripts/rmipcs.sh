#!/bin/csh -f
# '@(#)rmipcs.sh 22.1 03/24/08 1994-2005 '
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
# Frees the System V IPC queue and/or semaphore resources
#


set argn = ${#argv}
if ( $argn < 1 ) then
  echo " "
  echo "Usage:  rmipcs type (where type = 'q'- messageQs, 's'- semaphores, 'm'- shared memory, 'a'- all)"
  echo " "
  echo "        E.G.: rmipcs a, rmipcs q, etc..."
  echo " "
  echo "Deletes System V IPC message Queues, Semaphores, Shared Memory"
  echo " "
  echo "W A R N I N G:  Deleting Message Qs and/or Semaphores and/or Shared Memory while "
  echo "                Console Software and/or Vnmr are running will result in"
  echo "                UNPREDICTABLE Acquisition Failures\!"
  echo " "
  echo "Note: if you are not root or the appropriate user then for items that you cann't"
  echo "      delete you will see additional output and error messages."
  echo " "
  exit
else
   set type = $argv[1]
endif


set AWK="awk"
set ostype=`uname -s`
if ( x$ostype == "xLinux" ) then
   set AWK="awk"
else # SunOs
   set AWK="nawk"
endif


# verify the current process is root or vnmr1
switch (`uname -r`)
    case '4*': set user = `whoami`
        breaksw
    case '5*': set user = `id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
        breaksw
    case '2.6*': set user = `id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
        breaksw
    default:
	breaksw
endsw
 
if (($user != "root")) then
   echo " "
   echo "You are logged in as $user "
   echo "Remember you need to be root or the user that created the"
   echo "Message Qs and/or Semaphores and/or Shared Memory to be able "
   echo "to delete them."
   echo " "
endif

echo " "
echo "W A R N I N G:  Deleting Message Qs and/or Semaphores and/or Shared Memory while "
echo "                Console Software and/or Vnmr are running will result in"
echo "                UNPREDICTABLE Acquisition Failures\!"
echo " "
echo " "
echo " Proceed ? (y or n):"
set answer = $<
if ( "$answer" != "y") then
   exit 0
endif
/etc/init.d/pgsql stop > /dev/null

/usr/bin/ipcs > ipcs.tmp
$AWK -f - rmtype=$type pinf=ipcs.tmp ipcs.tmp << THEEND
BEGIN { flag = 0;
}
{
  onentry = 0;
  count = 0;
  while( getline < pinf )
  {
    keychar = substr(\$1,1,1);
    if ((rmtype == keychar) || (rmtype == "a"))
    {
#      search for q, m or s as 1st entry 
      if ( (keychar == "q" ) || (keychar == "s") || (keychar == "m") )
      {
       len = length(\$1);
#       print len 
       if (len == "1")
       {
	  ipc = sprintf("ipcrm -%s %s",\$1, \$2);
	  print ipc
          system(ipc)
       }
       else
       {
          idnum = substr(\$1,2,len-1)
	  ipc = sprintf("ipcrm -%s %s",keychar, idnum);
	  print ipc
          system(ipc)
       }
      }
    }
  }    
 
  exit  
}
THEEND
rm -f ipcs.tmp
/etc/init.d/pgsql start > /dev/null
echo "Completed"
