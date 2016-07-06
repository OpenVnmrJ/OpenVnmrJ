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

#ifndef Lock_FID_h
#define Lock_FID_h

#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
#else
    #include "ndds_standalone_type.h"
#endif


/*

*






*/

/*

*  Author: Greg Brissey  5/06/2004

*/

#include "NDDS_Obj.h"

/* topic name form */

/* topic names form: ?/lock1/cmdstrm, lock1/?/cmdstrm */
                
#define LOCK_PUB_FID_TOPIC_FORMAT_STR ("lock/fid")                
                        
#define SUB_FID_TOPIC_FORMAT_STR ("lock/fid")                
                        
#define LOCK_MAX_DATA (5000)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Lock_FIDTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Lock_FID
{
    struct DDS_ShortSeq  lkfid;

} Lock_FID;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Lock_FIDSeq, Lock_FID);
        
NDDSUSERDllExport
RTIBool Lock_FID_initialize(
        Lock_FID* self);
        
NDDSUSERDllExport
RTIBool Lock_FID_initialize_ex(
        Lock_FID* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Lock_FID_finalize(
        Lock_FID* self);
                        
NDDSUSERDllExport
void Lock_FID_finalize_ex(
        Lock_FID* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Lock_FID_copy(
        Lock_FID* dst,
        const Lock_FID* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif




#ifdef __cplusplus

    extern "C" {

#endif



extern void getLock_FIDInfo(NDDS_OBJ *pObj);



#ifdef __cplusplus

}

#endif




#endif /* Lock_FID_h */
