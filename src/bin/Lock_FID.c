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

  This file was generated from Lock_FID.idl using "rtiddsgen".
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



#ifndef Lock_FID_h
#include "Lock_FID.h"
#endif

/* ========================================================================= */
const char *Lock_FIDTYPENAME = "Lock_FID";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Lock_FIDTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Lock_FIDTypeSupport_create_data_ex(DDS_Boolean);

void getLock_FIDInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Lock_FIDTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Lock_FIDTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Lock_FIDTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Lock_FID_initialize(
    Lock_FID* sample) {
    return Lock_FID_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Lock_FID_initialize_ex(
    Lock_FID* sample,RTIBool allocatePointers)
{

    DDS_ShortSeq_initialize(&sample->lkfid);
                
    if (!DDS_ShortSeq_set_maximum(&sample->lkfid,
            ((LOCK_MAX_DATA)))) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Lock_FID_finalize(
    Lock_FID* sample)
{
    Lock_FID_finalize_ex(sample,RTI_TRUE);
}
        
void Lock_FID_finalize_ex(
    Lock_FID* sample,RTIBool deletePointers)
{

    DDS_ShortSeq_finalize(&sample->lkfid);
            
}

RTIBool Lock_FID_copy(
    Lock_FID* dst,
    const Lock_FID* src)
{

    if (!DDS_ShortSeq_copy_no_alloc(&dst->lkfid,
                                          &src->lkfid)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


/**
 * <<IMPLEMENTATION>>
 *
 * Defines:  TSeq, T
 *
 * Configure and implement 'Lock_FID' sequence class.
 */
#define T Lock_FID
#define TSeq Lock_FIDSeq
#define T_initialize_ex Lock_FID_initialize_ex
#define T_finalize_ex   Lock_FID_finalize_ex
#define T_copy       Lock_FID_copy

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

