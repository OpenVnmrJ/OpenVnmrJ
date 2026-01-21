
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Console_Conf.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Console_ConfPlugin_h
#define Console_ConfPlugin_h

#ifndef Console_Conf_h
#include "Console_Conf.h"
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
 * Console_Conf.
 *
 * By default, this type is struct Console_Conf
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Console_Conf)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Console_Conf, the
 * following restriction applies: the key of struct
 * Console_Conf must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Console_Conf.
 */
typedef Console_Conf Console_ConfKeyHolder;


extern RTIBool Console_ConfPlugin_serialize(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option);

extern RTIBool Console_ConfPlugin_serialize_data(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option);



extern RTIBool Console_ConfPlugin_deserialize(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option);

extern RTIBool Console_ConfPlugin_deserialize_data(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option);



extern unsigned int Console_ConfPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Console_ConfPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Console_ConfPlugin_print(
    const Console_Conf *sample,
    const char *description, int indent_level);

extern Console_Conf *Console_ConfPlugin_create_sample();

extern Console_Conf *Console_ConfPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Console_ConfPlugin_delete_sample(
    Console_Conf *sample);        

extern void Console_ConfPlugin_delete_sample_ex(
    Console_Conf *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Console_ConfPlugin_get_key_kind();

extern struct PRESTypePlugin *Console_ConfPlugin_new();

extern void Console_ConfPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Console_ConfKeyHolder *Console_ConfPlugin_create_key();

extern void Console_ConfPlugin_delete_key(
    Console_ConfKeyHolder *key);

extern RTIBool Console_ConfPlugin_instance_to_key(
    Console_ConfKeyHolder *key, const Console_Conf *instance);

extern RTIBool Console_ConfPlugin_key_to_instance(
    Console_Conf *instance, const Console_ConfKeyHolder *key);

extern RTIBool Console_ConfPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Console_Conf *instance);




extern RTIBool Console_ConfPlugin_serialize_key(
    struct RTICdrStream *stream, const Console_Conf *sample,
    void *serialize_option);

extern RTIBool Console_ConfPlugin_deserialize_key(
    struct RTICdrStream *stream, Console_Conf *sample,
    void *deserialize_option);

extern unsigned int Console_ConfPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Console_ConfPlugin_h */
