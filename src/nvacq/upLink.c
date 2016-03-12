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

  This Task  handles the FID Data UpLoad to the Host computer.
  It Waits on a msgQ which is written into via STM interrupt routine
and other functions. Then writes that FID Statblock & Data up to the
host via channel interface.

*/

/* #define NOT_THREADED */

#define THREADED_RECVPROC

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>
#include <wdLib.h>

#include "nvhardware.h"
#include "logMsgLib.h"
#include "fBufferLib.h"
#include "dataObj.h"
#include "dmaDrv.h"
#include "crc32.h"
#include "upLink.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "taskPriority.h"
#include "ddr.h"

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "Data_Upload.h"

#ifdef RTI_NDDS_4x
#include "Data_UploadPlugin.h"
#include "Data_UploadSupport.h"
#endif  /* RTI_NDDS_4x */

/* NDDS Primary Domain */
extern NDDS_ID NDDS_Domain;

extern char hostName[80];

/* 
 * fid_count & fid_ct are set to Zero when Recvproc subscription for data appears or -1 when it  dispears 
 * this has the effect that the master will set the status from inactive to idle or visa virsa 
 */
extern int fid_count, fid_ct;  

static int  dwnLnkrIsCon = 0;
static char *ErrStr = { "ERR" };
static char *OkStr = { "OK" };
static char RespondStr[20];

static char     msg_norm[ACQ_UPLINK_XFR_MSG_SIZE+4];
static char     msg_term[ACQ_UPLINK_XFR_MSG_SIZE+4];
static char     msg_acqi[ACQ_UPLINK_XFR_MSG_SIZE+4];
static char     msg_noda[ACQ_UPLINK_XFR_MSG_SIZE+4];
static char     msg_start[ACQ_UPLINK_XFR_MSG_SIZE+4];
 
#define NUM_DMA_BUFFERS 20
/* #define DMA_BUFFER_SIZE 64*1024*4 */
#define DMA_BUFFER_SIZE 256000  /* 64000 * 4, even number of NDDS issues */
#define DMA_FID_EVENT_ID  3000

/* TCB pointer for Nettask and Parser, used for priority changes */
 
static int pParserId,pNetTaskId;
 
static NDDS_ID pUploadPub = NULL;
static NDDS_ID pUploadSubRdy = NULL;

static int DataDmaChan = -1;

extern MSG_Q_ID pDspDataRdyMsgQ;    /* DSP -> dspDataXfer task */
extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */

extern MSG_Q_ID pDataTagMsgQ;	    /* dspDataXfer task  -> dataPublisher task */

static MSG_Q_ID pDataAddrMsgQ;      /* DMA drvier -> dataPublisher task */

static char *pCacheInvalidateBufferZone1;   
static FBUFFER_ID pDmaFreeList;     /* dspDataXfer task  -> DMA driver */
static char *pCacheInvalidateBufferZone2; 

static char* pubIssueDataPtr;	    /* initial buffer allocated during publication creation */

static WDOG_ID pXferInactiveWDog;
static WDOG_ID pXferStuckWDog;

static int  RecvprocReady = 0;
int  ddrActiveFlag = -1;

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */

int ddrUsePioFlag = 0;

int adcOvrFlowSent = -1;   /* set to elemId or FID ADC Overflow was sent for */

/* testing uplink thoughput with special receiver */
FID_STAT_BLOCK TstStatBlk;
int upLkTestFlag = 0;

/* WDog timeout flags, used in wait4Recvproc, to determine if upLink is still
 * in the process of sending data or is stuck within a publish call
 *
 */
static int InActiveTimeout = 0;
static int XferStuckTimeout = 0;

/*
 * Clear the Transfer Stuck within publish call flag 
 */
clrXferStuckTimeout()
{
    XferStuckTimeout = 0;
}

/*
 * Clear the Transfer Inactive flag 
 */
clrInActiveTimeout()
{
    InActiveTimeout = 0;
}

/* ISR for either InActiveTimeout or XferStuckTimeout  watch dog */
pXferWdISR(int *pWDogFlag)
{
    *pWDogFlag = 1;   /* i.e. InActiveTimeout or XferStuckTimeout */
    /* logMsg("WDOG pXferWdISR invoked\n"); 
    *if (pWDogFlag == &InActiveTimeout)
    *    logMsg("WDOG: InActiveTimeout\n");
    *else
    *    logMsg("WDOG: XferStuckTimeout\n");
    */
}

/*
 * this is how this works.
 * 1. dspDataXfer (task) waits on a msgQ, in which the DSP sends the Tag & Address of FID
 *       data ready to be transfered to host
 *    Sends the Tag via a MsgQ to the dataPublisher (task)
 *    Then it initiates the appropriate number of DMA transfer from the DSP to 405 memory buffers
 *    Then returns to wait for more messages from the DSP
 *
 * 2. dataPublisher initialial wait on the 'Tag' msgQ inwhich the dataDmaXferScheduler places the Tag
 *      of data read to transfer.
 *    It obtains the Statblock of this data.
 *    Publishes to the Host the standard trasnfer message, then the statblock
 *    Then it waits on the msgQ inwhich the DMA completed messages come, in the form
 *    of Address of the buffers filled.
 *    Then it publishes this data to the host
 *    Returns this buffer Address on to the buffer free list 
 *    From the statblock the routine knows when all the data has been DMA'd and published to the Host
 */
dataPublisher(MSG_Q_ID dataBufFreeList)
{
   PUBLSH_MSG pubMsg;
   Data_Upload *issue;
   NDDS_ID createDataUploadPub(NDDS_ID nddsId);
   int cntdown;

    /* watch dogs timers go off after set time of inactivity of sending data or stuck with NDDS publish */
    pXferInactiveWDog = wdCreate();
    pXferStuckWDog = wdCreate();

   /* Future use if needed */
   /* initBarrier(10); */
   /* initialDDRSyncComm(); */

   /* build construct and initial set FID transfer limit of 10 */
   /* This allows Recvproc two control the flow of data */
   initTransferLimit(10);

   DPRINT(-1,"dataPublisher :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     /* it would be normal for this wdog to expire here, this flag is only test at the end of an experiment */
     /* start inactive xfer wdog timeout of 5 seconds */ 
     wdStart(pXferInactiveWDog,calcSysClkTicks(1000), pXferWdISR,(int)  &InActiveTimeout);

     /* wait for dspDataXfer to send msg that FID is being transfered */
     msgQReceive(pDataTagMsgQ,(char*) &pubMsg,sizeof(pubMsg), WAIT_FOREVER);

     DPRINT(-1,"dataPublisher() - Start ++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

     DPRINT2(-1,"dataPublisher: recv'd: dataTag: %d, FID CRC: 0x%lx\n",pubMsg.tag,pubMsg.crc32chksum);
     /* DPRINT2(-1,"dataPublisher: doneCode: %d, errorcode: %d\n",pubMsg.donecode,pubMsg.errorcode); */

     /* the problem has been seen was the abort comes in prior to the recvproc message that's it ready */
     /* as a result the error never get sent and recvproc hangs */
     /* this is an attempt to solve this problem by waiting several seconds for the message to appear */
     /* eventually timing out */
     cntdown = 20;
     while ( (!RecvprocReady) && (cntdown > 0) )
     {
          DPRINT1(-4,"dataPublisher:  Recvproc NOT Ready.... giving it a chance, cntdown: %d \n",cntdown);
	  cntdown--;
          taskDelay(calcSysClkTicks(166)); /* taskDelay(10); */
     }
     if (!RecvprocReady)
     {
       DPRINT(-4,"dataPublisher:  Recvproc NOT Ready....skip transfer\n");
       /* wvDLogStp(); to stop WIndview logging, no-acode Recvproc Ready msg lost */
       continue;
     }
     /* if tag is < 0 then this is a exception Message, e.g. ABORT, HALT, etc. for Recvproc */
     if (pubMsg.tag >= 0)
     {
       int waitResult;
       /* send only warning to Recvproc that are related to data, ADC or Receiver overflow, etc. */
       if (pubMsg.donecode != WARNING_MSG)
       {
          wdCancel(pXferInactiveWDog);  /* cancle inactivity wdog,  about to send data */
          clrInActiveTimeout();		/* clear inactivity flag so there is no misstake */
          setEmacTxThreshold(1536);	/* up the eac ethernet threshold, to avoid underruns of emac TX */

          sendDataUp(&pubMsg);

         /* start inactive xfer wdog timeout of 5 seconds */ 
         wdStart(pXferInactiveWDog,calcSysClkTicks(5000), pXferWdISR,(int)  &InActiveTimeout);

         /* setting emac threshold back down is done in recvproc callback,  setEmacTxThreshold(64); */
          
#ifdef XXXX
          /* in keeping Fid & CT updated but I think take to much time, getting data up is more important */
          /* wvEvent(1,NULL,NULL);
           * publishFidCt();        from 1/2 to several msec to transmit this
           * wvEvent(2,NULL,NULL);  cause TX underruns here, would need to be place prior to threshold change
           */
#endif

          /* if uplink has reach a transfer limit then this call will pend. */
          if (upLkTestFlag != 1)  /* if not in test mode, do it */
          {
             /* start xfer stuck wdog timeout of 8 seconds */ 
             wdStart(pXferStuckWDog,calcSysClkTicks(8000), pXferWdISR, (int) &XferStuckTimeout);

             waitResult = transferLimitWait();

             wdCancel(pXferStuckWDog);
             clrXferStuckTimeout();

             if (waitResult == -1) /* aborted */
                continue;
          }

          /* let's test the barrier wait concept */
          /* barrierWait(); */
       }
       else
       {
          sendWarningUp(&pubMsg);
       }
     }
     else
     {
       if (pubMsg.tag == -42)
       {
          adcOvrFlowSent = -1;  /* at start of experiment reset ADC flag back to an unused elemID value */
          /* use to send msg to recvproc but this technique was dropped */
       }
       else
       {
          sendExceptionUp(&pubMsg);
          cleanUpLinkMsgQs();
        }
     }
     DPRINT(-1,"dataPublisher() - Complete =========================================================\n");
  }

}

sendDataUp(PUBLSH_MSG *pubMsg)
{
   FID_STAT_BLOCK *pStatBlk;
   long dataTag, dataAddr, dmaBufPtr;
   long bytesleft,bytes,i,xfrsize;
   long dmaBufOffset,dmaBytesleft;
   int status;
   unsigned long fidCrc;
   tcrc calcChksum,dspChksum;

#ifndef RTI_NDDS_4x
   NDDSPublicationReliableStatus nddsStatus;  /* diagnostic use */
#endif  /* RTI_NDDS_4x */

   Data_Upload *issue = pUploadPub->instance;
   strcpy(msg_norm,ACQ_UPLINK_MSG);

   dataTag = pubMsg->tag;
   pStatBlk =  dataGetStatBlk(pTheDataObject, pubMsg->tag);
#ifdef XXX
   if (upLkTestFlag == 0)
    pStatBlk =  dataGetStatBlk(pTheDataObject, pubMsg->tag);
   else
    pStatBlk = &TstStatBlk;
#endif

   if (pStatBlk == NULL)
   {
      errLogRet(LOGIT,debugInfo, "!!!!!!!!!!!!!!!!! E R R O R  Data Transfer StatBlock NULL !!!!!!!!!!!!!\n");
      taskSuspend(taskIdSelf());
   }

/*
   pStatBlk->doneCode = pubMsg->donecode;
   pStatBlk->errorCode = pubMsg->errorcode;
*/
 
   DPRINT3(-1,"dataPublisher: elemID: %lu, ct: %lu, Addr: 0x%lx\n",pStatBlk->elemId, pStatBlk->ct,pStatBlk->fidAddr);
   /* DPRINT3(-1,"dataPublisher: elemID: %lu, nt: %lu, ct: %lu\n",pStatBlk->elemId,  pStatBlk->nt, pStatBlk->ct); */

       /* Send the FID Stat_Block */

       /* start xfer stck wdog timeout of 5 seconds */ 
       wdStart(pXferStuckWDog,calcSysClkTicks(1000), pXferWdISR, (int) &XferStuckTimeout);

       bytes = sendStatBlk2Recvproc((char*)pStatBlk, sizeof(FID_STAT_BLOCK)); /* NDDS */

       wdCancel(pXferStuckWDog);
       clrXferStuckTimeout();

       bytesleft =  issue->totalBytes = pStatBlk->dataSize;
       issue->type = DATAUPLOAD_FID;
       issue->dataOffset = 0;
       issue->sn = 0; 
       issue->crc32chksum = pubMsg->crc32chksum;  /* send up given checksum */

       i = calcChksum = 0;
       /* Now wait for the DMA xfer from dsp to 405 completion, 
        * this may require more than one dma xfer 
        */
       panelLedOn(DATA_XFER_LED);

       // If a Send zero FID, then set bytesleft to zero, so as to not send any data at all
       if (pStatBlk->doneCode == EXP_FIDZERO_CMPLT)
           bytesleft = 0;
       while (bytesleft > 0)
       {
         /* obtain the dstAddr of the filled buffer from the DMA engine */
         msgQReceive(pDataAddrMsgQ, (char*) &dataAddr, 4, WAIT_FOREVER);
         DPRINT3(-1,"dataPublisher() - Recv'd DMA Buffer Addr: 0x%lx, bytesleft: %ld, offset: 0x%lx\n",
		       dataAddr,bytesleft,issue->dataOffset);

         dmaBufPtr = dataAddr;
         dmaBufOffset = 0;
         dmaBytesleft = (bytesleft < DMA_BUFFER_SIZE)  ? bytesleft : DMA_BUFFER_SIZE;
         pubMsg->crc32chksum = 0L;
         if(pubMsg->crc32chksum==0)
            calcChksum=pubMsg->crc32chksum;
         else if (issue->sn == 0)
            calcChksum = addbfcrcinc((char*)dmaBufPtr,dmaBytesleft,(tcrc*) 0);
         else
            calcChksum = addbfcrcinc((char*)dmaBufPtr,dmaBytesleft, &calcChksum);
         /* DPRINT1(-1,"calcChksum so far: 0x%lx\n",calcChksum); */

         while(dmaBytesleft > 0)
         {

            issue->sn = i++; 
            xfrsize = (dmaBytesleft < MAX_FIXCODE_SIZE)  ? dmaBytesleft : MAX_FIXCODE_SIZE;
#ifndef RTI_NDDS_4x
            issue->data.len = xfrsize;
            issue->data.val = (char*) dmaBufPtr;
#endif  /* RTI_NDDS_4x */

            DPRINT6(+1,"dataPublisher() - sn: %2d, Addr: 0x%08lx, offset: 0x%08lx, DMA bytesleft: %6ld, bytesleft: %7ld, xfrsize: %6ld\n",
		 issue->sn, dmaBufPtr, issue->dataOffset, dmaBytesleft, bytesleft, xfrsize);
     
            if (RecvprocReady)
            {
#ifdef RTI_NDDS_4x
                DDS_OctetSeq_set_maximum(&(issue->data), 0);
                DDS_OctetSeq_loan_contiguous(&(issue->data),
                                   (char*) dmaBufPtr,      // pointer
                                   0,                 // length
                                   DMA_BUFFER_SIZE); // maximum
                DDS_OctetSeq_set_length(&(issue->data), xfrsize);
#endif  /* RTI_NDDS_4x */
                /* start xfer stuck wdog timeout of 5 seconds */ 
                wdStart(pXferStuckWDog,calcSysClkTicks(5000), pXferWdISR,(int)  &XferStuckTimeout);

#ifndef RTI_NDDS_4x
                status = nddsPublishData(pUploadPub);
#else  /* RTI_NDDS_4x */
                status = publishFidData(pUploadPub);
#endif  /* RTI_NDDS_4x */

                wdCancel(pXferStuckWDog);
                clrXferStuckTimeout();

                if (status == -1)
                {
                      errLogRet(LOGIT,debugInfo,
	                  "!!!!!!!!!!!!!!!!! E R R O R  in Publishing Data  !!!!!!!!!!!!!!!!!!!!!!!!! \n");
                }
#ifdef RTI_NDDS_4x
                DDS_OctetSeq_unloan(&(issue->data));
#endif  /* RTI_NDDS_4x */
                 /* diagnostic usage */
/*
 *                NddsPublicationReliableStatusGet(pUploadPub->publication, &nddsStatus);
 *               if (nddsStatus.unacknowledgedIssues > 50)
 *               {
 *                 DPRINT1(-3,"dataPublisher: publish returned,  unacknowledge issues = %d\n", nddsStatus.unacknowledgedIssues);
 *               }
*/
            }
            else
            {
               DPRINT(-1,"dataPublisher() - Recvproc not Ready, skip data transfer\n");
            }

            issue->dataOffset += xfrsize;   /* increment total bytes transfered */
            dmaBufPtr += xfrsize;	    /* increment pointer within DMA buffer */

            bytesleft -= xfrsize; /* decrement bytes left to xfer */
            dmaBytesleft -= xfrsize; /* decrement bytes left in DMA buffer */
            /* DPRINT2(-1,"dataPublisher() - DMA bytesleft: %ld, bytesleft: %ld\n",
		dmaBytesleft,bytesleft); */
         }

         /* return dma buffer back to it's free list */
         fBufferReturn(pDmaFreeList,dataAddr);    
       }
       panelLedOff(DATA_XFER_LED);

   if (upLkTestFlag == 0)
   {      
       /* Message the DSP to free it's buffer as well */
       ddr_unlock_acm(pubMsg->dspIndex, 0 /* mode */ );
   }

       /* return data block to free list */
       dataFreeFidBlk(pTheDataObject,dataTag);

   if (upLkTestFlag == 0)
   {      
       DPRINT2(-1,"dataPublisher() - given CRC; 0x%lx, Calc CRC: 0x%lx\n",
              pubMsg->crc32chksum,calcChksum);
       if ( pubMsg->crc32chksum != calcChksum)
       {
           errLogRet(LOGIT,debugInfo,
	      "!!!!!!!!!!!!!!!!! E R R O R  Data Transfer DSP ---> 405: CRC32 mismatch  given: 0x%lx , calculated: 0x%lx !!!!!!!!!!!!!\n",pubMsg->crc32chksum, calcChksum);
           sendException(HARD_ERROR, SYSTEMERROR + IDATA_CRC_ERR, 0,0,NULL);
       }
   }      
}

/*
 * routine to transfer to recvproc the exception message 
 * 1st the ENDDAT message
 * 2nd the FidStatBlock with the donecode & errorcode set appropriately
 *
*/
sendWarningUp(PUBLSH_MSG *pubMsg)
{
   FID_STAT_BLOCK WrnStatBlk;
   FID_STAT_BLOCK *pStatBlk;
   int bytes;
   int tagid = 0;

   if ( (pubMsg->tag < 0 ) || (pubMsg->tag >=  pTheDataObject->maxFreeList))
   {
       errLogSysRet(LOGIT,debugInfo, "sendWarningUp: had to use a default tag of 0, tag %d was out of range 0-%d\n",
	    pubMsg->tag,pTheDataObject->maxFreeList);
       tagid = 0;
   }
   else 
        tagid = pubMsg->tag;

   
   pStatBlk =  dataGetStatBlk(pTheDataObject, tagid);
   DPRINT4(-1,"dataPublisher: Warning for tagId: %d, elemID: %lu, ct: %lu, Addr: 0x%lx\n",
		tagid,pStatBlk->elemId, pStatBlk->ct,pStatBlk->fidAddr);

   /* copy the statblk so as not to alter the original */
   memcpy(&WrnStatBlk,pStatBlk,sizeof(FID_STAT_BLOCK));

   
   WrnStatBlk.doneCode = pubMsg->donecode;
   WrnStatBlk.errorCode = pubMsg->errorcode;

   /* Send the Warning FID Stat_Block */
   bytes = sendStatBlk2Recvproc((char*) &WrnStatBlk, sizeof(FID_STAT_BLOCK)); /* NDDS */

   return(bytes);
}

/* 
 * allows send_warning in DDR_ACq.c to decide if it should send the exception that an ADC overflow
 * occurred.  This is done since only one ADC warning is wanted per FID
 *
 *  Author Greg Brissey   2/15/05
 */
checkAdcWarning(int tag)
{
   int tagid;
   FID_STAT_BLOCK *pStatBlk;

   if ( (tag < 0 ) || (tag >=  pTheDataObject->maxFreeList))
   {
       errLogSysRet(LOGIT,debugInfo, "sendWarningUp: had to use a default tag of 0, tag %d was out of range 0-%d\n",
	    tag,pTheDataObject->maxFreeList);
       tagid = 0;
   }
   else 
        tagid = tag;

   pStatBlk =  dataGetStatBlk(pTheDataObject, tagid);
   DPRINT4(-1,"dataPublisher: Warning for tagId: %d, elemID: %lu, ct: %lu, Addr: 0x%lx\n",
		tagid,pStatBlk->elemId, pStatBlk->ct,pStatBlk->fidAddr);

   DPRINT2(-1,"checkAdcWarning: adcOvrFlowSent: %d, elemId: %d\n",adcOvrFlowSent,pStatBlk->elemId);
   if  (adcOvrFlowSent == pStatBlk->elemId)
   {
       return(0);
   }
   else
   {
       adcOvrFlowSent = pStatBlk->elemId;
       return(1);
   }
}


/*
 * routine to transfer to recvproc the exception message 
 * 1st the ENDDAT message
 * 2nd the FidStatBlock with the donecode & errorcode set appropriately
 *
*/
sendExceptionUp(PUBLSH_MSG *pubMsg)
{
   FID_STAT_BLOCK ErrStatBlk;
   int bytes;

    DPRINT2(-1,"sendExceptionUp: doneCode: %d, errorcode: %d\n",pubMsg->donecode,pubMsg->errorcode);
   /* testAndSendStartUp(); */

   strcpy(msg_term,ACQ_UPLINK_ENDDATA_MSG);
   ErrStatBlk.elemId = -1;
   ErrStatBlk.ct = 0;
   ErrStatBlk.doneCode = pubMsg->donecode;
   ErrStatBlk.errorCode = pubMsg->errorcode;

   /* Send the FID Stat_Block */
   bytes = sendStatBlk2Recvproc((char*)&ErrStatBlk, sizeof(FID_STAT_BLOCK)); /* NDDS */

   return(bytes);
}

/* ================================================================================== */
/*    Data DMA from HPI COntrol  						*/
/*
*/
dataHpiDmaOn()
{
    set_register(DDR,DMARequest,1);
}
dataHpiDmaOff()
{
    set_register(DDR,DMARequest,0);
}

/* ================================================================================== */
/*
    receives message from the DSP when a fid block is ready for trasnsfer
*/
dspDataXfer(MSG_Q_ID pMsgsFromDsp)
{
   DSP_MSG dspMsg;
   PUBLSH_MSG pubMsg;
   FID_STAT_BLOCK *pStatBlk;
   long xfersize, bytesleft, offset;
   long srcAddr;
   char *dstAddr;
   unsigned long crc32chksum;
   int status;
   int donecode,errorcode;
   register long *srcPtr, *dstPtr, *endAddr;

   DPRINT(-1,"dspDataXfer: Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     /* wait for C67X DSP to send msg that data is ready */
     msgQReceive(pDspDataRdyMsgQ,(char*) &dspMsg,sizeof(DSP_MSG), WAIT_FOREVER);
 
     DPRINT(-1,"dspDataXfer() - Start --------------------------------------------------------\n");
     DPRINT3(-1,"dspDataXfer() - Tag: %d, DSP Data Addr: 0x%lx, CRC32: 0x%lx\n",dspMsg.tag,dspMsg.dataAddr,dspMsg.crc32chksum);
     DPRINT2(1,"dspDataXfer() - Data Cmplx NP: %d, srcIndex : %ld\n",dspMsg.np,dspMsg.srcIndex);

     /* Obtain pointer to the FID Stat Block */
     pStatBlk =  dataGetStatBlk(pTheDataObject, dspMsg.tag);
#ifdef XXX
   if (upLkTestFlag == 0)
     pStatBlk =  dataGetStatBlk(pTheDataObject, dspMsg.tag);
   else
     pStatBlk = &TstStatBlk;
#endif


     DPRINT3(+1,"dspDataXfer() - statBlk: startct: %lu, ct: %lu, nt: %lu\n",
       pStatBlk->startct,pStatBlk->ct,pStatBlk->nt);

     donecode = errorcode = 0;
     if (pStatBlk->ct == pStatBlk->nt)
     {
        donecode = EXP_FID_CMPLT;
     }
     else
     {
        donecode = BS_CMPLT;
     }
     pStatBlk->doneCode = donecode;
     pStatBlk->errorCode = errorcode;
     pStatBlk->fidAddr = (long *) dspMsg.dataAddr;
     DPRINT2(+1,"dspDataXfer() - statBlk Addr: 0x%lx, Data Addr: 0x%lx\n",pStatBlk, pStatBlk->fidAddr);

     /* The publisher sofar needs to know the Tag, dsp index, and the CRC checksum of the FID */
     pubMsg.tag = dspMsg.tag;
     pubMsg.dspIndex = dspMsg.srcIndex;
     pubMsg.crc32chksum = dspMsg.crc32chksum;

     /* 1st msgQ of publisher, start of FID transfer */
     msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(pubMsg), WAIT_FOREVER, MSG_PRI_NORMAL);


     /* Now base on the FID size and the DMA buffer size, we may have to use more than one
        DMA transfer to get the FID from the DSP side to the 405 side.
     */
     srcAddr = dspMsg.dataAddr;
     bytesleft = pStatBlk->dataSize;
     /* bytesleft = dspMsg.np * 8; testing, maybe a check here */
 
     while (bytesleft > 0)
     {
         /* a DMAing buffer of it's free List */
         dstAddr = fBufferGet(pDmaFreeList);    /* may pend if exhausted buffers on free list */


         xfersize = (bytesleft < DMA_BUFFER_SIZE)  ? bytesleft : DMA_BUFFER_SIZE;

         /* just a dma crc test check */
         /* crc32chksum = addbfcrc(srcAddr,xfersize); */

         DPRINT5(-1,"dspDataXfer() - Src: 0x%8lx, Dst: 0x%08lx, bytesleft: %7ld, xfrsize: %7ld (0x%06lx)\n",
		 srcAddr, dstAddr, bytesleft, xfersize, xfersize);

        if (ddrUsePioFlag)
        {
           /* program IO transfer of data */
           srcPtr = (long*) srcAddr;
           dstPtr = (long*) dstAddr;
           endAddr = srcPtr + xfersize/4;
           DPRINT3(-1,"dspDataXfer(): PIO: src: 0x%lx, dst: 0x%lx, end Addr: 0x%lx\n",srcPtr,dstPtr,endAddr);
           while( srcPtr < endAddr)
              *dstPtr++ = *srcPtr++;
           msgQSend(pDataAddrMsgQ, (char *)&(dstAddr), 4, WAIT_FOREVER, MSG_PRI_NORMAL);
        }
        else
        {
#ifdef  TEST_WITH_JUST_COPY
         /* for testing just put address into msgQ here rather than using the DMA driver */
         memcpy((char*)dstAddr,(char*)srcAddr,xfersize);
         msgQSend(pDataAddrMsgQ,(char*) &dstAddr, sizeof(long), WAIT_FOREVER, MSG_PRI_NORMAL);
#else
         /* Initiate DMA transfer from DSP to data buffer on 405 */
	 status = dmaXfer(DataDmaChan, MEMORY_TO_MEMORY /* _PACED */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, xfersize/4, NULL, pDataAddrMsgQ);
#endif
        }
         /* DPRINT3(1,"dspDataXfer: sn: %d, pub xfersize: %d, crc32: 0x%lx\n",issue->sn, xfersize,issue->crc32chksum); */
         srcAddr += xfersize;
         bytesleft -= xfersize; /* decrement bytes left to xfer */
     }
     DPRINT(-1,"dspDataXfer() - Complete --------------------------------------------------------\n");

  }
}

uplinkMsgQShow(int level)
{
     printf("\n----   DSP -> dspDataXfer   MsgQ --------------------------\n");
     msgQShow(pDspDataRdyMsgQ,level);

     printf("\n\n----  dspDataXfer task  -> dataPublisher task -------------\n");
     msgQShow(pDataTagMsgQ,level);

     printf("\n\n----  DMA Driver task  -> dataPublisher task -------------\n");
     msgQShow(pDataAddrMsgQ,level);
}

int clearUpLinkMsgs()
{
   DSP_MSG dspMsg;
   PUBLSH_MSG pubMsg;
   long dataAddr;
    int bytes;

   while ( (bytes = msgQReceive(pDspDataRdyMsgQ, (char*) &dspMsg, sizeof( DSP_MSG ), NO_WAIT)) != ERROR )
   {
      DPRINT5(-1,"clearUpLinkMsgs()  Xfr:  srcIndex: %d, dataAddr: 0x%lx, np: %lu, tag: %lu, crc: 0x%lx\n",
			dspMsg.srcIndex, dspMsg.dataAddr, dspMsg.np, dspMsg.tag, dspMsg.crc32chksum);
   }

   while ( (bytes = msgQReceive(pDataTagMsgQ, (char*) &pubMsg, sizeof( pubMsg ), NO_WAIT)) != ERROR )
   {
      DPRINT5(-1,"clearUpLinkMsgs()  dataPublisher:  tag: %d, crc: 0x%lx, donecode: 0x%lx, errorcode: %lu, dspIndex: %lu\n",
			pubMsg.tag, pubMsg.crc32chksum, pubMsg.donecode, pubMsg.errorcode, pubMsg.dspIndex);
   }

  return 0;
}

int cleanUpLinkMsgQs()
{
   DSP_MSG dspMsg;
   PUBLSH_MSG pubMsg;
   long dataAddr;
    int bytes;

   DPRINT(-1,"---------------   >>>>   cleanUpLinkMsgQs() \n");
   while ( (bytes = msgQReceive(pDspDataRdyMsgQ, (char*) &dspMsg, sizeof( DSP_MSG ), NO_WAIT)) != ERROR )
   {
      DPRINT5(-1,"cleanUpLinkMsgQs()  DSP->Xfr:  srcIndex: %d, dataAddr: 0x%lx, np: %lu, tag: %lu, crc: 0x%lx\n",
			dspMsg.srcIndex, dspMsg.dataAddr, dspMsg.np, dspMsg.tag, dspMsg.crc32chksum);
   }
   while ( (bytes = msgQReceive(pDataTagMsgQ, (char*) &pubMsg, sizeof( pubMsg ), NO_WAIT)) != ERROR )
   {
      DPRINT5(-1,"cleanUpLinkMsgQs()  Xfr->dataPublisher:  tag: %d, donecode: 0x%lx, errorcode: %lu, dspIndex: %lu, crc: 0x%lx\n",
			pubMsg.tag, pubMsg.donecode, pubMsg.errorcode, pubMsg.dspIndex, pubMsg.crc32chksum);
   }
   while ( (bytes = msgQReceive(pDataAddrMsgQ, (char*) &dataAddr, 4 , NO_WAIT)) != ERROR )
   {
      DPRINT1(-1," DMA  filled buffers for dataPublisher: 0x%lx\n",dataAddr);
      fBufferReturn(pDmaFreeList,dataAddr);    
   }

   return 0;
}

#ifndef  THREADED_RECVPROC
int sendMsg2Recvproc(char *msge, int size)
{
#ifndef THREADED
    Data_Upload *issue;
    issue = (Data_Upload *) pUploadPub->instance;
    issue->sn = issue->elemId = issue->dataOffset = issue->crc32chksum = 0;
    strncpy(pubIssueDataPtr,msge,80);
    issue->crc32chksum = addbfcrc(pubIssueDataPtr,size);
    issue->totalBytes = issue->data.len = size;
    /* issue->data.val = msge; */
    issue->data.val = pubIssueDataPtr;
    DPRINT3(-1,"sendMsg2Recvproc() - msge: '%s', size: %d, crc: 0x%lx\n",
		issue->data.val,issue->totalBytes,issue->crc32chksum);
    nddsPublishData(pUploadPub);
#else
    DPRINT2(-1,"sendMsg2Recvproc() - msge: '%s', size: %d  NOT Sent in THREADED Recvproc.\n",
		msge,size);

#endif
    /* NddsPublicationSend(pUploadPub->publication); */
}
#endif

#ifndef RTI_NDDS_4x
int sendStatBlk2Recvproc(char *pStatBlk,int size)
{
    Data_Upload *issue;
    int status;
    issue = (Data_Upload *) pUploadPub->instance;
    issue->type = DATAUPLOAD_FIDSTATBLK;
    issue->sn = issue->elemId = issue->dataOffset = 0;
    issue->totalBytes = issue->data.len = size;
    memcpy(pubIssueDataPtr,pStatBlk,size);
    /* issue->data.val = pStatBlk; */
    issue->data.val = pubIssueDataPtr;
    issue->crc32chksum = addbfcrc(pStatBlk,size);
    DPRINT3(-1,"sendStatBlk2Recvproc() - pStatBlk: 0x%lx, size: %d, crc: 0x%lx\n",
	issue->data.val, issue->totalBytes, issue->crc32chksum);

    status = nddsPublishData(pUploadPub);
    if (status == -1)
    {
          errLogRet(LOGIT,debugInfo,
	      "!!!!!!!!!!!!!!!!! E R R O R  in Publishing StatBlock  !!!!!!!!!!!!!!!!!!!!!!!!! \n");
    }
}
#else  /* RTI_NDDS_4x */
publishFidData(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Data_UploadDataWriter *Data_Upload_writer = NULL;

   Data_Upload_writer = Data_UploadDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (Data_Upload_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"publishFidData: DataWriter narrow error\n");
        return -1;
   }

   result = Data_UploadDataWriter_write(Data_Upload_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"publishFidData: DataWriter write error: %d\n",result);
   }
   return 0;
}

int sendStatBlk2Recvproc(char *pStatBlk,int size)
{
    Data_Upload *issue;
    int status;
    DDS_ReturnCode_t result;
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    Data_UploadDataWriter *Data_Upload_writer = NULL;
    issue = (Data_Upload *) pUploadPub->instance;
    issue->type = DATAUPLOAD_FIDSTATBLK;
    issue->sn = issue->elemId = issue->dataOffset = 0;
    issue->totalBytes = size;


    // DDS_OctetSeq_set_length(&(issue->data),size);
    /* this auto set length of sequence */
    DDS_OctetSeq_from_array(&(issue->data),pStatBlk,size);
    issue->crc32chksum = addbfcrc(pStatBlk,size);
    DPRINT3(-1,"sendStatBlk2Recvproc() - pStatBlk: 0x%lx, size: %d, crc: 0x%lx\n",
	pStatBlk, issue->totalBytes, issue->crc32chksum);

    Data_Upload_writer = Data_UploadDataWriter_narrow(pUploadPub->pDWriter);
    if (Data_Upload_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"sendStatBlk2Recvproc: DataWriter narrow error\n");
        return -1;
    }

    result = Data_UploadDataWriter_write(Data_Upload_writer,
                issue,&instance_handle);
    if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"sendStatBlk2Recvproc: DataWriter write error: %d\n",result);
          errLogRet(LOGIT,debugInfo,
	      "!!!!!!!!!!!!!!!!! E R R O R  in Publishing StatBlock  !!!!!!!!!!!!!!!!!!!!!!!!! \n");
    }
    return 0;
}

int Register_Data_Uploadkeyed_instance(NDDS_ID pNDDS_Obj,int key)
{
    Data_Upload *pIssue = NULL;
    Data_UploadDataWriter *Data_UploadWriter = NULL;

    Data_UploadWriter = Data_UploadDataWriter_narrow(pNDDS_Obj->pDWriter);
    if (Data_UploadWriter == NULL) {
        errLogRet(LOGIT,debugInfo, "Register_Data_Uploadkeyed_instance: DataReader narrow error.\n");
        return -1;
    }

    pIssue = pNDDS_Obj->instance;

    pIssue->key = key;

    // for Keyed Topics must register this keyed topic
    Data_UploadDataWriter_register_instance(Data_UploadWriter, pIssue);

    return 0;
}
#endif  /* RTI_NDDS_4x */

int startDataPublisher(int priority, int taskoptions, int stacksize)
{
    int uplink();
    Data_Upload *issue;
#ifdef RTI_NDDS_4x
    long key;
#endif  /* RTI_NDDS_4x */

    NDDS_ID createDataUploadPub(NDDS_ID nddsId);
    NDDS_ID createDataUploadSub(NDDS_ID nddsId);

    /* get a DMA Channel that will be now dedicated to moving data from the DSP to 405 memory */
    if (DataDmaChan == -1)
    {
       DataDmaChan = dmaGetChannel (3); /* channel 3 is for HPI DMARequest Reg. controlled dma transfers */
       if (DataDmaChan == -1)
         DPRINT(-1,"dmaGetChannel() ---- FAILED ---\n");
    }
    dataHpiDmaOn();    /* turn the HPI device paced control to DMA ON */

    DPRINT1(-1,"startDataPublisher: dmachannel: %d\n",DataDmaChan);

    /* create uplink publication that recvproc subscribes to */
    if (pUploadPub == NULL)
    {
       pUploadPub = createDataUploadPub(NDDS_Domain);
#ifdef RTI_NDDS_4x
       key =  BrdNum;   /* key indicates which DDR is sending */
       Register_Data_Uploadkeyed_instance(pUploadPub,key);
#endif  /* RTI_NDDS_4x */
       issue = (Data_Upload *) pUploadPub->instance;
       /* save the data.val pointer for publication destuction, we are going to overwrite this pointer */
#ifndef RTI_NDDS_4x
       pubIssueDataPtr = issue->data.val;
#else  /* RTI_NDDS_4x */
       pubIssueDataPtr = NULL;
#endif  /* RTI_NDDS_4x */
    }

    if (pUploadSubRdy == NULL)
    {
       pUploadSubRdy = createDataUploadSub(NDDS_Domain);
    }

    /* 
     * Messags from Dsp giving Tag, FID Address, CRC32 Chksum of a FID ready for transfer from the
     * DSP memory to the 405 memory.
    */
    pDspDataRdyMsgQ = msgQCreate(2000,sizeof(DSP_MSG),MSG_Q_FIFO);
    DPRINT1(-1,"pDspDataRdyMsgQ: 0x%lx\n",pDspDataRdyMsgQ);
     if (pDspDataRdyMsgQ == NULL)
          DPRINT(-1,"pDspDataRdyMsgQ --- FAILED --- to be built.\n");

   /* Msges from DSP Transfer Task indicating FID data is being transferred */
   /*  dspDataXfer sends, dataPublisher receives */
   pDataTagMsgQ = msgQCreate(2000,sizeof(PUBLSH_MSG),MSG_Q_PRIORITY);
   DPRINT1(-1,"pDataTagMsgQ: 0x%lx\n",pDataTagMsgQ);
   if (pDataTagMsgQ == NULL)
          DPRINT(-1,"pDataTagMsgQ --- FAILED --- to be built.\n");

   /* create a list of DMA buffer to use to transfer the FID data across the DSP HPI interface */
   /* Since these buffer are used by DMA to transfer data into memory, then areas must be
      invalidated in the cache by the DMA routines. However there can be spill over to 
      the size of a cache line, and an entire cache line is invalidated at a time.
      Thus we malloc space around these buffers to protect surronding used memory  from
      being colaterial damage for invalidating the cache.  Why you ask? Because it has happed!
      The case being the a MsgQ was very close to a DMA buffer that was being invalidated, thus
      the message that was just placed in it (dmaDrv.c) was corrupted.
		Greg Brissey   10/21/04
   */
   pCacheInvalidateBufferZone1 = (char*) malloc(1024);
   pDmaFreeList = fBufferCreate(NUM_DMA_BUFFERS, DMA_BUFFER_SIZE, NULL, DMA_FID_EVENT_ID);
   pCacheInvalidateBufferZone2 = (char*) malloc(1024);
   DPRINT1(-1,"pDmaFreeList: 0x%lx\n",pDmaFreeList);
   if (pDmaFreeList == NULL)
          DPRINT(-1,"pDmaFreeList --- FAILED --- to be built.\n");

   /* msgQ the DMA will put transfers completed, make it as big as the number of buffers */
   pDataAddrMsgQ = msgQCreate(NUM_DMA_BUFFERS,4,MSG_Q_FIFO);
   DPRINT1(-1,"pDataAddrMsgQ: 0x%lx\n",pDataAddrMsgQ);
   if (pDataAddrMsgQ == NULL)
          DPRINT(-1,"pDataAddrMsgQ --- FAILED --- to be built.\n");

   ddrUsePioFlag = 0;   /* default to DMA use, rather than PIO */

   /* Start UpLinker */
   if (taskNameToId("tFidPubshr") == ERROR)
     taskSpawn("tFidPubshr",priority+5,taskoptions,8192,dataPublisher,pDataTagMsgQ,
                2,3,4,5,6,7,8,9,10);
   if (taskNameToId("tDspXfr") == ERROR)
     taskSpawn("tDspXfr",120 /* 92 , priority */,taskoptions,stacksize,dspDataXfer,pDspDataRdyMsgQ,
                2,3,4,5,6,7,8,9,10);

   return(0);
}

/*
 * get the Data DMA Channel
 *
 */
int getDataDmaChan()
{
  return( DataDmaChan );
}

/*================================================================================*/
#ifndef RTI_NDDS_4x

/*
     Reliable Publication Status call back routine.
     At present we use this to indicate if a subscriber has come or gone
*/
void UplinkPubStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam)
{
      /* RCVR_DESC_ID pRcvrDesc = (RCVR_DESC_ID) callBackRtnParam; */
      switch(status->event)
      {  
#ifdef NOT_NEEDED_YET
      case NDDS_QUEUE_EMPTY:
        DPRINT1(+1,"'%s': Queue empty\n",status->nddsTopic);
        break;
      case NDDS_LOW_WATER_MARK:
        DPRINT2(+1,"'%s': At Low Water Mark - UnAck Issues: %d\n",
           status->nddsTopic,status->unacknowledgedIssues);
        break;
      case NDDS_HIGH_WATER_MARK:
        DPRINT2(-1,"'%s': At High Water Mark - UnAck Issues: %d\n",
           status->nddsTopic,status->unacknowledgedIssues);
        break;
      case NDDS_QUEUE_FULL:
        DPRINT2(-1,"'%s': Queue Full - UnAck Issues: %d\n",
           status->nddsTopic,status->unacknowledgedIssues);
        break;
#endif /* NOT_NEEDED_YET */
      case NDDS_SUBSCRIPTION_NEW:
        errLogRet(LOGIT,debugInfo, "'%s': A new reliable subscription for Pub: '%s' has Appeared.\n",
                        hostName,status->nddsTopic);
         if ( pUploadPub == callBackRtnParam) 
         {
	   DPRINT(-1,"set idle\n");
           fid_count = fid_ct = 0;
           sendFidCtStatus();
         }
        break;

      case NDDS_SUBSCRIPTION_DELETE:
        errLogRet(LOGIT,debugInfo,"'%s': A reliable subscription for Pub: '%s' has Disappeared.\n",
                        hostName,status->nddsTopic);
         if ( pUploadPub == callBackRtnParam) 
         {
	   DPRINT(-5,"set inactive\n");
           fid_count = fid_ct = -1;
           sendFidCtStatus();
         }
        break;

      default:
                /* NDDS_BEFORERTN_VETOED
                   NDDS_RELIABLE_STATUS
                */
        break;
      }
}     
#else  /* RTI_NDDS_4x */
#ifdef XXXX
void Data_Upload_SubscriptionMatched(void* listener_data, DDS_DataReader* reader,
                        const struct DDS_SubscriptionMatchedStatus *status)
{
/*
    DDS_Long    total_count
        The total cumulative number of times the concerned DDS_DataReader
        discovered a "match" with a DDS_DataWriter.
    DDS_Long    total_count_change
        The change in total_count since the last time the listener was
        called or the status was read.
    DDS_Long    current_count
        The current number of writers with which the DDS_DataReader is matched.
    DDS_Long    current_count_change
        The change in current_count since the last time the listener was
        called or the status was read.
    DDS_InstanceHandle_t        last_publication_handle
        A handle to the last DDS_DataWriter that caused the status to change.
*/
   printf("Subscription Matched, Current Matched: %d, Delta: %d\n",
            status->current_count,status->current_count_change);
   // errLogRet(LOGIT,debugInfo, "'%s': A new reliable subscription for Pub: '%s' has Appeared.\n",
   //                      hostName,status->nddsTopic);
   DPRINT(-1,"set idle\n");
   fid_count = fid_ct = 0;
   sendFidCtStatus();
}

void Default_RequestedLivelinessChanged(void* listener_data, DDS_DataReader* reader,
                      const struct DDS_LivelinessChangedStatus *status)
{
/*
    DDS_Long    alive_count
        The total count of currently alive DDS_DataWriter entities that
        write the DDS_Topic the DDS_DataReader reads.
    DDS_Long    not_alive_count
        The total count of currently not_alive DDS_DataWriter entities
        that write the DDS_Topic the DDS_DataReader reads.
    DDS_Long    alive_count_change
        The change in the alive_count since the last time the listener
        was called or the status was read.
    DDS_Long    not_alive_count_change
        The change in the not_alive_count since the last time the listener
        was called or the status was read.
    DDS_InstanceHandle_t        last_publication_handle
        An instance handle to the last remote writer to change its liveliness.
*/
   printf("Default_RequestedLivelinessChanged\n");
   printf("The total count of currently alive DDS_DataWriter entities: %d\n",status->alive_count);
   printf("The total count of currently not_alive DDS_DataWriter entities: %d\n",status->not_alive_count);
   printf("The change in the alive_count: %d\n",status->alive_count_change);
   printf("The change in the not_alive_count: %d\n", status->not_alive_count_change);
   printf("handle to the last remote writer to change its liveliness: 0x%lx\n",status->last_publication_handle);
   // errLogRet(LOGIT,debugInfo,"'%s': A reliable subscription for Pub: '%s' has Disappeared.\n",
   //                      hostName,status->nddsTopic);
   if (status->alive_count == 0)
   {
       DPRINT(-5,"set inactive\n");
       fid_count = fid_ct = -1;
       sendFidCtStatus();
   }
   
}
#endif
//      Handles the DDS_PUBLICATION_MATCHED_STATUS status.
void DataUpload_PublicationMatched(void* listener_data,
                                         DDS_DataWriter* writer,
                                         const struct DDS_PublicationMatchedStatus *status)
{
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(1,"------ Publication Type: '%s', Name: '%s' Matched -----------\n",
            DDS_TopicDescription_get_type_name(topicDesc),DDS_TopicDescription_get_name(topicDesc));
    DPRINT1(1,"  The total cumulative number of times the concerned DDS_DataWriter discovered a 'match' with a DDS_DataReader: %d\n",
              status->total_count);
    DPRINT1(1,"  The incremental changes in total_count since the last time the listener was called or the status was read: %d\n",
              status->total_count_change);
    DPRINT1(1,"  The current number of readers with which the DDS_DataWriter is matched: %d\n",
              status->current_count);
    DPRINT1(1,"  The change in current_count since the last time the listener was called or the status was read: %d\n",
              status->current_count_change);
    DPRINT1(1,"  A handle to the last DDS_DataReader that caused the the DDS_DataWriter's status to change: 0x%lx\n",
              status->last_subscription_handle);
    DPRINT(1," ----------------------------------------\n");
}

//      <<eXtension>> A change has occurred in the writer's cache of unacknowledged samples.
void DataUpload_ReliableWriterCacheChanged(void* listener_data,DDS_DataWriter* writer,
                                        const struct DDS_ReliableWriterCacheChangedStatus *status)
{
/*
    printf(" ------ CacheChanged -----------\n");
    printf("  The number of times the reliable writer's cache of unacknowledged samples has become empty: %d\n",
              status->empty_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has become full: %d\n",
              status->full_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has fallen to the low watermark: %d\n",
              status->low_watermark_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has risen to the high watermark: %d\n",
              status->high_watermark_reliable_writer_cache);
    printf("  The current number of unacknowledged samples in the writer's cache: %d\n",
              status->unacknowledged_sample_count);
    printf(" ----------------------------------------\n");
*/
}

//      <<eXtension>> A matched reliable reader has become active or become inactive.
void DataUpload_ReliableReaderActivityChanged(void* listener_data,DDS_DataWriter* writer,
                                        const struct DDS_ReliableReaderActivityChangedStatus *status)
{
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(1," ------ ReaderActivityChanged for: Type: '%s', Name: '%s'  -----------\n",
        DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));

    DPRINT1(1,"  The current number of reliable readers currently matched with this reliable writer: %d\n",
              status->active_count);
    DPRINT1(1,"  The number of reliable readers that have been dropped by this reliable writer because they failed to send acknowledgements in a timely fashion: %d\n",
              status->inactive_count);
    DPRINT1(1,"  The most recent change in the number of active remote reliable readers: %d\n",
              status->active_count_change);
    DPRINT1(1,"  The most recent change in the number of inactive remote reliable readers: %d\n",
              status->inactive_count_change);
    DPRINT1(1,"  The instance handle of the last reliable remote reader to be determined inactive: 0x%lx\n",
              status->last_instance_handle);
    DPRINT(1," ----------------------------------------\n");
   if (status->active_count == 0)
   {
       DPRINT(-1,"set inactive\n");
       fid_count = fid_ct = -1;
       sendFidCtStatus();
   }
   else if (status->active_count > 0)
   {
       DPRINT(-1,"set active\n");
       fid_count = fid_ct = 0;
       sendFidCtStatus();
   }
}



#endif  /* RTI_NDDS_4x */
      

/* NDDS */
/*
 * Create the Fid Data Upload Publication 
 *
 */
NDDS_ID createDataUploadPub(NDDS_ID nddsId)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     /* NDDSPublicationProperties publicationProperties;
     NDDSPublicationListener   myPublicationListener; */

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
    sprintf(pubtopic,CNTLR_PUB_UPLOAD_TOPIC_FORMAT_STR,hostName);
    strcpy(pPubObj->topicName,pubtopic);
#else /* RTI_NDDS_4x */
    strcpy(pPubObj->topicName,DATA_UPLOAD_M21TOPIC_STR);
#endif  /* RTI_NDDS_4x */
    pPubObj->pubThreadId = 130; /* UPLINKER_TASK_PRIORITY; DEFAULT_PUB_THREADID; taskIdSelf(); */

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getData_UploadInfo(pPubObj);
#ifndef RTI_NDDS_4x
    (*pPubObj->TypeRegisterFunc)();
    pPubObj->instance = (*pPubObj->TypeAllocFunc)();
    pPubObj->dataTypeSize = (*pPubObj->TypeSizeFunc)(0);
#endif  /* RTI_NDDS_4x */
 

    /* setup publication properties */
    pPubObj->queueSize  = 2;  /* about 2 fid of np=512K 5MB */
    pPubObj->highWaterMark = 1;
    pPubObj->lowWaterMark =  0;
    pPubObj->AckRequestsPerSendQueue = 1;
    /* pPubObj->AckRequestsPerSendQueue = 32; */
#ifndef RTI_NDDS_4x
    pPubObj->pubRelStatRtn = UplinkPubStatusRtn;
    pPubObj->pubRelStatParam =  (void*) pPubObj;
#endif  /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
    initPublication(pPubObj);
    attachPublicationMatchedCallback(pPubObj, DataUpload_PublicationMatched);
    attachReliableReaderActivityChangedCallback(pPubObj, DataUpload_ReliableReaderActivityChanged);
    /* give the node name for Sendproc to create proper publication back */
    attachDWDiscvryUserData(pPubObj, hostName, strlen(hostName)+1);
#endif  /* RTI_NDDS_4x */
    createPublication(pPubObj);

#ifndef RTI_NDDS_4x
    DPRINT2(-1,"pPubObj: 0x%lx, publication: 0x%lx\n", pPubObj,pPubObj->publication);
#endif  /* RTI_NDDS_4x */

    fid_count = fid_ct = -1;

    return(pPubObj);
}

initUpLink()
{
   startDataPublisher(UPLINKER_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
}


#ifndef RTI_NDDS_4x
delfidpub()
{
    Data_Upload *issue;
    issue = (Data_Upload *) pUploadPub->instance;
    /* must restore original allocated space that publication created */
    issue->data.val = pubIssueDataPtr;
    nddsPublicationDestroy(pUploadPub);
    nddsSubscriptionDestroy(pUploadSubRdy);
}
#endif  /* RTI_NDDS_4x */


/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 * Used by Recvproc to communicate with the DDR
 */
#ifndef RTI_NDDS_4x
RTIBool Data_UploadCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    PUBLSH_MSG pubMsg;
    Data_Upload *recvIssue;
    int bufcmd;

    if (DebugLevel > 0)
       printNDDSInfo(issue);

    if (issue->status == NDDS_FRESH_DATA)
    {
       recvIssue = (Data_Upload *) instance;
       DPRINT1(+1,"Data_UploadCallBack:  cmd: %d\n",recvIssue->sn);
       /* note that recvproc is ready for data */
       switch(recvIssue->sn)
       {
          case C_RECVPROC_READY:     /* recvproc ready for upload */
                     DPRINT(-1,"Data_UploadCallBack:  Recvproc Ready\n");
                     /* sentAtStartMsg = 0; */
                     wvEvent(991,NULL,NULL);
		     
		     RecvprocReady = 1;
		     ddrActiveFlag = (int) recvIssue->elemId;
                     DPRINT1(-2,"-------->  ddrActiveFlag: %d (ddrposition, -1 == nonactive)\n",ddrActiveFlag);
                     DPRINT1(-4,"-------->  maxTransferLimit: %d\n",recvIssue->totalBytes);
		     setMaxTransferLimit((int) recvIssue->totalBytes);
		     resetTransferLimit();
                     DPRINT1(-2,"-------->  FID Xfer Limit count: %d\n",recvIssue->totalBytes);
                     /* send msge which resets adcOvrFlowSent use to send msg to recvproc but does not now */
                     pubMsg.tag = -42;
                     pubMsg.donecode = pubMsg.errorcode = pubMsg.dspIndex = pubMsg.crc32chksum = 0;
                     msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(pubMsg), WAIT_FOREVER, MSG_PRI_NORMAL);
		     break;
          case C_RECVPROC_DONE:    /* recvproc done with data reception */
                     DPRINT(-1,"Data_UploadCallBack:  Recvproc Done \n");
                     wvEvent(992,NULL,NULL);
		     RecvprocReady = 0;
                     /* reset emac TX threashold */
                     setEmacTxThreshold(64);
                     InActiveTimeout = 1; /* set Inactivity flag so shandler can move on if still waiting on WDog */
                     releaseTransferLimitWait(); /* for cases of only one for xferLimit, otherwise FidPusher Hangs */
	             sendFidCtStatus();
		     XferLimitSummary();
		     break;

          case C_RECVPROC_CONTINUE_UPLINK:
                 DPRINT1(-2,"-------->  Mark Incremental FID Xfer: %d (interval)\n",recvIssue->totalBytes);
                 setTransferIntervalAt(recvIssue->totalBytes);
                 break;

          default:
                     DPRINT(-1,"Data_UploadCallBack:  default:  clear sentAtStartMsg\n");
                     /* sentAtStartMsg = 0; */
		     RecvprocReady = 0;
       }
    }
   return RTI_TRUE;
}

#else  /* RTI_NDDS_4x */

void Data_UploadCallback(void* listener_data, DDS_DataReader* reader)
{
   PUBLSH_MSG pubMsg;
   Data_Upload *recvIssue;
   int bufcmd;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;

   struct Data_UploadSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Data_UploadDataReader *Data_Upload_reader = NULL;

   Data_Upload_reader = Data_UploadDataReader_narrow(pUploadSubRdy->pDReader);
   if ( Data_Upload_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   retcode = Data_UploadDataReader_take(Data_Upload_reader,
                        &data_seq, &info_seq,
                        DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                        DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

   if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
   } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
   }

   numIssues = Data_UploadSeq_get_length(&data_seq);

   for (i=0; i < numIssues; i++)
   {
       info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
       if (info->valid_data)
       {

          recvIssue = (Data_Upload *) Data_UploadSeq_get_reference(&data_seq,i);
          DPRINT1(+1,"Data_UploadCallBack:  cmd: %d\n",recvIssue->sn);
          /* note that recvproc is ready for data */
          switch(recvIssue->sn)
          {
             case C_RECVPROC_READY:     /* recvproc ready for upload */
                     DPRINT(-1,"Data_UploadCallBack:  Recvproc Ready\n");
                     /* sentAtStartMsg = 0; */
                     wvEvent(991,NULL,NULL);
		     
		     RecvprocReady = 1;
		     ddrActiveFlag = (int) recvIssue->elemId;
                     DPRINT1(-2,"-------->  ddrActiveFlag: %d (ddrposition, -1 == nonactive)\n",ddrActiveFlag);
                     // DPRINT1(-4,"-------->  maxTransferLimit: %d\n",recvIssue->totalBytes);
		     setMaxTransferLimit((int) recvIssue->totalBytes);
		     resetTransferLimit();
                     DPRINT1(-2,"-------->  FID Xfer Limit count: %d\n",recvIssue->totalBytes);
                     /* send msge which resets adcOvrFlowSent use to send msg to recvproc but does not now */
                     pubMsg.tag = -42;
                     pubMsg.donecode = pubMsg.errorcode = pubMsg.dspIndex = pubMsg.crc32chksum = 0;
                     msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(pubMsg), WAIT_FOREVER, MSG_PRI_NORMAL);
		     break;
             case C_RECVPROC_DONE:    /* recvproc done with data reception */
                     DPRINT(-1,"Data_UploadCallBack:  Recvproc Done \n");
                     wvEvent(992,NULL,NULL);
		     RecvprocReady = 0;
                     /* reset emac TX threashold */
                     setEmacTxThreshold(64);
                     InActiveTimeout = 1; /* set Inactivity flag so shandler can move on if still waiting on WDog */
                     releaseTransferLimitWait(); /* for cases of only one for xferLimit, otherwise FidPusher Hangs */
	             sendFidCtStatus();
		     XferLimitSummary();
		     break;

             case C_RECVPROC_CONTINUE_UPLINK:
                 DPRINT1(-2,"-------->  Mark Incremental FID Xfer: %d (interval)\n",recvIssue->totalBytes);
                 setTransferIntervalAt(recvIssue->totalBytes);
                 break;

             default:
                     DPRINT(-1,"Data_UploadCallBack:  default:  clear sentAtStartMsg\n");
                     /* sentAtStartMsg = 0; */
		     RecvprocReady = 0;
          }
       }
   }
   retcode = Cntlr_CommDataReader_return_loan( Data_Upload_reader,
                  &data_seq, &info_seq);
   return;
}


#endif  /* RTI_NDDS_4x */

int wait4RecvprocDone(int trys)
{
  /* 1st wait on inactivity WDog to trip, or xfer stuck WDog to trip then go on */
  /* InActiveTimeout = XferStuckTimeout = 0; since call back,etc. can happen in different orders this didn't work out. */
  /*                                         it was cuase su and abort to hang for several seconds */
  while( (XferStuckTimeout == 0) && (InActiveTimeout == 0) )
  {
       DPRINT2(+1,"waiting on InActiveTimeout WDog, XferStuckTimeout; %d, InActiveTimeout: %d\n",
		XferStuckTimeout,InActiveTimeout);
       taskDelay(calcSysClkTicks(1000));  /* check every 2 seconds */
  }
  DPRINT2(+1,"waiting on InActiveTimeout WDog, XferStuckTimeout; %d, InActiveTimeout: %d\n",
		XferStuckTimeout,InActiveTimeout);
 
  for( ; trys >= 0; trys--)
  {
      DPRINT1(+1,"Checking RecvprocReady == 0, RecvprocReady = %d\n",
	RecvprocReady);
      /* taskDelay(calcSysClkTicks(1000*5));  */
      if (RecvprocReady == 0)
        break;

      taskDelay(calcSysClkTicks(17)); /* taskDelay(1); */
  }
  if (trys <= 0)
  {
     return -1;
  }
  else
     return 0;
}

int ddrActive()
{
    int active = 0;
    active = ((ddrActiveFlag >= 0) || (ddrActiveFlag < 0xffffffff)) ? 1 : 0;
    return active;
}



/* NDDS */
NDDS_ID createDataUploadSub(NDDS_ID nddsId)
{

   NDDS_ID  pSubObj;
   char subtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }

    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,nddsId,sizeof(NDDS_OBJ));

   /* sprintf(subtopic,CNTLR_SUB_UPLOAD_MCAST_TOPIC_FORMAT_STR); /* "h/ddr/upload/reply" */
   sprintf(subtopic,CNTLR_SUB_UPLOAD_TOPIC_FORMAT_STR,hostName);   /* "h/%s/upload/reply */


    /* strcpy(pSubObj->topicName,"wormhole_CodesDownld"); */
    strcpy(pSubObj->topicName,subtopic);

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getData_UploadInfo(pSubObj);

#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Data_UploadCallback;
    pSubObj->callBkRtnParam = NULL;
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    pSubObj->BE_DeadlineMillisec = 120 * 1000; /*  2 Minutes, for sub to considered lost */
    pSubObj->queueSize  = 64;
#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Data_UploadCallback,NULL);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj);
    return ( pSubObj );
}

#ifndef RTI_NDDS_4x
prtUpldSubStatus()
{
   NDDSRecvInfo info;
   if (NddsSubscriptionStatusGet(pUploadSubRdy->subscription, &info) == RTI_TRUE)
      printNDDSInfo(&info);
}

prtUpldPubStatus()
{
  int unackIssues;
  NDDSPublicationReliableStatus status;
  if (pUploadPub != NULL)
   unackIssues = NddsPublicationReliableStatusGet(pUploadPub->publication, &status);
   printReliablePubStatus(&status);
}
#else  /* RTI_NDDS_4x */
#endif  /* RTI_NDDS_4x */

sndADC()
{
    sendException(WARNING_MSG, 103 /* WARNINGS + ADCOVER */, 0,0,NULL);
}


#ifdef DMA_TESTING

static char *tstDmaBufPtr;
builddmabuf()
{
    char *bufferZone;
     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     tstDmaBufPtr = (char*) malloc(1024*1024); /* max DMA transfer in single shot */
     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     printf("bufaddr: 0x%08lx\n",tstDmaBufPtr);
}

qdma(int num2Q)
{
   int i,status;
   int dmaChannel;
   long *srcAddr;
   long *dstAddr;

   int xfersize = 64000;

   dmaChannel = getDataDmaChan();
   srcAddr = (long *) 0x90000000;
   dstAddr = (long *) tstDmaBufPtr;
   for (i=0; i < num2Q; i++)
   {
         /* a DMAing buffer of it's free List */
         dstAddr = fBufferGet(pDmaFreeList);    /* may pend if exhausted buffers on free list */

      /* Initiate DMA transfer from DSP to data buffer on 405 */
       printf("%d: chan: %d, srcAddr: 0x%lx, dstAddr: 0x%lx, TransferSize: %d\n",i+1,dmaChannel,srcAddr,dstAddr,xfersize/4);
       status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY_PACED /* MEMORY_TO_MEMORY ,FPGA_TO_MEMORY */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, xfersize/4, NULL, pDataAddrMsgQ);
   }
}

/*  Uses the  DMARequest reigster to enable or disable DMA
    via the Device paced feature of DMA on the 405   
 */
dmaa()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx\n",dmaReg);
}
dmaon()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    set_register(DDR,DMARequest,1);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    /* set_field(DDR,dma_request,0); */
   /* printf("DMA_Request Reg: %d\n",get_register(DDR,dma_request)); */
   printf("DMA_Request Reg: %d\n",get_register(DDR,DMARequest));
}

dmaoff()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    set_register(DDR,DMARequest,0);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    /* set_field(DDR,dma_request,1); */
   /* printf("DMA_Request Reg: %d\n",get_register(DDR,dma_request)); */
   printf("DMA_Request Reg: %d\n",get_register(DDR,DMARequest));
}

dmatgl()
{
    set_register(DDR,DMARequest,1);   /* on */
    set_register(DDR,DMARequest,0);   /* off */
}

chkdma()
{
   int dmaChannel, dmaReqsQd, free2Q;
   dmaChannel = getDataDmaChan();
   dmaReqsQd = dmaReqsInQueue(dmaChannel);
   free2Q = dmaReqsFreeToQueue(dmaChannel);
   printf("Data Transfer: DMA Channel: %d\n",dmaChannel);
   printf("Data Transfer: DMA_Request Reg: %d\n",get_field(DDR,dma_request));
   printf("Data Transfer: DMA device paced pended: %d\n",dmaGetDevicePacingStatus(dmaChannel));
   printf("Data Transfer: DMA Request Queued: %d, Remaining Queue Space: %d\n",dmaReqsQd, free2Q);
   printf("\n\n");
   dmaDebug(dmaChannel);
}
#endif

#ifdef XXXXX
wtDataMsgQ()
{
 unsigned long dataAddr;

 dataAddr = 0xbadac0deL;
 dataAddr = 0x33cb668L;
 DPRINT2(-1,"sending 0x%lx, in MsgQ: 0x%lx\n",dataAddr,pDataAddrMsgQ);
 msgQSend(pDataAddrMsgQ, (char *)&(dataAddr), 4, NO_WAIT, MSG_PRI_NORMAL);
}
rdDataMsgQ()
{
   unsigned long dataAddr;
   int stat;
   stat = msgQReceive(pDataAddrMsgQ, (char*) &dataAddr,4, 30);
   DPRINT2(-1,"rdDataMsgQ() - stat: %d, DMA Buffer Addr: 0x%lx\n", stat, dataAddr);
   return 0;
}
rdXfrMsgQ()
{
   PUBLSH_MSG pubMsg;
   int stat;
   stat =  msgQReceive(pDataTagMsgQ,(char*) &pubMsg,sizeof(pubMsg), 30);
   DPRINT3(-1,"rdXfrMsgQ(): stat: %d, dataTag: %d, FID CRC: 0x%lx\n",stat,pubMsg.tag,pubMsg.crc32chksum);
   return 0;
}
#endif

sndBlkErr(int Error)
{
   int bytes;
   FID_STAT_BLOCK *pStatBlk;
   pStatBlk = &TstStatBlk;

   pStatBlk->elemId = 1;	/* FID # */
   pStatBlk->startct = 1;	
   pStatBlk->ct = 1;	
   pStatBlk->nt = 32;	
   pStatBlk->np = 4096 / 4;	
   pStatBlk->dataSize = 4096;	 /* FID size in bytes */
   if (Error)
   {
     pStatBlk->doneCode = HARD_ERROR;
     pStatBlk->errorCode = SYSTEMERROR + IDATA_CRC_ERR;
   }
   else
   {
     pStatBlk->doneCode = WARNING_MSG	;
     pStatBlk->errorCode = WARNINGS+ADCOVER;
   }
   pStatBlk->fidAddr = (long*) 0x00000000;

   bytes = sendStatBlk2Recvproc((char*)pStatBlk, sizeof(FID_STAT_BLOCK)); /* NDDS */
}
sndBlk(int fidnum,int sizebytes)
{
   int bytes;
   PUBLSH_MSG pubMsg;
   FID_STAT_BLOCK *pStatBlk;
   upLkTestFlag = 1;

   pStatBlk = &TstStatBlk;

   pStatBlk->elemId = fidnum;	/* FID # */
   pStatBlk->startct = 1;	
   pStatBlk->ct = 1;	
   pStatBlk->nt = 32;	
   pStatBlk->np = sizebytes / 4;	
   pStatBlk->dataSize = sizebytes;	 /* FID size in bytes */
   pStatBlk->doneCode = 0;
   pStatBlk->errorCode = 0;
   pStatBlk->fidAddr = (long*) 0x90000000;

   bytes = sendStatBlk2Recvproc((char*)pStatBlk, sizeof(FID_STAT_BLOCK)); /* NDDS */
/*
    pubMsg.tag = 1;
    pubMsg.dspIndex = pubMsg.tag;
    pubMsg.crc32chksum = 0;
*/
}
sndFid(int fidnum,int dataSize)
{
   FID_STAT_BLOCK *pStatBlk;
   long dataTag, dataAddr, dmaBufPtr;
   long bytesleft,bytes,i,xfrsize;
   long dmaBufOffset,dmaBytesleft;
   int status;
   unsigned long fidCrc;
   tcrc calcChksum,dspChksum;
   Data_Upload *issue = pUploadPub->instance;

   issue->type = DATAUPLOAD_FID; 
   bytesleft =  issue->totalBytes = dataSize;
   issue->dataOffset = 0;
   issue->sn = 0; 
   dmaBufPtr = 0x10000;
   issue->crc32chksum = calcChksum = addbfcrcinc((char*)dmaBufPtr,dataSize,(tcrc*) 0);

   while(bytesleft > 0)
   {
        issue->sn = i++; 
        xfrsize = (bytesleft < 1024)  ? dmaBytesleft : 1024;
#ifndef RTI_NDDS_4x
        issue->data.len = xfrsize;
        issue->data.val = (char*) dmaBufPtr;
#else  /* RTI_NDDS_4x */
        DDS_OctetSeq_set_maximum(&(issue->data), 0);
        DDS_OctetSeq_loan_contiguous(&(issue->data),
                                   (char*) dmaBufPtr,      // pointer
                                   0,                 // length
                                   DMA_BUFFER_SIZE); // maximum
        DDS_OctetSeq_set_length(&(issue->data), xfrsize);
#endif  /* RTI_NDDS_4x */
        DPRINT6(-1,"sndFid() - sn: %2d, Addr: 0x%08lx, offset: 0x%08lx, DMA bytesleft: %6ld, bytesleft: %7ld, xfrsize: %6ld\n",
		 issue->sn, dmaBufPtr, issue->dataOffset, dmaBytesleft, bytesleft, xfrsize);
     
            
#ifndef RTI_NDDS_4x
        status = nddsPublishData(pUploadPub);
#else  /* RTI_NDDS_4x */
        status = publishFidData(pUploadPub);
#endif  /* RTI_NDDS_4x */
        if (status == -1)
        {
          errLogRet(LOGIT,debugInfo,
	      "!!!!!!!!!!!!!!!!! E R R O R  in Publishing Fid Data  !!!!!!!!!!!!!!!!!!!!!!!!! \n");
        }
#ifdef RTI_NDDS_4x
        DDS_OctetSeq_unloan(&(issue->data));
#endif  /* RTI_NDDS_4x */

        issue->dataOffset += xfrsize;   /* increment total bytes transfered */
        dmaBufPtr += xfrsize;	    /* increment pointer within DMA buffer */
        bytesleft -= xfrsize; /* decrement bytes left to xfer */
   }
}

/* with DMA involved */
tstUpLkwdma(long sizebytes)
{
   DSP_MSG dspMsg;
   FID_STAT_BLOCK *pStatBlk;
   dspMsg.srcIndex = 1;
   pStatBlk->elemId = 1;	/* FID # */
   pStatBlk->startct = 1;	
   pStatBlk->ct = 1;	
   pStatBlk->nt = 32;	
   pStatBlk->np = sizebytes / 4;	
   pStatBlk->dataSize = sizebytes;	 /* FID size in bytes */
   /* pStatBlk->doneCode = NO_DATA; */
   pStatBlk->doneCode = 0;
   dspMsg.dataAddr = 0x90000000L;
   dspMsg.np = sizebytes/4;
   dspMsg.tag = 1;
   dspMsg.crc32chksum = 0;
   msgQSend(pDspDataRdyMsgQ,(char*) &dspMsg, sizeof(DSP_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
}
/* without DMA involved */
tstUpLkSpeed(int sizebytes)
{
   PUBLSH_MSG pubMsg;
   char *dstAddr;
   long xfersize, bytesleft, offset;
   long fidnum, np,ct, endct, nt, dp, tag, dataaddr;
   FID_STAT_BLOCK *pStatBlk;

   xfersize = 256000; /* 64000*4; */

   upLkTestFlag = 1;

   pStatBlk = &TstStatBlk;

   pStatBlk->elemId = 1;	/* FID # */
   pStatBlk->startct = 1;	
   pStatBlk->ct = 1;	
   pStatBlk->nt = 32;	
   pStatBlk->np = sizebytes / 4;	
   pStatBlk->dataSize = sizebytes;	 /* FID size in bytes */
   /* pStatBlk->doneCode = NO_DATA; */
   pStatBlk->doneCode = 0;
   pStatBlk->errorCode = 0;
   pStatBlk->fidAddr = (long*) 0x90000000;
/*
   p2statb = (FID_STAT_BLOCK *) 
             allocAcqDataBlock( (ulong_t) fidnum, (ulong_t) np,
                                (ulong_t) ct, (ulong_t) endct,
                                (ulong_t) nt, (ulong_t) (np * dp),
                                (long *) &(tag),
                                (long *) &dataaddr);

   DPRINT2(-1,"nextScan: FID_STAT_BLOCK: 0x%lx,  tagId: %d\n",p2statb,tag);
*/

     /* The publisher sofar needs to know the Tag, dsp index, and the CRC checksum of the FID */
     pubMsg.tag = 1;
     pubMsg.dspIndex = pubMsg.tag;
     pubMsg.crc32chksum = 0;

     /* 1st msgQ of publisher, start of FID transfer */
     msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(PUBLSH_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);

     /* Now base on the FID size and the DMA buffer size, we may have to use more than one
        DMA transfer to get the FID from the DSP side to the 405 side.
     */
     /* srcAddr = 0x90000000; */
     bytesleft = sizebytes;
     while (bytesleft > 0)
     {
         /* a DMAing buffer of it's free List */
         dstAddr = fBufferGet(pDmaFreeList);    /* may pend if exhausted buffers on free list */
          msgQSend(pDataAddrMsgQ, (char *)&(dstAddr), 4, WAIT_FOREVER, MSG_PRI_NORMAL);
         /* srcAddr += xfersize; */
         bytesleft -= xfersize; /* decrement bytes left to xfer */
     }
}

tstFidXfer(int nfids, int np, int PublisherOnly)
{
   int tstFidTransfer();

   taskSpawn("XferTst",100,0,8192,tstFidTransfer,nfids, np,PublisherOnly,
              4,5,6,7,8,9,10);
}

/* PublisherOnly = , 1 - use dataPublisher task directly, 2 - use the dspDataXfer -> dataPublisher path
   3 = interrupt reader -> dspDataXfer -> dataPUlisher
*/
tstFidTransfer(int nfids, int np, int PublisherOnly)
{
   int i;
   PUBLSH_MSG pubMsg;
   DSP_MSG dspMsg;
   char *dstAddr;
   long xfersize, bytesleft, offset;
   long fidnum, ct, endct, nt, dp, tag, dataaddr, tag2send;
   FID_STAT_BLOCK *pStatBlk;
   long  sizebytes;
   typedef struct msg_struct 
   {
    int     id;   // command ID 
    int   arg1;   // command argument 
    int   arg2;   // command argument 
    int   arg3;   // command argument 
   } msg_struct;
   extern MSG_Q_ID msgsFromDDR;

   /* xfersize = 256000; /* 64000*4; */

   upLkTestFlag = 1;
   RecvprocReady = 1;

#ifdef XXX
   pStatBlk = &TstStatBlk;

   pStatBlk->startct = 1;	
   pStatBlk->ct = 1;	
   pStatBlk->nt = 1;	
   pStatBlk->np = np;
   pStatBlk->dataSize = np * 4;	 /* FID size in bytes */
   pStatBlk->doneCode = 0;
   pStatBlk->errorCode = 0;
   pStatBlk->fidAddr = (long*) 0x90000000;

#endif
  
   ct = endct = nt = 1;

   pubMsg.tag = 1;
   pubMsg.dspIndex = pubMsg.tag;
   pubMsg.crc32chksum = 0;

   dspMsg.srcIndex = 1;
   dspMsg.dataAddr = 0x90000000L;
   dspMsg.np = np;
   dspMsg.tag = 1;
   dspMsg.crc32chksum = 0;

   sizebytes = np * 4;

   set_data_info(np);   /* since DDR_Acq.c get_data_info() calc various parameters used by msgsFromDDR receiver */

   /* initial statblocks, since there is BS then number oftotalFidBlks is number fids being transfered */
   dataInitial(pTheDataObject, (ulong_t) nfids, (ulong_t) sizebytes);
   for (i=0; i < nfids; i++)
   {
       fidnum = i+1; /* FID # 1-nfids */
       pStatBlk = dataAllocAcqBlk(pTheDataObject, fidnum, np, ct, endct,
                                        nt,  sizebytes, &tag2send, NULL );

       pStatBlk->fidAddr = (long*) 0x90000000;
       printf("Fid: %d, tag2snd: %d\n",fidnum,tag2send);
       if (PublisherOnly == 1)
       {
         /* The publisher sofar needs to know the Tag, dsp index, and the CRC checksum of the FID */
         /* pubMsg.tag = 1; */
         pubMsg.tag = tag2send;
         pubMsg.dspIndex = pubMsg.tag;
         pubMsg.crc32chksum = 0;
         /* 1st msgQ of publisher, start of FID transfer */
         msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(PUBLSH_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);

         /* Now feed it data */
         bytesleft = sizebytes;
         while (bytesleft > 0)
         {
             /* a DMAing buffer of it's free List */
             dstAddr = fBufferGet(pDmaFreeList);    /* may pend if exhausted buffers on free list */


             xfersize = (bytesleft < DMA_BUFFER_SIZE)  ? bytesleft : DMA_BUFFER_SIZE;

             pubMsg.crc32chksum = 0;
             msgQSend(pDataAddrMsgQ, (char *)&(dstAddr), 4, WAIT_FOREVER, MSG_PRI_NORMAL);

             /* printf("bytesleft: %d, xxfersize: %d\n",bytesleft,xfersize); */

             bytesleft -= xfersize; /* decrement bytes left to xfer */
         }
       }
       else if (PublisherOnly == 2)
       {
           dspMsg.srcIndex = tag2send;
           dspMsg.dataAddr = 0x90000000L;
           dspMsg.np = np;
           dspMsg.tag = tag2send;
           dspMsg.crc32chksum = 0;
           dspMsg.crc32chksum = addbfcrc(pStatBlk->fidAddr,xfersize);
           DPRINT1(-6,"CRC; 0x%lx\n",dspMsg.crc32chksum);

          /* invoke dspDataXfer() task to DMA data to FidPubshr() */
          msgQSend(pDspDataRdyMsgQ,(char*) &dspMsg, sizeof(DSP_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
       }
       else
       {
          msg_struct msg;
          msg.id = 8; /* HOST_DATA; */
          msg.arg1 = tag2send;
          msg.arg2 = tag2send;
          msg.arg3 = 0; 	/* chksum */
          msgQSend(msgsFromDDR, (char *)&(msg), sizeof(msg_struct), WAIT_FOREVER, MSG_PRI_NORMAL);
       }
    }
#ifdef XXXX
   {
    Data_Upload *issue;
    int status;
    issue = (Data_Upload *) pUploadPub->instance;
    issue->type = DATAUPLOAD_FIDSTATBLK;
    issue->sn =  -1; /* mark end of transfer */
    issue->elemId = issue->dataOffset = 0;
    issue->totalBytes = issue->data.len = 0;
    nddsPublishData(pUploadPub);
   }
#endif 
}
