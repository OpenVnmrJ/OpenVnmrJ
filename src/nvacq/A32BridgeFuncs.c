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
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <msgQLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "nvhardware.h"
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "sysflags.h"
#include "errorcodes.h"
#include "expDoneCodes.h"
#include "nameClBufs.h"
#include "cntrlFifoBufObj.h"
#include "PSGFileHeader.h"
#include "lc.h"
#include "dataObj.h"
#include "AParser.h"
#include "rf_fifo.h"
#include "master_fifo.h"
#include "ACode32.h"
#include "cntlrStates.h"
#include "dmaReg.h"
#include "FFKEYS.h"
#include "masterAux.h"

/*---------------*/
extern int BrdType;  /* board type */
extern ACODE_ID pTheAcodeObject;
extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */
extern MSG_Q_ID pMsgesToAParser;
extern RING_ID  pSyncActionArgs;
extern int AbortingParserFlag;
extern int failAsserted;
extern int readuserbyte;

Acqparams acqReferenceData;
autodata *autoDP;

/* name buffer handle */
extern NCLB_ID nClBufId;

extern MSG_Q_ID pMsgesToAParser;

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */


static int startOnSyncFlag = 0;

static int fifoStarted4Exp = 0;

int icatDelayFix = 64;   // was 54

#define NOPREP 0    /* not a go('prep'), for SystemSync */

/*********************************************************
*
* Forward Function Defines
*
**********************************************************/

/********************************************************
 *  Transfer the List to the FIFO, via dma, etc.
 *
 ********************************************************/

int sendCntrlFifoList(int* list, int size, int xferNtimes)
{

   int freeTxDesc, freeSGnodes;
   /*  only greater than 1 if size is greater than a single DMA transfer can handle */
   int sgNodesPerSingleTransfer;    /* number of SG Nodes for the transfer of the buffer */
   int numXfersPerQedDMA;           /* number of transfers of the data within a SG list for a single DMA Q entry */
   int xfers,totalXfers,xfer2Q;
   int fifoDmaChan;
   int SGnodesBlockAt, WillBlockAt, QEntryNum;
   void startFifo4Exp();
   int startFifoCalled;
   int status;

   /* MaxSGListQSize = 50; */

   DPRINT3(-1,"sendCntrlFifoList(): buffer: 0x%lx, size: %d, times: %d\n",
	list, size, xferNtimes);

    /* catch the nop cases */
    if ((size < 1) || (xferNtimes < 1) || (list == NULL))
    {
       /* DPRINT(-1,"size, times or pattern pointer are ZERO, just return. \n"); */
       return(0);
    }

   /* is parser is being aborted then return from this routine */
   if ((AbortingParserFlag == 1) || ( failAsserted == 1))
	  return(0);  /* don't produce error */

   /*
       minimum requirements to do anything:
       1 free TxDesc  (freeTxDesc >= 1)
       1 free SG Node (freeSGnodes >= 1)

       1st do we have enough SG nodes for xferNtimes?
          A. we may want to break it up into several SG list
       2nd Don't have enought SG nodes.
          A. Use what we can, and submit
          b. Then startFifo if not started yet.
          c. Submit remainder, know that this WILL make us PEND.
   */

   fifoDmaChan = cntrlFifoDmaChanGet();
   freeTxDesc = dmaNumFreeTxDescs();
   freeSGnodes = dmaNumFreeSGListNodes();
   DPRINT2(-1,"sendCntrlFifoList(): Free TxDesc: %d, Free SgNodes: %d\n", freeTxDesc, freeSGnodes);

  /* we do not want to create exceeding long SG lists so we limit how large these can be
     via MaxSGListQSize
    nsubmits is the number of times we will need to submit an SG list to obtain the
    total number of transfers required.
  */

   sgNodesPerSingleTransfer = size / MAX_DMA_TRANSFER_SIZE +
               (((size % MAX_DMA_TRANSFER_SIZE) > 0) ? 1 : 0);

   /* limiting size of SG lists, to avoid possibly underflows */
   /* call to cntrlFifoXferSGList() will use addition SGnode if the size is greater than the max single */
   /* DMA transfer can handler (65K), thus we take that into account here */
   /* max is 50 divided by the nubmer of SGnodes per transfer plus one if there is any remaining */
   /* in the simple case where the transfer size < 64K this is 50 */

   numXfersPerQedDMA = 50 / sgNodesPerSingleTransfer + (((50 % sgNodesPerSingleTransfer) > 0) ? 1 : 0);

   DPRINT2(-1,"sgNodesPerSingleTransfer: %d, numXfersPerQedDMA: %d\n",sgNodesPerSingleTransfer, numXfersPerQedDMA);

   /* all the following calc are to determine where within the queuing of DMA
      requests will the SG nodes run out thus pending this routine and that
      of the task calling us. (Parser)
   */
   totalXfers = xferNtimes;
   xfer2Q = (totalXfers < numXfersPerQedDMA) ? totalXfers : numXfersPerQedDMA;
   SGnodesBlockAt = freeSGnodes / ( xfer2Q * sgNodesPerSingleTransfer);
   WillBlockAt = ( freeTxDesc < SGnodesBlockAt) ? freeTxDesc : SGnodesBlockAt;

   DPRINT2(-1,"SGnodesBlockAt: %d entry, WillBlockAt: %d entry\n",SGnodesBlockAt,WillBlockAt);

   /* flush any previous pending FIFO words into the FIFO, prior to Queuing this list for DMA into the FIFO */
   cntrlFifoBufForceRdy(pCntrlFifoBuf);

   startFifoCalled = 0;
   if (freeTxDesc < 1)
   {
        /* No more room to queue */
        /* Then start the FIFO in anticipation of PENDING */
        DPRINT(-1,"sendCntrlFifoList(); start Fifo Zero freeTxDesc buffers \n");
        startFifo4Exp();  /* startCntrlFifo(); */
        startFifoCalled = 1;
   }
   else if (freeSGnodes < (numXfersPerQedDMA * sgNodesPerSingleTransfer) )
   {
        /* One could do fancier calcs, but for now if there isn't enough */
        /* SG nodes to create a single SG list base on the numXfersPerQedDMA */
        /* Then start the FIFO in anticipation of PENDING */
        DPRINT3(-1,"sendCntrlFifoList(); start Fifo freeSGnodes (%d) < numXfersPerQedDMA (%d) * sgNodesPerSingleTransfer (%d)\n",
		freeSGnodes, numXfersPerQedDMA, sgNodesPerSingleTransfer);
        startFifo4Exp();  /* startCntrlFifo(); */
        startFifoCalled = 1;
   }

   totalXfers = xferNtimes;
   xfers = 1;
   QEntryNum = 0;
   while(totalXfers > 0)
   {
      QEntryNum += 1;
      xfer2Q = (totalXfers < numXfersPerQedDMA) ? totalXfers : numXfersPerQedDMA;
      DPRINT3(-1,"sendCntrlFifoList(): xfersQed; %d,  xfers: %d, xfers2Q: %d\n",QEntryNum,xfers,xfer2Q);
      /* is parser is being aborted then return from this routine */
      if ((AbortingParserFlag == 1) || ( failAsserted == 1))
      {
          DPRINT(-1,"sendCntrlFifoList(): AParser Aborted\n");
	  return(0); /* don't send a message from here */
      }

      if ( ( QEntryNum  >= WillBlockAt) && (startFifoCalled != 1) )
      {
          DPRINT2(-1,"sendCntrlFifoList(); start Fifo xfr (%d) == WillBlockAt(%d)\n",
		QEntryNum, WillBlockAt);
          startFifo4Exp();  /* startCntrlFifo() */
          startFifoCalled = 1;
      }
      status = cntrlFifoXferSGList(pCntrlFifoBuf, (long*) list, size, xfer2Q, 0, 0);
      if (status == -1)
         return(status);
      execNextDmaRequestInQueue(fifoDmaChan);   /* fire off DMA engine and begin actual transfer */
      totalXfers = totalXfers - xfer2Q;
      xfers = xfers + xfer2Q;
   }
   return(0);
}

/********************************************************
 *  Transfer the List to the FIFO, via dma, etc.
 *
 ********************************************************/
int writeCntrlFifoWord(int word)
{
   cntrlFifoBufPut(pCntrlFifoBuf,(unsigned long*) &word, 1);
}

int writeCntrlFifoBuf(int *list, int size)
{
   cntrlFifoBufPut(pCntrlFifoBuf,(unsigned long*) list, size);
}

/*
 * force the working buffer of acodes to be queued for DMA
 *
 */
int flushCntrlFifoRemainingWords()
{
    cntrlFifoBufForceRdy(pCntrlFifoBuf);
}

/*
 * Queue the buffer of fifo words for DMA into the FIFO
 * only if the number of words exceeds a minimum (128 at present)
 *
 */
int scheduleCntrlFifoRemainingWords()
{
    cntrlFifoBufScheduleRdy(pCntrlFifoBuf);
}

/*
 * Return the number of Fifo words in the Working Buffer
 *
 */
unsigned long wordsInFifoBuf()
{
 return (cntrlFifoBufWkEntries(pCntrlFifoBuf));
}



int startCntrlFifo()
{
    if (pCntrlFifoBuf->queueMode == CNTRL_DECODER_MODE)
    {
        DPRINT(-1,"startCntrlFifo(): start FIFO\n");
    }
    else
    {
       DPRINT1(-1,"startCntrlFifo():  fifoStarted4Exp: '%s'\n", ( (fifoStarted4Exp == 1) ? "YES" : "NO") );
       if (BrdType == MASTER_BRD_TYPE_ID)  /* Master */
       {
        cntrlFifoStart(); /* start_FIFO();   */
       }
       else
       {
         if (startOnSyncFlag == 0)
         {
	    DPRINT(-1," Cntlr used cntrlFifoStart() \n");
            cntrlFifoStart();
         }
         else
         {
	    startOnSyncFlag = 0;
	    DPRINT(-1," Cntlr used cntrlFifoStartAndSync() \n");
            cntrlFifoStartAndSync();
         }
       }
    }
    return 0;
}

/*
 * startFifo4Exp()
 * used to flag that the FIFO has been started for this experiment
 * and only under special curcomstances should it be restarted
 *
*/
void startFifo4Exp()
{

  if (fifoStarted4Exp == 0)
  {
     startCntrlFifo();
     fifoStarted4Exp = 1;
  }
  else
  {
     DPRINT(-1,"startFifo4Exp: already started 4 exp.\n");
  }
  return;
}

/**************************************************************
*
*  fifoClrStart4Exp - CLears the FIFO started for Exp Flag
*
*
* RETURNS:
*     void
*
* Note: reset in A_interp nextscan()
*/
void clearStart4ExpFlag()
{
  fifoStarted4Exp = 0;
  return;
}

/**************************************************************
*
*  fifoGetStart4Exp - Gets the FIFO started for Exp Flag
*
*
* RETURNS:
*     int fifo started for Exp. flag
*
*/
int getStart4ExpFlag()
{
  return(fifoStarted4Exp);
}


/**************************************************************
*
*  wait4CntrlFifoStop - return when Cntrl FIFO Stops
*
*
* RETURNS:
*     void
*
*/
void wait4CntrlFifoStop()
{
    if (pCntrlFifoBuf->queueMode == CNTRL_DECODER_MODE)
    {
        DPRINT(-1,"wait4CntrlFifoStop(): wait for FIFO to stop\n");
    }
    else
        cntrlFifoWait4StopItrp();
    return;
}

int allocDataWillBlock()
{
    return(dataAllocWillBlock(pTheDataObject));
}

int peekAtNextTag(int *nxtTag)
{
    return( dataPeekAtNextTag(pTheDataObject, nxtTag) );
}

FID_STAT_BLOCK *allocAcqDataBlock(ulong_t fidnum, ulong_t np, ulong_t ct, ulong_t endct,
                  ulong_t nt, ulong_t size, long *tag2snd, long *scan_data_adr )
{
    FID_STAT_BLOCK *p2statblk;

    p2statblk = dataAllocAcqBlk(pTheDataObject, fidnum, np, ct, endct,
                                        nt,  size, tag2snd, scan_data_adr );

    return(p2statblk);
}

/* obtain the FID StatBlock  from the Tag reference */
FID_STAT_BLOCK *getStatBlockByTag(int tagid)
{
    FID_STAT_BLOCK *p2statblk;

    p2statblk = dataGetStatBlk(pTheDataObject,tagid);

    return(p2statblk);
}

/* print out the data statblocks, level > 0 will print out each statblock 
   with its member values
       GMB   3/14/2012
*/
dumpStatBlocks(int level )
{
   stmShow(pTheDataObject, level);
}

int abortFifoBufTransfer()
{
    /* disable DMA channel then clear any DMA request and return
       buffers */
    cntrlFifoBufXfrAbort(NULL);
    return 0;
}

int resetExpBufsAndParser()
{
   acodeClear(pTheAcodeObject);
}

int SystemRollCall()
{
    if (BrdType == MASTER_BRD_TYPE_ID)  /* Master */
    {
       rollcall();
    }
    return 0;
}

int ClearCtlrStates()
{
    cntlrSetStateAll(CNTLR_READYnIDLE);  /* CNTLR_READYnIDLE 1 */
    return 0;
}


int SystemSync(int postdelay,int prepflag)
{
  int instrwords[20];
  int len,total;
  // DPRINT2(-7,"SystemSync: postdelay: %d, prerflag: %d\n",postdelay,prepflag);

    len = 0;
    if (BrdType == MASTER_BRD_TYPE_ID)  /* Master */
    {
	 /* issue SW Itr #4, then halt, fifo is restarted sync will be Asserted */
         len += fifoEncodeSWItr(4, instrwords);
         instrwords[len++] = encode_MASTERSetDuration(0,0); /* Haltop */
         instrwords[len++] = encode_MASTERSetGates(1,0xfdf,0);
         /* LEAVE THE SSHA GATE ALONE -needed for SETVT etc. not the first systemsync */
         len += fifoEncodeSystemSync(postdelay, &instrwords[len] );
         addPendingSWItr(SENDSYNC);
         rngBufPut(pSyncActionArgs,(char*) &prepflag,sizeof(int));
    }
    else
    {
       if (BrdType == RF_BRD_TYPE_ID)  /* Master */
       {
           int RfType = execFunc("getRfType", NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL);
           DPRINT1(-1,"SystemSync: RF Brd, rftype = %d\n",RfType);
           if (RfType == 1)
           {
              // postdelay = postdelay - 64;
              postdelay = postdelay - icatDelayFix;
            
              // Since SystemSync event in shandler.c always clears the cumulative FIFO count
              // only the icatDelayFix value is need to correct for the cumulative tick count
              // and no need to sum the number of SystemSync corrections

              postdelay = (postdelay < 4) ? 4 : postdelay;
           }
           DPRINT1(-1,"SystemSync: RF Brd, postdelay = %d\n",postdelay);
       }
	 /* issue SW Itr #4, now FIFO is paused on zero count waiting for SYNC */
       len += fifoEncodeSWItr(4, instrwords);
       len += fifoEncodeSystemSync(postdelay, &instrwords[len] );
       startOnSyncFlag = 1;
       addPendingSWItr(WAIT4ISYNC);
    }
    writeCntrlFifoBuf(instrwords,len);
    return 0;
}

/*========================================================*/
/* Sample stuff
/*========================================================*/
queueSamp(int action, unsigned long sample2ChgTo, int skipsampdetect, ACODE_ID pAcodeId, int bumpFlag)
{
   if (BrdType == MASTER_BRD_TYPE_ID)            // master controller
   {
      DPRINT1(-1,"Queue SAMP: Action: %d, loc: %lu \n",sample2ChgTo);
      rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
      rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(bumpFlag));

      signal2_syncop(GETSAMP);    // signal & stop fifo
      SystemSync(320,NOPREP);            // when done start all alike

#ifdef XXX
      signal_syncop(action | 0x80000,-1L,-1L);  /* action: GETASMP or LOADSAMP, signal & stop fifo */
#endif

      DPRINT(-10,"==========>  GetSamp: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

#ifdef INSTRUMENT
   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);

   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}

tstGetSample()
{
   int taskId;
   int taskPriority;
   int sample2ChgTo, skipsampdetect, bumpflag;
   taskId = taskIdSelf();
   taskPriorityGet(taskId, &taskPriority);
   DPRINT2(-1,"taskId: 0x%x, taskPriority = %d\n",taskId, taskPriority);

   taskPrioritySet(taskId,115);
   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,60);

  DPRINT(-1,"calling queueSample\n");
   sample2ChgTo = 10;
   skipsampdetect = 0;
   bumpflag = 0;
  queueSamp(GETSAMP,sample2ChgTo,skipsampdetect,pTheAcodeObject,bumpflag);
  DPRINT(-1,"Returned\n");

  taskPrioritySet(taskId,taskPriority);
}

queueLoadSamp(int action, unsigned long sample2ChgTo, int skipsampdetect,
                     ACODE_ID pAcodeId, int spinActive, int bumpFlag)
{
   DPRINT1(-10,"Queue SAMP: Action: %d, loc: %lu \n",sample2ChgTo);
   if (BrdType == MASTER_BRD_TYPE_ID)            // master controller
   {
      rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
      rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
      rngBufPut(pSyncActionArgs,(char*) &spinActive,sizeof(spinActive));
      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(bumpFlag));

      signal2_syncop(LOADSAMP);    // signal & stop fifo
      SystemSync(320,NOPREP);            // when done start all alike

#ifdef XXXXXX
   signal_syncop(action | 0x80000,-1L,-1L);  /* action: LOADSAMP, signal & stop fifo */
#endif

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();
      DPRINT(-10,"==========>  LoadSamp:  Suspend Parser\n");

#ifdef INSTRUMENT
   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);

   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }

}

tstLoadSample()
{
   int taskId;
   int taskPriority;
   int sample2ChgTo, skipsampdetect, bumpflag, spinActive;
   taskId = taskIdSelf();
   taskPriorityGet(taskId, &taskPriority);
   DPRINT2(-1,"taskId: 0x%x, taskPriority = %d\n",taskId, taskPriority);

   taskPrioritySet(taskId,115);
   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,115);

  DPRINT(-1,"calling queueSample\n");
  sample2ChgTo = 10;
  skipsampdetect = 0;
  bumpflag = 0;
  spinActive = 1;
  queueLoadSamp(LOADSAMP,sample2ChgTo,skipsampdetect,pTheAcodeObject, spinActive,bumpflag);
  DPRINT(-1,"Returned\n");

  taskPrioritySet(taskId,taskPriority);
}

void clearSystemSyncFlag()
{
   startOnSyncFlag = 0;
}

complain()
{
  errLogRet(LOGIT,debugInfo,"complain(): error\n");
}

sndParser(char *label, int numAcodes, int numTable,int fid)
{
     PARSER_MSG pCmd;
     DPRINT3(1,"APARSER, BaseLable: '%s', nAcodes: %d, NTables: %d\n",label,numAcodes,numTable);
     pCmd.parser_cmd = APARSER;
     pCmd.NumAcodes = numAcodes;
     pCmd.NumTables = numTable;
     pCmd.startFID = fid;
     strncpy(pCmd.AcqBaseBufName,label,32);
     msgQSend(pMsgesToAParser,(char*) &pCmd, sizeof(PARSER_MSG), NO_WAIT, MSG_PRI_NORMAL);
}

sparser()
{
     pMsgesToAParser = msgQCreate(10, sizeof(PARSER_MSG), MSG_Q_PRIORITY);
     startParser(110, 0, 8192);
}

tstparser()
{
   int decoder(unsigned long *buffer, int size);
   initNDDS(0);
   initDownld();
   initFifo();
   pMsgesToAParser = msgQCreate(10, sizeof(PARSER_MSG), MSG_Q_PRIORITY);
   startParser(110, 0, 1024*8);
   cntrlFifoBufSetDecoder(pCntrlFifoBuf,(CNTRLBUF_DECODEFUNC) decoder);
   cntrlFifoBufModeSet(pCntrlFifoBuf,CNTRL_DECODER_MODE);
}

usedecoder()
{
   int decoder(unsigned long *buffer, int size);
   cntrlFifoBufSetDecoder(pCntrlFifoBuf,(CNTRLBUF_DECODEFUNC) decoder);
   cntrlFifoBufModeSet(pCntrlFifoBuf,CNTRL_DECODER_MODE);
}

usefifo()
{
   cntrlFifoBufModeSet(pCntrlFifoBuf,CNTRL_DMA_MODE);
}

getPat(char *label, int pat_index)
{
    int *patternList,patternSize;
    int errorcode;
    strncpy(pTheAcodeObject->id,label,32);
    errorcode =  getPattern(pat_index,&patternList,&patternSize);
    printf("errorcode: %d, patterAddr: 0x%lx, size: %d\n",errorcode,patternList,patternSize);
}
getTbl(char *label,int table_index, int element)
{
   int value;
   int errorcode;
    strncpy(pTheAcodeObject->id,label,32);
   value = *TableElementPntrGet(table_index, element, &errorcode, "test");
   printf("errorcode: %d, value: %d\n",errorcode,value);
}

swInt(int swItrId)
{
   int fifowords[10];
   int num;
   num = fifoEncodeSWItr(swItrId, fifowords);
   writeCntrlFifoBuf(fifowords, num);
   /* haltop */
   writeCntrlFifoWord(encode_RFSetDuration(1,0000));
}

tstSW(int i)
{
    swInt(i);
    flushCntrlFifoRemainingWords();
    taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
    startCntrlFifo();
    wait4CntrlFifoStop();
}


tstItrs(int i, int delay)
{
   int k;
   int fifowords[10000];
   int num = 0;
   delay = (delay < 8) ? 8 : delay;
   for (k=0; k < i; k++)
   {
     num += fifoEncodeSWItr(1, &fifowords[num]);
     num += fifoEncodeDuration(1, delay, &fifowords[num]);
     num += fifoEncodeSWItr(2, &fifowords[num]);
     num += fifoEncodeDuration(1, delay, &fifowords[num]);
     num += fifoEncodeSWItr(3, &fifowords[num]);
     num += fifoEncodeDuration(1, delay, &fifowords[num]);
     num += fifoEncodeSWItr(4, &fifowords[num]);
     num += fifoEncodeDuration(1, delay, &fifowords[num]);
   }
   writeCntrlFifoBuf(fifowords, num);
   writeCntrlFifoWord(encode_RFSetDuration(1,0000));
   flushCntrlFifoRemainingWords();
   taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
   startCntrlFifo();
   /* wait4CntrlFifoStop(); */
}


int decoder(unsigned long *buffer, int size)
{
   int i;
   for(i=0; i < size; i++)
   {
      diagPrint(NULL,"  decoder(): FifoWord[%d]: 0x%lx\n",i,buffer[i]);
   }
}

prtfifowrds()
{
  int wordsInDataFifo,wordsInInstrFifo;
  wordsInDataFifo = cntrlDataFifoCount();
  wordsInInstrFifo = cntrlInstrFifoCount();
  diagPrint(NULL," Cntrl Fifo Counts:  Instructions: %d, Data: %d\n",wordsInInstrFifo,wordsInDataFifo);
}

clrfifodur()
{
   cntrlFifoCumulativeDurationClear();
}
prtfifodur()
{
   long long duration;
   cntrlFifoCumulativeDurationGet(&duration);
   diagPrint(NULL," Cntrl FIFO Duration (12.5ns ticks): %llu, %lf us \n",duration, (((double) duration) * .0125));
}
prtcfifo()
{
   long long duration;
   int wordsInDataFifo,wordsInInstrFifo;

   wordsInDataFifo = cntrlDataFifoCount();
   wordsInInstrFifo = cntrlInstrFifoCount();
   printf("fifo running: %d\n",cntrlFifoRunning());
   printf("Fifo: instruction count: %d, data count: %ld\n",wordsInInstrFifo,wordsInDataFifo);
   cntrlFifoCumulativeDurationGet(&duration);
   printf("Duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   printf("DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   printf("dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));
}

tstmsync()
{
    int instrwords[10];
    int len = 0;
    /* addPendingSWItr(SENDSYNC); */
    SystemSync(320 /* 100 nsec */, NOPREP);
    len += fifoEncodeSWItr(1, &instrwords[len]);
    instrwords[len++] = encode_RFSetDuration(0,0); /* Haltop */
    instrwords[len++] = encode_RFSetGates(1,0xfff,0);
    writeCntrlFifoBuf(instrwords,len);
    flushCntrlFifoRemainingWords();
    taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
    startCntrlFifo();
    wait4CntrlFifoStop();

}
tstssync()
{
    int instrwords[10];
    int len = 0;
    /* addPendingSWItr(WAIT4ISYNC); */
    SystemSync(8 /* 100 nsec */,NOPREP);
    len = fifoEncodeSWItr(2, &instrwords[len]);
    instrwords[len++] = encode_RFSetDuration(0,0); /* Haltop */
    instrwords[len++] = encode_RFSetGates(1,0xfff,0);
    writeCntrlFifoBuf(instrwords,len);
    flushCntrlFifoRemainingWords();
    taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
    cntrlFifoStartAndSync();
    wait4CntrlFifoStop();
}

static int *pPatternAddr = NULL;    /* for testing of the sendCntrlFifoList() function */
int *buildpattern( int Kwordsize)
{
   long *pPattern;
   long k;
   pPatternAddr = (int*) malloc(Kwordsize * 4 * 1024);
   pPattern = (long *) pPatternAddr;
   for(k=0; k < Kwordsize*1024; k++)
      fifoEncodeDuration(1, 80,&(pPattern[k]));    /* 1usec duration */
   return pPatternAddr;
}
delpattern()
{
    if (pPatternAddr != NULL)
	free(pPatternAddr);
    pPatternAddr = NULL;
    return 0;
}
/* test A32Bridge functions to start FIFO if DMA a pattern is going to pend task */
tstdmaxfer(int looper_max, int size)
{
   FIFOBUF_ID pfifoBuf;
   long instrwords[40];
   long haltop;
   int i,dmaChannel;
   int loop_count;
   long long duration;

   int taskPriority;
   int taskId;

   if (pPatternAddr == NULL)
   {
         printf("build pattern 1st, buildpattern( int Kwordsize)\n");
   }
   taskId = taskIdSelf();
   taskPriorityGet(taskId, &taskPriority);
   DPRINT2(-1,"taskId: 0x%x, taskPriority = %d\n",taskId, taskPriority);

   taskPrioritySet(taskId,60);
   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,60);

   fifoStarted4Exp = 0;
   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   sendCntrlFifoList((int*) pPatternAddr, size, looper_max);
   fifoEncodeDuration(1, 0, &haltop);
   writeCntrlFifoWord(haltop);
   flushCntrlFifoRemainingWords();
   taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
   cntrlFifoStart(); /* start_FIFO();   */

   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,taskPriority);
   taskPrioritySet(taskId,taskPriority);

   cntrlFifoWait4StopItrp();
   cntrlFifoCumulativeDurationGet(&duration);
   printf("Fifo duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   return 0;
}

/* test A32Bridge functions to start FIFO if DMA a pattern is going to pend task */
tstbufstrt(int nbufs, int size)
{
   FIFOBUF_ID pfifoBuf;
   long instrwords[40];
   long haltop;
   int i,dmaChannel;
   int loop_count;
   long long duration;

   int taskPriority;
   int taskId;

   if (pPatternAddr == NULL)
   {
         printf("build pattern 1st, buildpattern( int Kwordsize)\n");
   }
   taskId = taskIdSelf();
   taskPriorityGet(taskId, &taskPriority);
   DPRINT2(-1,"taskId: 0x%x, taskPriority = %d\n",taskId, taskPriority);

   taskPrioritySet(taskId,60);
   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,60);

   fifoStarted4Exp = 0;
   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   for (i=0; i < nbufs; i ++)
   {
      cntrlFifoBufPut(pCntrlFifoBuf,(unsigned long*)  pPatternAddr, size);
      flushCntrlFifoRemainingWords();
   }

   fifoEncodeDuration(1, 0, &haltop);
   writeCntrlFifoWord(haltop);
   flushCntrlFifoRemainingWords();
   taskDelay(calcSysClkTicks(1000));  /* 1000msec, 1 sec */
   cntrlFifoStart(); /* start_FIFO();   */

   DPRINT2(-1,"taskPrioritSet(0x%x,%d)\n",taskId,taskPriority);
   taskPrioritySet(taskId,taskPriority);

   cntrlFifoWait4StopItrp();
   cntrlFifoCumulativeDurationGet(&duration);
   printf("Fifo duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   return 0;
}

/*========================================================*/
/* VT stuff
/*========================================================*/
queueSetVT(unsigned int *p2CurrentWord, ACODE_ID pAcodeId)
{
   int vttype, pid, temp, tmpoff, tmpintlk;
   vttype   = *p2CurrentWord++; // Highland or None
   pid      = *p2CurrentWord++; // PID values
   temp     = *p2CurrentWord++; // requested temp or off
   tmpoff   = *p2CurrentWord++; // temp below which we cool gas
   tmpintlk = *p2CurrentWord++; // error, warning, ignore
   DPRINT5(-1,"SETVT: type=%d pid=%d temp=%d tmpoff=%d intlk=%d\n",
                                  vttype, pid,temp,tmpoff, tmpintlk);

   if (vttype == 0)
   {
      setVTtype(vttype);
      return;
   }
   if (BrdType == MASTER_BRD_TYPE_ID)            // master controller
   {
      /* place the arguments for setVT in ring buffer */
      /* In masterSyncAction we will get them out */
      rngBufPut(pSyncActionArgs,(char*) &vttype,  sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &pid,     sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &temp,    sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &tmpoff,  sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &tmpintlk,sizeof(int));
      signal2_syncop(SETVT);    // signal & stop fifo
      SystemSync(320,NOPREP);            // when done start all alike

      DPRINT(-1,"==========>  SetVT: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}

queueWait4VT(unsigned int *p2CurrentWord, ACODE_ID pAcodeId)
{
   int timeout, tmpintlk;
   tmpintlk = *p2CurrentWord++; // error, warning, ignore
   timeout   = *p2CurrentWord++; // Highland or None
   DPRINT2(-1,"WAIT4VT: intlk=%d, timeout=%d\n",
                                  tmpintlk, timeout);
   if (BrdType == MASTER_BRD_TYPE_ID)            // master controller
   {
      /* place the arguments for setVT in ring buffer */
      /* In masterSyncAction we will get them out */
      rngBufPut(pSyncActionArgs,(char*) &tmpintlk, sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &timeout,  sizeof(int));
      signal2_syncop(WAIT4VT);    // signal & stop fifo
      SystemSync(320,NOPREP);            // when done start all alike

      DPRINT(-10,"==========>  Wait4VT: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}

/*========================================================*/
/* Spin stuff
/*========================================================*/
queueSetSpin(unsigned int *p2CurrentWord, ACODE_ID pAcodeId)
{
   int speed, spinner, bumpflag;
   speed    = *p2CurrentWord++; // Spinner speed
   spinner  = *p2CurrentWord++; // spinner type
   bumpflag = *p2CurrentWord++; // can we bump the sample?
   DPRINT3(1,"SETSPIN: speed: %d, spinner: %d, bumpflag: %d\n",
				speed,spinner,bumpflag);
   if (BrdType == MASTER_BRD_TYPE_ID)             // master controller
   {
      /* place the arguments for setSpin in ring buffer */
      /* In masterSyncAction we will get them out */
      rngBufPut(pSyncActionArgs,(char*) &speed,    sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &spinner,  sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &bumpflag, sizeof(int));
      signal2_syncop(SETSPIN);   // signal & stop fifo
      SystemSync(320,NOPREP);             // when done start all alike

      DPRINT(-10,"==========>  SetSpin: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}

queueCheckSpin(unsigned int *p2CurrentWord, ACODE_ID pAcodeId)
{
   int delta, interlk, bumpflag;
   delta    = *p2CurrentWord++; // spinner max error
   interlk  = *p2CurrentWord++; // HARD_ERROR or WARNING_MSG
   bumpflag = *p2CurrentWord++; // can we bump the sample?
   DPRINT3(1,"CHECKSPIN: delta: %d, interlk: %d, bumpflag: %d\n",
				delta,interlk,bumpflag);
   if (BrdType == MASTER_BRD_TYPE_ID)             // master controller
   {
      /* place the arguments for setSpin in ring buffer */
      /* In masterSyncAction we will get them out */
      rngBufPut(pSyncActionArgs,(char*) &delta,    sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &interlk,sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &bumpflag, sizeof(int));
      signal2_syncop(CHECKSPIN); // signal & stop fifo
      SystemSync(320,NOPREP);             // when done start all alike

      DPRINT(-10,"==========>  CheckSpin: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}

/*========================================================*/
/* AutoLock Stuff
/*========================================================*/
queueAutoLock(unsigned int *p2CurrentWord, ACODE_ID pAcodeId)
{
    int lockmode,lpwrmax,lgainmax;

   lockmode = *p2CurrentWord++; // autolock mode
   lpwrmax  = *p2CurrentWord++; // max lock power
   lgainmax = *p2CurrentWord++; // max lock gain

   DPRINT3(-1,"Queue LOCKAUTO: mode: %d, maxlkpower: %d, maxlkgain: %d\n",
	lockmode,lpwrmax,lgainmax);

   if (BrdType == MASTER_BRD_TYPE_ID)             // master controller
   {
      /* place the arguments for autolock in ring buffer */
      /* In masterSyncAction we will get them out */
      rngBufPut(pSyncActionArgs,(char*) &lockmode,    sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &lpwrmax,sizeof(int));
      rngBufPut(pSyncActionArgs,(char*) &lgainmax, sizeof(int));
      signal2_syncop(LOCKAUTO);   // signal & stop fifo
      SystemSync(320,NOPREP);             // when done start all alike

      DPRINT(-10,"==========>  LOCKAUTO: Suspend Parser\n");

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();

      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}
/*========================================================*/
/* AutoShim
/*========================================================*/
queueAutoShim(ACODE_ID pAcodeId)
{
   DPRINT(1,"Queue AUTOSHIM\n");
   if (BrdType == MASTER_BRD_TYPE_ID)             // master controller
   {
      signal2_syncop(SHIMAUTO);   // signal & stop fifo
      SystemSync(320,NOPREP);             // when done start all alike
      DPRINT(1,"==========>  SHIMAUTO: Suspend Parser\n");
      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startCntrlFifo();
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else                         // all other controlers
   {
      SystemSync(320,NOPREP);            // wait for master
   }
}
/*========================================================*/
/* Read MRI User byte from APbus (reg 2)
/*========================================================*/
queueMRIRead(unsigned int index, int ticks, ACODE_ID pAcodeId)
{
int i, len, remainTicks;
int fifoWords[50];

   DPRINT2( 1,"Queue MRIRead: rt-index: %d ticks=%d\n", index,ticks);

   if (BrdType == MASTER_BRD_TYPE_ID)             // master controller
   {
      /* place the index for MRIRead in ring buffer */
      /* In masterSyncAction we will get it out */
      rngBufPut(pSyncActionArgs, (char *) &index,    sizeof(int));
      rngBufPut(pSyncActionArgs, (char *) &pAcodeId, sizeof(pAcodeId));

      len=0;
      fifoWords[len++] = (LATCHKEY | AUX | (2<<8) | AUX_READ_BIT | 1); // reg 2
      len += fifoEncodeDuration(1, (int) 316,    &fifoWords[len] );
      len += fifoEncodeSWItr(2, &fifoWords[len]);
      remainTicks = ticks - 1280;    // SWItr takes 8 usec, aux read is 50 nsec
      len += fifoEncodeDuration(1, remainTicks, &fifoWords[len] );
      len += fifoEncodeDuration(1, (int)320,    &fifoWords[len] );

      addPendingSWItr(MRIUSERBYTE);

      writeCntrlFifoBuf(fifoWords,len);

      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startFifo4Exp(); /* startCntrlFifo();with xgate stoping & start the FIFO, this could prematurely
                        * restart the fifo resulting in an fifo underflow.  */
      // DPRINT(-10,"==========>  MRI Read: Suspend Parser\n");
      // the ISR will call masterSyncAction where we restart the parser
      MriUserByteParserPriority();  /* raise parser to priority above network */
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
   else
   {
      len = 0;
      if(readuserbyte==0)
         len = fifoEncodeDuration(1, ticks-320, &fifoWords[0] );
      if((readuserbyte&0xff)!=0)
      {
         len = fifoEncodeDuration(1, 3200,       &fifoWords[0] );  //SAS give the master 40 uS to assertWarn
         len += fifoEncodeSWItr(2, &fifoWords[len]); //SAS this supposedly is 640 ticks
         len += fifoEncodeDuration(1, ticks-4160, &fifoWords[len] );
      }
      len += fifoEncodeDuration(1, 320,       &fifoWords[len] );
      if((readuserbyte&0xff)!=0)
      {
         addPendingSWItr(MRIUSERBYTE);
      }
      writeCntrlFifoBuf(fifoWords,len);
      cntrlFifoBufForceRdy(pCntrlFifoBuf);
      startFifo4Exp(); /* startCntrlFifo(); with xgate stoping & start the FIFO, this could prematurely
			* restart the fifo resulting in an fifo underflow.  */

      // DPRINT(-1,"==========>  MRI Read: Suspend Parser\n");
      // when we receive the CNTLR_RTVAR_UPDATE from master we restart parser
      MriUserByteParserPriority();  /* raise parser to priority above network */
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   }
}
