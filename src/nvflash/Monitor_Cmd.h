
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Monitor_Cmd.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Monitor_Cmd_h
#define Monitor_Cmd_h

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

* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved.

* This software contains proprietary and confidential

* information of Varian, Inc. and its contributors.

* Use, disclosure and reproduction is prohibited without

* prior consent.

*/

/*

*  Author: Greg Brissey  4/20/2004

*/

#include "NDDS_Obj.h"

/* Console Case Actions */

#ifndef HOSTACQSTRUCTS_H
                
#define NDDS_THROUGHPUT_TEST_PACKET_SIGNAL_END (-1)                
                        
#define ECHO (1)                
                        
#define XPARSER (2)                
                        
#define APARSER (3)                
                        
#define ABORTACQ (4)                
                        
#define HALTACQ (5)                
                        
#define STATINTERV (6)                
                        
#define STARTLOCK (7)                
                        
#define STARTINTERACT (8)                
                        
#define GETINTERACT (9)                
                        
#define STOPINTERACT (10)                
                        
#define AUPDT (11)                
                        
#define STOP_ACQ (12)                
                        
#define ACQDEBUG (13)                
                        
#define HEARTBEAT (14)                
                        
#define GETSTATBLOCK (15)                
                        
#define ABORTALLACQS (16)                
                        
#define OK2TUNE (17)                
                        
#define ROBO_CMD_ACK (18)                
        
/* Distinguish this one (console information)

   from CONF_INFO (configuration information) */
                
#define CONSOLEINFO (19)                
                        
#define DOWNLOAD (20)                
                        
#define STARTFIDSCOPE (21)                
                        
#define STOPFIDSCOPE (22)                
                        
#define GETCONSOLEDEBUG (23)                
                        
#define QUERY_CNTLRS_PRESENT (25)                
                        
#define FLASH_UPDATE (30)                
                        
#define FLASH_COMMIT (31)                
                        
#define FLASH_DELETE (32)                
                        
#define NDDS_VER_QUERY (33)                
        
#endif

#ifndef MAX_SHIMS_CONFIGURED
                
#define MAX_SHIMS_CONFIGURED (48)                
        
#endif

/* topic name form */

/* topic names form: h/master/cmdstrm, master/h/cmdstrm */
                
#define CNTLR_PUB_CMDS_TOPIC_FORMAT_STR ("master/h/cmdstrm")                
                        
#define HOST_SUB_CMDS_TOPIC_FORMAT_STR ("master/h/cmdstrm")                
                        
#define HOST_PUB_CMDS_TOPIC_FORMAT_STR ("h/master/cmdstrm")                
                        
#define CNTLR_SUB_CMDS_TOPIC_FORMAT_STR ("h/master/cmdstrm")                
        
/* download types */
                
#define DATA_FID (1)                
                        
#define MAX_IPv4_UDP_SIZE_BYTES (65535)                
                        
#define NDDS_MAX_Size_Serialize (64512)                
                        
#define MAX_FIXCODE_SIZE (64000)                
                        
#define CMD_MAX_STR_SIZE (512)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Monitor_CmdTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Monitor_Cmd
{
    DDS_Long  cmd;
    DDS_Long  arg1;
    DDS_Long  arg2;
    DDS_Long  arg3;
    DDS_Long  arg4;
    DDS_Long  arg5;
    DDS_UnsignedLong  crc32chksum;
    char*  msgstr; /* maximum length = ((CMD_MAX_STR_SIZE)) */

} Monitor_Cmd;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Monitor_CmdSeq, Monitor_Cmd);
        
NDDSUSERDllExport
RTIBool Monitor_Cmd_initialize(
        Monitor_Cmd* self);
        
NDDSUSERDllExport
RTIBool Monitor_Cmd_initialize_ex(
        Monitor_Cmd* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Monitor_Cmd_finalize(
        Monitor_Cmd* self);
                        
NDDSUSERDllExport
void Monitor_Cmd_finalize_ex(
        Monitor_Cmd* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Monitor_Cmd_copy(
        Monitor_Cmd* dst,
        const Monitor_Cmd* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif


#ifdef __cplusplus

    extern "C" {

#endif

extern void getMonitor_CmdInfo(NDDS_OBJ *pObj);

#ifdef __cplusplus

}

#endif


#endif /* Monitor_Cmd_h */
