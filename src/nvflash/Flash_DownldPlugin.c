
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Flash_Downld.idl using "rtiddsgen".
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



#ifndef Flash_DownldPlugin_h
#include "Flash_DownldPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Flash_DownldPlugin_serialize_data(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->cntlrId, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
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
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->md5digest, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->filename, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializePrimitiveArray(
        stream, (void*)sample->msgstr, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (DDS_OctetSeq_get_contiguous_bufferI(&sample->data) != NULL) {
        if (!RTICdrStream_serializePrimitiveSequence(
            stream,
            DDS_OctetSeq_get_contiguous_bufferI(&sample->data),
            DDS_OctetSeq_get_length(&sample->data),
            ((MAX_FLASHDATA_SIZE)),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    } else {
        if (!RTICdrStream_serializePrimitivePointerSequence(
            stream,
            (const void **)DDS_OctetSeq_get_discontiguous_bufferI(&sample->data),
            DDS_OctetSeq_get_length(&sample->data),
            ((MAX_FLASHDATA_SIZE)),
            RTI_CDR_OCTET_TYPE)) {
            return RTI_FALSE;
        }
    }
            
    return RTI_TRUE;
}



RTIBool Flash_DownldPlugin_serialize(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Flash_DownldPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Flash_DownldPlugin_deserialize_data(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option)
{

    RTICdrUnsignedLong sequence_length = 0;

    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->cntlrId, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
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
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->md5digest, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->filename, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_TYPE)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializePrimitiveArray(
        stream, (void*)sample->msgstr, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_TYPE)) {
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


RTIBool Flash_DownldPlugin_deserialize(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Flash_DownldPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Flash_DownldPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getPrimitiveArrayMaxSizeSerialized(
        current_alignment, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_TYPE);
            
    current_alignment +=  RTICdrType_getPrimitiveSequenceMaxSizeSerialized(
        current_alignment, ((MAX_FLASHDATA_SIZE)), RTI_CDR_OCTET_TYPE);
            
    return current_alignment - initial_alignment;
}



RTIBool Flash_DownldPlugin_serialize_key(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option)
{


    return RTI_TRUE;
}


RTIBool Flash_DownldPlugin_deserialize_key(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option)
{
    return RTI_TRUE;
}


unsigned int Flash_DownldPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    return current_alignment - initial_alignment;
}



unsigned int Flash_DownldPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Flash_DownldPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Flash_DownldPlugin_print(
    const Flash_Downld *sample,
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
        

    RTICdrType_printArray(
        sample->cntlrId, ((FLASH_MAX_CNTLRID_SIZE)), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "cntlrId", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->cmd, "cmd", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->status, "status", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->totalBytes, "totalBytes", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->dataOffset, "dataOffset", indent_level + 1);
            
    RTICdrType_printArray(
        sample->md5digest, ((FLASH_MAX_MD5_SIZE)), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "md5digest", indent_level + 1);
            
    RTICdrType_printArray(
        sample->filename, ((FLASH_MAX_NAME_SIZE)), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "filename", indent_level + 1);
            
    RTICdrType_printArray(
        sample->msgstr, ((FLASH_MAX_STR_SIZE)), RTI_CDR_CHAR_SIZE,
        (RTICdrTypePrintFunction)RTICdrType_printChar,
        "msgstr", indent_level + 1);
            
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


Flash_Downld *Flash_DownldPlugin_create_sample()
{
    return Flash_DownldPlugin_create_sample_ex(RTI_TRUE);
}
        

Flash_Downld *Flash_DownldPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Flash_Downld *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Flash_Downld);
                
    if (sample != NULL) {
        if (!Flash_Downld_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Flash_DownldPlugin_delete_sample(
    Flash_Downld *sample)
{
    Flash_DownldPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Flash_DownldPlugin_delete_sample_ex(
    Flash_Downld *sample,RTIBool deletePointers)
{
    Flash_Downld_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Flash_DownldPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_NO_KEY;
    
}


Flash_DownldKeyHolder *Flash_DownldPlugin_create_key()
{
    /* Note: If the definition of Flash_DownldKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Flash_DownldPlugin_create_sample();
}


void Flash_DownldPlugin_delete_key(
    Flash_DownldKeyHolder *key)
{
    /* Note: If the definition of Flash_DownldKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Flash_DownldPlugin_delete_sample(key);
}


RTIBool Flash_DownldPlugin_instance_to_key(
    Flash_DownldKeyHolder *dst, const Flash_Downld *src)
{

    return RTI_TRUE;
}


RTIBool Flash_DownldPlugin_key_to_instance(
    Flash_Downld *dst, const Flash_DownldKeyHolder *src)
{

    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Flash_DownldPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Flash_Downld *instance)
{
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

struct PRESTypePlugin *Flash_DownldPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Flash_DownldPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Flash_DownldPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Flash_DownldPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Flash_DownldPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Flash_Downld_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Flash_DownldPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Flash_DownldPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Flash_DownldPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Flash_DownldPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Flash_DownldPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Flash_DownldPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Flash_DownldPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Flash_DownldPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Flash_Downld_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Flash_Downld_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Flash_Downld_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Flash_Downld_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Flash_DownldPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Flash_DownldPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Flash_DownldPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Flash_DownldPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Flash_DownldPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Flash_DownldPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Flash_DownldPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Flash_DownldPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Flash_DownldPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

