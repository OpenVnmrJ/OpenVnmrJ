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
5-25-95,gmb  created 
*/

/*
DESCRIPTION

   This Task Handlers the Problems or Exceptions that happen to the
   system. For Example Fifo Errors (FORP,FOO,etc).

   The logic for handling these execption is encapsulated in this task
   thus there is one central place where the logic is placed.

*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "instrWvDefines.h"
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "hardware.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "adcObj.h"
#include "autoObj.h"
#include "tuneObj.h"
#include "vtfuncs.h"
#include "AParser.h"
#include "acodes.h"

extern void set2ExpEndState();  /* reprogram HSlines, ap registers to safe state */

extern MSG_Q_ID pTagFifoMsgQ;	/* MsgQ for Tag Fifo */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

extern RING_ID  pSyncActionArgs;  /* rngbuf for sync action function args */

/* Hardware Objects */
extern FIFO_ID		pTheFifoObject;
extern STMOBJ_ID	pTheStmObject;
extern ADC_ID		pTheAdcObject;
extern AUTO_ID		pTheAutoObject;
extern TUNE_ID		pTheTuneObject;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

extern ACODE_ID  pTheAcodeObject; /* Acode object */

extern int SA_Criteria;
extern int sampleHasChanged;	/* Global Sample change flag */
extern STATUS_BLOCK currentStatBlock;

WDOG_ID sHandlerWdTimeout;
int sHandlerTimeout;

startShandler(int priority, int taskoptions, int stacksize)
{
   int sHandler();

   if (taskNameToId("tSHandlr") == ERROR)
     taskSpawn("tSHandlr",priority,taskoptions,stacksize,sHandler,
		   pTagFifoMsgQ,2, 3,4,5,6,7,8,9,10);

     sHandlerWdTimeout = wdCreate();
     sHandlerTimeout = 0;

}

killShandler()
{
   int tid;
   if ((tid = taskNameToId("tSHandlr")) != ERROR)
	taskDelete(tid);
}

sHandlerWdISR()
{
   sHandlerTimeout = 1;
   wdCancel(sHandlerWdTimeout);
#ifdef INSTRUMENT
        wvEvent(EVENT_SHDLR_WDISR,NULL,NULL);
#endif
}

/*--------------------------------------------------------------*/
/* ShandlerAA							*/
/* 	Abort sequence for Shandler.				*/
/*--------------------------------------------------------------*/
ShandlerAA()
{
   int tid;
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_AA,NULL,NULL);
#endif
   if ((tid = taskNameToId("tSHandlr")) != ERROR)
   {
   	rngFlush(pSyncActionArgs);
   	fifoClrNoStart(pTheFifoObject);
#ifdef INSTRUMENT
	wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	semGive(pTheAcodeObject->pSemParseSuspend);
	taskRestart(tid);
   }
}

/*************************************************************
*
*  signalHandler - Wait for Messages that indecate a signal 
*    Then perform appropriate recovery.
*
*					Author Greg Brissey 5-25-95
*/
sHandler(MSG_Q_ID msges)
{
   long signalTag;
   int *val;
   int bytes;
   void signaldecode(long signaltype);
   void syncAction(MSG_Q_ID msges, long signaltype);

   DPRINT(1,"sHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     bytes = msgQReceive(msges, (char*) &signalTag, 
			  sizeof( long ), WAIT_FOREVER);
     DPRINT2(1,"sHandler: recv: %d bytes, Signal Type: %d\n",
			bytes, signalTag);

     if (signalTag & 0x4000)
       syncAction(msges, signalTag & (~0x4000));
     else
       signaldecode( signalTag );
   } 
}


/*********************************************************************
*
* signaldecode, based on the signal Type  decides what action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void signaldecode(long signaltype)
{
   char *token;
   int len;
   int cmd;
   int i;

   switch( signaltype )
   {
      case SETUP_CMPLT:
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETUPCMPLT,NULL,NULL);
#endif
	   DPRINT(0,"SETUP_CMPLT\n");
	   currentStatBlock.stb.AcqOpsComplFlags |= SETUP_CMPLT_FLAG;
	   setVTinterLk(0);
	   setLKInterlk(0);
	   setSpinInterlk(0);
           update_acqstate(ACQ_IDLE);
	   getstatblock();
   	   rngFlush(pSyncActionArgs);

           /* PARALLEL_CHANS */
           freeSorter();
	   clearParallelFreeBufs();

           set2ExpEndState();  /* reprogram HSlines, ap registers to safe state */
           stmHaltCode(pTheStmObject,(int)signaltype, 0); /* Donecode, Errorcode */

           fifoClrStart4Exp(pTheFifoObject);
	   if (pTheTuneObject != NULL)
	     semGive( pTheTuneObject->pSemAccessFIFO );

		break;

      case EXP_COMPLETE:
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_EXPCMPLT,NULL,NULL);
#endif
           SA_Criteria = 0;
	   DPRINT(0,"EXP_COMPLETE\n");
	   currentStatBlock.stb.AcqOpsComplFlags |= EXP_CMPLT_FLAG;
	   setVTinterLk(0);
	   setLKInterlk(0);
	   setSpinInterlk(0);
           update_acqstate(ACQ_IDLE);
	   getstatblock();
   	   rngFlush(pSyncActionArgs);

           /* PARALLEL_CHANS */
           freeSorter();
	   clearParallelFreeBufs();

           set2ExpEndState();  /* reprogram HSlines, ap registers to safe state */

           fifoClrStart4Exp(pTheFifoObject);
	   if (pTheTuneObject != NULL)
	     semGive( pTheTuneObject->pSemAccessFIFO );

		break;

      default:
		/* Who Cares */
		break;
   }
}

/*********************************************************************
*
* syncAction, based on the signal Type  decides what action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void syncAction(MSG_Q_ID msges, long signaltype)
{
   char *token;
   int len;
   int cmd;
   int i;

   switch( signaltype )
   {
      case LOCKAUTO:
	{
           int rstat;
	   short lockmode,lpwrmax,lgainmax;
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_LOCKAUTO,NULL,NULL);
#endif

	   rngBufGet(pSyncActionArgs,(char*) &lockmode,sizeof(short));
	   rngBufGet(pSyncActionArgs,(char*) &lpwrmax,sizeof(short));
	   rngBufGet(pSyncActionArgs,(char*) &lgainmax,sizeof(short));
	   DPRINT1(1,"DOING LOCKAUTO: lockmode: %d\n",(int) lockmode);
	   if ( (rstat = do_autolock((int) lockmode,(int)lpwrmax,(int)lgainmax))
			 != 0 )
           {
	        DPRINT(1,"autolock failed\n");
	        /* Besure to free the method string malloc in APint */
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = rstat;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    	        errLogRet(LOGIT,debugInfo,
			"AutoLock Error: status is %d",rstat);
           }
	   else
	   {
#ifdef INSTRUMENT
	     wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	     semGive(pTheAcodeObject->pSemParseSuspend);
           }
	}
	break;

      case SHIMAUTO:
	{
           int rstat;
	   short *methodstr,maxpwr,maxgain;
           short ntindx,npindx,dpfindx;
           short ac_offset; /* acode pointers */
	   short freeMethodStrFlag;
           unsigned int *rt_base, *rt_tbl; /* pointer to realtime buffer addr */
	   ACODE_ID pAcodeId;
           ulong_t *fidaddr;
           ulong_t *getshimfid();

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SHIMAUTO,NULL,NULL);
#endif
	   DPRINT(1,"DOING SHIMAUTO: \n");
	   rngBufGet(pSyncActionArgs,(char*) &methodstr,sizeof(methodstr));
	   rngBufGet(pSyncActionArgs,(char*) &ntindx,sizeof(ntindx));
	   rngBufGet(pSyncActionArgs,(char*) &npindx,sizeof(npindx));
	   rngBufGet(pSyncActionArgs,(char*) &dpfindx,sizeof(dpfindx));
	   rngBufGet(pSyncActionArgs,(char*) &ac_offset,sizeof(ac_offset));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	   rngBufGet(pSyncActionArgs,(char*) &maxpwr,sizeof(maxpwr));
	   rngBufGet(pSyncActionArgs,(char*) &maxgain,sizeof(maxgain));
	   rngBufGet(pSyncActionArgs,(char*) &freeMethodStrFlag,sizeof(freeMethodStrFlag));

	   DPRINT2(1,"pAcodeId: 0x%lx, pTheAcodeObject: 0x%lx \n",pAcodeId,pTheAcodeObject);
	   DPRINT3(1,"Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
		pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);
           DPRINT2(1,"maxpwr=%d, maxgain=%d",maxpwr,maxgain);
           rt_base = pAcodeId->cur_rtvar_base;
           rt_tbl=rt_base+1;
	   DPRINT4(1,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	   DPRINT2(1,">>>  dpf (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx]);
           /* ==========================================================
	      HOLY Moses, I'd have to pass ntindx, npindx, dpfindx, ac_offset, 
		pAcodeId arguments through more subroutines than you 
		could shake a stick at !!, 
	        do_autoshim() ->  
		[ run_simplex(), newnet(), selfsize(), trackbest(), 
		  lock_check(), getyvalue(), restart(), onedGet_io(), 
		  mbrak(), many more 
                ] -> Get_io() -> get_io() -> 
		     getfid() -> Getfid() -> getfiddata()
	      What A Tangle Web
	      So for Now I'll cheat.
	      I will set statics in datahandler where they are eventually used.
	      and have Getfid() -> getshimfid()
	      ============================================================ */
	   setFidParms(ntindx,npindx,dpfindx,ac_offset,pAcodeId); /* see dathandler.c */
	    /* just test getshimfid for now */
#ifdef FIDSHIMTEST
	   fidaddr = getshimfid(); 
           if (freeMethodStrFlag == 1)
	      free(methodstr);
	   semGive(pTheAcodeObject->pSemParseSuspend);
#endif
	   if ( (rstat = do_autoshim(methodstr,maxpwr,maxgain)) != 0 )
           {
	        DPRINT(1,"autoshim failed\n");
	        /* Besure to free the method string malloc in APint */

                if (freeMethodStrFlag == 1)
	           free(methodstr);

       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = rstat;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    	        errLogRet(LOGIT,debugInfo,
			"AutoShim Error: status is %d",rstat);
           }
	   else
	   {
	     /* Besure to free the method string malloc in APint */
             if (freeMethodStrFlag == 1)
	         free(methodstr);
#ifdef INSTRUMENT
	     wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	     semGive(pTheAcodeObject->pSemParseSuspend);
             /* fifoStart(pTheFifoObject);   /* done restart fifo */
           }
	}
	break;

      case AUTOGAIN:
	{
           int rstat;
           short ntindx,npindx,rcvgindx,apaddr,apdly,maxval,minval,stepval;
	   short dpfindx,adcbits;
           unsigned int *rt_base, *rt_tbl; /* pointer to realtime buffer addr */
           unsigned short ac_offset; /* acode pointers */
           unsigned short *ac_cur; /* acode pointers */
	   ACODE_ID pAcodeId;
	   ACODE_OBJ pAutoGainAcodeId;

	   DPRINT(1,"DOING AUTOGAIN: \n");

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_AUTOGAIN,NULL,NULL);
#endif
	   rngBufGet(pSyncActionArgs,(char*) &ntindx,sizeof(ntindx));
	   rngBufGet(pSyncActionArgs,(char*) &npindx,sizeof(npindx));
	   rngBufGet(pSyncActionArgs,(char*) &dpfindx,sizeof(dpfindx));
	   rngBufGet(pSyncActionArgs,(char*) &apaddr,sizeof(apaddr));
	   rngBufGet(pSyncActionArgs,(char*) &apdly,sizeof(apdly));
	   rngBufGet(pSyncActionArgs,(char*) &rcvgindx,sizeof(rcvgindx));
	   rngBufGet(pSyncActionArgs,(char*) &maxval,sizeof(maxval));
	   rngBufGet(pSyncActionArgs,(char*) &minval,sizeof(minval));
	   rngBufGet(pSyncActionArgs,(char*) &stepval,sizeof(stepval));
	   rngBufGet(pSyncActionArgs,(char*) &adcbits,sizeof(adcbits));
/*
	   rngBufGet(pSyncActionArgs,(char*) &ac_cur,sizeof(ac_cur));
*/
	   rngBufGet(pSyncActionArgs,(char*) &ac_offset,sizeof(ac_offset));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));

	   DPRINT3(1,"Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
		pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);
           rt_base = pAcodeId->cur_rtvar_base;
           rt_tbl=rt_base+1;
	   DPRINT4(1,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	   DPRINT4(1,">>>  dpf (rt_tbl[%d]) = %d, recvgain (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx],rcvgindx,rt_tbl[rcvgindx]);
	   DPRINT6(1,">>>  apaddr: 0x%x, apdelay: %d, max: %d, min: %d, step: %d, adcbits: %d\n",
		apaddr,apdly,maxval,minval,stepval,adcbits);
	   DPRINT2(1,"Current Acode location: 0x%lx, Acode = %d\n",
		ac_cur,*ac_cur);
	   DPRINT3(1,"Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
		pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);

	   DPRINT1(1,"Acode offset: %d\n", ac_offset);

	   if ( (rstat = do_autogain(pAcodeId,ntindx,npindx,dpfindx,
				     rcvgindx,apaddr,apdly,
				     maxval,minval,stepval,adcbits,ac_offset)) != 0 )
           {
	        DPRINT(1,"autoshim failed\n");
	        /* Besure to free the method string malloc in APint */
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = rstat;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    	        errLogRet(LOGIT,debugInfo,
			"AutoShim Error: status is %d",rstat);
           }
	   else
	   {
#ifdef INSTRUMENT
	     wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	     semGive(pTheAcodeObject->pSemParseSuspend);
             /* fifoStart(pTheFifoObject);   /* done restart fifo */
           }
	}
	break;

      case SETVT:
        {
           int rstat;
           short temp, ltmpxoff, pid, vttype, Tmpintlk;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETVT,NULL,NULL);
#endif
   	   fifoSetNoStart(pTheFifoObject);
	   DPRINT(0,"DOING SETVT\n");
	   rngBufGet(pSyncActionArgs,(char*) &vttype, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &Tmpintlk, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &pid, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &temp, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &ltmpxoff, sizeof( short ));
	   DPRINT2(1,"SETVT: vttype=%d, Tmpintlk=%d\n",vttype,Tmpintlk);
	   setVTtype((int) vttype);
	   setVTinterLk( ((Tmpintlk != 0) ? 1 : 0) );  /* set in='y' or in='n' */
	   setVTErrMode(Tmpintlk);  /* set error mode, warning(14) Error(15) */

	   DPRINT3(1,"SETVT: temp=%d, ltemp=%d, pid=%d\n",temp,ltmpxoff,pid);
           if (rstat = setVT((int)temp,(int)ltmpxoff,(int)pid)) /* set VT to temperature */
           {
    		 errLogRet(LOGIT,debugInfo,
				"VT Error: status is %d",rstat);
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = rstat;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
           }
	   else
           {
	     DPRINT1(1,"SETVT: startFifo rstat=%d \n",rstat);
   	     fifoClrNoStart(pTheFifoObject);
             fifoStart(pTheFifoObject);   /* done restart fifo */
           }
        }
	break;

      case WAIT4VT:
        {
           int rstat;
           short timeout,errmode;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_WAIT4VT,NULL,NULL);
#endif
   	   fifoSetNoStart(pTheFifoObject);
	   rngBufGet(pSyncActionArgs,(char*) &timeout, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &errmode, sizeof( short ));
	   setVTErrMode((int) errmode);
	   DPRINT2(1,"DOING WAIT4VT: ErrorMode:%d, timeout=%d\n",errmode,timeout);
	   if ( (rstat = wait4VT((int)timeout)) != 0 )
           {
		errLogRet(LOGIT,debugInfo,
				"VT Wait Error: status is %d",rstat);
		setVTinterLk(0);
		if (getVTErrMode() == HARD_ERROR)  /*(15)  Error */
	        {
	          /* disable all interlock tests, since this will stop the exp (for Error), don't want
		     continue error coming back while no exp. is running */
		  setLKInterlk(0);
		  setSpinInterlk(0);
       		  GenericException.exceptionType = HARD_ERROR;  
       		  GenericException.reportEvent = rstat;   /* errorcode is returned */
    		  /* send error to exception handler task */
    		  msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		     sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
		}
		else  /* WARNING */
		{
       		  GenericException.exceptionType = WARNING_MSG;  
       		  GenericException.reportEvent = rstat;   /* errorcode is returned */
	          DPRINT1(1,"WAIT4VT: start FIFO rstat=%d\n",rstat);
    		  /* send error to exception handler task */
    		  msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		     sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
   	          fifoClrNoStart(pTheFifoObject);
                  fifoStart(pTheFifoObject);   /* done restart fifo */
		}
           }
	   else
           {
	     DPRINT1(1,"WAIT4VT: start FIFO rstat=%d\n",rstat);
   	     fifoClrNoStart(pTheFifoObject);
             fifoStart(pTheFifoObject);   /* done restart fifo */
	   }
        }
	break;

      case SETSPIN:
        {
           int rstat;
           short bumpFlag,speed,MasThreshold;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETSPIN,NULL,NULL);
#endif
   	   fifoSetNoStart(pTheFifoObject);
	   DPRINT(1,"DOING SETSPIN\n");
	   rngBufGet(pSyncActionArgs,(char*) &speed, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &MasThreshold, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( short ));
	   DPRINT3(1,"DOING SETSPIN: speed: %d, MasThreshold: %d, bump flag: %d\n",
		      speed,MasThreshold,bumpFlag);
	   setSpinMASThres((int)MasThreshold);
	   if ( (rstat = setspin( (int) speed, (int) bumpFlag )) != 0)
           {
    		 errLogRet(LOGIT,debugInfo,
				"Spin Error: status is %d",rstat);
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = rstat;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
           }
	   else
           {
	     DPRINT1(1,"SETSPIN: start FIFO rstat=%d\n",rstat);
   	     fifoClrNoStart(pTheFifoObject);
             fifoStart(pTheFifoObject);   /* done restart fifo */
	   }

        }
	break;

      case CHECKSPIN:
        {
           int rstat;
           short bumpFlag,delta,errmode;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_CHECKSPIN,NULL,NULL);
#endif
   	   fifoSetNoStart(pTheFifoObject);
	   DPRINT(1,"DOING CHECKSPIN\n");
	   rngBufGet(pSyncActionArgs,(char*) &delta, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &errmode, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( short ));
	   DPRINT3(1,"DOING CHECKSPIN: Tolerance: %d Hz: Errmode %d  bump flag: %d\n",
		      delta,errmode,bumpFlag);
	   setSpinRegDelta((int) delta);
	   setSpinErrMode((int) errmode);
	   if ( (rstat = spinreg( (int) bumpFlag )) != 0)
           {
    		 errLogRet(LOGIT,debugInfo,
				"Spin Error: status is %d",rstat);


		setSpinInterlk(0);
		DPRINT1(0,"setSpinErrMode: %d, (1-Err,2-Warning)\n",getSpinErrMode());

       		GenericException.reportEvent = rstat;   /* errorcode is returned */

		if (getSpinErrMode() == HARD_ERROR)  /* (15) Error */
	        {
	          /* disable all interlock tests, since this will stop the exp, don't want
		     continue error coming back while no exp. is running */
		  setVTinterLk(0);
		  setLKInterlk(0);
       		  GenericException.exceptionType = HARD_ERROR;  
    		  /* send error to exception handler task */
    		  msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		     sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
		}
		else  /* WARNING */
		{
       		  GenericException.exceptionType = WARNING_MSG;  
    		  /* send error to exception handler task */
    		  msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		     sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	          DPRINT1(1,"CHECKSPIN: start FIFO rstat=%d\n",rstat);
   	          fifoClrNoStart(pTheFifoObject);
                  fifoStart(pTheFifoObject);   /* done restart fifo */
		}
           }
	   else
           {
	     DPRINT1(1,"CHECKSPIN: start FIFO rstat=%d\n",rstat);
   	     fifoClrNoStart(pTheFifoObject);
             fifoStart(pTheFifoObject);   /* done restart fifo */
	   }

        }
	break;

      case GETSAMP:
        {
	   long sample2ChgTo;
	   int   skipsampdetect, stat;
	   ACODE_ID pAcodeId;
           short bumpFlag;
	   int getSampFromMagnet(ACODE_ID pAcodeId, long newSample, long *presentSample, int skipsampdetect);

	   DPRINT(0,"DOING GETSAMP\n");
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_GETSAMP,NULL,NULL);
#endif
	   rngBufGet(pSyncActionArgs,(char*) &sample2ChgTo, sizeof( long ));
	   rngBufGet(pSyncActionArgs,(char*) &skipsampdetect, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( short ));	/* ignored for now */
	   DPRINT2(0,"DOING GETSAMP: Sample: %d, Skip Detect Sample Flag: %d\n",sample2ChgTo,skipsampdetect);

	   DPRINT3(0,
	       "GETSAMP: current: %d, new: %d Sample changed: '%s'\n",
		  currentStatBlock.stb.AcqSample,sample2ChgTo,
		  (sampleHasChanged == TRUE) ? "YES" : "NO" );

   	   if (sample2ChgTo != currentStatBlock.stb.AcqSample)  
   	   {
		int stat_before_samp;

		stat_before_samp = get_acqstate();
      		update_acqstate(ACQ_RETRIEVSMP);
      		getstatblock();

   	        fifoSetNoStart(pTheFifoObject);

                stat = getSampFromMagnet(pAcodeId, sample2ChgTo, &(currentStatBlock.stb.AcqSample),skipsampdetect);
	        if ( stat != 0 )
                {
	           DPRINT(1,"GETSAMP: failed\n");
       		   GenericException.exceptionType = HARD_ERROR;  
       		   GenericException.reportEvent = stat;   /* errorcode is returned */
    		   /* send error to exception handler task */
    		   msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		      sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    		   errLogRet(LOGIT,debugInfo,
				"GETSAMP Error: status is %d",stat);
                }
	        else
	        {
#ifdef INSTRUMENT
	          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
   	          fifoClrNoStart(pTheFifoObject);
	          semGive(pTheAcodeObject->pSemParseSuspend);
                }

      		update_acqstate(stat_before_samp);
                getstatblock();
              }
              else
              {
	         semGive(pTheAcodeObject->pSemParseSuspend);
              }
	      DPRINT1(0,"GETSAMP: Complete; Sample %d In Magnet\n",
			currentStatBlock.stb.AcqSample);
        }
	break;


      case LOADSAMP:
        {
           short bumpFlag,spinActive;
	   long sample2ChgTo;
	   int   skipsampdetect, stat;
	   ACODE_ID pAcodeId;
	   int putSampIntoMagnet(ACODE_ID pAcodeId, int newSample, long *presentSamp,
                                 int skipsampdetect, int spinActive, int bumpFlag);

	   DPRINT(0,"DOING LOADSAMP\n");
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_LOADSAMP,NULL,NULL);
#endif
	   rngBufGet(pSyncActionArgs,(char*) &sample2ChgTo, sizeof( long ));
	   rngBufGet(pSyncActionArgs,(char*) &skipsampdetect, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	   rngBufGet(pSyncActionArgs,(char*) &spinActive, sizeof( short ));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( short ));
	   DPRINT3(0,
		"LOADSAMP: Sample: %d, Detect Sample Flag: %d, bumpFlag=%d\n",
		sample2ChgTo,skipsampdetect,bumpFlag);

	   DPRINT3(0,
	      "LOADSAMP: present: %d, new: %d Sample changed: '%s'\n",
		  currentStatBlock.stb.AcqSample,sample2ChgTo,
		  (sampleHasChanged == TRUE) ? "YES" : "NO" );

   	   if (sample2ChgTo != currentStatBlock.stb.AcqSample)  
   	   {
		int stat_before_loadsamp;

		stat_before_loadsamp = get_acqstate();
	        update_acqstate(ACQ_LOADSMP);
      		getstatblock();

   	        fifoSetNoStart(pTheFifoObject);

                stat = putSampIntoMagnet(pAcodeId,
			sample2ChgTo, &(currentStatBlock.stb.AcqSample),
			skipsampdetect, (int)spinActive, (int)bumpFlag);
	        if ( stat != 0 )
                {
	           DPRINT(1,"LOADSAMP: failed\n");
       		   GenericException.exceptionType = HARD_ERROR;  
       		   GenericException.reportEvent = stat;   /* errorcode is returned */
    		   /* send error to exception handler task */
    		   msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		      sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    		   errLogRet(LOGIT,debugInfo,
				"LOADSAMP Error: status is %d",stat);
                }
	        else
	        {
#ifdef INSTRUMENT
	          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
   	          fifoClrNoStart(pTheFifoObject);
	          semGive(pTheAcodeObject->pSemParseSuspend);
                }

      		update_acqstate(stat_before_loadsamp);
                getstatblock();
              }
              else
	      {
	          semGive(pTheAcodeObject->pSemParseSuspend);
              }
	      DPRINT1(0,"LOADSAMP: Complete; Sample %d In Magnet\n",
			currentStatBlock.stb.AcqSample);
         }
         break;



      case SYNC_PARSER:
        {
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SYNC_PARSER,NULL,NULL);
#endif
	  DPRINT(1,"SYNC_PARSER: event has occurred, let parser continue\n");
#ifdef INSTRUMENT
          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	  semGive(pTheAcodeObject->pSemParseSuspend);
        }
	break;

      case NOISE_CMPLT:
        {
           /* start wdog timeout of 4 seconds */ 
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_NOISECMPLT,NULL,NULL);
#endif
           sHandlerTimeout = 0;
           wdStart(sHandlerWdTimeout,sysClkRateGet() * 4, sHandlerWdISR, 0);

	   while( ( stmDataBufsAllFree(pTheStmObject) == 0 ) && (sHandlerTimeout == 0) )
	   {
              DPRINT(0,"NOISE_CMPLT: not all free yet, wait 1 tick \n");
	      taskDelay(1);
	   }
           wdCancel(sHandlerWdTimeout);

           if (sHandlerTimeout == 1)
           {
	      DPRINT(-1,"NOISE_CMPLT: W-Dog Timeout!!\n");
	      /*
   	      stmPrtStatus(pTheStmObject);
   	      stmPrtRegs(pTheStmObject);
	      */

       	      GenericException.exceptionType = HARD_ERROR;  
       	      GenericException.reportEvent = SYSTEMERROR + NOISECMPLTWDOG;   /* errorcode is returned */
    	      /* send error to exception handler task */
    	      msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
	                  sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
    	      errLogRet(LOGIT,debugInfo, "NOISE_CMPLT: W-Dog Timeout");
	   }

	   DPRINT(1,"NOISE_CMPLT: data buffers all free, let parser continue\n");
#ifdef INSTRUMENT
          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	  
	  /* if we get here we shimmed, locked, so clear flag */
	  sampleHasChanged = 0;
	  /* clear this flag so fifo will be started later on */
          fifoClrStart4Exp(pTheFifoObject);

	  semGive(pTheAcodeObject->pSemParseSuspend);
	  /* don't start fifo here since there will nothing in the fifo at this point
	     let the parser continue until it decides to start the fifo */
        }
	break;


      default:
		/* Who Cares */
		break;
   }
}
