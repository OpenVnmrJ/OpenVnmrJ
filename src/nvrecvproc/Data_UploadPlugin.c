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



#ifndef Data_UploadPlugin_h
#include "Data_UploadPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Data_UploadPlugin_serialize_data(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->type)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->elemId)) {
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
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->deserializerFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->pPrivateIssueData)) {
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



RTIBool Data_UploadPlugin_serialize(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Data_UploadPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Data_UploadPlugin_deserialize_data(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    if (!RTICdrStream_deserializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->type)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->sn)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->elemId)) {
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
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->deserializerFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->pPrivateIssueData)) {
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


RTIBool Data_UploadPlugin_deserialize(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Data_UploadPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Data_UploadPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveSequenceMaxSizeSerialized(
        current_alignment, ((MAX_FIXCODE_SIZE)), RTI_CDR_OCTET_TYPE);
            
    return current_alignment - initial_alignment;
}



RTIBool Data_UploadPlugin_serialize_key(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


RTIBool Data_UploadPlugin_deserialize_key(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    if (!RTICdrStream_deserializeLong(
        stream, &sample->key)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


unsigned int Data_UploadPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    return current_alignment - initial_alignment;
}



unsigned int Data_UploadPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Data_UploadPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Data_UploadPlugin_print(
    const Data_Upload *sample,
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
            
    RTICdrType_printLong(
        &sample->type, "type", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->sn, "sn", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->elemId, "elemId", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->totalBytes, "totalBytes", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->dataOffset, "dataOffset", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->crc32chksum, "crc32chksum", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->deserializerFlag, "deserializerFlag", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->pPrivateIssueData, "pPrivateIssueData", indent_level + 1);
            
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


Data_Upload *Data_UploadPlugin_create_sample()
{
    return Data_UploadPlugin_create_sample_ex(RTI_TRUE);
}
        

Data_Upload *Data_UploadPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Data_Upload *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Data_Upload);
                
    if (sample != NULL) {
        if (!Data_Upload_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Data_UploadPlugin_delete_sample(
    Data_Upload *sample)
{
    Data_UploadPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Data_UploadPlugin_delete_sample_ex(
    Data_Upload *sample,RTIBool deletePointers)
{
    Data_Upload_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Data_UploadPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_USER_KEY;
    
}


Data_UploadKeyHolder *Data_UploadPlugin_create_key()
{
    /* Note: If the definition of Data_UploadKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Data_UploadPlugin_create_sample();
}


void Data_UploadPlugin_delete_key(
    Data_UploadKeyHolder *key)
{
    /* Note: If the definition of Data_UploadKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Data_UploadPlugin_delete_sample(key);
}


RTIBool Data_UploadPlugin_instance_to_key(
    Data_UploadKeyHolder *dst, const Data_Upload *src)
{

    if (!RTICdrType_copyLong(
        &dst->key, &src->key)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Data_UploadPlugin_key_to_instance(
    Data_Upload *dst, const Data_UploadKeyHolder *src)
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
RTIBool Data_UploadPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Data_Upload *instance)
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

struct PRESTypePlugin *Data_UploadPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Data_UploadPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Data_UploadPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Data_UploadPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Data_UploadPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Data_Upload_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Data_UploadPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Data_UploadPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Data_UploadPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Data_UploadPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Data_UploadPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Data_UploadPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Data_UploadPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Data_UploadPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Data_Upload_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Data_Upload_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Data_Upload_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Data_Upload_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Data_UploadPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Data_UploadPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Data_UploadPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Data_UploadPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Data_UploadPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Data_UploadPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Data_UploadPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Data_UploadPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Data_UploadPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

