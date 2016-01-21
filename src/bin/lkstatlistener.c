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

#include "Lock_Stat.h"   /* NDDS publication structure */

#ifdef RTI_NDDS_4x
#include "Lock_StatPlugin.h"   /* NDDS publication structure */
#include "Lock_StatSupport.h"   /* NDDS publication structure */
#endif  /* RTI_NDDS_4x */

#define MULTICAST_ENABLE 1
#define MULTICAST_DISABLE 0


static int verbose = 0;

/** from locknddscom.c ***/

static NDDS_ID  pLockStatSub = NULL;
static NDDS_ID  NDDS_Domain = NULL;

char ConsoleNicHostname[80];

/*  NDDS 4x callback version */

void Lock_StatCallback(void* listener_data, DDS_DataReader* reader)
{
   void lockStatAction(Lock_Stat *recvIssue);
   Lock_Stat *recvIssue;
   // void consolestatAction(Console_Stat *data);
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;
   DDS_TopicDescription *topicDesc;


   struct Lock_StatSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Lock_StatDataReader *Lock_Stat_reader = NULL;

   Lock_Stat_reader = Lock_StatDataReader_narrow(pLockStatSub->pDReader);
   if ( Lock_Stat_reader == NULL)
   {
        // errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   // DPRINT2(1,"Lock_StatCallback: Type: '%s', Name: '%s'\n",
   /*printf("Lock_StatCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
*/
        retcode = Lock_StatDataReader_take(Lock_Stat_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 // errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
        }

        numIssues = Lock_StatSeq_get_length(&data_seq);
        // DPRINT1(1,"Lock_StatCallback: numIssues: %d\n",numIssues);
       // printf("Lock_StatCallback: numIssues: %ld\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              recvIssue = (Lock_Stat *) Lock_StatSeq_get_reference(&data_seq,i);
              lockStatAction(recvIssue); // call our action function
           }
        }
        retcode = Lock_StatDataReader_return_loan( Lock_Stat_reader,
                  &data_seq, &info_seq);
   return;
}

NDDS_ID  createLockStatSub(char *subName)
{
    NDDS_ID pSubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }

    /* DPRINT2(-1,"---> pipe fd[0]: %d, fd[1]: %d\n",lockSubPipeFd[0],lockSubPipeFd[1]); */
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_StatInfo(pSubObj);

    /* NDDS issue callback routine */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    /* i.e. 5 times per second max */
    pSubObj->BE_UpdateMinDeltaMillisec = 250;
    initBESubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Lock_StatCallback, NULL);
    createSubscription(pSubObj);
    return(pSubObj);
}


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

/*** from lockindds.c ***/
//  lock_stat struct
//    DDS_Long  lkon;
//    DDS_Long  locked;
//    DDS_Long  lkpower;
//    DDS_Long  lkgain;
//    DDS_Long  lkphase;
//    DDS_Long  lkLevelR;
//    DDS_Long  lkLevelI;
//    DDS_Long  lkSlopeR;
//    DDS_Long  lkSlopeI;
//    DDS_Double  lkfreq;
int cnt = 0;
// routine that print out the publications values
// possible extensions buffer and write to file 
// alter speed
// time stamp at modulo points
void lockStatAction(Lock_Stat *recvIssue)
{
    /* REMEMBER this function is called in the context of the NDDS call-back pthread
                and must be fairly quick to complete..  e.g. no fft here
    */

    if (cnt++ % 200 == 0) {
    printf("Power: %ld, Gain: %ld, Phase: %ld, Freq: %g\n",
    (long) recvIssue->lkpower,
    (long) recvIssue->lkgain,
    (long) recvIssue->lkphase,
    (double) recvIssue->lkfreq );
    printf("R Level: %ld, I Level: %ld, R Slope:: %ld, I Slope: %ld\n",
    (long) recvIssue->lkLevelR,
    (long) recvIssue->lkLevelI,
    (long) recvIssue->lkSlopeR,
    (long) recvIssue->lkSlopeI );
    }
    else
    printf("%6ld,%6ld\n",(long) recvIssue->lkLevelR,(long) recvIssue->lkLevelI);
    return;
}

int main(int argc, char **argv)
{
    sigset_t   blockmask,oldmask;
    int   k,stat;
    char  message[102];

    initiateNDDS();
    if (NDDS_Domain == NULL) {
	    exit(0);
    }

    pLockStatSub = createLockStatSub((char*) LOCK_PUB_STAT_TOPIC_FORMAT_STR);
    if (pLockStatSub == NULL) {
	       exit(0);
	 }
    
    // stupid infinite wait
    while ( 1 ) {
       sleep(10);
    }

    NDDS_Shutdown(NDDS_Domain);
    sleep(1);

	fprintf(stderr, " lock stat  exit... \n");
    exit(0);
}

