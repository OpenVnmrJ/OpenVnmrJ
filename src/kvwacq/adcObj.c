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
#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
#include <msgQLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <objLib.h>
#include <errno.h>
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "vmeIntrp.h"
#include "hardware.h"
#include "timeconst.h"
#include "commondefs.h"
#include "adcObj.h"
#include "stmObj.h"
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"


/*
modification history
--------------------
1-4-95,gmb  created 
*/
/*

ADC Object Routines

   Interrupt Service Routines (ISR)

*/

extern MSG_Q_ID pMsgesToPHandlr;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;
extern STMOBJ_ID pTheStmObject;	/* STM Object */

static ADC_OBJ *adcObj;
static char *IDStr ="ADC Object";
static int  IdCnt = 0;
static ITR_MSG itrmsge;

/* counter to keep track of ss done (through fifo, not parser), */
/* used to enable or disable ADC overflow interrupt */
/* see adcOvldClear() in adcObj.c, and fifoTagFifo() in fifoObj.c */
extern int     sscnt_adcitrp;
/*-----------------------------------------------------------
|
|  Internal Functions
|
+---------------------------------------------------------*/
/*-------------------------------------------------------------
| Interrupt Service Routines (ISR) 
+--------------------------------------------------------------*/
/*******************************************
*
* adcException - Interrupt Service Routine
*
*   adc overflow, receiver overflow
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void adcException(ADC_ID pAdcId)
{
  unsigned short adcstatus;
  unsigned short adcTag;
  int stat;

  /* if adcOvldFlag is true disable interrupts */
  if (pAdcId->adcOvldFlag == TRUE)
  {
     adcItrpDisable(pAdcId,ADC_ALLITRPS);
     adcstatus = *ADC_STATR(pAdcId->adcBaseAddr); /* read status register */
     return;
  }

  adcstatus = *ADC_STATR(pAdcId->adcBaseAddr); /* read status register */
  if ( adcstatus & ADC_OVERFLOW_LATCHED )
  {
#ifdef INSTRUMENT
     wvEvent(EVENT_ADC_OVERFLOW,NULL,NULL);
#endif
     adcItrpDisable(pAdcId,ADC_ALLITRPS);
     DPRINT1(0,"ADC ISR:  Adc Overflow: 0x%x\n",adcstatus);
     itrmsge.donecode = WARNING_MSG;		/* donecode for Vnmr */
     itrmsge.errorcode =  WARNINGS + ADCOVER; /* errorcode for Vnmr */;
     itrmsge.msgType = INTERRUPT_OCCURRED;
     stat = msgQSend(pTheStmObject->pIntrpMsgs, (char *)(&itrmsge),sizeof(ITR_MSG),
		NO_WAIT,MSG_PRI_NORMAL);
     pAdcId->adcOvldFlag = TRUE;
  }
/* NOMERCURY Mercury does not have RCVR Overload */
  else
  {
     errLogRet(LOGIT,debugInfo,
      "adc ISR: NOT ADC or Recv Overload, Illegal condition: adc status 0x%x\n",
	adcstatus);
     stat = 0;
  }

  if (stat == ERROR)
  {
	if (errno == S_objLib_OBJ_UNAVAILABLE)
           errLogRet(LOGIT,debugInfo,"adc ISR: MsgQ full lost Interrupt!!!!!!!\n");
	else
           errLogSysRet(LOGIT,debugInfo,"adc ISR: MsgQ error: ");
  }
  return;
}

#ifdef XXX
/* for test while hardware is being stablized */
fakeadcol()
{
     GenericException.exceptionType = WARNING_MSG;  	/* donecode for Vnmr */
     GenericException.reportEvent = WARNINGS + ADCOVER; /* errorcode for Vnmr */
     /* send error to exception handler task, it knows what to do */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, sizeof(EXCEPTION_MSGE), 
		 NO_WAIT, MSG_PRI_URGENT);
}
fakercvol()
{
     GenericException.exceptionType = WARNING_MSG;  
     GenericException.reportEvent = WARNINGS + RECVOVER;  
     /* send error to exception handler task, it knows what to do */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, sizeof(EXCEPTION_MSGE), 
		 NO_WAIT, MSG_PRI_NORMAL);
}
#endif

/*-------------------------------------------------------------
| FIFO Object Public Interfaces
+-------------------------------------------------------------*/

/**************************************************************
*
*  adcCreate - create the ADC Object Data Structure & Semaphore
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 
ADC_ID  adcCreate(unsigned long baseAddr, int apBusAddr, int vector, int level, char* idstr)
/* unsigned long baseAddr - base address of FIFO */
/* int apBusAddr  - Ap Bus address*/
/* int   vector  - VME Interrupt vector number */
/* int   level   - VME Interrupt level */
/* char* idstr - user indentifier string */
{
   void adcException(ADC_ID pAdcId);

   char tmpstr[80];
   register ADC_OBJ *pAdcObj;
   short sr;
   long dspcoef;

  /* ------- malloc space for FIFO Object --------- */
  if ( (pAdcObj = (ADC_OBJ *) malloc( sizeof(ADC_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"adcCreate: Could not Allocate Space:");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pAdcObj,0,sizeof(ADC_OBJ));

  /* ------ Translate Bus address to CPU Board local address ----- */
  pAdcObj->adcBaseAddr = baseAddr;
/*  if (sysBusToLocalAdrs(FIFO_VME_ACCESS_TYPE,
/*              ((long)baseAddr & 0xffffff),&(pAdcObj->adcBaseAddr)) == -1)
/*  {
/*    errLogRet(LOGIT,debugInfo,
/*       "adcCreate: Can't Obtain Bus(0x%lx) to Local Address.",
/*	  baseAddr);
/*    adcDelete(pAdcObj);
/*    return(NULL);
/*  } */

  /* Check Base Vector for validity */
/* --------- ADC interupt vector numbers ------------ */
/* Each Addition ADC board, add 1 to vector number */
/* Covers ADC OverFlow, Receiver 1 or 2 OverLoad */

  if ( (vector >= ADC_ITRP_VEC) && (vector < (ADC_ITRP_VEC+8)) )
  {
     pAdcObj->vmeItrVector = vector;
  }
  else
  {
        errLogRet(LOGIT,debugInfo,
	  "adcCreate: Invalid Base Vector: 0x%x (Valid: 0x%x - 0x%x)\n",
	   vector,ADC_ITRP_VEC,ADC_ITRP_VEC+7);
        adcDelete(pAdcObj);
        return(NULL);
  }

  pAdcObj->vmeItrLevel = 2;
  pAdcObj->ApBusAddr = apBusAddr;

  /* ------ Create Id String ---------- */
  IdCnt++;
  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",IDStr,IdCnt);
     pAdcObj->pIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pAdcObj->pIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pAdcObj->pIdStr == NULL)
  {
     adcDelete(pAdcObj);
     errLogSysRet(LOGIT,debugInfo,
	"adcCreate: IdStr - Could not Allocate Space:");
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pAdcObj->pIdStr,tmpstr);
  }
  else
  {
     strcpy(pAdcObj->pIdStr,idstr);
  }

  pAdcObj->pSID = NULL;	/* SCCS ID */

  /* ------ Test for Boards Presents ---------- */
  if ( vxMemProbe((char*) (pAdcObj->adcBaseAddr + ADC_SR), 
		     VX_READ, 2, &sr) == ERROR)
  { 
    errLogRet(LOGIT,debugInfo,
       "adcCreate: Could not read ADC's Status register(0x%lx), Board 0x%lx NOT Present\n",
		(pAdcObj->adcBaseAddr + ADC_SR), pAdcObj->adcBaseAddr);
    if (IdCnt > 1)
    {
      adcDelete(pAdcObj);
      return(NULL);
    }
    else
    {
      pAdcObj->adcBaseAddr = 0xFFFFFFFF;
      pAdcObj->dspDownLoadAddr = (void *) 0xFFFFFFFF;
      pAdcObj->dspPromType = DSP_PROM_NO_DSP;
      return(pAdcObj);
    }
  }

  /* ----- reset board and get status register */
  adcReset(pAdcObj); /* reset board */
  pAdcObj->adcState = *ADC_STATR(pAdcObj->adcBaseAddr);

/*  /* read from Diagnostic PROM */
/*#ifdef PROM_INSTALLED
/*  pAdcObj->adcBrdVersion = FF_REG(pAdcObj->adcBaseAddr,0);
/*  DPRINT2(1,"adcCreate: ADC Board Version %d (0x%x) present.\n",
/*		pAdcObj->adcBrdVersion,pAdcObj->adcBrdVersion);
/*#else
/*  pAdcObj->adcBrdVersion = 0xff;
/*  DPRINT2(1,"adcCreate: Skipped PROM, made ADC Board Version %d (0x%x)\n",
/*		pAdcObj->adcBrdVersion,pAdcObj->adcBrdVersion);
/*#endif
/* */
/*  /* obtain status register which indicates if DSP is present */
/*  pAdcObj->optionsPresent = *ADC_STATR(pAdcObj->adcBaseAddr);
/* */
  /*adcConfigDsp( pAdcObj );  Do not configure DSP here; 
			  the DSP board is not ready yet */
  /* DSP is configured in getconf.c */

  /*------ Disable all interrupts on board -----------*/
  adcItrpDisable(pAdcObj,0xFF);

  /* ------- Connect VME interrupt vector to proper Semaphore to Give ----- */

   if ( intConnect( 
	INUM_TO_IVEC( pAdcObj->vmeItrVector ),  
		     adcException, pAdcObj) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"adcCreate: Could not connect ADC interrupt vector: ");
     adcDelete(pAdcObj);
     return(NULL);
   }

   DPRINT1(0,"ADC 0x%lx PRESENT.\n",pAdcObj->adcBaseAddr);
   return( pAdcObj );
}

int
adcConfigDsp( ADC_ID pAdcId )
{
	int	dspcoef;
	char	promVersion[ 4 ];

	if (pAdcId == NULL)
	  return( -1 );
	if (pAdcId->adcBaseAddr == 0xFFFFFFFF)
	  return( -1 );

  /* double check (via addr) if DSP board is present */

	if (vxMemProbe( (char *) (pAdcId->adcBaseAddr), VX_READ, 4, &dspcoef ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
		return( 0 );
	}

	if (vxMemProbe( (char *) (pAdcId->adcBaseAddr), VX_WRITE, 4, &dspcoef ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
		return( 0 );
	}

	if (vxMemProbe(
		(char *) (pAdcId->adcBaseAddr + DSP_PROM_VERSION_OFFSET),
			  VX_READ,
		sizeof( promVersion ),
			 &promVersion[ 0 ]
       ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
		return( 0 );
	}

	if (strncmp( &promVersion[ 0 ], "PROM", sizeof( promVersion ) ) == 0) {
		pAdcId->dspPromType = DSP_PROM_DOWNLOAD;
		pAdcId->dspDownLoadAddr = (void *) (pAdcId->adcBaseAddr + DSP_COEF_OFFSET);
	}
	else {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DOWNLOAD;
	}

	return( 0 );
}

/**************************************************************
*
*  adcDelete - Deletes ADC Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int adcDelete(ADC_ID pAdcId)
/* ADC_ID 	pAdcId - fifo Object identifier */
{
   if (pAdcId != NULL)
   {

      if (pAdcId->pIdStr != NULL)
	 free(pAdcId->pIdStr);
      free(pAdcId);
   }
}


/**************************************************************
*
*  adcOvldClear - Clears ADC Ovld Flag, enables interrupts, and reads 
*	status register.
*
*
* RETURNS:
*  void
*
*	Author M. Howitt  6/20/95
*/
void adcOvldClear(ADC_ID pAdcId)
/* ADC_ID 	pAdcId - fifo Object identifier */
{
  unsigned short adcstatus;

  if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;

/*  adcstatus = *ADC_ISTATR(pAdcId->adcBaseAddr); /* read status register */
/*  NOMERCURY */
    adcItrpEnable(pAdcId, ADC_RESET);	/* bit up */
    adcItrpDisable(pAdcId, ADC_RESET);	/* bit down */

  pAdcId->adcOvldFlag = FALSE;

  if ( ! sscnt_adcitrp )		/* only if ssct=0 */
     adcItrpEnable(pAdcId,ADC_ALLITRPS);

#ifdef INSTRUMENT
     wvEvent(EVENT_ADC_OVEFLW_CLR,NULL,NULL);
#endif
  DPRINT1(1,"adcOvldClear:  Adc Status: 0x%x\n",adcstatus);
}


/**************************************************************
*
*  adcItrpEnable - Set the ADC Interrupt Mask
*
*  This routines set the VME interrupt mask of the ADC. 
*
* RETURNS:
* void 
*
*/ 
void adcItrpEnable(ADC_ID pAdcId, int mask)
/* ADC_ID 	pAdcId - ADC Object identifier */
/* int mask;	 mask of interrupts to enable */
{
int	tmp;
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;
/*   if (mask != 0x4) 
/*   {  printf("adcItrpEnable(): mask != 0x4, ignored\n");
/*      return;
/*   } */

   /* enable when bit is set to 1 */
   tmp = ( (pAdcId->adcCntrlReg | mask) & 0x00FF);
   *ADC_CNTRL(pAdcId->adcBaseAddr) = pAdcId->adcCntrlReg =  tmp;
   DPRINT1(1,"control=%ld",tmp);
}

/**************************************************************
*
*  adcItrpDisable - Set the ADC Interrupt Mask
*
*  This routines set the VME interrupt mask of the ADC. 
*
* RETURNS:
* void 
*
*/ 
void adcItrpDisable(ADC_ID pAdcId, int mask)
/* ADC_ID 	pAdcId - ADC Object identifier */
/* int mask;	 mask of interrupts to disable */
{
int	tmp;
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;
/*   if (mask != 0x4) 
/*   {  printf("adcItrpEnable(): mask != 0x4, ignored\n");
/*      return;
/*   }
/* */

   /* disable when bit is unset (0)*/
   tmp = ( (pAdcId->adcCntrlReg & ~mask) & 0x00FF);
   *ADC_CNTRL(pAdcId->adcBaseAddr) = pAdcId->adcCntrlReg = tmp;
   DPRINT1(1,"control=%ld",tmp);
	/* Oops, read cntrl reg only gives byte, upper byte junk,
	   so mask this out, so that we don't reset,etc. the board */
}

/**************************************************************
*
*  adcReset - Resets ADC functions 
*
*
* RETURNS:
* 
*/
void adcReset(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   int state;

   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;

   /* clear all interrupts 1st */
   state = ( (pAdcId->adcCntrlReg & ~ADC_ALLITRPS) & 0x00FF );
   *ADC_CNTRL(pAdcId->adcBaseAddr) = pAdcId->adcCntrlReg = state;
/* set and unset reset */
   state = ((pAdcId->adcCntrlReg & 0x00FF) | ADC_RESET);
   *ADC_CNTRL(pAdcId->adcBaseAddr) =  pAdcId->adcCntrlReg = state;
   state = ((pAdcId->adcCntrlReg & 0x00FF) & ~ADC_RESET);
   *ADC_CNTRL(pAdcId->adcBaseAddr) = pAdcId->adcCntrlReg = state;
   DPRINT1(3,"control=%ld",state);
#ifdef INSTRUMENT
     wvEvent(EVENT_ADC_RESET,NULL,NULL);
#endif
}

/**************************************************************
*
*  adcStatReg - Gets ADC status register value
*
*
* RETURNS:
*  16-bit ADC Status Register Value
*/
short adcStatReg(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return(-1);

    return (*ADC_STATR(pAdcId->adcBaseAddr));
}

/**************************************************************
*
*  adcDSPpresent - returns 1 if DSP is present on ADC
*
*
* RETURNS:
*  1- DSP present, 0- if not present
*/
int adcDSPpresent(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return(0);

    return(((pAdcId->optionsPresent & ADC_DSP_NOTPRESENT) ? 0 : 1)) ;
}

/**************************************************************
*
*  adcItrpStatReg - Gets ADC status register value
*
*
* RETURNS:
*  16-bit ADC Status Register Value
*/
ushort_t adcItrpStatReg(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return(-1);

   return (*ADC_ISTATR(pAdcId->adcBaseAddr));
}


/**************************************************************
*
*  adcCntrlReg - Gets ADC control register value
*
*
* RETURNS:
*  16-bit ADC Control Register Value
*/
short adcCntrlReg(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return(-1);

    return ((pAdcId->adcCntrlReg) & 0xff);
}


/**************************************************************
*
*  adcIntrpMask - Gets ADC Interrupt Register mask
*
*
* RETURNS:
*  16-bit FIFO Interrupt Mask Value
*/
short adcIntrpMask(ADC_ID pAdcId)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return(-1);

    return ((pAdcId->adcCntrlReg) & ADC_ALLITRPS);
}

/**************************************************************
*
*  adcReadData - Reads the Real & Imaginary Data that the ADC converted
*
*     fills in data value pointers
*
* RETURNS:
*  void
*/
void adcReadData(ADC_ID pAdcId,short *real, short *imag)
/* pAdcId - fifo Object identifier */
{
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;

   /* make sure it's the real data we are reading */
/*   *ADC_CNTRL(pAdcId->adcBaseAddr) =  
/*	    ((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) & ~ADC_READ_IMGREG);
/*   *real = *ADC_READ(pAdcId->adcBaseAddr);
/*   *ADC_CNTRL(pAdcId->adcBaseAddr) = 
/*		((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) | ADC_READ_IMGREG);
/*   *imag = *ADC_READ(pAdcId->adcBaseAddr);
/*   *ADC_CNTRL(pAdcId->adcBaseAddr) =  
/*	    ((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) & ~ADC_READ_IMGREG); */
    return;
}

/**************************************************************
*
*  adcGenDspCodes - Generate Fifo words to set ADC's DSP Reg.
*   
*  Generates the appropriate Fifo Words to set the Ap DSP register.
*  This would be the 'M' or oversampling to be done.
*  0 - No DSP
*  1, 2, 3, 4, 5, 10, 15, 20, 25, 50, 75, 100, 150, 200, 
*     300, 400, 500, 1000, 2000 
*
*High Speed Line setting are not included.
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int adcGenDspCodes(ADC_ID pAdcId, int ovrsamp, unsigned long *pCodes)
/* pAdcId - ADC Object identifier */
/* ovrsamp - Over Sampling Rate */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   unsigned long lowAddr,apAdrData;

   if (pAdcId == NULL)
      return(-1);

   /* generate the low 16-bit of source Address */
  lowAddr = pAdcId->ApBusAddr + ADC_AP_DSPCNTRL;
  apAdrData = (lowAddr << 16) | (ovrsamp & 0xffff);
  *pCodes = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
  *(pCodes+1) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
  DPRINT3(1,"stmGenApCmds: low apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*pCodes,*(pCodes+1));

  /* Min Delay between Apbus Transactions */
  *(pCodes+2)= 0L | (( (AP_HW_MIN_DELAY_CNT) & 0x0fffffff) >> 5);  /* hw */
  *(pCodes+3) = (0xff & (AP_HW_MIN_DELAY_CNT)) << 27;   /* lw */

  return(4);
}

/**************************************************************
*
*  adcGenCntrlCodes - Generate Fifo words to set ADC's Control Reg.
*   
*  Generates the appropriate Fifo Words to set the Ap Control register.
*High Speed Line setting are not included.
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int adcGenCntrlCodes(ADC_ID pAdcId, unsigned long cntrl, unsigned long *pCodes)
/* pAdcId - ADC Object identifier */
/* cntrl - Source Address */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   if (pAdcId == NULL)
      return(-1);

  return(
   adcGenApRegCmds(pAdcId->ApBusAddr,ADC_AP_CNTRL1,ADC_AP_CNTRL2,cntrl,pCodes)
        );
}

/**************************************************************
*
*  adcGenApRegCmds - Generate Fifo words to set give Ap Register Cmds.
*   
*  Generates the appropriate Fifo Words to set the given ADC Ap Registers
*  High Speed Line setting are not included.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int adcGenApRegCmds(unsigned long baseAddr, unsigned long offsetLow, unsigned long offsetHigh, unsigned long data, unsigned long *pCodes)
/* baseAddr - Apbus Base Address */
/* offsetLow - Ap Address Offset to low 16-bits */
/* offsetHigh - Ap Address Offset to high 16-bits */
/* data - Source Address */
/* pCodes - Array to put generated codes. (fifo words) */
{
   unsigned long lowAddr,highAddr,apAdrData;
   unsigned long delay;

   /* generate the low 16-bit of source Address */
   lowAddr = baseAddr + offsetLow;
   apAdrData = (lowAddr << 16) | (data & 0xffff);
   *pCodes = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
   *(pCodes+1) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
   DPRINT3(1,"adcGenSrcAdrCodes: low apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*pCodes,*(pCodes+1));

   delay = AP_HW_MIN_DELAY_CNT;
   /* Min Delay between Apbus Transactions */
   *(pCodes+2)= 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   *(pCodes+3) = (0xff & delay) << 27;   /* lw */

   /* generate the high 16-bit of source Address */
   highAddr = baseAddr + offsetHigh;
   apAdrData = (highAddr << 16) | ((data >> 16) & 0xffff);
   *(pCodes+4) = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
   *(pCodes+5) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
   DPRINT3(1,"adcGenSrcAdrCodes: high apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*(pCodes+2),*(pCodes+3));

   /* Min Delay between Apbus Transactions */
   *(pCodes+6)= 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   *(pCodes+7) = (0xff & delay) << 27;   /* lw */

   return(8);
}

/**************************************************************
*
*  adcGenCntrl2Codes - Generate Fifo words to set high word in ADC's 
*			Control Reg.
*   
*  Generates the appropriate Fifo Words to set the high word in the 
*  Ap Control register.  This high word includes CTC, OVRLD enables.
*  High Speed Line setting are not included.
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int adcGenCntrl2Codes(ADC_ID pAdcId, unsigned long cntrl, unsigned long *pCodes)
/* pAdcId - ADC Object identifier */
/* cntrl - Source Address */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   unsigned long baseAddr,offsetHigh,highAddr,apAdrData;
   unsigned long delay;

   if (pAdcId == NULL)
      return(-1);

   baseAddr = pAdcId->ApBusAddr;
   offsetHigh = ADC_AP_CNTRL2;
   delay = AP_HW_MIN_DELAY_CNT;

   /* generate the high 16-bit of source Address */
   highAddr = baseAddr + offsetHigh;
   apAdrData = (highAddr << 16) | ((cntrl >> 16) & 0xffff);
   *(pCodes) = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
   *(pCodes+1) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
   DPRINT3(1,"adcGenCntrl2Codes: high apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*(pCodes),*(pCodes+1));

   /* Min Delay between Apbus Transactions */
   *(pCodes+2)= 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   *(pCodes+3) = (0xff & delay) << 27;   /* lw */

   return(4);
}

/********************************************************************
* adcShow - display the status information on the ADC Object
*
*  This routine display the status information of the ADC Object
*
*
*  RETURN
*   VOID
*
*/
void adcShow(ADC_ID pAdcId, int level)
/* ADC_ID pAdcId  - ADC Object ID */
/* int level 	   - level of information */
{
   int i;
   char *pstr;
   unsigned short status,rstatus;

   if (pAdcId == NULL)
   {
     printf("fifoShow: FIFO Object pointer is NULL.\n");
     return;
   }
   printf("\n\n-------------------------------------------------------------\n\n");
   if (pAdcId->adcBaseAddr == 0xFFFFFFFF)
     printf(">>>>>>>>>>  ADC Board NOT Present, ADC Object for Testing. <<<<<<<<<<<\n\n");
   printf("ADC Object: Board Addr: 0x%lx, '%s'\n",pAdcId->adcBaseAddr, pAdcId->pIdStr);

   printf("Board Ver: %d,  VME: vector 0x%x, level %d, Ap Addr: 0x%x\n",
		pAdcId->adcBrdVersion, pAdcId->vmeItrVector, 
		pAdcId->vmeItrLevel,pAdcId->ApBusAddr);

  if (pAdcId->adcBaseAddr == 0xFFFFFFFF)
     return;


   rstatus = *ADC_STATR(pAdcId->adcBaseAddr);
   printf("ADC Status Reg: 0x%x\n", rstatus);

   status = *ADC_ISTATR(pAdcId->adcBaseAddr);
   printf("ADC AP Mask Status Reg: 0x%x\n", status);

 printf("ADC:\n ");
 if (status & RCV1_OVERLD_LATCHED) 
 {
   pstr = "Receiver 1 OverLoad"; 
   printf("      %s\n",pstr);
 }
 else if (status & RCV2_OVERLD_LATCHED)
 {
   pstr = "Receiver 1 OverLoad";
   printf("     %s\n",pstr);
 }
 else
 {
   pstr = "No Receiver OverLoads";
   printf("     %s\n",pstr);
 }

 if (status & ADC_OVERFLOW_LATCHED) 
 {
   pstr = "ADC OverFLow"; 
 }
 else
   pstr = "No ADC OverLoads";
 printf("      %s\n",pstr);
 printf("      %s\n",(rstatus & ADC_INTR_PEND) ? "Interrpt Pending" :  "No Interrupts Pending");

  printf("Board Options: DSP  %s\n",
           ( (pAdcId->optionsPresent & ADC_DSP_NOTPRESENT) ? "Not Present" : "Present" ) 
	 );

 switch (pAdcId->dspPromType) {
    case DSP_PROM_NO_DSP:
        printf( "No DSP\n" );
        break;

    case DSP_PROM_NO_DOWNLOAD:
        printf( "1st version of DSP with no download to RAM\n" );
        break;

    case DSP_PROM_DOWNLOAD:
        printf( "2nd version of DSP with download to RAM\n" );
        break;
 }
 if (pAdcId->dspDownLoadAddr == (void *) 0xFFFFFFFF)
   printf("     No DSP download\n");
 else
   printf("     DSP download at 0x%x\n", pAdcId->dspDownLoadAddr);

 return;
}
