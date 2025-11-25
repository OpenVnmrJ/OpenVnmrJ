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



#ifndef Data_Upload_h
#include "Data_Upload.h"
#endif

/* ========================================================================= */
const char *Data_UploadTYPENAME = "Data_Upload";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Data_UploadTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Data_UploadTypeSupport_create_data_ex(DDS_Boolean);

void getData_UploadInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Data_UploadTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Data_UploadTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Data_UploadTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Data_Upload_initialize(
    Data_Upload* sample) {
    return Data_Upload_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Data_Upload_initialize_ex(
    Data_Upload* sample,RTIBool allocatePointers)
{
    if (!RTICdrType_initLong(&sample->key)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->type)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->sn)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->elemId)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->totalBytes)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->dataOffset)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->crc32chksum)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->deserializerFlag)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->pPrivateIssueData)) {
        return RTI_FALSE;
    }                
            
    DDS_OctetSeq_initialize(&sample->data);
                
    if (!DDS_OctetSeq_set_maximum(&sample->data,
            ((MAX_FIXCODE_SIZE)))) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Data_Upload_finalize(
    Data_Upload* sample)
{
    Data_Upload_finalize_ex(sample,RTI_TRUE);
}
        
void Data_Upload_finalize_ex(
    Data_Upload* sample,RTIBool deletePointers)
{

    DDS_OctetSeq_finalize(&sample->data);
            
}

RTIBool Data_Upload_copy(
    Data_Upload* dst,
    const Data_Upload* src)
{

    if (!RTICdrType_copyLong(
        &dst->key, &src->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->type, &src->type)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->sn, &src->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->elemId, &src->elemId)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->totalBytes, &src->totalBytes)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->dataOffset, &src->dataOffset)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->crc32chksum, &src->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->deserializerFlag, &src->deserializerFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->pPrivateIssueData, &src->pPrivateIssueData)) {
        return RTI_FALSE;
    }
            
    if (!DDS_OctetSeq_copy_no_alloc(&dst->data,
                                          &src->data)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


/**
 * <<IMPLEMENTATION>>
 *
 * Defines:  TSeq, T
 *
 * Configure and implement 'Data_Upload' sequence class.
 */
#define T Data_Upload
#define TSeq Data_UploadSeq
#define T_initialize_ex Data_Upload_initialize_ex
#define T_finalize_ex   Data_Upload_finalize_ex
#define T_copy       Data_Upload_copy

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

