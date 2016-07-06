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

#include <semLib.h>
#include "errorcodes.h"
#include "expDoneCodes.h"
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "Lock_Stat.h"
#include "Lock_FID.h"

#ifdef RTI_NDDS_4x
#include "Console_StatPlugin.h"
#include "Lock_StatPlugin.h"
#include "Lock_FIDPlugin.h"
#include "Console_StatSupport.h"
#include "Lock_StatSupport.h"
#include "Lock_FIDSupport.h"
#endif  /* RTI_NDDS_4x */

int createBESubscription();
void sendConsoleStatus();
void checkLkInterLk();
RTIBool enableSubscription();
RTIBool disableSubscription();

extern Console_Stat *pCurrentStatBlock;
extern NDDS_ID NDDS_Domain;


int	globalLocked;

static int LkInterLk = 0;

#ifndef RTI_NDDS_4x
RTIBool Lock_StatCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
Lock_Stat *lkInfo;
   if (pCurrentStatBlock == NULL)
      return RTI_TRUE;
   if (issue->status == NDDS_FRESH_DATA)
   {
      lkInfo = (Lock_Stat *) instance;
      DPRINT2(+3,"LockStatCallback(): lkgain=%d, lkpower=%d\n",
				lkInfo->lkgain, lkInfo->lkpower);
      pCurrentStatBlock->AcqLockLevel = lkInfo->lkLevelR;
      pCurrentStatBlock->AcqLockGain  = lkInfo->lkgain;
      pCurrentStatBlock->AcqLockPhase = lkInfo->lkphase;
      pCurrentStatBlock->AcqLockPower = lkInfo->lkpower;
      pCurrentStatBlock->gpaError     = lkInfo->lkLevelI;
      globalLocked = lkInfo->locked;
//      pCurrentStatBlock->AcqLockFreqAP = lkInfo->lkfreq; /* double !! */

      checkLkInterLk();
      sendConsoleStatus();
   }

   return RTI_TRUE;
}
#else  /* RTI_NDDS_4x */
/* 4x callback */
static NDDS_ID pLockStatSub;    /* 4x callback needs this reference */

void Lock_StatCallback(void* listener_data, DDS_DataReader* reader)
{
   Lock_Stat *lkInfo;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Lock_StatSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   long i,numIssues;
   Lock_StatDataReader *LockStat_reader = NULL;
   DDS_TopicDescription *topicDesc;


   LockStat_reader = Lock_StatDataReader_narrow(pLockStatSub->pDReader);
   if ( LockStat_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo, "LockStat_Callback: DataReader narrow error.\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+4,"Lock_StatCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));


   retcode = Lock_StatDataReader_take(LockStat_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
   if (retcode == DDS_RETCODE_NO_DATA) {
            return;
   } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "LockStat_Callback: next instance error %d\n",retcode);
            return;
   }

   if (pCurrentStatBlock == NULL)
   {
      retcode = Monitor_CmdDataReader_return_loan( LockStat_reader,
                  &data_seq, &info_seq);
      return;
   }

   numIssues = Monitor_CmdSeq_get_length(&data_seq);

   for (i=0; i < numIssues; i++)
   {

      info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
      if ( info->valid_data)
      {
          lkInfo = Lock_StatSeq_get_reference(&data_seq,i);
          DPRINT2(+3,"LockStatCallback(): lkgain=%d, lkpower=%d\n",
				lkInfo->lkgain, lkInfo->lkpower);
          pCurrentStatBlock->AcqLockLevel = lkInfo->lkLevelR;
          pCurrentStatBlock->AcqLockGain  = lkInfo->lkgain;
          pCurrentStatBlock->AcqLockPhase = lkInfo->lkphase;
          pCurrentStatBlock->AcqLockPower = lkInfo->lkpower;
          pCurrentStatBlock->gpaError     = lkInfo->lkLevelI;
          globalLocked = lkInfo->locked;
//      pCurrentStatBlock->AcqLockFreqAP = lkInfo->lkfreq; /* double !! */

          checkLkInterLk();
          sendConsoleStatus();
      }
   }
   retcode = Monitor_CmdDataReader_return_loan( LockStat_reader,
                  &data_seq, &info_seq);

   return;
}

#endif  /* RTI_NDDS_4x */

void setLkInterLk(int state)
{
   LkInterLk = state;
}

void checkLkInterLk()
{
   if (globalLocked) 	return; // all ok

   if (LkInterLk == HARD_ERROR)
      sendException(HARD_ERROR,SFTERROR + LOCKLOST, 0, 0, NULL);
   else if (LkInterLk == WARNING_MSG)
      sendException(WARNING_MSG,SFTERROR + LOCKLOST, 0, 0, NULL);

   LkInterLk = 0;   // one msg is all we need
}


NDDS_ID createMasterLkSub(char *topic)
{
   NDDS_ID pSubObj;
   DPRINT(-1,"createMasterLkSub()");
   /* malloc subscription space */
   if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
   {
      return(NULL);
   }

   /* zero out structure */
   memset(pSubObj,0,sizeof(NDDS_OBJ));
   memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

   strcpy(pSubObj->topicName,topic);

   /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
   getLock_StatInfo(pSubObj);

   /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
   pSubObj->callBkRtn = Lock_StatCallback;
   pSubObj->callBkRtnParam = NULL; /* write end of pipe */
#endif  /* RTI_NDDS_4x */
   /* pSubObj->callBkRtnParam = lockSubPipeFd; /* write end of pipe */
   pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifndef RTI_NDDS_4x
   createBESubscription(pSubObj);   /* 20 msec update rate max. (default) */
#else  /* RTI_NDDS_4x */
   initBESubscription(pSubObj);
   attachOnDataAvailableCallback(pSubObj,Lock_StatCallback,NULL);
   createSubscription(pSubObj);   /* 20 msec update rate max. (default) */
   pLockStatSub = pSubObj;   /* 4x needs reference to this in callback */
#endif  /* RTI_NDDS_4x */

   return(pSubObj);

}


/*
 ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    Lock Fid Subscription for Autolock
 ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

     Author:  Greg Brissey 1/10/2005
*/

NDDS_ID pLkFIdSub = NULL;
Lock_FID *pLkFid = NULL;

SEM_ID  pLkFidSem = NULL;


#ifndef RTI_NDDS_4x
RTIBool Lock_FIDCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
   Lock_FID *lkFID;
   if (pCurrentStatBlock == NULL)
      return RTI_TRUE;

   if (issue->status == NDDS_FRESH_DATA)
   {
      lkFID = (Lock_FID *) instance;
      DPRINT1( 2,"LockFIDCallback(): len=%d\n",lkFID->lkfid.len);
      if (pLkFidSem != NULL)
         semGive(pLkFidSem);
   }
   return RTI_TRUE;
}

getLkData(int *lkdata)
{
    int i;
    /* Now Attempt to take it when, when it would block that     */
    /*   is the state we want it in.                             */
    while (semTake(pLkFidSem,NO_WAIT) != ERROR);

     if ( semTake(pLkFidSem,(sysClkRateGet() * 5)) == ERROR )    /* block waiting for FID issue, timeout in 5 seconds */
	return(-1);

     for(i=0; i < pLkFid->lkfid.len; i++)
     {
         /* DPRINT2(-1," lkfid.val[%d] = %d\n",i,pLkFid->lkfid.val[i]); */
         lkdata[i] = (int) (pLkFid->lkfid.val[i]);
     }

     return(pLkFid->lkfid.len);
}

#else  /* RTI_NDDS_4x */

/* intermediate buffer for lock fid data */
static short lkdatabuf[5000];
static int lkdatalen;

void Lock_FIDCallback(void* listener_data, DDS_DataReader* reader)
{
   int dataLength;
   Lock_FID *lkFID;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Lock_FIDSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   long i,numIssues;
   Lock_FIDDataReader *LockFID_reader = NULL;
   DDS_TopicDescription *topicDesc;

   LockFID_reader = Lock_FIDDataReader_narrow(pLkFIdSub->pDReader);
   if ( LockFID_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo, "LockFID_Callback: DataReader narrow error.\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+4,"Lock_FIDCallback; Type: '%s', Name: '%s'\n",
       DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));

   retcode = Lock_FIDDataReader_take(LockFID_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
   if (retcode == DDS_RETCODE_NO_DATA) {
            return;
   } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "LockFID_Callback: next instance error %d\n",retcode);
            return;
   }


   numIssues = Lock_FIDSeq_get_length(&data_seq);

   for (i=0; i < numIssues; i++)
   {
      info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
      if ( info->valid_data)
      {
          lkFID = Lock_FIDSeq_get_reference(&data_seq,i);
          /* copy into intermediate buffer for getLkData() */
          lkdatalen = dataLength = DDS_ShortSeq_get_length(&(lkFID->lkfid));
          DDS_ShortSeq_to_array(&(lkFID->lkfid),lkdatabuf,dataLength);
          DPRINT1(4,"LockFIDCallback(): len=%d\n",dataLength);
          /*
          *int j;
          *for(j=0; j < 10; j++)
          *{
          *    // short data = DDS_ShortSeq_get(&(lkFID->lkfid),i);
          *    short *data = DDS_ShortSeq_get_reference(&(lkFID->lkfid),i);
          *    DPRINT2(-1," lkfid[%d] = %d\n",i,*data);
          *    // lkdata[i] = (int) *data;
          *}
          */
          if (pLkFidSem != NULL)
             semGive(pLkFidSem);
      }
   }
   retcode = Lock_FIDDataReader_return_loan( LockFID_reader,
                  &data_seq, &info_seq);

   return;
}


/* WARNING should verify this since I am not sure if the instance can be used
   after it's return from the take in the callback, may have to do the copy
   in the callback    gmb 7/25/07
*/
getLkData(int *lkdata)
{
    int i;
    int dataLength;
    /* Now Attempt to take it when, when it would block that     */
    /*   is the state we want it in.                             */
    while (semTake(pLkFidSem,NO_WAIT) != ERROR);

     if ( semTake(pLkFidSem,(sysClkRateGet() * 5)) == ERROR )    /* block waiting for FID issue, timeout in 5 seconds */
	return(-1);

     memcpy(lkdata,lkdatabuf,(lkdatalen * sizeof(short)));

     /* deemed not the way to do it, use intermediate buffer above
      *
      * dataLength = DDS_ShortSeq_get_length(&(pLkFid->lkfid));
      * // DDS_ShortSeq_to_array (&(pLkFid->lkfid), (short*) lkdata, dataLength);
      * for(i=0; i < dataLength; i++)
      * {
      *     short *data = DDS_ShortSeq_get_reference(&(pLkFid->lkfid),i);
      *     // DPRINT2(-1," lkfid.val[%d] = %d\n",i,pLkFid->lkfid.val[i]);
      *    lkdata[i] = (int) *data;
      * }
      */
     return(lkdatalen);
}
#endif  /* RTI_NDDS_4x */


NDDS_ID createMasterLkFidSub(char *topic,int updateRateInHz)
{
   NDDS_ID pSubObj;
   DPRINT(0,"createMasterLkSub()");
   /* malloc subscription space */
   if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
   {
      return(NULL);
   }

   /* zero out structure */
   memset(pSubObj,0,sizeof(NDDS_OBJ));
   memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

   strcpy(pSubObj->topicName,topic);

   /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
   getLock_FIDInfo(pSubObj);

   /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
   pSubObj->callBkRtn = Lock_FIDCallback;
   pSubObj->callBkRtnParam = NULL; /* write end of pipe */
#endif  /* RTI_NDDS_4x */
   pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

   /* check limits, 1 to 1000 Hz */
   if ( (updateRateInHz > 0) && (updateRateInHz <= 1000) )
       pSubObj->BE_UpdateMinDeltaMillisec = 1000/updateRateInHz; 
   else
       pSubObj->BE_UpdateMinDeltaMillisec = 1000;   /* max rate once a second */

   pSubObj->BE_DeadlineMillisec = 6000; /* no Pub in 6 sec then it's gone.. */

#ifndef RTI_NDDS_4x
   createBESubscription(pSubObj);   /* 20 msec update rate max. (default) */
#else  /* RTI_NDDS_4x */
   initBESubscription(pSubObj);   /* 20 msec update rate max. (default) */
   attachOnDataAvailableCallback(pSubObj,Lock_FIDCallback,NULL);
   createSubscription(pSubObj);   /* 20 msec update rate max. (default) */
#endif  /* RTI_NDDS_4x */

   pLkFid = (Lock_FID*) pSubObj->instance;   /* set point to Lock FID Issue */

   return(pSubObj);
}

initLockFidSub(int updateRateInHz)
{
    if ( pLkFidSem == NULL)
	pLkFidSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
    if (pLkFIdSub == NULL)
       pLkFIdSub = createMasterLkFidSub(SUB_FID_TOPIC_FORMAT_STR,updateRateInHz);
}
