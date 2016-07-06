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

#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include "logMsgLib.h"

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "mBufferLib.h"
#include "nameClBufs.h"
#include "cntrlFifoBufObj.h"
#include "dataObj.h"
#include "AParser.h"


/*
   System Globals 
*/

/* NDDS */
NDDS_ID NDDS_Domain;

#define HOSTNAME_SIZE 80

/* global hostname used for generating NDDS topics
  and other tests 
*/
char hostName[HOSTNAME_SIZE];
 
/*  cluster sizes and number for downlink */
MBUF_DESC mbClTbl [] =
   {
      { 256,      4,      0,      0},
      { 512,     50,      0,      0},
      { 1024,    50,      0,      0},
      { 2048,    50,      0,      0},
      { 8192,    25,      0,      0},
      { 16384,    25,      0,      0}
  };

int NumClustersIn_mbClTbl = sizeof(mbClTbl) / sizeof(MBUF_DESC);

/* main cluster buffer allocation for the 'name buffers' */
MBUFFER_ID mbufID ;

/* the 'name buffers' based on the cluster buffers */
NCLB_ID nClBufId;
 
int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */


/* Debuggging level, greater the number the greater the diagnostic output */
int DebugLevel;
MSG_Q_ID pMsgLogQ = NULL;     /* logMsgTask msgQ */

ACODE_ID pTheAcodeObject; /* Acode object */

SEM_ID pSemOK2Tune = NULL;        /* Mutex to control when tuning is allowed */
MSG_Q_ID pMsgesToAParser = NULL;  /* MsgQ used for Msges to Acode Parser */
MSG_Q_ID pMsgesToPHandlr = NULL;  /* MsgQ for Msges to Problem Handler */
MSG_Q_ID pDataTagMsgQ = NULL;     /* dspDataXfer task  -> dataPublisher task */
SEM_ID pPrepSem = NULL;        /* Semaphore for Imaging Prep. */
int    prepflag;                 /* Imaging prep flag used in shandler, SystemSyncUp() */

MSG_Q_ID pFifoSwItrMsgQ = NULL;	  /* MsgQ for shandler */
RING_ID  pPending_SW_Itrs = NULL; /* SW interrupt Type/Usage for shandler */
RING_ID  pSyncActionArgs = NULL;  /* shandler function arguments ring buffer */

FIFOBUF_ID pCntrlFifoBuf = NULL;  /* PS Timing Control FIFO Buffer Object */
int cntrlFifoDmaChan;             /* device paced DMA channel for PS control FIFO */

/*  FID data upload globals  */
MSG_Q_ID pDspDataRdyMsgQ = NULL;    /* DSP -> dspDataXfer task */
DATAOBJ_ID pTheDataObject = NULL;   /* FID statblock, etc. */

/* SA related globals */
SEM_ID  pSemSAStop;   /* Binary  Semaphore used to Stop upLinker for SA */
int     SA_Criteria;  /* criteria for SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
unsigned long SA_Mod;  /* modulo criteria for SA, ie which fid to stop at 'il'*/
unsigned long SA_CTs;  /* Completed Transients for SA */

/* Sample Change globals */
int sampleHasChanged = 0;	/* Global Sample Change Flag */

/* Fail and Warning Interrupt flags */
int failAsserted = 0;
int warningAsserted= 0;
int readuserbyte= 0;

/* Safe State Parameters */
int SafeGateVal = 0;		/* Gate seting for abort or end of experiment */
int SafeXAmpVal = 0;		/* PFG or Gradient X Amp settings */ 
int SafeYAmpVal = 0;		/* PFG or Gradient Y Amp settings */ 
int SafeZAmpVal = 0;		/* PFG or Gradient Z Amp settings */ 
int SafeB0AmpVal = 0;		/* Gradient B0 Amp settings */ 
int SafeXEccVal = 0;		/* Gradient X ECC settings */ 
int SafeYEccVal = 0;		/* Gradient Y ECC settings */ 
int SafeZEccVal = 0;		/* Gradient Z ECC settings */ 
int SafeB0EccVal = 0;		/* Gradient B0 ECC settings */ 
/* there are various other AUX & SPI devices but at present it has been decided that
 * these devices are best left alone 
 * safeAUXVal = 0;
 * safeSPIVal = 0;
 */

int enableSpyFlag = 0;  /* if > 0 then invoke the spy routines to monitor CPU usage  11/9/05 GMB */

#ifdef  INOVA_EXAMPLES
*____________________________________________________________________________
*EXCEPTION_MSGE HardErrorException;
*EXCEPTION_MSGE GenericException;

*MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
*MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */
*MSG_Q_ID pMsgesToAupdt;/* MsgQ used for Msges to Acode Updater */
*MSG_Q_ID pMsgesToXParser;/* MsgQ used for Msges to Xcode Parser */
*MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
*MSG_Q_ID pTagFifoMsgQ;	/* MsgQ for Tag Fifo */
*MSG_Q_ID pApFifoMsgQ;     /* MsgQ for AP Fifo ReadBack */
*MSG_Q_ID pUpLinkIMsgQ;	/* MsgQ used between UpLinker and STM Object */
*MSG_Q_ID pUpLnkHookMsgQ; /* MsgQ used between a Data process and STM Object */
*MSG_Q_ID  pIntrpQ;	/* for testing purposes */
*DLB_ID   pDlbDynBufs;   /* Dynamic Named Buffers */
*DLB_ID   pDlbFixBufs;   /* Fast Fix Size Named Buffers */
*RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function arguments */

*STATUS_BLOCK currentStatBlock;		/* Acqstat-like status block */

/* Revision Number for Interpreter & System */
____________________________________________________________________________
#endif

#ifndef NIRVANA_SYSTEM_REV
#define NIRVANA_SYSTEM_REV 1
#define NIRVANA_INTERP_REV 1
#endif

int SystemRevId = NIRVANA_SYSTEM_REV;
int InterpRevId = NIRVANA_INTERP_REV;

