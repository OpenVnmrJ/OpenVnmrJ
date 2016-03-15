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


#include <string.h>

#ifdef __cplusplus
#ifndef ndds_cpp_h
  #include "ndds/ndds_cpp.h"
#endif
#else
#ifndef ndds_c_h
  #include "ndds/ndds_c.h"
#endif
#endif

#ifndef osapi_type_h
  #include "osapi/osapi_type.h"
#endif
#ifndef osapi_heap_h
  #include "osapi/osapi_heap.h"
#endif

#ifndef osapi_utility_h
  #include "osapi/osapi_utility.h"
#endif

#ifndef cdr_type_h
  #include "cdr/cdr_type.h"
#endif

#ifndef cdr_type_h
  #include "cdr/cdr_encapsulation.h"
#endif

#ifndef cdr_stream_h
  #include "cdr/cdr_stream.h"
#endif

#ifndef pres_typePlugin_h
  #include "pres/pres_typePlugin.h"
#endif



#ifndef Console_StatPlugin_h
#include "Console_StatPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Console_StatPlugin_serialize_data(
    struct RTICdrStream *stream, const Console_Stat *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->dataTypeID)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqCtCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqFidCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqSample)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqLockFreqAP)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqLockFreq1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqLockFreq2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqSpinSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqSpinAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqSpinSpeedLimit)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqPneuBearing)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqPneuStatus)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqPneuVtAir)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqTickCountError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqChannelBitsConfig1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqChannelBitsConfig2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqChannelBitsActive1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->AcqChannelBitsActive2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqRcvrNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->Acqstate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqOpsComplCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqLSDVbits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqLockLevel)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqRecvGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqSpinSpan)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqSpinAdj)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqSpinMax)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqSpinActSp)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqSpinProfile)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqVTSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqVTAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqVTC)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqPneuVTAirLimits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqPneuSpinner)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqLockGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqLockPower)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqLockPhase)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->AcqShimValues, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqShimSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->AcqOpsComplFlags)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->rfMonError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->rfMonitor, (8), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->statblockRate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->gpaTuning, (9), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->gpaError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->probeId1, (20), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->gradCoilId, (12), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->consoleID)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}



RTIBool Console_StatPlugin_serialize(
    struct RTICdrStream *stream, const Console_Stat *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Console_StatPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Console_StatPlugin_deserialize_data(
    struct RTICdrStream *stream, Console_Stat *sample,
    void *deserialize_option)
{

    if (!RTICdrStream_deserializeLong(
        stream, &sample->dataTypeID)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqCtCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqFidCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqSample)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqLockFreqAP)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqLockFreq1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqLockFreq2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqSpinSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqSpinAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqSpinSpeedLimit)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqPneuBearing)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqPneuStatus)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqPneuVtAir)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqTickCountError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqChannelBitsConfig1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqChannelBitsConfig2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqChannelBitsActive1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->AcqChannelBitsActive2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqRcvrNpErr)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->Acqstate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqOpsComplCnt)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqLSDVbits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqLockLevel)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqRecvGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqSpinSpan)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqSpinAdj)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqSpinMax)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqSpinActSp)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqSpinProfile)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqVTSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqVTAct)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqVTC)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqPneuVTAirLimits)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqPneuSpinner)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqLockGain)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqLockPower)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqLockPhase)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->AcqShimValues, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqShimSet)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->AcqOpsComplFlags)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->rfMonError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->rfMonitor, (8), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->statblockRate)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->gpaTuning, (9), RTI_CDR_SHORT_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->gpaError)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->probeId1, (20), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->gradCoilId, (12), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->consoleID)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Console_StatPlugin_deserialize(
    struct RTICdrStream *stream, Console_Stat *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Console_StatPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Console_StatPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_TYPE);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, (8), RTI_CDR_SHORT_TYPE);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, (9), RTI_CDR_SHORT_TYPE);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, (20), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, (12), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    return current_alignment - initial_alignment;
}



RTIBool Console_StatPlugin_serialize_key(
    struct RTICdrStream *stream, const Console_Stat *sample,
    void *serialize_option)
{


    return RTI_TRUE;
}


RTIBool Console_StatPlugin_deserialize_key(
    struct RTICdrStream *stream, Console_Stat *sample,
    void *deserialize_option)
{

    return RTI_TRUE;
}


unsigned int Console_StatPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    return current_alignment - initial_alignment;
}



unsigned int Console_StatPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Console_StatPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Console_StatPlugin_print(
    const Console_Stat *sample,
    const char* description, int indent_level)
{

    if (description != NULL) {
        RTICdrType_printIndent(indent_level);
        RTILog_debug("%s:\n", description);
    }

    if (sample == NULL) {
        RTICdrType_printIndent(indent_level + 1);
        RTILog_debug("NULL\n");
        return;
    }
        

    RTICdrType_printLong(
        &sample->dataTypeID, "dataTypeID", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqCtCnt, "AcqCtCnt", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqFidCnt, "AcqFidCnt", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqSample, "AcqSample", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqLockFreqAP, "AcqLockFreqAP", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqLockFreq1, "AcqLockFreq1", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqLockFreq2, "AcqLockFreq2", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqNpErr, "AcqNpErr", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqSpinSet, "AcqSpinSet", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqSpinAct, "AcqSpinAct", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqSpinSpeedLimit, "AcqSpinSpeedLimit", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqPneuBearing, "AcqPneuBearing", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqPneuStatus, "AcqPneuStatus", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqPneuVtAir, "AcqPneuVtAir", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqTickCountError, "AcqTickCountError", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqChannelBitsConfig1, "AcqChannelBitsConfig1", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqChannelBitsConfig2, "AcqChannelBitsConfig2", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqChannelBitsActive1, "AcqChannelBitsActive1", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->AcqChannelBitsActive2, "AcqChannelBitsActive2", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqRcvrNpErr, "AcqRcvrNpErr", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->Acqstate, "Acqstate", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqOpsComplCnt, "AcqOpsComplCnt", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqLSDVbits, "AcqLSDVbits", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqLockLevel, "AcqLockLevel", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqRecvGain, "AcqRecvGain", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqSpinSpan, "AcqSpinSpan", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqSpinAdj, "AcqSpinAdj", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqSpinMax, "AcqSpinMax", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqSpinActSp, "AcqSpinActSp", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqSpinProfile, "AcqSpinProfile", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqVTSet, "AcqVTSet", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqVTAct, "AcqVTAct", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqVTC, "AcqVTC", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqPneuVTAirLimits, "AcqPneuVTAirLimits", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqPneuSpinner, "AcqPneuSpinner", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqLockGain, "AcqLockGain", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqLockPower, "AcqLockPower", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqLockPhase, "AcqLockPhase", indent_level + 1);
            
    RTICdrType_printArray(
        sample->AcqShimValues, ((MAX_SHIMS_CONFIGURED)), RTI_CDR_SHORT_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printShort,
        "AcqShimValues", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqShimSet, "AcqShimSet", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->AcqOpsComplFlags, "AcqOpsComplFlags", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->rfMonError, "rfMonError", indent_level + 1);
            
    RTICdrType_printArray(
        sample->rfMonitor, (8), RTI_CDR_SHORT_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printShort,
        "rfMonitor", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->statblockRate, "statblockRate", indent_level + 1);
            
    RTICdrType_printArray(
        sample->gpaTuning, (9), RTI_CDR_SHORT_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printShort,
        "gpaTuning", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->gpaError, "gpaError", indent_level + 1);
            
    RTICdrType_printArray(
        sample->probeId1, (20), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "probeId1", indent_level + 1);
            
    RTICdrType_printArray(
        sample->gradCoilId, (12), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "gradCoilId", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->consoleID, "consoleID", indent_level + 1);
            
}

/* ------------------------------------------------------------------------
 * Lifecycle Methods
 * ------------------------------------------------------------------------ */


Console_Stat *Console_StatPlugin_create_sample()
{
    return Console_StatPlugin_create_sample_ex(RTI_TRUE);
}
        

Console_Stat *Console_StatPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Console_Stat *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Console_Stat);
                
    if (sample != NULL) {
        if (!Console_Stat_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Console_StatPlugin_delete_sample(
    Console_Stat *sample)
{
    Console_StatPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Console_StatPlugin_delete_sample_ex(
    Console_Stat *sample,RTIBool deletePointers)
{
    Console_Stat_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Console_StatPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_NO_KEY;
    
}


Console_StatKeyHolder *Console_StatPlugin_create_key()
{
    /* Note: If the definition of Console_StatKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Console_StatPlugin_create_sample();
}


void Console_StatPlugin_delete_key(
    Console_StatKeyHolder *key)
{
    /* Note: If the definition of Console_StatKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Console_StatPlugin_delete_sample(key);
}


RTIBool Console_StatPlugin_instance_to_key(
    Console_StatKeyHolder *dst, const Console_Stat *src)
{

    return RTI_TRUE;
}


RTIBool Console_StatPlugin_key_to_instance(
    Console_Stat *dst, const Console_StatKeyHolder *src)
{

    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Console_StatPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Console_Stat *instance)
{
    int idIndex;

    idIndex = 3;

    id->value[3] = 0;
    id->value[2] = 0;
    id->value[1] = 0;
    id->value[0] = 0;


    /* By default, NDDSGen assumes that the key specified in the type
     * fully identifies instances of that type. If you would like to
     * further discriminate among instances based on the participant
     * from which they originate, set this out parameter to RTI_FALSE. In
     * that case, only set the value at the last index of the instance ID;
     * the first three values will be set automatically to the RTPS host ID and
     * application ID of the writer's domain participant. (See the
     * documentation for DDS_WireProtocolQosPolicy for a description of
     * the host ID and app ID.)
     */
    *is_unique = RTI_TRUE;
    return RTI_TRUE;
}












/* ------------------------------------------------------------------------
 * Plug-in Installation Methods
 * ------------------------------------------------------------------------ */

struct PRESTypePlugin *Console_StatPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Console_StatPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Console_StatPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Console_StatPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Console_StatPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Console_Stat_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Console_StatPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Console_StatPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_StatPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_StatPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Console_StatPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Console_StatPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Console_StatPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Console_StatPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Console_Stat_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Console_Stat_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Console_Stat_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Console_Stat_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Console_StatPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Console_StatPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Console_StatPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Console_StatPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Console_StatPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Console_StatPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Console_StatPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_StatPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Console_StatPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

