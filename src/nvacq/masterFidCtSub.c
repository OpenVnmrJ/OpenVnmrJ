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
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "FidCt_Stat.h"

#ifdef RTI_NDDS_4x
#include "FidCt_StatPlugin.h"
#include "FidCt_StatSupport.h"
#endif  /* RTI_NDDS_4x */

int createBESubscription();
void sendConsoleStatus();
RTIBool enableSubscription();
RTIBool disableSubscription();

extern Console_Stat *pCurrentStatBlock;
extern NDDS_ID NDDS_Domain;

#ifndef RTI_NDDS_4x
static NDDSSubscriber FidCtStatSubscriber;


RTIBool FidCt_StatCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
   FidCt_Stat *FidCtIssue;

   if (issue->status == NDDS_FRESH_DATA)
   {
      FidCtIssue = (FidCt_Stat *) instance;
      DPRINT2(3,"FidCt_StatCallback(): FID=%d, Ct=%d\n",
				FidCtIssue->FidCt, FidCtIssue->Ct);
      if (pCurrentStatBlock != NULL)
      {
         if ( (pCurrentStatBlock->Acqstate == ACQ_INACTIVE) && (FidCtIssue->FidCt == 0) && (FidCtIssue->Ct == 0) )
         {  
             /* DPRINT(-5,"Set IDLE\n"); */
             pCurrentStatBlock->Acqstate = ACQ_IDLE;
         }
         else if ( (pCurrentStatBlock->Acqstate == ACQ_IDLE) && (FidCtIssue->FidCt == -1) && (FidCtIssue->Ct == -1) )
         {
            pCurrentStatBlock->Acqstate = ACQ_INACTIVE;
             /* DPRINT(-5,"Set INACTIVE\n"); */
         }

         if ( (pCurrentStatBlock->Acqstate != ACQ_ACQUIRE) || FidCtIssue->FidCt || FidCtIssue->Ct )
         {
            pCurrentStatBlock->AcqFidCnt = FidCtIssue->FidCt;
            pCurrentStatBlock->AcqCtCnt  = FidCtIssue->Ct;
            sendConsoleStatus();
         }
      }
   }

   return RTI_TRUE;
}

#else  /* RTI_NDDS_4x */

static NDDS_ID pFidCt_StatSub;

void FidCt_StatCallback(void* listener_data, DDS_DataReader* reader)
{
   FidCt_Stat *FidCtIssue;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;

   struct FidCt_StatSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   FidCt_StatDataReader *FidCt_Stat_reader = NULL;

   FidCt_Stat_reader = FidCt_StatDataReader_narrow(pFidCt_StatSub->pDReader);
   if ( FidCt_Stat_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"FidCt_StatCallback: DataReader narrow error\n");
        return;
   }

   while(1)
   {

        // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        // a sequence containing samples from only the next (in a well-determined
        // but unspecified order) un-taken instance.
        retcode =  FidCt_StatDataReader_take_next_instance(
            FidCt_Stat_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);


        // retcode = Cntlr_CommDataReader_take(CntlrComm_reader,
        //                      &data_seq, &info_seq,
        //                      DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
        //                      DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"FidCt_StatCallback: next instance error %d\n",retcode);
                 break; // return;
        }

        numIssues = FidCt_StatSeq_get_length(&data_seq);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              FidCtIssue = (FidCt_Stat *) Cntlr_CommSeq_get_reference(&data_seq,i);
              DPRINT2(+3,"FidCt_StatCallback(): FID=%d, Ct=%d\n",
				        FidCtIssue->FidCt, FidCtIssue->Ct);
              if (pCurrentStatBlock != NULL)
              {
                 if ( (pCurrentStatBlock->Acqstate == ACQ_INACTIVE) && (FidCtIssue->FidCt == 0) && (FidCtIssue->Ct == 0) )
                 {  
                     /* DPRINT(-5,"Set IDLE\n"); */
                     pCurrentStatBlock->Acqstate = ACQ_IDLE;
                 }
                 else if ( (pCurrentStatBlock->Acqstate == ACQ_IDLE) && (FidCtIssue->FidCt == -1) && (FidCtIssue->Ct == -1) )
                 {
                    pCurrentStatBlock->Acqstate = ACQ_INACTIVE;
                     /* DPRINT(-5,"Set INACTIVE\n"); */
                 }

                 if ( (pCurrentStatBlock->Acqstate != ACQ_ACQUIRE) || FidCtIssue->FidCt || FidCtIssue->Ct )
                 {
                    pCurrentStatBlock->AcqFidCnt = FidCtIssue->FidCt;
                    pCurrentStatBlock->AcqCtCnt  = FidCtIssue->Ct;
                    sendConsoleStatus();
                 }
              }
           }
        }
        retcode = FidCt_StatDataReader_return_loan( FidCt_Stat_reader,
                  &data_seq, &info_seq);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   } // while
   return;
}


#endif  /* RTI_NDDS_4x */

NDDS_ID createMasterFidSub(char *topic)
{
   NDDS_ID pSubObj;
   DPRINT(-1,"createMasterFidSub()");
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
   getFidCt_StatInfo(pSubObj);

   /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
   pSubObj->callBkRtn = FidCt_StatCallback;
   pSubObj->callBkRtnParam = NULL;
#endif  /* RTI_NDDS_4x */
   pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
   pSubObj->BE_UpdateMinDeltaMillisec = 250;  /* 4 times a second */

#ifndef RTI_NDDS_4x
   createBESubscription(pSubObj);
#else  /* RTI_NDDS_4x */
   initBESubscription(pSubObj);
   attachOnDataAvailableCallback(pSubObj,FidCt_StatCallback,NULL);
   createSubscription(pSubObj);
#endif  /* RTI_NDDS_4x */

   return(pSubObj);
}

#ifndef RTI_NDDS_4x
/*
 * The Master via NDDS uses this callback function to create Subscriptions to the
 * FIdCT_StatusBlock BE Publications aimed at the MAster for updating the Stat block
 *
 *					Author Greg Brissey 10-26-04
 */
NDDSSubscription FidCt_StatPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDSSubscription pSub;
     NDDS_ID pNddsSub;
     DPRINT3(-1,"FidCT_StatPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
		nddsTopic, nddsType, callBackRtnParam);
     pNddsSub = createMasterFidSub((char*) nddsTopic );
     pSub = pNddsSub->subscription;
     return pSub;
}
/* DPRINT2(-1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam); */
/* pFidCtStatSubs[numLockCmdSubs]  = createLockCmdSub(NDDS_Domain, (char*) nddsTopic, (void *) &callbackParam ); */
/* pSub = pLockCmdSubs[numLockCmdSubs++]->subscription; */

/*
 *  Lock creates a pattern subscriber, to dynamicly allow subscription creation
 *  as host/master come on-line and publish to the Lock Parser 
 *
 *					Author Greg Brissey 10-26-04
 */
FidCtStatPatternSub()
{
    FidCtStatSubscriber = NddsSubscriberCreate(0);

    /* Lock subscribe to any publications from controllers */
    /* star/lock/cmds */
    NddsSubscriberPatternAdd(FidCtStatSubscriber,  
           FIDCT_SUB_PATTERN_TOPIC_STR, FidCt_StatNDDSType , FidCt_StatPatternSubCreate, (void *)NULL); 
}
#else  /* RTI_NDDS_4x */
FidCtStatPatternSub()
{
     pFidCt_StatSub = createMasterFidSub((char*)FIDCT_STAT_M21TOPIC_STR);
}
#endif  /* RTI_NDDS_4x */
