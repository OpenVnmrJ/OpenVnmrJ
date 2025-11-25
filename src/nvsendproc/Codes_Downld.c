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



#ifndef Codes_Downld_h
#include "Codes_Downld.h"
#endif

/* ========================================================================= */
const char *Codes_DownldTYPENAME = "Codes_Downld";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Codes_DownldTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Codes_DownldTypeSupport_create_data_ex(DDS_Boolean);

void getCodes_DownldInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Codes_DownldTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Codes_DownldTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Codes_DownldTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */



    
    

RTIBool Codes_Downld_initialize(
    Codes_Downld* sample) {
    return Codes_Downld_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Codes_Downld_initialize_ex(
    Codes_Downld* sample,RTIBool allocatePointers)
{
    if (!RTICdrType_initLong(&sample->key)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->cmdtype)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->status)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->totalBytes)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->dataOffset)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->sn)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->ackInterval)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->crc32chksum)) {
        return RTI_FALSE;
    }                
            
    sample->nodeId = DDS_String_alloc(((MAX_CNTLRSTR_SIZE)));
    if (sample->nodeId == NULL) {
        return RTI_FALSE;
    }
            
    sample->label = DDS_String_alloc(((MAX_STR_SIZE)));
    if (sample->label == NULL) {
        return RTI_FALSE;
    }
            
    sample->msgstr = DDS_String_alloc(((MAX_STR_SIZE)));
    if (sample->msgstr == NULL) {
        return RTI_FALSE;
    }
            
    DDS_OctetSeq_initialize(&sample->data);
                
    if (!DDS_OctetSeq_set_maximum(&sample->data,
            ((MAX_FIXCODE_SIZE)))) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Codes_Downld_finalize(
    Codes_Downld* sample)
{
    Codes_Downld_finalize_ex(sample,RTI_TRUE);
}
        
void Codes_Downld_finalize_ex(
    Codes_Downld* sample,RTIBool deletePointers)
{

    DDS_String_free(sample->nodeId);                
            
    DDS_String_free(sample->label);                
            
    DDS_String_free(sample->msgstr);                
            
    DDS_OctetSeq_finalize(&sample->data);
            
}

RTIBool Codes_Downld_copy(
    Codes_Downld* dst,
    const Codes_Downld* src)
{

    if (!RTICdrType_copyLong(
        &dst->key, &src->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->cmdtype, &src->cmdtype)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->status, &src->status)) {
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
        &dst->sn, &src->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->ackInterval, &src->ackInterval)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyUnsignedLong(
        &dst->crc32chksum, &src->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->nodeId, src->nodeId, ((MAX_CNTLRSTR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->label, src->label, ((MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->msgstr, src->msgstr, ((MAX_STR_SIZE)) + 1)) {
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
 * Configure and implement 'Codes_Downld' sequence class.
 */
#define T Codes_Downld
#define TSeq Codes_DownldSeq
#define T_initialize_ex Codes_Downld_initialize_ex
#define T_finalize_ex   Codes_Downld_finalize_ex
#define T_copy       Codes_Downld_copy

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

