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
DESCRIPTION

  This Construct is used to flow control groups of DDRs

*/

#ifndef RTI_NDDS_4x

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>

#include "nvhardware.h"
#include "logMsgLib.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "taskPriority.h"


#define MAX_DDRS 128
#define MAX_GRPS 128

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "Cntlr_Comm.h"

/* NDDS Primary Domain */
extern NDDS_ID NDDS_Domain;

extern char hostName[80];

extern int BrdNum;

static NDDSSubscriber  DDRGrpBarrierSubscriber;

static NDDS_ID pGroupSyncPub = NULL;
static NDDS_ID pGroupSyncSub[MAX_DDRS];
static int numSubs = 0;

/*
 * This is how this works.
 * 1. This contruct and pub/sub is a mechanism to Sychronize DDRs into groups in uploading Data.
 *    e.g. it is hoped to prevent exceeding the bandwidth of the host that might other happen.
 *          resulting in  system hangup.
 *   This construct is has two parts, start and finish 
 *   Start, will pend the calling task unless it is a member of the 'working group'
 *         
 *   Finish, broadcasts to all the participating DDRs (via NDDS pub/sub) 
 *           that the identified ddr has finished.
 *           The NDDS Callback for this then determines if all ddrs for that group have reported
 *           finished.  If they have thing the work group is incrmented to the next group.
 *           And if the ddr is part of this new group then the semaphore is given so that
 *           it can start.
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */


static int DDRStatus[MAX_DDRS];     /* array of active or not ddrs, used to determine groups */
static int DDRGrp[MAX_DDRS];        /* array giving the group of teh ddr via it's Ordinal # as the index */
static int numInGrp[MAX_GRPS];      /* the number in each group */
static int numAtGrpBarrier[MAX_GRPS]; /* the # of ddrs that have called finished */
static int ddrsInGrpAtBarrier[MAX_GRPS][MAX_DDRS];   /* list of ddrs that have reported finished for each group */

static int firststart = 1;

int workingGroup = 0;		/* the group presently running */
int myWorkGroup = -1;
int myOrdinalNum = -1;		/* e.g. ddr1-ddrxx, ddr1 has ordinal number Zero */
int numberOfGroups = 1;
int MaxInGroup = 2;		/* the group member size */

static SEM_ID   pBarrierWaitSem;
static int      numActiveDDRs;


/* prototype */
int sendDDRGrpSync(int cmd, int arg1,int arg2,int arg3,int arg4, char *strmsg);

/*
 * Initialize Barrier semaphore and interval value
 *
 * Author Greg Brissey  2/01/06
 *
 */
initGroupBarrier( int ddrOrdinalNum )
{
   int i,j;

   if (pBarrierWaitSem == NULL)
       pBarrierWaitSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   myOrdinalNum = ddrOrdinalNum;

   DPRINT1(-4,"initGroupBarrier: DDR: %d\n",myOrdinalNum);
   workingGroup = 0;
   numberOfGroups = 1;
   MaxInGroup = 2;
   /* mark all DDRs as inactive */
   for( i=0; i < MAX_DDRS; i++)
   {
       DDRStatus[i] = -999;
       DDRGrp[i] = -999;
   }
   for( i=0; i < MAX_GRPS; i++)
   {
      numAtGrpBarrier[i] = numInGrp[i] = 0;
   }
   for( i=0; i < MAX_GRPS; i++)
   {
      for( j=0; j < MAX_DDRS; j++)
      {
         ddrsInGrpAtBarrier[i][j] = -1;
      }
   }
}

/*
 *  Reset the counters and seamphore to their initial state
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
resetGrpBarrier()
{
   int i,j;
   workingGroup = 0;
   myWorkGroup = 0;
   firststart = 1;
   for( i=0; i < MAX_GRPS; i++)
   {
      numAtGrpBarrier[i] = 0;
   }
   for( i=0; i < numberOfGroups; i++)
   {
      for( j=0; j < numInGrp[i]; j++)
      {
         ddrsInGrpAtBarrier[i][j] = -1;
      }
   }
   while (semTake(pBarrierWaitSem,NO_WAIT) != ERROR);
   return 0;
}

setMyOrdinalNum(int ddrOrdinalNum)
{
   myOrdinalNum = ddrOrdinalNum;
}


/* Based on which DDR and if it's active and how many are in a group calc which group
 *  this DDR belongs
*/
setMyGrpNum()
{
   int i, groupnum, numActive,totalActive;
   
   totalActive = 0;
   workingGroup = 0;
   numberOfGroups = 1;
   /* go thought all ddrs and track which have reported being active to calc group memebership */
   for (numActive = 1, groupnum = 0, i=0; i <= MAX_DDRS; i++)
   {
      if (DDRStatus[i] > 0)  /* This ddr active? */
      { 
        /* When we reach the number of Active DDRs in a group
           increment the groupnumber and reset numActive to zero
           and keep going, When we are done the groupnum should the group
           this DDR should be in
        */
        if (numActive > MaxInGroup)
        {
	    groupnum++;
            numberOfGroups++;
            totalActive += numActive;
	    numActive = 1;
        }
        numInGrp[groupnum] = numActive;  /* update actual # of ddrs in this group */
        DDRGrp[i] = groupnum;		/* update which group this DDR belongs */
        DPRINT5(-4,"DDR%d, stat: %d, Active: %d, InGrp: %d, Grpnum: %d\n", i+1,DDRStatus[i],numActive,MaxInGroup,groupnum);
        numActive++;
      }
   }
   myWorkGroup = DDRGrp[myOrdinalNum];
   DPRINT3(-4,"DDR%d GrpNum: %d, # of Grps: %d\n",myOrdinalNum+1,myWorkGroup,numberOfGroups);
   if (totalActive != numActiveDDRs)
   {
       DPRINT2(-5,"numActiveDDRs: %d != totalActive: %d\n", totalActive, numActiveDDRs);
   }
   
   return 0;
}

/* 
 * determine if all ddrs belonging to the workingGroup have reported in as finished 
 *
 *		Author: Greg brissey  2/7/06
 */
int grpDone()
{
   int i;
   int status = 1;
   for (i=0; i < numInGrp[workingGroup]; i++)
   {
      if (ddrsInGrpAtBarrier[workingGroup][i] != 1)
      {
         status = 0;
         break;
      }
   }
}

/*
 * groupStart() 
 * This is call by the DDR upLink task prior to transfering data. 
 * If this DDR is not a member of the 'workingGroup' then it is pended.
 * 
 * If the ddr is part of the workingGroup but not all DDRs have reported
 * in as done then pend.  This is typyical that the ddr has report finished
 * and then calls Start prior to the other ddrs in the group reported finished 
 * Thus this ddr should pend.
 *
 * For the 1st time though, the intial starting group, must be allowed to continue
 * immediately therefore there is a firststart flag used to allow this only initially.
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
int groupStart()
{
    int mygroup;
    mygroup = DDRGrp[myOrdinalNum];
   DPRINT3(-4,"'ddr%d': groupStart: workingGroup: %d, myWorkGroup: %d \n",myOrdinalNum,workingGroup,mygroup);
   if ( mygroup != workingGroup)
   {
      firststart = 0;  /* if not in the first group clear this flag */
      DPRINT2(-4,"'ddr%d': groupStart: group: %d, Take semaphore & wait\n", myOrdinalNum,mygroup);
      semTake(pBarrierWaitSem,WAIT_FOREVER);
      DPRINT2(-4,"ddr%d': groupStart: Group: %d, Got Semaphore, Continuing \n",myOrdinalNum,mygroup); 
   } 
   else if ( grpDone() != 1 && (firststart != 1) ) /* if group is NOT done, and it not the 1st time though */
   {
      DPRINT2(-4,"'ddr%d': groupStart: group: %d, Take semaphore & wait\n", myOrdinalNum,mygroup);
      semTake(pBarrierWaitSem,WAIT_FOREVER);
      DPRINT2(-4,"'ddr%d': groupStart: Group: %d, Got Semaphore, Continuing \n",myOrdinalNum,mygroup); 
   }
   else
       firststart = 0;   /* let those in 1st group to continue, initially */
   return 0;
}

/*
 * groupFinish() 
 * This is call by the DDR upLink task after transfering data. 
 * 
 * A message is sent to all other participating DDRs, indicating which ddr and the group it is a member of
 * that has finished 
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
int groupFinish()
{
   /* report to all other DDRs that this cntlr is finished */
   sendDDRGrpSync(CNTLR_CMD_NEXT_WRKING_GRP, DDRGrp[myOrdinalNum],myOrdinalNum,0,0,NULL);  
   DPRINT3(-4,"'ddr%d': groupFinish: workingGroup: %d, myWorkGroup: %d \n",myOrdinalNum,workingGroup,DDRGrp[myOrdinalNum]);
   return 0;
}

/*
 * show routine for this contruct
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
grpshow(int level)
{
   showGrpBarrier(level);
}

showGrpBarrier(int level)
{
   int i;
   printf("DDR group barrier: \n");
   printf("# of Groups: %d, # in a group: %d\n", numberOfGroups, MaxInGroup);
   printf("'ddr%d': Group= %d, Present WorkingGroup: %d\n",myOrdinalNum+1,myWorkGroup,workingGroup);
   for( i=0; i < MAX_DDRS; i++)
   {  
      char *pstatstr;
      if (DDRStatus[i] > 0)
         pstatstr = "Active";
      else if (DDRStatus[i] == -1)
	 pstatstr = "InActive";
      else
	 continue;
	 /* pstatstr = "N/A"; */

      printf("ddr%d: '%s', Grp: %d  ",i+1,pstatstr,DDRGrp[i]);
      if (!(i%4)) printf("\n");
   }
   for( i=0; i < MAX_GRPS; i++)
   {  
      if (numInGrp[i] > 0)
       printf("Group %d, contains: %d ddrs\n",i,numInGrp[i]);
   }
   
   semShow(pBarrierWaitSem,level);
   printf("\n\n");
}

/* =================================================================================== */
/* =================================================================================== */
/* ++++++++++++++++++++++++++  DDR Sync Pub/Sun Routines +++++++++++++++++++++++++++++ */
/* =================================================================================== */
/* =================================================================================== */

static SEM_ID pDDRSyncPubMutex = NULL;

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
RTIBool DDRGrpSync_CommCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Cntlr_Comm *recvIssue;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        int mySeqNum, Active, group,mygroup, ddrOrdinalNum, i;

        recvIssue = (Cntlr_Comm *) instance;
        /* DPRINT6(-4,"\nDDRGrpSync_Comm CallBack:  nddsTopic: '%s', cmd: %d, arg1: %d, arg2: %d, arg3: %d \n",
        issue->nddsTopic,recvIssue->cmd,recvIssue->errorcode,recvIssue->warningcode, recvIssue->arg1, recvIssue->arg2); */
        switch(recvIssue->cmd)
        {
          /* This message indicated a ddr and if it's active in the data transfer */
          case CNTLR_CMD_SET_WRKING_GRP:
                mySeqNum = recvIssue->errorcode;
                Active = recvIssue->warningcode;
                DDRStatus[mySeqNum] = Active;
	        numActiveDDRs = recvIssue->arg1;
                setMyGrpNum();  /* recalc all ddr group memberships */
                while (semTake(pBarrierWaitSem,NO_WAIT) != ERROR);   /* make sure seamphore wll block */
                DPRINT3(-4," '%s': SyncGrpBarrierParser:  workingGroup: %d , numActiveDDRs: %d\n",
			recvIssue->cntlrId,workingGroup,numActiveDDRs);
                break;

           /* message indicating the ddr and it's group that has 'finished' */
           case CNTLR_CMD_NEXT_WRKING_GRP:
                group = recvIssue->errorcode;
                ddrOrdinalNum = recvIssue->warningcode;
                ddrsInGrpAtBarrier[group][ddrOrdinalNum] == 1;
                numAtGrpBarrier[group] += 1;
                mygroup =  DDRGrp[myOrdinalNum];
                DPRINT6(-4," '%s': Recv'd  myGrp: %d, Active Grp: %d, Updated Group: %d,Ordnial: %d, # @Barrier %d\n",
			recvIssue->cntlrId,mygroup,workingGroup,group,ddrOrdinalNum,numAtGrpBarrier[group]);
		    
                /* if the all the ddrs have reported 'finished' for the workingGroup, then move on
                 * to next group
                */
                if ( numAtGrpBarrier[workingGroup] == numInGrp[workingGroup] )
                {
                     for (i=0; i < numInGrp[workingGroup]; i++)  /* clear this grp */
                        ddrsInGrpAtBarrier[workingGroup][i] == -1;
		     numAtGrpBarrier[workingGroup] = 0;		 /* clear this as well */
                     workingGroup++;				 /* increment to next group */
                     workingGroup = (workingGroup % numberOfGroups); /* come back around to group 0 if needed */
                     /* if the new woring group is my group then give semaphore to unpend it */
                     if ( mygroup == workingGroup)
                     {
                      DPRINT2(-4," '%s': SyncGrpBarrierParser: workGroup: %d,  give Semaphore\n",recvIssue->cntlrId,workingGroup);
                      semGive(pBarrierWaitSem);
                     }
                }
                break;

           case 26:
                MaxInGroup = recvIssue->errorcode;
                DPRINT2(-4," '%s': SyncGrpBarrierParser:  MaxInGroup: %d\n", recvIssue->cntlrId,MaxInGroup);
                setMyGrpNum();
                break;

           case CNTLR_CMD_SET_NUM_ACTIVE_DDRS:
                numActiveDDRs = recvIssue->errorcode;
                break;

        }

    }
   return RTI_TRUE;
}

/*
 * Create a Exception Publication to communicate with the Cntrollers/Master
 *
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
NDDS_ID createDDRGrpSyncCommPub(NDDS_ID nddsId, char *topic, char *cntlrName)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     Cntlr_Comm  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* create the pub issue  Mutual Exclusion semaphore */
    pDDRSyncPubMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);


    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 0xbadc0de;  /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pPubObj);
         
    DPRINT2(-4,"Create Pub topic: '%s' for Cntlr: '%s'\n",pPubObj->topicName,cntlrName);
    createPublication(pPubObj);
    issue = (Cntlr_Comm  *) pPubObj->instance;
    strcpy(issue->cntlrId,cntlrName);   /* fill in the constant cntlrId string */
    return(pPubObj);
}        

/*
 * Create a Exception Subscription to communicate with the Controllers/Master
 *
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
NDDS_ID createDDRGrpSyncCommSub(NDDS_ID nddsId, char *topic, char *multicastIP, void *callbackArg)
{

   NDDS_ID  pSubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
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
    getCntlr_CommInfo(pSubObj);
 
    pSubObj->callBkRtn = DDRGrpSync_CommCallback;
    pSubObj->callBkRtnParam = NULL;
    if (multicastIP == NULL)
       pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    else
       strcpy(pSubObj->MulticastSubIP,multicastIP);

    DPRINT1(-4,"createDDRGrpSyncCommSub: create subscript for topic: '%s'\n",pSubObj->topicName);
    createSubscription(pSubObj);
    return ( pSubObj );
}


/*
 * The Barrier involved nodes via NDDS uses this callback function to create Subscriptions to the
 * DDRs/Master *
 *
 *                                      Author Greg Brissey  2/1/06
 *
 */
NDDSSubscription DDRGrpBarrier_CmdPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDSSubscription pSub;
     DPRINT3(-4,"DDRBarrier_CmdPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
		nddsTopic, nddsType, callBackRtnParam);
     pGroupSyncSub[numSubs] =  createDDRGrpSyncCommSub(NDDS_Domain, (char*) nddsTopic, 
			DDRSYNC_COMM_MULTICAST_IP, (void *) callBackRtnParam );
     pSub = pGroupSyncSub[numSubs++]->subscription;
     return pSub;
}

/*
 *  creates a pattern subscriber, to dynamicly allow subscription creation
 *  as master/ddrs come on-line and publish to the DDR Barrier
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */
DDRGrpBarrierPubPatternSub(void *callback)
{
    DDRGrpBarrierSubscriber = NddsSubscriberCreate(0);

    /* subscribe to any barrier publications from controllers */
    NddsSubscriberPatternAdd(DDRGrpBarrierSubscriber,  
           DDRGRPBARRIER_SUB_PATTERN_TOPIC_FORMAT_STR,  Cntlr_CommNDDSType , DDRGrpBarrier_CmdPatternSubCreate, (void *)callback); 
}


/*
 * DDRs will call this so they can participate in the group barrier
 *
 *                                      Author Greg Brissey  2/01/2006
 *
 */
void initialDDRGrpSyncComm()
{
    char pubtopic[128];
    DDRGrpBarrierPubPatternSub((void *) NULL);  /* start pattern subscription */
    sprintf(pubtopic,DDRGRPBARRIER_PUB_COMM_TOPIC_FORMAT_STR,hostName);
    DPRINT3(-1,"format: '%s', cntrlL '%s', topic: '%s'\n",DDRGRPBARRIER_PUB_COMM_TOPIC_FORMAT_STR,hostName,pubtopic);
    pGroupSyncPub = createDDRGrpSyncCommPub(NDDS_Domain,pubtopic, hostName );
}

/*
 * Used by All , to publish At Barrier message, etc.
 * A mulitcast publication thus all controllers will get this pub
 * even the one sending it
 *
 *
 *                                      Author Greg Brissey  2/01/06
 */
int sendDDRGrpSync(int cmd, int arg1,int arg2,int arg3,int arg4, char *strmsg)
{
   int status;
   Cntlr_Comm *issue;
/*
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_SENDEXCPT,NULL,NULL);
#endif
*/
   issue = pGroupSyncPub->instance;
   semTake(pDDRSyncPubMutex, WAIT_FOREVER);
      DPRINT(+1,"sendDDRGrpSync: got Mutex\n");
      issue->cmd  = cmd;  /* atoi( token ); */
      issue->errorcode = arg1;
      issue->warningcode = arg2;
      issue->arg1 = arg3;
      issue->arg2 = arg4;

      if (strmsg != NULL)
      {
         int strsize = strlen(strmsg);
        if (strsize <= COMM_MAX_STR_SIZE)
          strncpy(issue->msgstr,strmsg,COMM_MAX_STR_SIZE);
        else
          DPRINT2(-1,"msg to long: %d, max: %d\n",strsize,COMM_MAX_STR_SIZE);
      }
      status = nddsPublishData(pGroupSyncPub); /* send exception to other controllers via NDDS publication */
      if ( status == -1)
        DPRINT(-1,"sendDDRGrpSync: error in publishing\n");
      DPRINT(+1,"sendDDRGrpSync: give Mutex\n");
   semGive(pDDRSyncPubMutex);

/*
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_SENDEXCPT_CMPLT,NULL,NULL);
#endif
*/
     return 0;
}

/* 
 *  Some simple test routines to try it out and confirm proper operation..
 *
 *                                      Author Greg Brissey  2/01/06
 *
 */

initgrpbarcom()
{
  initGroupBarrier( BrdNum );
  initialDDRGrpSyncComm();
}

/* eg. for 4 ddr setactive 1,4 */
setactive(int active, int numactive)
{
  sendDDRGrpSync(CNTLR_CMD_SET_WRKING_GRP, BrdNum,active,numactive,0,NULL);
}

sndnumingrp(int ngroup)
{
  sendDDRGrpSync(26, ngroup,0,0,0,NULL);
}

tstgrpbarrier(int times, int delayticks)
{
    int i;
    int retval;
   int pTmpId, TmpPrior;
    int status;

   pTmpId = taskIdSelf();
   taskPriorityGet(pTmpId,&TmpPrior);
   taskPrioritySet(pTmpId,132);
   status = nddsPublicationIssuesWait(pGroupSyncPub, 5, 0);
   if ( status == -1)
        DPRINT(-1,"sendDDRGrpSync: error in waiting\n");
    for(i=0; i < times; i++)
    {
       /* DPRINT1(-1," %d: call barrier wait\n",i); */
       groupStart();
       ledOn();
       taskDelay(delayticks);
       ledOff();
       groupFinish();
    }
   taskPrioritySet(pTmpId,TmpPrior);
}


#endif  /* RTI_NDDS_4x */
