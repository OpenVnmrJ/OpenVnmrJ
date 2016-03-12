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
3-10-2004,gmb  created 
*/

/*
DESCRIPTION

  This Task  handles the Acodes & Xcode DownLoad to the Console computer.
  Communication is via a NDDS publication that the downlinker subscribes to.
  Its actions depending on the commands it receives.

*/

// #define TIMING_DIAG_ON    /* compile in timing diagnostics */

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>

#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "mBufferLib.h"
#include "nameClBufs.h"
#include "nvhardware.h"
#include "AParser.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "PSGFileHeader.h"

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "Codes_Downld.h"

#ifdef RTI_NDDS_4x
#include "Codes_DownldPlugin.h"
#include "Codes_DownldSupport.h"
#endif  /* RTI_NDDS_4x */

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */


/* NDDS Primary Domain */
extern NDDS_ID NDDS_Domain;

extern char hostName[80];

/* main cluster buffer allocation for the 'name buffers' */
extern MBUFFER_ID mbufID ;

/* name buffer handle */
extern NCLB_ID nClBufId;

extern ACODE_ID pTheAcodeObject;


unsigned long nDownldTables;
unsigned long nDownldPatterns;
unsigned long nDownldCodes;
int downldTimeOut = 15;

/* 
** downlink format 
** 1. keystring = "downLink",":",
** 2. Buffering_Type," ",
** 3. Label or BaseLable," ",
** 3. Size(dyn) or MaxSize(fixed) in bytes
** 4. Number of Buffers being sent (fixed)
** 5. Starting Number (fixed) used to gen. postfix (E.G. BaseLabel+f%d -> Exp1f1)
*/

static int  dwnLnkrIsCon = 0;
static char *ErrStr = { "ERR" };
static char *OkStr = { "OK" };
static char RespondStr[20];

static NDDS_ID pDwnldSub, pDwnldPubReply;

static NCLBP pDwnLdBuf = NULL;

static MSG_Q_ID pBufReqQ;     /* MsgQ for buffer address of ready buffers */

static SEM_ID numBufSemId;

static SEM_ID DwnldCmpltSemId = NULL;
static int DownLoadActive = 0; /* set to one when sendproc is done */

static SEM_ID pTransferInProgressMutex = NULL;
static SEM_ID pSemDwnldCntChg = NULL;

static int send2Sendproc(int cmdtype,int status,char *msgStr);
/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered. 
 *   called with the context of the NDDS task n_rtu7400
 *
 */
/* NDDSRecvInfo : 
 * RTINtpTime localTimeWhenReceived:     The local time when the issue was received 
 * const char* nddsTopic:                The topic of the subscription receiving the issue 
 * const char* nddsType:                 The type of the subscription receiving the issue 
 * int publicationId:                    The publication's unique Id 
 * NDDSSequenceNumber publSeqNumber:     Sending (publication) Sequence Number (e.g. publSeqNumber.high, publSeqNumber.low)
 * NDDSSequenceNumber recvSeqNumber:     Receiving Sequence Number 
 * RTINtpTime remoteTimeWhenPublished:   The remote time when the issue was published 
 * unsigned int senderAppId:             The sender's application Id 
 * unsigned int senderHostId:            The sender's host Id 
 * unsigned int senderNodeIP:            The sender's IP address 
 * NDDSRecvStatus status:                The status affects which fields are valid 
 * RTIBool validRemoteTimeWhenPublished: Whether or not a valid remote time was received 
 */
/* NDDSSequenceNumber: long high, unsigned long low */
/* NDDSRecvStatus status Values:
 *  NDDS_NEVER_RECEIVED_DATA:  Never received an issue, but a deadline occurred
 *  NDDS_NO_NEW_DATA:          Received at least one issue. A deadline occurred since the last issue received.
 *  NDDS_UPDATE_OF_OLD_DATA:   Received a new issue, whose time stamp is the same or older than the time stamp 
 *                               of the last fresh issue received. Most likely caused by the presence of multiple 
 *                               publications with the same publication topic running on computers with clocks 
 *                               that are not well synchronized.
 * NDDS_FRESH_DATA:            A new issue received
 * NDDS_DESERIALIZATION_ERROR: The deserialization method for the NDDSType returned an error. Most likely caused 
 *                               by an inconsistent interpretation of the NDDSType between the publishing and 
 *                               subscribing applications. This can be caused by an inconsistent specification 
 *                               of the NDDSType or of the maximum size of arrays or strings within the NDDSType. 
 */

#ifndef RTI_NDDS_4x
RTIBool Codes_DownldCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Codes_Downld *recvIssue;
    int bufcmd;

    if (issue->status != NDDS_FRESH_DATA)
    {
       /* DPRINT(-1,"---->  Codes_DownldCallback called, issue not fresh  <------\n"); */
       return RTI_TRUE;
    }

    if (DebugLevel > 0)
       printNDDSInfo(issue);

     recvIssue = (Codes_Downld *) instance;
      DPRINT6(2,"Downld CallBack:  cmdType: %d, status: %d, sn: %ld, totalBytes: %ld, datalen: %d, crc: 0x%lx\n",
        recvIssue->cmdtype,recvIssue->status, recvIssue->sn, recvIssue->totalBytes, recvIssue->data.len,recvIssue->crc32chksum); 

      panelLedOn(DATA_XFER_LED);
      switch(recvIssue->cmdtype)
      {
         case C_NAMEBUF_QUERY:

              /* use of a separate task to handle the reply is required, since the task may have to pend
	         until buffers are available 
              */
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_BUFNUM_QUERY,NULL,NULL);
             #endif
              bufcmd = C_NAMEBUF_QUERY;
              if ( msgQSend(pBufReqQ, (char*) &(bufcmd), 4 /*sizeof(int) */, NO_WAIT, MSG_PRI_NORMAL) == ERROR)
              {
                 errLogRet(LOGIT,debugInfo,
	              "!!!!!!!!!!!!!!!!! E R R O R  -- Lost buffer Q Query (C_NAMEBUF_QUERY), no space in msgQ\n");
                 sendException(HARD_ERROR, SYSTEMERROR + MSGQ_LOST_ERR, 0,0,NULL);
              }

	   break;
         case C_DOWNLOAD:
               nameClBufXfer(recvIssue);
		break;

          case C_DWNLD_START:
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_DNWLD_START,NULL,NULL);
             #endif
                DPRINT1(-1," --------  C_DNWLD_START Received, give Semaphore, timeout: %ld\n",recvIssue->status);

                nDownldTables = nDownldPatterns = nDownldCodes =0;
                downldTimeOut = recvIssue->status;
		/* issue->sn = arg2; */
                DownLoadActive = 1;
                /* if Sem is, empty then still empty, if full now empty, mission accomplished */
                semTake(numBufSemId,NO_WAIT);  /* just to be sure semaphore is empty */
                /* It's been observed on occasion that the Phandler is hung in taking this semaphore
                 *  when trying to delete all the downloaded acodes, etc.
	         *  This is semaphore is taken when an table/acode transfer from Sendproc starts 
                 *  then gives it back when the transfer for that table/acode is compelete
                 *  since a table/acode can take several issue to get downloaded completely. It appears
                 *  in some instantances that Sendproc is stop during this phase thus never giving this semaphore back
                 *  thus hanging phandler when it attempt to free all the download buffers
		 *  So we give it here and at C_DWNLD_CMPLT just in case.
                */
                /* semGive(pTransferInProgressMutex); /* free up transfer semaphore, just incase */
                break;

          case C_DWNLD_CMPLT:
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_DNWLD_FINISH,NULL,NULL);
             #endif
                DPRINT(-1," ---------- C_DNWLD_CMPLT Recieved, give Semaphore\n");
                DownLoadActive = 0;
		          semGive(DwnldCmpltSemId);
                /* semGive(pTransferInProgressMutex); /* free up transfer semaphore, just incase */
                break;
      }
      panelLedOff(DATA_XFER_LED);

   return RTI_TRUE;
}

#else  /* RTI_NDDS_4x */

void Codes_DownldCallback(void* listener_data, DDS_DataReader* reader)
{
   Codes_Downld *recvIssue;
   int bufcmd;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues,dataLength;

   struct Codes_DownldSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Codes_DownldDataReader *CodesDownld_reader = NULL;

   CodesDownld_reader = Codes_DownldDataReader_narrow(pDwnldSub->pDReader);
   if ( CodesDownld_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   retcode = Codes_DownldDataReader_take(CodesDownld_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

   if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
   } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
   }

   numIssues = Codes_DownldSeq_get_length(&data_seq);
   for (i=0; i < numIssues; i++)
   {
       info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
       if (info->valid_data)
       {

           // if (DebugLevel > 0)
              // printNDDSInfo(issue);

         recvIssue = (Codes_Downld *) Codes_DownldSeq_get_reference(&data_seq,i);
         dataLength = DDS_OctetSeq_get_length(&(recvIssue->data));
         DPRINT6(2,"Downld CallBack:  cmdType: %d, status: %d, sn: %ld, totalBytes: %ld, datalen: %d, crc: 0x%lx\n",
                           recvIssue->cmdtype,recvIssue->status, recvIssue->sn, recvIssue->totalBytes, 
                           dataLength,recvIssue->crc32chksum); 

         panelLedOn(DATA_XFER_LED);
         switch(recvIssue->cmdtype)
         {
            case C_NAMEBUF_QUERY:

              /* use of a separate task to handle the reply is required, since the task may have to pend
	         until buffers are available 
              */
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_BUFNUM_QUERY,NULL,NULL);
             #endif
              bufcmd = C_NAMEBUF_QUERY;
              if ( msgQSend(pBufReqQ, (char*) &(bufcmd), 4 /*sizeof(int) */, NO_WAIT, MSG_PRI_NORMAL) == ERROR)
              {
                 errLogRet(LOGIT,debugInfo,
	              "!!!!!!!!!!!!!!!!! E R R O R  -- Lost buffer Q Query (C_NAMEBUF_QUERY), no space in msgQ\n");
                 sendException(HARD_ERROR, SYSTEMERROR + MSGQ_LOST_ERR, 0,0,NULL);
              }

	      break;
            case C_DOWNLOAD:
                  nameClBufXfer(recvIssue);
#ifdef TIMING_DIAG_ON
                  {
                    double delta,duration;
                    double getTimeStampDurations(double *delta,double *duration);
                    updateTimeStamp(0);
                    getTimeStampDurations(&delta,&duration);
                    DPRINT2(-19,"C_DOWNLOAD: delta: %lf usec, duration: %lf usec\n",delta,duration);
                  }
#endif
		   break;

             case C_DWNLD_START:
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_DNWLD_START,NULL,NULL);
             #endif
                DPRINT1(-1," --------  C_DNWLD_START Received, give Semaphore, timeout: %ld\n",recvIssue->status);
#ifdef TIMING_DIAG_ON
                {
                    double delta,duration;
                    double getTimeStampDurations(double *delta,double *duration);
                    updateTimeStamp(0);
                    getTimeStampDurations(&delta,&duration);
                    DPRINT2(-19,"C_DWNLD_START: delta: %lf usec, duration: %lf usec\n",delta,duration);
                }
#endif
                nDownldTables = nDownldPatterns = nDownldCodes =0;
                downldTimeOut = recvIssue->status;
                /* issue->sn = arg2; */
                DownLoadActive = 1;
                /* if Sem is, empty then still empty, if full now empty, mission accomplished */
                semTake(numBufSemId,NO_WAIT);  /* just to be sure semaphore is empty */
                /* It's been observed on occasion that the Phandler is hung in taking this semaphore
                 *  when trying to delete all the downloaded acodes, etc.
                 *  This is semaphore is taken when an table/acode transfer from Sendproc starts 
                 *  then gives it back when the transfer for that table/acode is compelete
                 *  since a table/acode can take several issue to get downloaded completely. It appears
                 *  in some instantances that Sendproc is stop during this phase thus never giving this semaphore back
                 *  thus hanging phandler when it attempt to free all the download buffers
                 *  So we give it here and at C_DWNLD_CMPLT just in case.
                */
                /* semGive(pTransferInProgressMutex); /* free up transfer semaphore, just incase */
                   break;

             case C_DWNLD_CMPLT:
             #ifdef INSTRUMENT
                wvEvent(EVENT_DOWNLINK_DNWLD_FINISH,NULL,NULL);
             #endif
                DPRINT(-1," ---------- C_DNWLD_CMPLT Recieved, give Semaphore\n");
                DownLoadActive = 0;
		          semGive(DwnldCmpltSemId);
#ifdef TIMING_DIAG_ON
                {
                    double delta,duration;
                    double getTimeStampDurations(double *delta,double *duration);
                    updateTimeStamp(0);
                    getTimeStampDurations(&delta,&duration);
                    DPRINT2(-19,"C_DWNLD_CMPLT: delta: %lf usec, duration: %lf usec\n",delta,duration);
                }
#endif
                /* semGive(pTransferInProgressMutex); /* free up transfer semaphore, just incase */
                   break;
         }
         panelLedOff(DATA_XFER_LED);
       }
   }
   retcode = Codes_DownldDataReader_return_loan( CodesDownld_reader,
                  &data_seq, &info_seq);
   return;
}

#endif  /* RTI_NDDS_4x */

void incrementTypeCount(ComboHeader  *infoHeader)
{
    long headerType;
    headerType = infoHeader->comboID_and_Number & 0xF0000000;
    DPRINT2(+1,"comboID_and_Number: 0x%lx, ACODEHEADER: 0x%lx\n",infoHeader->comboID_and_Number,ACODEHEADER);
    switch (headerType)
    {
        case INITIALHEADER:
	     break;
       case ACODEHEADER: 
               nDownldCodes++;
               semGive(pSemDwnldCntChg);
               #ifdef INSTRUMENT
                   wvEvent(EVENT_DOWNLINK_DNWLD_ACODE,NULL,NULL);
               #endif
	       break;
       case PATTERNHEADER:
               nDownldPatterns++;
               #ifdef INSTRUMENT
                   wvEvent(EVENT_DOWNLINK_DNWLD_PATTERN,NULL,NULL);
               #endif
	       break;
       case  TABLEHEADER:
               nDownldTables++;
               #ifdef INSTRUMENT
                   wvEvent(EVENT_DOWNLINK_DNWLD_TABLE,NULL,NULL);
               #endif
               break;
       default: 
	       DPRINT1(-1,"Misc File %x\n",headerType);
               #ifdef INSTRUMENT
                   wvEvent(EVENT_DOWNLINK_DNWLD_MISC,NULL,NULL);
               #endif
               break;
    }
    DPRINT3(0,"Recv'd: Acodes: %d, Patterns: %d, Tables: %d\n", nDownldCodes,nDownldPatterns,nDownldTables);
}

/*
    This routine allocates a named buffer and places the data into it. If the data spans
    more than one issue, then the offset is non zero, and the named buffer is looked up
    and then appended to.
 
    The ackInterval is modulo with the sn, then at the ackInterval either an OK status
    of error status is returned.  The host side controls the ackInterval. The few the 
    ack required the faster the transfer but the more delayed an error might be reported.

    When the total bytes recieved equal the indicated totalBytes the named buffer is marked
    from LOADING to READY. At that time a CRC32 chksum is calculated and compared to the sent
    checksum. If they are not equal the error is recorded so when it is time for an ack the
    error can be reported.

*/
nameClBufXfer(Codes_Downld *issue)
{
       ComboHeader  *infoHeader;
       long headerType;
       int ackNow;
       long dataLength;

#ifndef RTI_NDDS_4x
       dataLength = issue->data.len;
#else  /* RTI_NDDS_4x */
       dataLength = DDS_OctetSeq_get_length(&(issue->data));
#endif  /* RTI_NDDS_4x */

        /* if addr offset zero, make a new entry, otherwise find entry and append to it */
        if (issue->dataOffset == 0)
        {
            DPRINT4(-1,"Receiving: '%s', total size: %ld, size sent:%ld, offset: %ld\n",
                 issue->label, issue->totalBytes, dataLength, issue->dataOffset);
            #ifdef INSTRUMENT
               wvEvent(EVENT_DOWNLINK_XFER_MUTEX_TAKE,NULL,NULL);
            #endif
            semTake(pTransferInProgressMutex,WAIT_FOREVER);

            #ifdef INSTRUMENT
               wvEvent(EVENT_DOWNLINK_MAKE_ENTRY,NULL,NULL);
            #endif
            pDwnLdBuf = nClbMakeEntry(nClBufId,issue->label,issue->totalBytes,0);
            if (pDwnLdBuf == NULL)   /* this is an error */
            {
               int bufcmd;

               #ifdef INSTRUMENT
                  wvEvent(EVENT_DOWNLINK_XFER_ERROR,NULL,NULL);
               #endif
               #ifdef INSTRUMENT
                  wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
               #endif

               semGive(pTransferInProgressMutex);

               /* tell sendproc TOO BIG & max size */
               bufcmd = 666;
               msgQSend(pBufReqQ, (char*) &bufcmd, 4 /*sizeof(int) */, NO_WAIT, MSG_PRI_NORMAL);
               sendException(HARD_ERROR, SYSTEMERROR + DYNBUFNOSPACE, 0,0,NULL);
	       return -1;
            }
        }
        else
        {
            #ifdef INSTRUMENT
               wvEvent(EVENT_DOWNLINK_XFER_MUTEX_TAKE,NULL,NULL);
            #endif
            semTake(pTransferInProgressMutex,WAIT_FOREVER);

            #ifdef INSTRUMENT
               wvEvent(EVENT_DOWNLINK_FIND_ENTRY,NULL,NULL);
            #endif
            pDwnLdBuf = nClbFindEntry(nClBufId,issue->label);
            DPRINT4(+2,"Receiving: '%s', total size: %ld, size sent:%ld, offset: %ld\n",
                 issue->label, issue->totalBytes, dataLength, issue->dataOffset);

            /* this is a result of a named buffer being deleted after the start of a transfer
             * since since should only happen during an aa or core dump on the procs 
             * this treated not as an error, but we will through this load load out
             */
            if (pDwnLdBuf == NULL)
            {
               #ifdef INSTRUMENT
                  wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
               #endif

               semGive(pTransferInProgressMutex);

            }
        }

        DPRINT1(1,"buffer state: %d (MT=1,LOADING=8,READY=2,DONE=4)\n",pDwnLdBuf->status);

        if ( pDwnLdBuf != NULL )
        {
             char *pDst;
             long diff;
             unsigned long calccrc;
             pDst =(char*) (((unsigned long) pDwnLdBuf->data_array) +  issue->dataOffset);
             diff = ((unsigned long) pDst) - ((unsigned long) pDwnLdBuf->data_array);
             DPRINT3(1,"dst strt: 0x%lx, cp@: 0x%lx, diff: %d\n", pDwnLdBuf->data_array, pDst, diff);
#ifndef RTI_NDDS_4x
             memcpy(pDst, issue->data.val,issue->data.len);
#else  /* RTI_NDDS_4x */
             DDS_OctetSeq_to_array(&(issue->data), pDst, dataLength);
#endif  /* RTI_NDDS_4x */

             if ((dataLength + issue->dataOffset) == issue->totalBytes)      
             {
                  #ifdef INSTRUMENT
                     wvEvent(EVENT_DOWNLINK_XFER_CMPLT,NULL,NULL);
                  #endif
                  calccrc = addbfcrc(pDwnLdBuf->data_array, issue->totalBytes);
		  DPRINT2(1,"recv'd crc: 0x%lx, calc crc: 0x%lx\n",issue->crc32chksum, calccrc);
                  pDwnLdBuf->status = READY;
		  if (issue->crc32chksum == calccrc)
                  {
                     pDwnLdBuf->status = READY;
	             infoHeader = (ComboHeader *) pDwnLdBuf->data_array;
                     incrementTypeCount(infoHeader);  /* increment appropriate counter */
                     #ifdef INSTRUMENT
                        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
                     #endif
                     semGive(pTransferInProgressMutex);
                  }
	          else
                  {
                     pDwnLdBuf->status = MT;
                     #ifdef INSTRUMENT
                        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
                     #endif
                     semGive(pTransferInProgressMutex);
                     errLogRet(LOGIT,debugInfo,
	              "!!!!!!!!!!!!!!!!! E R R O R  -- download: CRC32 CheckSUM mismatch  given: 0x%lx , calculated: 0x%lx !!!!!!!!!!!!!\n",issue->crc32chksum, calccrc);
                     sendException(HARD_ERROR, SYSTEMERROR + DWNLD_CRC_ERR, 0,0,NULL);
                  }
                  /* semGive(updateSemID); */
              }
              else
              {
                 #ifdef INSTRUMENT
                    wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
                 #endif
                 semGive(pTransferInProgressMutex);
              }

              DPRINT5(1,"Recv: '%s',1- 0x%lx 2- 0x%lx, 3- 0x%lx 4- 0x%lx\n",
		issue->label, pDwnLdBuf->data_array[0],
		pDwnLdBuf->data_array[1],
		pDwnLdBuf->data_array[2],
		pDwnLdBuf->data_array[3] );
        }
        else
        {
              DPRINT(-1,"pDwnLdBuf was NULL, namebuffer must of been deleted after transfer started \n");
       }
}

resetDwnldCmpltSem()
{
  DownLoadActive = 0;
  while (semTake(DwnldCmpltSemId,NO_WAIT) != ERROR);	
}

wait4DwnldCmplt(int timeoutTicks)
{
   int result;
     DPRINT1(-1,"wait4DwnldCmplt: DownLoadActive: %d\n",DownLoadActive);
     #ifdef INSTRUMENT
          wvEvent(EVENT_DOWNLINK_DWNLD_WAIT4CMPLT,NULL,NULL);
     #endif
     while (DownLoadActive != 0)
     {
          result = semTake(DwnldCmpltSemId,timeoutTicks);  
          if (result == ERROR)
          {
             #ifdef INSTRUMENT
                  wvEvent(EVENT_DOWNLINK_DWNLD_WAIT4CMPLT_ERROR,NULL,NULL);
             #endif
              return result;
          }
          DPRINT1(-1,"wait4DwnldCmplt: Got Sem, DownLoadActive: %d\n",DownLoadActive);
     }
     #ifdef INSTRUMENT
          wvEvent(EVENT_DOWNLINK_DWNLD_CMPLT,NULL,NULL);
     #endif
     return 0;
}

/*
 * routine call by the controller phandler after an error that deletes 
 * the download acodes,patterns, etc.
 * wait on a the DownLoadActive flag to determine if download has completed
 * If timeouts then deletes some to allow download to continue so that the end message can be recieved.
 * Does this max of twenty times prior to failing.
 * Once the end message is obtain it once again deletes any remaining download buffers
 */
freeAllDwnldBufsSynced()
{
   int result;
   int retrycnt;
   char rootName[64];
   sprintf(rootName,"%s",pTheAcodeObject->id);
   DPRINT(-1,"Waiting for Dwnld Completion message from Sendproc\n");
   /* timeout, incase Sendproc is waiting on us for number of free buffers returned. */
   /* if it's hung and we wait forever then nobody goes anyhwere */
   retrycnt = 20;
   #ifdef INSTRUMENT
         wvEvent(EVENT_DOWNLINK_SYNCDELETE,NULL,NULL);
   #endif
   while ( DownLoadActive != 0)
   {
        #ifdef INSTRUMENT
               wvEvent(EVENT_DOWNLINK_SYNCDELETE_SEMTAKE,NULL,NULL);
        #endif
        result = semTake(DwnldCmpltSemId,10);    /* 120 msec */
        DPRINT2(-1,"freeAllDwnldBufsSynced: DownLoadActive: %d, result: '%s'\n",
		DownLoadActive, ((result != ERROR) ? "OK" : "TIMEDOUT") );
        /* timed out, then free some buffers to get things going if needed */
        if (result == ERROR)
        {
           #ifdef INSTRUMENT
                  wvEvent(EVENT_DOWNLINK_SYNCDELETE_SEMTIMEOUT,NULL,NULL);
           #endif
           if ( --retrycnt  < 1 )
	      break;
           DPRINT(-1,"Sem Timed out, so free some to get things going if needed.\n");
           delBufsByRootName(rootName);  
        }
   }
   if ( retrycnt < 1) 
   {
       #ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINK_SYNCDELETE_FAIL,NULL,NULL);
       #endif
	/* Oops, can result in an endless loop if Sendproc never sends transfer DONE msge */
       /* sendException(HARD_ERROR, SYSTEMERROR + DWNLD_CMPLT_ERR, 0,0,NULL); */
       DPRINT(-1,"Never Got it., Free all buffers\n");
   }
   else
   {
      DPRINT(-1,"Got it., Free all buffers\n");
   }
   /* delete all named buffers starting with the root name */
   delBufsByRootName(rootName);  
   #ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINK_SYNCDELETE_CMPLT,NULL,NULL);
   #endif
}

downldCntShow()
{
   DPRINT3(-1,"nDownldCodes: %lu, nDownldTables: %lu, nDownldPatterns: %lu\n",
		nDownldCodes, nDownldTables, nDownldPatterns);
}
void resetDownldCnts()
{
    nDownldCodes = nDownldTables = nDownldPatterns = 0;
}

int wait4NumDownLoad(int numWait4, int seconds)
{
  int status, delayticks;
 
  #ifdef INSTRUMENT
       wvEvent(EVENT_DOWNLINK_WAIT4DWNLD_CNT,NULL,NULL);
  #endif

  delayticks = calcSysClkTicks(seconds * 1000);
  // DPRINT2(-19,"wait4NumDownLoad: timeout: %d sec, %d ticks\n",seconds,delayticks);

  while( nDownldCodes < numWait4 )
  {
      if ( (status = semTake(pSemDwnldCntChg, delayticks)) != OK )
	   {
         #ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINK_WAIT4DWNLD_TIMEOUT,NULL,NULL);
         #endif
         // DPRINT(-11,"wait4NumDownLoad: Take of Semaphore Timed Out.\n");
         return -1;
	    }
       #ifdef INSTRUMENT
          wvEvent(EVENT_DOWNLINK_WAIT4DWNLD_CNT_CMPLT,NULL,NULL);
       #endif
       // DPRINT(-11,"wait4NumDownLoad: Semaphore was Given, Try chkStates again.\n");
  }
  return nDownldCodes;
}

#ifdef RTI_NDDS_4x
int publishDownldReply(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Codes_DownldDataWriter *CodesDownld_writer = NULL;

   CodesDownld_writer = Codes_DownldDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (CodesDownld_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"DataWriter narrow error\n");
        return -1;
   }

   result = Codes_DownldDataWriter_write(CodesDownld_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"DataWriter write error: %d\n",result);
   }
   return 0;
}
#endif  /* RTI_NDDS_4x */

static
int send2Sendproc(int cmdtype,int status,char *msgStr)
{
     Codes_Downld *issue;
     RTINtpTime                maxWait    = {10,0};
     issue = (Codes_Downld *) pDwnldPubReply->instance;
     issue->cmdtype = cmdtype;
     issue->status = status;
     issue->totalBytes = 0;
     issue->dataOffset = 0;
     issue->sn = 0;
     strcpy(issue->msgstr,msgStr);
     issue->crc32chksum = 0;
#ifndef RTI_NDDS_4x
     issue->totalBytes = issue->data.len = 0;
     nddsPublishData(pDwnldPubReply);
#else  /* RTI_NDDS_4x */
     issue->totalBytes = 0;
     DDS_OctetSeq_set_length(&(issue->data),0);
     publishDownldReply(pDwnldPubReply);
#endif  /* RTI_NDDS_4x */
     /*  wait for a maximum of 10 sec for the send Q to drop to zero. */
     /* nddsPublicationIssuesWait(pDwnldPubReply, 10, 0); */
     return(0);
}

/*
 * marks the currenting downloading buffer as ready, 
 * Usage is for an abort so we can mark the buffer so it can be deleted
 *
 */
int clearCurrentNameBuffer()
{
   if (pDwnLdBuf == NULL)
      return(-1);
   pDwnLdBuf->status = READY;
    return(0);
}


#ifdef NO_YET_IMPLEMENTED
setDwnLkAbort()
{
    DownLinkerAbort = 1;
}

clrDwnLkAbort()
{
    DownLinkerAbort = 0;
    DownLinkerActive = 0;
}

int downLinkerIsActive()
{
    return(DownLinkerActive);
}

chngDwnLkrPrior(int priority)
{
   taskPrioritySet(priority);
}

restoreDwnLkrPrior()
{
   taskPrioritySet(DownLkrPrior);
}
#endif

/*
   task to reqport number of name buffers available back to the host
   Wait on the msgQ for requests.
   When a request comes in there are two possbilities.
   1. No avaiable buffers
       This task pends waiting on the 'numBufSemId' semaphore
        This semaphore must be given when a buffer is returned.
	 (see delBufs())
       When task comes back from the semaphore, it check again for free buffers.
       If still zero it will pend again.
       Otherwise it proceeds to step 2.

   2. Available buffer
      task publishes the number of avialble buffer to the host 
*/      
bufferRequest(void)
{
   NDDS_ID createCodeDownldPublication(NDDS_ID nddsId);
   int bytes, freeBufs,cmd,key;
   Codes_Downld *pIssue;
    /* create the publication within the task using it!! */
    pDwnldPubReply = createCodeDownldPublication(NDDS_Domain);
#ifdef RTI_NDDS_4x
    key = (BrdType << 8) | BrdNum;   /* key needs to be unique per board sent to Recvproc */
    Register_Codes_Downldkeyed_instance(pDwnldPubReply,key);
    pIssue = (Codes_Downld*) pDwnldPubReply->instance;
    strncpy(pIssue->nodeId,hostName,MAX_CNTLRSTR_SIZE);
    DPRINT1(+7,"----->> nodeId : '%s'\n",pIssue->nodeId);
#endif  /* RTI_NDDS_4x */
    FOREVER
    {
       /* DPRINT(-1,"bufferRequest() - msgQReceive()\n"); */
       bytes = msgQReceive(pBufReqQ, (char*) &cmd, 4, WAIT_FOREVER);
       DPRINT1(+2,"bufferRequest() cmd: %d\n",cmd); 
       switch(cmd)
       {
         case C_XFER_ACK:
               DPRINT(-1,"bufferRequest() - Send Sendproc C_XFER_ACK\n");
               send2Sendproc(C_XFER_ACK,0 /* OK */,"OK ");
	      break;

         case 666:
               DPRINT(-1,"bufferRequest() - Send Sendproc 666\n");
               send2Sendproc(666,0,"No SPACE");
	       break;

         case C_NAMEBUF_QUERY:
             freeBufs = nClbFreeBufs(nClBufId);
             /* DPRINT1(-1,"bufferRequest() - freeBufs: %d\n",freeBufs); */
             while (freeBufs < 1)
             {
                  DPRINT(-1,"bufferRequest() - freeBufs is Zero, wait till there are some.\n");
                  semTake(numBufSemId,WAIT_FOREVER);
                  freeBufs = nClbFreeBufs(nClBufId);
             }
             DPRINT1(-1,"bufferRequest() - Send Sendprocs freeBufs = %d\n",freeBufs);
             /* I thought that limiting the free buffers might stop sendproc thread from */
             /*   hoggin the CPU, no it just queuries and gets a new freeBuf size and keeps on going  */
             /*   I'll have to do something else.   GMB 2/18/05 				*/
             /* freeBufs = (freeBufs > PARSE_AHEAD_COUNT+1) ? PARSE_AHEAD_COUNT : freeBufs; */
             /* DPRINT1(-1,"bufferRequest() - Send Sendprocs freeBufs = %d\n",freeBufs); */
             send2Sendproc(C_QUERY_ACK,freeBufs,"NAMEBUF_QUERY: ACK");
	     break;

         default:
	     DPRINT1(-1,"Unknown request, cmd: %d\n",cmd);
             break;
       }
    }
}

startDwnLnk(int taskPriority,int taskOptions,int stackSize)
{
   int eatAcodes();
   NDDS_ID createCodeDownldSubscription(NDDS_ID nddsId);

   /* create the Transfer In Process Mutual Exclusion semaphore */
   pTransferInProgressMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);
    /* Since we need to be able to reset this semaphore from another task then
     *  it can't be a Mutex since a Mutex can only be free by the task that took it.
     */
    /* pTransferInProgressMutex = semBCreate(SEM_Q_FIFO,SEM_FULL); */

   DPRINT1(1,"startDwnLnk: NDDS_Domain: 0x%lx\n",NDDS_Domain);
   pDwnldSub = createCodeDownldSubscription(NDDS_Domain);

    /* DownLkrPrior = taskPriority; */

    if (taskNameToId("tBufReq") == ERROR)
     taskSpawn("tBufReq",taskPriority,taskOptions,stackSize,bufferRequest,
	        NULL,nClBufId,nClBufId,4,5,6,7,8,9,10); 


    /* if (taskNameToId("tEatCodes") == ERROR)
     * taskSpawn("tEatCodes",taskPriority+5,taskOptions,stackSize,eatAcodes,
 	*        NULL,nClBufId,nClBufId,4,5,6,7,8,9,10); 
	*/
}

prtDwnldSubStatus()
{
#ifndef RTI_NDDS_4x
   NDDSRecvInfo info;
   if (NddsSubscriptionStatusGet(pDwnldSub->subscription, &info) == RTI_TRUE)
      printNDDSInfo(&info);
#endif  /* RTI_NDDS_4x */
}

prtDwnldPubStatus()
{
#ifndef RTI_NDDS_4x
  int unackIssues;
  NDDSPublicationReliableStatus status;
  if (pDwnldPubReply != NULL)
   unackIssues = NddsPublicationReliableStatusGet(pDwnldPubReply->publication, &status);
   printReliablePubStatus(&status);
#endif  /* RTI_NDDS_4x */
}

killDwnLnkTasks()
{
   int tid;
   /* markBusy(DOWNLINKER_FLAGBIT); */
   if ((tid = taskNameToId("tBufReq")) != ERROR)
      taskDelete(tid);
/*
   nddsPublicationDestroy(pDwnldPubReply);
   nddsSubscriptionDestroy(pDwnldSub);
*/
}
/*
resumeDownLink()
{
   int tid;
   if ((tid = taskNameToId("tDownLink")) != ERROR)
      taskResume(tid);
}
*/

#ifdef XXXXX
createDomain()
{
   char nicIP[40];
   printf(")))))))) downLink  createDomain\n");
   DPRINT(-1,"downLink  createDomain\n");
   /* create Domain Zero, no debug output, no multicasting */
   NDDS_Domain =  nddsCreate(0,1,1, (char *) getLocalIP( nicIP ) );
}
#endif

#ifdef XXXX
#define DYN_BUF_NUM 100
#define FIX_BUF_NUM 100
#define DYN_BUF_SIZ 1024
#define FIX_BUF_SIZ 2048
#define CLB_WV_EVENT_ID 2000
initsys()
{
   int result;
   gethostname(hostName,30);
   printf("hostname: '%s'\n",hostName);

   createDomain();

   mbufID = mBufferCreate((MBUF_DESC *) &mbClTbl, NUM_CLUSTERS, CLB_WV_EVENT_ID);
   nClBufId = nClbCreate(mbufID,20);
   
   createCodeDownldSubscription(NDDS_Domain);
   createCodeDownldPublication(NDDS_Domain);
   /* createCodeStatPub(NDDS_Domain); */
   nddsPri(52);
   taskDelay(calcSysClkTicks(166));  /* was 10 ticks, this is equivilent and sys clock independent  */
   startDwnLnk(65,0,8192);
}
#endif

#ifdef RTI_NDDS_4x
int Register_Codes_Downldkeyed_instance(NDDS_ID pNDDS_Obj,int key)
{
    Codes_Downld *pIssue = NULL;
    Codes_DownldDataWriter *Codes_DownldWriter = NULL;

    Codes_DownldWriter = Codes_DownldDataWriter_narrow(pNDDS_Obj->pDWriter);
    if (Codes_DownldWriter == NULL) {
        errLogRet(LOGIT,debugInfo, "Register_Codes_Downldkeyed_instance: DataReader narrow error.\n");
        return -1;
    }

    pIssue = pNDDS_Obj->instance;

    pIssue->key = key;

    // for Keyed Topics must register this keyed topic
    Codes_DownldDataWriter_register_instance(Codes_DownldWriter, pIssue);

    return 0;
}
#endif  /* RTI_NDDS_4x */
/* NDDS */
NDDS_ID createCodeDownldSubscription(NDDS_ID nddsId)
{

   NDDS_ID  pSubObj;
   char subtopic[128];

   /* FBUFFER_ID fBufferCreate(int nBuffers, int bufSize, char* memAddr,int eventid) */
    /* create a buffer pool of fixed size buffer for the NDDS callback routine to
       put the Acodes */

    /* create a MsgQ to place the waiting buffer beginning address to be read */
     pBufReqQ = msgQCreate(1024,4 /* sizeof(int) */,MSG_Q_FIFO);
  
     numBufSemId = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

     DwnldCmpltSemId = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

     pSemDwnldCntChg = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {  
        return(NULL);
    }  

    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,nddsId,sizeof(NDDS_OBJ));

   /* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
   /* sprintf(subtopic,"h/%s/dwnld/strm",hostName); */
   sprintf(subtopic,CNTLR_SUB_TOPIC_FORMAT_STR,hostName);
 

    /* strcpy(pSubObj->topicName,"wormhole_CodesDownld"); */
    strcpy(pSubObj->topicName,subtopic);

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCodes_DownldInfo(pSubObj);

#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Codes_DownldCallback; 
    pSubObj->callBkRtnParam = NULL;
#endif  /* RTI_NDDS_4x */

    pSubObj->queueSize = CTLR_CODES_DOWNLD_SUB_QSIZE;
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifdef RTI_NDDS_4x
    initSubscription(pSubObj); 
    attachOnDataAvailableCallback(pSubObj, Codes_DownldCallback,NULL);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj); 
    return ( pSubObj );
}

/* NDDS */
NDDS_ID createCodeDownldPublication(NDDS_ID nddsId)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     Codes_Downld *pIssue;

    /* BUild Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    /* sprintf(pubtopic,"%s/h/dwnld/reply",hostName); */
#ifndef RTI_NDDS_4x
    sprintf(pubtopic,CNTLR_PUB_TOPIC_FORMAT_STR,hostName);
    strcpy(pPubObj->topicName,pubtopic);
#else /* RTI_NDDS_4x */
    strcpy(pPubObj->topicName,CNTLR_CODES_DOWNLD_PUB_M21_STR);
#endif  /* RTI_NDDS_4x */

    pPubObj->pubThreadId = 51;  /* DEFAULT_PUB_THREADID; taskIdSelf(); */
    pPubObj->queueSize = CTLR_CODES_DOWNLD_PUB_QSIZE;
    pPubObj->highWaterMark = CTLR_CODES_DOWNLD_PUB_HIWATER;
    pPubObj->lowWaterMark = CTLR_CODES_DOWNLD_PUB_LOWWATER;
    pPubObj->AckRequestsPerSendQueue = CTLR_CODES_DOWNLD_PUB_ACKSPERQ;

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCodes_DownldInfo(pPubObj);
#ifdef RTI_NDDS_4x
    initPublication(pPubObj);
    /* give the node name for Sendproc to create proper publication back */
    attachDWDiscvryUserData(pPubObj, hostName, strlen(hostName)+1);
#endif  /* RTI_NDDS_4x */
    createPublication(pPubObj);
    return(pPubObj);
}


/*
* nddsPri(int level)
* {
*    int tid;
* 
*   if ((tid = taskNameToId("n_rtu7400")) != ERROR)
*   {
*      taskPrioritySet(tid,level);
*   }
* 
* }
*/

bufShow(int level)
{
   dumpBufs(level);
}
dumpBufs(int level)
{
    nClbShow(nClBufId,level);
}

delBufsByName(char *label)
{
   int status;
   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_BYNAME,NULL,NULL);
   #endif
   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_TAKE,NULL,NULL);
   #endif
   semTake(pTransferInProgressMutex,WAIT_FOREVER);

     // status == 0 if succeeded in deleting
     status = nClbFreeByName(nClBufId, label);
    
     // though this may not be 100% accurate, 99.9% deleted file are Acodes
     DPRINT2(-1,"delBufsByName: nDownldCodes, prev: %d, now: %d\n",nDownldCodes,nDownldCodes-1);
     if ( status == 0 )
     {
        nDownldCodes = nDownldCodes - 1;
        if ( nDownldCodes < 0 ) 
            nDownldCodes = 0;
     }

     #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
     #endif
   semGive(pTransferInProgressMutex);

   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_SEMGIVE,NULL,NULL);
   #endif
   semGive(numBufSemId);
   return(status);
}

delBufsByRootName(char *label)
{
   int num;

   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_BYROOTNAME,NULL,NULL);
   #endif
   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_TAKE,NULL,NULL);
   #endif

   semTake(pTransferInProgressMutex,WAIT_FOREVER);
     num = nClbFreeByRootName(nClBufId,label);
     // DPRINT2(-3,"delBufsByName: nDownldCodes, prev: %d, now: %d\n",nDownldCodes,nDownldCodes-num);   testing
     // nDownldCodes = nDownldCodes - num;   testing

     // routine for error, aborts, end of experiment/setup thus this is a good place to zero 
     // these values.  a fix to avoid the case where C_DWNLD_START happen after the AParser test waiting for
     // Acodes, and determine if Acodes can reside in memeory    GMB 10/22/2010
     nDownldCodes = nDownldTables = nDownldPatterns = 0;
    
     
     #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
     #endif
   semGive(pTransferInProgressMutex);

   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_SEMGIVE,NULL,NULL);
   #endif
   semGive(numBufSemId);
   DPRINT1(-1,"delBufsByRootName(): number deleted: %d\n",num);
   if (num > 0)
      semGive(numBufSemId);
}

delAllBufs()
{
   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_ALL,NULL,NULL);
   #endif
   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_XFER_MUTEX_TAKE,NULL,NULL);
   #endif
  semTake(pTransferInProgressMutex,WAIT_FOREVER);

     nClbFreeAll(nClBufId);

  #ifdef INSTRUMENT
      wvEvent(EVENT_DOWNLINK_XFER_MUTEX_GIVE,NULL,NULL);
  #endif
  semGive(pTransferInProgressMutex);

   #ifdef INSTRUMENT
        wvEvent(EVENT_DOWNLINK_DELETE_SEMGIVE,NULL,NULL);
   #endif
  semGive(numBufSemId);
}

delem()
{
   nddsPublicationDestroy(pDwnldPubReply);
   nddsSubscriptionDestroy(pDwnldSub);
}

eatAcodes()
{
     NCLBP entry;
     int i = 0;
     
     FOREVER
     {   
       entry = (NCLBP) -1;
       while(entry != NULL)
       {
         i = 0;
         entry = (NCLBP) hashGetNxtEntry(nClBufId->pNamTbl,&i);
         DPRINT2(-1,"eatAcodes: entry: 0x%lx, i=%d\n",entry,i);
         if (entry != NULL)
         {
          DPRINT1(-1,"eatCodes: delete: '%s'\n",entry->id);
           nClbFree(entry);
           semGive(numBufSemId);
           taskDelay(calcSysClkTicks(166));  /* was 10 ticks, this is equivilent and sys clock independent  */
         }
       }
       /* taskDelay(60*60); */
       taskDelay(calcSysClkTicks(60000));  /* 60 second */
     }
}

dwnldShow(int level)
{
    /* mBufferShow(mbufID, 1); already done in below function */
    nClbShow(nClBufId,level);
}
