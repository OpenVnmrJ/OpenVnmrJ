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

/* Nirvana LockI */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "data.h"
#include "group.h"
#include "ACQ_SUN.h"
#include "vnmrsys.h"
#include "shrexpinfo.h"
#include "expQfuncs.h"
#include "shrstatinfo.h"
#include "acqcmds.h"
#include "graphics.h"
#include "mfileObj.h"
#include "sockets.h"
#include "errLogLib.h"

#include "Lock_FID.h"   /* NDDS publication structure */

#ifdef RTI_NDDS_4x
#include "Lock_FIDPlugin.h"   /* NDDS publication structure */
#include "Lock_FIDSupport.h"   /* NDDS publication structure */
#endif  /* RTI_NDDS_4x */

#define MULTICAST_ENABLE 1
#define MULTICAST_DISABLE 0


static MFILE_ID ifile = NULL;
static TIMESTAMP currentTimeStamp;

static int dispCount;
static int verbose = 0;
static int vtest = 0;
static int vstandby = 0;
static int vnmrFd = -1;
static int stoped = 0;
static char *vnmrAddr = NULL;
static unsigned int fidNum = 0;
static FID_STAT_BLOCK *fidstataddr = NULL;

/** from locknddscom.c ***/

static NDDS_ID  NDDS_Domain = NULL;
static NDDS_ID  pLockSub = NULL;

char ConsoleNicHostname[80];
extern int initVnmrComm();
extern int sendToVnmr();
#ifndef RTI_NDDS_4x
extern int createBESubscription();
extern RTIBool enableSubscription();
extern RTIBool disableSubscription();
#else /* RTI_NDDS_4x */
/* dummy functions for 4x */
enableSubscription() { };
disableSubscription() { };
#endif  /* RTI_NDDS_4x */

static int ignoreLockFIDSub = 0;

void saveLockData(short* data, int size)
{
     int k;
     short  *ptr;
     char mess[20];

     if (stoped)
	return;
     if (data == NULL) {
	fprintf(stderr, "Error: nvlocki fid data is NULL \n");
        if (verbose)
	   printf("Error: nvlocki fid data is NULL \n");
	return;
     }
     if (ifile == NULL)
	return;
     if (size < 4 || size > 1024) {
	fprintf(stderr, "Error: nvlocki fid data size is  %d \n", size);
        if (verbose)
	   printf("Error: nvlocki fid data size is  %d \n", size);
	return;
     }
     if (verbose)
	fprintf(stderr, "nvlocki:  fid data size is  %d \n", size);
     fidNum++;
     fidstataddr->elemId = fidNum;
     fidstataddr->np = size;
     fidstataddr->dataSize = size;
     ptr = (short *) fidstataddr + sizeof( FID_STAT_BLOCK );
     k = sizeof(short) * size;
     memcpy(ptr, data, k);
     if (vnmrAddr != NULL) {
        sprintf(mess, "locki('ready')\n");
	sendToVnmr(mess);
     }
}

void crateDummyData()
{
    short d[24];
    int   s;

    for (s = 0; s < 24; s++)
	d[s] = fidNum;
    saveLockData(d, 24); 
}


#ifndef RTI_NDDS_4x
/* NDDS 3x callback version */
RTIBool Lock_FIDCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
   Lock_FID *recvIssue;
   int   i;
   short *rptr,*iptr;

   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (verbose)
	fprintf(stderr,"Lock_FIDCallback ... \n");
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (Lock_FID *) instance;
     if (verbose)
         fprintf(stderr," lkfid.len: %lu\n",recvIssue->lkfid.len);

     dispCount++;
     if (!ignoreLockFIDSub)
     {
        if (recvIssue->lkfid.len > 0)
        {
           if (recvIssue->lkfid.val == NULL) {
		fprintf(stderr, "Error: nvlocki data is NULL. \n");
   		return RTI_TRUE;
	   }
	   if (verbose) {
              rptr = recvIssue->lkfid.val;
              iptr = recvIssue->lkfid.val+1;
              for(i=0;i < 12; i++)
              {
                 fprintf(stderr,"lkval[%d]:  r=%d, i=%d\n",i, *rptr++,*iptr++);
                 rptr++; iptr++;
              }
           }
           saveLockData(recvIssue->lkfid.val, recvIssue->lkfid.len);
        }
     }
   }
   else
   {
      if (verbose)
	fprintf(stderr," data were not available.\n");
      /* Not the best solution, but somehow nvlocki gets into a
       * state where it does not get FRESH data, even though
       * nddsSpy says FRESH data are available. Restarting
       * nvlocki fixes it.
       */
      if (vnmrAddr != NULL)
	sendToVnmr("locki('exit') lock_scan\n");
   }

   return RTI_TRUE;
}

#else /* RTI_NDDS_4x */

/*  NDDS 4x callback version */
void Lock_FIDCallback(void* listener_data, DDS_DataReader* reader)
{
   Lock_FID *recvIssue;
   short *rptr,*iptr;

   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   int i,numIssues;
   DDS_TopicDescription *topicDesc;


   struct Lock_FIDSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Lock_FIDDataReader *Lock_FID_reader = NULL;

   Lock_FID_reader = Lock_FIDDataReader_narrow(pLockSub->pDReader);
   if ( Lock_FID_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+3,"Console_StatCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
        retcode = Lock_FIDDataReader_take(Lock_FID_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
        }

        numIssues = Lock_FIDSeq_get_length(&data_seq);
        DPRINT1(+3,"Console_StatCallback: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              int dataLength;
              short *pLockFidData;
              recvIssue = (Lock_FID *) Lock_FIDSeq_get_reference(&data_seq,i);

              dataLength = DDS_ShortSeq_get_length(&(recvIssue->lkfid));
              if (verbose)
                  fprintf(stderr," lkfid len: %d\n",dataLength);
              DPRINT1(+3,"Console_StatCallback: lkfid len: %d\n",dataLength);

              dispCount++;
              if (!ignoreLockFIDSub)
              {
                 if (dataLength > 0)
                 {
                    pLockFidData = (short*) DDS_ShortSeq_get_contiguous_bufferI(&recvIssue->lkfid);
                    if (pLockFidData == NULL) {
		         fprintf(stderr, "Error: nvlocki data is NULL. \n");
   		         return;
	            }
	            if (verbose) {
                       rptr = pLockFidData;
                       iptr = pLockFidData+1;
                       for(i=0;i < 12; i++)
                       {
                          DPRINT3(-5,"lkval[%d]:  r=%d, i=%d\n",i, *rptr++,*iptr++);
                          // fprintf(stderr,"lkval[%d]:  r=%d, i=%d\n",i, *rptr++,*iptr++);
                          rptr++; iptr++;
                       }
                    }
                    
                    saveLockData(pLockFidData, dataLength);
                 }
              }
           }
        }
        retcode = Lock_FIDDataReader_return_loan( Lock_FID_reader,
                  &data_seq, &info_seq);
   return;
}

#endif  /* RTI_NDDS_4x */

static
char *gethostIP(char* hname, char *localIP)
{
   int ipval;
   struct in_addr in;
   struct hostent *hp;
   char **p;

   if (verbose)
      fprintf(stderr, "gethostbyname  %s\n", hname);
   if ( (hp = gethostbyname(hname)) == NULL) {
     fprintf(stderr, "error in getting hostname  '%s'\n", hname);
     return (NULL);
   }
   p = hp->h_addr_list;
   memcpy(&in.s_addr, *p, sizeof (in.s_addr));
   strcpy(localIP, inet_ntoa(in));
   if (verbose)
 	fprintf(stderr, "Local IP: '%s'\n",localIP);
   return(localIP);
}


/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications
*
***************************************************************/
int initiateNDDS(void)
{
    sigset_t   blockmask,oldmask;
    int stat;
    char *data;
    char localIP[80];
    char *get_console_hostname(void);
    int nddsDebugLevel;

    if (verbose)
	fprintf(stderr, "initiateNDDS....\n");
    strncpy(ConsoleNicHostname, get_console_hostname(), 47);
    ConsoleNicHostname[47] = '\0';

    /* set mask to block all signals, now any new threads will be spawned
       with this signal mask
    */
    data = gethostIP(ConsoleNicHostname, localIP);
    if (data == NULL)
        return (0);

   /* default verbosirty level for ndds is -1 not zero. */
   nddsDebugLevel = (verbose < 1) ? -1 : verbose;

#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(0, nddsDebugLevel, MULTICAST_ENABLE, localIP);
#else
    NDDS_Domain = nddsCreate(0, nddsDebugLevel, MULTICAST_DISABLE, localIP);
#endif

    if (NDDS_Domain == NULL) {
        if (verbose)
	    fprintf(stderr, "nddsCreate  failed....\n");
        return (0);
    }

    if (verbose)
	fprintf(stderr, "initiateNDDS  successful....\n");
   return (1);
}

#ifdef NOT_USED_ANYMORE
void ignoreLockSub(int on)
{
    ignoreLockFIDSub = (on > 0) ? 1 : 0;
    if (pLockSub == NULL)
	return;
    if (ignoreLockFIDSub)
    {
      if (verbose)
	fprintf(stderr, " disable Lock subscription \n");
      disableSubscription(pLockSub);
    }
    else
    {
      if (verbose)
	fprintf(stderr, " enable Lock subscription \n");
      enableSubscription(pLockSub);
    }
}
#endif


NDDS_ID  createLockSub(char *subName)
{
    NDDS_ID pSubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }

    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);
    fprintf(stderr,"createLockSub: topic: '%s' ... \n",pSubObj->topicName);  // for testing 

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */

    getLock_FIDInfo(pSubObj);

    /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Lock_FIDCallback;
    pSubObj->callBkRtnParam = NULL; /* write end of pipe */
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

    /* i.e. 4 times per second max, should be good enough for Lock Display */
    // pSubObj->BE_UpdateMinDeltaMillisec = 250;
    // pSubObj->BE_UpdateMinDeltaMillisec = 400;
    pSubObj->BE_UpdateMinDeltaMillisec = 100;  /* George Gray likes this much better, GMB */

#ifndef RTI_NDDS_4x
    createBESubscription(pSubObj);
#else /* RTI_NDDS_4x */
    initBESubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Lock_FIDCallback, NULL);
    createSubscription(pSubObj);
#endif  /* RTI_NDDS_4x */

    return(pSubObj);

}

void initiatePubSub()
{
    if (verbose)
	fprintf(stderr, " initiatePubSub ... \n");
    pLockSub = createLockSub(SUB_FID_TOPIC_FORMAT_STR);   /* "lock/fid" */
    if (verbose) {
	if (pLockSub == NULL)
	   fprintf(stderr, "  initiatePubSub failed... \n");
	else
	   fprintf(stderr, "  initiatePubSub done \n");
    }
}



/*** from lockindds.c ***/

static void
openShrMem()
{
    char                dataPath[ MAXPATH ];
    int      		dataSize;
    int      		ptSize;
    int      		numPts;
    unsigned long       *iaddr;

    if (getenv("vnmrsystem") == NULL)
	return;
    sprintf(dataPath, "%s/acqqueue/nvlocki.Data", getenv("vnmrsystem"));
    ptSize = sizeof( short );
    numPts = 1024;
    dataSize = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK ) + ptSize * numPts;
    ifile = mOpen(dataPath, dataSize, O_RDWR | O_CREAT);
    if (ifile == NULL) {
        fprintf( stderr, "nvlocki: can not access %s\n", dataPath);
	return;
    }
    if (verbose)
        fprintf( stderr, "nvlocki:  open shared memory.\n" );
    iaddr = (unsigned long *) ifile->mapStrtAddr;
    currentTimeStamp = *(TIMESTAMP *) iaddr;
    fidstataddr = (FID_STAT_BLOCK *)(ifile->mapStrtAddr + sizeof(TIMESTAMP));
    fidNum = 0;
    fidstataddr->elemId = 0;
    fidstataddr->doneCode = DATA_OK;
}


int main(int argc, char **argv)
{
    sigset_t   blockmask,oldmask;
    int   k,stat;
    char  message[102];

    /* set mask to block all signals, now any new threads will be spawned
       with this signal mask
    */
    sigfillset( &blockmask );
    stat = pthread_sigmask(SIG_BLOCK,&blockmask,&oldmask);
    if (stat != 0) {
        perror("Failed to set mask to block all signals:\n");
        return (0);
    }

    for (k = 0; k < argc; k++) {
	if (strcasecmp(argv[k], "-pipe") == 0) {
	    k++;
	    if (argv[k] != NULL) {
		vnmrFd = atoi(argv[k]);
	    }
	}
	else if (strcasecmp(argv[k], "-addr") == 0) {
	    k++;
	    if (argv[k] != NULL) {
		vnmrAddr = (char *) malloc(strlen(argv[k]) + 2);
		strcpy(vnmrAddr, argv[k]);
	    }
	}
	else if (strcasecmp(argv[k], "-debug") == 0)
	    verbose = 1;
	else if (strcasecmp(argv[k], "-test") == 0)
	    vtest = 1;
	else if (strcasecmp(argv[k], "-standby") == 0)
	    vstandby = 1;
    }
    stoped = 0;
    if (vstandby)
        stoped = 1;

    if (vnmrAddr != NULL)
	initVnmrComm(vnmrAddr);

    if (vnmrFd < 0) {
	verbose = 1;
	vtest = 1;
    }

    dispCount = 0;
    initiateNDDS();
    if (NDDS_Domain == NULL) {
	if (!vtest)
	    exit(0);
    }

    openShrMem();
    if (ifile == NULL)
	exit(0);

    if (NDDS_Domain != NULL) {
        initiatePubSub();
        if (pLockSub == NULL) {
	   if (!vtest)
	       exit(0);
	}
    }
   
    if (vnmrFd >= 0) {
        if (vstandby) {
            stoped = 1;
	    if (pLockSub != NULL)
      	       disableSubscription(pLockSub);
	}
	while ((k = read(vnmrFd, message, 100)) > 0) {
	     message[k] = '\0';
     	     if (verbose)
	   	fprintf(stderr, "nvlocki got message:  %s\n", message);
	     if (strcmp(message, "quit") == 0) {
		 stoped = 1;
     		 if (verbose)
		     fprintf(stderr, "nvlocki exit now.\n");
		 break;
	     }
	     else if (strcmp(message, "exit") == 0 || strcmp(message, "stop") == 0) {
		if (stoped == 0) {
     		    if (verbose)
		   	fprintf(stderr, "nvlocki:  stop now.\n");
		    stoped = 1;
		    if (pLockSub != NULL)
      	                disableSubscription(pLockSub);
		}
	     }
	     if (strcmp(message, "start") == 0) {
		if (stoped) {
		    stoped = 0;
     		    if (verbose)
		        fprintf(stderr, "nvlocki:  resume.\n");
		    if (pLockSub != NULL)
      		        enableSubscription(pLockSub);
		}
	     }
	     if (strcmp(message, "fake") == 0) {
		 crateDummyData();
	     }
	}
     	if (k <= 0 && verbose)
	   	fprintf(stderr, "Error: nvlocki read pipe failed.\n");
    }
    else if (pLockSub != NULL) { /* for test only */
       while (dispCount < 3) {
           // sleep(1);  // for testing with ddd
	} 
    }

    if (pLockSub != NULL) {
       nddsSubscriptionDestroy(pLockSub);
       sleep(1);
#ifndef RTI_NDDS_4x
       NddsDestroy(NDDS_Domain->domain); 
#else /* RTI_NDDS_4x */
      NDDS_Shutdown(NDDS_Domain);
#endif  /* RTI_NDDS_4x */
       sleep(1);
       /* disableSubscription(pLockSub); */
       /* ignoreLockSub(1); */
    }

    mClose(ifile);
    if (verbose)
	fprintf(stderr, " nvlocki  exit... \n");
    exit(0);
}

