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



#ifndef Codes_DownldPlugin_h
#include "Codes_DownldPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Codes_DownldPlugin_serialize_data(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->cmdtype)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeShort(
        stream, &sample->status)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->totalBytes)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->dataOffset)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->ackInterval)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (sample->nodeId == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->nodeId, ((MAX_CNTLRSTR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->label == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->label, ((MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->msgstr == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->msgstr, ((MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (DDS_OctetSeq_get_contiguous_bufferI(&sample->data) != NULL) {
        if (!RTICdrStream_serializePrimitiveSequence(
            stream,
            DDS_OctetSeq_get_contiguous_bufferI(&sample->data),
            DDS_OctetSeq_get_length(&sample->data),
            ((MAX_FIXCODE_SIZE)),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    } else {
        if (!RTICdrStream_serializePrimitivePointerSequence(
            stream,
            (const void **)DDS_OctetSeq_get_discontiguous_bufferI(&sample->data),
            DDS_OctetSeq_get_length(&sample->data),
            ((MAX_FIXCODE_SIZE)),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    }
            
    return RTI_TRUE;
}



RTIBool Codes_DownldPlugin_serialize(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Codes_DownldPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Codes_DownldPlugin_deserialize_data(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    if (!RTICdrStream_deserializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->cmdtype)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeShort(
        stream, &sample->status)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->totalBytes)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->dataOffset)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->ackInterval)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->nodeId, ((MAX_CNTLRSTR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->label, ((MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->msgstr, ((MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    if (DDS_OctetSeq_get_contiguous_bufferI(&sample->data) != NULL) {
        if (!RTICdrStream_deserializePrimitiveSequence(
            stream,
            DDS_OctetSeq_get_contiguous_bufferI(&sample->data),
            &sequence_length,
            DDS_OctetSeq_get_maximum(&sample->data),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    } else {
        if (!RTICdrStream_deserializePrimitivePointerSequence(
            stream,
            (void **)DDS_OctetSeq_get_discontiguous_bufferI(&sample->data),
            &sequence_length,
            DDS_OctetSeq_get_maximum(&sample->data),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    }
    if (!DDS_OctetSeq_set_length(&sample->data, sequence_length)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Codes_DownldPlugin_deserialize(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Codes_DownldPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Codes_DownldPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getShortMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((MAX_CNTLRSTR_SIZE)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((MAX_STR_SIZE)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((MAX_STR_SIZE)) + 1);
            
    current_alignment +=  RTICdrType_getPrimitiveSequenceMaxSizeSerialized(
        current_alignment, ((MAX_FIXCODE_SIZE)), RTI_CDR_OCTET_TYPE);
            
    return current_alignment - initial_alignment;
}



RTIBool Codes_DownldPlugin_serialize_key(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


RTIBool Codes_DownldPlugin_deserialize_key(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option)
{

    if (!RTICdrStream_deserializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


unsigned int Codes_DownldPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    return current_alignment - initial_alignment;
}



unsigned int Codes_DownldPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Codes_DownldPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Codes_DownldPlugin_print(
    const Codes_Downld *sample,
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
        &sample->key, "key", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->cmdtype, "cmdtype", indent_level + 1);
            
    RTICdrType_printShort(
        &sample->status, "status", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->totalBytes, "totalBytes", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->dataOffset, "dataOffset", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->sn, "sn", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->ackInterval, "ackInterval", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->crc32chksum, "crc32chksum", indent_level + 1);
            
    if (&sample->nodeId==NULL) {
        RTICdrType_printString(
            NULL, "nodeId", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->nodeId, "nodeId", indent_level + 1);                
    }
            
    if (&sample->label==NULL) {
        RTICdrType_printString(
            NULL, "label", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->label, "label", indent_level + 1);                
    }
            
    if (&sample->msgstr==NULL) {
        RTICdrType_printString(
            NULL, "msgstr", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->msgstr, "msgstr", indent_level + 1);                
    }
            
    if (&sample->data == NULL) {
        RTICdrType_printIndent(indent_level+1);
        RTILog_debug("data: NULL\n");    
    } else {
    
        if (DDS_OctetSeq_get_contiguous_bufferI(&sample->data) != NULL) {
            RTICdrType_printArray(
                DDS_OctetSeq_get_contiguous_bufferI(&sample->data),
                DDS_OctetSeq_get_length(&sample->data),
                RTI_CDR_OCTET_SIZE,
                (RTICdrTypePrintFunction)RTICdrType_printOctet,
                "data", indent_level + 1);
        } else {
            RTICdrType_printPointerArray(
                DDS_OctetSeq_get_discontiguous_bufferI(&sample->data),
                DDS_OctetSeq_get_length(&sample->data),
               (RTICdrTypePrintFunction)RTICdrType_printOctet,
               "data", indent_level + 1);
        }
    
    }
            
}

/* ------------------------------------------------------------------------
 * Lifecycle Methods
 * ------------------------------------------------------------------------ */


Codes_Downld *Codes_DownldPlugin_create_sample()
{
    return Codes_DownldPlugin_create_sample_ex(RTI_TRUE);
}
        

Codes_Downld *Codes_DownldPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Codes_Downld *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Codes_Downld);
                
    if (sample != NULL) {
        if (!Codes_Downld_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Codes_DownldPlugin_delete_sample(
    Codes_Downld *sample)
{
    Codes_DownldPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Codes_DownldPlugin_delete_sample_ex(
    Codes_Downld *sample,RTIBool deletePointers)
{
    Codes_Downld_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Codes_DownldPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_USER_KEY;
    
}


Codes_DownldKeyHolder *Codes_DownldPlugin_create_key()
{
    /* Note: If the definition of Codes_DownldKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Codes_DownldPlugin_create_sample();
}


void Codes_DownldPlugin_delete_key(
    Codes_DownldKeyHolder *key)
{
    /* Note: If the definition of Codes_DownldKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Codes_DownldPlugin_delete_sample(key);
}


RTIBool Codes_DownldPlugin_instance_to_key(
    Codes_DownldKeyHolder *dst, const Codes_Downld *src)
{

    if (!RTICdrType_copyLong(
        &dst->key, &src->key)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Codes_DownldPlugin_key_to_instance(
    Codes_Downld *dst, const Codes_DownldKeyHolder *src)
{

    if (!RTICdrType_copyLong(
        &dst->key, &src->key)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Codes_DownldPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Codes_Downld *instance)
{
    int idIndex;

    idIndex = 3;

    id->value[3] = 0;
    id->value[2] = 0;
    id->value[1] = 0;
    id->value[0] = 0;

        
    /* Key field: key
     * Add this field to the instance ID.
     */
    id->value[idIndex--]
        = (unsigned int) instance->key;

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

struct PRESTypePlugin *Codes_DownldPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Codes_DownldPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Codes_DownldPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Codes_DownldPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Codes_DownldPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Codes_Downld_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Codes_DownldPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Codes_DownldPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Codes_DownldPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Codes_DownldPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Codes_DownldPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Codes_DownldPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Codes_DownldPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Codes_DownldPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Codes_Downld_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Codes_Downld_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Codes_Downld_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Codes_Downld_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Codes_DownldPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Codes_DownldPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Codes_DownldPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Codes_DownldPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Codes_DownldPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Codes_DownldPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Codes_DownldPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Codes_DownldPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Codes_DownldPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

