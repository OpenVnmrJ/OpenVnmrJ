/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* globals.c 11.1 07/09/07 - Global Parameters */
/* 
 */

#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "adcObj.h"
#include "autoObj.h"
#include "sibObj.h"
#include "namebufs.h"
#include "AParser.h"
#include "REV_NUMS.h"

/*
   System Globals 
*/

EXCEPTION_MSGE HardErrorException;
EXCEPTION_MSGE GenericException;

FIFO_ID   pTheFifoObject; /* FIFO Object */
STMOBJ_ID pTheStmObject;/* STM Object */
ADC_ID   pTheAdcObject; /* ADC Object */
ACODE_ID pTheAcodeObject; /* Acode object */
AUTO_ID  pTheAutoObject; /* Automation object */
SIB_ID   pTheSibObject; /* Safety Interface Board */

MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */
MSG_Q_ID pMsgesToAParser;/* MsgQ used for Msges to Acode Parser */
MSG_Q_ID pMsgesToAupdt;/* MsgQ used for Msges to Acode Updater */
MSG_Q_ID pMsgesToXParser;/* MsgQ used for Msges to Xcode Parser */
MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
MSG_Q_ID pTagFifoMsgQ;	/* MsgQ for Tag Fifo */
MSG_Q_ID pApFifoMsgQ;     /* MsgQ for AP Fifo ReadBack */
MSG_Q_ID pUpLinkIMsgQ;	/* MsgQ used between UpLinker and STM Object */
MSG_Q_ID pUpLnkHookMsgQ; /* MsgQ used between a Data process and STM Object */
MSG_Q_ID  pIntrpQ;	/* for testing purposes */
DLB_ID   pDlbDynBufs;   /* Dynamic Named Buffers */
DLB_ID   pDlbFixBufs;   /* Fast Fix Size Named Buffers */
RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function arguments */

STATUS_BLOCK currentStatBlock;		/* Acqstat-like status block */

/* SA related globals */
SEM_ID  pSemSAStop;   /* Binary  Semaphore used to Stop upLinker for SA */
int     SA_Criteria;  /* criteria for SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
unsigned long SA_Mod;  /* modulo criteria for SA, ie which fid to stop at 'il'*/
unsigned long SA_CTs;  /* Completed Transients for SA */

/* Sample Change globals */
int sampleHasChanged = 0;	/* Global Sample Change Flag */

/* Revision Number for Interpreter & System */
#ifndef INOVA_SYSTEM_REV
#define INOVA_SYSTEM_REV 0
#define INOVA_INTERP_REV 0
#endif
int SystemRevId = INOVA_SYSTEM_REV;
int InterpRevId = INOVA_INTERP_REV;

int MSRType;	/* Version of MSR (Automation) board  MSRI and MSRII */
