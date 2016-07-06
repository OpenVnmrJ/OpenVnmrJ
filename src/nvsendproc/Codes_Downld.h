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

  This file was generated from Codes_Downld.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Codes_Downld_h
#define Codes_Downld_h

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






*/

/*

*  Author: Greg Brissey  4/28/2004

*/

#include "NDDS_Obj.h"

/* cmd types */
                
#define C_DOWNLOAD (1)                
                        
#define C_REPLY_ACK (2)                
                        
#define C_XFER_NUMBER (3)                
                        
#define C_XFER_ACK (4)                
                        
#define C_XFER_ABORT (5)                
                        
#define C_NAMEBUF_QUERY (6)                
                        
#define C_QUERY_ACK (7)                
                        
#define C_DWNLD_START (8)                
                        
#define C_DWNLD_CMPLT (9)                
        
/* topic name form */

/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
                
#define HOST_PUB_TOPIC_FORMAT_STR ("h/%s/dwnld/strm")                
                        
#define CNTLR_SUB_TOPIC_FORMAT_STR ("h/%s/dwnld/strm")                
        
/* The Robart's & 128x128 EPI Settings */
                
#define HOST_CODES_DOWNLD_PUB_QSIZE (2)                
                        
#define HOST_CODES_DOWNLD_PUB_HIWATER (1)                
                        
#define HOST_CODES_DOWNLD_PUB_LOWWATER (0)                
                        
#define HOST_CODES_DOWNLD_PUB_ACKSPERQ (1)                
                        
#define HOST_CODES_DOWNLD_SUB_QSIZE (4)                
                        
#define CTLR_CODES_DOWNLD_SUB_QSIZE (4)                
                        
#define CTLR_CODES_DOWNLD_PUB_QSIZE (2)                
                        
#define CTLR_CODES_DOWNLD_PUB_HIWATER (1)                
                        
#define CTLR_CODES_DOWNLD_PUB_LOWWATER (0)                
                        
#define CTLR_CODES_DOWNLD_PUB_ACKSPERQ (1)                
                        
#define CNTLR_PUB_TOPIC_FORMAT_STR ("%s/h/dwnld/reply")                
                        
#define HOST_SUB_TOPIC_FORMAT_STR ("%s/h/dwnld/reply")                
                        
#define CNTLR_CODES_DOWNLD_PUB_M21_STR ("cntlr/dwnld/reply")                
                        
#define HOST_CODES_DOWNLD_SUB_M21_STR ("cntlr/dwnld/reply")                
        
/* download types */
                
#define DYNAMIC (1)                
                        
#define TABLES (2)                
                        
#define FIXED (3)                
                        
#define VME (4)                
                        
#define MAX_IPv4_UDP_SIZE_BYTES (65535)                
                        
#define NDDS_MAX_Size_Serialize (64512)                
                        
#define MAX_FIXCODE_SIZE (32000)                
                        
#define MAX_STR_SIZE (512)                
                        
#define MAX_CNTLRSTR_SIZE (32)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Codes_DownldTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Codes_Downld
{
    DDS_Long  key;
    DDS_Short  cmdtype;
    DDS_Short  status;
    DDS_UnsignedLong  totalBytes;
    DDS_UnsignedLong  dataOffset;
    DDS_UnsignedLong  sn;
    DDS_UnsignedLong  ackInterval;
    DDS_UnsignedLong  crc32chksum;
    char*  nodeId; /* maximum length = ((MAX_CNTLRSTR_SIZE)) */
    char*  label; /* maximum length = ((MAX_STR_SIZE)) */
    char*  msgstr; /* maximum length = ((MAX_STR_SIZE)) */
    struct DDS_OctetSeq  data;

} Codes_Downld;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Codes_DownldSeq, Codes_Downld);
        
NDDSUSERDllExport
RTIBool Codes_Downld_initialize(
        Codes_Downld* self);
        
NDDSUSERDllExport
RTIBool Codes_Downld_initialize_ex(
        Codes_Downld* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Codes_Downld_finalize(
        Codes_Downld* self);
                        
NDDSUSERDllExport
void Codes_Downld_finalize_ex(
        Codes_Downld* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Codes_Downld_copy(
        Codes_Downld* dst,
        const Codes_Downld* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif


#ifdef __cplusplus

    extern "C" {

#endif



extern void getCodes_DownldInfo(NDDS_OBJ *pObj);



#ifdef __cplusplus

}

#endif


#endif /* Codes_Downld_h */
