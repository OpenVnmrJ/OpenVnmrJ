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

/* #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#ifdef VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>

#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "rngLLib.h"
#include "timeconst.h"
#include "acodes.h" 
#include "stmObj.h"	/* different for SUN and vxworks */
#include "fifoObj.h"	/* different for SUN and vxworks */
#include "adcObj.h"	/* different for SUN and vxworks */
#include "autoObj.h"
#include "sibObj.h"
#include "pcsObj.h"
#include "tuneObj.h"
#include "lkapio.h"
#include "lock_interface.h"
#include "AParser.h"
#include "A_interp.h"
#include "hardware.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "config.h"
#include "hostAcqStructs.h"
#include "vtfuncs.h"
#include "mboxcmds.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef DUMMYINT
#define DUMMYINT 0
#endif

#ifndef DEBUG_ACQI_UPDT
#define  DEBUG_ACQI_UPDT 9
#endif

#ifndef DEBUG_ACODES
#define  DEBUG_ACODES 9
#endif

#ifndef DEBUG_APWORDS
#define  DEBUG_APWORDS 9
#endif

#define SSHA_INDEX  63		/* must match define in vwauto::mboxTasks.c  */
#define JHWSHIM     62		/*  dac mask & gate on flag for hwshimming */
	/* this is max shimdac value allowed, usu. 0-40, up to 63 */


extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pUpLinkIMsgQ;	/* MsgQ used between UpLinkerI and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;

extern FIFO_ID pTheFifoObject;
extern STMOBJ_ID pTheStmObject;
extern AUTO_ID pTheAutoObject;
extern TUNE_ID pTheTuneObject;
extern SIB_ID pTheSibObject;

extern RING_ID  pSyncActionArgs;  /* rngbuf for sync action function args */

extern STATUS_BLOCK currentStatBlock;

extern int InterpRevId;

/* Configuration */
extern struct 	_conf_msg {
	long	msg_type;
	struct	_hw_config hw_config;
} conf_msg;

/* These need to be put into a global structure */
/* int fifolpsize = 2048; */
extern int fifolpsize;

/* STM scans - Should go into acode object? */
/* unsigned long cur_scan_data_adr; */
/*unsigned long prev_scan_data_adr;*/

/* experiment */
static void (*fifoStuffRtn)();
static int implclpsz = 4;
/* static int implclpsz = 128; */

/* warning message */
EXCEPTION_MSGE acodeWarningException;

/* High Speed Lines */
static unsigned int curhslines[2] = {0,0}; 
				/* 0 = Current HS line state bits 0-26 */
				/* 1 = Optional 32 bits of HS lines	*/
static int opthslines_flag = 0;
static int sampleSpinRate = 0;

/* Small Angle Phase of channel for JPSG 10/26/99 */
static int smphase[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};  /* small angle phase for channel 0-9 */

/* Obliquing matrix for rotating imaging gradients in software */
static int oblgradmatrix[9] = {0,0,0,0,0,0,0,0,0};

extern int sampleHasChanged;	/* Global Sample change flag */
extern int adjLkGainforShim;

extern MSG_Q_ID pMsgesToHost;
   
extern unsigned short *getAcodeSet(
	char *expname,
	int cur_acode_set,
	int interactive_flag,
        int timeVal
);
extern int rmAcodeSet(char *expname, int cur_acode_set);
extern int markAcodeSetDone(char *expname, int cur_acode_set);
extern testsendstm(int ct, int nt, int tag2snd);

/* testing */
extern int createFifofile(char *id);
extern fifoWriteIt(int FifoId, unsigned long *fifowords, int num);
/* extern fifoStuffIt(int FifoId, unsigned long *fifowords, int num); */
extern testsendstm(int ct, int nt, int tag2snd);
extern tstacquire(short flags, int np, int ct, int fid_num, long *stmaddr);

/******* Specials for multiple rcvr interleave *******/
extern void initscan2();
extern void nextscan2();
extern void endofscan2();
/******* END Specials for multiple rcvr interleave *******/

int jsetphase(ushort apdelay, ushort apaddr,long phase[], ushort channel );
int jsetphase90(ushort apdelay, ushort apaddr,long phase[], ushort channel );
int jsetlkdecphase90(ushort apdelay, ushort apaddr,long phase, ushort channel );

static queueSamp(int action, unsigned long sample2ChgTo, int skipsampdetect, ACODE_ID pAcodeId, short bumpFlag);
static queueLoadSamp(int action, unsigned long sample2ChgTo, int skipsampdetect,
                     ACODE_ID pAcodeId, short spinActive, short bumpFlag);

static int ssha_activated = 1;
static int ssha_started = 0;

/* must be outside APint() this autogain,etc recursively call APint()
   and pParallelIBufs is setup prior to this */
static RINGL_ID pParallelIBufs = NULL;	/* free buffers of parallel input */

#ifdef VXWORKS
#define getuint(V) *(unsigned int *)(V)

#else
getuint(unsigned short *var)
{
 union {
   unsigned int ival;
   unsigned short sval[2];
 } temp;
   temp.sval[0] = var[0];
   temp.sval[1] = var[1];
 return(temp.ival);
}
#endif


/*  phandler.c needs this  */

activate_ssha()
{
	ssha_activated = 1;
}

static
deactivate_ssha()
{
	ssha_activated = 0;
}

static
reset_ssha()
{
	setshimviaAp( SSHA_INDEX, 2 ); /* this causes output at end?? */
}

static int
is_ssha_activated()
{
	return( ssha_activated );
}

static int
has_ssha_started()
{
	return( ssha_started );
}

/*  Note:  A number of DPRINTn calls have debug level set to 9.
           The idea is these proved useful in the past, but I
           do not want them cluttering up the display.  If you
           want to turn them on, the suggestion is to change
           the debug level to a lower number, say 0 and recompile.  */

/*-------------------------------------------------------------
| Interrupt Service Routines (ISR) 
+--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*								*/
/* fifoSW1aupdt - Interrupt Service Routine signaling the end	*/
/*		of interactive updates.				*/
/*								*/
/* RETURNS:							*/
/*  void							*/
/*--------------------------------------------------------------*/
void fifoSW1aupdt(ACODE_ID pAcodeId)
{
     DPRINT1(0,"FIFO IST:  SW1aupdt, pAcodeId=0x%lx\n",pAcodeId);
#ifdef INSTRUMENT
     wvEvent(EVENT_INTRP_UPDATE,NULL,NULL);
#endif
     semGive(pAcodeId->pSemParseUpdt);
}

/*****************************************************************
* fifoSW2ISR - ISR for Fifo SW Interrupt 2
*
*  Now this interrupt signals a Stop Acquisition if enabled.
*
* Since the FIFO SW interrupt is dedicated, this ISR is registered in
* systemInit.c  (this avoids possible memory leaks from multiply registration)
*
*				Author: Greg Brissey  5-17-95
*/
void fifoSW2ISR()
{
#ifdef INSTRUMENT
     wvEvent(EVENT_STM_PSEUDO_CT,NULL,NULL);
#endif
  DPRINT(0,"FIFO IST:  SW2 Stop Acquisition.\n");
  iscPseudoCT(pTheStmObject);
}


/*************************************************
*
* A_interp.c - the ACODE INTERPRETER 
*
*
**************************************************/

static int APint_run;
static int AP_Suspend = FALSE;

void abortAPint()
{
#ifdef INSTRUMENT
     wvEvent(EVENT_PARSE_STOP,NULL,NULL);
#endif
    APint_run = FALSE;
    AP_Suspend = TRUE;
}

void resetAPint()
{
#ifdef INSTRUMENT
     wvEvent(EVENT_PARSE_STOP,NULL,NULL);
#endif
    AP_Suspend = FALSE;
}

int APSuspendVal()
{
    return(AP_Suspend);
}

void stopAPint()
{
#ifdef INSTRUMENT
     wvEvent(EVENT_PARSE_STOP,NULL,NULL);
#endif
   APint_run = FALSE;
}

/* nulls the parallel channel FreeBuf pointer */
void clearParallelFreeBufs()
{
   pParallelIBufs = NULL;
}

/******************************************************
*
*   A T T E N T I O N  !!!!!!
*
*   Did You Add/Change any Acode Arguments ?
*     The besure to increment the INOVA_INTERP_REV define in REV_NUMS.h
*     Besure to check the Lock Display Acodes in AParser.c for affect acodes
*
********************************************************/

int APint( ACODE_ID pAcodeId )
{
    unsigned short *ac_cur,*ac_base,*ac_end,*ac_stack[10]; /* acode pointers */
   unsigned short *ac_start;
   long *rt_base, *rt_tbl;	/* pointer to realtime buffer addresses */
   unsigned long *rtv_ptr;
   short *acode;		/* acode location */
   short alength;	/* length of acode values in short words*/
   short opcode, a, b, c;
   short lkfreqapvals[ 2 ];
   int index, tmp, status;
   long loc_npsize, loc_tot_elem;
   int  p2ap_port;
   int i,retrys;
   char *elem_addr;
   char tmpbufname[64];
   /* RINGL_ID pParallelIBufs;	/* free buffers of parallel input */
   PCHANSORT_ID pcsId;		/* Parallel Sorting Object */
   int parallelstart[20];
   int parallelindex = -1;

   void signal_syncop(int tagword,long secs,long ticks);

   pcsId = NULL;
   adjLkGainforShim = 0;
   status = 0;
   ssha_started = 0;
   ac_base = pAcodeId->cur_acode_base;
   rt_base = (long *) pAcodeId->cur_rtvar_base;
   rt_tbl=rt_base+1;
   
   /* init grad angle matrix */
   clearOblGradMatrix();
   

   /* PARSER */
   ac_cur = ac_start = ac_base+1;
   APint_run = TRUE;
   AP_Suspend = FALSE;
   /* autogain, make correction in jump base */
   if (pAcodeId->interactive_flag == ACQ_AUTOGAIN)
       ac_base = pAcodeId->cur_jump_base;

   if (pAcodeId->interactive_flag != ACQ_AUTOGAIN)
      resetParserSem(pAcodeId);

   while (APint_run == TRUE)
   {
      acode = ac_cur++;
      alength = *ac_cur++;
      DPRINT2( DEBUG_ACODES, "acode: %d, length: %d\n", *acode, alength );
      DPRINT4( 1, "acode[%ld]: %d, length: %d, address: 0x%x\n",
		  (ac_cur-ac_start),*acode, alength, acode );
#ifdef INTERP_INSTRUMENT
#ifdef INSTRUMENT
     wvEvent(EVENT_PARSE_ACODE,acode,sizeof(short));
#endif
#endif
      switch (*acode)
      {
	/* INTERP_REV_CHK						*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - Interpreter revision id					*/
	/*  2 - spare							*/
	case INTERP_REV_CHK: 
	   DPRINT1(2,"INTERP_REV_CHK : %d\n",*ac_cur);
	   if ((int)(*ac_cur) != InterpRevId)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + PSGIDERROR;
    		errLogRet(LOGIT,debugInfo,
		 "INTERP_REV_CHK Error: %d expected: %d.",(int)(*ac_cur), 
							InterpRevId);
	   }
	   ac_cur+=2;
	   break;

	/* INIT_STM							*/
	/*  arguments: 							*/
	/*  0 - short word count: 5					*/
	/*  1 - rtoffset to number of data points			*/
	/*  2 - rtoffset to precision, double or single (in bytes)	*/
	/*  3 - rtoffset to total number of data elements including	*/
	/*		blocksize elements.				*/
	/*  4 - Flag indicating to use High Speed STM/ADC (5MHz) 	*/
	/*  5 - rtoffset to active receivers.			 	*/
	case INIT_STM: 
           {
              int SwGT500KHz, active,i;
              long *activeRcvrs;
   	      int rstat;
              STMOBJ_ID pStmId;
	      DPRINT5(1,"INIT_STM (%lu, %lu, %lu, %d, 0x%x)\n",
		      (ulong_t)(rt_tbl[*ac_cur]),
		      (ulong_t)(rt_tbl[*(ac_cur+1)]),
		      (ulong_t)(rt_tbl[*(ac_cur+2)]),
		      *(ac_cur+3),
		      (ulong_t)(rt_tbl[*(ac_cur+4)]));

              SwGT500KHz = *(ac_cur+3);
              activeRcvrs = &rt_tbl[*(ac_cur+4)];

              /* iterate though all Stm Objects, if active and not capable
		 of 'sw' then disable it, If no active stm are left then check
		 last stm to (1 or 5MHz STM/ADC) and activate if present */
              for (i=0; i<MAX_STM_OBJECTS; i++)
              {   
                  pStmId=stmGetStmObjByIndex(i);
                  active = stmIsActive(activeRcvrs, i);
                  if ((pStmId == NULL) && active) {
                     APint_run = FALSE;
		     status = HDWAREERROR + TOOFEWRCVRS;
    		     errLogRet(LOGIT,debugInfo,
		     "INIT_STM Error: Requested STM %d, but handle is NULL\n",
					i);
		     ac_cur += 5;
		     break; /* from for-loop */
                  }
                  DPRINT4(1,"INIT_STM: index=%d, pStmObj: 0x%lx, type: %d, active: %d\n",
	                      i,pStmId,pStmId->stmBrdVersion,active);
                  if (active)
                  {
	              if ( (SwGT500KHz>0) && stmIsStdStmObj(pStmId) )
	                 stmSetInactive(activeRcvrs, i);
                  }
              }
              if ( ! APint_run) break; /* from case: */

              DPRINT2(1,"INIT_STM: SwGT500KHz: %d, activeRcvrs: 0x%lx\n",SwGT500KHz,*activeRcvrs);
	      if ((SwGT500KHz != 0) && (*activeRcvrs == 0) )
	      {
                 if ((rstat = stmGetHsStmObj(&pStmId)) != 0)
                 {
	           APint_run = FALSE;
                    /* rstat = HDWAREERROR+NOHSDTMADC; */
	           status = rstat;
    	           errLogRet(LOGIT,debugInfo,
			     "High Speed DTM/ADC Not Present: errorcode is %d",
			     rstat);
                 }
	         else
                 {
		     i = stmGetHsStmIdx();
		     rt_tbl[*(ac_cur+4)] = 1 << i; /* Set active rcvr mask */

		     /* DPRINT1(-11,"INIT_STM: HS recvr channel index: %d\n",i); */
		     /*DPRINT2(-11,"INIT_STM: HS recvr channel index: %d, act recvr mask: 0x%x\n",
			   i,rt_tbl[*(ac_cur+4)]); */
                 }
              }
	      i = -1;
	      /* Default STM is the first active one */
	      pTheStmObject = stmGetActive(&rt_tbl[*(ac_cur+4)], &i);
	      DPRINT2(1,"INIT_STM: pTheStmObject = stmList[%d] = 0x%lx\n",i,pTheStmObject);

	      init_stm(&rt_tbl[*(ac_cur+4)], pAcodeId,
		       (ulong_t)(rt_tbl[*ac_cur]),
		       (ulong_t)(rt_tbl[*(ac_cur+1)]),
		       (ulong_t)(rt_tbl[*(ac_cur+2)]), *(ac_cur+3) );
	      ac_cur+=5;
           }
	   break;

	/* INIT_HS_STM							*/
	/*  arguments: 							*/
	/*  None1 							*/
	case INIT_HS_STM: 
           DPRINT(-1,"INIT_HS_STM, Obsolete\n");
	   break;

	/* INIT_FIFO							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - flags: 							*/
	/*  2 - spare							*/
	case INIT_FIFO:
	   /* init_fifo(char *id, short flags, short size) */
	   if (init_fifo(pAcodeId,*(ac_cur),*(ac_cur+1)) < 0)
	   {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
	   }
	   ac_cur+=2;
	   break;

	/* INIT_ADC							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/*  1 - flags: 							*/
	/*  2 - spare 							*/
	/*  3 - rtoffset to active receivers.				*/
	case INIT_ADC:
	  init_adc(&rt_tbl[*(ac_cur+2)], *(ac_cur), &rt_tbl[*(ac_cur+1)]);
	  ac_cur+=3;
	  break;

	case FIFOSTART:
	   fifoStart(pTheFifoObject);
	   break;

	case FIFOSTARTSYNC:
	   fifoStartSync(pTheFifoObject);
	   break;

	case FIFOWAIT4STOP:
	   fifoWait4Stop(pTheFifoObject);
	   break;

/*  fifo wait for stop with interrupt.
    This A-code was created in connection with the lock display.  The analysis
    is that the FIFO was reported as stopped, because it had never started,
    since in the lock display we start the FIFO on sync, not necessarily
    concurrent with the FIFOSTART a-code.  By waiting for an interrupt, the
    A-code interpreter insures that the FIFO actually started.  (The FIFO
    object takes a semaphore, which is given by the interrupt.)			*/

	case FIFOWAIT4STOP_2:
	   fifoWait4StopItrp(pTheFifoObject);
	   DPRINT(9,"FIFOWAIT4STOP_2 completes... \n");
	   break;

	case FIFOHALT:
	   /* fifoHaltop(pTheFifoObject); */
	   DPRINT(2,"FIFOHALT... \n");
	   fifoStuffCmd(pTheFifoObject,HALTOP,0);
	   break;

	case FIFOHARDRESET:
	   DPRINT(2,"FIFOHARDRESET... \n");
	   fifoReset(pTheFifoObject, RESETFIFO);
	   break;

	case CLRHSL:
	   DPRINT(2,"CLRHSL... \n");
	   fifoClrHsl(pTheFifoObject,STD_HS_LINES);
	   break;

	case LOADHSL:
	   DPRINT1(2,"LOADHSL : 0x%lx\n",getuint(ac_cur));
	   fifoLoadHsl(pTheFifoObject,STD_HS_LINES, getuint(ac_cur));
	   ac_cur += 2;
	   break;

	case MASKHSL:
	   DPRINT1(2,"MASKHSL : 0x%lx\n",getuint(ac_cur));
	   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, getuint(ac_cur));
	   ac_cur += 2;
	   break;

	case UNMASKHSL:
	   DPRINT1(2,"UNMASKHSL : 0x%lx\n",getuint(ac_cur));
	   fifoUnMaskHsl(pTheFifoObject,STD_HS_LINES, getuint(ac_cur));
	   ac_cur += 2;
	   break;

	case SAFEHSL:
	   DPRINT1(2,"SAFEHSL : 0x%lx\n",getuint(ac_cur));
	   fifoSafeHsl(pTheFifoObject,STD_HS_LINES, getuint(ac_cur));
	   ac_cur += 2;
	   break;

	case PAD_DELAY:
        {
           int len;

           len = alength - 1;
	   DPRINT2(2,"PAD_DELAY ct index= %d val = %d\n",
                      *ac_cur, (int) rt_tbl[ *ac_cur ]);
           if ( (int) rt_tbl[ *ac_cur++ ] == 0)
           {
      	      update_acqstate((pAcodeId->interactive_flag == ACQI_INTERACTIVE) ?
						ACQ_INTERACTIVE : ACQ_PAD);
              while (len-- > 0)
              {
                 if (*ac_cur++ == EVENT1_DELAY)
                 {
	            delay( 0, getuint(ac_cur));
                    len -= 2;
	            ac_cur += 2;
                 }
                 else
                 {
	            delay( getuint(ac_cur), (unsigned long)getuint(ac_cur+2));
                    len -= 4;
	            ac_cur += 4;
                 }
              }
           }
           else
           {
	      ac_cur += len;
           }
        }
	   break;

	case LOADHSL_R:
	   DPRINT(2,"LOADHSL_R \n");
	   fifoLoadHsl(pTheFifoObject,STD_HS_LINES, rt_tbl[*ac_cur++]);
	   break;

	case MASKHSL_R:
	   {
	      int stateindx,hslineindx;
	      DPRINT(2,"MASKHSL_R \n");
	      hslineindx = *ac_cur++;
	      stateindx = *ac_cur++;
	      if (rt_tbl[stateindx])
	         fifoMaskHsl(pTheFifoObject,STD_HS_LINES, rt_tbl[hslineindx]);
	      else
	         fifoUnMaskHsl(pTheFifoObject,STD_HS_LINES, rt_tbl[hslineindx]);
	   }
	   break;

	case UNMASKHSL_R:
	   break;

	/* EVENT1_DELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - #ticks with 10ns/tick (2**31 ??)	(int)		*/
	case EVENT1_DELAY:
	   DPRINT(2,"EVENT1_DELAY... \n");
	   delay( 0, getuint(ac_cur));
	   ac_cur += 2;
	   break;

	/* EVENT2_DELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - #seconds 	(int)					*/
	/*  2 - remainder of a second in 10 ns ticks 	(int)		*/
	case EVENT2_DELAY:
	   DPRINT(2,"EVENT2_DELAY... \n");
	   delay( getuint(ac_cur), (unsigned long)getuint(ac_cur+2));
	   ac_cur += 4;
	   break;

	/* DELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - rtoffset to pointer to delay in table 			*/
	case DELAY: 
	   DPRINT(2,"DELAY... \n");
	   rtv_ptr = (unsigned long *)rt_tbl[*ac_cur++];
	   delay( (int) *rtv_ptr, (unsigned long) *(rtv_ptr+1) );
	   break;

	/* JDELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - rtoffset to pointer to delay in table 			*/
	/*  2 - count: 1 or 2						*/
	case JDELAY: 
	   DPRINT(2,"JDELAY... \n");
	   if ( *(ac_cur+1) == 2) 
	        delay( (int)rt_tbl[*ac_cur], (unsigned long) rt_tbl[*ac_cur+1] );
	   else
	        delay( (int)0, (unsigned long) rt_tbl[*ac_cur] );
	   ac_cur+=2;
	   break;

	/* VDELAY							*/
	/*  Similar to VGRADIENT, but with no boundry checking		*/
	/*  arguments: 							*/
	/*  0 - short word count: 5					*/
	/*  1 - rtoffset to value 					*/
	/*  2 - increment in ticks (long)		 		*/
	/*  3 - base offset in ticks(long)		 		*/
	case VDELAY: 
	   DPRINT3(2,"VDELAY: RTtabIndex: %d increment: %d offset: %d\n",
			*ac_cur,getuint(ac_cur+1),getuint(ac_cur+3));
	   vdelay(rt_tbl[*ac_cur], getuint(ac_cur+1),getuint(ac_cur+3) );
	   ac_cur += 5;
	   break;

	/* INITDELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 5					*/
	/*  1 - rtoffset to place delay words in rttable		*/
	/*  2 - #seconds 	(int)					*/
	/*  3 - remainder of a second in 10 ns ticks 	(int)		*/
	case INITDELAY:
	   DPRINT3(2,"INITDELAY: RTtabIndex: %d seconds: %d ticks: %d\n",
			*ac_cur,getuint(ac_cur+1),getuint(ac_cur+3));
	   rt_tbl[*ac_cur] = getuint(ac_cur+1);
	   rt_tbl[*ac_cur + 1] = getuint(ac_cur+3);
	   ac_cur += 5;
	   break;

	/* INCRDELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - rtoffset to get init delay words in rttable		*/
	/*  2 - multiplier 	(short)					*/
	case INCRDELAY:
	   DPRINT2(2,"INCRDELAY: RTtabIndex: %d multiplier: %d\n",
					*ac_cur,*(ac_cur+1));
	   incrdelay(&rt_tbl[ *ac_cur], (long)*(ac_cur+1));
	   ac_cur += 2;
	   break;

	/* INCRDELAY_R							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - rtoffset to get init delay words in rttable		*/
	/*  2 - rtoffset to multiplier 					*/
	case INCRDELAY_R:
	   DPRINT2(2,"INCRDELAY: RTtabIndex: %d multiplier: %d\n",
					*ac_cur,rt_tbl[*(ac_cur+1)]);
	   incrdelay(&rt_tbl[ *ac_cur], rt_tbl[*(ac_cur+1)]);
	   ac_cur += 2;
	   break;

	/* SETPHASE90_R							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - table index		 				*/
	/*  2 - rtoffset to phase90					*/
	case SETPHASE90_R:
        {
           int len,chan,index;

           len = *ac_cur++;
           chan = *ac_cur++;
           if (len & 0x8)
           {
              unsigned short *iptr;

              len >>= 4;
              index = rt_tbl[ *ac_cur++ ] % len;
              iptr = ac_cur;
              ac_cur += (len + 7) / 8;
              iptr += index / 8;
              len = index % 8;
              len <<= 1;
              index = *iptr;
              index = (index >> len) & 3;
           }
           else
           {
              ac_cur++; /* not needed for this case */
              index = rt_tbl[ *ac_cur++ ];
           }
	   DPRINT1(2,"SETPHASE90_R to %d\n",index);
	   if (setphase90(pAcodeId,chan,index) != 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "setphase90: Invalid table index: %d.",chan);
	   }
	   break;
        }

	case SETLKDECPHAS90:		/* Lock/Decoupler board phase */
        {
           int len,chan,index;
	   int setlkdecphase90(ACODE_ID pAcodeId, short tbl, int incr);

	   DPRINT(2,"SETLKDECPHAS90... \n");

           len = *ac_cur++;
           chan = *ac_cur++;
           if (len & 0x8)
           {
              unsigned short *iptr;

              len >>= 4;
              index = rt_tbl[ *ac_cur++ ] % len;
              iptr = ac_cur;
              ac_cur += (len + 7) / 8;
              iptr += index / 8;
              len = index % 8;
              len <<= 1;
              index = *iptr;
              index = (index >> len) & 3;
           }
           else
           {
              ac_cur++; /* not needed for this case */
              index = rt_tbl[ *ac_cur++ ];
           }
	   DPRINT1(2,"SETLKDECPHAS90 to %d\n",index);
	   if (setlkdecphase90(pAcodeId,chan,index) != 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "setphase90: Invalid table index: %d.",chan);
	   }
	   break;
        }

	case JSETLKDECPHAS90:		/* Lock/Decoupler board phase */
        {
           int len,chan,index;
	   int setlkdecphase90(ACODE_ID pAcodeId, short tbl, int incr);

	   DPRINT(2,"JSETLKDECPHAS90... \n");

           /* len = *ac_cur++; */
           /* int jsetlkdecphase90(ushort apdelay, ushort apaddr,long phase[], ushort channel ) */
	   if (jsetlkdecphase90(*ac_cur,*(ac_cur+3),rt_tbl[*(ac_cur+1)],*(ac_cur+2)) != 0)
	   {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "setphase90: Invalid table index: %d.",chan);
	   }
           ac_cur += 4;
	   break;
        }

	case JSETLKDECATTN:		/* Lock/Decoupler board power/attn */

	  DPRINT4(1,"JSETLKDECATTN: apaddr: 0x%x, value: %d, apdelay: %d, channel: %d\n",
	    *(ac_cur+3), rt_tbl[*(ac_cur+1)],*ac_cur,*(ac_cur+2));

          
	  /* just initialize values to be set later (i.e. 2 ) */
          /* setLockDecPower( apaddr, decpower, apdelay, 0 );   */
          setLockDecPower( *(ac_cur+3), rt_tbl[*(ac_cur+1)], *ac_cur, 0 );  
          ac_cur += 4;

          break;

	case SETLKDECATTN:
        {
          int apaddr,decpower,apdelay;

          apaddr = *ac_cur++;
          decpower = *ac_cur++;
          apdelay = *ac_cur++;

	  DPRINT3(1,"SETLKDECATTN: apaddr: 0x%x, value: %d, apdelay: %d\n",
	    apaddr, decpower,apdelay);

          
	  /* just initialize values to be set later (i.e. 2 ) */
          setLockDecPower( apaddr, decpower, apdelay, 0 ); 

          break;
        }

	case SETLKDEC_ONOFF:		/* Lock/Decoupler board, Decoupler On/Off */
        {
          int apaddr,decOn,apdelay;
          apaddr = *ac_cur++;
          decOn = *ac_cur++;
          apdelay = *ac_cur++;
	  DPRINT3(1,"SETLKDEC_ONOFF: apaddr: 0x%x, value: %d, apdelay: %d\n",
	    apaddr, decOn,apdelay);

	  setlkdecOnOff(apaddr,decOn,apdelay);
          
          break;
        }

	/* PHASESTEP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - table index		 				*/
	/*  2 - phase stepsize value					*/
	case PHASESTEP:
	{
	   long *tbl_addr;
	   DPRINT(2,"PHASESTEP... \n");
	   tbl_addr = (long *)tbl_element_addr(pAcodeId->table_ptr,*ac_cur++,0);
	   if (tbl_addr == NULL) {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "PHASESTEP: Invalid table index: %d.",*(ac_cur-1));
		break;
	   }
	   tbl_addr[IPHASESTEP] = (long) *ac_cur++;
	   tbl_addr[IPHASEQUADRANT] = 0;
	   break;
	}

	/* SETPHASE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - table index		 				*/
	/*  2 - phase increment						*/
	case SETPHASE:
	   DPRINT(2,"SETPHASE90... \n");
	   /* setphase(*ac_cur & 0xff,(*ac_cur >> 8)& 0xff, *(ac_cur+1)); */
	   if (setphase(pAcodeId,*ac_cur & 0xff, *(ac_cur+1)) != 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "setphase: Invalid table index: %d.",*ac_cur);
	   }
	   ac_cur += 2;
	   break;

	/* SETPHASE_R							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - table index		 				*/
	/*  2 - rtoffset to phase increment				*/
	case SETPHASE_R:
	   DPRINT(2,"SETPHASE90... \n");
	   /* setphase(*ac_cur&0xff,(*ac_cur>>8)&0xff,rt_tbl[*(ac_cur+1)]); */
	   if (setphase(pAcodeId,*ac_cur & 0xff, rt_tbl[*(ac_cur+1)]) != 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
			  "setphase: Invalid table index: %d.",*ac_cur);
	   }
	   ac_cur += 2;
	   break;

	/* RECEIVERGAIN							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/*  1 - apaddr							*/
	/*  2 - apdelay							*/
	/*  3 - rtoffset to gain					*/
	case RECEIVERGAIN:
	   DPRINT1(2,"RECEIVERGAIN.. set to %d\n", rt_tbl[*(ac_cur+2)] );
	   receivergain(*ac_cur, *(ac_cur+1), rt_tbl[*(ac_cur+2)]);
	   ac_cur+=3;
	   break;

	case GETSAMP:
          {
              short bumpFlag;
              int ret,lsw,hsw,skipsampdetect;
              unsigned long sample2ChgTo;
	      /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
		 roboproc needs
                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack
              */

	      /* Aaah you say why don't you just get it as a long? Because psg puts the codes in as shorts
                 and a long might not be aligned on the proper boundard (BUSS ERROR) , PSG has the same
		 problem placing a long into the acode stream can lead to a bus error because of misalignment
		 of the long word (Ok Frits)
              */
	      lsw = *ac_cur++;
	      hsw = *ac_cur++;
	      skipsampdetect = (int) *ac_cur++;
	      bumpFlag = *ac_cur++;
              DPRINT2(0,"GETSAMP: lsw: %u, hsw: %u \n",lsw,hsw);
              sample2ChgTo = ((hsw << 16) & 0xffff0000) | (lsw & 0xffff);
   	      if ( (sample2ChgTo != currentStatBlock.stb.AcqSample) && 
		   ( ((currentStatBlock.stb.AcqSample % 1000) != 0) ||
                      (skipsampdetect == 0) || (skipsampdetect == 2) ))
   	      {
                queueSamp(GETSAMP,sample2ChgTo,skipsampdetect,pAcodeId,bumpFlag);
	        DPRINT(0,"==========> GETSAMP: Resume Parser\n");
	      }
	      else
	      {
	        DPRINT2(0,"sample2ChgTo (%d) == PresentSample (%d), GETSAMP Skipped\n",
				sample2ChgTo,currentStatBlock.stb.AcqSample);
	      }
          }
	  break;


	case JGETSAMP:
          {
              short bumpFlag;
              int skipsampdetect;
              unsigned long sample2ChgTo;
	      /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
		 roboproc needs
                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack
              */

	      sample2ChgTo = (unsigned long) rt_tbl[*ac_cur++];
	      skipsampdetect = (int) rt_tbl[*ac_cur++];
              bumpFlag = *ac_cur++;
              DPRINT3(0,"JGETSAMP: sample2Chng: %ld, skipSampDetect: %d, bumpFlag: %d\n",
		sample2ChgTo,skipsampdetect,bumpFlag);

   	      if ( (sample2ChgTo != currentStatBlock.stb.AcqSample) && 
		   ( ((currentStatBlock.stb.AcqSample % 1000) != 0) ||
                      (skipsampdetect == 0) || (skipsampdetect == 2) ))
   	      {
                queueSamp(GETSAMP,sample2ChgTo,skipsampdetect,pAcodeId,bumpFlag);
	        DPRINT(0,"==========> GETSAMP: Resume Parser\n");
	      }
	      else
	      {
	        DPRINT2(0,"sample2ChgTo (%d) == PresentSample (%d), GETSAMP Skipped\n",
				sample2ChgTo,currentStatBlock.stb.AcqSample);
	      }
          }
	  break;


	case LOADSAMP:
          {
              short bumpFlag,spinActive;
              int ret,lsw,hsw,skipsampdetect;
	      unsigned long sample2ChgTo;

	      /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
		 roboproc needs
		   sampleloc is an encoded number

                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack
		        
              */

	      /* Aaah you say why don't you just get it as a long? Because psg puts the codes in as shorts
                 and a long might not be aligned on the proper boundard (BUSS ERROR) , PSG has the same
		 problem placing a long into the acode stream can lead to a bus error because of misalignment
		 of the long word (Ok Frits)
              */
	      lsw = *ac_cur++;
	      hsw = *ac_cur++;
	      skipsampdetect = *ac_cur++;
              spinActive = *ac_cur++;
              bumpFlag = *ac_cur++;
              DPRINT2(0,"GETSAMP: lsw: %u, hsw: %u \n",lsw,hsw);
              sample2ChgTo = ((hsw << 16) & 0xffff0000) | (lsw & 0xffff);

   	      if ( ((sample2ChgTo % 1000) != 0) )
   	      {
                queueLoadSamp(LOADSAMP,sample2ChgTo,skipsampdetect,pAcodeId,
                              spinActive,bumpFlag);
              }
	      else
              {
	        DPRINT2(0,"sample2ChgTo (%d), loc (%d) == 0, LOADSAMP Skipped\n",
				sample2ChgTo, (sample2ChgTo % 1000));
              }

          }
	  break;


	case JLOADSAMP:
          {
              short bumpFlag,spinActive;
              int ret,lsw,hsw,skipsampdetect;
	      unsigned long sample2ChgTo;

	      /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
		 roboproc needs
		   sampleloc is an encoded number

                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack
		        
              */

	      sample2ChgTo = (unsigned long) rt_tbl[*ac_cur++];
	      skipsampdetect = (int) rt_tbl[*ac_cur++];
              spinActive = *ac_cur++;
              bumpFlag = *ac_cur++;
              DPRINT3(0,"JLOADSAMP: sample2Chng: %ld, skipSampDetect: %d, bumpFlag: %d\n",
		sample2ChgTo,skipsampdetect,bumpFlag);

   	      if ( ((sample2ChgTo % 1000) != 0) )
   	      {
                queueLoadSamp(LOADSAMP,sample2ChgTo,skipsampdetect,pAcodeId,
                              spinActive,bumpFlag);
              }
	      else
              {
	        DPRINT2(0,"sample2ChgTo (%d), loc (%d) == 0, LOADSAMP Skipped\n",
				sample2ChgTo, (sample2ChgTo % 1000));
              }
	      DPRINT(0,"==========> JLOADSAMP: Resume Parser\n");

          }
	  break;


	case LOADSHIM:
	   DPRINT(2,"LOADSHIM... \n");
           if ( (int) rt_tbl[ *ac_cur++ ] == 0)
           {
              int index;
              short dacs[8];
              int num = *ac_cur++;
              dacs[0]=0;  /* Ingored */
#ifdef TODO
              for (index = Z0 + 1; index < num; index++)
#endif
              for (index = 2; index < num; index++)
              {
                 dacs[1]=(short) index; /* Z0 DAC */
                 dacs[2]=(short) *ac_cur++;
	         DPRINT2(3,"Shim %d = %d\n", (int) dacs[1], (int) dacs[2]);
                 shimHandler( dacs, 3 );
              }
           }
           else
           {
              int num = *ac_cur++;
	      ac_cur += num;
           }
	   break;

	case SETSHIMS:
	   DPRINT1(2,"SETSHIMS... num: %d  \n",*ac_cur);
           {
	      int i;
              short dacs[3];
              int num = *ac_cur++;
              dacs[0]=0;  /* Ingored */
              for (i=0; i<num; i++)
              {
              	dacs[1]=(short) *ac_cur++; /* DAC num */
              	dacs[2]=(short) *ac_cur++; /* value */
	      	DPRINT2(2,"Shim %d = %d\n", (int) dacs[1], (int) dacs[2]);
              	shimHandler( dacs, 3 );
	      }
           }
	   break;


	/* SETSHIM_AP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - dac number	 					*/
	/*  2 - dac value	 					*/
	case SETSHIM_AP:
           {
	   /* setshimviaAp(int dac, int value) */
	      int dac,value;
	      dac = *ac_cur++;
	      value = *(short*)ac_cur++;
	      DPRINT2(2,"SETSHIM_AP... dac: %d, value: %d  \n",dac,value);
	      /*  setshimviaAp(int dac, int value) */
	      if (setshimviaAp(dac,value) == -1)
	      {
	        DPRINT(-1,"SETSHIM_AP failed, dac or value out of range.\n");
		APint_run = FALSE;
		status = SHIMERROR+SHIMDACLIM;   /* the error is not quite right, but good enough for now */
    		errLogRet(LOGIT,debugInfo,
				"SETSHIM_AP Error: status is %d",status);
	      }
           }
	   break;


	/* JSETSHIM_AP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - dac number	 					*/
	/*  2 - dac value	 					*/
	case JSETSHIM_AP:
           {
	      int dac,value;
	      dac = *ac_cur++;
	      value = rt_tbl[*ac_cur++];
	      DPRINT2(2,"JSETSHIM_AP... dac: %d, value: %d  \n",dac,value);
	      if (setshimviaAp(dac,value) == -1)
	      {
	        DPRINT(-1,"SETSHIM_AP failed, dac or value out of range.\n");
		APint_run = FALSE;
		status = SHIMERROR+SHIMDACLIM;   /* the error is not quite right, but good enough for now */
    		errLogRet(LOGIT,debugInfo,
				"SETSHIM_AP Error: status is %d",status);
	      }
           }
	   break;

	/* APRTOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 6					*/
	/*  1 - apdelay		 					*/
	/*  2 - apaddr		 					*/
	/*  3 - maxval		 					*/
	/*  4 - minval		 					*/
	/*  5 - offset		 					*/
	/*  6 - rtoffset to value		 			*/
	case APRTOUT: 
	   DPRINT(2,"APRTOUT... \n");
	   if (aprtout(*ac_cur,*(ac_cur+1),*(ac_cur+2),*(ac_cur+3),
					*(ac_cur+4),rt_tbl[*(ac_cur+5)]) > 0)
	   {
		acodeWarningException.exceptionType = WARNING_MSG;  
		acodeWarningException.reportEvent = WARNINGS + APRTOVER;  
		msgQSend(pMsgesToPHandlr, (char*) &acodeWarningException, 
			sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	   }
	   ac_cur += 6;
	   break;

	/* APBCOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: n					*/
	/*  1 - apdelay	*/
	/*  ...			*/
	/*  n -		*/
	case APBCOUT:
	   DPRINT(2,"APBCOUT... \n");
	   ac_cur = apbcout(ac_cur);
	   break;

	/* TAPBCOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - rtoffset to pointer to apwords in table 		*/
	case TAPBCOUT:
	   DPRINT(2,"TAPBCOUT... \n");
	   rtv_ptr = (unsigned long *)rt_tbl[*ac_cur++];
	   rtv_ptr = (unsigned long *) apbcout((short *) rtv_ptr);
	   break;

	/* EXEAPREAD							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - apdelay		 					*/
	/*  2 - apaddr		 					*/
	case EXEAPREAD:
	   DPRINT(2,"EXEAPREAD... \n");
	   readapaddr( *(ac_cur+1), *ac_cur);
	   ac_cur += 2;
	   break;

	/* GETAPREAD							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - apaddr		 					*/
	/*  2 - rtoffset to real time variable				*/
	case GETAPREAD:
	   DPRINT2(2,"GETAPREAD apaddr=0x%x rtoffset=%d\n",*ac_cur,*(ac_cur+1));
	   if (getapread(*ac_cur, &rt_tbl[*(ac_cur+1)]) < 0)
	   {
		acodeWarningException.exceptionType = WARNING_MSG;  
		acodeWarningException.reportEvent = WARNINGS + APREADFAIL;  
		msgQSend(pMsgesToPHandlr, (char*) &acodeWarningException, 
			sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	   }
	   ac_cur += 2;
	   break;

	/* JAPWFGOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 6					*/
	/* arg1: apbus delay to insert					*/
	/* arg2: ap wfg base address   					*/
	/* arg3: rtwfgaddr (rtvar where wfg addr is stored)		*/
	/* arg4: offset value (if any, from wfgaddr)			*/
	/* arg5: rtdata (rt address where data for wfg is stored)	*/
	/* arg6: number of values to set 				*/
	case JAPWFGOUT:
	/* japbcout(ushort apdelay,ushort apaddr,long wfgaddr, ushort wfgoffset,
	/* 				long data[],short apcount) */
        DPRINT3(1,"JAPWFGOUT: rtoffset %d nval %d addr 0x%x\n",*(ac_cur+1),
        					*(ac_cur+2),*(ac_cur+3));
	   japwfgout(*ac_cur,*(ac_cur+1),rt_tbl[*(ac_cur+2)],*(ac_cur+3),
	   				&rt_tbl[*(ac_cur+4)],*(ac_cur+5));
	   ac_cur+=6;
	   break;

	/* JAPBCOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/* arg1: apbus delay to insert	*/
	/* arg2: rtoffset (rtvar where value is stored) */
	/* arg3: number of values to set */
	/* arg4: address  | byte flag | increment flag */
	case JAPBCOUT:
	   DPRINT(2,"JAPBCOUT... \n");
	/* japbcout(ushort apdelay,ushort apaddr,long data[],short apcount) */
        DPRINT3(1,"JAPBCOUT: rtoffset %d nval %d addr 0x%x\n",*(ac_cur+1),
        					*(ac_cur+2),*(ac_cur+3));
	   japbcout(*ac_cur,*(ac_cur+3),&rt_tbl[*(ac_cur+1)],*(ac_cur+2));
	   ac_cur+=4;
	   break;

	/* JTAPBCOUT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 6 + (nindices	- 1)			*/
	/* arg1: apbus delay to insert	*/
	/* arg2: table ID */
	/* arg3: Num of indicies: 1 to n  */
	/* arg4: rtoffset (rtvar0 to index into table) */
	/* arg4: rtoffset (rtvar1 to index into table) */
	/* arg4: ... to n
	/* arg5: number of values to set */
	/* arg6: address  | byte flag | increment flag */
	case JTAPBCOUT:
        DPRINT1(1,"JTAPBCOUT: table# %d\n",*(ac_cur+1));
           i = 0;
	   if (*(ac_cur+2)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+2);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+3+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       1,&rt_tbl[*(ac_cur+3)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTAPBCOUT Error: Invalid table element: %d.",*(ac_cur+1));
	   	ac_cur=ac_cur+6+i;
		break;
	   }
           DPRINT4(1,"JTAPBCOUT: apdelay: %d, apaddr: 0x%lx, dataaddr: 0x%lx, cnt: %d\n",
		*ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
	   japbcout(*ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
	   ac_cur=ac_cur+6+i;
	   break;

	/* JSETFREQ							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/* arg1: apbus delay to insert	*/
	/* arg2: rtoffset (rtvar where value is stored) */
	/* arg3: base address  | byte flag ?  */
	/* arg4: pts number  */
	case JSETFREQ:
	   DPRINT(2,"JSETFREQ... \n");
	   jsetfreq(*ac_cur,*(ac_cur+2),&rt_tbl[*(ac_cur+1)],*(ac_cur+3));
	   ac_cur+=4;
	   break;

	/* JTSETFREQ							*/
	/*  arguments: 							*/
	/*  0 - short word count: 6 + (nindices	- 1)			*/
	/* arg1: apbus delay to insert	*/
	/* arg2: table ID */
	/* arg3: Num of indicies: 1 to n  */
	/* arg4: rtoffset (rtvar0 to index into table) */
	/* arg4: rtoffset (rtvar1 to index into table) */
	/* arg4: ... to n */
	/* arg5: address  | byte flag ? | increment flag ? */
	/* arg6: pts no. */
	case JTSETFREQ:
	   DPRINT(2,"JTSETFREQ... \n");
           i = 0;
	   if (*(ac_cur+2)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+2);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+3+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       1,&rt_tbl[*(ac_cur+3)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		    "JTSETFREQ Error: Invalid table element: %d.",*(ac_cur+1));
	   	ac_cur=ac_cur+6+i;
		break;
	   }
	   jsetfreq(*ac_cur,*(ac_cur+4+i),(long *)elem_addr,*(ac_cur+5+i));
	   ac_cur=ac_cur+6+i;
	   break;


	case JTSETPHASE:
	   DPRINT(1,"JTSETPHASE... \n");
           i = 0;
	   if (*(ac_cur+2)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+2);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+3+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       1,&rt_tbl[*(ac_cur+3)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTSETPHASE Error: Invalid table element: %d.",*(ac_cur+1));
	   	ac_cur=ac_cur+6+i;
		break;
	   }
	   jsetphase(*ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
	   ac_cur=ac_cur+6+i;
           break;

	case JTSETPHASE90:
	   DPRINT(1,"JTSETPHASE90... \n");
           i = 0;
	   if (*(ac_cur+2)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+2);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+3+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       1,&rt_tbl[*(ac_cur+3)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTSETPHASE90 Error: Invalid table element: %d.",*(ac_cur+1));
	   	ac_cur=ac_cur+6+i;
		break;
	   }
	   jsetphase90(*ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
	   ac_cur=ac_cur+6+i;
           break;



	case JSETPHASE:
	   DPRINT(1,"JSETPHASE... \n");
           i = 0;
	   jsetphase(*ac_cur,*(ac_cur+3),&rt_tbl[*(ac_cur+1)],*(ac_cur+2));
	   ac_cur+=4;
           break;

	case JSETPHASE90:
	   DPRINT(1,"JSETPHASE90... \n");
           i = 0;
	   jsetphase90(*ac_cur,*(ac_cur+3),&rt_tbl[*(ac_cur+1)],*(ac_cur+2));
	   ac_cur+=4;
           break;

	case JAPSETPHASE:
	   DPRINT(1,"JSETPHASE... \n");
           i = 0;
	   japsetphase(*ac_cur,*(ac_cur+3),&rt_tbl[*(ac_cur+1)],*(ac_cur+2));
	   ac_cur+=4;
           break;




	/* RECEIVERGAIN							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/*  1 - apaddr							*/
	/*  2 - apdelay							*/
	/*  3 - rtoffset to gain					*/
	case JTRCVRGAIN:
	   DPRINT(1,"JTRCVRGAIN... \n");
           i = 0;
	   if (*(ac_cur+2)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+2);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+3+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur+1),
						       1,&rt_tbl[*(ac_cur+3)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTRCVRGAIN Error: Invalid table element: %d.",*(ac_cur+1));
	   	ac_cur=ac_cur+6+i;
		break;
	   }
	   receivergain(*(ac_cur+5+i),*ac_cur,*(long *)elem_addr);
	   ac_cur=ac_cur+6+i;
	   break;


	/* SAFETYCHECK							*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - SUflag: 1==>su						*/
	case SAFETYCHECK:
	   DPRINT(2,"SAFETYCHECK ......");
	   sibStatusCheck(pTheSibObject, *ac_cur);
           /* Disable GPA on SU, otherwise enable it. */
           /*
            * NB: gpaTuneSet() could take some time (seconds) to complete,
            * so this is not an acode to use while the FIFO is running.
            * If there is no MTS gradient amp, it should be fast.
            */
           /*i = (*ac_cur == 1) ? 0 : 1; /* Disable on "su", otherwise enable */
           gpaTuneSet(pTheAutoObject, SET_GPAENABLE, GPA_ENABLE_ON);
	   DPRINT(2,"...... SAFETYCHECK done");
	   ac_cur += alength;
	   break;

	/* SETTUNEFREQ							*/
	/*  arguments: 							*/
	/*  0 - short word count: n					*/
	/*  1 - channel		*/
	/*  3 - apwordcount	*/
	/*  4 - apwords		*/
	/*  ...			*/
	/*  n -	Hi/Low Band select	*/
	case SETTUNEFREQ:
	   DPRINT(2,"SETTUNEFREQ... \n");
	   ac_cur = settunefreq(ac_cur);
	   break;

	/* TUNESTART							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - channel		*/
	/*  2 - attenuation	*/
	case TUNESTART:
	   DPRINT(2,"TUNESTART... \n");
	   tuneStart(*ac_cur, *(ac_cur+1));
	   ac_cur += 2;
	   break;

	/* VGRADIENT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 7					*/
	/*  1 - apdelay		 					*/
	/*  2 - apaddr		 					*/
	/*  3 - maxval		 					*/
	/*  4 - minval		 					*/
	/*  5 - rtoffset to value 					*/
	/*  6 - signed increment		 			*/
	/*  7 - base offset			 			*/
	case VGRADIENT:
	   DPRINT(2,"VGRADIENT .....\n");
	   status = vgradient(*ac_cur, *(ac_cur+1), (int)(short)*(ac_cur+2),
		(int)(short)*(ac_cur+3), rt_tbl[*(ac_cur+4)], 
		(int)(short)*(ac_cur+5),(int)(short)*(ac_cur+6));
	   if (status != 0) {
		if (status > 0) {
		   status = 0;
		   acodeWarningException.exceptionType = WARNING_MSG;  
		   acodeWarningException.reportEvent = WARNINGS + VGRADOVER;  
		   msgQSend(pMsgesToPHandlr, (char*) &acodeWarningException, 
			sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
		} else {
		   APint_run = FALSE;
		   status = HDWAREERROR + INVALIDACODE;
		}
	   }
	   ac_cur += 7;
	   break;

	/* INCGRADIENT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 7					*/
	/*  1 - apdelay		 					*/
	/*  2 - apaddr		 					*/
	/*  3 - maxval		 					*/
	/*  4 - minval		 					*/
	/*  5 - rtoffset to value 1					*/
	/*  6 - signed increment 1		 			*/
	/*  7 - rtoffset to value 2					*/
	/*  8 - signed increment 2		 			*/
	/*  9 - rtoffset to value 3					*/
	/*  10 - signed increment 3		 			*/
	/*  11 - base offset			 			*/
	case INCGRADIENT:
	   DPRINT(2,"INCGRADIENT .....\n");
	   status = incgradient(*ac_cur, *(ac_cur+1), 
			     (int)(short)*(ac_cur+2),(int)(short)*(ac_cur+3), 
				rt_tbl[*(ac_cur+4)], (int)(short)*(ac_cur+5),
				rt_tbl[*(ac_cur+6)], (int)(short)*(ac_cur+7),
				rt_tbl[*(ac_cur+8)], (int)(short)*(ac_cur+9),
						    (int)(short)*(ac_cur+10));
	   if (status != 0) {
		if (status > 0) {
		   status = 0;
		   acodeWarningException.exceptionType = WARNING_MSG;  
		   acodeWarningException.reportEvent = WARNINGS + VGRADOVER;  
		   msgQSend(pMsgesToPHandlr, (char*) &acodeWarningException, 
			sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
		} else {
		   APint_run = FALSE;
		   status = HDWAREERROR + INVALIDACODE;
		}
	   }
	   ac_cur += 11;
	   break;

	/* JSETGRADMATRIX						*/
	/*   Sets gradient matrix from rtvar.				*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/* arg1: matrix component: RO_AXIS,PE_AXIS,SS_AXIS		*/
	/* arg2: rtoffset - rtvar location where x,y,z gradient 	*/
	/*			components are stored. 			*/
	case JSETGRADMATRIX:
	   jsetgradmatrix(*ac_cur,&rt_tbl[*(ac_cur+1)]);
	   ac_cur+=2;
	   break;

	/* JTSETGRADMATRIX						*/
	/*   Sets gradient matrix from rtvar pointer to rttable		*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/* arg1: matrix component: RO_AXIS,PE_AXIS,SS_AXIS		*/
	/* arg2: rtoffset - rtvar location where x,y,z gradient 	*/
	/*			components are stored. 			*/
	case JTSETGRADMATRIX:
	   jsetgradmatrix(*ac_cur,(long *)rt_tbl[*(ac_cur+1)]);
	   ac_cur+=2;
	   break;

	/* JSETOBLGRAD							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4 					*/
	/* arg1: apdelay - total time to set all instructions		*/
	/* arg2: apaddr							*/
	/* arg3: axis: X_AXIS, Y_AXIS, Z_AXIS				*/
	/* arg4: gradselect: SEL_WFG,SEL_PFG1,SEL_PFG2,SEL_TRIAX or'd	*/
	/*       with any other specific selection information.		*/
	case JSETOBLGRAD:
	   jsetoblgrad(*ac_cur,*(ac_cur+1),*(ac_cur+2),*(ac_cur+3));
	   ac_cur+=4;
	   break;

	/* BRANCH							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - code offset (int)					*/
	case BRANCH:
	   DPRINT(2,"BRANCH \n");
	   ac_cur = ac_cur+getuint(ac_cur);
	   acode = ac_cur - alength - 2;
	   DPRINT2(1,"BRANCH ac_cur %d acode %d\n",ac_cur,acode);
	   break;

	/* BRA_EQ							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset (a==b) (int)	*/
	case BRA_EQ: 
	   DPRINT(2,"BRA_EQ \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] == rt_tbl[b] ) {
		ac_cur = ac_cur+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* BRA_NE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset (a!=b) (int)	*/
	case BRA_NE: 
	   DPRINT(2,"BRA_NE \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] != rt_tbl[b] ) {
		ac_cur = ac_cur+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* BRA_GT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset (a > b) (int)	*/
	case BRA_GT: 
	   DPRINT(2,"BRA_GT \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] > rt_tbl[b] ) {
		ac_cur = ac_cur+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* BRA_LT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset (a < b) (int)	*/
	case BRA_LT: 
	   DPRINT(2,"BRA_LT \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] < rt_tbl[b] ) {
		ac_cur = ac_cur+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* JUMP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - code offset from acode base (int)			*/
	case JUMP:
	   DPRINT(2,"JUMP \n");
	   ac_cur = ac_base+getuint(ac_cur);
	   acode = ac_cur - alength - 2;
	   break;

	/* JMP_LT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/	
	/*  3 - code offset  from acode base (a < b) (int)		*/
	case JMP_LT: 
	   DPRINT(2,"JMP_LT \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] < rt_tbl[b] ) {
		ac_cur = ac_base+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* JMP_NE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset  from acode base (a != b) (int)		*/
	case JMP_NE: 
	   DPRINT(2,"JMP_NE \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( rt_tbl[a] != rt_tbl[b] ) {
		ac_cur = ac_base+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

	/* JMP_MOD2							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - rtoffset for a	*/
	/*  2 - rtoffset for b	*/
	/*  3 - code offset  from acode base (a != b) (int)		*/
	case JMP_MOD2: 
	   DPRINT(2,"JMP_MOD2 \n");
	   a = *ac_cur++;
	   b = *ac_cur++;
	   if ( (rt_tbl[a] % 2) != rt_tbl[b] ) {
		ac_cur = ac_base+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
	   }
	   break;

         case PARALLEL_INIT:
           {
              int maxchan = rt_tbl[*(ac_cur++)];
              int maxsize = rt_tbl[*(ac_cur++)];
	      DPRINT2(0,"PARALLEL_INIT: maxchan: %d, maxsize: %d\n",maxchan,maxsize);
	      pParallelIBufs = initializeSorterBufs(maxchan,maxsize);
	      DPRINT3(0,"PARALLEL_INIT: maxchan: %d, maxsize: %d, pParallelIBufs: 0x%lx\n",maxchan,maxsize,pParallelIBufs);
           }
           break;
	      
	 case PARALLEL:
           {
	      int workingChan  = *ac_cur++;
              int parallelizeflag = rt_tbl[*(ac_cur++)];
	      DPRINT2(0,">>>>>> PARALLEL: Working Channel: %d, parallelizeflag = %d\n",
				workingChan,parallelizeflag);

              if (parallelizeflag != 0)
              {
	        DPRINT1(0,">>>>>> PARALLEL: Working Channel: %d\n",workingChan);
	        pchanStart(pcsId,workingChan);
              }
           }
	   break;

	 case PARALLEL_START:
           {
	      PCHANSORT_ID pcsId2;
 	      RINGL_ID pchanBuf;

	      int numchans  = *ac_cur++;
              int parallelizeflag = rt_tbl[*(ac_cur++)];
	      /* parallelstart[++parallelindex] = parallelizeflag; */
	      DPRINT2(0,"PARALLEL_START: numchans: %d, parallelize flag: %d -------------------------\n",numchans,parallelizeflag);
              /* PCHANSORT_ID */
              pcsId2 = pcsId;
	      DPRINT1(0,"----- Channel Content Prior to Embedded Parallel Channels (pcsId=0x%lx)\n",
		pcsId2);
              if (DebugLevel > 0)
	         pchanShowChans(pcsId2);
	      DPRINT4(0,"PARALLEL_START: numchans: %d, pParallelIBufs: 0x%lx, pcsId2: 0x%lx, fifoObj: 0x%lx\n",numchans,pParallelIBufs,pcsId2,pTheFifoObject);
              if (parallelizeflag != 0)
              {
	         pcsId = pchanSortCreate(numchans,"parallel", pParallelIBufs,pcsId2,pTheFifoObject);
                 DPRINT2(0,"Parent pcsOBj: 0x%lx,  New pcsObj: 0x%lx\n",pcsId2,pcsId);
	         fifoSetPChanId(pTheFifoObject, pcsId);
              }
           }
	   break;

	 case PARALLEL_END:
	     { 
	       PCHANSORT_ID pcsId2 = pcsId;
               int parallelizeflag = rt_tbl[*(ac_cur++)];
	       DPRINT1(0,"PARALLEL_END: parallelizeflag: %d ------------------------------------\n",parallelizeflag);
	       /* DPRINT1(-1,"PARALLEL_END: parallelizeflag: %d\n",parallelstart[parallelindex]); */
               /* if (parallelstart[parallelindex--] != 0)  */
               if (parallelizeflag != 0)
               {
	          pchanSort(pcsId);
	          pcsId = pchanGetParent(pcsId);
                  DPRINT2(0,"Embedded pcsObj: 0x%lx,  Parent pcsObj: 0x%lx\n",pcsId2,pcsId);
                  pchanSortDelete(pcsId2);
	          fifoSetPChanId(pTheFifoObject, pcsId);
               }
             }

	   break;

	/* 
         * lock parallel channel, used when multiple instructions must be atomic 
         * e.g. the 8 apbus instruction to set frequency 
         */
	case LOCKPCHAN:
	    /* if not in a parallel channel then skip it */
     	    if (pTheFifoObject->pPChanObj)
	    {
#ifdef INSTRUMENT
      		wvEvent(EVENT_PARALLEL_LOCK,NULL,NULL);
#endif
                pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_LOCK,0);
            }
	   break;

	/* 
         * unlock parallel channel, used when multiple instructions must be atomic 
         */
	case UNLOCKPCHAN:
	    /* if not in a parallel channel then skip it */
     	    if (pTheFifoObject->pPChanObj)
	    {
#ifdef INSTRUMENT
      		wvEvent(EVENT_PARALLEL_LOCK,NULL,NULL);
#endif
                pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_UNLOCK,0);
	    }
	   break;

	/* RTINIT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/*  1 - value (int)	*/
	/*  2 - rtoffset to put value	*/
	case RTINIT:
	   DPRINT(2,"RTINIT\n");
           tmp = alength / 3;
           while (tmp > 0)
           {
	      rt_tbl[*(ac_cur+2)] = getuint(ac_cur);
	      DPRINT2(2,">>>>  rt_tbl[%d] = %d\n",*(ac_cur+2),rt_tbl[*(ac_cur+2)]);
	      ac_cur += 3;
              tmp--;
           }
	   break;

	/* RTOP								*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - OPCODE (short)	*/
	/*  2 - rtoffset to value	*/
	case RTOP:
	   DPRINT(2,"RTOP\n");
	   switch (*ac_cur++)
	   {
		case CLR:
			rt_tbl[*ac_cur++] = 0;
			break;
		case INC:
			rt_tbl[*ac_cur++]++;
			break;
		case DEC:
			rt_tbl[*ac_cur++]--;
			break;
		default:
			DPRINT1(1,"RTOP: NO OPCODE %x\n",*(ac_cur-1));
	   }
	   break;

	/* RT2OP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/*  1 - OPCODE (short)	*/
	/*  2 - rtoffset to source	*/
	/*  3 - rtoffset to destination	*/
	case RT2OP:
	   DPRINT(2,"RT2OP\n");
	   opcode = *ac_cur++;
	   a = *ac_cur++;
	   b = *ac_cur++;
	   switch (opcode)
	   {
		case SET:
			rt_tbl[b] = rt_tbl[a];
			break;
		case MOD2:
			rt_tbl[b] = rt_tbl[a] & 1;	/* "b = a % 2" */	
			break;
		case MOD4:
			rt_tbl[b] = rt_tbl[a] & 3;	/* "b = a % 4" */	
			break;
		case HLV:
			rt_tbl[b] = rt_tbl[a] / 2;	/* "b = a / 2" */	
			break;
		case DBL:
			rt_tbl[b] = rt_tbl[a] * 2;	/* "b = a * 2" */	
			break;
		case NOT:
			rt_tbl[b] = ~rt_tbl[a];		/* "b = ~a" */	
			break;
		case NEG:
			rt_tbl[b] = rt_tbl[a] * -1;	/* "b = a * -1" */	
			break;
		default:
			DPRINT1(1,"RT2OP: NO OPCODE %x\n",*(ac_cur-1));
			ac_cur+=2;
	   }
	   break;

	/* RT3OP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4					*/
	/*  1 - OPCODE (short)	arg3 = arg1 OPCODE arg2 */
	/*  2 - rtoffset to source	*/
	/*  3 - rtoffset to source2	*/
	/*  4 - rtoffset to destination	*/
	case RT3OP:
	   DPRINT(2,"RT3OP\n");
	   opcode = *ac_cur++;
	   a = *ac_cur++;
	   b = *ac_cur++;
	   c = *ac_cur++;
	   switch (opcode)
	   {
		case ADD:
			rt_tbl[c] = rt_tbl[a] + rt_tbl[b];
			break;
		case SUB:
			rt_tbl[c] = rt_tbl[a] - rt_tbl[b];
			break;
		case MUL:
			rt_tbl[c] = rt_tbl[a] * rt_tbl[b];
			break;
		case DIV:
			if (rt_tbl[b] == 0)
			{
			  APint_run=FALSE;
			  status = HDWAREERROR + INVALIDACODE;
    			  errLogRet(LOGIT,debugInfo,
				"RT3OP: DIV Error: Zero Divide!");
			}
			else
			  rt_tbl[c] = rt_tbl[a] / rt_tbl[b];
			break;
		case MOD:
			if (rt_tbl[b] == 0)
			{
			  APint_run=FALSE;
			  status = HDWAREERROR + INVALIDACODE;
    			  errLogRet(LOGIT,debugInfo,
				"RT3OP: MOD Error: Zero Divide!");
			}
			else
			  rt_tbl[c] = rt_tbl[a] % rt_tbl[b];
			break;
		case OR:
			rt_tbl[c] = rt_tbl[a] | rt_tbl[b];
			break;
		case AND:
			rt_tbl[c] = rt_tbl[a] & rt_tbl[b];
			break;
		case XOR:
			rt_tbl[c] = rt_tbl[a] ^ rt_tbl[b];
			break;
		case LSL:
			(ulong_t)rt_tbl[c] = (ulong_t)rt_tbl[a] <<
							 (ulong_t)rt_tbl[b];
			break;
		case LSR:
			(ulong_t)rt_tbl[c] = (ulong_t)rt_tbl[a] >>
							 (ulong_t)rt_tbl[b];
			break;
		case LT:
			rt_tbl[c] = (rt_tbl[a] < rt_tbl[b]) ?  1 : 0;
			break;
		case GT:
			rt_tbl[c] = (rt_tbl[a] > rt_tbl[b]) ?  1 : 0;
			break;
		case GE:
			rt_tbl[c] = (rt_tbl[a] >= rt_tbl[b]) ?  1 : 0;
			break;
		case LE:
			rt_tbl[c] = (rt_tbl[a] <= rt_tbl[b]) ?  1 : 0;
			break;
		case EQ:
			rt_tbl[c] = (rt_tbl[a] == rt_tbl[b]) ?  1 : 0;
			break;
		case NE:
			rt_tbl[c] = (rt_tbl[a] != rt_tbl[b]) ?  1 : 0;
			break;
		default:
			DPRINT1(1,"RT3OP: NO OPCODE %x\n",opcode);
	   }
	   break;

	/* RTERROR							*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - Error code	*/
	case RTERROR:
	   DPRINT(2,"RTERROR\n");
	   APint_run=FALSE;
	   status = RTPSGERROR + *ac_cur++;
    	   errLogRet(LOGIT,debugInfo,
			"RTERROR: Real-time Error = %d.",status-RTPSGERROR);
	   break;

	/* TASSIGN							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/* arg1: table ID */
	/* arg2: rtoffset (rtvar to index into table) */
	/* arg3: rtoffset (rt var to place table value */
	case TASSIGN:
	   DPRINT3(2,"TASSIGN... table id: %d, indexos: %d, indexval: %d\n",
		*ac_cur,*(ac_cur+1),rt_tbl[*(ac_cur+1)]);
	   elem_addr = tbl_element_addr(pAcodeId->table_ptr,
						*ac_cur,rt_tbl[*(ac_cur+1)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"TASSIGN Error: Invalid table element.");
		break;
	   }
	   ac_cur+=2;
	   DPRINT2(2,"rtos: %d, value: 0x%lx\n",*ac_cur,*(ulong_t *)(elem_addr));
	   rt_tbl[*ac_cur++] = *(ulong_t *)(elem_addr);
	   break;

	/* TSETPTR							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/* arg1: table ID */
	/* arg2: rtoffset (rtvar to index into table) */
	/* arg3: rtoffset (rt var to place pointer to table value */
	case TSETPTR:
	   DPRINT(2,"TSETPTR... \n");
	   elem_addr = tbl_element_addr(pAcodeId->table_ptr,
						*ac_cur,rt_tbl[*(ac_cur+1)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"TSETPTR Error: Invalid table element.");
		break;
	   }
	   ac_cur+=2;
	   rt_tbl[*ac_cur++] = (ulong_t)elem_addr;
	   break;

	/* JTASSIGN							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3 + nindices				*/
	/* arg1: table ID */
	/* arg2: Num of indicies: 1 to n  */
	/* arg3: rtoffset (rtvar0 to index into table) */
	/* arg3: rtoffset (rtvar1 to index into table) */
	/* arg3: .... to n */
	/* arg4: rtoffset (rtvar location to put value from table) */
	case JTASSIGN:
	   DPRINT3(2,"JTASSIGN... table id: %d, 1st indexos: %d, 1st indexval: %d\n",
		*ac_cur,*(ac_cur+2),rt_tbl[*(ac_cur+2)]);
	   i = 0;
	   if (*(ac_cur+1)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+1);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+2+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur),
						       1,&rt_tbl[*(ac_cur+2)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTASSIGN Error: Invalid table element: %d.",*(ac_cur));
	   	ac_cur=ac_cur+4+i;
		break;
	   }
	   DPRINT2(2,"rtos: %d, value: 0x%lx\n",*(ac_cur+3+i),*(ulong_t *)(elem_addr));
	   rt_tbl[*(ac_cur+3+i)] = *(ulong_t *)(elem_addr);
	   ac_cur=ac_cur+4+i;
	   break;

	/* JRTASSIGN							*/
	/*  This is basically the same as JTASSIGN except that it gets	*/
	/*  its table from a realtime variable and can assign more than	*/
	/*  one long word from a table to the realtime variables.	*/ 
	/*  arguments: 							*/
	/*  0 - short word count: 4 + nindices				*/
	/* arg1: rtvar with table ID */
	/* arg2: Num of indicies: 1 to n  */
	/* arg3: rtoffset (rtvar0 to index into table) */
	/* arg3: rtoffset (rtvar1 to index into table) */
	/* arg3: .... to n */
	/* arg4: rtoffset (rtvar start location to put values from table) */
	/* arg5: number of values (longs) to assign from rtoffset */
	case JRTASSIGN:
	   DPRINT3(1,"JRTASSIGN... table id: %d, 1st indexos: %d, 1st indexval: %d\n",
		rt_tbl[*(ac_cur)],*(ac_cur+2),rt_tbl[*(ac_cur+2)]);
	   i = 0;
	   if (*(ac_cur+1)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+1);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+2+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,rt_tbl[*(ac_cur)],
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,rt_tbl[*(ac_cur)],
						       1,&rt_tbl[*(ac_cur+2)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JRTASSIGN Error: Invalid table element: %d.",rt_tbl[*(ac_cur)]);
	   	ac_cur=ac_cur+5+i;
		break;
	   }
	   for (index=0; index<*(ac_cur+4+i); index++) {
	      DPRINT2(1,"rtos: %d, value: 0x%lx\n",*(ac_cur+3+i),*(((ulong_t *)elem_addr)+index));
	      rt_tbl[*(ac_cur+3+i)+index] = *(((ulong_t *)elem_addr)+index);
	   }
	   ac_cur=ac_cur+5+i;
	   break;

	/* JTSETPTR							*/
	/*  arguments: 							*/
	/*  0 - short word count: 4 + (nindices	- 1)			*/
	/* arg1: table ID */
	/* arg2: Num of indicies: 1 to n  */
	/* arg3: rtoffset (rtvar to index into table) */
	/* arg4: rtoffset (rtvar ptr to location in table) */
	case JTSETPTR:
	   DPRINT(2,"JTSETPTR... \n");
	   i = 0;
	   if (*(ac_cur+1)  >  1) {
	       int nindex;
	       long indextbl[20];
	       nindex = *(ac_cur+1);
	       if (nindex <= 20) {
	          for ( i=0; i<nindex; i++) {
	             indextbl[i] = rt_tbl[*(ac_cur+2+i)];
	          }
	          elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur),
						       nindex,indextbl);
		  i = i-1;  /* set index back to be used in counting acodes */
	       }
	       else elem_addr = NULL;
	   }
	   else 
	       elem_addr = jtbl_element_addr(pAcodeId->table_ptr,*(ac_cur),
						       1,&rt_tbl[*(ac_cur+2)]);
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
		   "JTSETPTR Error: Invalid table element: %d.",*(ac_cur));
	   	ac_cur=ac_cur+4+i;
		break;
	   }
	   rt_tbl[*(ac_cur+3+i)] = (ulong_t)elem_addr;
	   ac_cur=ac_cur+4+i;
	   break;

	/* APTABLE (PSG Compatibility acode)				*/
	/*  arguments: 							*/
	/*  0 - short word count: n + 3					*/
	/* arg1: number of short words in table (short)			*/
	/* arg2: auto increment flag(short)				*/
	/* arg3: divn (short)						*/
	/* arg4: auto-inc counter (short)				*/
	case APTABLE:
	   DPRINT(2,"APTABLE .....\n");
	   /* skip over table */
	   a = *ac_cur++;
	   ac_cur = ac_cur + (3 + a);
	   break;


	/* APTASSIGN (PSG Compatibility acode)				*/
	/*  arguments: 							*/
	/*  0 - short word count: n + 3					*/
	/* arg1: acode loc			*/
	/* arg2: index ptr (exists if no autoincrement in table)	*/
	/* arg2 or arg3: index into rttable				*/
	case APTASSIGN:
	   DPRINT(2,"APTASSIGN .....\n");
	   /* skip over table */
	   if ( *(ac_base + *ac_cur + 1) == 1)
	   {
	   	elem_addr = aptbl_element_addr(ac_base,*ac_cur,*(ac_cur+1));
		ac_cur+=2;
	   }
	   else
	   {
	   	elem_addr = aptbl_element_addr(ac_base,*ac_cur,
							rt_tbl[*(ac_cur+1)]);
		ac_cur+=2;
	   }
	   if (elem_addr == NULL)
	   {
		APint_run=FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"APTASSIGN Error: Invalid table element.");
		break;
	   }
	   rt_tbl[*ac_cur++] = *(short *)(elem_addr);
	   break;

	case SETPRESIG:
	   DPRINT(2,"SETPRESIG .....\n");
	   setpresig(*ac_cur++);
	   break;

	/* CLEARSCANDATA
	 * arguments:
	 * 0 - short word count: 2
	 * 1 - rtoffset to dtm control words.
	 * 2 - rtoffset to active receivers.
	 */
	case CLEARSCANDATA:
	  DPRINT2(2,"CLEARSCANDATA (0x%x, 0x%x)\n",
		  rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)]);
	  clearscandata(&rt_tbl[*(ac_cur+1)], (int*) &rt_tbl[*ac_cur]);
	  ac_cur += 2;
	  break;

	/* RECEIVERCYCLE
	 * arguments:
	 * 0 - short word count: 3
	 * 1 - oph
	 * 2 - rtoffset to dtm control words.
	 * 3 - rtoffset to active receivers.
	 */
	case RECEIVERCYCLE:
	  DPRINT3(2,"RECEIVERCYCLE (%d, 0x%x, 0x%x)\n",
		   rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)]);
	   receivercycle(&rt_tbl[*(ac_cur+2)],
			 rt_tbl[*ac_cur],
			 (int*) &rt_tbl[*(ac_cur+1)]);
	   ac_cur+=3;
	   break;

	/* ENABLEOVRLDERR
	 * arguments:
	 * 0 - short word count: 4
	 * 1 - ssctr
	 * 2 - ct
	 * 3 - adc control word
	 * 4 - rtoffset to active receivers.
	 */
	case ENABLEOVRLDERR:
#ifdef OLD_ENABLES
	  DPRINT4(2,"ENABLEOVRLDERR (%d, %d, 0x%x, 0x%x)\n",
		  rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)],
		  rt_tbl[*(ac_cur+3)]);
	   enableOvrLd(&rt_tbl[*(ac_cur+3)], rt_tbl[*ac_cur],
		       rt_tbl[*(ac_cur+1)], &rt_tbl[*(ac_cur+2)]);
	   ac_cur+=4;
#else
	  DPRINT5(2,"ENABLEOVRLDERR (%d, %d, 0x%x, 0x%x, 0x%x)\n",
		  rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)],
		  rt_tbl[*(ac_cur+3)],rt_tbl[*(ac_cur+4)]);
	   enableOvrLdN(&rt_tbl[*(ac_cur+4)], rt_tbl[*ac_cur],
		       rt_tbl[*(ac_cur+1)], (int*) &rt_tbl[*(ac_cur+2)], &rt_tbl[*(ac_cur+3)]);
	   ac_cur+=5;
#endif
	   break;

	/* DISABLEOVRLDERR
	 * arguments:
	 * 0 - short word count: 2
	 * 1 - adc control word
	 * 2 - rtoffset to active receivers.
	 */
	case DISABLEOVRLDERR:
#ifdef OLD_ENABLES
	   DPRINT(2,"DISABLEOVRLDERR .....\n");
	   /* disableOvrLd(ulong_t *adccntrl) */
	   disableOvrLd(&rt_tbl[*(ac_cur+1)], &rt_tbl[*ac_cur]);
	   ac_cur+=2;
#else
	   DPRINT3(2,"DISABLEOVRLDERR (dtmcntrl: 0x%x, adccntrl: 0x%x, activercvrs: 0x%x)\n",
	        rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)]);
	   /* disableOvrLd(ulong_t *adccntrl) */
	   disableOvrLdN(&rt_tbl[*(ac_cur+2)], (int*) &rt_tbl[*ac_cur],&rt_tbl[*(ac_cur+1)]);
	   ac_cur+=3;
#endif
	   break;

	case ENABLEHSSTMOVRLDERR:
	   /* DPRINT(-2,"ENABLEHSSTMOVRLDERR ..... not used!!\n"); */
	   errLogRet(LOGIT,debugInfo,"ENABLEHSSTMOVRLDERR ..... not used.\n");
	   /* void enableStmAdcOvrLd(int ssct, int ct, int *dtmcntrl) */
	   /* enableStmAdcOvrLd(rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)],(int*) &rt_tbl[*(ac_cur+2)]); */
	   ac_cur+=3;
	   status = HDWAREERROR + INVALIDACODE;
	   APint_run = FALSE;
	   break;

	case DISABLEHSSTMOVRLDERR:
	   /* DPRINT(-2,"DISABLEHSSTMOVRLDERR ..... not used!!\n"); */
	   errLogRet(LOGIT,debugInfo,"DISABLEHSSTMOVRLDERR ..... not used.\n");
	   /* void disableStmAdcOvrLd(int *dtmcntrl) */
	   /* disableStmAdcOvrLd((int*) &rt_tbl[*ac_cur]); */
	   ac_cur+=1;
	   status = HDWAREERROR + INVALIDACODE;
	   APint_run = FALSE;
	   break;


	case INITSCAN:
	   DPRINT(2,"INITSCAN .....\n");
	   /*initscan(int np, int ss, int *ssct, int nt, int *ct, int bs, 
							int cbs, int maxsum) */
	   initscan(&rt_tbl[*(ac_cur+8)],
		    pAcodeId,rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], 
		    (int*) &rt_tbl[*(ac_cur+2)], rt_tbl[*(ac_cur+3)],
		    rt_tbl[*(ac_cur+4)], rt_tbl[*(ac_cur+5)],
		    rt_tbl[*(ac_cur+6)],rt_tbl[*(ac_cur+7)]);
	   ac_cur+=9;
	   break;

	case NEXTSCAN:
	   DPRINT(2,"NEXTSCAN .....\n");
	   /* Start Fifo watchdog to start fifo if stmAllocAcqBlk blocks */
	   /* within nextscan. 						 */
	   /* acodeStartFifoWD(pAcodeId,pTheFifoObject); */

	   nextscan(&rt_tbl[*(ac_cur+7)],
		    pAcodeId,rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], 
		    rt_tbl[*(ac_cur+2)], rt_tbl[*(ac_cur+3)],
		    rt_tbl[*(ac_cur+4)], (int*) &rt_tbl[*(ac_cur+5)],
		    rt_tbl[*(ac_cur+6)]);
	   /* acodeCancelFifoWD(pAcodeId); */
	   ac_cur+=8;
	   break;

	case ENDOFSCAN:
	   DPRINT(2,"ENDOFSCAN .....\n");
	   /* endofscan( ss, *ssct, nt, *ct, bs, *cbs) */
	   endofscan(&rt_tbl[*(ac_cur+6)],
		     rt_tbl[*ac_cur], (int*) &rt_tbl[*(ac_cur+1)],
		     rt_tbl[*(ac_cur+2)], (int*) &rt_tbl[*(ac_cur+3)],
		     rt_tbl[*(ac_cur+4)], (int*) &rt_tbl[*(ac_cur+5)]);
	   ac_cur += 7;
	   break;


/******* Specials for multiple rcvr interleave *******/
	case INITSCAN2:
	   DPRINT(2,"INITSCAN2 .....\n");
	   initscan2(&rt_tbl[*(ac_cur+9)],
		     pAcodeId,rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)], 
		     (int*) &rt_tbl[*(ac_cur+3)], rt_tbl[*(ac_cur+4)],
		     rt_tbl[*(ac_cur+5)], rt_tbl[*(ac_cur+6)],
		     rt_tbl[*(ac_cur+7)],rt_tbl[*(ac_cur+8)],
		     *ac_cur);
	   ac_cur+=10;
	   break;

	case NEXTSCAN2:
	   DPRINT(2,"NEXTSCAN2 .....\n");
	   nextscan2(&rt_tbl[*(ac_cur+8)],
		     pAcodeId,rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)], 
		     rt_tbl[*(ac_cur+3)], rt_tbl[*(ac_cur+4)],
		     rt_tbl[*(ac_cur+5)], (int*) &rt_tbl[*(ac_cur+6)],
		     rt_tbl[*(ac_cur+7)], *ac_cur);
	   ac_cur+=9;
	   break;

	case ENDOFSCAN2:
	   DPRINT(2,"ENDOFSCAN2 .....\n");
	   endofscan2(&rt_tbl[*(ac_cur+7)],
		      rt_tbl[*(ac_cur+1)], (int*) &rt_tbl[*(ac_cur+2)],
		      rt_tbl[*(ac_cur+3)], (int*) &rt_tbl[*(ac_cur+4)],
		      rt_tbl[*(ac_cur+5)], (int*) &rt_tbl[*(ac_cur+6)],
		      *ac_cur);
	   ac_cur += 8;
	   break;
/******* END Specials for multiple rcvr interleave *******/

	case SET_DATA_OFFSETS:
	   DPRINT(2,"SET_DATA_OFFSETS .....\n");
	   setscanvalues(&rt_tbl[*(ac_cur+3)], pAcodeId, rt_tbl[*ac_cur],
			 rt_tbl[*(ac_cur+1)], 0, (int*)&rt_tbl[*(ac_cur+2)]);
	   ac_cur+=4;
	   break;

	case SET_NUM_POINTS:
	   DPRINT(2,"SET_NUM_POINTS .....\n");
	   setscanvalues(&rt_tbl[*(ac_cur+2)], pAcodeId, 0, 0,
			 rt_tbl[*ac_cur], (int*)&rt_tbl[*(ac_cur+1)]);
	   ac_cur+=3;
	   break;

	/* BEGINHARDLOOP						*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/* arg1: rtoffset to number of hw loop cycles */
	case BEGINHWLOOP:
	   DPRINT(2,"BEGINHWLOOP ......");
           tmp = rt_tbl[ *ac_cur ];
	   if (tmp > 1)
	   {
	      DPRINT1(2,"HWLOOP %d times\n",tmp);
	      fifoStuffCmd(pTheFifoObject,CL_START_LOOP | CL_END_LOOP, (tmp - 1) );
	      fifoBeginHardLoop(pTheFifoObject);
              ac_cur += 3;
	   }
           else if (tmp < 1)
           {
              ac_cur++;
	      ac_cur = ac_cur+getuint(ac_cur);
	      DPRINT(2,"SKIP HWLOOP ");
	      acode = ac_cur - alength - 2;
              if ( (*(ac_cur-2) != ENDHWLOOP) || (*(ac_cur-1) != 0) )
              {
	         DPRINT(1,"HWLOOP JUMP INCORRECT");
              }
           }
	   else
           {
	      DPRINT(2,"HWLOOP of 1 is no LOOP\n");
              ac_cur += 3;
           }
	   break;

	/* ENDHARDLOOP							*/
	/*  arguments: 							*/
	/*  0 - short word count: 0					*/
	case ENDHWLOOP:
	   DPRINT(2,"ENDHWLOOP ......");
	   fifoEndHardLoop(pTheFifoObject);
	   break;

	/* TACQUIRE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 3					*/
	/* arg1: flags (hwlooping) (short)*/
	/* arg2: rtoffset to np 	*/
	/* arg3: rtoffset to delay which is never greater than 2 sec. 	*/
	case TACQUIRE:
	   DPRINT(2,"TACQUIRE ......");
	   /* acquire(short flags, int np, int dwell); */
	   if (acquire(*ac_cur,rt_tbl[*(ac_cur+1)],rt_tbl[*(ac_cur+2)]) < 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"Acquire Error: dwell or np too large.");
	   }
	   ac_cur+=3;
	   break;

	/* ACQUIRE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 5					*/
	/* arg1: flags (hwlooping) (short)*/
	/* arg2: np (int)	*/
	/* arg3: delay in 10 ns ticks which is never greater than 2 sec. */
	case ACQUIRE:
	   DPRINT3(2,"ACQUIRE flag:%d, np:%d, dwell: %d ......",*ac_cur,getuint(ac_cur+1), getuint(ac_cur+3));
	   if (acquire(*ac_cur,getuint(ac_cur+1), getuint(ac_cur+3)) < 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"Acquire Error: dwell or np too large.");

	   }
	   ac_cur+=5;
	   break;

	/* JACQUIRE							*/
	/*  arguments: 							*/
	/*  0 - short word count: 5					*/
	/* arg1: flags (hwlooping) (short)*/
	/* arg2: rt np (int)	*/
	/* arg3: rt dwell in 10 ns ticks which is never greater than 2 sec. */
	case JACQUIRE:
	   DPRINT5(0,"JACQUIRE flag:%d, np(%d):%d, dwell(%d): %d ......",
		    *ac_cur,*(ac_cur+1),rt_tbl[*(ac_cur+1)],*(ac_cur+2),rt_tbl[*(ac_cur+2)]);
	   if (acquire(*ac_cur,rt_tbl[*(ac_cur+1)], rt_tbl[*(ac_cur+2)]) < 0) {
		APint_run = FALSE;
		status = HDWAREERROR + INVALIDACODE;
    		errLogRet(LOGIT,debugInfo,
				"Acquire Error: dwell or np too large.");

	   }
	   ac_cur+=3;
	   break;

	case TSTACQUIRE:
	   DPRINT(3,"TSTACQUIRE ......");
	   tstacquire(*ac_cur,rt_tbl[*(ac_cur+1)],rt_tbl[*(ac_cur+2)],
	   			rt_tbl[*(ac_cur+3)],(long*)pAcodeId->cur_scan_data_adr);
	   ac_cur+=4;
	   break;

	/* NEXTCODESET							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - rtoffset to il (interleave flag)			*/
	/*  2 - rtoffset to cbs (completed blocksize count)		*/
	/*  3 - rtoffset to nt  (number transients)			*/
	/*  4 - rtoffset to ct  (completed transients)			*/
	case NEXTCODESET:
	   DPRINT(2,"NEXTCODESET .....\n");
	   /* nextcodeset(pAcodeId,il,*cbs,nt,ct) */
	   ac_base=nextcodeset(pAcodeId,rt_tbl[*ac_cur],(int*) &rt_tbl[*(ac_cur+1)],
				rt_tbl[*(ac_cur+2)],rt_tbl[*(ac_cur+3)]);
	   if (ac_base == NULL) 
           {
		if (pAcodeId->cur_acode_set < pAcodeId->num_acode_sets) 
		{
		   status = SYSTEMERROR+NOACODESET;
    		   errLogRet(LOGIT,debugInfo,
			"NEXTCODESET Error: No Acodes for set: %d.",
						pAcodeId->cur_acode_set);
		}
		else 
		{
		   fifoStuffCmd(pTheFifoObject,HALTOP,0);
	           if ( fifoGetStart4Exp(pTheFifoObject) == 0 )  /* all done parsing, start the Exp ? */
	           {
		      DPRINT(0,"Done Parsing, Start FIFO\n");
#ifdef INSTRUMENT
		  wvEvent(97,NULL,NULL);
#endif
		      fifoStart4Exp(pTheFifoObject);
	           }
		}
		APint_run = FALSE;
	        ac_cur+=4;
	   }
	   else {
		ac_cur = ac_start = ac_base+1;
		acode = ac_cur - alength - 2;
	   }
	   break;

	/* ACQI_UPDT							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - rtoffset for acqi interactive flag (if acqi sets flag)	*/
	/*  2 - rtoffset to 1 word delay (100nsecs - 2secs)		*/
	/* added LOCK_UPDT, for the lock display. */
	case ACQI_UPDT:
	case LOCK_UPDT:
	   DPRINT(2,"ACQI_UPDT .....\n");
	   acqi_updt(pAcodeId,*acode,(int*) &rt_tbl[*ac_cur],rt_tbl[*(ac_cur+1)] );
	   ac_cur+=2;
	   break;

	/* INIT_ACQI_UPDT						*/
	/*  Used in conjunction with START_ACQI_UPDT			*/
	/*  arguments: 							*/
	/*  0 - short word count: 0					*/
	case SET_ACQI_UPDT_CMPLT:
	   DPRINT(2,"SET_ACQI_UPDT_CMPLT .....\n");
	   set_acqi_updt_cmplt();
	   break;

	/* START_ACQI_UPDT						*/
	/*  Used in conjunction with SET_ACQI_UPDT_CMPLT		*/
	/*  arguments: 							*/
	/*  0 - short word count: 0					*/
	case START_ACQI_UPDT:
	   DPRINT(2,"START_ACQI_UPDT .....\n");
	   start_acqi_updt(pAcodeId);
	   break;

	/* SIGNAL_COMPLETION						*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - Completion Code						*/
	case SIGNAL_COMPLETION:
	   DPRINT(2,"SIGNAL_COMPLETION .....\n");
   	   if (pAcodeId->interactive_flag != ACQ_AUTOGAIN)
	      signal_completion(*ac_cur++);
           else
	   {
	     DPRINT(2,">> Skipped do to AUTOGAIN\n");
             ac_cur++;
           }
	   break;
	case STATBLOCK_UPDT:
	   DPRINT(2,"STATBLOCK_UPDT .....\n");
   	   if (pAcodeId->interactive_flag != ACQ_AUTOGAIN)
           {
              update_acqstate(ACQ_ACQUIRE);
              getstatblock();   /* force statblock upto hosts */
           }
           else
           {
	     DPRINT(2,">> Skipped do to AUTOGAIN\n");
           }
	   break;

/*  END_PARSE no longer stuffs a HALT OP into the FIFO.  It was taken out
    in connection with interactive lock display.  Those A-codes conclude
    by stuffing a HALT OP, starting the FIFO and waiting for it to halt,
    so the END_PARSE a-code itself do not need to insert a HALT OP in
    the FIFO for interactive lock display..

    The flip side is if the FIFO runs out of words without encountering
    a HALT OP, a FOO interrupt occurs and bad things happen.		*/
    
	case END_PARSE: 
	   /*fifoStuffCmd(pTheFifoObject,HALTOP,0);*/
	   APint_run = FALSE;
	   break;

/*  The lock freqncy arrives as the AP word itself.  Because it is a 32
    bit quantity, while the A-code's are processed as 16 bit quantities,
    the lock frequency AP word is transferred as 2 short words.  05/1997  */

	case LOCKFREQ_I:
	   lkfreqapvals[ 0 ] = *ac_cur++;
	   lkfreqapvals[ 1 ] = *ac_cur++;
	   DPRINT2( 2, "LOCKFREQ_I: low word: 0x%x, high word: 0x%x\n",
		        lkfreqapvals[ 0 ], lkfreqapvals[ 1 ] );
	   setLockFreqAPfromShorts( &lkfreqapvals[ 0 ], FALSE );
	   break;

/*  Set lock gain, phase or power, obtaining the value from the A-code stream  */

	case LOCKGAIN_I:
	case LOCKPHASE_I:
	case HOMOSPOIL:
	   setLockPar( *acode, (int) *ac_cur++, FALSE );
	   break;

	case LOCKPOWER_I:
	   setLockPower( (int) *ac_cur++, FALSE );
	   break;

/*  Set lock gain, phase or power using a Real Time variable  */

	case LOCKGAIN_R:
	   setLockPar(LOCKGAIN_I, (int) rt_tbl[*ac_cur++], FALSE );
	   break;
	case LOCKPHASE_R:
	   setLockPar( LOCKPHASE_I, (int) rt_tbl[ *ac_cur++ ], FALSE );
	   break;

	case HOMOSPOIL_R:
	   setLockPar( HOMOSPOIL, (int) rt_tbl[ *ac_cur++ ], FALSE );
	   break;

	case LOCKPOWER_R:
	   setLockPower( (int) rt_tbl[ *ac_cur++ ], FALSE );
	   break;

	case LOCKFREQ_R:
           DPRINT2(2,"LOCKFREQ_R: FreqVal: 0x%lx, %ld\n",rt_tbl[ *ac_cur ],rt_tbl[ *ac_cur ]);
	   setLockFreqAPfromLong( (int) rt_tbl[ *ac_cur++ ], FALSE );
	   break;

	case LOCKSETTC:
	   DPRINT1(2,"Set long TC = %d\n", *ac_cur);
	   storeLockPar( *acode, (int) *ac_cur++ );
	   break;

	case LOCKACQTC:
	   DPRINT1(2,"Set short (acq) TC = %d\n", *ac_cur);
	   storeLockPar( LOCKSETACQTC, (int) *ac_cur++ );
           if ( (int) rt_tbl[ *ac_cur++ ] == 0)
           {
	      setLockPar( *acode, DUMMYINT, FALSE );
           }
	   break;

	case LOCKSETTC_R:
	   DPRINT1(2,"Set long TC = %d\n", rt_tbl[ *ac_cur ]);
	   storeLockPar( LOCKSETTC, (int) rt_tbl[ *ac_cur++ ] );
	   break;

	case LOCKACQTC_R:
	   DPRINT1(2,"Set short (acq) TC = %d\n", rt_tbl[ *ac_cur ]);
	   storeLockPar( LOCKSETACQTC, (int) rt_tbl[ *ac_cur++ ] );
           if ( (int) rt_tbl[ *ac_cur++ ] == 0)
           {
	      setLockPar( LOCKACQTC, DUMMYINT, FALSE );
           }
	   break;

	case LOCKMODE_R:
	   DPRINT1(2,"LOCKMODE_R: %d\n", rt_tbl[ *ac_cur] );
	   setLockPar(LOCKMODE_I, rt_tbl[ *ac_cur++ ], FALSE);
	   break;

	case LOCKMODE_I:
	   DPRINT1(2,"LOCKMODE_I: %d\n", *ac_cur);
	   setLockPar(LOCKMODE_I, *ac_cur++, FALSE);
	   break;

	case LOCKAUTO:
           if ( (int) rt_tbl[ *ac_cur ] == 0)
           {
              int ret;
              short lockmode,lpwrmax,lgainmax;

              lockmode = (short) (*(ac_cur + 1));
              lpwrmax  = (short) (*(ac_cur + 2));
              lgainmax = (short) (*(ac_cur + 3));
	      rngBufPut(pSyncActionArgs,(char*) &lockmode,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &lpwrmax,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &lgainmax,sizeof(short));
	      signal_syncop(LOCKAUTO,-1L,-1L);  /* signal & stop fifo */
	      fifoStart(pTheFifoObject);
	      DPRINT(0,"==========> LOCKAUTO: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	      DPRINT(0,"==========> LOCKAUTO: Resume Parser\n");
           }
	   ac_cur += 4;
	   break;

    /*  If this A-code decides to autoshim, it sends a message to
        the Signal Handler via the FIFO and then takes a semaphore.
        The semaphore is not expected to be available until the auto
        shim operation completes.  Shimming on the FID introduces an
        additional complication, since A-codes have to be interpreted
        to get a FID.  Thus this program will run again, but the
        signal handler task will be the one running it.  It turned
        out the flag marking whether the FIFO had been started or not
        had to be saved. the autoshim operation will start the FIFO;
        and when it returns the FIFO will be flagged as having been
        started although it now has halted.  Thus when the "regular"
        experiment resumes, the FIFO does not get started when it
        completes, causing problems in the console and associated
        host computer programs.   April 1997				*/

#define   FREE_METHOD_STRING ((short) 1)
#define   FREE_METHOD_STRING_NOT ((short) 0)

	case SHIMAUTO:
           {
              short ctval,whenshim,shimmode,len;
	      short ac_offset, ntindx, npindx, dpfindx;
              int bytelen;
              short maxpwr,maxgain;
	      short *methodstr;

	      DPRINT(2,"SHIMAUTO...\n");

              ctval    = *ac_cur++;
              whenshim = *ac_cur++;
              shimmode = *ac_cur++;
              len      = *ac_cur++;

	      ac_offset = *ac_cur++;
	      ac_offset--;	/* back off one since APint() adds one */
	      ntindx = *ac_cur++;
	      npindx = *ac_cur++;
	      dpfindx = *ac_cur++;
              maxpwr  = *ac_cur++;
              maxgain = *ac_cur++;
	
	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);

              if ( ( shimmode & 1 ) && 
	        chkshim((int)whenshim,(int)rt_tbl[ctval],&sampleHasChanged) )
              {
                 int ret;
	         int fifoStartFlag;

	         DPRINT(0,"Would call autoshim .....\n");
		 /* malloc space for method string & put in the method string */
		 bytelen = ((int)len * sizeof(short));
	         methodstr = (short*) malloc(bytelen);
		 if ( methodstr == NULL )
		 {
	             DPRINT(1,"autoshim failed\n");
		     APint_run = FALSE;
		     status = SYSTEMERROR + MALLOCNOSPACE;
    		     errLogRet(LOGIT,debugInfo,
				"AutoShim Malloc Error: ");
	      	     ac_cur += len;
		     break;
		 }
		 memcpy((char*) methodstr,(char*) ac_cur, bytelen);
		 autoShim(pAcodeId,ntindx,npindx,dpfindx,maxpwr,maxgain,
						     ac_offset,methodstr,FREE_METHOD_STRING);
              }
	      ac_cur += len;
           }
	   break;

	case JSHIMAUTO:
           {
              short ctindx,whenshim,shimmode,len;
	      short ac_offset,ntindx,npindx,dpfindx;
              int bytelen,shimatfid,fidcnt;
              short maxpwr,maxgain;
	      short *methodstr;

	      DPRINT(2,"JSHIMAUTO...\n");
              ctindx    = *ac_cur++;
              whenshim = *ac_cur++;
              shimmode = *ac_cur++;
              len      = *ac_cur++;
	      ac_offset = *ac_cur++;
	      ac_offset--;	/* back off one since APint() adds one */

	      shimatfid = (int) rt_tbl[*ac_cur++];
	      fidcnt = (int) rt_tbl[*ac_cur++];
	      DPRINT4(2,">>>  whenshim = %d shimmode = %d shimatfid = %d ct = %d\n",
	             whenshim,shimmode,shimatfid,rt_tbl[ctindx]);
	      /* set shimatfid to 1 if fidcnt mod shimatfid equals zero */
	      if (( shimmode & 1 ) && (shimatfid > 0) )
	      {
	          shimatfid = fidcnt%shimatfid;
	      }
	      if (shimatfid == 0) shimatfid = 1;
	      npindx = *ac_cur++;
	      ntindx = *ac_cur++;
	      dpfindx = *ac_cur++;
              maxpwr  = *ac_cur++;
              maxgain = *ac_cur++;
	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	
              if ( (shimatfid) &&
               chkshim((int) whenshim,(int) rt_tbl[ ctindx ],&sampleHasChanged) )
              {
	         DPRINT(0,"Would call autoshim .....\n");
		 /* set address of method string */
	         methodstr = (short*) &rt_tbl[*ac_cur];
		 if ( len == 0 )
		 {
	             DPRINT(1,"autoshim failed\n");
		     APint_run = FALSE;
		     status = SHIMERROR + SHIMNOMETHOD;
    		     errLogRet(LOGIT,debugInfo,
				"AutoShim Method String Error: len = %d",len);
	      	     ac_cur++;  /* increment past method string pointer */
		     break;
		 }
		 autoShim(pAcodeId,ntindx,npindx,dpfindx,maxpwr,maxgain,
						     ac_offset,methodstr,FREE_METHOD_STRING_NOT);
              }
	      ac_cur++;  /* increment past method string pointer */
           }
	   break;

	case AUTOGAIN:
           {
              short ntindx,npindx,rcvgindx,apaddr,maxval,apdly,minval,stepval;
	      short dpfindx,adcbits;
	      short ac_offset;

	      DPRINT(0,"AUTOGAIN.....\n");
              ntindx    = *ac_cur++;
              npindx    = *ac_cur++;
              dpfindx   = *ac_cur++;
              apaddr    = *ac_cur++;
              apdly     = *ac_cur++;
              rcvgindx  = *ac_cur++;
              maxval    = *ac_cur++;
              minval    = *ac_cur++;
              stepval    = *ac_cur++;
              adcbits    = *ac_cur++;	/* 12, 16 or 20 bits */
	      ac_offset = *ac_cur++; /* offset to acodes to acquire FID */
	      ac_offset--;	/* back off one since APint() adds one */
	      DPRINT2(2,"Current Acode location: 0x%lx, Acode = %d\n",
		ac_cur,*ac_cur);
	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	      DPRINT4(2,">>>  dpf (rt_tbl[%d]) = %d, recvgain (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx],rcvgindx,rt_tbl[rcvgindx]);

	      autoGain(pAcodeId,ntindx,npindx,rcvgindx,apaddr,apdly,maxval,
	           minval,stepval,dpfindx,adcbits,ac_offset);
           }
	   break;

	case JAUTOGAIN:
           {
              short ntindx,npindx,rcvgindx,apaddr,maxval,apdly,minval,stepval;
	      short dpfindx,adcbits;
	      short ac_offset;

	      DPRINT(0,"JAUTOGAIN.....\n");
              ntindx    = *ac_cur++;
              npindx    = *ac_cur++;
              dpfindx   = *ac_cur++;
              apaddr    = *ac_cur++;
              apdly     = *ac_cur++;
              rcvgindx  = *ac_cur++;
              maxval    = *ac_cur++;
              minval    = *ac_cur++;
              stepval    = *ac_cur++;
              adcbits    = *ac_cur++;	/* 12, 16 or 20 bits */
              
              /* offset to acodes to acquire FID */
	      ac_offset = (short) rt_tbl[ *ac_cur++ ]; 
	      ac_offset--;	/* back off one since APint() adds one */
	      DPRINT2(2,"Current Acode location: 0x%lx, Acode = %d\n",
		ac_cur,*ac_cur);
	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	      DPRINT4(2,">>>  dpf (rt_tbl[%d]) = %d, recvgain (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx],rcvgindx,rt_tbl[rcvgindx]);

	      autoGain(pAcodeId,ntindx,npindx,rcvgindx,apaddr,apdly,maxval,
	           minval,stepval,dpfindx,adcbits,ac_offset);
           }
	   break;

	case LOCKZ0_I:
          {
              short dacs[4];

              set_shimset( *ac_cur );
              establishShimType( *ac_cur++ );
              dacs[0]=0;  /* Ingored */
              dacs[1]=(short) 1; /* Z0 DAC */
              dacs[2]=(short) *ac_cur++;
	      DPRINT2(3,"Shim %d = %d\n", (int) dacs[1], (int) dacs[2]);
              shimHandler( dacs, 3 );
           }
	   break;

	case LOCKCHECK:
           {
              DPRINT(1,"LOCKCHECK\n");
	      setLkErrMode((int) (*ac_cur++));
	      setLKInterlk(1);
           }
	   break;

	case JLOCKCHECK:
           {
              DPRINT(1,"JLOCKCHECK\n");
	      setLkErrMode((int) rt_tbl[ *ac_cur++ ]);
	      setLKInterlk(1);
           }
	   break;


	case SETVT:
           {
              short vttype,pid,temp,ltmpxoff,Tmpintlk;
              int rstat;

              vttype = *ac_cur++;
              pid    = *ac_cur++;
              temp = *ac_cur++;
              ltmpxoff = *ac_cur++;
              Tmpintlk      = *ac_cur++;
              DPRINT5(1,"SETVT: type: %d, pid; %d, temp: %d, ltmpxoff: %d, intlk = %d\n",
		vttype, pid,temp,ltmpxoff, Tmpintlk);


	      if (vttype == 0)
              {
	         setVTtype((int) vttype);
	         break;
              }

              /*  These are now done in shandler, otherwise FOO could occur.
               *
	       *  setVTtype((int) vttype);
	       *  setVTinterLk( ((Tmpintlk != 0) ? 1 : 0) );
	       *  setVTErrMode(Tmpintlk);
               */

	      /* 1st place the arguments that setVT will need in the pSyncActionArgs
		 ring buffer.
		 setVT(temp,ltmpxoff,pid)
		 The Tag interrupt, will result the shandler calling setVT()
	      */
	      rngBufPut(pSyncActionArgs,(char*) &vttype,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &Tmpintlk,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &pid,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &temp,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &ltmpxoff,sizeof(short));
	      signal_syncop(SETVT,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;

	case JSETVT:
           {
              short vttype,pid,temp,ltmpxoff,Tmpintlk;
              int rstat;

              vttype = *ac_cur++;
              pid    = (short)rt_tbl[ *ac_cur++ ];
              temp = (short)rt_tbl[ *ac_cur++ ];
              ltmpxoff = (short)rt_tbl[ *ac_cur++ ];
              Tmpintlk      = (short)rt_tbl[ *ac_cur++ ];
              DPRINT5(1,"SETVT: type: %d, pid; %d, temp: %d, ltmpxoff: %d, intlk = %d\n",
		vttype, pid,temp,ltmpxoff, Tmpintlk);

	      if (vttype == 0)
              {
	         setVTtype((int) vttype);
	         break;
              }

              /*  These are now done in shandler, otherwise FOO could occur.
               *
	       *  setVTtype((int) vttype);
	       *  setVTinterLk( ((Tmpintlk != 0) ? 1 : 0) );
	       *  setVTErrMode(Tmpintlk);
               */

	      /* 1st place the arguments that setVT will need in the pSyncActionArgs
		 ring buffer.
		 setVT(temp,ltmpxoff,pid)
		 The Tag interrupt, will result the shandler calling setVT()
	      */
	      rngBufPut(pSyncActionArgs,(char*) &vttype,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &Tmpintlk,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &pid,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &temp,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &ltmpxoff,sizeof(short));
	      signal_syncop(SETVT,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;

	case WAIT4VT:
           {
              short timeout;
	      short errmode;
              int rstat;

              /* ac_cur++;		/* skip high order timeout */

	      errmode = *ac_cur++;		/* Error or Warning */

              timeout = *ac_cur++;

              DPRINT1(1,"WAIT4VT: timeout period: %d sec\n",timeout);

	      if (getVTtype() == 0)
		break;


	      /* 1st place the arguments that wait4VT will need in the pSyncActionArgs
		 ring buffer.
		 wait4VT(timeout)
		 The Tag interrupt, will result the shandler calling wait4VT()
	      */
	      rngBufPut(pSyncActionArgs,(char*) &timeout,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &errmode,sizeof(short));
	      signal_syncop(WAIT4VT,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;


	case JWAIT4VT:
           {
              short timeout;
	      short errmode;
              int rstat;

              /* ac_cur++;		/* skip high order timeout */

              errmode    = (short)rt_tbl[ *ac_cur++ ]; /* Error or Warning */

              timeout    = (short)rt_tbl[ *ac_cur++ ]; 

              DPRINT1(1,"JWAIT4VT: timeout period: %d sec\n",timeout);

	      if (getVTtype() == 0)
		break;


	      /* 1st place the arguments that wait4VT will need in the pSyncActionArgs
		 ring buffer.
		 wait4VT(timeout)
		 The Tag interrupt, will result the shandler calling wait4VT()
	      */
	      rngBufPut(pSyncActionArgs,(char*) &timeout,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &errmode,sizeof(short));
	      signal_syncop(WAIT4VT,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;


	case SETSPIN:
           {
              short bumpFlag,speed,MasThreshold;
              int rstat;

              speed = *ac_cur++;
              MasThreshold  = *ac_cur++;
              bumpFlag = *ac_cur++;
              DPRINT3(1,"SETSPIN: speed: %d, MasThreshold; %d, bump flag: %d\n",
			 speed,MasThreshold,bumpFlag);

	      /* setSpintype((int) MasThreshold); liquids/solids */

	      /* 1st place the arguments that setspinnreg will need in the pSyncActionArgs
		 ring buffer.
		 setspinnreg(speed,MasThreshold)
		 The Tag interrupt, will result the shandler calling setspinnreg()  (vtfuncs.c)
	      */
	      rngBufPut(pSyncActionArgs,(char*) &speed,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &MasThreshold,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(short));
	      signal_syncop(SETSPIN,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;

	case JSETSPIN:
           {
              short bumpFlag,speed,MasThreshold;
              int rstat;

	      speed = (short) rt_tbl[*ac_cur++];
	      MasThreshold = (short) rt_tbl[*ac_cur++];
              bumpFlag = (short) rt_tbl[*ac_cur++];
              DPRINT3(1,"JSETSPIN: speed: %d, MasThreshold; %d, bump flag: %d\n",
			 speed,MasThreshold,bumpFlag);

	      /* setSpintype((int) MasThreshold); liquids/solids */

	      /* 1st place the arguments that setspinnreg will need in the pSyncActionArgs
		 ring buffer.
		 setspinnreg(speed,MasThreshold)
		 The Tag interrupt, will result the shandler calling setspinnreg()  (vtfuncs.c)
	      */
	      rngBufPut(pSyncActionArgs,(char*) &speed,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &MasThreshold,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(short));
	      signal_syncop(SETSPIN,-1L,-1L);  /* signal & stop fifo */
	   }
	   break;

	case CHECKSPIN:
           {
              short bumpFlag,delta,errmode;

              delta = *ac_cur++;
              errmode = *ac_cur++;
              bumpFlag = *ac_cur++;
              DPRINT(1,"CHECKSPIN\n");
	      rngBufPut(pSyncActionArgs,(char*) &delta,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &errmode,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(short));
	      signal_syncop(CHECKSPIN,-1L,-1L);  /* signal & stop fifo */
/*
	      setSpinRegDelta((*ac_cur++));
	      setSpinErrMode((*ac_cur++));
	      setSpinInterlk(2);
*/
           }
	   break;

	case JCHECKSPIN:
           {
              short bumpFlag,delta,errmode;

	      delta = (short) rt_tbl[*ac_cur++];
	      errmode = (short) rt_tbl[*ac_cur++];
              bumpFlag = (short) rt_tbl[*ac_cur++];
              DPRINT(1,"JCHECKSPIN\n");
	      rngBufPut(pSyncActionArgs,(char*) &delta,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &errmode,sizeof(short));
	      rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(short));
	      signal_syncop(CHECKSPIN,-1L,-1L);  /* signal & stop fifo */
           }
	   break;


	case XGATE:
           {
              short rtparm ,count;
              int rstat;

              rtparm = *ac_cur++;
	      DPRINT1(0,"XGATE: rtflag = %d\n",rtparm);
              if (rtparm)
	         count = rt_tbl[*ac_cur++];
	      else
	         count = *ac_cur++;

	      DPRINT1(0,"XGATE: count = %d\n",count);
              if ( count > 0)
              {
	        if ( (rstat = fifoExternGate(pTheFifoObject,(int) count)) != 0)
                {
		  APint_run = FALSE;
		  status = rstat;
    		  errLogRet(LOGIT,debugInfo,
			"Fifo Board Not Present: errorcode is %d",rstat);
                }
              }
	   }
	   break;

	case ROTORSYNC_TRIG:
           {
              short rtparm ,count;
              int rstat;

              rtparm = *ac_cur++;
	      DPRINT1(1,"ROTORSYNC: rtflag = %d\n",rtparm);
              if (rtparm)
	         count = rt_tbl[*ac_cur++];
	      else
	         count = *ac_cur++;
	      DPRINT1(1,"ROTORSYNC: count = %d\n",count);
              if ( count > 0)
              {
	        if ( (rstat = fifoRotorSync(pTheFifoObject,count)) != 0)
                {
		  APint_run = FALSE;
		  status = rstat;
    		  errLogRet(LOGIT,debugInfo,
			"Rotor Sync Hardware Not Present: errorcode is %d",rstat);
                }
              }
	   }
	   break;

	case READHSROTOR:
           {
              short rtindex,delay;
              int rstat;

              rtindex = *ac_cur++;
	      rstat = fifoRotorRead(pTheFifoObject,&(rt_tbl[rtindex]));
	      if ( rstat != 0)
              {
		APint_run = FALSE;
		status = rstat;
    		errLogRet(LOGIT,debugInfo,
		 "Rotor Sync Hardware Not Present: errorcode is %d",rstat);
              }
	      DPRINT2(1,">>>  rotor speed (rt_tbl[%d]) = %d\n",
		rtindex,rt_tbl[rtindex]);
	   }
	   break;

	case SYNC_PARSER:
           {
	      DPRINT(1,"SYNC_PARSER: \n");
 	      /* signal & delay fifo */
              if ( (*(int *) (ac_cur) != -1) && (*(int *) (ac_cur+2) != -1) )
              {
	         signal_syncop(SYNC_PARSER,getuint(ac_cur),getuint(ac_cur+2)); 
	         fifoStart(pTheFifoObject);
              }
              else
              {
                 update_acqstate(ACQ_SYNCED);
                 getstatblock();   /* force statblock upto hosts */
              }
	      DPRINT(0,"==========> SYNC_PARSER: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif

              ac_cur += 4;
	      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	      DPRINT(0,"==========> SYNC_PARSER: Resume Parser\n");
	   }
	   break;

	case JSYNC_PARSER:
           {
	      DPRINT(1,"JSYNC_PARSER: \n");
 	      /* signal & delay fifo */
              if ( ((int)rt_tbl[*ac_cur] != -1) && ((int)rt_tbl[*ac_cur+1] != -1) )
              {
	         signal_syncop(SYNC_PARSER,(int)rt_tbl[*ac_cur],
	                                   (unsigned long) rt_tbl[*ac_cur+1]); 
	         fifoStart(pTheFifoObject);
              }
              else
              {
                 update_acqstate(ACQ_SYNCED);
                 getstatblock();   /* force statblock upto hosts */
              }
	      DPRINT(0,"==========> SYNC_PARSER: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif

              ac_cur++;
	      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	      DPRINT(0,"==========> SYNC_PARSER: Resume Parser\n");
	   }
	   break;

	case NOISE_CMPLT:
           {
	      DPRINT(1,"NOISE_CMPLT: \n");
   	      if (pAcodeId->interactive_flag != ACQ_AUTOGAIN)
	      {
	        signal_syncop(NOISE_CMPLT,-1L,-1L);  /* signal & stop fifo */
	        fifoStart(pTheFifoObject);
	        DPRINT(0,"==========> NOISE_CMPLT: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	        semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
                fifoResetStufferFlagNSem(pTheFifoObject);
	        DPRINT(0,"==========> NOISE_CMPLT: Resume Parser\n");
	      } 
              else
	      {
	        DPRINT(2,">> Skipped do to AUTOGAIN\n");
              }

#ifdef XXX
	      signal_syncop(NOISE_CMPLT,-1L,-1L);  /* signal & stop fifo */
	      fifoStart(pTheFifoObject);
	      DPRINT(0,"==========> NOISE_CMPLT: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	      DPRINT(0,"==========> NOISE_CMPLT: Resume Parser\n");
#endif 
	   }
	   break;



/* It's called a SOFTDELAY Immediate because it gets the delay directly
   from the A_code stream, instead of referencing some location in memory.  */

	case SOFTDELAY_I:
	   taskDelay( (int) (*ac_cur++) );
	   break;

	/* WGLOAD							*/
	/*  arguments: 							*/
	/*  0 - short word count: 0					*/
	/*  loads waveform generators from buffer "<exp>wf"		*/
	case WGLOAD:
	   DPRINT(2,"WGLOAD .....\n");
	   wgload(pAcodeId);
	   break;

	case SSHA_INOVA:
	   {
	      int value;
	      int ssha_arg;
	      int ssha_passvalue;

	      value = *(short *) (ac_cur++);
	      ssha_arg = *(short *) (ac_cur++);
	      /*errLogRet( LOGIT, debugInfo, "SSHA: %d\n", value );*/
	      DPRINT2(1, "SSHA_INOVA: value=%d mask=%d\n", value, ssha_arg );
	      if (ssha_activated)
	      {
/* add value+delaytime, interpret sum down on AP bus, make delaytime multiple of 10 sec */
/* also send masked bits for which shims to use */
/*		  setshimviaAp(SSHA_INDEX,value); */
		  ssha_passvalue = value;
		  if (value == 1)
		    ssha_arg = (ssha_arg-1)/10;
		  ssha_passvalue += (ssha_arg << 2);
		  setshimviaAp(SSHA_INDEX,ssha_passvalue);
		  if (value == 1)
		    ssha_started = 1;
	      }
	   }
	   break;

	case JBKGDHWSHIM:    /* JPSG variant of SSHA or BacKGound HardWare Shimming */
	   {
	      int willshim,masklen,maskloc,maskval,gateon;
	      int ssha_arg;
	      int ssha_passvalue;

	      /* Global on & off flag, 0 - turn off BG shimming, 1- turn on BG shimming */
	      /* these are rt_tbl entries so that hwshimming could be turn off interactively */

	      willshim = rt_tbl[ *ac_cur++ ];
	      maskloc = *(short *) (ac_cur);
	      maskval = rt_tbl[ *ac_cur++ ];
	      gateon = rt_tbl[ *ac_cur++ ];


	      DPRINT4(0,"JBKGDHWSHIM: willshim: %d, maskloc: %d, maskval: 0x%x, turnon: %d\n",
			willshim, maskloc,maskval,gateon);
	     
	      if (willshim)
	      {
		  setshimviaAp(JHWSHIM, (maskval & 0xfc) | (gateon & 0x1));

		  if (gateon == 1)
		    ssha_started = 1;
	      }
	   }
	   break;

/*
	case SSHAMASK_INOVA:
	   {
	      int sshamask[ 2 ];

	      sshamask[ 0 ] = *(int *) (ac_cur);
	      ac_cur = ac_cur + 2;
	      sshamask[ 1 ] = *(int *) (ac_cur);
	      ac_cur = ac_cur + 2;
	      errLogRet( LOGIT, debugInfo, "SSHA mask: 0x%08x 0x%08x\n", sshamask[ 0 ], sshamask[ 1 ] );
	      DPRINT2(-1 , "SSHAMASK_INOVA mask: 0x%08x 0x%08x\n", sshamask[ 0 ], sshamask[ 1 ] );
	   }
	   break;
*/
	default: 
	   errLogRet(LOGIT,debugInfo,
		"NO SUCH CODE %d, length: %d, (address: 0x%x)\n",
		*(acode),alength, acode);
	   for (i=0; i<alength; i++)
	   {
	       DPRINT2(0," i: %d   code: 0x%x\n",i,*ac_cur);
	       ac_cur++;
	   }
	   status = HDWAREERROR + INVALIDACODE;
	   APint_run = FALSE;
	   break;

      } /* end of SWITCH */
      if ((int) ac_cur != (int)(acode + alength + 2) )
      {
	errLogRet(LOGIT,debugInfo,
		"ACODE LENGTH MISMATCH: acode=%d acode_ptr=0x%x length=%d\n",
			*acode,acode,alength);
        errLogRet(LOGIT,debugInfo," ac_cur (0x%x) != codes (0x%x)\n",
                   (int) ac_cur, (int)(acode + alength + 2));
	status = HDWAREERROR + INVALIDACODE;
	APint_run = FALSE;
      }
   }

#ifdef FIFOBUFOBJ
   /* Done Parsing, Force any fifo words in the buffers to be put into the FIFO */
   fifoBufForceRdy(pTheFifoObject->pFifoWordBufs);  /* Must do This in a Task Context not Interrupt Context */
#endif

   DPRINT(2,"finish APint \n");
   APint_run = TRUE;
   if (AP_Suspend == TRUE)
   {
      DPRINT(2,"APint abort, Suspend task\n");
#ifdef INSTRUMENT
      wvEvent(EVENT_PARSE_SUSPENDED,NULL,NULL);
#endif
      taskSuspend(0);
      AP_Suspend = FALSE;
   }
   return(status);
}

/************************************************************************/
/* APint Routines							*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/* init_fifo								*/
/* 	Initializes the fifo object.  For debugging, can open a nfs 	*/
/*	file for writing.  The file will be /tmp/<id>.Fifo		*/
/*	Checks the interactive flag of acode object and enables sw1	*/
/*	interrupt.							*/
/*	Arguments:							*/
/*		flags	: bit0 - 1=Extra HSL's				*/
/*			: bit1 - 1=Output to nfs file			*/
/*		size	: unclear at the moment but could be		*/
/*			  the number of scans/array elements ahead to	*/
/*			  stuff.					*/
/*----------------------------------------------------------------------*/
int init_fifo(ACODE_ID pAcodeId, short flags, short size)
{
 int status;
#ifdef XXXX
 char fifofile[128];
   if (OPTHSLINES_BIT & flags)
	opthslines_flag = TRUE;
   if (FIFODEBUG_BIT & flags)
   {
	
	if (createFifofile(id) > 0)
	   fifoStuffRtn = &fifoWriteIt;
	else
	   return(-1);

   }

   return(0);
#endif
   if (FIFODEBUG_BIT & flags)
   {
	if ((status = fifoInitLog(pTheFifoObject)) < 0)
	   return(status);
   }
   else
   {
	fifoReset(pTheFifoObject,  RESETSTATEMACH |
				RESETTAGFIFO | RESETAPRDBKFIFO);
	/* Note: SW2_I will be enabled if stop acquisition requested */
	fifoItrpEnable(pTheFifoObject, FSTOPPED_I | FSTRTEMPTY_I | FSTRTHALT_I |
			   NETBL_I | FORP_I | PFAMFULL_I | TAGFNOTEMPTY_I |
			   SW1_I | SW3_I | SW4_I ); 
	autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS);
   }

   return(0);
}

/*----------------------------------------------------------------------*/
/* init_stm								*/
/* 	Initializes the stm objects. Allocates max buffer size and 	*/
/*	total number of desired buffers					*/
/*	Arguments:							*/
/*		numpoints	: number of data points			*/
/*		numbytes	: number of bytes per data point	*/
/*		tot_elem	: number of total data elements		*/
/*				  including blocksizes			*/
/*----------------------------------------------------------------------*/
void init_stm(long *activeRcvrs, ACODE_ID pAcodeId,
	      ulong_t numpoints, ulong_t numbytes, ulong_t tot_elem, int SwGT500KHz)
{
   ulong_t loc_npsize;
   STMOBJ_ID pStmObj;
   int i;
   int j;

   DPRINT5(2, "init_stm: activeRcvrs: 0x%lx, np %ld, nbytes %ld, total elements %ld, SW>500K %d\n",
               *activeRcvrs, numpoints, numbytes,tot_elem,SwGT500KHz);
   for (i=-1, j=0, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       DPRINT2(2,"\t STM[%d] 0x%x  - init \n", i,pStmObj);
       /* Clear High Speed STM ADC for next fid. */
       stmAdcOvldClear(pStmObj);

       /* It's the 1MHz STM/ADC need to select which input Liquids or
          WideLine.  This AP COntrol bit (0xa) will be orred in/out within
          the stmGenCntrlCmds() function. 
       */
       stmInputSwitch(pStmObj,SwGT500KHz);

       /* ADM_ADCOVFL_MASK is unused in standard STM
        * so it's OK to set all the time */
       stmItrpEnable(pStmObj, (RTZ_ITRP_MASK | RPNZ_ITRP_MASK | 
			       MAX_SUM_ITRP_MASK | APBUS_ITRP_MASK |
			       ADM_ADCOVFL_MASK));

       loc_npsize = numpoints*numbytes;
       if (pAcodeId->interactive_flag == ACQI_INTERACTIVE)
	   stmInitial(pStmObj, tot_elem, loc_npsize, pUpLinkIMsgQ, j++);
       else
	   stmInitial(pStmObj, tot_elem, loc_npsize, pUpLinkMsgQ, j++);
   }
}

/*----------------------------------------------------------------------*/
/* init_adc								*/
/* 	Initializes the adc object.  For debugging, can open a nfs 	*/
/*	Arguments:							*/
/*		flags	: bit0-bit3 = select audio inputs		*/
/*				0 = rcvr1				*/
/*				1 = rcvr2				*/
/*				2 = lock				*/
/*				3 = test				*/
/*				4-15 = spare				*/
/*			  bit5-bit7 = spare				*/
/*			  bit8 = Enable rcvr overload			*/
/*			  bit9 = Enable ADC overload			*/
/*			  bit10 = Enable CTC				*/
/*			  bit11 = Enable DSP				*/
/*		spare	: unclear at the moment, possibly adc select	*/
/*----------------------------------------------------------------------*/
void init_adc(long *activeRcvrs, unsigned short flags,
	      unsigned long *adccntrl)
{
    ushort_t adcistat;
    ADC_ID pAdcObj;
    int i;

    DPRINT3(2, "init_adc: activeRcvrs: 0x%lx, flags=%d,  adccntrl=0x%lx\n",
		*activeRcvrs, flags, *adccntrl);
    for (i=-1, pAdcObj=adcGetActive(activeRcvrs, &i);
	 pAdcObj;
	 pAdcObj=adcGetActive(activeRcvrs, &i))
    {
	DPRINT3(2, "\tActive ADC[%d] 0x%lx, adccntrl=0x%lx -  init\n", 
			i, pAdcObj,*adccntrl);
	/* Clear ADC and Receiver Overload Flag, Enable interrupts */
	adcOvldClear(pAdcObj);

	    /* read interrupt register and clear any lingering latched states */
	adcistat = adcItrpStatReg(pAdcObj);

	if (flags == 1 /*HS_DTM_ADC*/)
	{
	    /* Using HS STM_ADC; disable interrupts on std ADC*/
	    adcItrpDisable(pAdcObj, ADC_ALLITRPS);
	}
    }
}

/*----------------------------------------------------------------------*/
/* delay								*/
/*	This routine has two arguments. The first argument is the	*/
/*	number of seconds to delay the second argument is the number	*/
/*	of 12.5 ns ticks to delay.					*/
/*	This routine will take the number of seconds add to that number	*/
/*	the number seconds that are in the ticks argument.  The 	*/
/*	remainder of ticks are stuffed into the fifo. followed by a	*/
/*	delay word for each second.					*/
/*----------------------------------------------------------------------*/
void delay(int seconds, unsigned long ticks)
{
   DPRINT2( 1, "delay starts with seconds: %d ticks: %d\n",seconds,ticks );
   seconds = seconds + (int)(ticks/(unsigned int)NUM_TICKS_SECOND);
   ticks = ticks%NUM_TICKS_SECOND;
   if (seconds > 0)
   {
	int i;
	if (ticks > 0)
	{
	   if (ticks >= MIN_DELAY_TICKS)
		fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks);
		/* fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks-HW_DELAY_FUDGE); */
	   else
	   {
		fifoStuffCmd(pTheFifoObject,CL_DELAY,ONE_SECOND_DELAY+ticks);
		seconds--;
	   }
	}
	for (i=1; i<=seconds; i++)
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,ONE_SECOND_DELAY);
   }
   else
   {
	if (ticks >= MIN_DELAY_TICKS)
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks);
	   /* fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks-HW_DELAY_FUDGE); */
	else
	{
	   DPRINT(3,"delay: Delay less than delay minimum \n");
	}
   }
}

/*----------------------------------------------------------------------*/
/* vdelay								*/
/*	This routine has 3 arguments. The first argument is a 		*/
/*	multiplier value, the second is the value to be multiplied, 	*/
/*	and the third is an offset.  The increment and the offset are	*/
/*	in ticks. 						 	*/
/*	The routine will multiply the first argument by the second,	*/
/*	add the third argument, and call the delay routine.		*/
/*----------------------------------------------------------------------*/
void vdelay(long multval, long increment, long offset)
{
   long long ticks;
   int seconds;

   ticks = (long long)multval*(long long)increment + offset;
   seconds = 0;
   if (ticks > NUM_TICKS_SECOND)
   {
	seconds = seconds + (ticks/(long long)NUM_TICKS_SECOND);
   	ticks = ticks%NUM_TICKS_SECOND;
   }
   delay(seconds,(unsigned long)ticks);

}

/*----------------------------------------------------------------------*/
/* incrdelay								*/
/*	This routine has two arguments. The first argument is a 	*/
/*	pointer to a delay which is two longwords the first longword 	*/
/*	is the number of seconds to delay and the second longword is 	*/
/*	the number of 12.5 ns ticks to delay.  The second argument is 	*/
/*	a long integer to multiply the delay.				*/
/*	This routine will multiply the delay by the second argument	*/
/*	and call the delay routine.					*/
/*----------------------------------------------------------------------*/
void incrdelay(long delayword[2], long multval)
{
   long long ticks;
   int seconds;

   DPRINT3( 3, "incrdelay seconds: %d ticks: %d  multval: %d\n",
			delayword[0],delayword[1], multval);

   ticks = delayword[1];
   ticks = ticks*multval;
   seconds = delayword[0]*multval;
   if (ticks > NUM_TICKS_SECOND)
   {
	seconds = seconds + (ticks/(long long)NUM_TICKS_SECOND);
   	ticks = ticks%NUM_TICKS_SECOND;
   }
   delay(seconds,(unsigned long)ticks);

}


/*----------------------------------------------------------------------*/
/* acquire								*/
/*	Performs an acquisition.					*/
/*	Arguments:							*/
/*		flags	: hardware looping flag				*/
/*			  if > 0, provide own hardware loop		*/
/*			: if = 2, multiple hardware loops in use 	*/
/*		np	: number of points 				*/
/*		dwell	: delay between data points in 12.5 ns ticks	*/
/*			  This routine takes care of HW fudge for 1st	*/
/*			  100 ns.					*/
/*	Note: This routine does not check fifo size boundries if it	*/
/*		is being run within another hardware loop.		*/
/*	      This routine limits the dwell time to 2 seconds max.	*/
/*----------------------------------------------------------------------*/
int acquire(short flags, int np, int dwell)
{
   int i, loopsz, numhwloops, remnp, wordsperpoint;
   /*DPRINT4(-1,"----- acquire flag:%d, np: %d, , ctc(s): %d, dwell: %d   ......",
		    flags,np, np>>1,dwell); */
   np = np >> 1;		/* divide by 2 for num of CTC's */

   /* if in parallel channel then put in channel lock */
   if (pTheFifoObject->pPChanObj)
   {
       pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_LOCK,0);
       pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_ACQUIRESTRT,0);
   }

   /* implicit looping to be set in top byte of flags word */
   loopsz = ((ushort)flags & 0xff00) >> 7;	/* 2 - 255*2 */
   flags = flags & 0x00ff;
   /* hardware looping */
   if (flags > 0)
   {
	/* Provide hardware loop. implclpsz must be greater or equal	*/
	/* to two.							*/
	if (loopsz <= 1)
	   loopsz = implclpsz;
	numhwloops = np/loopsz;
   	if ( numhwloops > MAX_HW_LOOPS )
	{
	   loopsz = np/(MAX_HW_LOOPS+1);
	   if ( (np%(MAX_HW_LOOPS+1)) > 0)
		loopsz+=1;
	   numhwloops = np/loopsz;
	}
	remnp = np%loopsz;
	/* added this to handle np < 8 , ie no hardloop required */
        if (numhwloops <= 1)
	{
	   remnp = remnp +  numhwloops*loopsz;
           loopsz = 0;
	}
   }
   else {
	/* no hwlooping or already provided */
	loopsz=np;
	remnp = 0;
	numhwloops = 0;
   }

   /* dwell delay */
   if ( dwell > (2*NUM_TICKS_SECOND) )
   {
	DPRINT(3,"acquire: dwell time greater than 2 seconds \n");
	return(-1);
   }
   if (loopsz > fifolpsize)
   {
	DPRINT(3,"acquire: loop size greater than fifo loop size.\n");
	return(-1);
   }

    /* DPRINT5(-1," -- acquire: %d(lpsiz) * %d(nloops) = %d + %d(rempts) = %d  ctcs\n",
	loopsz,numhwloops,loopsz*numhwloops,remnp,(loopsz*numhwloops)+remnp); */
   /** stuff fifo **/
   dwell = dwell-HW_DELAY_FUDGE;
   if (numhwloops > 1)
   {
	fifoStuffCmd(pTheFifoObject,CL_START_LOOP | CL_END_LOOP,numhwloops-1);
	fifoStuffCmd(pTheFifoObject,CL_START_LOOP | CL_CTC,dwell);
	loopsz-=2;		/* remove start and end hardloop words */
   }
   for (i=0; i<loopsz; i++)
	fifoStuffCmd(pTheFifoObject,CL_CTC,dwell);

   if (numhwloops > 1)
	fifoStuffCmd(pTheFifoObject,CL_CTC | CL_END_LOOP,dwell);

   for (i=0; i<remnp; i++)
	fifoStuffCmd(pTheFifoObject,CL_CTC,dwell);

   /* if in parallel channel then put in channel lock */
   if (pTheFifoObject->pPChanObj)
   {
       pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_ACQUIREEND,0);
       pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_UNLOCK,0);
   }
   return(0);
}

/*----------------------------------------------------------------------*/
/* receivercycle							*/
/* 	Sets receiver cycling on dtm.					*/
/*----------------------------------------------------------------------*/
void receivercycle(long *activeRcvrs, int oph, int *dtmCntrlArray)
{
    ushort stm_control_word;
    unsigned long Codes[10];
    int cnt;
    int i,j;
    int *pDtmCntrl;
    STMOBJ_ID pStmObj;

    DPRINT2(1,"receivercycle: oph: %d, dmcntrl: 0x%lx\n",oph, *dtmCntrlArray);
    for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	 pStmObj;
	 pStmObj=stmGetActive(activeRcvrs, &i))
    {
	pDtmCntrl = dtmCntrlArray + i;

	    /* clear phase bits */
	stm_control_word = *pDtmCntrl & ~STM_AP_PHASE_CYCLE_MASK;

	/* set phase */
	stm_control_word |= (oph & STM_AP_PHASE_CYCLE_MASK);
	*pDtmCntrl = stm_control_word;
	DPRINT2(1,"receivercycle new phase: oph: %d, dmcntrl: 0x%lx\n"
		,oph, *pDtmCntrl);

	cnt = stmGenCntrlCmds(pStmObj, stm_control_word, Codes);

 	fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
 	fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

	/* cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	/* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
    }
}

/*----------------------------------------------------------------------*/
/* clearscandata							*/
/* 	Sets data zero bit on dtm.					*/
/*----------------------------------------------------------------------*/
void clearscandata(long *activeRcvrs, int *dtmCntrlArray)
{
    ushort stm_control_word;
    STMOBJ_ID pStmObj;
    unsigned long Codes[10];
    int cnt;
    int i;
    int *pDtmCntrl;

    for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	 pStmObj;
	 pStmObj=stmGetActive(activeRcvrs, &i))
    {
	pDtmCntrl = dtmCntrlArray + i;

	/* set memory zero */
	*pDtmCntrl |= STM_AP_MEM_ZERO ;
	stm_control_word = *pDtmCntrl;

	cnt = stmGenCntrlCmds(pStmObj, stm_control_word, Codes);

 	fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
 	fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

	/* cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	/* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
    }
}

/*----------------------------------------------------------------------*/
/* initscan								*/
/* 	Sets current number of transients using Completed Blocksizes.	*/
/*	Allocates STM (DPM) buffers.					*/
/*	Arguments:							*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		np	: number of points for the scan			*/
/*		ss	: number of steady state transients		*/
/*		*ssct	: Pointer to Steady State Completed Transients	*/
/*		nt	: Number of Transients.				*/
/*		ct	: Completed Transients				*/
/*		bs	: blocksize					*/
/*		cbs	: Completed blocksizes				*/
/*		maxsum	: maximum accumulated value			*/
/*----------------------------------------------------------------------*/
void initscan(long *activeRcvrs, ACODE_ID pAcodeId,
	      int np, int ss, int *ssct, int nt,
	      int ct, int bs, int cbs, int maxsum)
{
   ushort stm_control_word;
   unsigned long Codes[30];
   int cnt,endct,nt_remaining;
   int i;
   STMOBJ_ID pStmObj;

   DPRINT5(1,"init scan: np:%d ss:%d ssct:%d nt:%d ct:%d",np,ss,*ssct,nt,ct);
   DPRINT3(1," bs:%d cbs:%d maxsum:%d\n", bs, cbs, maxsum);
   *ssct = ss;
   nt_remaining = (nt - ct) + ss;

   DPRINT1(1,"Nt remaining: 0x%lx\n",nt_remaining);

   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       DPRINT2(3,"initscan: rcvr idx = %d, np=%d\n", i, np);
       pStmObj->cur_scan_data_adr = 0;
       pStmObj->initial_scan_num = ct;
       /* set max_sum  */
       cnt = stmGenMaxSumCodes(pStmObj, maxsum, Codes);

       /* set number of transients remaining */
       cnt += stmGenNtCntCodes(pStmObj, nt_remaining, &Codes[cnt]);

       /* set num_points */
       cnt += stmGenNpCntCodes(pStmObj, np, &Codes[cnt]);

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
   }
}

/*----------------------------------------------------------------------*/
/* nextscan								*/
/* 	Sends stm information to the apbus: source, destination addr 	*/
/*	Sets phase mode and if ct == 0 (steady state or first scan) 	*/
/*	zeroes source memory.						*/
/*	Arguments:							*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		pAcodeId	: 					*/
/*		np	: number of points for the scan			*/
/*		dp	: bytes per data point 				*/
/*		nt	: Number of Transients.				*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		dtmCntrlArray	: stm control info			*/
/*		fidnum	: fid number					*/
/*----------------------------------------------------------------------*/
void nextscan(long *activeRcvrs, ACODE_ID pAcodeId,
	      int np, int dp, int nt, int ct, int bs,
	      int *dtmCntrlArray, int fidnum)
{
 unsigned long Codes[55];
 int cnt,endct,modct;
 unsigned long scan_data_adr;
 FID_STAT_BLOCK *p2statb;
 ushort stm_control_word;
 STMOBJ_ID pStmObj;
 int *pDtmCntrl;
 int i;

   DPRINT4( 1, "next scan: np:%d dp:%d nt:%d ct:%d", np, dp, nt, ct);
   DPRINT3( 1, " bs:%d pDtmCntrl:0x%x fidnum:%d\n", bs, *dtmCntrlArray, fidnum);
   if (bs > 0) modct = bs;
   else modct = nt;

   if ((fidnum == 0) && (ct == 0))
   {
       DPRINT2(0,"nextscan: FID: %d, CT: %d, Reseting startFLag to 0\n",
	       fidnum,ct);
       fifoClrStart4Exp(pTheFifoObject);
   }
   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       pDtmCntrl = dtmCntrlArray + i;
       DPRINT2(3,"nextscan(): stm #%d at 0x%x\n", i, pStmObj);
       if ( ((ct % modct) == 0 ) || (pStmObj->cur_scan_data_adr==0) )
       {
	   if ((bs < 1) || ((nt-ct) < bs))
	       endct = nt;
	   else 
	       endct = ct + bs;
	   /* Only get new buffer if start of scan or blocksize */
	   if ((ct != pStmObj->initial_scan_num ) || 
	       ((pStmObj->cur_scan_data_adr==0)))
	   {
	       /* Now if stmAllocAcqBlk() going to block, we need to decide
		* if the experiment has not really been started, if not we
		* should start the FIFO now before we pend.
		* Otherwise everbody will just wait forever.
		*/
	       DPRINT(1,"nextscan: ");
	       DPRINT4(1,"FID=%d, CT=%d, WillPend?=%d, startFlag=%d \n",
		       fidnum, ct, stmAllocWillBlock(pStmObj),
		       fifoGetStart4Exp(pTheFifoObject));
	       if ( stmAllocWillBlock(pStmObj) == 1 )
	       {
		   /* Probable only need to start the fifo if:
		      1. No Task is pended on pAcodeId->pSemParseSuspend
		      semaphore
		      Note : not an issue since the parser would not be
		      here if it was pended.
		      2. No has done the initial start of the fifo for
		      this Exp.
		   */
		   /* if ( (fidnum == 1) && (ct > 1) ) */
		   if ( fifoGetStart4Exp(pTheFifoObject) == 0 )
		   {
		       DPRINT(0,
			      "nextscan: about to block, starting FIFO\n");
#ifdef INSTRUMENT
		       wvEvent(96,NULL,NULL);
#endif
		       fifoStart4Exp(pTheFifoObject);
		   }

		   /* though rare, if buffer being waited on has the CT complete
   		      then by forcing the buffer free then the system will continue
		      otherwise the system will FIFO underflow, however if we do find 
		      the system in this condition then we are real close to FIFO underflowing 
		      and this test may add another few milliseconds before FOOing.
		      Thus I'm not sure the time the test takes is worth it.
		      see. fifoBufTaskPended() in fifoBufObj.c for the details of test.
		      Thus I'm commenting it out for now.
		   */
/*                   if (fifoStufferPendedOnBufs(pTheFifoObject) == 1)	*/
/*		   {	*/
/*#ifdef INSTRUMENT	*/
/*     		        wvEvent(6666,NULL,NULL);	*/
/*#endif	*/
/*    		       fifoBufForceRdy(pTheFifoObject->pFifoWordBufs);  	*/
/*		   }	*/
	       }
	       p2statb = stmAllocAcqBlk(pStmObj, 
					(ulong_t) fidnum, (ulong_t) np,
					(ulong_t) ct, (ulong_t) endct,
					(ulong_t) nt, (ulong_t) (np * dp), 
					(long *)&pStmObj->tag2snd,
					&scan_data_adr );
	   }
	   else
	       scan_data_adr = pStmObj->cur_scan_data_adr;
	   if (pStmObj->cur_scan_data_adr == 0) 
	       pStmObj->cur_scan_data_adr = scan_data_adr;

	   DPRINT2(1, "Source addr: 0x%lx  Dest addr: 0x%lx\n",
		   pStmObj->cur_scan_data_adr, scan_data_adr);
	   /* Send tag word */
	   cnt = stmGenTagCodes(pStmObj, pStmObj->tag2snd, Codes);

	   /* set source Address of Data */
	   cnt += stmGenSrcAdrCodes(pStmObj, pStmObj->cur_scan_data_adr, 
				    &Codes[cnt]);
	   /* set Destination Address of Data */
	   cnt += stmGenDstAdrCodes(pStmObj, scan_data_adr, &Codes[cnt]);
	   pStmObj->prev_scan_data_adr = pStmObj->cur_scan_data_adr;
	   pStmObj->cur_scan_data_adr = scan_data_adr;
       }
       else {

	   /* Send tag word */
	   cnt = stmGenTagCodes(pStmObj, pStmObj->tag2snd, Codes);

	   /* set source Address of Data */
	   cnt += stmGenSrcAdrCodes(pStmObj, pStmObj->cur_scan_data_adr, 
				    &Codes[cnt]);
	   /* set Destination Address of Data */
	   cnt += stmGenDstAdrCodes(pStmObj, pStmObj->cur_scan_data_adr,
				    &Codes[cnt]);
	   pStmObj->prev_scan_data_adr = pStmObj->cur_scan_data_adr;
       }

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

       /* reset num_points */
       /* stm_control_word =  STM_AP_RELOAD_NP | STM_AP_RELOAD_ADDRS; */
       stm_control_word =  STM_AP_RELOAD_NP_ADDRS ;
       cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);

       /* zero source data if steady state first transient (ct == 0) */
       if (ct == pStmObj->initial_scan_num){
	   *pDtmCntrl |= STM_AP_MEM_ZERO;
       }else{
	   *pDtmCntrl &= ~STM_AP_MEM_ZERO;
       }

       cnt += stmGenCntrlCodes(pStmObj, *pDtmCntrl, &Codes[cnt]);

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
   }
}

/*----------------------------------------------------------------------*/
/* endofscan								*/
/* 	Increments Steady State Completed Transient counter or  	*/
/*	Completed Transient counter.  Checks for ct==nt and blocksize	*/
/*	and enables interrupts for either condition			*/
/*	Arguments:							*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		ss	: steady state transients			*/
/*		*ssct	: ptr to steady state completed transients	*/
/*		nt	: number of transients				*/
/*		*ct	: ptr to completed transients			*/
/*		bs	: blocksize					*/
/*		*cbs	: ptr to completed blocksize			*/
/*----------------------------------------------------------------------*/
void endofscan(long *activeRcvrs,
	       int ss, int *ssct, int nt, int *ct, int bs, int *cbs)
{
 unsigned long fifo_control_word, fifo_data_word, Codes[20];
 int cnt;
 ushort stm_control_word;
 STMOBJ_ID pStmObj;
 int isss;
 int i;

   DPRINT4(1,"endofscan: ss:%d ssct:%d nt:%d ct:%d",ss,*ssct,nt,*ct);
   DPRINT2(1," bs:%d cbs:%d\n", bs, *cbs);

   /* delay for stm to complete */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT4_USEC);

   /* We manipulate the ssct and ct counters here because we do not (yet)
    * have separate ones for each receiver channel.
    * "Increment" steady state counter or completed transient counter. */
   if (*ssct > 0) {
       isss = TRUE;
       *ssct -= 1;
   } else {
       isss = FALSE;
       *ct += 1;
   }

   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
	   /* initialize for sa */
	   fifo_control_word = SWINTRP;
	   fifo_data_word = FIFO_SW_INTRP2;

	   /* Enable np |= 0 interrupt */
	   stm_control_word = STM_AP_RPNZ_ITRP_MASK;

	   /* Enable maxsum if requested, for now always enable */
	   if (*ct > 1)  /* don't enable for 1st transient */
	       stm_control_word = stm_control_word | STM_AP_MAX_SUM_ITRP_MASK;

	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

	   stm_control_word=0;

	   /* send interrupt error  mask */
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

	   /* zero control word to disable interrupts for next send */
	   stm_control_word = 0;

	   if ( ! isss )
	   {
	       if (*ct == nt)
	       {
		   DPRINT(3,"*enable EOS interrupt*\n");
		   stm_control_word = stm_control_word | STM_AP_RTZ_ITRP_MASK;

		   /* no sa interrupt if fid cmplt */
		   fifo_control_word = CL_DELAY;
		   fifo_data_word = CNT100NSEC_MIN;
	       }
	       else if (*ct < nt)
	       {
		   /* Check BlockSize */
		   if ((bs > 0) && (*ct%bs == 0))
		   {
		       /* set completed blocksizes and blocksize interrupt */
		       *cbs = *ct/bs;
		       stm_control_word |= STM_AP_IMMED_ITRP_MASK;

		       /* no sa interrupt if blocksize */
		       fifo_control_word = CL_DELAY;
		       fifo_data_word = CNT100NSEC_MIN;
		   }
	       }
	       else
	       {
		   /* Error Processing */
		   DPRINT(1,"endofscan: ct > nt\n");
		   /* run = stop; */
	       }

	   }

	   /* Send control word for stop acquisition (sa) interrupt */
	   fifoStuffCmd(pTheFifoObject,fifo_control_word,fifo_data_word);

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);
   
	   /* Send control word for blocksize or endofscan interrupts */
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

	   /* Send control words to clear all interrupts */
	   stm_control_word = 0;
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);

	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
       }
}


/******* Specials for multiple rcvr interleave *******/
/*
 * If nt > 1, acquire only on first channel for the first ct, then nt2 times
 * only on all other channels.  Repeat this cycle until nt is used up.
 */
static int
skipThisAcq(int ircvr,		/* Index of this rcvr (0 to n-1) */
	    int nct,		/* # transients already completed (ct) */
	    int rnt,		/* Total number of requested transients */
	    int nt2)		/* # of transients per cycle on chan 2 */
{
    if (!nt2 || rnt <= 1) {
	return FALSE;		/* Never skip if only one transient */
    } else {
	nct %= (nt2 + 1);
	return (ircvr && !nct) || (!ircvr && nct);
    }
}

static int
nSkippedAcqs(int ircvr,
	     int nct,		/* # transients already completed (ct) */
	     int rnt,		/* Total number of requested transients */
	     int nt2)		/* # of transients per cycle on chan 2 */
{
    int nskips;

    DPRINT4(5,"nSkippedAcqs(rcvr=%d, ct=%d, nt=%d, nt2=%d)",
	    ircvr, nct, rnt, nt2);
    if (!nt2 || rnt <= 1) {
	return 0;
    } else {
	nskips = (rnt - nct) / (nt2 + 1);
	if (!ircvr) {
	    nskips *= nt2;
	}
	DPRINT1(5,"nskips=%d", nskips);
	return nskips;
    }
}

static int
skipAnyAcq(int ircvr,		/* rcvr # [0:nactive-1] */
	   int rnt,		/* Total number of requested transients */
	   int nt2)		/* # of transients per cycle on chan 2 */
{
    return nt2 && rnt > 1;
}

static int
firstUnskippedAcq(int ircvr,	/* rcvr # [0:nactive-1] */
		  int nct,	/* # transients already completed (ct) */
		  int ict,	/* First ct */
		  int nt2)	/* # of transients per cycle on chan 2 */
{
    if (!nt2 && ict == nct) {
	return TRUE;
    }
    if ((!nt2 || !ircvr) && ict == nct) {
	return TRUE;
    }
    if (ircvr && nct == ict + 1) {
	return TRUE;
    }
    return FALSE;
}

/*
/* initscan2
/* 	Sets current number of transients using Completed Blocksizes.
/*	Allocates STM (DPM) buffers.
/*	Arguments:
/*		activeRcvrs	: ptr to active rcvrs
/*		np	: number of points for the scan
/*		ss	: number of steady state transients
/*		*ssct	: Pointer to Steady State Completed Transients
/*		nt	: Number of Transients.
/*		ct	: Completed Transients
/*		bs	: blocksize
/*		cbs	: Completed blocksizes
/*		maxsum	: maximum accumulated value
/*		nt2	: For interleaved multi-nuc acquisition
 */
void initscan2(long *activeRcvrs, ACODE_ID pAcodeId,
	      int np, int ss, int *ssct, int nt,
	      int ct, int bs, int cbs, int maxsum, int nt2)
{
   ushort stm_control_word;
   unsigned long Codes[30];
   int cnt,endct,nt_remaining;
   int i;
   int ircvr;
   STMOBJ_ID pStmObj;

   DPRINT5(1,"initscan2: np:%d ss:%d ssct:%d nt:%d ct:%d",np,ss,*ssct,nt,ct);
   DPRINT3(1," bs:%d cbs:%d maxsum:%d\n", bs, cbs, maxsum);
   *ssct = ss;

   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       DPRINT2(3,"initscan2: rcvr idx = %d, np=%d\n", i, np);
       ircvr = pStmObj->activeIndex;
       pStmObj->bsPending = FALSE;
       pStmObj->cur_scan_data_adr = 0;
       pStmObj->initial_scan_num = ct;
       /* set max_sum  */
       cnt = stmGenMaxSumCodes(pStmObj, maxsum, Codes);

       /* set number of transients remaining */
       pStmObj->nSkippedAcqs = nSkippedAcqs(ircvr, ct, nt, nt2);
       nt_remaining = (nt - ct) + ss - pStmObj->nSkippedAcqs;
       DPRINT1(1,"Nt remaining: 0x%lx\n",nt_remaining);
       DPRINT5(1,"rcvr=%d, ss=%d, nt=%d, nSkippedAcqs=%d, nt_remaining=%d\n",
	       ircvr, ss, nt, pStmObj->nSkippedAcqs, nt_remaining);
       cnt += stmGenNtCntCodes(pStmObj, nt_remaining, &Codes[cnt]);

       /* set num_points */
       cnt += stmGenNpCntCodes(pStmObj, np, &Codes[cnt]);

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
   }
}

/*-----------------------------------------------------------------------*/
/* nextscan2
/* 	Sends stm information to the apbus: source, destination addr
/*	Sets phase mode and if ct == 0 (steady state or first scan)
/*	zeroes source memory.
/*	Arguments:
/*		activeRcvrs	: ptr to active rcvrs
/*		pAcodeId	:
/*		np	: number of points for the scan
/*		dp	: bytes per data point
/*		nt	: Number of Transients.
/*		ct	: completed transients
/*		bs	: blocksize
/*		dtmCntrlArray	: stm control info
/*		fidnum	: fid number
/*		nt2	: For interleaved multi-nuc acquisition
/*-----------------------------------------------------------------------*/
void nextscan2(long *activeRcvrs, ACODE_ID pAcodeId,
	      int np, int dp, int nt, int ct, int bs,
	      int *dtmCntrlArray, int fidnum, int nt2)
{
 unsigned long Codes[55];
 int cnt,endct,modct;
 unsigned long scan_data_adr;
 FID_STAT_BLOCK *p2statb;
 ushort stm_control_word;
 STMOBJ_ID pStmObj;
 int *pDtmCntrl;
 int skip;
 int i;

   DPRINT4( 1, "nextscan2: np:%d dp:%d nt:%d ct:%d", np, dp, nt, ct);
   DPRINT3( 1, " bs:%d pDtmCntrl:0x%x fidnum:%d\n", bs, *dtmCntrlArray, fidnum);
   if (bs > 0) modct = bs;
   else modct = nt;		/* ==> Never trigger bs processing */

   if ((fidnum == 0) && (ct == 0))
   {
       DPRINT2(0,"nextscan2: FID: %d, CT: %d, Reseting startFLag to 0\n",
	       fidnum,ct);
       fifoClrStart4Exp(pTheFifoObject);
   }
   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       pDtmCntrl = dtmCntrlArray + i;
       DPRINT2(3,"nextscan2(): stm #%d at 0x%x\n", i, pStmObj);
       skip = skipThisAcq(pStmObj->activeIndex, ct, nt, nt2);
       if (!pStmObj->bsPending) /*???*/
	   pStmObj->bsPending = ct % modct == 0;
       if ( (pStmObj->bsPending && !skip) || (pStmObj->cur_scan_data_adr==0) )
       {
	   /* Only get new buffer if start of scan or blocksize */
	   pStmObj->bsPending = FALSE;
	   if ((ct != pStmObj->initial_scan_num ) || 
	       ((pStmObj->cur_scan_data_adr==0)))
	   {
	       /* Now if stmAllocAcqBlk() going to block, we need to decide
		* if the experiment has not really been started, if not we
		* should start the FIFO now before we pend.
		* Otherwise everybody will just wait forever.
		*/
	       DPRINT(1,"nextscan2: ");
	       DPRINT4(1,"FID=%d, CT=%d, WillPend?=%d, startFlag=%d \n",
		       fidnum, ct, stmAllocWillBlock(pStmObj),
		       fifoGetStart4Exp(pTheFifoObject));
	       if ( stmAllocWillBlock(pStmObj) == 1 )
	       {
		   /* Probable only need to start the fifo if:
		      1. No Task is pended on pAcodeId->pSemParseSuspend
		      semaphore
		      Note : not an issue since the parser would not be
		      here if it was pended.
		      2. No has done the initial start of the fifo for
		      this Exp.
		   */
		   /* if ( (fidnum == 1) && (ct > 1) ) */
		   if ( fifoGetStart4Exp(pTheFifoObject) == 0 )
		   {
		       DPRINT(0,"nextscan2: about to block, starting FIFO\n");
#ifdef INSTRUMENT
		       wvEvent(96,NULL,NULL);
#endif
		       fifoStart4Exp(pTheFifoObject);
		   }
	       }
	       if ((bs < 1) || ((nt-ct) < bs)) {
		   endct = nt - pStmObj->nSkippedAcqs;
	       } else {
		   int rnt;
		   rnt = bs * (1 + (ct / bs));
		   endct = rnt - nSkippedAcqs(pStmObj->activeIndex,
					      0, rnt, nt2);
	       }
	       DPRINT5(1,"alloc blk: nt=%d, ct=%d, bs=%d, nskip=%d, endct=%d\n",
		       nt, ct, bs, pStmObj->nSkippedAcqs, endct);
	       p2statb = stmAllocAcqBlk(pStmObj, 
					(ulong_t) fidnum, (ulong_t) np,
					(ulong_t) ct, (ulong_t) endct,
					(ulong_t) (nt - pStmObj->nSkippedAcqs),
					(ulong_t) (np * dp), 
					(long *)&pStmObj->tag2snd,
					&scan_data_adr );
	       DPRINT5(1,"rcvr=%d, ct=%d, endct=%d, nt=%d, data addr = 0x%x",
		       pStmObj->activeIndex, ct, endct,
		       nt - pStmObj->nSkippedAcqs,
		       scan_data_adr);
	   }
	   else
	       scan_data_adr = pStmObj->cur_scan_data_adr;
	   if (pStmObj->cur_scan_data_adr == 0) 
	       pStmObj->cur_scan_data_adr = scan_data_adr;

	   DPRINT3(1, "ct=%d, Source addr: 0x%lx  Dest addr: 0x%lx\n",
		   ct, pStmObj->cur_scan_data_adr, scan_data_adr);
	   /* Send tag word */
	   cnt = stmGenTagCodes(pStmObj, pStmObj->tag2snd, Codes);

	   /* set source Address of Data */
	   cnt += stmGenSrcAdrCodes(pStmObj, pStmObj->cur_scan_data_adr, 
				    &Codes[cnt]);
	   /* set Destination Address of Data */
	   cnt += stmGenDstAdrCodes(pStmObj, scan_data_adr, &Codes[cnt]);
	   DPRINT2(1,"src data addr = 0x%x, dst data addr = 0x%x",
		   pStmObj->cur_scan_data_adr, scan_data_adr);
	   pStmObj->prev_scan_data_adr = pStmObj->cur_scan_data_adr;
	   pStmObj->cur_scan_data_adr = scan_data_adr;
       }
       else {

	   /* Send tag word */
	   cnt = stmGenTagCodes(pStmObj, pStmObj->tag2snd, Codes);

	   /* set source Address of Data */
	   cnt += stmGenSrcAdrCodes(pStmObj, pStmObj->cur_scan_data_adr, 
				    &Codes[cnt]);
	   /* set Destination Address of Data */
	   cnt += stmGenDstAdrCodes(pStmObj, pStmObj->cur_scan_data_adr,
				    &Codes[cnt]);
	   DPRINT2(1,"ct=%d, data addr = 0x%x",
		   ct, pStmObj->cur_scan_data_adr);
	   pStmObj->prev_scan_data_adr = pStmObj->cur_scan_data_adr;
       }

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

       /* reset num_points */
       /* stm_control_word =  STM_AP_RELOAD_NP | STM_AP_RELOAD_ADDRS; */
       stm_control_word =  STM_AP_RELOAD_NP_ADDRS ;
       cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);

       /* zero source data if steady state first transient (ct == 0) */
       if (firstUnskippedAcq(pStmObj->activeIndex,
			     ct,
			     pStmObj->initial_scan_num,
			     nt2))
       {
	   DPRINT2(1,"Zero memory: rcvr=%d, ct=%d",
		  pStmObj->activeIndex, ct);
	   *pDtmCntrl |= STM_AP_MEM_ZERO;
       }else{
	   *pDtmCntrl &= ~STM_AP_MEM_ZERO;
       }
       if (skip) {
	   DPRINT3(1,"nextscan2: rcvr=%d, ct=%d, skip=%d\n",
		   pStmObj->activeIndex, ct, skip);
	   *pDtmCntrl &= ~STM_AP_ENABLE_STM;/* No data collection */
       } else {
	   *pDtmCntrl |= STM_AP_ENABLE_STM;
       }

       cnt += stmGenCntrlCodes(pStmObj, *pDtmCntrl, &Codes[cnt]);

       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
   }
}

/*-----------------------------------------------------------------------*/
/* endofscan2
/* 	Increments Steady State Completed Transient counter or
/*	Completed Transient counter.  Checks for ct==nt and blocksize
/*	and enables interrupts for either condition
/*	Arguments:
/*		activeRcvrs	: ptr to active rcvrs
/*		ss	: steady state transients
/*		*ssct	: ptr to steady state completed transients
/*		nt	: number of transients
/*		*ct	: ptr to completed transients
/*		bs	: blocksize
/*		*cbs	: ptr to completed blocksize
/*		nt2	: For interleaved multi-nuc acquisition
/*-----------------------------------------------------------------------*/
void endofscan2(long *activeRcvrs,
	       int ss, int *ssct, int nt, int *ct, int bs, int *cbs, int nt2)
{
 unsigned long fifo_control_word, fifo_data_word, Codes[20];
 int cnt;
 ushort stm_control_word;
 STMOBJ_ID pStmObj;
 int isss;
 int oldct;
 int i;

   DPRINT4(1,"endofscan2: ss:%d ssct:%d nt:%d ct:%d",ss,*ssct,nt,*ct);
   DPRINT2(1," bs:%d cbs:%d\n", bs, *cbs);

   /* delay for stm to complete */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT4_USEC);

   oldct = *ct;

   /* We manipulate the ssct and ct counters here because we do not
    * have separate ones for each receiver channel.
    * "Increment" steady state counter or completed transient counter. */
   if (*ssct > 0) {
       isss = TRUE;
       *ssct -= 1;
   } else {
       isss = FALSE;
       *ct += 1;
   }

   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
	   /* initialize for sa */
	   fifo_control_word = SWINTRP;
	   fifo_data_word = FIFO_SW_INTRP2;

       DPRINT1(99,"endofscan2: oldct=%d\n", oldct);
       if (!skipThisAcq(pStmObj->activeIndex, oldct, nt, nt2)) {
	   /* We should be collecting data */

	   /* Enable np |= 0 interrupt */
	   stm_control_word = STM_AP_RPNZ_ITRP_MASK;

	   /* Enable maxsum if requested, for now always enable */
	   if (!firstUnskippedAcq(pStmObj->activeIndex,
				  oldct,
				  pStmObj->initial_scan_num,
				  nt2))
	   {
	       /*if (*ct > 1)  /* don't enable for 1st transient */
	       stm_control_word = stm_control_word | STM_AP_MAX_SUM_ITRP_MASK;
	   }

	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);
       }
       
	   stm_control_word=0;

	   /* send interrupt error  mask */
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

	   /* zero control word to disable interrupts for next send */
	   stm_control_word = 0;

	   if ( ! isss )
	   {
	       if (*ct == nt)
	       {
		   DPRINT(3,"*enable EOS interrupt*\n");
		   stm_control_word = stm_control_word | STM_AP_RTZ_ITRP_MASK;

		   /* no sa interrupt if fid cmplt */
		   fifo_control_word = CL_DELAY;
		   fifo_data_word = CNT100NSEC_MIN;
	       }
	       else if (*ct < nt)
	       {
		   /* Check BlockSize */
		   if ((bs > 0) && (*ct%bs == 0))
		   {
		       /* set completed blocksizes and blocksize interrupt */
		       *cbs = *ct/bs;
		       stm_control_word |= STM_AP_IMMED_ITRP_MASK;

		       /* no sa interrupt if blocksize */
		       fifo_control_word = CL_DELAY;
		       fifo_data_word = CNT100NSEC_MIN;
		   }
	       }
	       else
	       {
		   /* Error Processing */
		   DPRINT(1,"endofscan2: ct > nt\n");
		   /* run = stop; */
	       }

	   }

	   /* Send control word for stop acquisition (sa) interrupt */
	   fifoStuffCmd(pTheFifoObject,fifo_control_word,fifo_data_word);

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);
   
	   /* Send control word for blocksize or endofscan interrupts */
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);
	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

	   /* delay */
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

	   /* Send control words to clear all interrupts */
	   stm_control_word = 0;
	   cnt = stmGenCntrlCodes(pStmObj, stm_control_word, Codes);

	   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
       }
}
/******* END Specials for multiple rcvr interleave *******/

/*----------------------------------------------------------------------*/
/* setscanvalues							*/
/* 	Sets np , prev data location, dest data location if different.	*/
/*		than zero.						*/
/*	Arguments:							*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		srcoffset	: offset to src data address		*/
/*		destoffset	: offset to dest data address		*/
/*		np		: number of points for the scan		*/
/*		dtmCntrlArray	: stm control info			*/
/*----------------------------------------------------------------------*/
void setscanvalues(long *activeRcvrs, ACODE_ID pAcodeId,
		   int srcoffset, int destoffset, int np, int *dtmCntrlArray)
{
   unsigned long Codes[30];
   unsigned long tmp_src_adr, tmp_dest_adr;
   int cnt;
   ushort stm_control_word;
   int *pDtmCntrl;
   STMOBJ_ID pStmObj;
   int i;

   DPRINT3(0,"set scan: srcoffset:%d destoffset:%d np:%d\n",srcoffset,
							destoffset,np);
   for (i=-1, pStmObj=stmGetActive(activeRcvrs, &i);
	pStmObj;
	pStmObj=stmGetActive(activeRcvrs, &i))
   {
       pDtmCntrl = dtmCntrlArray + i;
       cnt=0;
       if ((srcoffset != 0) || (destoffset != 0))
       {
	   tmp_src_adr = pStmObj->prev_scan_data_adr + srcoffset;
	   tmp_dest_adr = pAcodeId->cur_scan_data_adr + destoffset;
	   /* set source Address of Data */
	   cnt += stmGenSrcAdrCodes(pStmObj, tmp_src_adr, 
				    &Codes[cnt]);
	   /* set Destination Address of Data */
	   cnt += stmGenDstAdrCodes(pStmObj, tmp_dest_adr,
				    &Codes[cnt]);
	   /* stm_control_word =  STM_AP_RELOAD_ADDRS; */
	   stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
	   cnt += stmGenCntrlCodes(pStmObj, stm_control_word, &Codes[cnt]);
	   DPRINT3(3,"setscanvalues(): stm=0x%x, src=0x%x, dst=0x%x\n",
		   pStmObj, tmp_src_adr, tmp_dest_adr);
       }
       if (np != 0) 
       {
	   /* set num_points */
	   cnt += stmGenNpCntCodes(pStmObj, np, &Codes[cnt]);
	   /* stm_control_word =  STM_AP_RELOAD_NP; */
	   stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
	   cnt += stmGenCntrlCodes(pStmObj, stm_control_word, &Codes[cnt]);
	   DPRINT3(3,"setscanvalues(): stm=0x%x, np=%d, ctrl=0x%x\n",
		   pStmObj, np, stm_control_word);
       }

       /* reset dtm control word */
       DPRINT1(3,"setscanvalues(): reset ctl word, stm=0x%x\n",
	       pStmObj);
       cnt += stmGenCntrlCodes(pStmObj, *pDtmCntrl, &Codes[cnt]);
       fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
   }
}

/*----------------------------------------------------------------------*/
/* nextcodeset								*/
/*	Returns pointer to the next set of acodes.			*/
/*----------------------------------------------------------------------*/
unsigned short
*nextcodeset( ACODE_ID pAcodeId, int il, int *cbs, int nt, int ct)
{
 unsigned short *ac_base;

 /* autogain don't delete acode set */
 if (pAcodeId->interactive_flag == ACQ_AUTOGAIN)
   return(NULL);

   if (il == FALSE)  *cbs = 0;
   else 
   {
	if (ct < nt)
	   *cbs = *cbs - 1;	/* Set for next blocksize interval */
   }
   markAcodeSetDone(pAcodeId->id,pAcodeId->cur_acode_set);
   rmAcodeSet(pAcodeId->id,pAcodeId->cur_acode_set);
   pAcodeId->cur_acode_set++;
   DPRINT6(1,"nextcodeset:: il:%d cbs:%d nt:%d ct:%d cacode:%d nacode:%d\n",
    il,*cbs,nt,ct,pAcodeId->cur_acode_set,pAcodeId->num_acode_sets);
   if (pAcodeId->cur_acode_set < pAcodeId->num_acode_sets)
	pAcodeId->cur_acode_base = 
		getAcodeSet(pAcodeId->id,pAcodeId->cur_acode_set,
                            pAcodeId->interactive_flag, 10);
   else if (il == TRUE) {
	if (ct >= nt)
	   pAcodeId->cur_acode_base = NULL;
	else
	{
	   *cbs = *cbs + 1;	/* set back for next round */
	   pAcodeId->cur_acode_set = 0;
	   fifoStart(pTheFifoObject);
	   pAcodeId->cur_acode_base = 
		getAcodeSet(pAcodeId->id,pAcodeId->cur_acode_set,
                            pAcodeId->interactive_flag, 10);
	}
   }
   else
	pAcodeId->cur_acode_base = NULL;

   ac_base = pAcodeId->cur_acode_base;
   return(ac_base);
}

/*----------------------------------------------------------------------*/
/* enableOvrLd								*/
/* 	Enable ADC & Receiver OverLoad AP interrupts Masks 		*/
/*      This done for the 1st non ss transient for each FID		*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		ssct	: steady state completed transients		*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
#ifdef OLD_ENABLES
void enableOvrLd(long *activeRcvrs, int ssct, int ct, ulong_t *adccntrl)
{
    unsigned long Codes[20];
    unsigned long adc_control_word;
    int cnt;
    ADC_ID pAdcObj;
    int i;

    for (i=-1, pAdcObj=adcGetActive(activeRcvrs, &i);
	 pAdcObj;
	 pAdcObj=adcGetActive(activeRcvrs, &i))
    {
	adc_control_word = *(adccntrl + i);
	DPRINT3(-11,"enableOvrLd: ssct: %ld, ct: %ld, control word = 0x%lx\n",
		ssct,ct,adc_control_word);
	/* Only 1st transient allow adc or receiver overload */
	if (ssct != 0)
	{
	    adc_control_word &= ( ~(ADC_AP_ENABLE_ADC_OVERLD
				    | ADC_AP_ENABLE_RCV2_OVERLD
				    | ADC_AP_ENABLE_RCV1_OVERLD) );
	    DPRINT1(1,"enableOvrLd: disable ADC itrps, control word = 0x%lx\n",
		    adc_control_word);
	}

	cnt = adcGenCntrl2Cmds(pAdcObj, adc_control_word, &Codes[0]);
 	fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
 	fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

	/* cnt = adcGenCntrl2Codes(pAdcObj, adc_control_word, &Codes[0]);
	/* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
    }
}

/*----------------------------------------------------------------------*/
/* disableOvrLd								*/
/* 	Disable ADC & Receiver OverLoad AP interrupts Masks 		*/
/*      This done right after the acquire 				*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void disableOvrLd(long *activeRcvrs, ulong_t *adccntrl)
{
    unsigned long Codes[20];
    unsigned long adc_control_word;
    int cnt;
    ADC_ID pAdcObj;
    int i;

    for (i=-1, pAdcObj=adcGetActive(activeRcvrs, &i);
	 pAdcObj;
	 pAdcObj=adcGetActive(activeRcvrs, &i))
    {
	adc_control_word = (ulong_t) *(adccntrl + i);
	adc_control_word &= ~(ADC_AP_ENABLE_ADC_OVERLD
			      | ADC_AP_ENABLE_RCV2_OVERLD
			      | ADC_AP_ENABLE_RCV1_OVERLD);
	DPRINT1(-11,"disableOvrLd: disable ADC itrps, control word = 0x%lx\n",
		adc_control_word);
	cnt = adcGenCntrl2Cmds(pAdcObj, adc_control_word, &Codes[0]);
 	fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
 	fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

	/* cnt = adcGenCntrl2Codes(pAdcObj, adc_control_word, &Codes[0]);
	/* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
    }
}
/* #endif */

/*----------------------------------------------------------------------*/
/* enableStmAdcOvrLd							*/
/* 	Enable HS STM/ADC ADC OverLoad AP interrupts Masks 		*/
/*      This done for the 1st non ss transient for each FID		*/
/*		ssct	: steady state completed transients		*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void enableStmAdcOvrLd(int ssct, int ct, int *dtmcntrl)
{
 unsigned long Codes[20];
 unsigned short stm_control_word;
 int cnt;

 /* Only 1st transient allow adc or receiver overload */
 if (ssct == 0)
 {
   dtmcntrl += stmGetHsStmIdx(); /* Reference correct ctl word */
   /* Set HS STM/ADC   ADC OverLoad Interupt bit */
   DPRINT1(1,"enableStmAdcOvrLd: ssct==0 , control word = 0x%x\n",*dtmcntrl);
   *dtmcntrl = *dtmcntrl | ADM_AP_OVFL_ITRP_MASK;
   stm_control_word = *dtmcntrl;
   DPRINT2(1,"enableStmAdcOvrLd: ssct==0 enable HS STM/ADC Overload itrp, control word = 0x%x (0x%x)\n",
		stm_control_word,*dtmcntrl);
    cnt = stmGenCntrlCmds(stmGetSelectedHsStmObj(), stm_control_word, Codes);

   /* cnt = stmGenCntrlCodes(stmGetSelectedHsStmObj(), stm_control_word, Codes); */
 }
 else
 {
    /* clear HS STM/ADC   ADC OverLoad Interupt bit */
    DPRINT1(1,"enableStmAdcOvrLd: ssct==0 , control word = 0x%x\n",*dtmcntrl);
    *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK;
    stm_control_word = *dtmcntrl;
    DPRINT2(1,"enableStmAdcOvrLd: ssct!=0 disable ADC itrps, control word = 0x%x (0x%x)\n",stm_control_word,*dtmcntrl);
    cnt = stmGenCntrlCmds(stmGetSelectedHsStmObj(), stm_control_word, Codes);
    /* cnt = stmGenCntrlCodes(stmGetSelectedHsStmObj(), stm_control_word, Codes); */
 }
 fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
 fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

 /* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
}

/*----------------------------------------------------------------------*/
/* disableStmAdcOvrLd							*/
/* 	Disable HS STM/ADC ADC OverLoad AP interrupts Masks 		*/
/*      This done right after the acquire 				*/
/*		*dtmcntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void disableStmAdcOvrLd(int *dtmcntrl)
{
 unsigned long Codes[20];
 unsigned short stm_control_word;
 int cnt;

   dtmcntrl += stmGetHsStmIdx(); /* Reference correct ctl word */
    /* clear HS STM/ADC   ADC OverLoad Interupt bit */
   DPRINT1(1,"disableStmAdcOvrLd: control word = 0x%x\n",*dtmcntrl);
    *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK;
    stm_control_word = *dtmcntrl;
    DPRINT2(1,"disableStmAdcOvrLd: disable HS STM/ADC ADC itrps, control word = 0x%x (0x%x)\n",
		stm_control_word,*dtmcntrl);

    cnt = stmGenCntrlCmds(stmGetSelectedHsStmObj(), stm_control_word, Codes);
    fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
    fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

    /* cnt = stmGenCntrlCodes(stmGetSelectedHsStmObj(), stm_control_word, Codes);
    /* fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */
}

#endif



/*----------------------------------------------------------------------*/
/* enableOvrLdN								*/
/* 	Enable ADC & Receiver OverLoad AP interrupts Masks 		*/
/*      This done for the 1st non ss transient for each FID		*/
/*		activeRcvrs	: ptr to active rcvrs			*/
/*		ssct	: steady state completed transients		*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		*dtmcntrl: ptr to dtm control word 			*/
/*		*adccntrl: ptr to adc control word 			*/
/* depending on hardware the ADC overflow control may either reside on  */
/* a seperate ADC board or on the DTM directly (1 & 5 MHz STM/ADC)      */
/*----------------------------------------------------------------------*/
void enableOvrLdN(long *activeRcvrs, int ssct, int ct, int *dtmCntrlArray, ulong_t *adcCntrlArray)
{
    unsigned long Codes[20];
    unsigned long adc_control_word;
    unsigned short stm_control_word;
    int cnt;
    STMOBJ_ID pStmObj;
    ADC_ID pAdcObj;
    int *pDtmCntrl;
    ulong_t* pAdcCntrl;
    int i;

    DPRINT5(1,"enableOvrLdN: ssct=%ld, ct=%ld, dtmcntrl[0]: 0x%lx, adccntrl[0]: 0x%lx active rcvrs: 0x%lx\n",
	ssct,ct,*dtmCntrlArray,*adcCntrlArray,*activeRcvrs);

    for (i=-1, pStmObj = stmGetActive(activeRcvrs, &i);
	 pStmObj;
         pStmObj = stmGetActive(activeRcvrs, &i))
    {
        pAdcObj = (ADC_ID) stmGetAttachedADC(pStmObj);

	/* stm_control_word = *(dtmcntrl + i);
	   adc_control_word = *(adccntrl + i);
        */
        pDtmCntrl = dtmCntrlArray + i;
        pAdcCntrl = adcCntrlArray + i;
	DPRINT6(1,"enableOvrLdN: StmObj[%d]: 0x%lx, AdcObj[%d]: 0x%lx, DTM & ADC control word = 0x%lx, 0x%lx\n",
		i,pStmObj, i, pAdcObj, stm_control_word,adc_control_word);

        /* set adc overflow on ADC if present otherwise must be on STM directly */
        if (pAdcObj != NULL)
        {
	   /* Only 1st transient allow adc or receiver overload */
	   if (ssct != 0)
	   {
		/*
	       adc_control_word &= ( ~(ADC_AP_ENABLE_ADC_OVERLD
				    | ADC_AP_ENABLE_RCV2_OVERLD
				    | ADC_AP_ENABLE_RCV1_OVERLD) );
	       DPRINT1(1,"enableOvrLd: enable ADC itrps, control word = 0x%lx\n",
		       adc_control_word);
               */
	       *pAdcCntrl &= ( ~(ADC_AP_ENABLE_ADC_OVERLD
				    | ADC_AP_ENABLE_RCV2_OVERLD
				    | ADC_AP_ENABLE_RCV1_OVERLD) );
	       DPRINT2(1,"enableOvrLd: enable ADC (0x%lx) itrps, control word = 0x%lx\n",
		       pAdcObj->adcBaseAddr,*pAdcCntrl);
	   }
           else
           {
	       *pAdcCntrl |= ( (ADC_AP_ENABLE_ADC_OVERLD
				    | ADC_AP_ENABLE_RCV2_OVERLD
				    | ADC_AP_ENABLE_RCV1_OVERLD) );
	       DPRINT2(1,"enableOvrLd: enable ADC (0x%lx) itrps, control word = 0x%lx\n",
		       pAdcObj->adcBaseAddr,*pAdcCntrl);
           }
	   adc_control_word = *pAdcCntrl;
	   cnt = adcGenCntrl2Cmds(pAdcObj, adc_control_word, &Codes[0]);
        }
        else
        {
           /* Only 1st transient allow adc or receiver overload */
           if (ssct == 0)
           {
             /* Set HS STM/ADC   ADC OverLoad Interupt bit */
             /* DPRINT1(1,"enableOvrLdN: ssct==0 , DTM control word = 0x%x\n",*dtmcntrl); */
             DPRINT1(1,"enableOvrLdN: ssct==0 , DTM control word = 0x%x\n",*pDtmCntrl);

             /* *dtmcntrl = *dtmcntrl | ADM_AP_OVFL_ITRP_MASK; */
             *pDtmCntrl  = *pDtmCntrl | ADM_AP_OVFL_ITRP_MASK;
             /* stm_control_word = *dtmcntrl; */
             stm_control_word = *pDtmCntrl;

             DPRINT2(1,"enableOvrLdN: ssct==0 enable HS STM/ADC Overload itrp, control word = 0x%x (0x%x)\n",
                        stm_control_word,*pDtmCntrl);
           }
           else
           {
              /* clear HS STM/ADC   ADC OverLoad Interupt bit */
              DPRINT1(1,"enableOvrLdN: disable ssct!=0 , DTM control word = 0x%x\n",*pDtmCntrl);

              /* *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK; */
              *pDtmCntrl = *pDtmCntrl & ~ADM_AP_OVFL_ITRP_MASK;
              stm_control_word = *pDtmCntrl;

              DPRINT2(1,"enableOvrLdN: ssct!=0 disable STM/ADC itrps, control word = 0x%x (0x%x)\n",
                         stm_control_word,*pDtmCntrl);
           }
           /* cnt = stmGenCntrlCmds(stmGetSelectedHsStmObj(), stm_control_word, Codes); */
           cnt = stmGenCntrlCmds(pStmObj, stm_control_word, Codes);
        }
        fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
        fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	
    }
}

/*----------------------------------------------------------------------*/
/* disableOvrLdN							*/
/* 	Disable ADC & Receiver OverLoad AP interrupts Masks 		*/
/*      This done right after the acquire 				*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void disableOvrLdN(long *activeRcvrs, int *dtmCntrlArray, ulong_t *adcCntrlArray)
{
    unsigned long Codes[20];
    unsigned long adc_control_word;
    unsigned short stm_control_word;
    int cnt;
    STMOBJ_ID pStmObj;
    ADC_ID pAdcObj;
    int *pDtmCntrl;
    ulong_t *pAdcCntrl;
    int i;
    DPRINT3(1,"disableOvrLdN: dtmcntrl[0]: 0x%lx, adccntrl[0]: 0x%lx active rcvrs: 0x%lx\n",
	*dtmCntrlArray,*adcCntrlArray,*activeRcvrs);

    for (i=-1, pStmObj = stmGetActive(activeRcvrs, &i);
	 pStmObj;
         pStmObj = stmGetActive(activeRcvrs, &i))
    {
        pAdcObj = (ADC_ID) stmGetAttachedADC(pStmObj);
	DPRINT4(1,"disableOvrLdN: stmId[%d]: 0x%lx, adcId[%d]: 0x%lx\n",
				i,pStmObj,i,pAdcObj);

        /* set adc overflow on ADC if present otherwise must be on STM directly */
        if (pAdcObj != NULL)
        {
           /*
	   adc_control_word = *(adccntrl + i);
	   adc_control_word &= ~(ADC_AP_ENABLE_ADC_OVERLD
			      | ADC_AP_ENABLE_RCV2_OVERLD
			      | ADC_AP_ENABLE_RCV1_OVERLD);
	   DPRINT2(1,"disableOvrLdN: disable ADC itrps, control word = 0x%lx\n",
		   adc_control_word);
           */
            pAdcCntrl = adcCntrlArray + i;
	   *pAdcCntrl &= ~(ADC_AP_ENABLE_ADC_OVERLD
			      | ADC_AP_ENABLE_RCV2_OVERLD
			      | ADC_AP_ENABLE_RCV1_OVERLD);
	   DPRINT2(1,"disableOvrLdN: disable ADC (0x%lx) itrps, control word = 0x%lx\n",
		   pAdcObj->adcBaseAddr,*pAdcCntrl);
	   adc_control_word = *pAdcCntrl;
	   cnt = adcGenCntrl2Cmds(pAdcObj, adc_control_word, &Codes[0]);
        }
        else
        {
	    /* stm_control_word = *(dtmcntrl + i); */
            pDtmCntrl = dtmCntrlArray + i;
            /* clear HS STM/ADC   ADC OverLoad Interupt bit */
            DPRINT1(1,"disableOvrLdN: STM/ADC control word = 0x%x\n",*pDtmCntrl);
            *pDtmCntrl = *pDtmCntrl & ~ADM_AP_OVFL_ITRP_MASK;
            /* *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK; */
            stm_control_word = *pDtmCntrl;
            DPRINT2(1,"disableOvrLdN: disable HS STM/ADC ADC itrps, control word = 0x%x (0x%x)\n",
		stm_control_word,*pDtmCntrl);
            cnt = stmGenCntrlCmds(pStmObj, stm_control_word, Codes);

        }
        fifoStuffCmd(pTheFifoObject,Codes[0],Codes[1]);	
        fifoStuffCmd(pTheFifoObject,Codes[2],Codes[3]);	

    }
}


/*----------------------------------------------------------------------*/
/* acqi_updt								*/
/*	checks to see if interactive experiment.  			*/
/*	If interactive,							*/
/*	o Sets rtvar for branch control.  				*/
/*	o Puts sw interrupt into fifo.					*/
/*	o Puts delay into fifo if requested.  				*/
/*	o Gives up acode mutex semaphore to allow the update routine to	*/
/*	  insert updates, and takes the	parse updt semaphore to wait	*/
/*	  until the sw interrupt occurs to continue parseing		*/
/*----------------------------------------------------------------------*/
void acqi_updt(ACODE_ID pAcodeId,int acode,int *acqi_flag,int updtdelay)
{

   DPRINT( DEBUG_ACQI_UPDT, "acqi update starts\n" );
   if (pAcodeId->interactive_flag == ACQI_INTERACTIVE)
   {
     if (updtdelay != 0)
     {
	/* For a while it was necessary to insert a delay of 0.5 or 1.0 sec
           before the SW1 interrupt to get the system to work right.  This
	   appears to no longer be the case, so the delay has been removed.

           Notice that there is a separate delay, specified in the argument
           list, that occurs after the SW1 interrupt.			   */

   	/* Put sw1 interrupt into fifo */
	fifoStuffCmd(pTheFifoObject,SWINTRP,FIFO_SW_INTRP1);

	/* delay for interrupt*/
	fifoStuffCmd(pTheFifoObject,CL_DELAY,SW1_HW_INTERRUPT_DELAY);

   	/* Put delay into fifo: no seconds, updtdelay ticks. */
        if (updtdelay > 0)
   	   delay(0, updtdelay);

	/* put HALT OP into the FIFO if acquiring lock to avoid FOO */

	if (acode == LOCK_UPDT)
	{
	   fifoStuffCmd(pTheFifoObject,HALTOP,0);
	   fifoStartSync(pTheFifoObject);
	}
	else
        {
           if (updtdelay < 0)
	      fifoStuffCmd(pTheFifoObject,HALTOP,0);
	   fifoStart(pTheFifoObject);		/* always start */
        }

   	/* Give acodeObj mutex semaphore */
   	semGive(pAcodeId->pAcodeControl);


   	/* Take parser update semaphore to wait for interrupt to 	*/
   	/* continue parsing. 					*/
   	/* Change WAIT_FOREVER to a timeout?  */
#ifdef INSTRUMENT
        wvEvent(EVENT_INTRP_UPDATE,NULL,NULL);
#endif


   	semTake(pAcodeId->pSemParseUpdt, WAIT_FOREVER);

   	/* Take acodeObj mutex semaphore to continue parsing */
   	semTake(pAcodeId->pAcodeControl, WAIT_FOREVER);

     }
     else {
	/* delay for interrupt*/
	fifoStuffCmd(pTheFifoObject,CL_DELAY,SW1_HW_INTERRUPT_DELAY);
	fifoStart(pTheFifoObject);		/* always start */
     }
	
     /* set rtvar interactive flag*/
     *acqi_flag = 1;
   }
   else
   { 
	*acqi_flag = 0;
   	/* Put delay into fifo: no seconds, updtdelay ticks.	*/
	/* Add time for fifo word.				*/
   	delay(0, (updtdelay+SW1_INTERRUPT_DELAY));
   }
   DPRINT( DEBUG_ACQI_UPDT, "acqi update completes\n" );
}

/*----------------------------------------------------------------------*/
/* set_acqi_updt_cmplt							*/
/*	o Puts sw interrupt that denotes the end  of the update 	*/
/*	  interval into the fifo.					*/
/*----------------------------------------------------------------------*/
void set_acqi_updt_cmplt(void)
{
   int pTmpId, TmpPrior;

   DPRINT( DEBUG_ACQI_UPDT, "set acqi update cmplt\n" );
   /* Put sw1 interrupt into fifo */
   fifoStuffCmd(pTheFifoObject,SWINTRP,FIFO_SW_INTRP1);

   /* delay for interrupt*/
   fifoStuffCmd(pTheFifoObject,CL_DELAY,SW1_HW_INTERRUPT_DELAY);

   /* Lower task priority */
   /* pTmpId = taskNameToId("tParser"); */
   /* taskPriorityGet(pTmpId,&TmpPrior); */
   /* taskPrioritySet(pTmpId,APARSER_TASK_PRIORITY);  */

	
    DPRINT( DEBUG_ACQI_UPDT, "end set acqi update cmplt\n" );
}

/*----------------------------------------------------------------------*/
/* start_acqi_updt							*/
/*	o Gives up acode mutex semaphore to allow the update routine to	*/
/*	  insert updates, and takes the	parse updt semaphore to wait	*/
/*	  until the sw interrupt occurs to continue parseing		*/
/*	o Needs to be used in conjunction with set_acqi_updt_cmplt.		*/
/*----------------------------------------------------------------------*/
void start_acqi_updt(ACODE_ID pAcodeId)
{
   int pTmpId, TmpPrior;
   DPRINT( DEBUG_ACQI_UPDT, "start acqi update starts\n" );


   fifoStart(pTheFifoObject);		/* always start */

   /* Give acodeObj mutex semaphore */
   semGive(pAcodeId->pAcodeControl);

   /* Bump task priority */
   /* pTmpId = taskNameToId("tParser"); */
   /* taskPriorityGet(pTmpId,&TmpPrior); */
   /* taskPrioritySet(pTmpId,(MONITOR_TASK_PRIORITY+1));  */

   /* Take parser update semaphore to wait for interrupt to 	*/
   /* continue parsing. 					*/
   /* Change WAIT_FOREVER to a timeout?  */
#ifdef INSTRUMENT
        wvEvent(EVENT_INTRP_UPDATE,NULL,NULL);
#endif

   semTake(pAcodeId->pSemParseUpdt, WAIT_FOREVER);

   /* Take acodeObj mutex semaphore to continue parsing */
   semTake(pAcodeId->pAcodeControl, WAIT_FOREVER);

	
   DPRINT( DEBUG_ACQI_UPDT, "start acqi update completes\n" );
}

/*----------------------------------------------------------------------*/
/* setshimviaAp								*/
/*	Stuffs shim setting Ap Bus words into the fifo.  		*/
/*	Ap Bus, causes MSR to set shim via the AP Bus 			*/ 
/*----------------------------------------------------------------------*/
setshimviaAp(int dac, int value)
{
   int cnt;
   unsigned long Codes[50];
   cnt = autoGenShimApCodes(pTheAutoObject,dac,value,Codes);

   if (dac != SSHA_INDEX)
     DPRINT2( 1, "setshimviaAp, dac: %d, value:%d\n", dac, value );
   if (cnt < 0)
      return(-1);
#if 0
	DPRINT( 1, "setshimviaAp:\n" );
	for (iter = 0; iter < cnt; iter++) {
	  DPRINT2( 1, "%d: 0x%x\n", iter, Codes[ iter ] );
	}
#endif
   fifoStuffIt(pTheFifoObject, Codes, cnt/2);
   return(0);
}

/*----------------------------------------------------------------------*/
/* signal_completion							*/
/*	Stuffs a completion tag into the fifo.  It uses the supplied	*/
/*	argument as a tagword. 						*/ 
/*	tag is used as a donecode to signal the host.			*/
/*----------------------------------------------------------------------*/
void signal_completion(ushort tagword)
{
   /* Put tag into fifo */
   fifoStuffCmd(pTheFifoObject,CL_SW_ID_TAG,(int) tagword);
 
   /* delay to service the interrupt */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

}

/*----------------------------------------------------------------------*/
/* signal_syncop							*/
/*	Stuffs an action operation tag into the fifo.			*/
/*      This tag is followed by:					*/
/*	1. haltop in the fifo, thus stopping the Exp. When the requested*/
/*	   action is complete the fifo will be 	restarted.		*/
/*	2. delay in seconds, fifo not stopped				*/
/*      3. Nothing, operation is considered done immediately	        */
/*----------------------------------------------------------------------*/
void signal_syncop(int tagword,long secs,long ticks)
{
   /* Put tag into fifo */
   fifoStuffCmd(pTheFifoObject,CL_SW_ID_TAG,(int) (tagword | 0x4000));
 
   DPRINT1(2,"signal_syncop: ticks: %ld\n",ticks);
   if ((ticks == -1L) || (secs == -1L))
   {
     /* halt the fifo, the requested action when complete should restart fifo */
      fifoStuffCmd(pTheFifoObject,HALTOP,0);
   }
   else 
   {
      /* delay to service the interrupt */
      fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

      delay(secs, ticks);  /* put completion delay into fifo */
   }
}

/*----------------------------------------------------------------------*/
/* tbl_element_addr							*/
/*	Returns the address of a table element.				*/
/*	Arguments:							*/
/*		*tbl_ptr	: Top of Table Addesses.		*/
/*		tindex		: table index				*/
/*		index		: index into table.			*/
/*----------------------------------------------------------------------*/
char *tbl_element_addr(TBL_ID *tblptr,short tindex,int index)
{
 TBL_ID tbl_ptr;
    tbl_ptr = tblptr[tindex];
    if (tbl_ptr == NULL)
    {
	/* IF table non existent, Get table pointer from */
	/* namebufs or host.				 */
	DPRINT(1,"tbl_element_addr: table ptr is null.\n");
	return((char *) NULL);
    }
    index = index/tbl_ptr->mod_factor;
    if (index > (tbl_ptr->num_entries-1))
    {
	DPRINT(1,"tbl_element_addr: index greater than number of entries.\n");
	return((char *) NULL);
    }
    return( ((char *)(&tbl_ptr->entry) + (tbl_ptr->size_entry*index)) );
}

/*----------------------------------------------------------------------*/
/* jtbl_element_addr							*/
/*	Returns the address of a table element.				*/
/*	Arguments:							*/
/*		*tbl_ptr	: Top of Table Addesses.		*/
/*		tindex		: table index				*/
/*		nindex		: num of indices (dims) into table.	*/
/*		index		: index(indicies) into table.		*/
/*	*/
/* Jtable Header format:  Each header entry is a 32 bin int. 	*/
/* 	Entry 1:	num entries		*/
/* 	Entry 2: 	size of each entry (in 32 bit integers	*/
/*	Entry 3:	num of dimensions	*/
/*	Entry 4:	num of dim1 entries	*/
/*	Entry 5:	dim1 mod_factor		*/
/* 	opt Entry 6:	num of dim2 entries	*/
/*	opt Entry 7:	dim2 mod_factor		*/
/* 	...					*/
/* 	opt Entry 2+2N:	num of dimN entries	*/
/*	opt Entry 3+2N:	dimN mod_factor		*/
/*----------------------------------------------------------------------*/
char *jtbl_element_addr(TBL_ID *tblptr,short tindex,short nindex,long index[])
{
 int *tbl_ptr,*tbl_curptr;
 int nentries,size_entry,ndim,dindex,i,mod_factor,dim,prev_dim;
    /* DPRINT2(2,"jtbl_element_addr: tindex %d, nindex %d \n",tindex,nindex); */
    tbl_ptr = (int *)tblptr[tindex];
    if (tbl_ptr == NULL)
    {
	/* IF table non existent, Get table pointer from */
	/* namebufs or host.				 */
	DPRINT(-1,"tbl_element_addr: table ptr is null.\n");
	return((char *) NULL);
    }
    
    /* Get table header information */
    tbl_curptr = tbl_ptr;
    nentries = *tbl_curptr++;	/* Get total num of entries in table */
    size_entry = *tbl_curptr++;	/* Get size of each entry (in 32 bin ints) */
    ndim = *tbl_curptr++;	/* Get num of dimensions in table */
    DPRINT4(2,"jtbl_element_addr: tblptr %ld, nentries %d size_entry %d ndim %d\n",
    			tbl_ptr,nentries,size_entry,ndim);
    if (ndim != nindex) {
	DPRINT(-1,"jtbl_element_addr: dims in table NE to dims in acode.\n");
	return((char *) NULL);
    }
    
    /* Get index into table */
    i = 0;
    dindex = 0;
    prev_dim = 1;
    while (i < nindex) {
        dim = *tbl_curptr;
        mod_factor = *(tbl_curptr+1);
        /*DPRINT3(2,"jtbl_element_addr: mod_factor %d, index %d prev_dim %d\n",
        	mod_factor,index[i],prev_dim); */
	if (mod_factor > 0)
	    dindex = dindex + (index[i]%mod_factor)*prev_dim;
	else if (dim > 0)
	    dindex = dindex + (index[i]%dim)*prev_dim;
	else {
    	    errLogRet(LOGIT,debugInfo,"jtbl_element_addr: Table dimension zero.");
	    return((char *) NULL);
	}
        prev_dim = dim*prev_dim;
        tbl_curptr += 2;
        i++;
    }
    /*DPRINT3(2,"jtbl_element_addr: nentries %d, index: %d, index %% nentries: %d \n",
        	nentries,dindex,dindex % nentries); */
    /* 10-19-98 have index cycle thought table and not go off the end  GMB */
    /* dindex = dindex % nentries; */
    if (dindex > nentries)
    {
    	errLogRet(LOGIT,debugInfo,
	  "jtbl_element_addr: Index %d greater than num entries %d.",dindex,nentries);
	return((char *) NULL);
    }
    return( (char *)((tbl_curptr) + (size_entry*dindex)) );
}

/*----------------------------------------------------------------------*/
/* aptbl_element_addr	(PSG Compatability)				*/
/*	Returns the address of an ap table element.			*/
/*	Arguments:							*/
/*		acbase		: Base Addess of Acodes.		*/
/*		tindex		: ap table index			*/
/*		index		: index into table.			*/
/*----------------------------------------------------------------------*/
char *aptbl_element_addr(ushort *acbase,ushort tindex,int index)
{
 ushort *tbl_ptr;
 int Tsize,divn,ivar,autoincflag;
    tbl_ptr = acbase + tindex;
    if (tbl_ptr == NULL)
    {
	/* IF table non existent, Get table pointer from */
	/* namebufs or host.				 */
	DPRINT(1,"aptbl_element_addr: table ptr is null.\n");
	return((char *) NULL);
    }
    Tsize = *tbl_ptr++;		/* aptable size (num short words) */
    autoincflag = *tbl_ptr++;	/* auto inc flag */
    divn = *tbl_ptr++;

    if (autoincflag == 1)	/* auto increment */
    {
	index = *tbl_ptr;	/* index into table */
	index = index/divn;
	/* auto increment */
	*tbl_ptr += 1;
	*tbl_ptr %= (divn*Tsize);	/* modulo the auto_inc counter by
                                           the divn-return factor * the
                                           number of elements in the table.
                                           What about integer overflow? */
    }
    else
    {
	index = index/divn;
    }

    index = index%Tsize;	/* modulo with the table size.	*/
    if (index > (Tsize))
    {
	DPRINT(1,"aptbl_element_addr: index greater than number of entries.\n");
	return((char *) NULL);
    }
    tbl_ptr++;			/* increment address to start of table */
    return( (char *)((tbl_ptr) + (index)) );
}

/*----------------------------------------------------------------------*/
/* setphase90								*/
/* 	Sets 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
int setphase90(ACODE_ID pAcodeId, short tbl, int incr)
{
   long *tbl_addr, index;
   tbl_addr = (long *)tbl_element_addr(pAcodeId->table_ptr,tbl,0);
   if (tbl_addr == NULL) 
	return(-1);


   /* clear 0,90,180,270 phase bits */
   fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, tbl_addr[IPHASEBITS]);

   /* select trans phase quadrent */
   index = (ulong_t)(incr + tbl_addr[IPHASEQUADRANT]) & 3;
   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, tbl_addr[index]);
   return(0);
}


/*----------------------------------------------------------------------*/
/* jsetphase90								*/
/* 	Sets 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
int jsetphase90(ushort apdelay, ushort apaddr,long phase[], ushort channel )
{

   long phaseBits,phase90Bit,phase180Bit,phase270Bit,phaseMaskBits;
   long phase360;
   int quadrent, num360s;
   short saphase;

   DPRINT5(1,"jsetphase90: apdelay=0x%x, apaddr: 0x%x, value: %ld (0x%lx), channel: %d\n",
	apdelay,apaddr,phase[0],phase[0],channel);

   phase90Bit  =   (1 << ((channel)*5 + 3));
   phase180Bit =   (1 << ((channel)*5 + 4));
   phaseMaskBits = phase270Bit =   (3 << ((channel)*5 + 3));

   DPRINT4(1,"Bits: 90: 0x%lx, 180: 0x%lx, 270: 0x%lx, mask: 0x%lx\n",
         phase90Bit,phase180Bit,phase270Bit,phaseMaskBits);

   /* Extract quadrature phase     */
   phase360 = phase[0] % 36000L;  /* degrees from 0 - 360,   36000 == 360.00 degrees */
   quadrent = phase360 / 9000L;  /* quadrent 0 - 3 */
	
   DPRINT2(1,"jsetphase90: quad: 0x%x, phase360: %d, %d\n",quadrent,phase360);

   phaseBits = (quadrent << ((channel)*5 + 3));


   DPRINT3(1,"jsetphase90: quad: %d, mask bits: 0x%lx,  phase bits: 0x%lx\n",quadrent,phaseMaskBits,phaseBits);
   /* clear 0,90,180,270 phase bits */
   fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, phaseMaskBits);

   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, phaseBits);

   return(0);
}



/*----------------------------------------------------------------------*/
/* setlkdecphase90							*/
/* 	Sets 0,90,180,270 degree ap register phase bits, for lock/dec.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
int setlkdecphase90(ACODE_ID pAcodeId, short tbl, int incr)
{
   long *tbl_addr, index,phase,hsline;
   ushort addr;

   tbl_addr = (long *)tbl_element_addr(pAcodeId->table_ptr,tbl,0);
   if (tbl_addr == NULL) 
	return(-1);

   /* select trans phase quadrent */
   index = (ulong_t)(incr + tbl_addr[IPHASEQUADRANT]) & 3;
   phase = tbl_addr[index];
   DPRINT3(1,"setlkdecphase90: index; %ld, tbl_addr[%ld] = 0x%lx phase\n",
	index,index,tbl_addr[index]);

   /* select trans phase quadrent */
   index = (ulong_t)(incr + tbl_addr[IPHASEQUADRANT]) & 3;

   /* translate ap register phase bits to the equivalent HSlines of that channel */
   if (tbl < 5)  /* only upto the 4th not the fifth */
   {
     switch (phase)
     {
       case 0:	/* phase 0 */
	hsline = 0;
	break;

       case 0x40:	/* phase 90 */
	hsline = (1 << ((tbl*5)+3));
	break;

       case 0x80: /* phase 180 */
	hsline = (1 << ((tbl*5)+4));
	break;

       case 0xC0: /* phase 270 */
	hsline = (3 << ((tbl*5)+3));
	break;
     }

     /* clear 0,90,180,270 phase bits */
     fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, (3 << ((tbl*5)+3)));

     /* Set appropriate HS phase lines to allow user to see phases changing */
     /* Note: this is just for show! */
     fifoMaskHsl(pTheFifoObject,STD_HS_LINES, hsline);
  }

   addr = 0xb00 + (0x94 + (tbl*16));
   DPRINT2(1,"setlkdecphase90: ap addr; 0x%x, apval: 0x%x \n",
	addr,(short) phase);
   /* writeapword(ushort apaddr,ushort apval, ushort apdelay) */
   writeapword(addr,(ushort) phase, (ushort) 40);  /* 40 * 12.5 = 500 nsec delay */
   
   return(0);
}



/*----------------------------------------------------------------------*/
/* jsetlkdecphase90							*/
/* 	Sets small angle phase and 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
int jsetlkdecphase90(ushort apdelay, ushort apaddr,long phase, ushort channel )
{
   long phaseBits,phase90Bit,phase180Bit,phase270Bit,phaseMaskBits;
   int hsline;

   DPRINT5(1,"jsetlkdecphase90: apdelay=0x%x, apaddr: 0x%x, value: %ld (0x%lx), channel: %d\n",
	apdelay,apaddr,phase,phase,channel);

   /* translate ap register phase bits to the equivalent HSlines of that channel */
   if (channel < 4)  /* start at 0, only upto the 4th not the fifth */
   {
     switch (phase)
     {
       case 0:	/* phase 0 */
	hsline = 0;
	break;

       case 0x40:	/* phase 90 */
	hsline = (1 << ((channel*5)+3));
	break;

       case 0x80: /* phase 180 */
	hsline = (1 << ((channel*5)+4));
	break;

       case 0xC0: /* phase 270 */
	hsline = (3 << ((channel*5)+3));
	break;
     }

     /* clear 0,90,180,270 phase bits */
     fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, (3 << ((channel*5)+3)));

     /* Set appropriate HS phase lines to allow user to see phases changing */
     /* Note: this is just for show! */
     fifoMaskHsl(pTheFifoObject,STD_HS_LINES, hsline);
  }

   /* writeapword(ushort apaddr,ushort apval, ushort apdelay) */
   writeapword(apaddr,(ushort) phase, apdelay );  /* was 40 * 12.5 = 500 nsec delay */

   return(0);
}



int setlkdecOnOff(int apaddr, int decOn, int apdelay)
{
   long *tbl_addr, index,phase ;
   ushort addr;

   if (decOn)
   {
     /*DPRINT3(1,
     "setlkdecOnOff: Set Decoupler Power: addr: 0x%x, value: %d, apdelay: %d\n",
	    lkdecAddr, lkdecpwr,lkdecApDelay); */

     /* Now use the initialize values, and actual set the dec power */
     /* setLockDecPower( 0, 0, 0, 0 ); */

     /* setLockDecPower( lkdecAddr, lkdecpwr, lkdecApDelay, 0 ); */
     DPRINT2(1,"setlkdecOnOff: writing 0x%x to 0x%x\n",0xef83,apaddr);
     writeapword((ushort)apaddr,(ushort) 0xef83, (ushort) apdelay);  /* 40 * 12.5 = 500 nsec delay */
   }
   else
   {
     /* reset lock power which may of been corrupted by the decoupler power setting */
     /* DPRINT1(1,"setlkdecOnOff: reset lockpwr to  %d \n",
		currentStatBlock.stb.AcqLockPower); */
     /* setLockPower( (int) currentStatBlock.stb.AcqLockPower, 0 ); */

     DPRINT2(1,"setlkdecOnOff: writing 0x%x to 0x%x\n",0xef01,apaddr);
     writeapword((ushort) apaddr,(ushort) 0xef01, (ushort) apdelay);  /* 40 * 12.5 = 500 nsec delay */
   }
}

/*----------------------------------------------------------------------*/
/* setphase								*/
/* 	Sets small angle phase and 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
int setphase(ACODE_ID pAcodeId, short tbl, int phaseincr)
{
   long *tbl_addr, index;
   short saphase;
   tbl_addr = (long *)tbl_element_addr(pAcodeId->table_ptr,tbl,0);
   if (tbl_addr == NULL) 
	return(-1);

   /* clear 0,90,180,270 phase bits */
   fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, tbl_addr[IPHASEBITS]);

   /* select trans phase quadrent */
   index = (ulong_t)((phaseincr * tbl_addr[IPHASESTEP])/tbl_addr[IPHASEPREC]) 
									& 3;
   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, tbl_addr[index]);

   /* set phase quadrent */
   tbl_addr[IPHASEQUADRANT] = index;

   /* set small angle phase shift */
   saphase = (phaseincr * tbl_addr[IPHASESTEP]) % tbl_addr[IPHASEPREC];
   writeapword(tbl_addr[IPHASEAPADDR],saphase,tbl_addr[IPHASEAPDELAY]);

   return(0);
}

/*----------------------------------------------------------------------*/
/* jsetphase								*/
/* 	Sets small angle phase and 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/* modified 10/26/99							*/
/*   this modification,  changes the behavior, rather than set small    */
/* angle phase (SAP) all the time, the present SAP is stored and check  */
/* if defferent then set both SAP and Quad as before, if not then just  */
/* just set quad phase but preceed Quad phase with 500 nsec delay       */
/* to retain the constant duration of phase setting		        */
/*----------------------------------------------------------------------*/
int jsetphase(ushort apdelay, ushort apaddr,long phase[], ushort channel )
{
   long phaseBits,phase90Bit,phase180Bit,phase270Bit,phaseMaskBits;
   long phase360;
   int quadrent, num360s;
   short saphase;
   short remaindelay;

   DPRINT5(1,"jsetphase: apdelay=0x%x, apaddr: 0x%x, value: %ld (0x%lx), channel: %d\n",
	apdelay,apaddr,phase[0],phase[0],channel);

   phase90Bit  =   (1 << ((channel)*5 + 3));
   phase180Bit =   (1 << ((channel)*5 + 4));
   phaseMaskBits = phase270Bit =   (3 << ((channel)*5 + 3));

   DPRINT4(1,"Bits: 90: 0x%lx, 180: 0x%lx, 270: 0x%lx, mask: 0x%lx\n",
         phase90Bit,phase180Bit,phase270Bit,phaseMaskBits);
   /* Extract quadrature phase     */
   /* quadrent = (phase[0] / 9000) % 4;
   num360s = phase[0] / 36000;
   */
   phase360 = phase[0] % 36000;  /* degrees from 0 - 360,   36000 == 360.00 degrees */
   quadrent = phase360 / 9000L;  /* quadrent 0 - 3 */
   saphase = (short) ( (phase360 % 9000L) / 25L);   /* phase is .25 degrees increments i.e. /25L) */
   /* saphase = (short) (phase[0] - (quadrent * 9000) - (num360s * 36000)); */

   DPRINT4(1,"jsetphase: quad: 0x%x, phase360: %d, small phase: %d, previous SAP: %d\n",
		quadrent,phase360,saphase*25,smphase[channel]*25);

   phaseBits = (quadrent << ((channel)*5 + 3));

   /* DPRINT2(0,"jsetphase: >>>>>>> present SAP: %d, new SAP: %d\n",smphase[channel]*25,saphase*25); */
   if (smphase[channel] != saphase)
   {
      /* DPRINT(0,"Do Quad & SAP\n"); */
      smphase[channel] = saphase;
      /* set small angle phase shift */ /* now small ange should be set prior to quadrent */
      /* 24 * 12.5 = 300 nsecs */
      writeapword(apaddr,saphase,16); /* set small angle and wait 300 nsec prior to changing quadrent */

      DPRINT2(1,"jsetphase: mask bits: 0x%lx,  phase bits: 0x%lx\n",phaseMaskBits,phaseBits);
      /* clear 0,90,180,270 phase bits */
      fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, phaseMaskBits);

      fifoMaskHsl(pTheFifoObject,STD_HS_LINES, phaseBits);

      remaindelay = apdelay - 16;
      /* DPRINT1(0,"jsetphase: remaining delay: %d\n",remaindelay); */
      if (remaindelay < 8) remaindelay = 8;
      fifoStuffCmd(pTheFifoObject,CL_DELAY,remaindelay);
   }
   else
   {
      /* DPRINT(0,"Do Just Quad\n"); */
      /* delay for 400 nsec prior to setting Quad phase */
      /* fifoStuffCmd(pTheFifoObject,CL_DELAY,40); /* wait 500 nsec prior to changing quadrent */
      fifoStuffCmd(pTheFifoObject,CL_DELAY,32); /* wait 400 nsec prior to changing quadrent */

      DPRINT2(1,"jsetphase: mask bits: 0x%lx,  phase bits: 0x%lx\n",phaseMaskBits,phaseBits);
      /* clear 0,90,180,270 phase bits */
      fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, phaseMaskBits);

      fifoMaskHsl(pTheFifoObject,STD_HS_LINES, phaseBits);

      remaindelay = apdelay - 24; /* 32 - 8(apbus 100nsec not in apdelay) */
      /* DPRINT1(0,"jsetphase: remaining delay: %d\n",remaindelay); */
      if (remaindelay < 8) remaindelay = 8;
      fifoStuffCmd(pTheFifoObject,CL_DELAY,remaindelay);
   }

   /* set small angle phase shift */
   /* writeapword(apaddr,saphase,apdelay); */ /* reported error 300nsec between quad & small */

   return(0);
}


/*----------------------------------------------------------------------*/
/* japsetphase								*/
/*	Adds current phase to supplied phase then...			*/
/* 	Sets small angle phase and 0,90,180,270 degree phase bits.	*/
/*	Arguments: 							*/
/*		table index : pointer to current acode.			*/
/*		phase increment :					*/
/* NOTE: This currently does not take into account Extended HS Lines.	*/
/*----------------------------------------------------------------------*/
void japsetphase(ushort apdelay, ushort apaddr,long phase[], ushort channel )
{
   long phaseBits,phase90Bit,phase180Bit,phase270Bit,phaseMaskBits;
   long phase360;
   int quadrent, num360s;
   short saphase;
   short remaindelay;

   DPRINT5(1,"japsetphase: apdelay=0x%x, apaddr: 0x%x, value: %ld (0x%lx), channel: %d\n",
	apdelay,apaddr,phase[0],phase[0],channel);

   phase90Bit  =   (1 << ((channel)*5 + 3));
   phase180Bit =   (1 << ((channel)*5 + 4));
   phaseMaskBits = phase270Bit =   (3 << ((channel)*5 + 3));

   DPRINT4(1,"Bits: 90: 0x%lx, 180: 0x%lx, 270: 0x%lx, mask: 0x%lx\n",
         phase90Bit,phase180Bit,phase270Bit,phaseMaskBits);
   /* Extract quadrature phase     */
   /* quadrent = (phase[0] / 9000) % 4;
   num360s = phase[0] / 36000;
   */
   /* Add current small angle phase to phase */
   phase360 = phase[0] + (smphase[channel]*25L);
   phase360 = phase360 % 36000;  /* degrees from 0 - 360,   36000 == 360.00 degrees */
   quadrent = phase360 / 9000L;  /* quadrent 0 - 3 */
   saphase = (short) ( (phase360 % 9000L) / 25L);   /* phase is .25 degrees increments i.e. /25L) */
   /* saphase = (short) (phase[0] - (quadrent * 9000) - (num360s * 36000)); */

   DPRINT4(1,"jsetphase: quad: 0x%x, phase360: %d, small phase: %d, previous SAP: %d\n",
		quadrent,phase360,saphase*25,smphase[channel]*25);

   phaseBits = (quadrent << ((channel)*5 + 3));

   /* DPRINT2(0,"jsetphase: >>>>>>> present SAP: %d, new SAP: %d\n",smphase[channel]*25,saphase*25); */

      /* DPRINT(0,"Do Quad & SAP\n"); */
      /* smphase[channel] = saphase; */
      /* set small angle phase shift */ /* now small ange should be set prior to quadrent */
      /* 24 * 12.5 = 300 nsecs */
      writeapword(apaddr,saphase,16); /* set small angle and wait 300 nsec prior to changing quadrent */

      DPRINT2(1,"jsetphase: mask bits: 0x%lx,  phase bits: 0x%lx\n",phaseMaskBits,phaseBits);
      /* clear 0,90,180,270 phase bits */
      fifoUnMaskHsl(pTheFifoObject, STD_HS_LINES, phaseMaskBits);

      fifoMaskHsl(pTheFifoObject,STD_HS_LINES, phaseBits);

      remaindelay = apdelay - 16;
      /* DPRINT1(0,"jsetphase: remaining delay: %d\n",remaindelay); */
      if (remaindelay < 8) remaindelay = 8;
      fifoStuffCmd(pTheFifoObject,CL_DELAY,remaindelay);

   /* set small angle phase shift */
   /* writeapword(apaddr,saphase,apdelay); */ /* reported error 300nsec between quad & small */

   /* return(0); */
}

int
calcRecvGainVals( int gain, ushort *pPreampAttn, ushort *pRset )
{
   ushort preampattn, rset;

   if (gain >= 18)  {
	preampattn=0;
	gain = gain - 18;
   }
   else if (gain >= 12)  {
	preampattn = 2;
	gain = gain - 12;
   }
   else if (gain >= 6)  {
	preampattn = 1;
	gain = gain - 6;
   }
   else {
	preampattn = 3;
   }

   rset = 0x3e; 		/* 0x3e */
   if (gain >= 14) {
	rset = rset & 0x3d;	/* set 14 dB input */
	gain -= 14;
   }
   if (gain >= 14) {
	rset = rset & 0x3b;	/* set 14 dB output */
	gain -= 14;
   }
   if (gain >= 8) {
	rset = rset & 0x37;	/* set 8 dB  */
	gain -= 8;
   }
   if (gain >= 4) {
	rset = rset & 0x2f;	/* set 4 dB output */
	gain -= 4;
   }
   if (gain >= 2) {
	rset = rset & 0x1f;	/* set 2 dB output */
	gain -= 2;
   }

   *pPreampAttn = preampattn;
   *pRset = rset;
   return( gain );
}

/*----------------------------------------------------------------------*/
/* receivergain								*/
/* 	Sets the preamp attenuators in the magnet leg and the receiver	*/
/*	gain in the receiver controller.  The preamp attenuators are 	*/
/*	controlled by the same apchip that controls the lock		*/
/*	attenuator.							*/
/*	Arguments: 							*/
/*		gain  : total receiver gain which includes preamp 	*/
/*			attenuators and gain.				*/
/*----------------------------------------------------------------------*/
void receivergain(ushort apaddr, ushort apdelay, int gain)
{
 ushort preampattn, rset, lkpreampreg;
 short tmpgain;

 DPRINT3(1,"receivergain: apaddr: 0x%lx, apdelay: %d, gain: %d\n",
		apaddr, apdelay, gain);
   tmpgain = gain;

   if (gain >= 18)  {
	preampattn=0;
	gain = gain - 18;
   }
   else if (gain >= 12)  {
	preampattn = 2;
	gain = gain - 12;
   }
   else if (gain >= 6)  {
	preampattn = 1;
	gain = gain - 6;
   }
   else {
	preampattn = 3;
   }

   rset = 0x3e; 		/* 0x3e */
   if (gain >= 14) {
	rset = rset & 0x3d;	/* set 14 dB input */
	gain -= 14;
   }
   if (gain >= 14) {
	rset = rset & 0x3b;	/* set 14 dB output */
	gain -= 14;
   }
   if (gain >= 8) {
	rset = rset & 0x37;	/* set 8 dB  */
	gain -= 8;
   }
   if (gain >= 4) {
	rset = rset & 0x2f;	/* set 4 dB output */
	gain -= 4;
   }
   if (gain >= 2) {
	rset = rset & 0x1f;	/* set 2 dB output */
	gain -= 2;
   }

   /* set preamp atten using lock backing store */
   lkpreampreg = getlkpreampgainvalue(); 
   lkpreampreg = (lkpreampreg & ~0x0003) | preampattn;
   writeapword( getlkpreampgainapaddr(), lkpreampreg | 0xff00, apdelay );
   storelkpreampgainvalue(lkpreampreg);


   /* set receiver */
   writeapword( apaddr, rset | 0xff00, apdelay );

   /* Update status block */
   currentStatBlock.stb.AcqRecvGain = tmpgain;

}

/*----------------------------------------------------------------------*/
/*									*/
/* setpresig								*/
/*	Set the register that controls the signal handling bit.		*/
/*	Address: b4a							*/
/*	Get stored value and set it.		*/
/*	1 parameter:							*/
/*		1st: LARGE (1) or NORMAL (0)				*/
/*----------------------------------------------------------------------*/
void setpresig(ushort signaltype)
{
 ushort lkpreampreg;

	if (conf_msg.hw_config.mleg_conf == MLEG_SIS_PIC)
	{
	   signaltype = (signaltype == LARGE) ? 0x0 : 0x8;
        }
        else   /* MLEG_LGSIGMODE or New PIC */
        {
	   signaltype = (signaltype == LARGE) ? 0x8 : 0x0;
        }
/*
	if (conf_msg.hw_config.mleg_conf == MLEG_SIS_PIC)
	   if (signaltype == LARGE) signaltype = 0x0;
	   else signaltype = 0x8;
	else if (signaltype == LARGE) signaltype = 0x8;
	   else signaltype = 0x0;
*/

	/* write value to register */
	/* set preamp atten using lock backing store */
	lkpreampreg = getlkpreampgainvalue(); 
	lkpreampreg = (lkpreampreg & ~0x0008) | signaltype;
	writeapword(getlkpreampgainapaddr(),lkpreampreg | 0xff00,AP_MIN_DELAY_CNT);
	storelkpreampgainvalue(lkpreampreg);
}

/*----------------------------------------------------------------------*/
/* apbcout								*/
/* 	Interprets APbus codes.	*/
/*	Arguments: 							*/
/*		intrpp 	: pointer to current acode.			*/
/*----------------------------------------------------------------------*/
#define APADDRMSK	0x0fff 
#define APBYTF		0x1000
#define APINCFLG	0x2000

#define APCCNTMSK	0x0fff
#define APCONTNUE	0x1000

unsigned short *apbcout(unsigned short *intrpp)
{
 unsigned short addr; 
 unsigned short incf; 
 unsigned short bytf; 
 short apcount; 
 unsigned short aplast;
 unsigned short apdelay;
 unsigned short latch;

   latch = 0xef00;    /* for byte mode and ap&f board only */
   apdelay = *intrpp++;
   do 
   {
         apcount = (*intrpp) & APCCNTMSK; 
         aplast = (*intrpp) & APCONTNUE;
         intrpp++; 
 	 addr = (*intrpp) & APADDRMSK; 
	 bytf = (*intrpp) & APBYTF; 
	 incf = (*intrpp) & APINCFLG;
	 intrpp++; 
         DPRINT2(1,"apbcout: apcount: %d, aplast: %d\n",apcount,aplast);
         DPRINT2(1,"apbcout: byteflag: %d, incrFlg: %d\n",bytf,incf);
	 do
	 { 
	    if (!bytf)
	    { 
                DPRINT2(1,"apbcout: apaddr: 0x%x, apdata 0x%x\n",addr,*intrpp);
		writeapword(addr,*intrpp,apdelay);
	 	apcount--; 
	    } 
	    else
	    { 
	 	/* check byte order */
                DPRINT2(1,"apbcout: apaddr: 0x%x, apdata 0x%x\n",addr,((*intrpp >> 8)|latch));
		writeapword(addr,(*intrpp >> 8)|latch,apdelay);
		latch = 0xff00; /* after very first hold high */
	 	apcount--;
	 	if (apcount > 0) 
	 	{
	 	    if (incf) 
	 		addr += 1; 
                    DPRINT2(1,"apbcout: apaddr: 0x%x, apdata 0x%x\n",addr,(*intrpp|latch));
		    writeapword(addr,*intrpp|latch,apdelay);
	 	    apcount--; 
	 	} 
	    } 
	    if (incf) 
	        addr += 1;
	    intrpp++; 
	 } while (apcount > 0);
   } while (aplast != 0); 
   return(intrpp);
}

#define APBYTEBUSMASK	0xff00
/*----------------------------------------------------------------------*/
/* aprtout								*/
/* 	Outputs a value in the realtime variable table to an apaddress.	*/
/*	First it adds an offset to value then it checks against min 	*/
/*	and max values before sending it.				*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		maxval	: maximum value after offset.			*/
/*		minval	: minimum value after offset.			*/
/*		offset	: offset to be added before comparison.		*/
/*		apvalue	: value to be sent to apbus.			*/
/*----------------------------------------------------------------------*/
int aprtout(ushort apdelay, ushort apaddr, short maxval, short minval,
						 short offset, int apval)
{
   int status;
   status = 0;
   apval = apval+offset;
   /* if maxval == -1 skip checks */
   if (maxval != (short)(-1))
   {
     if (apval > maxval)
     {
	/* return warning message to phandler */
    	errLogRet(LOGIT,debugInfo,
		"aprtout Warning: Overflow value: %d , max: %d.",apval,maxval);
	apval = maxval;
	status = 1;
     }
     if (apval < minval)
     {
	/* return warning message to phandler */
    	errLogRet(LOGIT,debugInfo,
	    "aprtout Warning: Underflow value: %d , max: %d.",apval,minval);
	apval = minval;
	status = 1;
     }
   }
   /* byte or word size encoded in apaddr */
   if ((apaddr & APBYTF) != 0)
   {
	/* SPECIAL CASE: Assume byte write and maxval == 79 is  */
	/* to 1dB Attenuators need to left shift value for 0.5 	*/
	/* dB lsb.		 				*/
	if (maxval == 79)
	   apval = apval << 1;
	writeapword(apaddr & APADDRMSK,(ushort)(apval|APBYTEBUSMASK),apdelay);
   }
   else
	writeapword(apaddr,(ushort)(apval),apdelay);

   return(status);
}

/*----------------------------------------------------------------------*/
/* japbcout								*/
/* 	Outputs a value or values from the realtime variable table 	*/
/*	to apaddress(es).  In the address field are the APBYTF and	*/
/*	and APINCFLG flags for defining byte values and whether to	*/
/*	increment the apbus address					*/
/* NOTE: Currently the data is not packed into the high order word	*/
/*	 of the data integer.						*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		data[]	: starting location of data .			*/
/*			  can be from table or rtvar buffer		*/
/*		count	: number of values to stuff.			*/
/*----------------------------------------------------------------------*/
extern int japbcout(ushort apdelay,ushort apaddr,long data[],short apcount)
{
 unsigned short addr,incf,bytf,apdata,latch;
 int i;
 int intflag;  /* use integer data from the msb */
 
   /* byte or word size encoded in apaddr */
   latch = 0xef00;    /* for byte mode and ap&f board only */
   bytf = (apaddr) & APBYTF; 
   incf = (apaddr) & APINCFLG;
   addr = (apaddr) & APADDRMSK;
   
   /* determine whether or not to start at msb of integer data */
   /* or the msb of short data. */
   if ((apcount>2) || ((!bytf) && (apcount>1))) intflag = 1;
   else intflag = 0;
   
   i = 0;
   do { 
   	if (intflag) 
        {
   	    if (i%2) 
		apdata = (short) (data[i/2]);
   	    else 
		apdata = (short) (data[i/2] >> 16);
   	}
   	else  
	    apdata = (short)data[i];

        DPRINT3(1,"japbcout: apdata 0x%x data[%d] 0x%x\n",apdata,i,data[i]);
	if (!bytf) { 
	    writeapword(addr,apdata,apdelay);
	    apcount--; 
	} 
	else { 
	    /* check byte order */
	    writeapword(addr,(apdata >> 8)|latch,apdelay);
        DPRINT1(1,"japbcoutbyte: apdata1 0x%x \n",(apdata >> 8)|latch);
	    latch = 0xff00; /* after very first hold high */
	    apcount--;
	    if (apcount > 0) {
	 	if (incf) 
	 	    addr += 1; 
		writeapword(addr,apdata|latch,apdelay);
        DPRINT1(1,"japbcoutbyte: apdata2 0x%x \n",apdata|latch);
	 	apcount--; 
	    } 
	} 
	if (incf) 
	    addr += 1;
	i++; 
   } while (apcount > 0);
}

#define WFG_ADDR_REG	0
#define WFG_LOAD_REG	1
/*----------------------------------------------------------------------*/
/* japwfgout								*/
/* 	Outputs a value or values from the realtime variable table 	*/
/*	to wavegen address(es).  Calls apbcout to set the wavegen 	*/
/*	address and japbcout to write the values.  This also contains	*/
/*	an offset field which allows retrieving the instruction start	*/
/*	address and indexing into the instruction.			*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: board apbus address				*/
/*		wfgaddr	: wavegen address 				*/
/*		offset 	: offset to add to wavegen address		*/
/*		data[]	: starting location of data .			*/
/*			  can be from table or rtvar buffer		*/
/*		count	: number of values to stuff.			*/
/*----------------------------------------------------------------------*/
extern int japwfgout(ushort apdelay,ushort apaddr,long wfgaddr,short offset,
						long data[],short count)
{
 unsigned short addr,wfgdata_addr,wfgaddrreg,wfgloadreg,apdata;
 int i,apcount;
 
   /* byte or word size encoded in apaddr */
   addr = (apaddr) & APADDRMSK;
   wfgaddrreg = addr+WFG_ADDR_REG;
   wfgloadreg = addr+WFG_LOAD_REG;
   wfgdata_addr = (unsigned short)wfgaddr+offset;
   
   /* Since every wfg word is 32 bits always start at msb of integer data */
   
   /* first set the wfg start addr */
   writeapword(wfgaddrreg,wfgdata_addr,apdelay);
   
   apcount = count*2;	/* count is the number of wfg data words to load */
   			/* apcount is the number of apwords to load	 */
   			/* count is 32 bit words, apcount is 16 bit words*/
   i = 0;
   /* japwfgout loads the low order word and then the upper word,	*/
   /* which is how the wavegen memory loader expects it.		*/
   do { 
   	if (i%2) 
	    apdata = (short) (data[i/2] >> 16);
   	else 
	    apdata = (short) (data[i/2]);

        DPRINT3(1,"japwfgout: apdata 0x%x data[%d] 0x%x\n",apdata,i,data[i]);
	writeapword(wfgloadreg,apdata,apdelay);
	apcount--; 
	i++; 
   } while (apcount > 0);
}

/*----------------------------------------------------------------------*/
/* jsetfreq								*/
/* 	Outputs frequency or frequencies from a realtime variable  	*/
/*	location or table location.  Gets an apbase address and	*/
/*	pts number							*/
/* NOTE: Currently the routine knows how many words to stuff so		*/
/*	 the count is not passed.  The routine also knows how to	*/
/*	 stuff the codes.						*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		data[]	: starting location of data .			*/
/*			  can be from table or rtvar buffer		*/
/*		ptsnum	: pt.			*/
/*----------------------------------------------------------------------*/
extern int jsetfreq(ushort apdelay,ushort baseapaddr,long data[],short ptsno)
{
 unsigned short addr,incf,bytf,apdata[2],latch,apcount;
 int i,overunder;
 DPRINT5(1,"jsetfreq: apdly: %d, 0x%x, apaddr: 0x%x, data adr: 0x%lx, pts#: 0x%x\n",
	apdelay,apdelay,baseapaddr,data,ptsno);
   apcount = 5;
   /* get overrange/underrange flag */
   overunder = data[0];
   
   /* if in parallel channel then put in channel lock */
   if (pTheFifoObject->pPChanObj)
       pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_LOCK,0);

   /* send frequency values to pts */
   latch = 0xef00;    /* for byte mode and ap&f board only */
   addr = (baseapaddr) & APADDRMSK;
   writeapword(addr,0|latch,apdelay);
   DPRINT3(1,"   writeapword: addr: 0x%x, values: 0x%x, delay: 0x%x\n",addr,(0|latch),apdelay);
   latch = 0xff00; /* after very first hold high */
   addr = addr + 1;
   i = 1;
   do { 
        apdata[0] = (ushort)data[i] ;
	/* check byte order, write as bytes */
	writeapword(addr,(apdata[0] >> 8)|latch,apdelay);
        DPRINT3(1,"   writeapword: addr: 0x%x, values: 0x%x, delay: 0x%x\n",addr,((apdata[0] >> 8)|latch),apdelay);
	apcount--;
	if (apcount > 0)
        {
	   writeapword(addr,apdata[0]|latch,apdelay);
           DPRINT3(1,"   writeapword: addr: 0x%x, values: 0x%x, delay: 0x%x\n",addr,(apdata[0]|latch),apdelay);
        }
	apcount--;
	i++; 
   } while (i < 4);
   
   /* write pts select bit */
   addr = addr + 1;
   writeapword(addr,(ushort)ptsno|latch,apdelay);   
   DPRINT3(1,"   writeapword: addr: 0x%x, values: 0x%x, delay: 0x%x\n",addr,(ptsno|latch),apdelay);
   writeapword(addr,0x0|latch,apdelay);   
   DPRINT3(1,"   writeapword: addr: 0x%x, values: 0x%x, delay: 0x%x\n",addr,(0|latch),apdelay);

   /* if in parallel channel then turn off channel lock */
   if (pTheFifoObject->pPChanObj)
      pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_CHAN_UNLOCK,0);
}

/*----------------------------------------------------------------------*/
/* settunefreq								*/
/* 	Stores APbus codes and band select to set tune frequencies.	*/
/*	Arguments: 							*/
/*		intrpp 	: pointer to current acode.			*/
/*----------------------------------------------------------------------*/
unsigned short *settunefreq(unsigned short *intrpp)
{
   ushort channel,bandselect,freqcode,type;
   int nfreqcodes,i;

   channel = *intrpp++;
   type = channel & 0xff00;	/* mask off lower byte to get jpsg type. */
   channel = channel & 0x00ff;	/* mask off upper byte to get channel num. */
   nfreqcodes = (int)*intrpp++;
   if ((pTheTuneObject != NULL) && (nfreqcodes <= MAX_FREQ_CODES))
   {
     for (i=0; i<nfreqcodes; i++)
     {
	/* store freq codes */
	pTheTuneObject->tuneChannel[channel-1].ptsapb[i] = *intrpp++;
	DPRINT2(1,"settunefreq: acode %d: 0x%x\n",i,
			pTheTuneObject->tuneChannel[channel-1].ptsapb[i]);
     }
     pTheTuneObject->tuneChannel[channel-1].band = *intrpp++;
     pTheTuneObject->tuneChannel[channel-1].type = type;
     DPRINT2(1,"settunefreq: channel= %d  bandselect= %d\n",channel,
				pTheTuneObject->tuneChannel[channel-1].band);
   } 
   else
   {
     intrpp = intrpp + (nfreqcodes+1);
     DPRINT(0,"settunefreq: pTheTuneObject is NULL.\n");
   }

   return(intrpp);
}


/*----------------------------------------------------------------------*/
/* incgradient								*/
/*	Performs the following equation: 				*/
/*		value = a*x + b*y + c*z + base				*/
/*	Then calls gradient to set the value.			 	*/ 
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		maxval	: maximum value to be set.			*/
/*		minval	: minimum value to be set.			*/
/*		vmult	: multiplier.					*/
/*		incr	: increment.					*/
/*		base	: base offset.					*/
/*----------------------------------------------------------------------*/
int incgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
    int vmult1,int incr1,int vmult2,int incr2,int vmult3,int incr3,int base)
{
   int value;
   value = incr1*vmult1 + incr2*vmult2 + incr3*vmult3 + base;
   return( gradient(apdelay,apaddr,maxval,minval,value) );
}

/*----------------------------------------------------------------------*/
/* vgradient								*/
/*	Performs the following equation: value = vmult*incr + base.	*/
/*	Then calls gradient to set the value.			 	*/ 
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		maxval	: maximum value to be set.			*/
/*		minval	: minimum value to be set.			*/
/*		vmult	: multiplier.					*/
/*		incr	: increment.					*/
/*		base	: base offset.					*/
/*----------------------------------------------------------------------*/
int vgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
					int vmult,int incr,int base)
{
   int value;
   value = vmult*incr + base;
   return( gradient(apdelay,apaddr,maxval,minval,value) );
}

/*----------------------------------------------------------------------*/
/* gradient								*/
/*	Gradient checks the value against its min and max values before */ 
/*	sending it to the requested apbus address.  The apbus address	*/
/*	is encoded with the details of the size of the value to be	*/
/*	sent.  Bit 12 and 13 (msb = bit 15) are defined as in apbcout.	*/
/*	Bits 14-15 are defined as: 0=16(12) bit value, 1=20 bit value,	*/
/*			2,3 are undefined.				*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		maxval	: maximum value to be set.			*/
/*		minval	: minimum value to be set.			*/
/*		value	: value.					*/
/*----------------------------------------------------------------------*/
int gradient(ushort apdelay,ushort apaddr,int maxval,int minval,int value)
{
   int status,wordsize;
   ushort bytf,incf;
   status = 0;
   DPRINT4(1,"gradient: apaddr: 0x%x apdelay: 0x%x, maxval: %d value: %d\n",
   		apaddr,apdelay,maxval,value);
   if ( value < minval) {
    	errLogRet(LOGIT,debugInfo,
	   "gradient Warning: Underflow value: %d , max: %d.",value,minval);
	value = minval;
	status = 1;
   }
   if ( value > maxval) {
    	errLogRet(LOGIT,debugInfo,
	   "gradient Warning: Overflow value: %d , max: %d.",value,maxval);
	value = maxval;
	status = 1;
   }
   wordsize = (apaddr & GRADDATASIZ) >> 14;
   bytf = apaddr & APBYTF; 
   incf = apaddr & APINCFLG;
   apaddr = apaddr & APADDRMSK; 
   switch(wordsize) {
	case 0:			/* 16 bit */
	   {
		if (!bytf)
		    writeapword(apaddr,(ushort)value,apdelay);
		else {
		    short apdata;
		    apdata = value;
		    writeapword(apaddr,(apdata >> 8)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    writeapword(apaddr,apdata|APSTDLATCH,apdelay);
		}
	   }
	   break;
	case 1:			/* 20 bit */
	   {
		if (!bytf)
		{
		    status= -1;
    		    errLogRet(LOGIT,debugInfo,
			"gradient Error: Nonexistent 20 bit apword setting.");
		}
		else {
		    short apdata;
		    apdata = value;
		    writeapword(apaddr,(apdata >> 12)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    writeapword(apaddr,(apdata >> 4)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    writeapword(apaddr,(apdata << 4)|APSTDLATCH,apdelay);
		}
	   }
	   break;
	default:
	   status = -1;
    	   errLogRet(LOGIT,debugInfo,
		"gradient Error: Unknown gradient data size setting.");
	   break;
   }
   return(status);
}

/*----------------------------------------------------------------------*/
/* jsetgradmatrix							*/
/* 	Outputs x, y and z components of oblique gradient axis.  	*/
/*	These will be stored in the gradient oblique matrix.		*/
/*	Arguments:							*/
/*		gradcomponent	: ss, pe or ro axis component.		*/
/*		      	ro=RO_AXIS=0 pe=PE_AXIS=1 ss=SS_AXIS=2		*/
/*		data[]	: starting location of data, 3 longs.		*/
/*			  can be from table or rtvar buffer		*/
/* 	Definition of oblgradmatrix: 9 longs				*/
/*		0 = ro(x)  1 = pe(x)  2 = ss(x)				*/
/*		3 = ro(y)  4 = pe(y)  5 = ss(y)				*/
/*		6 = ro(z)  7 = pe(z)  8 = ss(z)				*/
/*----------------------------------------------------------------------*/
void jsetgradmatrix(ushort gradcomponent,long data[])
{
 int i,count;
 unsigned long tempval;

   /* get obl matrix values and set matrix. */
   count = 3;
   for (i=0; i<count; i++) {
      tempval = (data[i] & 0x0fffffff) | ((gradcomponent + i*count) << 28);
      DPRINT4(1,"jsetgradmatrix: i: %d gradcomponent: %d, data: %d pword: 0x%lx\n",
	    i,gradcomponent,data[i],tempval);
      /* if in parallel channel then put in channel, otherwise set directly. */
      if (pTheFifoObject->pPChanObj)
          pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_SETOBLMATRIX,tempval);
      else
          setgradmatrix(tempval);
   }
}

/*----------------------------------------------------------------------*/
/* jsetoblgrad								*/
/*	"jsetoblgrad" either calls "setoblgrad" or encodes a parallel	*/ 
/*	event word and puts it in the parallel event buffer.  This will	*/
/*	eventually add up the matrix components of the x, y or z 	*/
/* 	gradient given by "axis" and set that gradient.  "apdelay" and	*/
/*	"apaddr" are defined the same way as in the "gradient" routine.	*/
/*	For this oblgrad all values calculated according to 		*/
/*	OBLGRAD_BITS2SCALE (currently 20).  "nbits" defines the number	*/
/*	of bits for the particular gradient and the number of bits to	*/
/*	shift the value will be encoded in the event word.		*/
void jsetoblgrad(ushort apdelay,ushort apaddr,ushort axis,ushort select)
{
   ushort rshift;
   unsigned long tempval;
   
   /* rshift = OBLGRAD_BITS2SCALE - nbits; */
   /* apdelay = (rshift << 12) | (apdelay & 0x0fff); */
   tempval = (select << 16) | (apaddr & 0x0000ffff);
   DPRINT4(1,"jsetoblgrad: select: %d apdelay: 0x%x apaddr: 0x%x tempval: 0x%lx\n",
   		select,apdelay,apaddr,tempval);   
   /* if in parallel channel then put in channel, otherwise set directly. */
   if (pTheFifoObject->pPChanObj) {
       if (axis == Z_AXIS)
          pchanPut((PCHANSORT_ID)pTheFifoObject->pPChanObj,PCHAN_SETOBLGRADZ | apdelay,
          								tempval);
       else if (axis == Y_AXIS)
          pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_SETOBLGRADY | apdelay,
          								tempval);
       else
          pchanPut((PCHANSORT_ID) pTheFifoObject->pPChanObj,PCHAN_SETOBLGRADX | apdelay,
          								tempval);
   }
   else
       setoblgrad(axis,tempval);
}

/*----------------------------------------------------------------------*/
/* wgload								*/
/* 	Finds waveform buffer and loads waveform generators.		*/
/*----------------------------------------------------------------------*/
#define TERMINATOR 0xa5b6c7d8		/* see wg.c */
void wgload(ACODE_ID pAcodeId)
{
 unsigned short *wf_base,*wf_pntr,apaddr,apbase,wgaddr,nelems;

   wf_base = getWFSet(pAcodeId->id);
   if (wf_base == NULL) 
	return;
   wf_pntr = wf_base;
   while ((wf_pntr != 0L) && (*wf_pntr > 0) && 
					( *(int *)wf_pntr != TERMINATOR) )
   {
	apbase = *wf_pntr++;		/* ap buss base address */
	wgaddr  = *wf_pntr++;		/* wg on board address  */
	nelems  = *wf_pntr++;
	DPRINT3( 1, "wgload: apbase=0x%x wgaddr=0x%x nelems=%d\n",
					 apbase, wgaddr, nelems);
	wf_pntr++;			/* skip bogus word */
	apaddr = apbase | 2;		/* select command register */
	writeapword(apaddr,0x80,STD_APBUS_DELAY);   /* reset each time */
	apaddr = apbase;		/* select start address register */
	writeapword(apaddr,wgaddr,STD_APBUS_DELAY); /* write */

	apaddr = apbase | 1;		/* select data input register */
	/*****************************************************/
	/* nelems must be an unsigned int to work!!            */
	/*****************************************************/
	while (nelems-- > 0)
	{
	   writeapword(apaddr,*(wf_pntr+1),STD_APBUS_DELAY); /* write lsw */
	   writeapword(apaddr,*wf_pntr++,STD_APBUS_DELAY); /* write msw */
	   wf_pntr++;
	   if ((nelems%10) == 0)
   		fifoStuffCmd(pTheFifoObject,CL_DELAY,WGLOAD_DELAY);  /* ~ 25 msec */
	}
   }
}

/*----------------------------------------------------------------------*/
/* getapread								*/
/*	Looks for value in ap readback fifo and if address matches	*/
/*	passed address puts value into rtvar location.			*/
/*	fiforeadword = 10bits address, 8bits data;
/*	If successful, returns 0; otherwise returns -1.			*/
/*	Arguments: 							*/
/*		apaddr 	: address of apbus location.			*/
/*		*rtvar	: address of rtvar location.			*/
/*----------------------------------------------------------------------*/
int getapread(ushort apaddr, long *rtvar)
{
    long readfifo=0, readaddr;
    /* shift off register bits for comparison with readbackword */
    apaddr = apaddr >> 2;
    readaddr = 0;
    while ((readfifo != -1) && (readaddr != apaddr))
    {
	readfifo=fifoApReadBk(pTheFifoObject);
	*rtvar = readfifo & 0xff;
	readaddr = (readfifo >> 8) & 0x3ff;
	DPRINT4(3,
	"getapread: readfifo=0x%x readaddr=0x%x *rtvar=0x%x apaddr=0x%x\n", 
		readfifo,readaddr,*rtvar,apaddr);
    }
    if (readfifo == -1) 
    {
	return(-1);
	*rtvar = -1;
    }
    else
	return(0);
    
}

/*----------------------------------------------------------------------*/
/* writeapword								*/
/*	sends apword to fifo.						*/
/*	Arguments: 							*/
/*		apaddr 	: address of apbus location.			*/
/*		apval	: value to be stored at that location.		*/
/*		apdelay : delay in 12.5 ns ticks to be sent to apbus	*/
/*			  max is about 650 usecs			*/
/*			  min is 500 ns (total delay 100ns is min) 	*/
/*----------------------------------------------------------------------*/
void writeapword(ushort apaddr,ushort apval, ushort apdelay)
{
 ushort *shortfifoword;
 ulong_t fifoword;
 
 /*DPRINT2( DEBUG_APWORDS, "writeapword: 0x%x, 0x%x\n", apaddr, apval ); */
 DPRINT2( 3, "writeapword: 0x%x, 0x%x\n", apaddr, apval );
 shortfifoword = (short *) &fifoword;
 shortfifoword[0] = APWRITE | apaddr;
 shortfifoword[1] = apval;
 fifoStuffCmd(pTheFifoObject,CL_AP_BUS,fifoword);
 /* fifoword = apdelay-HW_DELAY_FUDGE; */
 /* fifoStuffCmd(pTheFifoObject,CL_DELAY,fifoword); */
 fifoStuffCmd(pTheFifoObject,CL_DELAY,apdelay);
}

void
writeapwordStandAlone(ushort apaddr,ushort apval, ushort apdelay)
{
	DPRINT( DEBUG_APWORDS, "write AP word, stand alone starts\n" );
	init_fifo( NULL, 0, 0 );
	writeapword( apaddr, apval, apdelay );
	fifoStuffCmd(pTheFifoObject,HALTOP,0);
	/*fifoHaltop( pTheFifoObject );  GMB says so, 03/13/95 */
	/* taskDelay( 1 ); */
	fifoStart( pTheFifoObject );
	fifoWait4Stop( pTheFifoObject );

	DPRINT( DEBUG_APWORDS, "write AP word, stand alone completes\n" );
}

/* check on presents of sample via the 3 methods available */
multiDetectSample()
{
   int locked,sampdetected,speed,sampinmag;
   sampdetected = autoSampleDetect(pTheAutoObject);
   locked = speed = 0;
   if (!sampdetected)
   {
      taskDelay(60*5);
      locked = autoLockSense(pTheAutoObject);
      speed = (autoSpinValueGet(pTheAutoObject)) / 10;
   }
/*
   locked = autoLockSense(pTheAutoObject);
   speed = (autoSpinValueGet(pTheAutoObject)) / 10;
*/
   /* sampinmag = (locked || speed || sampdetected ); */
   sampinmag = ((locked == 1) || (speed > 0) || (sampdetected == 1) );
   DPRINT2(0,
      "multiDetectSample -   Sample: detected: '%s',  locked: '%s'\n",
      (sampdetected == 1) ? "TRUE" : "FALSE",
      (locked == 1) ? "TRUE" : "FALSE"); 
   DPRINT2(0,
      "multiDetectSample -   Sample: spinning speed(Hz): %d, In Mag: '%s'\n",
      speed, (sampinmag == 1) ? "YES" : "NO" );
   return(sampinmag);
}

getSampFromMagnet(ACODE_ID pAcodeId, long newSample, long *presentSample, int skipsampdetect)
{
   int hostcmd[4];
   int *ptr;
   int roboStat,sampinmag;
   int Status;
   long removedSamp;
   int  lcSample;

   /* remove sample only if new one needed */
   removedSamp = *presentSample; /* save this sample number */


   /* If LC Sample (e.i. gilson sample handler) value then don't test for sample presents in magnet */
   if ( (newSample > 999) || (skipsampdetect != 0))
      lcSample = 1;
   else
      lcSample = 0;

   DPRINT2(0,"getSampFromMagnet: get Sample %lu from Magnet, Skip Sample Detect Flag: %d\n",
		  *presentSample,skipsampdetect);

   
   Status = 0;

   sampleSpinRate = currentStatBlock.stb.AcqSpinSet;

   /* check on presents of sample via the 3 methods available */
   if (lcSample != 1)
   {
      sampinmag = multiDetectSample();
      DPRINT1(0,"getSampFromMagnet -   Sample In Mag: '%s'\n",
      (sampinmag == 1) ? "YES" : "NO" );
   }
      /* Special tests for AS7600 robot */
   if (skipsampdetect == 2)
   {
      if ( *presentSample == 0 )
      {
         /* If unknown sample (0) check for sample present */
         sampinmag = multiDetectSample();
         if ( ! sampinmag )
         {
            *presentSample = -99;
            DPRINT(0,"getSampFromMagnet   Sample changed from 0 to -99\n");
         }
      }
      else if ( *presentSample == -99 )
      {
         /* If declared empty, do the quick check to confirm */
         if (autoSampleDetect(pTheAutoObject))
         {
            *presentSample = 0;
            DPRINT(0,"getSampFromMagnet   Sample changed from -99 to 0\n");
         }
      }
   }


      ptr = hostcmd;
      *ptr++ = CASE;
      *ptr++ = GET_SAMPLE;
      *ptr++ = (*presentSample);
      /* astatus = chgsmp('R',samplein,&(acode->status)); */
      msgQSend(pMsgesToHost,(char*) &hostcmd,(3 * sizeof(int)),
                  WAIT_FOREVER, MSG_PRI_NORMAL);
      DPRINT(0,"==========>  Suspend getSampFromMagnet() \n");
#ifdef INSTRUMENT
      wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
      rngBufGet(pSyncActionArgs,(char*) &roboStat,sizeof(roboStat));
      DPRINT1(0,"==========> Get Sample Complete: robo result: %d, Resume getSampFromMagnet()\n",roboStat);

    if ( ! lcSample)
    {
      /* Its an error if a sample was there and not removed */
      /* A. Start automation no sample, sample Zero,  sample change failure - Continue */
      /* B. Start automation Sample present, sample Zero, sample change failure - Fail. */
      /* C. Sample NOT zero, sample change failure - Fail. */
      /* if (roboStat && ((removedSamp != 0) || sampinmag)) */
      /* If the robo has an error then always assume the sample is in the magnet!!
         This  is 5.1 method an also suggested to be kept by Steve McKenna
         Possible enhance is to change the robot 501 error to 516 error if the sample
         was not redetected after robot failed to get it
      */
      if (roboStat && (removedSamp != 0))
      {
         /* if (samplein != 0 || acode->status == SPNFAIL || flag) */
         /* This test could be done by Roboproc */
         /* if (currentStatBlock.stb.AcqSample != 0 || flag) */
         Status = roboStat;
         sampleHasChanged = FALSE;   
	 /* incase eject is not off, do it now */
	 autoSampleInsert(pTheAutoObject);

         /* check on presents of sample via the 3 methods available */
         sampinmag = multiDetectSample();
         DPRINT1(0,"Failed to Get Sample -   Sample In Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         /* if ( (!locked) && (!sampdetected) ) */
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
         sampinmag = multiDetectSample();
         DPRINT1(0,"getSampFromMagnet -   Sample In Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         /* if (locked || sampdetected) */
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
    else	/* LC Sample */
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

   /* Tdelay(20); */
   /* spinner('B',0,0,PRTON); */  /* Turn on bearing air */
   DPRINT2(0,
         "getSampFromMagnet: Done; Sample Now in Mag: %d, changed: '%s'\n",
               *presentSample, (sampleHasChanged == TRUE) ? "YES" : "NO" );
   return(Status);
}

putSampIntoMagnet(ACODE_ID pAcodeId, long newSample, long *presentSamp,
                  int skipsampdetect, int spinActive, int bumpFlag)
{
   int hostcmd[4];
   int *ptr;
   int roboStat,locked,sampdetected,speed,sampinmag,cnt;
   int Status;
   int  lcSample;

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
     sampinmag = multiDetectSample();
     DPRINT1(0," Put only if Smaple is NOT in Mag.   Sample In Mag: '%s'\n",
        (sampinmag == 1) ? "YES" : "NO" );
   }
   else
     sampinmag = 0;	/* just so the next test is passed */

   /* if ((!locked) && (!sampdetected) && (speed == 0)) */
   if ( !sampinmag )
   { 
      /* astatus = chgsmp('L',mask,&(acode->status)); */
      ptr = hostcmd;
      *ptr++ = CASE;
      *ptr++ = LOAD_SAMPLE;
      *ptr++ = newSample;
      msgQSend(pMsgesToHost,(char*) &hostcmd,(3 * sizeof(int)),
                  WAIT_FOREVER, MSG_PRI_NORMAL);
      DPRINT(0,"==========> Suspend putSampIntoMagnet()\n");
#ifdef INSTRUMENT
      wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
      rngBufGet(pSyncActionArgs,(char*) &roboStat,sizeof(roboStat));
      DPRINT1(0,"==========> Load Sample Complete: result: %d,  Resume putSampIntoMagnet()\n",roboStat);

   if ( !lcSample )
   {
      if (roboStat)
      {      
         sampleHasChanged = FALSE;
         Status = roboStat;
	 /* incase eject is not off, do it now */
	 autoSampleInsert(pTheAutoObject);

         /* check on presents of sample via the 3 methods available */
         sampinmag = multiDetectSample();
         DPRINT1(0,"ROBO Err, Sample is in Mag: '%s'\n",
            (sampinmag == 1) ? "YES" : "NO" );
         /* if ( locked || sampdetected || speed ) */
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
            sampinmag = multiDetectSample();
            DPRINT1(0,"putSampFromMagnet -   Sample In Mag: '%s'\n",
               (sampinmag == 1) ? "YES" : "NO" );
            if ((sampinmag) || (cnt == 2))
               break;
	    if (bumpFlag)
            {  DPRINT(0,"putSampFromMagnet: Bump Air\n");
               autoSampleBump(pTheAutoObject);
	    }
	    else
	       DPRINT(0,"putSampFromMagnet: NOT Bumping Air\n");
         }
         /* if ( locked || sampdetected || speed ) */
         if ( sampinmag )
         {
            DPRINT(0,"==========> Load Sample: Sample Detected\n");
            sampleHasChanged = TRUE; /* successful completion */
            *presentSamp= (short) newSample;
            if (!spinActive)
            {
               setspin( (int) sampleSpinRate, (int) bumpFlag );
            }
         }      
         else   /* Ooops, no sample detected signal insetion error */
         {
            DPRINT(0,"==========> Load Sample: Sample NOT Detected\n");
            sampleHasChanged = FALSE;
            Status = SMPERROR + SMPINFAIL;
            *presentSamp= (short) newSample;	/* be safe and assume its in the magnet */
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
         sampleHasChanged = TRUE; /* successful completion */
         *presentSamp= newSample;
         if ( newSample && (skipsampdetect == 2) )
         {
            int max = 20;
            while ( !autoSampleDetect(pTheAutoObject) && (max > 0) )
            {
               taskDelay(sysClkRateGet()*1);  /* 1 sec */
               if (max == 20)
                  DPRINT(0,"==> Load Sample: Waiting for Sample Detect\n");
               max--;
            }
            DPRINT2(0,"==> Load Sample: detect= %d after %d sec.\n",
                           autoSampleDetect(pTheAutoObject), 20-max);
            if ( !autoSampleDetect(pTheAutoObject) )
            {
/*
                     send2Expproc(CASE,FAILPUT_SAMPLE,newSample, 0, NULL, 0);
                     DPRINT(0,"==========> Suspend failputSampIntoMagnet()\n");
                     msgQReceive(pRoboAckMsgQ, (char*) &roboStat,
                                            sizeof(int), WAIT_FOREVER);
                     DPRINT1(0,"==========> Failput Sample Complete: robo result: %d\n",roboStat);
 */
               ptr = hostcmd;
               *ptr++ = CASE;
               *ptr++ = FAILPUT_SAMPLE;
               *ptr++ = newSample;
               msgQSend(pMsgesToHost,(char*) &hostcmd,(3 * sizeof(int)),
                           WAIT_FOREVER, MSG_PRI_NORMAL);
               DPRINT(0,"==========> Suspend putSampIntoMagnet()\n");
               semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
               rngBufGet(pSyncActionArgs,(char*) &roboStat,sizeof(roboStat));
                     sampleHasChanged = FALSE;
                     *presentSamp= -99;
                     Status = SMPERROR + SMPINFAIL;
                     errLogRet(LOGIT,debugInfo,
                           "failputSampIntoMagnet: Robo Error: status is %d",roboStat);
            }
         }
      }
    }
   }
   else
   {
      DPRINT(0,"putSampIntoMagnet: detected sample still present\n");
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

static queueSamp(int action, unsigned long sample2ChgTo, int skipsampdetect, ACODE_ID pAcodeId, short bumpFlag)
{
   DPRINT1(0,"Queue SAMP: Action: %d, loc: %lu \n",sample2ChgTo);
   rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
   rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
   rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
   rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(bumpFlag));
   signal_syncop(action,-1L,-1L);  /* action: GETASMP or LOADSAMP, signal & stop fifo */
   fifoStart(pTheFifoObject);
   DPRINT(0,"==========>  Suspend Parser\n");

#ifdef INSTRUMENT
   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
/*
   Note: that putSampFromMagnet() also take pSemParseSuspend, to wait for the roboproc to report
      status. Thus shandler that calls putSampFromMagnet() is also pended on this semaphore.
      The semaphore was changed from FIFO to PRIORITY type semaphore so that shandler is released
      then the parser rather than the other way around.   3/11/96
*/
   semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
}

static queueLoadSamp(int action, unsigned long sample2ChgTo, int skipsampdetect,
                     ACODE_ID pAcodeId, short spinActive, short bumpFlag)
{
   DPRINT1(0,"Queue SAMP: Action: %d, loc: %lu \n",sample2ChgTo);
   rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
   rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
   rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
   rngBufPut(pSyncActionArgs,(char*) &spinActive,sizeof(spinActive));
   rngBufPut(pSyncActionArgs,(char*) &bumpFlag,sizeof(bumpFlag));
   signal_syncop(action,-1L,-1L);  /* action: LOADSAMP, signal & stop fifo */
   fifoStart(pTheFifoObject);
   DPRINT(0,"==========>  Suspend Parser\n");

#ifdef INSTRUMENT
   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
/*
   Note: that putSampFromMagnet() also take pSemParseSuspend, to wait for the roboproc to report
      status. Thus shandler that calls putSampFromMagnet() is also pended on this semaphore.
      The semaphore was changed from FIFO to PRIORITY type semaphore so that shandler is released
      then the parser rather than the other way around.   3/11/96
*/
   semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
}

void autoShim(ACODE_ID pAcodeId,short ntindx,short npindx,short dpfindx,
    short maxpwr,short maxgain,short ac_offset,short *methodstr, short freeMethodStrFlag)
{
   int fifoStartFlag;
   int sshaIsActivated;	/* distinguish this automatic from static 'ssha_activated' */

   DPRINT1(2,">>>  Acode Offset: %d \n",ac_offset);
	

   DPRINT(0,"Would call autoshim .....\n");

   sshaIsActivated = is_ssha_activated();
   DPRINT1( -1, "SHIMAUTO with ssha is activated: %d\n", sshaIsActivated );
   deactivate_ssha();
   if (has_ssha_started())
   {
       reset_ssha();
   }

   fifoStartFlag = fifoGetStart4Exp(pTheFifoObject);
   DPRINT1(1,"SHIMAUTO: FIFO get start flag: %d\n", fifoStartFlag);

   /* put address of method string into buffer */
   rngBufPut(pSyncActionArgs,(char*) &methodstr,sizeof(methodstr));
   rngBufPut(pSyncActionArgs,(char*) &ntindx,sizeof(ntindx));
   rngBufPut(pSyncActionArgs,(char*) &npindx,sizeof(npindx));
   rngBufPut(pSyncActionArgs,(char*) &dpfindx,sizeof(dpfindx));
   rngBufPut(pSyncActionArgs,(char*) &ac_offset,sizeof(ac_offset));
   rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
   rngBufPut(pSyncActionArgs,(char*) &maxpwr,sizeof(maxpwr));
   rngBufPut(pSyncActionArgs,(char*) &maxgain,sizeof(maxgain));
   rngBufPut(pSyncActionArgs,(char*) &freeMethodStrFlag,sizeof(freeMethodStrFlag));
   signal_syncop(SHIMAUTO,-1L,-1L);  /* signal & stop fifo */
   fifoStart(pTheFifoObject);
   DPRINT(0,"==========> SHIMAUTO: Suspend Parser\n");
#ifdef INSTRUMENT
	   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
   semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
   DPRINT(0,"==========> SHIMAUTO: Resume Parser\n");
   fifoSetStart4Exp(pTheFifoObject, fifoStartFlag );

   DPRINT1( -1, "SHIMAUTO with ssha is activated at point 2: %d\n", sshaIsActivated );
   if (sshaIsActivated)
   {
       activate_ssha();
   }

}

void autoGain(ACODE_ID pAcodeId,short ntindx,short npindx,short rcvgindx,
     short apaddr,short apdly,short maxval,short minval,short stepval,
     short dpfindx,short adcbits,short ac_offset)
{

      DPRINT6(2,">>>  apaddr: 0x%x, delay: %d, gain max: %d, min %d, step %d, adcbits: %d \n",
 	apaddr,apdly,maxval,minval,stepval,adcbits);

      DPRINT3(2,"Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);
      DPRINT1(2,">>>  Acode Offset to FID Acquire: %d \n",ac_offset);

      rngBufPut(pSyncActionArgs,(char*) &ntindx,sizeof(ntindx));
      rngBufPut(pSyncActionArgs,(char*) &npindx,sizeof(npindx));
      rngBufPut(pSyncActionArgs,(char*) &dpfindx,sizeof(dpfindx));
      rngBufPut(pSyncActionArgs,(char*) &apaddr,sizeof(apaddr));
      rngBufPut(pSyncActionArgs,(char*) &apdly,sizeof(apdly));
      rngBufPut(pSyncActionArgs,(char*) &rcvgindx,sizeof(rcvgindx));
      rngBufPut(pSyncActionArgs,(char*) &maxval,sizeof(maxval));
      rngBufPut(pSyncActionArgs,(char*) &minval,sizeof(minval));
      rngBufPut(pSyncActionArgs,(char*) &stepval,sizeof(stepval));
      rngBufPut(pSyncActionArgs,(char*) &adcbits,sizeof(adcbits));

      /* now use the FIDCODE to mark proper acode offset to acquire FID */
      rngBufPut(pSyncActionArgs,(char*) &ac_offset,sizeof(ac_offset));

      rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
      signal_syncop(AUTOGAIN,-1L,-1L);  /* signal & stop fifo */
      fifoStart(pTheFifoObject);
      DPRINT(0,"==========> AUTOGAIN: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
      DPRINT(0,"==========> AUTOGAIN: Resuming Parser\n");
}

#ifdef TEST_MSR_APBUS_INTRP
genreset()
{
   fifoReset(pTheFifoObject, RESETFIFOBRD);
}

gencd(int dac, int value)
{
   int cnt,i;
   unsigned long Codes[250],src;
   cnt = autoGenShimApCodes(pTheAutoObject,dac,value,Codes);
   printf("codes: %d\n",cnt);
   fifoStuffIt(pTheFifoObject, Codes, cnt/2);
   taskDelay(10);
}

gencode(int dac, int value, int time)
{
   int ap1,ap2;
   ap1  = ((((short) dac) << 8) & 0x3f00);
   ap1 |= ((((short)value) >> 8) & 0xff);
   ap1 |= 0x8000;  /* mark as dac number & hi nibble value */
   ap2 = (((short)value) & 0xff);
   ap2 |= 0xC000;
   printf("1: 0x%x, 2: 0x%x\n",ap1,ap2);
   genSi(ap1);
   itdelay(time);
   genSi(ap2);
   itdelay(time);
}
genSi(int value)
{
   int cnt,i;
   unsigned long Codes[250],src;
   DPRINT2(-1,"Value: %d (0x%x)\n",value,value);
   cnt = autoGenShimMBoxCodes(pTheAutoObject, value , Codes);
   fifoStuffIt(pTheFifoObject, Codes, cnt/2);
   taskDelay(10);
}
itdelay(int usec)
{
   unsigned long ticks;
   /* ticks = 80000L * msec;   /* N msec */
   ticks = 80L * usec;   /* N usec */
   /* fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks - HW_DELAY_FUDGE); */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks);
   taskDelay(10);
}
itstrt()
{
   fifoStuffCmd(pTheFifoObject,HALTOP,0);
   taskDelay(10);
   fifoStart(pTheFifoObject);
   taskDelay(10);
   fifoWait4Stop(pTheFifoObject);
}

#endif /* TEST_MSR_APBUS_INTRP */
