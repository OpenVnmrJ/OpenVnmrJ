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

  This file was generated from Console_Stat.idl using "rtiddsgen".
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



#ifndef Console_Stat_h
#include "Console_Stat.h"
#endif

/* ========================================================================= */
const char *Console_StatTYPENAME = "Console_Stat";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Console_StatTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Console_StatTypeSupport_create_data_ex(DDS_Boolean);

void getConsole_StatInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Console_StatTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Console_StatTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Console_StatTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Console_Stat_initialize(
    Console_Stat* sample) {
    return Console_Stat_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Console_Stat_initialize_ex(
    Console_Stat* sample,RTIBool allocatePointers)
{

    if (!RTICdrType_initLong(&sample->dataTypeID)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqCtCnt)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqFidCnt)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqSample)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqLockFreqAP)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqLockFreq1)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqLockFreq2)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqNpErr)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqSpinSet)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqSpinAct)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqSpinSpeedLimit)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqPneuBearing)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqPneuStatus)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqPneuVtAir)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqTickCountError)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqChannelBitsConfig1)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqChannelBitsConfig2)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqChannelBitsActive1)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->AcqChannelBitsActive2)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqRcvrNpErr)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->Acqstate)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqOpsComplCnt)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqLSDVbits)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqLockLevel)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqRecvGain)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqSpinSpan)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqSpinAdj)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqSpinMax)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqSpinActSp)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqSpinProfile)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqVTSet)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqVTAct)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqVTC)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqPneuVTAirLimits)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqPneuSpinner)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqLockGain)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqLockPower)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqLockPhase)) {
        return RTI_FALSE;
    }                
                
    if (!RTICdrType_initArray(
        sample->AcqShimValues, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_initShort(&sample->AcqShimSet)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->AcqOpsComplFlags)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initShort(&sample->rfMonError)) {
        return RTI_FALSE;
    }                
                
    if (!RTICdrType_initArray(
        sample->rfMonitor, (8), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_initShort(&sample->statblockRate)) {
        return RTI_FALSE;
    }                
                
    if (!RTICdrType_initArray(
        sample->gpaTuning, (9), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_initShort(&sample->gpaError)) {
        return RTI_FALSE;
    }                
                
    if (!RTICdrType_initArray(
        sample->probeId1, (20), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
                
    if (!RTICdrType_initArray(
        sample->gradCoilId, (12), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_initShort(&sample->consoleID)) {
        return RTI_FALSE;
    }                
            

    return RTI_TRUE;
}

void Console_Stat_finalize(
    Console_Stat* sample)
{
    Console_Stat_finalize_ex(sample,RTI_TRUE);
}
        
void Console_Stat_finalize_ex(
    Console_Stat* sample,RTIBool deletePointers)
{

}

RTIBool Console_Stat_copy(
    Console_Stat* dst,
    const Console_Stat* src)
{

    if (!RTICdrType_copyLong(
        &dst->dataTypeID, &src->dataTypeID)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqCtCnt, &src->AcqCtCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqFidCnt, &src->AcqFidCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqSample, &src->AcqSample)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqLockFreqAP, &src->AcqLockFreqAP)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqLockFreq1, &src->AcqLockFreq1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqLockFreq2, &src->AcqLockFreq2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqNpErr, &src->AcqNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqSpinSet, &src->AcqSpinSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqSpinAct, &src->AcqSpinAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqSpinSpeedLimit, &src->AcqSpinSpeedLimit)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqPneuBearing, &src->AcqPneuBearing)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqPneuStatus, &src->AcqPneuStatus)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqPneuVtAir, &src->AcqPneuVtAir)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqTickCountError, &src->AcqTickCountError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqChannelBitsConfig1, &src->AcqChannelBitsConfig1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqChannelBitsConfig2, &src->AcqChannelBitsConfig2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqChannelBitsActive1, &src->AcqChannelBitsActive1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->AcqChannelBitsActive2, &src->AcqChannelBitsActive2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqRcvrNpErr, &src->AcqRcvrNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->Acqstate, &src->Acqstate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqOpsComplCnt, &src->AcqOpsComplCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqLSDVbits, &src->AcqLSDVbits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqLockLevel, &src->AcqLockLevel)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqRecvGain, &src->AcqRecvGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqSpinSpan, &src->AcqSpinSpan)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqSpinAdj, &src->AcqSpinAdj)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqSpinMax, &src->AcqSpinMax)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqSpinActSp, &src->AcqSpinActSp)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqSpinProfile, &src->AcqSpinProfile)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqVTSet, &src->AcqVTSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqVTAct, &src->AcqVTAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqVTC, &src->AcqVTC)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqPneuVTAirLimits, &src->AcqPneuVTAirLimits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqPneuSpinner, &src->AcqPneuSpinner)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqLockGain, &src->AcqLockGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqLockPower, &src->AcqLockPower)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqLockPhase, &src->AcqLockPhase)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->AcqShimValues, src->AcqShimValues, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqShimSet, &src->AcqShimSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->AcqOpsComplFlags, &src->AcqOpsComplFlags)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->rfMonError, &src->rfMonError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->rfMonitor, src->rfMonitor, (8), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->statblockRate, &src->statblockRate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->gpaTuning, src->gpaTuning, (9), RTI_CDR_SHORT_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->gpaError, &src->gpaError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->probeId1, src->probeId1, (20), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyArray(
        dst->gradCoilId, src->gradCoilId, (12), RTI_CDR_CHAR_SIZE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyShort(
        &dst->consoleID, &src->consoleID)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


/**
 * <<IMPLEMENTATION>>
 *
 * Defines:  TSeq, T
 *
 * Configure and implement 'Console_Stat' sequence class.
 */
#define T Console_Stat
#define TSeq Console_StatSeq
#define T_initialize_ex Console_Stat_initialize_ex
#define T_finalize_ex   Console_Stat_finalize_ex
#define T_copy       Console_Stat_copy

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

