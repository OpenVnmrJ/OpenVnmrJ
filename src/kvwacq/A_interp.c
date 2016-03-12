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
#include "timeconst.h"
#include "acodes.h" 
#include "stmObj.h"	/* different for SUN and vxworks */
#include "fifoObj.h"	/* different for SUN and vxworks */
#include "adcObj.h"	/* different for SUN and vxworks */
#include "autoObj.h"
#include "sibObj.h"
#include "spinObj.h"
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


extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pUpLinkIMsgQ;	/* MsgQ used between UpLinkerI and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;

extern FIFO_ID		pTheFifoObject;
extern STMOBJ_ID	pTheStmObject;
extern ADC_ID		pTheAdcObject;
extern AUTO_ID		pTheAutoObject;
extern TUNE_ID		pTheTuneObject;
extern SIB_ID		pTheSibObject;
extern SPIN_ID		pTheSpinObject;

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

/* counter to keep track of ss done (through fifo, not parser), */
/* used to enable or disable ADC overflow interrupt */
/* see adcOvldClear() in adcObj.c, and fifoTagFifo() in fifoObj.c */
extern int     sscnt_adcitrp;
/* rgain straightens out the lock and obs receiver gain, see lock_interface.c */
extern  ushort  rgain[];

/* STM scans - Should go into acode object? */
/* unsigned long cur_scan_data_adr; */
unsigned long prev_scan_data_adr;

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

extern STMOBJ_ID pTheStmObject;
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
   char *elem_addr, *jtbl_element_addr(TBL_ID *,short,short,long *);
   char tmpbufname[64];
   void japbcout(ushort,ushort,long *,short);
   void signal_syncop(int tagword,long secs,long ticks);
   void jdelay(int, int, int);

   adjLkGainforShim = 0;
   status = 0;
   ac_base = pAcodeId->cur_acode_base;
   rt_base = (long *) pAcodeId->cur_rtvar_base;
   rt_tbl=rt_base+1;
   

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
      DPRINT3( 1, "acode[%ld]: %d, length: %d\n", (ac_cur-ac_start),*acode, alength );
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
	/*  0 - short word count: 3					*/
	/*  1 - rtoffset to number of data points			*/
	/*  2 - rtoffset to precision, double or single (in bytes)	*/
	/*  3 - rtoffset to total number of data elements including	*/
	/*		blocksize elements.				*/
	/*  4 - Flag indicating to use High Speed STM/ADC (5MHz) 	*/
	case INIT_STM: 
           {
   	      int rstat;
              STMOBJ_ID stmGetStdStmObj();
              STMOBJ_ID pAdmId;
	      DPRINT(1,"INIT_STM\n");
              DPRINT4(1,"arg1: %lu, arg2: %lu, arg3: %lu, arg4: %d\n",
			(ulong_t)(rt_tbl[*ac_cur]),(ulong_t)(rt_tbl[*(ac_cur+1)]),
			(ulong_t)(rt_tbl[*(ac_cur+2)]),*(ac_cur+3));
	      if (*(ac_cur+3) != 0)
	      {
                 if ((rstat = stmGetHsStmObj(&pAdmId)) != 0)
                 {
	           APint_run = FALSE;
                    /* rstat = HDWAREERROR+NOHSDTMADC; */
	           status = rstat;
    	           errLogRet(LOGIT,debugInfo,
			   "High Speed DTM/ADC Not Present: errorcode is %d",rstat);
                 }
	         else
                 {
		   pTheStmObject = pAdmId;
                 }
              }
	      else
              {
	         pTheStmObject = stmGetStdStmObj();	/* Use the Standard 1st STM Board */
	      }
	      init_stm(pAcodeId, pTheStmObject, (ulong_t)(rt_tbl[*ac_cur]),
		  (ulong_t)(rt_tbl[*(ac_cur+1)]), (ulong_t)(rt_tbl[*(ac_cur+2)]) );
	      ac_cur+=4;
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
	/*  0 - short word count: 2					*/
	/*  1 - flags: 							*/
	/*  2 - spare							*/
	case INIT_ADC:
           {
              STMOBJ_ID pAdmId;
	      /* init_adc(unsigned short flags, unsigned long *adccntrl) */
	      init_adc(pTheAdcObject,*(ac_cur),&rt_tbl[*(ac_cur+1)],rt_tbl[*(ac_cur+2)]);
	      ac_cur+=3;
           }
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
	      alength = 1;
      	      update_acqstate((pAcodeId->interactive_flag == ACQI_INTERACTIVE) ?
						ACQ_INTERACTIVE : ACQ_PAD);
/*              while (len-- > 0)
/*              {
/*                 if (*ac_cur++ == EVENT1_DELAY)
/*                 {
/*	            delay( 0, getuint(ac_cur));
/*                    len -= 2;
/*	            ac_cur += 2;
/*                 }
/*                 else
/*                 {
/*	            delay( getuint(ac_cur), (unsigned long)getuint(ac_cur+2));
/*                    len -= 4;
/*	            ac_cur += 4;
/*                 }
/*              } */
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
	   DPRINT(2,"MASKHSL_R \n");
	   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, rt_tbl[*ac_cur++]);
	   break;

	case UNMASKHSL_R:
	   DPRINT(2,"UNMASKHSL_R \n");
	   fifoUnMaskHsl(pTheFifoObject,STD_HS_LINES, rt_tbl[*ac_cur++]);
	   break;

        case EVENTN:
           { int i,loopcount,wordcount;
              loopcount = *ac_cur >> 6;
              wordcount = *ac_cur & 0x3f;
              ac_cur++;
              if (loopcount > 0)
              {  fifoStuffCmd(pTheFifoObject,CL_LOOP_COUNT,loopcount);
                 fifoBeginHardLoop(pTheFifoObject);
                 fifoStuffCmd(pTheFifoObject,NULL,0x7FFF);
                 fifoStuffCmd(pTheFifoObject,NULL,0x7FFF);
                 fifoStuffCmd(pTheFifoObject,NULL,0x7FFF);
                 fifoEndHardLoop(pTheFifoObject);
              }
              for (i=0; i<wordcount; i++)
                 fifoStuffCmd(pTheFifoObject,NULL,*ac_cur++);
           }
           break;

/*	/* EVENT1_DELAY							*/
/*	/*  arguments: 							*/
/*	/*  0 - short word count: 2					*/
/*	/*  1 - #ticks with 10ns/tick (2**31 ??)	(int)		*/
/*	case EVENT1_DELAY:
/*	   DPRINT(2,"EVENT1_DELAY... \n");
/*	   delay( 0, getuint(ac_cur));
/*	   ac_cur += 2;
/*	   break;
/*
/*	/* EVENT2_DELAY							*/
/*	/*  arguments: 							*/
/*	/*  0 - short word count: 4					*/
/*	/*  1 - #seconds 	(int)					*/
/*	/*  2 - remainder of a second in 10 ns ticks 	(int)		*/
/*	case EVENT2_DELAY:
/*	   DPRINT(2,"EVENT2_DELAY... \n");
/*	   delay( getuint(ac_cur), (unsigned long)getuint(ac_cur+2));
/*	   ac_cur += 4;
/*	   break;
 */
	/* DELAY							*/
	/*  arguments: 							*/
	/*  0 - short word count: 1					*/
	/*  1 - rtoffset to pointer to delay in table 			*/
	case DELAY: 
	   DPRINT(2,"DELAY... \n");
	   rtv_ptr = (unsigned long *)rt_tbl[*ac_cur++];
	   jdelay( (int) *rtv_ptr, (int) *(rtv_ptr+1), (int) *(rtv_ptr+2) );
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

/* 	case SET_LKFAST:
/* 		setLockPar(SET_LK2KCF,0,FALSE);
/* 		break;
/* 	case SET_LKSLOW:
/* 		setLockPar(SET_LK2KCS,0,FALSE);
/* 		break;
/* */
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

	case SETLKDECATTN:		/* Lock/Decoupler board power/attn */
        {
          int apaddr,decpower,apdelay;

          apaddr = *ac_cur++;
          decpower = *ac_cur++;
          apdelay = *ac_cur++;

	  DPRINT3(2,"SETLKDECATTN: apaddr: 0x%x, value: %d, apdelay: %d\n",
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
	  DPRINT3(2,"SETLKDEC_ONOFF: apaddr: 0x%x, value: %d, apdelay: %d\n",
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
	   tbl_addr[IPHASESTEP] = *(long *)ac_cur;
	   ac_cur += 2;
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
	   if (setphase(pAcodeId,*ac_cur, *(ac_cur+1)) != 0) {
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
	   if (setphase(pAcodeId,*ac_cur, rt_tbl[*(ac_cur+1)]) != 0) {
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
              int ret,lsw,hsw,skipsampdetect;
              unsigned long sample2ChgTo;
	      /* the sample number is now 2 shorts and encodes Gilson positional & type information that
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
	      skipsampdetect = *ac_cur++;
              DPRINT2(0,"GETSAMP: lsw: %u, hsw: %u \n",lsw,hsw);
              sample2ChgTo = ((hsw << 16) & 0xffff0000) | (lsw & 0xffff);
   	      if ( (sample2ChgTo != currentStatBlock.stb.AcqSample) && 
		   ( ((currentStatBlock.stb.AcqSample % 1000) != 0) ||
                     (skipsampdetect == 0) || (skipsampdetect == 2) ))
   	      {
	        rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
	        rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
	        rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	        signal_syncop(GETSAMP,-1L,-1L);  /* signal & stop fifo */
                DPRINT1(0,"GETSAMP: loc: %lu \n",sample2ChgTo);
	        fifoStart(pTheFifoObject);
	        DPRINT(0,"==========> GETSAMP: Suspend Parser\n");

#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
/*
	Note: that getSampFromMagnet() also take pSemParseSuspend, to wait for the roboproc to report
              status. Thus shandler that calls getSampFromMagnet() is also pended on this semaphore.
              The semaphore was changed from FIFO to PRIORITY type semaphore so that shandler is released
              then the parser rather than the other way around.   3/11/96
*/
	        semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	        DPRINT(0,"==========> GETSAMP: Resume Parser\n");
	      }
	      else
	        DPRINT2(0,"sample2ChgTo (%d) == PresentSample (%d), GETSAMP Skipped\n",
				sample2ChgTo,currentStatBlock.stb.AcqSample);
          }
		break;

	case LOADSAMP:
          {
              short spinActive;
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

	      /* Aaah you say why don't you just get it as a long?
                 Because psg puts the codes in as shorts and a long 
                 might not be aligned on the proper boundard (BUSS ERROR),
                 PSG has the same problem placing a long into the acode
                 stream can lead to a bus error because of misalignment
		 of the long word (Ok Frits)
              */
	      lsw = *ac_cur++;
	      hsw = *ac_cur++;
	      skipsampdetect = *ac_cur++;
	      spinActive = *ac_cur++;
              DPRINT2(0,"GETSAMP: lsw: %u, hsw: %u \n",lsw,hsw);
              sample2ChgTo = ((hsw << 16) & 0xffff0000) | (lsw & 0xffff);

              DPRINT1(0,"LOADSAMP: loc: %lu \n",sample2ChgTo);
   	      if ( ((sample2ChgTo % 1000) != 0) )
   	      {
	        rngBufPut(pSyncActionArgs,(char*) &sample2ChgTo,sizeof(long));
	        rngBufPut(pSyncActionArgs,(char*) &skipsampdetect,sizeof(int));
	        rngBufPut(pSyncActionArgs,(char*) &pAcodeId,sizeof(pAcodeId));
	        rngBufPut(pSyncActionArgs,(char*) &spinActive,sizeof(short));
	        signal_syncop(LOADSAMP,-1L,-1L);  /* signal & stop fifo */
	        fifoStart(pTheFifoObject);
	        DPRINT(0,"==========>LOADSAMP:  Suspend Parser\n");

#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
/*
	Note: that putSampFromMagnet() also take pSemParseSuspend, 
        to wait for the roboproc to report status. Thus shandler 
        that calls putSampFromMagnet() is also pended on this semaphore.
        The semaphore was changed from FIFO to PRIORITY type semaphore 
        so that shandler is released then the parser rather than the
        other way around.   3/11/96
*/
	        semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
	        DPRINT(0,"==========> LOADSAMP: Resume Parser\n");
	      }
	      else
	        DPRINT2(0,"sample2ChgTo (%d), loc (%d) == 0, LOADSAMP Skipped\n",
				sample2ChgTo, (sample2ChgTo % 1000));
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

        /* JTAPBCOUT                                                    */
        /*  arguments:                                                  */
        /*  0 - short word count: 6 + (nindices - 1)                    */
        /* arg1: apbus delay to insert  */
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
           if (*(ac_cur+2)  >  1)
           {
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
           DPRINT4(1,"JTAPBCOUT: apdelay: %d, apaddr: 0x%lx, dataaddr: 0x%lx, cn
t: %d\n",
                *ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
           japbcout(*ac_cur,*(ac_cur+5+i),(long *)elem_addr,*(ac_cur+4+i));
           ac_cur=ac_cur+6+i;
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

	/* SAFETYCHECK							*/
	/*  arguments: 							*/
	/*  0 - short word count: 0					*/
	case SAFETYCHECK:
	   DPRINT(2,"SAFETYCHECK ......");
	   sibStatusCheck(pTheSibObject);
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

	/* BRANCH							*/
	/*  arguments: 							*/
	/*  0 - short word count: 2					*/
	/*  1 - code offset (int)					*/
	case BRANCH:
	   DPRINT(2,"BRANCH \n");
	   ac_cur = ac_cur+getuint(ac_cur);
	   acode = ac_cur - alength - 2;
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
	   if ( rt_tbl[a] % 2 != rt_tbl[b] ) {
		ac_cur = ac_base+getuint(ac_cur);
		acode = ac_cur - alength - 2;
	   }
	   else {
		ac_cur = ac_cur + 2;
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
			rt_tbl[c] = rt_tbl[a] / rt_tbl[b];
			break;
		case MOD:
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
	   DPRINT(2,"TASSIGN... \n");
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

	case CLEARSCANDATA:
	   DPRINT(2,"CLEARSCANDATA .....\n");
	   /*clearscandata(int *dtmcntrl) */
	   clearscandata((int*) &rt_tbl[*ac_cur++]);
	   break;

	case RECEIVERCYCLE:
	   DPRINT(2,"RECEIVERCYCLE .....\n");
	   /*receivercycle(int oph, int *dtmcntrl) */
	   receivercycle(rt_tbl[*ac_cur], (int*) &rt_tbl[*(ac_cur+1)]);
	   ac_cur+=2;
	   break;

	case ENABLEOVRLDERR:
	   DPRINT(2,"ENABLEOVRLDERR .....\n");
	   /* enableOvrLd(int ssct, int ct, ulong_t *adccntrl) */
	   enableOvrLd(rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)],&rt_tbl[*(ac_cur+2)]);
	   ac_cur+=3;
	   break;

	case DISABLEOVRLDERR:
	   DPRINT(2,"DISABLEOVRLDERR .....\n");
	   /* disableOvrLd(ulong_t *adccntrl) */
	   disableOvrLd(&rt_tbl[*ac_cur]);
	   ac_cur+=1;
	   break;

	case ENABLEHSSTMOVRLDERR:
	   DPRINT(2,"ENABLEHSSTMOVRLDERR .....\n");
	   /* void enableStmAdcOvrLd(int ssct, int ct, int *dtmcntrl) */
	   enableStmAdcOvrLd(rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)],(int*) &rt_tbl[*(ac_cur+2)]);
	   ac_cur+=3;
	   break;

	case DISABLEHSSTMOVRLDERR:
	   DPRINT(2,"DISABLEHSSTMOVRLDERR .....\n");
	   /* void disableStmAdcOvrLd(int *dtmcntrl) */
	   disableStmAdcOvrLd((int*) &rt_tbl[*ac_cur]);
	   ac_cur+=1;
	   break;


	case SET_GR_RELAY:
           {
              short relay_pos;

              relay_pos = *ac_cur++;
              DPRINT1(1,"SET_GR_RELAY: pos: %d\n", relay_pos);

	      rngBufPut(pSyncActionArgs,(char*) &relay_pos,sizeof(short));
	      signal_syncop(SET_GR_RELAY,-1L,-1L);  /* signal & stop fifo */

/*	      fifoStart(pTheFifoObject);
/*	      DPRINT(0,"==========> SET_GR_RELAY: Suspend Parser\n");
/*	      semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
/*	      DPRINT(0,"==========> SET_GR_RELAY: Resume Parser\n");
/* */
	   }
	   break;

	case OBSLOCK:
	   adcItrpEnable(pTheAdcObject, 0x01);
	   break;

	case INITSCAN:
	   DPRINT(2,"INITSCAN .....\n");
	   /*initscan(int np, int ss, int *ssct, int nt, int *ct, int bs, 
							int cbs, int maxsum) */
	   initscan(pAcodeId,rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], 
		(int*) &rt_tbl[*(ac_cur+2)], rt_tbl[*(ac_cur+3)],
		rt_tbl[*(ac_cur+4)], rt_tbl[*(ac_cur+5)],
		rt_tbl[*(ac_cur+6)],rt_tbl[*(ac_cur+7)]);
	   ac_cur+=8;
	   break;

	case NEXTSCAN:
	   DPRINT(2,"NEXTSCAN .....\n");
	   /* nextscan(int np, int dp, int nt, int ct, int bs, 		*/
	   /* 			int *dtmcntrl, int fidnum) 	*/
	   /* Start Fifo watchdog to start fifo if stmAllocAcqBlk blocks */
	   /* within nextscan. 						 */
	   /* acodeStartFifoWD(pAcodeId,pTheFifoObject); */

	   { short np,dp,nt;
                np = rt_tbl[*ac_cur];
		dp = rt_tbl[*(ac_cur+1)];
		nt = rt_tbl[*(ac_cur+2)];
	   }
	   nextscan(pAcodeId,rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], 
		rt_tbl[*(ac_cur+2)], rt_tbl[*(ac_cur+3)],
		rt_tbl[*(ac_cur+4)], (int*) &rt_tbl[*(ac_cur+5)],
		rt_tbl[*(ac_cur+6)]);
	   /* acodeCancelFifoWD(pAcodeId); */
	   ac_cur+=7;
	   break;

	case ENDOFSCAN:
	   DPRINT(2,"ENDOFSCAN .....\n");
	   /* endofscan( ss, *ssct, nt, *ct, bs, *cbs) */
	   endofscan(rt_tbl[*ac_cur], (int*) &rt_tbl[*(ac_cur+1)],rt_tbl[*(ac_cur+2)],
	    (int*) &rt_tbl[*(ac_cur+3)], rt_tbl[*(ac_cur+4)], (int*) &rt_tbl[*(ac_cur+5)]);
	   ac_cur += 6;
	   break;

	case SET_DATA_OFFSETS:
	   DPRINT(2,"SET_DATA_OFFSETS .....\n");
	   /*setscanvalues(int srcoffset,int destoffset,int np,int dtmcntrl) */
	   setscanvalues(pAcodeId,rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)], 0,
							rt_tbl[*(ac_cur+2)]);
	   ac_cur+=3;
	   break;

	case SET_NUM_POINTS:
	   DPRINT(2,"SET_NUM_POINTS .....\n");
	   /*setscanvalues(int srcoffset,int destoffset,int np,int dtmcntrl) */
	   setscanvalues(pAcodeId, 0, 0, rt_tbl[*ac_cur], rt_tbl[*(ac_cur+1)]);
	   ac_cur+=2;
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
		      fifoStart4Exp(pTheFifoObject);
	           }
/*
		   DPRINT(-1,"Done Parsing Start FIFO\n");
		   fifoStart(pTheFifoObject);
*/
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

	case SAVELOCKPH:
           {  
              short  amptype[2];/* sysNvRamGet append a NULL so we need 2 */
              /* first save the lock phase value */
              currentStatBlock.stb.AcqLockPhase = *ac_cur++;
              /* get value for amptype from NvRam */
              /* compare, if different set; else skip */
              sysNvRamGet(amptype, 2, (PTS_OFFSET + 2)*2 + 0x500);
	      if (amptype[0] != *ac_cur)
	  {
		DPRINT1(3,"changing to %d\n",*ac_cur);
                 sysNvRamSet(ac_cur, 2, (PTS_OFFSET + 2)*2 + 0x500);
	  }
                 ac_cur++;
           }
	   break;

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
	case LOCKPHASE_R:
	   setLockPar( *acode, (int) rt_tbl[ *ac_cur++ ], FALSE );
	   break;

	case LOCKPOWER_R:
	   setLockPower( (int) rt_tbl[ *ac_cur++ ], FALSE );
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

	case LOCKMODE_I:
	   DPRINT1(2,"LOCKMODE_I: %d\n", *ac_cur);
	   setLockPar(LOCKMODE_I, *ac_cur++, FALSE);
	   break;

	case LOCKAUTO:
           if ( (int) rt_tbl[ *ac_cur ] == 0)
           {
              int ret;
              short lockmode,lpwrmax, lgainmax;

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

	case SHIMAUTO:
           {
              short ctval,whenshim,shimmode,len;
	      short ac_offset, ntindx, npindx, dpfindx;
              int bytelen;
              short  maxpwr,maxgain;
	      short *methodstr;


              ctval    = *ac_cur++;
              whenshim = *ac_cur++;
              shimmode = *ac_cur++;
              len      = *ac_cur++;

	      ac_offset = *ac_cur++;
	      DPRINT1(2,">>>  Acode Offset: %d \n",ac_offset);
	      ac_offset--;	/* back off one since APint() adds one */
	      ntindx = *ac_cur++;
	      npindx = *ac_cur++;
	      dpfindx = *ac_cur++;
	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	      DPRINT2(2,">>>  dpf (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx]);
              maxpwr  = *ac_cur++;
              maxgain = *ac_cur++;
	
              if ( ( shimmode & 1 ) && 
	             chkshim( (int) whenshim, (int) rt_tbl[ ctval ], 
				&sampleHasChanged ) 
                 )
              {
                 int ret;
	         int fifoStartFlag;

	         DPRINT(0,"Would call autoshim .....\n");
	         /* 1st place the pointer to arguments that do_autoshim
		 /*  will need in the pSyncActionArgs ring buffer.
		 /* do_autoshim((int *) ac_cur)
		 /* The Tag interrupt, will result in the shandler calling
		 /* do_autoshim()
	         */
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
		     break;
		 }
		 memcpy((char*) methodstr,(char*) ac_cur, bytelen);

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
	         signal_syncop(SHIMAUTO,-1L,-1L);  /* signal & stop fifo */
		 fifoStart(pTheFifoObject);
		 DPRINT(0,"==========> SHIMAUTO: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
		 semTake(pAcodeId->pSemParseSuspend,WAIT_FOREVER);
		 DPRINT(0,"==========> SHIMAUTO: Resume Parser\n");
	         fifoSetStart4Exp(pTheFifoObject, fifoStartFlag );

#ifdef XXX
	         if ( (ret = do_autoshim((int *)ac_cur,maxpwr,maxgain)) != 0 )
                 {
	             DPRINT(1,"autoshim failed\n");
		     APint_run = FALSE;
		     status = ret;
    		     errLogRet(LOGIT,debugInfo,
				"AutoShim Error: status is %d",ret);
                 }
#endif
              }
	      ac_cur += len;
           }
	   break;

	case AUTOGAIN:
           {
              short ntindx,npindx,rcvgindx,apaddr,maxval,apdly,minval,stepval;
	      short dpfindx,adcbits;
	      short ac_offset;
              int bytelen;

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

	      DPRINT4(2,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
		ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
	      DPRINT4(2,">>>  dpf (rt_tbl[%d]) = %d, recvgain (rt_tbl[%d]) = %d\n",
		dpfindx,rt_tbl[dpfindx],rcvgindx,rt_tbl[rcvgindx]);
	      DPRINT6(2,">>>  apaddr: 0x%x, delay: %d, gain max: %d, min %d, step %d, adcbits: %d \n",
  		apaddr,apdly,maxval,minval,stepval,adcbits);

	      DPRINT2(2,"Current Acode location: 0x%lx, Acode = %d\n",
		ac_cur,*ac_cur);
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
	/*
              ac_cur--; 
	      rngBufPut(pSyncActionArgs,(char*) &ac_cur,sizeof(ac_cur));
	*/
	      /* now use the FIDCODE to mark proper acode offset to acquire FID */
	      rngBufPut(pSyncActionArgs,(char*) &ac_offset,sizeof(ac_offset));
/*
              ac_cur++;
*/
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

	      setVTtype((int) vttype);

	      if (vttype == 0)
	         break;

	      setVTinterLk( ((Tmpintlk != 0) ? 1 : 0) );  /* set in='y' or in='n' */
	      setVTErrMode(Tmpintlk);  /* set error mode, warning(14) Error(15) */

	      /* 1st place the arguments that setVT will need in the pSyncActionArgs
		 ring buffer.
		 setVT(temp,ltmpxoff,pid)
		 The Tag interrupt, will result the shandler calling setVT()
	      */
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

	      /* 1st place the arguments that setspinnreg will need 
		 in the pSyncActionArgs ring buffer.
		 setspinnreg(speed,MasThreshold)
		 The Tag interrupt, will result the shandler calling 
		 setspinnreg()  (spinObj.c)
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
              ac_cur += 4;
	      DPRINT(0,"==========> SYNC_PARSER: Suspend Parser\n");
#ifdef INSTRUMENT
		   wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif

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

	default: 
	   errLogRet(LOGIT,debugInfo,
		"NO SUCH CODE %d, length: %d\n",*(acode),alength);
	   if (alength > 0) {
		for (i=0; i<alength; i++)
		{
		   DPRINT2(0," i: %d   code: 0x%x\n",i,*ac_cur);
		   ac_cur++;
		}
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
	fifoReset(pTheFifoObject,  RESETSTATEMACH | RESETFIFO |
				RESETTAGFIFO | RESETAPRDBKFIFO);
	/* Note: SW2_I will be enabled if stop acquisition requested */
	fifoItrpEnable(pTheFifoObject,
                         FSTOPPED_I      |              /* 0x20 */
                      /* FSTRTEMPTY_I    |              /* 0x10 */
                      /* FSTRTHALT_I     |              /* 0x10 */
                      /* NETBL_I         | */           /* 0x10 */
                         FORP_I          |              /* 0x00 */
                         PFAMFULL_I      |              /* 0x80 */
                         TAGFNOTEMPTY_I  |              /* 0x04 */
                         SW1_I           |              /* 0x00 */
                         SW3_I           |              /* 0x00 */
                         SW4_I                          /* 0x00 */
                         );
	autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS);
   }

   return(0);
}

/*----------------------------------------------------------------------*/
/* init_fifo								*/
/* 	Initializes the stm object. Allocates max buffer size and 	*/
/*	total number of desired buffers					*/
/*	Arguments:							*/
/*		numpoints	: number of data points			*/
/*		numbytes	: number of bytes per data point	*/
/*		tot_elem	: number of total data elements		*/
/*				  including blocksizes			*/
/*----------------------------------------------------------------------*/
void init_stm(ACODE_ID pAcodeId, STMOBJ_ID pStmId, ulong_t numpoints, 
			ulong_t numbytes, ulong_t tot_elem)
{
   ulong_t loc_npsize;

   /* Clear High Speed STM ADC for next fid. */
/*   stmAdcOvldClear(pStmId); */

/* ADM_ADCOVFL_MASK is unused in standard STM so it's OK to set all the time */
/* for MERCURY we use the pTheFifoObject, fifo and stm are on one board */

   stmItrpEnable(pTheFifoObject, (RTZ_ITRP_MASK | RPNZ_ITRP_MASK | 
		  MAX_SUM_ITRP_MASK | APBUS_ITRP_MASK | ADM_ADCOVFL_MASK));
   loc_npsize = numpoints*numbytes;
   if (pAcodeId->interactive_flag == ACQI_INTERACTIVE)
	stmInitial(pStmId, tot_elem, loc_npsize, pUpLinkIMsgQ, 0);
   else
	stmInitial(pStmId, tot_elem, loc_npsize, pUpLinkMsgQ, 0);
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
void init_adc(ADC_ID pAdcId,unsigned short flags, unsigned long *adccntrl, int ss)
{
 ushort_t adcistat;
   
   sscnt_adcitrp = ss;
   pTheAdcObject->adcCntrlReg = *adccntrl;
   /* Clear ADC and Receiver Overload Flag, Enable interrupts */
   adcOvldClear(pAdcId);

   DPRINT1(1,"init_adc:  adccntrl: 0x%lx\n",*adccntrl);

   /* read interrupt register and clear any lingering latched states */
/*   adcistat = adcItrpStatReg(pTheAdcObject); */
/*  NOMERCURY */

   if (flags == 1 /*HS_DTM_ADC*/)	/* if using HS STM?ADC then disable */
   {
      adcItrpDisable(pTheAdcObject,ADC_ALLITRPS);
   }

}

/*----------------------------------------------------------------------*/
/* delay								*/
/*	This routine has two arguments. The first argument is the	*/
/*	number of seconds to delay the second argument is the number	*/
/*	of 100 ns ticks to delay.					*/
/*	This routine will take the number of seconds add to that number	*/
/*	the number seconds that are in the ticks argument.  The 	*/
/*	remainder of ticks are stuffed into the fifo. followed by a	*/
/*	delay word for each second.					*/
/* note  that this implementation leaves agap between 1.6 ms and 1 sec  */
/*----------------------------------------------------------------------*/
void delay(int seconds, unsigned long ticks)
{
   DPRINT2( 3, "delay starts with seconds: %d ticks: %d\n",seconds,ticks );
   seconds = seconds + (int)(ticks/(unsigned int)NUM_TICKS_SECOND);
   ticks = ticks%NUM_TICKS_SECOND;
   if (seconds > 0)
   {
	int i;
	if (ticks > 0)
	{
	   if (ticks >= MIN_DELAY_TICKS)
		fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks-HW_DELAY_FUDGE);
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
	   fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks-HW_DELAY_FUDGE);
	else
	{
	   DPRINT(3,"delay: Delay less than delay minimum \n");
	}
   }
}

/*----------------------------------------------------------------------*/
/* jdelay								*/
/*	This routine takes three arguments. First is numbe rof 16 sec   */
/* 	steps, second is msec step, third is 100 nsec steps		*/
/*----------------------------------------------------------------------*/
void   jdelay(int cnt_16s, int cnt_ms, int cnt_100n )
{
int i;
   for (i=0; i<cnt_16s; i++)
   {
      fifoStuffCmd(pTheFifoObject,CL_DELAY,16*ONE_SECOND_DELAY);
   }
   if (cnt_ms != 0x8000)
      fifoStuffCmd(pTheFifoObject,CL_DELAY,cnt_ms);
   if (cnt_100n != 0x8000) 
      fifoStuffCmd(pTheFifoObject,CL_DELAY,cnt_100n);
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
   int dwell1,dwell2;
   np = np >> 1;		/* divide by 2 for num of CTC's */

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
/*  errLogRet(LOGIT,debugInfo,
/*      "acquire(): np=%x, remnp=%d, numloops=%d\n", np,remnp,numhwloops); */

   /* dwell delay */
/*   if ( dwell > (2*NUM_TICKS_SECOND) )
/*   {
/*	DPRINT(3,"acquire: dwell time greater than 2 seconds \n");
/*	return(-1);
/*   } NOMERCURY */
   if (loopsz > fifolpsize)
   {
	DPRINT(3,"acquire: loop size greater than fifo loop size.\n");
	return(-1);
   }

   /** stuff fifo **/
/*   dwell = dwell-HW_DELAY_FUDGE; NOMERCURY */
   dwell1 = dwell >> 16;
   dwell2 = dwell &  0xFFFF;
   if (numhwloops > 1)
   {
	fifoStuffCmd(pTheFifoObject,CL_START_LOOP | CL_END_LOOP,numhwloops-1);
	fifoStuffCmd(pTheFifoObject,CL_START_LOOP | CL_CTC,dwell1);
        if (dwell2 != 0)
           fifoStuffCmd(pTheFifoObject,NULL,               dwell2);
	loopsz-=2;		/* remove start and end hardloop words */
   }
   for (i=0; i<loopsz; i++)
   {	fifoStuffCmd(pTheFifoObject,CL_CTC,dwell1);
      if (dwell2 != 0)
         fifoStuffCmd(pTheFifoObject,NULL,dwell2);
   }


   if (numhwloops > 1)
   {  if (dwell2 != 0)
      {
         fifoStuffCmd(pTheFifoObject,CL_CTC,     dwell1);
         fifoStuffCmd(pTheFifoObject,CL_END_LOOP,dwell2);
      }
      else
      {
         fifoStuffCmd(pTheFifoObject,CL_CTC | CL_END_LOOP,dwell1);
      }
   }
   for (i=0; i<remnp; i++)
   {  fifoStuffCmd(pTheFifoObject,CL_CTC,dwell1);
      if (dwell2 != 0)
         fifoStuffCmd(pTheFifoObject,NULL,  dwell2);
   }

   return(0);
}

/*----------------------------------------------------------------------*/
/* receivercycle							*/
/* 	Sets receiver cycling on dtm.					*/
/*----------------------------------------------------------------------*/
void receivercycle(int oph, int *dtmcntrl)
{
 ushort stm_control_word;
 unsigned long Codes[10];
 int cnt;

   /* clear phase bits */
   stm_control_word = *dtmcntrl & ~STM_AP_PHASE_CYCLE_MASK;

   /* set phase */
   stm_control_word = stm_control_word | (oph & STM_AP_PHASE_CYCLE_MASK);
   *dtmcntrl = stm_control_word;

   cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
}

/*----------------------------------------------------------------------*/
/* clearscandata							*/
/* 	Sets data zero bit on dtm.					*/
/*----------------------------------------------------------------------*/
void clearscandata(int *dtmcntrl)
{
 ushort stm_control_word;
 unsigned long Codes[10];
 int cnt;

   /* set memory zero */
   *dtmcntrl = *dtmcntrl | STM_AP_MEM_ZERO ;
   stm_control_word = *dtmcntrl;

   cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
}

/*----------------------------------------------------------------------*/
/* initscan								*/
/* 	Sets current number of transients using Completed Blocksizes.	*/
/*	Allocates STM (DPM) buffers.					*/
/*	Arguments:							*/
/*		np	: number of points for the scan			*/
/*		ss	: number of steady state transients		*/
/*		*ssct	: Pointer to Steady State Completed Transients	*/
/*		nt	: Number of Transients.				*/
/*		ct	: Completed Transients				*/
/*		bs	: blocksize					*/
/*		cbs	: Completed blocksizes				*/
/*		maxsum	: maximum accumulated value			*/
/*----------------------------------------------------------------------*/
void initscan(ACODE_ID pAcodeId, int np, int ss, int *ssct, int nt,
					 int ct, int bs, int cbs, int maxsum)
{
 ushort stm_control_word;
   unsigned long Codes[30];
   int cnt,endct,nt_remaining;

   DPRINT5(1,"init scan: np:%d ss:%d ssct:%d nt:%d ct:%d",np,ss,*ssct,nt,ct);
   DPRINT3(1," bs:%d cbs:%d maxsum:%d\n", bs, cbs, maxsum);
   *ssct = ss;
   nt_remaining = (nt - ct) + ss;
   pAcodeId->cur_scan_data_adr = 0;
   pAcodeId->initial_scan_num = ct;

/*   errLogRet(LOGIT,debugInfo,"Nt remaining: 0x%lx\n",nt_remaining); */

   /* set max_sum  */
/*    cnt = stmGenMaxSumCodes(pTheStmObject, maxsum, Codes); NOMERCURY */

   /* set number of transients remaining */
    cnt = stmGenNtCntCodes(pTheStmObject, nt_remaining, Codes);
   DPRINT1(1,"Nt done=%d\n",nt_remaining);

   /* set num_points */
    cnt += stmGenNpCntCodes(pTheStmObject, np, &Codes[cnt]);
   DPRINT1(1,"Np done=%d\n",np);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   DPRINT(1,"Stuffed\n");
}

/*----------------------------------------------------------------------*/
/* nextscan								*/
/* 	Sends stm information to the apbus: source, destination addr 	*/
/*	Sets phase mode and if ct == 0 (steady state or first scan) 	*/
/*	zeroes source memory.						*/
/*	Arguments:							*/
/*		np	: number of points for the scan			*/
/*		dp	: bytes per data point 				*/
/*		nt	: Number of Transients.				*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		dtmcntrl	: stm control info			*/
/*		fidnum	: fid number					*/
/*----------------------------------------------------------------------*/
void nextscan(ACODE_ID pAcodeId, int np, int dp, int nt, int ct, int bs, int *dtmcntrl, int fidnum)
{
 ushort stm_control_word;
 unsigned long Codes[55];
 int cnt,endct,modct;
 unsigned long scan_data_adr;
 FID_STAT_BLOCK *p2statb;

   DPRINT4( 1, "next scan: np:%d dp:%d nt:%d ct:%d", np, dp, nt, ct);
   DPRINT3( 1, " bs:%d dtmcntrl:0x%x fidnum:%d\n", bs, *dtmcntrl, fidnum);
   if (bs > 0) modct = bs;
   else modct = nt;

   if ((fidnum == 0) && (ct == 0))
   {
       DPRINT2(0,"nextscan: FID: %d, CT: %d, Reseting startFLag to 0\n",fidnum,ct);
       fifoClrStart4Exp(pTheFifoObject);
   }
   if ( ((ct % modct) == 0 ) || (pAcodeId->cur_scan_data_adr==0) )
   {
	if ((bs < 1) || ((nt-ct) < bs))
	   endct = nt;
	else 
	   endct = ct + bs;
	/* Only get new buffer if start of scan or blocksize */
	if ((ct != pAcodeId->initial_scan_num ) || 
				((pAcodeId->cur_scan_data_adr==0)))
	{
           /* Now if stmAllocAcqBlk() going to block, we need to decide if the experiment
              has not really been started, if not we should start the FIFO Now before we pend.
              Otherwise everbody will just wait forever
           */
	   DPRINT4(1,"nextscan: FID: %d, CT: %d, WillPend?: %d, startFlag: %d \n",
		fidnum,ct,stmAllocWillBlock(pTheStmObject),fifoGetStart4Exp(pTheFifoObject));
           if ( stmAllocWillBlock(pTheStmObject) == 1 )
           {
               /* Probable only need to start the fifo if:
		  1. No Task is pended on pAcodeId->pSemParseSuspend semaphore
			Note : not an issue since the parser would not be is it was pended.
		  2. No has done the initial start of the fifo for this Exp.
               */
	       /* if ( (fidnum == 1) && (ct > 1) ) */
	       if ( fifoGetStart4Exp(pTheFifoObject) == 0 )
	       {
		  DPRINT(0,"nextscan: about to block, starting FIFO\n");
		  fifoStart4Exp(pTheFifoObject);
	       }
           }
	   p2statb = stmAllocAcqBlk(pTheStmObject, 
	   	(ulong_t) fidnum, (ulong_t) np, (ulong_t) ct, 
		(ulong_t) endct, (ulong_t) nt, (ulong_t) (np * dp), 
		(long *)&pAcodeId->tag2snd, &scan_data_adr );
	}
	else
	   scan_data_adr = pAcodeId->cur_scan_data_adr;
	if (pAcodeId->cur_scan_data_adr == 0) 
	   pAcodeId->cur_scan_data_adr = scan_data_adr;

	DPRINT2(1, "Source addr: 0x%lx  Dest addr: 0x%lx\n",
					pAcodeId->cur_scan_data_adr, scan_data_adr);
	/* Send tag word */
	cnt = stmGenTagCodes(pTheStmObject, pAcodeId->tag2snd, Codes);

        /* NOMERCURY */
        cnt += stmGenSrcDstAdrCodes(pTheStmObject, pAcodeId->cur_scan_data_adr,
                        scan_data_adr, &Codes[cnt]);

	prev_scan_data_adr = pAcodeId->cur_scan_data_adr;
	pAcodeId->cur_scan_data_adr = scan_data_adr;
   }
   else {

	/* Send tag word */
	cnt = stmGenTagCodes(pTheStmObject, pAcodeId->tag2snd, Codes);

        /* NOMERCURY */
        cnt += stmGenSrcDstAdrCodes(pTheStmObject, pAcodeId->cur_scan_data_adr,
                        pAcodeId->cur_scan_data_adr, &Codes[cnt]);

	prev_scan_data_adr = pAcodeId->cur_scan_data_adr;
   }

   /* reset num_points */ /* NOMERCURY */
   stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
   /* zero source data if steady state first transient (ct == 0) */
   cnt += stmGenCntrlCodes(pTheStmObject,(ushort)stm_control_word,&Codes[cnt]);
   if (ct == pAcodeId->initial_scan_num)
	*dtmcntrl |= STM_AP_MEM_ZERO;
   else
        *dtmcntrl = *dtmcntrl & (~STM_AP_MEM_ZERO);

   cnt += stmGenCntrlCodes(pTheStmObject,(ushort)*dtmcntrl,&Codes[cnt]);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

}

/*----------------------------------------------------------------------*/
/* endofscan								*/
/* 	Increments Steady State Completed Transient counter or  	*/
/*	Completed Transient counter.  Checks for ct==nt and blocksize	*/
/*	and enables interrupts for either condition			*/
/*	Arguments:							*/
/*		ss	: steady state transients			*/
/*		*ssct	: ptr to steady state completed transients	*/
/*		nt	: number of transients				*/
/*		*ct	: ptr to completed transients			*/
/*		bs	: blocksize					*/
/*		*cbs	: ptr to completed blocksize			*/
/*----------------------------------------------------------------------*/
void endofscan(int ss, int *ssct, int nt, int *ct, int bs, int *cbs)
{
 ushort stm_control_word;
 unsigned long fifo_control_word, fifo_data_word, Codes[20];
 int cnt;

   DPRINT4(1,"endofscan: ss:%d ssct:%d nt:%d ct:%d",ss,*ssct,nt,*ct);
   DPRINT2(1," bs:%d cbs:%d\n", bs, *cbs);

   /* initialize for sa */
   fifo_control_word = CL_SW_ID_TAG;		/* Signal end of scan */
   fifo_data_word = (int) (800 | 0x4000);	/* in case SA was done */

   /* delay for stm to complete */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT4_USEC);

   /* Enable np |= 0 interrupt */
   stm_control_word = STM_AP_RPNZ_ITRP_MASK;

/*   /* Enable maxsum if requested, for now always enable */
/*   if (*ct > 0)  /* don't enable for 1st transient */
/*     stm_control_word = stm_control_word | STM_AP_MAX_SUM_ITRP_MASK;
/* */
   cnt = stmGenIntrpCodes(pTheStmObject, stm_control_word, Codes);
   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   /* delay */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

   stm_control_word=0;

   /* send interrupt error  mask */
   cnt = stmGenIntrpCodes(pTheStmObject, stm_control_word, Codes);
   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   /* delay */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

   /* zero control word to disable interrupts for next send */
   stm_control_word = 0;

   /* Increment steady state counter or completed transient counter */
   if (*ssct > 0)
	*ssct = *ssct-1;
   else
   {
	*ct = *ct+1;
	if (*ct == nt)
	{
	   /* enable end of scan interrupt */
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
		stm_control_word = stm_control_word | STM_AP_IMMED_ITRP_MASK;

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

   /* Send control word for blocksize or endofscan interrupts */
   cnt = stmGenIntrpCodes(pTheStmObject, stm_control_word, Codes);
   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   /* delay */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);

   /* Send control words to clear all interrupts */
   stm_control_word = 0;
   cnt = stmGenIntrpCodes(pTheStmObject, stm_control_word, Codes);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   /* Send control word for stop acquisition (sa) interrupt */
   fifoStuffCmd(pTheFifoObject,fifo_control_word,fifo_data_word);

   /* delay */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,STM_HW_INTERRUPT_DELAY);
   
}

/*----------------------------------------------------------------------*/
/* setscanvalues							*/
/* 	Sets np , prev data location, dest data location if different.	*/
/*		than zero.						*/
/*	Arguments:							*/
/*		srcoffset	: offset to src data address		*/
/*		destoffset	: offset to dest data address		*/
/*		np		: number of points for the scan		*/
/*		dtmcntrl	: stm control info			*/
/*----------------------------------------------------------------------*/
void setscanvalues(ACODE_ID pAcodeId, int srcoffset, int destoffset, 
						int np, int dtmcntrl)
{
   unsigned long Codes[30];
   unsigned long tmp_src_adr, tmp_dest_adr;
   int cnt;
   ushort stm_control_word;

   DPRINT3(0,"set scan: srcoffset:%d destoffset:%d np:%d\n",srcoffset,
							destoffset,np);
   cnt=0;
   if ((srcoffset != 0) || (destoffset != 0))
   {
	tmp_src_adr = prev_scan_data_adr + srcoffset;
	tmp_dest_adr = pAcodeId->cur_scan_data_adr + destoffset;
	/* set source Address of Data */
	cnt += stmGenSrcAdrCodes(pTheStmObject, tmp_src_adr, 
							&Codes[cnt]);
	/* set Destination Address of Data */
	cnt += stmGenDstAdrCodes(pTheStmObject, tmp_dest_adr,
							&Codes[cnt]);
   	/* stm_control_word =  STM_AP_RELOAD_ADDRS; */
   	stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
   	cnt += stmGenCntrlCodes(pTheStmObject, stm_control_word, &Codes[cnt]);
   }
   if (np != 0) 
   {
   	/* set num_points */
	cnt += stmGenNpCntCodes(pTheStmObject, np, &Codes[cnt]);
   	/* stm_control_word =  STM_AP_RELOAD_NP; */
   	stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
   	cnt += stmGenCntrlCodes(pTheStmObject, stm_control_word, &Codes[cnt]);
   }

   /* reset dtm control word */
   cnt += stmGenCntrlCodes(pTheStmObject, (ushort)dtmcntrl, &Codes[cnt]);
   fifoStuffIt(pTheFifoObject, Codes, cnt/2);  /* into Fifo Buffer */

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
/*		ssct	: steady state completed transients		*/
/*		ct	: completed transients				*/
/*		bs	: blocksize					*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void enableOvrLd(int ssct, int ct, ulong_t *adccntrl)
{
 unsigned long Codes[20];
 unsigned long adc_control_word;
 int cnt;

/* MERCURY has no APBUS control for the ADC. It is written directly over */
/* the VME bus. This is OK for obs. vs. lock. All acquires are done       */
/* separately (for now). I don't know what it will do for ADC overflows.  */
/* We must either leave it on, always and deal with it in the IRS, or     */
/* I expect we will get overflow without interrupts.                      */

 /* Only 1st transient allow adc or receiver overload */
 adc_control_word = (ulong_t) *adccntrl;
 DPRINT3(1,"enableOvrLd: ssct: %ld, ct: %ld, enable ADC itrps, control word = 0x%lx\n",
		ssct,ct,adc_control_word);
 if (ssct == 0)
 {
   DPRINT1(1,"enableOvrLd: ssct==0 && ct==0, enable ADC itrps, control word = 0x%lx\n",adc_control_word);
   adcItrpEnable(pTheAdcObject, adc_control_word);
 }
 else
 {
    adc_control_word = adc_control_word & ~0x4;
     DPRINT1(1,"enableOvrLd: ssct!=0 && ct!=0: disable ADC itrps, control word = 0x%lx\n",adc_control_word);
    adcItrpEnable(pTheAdcObject, adc_control_word);
 }
}

/*----------------------------------------------------------------------*/
/* disableOvrLd								*/
/* 	Disable ADC & Receiver OverLoad AP interrupts Masks 		*/
/*      This done right after the acquire 				*/
/*		*adccntrl: ptr to adc control word 			*/
/*----------------------------------------------------------------------*/
void disableOvrLd(ulong_t *adccntrl)
{
 unsigned long Codes[20];
 unsigned long adc_control_word;
 int cnt;

/* adc_control_word = (ulong_t) *adccntrl;
/* adc_control_word = adc_control_word & 
/*	( ~(ADC_AP_ENABLE_ADC_OVERLD | ADC_AP_ENABLE_RCV2_OVERLD |
/*						ADC_AP_ENABLE_RCV1_OVERLD) );
/* DPRINT1(1,"disableOvrLd: disable ADC itrps, control word = 0x%lx\n",adc_control_word);
/* cnt = adcGenCntrl2Codes(pTheAdcObject, adc_control_word, &Codes[0]);
/* fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
}


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
   /* Set HS STM/ADC   ADC OverLoad Interupt bit */
   DPRINT1(1,"enableStmAdcOvrLd: ssct==0 , control word = 0x%x\n",*dtmcntrl);
   *dtmcntrl = *dtmcntrl | ADM_AP_OVFL_ITRP_MASK;
   stm_control_word = *dtmcntrl;
   DPRINT2(1,"enableStmAdcOvrLd: ssct==0 enable HS STM/ADC Overload itrp, control word = 0x%x (0x%x)\n",stm_control_word,*dtmcntrl);
   cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);
 }
 else
 {
    /* clear HS STM/ADC   ADC OverLoad Interupt bit */
   DPRINT1(1,"enableStmAdcOvrLd: ssct==0 , control word = 0x%x\n",*dtmcntrl);
    *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK;
    stm_control_word = *dtmcntrl;
    DPRINT2(1,"enableStmAdcOvrLd: ssct!=0 disable ADC itrps, control word = 0x%x (0x%x)\n",stm_control_word,*dtmcntrl);
    cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);
 }
 fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
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

    /* clear HS STM/ADC   ADC OverLoad Interupt bit */
   DPRINT1(1,"disableStmAdcOvrLd: control word = 0x%x\n",*dtmcntrl);
    *dtmcntrl = *dtmcntrl & ~ADM_AP_OVFL_ITRP_MASK;
    stm_control_word = *dtmcntrl;
    DPRINT2(1,"disableStmAdcOvrLd: disable HS STM/ADC ADC itrps, control word = 0x%x (0x%x)\n",stm_control_word,*dtmcntrl);
    cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);
    fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
}

#define TIME1MS         0x4000

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
   if ( (pAcodeId->interactive_flag == ACQI_INTERACTIVE) ||
        (updtdelay == -1) )
   {
     if (updtdelay != 0)
     {
	/* For a while it was necessary to insert a delay of 0.5 or 1.0 sec
           before the SW1 interrupt to get the system to work right.  This
	   appears to no longer be the case, so the delay has been removed.

           Notice that there is a separate delay, specified in the argument
           list, that occurs after the SW1 interrupt.			   */

   	/* Put TAG MERCURY interrupt into fifo */
	fifoStuffCmd(pTheFifoObject,CL_SW_ID_TAG,(int) (700 | 0x4000));

	/* delay for interrupt*/
	fifoStuffCmd(pTheFifoObject,CL_DELAY,SW1_HW_INTERRUPT_DELAY);

   	/* Put delay into fifo: no seconds, updtdelay ticks. */
	/* Mercury: the FIFO timer word is limited to 14 bits */
	/* but a 2nd timer base of 1 ms is also available */
	/* this work-a-round limits the resolution of */
	/* relaxdelay to 1 ms, which should not be a problem */
	/*fifoStuffCmd(pTheFifoObject,CL_DELAY,updtdelay);*/
        if (updtdelay > 0)
	   fifoStuffCmd(pTheFifoObject, TIME1MS, updtdelay / 10000 );

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
   if (cnt < 0)
      return(-1);
   fifoStuffIt(pTheFifoObject, Codes, cnt);
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
/*		tindex		: table index
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
/* jtbl_element_addr                                                    */
/*      Returns the address of a table element.                         */
/*      Arguments:                                                      */
/*              *tbl_ptr        : Top of Table Addesses.                */
/*              tindex          : table index                           */
/*              nindex          : num of indices (dims) into table.     */
/*              index           : index(indicies) into table.           */
/*      */
/* Jtable Header format:  Each header entry is a 32 bin int.    */
/*      Entry 1:        num entries             */
/*      Entry 2:        size of each entry (in 32 bit integers  */
/*      Entry 3:        num of dimensions       */
/*      Entry 4:        num of dim1 entries     */
/*      Entry 5:        dim1 mod_factor         */
/*      opt Entry 6:    num of dim2 entries     */
/*      opt Entry 7:    dim2 mod_factor         */
/*      ...                                     */
/*      opt Entry 2+2N: num of dimN entries     */
/*      opt Entry 3+2N: dimN mod_factor         */
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
        /* namebufs or host.                             */
        DPRINT(-1,"tbl_element_addr: table ptr is null.\n");
        return((char *) NULL);
    }

    /* Get table header information */
    tbl_curptr = tbl_ptr;
    nentries = *tbl_curptr++;   /* Get total num of entries in table */
    size_entry = *tbl_curptr++; /* Get size of each entry (in 32 bin ints) */
    ndim = *tbl_curptr++;       /* Get num of dimensions in table */
    DPRINT4(2,"jtbl_element_addr: tblptr %ld, nentries %d size_entry %d ndim %d\
n",
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
            errLogRet(LOGIT,debugInfo,"jtbl_element_addr: Table dimension zero."
);
            return((char *) NULL);
        }
        prev_dim = dim*prev_dim;
        tbl_curptr += 2;
        i++;
    }
    /*DPRINT3(2,"jtbl_element_addr: nentries %d, index: %d, index %% nentries: %
d \n",
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
   index = (ulong_t)(incr) & 3;
   fifoMaskHsl(pTheFifoObject,STD_HS_LINES, tbl_addr[index]);
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
/* NOTE: Mercury uses the IPHASEQUADRANT to save the small angle phase  */
/*       whereas INOVA uses it to save the 0,90,180,270 quadrant	*/
/*       They were first, so they had the choice of name		*/
/*       See the lack of IPHASEQUADRANT in setphase90()			*/
/*----------------------------------------------------------------------*/
int setphase(ACODE_ID pAcodeId, short tbl, int phaseincr)
{
   long *tbl_addr, index;
   short saphase;
   tbl_addr = (long *)tbl_element_addr(pAcodeId->table_ptr,tbl&0xFF,0);
   if (tbl_addr == NULL) 
	return(-1);

   if (tbl&0x200)	/* 32-bit phase shift */
   {  unsigned int uangle,dangle;
      uangle = -(phaseincr * tbl_addr[IPHASESTEP]);
      dangle = uangle - (unsigned int)tbl_addr[IPHASEQUADRANT];
      tbl_addr[IPHASEQUADRANT] = uangle;

      uangle += tbl_addr[IPHASEAPDELAY];
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT,
		 (tbl_addr[IPHASEAPADDR]&0xf00)+0x14);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x100|((uangle    )&0xFF));
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x100|((uangle>> 8)&0xFF));
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x100|((uangle>>16)&0xFF));
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x100|((uangle>>24)&0xFF));
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT,
		 (tbl_addr[IPHASEAPADDR]&0xf00)+0x1e);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,0x100);
       
   }
   else
   {  if (tbl_addr[IPHASEQUADRANT])	/* reset to zero, if not at zero */
      {
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, tbl_addr[IPHASEAPADDR]);
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0xa17);
         saphase = (256-tbl_addr[IPHASEQUADRANT]) & 0xFF;
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0xa00|saphase);
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, tbl_addr[IPHASEAPADDR]);
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0xa1e);
         fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0xa00);
      }

      /* set small angle phase shift */
      saphase = -(phaseincr * tbl_addr[IPHASESTEP]) & 0xFF;

      /* save  small phase angle */
      tbl_addr[IPHASEQUADRANT] = saphase;

      /* set new phase */
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, tbl_addr[IPHASEAPADDR]);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0xa17);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0xa00|saphase);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, tbl_addr[IPHASEAPADDR]);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0xa1e);
      fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0xa00);
   }

   return(0);
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

/* location to save receiver bits for 55 kHz and HB/LB select */
static ushort   obsRcvrMode;

static int saveRcvrmode(ushort mode)
{
   obsRcvrMode = mode;
}

ushort getRcvrmode()
{
   return(obsRcvrMode);
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
void receivergain(ushort apaddr, ushort rcvrmode, int gain)
{
 ushort tmpgain,tmp,tmp2;


   if (gain > 39)
   {  gain = 39;
   }
   tmpgain = gain;

   saveRcvrmode(rcvrmode);

   tmp = (apaddr&0xF00) | (((gain/10)<<1)&0xFF) | rcvrmode;
   tmp2 = (apaddr&0xF00) | rgain[ (gain%10) ];
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_SLCT, apaddr);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_WRT,  0xA80);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_INCWR,tmp);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_SLCT, apaddr);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_WRT,  0xA50);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_INCWR,tmp2);

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
   if ((conf_msg.hw_config.mleg_conf==MLEG_SIS_PIC) | 
			(conf_msg.hw_config.mleg_conf==MLEG_LGSIGMODE))
   {
	if (conf_msg.hw_config.mleg_conf == MLEG_SIS_PIC)
	   if (signaltype == LARGE) signaltype = 0x0;
	   else signaltype = 0x8;
	else if (signaltype == LARGE) signaltype = 0x8;
	   else signaltype = 0x0;

	/* write value to register */
	/* set preamp atten using lock backing store */
	lkpreampreg = getlkpreampgainvalue(); 
	lkpreampreg = (lkpreampreg & ~0x0008) | signaltype;
	writeapword(getlkpreampgainapaddr(),lkpreampreg | 0xff00,AP_MIN_DELAY_CNT);
	storelkpreampgainvalue(lkpreampreg);

   }
}

/*----------------------------------------------------------------------*/
/* apbcout								*/
/* 	Interprets APbus codes.	*/
/*	Arguments: 							*/
/*		intrpp 	: pointer to current acode.			*/
/*----------------------------------------------------------------------*/
/*#define APADDRMSK	0x0fff 
/*#define APBYTF		0x1000
/*#define APINCFLG	0x2000
/*
/*#define APCCNTMSK	0x0fff
/*#define APCONTNUE	0x1000
 */
#define AS_IS   NULL
unsigned short *apbcout(unsigned short *intrpp)
{
int     apcount;
   apcount = *intrpp++;
   do
   {
      fifoStuffCmd(pTheFifoObject,AS_IS,*intrpp);
      intrpp++; apcount--;
   } while (apcount > 0);
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
/* japbcout                                                             */
/*      Outputs a value or values from the realtime variable table      */
/*      to apaddress(es).  In the address field are the APBYTF and      */
/*      and APINCFLG flags for defining byte values and whether to      */
/*      increment the apbus address                                     */
/* NOTE: Currently the data is not packed into the high order word      */
/*       of the data integer.                                           */
/*      Arguments:                                                      */
/*              apdelay : apbusdelay                                    */
/*              apaddr  : apbus address                                 */
/*              data[]  : starting location of data .                   */
/*                        can be from table or rtvar buffer             */
/*              count   : number of values to stuff.                    */
/*----------------------------------------------------------------------*/
void japbcout(ushort apdelay,ushort apaddr,long *data,short apcount)
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

/*----------------------------------------------------------------------*/
/* settunefreq								*/
/* 	Stores APbus codes and band select to set tune frequencies.	*/
/*	Arguments: 							*/
/*		intrpp 	: pointer to current acode.			*/
/*----------------------------------------------------------------------*/
unsigned short *settunefreq(unsigned short *intrpp)
{
   ushort channel,bandselect,freqcode;
   int nfreqcodes,i;

   channel = *intrpp++;
   nfreqcodes = (int)*intrpp++;
   if (pTheTuneObject != NULL)
   {
     for (i=0; i<nfreqcodes; i++)
     {
	/* store freq codes */
	pTheTuneObject->tuneChannel[channel-1].ptsapb[i] = *intrpp++;
	DPRINT2(1,"settunefreq: acode %d: 0x%x\n",i,
			pTheTuneObject->tuneChannel[channel-1].ptsapb[i]);
     }
     pTheTuneObject->tuneChannel[channel-1].band = *intrpp++;
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
/* wgload								*/
/* 	Finds waveform buffer and loads waveform generators.		*/
/*----------------------------------------------------------------------*/
#define TERMINATOR 0xa5b6c7d8		/* see wg.c */
void wgload(ACODE_ID pAcodeId)
{
unsigned short	*wf_pntr,apaddr,apbase,wgHi,wgLo,nelems;

   wf_pntr = getWFSet(pAcodeId->id);
   if (wf_pntr == NULL) 
	return;
   while ( (*wf_pntr > 0) && ( *(int *)wf_pntr != TERMINATOR) )
   {
	apaddr = *wf_pntr++;		/* ap buss address */
	apbase = apaddr & 0xF00;
	wgHi   = (*wf_pntr)>>8;		/* wg on board address  */
        wgLo   = (*wf_pntr)&0xFF;
        wf_pntr++;
	nelems  = *wf_pntr++;
	DPRINT4( -1, "wgload: apaddr=0x%x wgaddr=0x%x%02x nelems=%d\n",
					 apaddr, wgHi, wgLo, nelems);
	wf_pntr++;			/* skip bogus word */
	writeapword(apaddr+2,0x01,STD_APBUS_DELAY);	/* reset each time */
	fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT,apaddr+1);
	fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgLo);
	fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgHi);

	/*****************************************************/
	/* nelems must be an unsigned int to work!!            */
	/*****************************************************/
	fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT,apaddr);
	while (nelems-- > 0)
	{
           wgHi = (*(wf_pntr+1))>>8;
	   wgLo = (*(wf_pntr+1))&0xFF;
	   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgLo);
	   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgHi);
           wgHi = (*(wf_pntr))>>8;
	   wgLo = (*(wf_pntr))&0xFF;
	   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgLo);
	   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,apbase | wgHi);
	   wf_pntr += 2;
	   if ((nelems%10) == 0)
   		fifoStuffCmd(pTheFifoObject,CL_DELAY,0x4007);  /* ~ 7 msec */
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
	readaddr = apaddr;
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
/*		apdelay : delay in 12.5 usec ticks to be sent to apbus	*/
/*			  max is about 650 usecs			*/
/*			  min is 500 ns (total delay 100ns is min) 	*/
/*----------------------------------------------------------------------*/
void writeapword(ushort apaddr,ushort apval, ushort apdelay)
{
/* MERCURY */
ushort  temp;

 DPRINT2( DEBUG_APWORDS, "writeapword: 0x%x, 0x%x\n", apaddr, apval );
/* the reason we go through this twist is that everyone insist that */
/* Mercury software stays as similar to the INOVA software as possible */
/* So we keep on passing the delay and ignore it, twist and turn */
/* to make the differences in hardware obtruce. All this means that */
/* Mercury is twisted and ubtuse, as here */
/* Just because the INOVA has a 16 bit Apbus, Mercury is the old 8-bit */
   temp = (apaddr&0xF00) | (apval & 0xFF);
   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, apaddr);
   fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  temp);

}

void
writeapwordStandAlone(ushort apaddr,ushort apval, ushort apdelay)
{
        DPRINT( DEBUG_APWORDS, "write AP word, stand alone starts\n" );
        init_fifo( NULL, 0, 0 );
        writeapword( apaddr, apval, apdelay );
        fifoStuffCmd(pTheFifoObject,HALTOP,0);
        fifoStart( pTheFifoObject );
        fifoWait4Stop( pTheFifoObject );

        DPRINT( DEBUG_APWORDS, "write AP word, stand alone completes\n" );
}

/* check on presents of sample via the 3 methods available */
multiDetectSample()
{
   int locked,sampdetected,speed,sampinmag;
/*   sampdetected = spinStat(pTheSpinObject,'T'); */
   locked = speed = 0;
/*   if (!sampdetected) */
   {
      taskDelay(60*5);
      locked = autoLockSense(pTheAutoObject);
      speed = (autoSpinValueGet(pTheAutoObject)) / 10;
   }
/*
   locked = autoLockSense(pTheAutoObject);
   speed = (autoSpinValueGet(pTheAutoObject)) / 10;
*/
   sampinmag = (locked || speed /* || sampdetected MERCURY */ );
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


   /* If LC Sample (i.e. gilson sample handler) value then don't test for sample presents in magnet */
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
         sampinmag = getSampleDetect();
         if ( ! sampinmag )
         {
            *presentSample = -99;
            DPRINT(0,"getSampFromMagnet   Sample changed from 0 to -99\n");
         }
      }
      else if ( *presentSample == -99 )
      {
         /* If declared empty, do the quick check to confirm */
         if (getSampleDetect())
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
      if (roboStat && (removedSamp != 0))
      {
         Status = roboStat;
         sampleHasChanged = FALSE;   
         /* Assume worst case that sample is still LC Probe */
         *presentSample = removedSamp;
      }
      else
      {
            sampleHasChanged = TRUE;   
            *presentSample = 0;
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
                  int skipsampdetect, int spinActive)
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
   
/*            /* check on presents of sample via the 3 methods available */
/*            sampinmag = multiDetectSample();
/*            DPRINT1(0,"ROBO Err, Sample is in Mag: '%s'\n",
/*               (sampinmag == 1) ? "YES" : "NO" );
/*            /* if ( locked || sampdetected || speed ) */
/*            if ( sampinmag )
/*            {
/*               *presentSamp = (short) newSample;
/*               sampleHasChanged = TRUE;
/*            } 
/* */
            errLogRet(LOGIT,debugInfo,
                     "putSampIntoMagnet: Robo Error: status is %d",roboStat);
         }
         else 
         {
/*            /* check on presents of sample, if not there bump the air just incase it's stuck */
/*            for(cnt=0; cnt < 3; cnt++)
/*            {
/*               /* check on presents of sample via the 3 methods available */
/*               DPRINT(0,"putSampFromMagnet: check for Sample\n");
*/
/*       sampinmag = multiDetectSample();
/*	taskDelay(60*2);
/*               DPRINT1(0,"putSampFromMagnet -   Sample In Mag: '%s'\n",
/*                  (sampinmag == 1) ? "YES" : "NO" );
/*               if ((sampinmag) || (cnt == 2))
/*                  break;
/*               DPRINT(0,"putSampFromMagnet: Bump Air\n");
/*               autoSampleBump(pTheAutoObject);
/*
/*            }
/*            /* if ( locked || sampdetected || speed ) */
/*            if ( sampinmag )
/*            {
/* */
               DPRINT(0,"==========> Load Sample: Sample Detected\n");
               sampleHasChanged = TRUE; /* successful completion */
               *presentSamp= (short) newSample;
               if (!spinActive)
	          setspin( (int) sampleSpinRate, (int) 0 );
/*            }      
/*            else   /* Ooops, no sample detected signal insetion error */
/*            {
/*               DPRINT(0,"==========> Load Sample: Sample NOT Detected\n");
/*               sampleHasChanged = FALSE;
/*               Status = SMPERROR + SMPINFAIL;
/*               *presentSamp= (short) newSample;	/* be safe and assume its in the magnet */
/*               errLogRet(LOGIT,debugInfo,
/*                           "putSampIntoMagnet Error: status is %d",roboStat);
/*            }
/* */
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
               while ( !getSampleDetect() && (max > 0) )
               {
                  taskDelay(sysClkRateGet()*1);  /* 1 sec */
                  if (max == 20)
                     DPRINT(0,"==> Load Sample: Waiting for Sample Detect\n");
                  max--;
               }
               DPRINT2(0,"==> Load Sample: detect= %d after %d sec.\n",
                           getSampleDetect(), 20-max);
               if ( !getSampleDetect() )
               {
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
   fifoStuffIt(pTheFifoObject, Codes, cnt);
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
   fifoStuffIt(pTheFifoObject, Codes, cnt);
   taskDelay(10);
}
itdelay(int usec)
{
   unsigned long ticks;
   /* ticks = 80000L * msec;   /* N msec */
   ticks = 80L * usec;   /* N usec */
   fifoStuffCmd(pTheFifoObject,CL_DELAY,ticks - HW_DELAY_FUDGE);
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
