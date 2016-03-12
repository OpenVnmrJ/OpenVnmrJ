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
/*
modification history
--------------------
4-26-05,gmb  created 
2005-05-09 fv mods
*/

/*
DESCRIPTION

    Gradient communication functions

*/
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>

#include "gradient.h"
#include "nvhardware.h"
#include "errorcodes.h"
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"
#include "acqcmds.h"
#ifdef LKRATE               // :TODO: need to sort this out - duplicate entry between acqcmds.h and Lock_Cmd.h!
#undef LKRATE
#include "Lock_Cmd.h"
#endif

#ifdef RTI_NDDS_4x
#include "Lock_CmdPlugin.h"
#include "Lock_CmdSupport.h"
#endif  /* RTI_NDDS_4x */

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;

#ifndef RTI_NDDS_4x
static NDDSSubscriber GradCmdSubscriber;
#endif  /* RTI_NDDS_4x */

/* save xyz shim values to restore after abort */
int XYZshims[3] = { 32768, 32768, 32768 }; // 0=-5V, 32768=0V, 65535=+5V
extern SEM_ID shimRestoreBin;

/* Gradient Pub */
NDDS_ID pGradPub = NULL;
NDDS_ID pGradSub = NULL;

MSG_Q_ID pMsgesToGradParser = NULL;

static int callbackParam = 0;

/*
 *   The NDDS callback routine, the routine is call when 
 *   an issue of the subscribed topic is delivered.
 *   Called with the context of the NDDS task n_rtu7400
 *
 */
#ifndef RTI_NDDS_4x
RTIBool Grad_CmdCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Lock_Cmd *recvIssue;
    void GradCmdParser(Lock_Cmd *issue);
    int *param;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        recvIssue = (Lock_Cmd *) instance;
        DPRINT4(-1,"gradient Lock_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d\n",
        recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3 );
        msgQSend(pMsgesToGradParser, (char*) recvIssue, sizeof(Lock_Cmd), NO_WAIT, MSG_PRI_NORMAL);
    }
   return RTI_TRUE;
}
#else  /* RTI_NDDS_4x */
/* 4x callback */
void Grad_CmdCallback(void* listener_data, DDS_DataReader* reader)
{
   Lock_Cmd *recvIssue;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Lock_CmdSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   long i,numIssues;
   Lock_CmdDataReader *LockCmd_reader = NULL;

   LockCmd_reader = Lock_CmdDataReader_narrow(pGradSub->pDReader);
   if ( LockCmd_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo, "Lock_CmdCallback: DataReader narrow error.\n");
        return;
   }

   while(1)
   {
      // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
      // a sequence containing samples from only the next (in a well-determined
      // but unspecified order) un-taken instance.
      retcode =  Lock_CmdDataReader_take_next_instance(
            LockCmd_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        // retcode = Lock_CmdDataReader_take(LockCmd_reader,
        //                       &data_seq, &info_seq,
        //                       DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
        //                       DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

      if (retcode == DDS_RETCODE_NO_DATA) {
            break ; // return;
      } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "Lock_CmdCallback: next instance error %d\n",retcode);
            break ; // return;
      }

      numIssues = Lock_CmdSeq_get_length(&data_seq);

      for (i=0; i < numIssues; i++)
      {
         info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
         if ( info->valid_data)
         {
             recvIssue = Lock_CmdSeq_get_reference(&data_seq,i);
             DPRINT4(-1,"gradient Lock_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %lf\n",
                    recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3 );
             msgQSend(pMsgesToGradParser, (char*) recvIssue, sizeof(Lock_Cmd), NO_WAIT, MSG_PRI_NORMAL);
         }
      }
      retcode = Lock_CmdDataReader_return_loan( LockCmd_reader,
                  &data_seq, &info_seq);
      DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   }  // while

   return;
}

#endif  /* RTI_NDDS_4x */
 
void GradParser()
{
   Lock_Cmd  cmdIssue;
   int	ival;
   void GradCmdParser(Lock_Cmd *issue);

   FOREVER 
   {
      ival = msgQReceive(pMsgesToGradParser,(char*) &cmdIssue,
					sizeof(Lock_Cmd),WAIT_FOREVER);
      DPRINT1(-2, "GradParse:  msgQReceive returned %d\n", ival );
      if (ival == ERROR)
      {
         printf("Gradient PARSER Q ERROR\n");
         return;
      }
      else
      {
         GradCmdParser(&cmdIssue);
      }
   }
}


/*
 * The GradCmdParser, this is the command interpreter for the gradient
 * controller
 * Any publication to star/gradient1/cmd pattern will be processed here.
 * Typical commands are to set the X, Y, Z shims, gradient control values
 *
 *   Author: Greg Brissey 5/05/04
 */

void GradCmdParser(Lock_Cmd *issue)
{
   int dacValue;
   int cmd;
   DPRINT4(-2,"GradCmdParser cmd: %d, arg1: %d, arg2: %d, arg3: %lf\n",
			 issue->cmd,issue->arg1,issue->arg2,issue->arg3);

   switch( issue->cmd )
   {
     case SHIMDAC:
          DPRINT2(-2,"SHIMDAC, dac=%d, value=%d\n",issue->arg1,issue->arg2);
          /* chip select X=2, Y=4, Z=6 */
          dacValue = issue->arg2+0x8000;
          switch(issue->arg1)
          {  case 16:		// X shim
                  XYZshims[0] = dacValue;
                  setspi(2, dacValue);
                  break;
             case 17:           // Y shim
                  XYZshims[1] = dacValue;
                  setspi(4, dacValue);
                  break;
             case 2:		// Z1 for Varian Shim
             case 3:		// Z1C shim
                  XYZshims[2] = dacValue;
                  setspi(6, dacValue);
                  break;
             default:
                  errLogSysRet(LOGIT,debugInfo,
			"Illegal shim dac value %d\n",issue->arg1);
                  break;
          }
	  break;
     case 15:
          semGive(shimRestoreBin);
          break;
     default:
	  DPRINT1(-1,"Unknown command: %d\n",issue->cmd);
          break;

   }
   return;
}


/*
 * Create a Subscription Topic to communicate with the Controllers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createGradCmdSub(NDDS_ID nddsId, char *topic, void *callbackArg)
{

   NDDS_ID  pSubObj;
   char subtopic[128];

    /* Build Data type for both publication and subscription to Master */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }
 
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,nddsId,sizeof(NDDS_OBJ));
 
    strcpy(pSubObj->topicName,topic);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_CmdInfo(pSubObj);
 
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Grad_CmdCallback;
    pSubObj->callBkRtnParam = callbackArg;
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Grad_CmdCallback, &callbackArg);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj);
    return ( pSubObj );
}

/*
 * The Grad via NDDS uses this callback function to create Subscriptions to the
 * Host/MAster Gradient Command Publications aimed at the Gradient Controller 
 *
 *					Author Greg Brissey 4-26-05
 */
#ifndef RTI_NDDS_4x
NDDSSubscription Grad_CmdPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDS_ID  pSubObj;
     NDDSSubscription pSub;
     DPRINT3(-1,"Grad_CmdPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
		nddsTopic, nddsType, callBackRtnParam);
     DPRINT2(-1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",
					&callbackParam,callBackRtnParam);
     pSubObj = createGradCmdSub(NDDS_Domain, (char*) nddsTopic, 
						(void *) &callbackParam );
     pSub = pSubObj->subscription;
     return pSub;
}
#endif  /* RTI_NDDS_4x */

/*
 *  Gradient creates a pattern subscriber, to dynamicly allow subscription
 *  creation as host/master come on-line and publish to the Gradient Parser 
 *
 *					Author Greg Brissey 5-06-04
 */
gradCmdPubPatternSub()
{
#ifndef RTI_NDDS_4x
    GradCmdSubscriber = NddsSubscriberCreate(0);

    /* Gradient subscribe to any publications from controllers */
    /* star/gradient/comm */
    NddsSubscriberPatternAdd(GradCmdSubscriber,  
           "*/gradient/comm" /* GRAD_SUB_CMDS_PATTERN_TOPIC_STR */,
           Lock_CmdNDDSType , Grad_CmdPatternSubCreate,
           (void *)callbackParam); 
#else  /* RTI_NDDS_4x */
    pGradSub = createGradCmdSub(NDDS_Domain, GRAD_CMDS_TOPIC_STR, &callbackParam);
#endif  /* RTI_NDDS_4x */
}

startGradParser(int priority, int taskoptions, int stacksize)
{
   if (pMsgesToGradParser == NULL)
      pMsgesToGradParser = msgQCreate(300,sizeof(Lock_Cmd),MSG_Q_PRIORITY);
   if (pMsgesToGradParser == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"could not create Gradient Parser MsgQ, ");
      return;
   }
   
   if (taskNameToId("tGradParser") == ERROR)
      taskSpawn("tGradParser",priority,0,stacksize,GradParser,
						1,2,3,4,5,6,7,8,9,10);
}

killGradParser()
{
   int tid;
   if ((tid = taskNameToId("tGradParser")) != ERROR)
      taskDelete(tid);
}

SEM_ID  shimRestoreBin;

void shimRestorer()
{
   int i;
   FOREVER
   {
      semTake(shimRestoreBin,WAIT_FOREVER);
      wait4ParserReady(WAIT_FOREVER);
      taskDelay(1);             // wait one more second
      setspi(2, XYZshims[0]);
      setspi(4, XYZshims[1]);
      setspi(6, XYZshims[2]);
      DPRINT(-1,"Restorer: Shims restore done ");
   }
}

void startShimRestorer(int priority, int taskoptions, int stacksize)
{
   shimRestoreBin = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   if (shimRestoreBin == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"could not create Shim Restore Mutext");
      return;
   }

   if (taskNameToId("tShimRstore") == ERROR)
      taskSpawn("tShimRestore",priority,0,stacksize,shimRestorer,
                                                1,2,3,4,5,6,7,8,9,10);
}


xyzShow()
{

DPRINT3(-100,"X=%d, Y=%d, Z=%d\n",XYZshims[0],XYZshims[1],XYZshims[2]);

}
