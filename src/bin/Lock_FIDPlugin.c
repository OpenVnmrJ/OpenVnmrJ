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



#ifndef Lock_FIDPlugin_h
#include "Lock_FIDPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Lock_FIDPlugin_serialize_data(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option)
{

    if (DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid) != NULL) {
        if (!RTICdrStream_serializePrimitiveSequence(
            stream,
            DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid),
            DDS_ShortSeq_get_length(&sample->lkfid),
            ((LOCK_MAX_DATA)),
            RTI_CDR_SHORT_TYPE)) {
            return RTI_FALSE;
        }
    } else {
        if (!RTICdrStream_serializePrimitivePointerSequence(
            stream,
            (const void **)DDS_ShortSeq_get_discontiguous_bufferI(&sample->lkfid),
            DDS_ShortSeq_get_length(&sample->lkfid),
            ((LOCK_MAX_DATA)),
            RTI_CDR_SHORT_TYPE)) {
            return RTI_FALSE;
        }
    }
            
    return RTI_TRUE;
}



RTIBool Lock_FIDPlugin_serialize(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Lock_FIDPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Lock_FIDPlugin_deserialize_data(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    if (DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid) != NULL) {
        if (!RTICdrStream_deserializePrimitiveSequence(
            stream,
            DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid),
            &sequence_length,
            DDS_ShortSeq_get_maximum(&sample->lkfid),
            RTI_CDR_SHORT_TYPE)) {
            return RTI_FALSE;
        }
    } else {
        if (!RTICdrStream_deserializePrimitivePointerSequence(
            stream,
            (void **)DDS_ShortSeq_get_discontiguous_bufferI(&sample->lkfid),
            &sequence_length,
            DDS_ShortSeq_get_maximum(&sample->lkfid),
            RTI_CDR_SHORT_TYPE)) {
            return RTI_FALSE;
        }
    }
    if (!DDS_ShortSeq_set_length(&sample->lkfid, sequence_length)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Lock_FIDPlugin_deserialize(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Lock_FIDPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Lock_FIDPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getPrimitiveSequenceMaxSizeSerialized(
        current_alignment, ((LOCK_MAX_DATA)), RTI_CDR_SHORT_TYPE);
            
    return current_alignment - initial_alignment;
}



RTIBool Lock_FIDPlugin_serialize_key(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option)
{


    return RTI_TRUE;
}


RTIBool Lock_FIDPlugin_deserialize_key(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    return RTI_TRUE;
}


unsigned int Lock_FIDPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    return current_alignment - initial_alignment;
}



unsigned int Lock_FIDPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Lock_FIDPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Lock_FIDPlugin_print(
    const Lock_FID *sample,
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
        

    if (&sample->lkfid == NULL) {
        RTICdrType_printIndent(indent_level+1);
        RTILog_debug("lkfid: NULL\n");    
    } else {
    
        if (DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid) != NULL) {
            RTICdrType_printArray(
                DDS_ShortSeq_get_contiguous_bufferI(&sample->lkfid),
                DDS_ShortSeq_get_length(&sample->lkfid),
                RTI_CDR_SHORT_SIZE,
                (RTICdrTypePrintFunction)RTICdrType_printShort,
                "lkfid", indent_level + 1);
        } else {
            RTICdrType_printPointerArray(
                DDS_ShortSeq_get_discontiguous_bufferI(&sample->lkfid),
                DDS_ShortSeq_get_length(&sample->lkfid),
               (RTICdrTypePrintFunction)RTICdrType_printShort,
               "lkfid", indent_level + 1);
        }
    
    }
            
}

/* ------------------------------------------------------------------------
 * Lifecycle Methods
 * ------------------------------------------------------------------------ */


Lock_FID *Lock_FIDPlugin_create_sample()
{
    return Lock_FIDPlugin_create_sample_ex(RTI_TRUE);
}
        

Lock_FID *Lock_FIDPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Lock_FID *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Lock_FID);
                
    if (sample != NULL) {
        if (!Lock_FID_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Lock_FIDPlugin_delete_sample(
    Lock_FID *sample)
{
    Lock_FIDPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Lock_FIDPlugin_delete_sample_ex(
    Lock_FID *sample,RTIBool deletePointers)
{
    Lock_FID_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Lock_FIDPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_NO_KEY;
    
}


Lock_FIDKeyHolder *Lock_FIDPlugin_create_key()
{
    /* Note: If the definition of Lock_FIDKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Lock_FIDPlugin_create_sample();
}


void Lock_FIDPlugin_delete_key(
    Lock_FIDKeyHolder *key)
{
    /* Note: If the definition of Lock_FIDKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Lock_FIDPlugin_delete_sample(key);
}


RTIBool Lock_FIDPlugin_instance_to_key(
    Lock_FIDKeyHolder *dst, const Lock_FID *src)
{

    return RTI_TRUE;
}


RTIBool Lock_FIDPlugin_key_to_instance(
    Lock_FID *dst, const Lock_FIDKeyHolder *src)
{

    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Lock_FIDPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Lock_FID *instance)
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

struct PRESTypePlugin *Lock_FIDPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Lock_FIDPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Lock_FIDPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Lock_FIDPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Lock_FIDPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Lock_FID_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Lock_FIDPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Lock_FIDPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Lock_FIDPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Lock_FIDPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Lock_FIDPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Lock_FIDPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Lock_FIDPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Lock_FIDPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Lock_FID_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Lock_FID_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Lock_FID_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Lock_FID_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Lock_FIDPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Lock_FIDPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Lock_FIDPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Lock_FIDPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Lock_FIDPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Lock_FIDPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Lock_FIDPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Lock_FIDPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Lock_FIDPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

