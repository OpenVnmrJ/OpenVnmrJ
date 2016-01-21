: '@(#)whatsin.sh 1.1 29 Nov 1995 1991-1994 '
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
:
if test $# = 1
then
  logdir=$1
else
  echo""
  echo "Log file must to provided as an argument."
  exit
fi

if test ! -f $logdir
  then
    echo""
    echo "Log file $logdir not found."
  else

        echo $logdir
#        grep -s  warning ${logdir}
#        Skip Classpath errors from DashO
        grep -s ERROR ${logdir} | grep -v "Check Classpath" | grep -v "Check CLASSPATH"
        grep -s Fatal ${logdir}
        grep -w "fatal " ${logdir}
        grep -w "ld: fatal:" ${logdir}
        grep -s "acomp failed" ${logdir}
        grep -s "cannot access" ${logdir}
        grep -s "cannot find" ${logdir}
        grep -s "No such file" ${logdir}
        grep -s "syntax error" ${logdir}
        grep -s "undefined reference" ${logdir}
        grep -s "Error 1" ${logdir}
        grep -s "Error copying" ${logdir}
        grep -s "does not exist" ${logdir}
        grep -s "Permission denied" ${logdir}
        grep -s "Unable to remove" ${logdir}
        grep -s "File format not recognized" ${logdir}
        grep -s "building terminated" ${logdir}
        echo ""
        echo "In $logdir there are : "
        echo "   Total of  `grep -s ERROR ${logdir} | grep -v "Check Classpath" | grep -v "Check CLASSPATH" | wc -l`  ERRORS ."
        echo "             `grep -c Fatal ${logdir}`  Fatal errors ."
        echo "             `grep -c \"fatal \" ${logdir}`  fatal errors ."
        echo "             `grep -c \"ld: fatal:\" ${logdir}`  ld: fatal: ."
        echo "             `grep -c \"acomp failed\" ${logdir}`  acomp failed ."
        echo "             `grep -c \"cannot access\" ${logdir}`  cannot access ."
        echo "             `grep -c \"cannot find\" ${logdir}`  cannot find ."
        echo "             `grep -c \"No such file\" ${logdir}`  No such file ."
        echo "             `grep -c \"syntax error\" ${logdir}`  syntax error ."
        echo "             `grep -c \"undefined reference\" ${logdir}`  undefined reference ."
        echo "             `grep -c \"Error 1\" ${logdir}`  Error 1 ."
        echo "             `grep -c \"Error copying\" ${logdir}`  Error copying ."
        echo "             `grep -c \"does not exist\" ${logdir}`  does not exist ."
        echo "             `grep -c \"Permission denied\" ${logdir}`  Permission denied."
        echo "             `grep -c \"Unable to remove\" ${logdir}`  Unable to remove."
        echo "             `grep -c \"File format not recognized\" ${logdir}`  File format not recognized."
        echo "             `grep -c \"building terminated\" ${logdir}`  building terminated."
        echo "             `grep -c warning ${logdir}`  warnings ."
        echo ""
fi
