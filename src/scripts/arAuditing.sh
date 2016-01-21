: '@(#)arAuditing.sh 22.1 03/24/08 1999-2002 '
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
#!/bin/sh

# exit if $vnmruser/p11 non-exist
if [ ! -d $vnmruser/p11 ]; then 
	echo " $vnmruser/p11 not exist. "
	exit 0
fi

# exit if  both copies and images non-exist
if [ ! -d $vnmruser/p11/copies -a ! -d $vnmruser/p11/images ]; then 
	echo " no file has been saved. "
	exit 0
fi

date=`date +%y%m%d.%H.%M`

if [ $# -gt 0 ]; then
    outfile=$1
else
    outfile=audit$date 
fi

if [ ! -d $vnmruser/p11/properties ]; then
	mkdir $vnmruser/p11/properties
fi

cd $vnmruser/p11

# copy property files
cp -f $vnmruser/templates/vnmrj/properties/userResources_*.properties properties
cp -f $vnmrsystem/templates/vnmrj/properties/cmdResources.properties properties
cp -f $vnmrsystem/templates/vnmrj/properties/paramResources.properties properties

if [ ! -d $vnmruser/p11/archivedAudit ]; then
	mkdir $vnmruser/p11/archivedAudit
fi

tar cf $vnmruser/p11/archivedAudit/$outfile.tar copies images properties
compress $vnmruser/p11/archivedAudit/$outfile.tar

\rm -rf properties/*
\rm -rf copies/*
\rm -rf images/*

echo " auditing is archived as $vnmruser/p11/archivedAudit/$outfile.tar.Z "
exit 1
