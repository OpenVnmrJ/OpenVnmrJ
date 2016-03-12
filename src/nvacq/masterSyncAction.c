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
11-15-04,gmb  created 
*/

/*
DESCRIPTION

   These routines handle the master automation and sync op functions
   For Example Setting VT, , setting spin, Sample changing, etc.
*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "instrWvDefines.h"
#include "taskPriority.h"
#include "nvhardware.h"
/* #include "hostAcqStructs.h" */
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "AParser.h"
#include "ACode32.h"
#include "Cntlr_Comm.h"
#include "Lock_Cmd.h"
#include "cntlrStates.h"
#include "shandler.h"
#include "fifoFuncs.h"
#include "Console_Stat.h"
#include "autolock.h"
#include "master.h"

extern MSG_Q_ID pFifoSwItrMsgQ;	  /* MsgQ for shandler */
extern RING_ID  pPending_SW_Itrs; /* SW interrupt Type/Usage for shandler */
extern RING_ID  pSyncActionArgs; /* shandler function arguments ring buffer */

extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */

extern MSG_Q_ID pMsgesToAutoLock; /* MsgQ for autolock */

extern ACODE_ID  pTheAcodeObject; /* Acode object */

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient,Etc.*/
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */
extern int warningAsserted;
extern int readuserbyte;


extern int sampleHasChanged;	/* Global Sample change flag */
extern int host_abort;
extern int failAsserted;
extern int tnlk_flag;

extern Console_Stat	*pCurrentStatBlock;

WDOG_ID sHandlerWdTimeout;
extern SEM_ID pRoboAckSem;
extern MSG_Q_ID pRoboAckMsgQ;

#define CASE		(2)
#define GET_SAMPLE	(9)
#define LOAD_SAMPLE	(10)
#define REPUT_SAMPLE	(30)
#define FAILPUT_SAMPLE	(31)

#define LSDV_SPIN_REG   0x10
#define LSDV_VT_REG     0x0800

/* dummy up for now */
/*
SampleBump() { DPRINT(-1,"SampleBump....\n");  }
int LockSense() { DPRINT(-1,"Sample Locked.\n"); return 1; }
int SpinValueGet() {  return 10; }
int SampleDetect() { return 1; }
int SampleInsert() { DPRINT(-1,"Sample Insert....\n"); }
int SampleEject() { DPRINT(-1,"Sample Eject....\n"); }
int setspin( int sampleSpinRate, int bumpFlag ) { }
*/

extern int spinreg(int bumpflag, int errmode);
extern int do_autoshim(char *comstr, short maxpwr, short maxgain);
static int shimMethod[1024];
static int oldSpinSet = -1;
static int oldTempSet = 30000;

int setShimMethod(int meth[], int len)
{
   int i;

   if (len > 1023)
   {
      DPRINT(-1,"autoShim failed method too complex\n");
    	/* send error to exception handler task */
      return(-1);
   }
  
   for (i=0; i < len; i++)
      shimMethod[i] = meth[i];
   return(0);
}

/*********************************************************************
*
* syncAction, based on the signal Type  decides what action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void syncAction(long signaltype)
{
   char *token;
   int len;
   int cmd;
   int i;

   switch( signaltype )
   {

      case LOCKAUTO:
	{
           int errorcode;
	   int lockmode,lpwrmax,lgainmax;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_LOCKAUTO,NULL,NULL);
#endif

	   rngBufGet(pSyncActionArgs,(char*) &lockmode,sizeof(int));
	   rngBufGet(pSyncActionArgs,(char*) &lpwrmax,sizeof(int));
	   rngBufGet(pSyncActionArgs,(char*) &lgainmax,sizeof(int));
	   DPRINT1(-1,"DOING LOCKAUTO: lockmode: %d\n",(int) lockmode);
	   if ( ((errorcode = doAutoLock((int) lockmode,(int)lpwrmax,(int)lgainmax, sampleHasChanged) ) 
			 != 0 ) && !host_abort)
           {
	        DPRINT(1,"autolock failed\n");
	        /* Besure to free the method string malloc in APint */
    		/* send error to exception handler task */
                sendException(HARD_ERROR, errorcode, 0,0,NULL);
    		errLogRet(LOGIT,debugInfo,
			"AutoLock Error: status is %d",errorcode);
           }
	   else
	   {
#ifdef INSTRUMENT
	     wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
             if (tnlk_flag) setTnEqualsLkState();
             host_abort = 0;
	     semGive(pTheAcodeObject->pSemParseSuspend);
           }
	}
	break;

      case SHIMAUTO:
	{
           int errorcode;
	   char *methodString;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_LOCKAUTO,NULL,NULL);
#endif
           methodString = (char *) shimMethod;
           if ( ((errorcode = do_autoshim( methodString, (short) 68, (short) 48) ) != 0) && !host_abort)
           {
	        DPRINT(1,"autoShim failed\n");
    		/* send error to exception handler task */
                sendException(HARD_ERROR, errorcode, 0,0,NULL);
    		errLogRet(LOGIT,debugInfo,
			"AutoShim Error: status is %d",errorcode);
           }
	   else
	   {
#ifdef INSTRUMENT
	     wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
             if (tnlk_flag) setTnEqualsLkState();
             host_abort = 0;
	     semGive(pTheAcodeObject->pSemParseSuspend);
           }
	}
	break;

      case GETSAMP:
        {
	   long sample2ChgTo;
	   int   skipsampdetect, stat;
	   ACODE_ID pAcodeId;
           int bumpFlag;
	   int getSampFromMagnet(ACODE_ID pAcodeId, long newSample, long *presentSample, int skipsampdetect);

	   DPRINT(0,"DOING GETSAMP\n");

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_GETSAMP,NULL,NULL);
#endif
	   rngBufGet(pSyncActionArgs,(char*) &sample2ChgTo, sizeof( long ));
	   rngBufGet(pSyncActionArgs,(char*) &skipsampdetect, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( int ));	/* ignored for now */
	   DPRINT2(0,"DOING GETSAMP: Sample: %d, Skip Detect Sample Flag: %d\n",sample2ChgTo,skipsampdetect);

	   DPRINT3(-1,
	       "GETSAMP: current: %d, new: %d Sample changed: '%s'\n",
		  pCurrentStatBlock->AcqSample,sample2ChgTo,
		  (sampleHasChanged == TRUE) ? "YES" : "NO" );


   	   /* if (sample2ChgTo != pCurrentStatBlock->AcqSample)   */

           if ( (sample2ChgTo != pCurrentStatBlock->AcqSample) &&
                   ( ((pCurrentStatBlock->AcqSample % 1000) != 0) || 
                   (skipsampdetect == 0) || (skipsampdetect == 2) ))
   	   {
		int stat_before_samp;

		stat_before_samp = getAcqState();
                setAcqState( ACQ_RETRIEVSMP );

   	        /* fifoSetNoStart(pTheFifoObject); */

                stat = getSampFromMagnet(pAcodeId, sample2ChgTo, &(pCurrentStatBlock->AcqSample),skipsampdetect);

               /* error or not give the parser semaphore, less we get stuck in an error abort */
#ifdef INSTRUMENT
	          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
               /* error or not give the parser semaphore, less we get stuck in an error abort */
	        semGive(pTheAcodeObject->pSemParseSuspend);

	        if ( (stat != 0) && (stat != -99) )   /* -99 == aborted by some other error */
                {
	           DPRINT(1,"GETSAMP: failed\n");
    		   /* send error to exception handler task */
                   sendException(HARD_ERROR, stat, 0,0,NULL);
    		   errLogRet(LOGIT,debugInfo,
				"GETSAMP Error: status is %d",stat);
                }
#ifdef NOW_pSemParseSuspend_GIVEN_AT_TOP
*	        else
*	        {
*#ifdef INSTRUMENT
*	          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
*#endif
*   	          /* fifoClrNoStart(pTheFifoObject); */
*	          semGive(pTheAcodeObject->pSemParseSuspend);
*                }
#endif   /* NOW_pSemParseSuspend_GIVEN_AT_TOP */

      		setAcqState(stat_before_samp);
              }
              else
              {
	         semGive(pTheAcodeObject->pSemParseSuspend);
              }
	      DPRINT1(0,"GETSAMP: Complete; Sample %d In Magnet\n",
			pCurrentStatBlock->AcqSample);
        }
	break;


      case LOADSAMP:
        {
           int bumpFlag,spinActive;
	   long sample2ChgTo;
	   int   skipsampdetect, stat;
	   ACODE_ID pAcodeId;
           int putSampIntoMagnet(ACODE_ID pAcodeId, long newSample, long *presentSamp,
                  int skipsampdetect, int spinActive, int bumpFlag);

	   DPRINT(0,"DOING LOADSAMP\n");

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_LOADSAMP,NULL,NULL);
#endif
	   rngBufGet(pSyncActionArgs,(char*) &sample2ChgTo, sizeof( long ));
	   rngBufGet(pSyncActionArgs,(char*) &skipsampdetect, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	   rngBufGet(pSyncActionArgs,(char*) &spinActive, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &bumpFlag, sizeof( int ));
	   DPRINT3(-1,
		"LOADSAMP: Sample: %d, Detect Sample Flag: %d, bumpFlag=%d\n",
		sample2ChgTo,skipsampdetect,bumpFlag);

	   DPRINT3(-1,
	      "LOADSAMP: present: %d, new: %d Sample changed: '%s'\n",
		  pCurrentStatBlock->AcqSample,sample2ChgTo,
		  (sampleHasChanged == TRUE) ? "YES" : "NO" );

              if ( ((sample2ChgTo % 1000) != 0) )
   	   /* if (sample2ChgTo != pCurrentStatBlock->AcqSample)   */
           if ( (sample2ChgTo != pCurrentStatBlock->AcqSample) && ((sample2ChgTo % 1000) != 0) )
   	   {
		int stat_before_loadsamp;

		stat_before_loadsamp = getAcqState();

	        setAcqState(ACQ_LOADSMP);

   	        /* fifoSetNoStart(pTheFifoObject); */

                stat = putSampIntoMagnet(pAcodeId,
			sample2ChgTo, &(pCurrentStatBlock->AcqSample),
			skipsampdetect, (int)spinActive, (int)bumpFlag);

#ifdef INSTRUMENT
	         wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
               /* error or not give the parser semaphore, less we get stuck in an error abort */
	        semGive(pTheAcodeObject->pSemParseSuspend);

	        /* if ( stat != 0 ) */
	        if ( (stat != 0) && (stat != -99) )   /* -99 == aborted by some other error */
                {
	           DPRINT(1,"LOADSAMP: failed\n");
    		   /* send error to exception handler task */
                   sendException(HARD_ERROR, stat, 0,0,NULL);
    		   errLogRet(LOGIT,debugInfo,
				"LOADSAMP Error: status is %d",stat);
                }
#ifdef NOW_pSemParseSuspend_GIVEN_AT_TOP
*	        else
*	        {
*#ifdef INSTRUMENT
*	          wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
*#endif
*   	          /* fifoClrNoStart(pTheFifoObject); */
*	          semGive(pTheAcodeObject->pSemParseSuspend);
*                }
#endif   /* NOW_pSemParseSuspend_GIVEN_AT_TOP */

      		setAcqState(stat_before_loadsamp);
              }
              else
	      {
	          semGive(pTheAcodeObject->pSemParseSuspend);
              }
	      DPRINT1(0,"LOADSAMP: Complete; Sample %d In Magnet\n",
			pCurrentStatBlock->AcqSample);
         }
         break;
      case SETVT:
        {
           int rstat;
           int temp, tmpoff, pid, vttype, tmpintlk;

	   DPRINT(0,"DOING SETVT\n");
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETVT,NULL,NULL);
#endif
   	   /* fifoSetNoStart(pTheFifoObject); */
	   rngBufGet(pSyncActionArgs,(char*) &vttype,   sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pid,      sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &temp,     sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &tmpoff,   sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &tmpintlk, sizeof( int ));
	   DPRINT2(1,"SETVT: vttype=%d, tmpintlk=%d\n",vttype,tmpintlk);
           oldTempSet = pCurrentStatBlock->AcqVTSet;
	   setVTtype(vttype);
	   setVTinterLk( ((tmpintlk != 0) ? 1 : 0) );	/* is tin='y' or  'n' */
	   setVTErrMode(tmpintlk);   /* set error mode, warning(14) Error(15) */

	   DPRINT3(1,"SETVT: temp=%d, ltemp=%d, pid=%d\n",temp,tmpoff,pid);
           if (rstat = setVT((int)temp,(int)tmpoff,(int)pid)) /* set VT */
           {
              errLogRet(LOGIT,debugInfo, "VT Error: status is %d",rstat);
              /* send error to exception handler task */
              sendException(HARD_ERROR, rstat, 0,0,NULL);
           }
	   else
           {
	      DPRINT1(1,"SETVT: give SEM rstat=%d \n",rstat);
              clearStart4ExpFlag();  // alow to start fifo again
              semGive(pTheAcodeObject->pSemParseSuspend);
           }
        }
	break;

      case WAIT4VT:
        {
           int rstat;
           int timeout,errmode;
           int doWait = 1;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_WAIT4VT,NULL,NULL);
#endif

   	   /* fifoSetNoStart(pTheFifoObject); */
	   rngBufGet(pSyncActionArgs,(char*) &errmode, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &timeout, sizeof( int ));
	   setVTErrMode((int) errmode);
           if ( (pCurrentStatBlock->AcqVTSet == oldTempSet) &&
               ((pCurrentStatBlock->AcqLSDVbits & LSDV_VT_REG) == LSDV_VT_REG) )
           {
                doWait = 0;  /* Already regulated. skip wait4VT() */
           }

	   DPRINT2(-1,"DOING WAIT4VT: ErrorMode:%d, timeout=%d\n",	
				errmode,timeout);
	   if (  doWait && ( (rstat = wait4VT((int)timeout)) != 0) )
           {
		DPRINT1(-10,"VT Wait Error: status is %d",rstat);
		setVTinterLk(0);
		if (errmode == HARD_ERROR)  /*(15)  Error */
	        {
	          /* disable all interlock tests, since this will 
                   * stop the exp (for Error), don't want 
		   * continues errors coming back while no exp. is running
                   */
		  //  setLKInterlk(0);
		  setSpinInterlk(0);
    		  /* send error to exception handler task */
                  sendException(HARD_ERROR, rstat, 0,0,NULL);
		}
		else  /* WARNING or no message */
		{
                  if (errmode)
                     sendException(WARNING_MSG, rstat, 0,0,NULL);
	          DPRINT1(1,"WAIT4VT: give SEM rstat=%d\n",rstat);
	          /* DPRINT1(1,"WAIT4VT: start FIFO rstat=%d\n",rstat);
    		  /* send error to exception handler task */
   	          /* fifoClrNoStart(pTheFifoObject);
                  /* fifoStart(pTheFifoObject);   /* done restart fifo */
                  clearStart4ExpFlag();  // alow to start fifo again
                  semGive(pTheAcodeObject->pSemParseSuspend);
		}
           }
	   else
           {
	     DPRINT1(1,"WAIT4VT: give SEM dowait= %d \n", doWait);
             clearStart4ExpFlag();  // alow to start fifo again
             semGive(pTheAcodeObject->pSemParseSuspend);
	   }
        }
	break;

      case SETSPIN:
        {
           int rstat;
           int bumpflag,speed,spinner;

#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETSPIN,NULL,NULL);
#endif
	   DPRINT(-1,"DOING SETSPIN\n");
   	   /* fifoSetNoStart(pTheFifoObject); */

	   rngBufGet(pSyncActionArgs,(char*) &speed,     sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &spinner, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &bumpflag,  sizeof( int ));
	   DPRINT3(1,"DOING SETSPIN: speed: %d, spinner: %d, bumpflag: %d\n",
						speed,spinner,bumpflag);
      oldSpinSet = pCurrentStatBlock->AcqSpinSet;
	   setSpinnerType((int)spinner);
	   if ( (rstat = spinValueSet( (int) speed, (int) bumpflag )) != 0)
      {
		   errLogRet(LOGIT,debugInfo, "Spin Error: status is %d",rstat);
    		/* send error to exception handler task */
    		sendException(HARD_ERROR, rstat, 0, 0, NULL);
      }
	   else
      {
	     DPRINT1(1,"SETSPIN: give SEM rstat=%d\n",rstat);
             clearStart4ExpFlag();  // alow to start fifo again
             semGive(pTheAcodeObject->pSemParseSuspend);
	   }
      }
      break;

      case CHECKSPIN:
        {
           int rstat;
           int delta,errmode,bumpflag;
           int doWait = 1;

#ifdef INSTRUMENT
	        wvEvent(EVENT_SHDLR_CHECKSPIN,NULL,NULL);
#endif
	       DPRINT(1,"DOING CHECKSPIN\n");
   	   /* fifoSetNoStart(pTheFifoObject); */
	       rngBufGet(pSyncActionArgs,(char*) &delta,    sizeof( int ));
	       rngBufGet(pSyncActionArgs,(char*) &errmode,  sizeof( int ));
	       rngBufGet(pSyncActionArgs,(char*) &bumpflag, sizeof( int ));
	       DPRINT3(1,"DOING CHECKSPIN: Tol: %d Hz: Errmode %d  bumpflag: %d\n",
		              delta,errmode,bumpflag);
	       setSpinRegDelta((int) delta);
	       //setSpinErrMode((int) errmode);
          if ( (pCurrentStatBlock->AcqSpinSet == oldSpinSet) &&
               ((pCurrentStatBlock->AcqLSDVbits & LSDV_SPIN_REG) == LSDV_SPIN_REG) )
          {
                doWait = 0;  /* Already regulated. skip spinreg() */
          }

	       if ( doWait &&  ( (rstat = spinreg( bumpflag, errmode )) != 0) )
          {
              errLogRet(LOGIT,debugInfo,"Spin Error: status is %d",rstat);

              setSpinInterlk(0);
              DPRINT1(0,"setSpinErrMode: %d, (1-Err,2-Warning)\n",errmode);
              //                                    getSpinErrMode());

             if (errmode == HARD_ERROR)  /* (15) Error */
	          {
	               /* disable all interlock tests, since this will 
                  * stop the exp, don't want continue error coming 
                  * back while no exp. is running 
                  */
		             setVTinterLk(0);
		             // setLKInterlk(0);
    		          /* send error to exception handler task */
                 sendException(HARD_ERROR, rstat, 0,0,NULL);
              }
              else  /* WARNING or no message */
              {
                  if (errmode)
                     sendException(WARNING_MSG, rstat, 0,0,NULL);
	          DPRINT1(1,"CHECKSPIN: start FIFO rstat=%d\n",rstat);
                  clearStart4ExpFlag();  // alow to start fifo again
                  semGive(pTheAcodeObject->pSemParseSuspend);
		        }
           }
	        else
           {
	          DPRINT1(1,"CHECKSPIN: give SEM dowait=%d\n",doWait);
             clearStart4ExpFlag();  // alow to start fifo again
             semGive(pTheAcodeObject->pSemParseSuspend);
	        }

        }
        break;

      case MRIUSERBYTE:
        {  int index;
           int *rtVar;
           ACODE_ID pAcodeId;
           int userbyte;

           semGive(pTheAcodeObject->pSemParseSuspend);

	   rngBufGet(pSyncActionArgs, (char*) &index, sizeof( int ));
	   rngBufGet(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
           rtVar = (int *) pAcodeId->pLcStruct;/*(int *) * &(acqReferenceData)*/
           userbyte = get_field(MASTER,aux_read);
           rtVar[index] = 0;

           switch(readuserbyte)
           {
                case 0: send2AllCntlrs(CNTLR_RTVAR_UPDATE, index, userbyte, 0, NULL);
                        break;
                case 1: if((userbyte&0x1)==0x1){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 2: if((userbyte&0x2)==0x2){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 3: if((userbyte&0x4)==0x4){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 4: if((userbyte&0x8)==0x8){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 5: if((userbyte&0x10)==0x10){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 6: if((userbyte&0x20)==0x20){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 7: if((userbyte&0x40)==0x40){rtVar[index]=0x1;assertWarningLine();}
                        break;
                case 8: if((userbyte&0x80)==0x80){rtVar[index]=0x1;assertWarningLine();}
                        break;
                default: sendException(HARD_ERROR, SFTERROR+READBIT, 0,0,NULL);
                         break;
           }
           resetShandlerPriority();

           // continue parsing the codes
        }
	break;

      default:
		/* Who Cares */
		break;
   }
}




/* check on presents of sample via the 3 methods available */
multiDetectSample(int err)
{
   int locked,sampdetected,speed,sampinmag;
   int count;
   /* sampdetected = SampleDetect(); */
   sampdetected = detectSample();
   locked = speed = 0;
   if (!sampdetected)
   {
      taskDelay(calcSysClkTicks(5000));  /* 5 sec,  taskDelay(60*5); */
      /* locked = LockSense(); */
      /* speed = (SpinValueGet()) / 10; */
      locked = lockSense();
      speed = (spinValueGet()) / 10;
      sampdetected = detectSample();
   }
   if (err)
   {
      /* Only do extra tests if a robot err was detected */
      count = 0;
      while ( ! sampdetected && (count < 5) )
      {
         taskDelay(calcSysClkTicks(1000));
         sampdetected = detectSample();
         DPRINT1(0, "multiDetectSample -   Sample: detected: '%s'\n",
         (sampdetected) ? "TRUE" : "FALSE");
         count++;
      }
   }
 
   
   sampinmag = ((locked == 1) || (speed > 0) || sampdetected );
   DPRINT2(0,
      "multiDetectSample -   Sample: detected: '%s',  locked: '%s'\n",
      (sampdetected) ? "TRUE" : "FALSE",
      (locked == 1) ? "TRUE" : "FALSE");
   DPRINT2(0,
      "multiDetectSample -   Sample: spinning speed(Hz): %d, In Mag: '%s'\n",
      speed, (sampinmag == 1) ? "YES" : "NO" );
   return(sampinmag);
}

getSampFromMagnet(ACODE_ID pAcodeId, long newSample, 
                  long *presentSample, int skipsampdetect)
{
   int hostcmd[4];
   int *ptr;
   int roboStat,sampinmag;
   int Status;
   long removedSamp;
   int  lcSample;
   int bytes;

   int sampleSpinRate;  /* ??? */



   /* If LC Sample (e.i. gilson sample handler) value then don't test for sample presents in magnet */
   if ( (newSample > 999) || (skipsampdetect != 0))
      lcSample = 1;
   else
      lcSample = 0;

   DPRINT2(0,"getSampFromMagnet: get Sample %lu from Magnet, Skip Sample Detect Flag: %d\n",
                  *presentSample,skipsampdetect);
   Status = 0;

   sampleSpinRate = pCurrentStatBlock->AcqSpinSet;

   /* check on presents of sample via the 3 methods available */
   if (lcSample != 1)
   {
      sampinmag = multiDetectSample(0);
      DPRINT1(0,"getSampFromMagnet -   Sample In Mag: '%s'\n",
      (sampinmag == 1) ? "YES" : "NO" );
   }

   /* Special tests for AS7600 robot */
   if (skipsampdetect == 2)
   {
      if ( *presentSample == 0 )
      {
         /* If unknown sample (0) check for sample present */
         sampinmag = multiDetectSample(0);
         if ( ! sampinmag )
         {
            *presentSample = -99;
            DPRINT(0,"getSampFromMagnet   Sample changed from 0 to -99\n");
         }
      }
      else if ( *presentSample == -99 )
      {
         /* If declared empty, do the quick check to confirm */
         if (detectSample())
         {
            *presentSample = 0;
            DPRINT(0,"getSampFromMagnet   Sample changed from -99 to 0\n");
         }
      }
   }
   /* remove sample only if new one needed */
   removedSamp = *presentSample; /* save this sample number */

      send2Expproc(CASE,GET_SAMPLE,(*presentSample), 0, NULL, 0);
      DPRINT(0,"==========>  Suspend getSampFromMagnet() \n");
#ifdef INSTRUMENT
      wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif

      bytes = msgQReceive(pRoboAckMsgQ, (char*) &roboStat, sizeof(int), WAIT_FOREVER);
      DPRINT1(0,"==========> Get Sample Complete: robo result: %d, Resume getSampFromMagnet()\n",roboStat);
      /* if roboStat == -99  aborted */

#ifdef NOW_USEING_MSGQ
      semTake(pRoboAckSem,WAIT_FOREVER);
      DPRINT(0,"==========>  Returned from RoboAckSem \n");
      rngBufGet(pSyncActionArgs,(char*) &roboStat,sizeof(roboStat));
#endif

    if ( ! lcSample)
    {
      /* Its an error if a sample was there and not removed */
      /* A. Start automation no sample, sample Zero,  sample change failure - Continue */
      /* B. Start automation Sample present, sample Zero, sample change failure- Fail. */
      /* C. Sample NOT zero, sample change failure - Fail. */
      /* if (roboStat && ((removedSamp != 0) || sampinmag)) */
      /* If the robo has an error then always assume the sample is in the magnet!!
         This  is 5.1 method an also suggested to be kept by Steve McKenna
         Possible enhance is to change the robot 501 error to 516 error if the sample
         was not redetected after robot failed to get it
      */
      if (roboStat && (removedSamp != 0))
      {
         /* This test could be done by Roboproc */
         Status = roboStat;
         sampleHasChanged = FALSE;
         /* incase eject is not off, do it now */
         /* SampleInsert(); */
         insertSample();

         /* check on presents of sample via the 3 methods available */
         sampinmag = multiDetectSample(roboStat);
         DPRINT1(0,"Failed to Get Sample -   Sample In Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         /* So robot failed to get sample & the sample is not detected
            this is a new errorcode  516
         */

         if ( (roboStat == ( SMPERROR + RTVNOSAMPLE )) && (!sampinmag) )
         {
            Status = SMPERROR + RTVNOSAMPNODETECT;
         }
         /* Assume worst case that sample is stuck in bore some where */
         /* reset present sample since the Eject set this to zero (X_interp) */
         *presentSample = removedSamp;
         errLogRet(LOGIT,debugInfo,
                  "getSampFromMagnet Error: status is %d",roboStat);
      }
      else
      {
         /* test to be sure sample got removed */
         /* check on presents of sample via the 3 methods available */
         sampinmag = multiDetectSample(0);
         DPRINT1(0,"getSampFromMagnet -   Sample In Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         if ( sampinmag )

         {
            DPRINT(0,"getSampFromMagnet: detected sample still present\n");
            Status = SMPERROR + SMPRMFAIL;
            sampleHasChanged = FALSE;
            *presentSample = removedSamp;
            errLogRet(LOGIT,debugInfo,
                  "getSampFromMagnet Error: status is %d",Status);
         }
         else
         {
            sampleHasChanged = TRUE;
            *presentSample = 0;
         }
      }

    }
    else        /* LC Sample */
    {
      if (roboStat && ((removedSamp != 0) || (skipsampdetect == 2)) )
      {
         Status = roboStat;
         sampleHasChanged = FALSE;
         /* Assume worst case that sample is still LC Probe */
         *presentSample = removedSamp;
      }
      else
      {
            sampleHasChanged = TRUE;
            *presentSample = (skipsampdetect == 2) ? -99 : 0;
      }
    }

   DPRINT2(0,
         "getSampFromMagnet: Done; Sample Now in Mag: %d, changed: '%s'\n",
               *presentSample, (sampleHasChanged == TRUE) ? "YES" : "NO" );
   return(Status);
}

int putSampIntoMagnet(ACODE_ID pAcodeId, long newSample, long *presentSamp,
                  int skipsampdetect, int spinActive, int bumpFlag)
{
   int hostcmd[4];
   int *ptr;
   int roboStat,locked,sampdetected,speed,sampinmag,cnt;
   int Status;
   int  lcSample;
   int bytes;

   DPRINT2(0,"putSampIntoMagnet: Sample %d tobe put into Magnet, Skip Sample Detect Flag: %d\n",
                newSample,skipsampdetect);

   Status = 0;

   /* If LC Sample (e.i. gilson sample handler) value then don't test for sample presents in magnet */
   if ( (newSample > 999) || (skipsampdetect != 0) )
      lcSample = 1;
   else
      lcSample = 0;

   /* only bother to get it if its NOT present */

   if ( !lcSample )
   {
     /* check on presents of sample via the 3 methods available */
     sampinmag = multiDetectSample(0);
     DPRINT1(0," Put only if Sample is NOT in Mag.   Sample In Mag: '%s'\n",
        (sampinmag == 1) ? "YES" : "NO" );
   }
   else
     sampinmag = 0;     /* just so the next test is passed */

   if ( !sampinmag )
   {
      /* astatus = chgsmp('L',mask,&(acode->status)); */
      send2Expproc(CASE,LOAD_SAMPLE,newSample, 0, NULL, 0);
      DPRINT(0,"==========> Suspend putSampIntoMagnet()\n");

#ifdef INSTRUMENT
      wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif


      bytes = msgQReceive(pRoboAckMsgQ, (char*) &roboStat, sizeof(int), WAIT_FOREVER);
      DPRINT1(0,"==========> Get Sample Complete: robo result: %d, Resume getSampFromMagnet()\n",roboStat);
      /* if roboStat == -99  aborted */

#ifdef NOT_USEING_SEM
      semTake(pRoboAckSem,WAIT_FOREVER);
      DPRINT(0,"==========>  Returned from RoboAckSem \n");
      rngBufGet(pSyncActionArgs,(char*) &roboStat,sizeof(roboStat));
      DPRINT1(0,"==========> Load Sample Complete: result: %d,  Resume putSampIntoMagnet()\n",roboStat);
#endif

   if ( !lcSample )
   {
      if (roboStat)
      {
         sampleHasChanged = FALSE;
         Status = roboStat;
         /* incase eject is not off, do it now */
         insertSample();
         /* SampleInsert(); */

         /* check on presents of sample via the 3 methods available */
         sampinmag = multiDetectSample(roboStat);
         DPRINT1(0,"ROBO Err, Sample is in Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         if ( sampinmag )
         {
            *presentSamp = (short) newSample;
            sampleHasChanged = TRUE;
         }
         errLogRet(LOGIT,debugInfo,
                     "putSampIntoMagnet: Robo Error: status is %d",roboStat);
      }
      else
      {
         /* check on presents of sample, if not there bump the air just incase it's stuck */
         for(cnt=0; cnt < 3; cnt++)
         {
            /* check on presents of sample via the 3 methods available */
            DPRINT(0,"putSampFromMagnet: check for Sample\n");
            sampinmag = multiDetectSample(1);
            DPRINT1(0,"putSampFromMagnet -   Sample In Mag: '%s'\n",
               (sampinmag == 1) ? "YES" : "NO" );
            if ((sampinmag) || (cnt == 2))
               break;
            if (bumpFlag)
            {  DPRINT(0,"putSampFromMagnet: Bump Air\n");
               bumpSample();
               /* SampleBump(); */
            }
            else
            {
               DPRINT(0,"putSampFromMagnet: NOT Bumping Air\n");
            }
         }
         /* if ( locked || sampdetected || speed ) */
         if ( sampinmag )
         {
            DPRINT(0,"==========> Load Sample: Sample Detected\n");
            sampleHasChanged = TRUE; /* successful completion */
            *presentSamp= (short) newSample;
            if (!spinActive)
            {
               int sampleSpinRate = pCurrentStatBlock->AcqSpinSet;
               /* setspin( (int) sampleSpinRate, (int) bumpFlag ); */
               spinValueSet( sampleSpinRate, bumpFlag );
            }
         }
         else   /* Ooops, no sample detected signal insertion error */
         {
            DPRINT(0,"==========> Load Sample: Sample NOT Detected\n");
            sampleHasChanged = FALSE;
            Status = SMPERROR + SMPINFAIL;
            *presentSamp= (short) newSample;    /* be safe and assume its in the magnet */
            errLogRet(LOGIT,debugInfo,
                        "putSampIntoMagnet Error: status is %d",roboStat);
         }
      }
    }
    else
    {
      if (roboStat)
      {
         sampleHasChanged = FALSE;
         Status = roboStat;
         errLogRet(LOGIT,debugInfo,
                     "putSampIntoMagnet: Robo Error: status is %d",roboStat);
      }
      else
      {
         int tmpsmp;

         sampleHasChanged = TRUE; /* successful completion */
         tmpsmp = *presentSamp;
         *presentSamp= newSample;
         if ( newSample && (skipsampdetect == 2) )
         {
            int max = 20;
            while ( !detectSample() && (max > 0) && !failAsserted )
            {
               taskDelay(calcSysClkTicks(1000));  /* 1 sec */
               if (max == 20)
                  DPRINT(0,"==> Load Sample: Waiting for Sample Detect\n");
               max--;
            }
            DPRINT3(0,"==> Load Sample: detect= %d after %d sec. aborted= %d\n",
                           detectSample(), 20-max, failAsserted);
            if ( !detectSample() && !failAsserted ) /* Try once more */
            {
               send2Expproc(CASE,REPUT_SAMPLE,newSample, 0, NULL, 0);
               DPRINT(0,"==========> Suspend reputSampIntoMagnet()\n");
               bytes = msgQReceive(pRoboAckMsgQ, (char*) &roboStat, sizeof(int), WAIT_FOREVER);
               DPRINT1(0,"==========> Reput Sample Complete: robo result: %d\n",roboStat);
               if (roboStat)
               {
                  sampleHasChanged = FALSE;
                  *presentSamp= tmpsmp;
                  Status = roboStat;
                  errLogRet(LOGIT,debugInfo,
                        "reputSampIntoMagnet: Robo Error: status is %d",roboStat);
               }
               else  /* wait a second time for sample to appear */
               {
                  *presentSamp= newSample;
                  max = 20;
                  while ( !detectSample() && (max > 0) && !failAsserted )
                  {
                     taskDelay(calcSysClkTicks(1000));  /* 1 sec */
                     if (max == 20)
                        DPRINT(0,"==> Load Sample: Waiting for Sample Detect\n");
                     max--;
                  }
                  if ( !detectSample() && !failAsserted ) /* Final failure */
                  {
                     send2Expproc(CASE,FAILPUT_SAMPLE,newSample, 0, NULL, 0);
                     DPRINT(0,"==========> Suspend failputSampIntoMagnet()\n");
                     bytes = msgQReceive(pRoboAckMsgQ, (char*) &roboStat,
                                            sizeof(int), WAIT_FOREVER);
                     DPRINT1(0,"==========> Failput Sample Complete: robo result: %d\n",roboStat);
                     sampleHasChanged = FALSE;
                     *presentSamp= -99;
                     Status = roboStat;
                     errLogRet(LOGIT,debugInfo,
                           "failputSampIntoMagnet: Robo Error: status is %d",roboStat);
                  }
               }
            }
         }
      }
    }
   }
   else
   {      DPRINT(0,"putSampIntoMagnet: detected sample still present\n");
      Status = SMPERROR + SMPRMFAIL;
      sampleHasChanged = FALSE;
      errLogRet(LOGIT,debugInfo,
                  "putSampIntoMagnet Error: status is %d",Status);
   }
   DPRINT2(0,
         "putSampIntoMagnet: Done; Sample %d in Magnet, Sample changed: '%s'\n",
         *presentSamp, (sampleHasChanged == TRUE) ? "YES" : "NO" );
   return(Status);
}


#ifdef NOT_ON_LOCKCNTLR
/*
 * strtAutoLock( mode, maxpower, maxgain )
 *
 *     Author: Greg Brissey    1/06/2005
 */
*strtAutoLock(int mode, int maxpower, int maxgain)
*{
*   ALOCK_MSG alkMsge;
*   int errorcode;
*   alkMsge.mode = mode;
*   alkMsge.maxpwr = maxpower;
*   alkMsge.maxgain = maxgain;
*   alkMsge.arg4 = sampleHasChanged;
*
*   /* do_autolock(int mode, int maxpower, int maxgain) */
*   /* send2Lock(LK_AUTOLOCK, mode, maxpower, ((double) maxgain), (double) sampleHasChanged); */
*
*   msgQSend(pMsgesToAutoLock, (char*) &alkMsge, sizeof(ALOCK_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
*
*/*
*   semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
*   rngBufGet(pSyncActionArgs,(char*) &errorcode,sizeof(errorcode));
**/
*   return(errorcode);
*}
#endif


