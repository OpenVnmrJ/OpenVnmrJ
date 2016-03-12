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

// #define TIMING_DIAG_ON    /* compile in timing diagnostics */

#define SEND_APARSE_EARLY   // uncomment or comment both defines in AParser.c & A32Interp.c

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
#include <setjmp.h>

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
#include "fifoFuncs.h"
#include "Console_Stat.h"    /* acq states */

#include "taskPriority.h"
#include "sysUtils.h"
#include "Cntlr_Comm.h"

/*---------------*/

extern int enableSpyFlag;  /* if > 0 then invoke the spy routines to monitor CPU usage  11/9/05 GMB */

extern ACODE_ID pTheAcodeObject;
extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */
extern RING_ID  pSyncActionArgs;

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */

static int FifoFd = -1;

Acqparams acqReferenceData;
autodata *autoDP;

/* name buffer handle */
extern NCLB_ID nClBufId;

extern SEM_ID pSemOK2Tune;
extern SEM_ID pPrepSem;

extern MSG_Q_ID pMsgesToAParser;

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */

extern int failAsserted;
extern int warningAsserted;
/* extern int host_abort; */

extern int patternType;    /* LED idle pattern display type, 0=normal idle, 1=exception thrown */

/*********************************************************
*
* Forward Function Defines
*
**********************************************************/

void acodeDelete(ACODE_ID pAcodeId);
ACODE_OBJ *acodeCreate(char *expname);
void APparser();
unsigned int *getAcodeSet(char *expname, int acode_index, unsigned int *size, int timeVal);

int AbortingParserFlag = 0;

int *pGradWfg = NULL;

/* parser priority is raised for MRI User Read Byte function, above tNetTask */
int ParserTaskId = -1;    /* Parser Task Id */
int ParserTaskStdPriority = -1;    /* Parser Task Standard Priority */

int ParseAheadCnt;
int ParseAheadInterval;

extern int nDownldCodes;

/* for interleave (il='y') indicates if all the Acodes are resident in the controller's memory
    (in named buffers) */
int residentFlag = 0;

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

/*
* The following contruct is used to prevent the APaser task from calling NDDS functions directly in it's context.
* If NDDS functions are called within the AParser task context, when the APaser task is restarted, this results in a loss
* of Memory. Memory loss can be substantial resulting in experiments no longer being able to run.
*     GMB 4/10/2012
*/

static MSG_Q_ID pMsgesToParsCom = NULL;

#define COM_ROLLCALL (1)
#define COM_SEND2LOCK2 (2)
#define COM_SEND2ALLCNTLRS (3)
#define COM_SENDEXCEPTION (4)
typedef struct {
                 int com_cmd;
                 int cmd;
                 long  larg1;
                 long  larg2;
                 long  larg3;
                 double  dblarg1;
                 double  dblarg2;
                 char*   strarg;
               }   PARSER_COM_MSG;

// call rollcall in context of the ParseCom task, not AParser task
void SendRollCallViaParsCom() 
{
      PARSER_COM_MSG  parserComMsg;
      if (BrdType != MASTER_BRD_TYPE_ID) // if not a Master then skip this call
         return;
      parserComMsg.com_cmd = COM_ROLLCALL;
      msgQSend(pMsgesToParsCom,(char*) &parserComMsg, sizeof(PARSER_COM_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
}

// call send2Lock in context of the ParseCom task, not AParser task
void Send2LockViaParsCom(int Lk_Cmd, long value, long value2, double arg3, double arg4 ) 
{
      PARSER_COM_MSG  parserComMsg;
      if (BrdType != MASTER_BRD_TYPE_ID) // if not a Master then skip this call
         return;
       parserComMsg.com_cmd = COM_SEND2LOCK2;
       parserComMsg.cmd = Lk_Cmd;
       parserComMsg.larg1 = value;
       parserComMsg.larg2  = value2;
       parserComMsg.dblarg1 = arg3;
       parserComMsg.dblarg2 = arg4;
      msgQSend(pMsgesToParsCom,(char*) &parserComMsg, sizeof(PARSER_COM_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
}

// call send2AllCntlrs in context of the ParseCom task, not AParser task
send2AllCntlrsViaParsCom(int parse_cmd,  long num_acodes,  long num_tables, long cur_acode,  char *Id)
{
    PARSER_COM_MSG  parserComMsg;
    if (BrdType != MASTER_BRD_TYPE_ID) // if not a Master then skip this call
       return;
    parserComMsg.com_cmd = COM_SEND2ALLCNTLRS;
    parserComMsg.cmd = parse_cmd;
    parserComMsg.larg1 = num_acodes;
    parserComMsg.larg2 = num_tables;
    parserComMsg.larg3 = cur_acode;
    parserComMsg.strarg = Id;
    
    msgQSend(pMsgesToParsCom,(char*) &parserComMsg, sizeof(PARSER_COM_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
}


// call sendException in context of the ParseCom task, not AParser task
sendExceptionViaParsCom(int error, int errorcode,  int d1, int d2, char *str)
{
    PARSER_COM_MSG  parserComMsg;
    parserComMsg.com_cmd = COM_SENDEXCEPTION;
    parserComMsg.cmd = error;
    parserComMsg.larg1 = errorcode;
    parserComMsg.larg2 = d1;
    parserComMsg.larg3 = d2;
    parserComMsg.strarg = str;
    
    msgQSend(pMsgesToParsCom,(char*) &parserComMsg, sizeof(PARSER_COM_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
}

// separate task to handles AParser NDDS communication needs, which are small
void ParserCom()
{
    union {
      long long lword;
      double dword;
      int  nword[2];
     }  encoder1,encoder2;

   PARSER_COM_MSG  parserComMsg;
   int ival;
   // ParsComTaskId = taskIdSelf();
   FOREVER
   {
     ival = msgQReceive(pMsgesToParsCom,(char*) &parserComMsg,sizeof(PARSER_COM_MSG),WAIT_FOREVER);
     if (ival == ERROR)
     {
	      errLogSysRet(LOGIT,debugInfo,"PARSER COM ERROR");
	      return;
     }
     else
     {
       switch(parserComMsg.com_cmd)
       {
          case COM_ROLLCALL:  
               rollcall();
               DPRINT(1,"ParserCom: envoking rollcall\n");
		          break;
          case COM_SEND2LOCK2:
              // send2Lock(LK_SET_PHASE, *p2CurrentWord++, 0, 0.0, 0.0);
              // send2Lock(parserComMsg.cmd, parserComMsg.larg1, parserComMsg.larg2, parserComMsg.dblarg1, parserComMsg.dblarg2);
              encoder1.dword = parserComMsg.dblarg1;
              encoder2.dword = parserComMsg.dblarg2;
              execFunc("send2LockEncoded",(int*) parserComMsg.cmd, (int*) parserComMsg.larg1,(int*) parserComMsg.larg2, 
                           (int*) encoder1.nword[0], (int*) encoder1.nword[1], (int*) encoder2.nword[0], (int*) encoder2.nword[1], NULL);
              DPRINT5(1,"ParserCom: send2Lock(%d,%d,%d,%f,%f)\n",
                parserComMsg.cmd, parserComMsg.larg1, parserComMsg.larg2, parserComMsg.dblarg1, parserComMsg.dblarg2);
		        break;
          case COM_SEND2ALLCNTLRS:
               send2AllCntlrs(parserComMsg.cmd, parserComMsg.larg1,parserComMsg.larg2,parserComMsg.larg3,parserComMsg.strarg);
              DPRINT5(1,"ParserCom: send2AllCntlrs(%d,%d,%d,%d,'%s')\n",
                       parserComMsg.cmd, parserComMsg.larg1,parserComMsg.larg2,parserComMsg.larg3,parserComMsg.strarg);
		        break;
          case COM_SENDEXCEPTION:
               sendException(parserComMsg.cmd, parserComMsg.larg1, parserComMsg.larg2, parserComMsg.larg3, parserComMsg.strarg);
               DPRINT2(1,"ParserCom: sendException(%d,%d,0,0,'NULL)\n", parserComMsg.cmd, parserComMsg.larg1);
		        break;

        default:

           break;
       }
     }
  }
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

/*
* create the Acode object and then start the parser task
*
*/
startParser(int priority, int taskoptions, int stacksize)
{
   if (pTheAcodeObject != NULL)
      acodeDelete(pTheAcodeObject);
   else
      pTheAcodeObject = acodeCreate("");

    /* create the pub issue  Mutual Exclusion semaphore */
   if (BrdType == MASTER_BRD_TYPE_ID)
       pSemOK2Tune = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
   else
       pSemOK2Tune = NULL;

   if (BrdType == MASTER_BRD_TYPE_ID)
       pPrepSem = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
   else
       pPrepSem = NULL;

    /* pSemOK2Tune = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE); */

   pSyncActionArgs = rngCreate(1000 * sizeof(long));

   if ( (BrdType == GRAD_BRD_TYPE_ID) || (BrdType == PFG_BRD_TYPE_ID) )
   {
      pGradWfg = (int *) malloc(1024 * sizeof(int));
   }

   AbortingParserFlag = 0;

   ParserTaskStdPriority = priority;

   if ( pMsgesToParsCom == NULL)
      pMsgesToParsCom = msgQCreate(10, sizeof(PARSER_COM_MSG), MSG_Q_PRIORITY);

   if (taskNameToId("tParsCom") == ERROR)
      taskSpawn("tParsCom",priority-1,taskoptions,stacksize,ParserCom,
		      pMsgesToParsCom,2,3,4,5,6,7,8,9,10);

   if (taskNameToId("tParser") == ERROR)
    taskSpawn("tParser",priority,taskoptions,stacksize,APparser,
		  pMsgesToAParser,2,3,4,5,6,7,8,9,10);
}

/* set parser to a high priority 
 * called from the A32BrigdeFUncs.c for MRI read UserByte 
 */
MriUserByteParserPriority()
{
    wvEvent(EVENT_PARSER_PRIOR_RAISED,NULL,NULL);
    taskPrioritySet(ParserTaskId,MRIUSERBYTE_APARSER_PRIORITY);
}

/* reset parser back to it's standard priority 
 * called from the phandlers for errors, etc. 
 * in case parser was left at higher priority from aborted MRI read User Byte
 */
resetParserPriority()
{
    wvEvent(EVENT_PARSER_PRIOR_LOWER,NULL,NULL);
    taskPrioritySet(ParserTaskId,ParserTaskStdPriority);
}


killParser()
{
   int tid;

   markBusy(APARSER_FLAGBIT);
   if ((tid = taskNameToId("tParser")) != ERROR)
	taskDelete(tid);
}

/*********************************************************
*
* AParser.c - the ACODE Parser Task 
*  Parser Task that runs forever waiting on messages 
*
**********************************************************/
void APparser()
{
   PARSER_MSG  parserMsg;
   int ival;
   int errorcode;
   char  expid[EXP_BASE_NAME_SIZE+1];

   ParseAheadCnt = PARSE_AHEAD_COUNT;
   ParseAheadInterval = PARSE_AHEAD_ITR_INTERVAL;

   ParserTaskId = taskIdSelf();

   FOREVER
   {
#ifdef INSTRUMENT
	wvEvent(EVENT_PARSE_READY,NULL,NULL);
#endif
     /* setjump(jmpBuf); */

     markReady(APARSER_FLAGBIT);

     ival = msgQReceive(pMsgesToAParser,(char*) &parserMsg,sizeof(PARSER_MSG),WAIT_FOREVER);

     
#ifdef INSTRUMENT
	wvEvent(EVENT_PARSE_BUSY,NULL,NULL);
#endif
     markBusy(APARSER_FLAGBIT);
     if (ival == ERROR)
     {
	printf("PARSER Q ERROR\n");
	errLogSysRet(LOGIT,debugInfo,"PARSER Q ERROR");
	return;
     }
     else
     {
       switch(parserMsg.parser_cmd)
       {
/*
          case PARSE_ACODE:  
                errorcode = APstart(&parserMsg, 0);
		  break;
          case ACQI_PARSE:
                  errorcode = APstart(cmdstring, ACQI_INTERACTIVE);
		  break;
*/
        default:
               if (enableSpyFlag > 0)
               {
                 spyClkStart(200);  /* turn on spy clock interrupts, etc. sample 200 times/sec  */
               }

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("Recvd APARSE, init rollcall:");
#endif
                // SystemRollCall();
               SendRollCallViaParsCom() ; // if not a Master then this routine just returns doing nothing.

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("Recvd APARSE, cmplt rollcall:");
#endif

		/* taskDelay(60*5); /* delay to allow code to download */
	        ClearCtlrStates();
                ledOff();
                panelLedAllOff();

                 /* take mutex controlling tuing permission */
                 /* do not allow tuning during an experiment */
                if (pSemOK2Tune != NULL)  /* only not NULL for master */
		   semTake(pSemOK2Tune,WAIT_FOREVER);

                AbortingParserFlag = failAsserted = warningAsserted = 0;  /* host_abort at sometime too */

#ifdef SEND_APARSE_EARLY
               /* previously APARSE was sent to all the other controller within the Acode ROLLCALL (A32Interp.c)
                * this delayed the start of parsing by the other controller by ~140 msec delaying the start (bug 4420)
                * Now that rollcall is performed here as well, we take the oprotunity to send the message much earlier
                * in the process ( now delay is just ~ 6 msec )  [ all time can vary depending on sequence ] 
                * It appears to safe to do this here, being below the ClearCtlrStates() call. otherwise a race contintion would
                * exist.  We ifdef this just incase problems are run into, an easy reverting back to the old is possible
                *    GMB   8/6/2010
                */
               if (BrdType == MASTER_BRD_TYPE_ID)
               {
                  strncpy(expid,parserMsg.AcqBaseBufName,EXP_BASE_NAME_SIZE);
                  // this needs to be after ClearCtlrStates() above.... GMB 8/5/2010
                  DPRINT4(1,"send2AllCntlrsViaParsCom: CNTLR_CMD_APARSER, numAcodes: %d, nTables: %d, startFid: %d, expid: '%s'\n",
                           parserMsg.NumAcodes,parserMsg.NumTables,parserMsg.startFID,expid);
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
                  TSPRINT("Send APARSER to other cntrls:");
#endif
                  //send2AllCntlrs(CNTLR_CMD_APARSER, parserMsg.NumAcodes,
				      //            parserMsg.NumTables,parserMsg.startFID,expid);
                  send2AllCntlrsViaParsCom(CNTLR_CMD_APARSER, parserMsg.NumAcodes,
				                  parserMsg.NumTables,parserMsg.startFID,expid);
               }

               if (BrdType == DDR_BRD_TYPE_ID)
               {
                    resetZeroFidSyncTicks();
               }
#endif   // SEND_APARSE_EARLY

                /* cleanUpLinkMsgQs() is only present on the DDRs, but with execFunc() if it not there 
                   then no harm done */
                /* this is an attempt to avoid problems where for some reason left-over DMA buffers where
                   present so subsquuuit go would get the wrong data and CRC checksum error would be reported
	           by the uplink */

                execFunc("cleanUpLinkMsgQs",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("Recvd APARSE, calling APstart():");
#endif

                errorcode = APstart(&parserMsg, 0);
                 /* give mutex controlling tuing permission */
		/* semGive(pSemOK2Tune);     no given in shandler or phandler */
		  break;
       }
       if (errorcode != 0)
       {
          // sendException(HARD_ERROR, errorcode, 0,0,NULL);
          sendExceptionViaParsCom(HARD_ERROR, errorcode, 0,0,NULL);
       }
     }
   }
}

/*******************************************************
* Initial Acode object structure and call interperter
*  
********************************************************/
int APstart(PARSER_MSG *cmds, int interactive_flag)
{
   char *cptr1,*cptr2;
   int num_acode_sets, cur_acode_set, num_tables, index, tmp, status;
   int i,retrys;
   NCLBP active;
   TBL_ID tbl_ptr;
   int tbl_index;
   char expname[64];
   int errorcode;
   int num2wait4, numCodesRecvd;
   int timeout;
   int freeBufs;
   extern int downldTimeOut;

/*
        PARSER_MSG
        int parser_cmd;
	unsigned long NumAcodes;
	unsigned long NumTables;
	unsigned long startFID;
        int      spare1;
	char AcqBaseBufName[32];
*/

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("APstart(): Entering routine:");
#endif
    patternType = 0;  /* preset idle pattern */
    haltLedShow();   /* stop idle LED pattern display */
    setAcqState(ACQ_ACQUIRE);   /* status update to Vnmrj */

   /* Parse Command -  Get exp name 	*/
   strncpy(expname,cmds->AcqBaseBufName,64);

   /* Set default argument values */
   num_acode_sets = 1;
   num_tables = 0;
   cur_acode_set = 0;

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("APstart(): cmplt LED stop, init parms:");
#endif

   /* make sure Imaging Prep Sem is empty to start */
   while (semTake(pPrepSem,NO_WAIT) != ERROR);	


   /* Copy Experiment Name */
   strncpy(pTheAcodeObject->id,expname,EXP_BASE_NAME_SIZE); 

   /**** Setup Acodes   *****/
   /* num_acode_sets maybe used to indicate number of Fid to be acquired (ddr_init.c), Beware */
   pTheAcodeObject->num_acode_sets = cmds->NumAcodes;
   pTheAcodeObject->cur_acode_set = cmds->startFID;
   pTheAcodeObject->num_tables = cmds->NumTables;

   DPRINT4(-1,"APstart()-  ExpName: '%s'  Num Acodes: %lu Num Tables: %lu, startFid: %d\n",
		pTheAcodeObject->id,pTheAcodeObject->num_acode_sets,
                pTheAcodeObject->num_tables,pTheAcodeObject->cur_acode_set);

   /* parse ahead is 32 for twice that is good, plus the lc & init ps adddes 2 more, gives 66 */
   /* num2wait4 = (cmds->NumAcodes < 66) ?  cmds->NumAcodes + 1 : 64 + 1; */
   /* Now wait4NumDownLoad count only the Acode download so we can use NumAcodes directly */
   /* NumAcodes = 0 for SU but does have an acode so we add one to the NumAcodes */

   /* num2wait4 = (cmds->NumAcodes < 65) ?  cmds->NumAcodes: 64; */
   num2wait4 = (cmds->NumAcodes < PARSE_AHEAD_COUNT+1) ? cmds->NumAcodes: PARSE_AHEAD_COUNT;

   DPRINT1(-1,"APstart(): Waiting for at least %d codes to be received\n",num2wait4);
   /* if ((numCodesRecvd = wait4NumDownLoad(num2wait4, 10)) == -1) /* 10 times at, once per 130 msec, gives ~1 sec */
   /* downldTimeOut is sent by sendproc but might be late so we make sure we have at least a proper timeout */
   if (downldTimeOut > 8)
       timeout = downldTimeOut;
   else
       timeout = 45;  /* default max timeout */
   if ((numCodesRecvd = wait4NumDownLoad(num2wait4, timeout)) == -1) /* 50 times at, once per 130 msec, gives ~6 sec */
   {
         downldCntShow();
         errorcode = SYSTEMERROR+NOACODESET;
	 return(errorcode);
   }
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("APstart(): req. Acodes recv, nClbFreeBufs start ");
#endif
   freeBufs = nClbFreeBufs(nClBufId);
   DPRINT3(-1,"CodesDwnLded: %d, freeBufs: %d, total Acodes: %d\n", nDownldCodes, freeBufs,cmds->NumAcodes);
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("APstart(): nClbFreeBufs cmplt ");
#endif
   /* determine if all the pulse seqnce acodes will reside in memory, for il use */
   if (nDownldCodes + freeBufs >= cmds->NumAcodes)
       residentFlag =  1;
   else
       residentFlag = 0;
   DPRINT1(+1,"Acode residentFlag: %d\n",residentFlag);
   downldCntShow();
   DPRINT1(+1,"APstart(): recv'd %d Codes\n",numCodesRecvd);
   /* taskDelay(60*2); /* delay to allow code to download */

   /***** Initialize Exp  *****/

#ifdef NOTYETIMPLEMENTED
   if (pTheTuneObject)
     semTake( pTheTuneObject->pSemAccessFIFO, WAIT_FOREVER );

/*  see comment dated February 5, 1998, in getAcodeSet
    for explanation of why the next line was commented out.	*/

   /*update_acqstate( (interactive_flag == ACQI_INTERACTIVE) ? ACQ_INTERACTIVE : ACQ_PARSE );*/
   update_acqstate( ACQ_PARSE );
   getstatblock();	/* send statblock up to host now */
#endif

   errorcode = acodeInit(pTheAcodeObject,interactive_flag);
   if (errorcode != 0)
   {
	errLogRet(LOGIT,debugInfo,"APstart: Exp: %s not initialized!",
								expname);
	return(errorcode);
   }


   /*****  Start Parser   *****/
   /* fifoClrStart4Exp(); */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("AParser: Begin resets ");
#endif
   cntrlFifoReset();
   resetFifoHoldRegs();
   cntrlFifoReset();   /* just incase there are repeat instruction in the holding registers */
   /* The following preserves the state of RF high speed lines, for, e,g, amp CW bit
    * If amp is in CW mode, the following resetFifoHoldRegs() puts it back into
    * CW mode at the beginning of the experiment (an su would do this). So CW is not
    * toggled into pulsed mode at the beginning of su/go.
    */
   if (BrdType == RF_BRD_TYPE_ID)
      resetFifoHoldRegs();

   /* resetRoboAckSem();  /* reset parser suspending semaphore */
   resetRoboAckMsgQ(); /* clear out sample change Ack msgQ */
   resetParserSem(pTheAcodeObject);  /* reset parser suspending semaphore */
   setFifoOutputSelect(SELECT_FIFO_CONTROLLED_OUTPUT);
   cntrlFifoCumulativeDurationClear();
   clearSystemSyncFlag();
   clearStart4ExpFlag();
   cntrlFifoBufInit(pCntrlFifoBuf);

   /* parseAheadInit(PARSE_AHEAD_COUNT,PARSE_AHEAD_ITR_LOCATION); /* was (32,16), now (64,48) help EPI exp on DDR */
   /* parseAheadInit(PARSE_AHEAD_COUNT,PARSE_AHEAD_ITR_INTERVAL); /* parser ahead 64, parser interval 2 */
   parseAheadInit(ParseAheadCnt,ParseAheadInterval); /* parser ahead 64, default parser interval 4 */

   ShandlerReset();  /* flush the ring buffers */

   /* function only exists on the gradient controller */
   execFunc("enableSlewExceededItr",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("AParser: Start Acode Parser");
#endif
   errorcode = A32_interp(pTheAcodeObject);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
   TSPRINT("AParser: Parser FInished: ");
#endif

   /*****  Cleanup   *****/
   acodeClear(pTheAcodeObject);

   /* fifoCloseLog(); */

   return(errorcode);
}

/********************************************************/
/* Init Acode Object -  				*/
/*	Set up tables, Experiment info structure.	*/
/********************************************************/
acodeInit(ACODE_OBJ *pAcodeObj,int interactive_flag)
{
   unsigned short *ac_cur,*ac_base,*ac_end; /* acode pointers */
   unsigned short *ac_start;
   long *rt_base, *rt_tbl;	/* pointer to realtime buffer addresses */
   short *acode;		/* acode location */
   short alength;	/* length of acode values in short words*/
   int cur_table;
   int index, tmp, not_found;
   int i,retrys;
   int error;
   NCLBP active;
   char tmpbufname[64];
   char numstring[8];


   numstring[0] = '\0';

   /* Get new acode set 6 times pausing second between tests */
   retrys = 5;
   while (retrys > 0)
   {
     /*
     if (Aborted() == TRUE)
     {
        acodeClear(pAcodeObj);
        return(SYSTEMERROR+NOACODESET);
     }
     */
     pAcodeObj->cur_acode_base = pAcodeObj->cur_jump_base =
		getAcodeSet(pAcodeObj->id,pAcodeObj->cur_acode_set, &(pAcodeObj->cur_acode_size), 1);
     if (pAcodeObj->cur_acode_base != NULL)
     {
	   taskDelay(0);	/* startup delay of 1.0 seconds */
	   break;
     }
     retrys--;
     DPRINT1(0,"acodeInit:  retrys: %d\n",retrys);
     /* taskDelay(sysClkRateGet()*1); */
     taskDelay(0);
   }
   if (retrys == 0)
   {
      acodeClear(pAcodeObj);
      return(SYSTEMERROR+NOACODESET);
   }

   /**** Get Named LC buffer  ****/
   error = getLCSet(pAcodeObj->id);
   if (error != 0)
   {
	errLogRet(LOGIT,debugInfo,"acodeInit: Didn't Find: %s",tmpbufname);
        acodeClear(pAcodeObj);
        return(error);
   }
   
   /**** Create Interactive Info. ****/
   pAcodeObj->interactive_flag = interactive_flag;


#ifdef XXXXX
   /**** Get Tables  ****/
   if (num_tables > 0)
   {
	pAcodeObj->table_ptr = (TBL_ID *) 
			malloc((sizeof(pAcodeObj->table_ptr)*num_tables));
	if (pAcodeObj->table_ptr == NULL)
	{
    	   errLogSysRet(LOGIT,debugInfo,
				"acodeInit: Could not Allocate TBL Space:");
    	   return(SYSTEMERROR+NOACODETBL);
	}
	cur_table = 0;
	not_found = FALSE;
	while ((cur_table < num_tables) || not_found)
	{
   	   sprintf(numstring,"t%d",cur_table);
   	   DPRINT1(1,"acodeCreate: exp suffix for table buffer: %s\n",
								numstring);

   	   strcpy(tmpbufname,expname);
   	   strcat(tmpbufname,numstring);
   	   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   	   if (active == NULL)
	   {
		not_found = TRUE;
	   }
	   else 
	   {
		pAcodeObj->table_ptr[cur_table] = (TBL_ID) dlbGetPntr(active);
	   }
	   cur_table++;
	}
	if (not_found)
	{
	   if (cur_table == 0)
	   {
        	acodeClear(pAcodeObj);
		return(SYSTEMERROR+NOACODETBL);
	   }
           else
	   {
	    	/* Fill in pointer array.  All tables may not be sent */
	    	while (cur_table < num_tables)
	    	{
	    	   pAcodeObj->table_ptr[cur_table] = NULL;
		   cur_table++;
	    	}
	   }
	}
   }
#endif

   return(0);

}


/********************************************************/
/* Get Acode Set -  					*/
/********************************************************/
unsigned int *getAcodeSet(char *expname, int acode_index, unsigned int *size, int timeVal)
{
 char tmpbufname[64];
 char *tmpptr;
 unsigned int *ac_base;		/* acode pointer */
 NCLBP active;
 ComboHeader  *infoHeader;


   /* sprintf(tmpbufname,"%sf%u",expname,acode_index); */
   sprintf(tmpbufname,ACODE_FORMAT_STR,expname,acode_index);

   /* Get Named acode buffer */
   DPRINT1(-1,"getAcodeSet: Find acode buffer: %s\n",tmpbufname);
   /* Pend if name buffer not there, timeout in 1 sec */
   active = nClbFindEntryP(nClBufId,tmpbufname,sysClkRateGet() * timeVal);
   if (active == NULL)
   {
	errLogRet(LOGIT,debugInfo,"getAcodeBuf: Didn't Find acode buffer: %s\n",
		   tmpbufname);
        return(NULL);
   }
   tmpptr = (char *) nClbGetPntr(active); 
   infoHeader = (ComboHeader *) tmpptr; 
   ac_base = (unsigned int *) (tmpptr + sizeof(ComboHeader)); 
   
   if ((infoHeader->comboID_and_Number & 0xF0000000)  != ACODEHEADER)   /* is it an acode file? Remove header id?*/
   {
	errLogRet(LOGIT,debugInfo,"getAcodeSet: Wasn't an Acode buffer: %s\n",
		tmpbufname);
	return(NULL);
   }
   if ((infoHeader->comboID_and_Number & 0x0FFFFFFF)  != acode_index)   /* is it an acode file? Remove header id?*/
   {
	errLogRet(LOGIT,debugInfo,"getAcodeSet: Indexs do not match: requested: %d, got: %d\n",
		acode_index,(infoHeader->comboID_and_Number & 0x0FFFFFFF));
	return(NULL);
   }
   *size = infoHeader->sizeInBytes;

   active->status = PARSING;

/*  ACQI was causing problems when it aborted a FID display before the
    buffers were downloaded.  The abort would hang for a noticeable amount
    of time until those buffers were all downloaded.  This delay then
    caused other problems for ACQI.  To remedy this, we do not allow ACQI's
    FID display to stop until these buffers are downloaded, and we note
    this event by changing the acquisition state from PARSE to INTERACTIVE
    when this download completes.	February 5, 1998.		*/
    
#ifdef XXXXX
   if (interactive_flag == ACQI_INTERACTIVE)
   {
	update_acqstate( ACQ_INTERACTIVE );
	getstatblock();                     /* send statblock up to host now */
   }
#endif

   return(ac_base);
}

int getLCSet(ACODE_ID pAcodeId)
{
   char tmpbufname[64];
   char *tmpptr;
   NCLBP active;
   ComboHeader  *infoHeader;

   sprintf(tmpbufname,LC_FORMAT_STR,pAcodeId->id,0);
   active = nClbFindEntryP(nClBufId,tmpbufname,sysClkRateGet() * 1);
   if (active == NULL)
   {
	errLogRet(LOGIT,debugInfo,"getLCSet: Didn't Find: %s",tmpbufname);
        return(SYSTEMERROR+NOACODERTV);
   }
   
   tmpptr = (char *) nClbGetPntr(active); 
   infoHeader = (ComboHeader *) tmpptr; 
   pAcodeId->pLcStruct = (Acqparams *) (tmpptr + sizeof(ComboHeader)); 
   pAcodeId->pAutodataStruct = (autodata *) (tmpptr + sizeof(ComboHeader) + sizeof(Acqparams)); 
   pAcodeId->pLcStruct->autop = pAcodeId->pAutodataStruct;

   return(0);
}

char *getAgPatternName(int pat_index, long long loops, int policy ,char *agPatternName)
{
   sprintf(agPatternName,"%sp%d_%lld_%d",pTheAcodeObject->id,pat_index,loops,policy);
   return(agPatternName);
}

#ifdef USE_AGGREGATE_PATTERN
buildAgPattern(int patid )
{
    NCLBP active;
/*
     pDwnLdBuf = nClbMakeEntry(nClBufId,issue->label,issue->totalBytes,0);
     pDwnLdBuf = nClbMakeEntry(nClBufId,issue->label,issue->totalBytes,0);
     pDwnLdBuf = nClbFindEntry(nClBufId,issue->label);
     pDwnLdBuf->status = READY;
*/
}
/* return pointer to beginning of pattern */
char *getAgPattern(char *AgPatternName, int *size)
{
   NCLBP active;
   char *tmpptr;

   *size = 0;

   active = nClbFindEntry(nClBufId,AgPatternName);
   if (active != NULL)
   {
     tmpptr = (char *) nClbGetPntr(active); 
     *size = *((int *)tmpptr);
     tmpptr += sizeof(int);
   }
   else
     tmpptr = NULL;

  return(tmpptr);
}
#endif


int getPattern(int pat_index,int* *patternList,int *patternSize)
{
   char tmpbufname[64];
   char *tmpptr;
   NCLBP active;
   ComboHeader  *infoHeader;
   int aliasname;

   /* sprintf(tmpbufname,"%sp%u",expname,acode_index); */
   sprintf(tmpbufname,PATTERN_FORMAT_STR,pTheAcodeObject->id,pat_index);

   /* Get Named acode buffer */
   DPRINT1(-1,"getPattern: Find Pattern buffer: %s\n",tmpbufname);
   /* Pend if name buffer not there, timeout in 1 sec */
   active = nClbFindEntry(nClBufId,tmpbufname);
   if (active == NULL)
   {
      /* pattern not found, now look it up in the pattern alias list */

      if (pat_index < 0x7FFFFF)
      {
        aliasname = getPatternAlias(pat_index);
        DPRINT2(+1,"pattern alias for %d is found to be %d\n",pat_index, aliasname);
        
        if ( (aliasname > 0) && (aliasname < 0x7FFFFF) )
            pat_index = aliasname;

        sprintf(tmpbufname,PATTERN_FORMAT_STR,pTheAcodeObject->id,pat_index);
        /* Pend if name buffer not there, timeout in 1 sec */
        active = nClbFindEntry(nClBufId,tmpbufname);
      }
        
      if (active == NULL)
      {
          errLogRet(LOGIT,debugInfo,"getPattern: Didn't Find pattern buffer: %s\n",
		   tmpbufname);
          return(SYSTEMERROR+NOACODEPAT);
      }
   }
   tmpptr = (char *) nClbGetPntr(active); 
   infoHeader = (ComboHeader *) tmpptr; 
   
   if ((infoHeader->comboID_and_Number & 0xF0000000)  != PATTERNHEADER)   /* is it an acode file? Remove header id?*/
   {
	errLogRet(LOGIT,debugInfo,"getPattern: Wasn't a Pattern/WF buffer: %s\n",
		tmpbufname);
        return(SYSTEMERROR+NOACODETBL);
   }
   if ((infoHeader->comboID_and_Number & 0x0FFFFFFF)  != pat_index)   /* is it an acode file? Remove header id?*/
   {
	errLogRet(LOGIT,debugInfo,"getPattern: Indices do not match: requested: %d, got: %d\n",
		pat_index,(infoHeader->comboID_and_Number & 0x0FFFFFFF));
        return(SYSTEMERROR+NOACODETBL);
   }

   *patternList = (int *) (tmpptr + sizeof(ComboHeader)); 
   *patternSize = (infoHeader->sizeInBytes / sizeof(int) );

   return(0);
}


/* return the pattern number that is the same (alias) as the sought pattern */

int getPatternAlias(int new_patindex)
{
   int *patList1, patSize1, temp, i;
 
   /* 0x7FFFFF is a special pattern that is the pattern alias list */

   temp = getPattern(0x7FFFFF, &patList1, &patSize1);

   if (temp == 0)
   {
      for(i=0; i<patSize1; i=i+2)
      {
         if (*(patList1 + i) == new_patindex)
         {
             return( *(patList1+i+1) );
         }
      }
      return(-1);
   }
   else
   {
      errLogRet(LOGIT,debugInfo,"getPatternAlias: Didn't Find pattern alias list\n");
      return(-1);
   }
}



int *TableElementPntrGet(int table_index, int element, int *errorcode, char *emssg)
{
   char tmpbufname[64];
   char *tmpptr;
   NCLBP active;
   ComboHeader  *infoHeader;
   int *tableList;
   int tableSize,autoIncrFlag,divNFactor,modFactor;
   int index;

   *errorcode = 0;
   /* sprintf(tmpbufname,"%sp%u",expname,acode_index); */
   sprintf(tmpbufname,TABLE_FORMAT_STR,pTheAcodeObject->id,table_index);

   /* Get Named acode buffer */
   DPRINT2(+1,"TableElementPntrGet: Find element: %d in table: '%s'\n",element,tmpbufname);
   /* Pend if name buffer not there, timeout in 1 sec */
   active = nClbFindEntry(nClBufId,tmpbufname);
   if (active == NULL)
   {
	errLogRet(LOGIT,debugInfo,"getPattern: Didn't Find table buffer: %s\n",
		   tmpbufname);
        *errorcode = SYSTEMERROR+NOACODETBL;
        return(NULL);
   }
   tmpptr = (char *) nClbGetPntr(active); 
   infoHeader = (ComboHeader *) tmpptr; 
   
   if ((infoHeader->comboID_and_Number & 0xF0000000)  != TABLEHEADER)   /* is it an table file? */
   {
	errLogRet(LOGIT,debugInfo,"getPattern: Wasn't a Table buffer: %s\n",
		tmpbufname);
        *errorcode = SYSTEMERROR+NOACODETBL;
        return(NULL);
   }
   if ((infoHeader->comboID_and_Number & 0x0FFFFFFF)  != table_index)   /* is it the right index ? */
   {
	errLogRet(LOGIT,debugInfo,"getPattern: Indices do not match: requested: %d, got: %d\n",
		table_index,(infoHeader->comboID_and_Number & 0x0FFFFFFF));
        *errorcode = SYSTEMERROR+NOACODETBL;
        return(NULL);
   }

   tableList = (int *) (tmpptr + sizeof(ComboHeader)); 
   tableSize = infoHeader->details.TBL.numberElements;
   autoIncrFlag = infoHeader->details.TBL.auto_inc_flag;
   divNFactor = infoHeader->details.TBL.divn_factor;
   modFactor = 0;

   DPRINT5(+1,"TableElementPntrGet(): Table: '%s', tableList Addr: 0x%lx, tableSize; %d, autoIncrFlag; %d, divNFactor: %d\n",
	tmpbufname,tableList,tableSize,autoIncrFlag,divNFactor);	

   if ( !autoIncrFlag)
   {
      index = element % tableSize;
   }
   else
   {
      /* an autoincrement table does divs then mods and increments. */
      index = infoHeader->details.TBL.autoIndex++;
      index /= divNFactor;
      index /= tableSize;
   }

   DPRINT3(+1,"TableElementPntrGet(): return: &tableList[%d] = 0x%lx, value = %d\n",index,&tableList[index],tableList[index]);
   return(&tableList[index]);
}

givePrepSem()
{
     semGive(pPrepSem);
}
  
/********************************************************/
/* acodeStartFifoWD -  Starts fifo watchdog timer.      */
/* RETURNS:						*/
/*  void						*/
/********************************************************/
void acodeStartFifoWD(ACODE_ID pAcodeId)
{
#ifdef XXX
    if (wdStart(pAcodeId->wdFifoStart,sysClkRateGet()/100, 
				fifoStart,(int) pFifoId) == ERROR)
    {
	DPRINT(0,"acodeStartFifoWD: Error in starting.\n");
    }
    DPRINT(1,"acodeStartFifoWD: Watchdog started.\n");
#endif
}

/********************************************************/
/* acodeCancelFifoWD -  Cancels fifo watchdog timer.    */
/* RETURNS:						*/
/*  void						*/
/********************************************************/
void acodeCancelFifoWD(ACODE_ID pAcodeId)
{
    wdCancel(pAcodeId->wdFifoStart);
    DPRINT(1,"acodeCancelFifoWD: Watchdog Canceled.\n");
}

/********************************************************/
/* Remove Acode Set -  					*/
/********************************************************/
int rmAcodeSet(char *expname, int cur_acode_set)
{
 extern delBufsByName(char *label);
 char tmpbufname[64];
 int  status;

   sprintf(tmpbufname,ACODE_FORMAT_STR,expname,cur_acode_set);
   status = delBufsByName(tmpbufname);
   return(status);
}

/********************************************************/
/* Set Acode buffer as Done 				*/
/********************************************************/
int markAcodeSetDone(char *expname, int cur_acode_set)
{
 char numstring[8];
 char tmpbufname[64];
 NCLBP active;

   numstring[0] = '\0';
   sprintf(numstring,"f%d",cur_acode_set);
   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,numstring);
   DPRINT1(1,"markAcodeSetDone: acode buffer: %s\n",tmpbufname);
   active = nClbFindEntry(nClBufId,tmpbufname);
   if (active != NULL)
     active->status = DONE;
   else
      return(-1);
   return(0);
}
/********************************************************/
/* Remove RTvar Set -  					*/
/********************************************************/
int rmRTvarSet(char *expname)
{
 extern delBufsByName(char *label);
 char numstring[8];
 char tmpbufname[64];
 int stat;

   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"rtv");
   DPRINT1(1,"rmRTvarSet: Free RT Parm buffer: %s\n",tmpbufname);
   stat = delBufsByName(tmpbufname);
   /* stat = nClbFreeByName(nClBufId, tmpbufname); */
   return(stat);
}

/********************************************************/
/* Remove WF Set -  					*/
/********************************************************/
int rmWFSet(char *expname)
{
 extern delBufsByName(char *label);
 char numstring[8];
 char tmpbufname[64];
 int stat;

   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"wf");
   stat = delBufsByName(tmpbufname);
   /* stat = nClbFreeByName(nClBufId, tmpbufname); */
   DPRINT1(1,"rmWFSet: Free WF buffer: %s\n",tmpbufname);
   return(stat);
}

/********************************************************/
/* Remove All Acode Sets -  					*/
/********************************************************/
int rmAcodeSets(char *expname, int start_acode_set, int num_acode_sets)
{
 char numstring[8];
 char tmpbufname[64];
 int  status,i;

   /* Get Named acode buffer */
   sprintf(tmpbufname,"%sf",expname);
   DPRINT1(1,"rmAcodeBuf: Free acode buffer with root name: %s\n",tmpbufname);
   return(delBufsByRootName(tmpbufname));
}

/********************************************************/
/* Remove Tables -  					*/
/********************************************************/
int rmTables(ACODE_ID pAcodeId, char *expname, int num_tables)
{
 extern delBufsByName(char *label);
 char numstring[8];
 char tmpbufname[64];
 int  status, i;
   for (i=0; i<num_tables; i++)
   {
   	/* Get Named acode buffer */
   	sprintf(numstring,"t%d",i);
   	strcpy(tmpbufname,expname);
   	strcat(tmpbufname,numstring);
        status = delBufsByName(tmpbufname);
	/* status = nClbFreeByName(nClBufId, tmpbufname); */
	pAcodeId->table_ptr[i] = NULL;
   	DPRINT1(1,"rmTables: Free Table: %s\n",tmpbufname);
   }
   free(pAcodeId->table_ptr);
   return(status);
}

/********************************************************/
/* Get Waveform Set -  					*/
/********************************************************/
unsigned short
*getWFSet(char *expname)
{
 char tmpbufname[64];
 unsigned short *wf_base;		/* waveform pointer */
 NCLBP active;

   /**** Get Named Waveform buffer  ****/
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"wf");
   active = nClbFindEntry(nClBufId,tmpbufname);
   if (active == NULL)
   {
        DPRINT1(1,"getWFSet: Warning, didn't Find: %s\n",tmpbufname);
	return(NULL);
   }
   
   wf_base = (unsigned short *) nClbGetPntr(active);

   return(wf_base);
}

/********************************************************/
/* acodeClear -  Clears acode object and resources	*/
/*	Set up tables, Experiment info structure.	*/
/* RETURNS:						*/
/*  OK or ERROR						*/
/********************************************************/
int acodeClear(ACODE_ID pAcodeId)
{
   char rootName[64];
   if (pAcodeId != NULL)
   {
       /* delete all buffers start with exp name, e.g. 'exp2' */
       sprintf(rootName,"%s",pAcodeId->id);
       /* delete all named buffers starting with the root name */

       /* remove for test for now  */
        delBufsByRootName(rootName);  
      
#ifdef XXXX
	if (pAcodeId->cur_rtvar_base != NULL)
	{
	   rmRTvarSet(pAcodeId->id);
	   pAcodeId->cur_rtvar_base = NULL;
	}
	if (pAcodeId->num_tables != 0)
	{
	   rmTables(pAcodeId,pAcodeId->id,pAcodeId->num_tables);
	   pAcodeId->num_tables = 0;
	}
	if (pAcodeId->num_acode_sets != 0)
	{
	   rmAcodeSets(pAcodeId->id,pAcodeId->cur_acode_set,
					 pAcodeId->num_acode_sets);
	   pAcodeId->num_acode_sets = 0;
	}
	pAcodeId->cur_acode_base = NULL;

	/* remove any waveform buffers */
	rmWFSet(pAcodeId->id);

#endif

	if (pAcodeId->pAcodeControl != NULL)
           semGive(pAcodeId->pAcodeControl);

        /* Now Attempt to take it when, when it would block that
           is the state we want it in.
        */
#ifdef INSTRUMENT
	wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
        while (semTake(pAcodeId->pSemParseSuspend,NO_WAIT) != ERROR);	

	if (pAcodeId->wdFifoStart != NULL)
	   wdDelete(pAcodeId->wdFifoStart);


	strcpy(pAcodeId->id,"");
	pAcodeId->interactive_flag = 0;
   }
   return(0);
}

resetParserSem(ACODE_ID pAcodeId)
{
 while (semTake(pAcodeId->pSemParseSuspend,NO_WAIT) != ERROR);	
}

cleanParser()
{
  int tid;
  if (pMsgesToAParser != NULL)
  {
    msgQDelete(pMsgesToAParser);
    pMsgesToAParser = NULL;
  }
  tid = taskNameToId("tParser");
  if (tid != ERROR)
    taskDelete(tid);
}

cleanMsgesToAParser()
{
  char xmsge[ CONSOLE_MSGE_SIZE ];
  int numleft;
  if (pMsgesToAParser != NULL)
  {
    numleft = msgQNumMsgs(pMsgesToAParser);
    if (numleft)
    {
       while (msgQReceive(pMsgesToAParser, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, 
						NO_WAIT) > 0)
       {
	   DPRINT(1,"cleanMsgesToAParser: Msg Discarded.\n");
       }
    }
  }
}

acodeResetUpdtSem(ACODE_ID pAcodeId)
{
   if (pAcodeId != NULL)
   {

	if (pAcodeId->pSemParseUpdt != NULL)
	{
	   /* give semi thus unblocking stuffing */
	   semGive(pAcodeId->pSemParseUpdt);

	   /* Now Attempt to take it when, when it would block that 	*/
	   /*   is the state we want it in.				*/
	   while (semTake(pAcodeId->pSemParseUpdt,NO_WAIT) != ERROR);
	}

   }
}

void giveParseSem()
{
   if ( (pTheAcodeObject != NULL) && (pTheAcodeObject->pSemParseSuspend != NULL) )
      semGive(pTheAcodeObject->pSemParseSuspend);
}

/*--------------------------------------------------------------*/
/* AParserAA							*/
/* 	Abort sequence for Aparser.				*/
/*--------------------------------------------------------------*/
AParserAA()
{
   int tid;
   if ((tid = taskNameToId("tParser")) != ERROR)
   {
	/* taskSuspend(tid); */
        /* markBusy(APARSER_FLAGBIT); */

        /* returns 1 for ready, other for timeout */
        /* if ( wait4ParserReady(2)  != 1 ) */
        if ( parserBusy() )
        {
           DPRINT(-1,"Restart Parser\n");
	   cleanMsgesToAParser();
           AbortingParserFlag = 1;
	   acodeResetUpdtSem(pTheAcodeObject);
           parseCntDownReset();
           /* give Tuning Control Mutex */
           /* since this is deletion safe we can not restart parser while it holds this semaphore */
	   semGive(pSemOK2Tune);  

#ifdef INSTRUMENT
	   wvEvent(EVENT_PARSE_RESTART,NULL,NULL);
#endif

	    DPRINT(-1,"AParserAA: Restart Parser\n");
	    taskRestart(tid);
            parseAheadSemReset();
	    acodeClear(pTheAcodeObject);
        }
	else
	{
	   DPRINT(-1,"AParserAA: No need to Restart Parser\n");
        }
   }
}

/********************************************************/
/* Create Acode Object -  				*/
/*	 Zero info structure.				*/
/********************************************************/
ACODE_OBJ *acodeCreate(char *expname)
{
   ACODE_OBJ *pAcodeObj;


   /**** SETUP EXP Buffer ****/
   pAcodeObj = (ACODE_OBJ *) malloc(sizeof(ACODE_OBJ));
   if (pAcodeObj == NULL)
   {
    	errLogSysRet(LOGIT,debugInfo,"acodeCreate: Could not Allocate Space:");
    	return(NULL);
   }
   /* zero out structure so we don't free something by mistake */
   memset(pAcodeObj,0,sizeof(ACODE_OBJ));


   strcpy(pAcodeObj->id,expname); 


   /**** Create Interactive Acode Control Semaphore. ****/
   /* This was originally a mutex semaphore but was changed to	*/
   /* binary semaphore to provide more flexibility.		*/
   pAcodeObj->pAcodeControl = NULL;
   pAcodeObj->pAcodeControl = semBCreate(SEM_Q_PRIORITY, SEM_FULL);

   /**** Create Interactive Updt Semaphore. ****/
   pAcodeObj->pSemParseUpdt = NULL;
   pAcodeObj->pSemParseUpdt = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   /**** Create Sync Operations needing FIFO (AUTOLOCK, AUTOSHIM) ****/
   pAcodeObj->pSemParseSuspend = NULL;
/*
  Note: that putSampFromMagnet() & getSampFromMagnet() called from shandler 
        also take pSemParseSuspend, to wait for the roboproc to report during
        the GETSAMP & LOADSAMP acodes. These Acodes also take the pSemParseSuspend 
        in the standard shandler approach.
        The semaphore was changed from FIFO to PRIORITY type so that the 
        shandler (of higher priority) is released
        then the parser (lower priority) rather than the other way around.   3/11/96
*/
   /* pAcodeObj->pSemParseSuspend = semBCreate(SEM_Q_FIFO,SEM_EMPTY); */
   pAcodeObj->pSemParseSuspend = semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);

   /**** Create Fifo Start WatchDog Timer ****/
   pAcodeObj->wdFifoStart = wdCreate();

   return(pAcodeObj);
}

/********************************************************/
/* acodeDelete -  Deletes acode object and resources	*/
/*	Set up tables, Experiment info structure.	*/
/* RETURNS:						*/
/*  	void						*/
/********************************************************/
void acodeDelete(ACODE_ID pAcodeId)
{
   if (pAcodeId != NULL)
   {
	if (pAcodeId->cur_rtvar_base != NULL)
	{
	   rmRTvarSet(pAcodeId->id);
	   pAcodeId->cur_rtvar_base = NULL;
	}
	if (pAcodeId->num_tables != 0)
	{
	   rmTables(pAcodeId,pAcodeId->id,pAcodeId->num_tables);
	   pAcodeId->num_tables = 0;
	}
	if (pAcodeId->cur_acode_base != NULL)
	{
	   rmAcodeSet(pAcodeId->id, pAcodeId->cur_acode_set);
	   pAcodeId->cur_acode_base = NULL;
	}
	if (pAcodeId->pAcodeControl != NULL)
           semDelete(pAcodeId->pAcodeControl);
	if (pAcodeId->pSemParseUpdt != NULL)
           semDelete(pAcodeId->pSemParseUpdt);
	if (pAcodeId->pSemParseSuspend != NULL)
           semDelete(pAcodeId->pSemParseSuspend);

	if (pAcodeId->wdFifoStart != NULL)
           wdDelete(pAcodeId->wdFifoStart);

	free(pAcodeId);
	pTheAcodeObject = NULL;
   }
}

/*----------------------------------------------------------------------*/
/* acodeShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID acodeShwResrc(register ACODE_ID pAcodeId, int indent )
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sAcode Obj: '%s', 0x%lx\n",spaces,pAcodeId->id,pAcodeId);
   printf("%s Binary Sems: pAcodeControl ----- 0x%lx\n",spaces,pAcodeId->pAcodeControl);
   printf("%s              pSemParseUpdt  ---- 0x%lx\n",spaces,pAcodeId->pSemParseUpdt);
   printf("%s              pSemParseSuspend  - 0x%lx\n",spaces,pAcodeId->pSemParseSuspend);
   printf("%s W-Dog:       wdFifoStart  ------ 0x%lx\n",spaces,pAcodeId->wdFifoStart);
}

initPatBaseName(char *basename)
{
   strncpy(pTheAcodeObject->id,basename,EXP_BASE_NAME_SIZE); 
   printf("pTheAcodeObject->id: '%s'\n",pTheAcodeObject->id);
}

abortDma(int chan)
{
     abortActiveDma(chan);
     clearDmaRequestQueue(chan);
     cntrlFifoReset();
}
prtPat(int pat_index)
{
    int i,result,steps;
    int clr,cnt,phase,amp;
    long long duration;
    unsigned long l;
    int *patternList;
    int patternSize;
    result = getPattern(pat_index, &patternList, &patternSize);
    if (result != 0)
    {
       printf("use initPatBaseName('basename') to initialize basename if needed., use bufShow to determine base name\n");
       return;
    }
    printf("index: %d, size: %ld, Addr: 0x%lx\n",pat_index,patternSize,patternList);
    /* DPRINT3(-1,"index: %d, size: %ld, Addr: 0x%lx\n",pat_index,patternSize,patternList); */
    steps=0;
    duration=0LL;
    for(i=0; i < patternSize; i++)
    {
        if ( ( ( *patternList & 0x7C000000) >> 26) == 1 )   /* a duration count */
        {
           l = *patternList & 0x3FFFFFF;
           duration += l;
           steps++;
           if ( i < 20)
              printf("pat[%d] = 0x%lx [duration = %lu ticks, %lf ns]\n",i,*patternList,l,l*12.5);
              /* DPRINT4(-1,"pat[%d] = 0x%lx [duration = %lu ticks, %lf ns]\n",i,*patternList,l,l*12.5); */
        }
        else if ( ( ( *patternList & 0x7C000000) >> 26) == 2 )
        {
           l = *patternList & 0xFFFFFF;
           if ( i < 20)
             printf("pat[%d] = 0x%lx [ set gates: 0x%lx ] \n",i,*patternList,l);
        }
        else if ( ( ( *patternList & 0x7C000000) >> 26) == 4 )  /* encode_RFSetPhase */
        {
           clr =  (*patternList >> 25) & 1;
           cnt = (*patternList >> 16) &  0xFF;
           phase = (*patternList & 0xFFFF);
           if ( i < 20)
             printf("pat[%d] = 0x%lx [ set phase: clr: %d, count: %d, phase: 0x%lx ] \n",i,*patternList,clr,cnt,phase);
        }
        else if ( ( ( *patternList & 0x7C000000) >> 26) == 5 )  /* encode_RFSetPhaseC */
        {
           clr =  (*patternList >> 25) & 1;
           cnt = (*patternList >> 16) &  0xFF;
           phase = (*patternList & 0xFFFF);
           l = *patternList & 0xFFFFFF;
           if ( i < 20)
             printf("pat[%d] = 0x%lx [ set phaseC: clr: %d, count: %d, phase: 0x%lx ] \n",i,*patternList,clr,cnt,phase);
        }
        else if ( ( ( *patternList & 0x7C000000) >> 26) == 6 )
        {
           clr =  (*patternList >> 25) & 1;
           cnt = (*patternList >> 16) &  0xFF;
           amp = (*patternList & 0xFFFF);
           l = *patternList & 0x0FFFF;
           if ( i < 20)
             printf("pat[%d] = 0x%lx [ set amp: clr: %d, count: %d, phase: 0x%lx ] \n",i,*patternList,clr,cnt,amp);
        }
        else if ( ( ( *patternList & 0x7C000000) >> 26) == 7 )
        {
           clr =  (*patternList >> 25) & 1;
           cnt = (*patternList >> 16) &  0xFF;
           amp = (*patternList & 0xFFFF);
           l = *patternList & 0xFFFFFF;
           if ( i < 20)
             printf("pat[%d] = 0x%lx [ set amp: clr: %d, count: %d, phase: 0x%lx ] \n",i,*patternList,clr,cnt,amp);
        }
        else
        {
           if ( i < 20)
             printf("pat[%d] = 0x%lx \n",i,*patternList);
        }
        patternList++; 
    }
    printf("Total Steps: %lu, duration: %llu (0x%llx) ticks, %lf sec\n", steps, duration,duration,duration*12.5e-9);
    /* DPRINT4(-1,"Total Steps: %lu, duration: %llu (0x%llx) ticks, %lf sec\n", steps, duration,duration,duration*12.5e-9); */
}

/*----------------------------------------------------------------------*/
/* acodeShow								*/
/*	Displays information on the acodes.				*/
/*----------------------------------------------------------------------*/
acodeShow(ACODE_ID pAcodeId, int level)
{
 char tmpbufname[64],numstring[8];
 int i;
 NCLBP active;


   if (pAcodeId == NULL)
   {
   	printf("Acode structure does not exist!!!\n");
	return;
   }

   printf("Acode/exp Name: %s\n\n",pAcodeId->id);
   printf("	Interactive Flag:  %d\n", pAcodeId->interactive_flag);
   printf("	Num of Acode Sets: %d\n", pAcodeId->num_acode_sets);
   printf("	Current Acode Set: %d\n", pAcodeId->cur_acode_set);
   printf("	Number of Tables:  %d\n", pAcodeId->num_tables);

   printf("Acode Update Semaphore: \n");
   printSemInfo(pAcodeId->pSemParseUpdt,"Acode Update Semaphore",level);
   /* semShow(pAcodeId->pSemParseUpdt,level); */

   printf("Acode Control Semaphore: \n");
   printSemInfo(pAcodeId->pAcodeControl,"Acode Control Semaphore",level);
   /* semShow(pAcodeId->pAcodeControl,level); */

   printf("Parser Suspension Semaphore: \n");
   printSemInfo(pAcodeId->pSemParseSuspend,"Acode Suspension Semaphore",level);
   /* semShow(pAcodeId->pSemParseSuspend,level); */

   printf("Fifo Start Watchdog Timer: \n");
   wdShow(pAcodeId->wdFifoStart);

#ifdef XXXXX
   /* Display info on Current Acode Buffer */
   strcpy(tmpbufname,pAcodeId->id);
   sprintf(numstring,"f%d",pAcodeId->cur_acode_set);
   strcat(tmpbufname,numstring);
   printf("\nAcode Info -- Buffername: %s\n",tmpbufname);
   active = dlbFindEntry(pDlbFixBufs,tmpbufname);
   if (active == NULL)
	printf("No Current Acode Buffer\n");
   else
	dlbPrintEntry(active);


   /* Display info on Real Time Variables */
   strcpy(tmpbufname,pAcodeId->id);
   strcat(tmpbufname,"rtv");
   printf("\nRTvar Info -- Buffername: %s\n",tmpbufname);
   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   if (active == NULL)
	printf("Could not Find Real Time Variable Buffer for Acode Id\n");
   else
   {
	unsigned int *rtptr;
	dlbPrintEntry(active);
	rtptr = pAcodeId->cur_rtvar_base;
	printf("Buffer Id = 0x%x (hex) %d (dec)\n",*rtptr,*rtptr);
	rtptr++;
	printf("\n Entry    decimal value    hex value \n");
	for (i=0; i< pAcodeId->cur_rtvar_size-1; i++)
	{
	  printf(" %6d     %10d      0x%8x\n",i,*rtptr,*rtptr);
	  rtptr++;
	}  
   }

   /* Display info on Tables */
   if (pAcodeId->num_tables > 0)
   {
	printf("\nTable Info -- \n");
	for (i=0; i < pAcodeId->num_tables; i++)
	{
	   strcpy(tmpbufname,pAcodeId->id);
	   sprintf(numstring,"t%d",i);
	   strcat(tmpbufname,numstring);
	   printf("\nTable Buffername: %s\n",tmpbufname);
	   active = dlbFindEntry(pDlbFixBufs,tmpbufname);
	   if (active == NULL)
		printf("Could not Find Named Table Buffer\n");
	   else
		dlbPrintEntry(active);
	   if (pAcodeId->table_ptr[i] == NULL)
		printf("Null Table Ptr\n");
	   else
	   {
		printf("  Num Entries: %d\n",
					pAcodeId->table_ptr[i]->num_entries);
		printf("  Size Entry:  %d\n",
					pAcodeId->table_ptr[i]->size_entry);
		printf("  Mod Factor:  %d\n",
					pAcodeId->table_ptr[i]->mod_factor);
		printf("  Start Addr:  0x%x\n",&pAcodeId->table_ptr[i]->entry);
	   }

	}
   }
#endif 

}

enableSpy()
{
  enableSpyFlag = 1;
}


disableSpy()
{
  enableSpyFlag = 0;
}

stopSpy()
{
  enableSpyFlag = 0;
  spyClkStop();  /* turn off spy clock interrupts, etc. */
  spyReport();   /* report CPU usage */
}


/*  These programs setup a canned sequence of A_codes to get the Lock signal  */

/* see acodes.h for these defines */
/* #define  LOCK_CHAN	2 */
/* #define  TEST_CHAN	0 */

typedef struct {
	int	value;
	short	offset;
} rtpar;


static int	stopoffset;

int
getstopoffset()
{
	int	retval;

	retval = stopoffset;
	retval++;
	return( retval );
}

static
putcode_long(unsigned int lcode,unsigned short *acptr)
{
	unsigned short *scode;

	scode = (unsigned short *) &lcode;
	*(acptr)++ = scode[0];
	*(acptr)++ = scode[1];
}

static
dump_acodes( unsigned short *acptr, unsigned int acount )
{
	int	iter;

	for (iter = 0; iter < acount; iter++) {
		printf( "%2d:  %d\n", iter, *acptr );
		acptr++;
	}
}

#ifdef TEST_JMPLONG
static int tstval = 0;
SEM_ID ptstSem;
jmp_buf stackStartBug;
int tabort = 0;

tstJmp()
{
   int arg, arg2;
    arg = arg2 = 0;
   /* jmp_buf stackStartBug; */

   setjmp(stackStartBug);
   DPRINT(-1,"entring loop\n");
   DPRINT2(-1," %d, %d\n",arg,arg2);
   DPRINT1(-1,"tabort %d\n",tabort);
   FOREVER
   {
       semTake(ptstSem,WAIT_FOREVER);
       arg++;
       arg2 += 2;
       DPRINT2(-1,"got sem: %d, %d\n",arg,arg2);
       if (tabort == 1)
         longjmp(stackStartBug,5);
   }
}

gsem()
{
  semGive(ptstSem);
}

jj()
{
    tabort = 1;
    semGive(ptstSem);
}

strtJtask()
{
    ptstSem = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    taskSpawn("tJmp",100,0,8192,tstJmp,
		  pMsgesToAParser,2,3,4,5,6,7,8,9,10);
}
#endif
