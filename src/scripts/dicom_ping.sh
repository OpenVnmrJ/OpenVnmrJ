: '@(#)dicom_ping.sh 22.1 03/24/08 2003-2004 '
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

#********************************************************************/
#* Name        : dicom_ping                                         */
#* This shell script which will ping dicom storage server.          */
#* The storage server is defined in the following file              */
#*      /vnmr/dicon/conf/dicom_store.cfg                            */
#* If the server is active then the return value will be 0.         */
#********************************************************************/

# if there is an arg, ite should be the appdir path
# to the appropriate ...dicom/conf directory
if [ $# -lt 1 ]
then
    # no ard, default to vnmrsystem
    DICOM_CONF_DIR=$vnmrsystem/dicom/conf
else
    DICOM_CONF_DIR=$1
fi

DICOM_BIN_DIR=$vnmrsystem/dicom/bin
LOGFILE=

export DICOM_BIN_DIR
export DICOM_CONF_DIR
export LOGFILE

$DICOM_BIN_DIR/store_image -p
