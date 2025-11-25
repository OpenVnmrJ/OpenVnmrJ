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

  This file was generated from Monitor_Cmd.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/


#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
        #ifndef dds_c_log_impl_h              
            #include "dds_c/dds_c_log_impl.h"                                
        #endif        
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
    
    #ifndef cdr_type_h
        #include "cdr/cdr_type.h"
    #endif    

    #ifndef osapi_heap_h
        #include "osapi/osapi_heap.h" 
    #endif
#else
    #include "ndds_standalone_type.h"
#endif



#ifndef Monitor_Cmd_h
#include "Monitor_Cmd.h"
#endif

/* ========================================================================= */
const char *Monitor_CmdTYPENAME = "Monitor_Cmd";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Monitor_CmdTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Monitor_CmdTypeSupport_create_data_ex(DDS_Boolean);

void getMonitor_CmdInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Monitor_CmdTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Monitor_CmdTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Monitor_CmdTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Monitor_Cmd_initialize(
    Monitor_Cmd* sample) {
    return Monitor_Cmd_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Monitor_Cmd_initialize_ex(
    Monitor_Cmd* sample,RTIBool allocatePointers)
{

    if (!RTICdrType_initLong(&sample->cmd)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->arg1)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->arg2)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->arg3)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->arg4)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->arg5)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->crc32chksum)) {
        return RTI_FALSE;
    }                
            
    sample->msgstr = DDS_String_alloc(((CMD_MAX_STR_SIZE)));
    if (sample->msgstr == NULL) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Monitor_Cmd_finalize(
    Monitor_Cmd* sample)
{
    Monitor_Cmd_finalize_ex(sample,RTI_TRUE);
}
        
void Monitor_Cmd_finalize_ex(
    Monitor_Cmd* sample,RTIBool deletePointers)
{

    DDS_String_free(sample->msgstr);                
            
}

RTIBool Monitor_Cmd_copy(
    Monitor_Cmd* dst,
    const Monitor_Cmd* src)
{

    if (!RTICdrType_copyLong(
        &dst->cmd, &src->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->arg1, &src->arg1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->arg2, &src->arg2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->arg3, &src->arg3)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->arg4, &src->arg4)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->arg5, &src->arg5)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->crc32chksum, &src->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->msgstr, src->msgstr, ((CMD_MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


/**
 * <<IMPLEMENTATION>>
 *
 * Defines:  TSeq, T
 *
 * Configure and implement 'Monitor_Cmd' sequence class.
 */
#define T Monitor_Cmd
#define TSeq Monitor_CmdSeq
#define T_initialize_ex Monitor_Cmd_initialize_ex
#define T_finalize_ex   Monitor_Cmd_finalize_ex
#define T_copy       Monitor_Cmd_copy

#ifndef NDDS_STANDALONE_TYPE
#include "dds_c/generic/dds_c_sequence_TSeq.gen"
#ifdef __cplusplus
#include "dds_cpp/generic/dds_cpp_sequence_TSeq.gen"
#endif
#else
#include "dds_c_sequence_TSeq.gen"
#ifdef __cplusplus
#include "dds_cpp_sequence_TSeq.gen"
#endif
#endif

#undef T_copy
#undef T_finalize_ex
#undef T_initialize_ex
#undef TSeq
#undef T

