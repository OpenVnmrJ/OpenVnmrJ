
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Flash_Downld.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Flash_DownldPlugin_h
#define Flash_DownldPlugin_h

#ifndef Flash_Downld_h
#include "Flash_Downld.h"
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
 * Flash_Downld.
 *
 * By default, this type is struct Flash_Downld
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Flash_Downld)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Flash_Downld, the
 * following restriction applies: the key of struct
 * Flash_Downld must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Flash_Downld.
 */
typedef Flash_Downld Flash_DownldKeyHolder;


extern RTIBool Flash_DownldPlugin_serialize(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option);

extern RTIBool Flash_DownldPlugin_serialize_data(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option);



extern RTIBool Flash_DownldPlugin_deserialize(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option);

extern RTIBool Flash_DownldPlugin_deserialize_data(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option);



extern unsigned int Flash_DownldPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Flash_DownldPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Flash_DownldPlugin_print(
    const Flash_Downld *sample,
    const char *description, int indent_level);

extern Flash_Downld *Flash_DownldPlugin_create_sample();

extern Flash_Downld *Flash_DownldPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Flash_DownldPlugin_delete_sample(
    Flash_Downld *sample);        

extern void Flash_DownldPlugin_delete_sample_ex(
    Flash_Downld *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Flash_DownldPlugin_get_key_kind();

extern struct PRESTypePlugin *Flash_DownldPlugin_new();

extern void Flash_DownldPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Flash_DownldKeyHolder *Flash_DownldPlugin_create_key();

extern void Flash_DownldPlugin_delete_key(
    Flash_DownldKeyHolder *key);

extern RTIBool Flash_DownldPlugin_instance_to_key(
    Flash_DownldKeyHolder *key, const Flash_Downld *instance);

extern RTIBool Flash_DownldPlugin_key_to_instance(
    Flash_Downld *instance, const Flash_DownldKeyHolder *key);

extern RTIBool Flash_DownldPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Flash_Downld *instance);




extern RTIBool Flash_DownldPlugin_serialize_key(
    struct RTICdrStream *stream, const Flash_Downld *sample,
    void *serialize_option);

extern RTIBool Flash_DownldPlugin_deserialize_key(
    struct RTICdrStream *stream, Flash_Downld *sample,
    void *deserialize_option);

extern unsigned int Flash_DownldPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Flash_DownldPlugin_h */
