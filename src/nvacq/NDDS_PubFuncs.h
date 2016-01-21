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

#ifndef NDDS__PubFuncs_h
#define NDDS__PubFuncs_h

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

#ifdef RTI_NDDS_4x

extern void attachPublicationMatchedCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_PublicationMatchedCallback callback);
extern void attachOfferedDeadlineMissedCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_OfferedDeadlineMissedCallback callback);
extern void attachOfferedIncompatibleQosCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_OfferedIncompatibleQosCallback callback);
extern void attachLivelinessLostCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_LivelinessLostCallback callback);
extern void attachReliableWriterCacheChangedCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_ReliableWriterCacheChangedCallback callback);
extern void attachReliableReaderActivityChangedCallback(NDDS_ID pNDDS_Obj,
              DDS_DataWriterListener_ReliableReaderActivityChangedCallback callback);
extern void attachDWriterUserData(NDDS_ID pNDDS_Obj, void *pUserdata);

extern void attachDWDiscvryUserData(NDDS_ID pNDDS_Obj, void *pUserdata, long length);

extern int initPublication(NDDS_ID pNDDS_Obj);
extern int initBEPublication(NDDS_ID pNDDS_Obj);
extern int createPublication(NDDS_ID pNDDS_Obj);

#else 
extern int createPublication(NDDS_ID pNDDS_Obj);
#endif  /* RTI_NDDS_4x */
extern RTIBool nddsPublicationDestroy(NDDS_ID pNDDS_Obj);
extern int nddsWait4Subscribers(NDDS_ID pNDDS_Obj, int timeout, int numberSubscribers);
extern int nddsPublicationIssuesWait(NDDS_ID pNDDS_Obj, int timeOut, int Qlevel);
extern int nddsPublishData(NDDS_ID pNDDS_Obj);


#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 
#endif
 
#ifdef __cplusplus
}
#endif
 
#endif


