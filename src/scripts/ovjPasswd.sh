#!/bin/bash
#
# Copyright (C) 2017  Dan Iverson
#
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
#
# For more information, see the LICENSE file.
#
#  This script will echo an encrypted "operator" password that can be used in
#  adm/users/operators/vjpassword
#  An optional argument is the password be be encrypted.

if [[ x$vnmrsystem = "x" ]] ;
then
   vnmrsystem=/vnmr
fi

if [[ $# -eq 1 ]] ; then
   ovjPasswd=$(java -jar ${vnmrsystem}/java/passwd.jar $1)
else
   ovjPasswd=$(java -jar ${vnmrsystem}/java/passwd.jar)
fi

echo ${ovjPasswd}
