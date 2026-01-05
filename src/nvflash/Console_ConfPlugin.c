
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Console_Conf.idl using "rtiddsgen".
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



#ifndef Console_ConfPlugin_h
#include "Console_ConfPlugin.h"
#endif


/* ------------------------------------------------------------------------
 * (De)Serialization Methods
 * ------------------------------------------------------------------------ */


RTIBool Console_ConfPlugin_serialize_data(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option)
{

    if (!RTICdrStream_serializeLong(
        stream, &sample->structVersion)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->ConsoleTypeFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_serializeLong(
        stream, &sample->SystemRevId)) {
        return RTI_FALSE;
    }
            
    if (sample->VxWorksVersion == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->VxWorksVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->RtiNddsVersion == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->RtiNddsVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->PsgInterpVersion == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->PsgInterpVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->CompileDate == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->CompileDate, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->ddrmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->ddrmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->gradientmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->gradientmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->lockmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->lockmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->mastermd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->mastermd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->nvlibmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->nvlibmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->nvScriptmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->nvScriptmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->pfgmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->pfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->lpfgmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->lpfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->rfmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->rfmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->vxWorksKernelmd5 == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->vxWorksKernelmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (sample->fpgaLoadStr == NULL) {
        return RTI_FALSE;
    }
    if (!RTICdrStream_serializeString(
        stream, sample->fpgaLoadStr, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}



RTIBool Console_ConfPlugin_serialize(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option)
{
    if (!RTICdrStream_serializeCdrEncapsulationDefault(stream)) {
        return RTI_FALSE;
    }
    return Console_ConfPlugin_serialize_data(
        stream, sample, serialize_option); 
} 





RTIBool Console_ConfPlugin_deserialize_data(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option)
{

    if (!RTICdrStream_deserializeLong(
        stream, &sample->structVersion)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->ConsoleTypeFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeLong(
        stream, &sample->SystemRevId)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->VxWorksVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->RtiNddsVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->PsgInterpVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->CompileDate, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->ddrmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->gradientmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->lockmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->mastermd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->nvlibmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->nvScriptmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->pfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->lpfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->rfmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->vxWorksKernelmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrStream_deserializeString(
        stream, sample->fpgaLoadStr, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    return RTI_TRUE;
}


RTIBool Console_ConfPlugin_deserialize(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option)
{
    if (!RTICdrStream_deserializeCdrEncapsulationAndSetDefault(stream)) {
        return RTI_FALSE;
    }
    return Console_ConfPlugin_deserialize_data(
        stream, sample, deserialize_option); 
}





unsigned int Console_ConfPlugin_get_max_size_serialized_data(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getLongMaxSizeSerialized(
        current_alignment);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    current_alignment +=  RTICdrType_getStringMaxSizeSerialized(
        current_alignment, ((CONSOLE_CONF_MAX_STR_LEN)) + 1);
            
    return current_alignment - initial_alignment;
}



RTIBool Console_ConfPlugin_serialize_key(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option)
{


    return RTI_TRUE;
}


RTIBool Console_ConfPlugin_deserialize_key(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option)
{

    return RTI_TRUE;
}


unsigned int Console_ConfPlugin_get_max_size_serialized_key(
    unsigned int current_alignment)
{
        
    unsigned int initial_alignment = current_alignment;
        

    return current_alignment - initial_alignment;
}



unsigned int Console_ConfPlugin_get_max_size_serialized(
    unsigned int current_alignment)
{
    unsigned int initial_alignment = current_alignment;

    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    current_alignment += RTICdrType_getUnsignedShortMaxSizeSerialized(current_alignment);
    
    return (current_alignment - initial_alignment) + 
           Console_ConfPlugin_get_max_size_serialized_data(
               current_alignment);
}

/* ------------------------------------------------------------------------
 * Utility Methods
 * ------------------------------------------------------------------------ */


void Console_ConfPlugin_print(
    const Console_Conf *sample,
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
        &sample->structVersion, "structVersion", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->ConsoleTypeFlag, "ConsoleTypeFlag", indent_level + 1);
            
    RTICdrType_printLong(
        &sample->SystemRevId, "SystemRevId", indent_level + 1);
            
    if (&sample->VxWorksVersion==NULL) {
        RTICdrType_printString(
            NULL, "VxWorksVersion", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->VxWorksVersion, "VxWorksVersion", indent_level + 1);                
    }
            
    if (&sample->RtiNddsVersion==NULL) {
        RTICdrType_printString(
            NULL, "RtiNddsVersion", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->RtiNddsVersion, "RtiNddsVersion", indent_level + 1);                
    }
            
    if (&sample->PsgInterpVersion==NULL) {
        RTICdrType_printString(
            NULL, "PsgInterpVersion", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->PsgInterpVersion, "PsgInterpVersion", indent_level + 1);                
    }
            
    if (&sample->CompileDate==NULL) {
        RTICdrType_printString(
            NULL, "CompileDate", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->CompileDate, "CompileDate", indent_level + 1);                
    }
            
    if (&sample->ddrmd5==NULL) {
        RTICdrType_printString(
            NULL, "ddrmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->ddrmd5, "ddrmd5", indent_level + 1);                
    }
            
    if (&sample->gradientmd5==NULL) {
        RTICdrType_printString(
            NULL, "gradientmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->gradientmd5, "gradientmd5", indent_level + 1);                
    }
            
    if (&sample->lockmd5==NULL) {
        RTICdrType_printString(
            NULL, "lockmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->lockmd5, "lockmd5", indent_level + 1);                
    }
            
    if (&sample->mastermd5==NULL) {
        RTICdrType_printString(
            NULL, "mastermd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->mastermd5, "mastermd5", indent_level + 1);                
    }
            
    if (&sample->nvlibmd5==NULL) {
        RTICdrType_printString(
            NULL, "nvlibmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->nvlibmd5, "nvlibmd5", indent_level + 1);                
    }
            
    if (&sample->nvScriptmd5==NULL) {
        RTICdrType_printString(
            NULL, "nvScriptmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->nvScriptmd5, "nvScriptmd5", indent_level + 1);                
    }
            
    if (&sample->pfgmd5==NULL) {
        RTICdrType_printString(
            NULL, "pfgmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->pfgmd5, "pfgmd5", indent_level + 1);                
    }
            
    if (&sample->lpfgmd5==NULL) {
        RTICdrType_printString(
            NULL, "lpfgmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->lpfgmd5, "lpfgmd5", indent_level + 1);                
    }
            
    if (&sample->rfmd5==NULL) {
        RTICdrType_printString(
            NULL, "rfmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->rfmd5, "rfmd5", indent_level + 1);                
    }
            
    if (&sample->vxWorksKernelmd5==NULL) {
        RTICdrType_printString(
            NULL, "vxWorksKernelmd5", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->vxWorksKernelmd5, "vxWorksKernelmd5", indent_level + 1);                
    }
            
    if (&sample->fpgaLoadStr==NULL) {
        RTICdrType_printString(
            NULL, "fpgaLoadStr", indent_level + 1);                
    } else {
        RTICdrType_printString(
            sample->fpgaLoadStr, "fpgaLoadStr", indent_level + 1);                
    }
            
}

/* ------------------------------------------------------------------------
 * Lifecycle Methods
 * ------------------------------------------------------------------------ */


Console_Conf *Console_ConfPlugin_create_sample()
{
    return Console_ConfPlugin_create_sample_ex(RTI_TRUE);
}
        

Console_Conf *Console_ConfPlugin_create_sample_ex(RTIBool allocatePointers)
{

    Console_Conf *sample = NULL;

    RTIOsapiHeap_allocateStructure(&sample, Console_Conf);
                
    if (sample != NULL) {
        if (!Console_Conf_initialize_ex(sample,allocatePointers)) {
            RTIOsapiHeap_freeStructure(sample);
            return NULL;
        }
    }

    return sample;
}


void Console_ConfPlugin_delete_sample(
    Console_Conf *sample)
{
    Console_ConfPlugin_delete_sample_ex(sample,RTI_TRUE);
}
        

void Console_ConfPlugin_delete_sample_ex(
    Console_Conf *sample,RTIBool deletePointers)
{
    Console_Conf_finalize_ex(sample,deletePointers);
    RTIOsapiHeap_freeStructure(sample);
}

 


/* ------------------------------------------------------------------------
 * Key Manipulation Methods
 * ------------------------------------------------------------------------ */


PRESTypePluginKeyKind Console_ConfPlugin_get_key_kind()
{
        

    return PRES_TYPEPLUGIN_NO_KEY;
    
}


Console_ConfKeyHolder *Console_ConfPlugin_create_key()
{
    /* Note: If the definition of Console_ConfKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    return Console_ConfPlugin_create_sample();
}


void Console_ConfPlugin_delete_key(
    Console_ConfKeyHolder *key)
{
    /* Note: If the definition of Console_ConfKeyHolder
     * changes, the implementation of this method will need to change
     * accordingly.
     */
    Console_ConfPlugin_delete_sample(key);
}


RTIBool Console_ConfPlugin_instance_to_key(
    Console_ConfKeyHolder *dst, const Console_Conf *src)
{

    return RTI_TRUE;
}


RTIBool Console_ConfPlugin_key_to_instance(
    Console_Conf *dst, const Console_ConfKeyHolder *src)
{

    return RTI_TRUE;
}


/* Fill in the fields of the given instance ID based on the key field(s)
 * of the given instance sample.
 *
 * Important: The fields of the instance ID cannot all be set to zero!
 */
RTIBool Console_ConfPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Console_Conf *instance)
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

struct PRESTypePlugin *Console_ConfPlugin_new()
{
    struct PRESTypePlugin *plugin = NULL;
    const struct PRESTypePluginVersion PLUGIN_VERSION = 
        PRES_TYPE_PLUGIN_VERSION_1_1;
    
    RTIOsapiHeap_allocateStructure(
        &plugin, struct PRESTypePlugin);

    plugin->version = PLUGIN_VERSION;

    plugin->data.serializeFnc =
        (RTICdrStreamSerializeFunction)
        Console_ConfPlugin_serialize;

    plugin->data.serializeDataFnc =
        (RTICdrStreamSerializeFunction)
        Console_ConfPlugin_serialize_data;  
    plugin->data.deserializeFnc =
        (RTICdrStreamDeserializeFunction)
        Console_ConfPlugin_deserialize;

    plugin->data.deserializeDataFnc =
        (RTICdrStreamDeserializeFunction)
        Console_ConfPlugin_deserialize_data;
    plugin->data.copyFnc =
        (PRESTypePluginDataCopyFunction)
        Console_Conf_copy;
    plugin->data.getKeyKindFnc =
        (PRESTypePluginDataGetKeyKindFunction)
        Console_ConfPlugin_get_key_kind;
    plugin->data.printFnc =
        (RTICdrTypePrintFunction)
        Console_ConfPlugin_print;
    plugin->data.getMaxSizeSerializedFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_ConfPlugin_get_max_size_serialized;
    plugin->data.getMaxSizeSerializedDataFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_ConfPlugin_get_max_size_serialized_data;
    plugin->data.createSampleExFnc =
        (PRESTypePluginDataCreateSampleExFunction)
        Console_ConfPlugin_create_sample_ex;
    plugin->data.createSampleFnc =
        (PRESTypePluginDataCreateSampleFunction)
        Console_ConfPlugin_create_sample;        
    plugin->data.destroySampleExFnc =
        (PRESTypePluginDataDestroySampleExFunction)
        Console_ConfPlugin_delete_sample_ex;
    plugin->data.destroySampleFnc =
        (PRESTypePluginDataDestroySampleFunction)
        Console_ConfPlugin_delete_sample;        
    plugin->data.initializeExFnc =
        (PRESTypePluginDataInitializeExFunction)
        Console_Conf_initialize_ex;
    plugin->data.initializeFnc =
        (PRESTypePluginDataInitializeFunction)
        Console_Conf_initialize;        
    plugin->data.finalizeExFnc =
        (PRESTypePluginDataFinalizeExFunction)
        Console_Conf_finalize_ex;
    plugin->data.finalizeFnc =
        (PRESTypePluginDataFinalizeFunction)
        Console_Conf_finalize;
        
    plugin->data.instanceToKeyFnc =
        (PRESTypePluginDataInstanceToKeyFunction)
        Console_ConfPlugin_instance_to_key;
    plugin->data.keyToInstanceFnc =
        (PRESTypePluginDataKeyToInstanceFunction)
        Console_ConfPlugin_key_to_instance;
    plugin->data.instanceToGuidFnc =
        (PRESTypePluginDataInstanceToGuidFunction)
        Console_ConfPlugin_instance_to_id;


    plugin->data.createKeyFnc =
        (PRESTypePluginDataCreateKeyFunction)
        Console_ConfPlugin_create_key;
    plugin->data.destroyKeyFnc =
        (PRESTypePluginDataDestroyKeyFunction)
        Console_ConfPlugin_delete_key;

    
    plugin->data.serializeKeyFnc =
        (RTICdrStreamSerializeFunction)
        Console_ConfPlugin_serialize_key;
    plugin->data.deserializeKeyFnc =
        (RTICdrStreamDeserializeFunction)
        Console_ConfPlugin_deserialize_key;
    plugin->data.getMaxSizeSerializedKeyFnc =
        (RTICdrTypeGetMaxSizeSerializedFunction)
        Console_ConfPlugin_get_max_size_serialized_key;
        
       

    plugin->dataPoolFactory = PRES_TYPEPLUGIN_DEFAULT_DATA_POOL_FACTORY;

    
    plugin->typeCode = NULL;    
    

    plugin->languageKind = PRES_TYPEPLUGIN_DDS_TYPE;
    
    return plugin;
}


void Console_ConfPlugin_delete(struct PRESTypePlugin *self)
{
    RTIOsapiHeap_freeStructure(self);
}

