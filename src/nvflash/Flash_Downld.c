
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Flash_Downld.idl using "rtiddsgen".
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



#ifndef Flash_Downld_h
#include "Flash_Downld.h"
#endif

/* ========================================================================= */
const char *Flash_DownldTYPENAME = "Flash_Downld";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Flash_DownldTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Flash_DownldTypeSupport_create_data_ex(DDS_Boolean);

void getFlash_DownldInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Flash_DownldTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Flash_DownldTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Flash_DownldTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Flash_Downld_initialize(
    Flash_Downld* sample) {
    return Flash_Downld_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Flash_Downld_initialize_ex(
    Flash_Downld* sample,RTIBool allocatePointers)
{

    if (!RTICdrType_initArray(
        sample->cntlrId, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_initLong(&sample->cmd)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->status)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->totalBytes)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initUnsignedLong(&sample->dataOffset)) {
        return RTI_FALSE;
    }                
                
    if (!RTICdrType_initArray(
        sample->md5digest, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
                
    if (!RTICdrType_initArray(
        sample->filename, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
                
    if (!RTICdrType_initArray(
        sample->msgstr, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    DDS_OctetSeq_initialize(&sample->data);
                
    if (!DDS_OctetSeq_set_maximum(&sample->data,
            ((MAX_FLASHDATA_SIZE)))) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Flash_Downld_finalize(
    Flash_Downld* sample)
{
    Flash_Downld_finalize_ex(sample,RTI_TRUE);
}
        
void Flash_Downld_finalize_ex(
    Flash_Downld* sample,RTIBool deletePointers)
{

    DDS_OctetSeq_finalize(&sample->data);
            
}

RTIBool Flash_Downld_copy(
    Flash_Downld* dst,
    const Flash_Downld* src)
{

    if (!RTICdrType_copyArray(
        dst->cntlrId, src->cntlrId, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->cmd, &src->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
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
            
    if (!RTICdrType_copyArray(
        dst->md5digest, src->md5digest, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->filename, src->filename, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->msgstr, src->msgstr, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_SIZE)) {
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
 * Configure and implement 'Flash_Downld' sequence class.
 */
#define T Flash_Downld
#define TSeq Flash_DownldSeq
#define T_initialize_ex Flash_Downld_initialize_ex
#define T_finalize_ex   Flash_Downld_finalize_ex
#define T_copy       Flash_Downld_copy

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

