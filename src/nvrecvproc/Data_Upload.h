/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Data_Upload.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Data_Upload_h
#define Data_Upload_h

#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
#else
    #include "ndds_standalone_type.h"
#endif


/*

* Copyright (c) 1999-2007 Varian,Inc. All Rights Reserved.





*

*/

#include "ndds/ndds_c.h"

#include "NDDS_Obj.h"

/* cmd types */
                
#define C_UPLOAD (1)                
                        
#define C_RECVPROC_READY (10)                
                        
#define C_RECVPROC_DONE (20)                
                        
#define C_RECVPROC_CONTINUE_UPLINK (30)                
                        
#define NO_DATA (0)                
                        
#define ERROR_BLK (11)                
                        
#define COMPLETION_BLK (22)                
                        
#define DATA_BLK (42)                
        
/* topic name form */

/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
                
#define CNTLR_PUB_UPLOAD_TOPIC_FORMAT_STR ("%s/h/upload/strm")                
                        
#define HOST_SUB_UPLOAD_TOPIC_FORMAT_STR ("%s/h/upload/strm")                
                        
#define DATA_UPLOAD_M21TOPIC_STR ("upload/strm")                
                        
#define CNTLR_SUB_UPLOAD_TOPIC_FORMAT_STR ("h/%s/upload/reply")                
                        
#define HOST_PUB_UPLOAD_TOPIC_FORMAT_STR ("h/%s/upload/reply")                
                        
#define CNTLR_SUB_UPLOAD_MCAST_TOPIC_FORMAT_STR ("h/ddr/upload/reply")                
                        
#define HOST_PUB_UPLOAD_MCAST_TOPIC_FORMAT_STR ("h/ddr/upload/reply")                
        
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
                
#define DATAUPLOAD_FIDSTATBLK (1)                
                        
#define DATAUPLOAD_FID (2)                
                        
#define DATA_FID (1)                
                        
#define MAX_IPv4_UDP_SIZE_BYTES (65535)                
                        
#define NDDS_MAX_Size_Serialize (64512)                
                        
#define MAX_FIXCODE_SIZE (64000)                
                        
#define MAX_STR_SIZE (512)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Data_UploadTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Data_Upload
{
    DDS_Long  key;
    DDS_Long  type;
    DDS_Long  sn;
    DDS_UnsignedLong  elemId;
    DDS_UnsignedLong  totalBytes;
    DDS_UnsignedLong  dataOffset;
    DDS_UnsignedLong  crc32chksum;
    DDS_Long  deserializerFlag;
    DDS_UnsignedLong  pPrivateIssueData;
    struct DDS_OctetSeq  data;

} Data_Upload;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Data_UploadSeq, Data_Upload);
        
NDDSUSERDllExport
RTIBool Data_Upload_initialize(
        Data_Upload* self);
        
NDDSUSERDllExport
RTIBool Data_Upload_initialize_ex(
        Data_Upload* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Data_Upload_finalize(
        Data_Upload* self);
                        
NDDSUSERDllExport
void Data_Upload_finalize_ex(
        Data_Upload* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Data_Upload_copy(
        Data_Upload* dst,
        const Data_Upload* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
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


#endif /* Data_Upload_h */
