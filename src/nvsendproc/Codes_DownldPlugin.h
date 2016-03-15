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

#ifndef Codes_DownldPlugin_h
#define Codes_DownldPlugin_h

#ifndef Codes_Downld_h
#include "Codes_Downld.h"
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
 * Codes_Downld.
 *
 * By default, this type is struct Codes_Downld
 * itself. However, if for some reason this choice is not practical for your
 * system (e.g. if sizeof(struct Codes_Downld)
 * is very large), you may redefine this typedef in terms of another type of
 * your choosing. HOWEVER, if you define the KeyHolder type to be something
 * other than struct Codes_Downld, the
 * following restriction applies: the key of struct
 * Codes_Downld must consist of a
 * single field of your redefined KeyHolder type and that field must be the
 * first field in struct Codes_Downld.
 */
typedef Codes_Downld Codes_DownldKeyHolder;


extern RTIBool Codes_DownldPlugin_serialize(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option);

extern RTIBool Codes_DownldPlugin_serialize_data(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option);



extern RTIBool Codes_DownldPlugin_deserialize(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option);

extern RTIBool Codes_DownldPlugin_deserialize_data(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option);



extern unsigned int Codes_DownldPlugin_get_max_size_serialized(
    unsigned int current_alignment);

extern unsigned int Codes_DownldPlugin_get_max_size_serialized_data(
    unsigned int current_alignment);

extern void Codes_DownldPlugin_print(
    const Codes_Downld *sample,
    const char *description, int indent_level);

extern Codes_Downld *Codes_DownldPlugin_create_sample();

extern Codes_Downld *Codes_DownldPlugin_create_sample_ex(RTIBool allocatePointers);        

extern void Codes_DownldPlugin_delete_sample(
    Codes_Downld *sample);        

extern void Codes_DownldPlugin_delete_sample_ex(
    Codes_Downld *sample,RTIBool deletePointers);

extern PRESTypePluginKeyKind Codes_DownldPlugin_get_key_kind();

extern struct PRESTypePlugin *Codes_DownldPlugin_new();

extern void Codes_DownldPlugin_delete(struct PRESTypePlugin *plugin);

/* The following are used if your key kind is USER_KEY */

extern Codes_DownldKeyHolder *Codes_DownldPlugin_create_key();

extern void Codes_DownldPlugin_delete_key(
    Codes_DownldKeyHolder *key);

extern RTIBool Codes_DownldPlugin_instance_to_key(
    Codes_DownldKeyHolder *key, const Codes_Downld *instance);

extern RTIBool Codes_DownldPlugin_key_to_instance(
    Codes_Downld *instance, const Codes_DownldKeyHolder *key);

extern RTIBool Codes_DownldPlugin_instance_to_id(
    DDS_InstanceId_t *id, RTIBool *is_unique,
    const Codes_Downld *instance);




extern RTIBool Codes_DownldPlugin_serialize_key(
    struct RTICdrStream *stream, const Codes_Downld *sample,
    void *serialize_option);

extern RTIBool Codes_DownldPlugin_deserialize_key(
    struct RTICdrStream *stream, Codes_Downld *sample,
    void *deserialize_option);

extern unsigned int Codes_DownldPlugin_get_max_size_serialized_key(
    unsigned int current_alignment);

#ifdef __cplusplus
}
#endif
        

#endif /* Codes_DownldPlugin_h */
