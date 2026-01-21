
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Flash_Downld.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Flash_Downld_h
#define Flash_Downld_h

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

*

* Varian,Inc. All Rights Reserved.

* This software contains proprietary and confidential

* information of Varian, Inc. and its contributors.

* Use, disclosure and reproduction is prohibited without

* prior consent.

*/

/*

*  Author: Greg Brissey  4/20/2004

*/

#include "NDDS_Obj.h"

/* cmd types */
                
#define FLASH_DOWNLD_FILE (1)                
                        
#define FLASH_DWNLD_COMMIT (2)                
                        
#define FLASH_DELETE_FILEPAT (3)                
                        
#define FLASH_REPLY_ACK (4)                
                        
#define FLASH_XFER_NUMBER (5)                
                        
#define FLASH_XFER_ACK (6)                
                        
#define FLASH_XFER_ABORT (7)                
                        
#define FLASH_NAMEBUF_QUERY (8)                
                        
#define FLASH_QUERY_ACK (9)                
                        
#define FLASH_DWNLD_START (10)                
                        
#define FLASH_DWNLD_CMPLT (11)                
        
/* topic name form */
                
#define FLASH_HOST_PUB_TOPIC_STR ("h/flashdwnld/strm")                
                        
#define FLASH_CNTLR_SUB_TOPIC_STR ("h/flashdwnld/strm")                
                        
#define FLASH_CNTLR_SUB_TOPIC_PATTERN ("h/flashdwnld/*")                
                        
#define FLASH_HOST_PUB_TOPIC_FORMAT_STR ("h/%s/flashdwnld/strm")                
                        
#define FLASH_CNTLR_SUB_TOPIC_FORMAT_STR ("h/%s/flashdwnld/strm")                
                        
#define FLASH_CNTLR_PUB_TOPIC_FORMAT_STR ("%s/h/flashdwnld/request")                
                        
#define FLASH_HOST_SUB_TOPIC_FORMAT_STR ("%s/h/flashdwnld/request")                
                        
#define FLASH_HOST_SUB_TOPIC_PATTERN ("*/h/flashdwnld/request")                
                        
#define FLASH_CNTLR_PUB_Q_SIZE (1)                
                        
#define FLASH_CNTLR_PUB_HIWATERMARK (1)                
                        
#define FLASH_CNTLR_PUB_LOWWATERMARK (0)                
                        
#define FLASH_CNTLR_PUB_ACKS_PER_SENDQ (1)                
                        
#define FLASH_HOST_SUB_Q_SIZE (2)                
                        
#define FLASH_HOST_PUB_Q_SIZE (10)                
                        
#define FLASH_HOST_PUB_HIWATERMARK (2)                
                        
#define FLASH_HOST_PUB_LOWWATERMARK (0)                
                        
#define FLASH_HOST_PUB_ACKS_PER_SENDQ (10)                
                        
#define FLASH_CNTLR_SUB_Q_SIZE (10)                
                        
#define MAX_IPv4_UDP_SIZE_BYTES (65535)                
                        
#define NDDS_MAX_Size_Serialize (64512)                
                        
#define MAX_FLASHDATA_SIZE (16384)                
                        
#define FLASH_MAX_NAME_SIZE (64)                
                        
#define FLASH_MAX_STR_SIZE (512)                
                        
#define FLASH_MAX_CNTLRID_SIZE (12)                
                        
#define FLASH_MAX_MD5_SIZE (34)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Flash_DownldTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Flash_Downld
{
    DDS_Char  cntlrId[(FLASH_MAX_CNTLRID_SIZE)];
    DDS_Long  cmd;
    DDS_Long  status;
    DDS_UnsignedLong  totalBytes;
    DDS_UnsignedLong  dataOffset;
    DDS_Char  md5digest[(FLASH_MAX_MD5_SIZE)];
    DDS_Char  filename[(FLASH_MAX_NAME_SIZE)];
    DDS_Char  msgstr[(FLASH_MAX_STR_SIZE)];
    struct DDS_OctetSeq  data;

} Flash_Downld;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Flash_DownldSeq, Flash_Downld);
        
NDDSUSERDllExport
RTIBool Flash_Downld_initialize(
        Flash_Downld* self);
        
NDDSUSERDllExport
RTIBool Flash_Downld_initialize_ex(
        Flash_Downld* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Flash_Downld_finalize(
        Flash_Downld* self);
                        
NDDSUSERDllExport
void Flash_Downld_finalize_ex(
        Flash_Downld* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Flash_Downld_copy(
        Flash_Downld* dst,
        const Flash_Downld* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif




#ifdef __cplusplus

    extern "C" {

#endif



extern void getFlash_DownldInfo(NDDS_OBJ *pObj);



#ifdef __cplusplus

}

#endif




#endif /* Flash_Downld_h */
