: '@(#)protopub.sh 22.1 03/24/08 2003-2005 '
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
: /bin/sh


cd ~/vnmrsys
id=`id | sed -e 's/[^(]*[(]\([^)]*\)[)].*/\1/'`
echo "The user is $id "

mkdir $1_proto; mkdir $1_proto/parlib; mkdir $1_proto/psglib;
mkdir $1_proto/templates; mkdir $1_proto/templates/layout
mkdir $1_proto/templates/vnmrj; mkdir $1_proto/templates/vnmrj/protocols
mkdir $1_proto/maclib

cp -r parlib/$1.par $1_proto/parlib
cp psglib/$2.c $1_proto/psglib
cp maclib/$1 $1_proto/maclib
if ( test $# = 3 ) then
   cp maclib/$3 $1_proto/maclib
fi

cp -r templates/layout/$2 $1_proto/templates/layout
cp templates/vnmrj/protocols/$1.xml $1_proto/templates/vnmrj/protocols

cd $1_proto/templates/vnmrj/protocols
 cat $1.xml | sed 's/"'$id'"/"varian"/' > new.xml
rm $1.xml; mv new.xml $1.xml
cd ../../..
tar cvf $1_proto.tar psglib templates parlib maclib
compress $1_proto.tar
cp $1_proto.tar.Z ../User_Protocols/$1_proto.tar.Z

vnmradmin=`ls -ld $vnmrsystem/bin | awk '{print $3}'`

if ( test x$id = x$vnmradmin ) then
   cp $1_proto.tar.Z /vnmr
   cd /vnmr
   zcat $1_proto.tar.Z | tar xvf -
   cp ~/vnmrsys/seqlib/$2 /vnmr/seqlib
   rm $1_proto.tar.Z 
   rm -r $vnmruser/$1_proto
fi
