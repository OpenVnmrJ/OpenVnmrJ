
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Monitor_Cmd.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Monitor_CmdPlugin_h
#define Monitor_CmdPlugin_h

#ifndef Monitor_Cmd_h
#include "Monitor_Cmd.h"
#endif




struct RTICdrStream;
struct MIGRtpsGuid;

#ifndef pres_typePlugin_h
#include "pres/pres_typePlugin.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* The type used to store keys for instances of type struct
 * Monitor_Cmd.
 *
 * By default, this type is struct Monitor_Cmd
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Monitor_Cmd)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Monitor_Cmd, the
 * following restriction applies: the key of struct
 * Monitor_Cmd must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Monitor_Cmd.
 */
typedef Monitor_Cmd Monitor_CmdKeyHolder;


extern RTIBool Monitor_CmdPlugin_serialize(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option);

extern RTIBool Monitor_CmdPlugin_serialize_data(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option);



extern RTIBool Monitor_CmdPlugin_deserialize(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option);

extern RTIBool Monitor_CmdPlugin_deserialize_data(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option);



extern unsigned int Monitor_CmdPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Monitor_CmdPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Monitor_CmdPlugin_print(
    const Monitor_Cmd *sample,
    const char *description, int indent_level);

extern Monitor_Cmd *Monitor_CmdPlugin_create_sample();

extern Monitor_Cmd *Monitor_CmdPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Monitor_CmdPlugin_delete_sample(
    Monitor_Cmd *sample);        

extern void Monitor_CmdPlugin_delete_sample_ex(
    Monitor_Cmd *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Monitor_CmdPlugin_get_key_kind();

extern struct PRESTypePlugin *Monitor_CmdPlugin_new();

extern void Monitor_CmdPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Monitor_CmdKeyHolder *Monitor_CmdPlugin_create_key();

extern void Monitor_CmdPlugin_delete_key(
    Monitor_CmdKeyHolder *key);

extern RTIBool Monitor_CmdPlugin_instance_to_key(
    Monitor_CmdKeyHolder *key, const Monitor_Cmd *instance);

extern RTIBool Monitor_CmdPlugin_key_to_instance(
    Monitor_Cmd *instance, const Monitor_CmdKeyHolder *key);

extern RTIBool Monitor_CmdPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Monitor_Cmd *instance);




extern RTIBool Monitor_CmdPlugin_serialize_key(
    struct RTICdrStream *stream, const Monitor_Cmd *sample,
    void *serialize_option);

extern RTIBool Monitor_CmdPlugin_deserialize_key(
    struct RTICdrStream *stream, Monitor_Cmd *sample,
    void *deserialize_option);

extern unsigned int Monitor_CmdPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Monitor_CmdPlugin_h */
