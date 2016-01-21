/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  WARNING!!  besure to reflect any changes to the custom deserializer back to Data_Upload.x */
/*
 */
/* -----------------------------------------------------------------------
 * Note this file use to be generated from the Data_Upload.x file,  however
 * a custom Deserializer has been written, which is now used.
 * Note all the code is generated via a nddsgen on Data_Upload.x however
 * the original Deserializer must be commented out 
 * To allow the makefile to work this file has been put into SCCS so it can be 
 *  directly used.
 *
 *            Author:  Greg Brissey   8/26/05
 * ------------------------------------------------------------------------
 */
/**
/*
 *
 *		built from:	Data_Upload.x
 */

#ifndef Data_Upload_h
#define Data_Upload_h

#ifndef ndds_cdr_h
#include "ndds/ndds_cdr.h"
#endif

/* 
* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
*
*/

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"


/* cmd types */

#define C_UPLOAD 1
#define C_RECVPROC_READY 10
#define C_RECVPROC_DONE 20
#define C_RECVPROC_CONTINUE_UPLINK 30
#define NO_DATA 0
#define ERROR_BLK 11
#define COMPLETION_BLK 22
#define DATA_BLK 42

/* topic name form */
/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */

#define CNTLR_PUB_UPLOAD_TOPIC_FORMAT_STR "%s/h/upload/strm"
#define HOST_SUB_UPLOAD_TOPIC_FORMAT_STR "%s/h/upload/strm"
#define CNTLR_SUB_UPLOAD_TOPIC_FORMAT_STR "h/%s/upload/reply"
#define HOST_PUB_UPLOAD_TOPIC_FORMAT_STR "h/%s/upload/reply"
#define CNTLR_SUB_UPLOAD_MCAST_TOPIC_FORMAT_STR "h/ddr/upload/reply"
#define HOST_PUB_UPLOAD_MCAST_TOPIC_FORMAT_STR "h/ddr/upload/reply"

/* download types */

#ifndef VXWORKS
#include "rcvrDesc.h"
#include "workQObj.h"
#include "errLogLib.h"
#include "expDoneCodes.h"
#include "memorybarrier.h"
#include "rcvrDesc.h"

extern membarrier_t TheMemBarrier;

#endif


#define DATAUPLOAD_FIDSTATBLK 1
#define DATAUPLOAD_FID 2
#define DATA_FID 1
#define MAX_IPv4_UDP_SIZE_BYTES 65535
#define NDDS_MAX_Size_Serialize 64512
#define MAX_FIXCODE_SIZE 64000
#define MAX_STR_SIZE 512

typedef struct Data_Upload {
    long type;
    long sn;
    unsigned long elemId;
    unsigned long totalBytes;
    unsigned long dataOffset;
    unsigned long crc32chksum;
    long deserializerFlag;
    unsigned long pPrivateIssueData;
    struct {
    	unsigned int len;
    	char *val;
    } data;
} Data_Upload;

#include "ndds/ndds_c.h"


#ifdef __cplusplus
extern "C" {
#endif

#define Data_UploadNDDSType "Data_Upload"
extern RTIBool Data_UploadSerialize(NDDSCDRStream *nddsds,
	                             Data_Upload *instance,
                                     int options);
extern int     Data_UploadMaxSize(int size);
extern RTIBool Data_UploadNddsRegister();
extern Data_Upload *Data_UploadAllocate();
extern Data_Upload *Data_UploadDeserialize(NDDSCDRStream *nddsds,
	                                     Data_Upload *instance);
extern RTIBool Data_UploadPrint(Data_Upload *nddsData_Upload,
				 unsigned int indent);
extern RTIBool NddsCDRSerialize_Data_Upload(NDDSCDRStream *, Data_Upload *);
extern RTIBool NddsCDRDeserialize_Data_Upload(NDDSCDRStream *, 
					       Data_Upload *);
extern int NddsCDRMaxSize_Data_Upload(int size);
extern RTIBool NddsCDRInitialize_Data_Upload(Data_Upload *);
extern RTIBool NddsCDRPrint_Data_Upload(Data_Upload *, const char *desc, 
					 int indent);



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
    extern "C" {
#endif

extern void getData_UploadInfo(NDDS_OBJ *pObj);

#ifndef VXWORKS
extern int getNewDataValPtr(Data_Upload *objp);
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
    extern "C" {
#endif

extern void Data_UploadAllNddsRegister();

#ifdef __cplusplus
}
#endif

#endif /* Data_Upload_h*/
