/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#ifndef NDDS_SubFuncs_h
#define NDDS_SubFuncs_h

#ifndef RTI_NDDS_4x
  #ifndef ndds_cdr_h
    #include "ndds/ndds_cdr.h"
  #endif
#else
  #ifndef ndds_c_h
    #include "ndds/ndds_c.h"
  #endif
#endif

#ifndef NDDS_Obj_h
#include "NDDS_Obj.h"
#endif

 
/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern int createSubscription(NDDS_ID pNDDS_Obj);
extern int createBESubscription(NDDS_ID pNDDS_Obj);
#ifdef RTI_NDDS_4x
extern void attachOnDataAvailableCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_DataAvailableCallback callback, void *pUserData);
extern void attachSampleLostCallback(NDDS_ID pNDDS_Obj, DDS_DataReaderListener_SampleLostCallback callback);
extern void attachDeadlineMissedCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_RequestedDeadlineMissedCallback callback);
extern void attachLivelinessChangedCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_LivelinessChangedCallback callback);
extern void attachSampleRejectedChangedCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_SampleRejectedCallback callback);
extern void attachSubscriptionMatchedCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_SubscriptionMatchedCallback callback);
extern void attachRequestedIncompatibleQosCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_RequestedIncompatibleQosCallback callback);
extern int initSubscription(NDDS_ID pNDDS_Obj);

#endif
/*
 * extern RTIBool nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj);
 * extern RTIBool disableSubscription(NDDS_ID pNDDS_Obj);
 * extern RTIBool enableSubscription(NDDS_ID pNDDS_Obj);
 * extern RTIBool nddsSubscriptionRemove(NDDS_ID pNDDS_Obj);
 * extern RTIBool nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj)
 */

#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 
#endif
 
#ifdef __cplusplus
}
#endif
 
#endif


