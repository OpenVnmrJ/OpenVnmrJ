
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Monitor_Cmd.idl using "rtiddsgen".
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



#ifndef Monitor_CmdPlugin_h
#include "Monitor_CmdPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Monitor_CmdPlugin_serialize_data(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->arg1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->arg2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->arg3)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->arg4)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->arg5)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeUnsignedLong(
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (sample->msgstr == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->msgstr, ((CMD_MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}



RTIBool Monitor_CmdPlugin_serialize(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Monitor_CmdPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Monitor_CmdPlugin_deserialize_data(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option)
{

    if (!RTICdrStream_deserializeLong(
        stream, &sample->cmd)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->arg1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->arg2)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->arg3)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->arg4)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->arg5)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeUnsignedLong(
        stream, &sample->crc32chksum)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->msgstr, ((CMD_MAX_STR_SIZE)) + 1)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Monitor_CmdPlugin_deserialize(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Monitor_CmdPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Monitor_CmdPlugin_get_max_size_serialized_data(
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
            
    current_alignment +=  RTICdrType_getUnsignedLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CMD_MAX_STR_SIZE)) + 1);
            
    return current_alignment - initial_alignment;
}



RTIBool Monitor_CmdPlugin_serialize_key(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option)
{


    return RTI_TRUE;
}


RTIBool Monitor_CmdPlugin_deserialize_key(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option)
{

    return RTI_TRUE;
}


unsigned int Monitor_CmdPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    return current_alignment - initial_alignment;
}



unsigned int Monitor_CmdPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Monitor_CmdPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Monitor_CmdPlugin_print(
    const Monitor_Cmd *sample,
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
        &sample->cmd, "cmd", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->arg1, "arg1", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->arg2, "arg2", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->arg3, "arg3", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->arg4, "arg4", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->arg5, "arg5", indent_level + 1);
            
    RTICdrType_printUnsignedLong(
        &sample->crc32chksum, "crc32chksum", indent_level + 1);
            
    if (&sample->msgstr==NULL) {
        RTICdrType_printString(
            NULL, "msgstr", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->msgstr, "msgstr", indent_level + 1);                
    }
            
}

/* ------------------------------------------------------------------------
 * Lifecycle Methods
 * ------------------------------------------------------------------------ */


Monitor_Cmd *Monitor_CmdPlugin_create_sample()
{
    return Monitor_CmdPlugin_create_sample_ex(RTI_TRUE);
}
        

Monitor_Cmd *Monitor_CmdPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Monitor_Cmd *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Monitor_Cmd);
                
    if (sample != NULL) {
        if (!Monitor_Cmd_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Monitor_CmdPlugin_delete_sample(
    Monitor_Cmd *sample)
{
    Monitor_CmdPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Monitor_CmdPlugin_delete_sample_ex(
    Monitor_Cmd *sample,RTIBool deletePointers)
{
    Monitor_Cmd_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Monitor_CmdPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_NO_KEY;
    
}


Monitor_CmdKeyHolder *Monitor_CmdPlugin_create_key()
{
    /* Note: If the definition of Monitor_CmdKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Monitor_CmdPlugin_create_sample();
}


void Monitor_CmdPlugin_delete_key(
    Monitor_CmdKeyHolder *key)
{
    /* Note: If the definition of Monitor_CmdKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Monitor_CmdPlugin_delete_sample(key);
}


RTIBool Monitor_CmdPlugin_instance_to_key(
    Monitor_CmdKeyHolder *dst, const Monitor_Cmd *src)
{

    return RTI_TRUE;
}


RTIBool Monitor_CmdPlugin_key_to_instance(
    Monitor_Cmd *dst, const Monitor_CmdKeyHolder *src)
{

    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Monitor_CmdPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Monitor_Cmd *instance)
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

struct PRESTypePlugin *Monitor_CmdPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Monitor_CmdPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Monitor_CmdPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Monitor_CmdPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Monitor_CmdPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Monitor_Cmd_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Monitor_CmdPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Monitor_CmdPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Monitor_CmdPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Monitor_CmdPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Monitor_CmdPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Monitor_CmdPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Monitor_CmdPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Monitor_CmdPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Monitor_Cmd_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Monitor_Cmd_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Monitor_Cmd_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Monitor_Cmd_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Monitor_CmdPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Monitor_CmdPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Monitor_CmdPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Monitor_CmdPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Monitor_CmdPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Monitor_CmdPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Monitor_CmdPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Monitor_CmdPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Monitor_CmdPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

