: '@(#)dicom_store.sh 22.1 03/24/08 2003-2006 '
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
#* Name        : dicom_store.sh                                     */
#********************************************************************/
#  03-17-2010 M. Kritzer - simplified to call storescu              */
#        does not convert - just sends to server                    */


if [ $# -lt 1 ]
then
   echo " dicom_store error: "
   echo "  Usage --  dicom_store image_directory [conf_directory]"
   exit 1
fi

test -d $1 

DICOM_DATA_DIR=$1

# These were not being used and were thus commented out 8/20/10 - GRS
#dicom_tag=$2
#imagesout=$3

# if there are 2 args, the second one should be the appdir path
# to the appropriate ...dicom/conf directory
if [ $# -lt 2 ]
then
    # no 2nd ard, default to vnmrsystem
    DICOM_CONF_DIR=$vnmrsystem/dicom/conf
else
    DICOM_CONF_DIR=$2
fi

DICOM_BIN_DIR=$vnmrsystem/dicom/bin
LOGFILE=

export DICOM_BIN_DIR
export DICOM_DATA_DIR
export DICOM_CONF_DIR
export LOGFILE


# Change to the directory where the dcm files are located 
cd $DICOM_DATA_DIR
$DICOM_BIN_DIR/store_image -p

$DICOM_BIN_DIR/store_image $DICOM_DATA_DIR 





