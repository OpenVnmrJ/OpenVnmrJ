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

#ifndef Data_UploadPlugin_h
#define Data_UploadPlugin_h

#ifndef Data_Upload_h
#include "Data_Upload.h"
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
 * Data_Upload.
 *
 * By default, this type is struct Data_Upload
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Data_Upload)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Data_Upload, the
 * following restriction applies: the key of struct
 * Data_Upload must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Data_Upload.
 */
typedef Data_Upload Data_UploadKeyHolder;


extern RTIBool Data_UploadPlugin_serialize(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option);

extern RTIBool Data_UploadPlugin_serialize_data(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option);



extern RTIBool Data_UploadPlugin_deserialize(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option);

extern RTIBool Data_UploadPlugin_deserialize_data(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option);



extern unsigned int Data_UploadPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Data_UploadPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Data_UploadPlugin_print(
    const Data_Upload *sample,
    const char *description, int indent_level);

extern Data_Upload *Data_UploadPlugin_create_sample();

extern Data_Upload *Data_UploadPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Data_UploadPlugin_delete_sample(
    Data_Upload *sample);        

extern void Data_UploadPlugin_delete_sample_ex(
    Data_Upload *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Data_UploadPlugin_get_key_kind();

extern struct PRESTypePlugin *Data_UploadPlugin_new();

extern void Data_UploadPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Data_UploadKeyHolder *Data_UploadPlugin_create_key();

extern void Data_UploadPlugin_delete_key(
    Data_UploadKeyHolder *key);

extern RTIBool Data_UploadPlugin_instance_to_key(
    Data_UploadKeyHolder *key, const Data_Upload *instance);

extern RTIBool Data_UploadPlugin_key_to_instance(
    Data_Upload *instance, const Data_UploadKeyHolder *key);

extern RTIBool Data_UploadPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Data_Upload *instance);




extern RTIBool Data_UploadPlugin_serialize_key(
    struct RTICdrStream *stream, const Data_Upload *sample,
    void *serialize_option);

extern RTIBool Data_UploadPlugin_deserialize_key(
    struct RTICdrStream *stream, Data_Upload *sample,
    void *deserialize_option);

extern unsigned int Data_UploadPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Data_UploadPlugin_h */
