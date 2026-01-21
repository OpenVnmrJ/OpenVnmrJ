
/*
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/*
*  Author: Greg Brissey   11/06/2007
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>


#include <errno.h>
#include <signal.h>

#ifdef RTI_NDDS_4x

#include "errLogLib.h"

// #include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "NDDS_SubFuncs.h"
#include "NDDS_PubFuncs.h"
#include "Console_Conf.h"
#include "Console_ConfPlugin.h"
#include "Console_ConfSupport.h"

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

#define NDDS_DOMAIN_NUMBER 0
#define  MULTICAST_ENABLE  1     /* enable multicasting for NDDS */
#define  MULTICAST_DISABLE 0     /* disable multicasting for NDDS */
#define  NDDS_DBUG_LEVEL 3

#ifndef QUERY_NDDS_COMP
static char  *ConsoleHostName = "wormhole";
#endif

extern NDDS_ID NDDS_Domain;
NDDS_ID pConfSub;

extern int initBESubscription(NDDS_ID pNDDS_Obj);
/* required HB for operation */
extern char ProcName[256];

extern pthread_t main_threadId;    /* main thread Id, so we can signal just this thread */

/*     NDDS additions */
/*---------------------------------------------------------------------------------- */
void printConfIssue(Console_Conf *recvIssue)
{
     printf("VxWorks Version: '%s'\n",recvIssue->VxWorksVersion);
     printf("RTI NDDS Version: '%s'\n",recvIssue->RtiNddsVersion);
     printf("PSG/Interpreter Version: '%s'\n",recvIssue->PsgInterpVersion);
     printf("Compile Data Time: '%s'\n",recvIssue->CompileDate);
     printf("MD5 Signitures for:\n");
     printf("         ddr: '%s'\n",recvIssue->ddrmd5);
     printf("    gradient: '%s'\n",recvIssue->gradientmd5);
     printf("        lock: '%s'\n",recvIssue->lockmd5);
     printf("      master: '%s'\n",recvIssue->mastermd5);
     printf("       nvlib: '%s'\n",recvIssue->nvlibmd5);
     printf("    nvScript: '%s'\n",recvIssue->nvScriptmd5);
     printf("         pfg: '%s'\n",recvIssue->pfgmd5);
     printf("        lpfg: '%s'\n",recvIssue->lpfgmd5);
     printf("          rf: '%s'\n",recvIssue->rfmd5);
     printf("     vxWorks: '%s'\n",recvIssue->vxWorksKernelmd5);
}

void Console_ConfCallback(void* listener_data, DDS_DataReader* reader)
{
   extern void processConfIssue(Console_Conf *recvIssue);
   Console_Conf *recvIssue;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   int i,numIssues;
   DDS_TopicDescription *topicDesc;


   struct Console_ConfSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Console_ConfDataReader *Console_Conf_reader = NULL;

   Console_Conf_reader = Console_ConfDataReader_narrow(pConfSub->pDReader);
   if ( Console_Conf_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(-5,"Console_ConfCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
        retcode = Console_ConfDataReader_take(Console_Conf_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
        }

        numIssues = Console_ConfSeq_get_length(&data_seq);
        DPRINT1(-1,"Console_ConfCallback: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              recvIssue = (Console_Conf *) Console_ConfSeq_get_reference(&data_seq,i);
              processConfIssue(recvIssue);
           }
        }
        retcode = Console_ConfDataReader_return_loan( Console_Conf_reader,
                  &data_seq, &info_seq);
   return;
}

/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications 
*   
***************************************************************/
#ifndef QUERY_NDDS_COMP
void initiateNDDS(int debuglevel)
{
    char localIP[80];
    /* NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP) */

#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_ENABLE,(char*) getHostIP(ConsoleHostName,localIP));
#else
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_DISABLE,(char*) getHostIP(ConsoleHostName,localIP));
#endif

   if (NDDS_Domain == NULL)
      errLogQuit(LOGOPT,debugInfo,"Sendproc: initiateNDDS(): NDDS domain failed to initialized\n");
}


void DestroyDomain()
{
   if (NDDS_Domain != NULL)
   {
      DPRINT1(1,"Infoproc: Destroy Domain: 0x%lx\n",NDDS_Domain);
      NDDS_Shutdown(NDDS_Domain); /* NddsDomainHandleGet(0) */
      usleep(400000);  /* 400 millisec sleep, give time for msge to be sent to NddsManager */
   }
}
#endif


NDDS_ID  createConsoleConfSub(char *subName)
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
    getConsole_ConfInfo(pSubObj);
 
    /* NDDS issue callback routine */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    /* i.e. 1 times per second max */
    pSubObj->BE_UpdateMinDeltaMillisec = 1000; 
    initBESubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Console_ConfCallback, NULL);
    createSubscription(pSubObj);
    return(pSubObj);
}




int initConfSub()
{
   /* topic names form: sub: master/h/constat, h/acqstat */
   pConfSub = createConsoleConfSub(HOST_SUB_CONF_TOPIC_FORMAT_STR);
   return 0;
}


#endif  /* RTI_NDDS_4x */
 


