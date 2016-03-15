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

#ifndef Lock_FIDPlugin_h
#define Lock_FIDPlugin_h

#ifndef Lock_FID_h
#include "Lock_FID.h"
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
 * Lock_FID.
 *
 * By default, this type is struct Lock_FID
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Lock_FID)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Lock_FID, the
 * following restriction applies: the key of struct
 * Lock_FID must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Lock_FID.
 */
typedef Lock_FID Lock_FIDKeyHolder;


extern RTIBool Lock_FIDPlugin_serialize(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option);

extern RTIBool Lock_FIDPlugin_serialize_data(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option);



extern RTIBool Lock_FIDPlugin_deserialize(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option);

extern RTIBool Lock_FIDPlugin_deserialize_data(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option);



extern unsigned int Lock_FIDPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Lock_FIDPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Lock_FIDPlugin_print(
    const Lock_FID *sample,
    const char *description, int indent_level);

extern Lock_FID *Lock_FIDPlugin_create_sample();

extern Lock_FID *Lock_FIDPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Lock_FIDPlugin_delete_sample(
    Lock_FID *sample);        

extern void Lock_FIDPlugin_delete_sample_ex(
    Lock_FID *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Lock_FIDPlugin_get_key_kind();

extern struct PRESTypePlugin *Lock_FIDPlugin_new();

extern void Lock_FIDPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Lock_FIDKeyHolder *Lock_FIDPlugin_create_key();

extern void Lock_FIDPlugin_delete_key(
    Lock_FIDKeyHolder *key);

extern RTIBool Lock_FIDPlugin_instance_to_key(
    Lock_FIDKeyHolder *key, const Lock_FID *instance);

extern RTIBool Lock_FIDPlugin_key_to_instance(
    Lock_FID *instance, const Lock_FIDKeyHolder *key);

extern RTIBool Lock_FIDPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Lock_FID *instance);




extern RTIBool Lock_FIDPlugin_serialize_key(
    struct RTICdrStream *stream, const Lock_FID *sample,
    void *serialize_option);

extern RTIBool Lock_FIDPlugin_deserialize_key(
    struct RTICdrStream *stream, Lock_FID *sample,
    void *deserialize_option);

extern unsigned int Lock_FIDPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Lock_FIDPlugin_h */
