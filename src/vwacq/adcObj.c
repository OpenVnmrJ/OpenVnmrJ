/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* adcObj.c  11.1 07/09/07 - ADC Object Source Modules */
#ifndef LINT
#endif
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


#define HW_DELAY_FUDGE 3L

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

static ADC_OBJ *adcObj;
static ADC_ID AdcList[MAX_ADC_OBJECTS];
static char *IDStr ="ADC Object";
static int nAdcsPresent = 0;
static ITR_MSG itrmsge;

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
  STMOBJ_ID pStmId;
  DPRINT2(0,"ADC (0x%lx @ vme 0x%lx) ISR:  Exception",pAdcId,pAdcId->adcBaseAddr);

  /* if adcOvldFlag is true disable interrupts */
  /* DPRINT1(0,"pAdcId->adcOvldFlag = %d",pAdcId->adcOvldFlag); */
  if (pAdcId->adcOvldFlag == TRUE)
  {
     adcItrpDisable(pAdcId,ADC_ALLITRPS);
     adcstatus = *ADC_ISTATR(pAdcId->adcBaseAddr); /* read status register */
     return;
  }

  /* Get STM corresponding to this ADC */
  pStmId = stmGetStmObjByIndex(pAdcId->index);
  DPRINT3(0,"pAdc index=%d -> pStm: 0x%lx @ vme 0x%lx",pAdcId->index,pStmId,pStmId->stmBaseAddr);

  adcstatus = *ADC_ISTATR(pAdcId->adcBaseAddr); /* read status register */
  itrmsge.tag = (short) (*STM_TAG(pStmId->stmBaseAddr));  /* get tag id */
  itrmsge.stmId = pStmId;
  itrmsge.count = ( *STM_CTHW(pStmId->stmBaseAddr) << 16 ) |
				      *STM_CTLW(pStmId->stmBaseAddr);
  if ( adcstatus & ADC_OVERFLOW_LATCHED )
  {
#ifdef INSTRUMENT
     wvEvent(EVENT_ADC_OVERFLOW,NULL,NULL);
#endif
     DPRINT1(0,"ADC ISR:  Adc Overflow: 0x%x\n",adcstatus);
     itrmsge.donecode = WARNING_MSG;		/* donecode for Vnmr */
     itrmsge.errorcode =  WARNINGS + ADCOVER; /* errorcode for Vnmr */;
     itrmsge.msgType = INTERRUPT_OCCURRED;
     stat = msgQSend(pStmId->pIntrpMsgs, (char *)(&itrmsge),sizeof(ITR_MSG),
		NO_WAIT,MSG_PRI_NORMAL);
     pAdcId->adcOvldFlag = TRUE;
  }
  else if ( adcstatus & (RCV1_OVERLD_LATCHED | RCV2_OVERLD_LATCHED) )
  {
#ifdef INSTRUMENT
     wvEvent(EVENT_RCV_OVERFLOW,NULL,NULL);
#endif
     DPRINT3(0,"ADC ISR: tag: %d, count: %ld,  Receiver Overflow: 0x%x\n",itrmsge.tag,itrmsge.count,adcstatus);
     itrmsge.donecode = WARNING_MSG;		/* donecode for Vnmr */
     itrmsge.errorcode =  WARNINGS + RECVOVER; /* errorcode for Vnmr */;
     itrmsge.msgType = INTERRUPT_OCCURRED;
     stat = msgQSend(pStmId->pIntrpMsgs, (char *)(&itrmsge),sizeof(ITR_MSG),
		NO_WAIT,MSG_PRI_NORMAL);
     pAdcId->adcOvldFlag = TRUE;
  }
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
/* ADC_ID  adcCreate(int adcChannel, int level, char* idstr) */
/* int   adcChannel  - Index number of this ADC; 0, ... */
/* int   level   - VME Interrupt level (NOT USED) */
/* char* idstr - user indentifier string */
ADC_ID  adcCreate(int adcChannel)
/* int   adcChannel  - Index number of this ADC; 0-3 
/*   (though they could be 2 500KHz ADCs per DTM for a total of 8 ADCs */
/*    this has never been and is unsupported in many ways */
/*   So the assumption if 0-1st DTM, 1- 2nd DTM, or channel, etc... */
{
   void adcException(ADC_ID pAdcId);

   unsigned long baseAddr;
   unsigned long eePromAddr;
   int apBusAddr;
   int vector;
   
   char tmpstr[80];
   register ADC_OBJ *pAdcObj;
   short sr;
   long dspcoef;

   baseAddr = ADC_BASE_ADR + adcChannel * 0x80000;
   eePromAddr = baseAddr | 0xe0000000L;
   apBusAddr = ADC_AP_ADR1A + adcChannel * 0x20;
   vector = ADC_ITRP_VEC + adcChannel * 2;

  DPRINT4(1,"VME Base: 0x%lx, VME EEprom: 0x%lx, AP: 0x%x, vector: 0x%x\n",
	      baseAddr,eePromAddr,apBusAddr,vector);
  /* ------- malloc space for FIFO Object --------- */
  if ( (pAdcObj = (ADC_OBJ *) malloc( sizeof(ADC_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"adcCreate: Could not Allocate Space:");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pAdcObj,0,sizeof(ADC_OBJ));

  pAdcObj->index = adcChannel;

  /* ------ Translate Bus address to CPU Board local address ----- */
  if (sysBusToLocalAdrs(FIFO_VME_ACCESS_TYPE,
              ((long)baseAddr & 0xffffff),&(pAdcObj->adcBaseAddr)) == -1)
  {
    errLogRet(LOGIT,debugInfo,
       "adcCreate: Can't Obtain Bus(0x%lx) to Local Address.",
	  baseAddr);
    adcDelete(pAdcObj);
    return(NULL);
  }

  /* DPRINT2(-11,"Local Base: 0x%lx, Local EEprom: 0x%lx\n",
	      pAdcObj->adcBaseAddr,eePromAddr); */

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
  sprintf(tmpstr,"%s %d",IDStr,adcChannel+1);
  pAdcObj->pIdStr = (char *) malloc(strlen(tmpstr)+2);

  if (pAdcObj->pIdStr == NULL)
  {
     adcDelete(pAdcObj);
     errLogSysRet(LOGIT,debugInfo,
	"adcCreate: IdStr - Could not Allocate Space:");
     return(NULL);
  }
  strcpy(pAdcObj->pIdStr,tmpstr);

  pAdcObj->pSID = SCCSid;	/* SCCS ID */

  /* ------ Test for Board's Presence ---------- */
  if ( vxMemProbe((char*) (pAdcObj->adcBaseAddr + ADC_SR), 
		     VX_READ, 2, &sr) == ERROR)
  { 
    DPRINT2(1,
	    "adcCreate: Could not read ADC's Status register(0x%lx), Board 0x%lx NOT Present\n",
	    (pAdcObj->adcBaseAddr + ADC_SR), pAdcObj->adcBaseAddr);
    adcDelete(pAdcObj);
    return(NULL);
    /*
    if (adcChannel > 0)
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
    */
  }

  /* ----- reset board and get status register */
  adcReset(pAdcObj); /* reset board */
  pAdcObj->adcState = *ADC_STATR(pAdcObj->adcBaseAddr);

  /* ----- Obtain board ID from EEPROM ------ */
  /* 500 KHz ADC = 3001 */
  pAdcObj->adcBrdVersion = getEEpromBrdId(eePromAddr);

  /* If EEPROM doesn't give the right type back then force it to a proper type
     (the EEPROM reading has a special VME AM code which doesn't always work out.)
      3001 it's the only valid type of ADC for Inova 4/25/01 gmb 
  */
  if ( pAdcObj->adcBrdVersion != 3001 )
  {
        DPRINT2(-1,"adcCreate: ADC Board Unknown Type: %d Force to 3001 - '500KHz',  @ VME addr: 0x%lx.\n",
		pAdcObj->adcBrdVersion,pAdcObj->adcBaseAddr);
	pAdcObj->adcBrdVersion = 3001;   
  }
  else
  {
        DPRINT2(-1,"adcCreate: ADC Board Type: %d - '500KHz' present @ VME addr. 0x%lx.\n",
		pAdcObj->adcBrdVersion,pAdcObj->adcBaseAddr);
  }

/*
#ifdef PROM_INSTALLED
  pAdcObj->adcBrdVersion = FF_REG(pAdcObj->adcBaseAddr,0);
  DPRINT2(1,"adcCreate: ADC Board Version %d (0x%x) present.\n",
		pAdcObj->adcBrdVersion,pAdcObj->adcBrdVersion);
#else
  pAdcObj->adcBrdVersion = 0xff;
  DPRINT2(1,"adcCreate: Skipped PROM, made ADC Board Version %d (0x%x)\n",
		pAdcObj->adcBrdVersion,pAdcObj->adcBrdVersion);
#endif
*/

  /* obtain status register which indicates if DSP is present */
  pAdcObj->optionsPresent = *ADC_STATR(pAdcObj->adcBaseAddr);

  /*adcConfigDsp( pAdcObj );  Do not configure DSP here; the DSP board is not ready yet */
  /* DSP is configured in getconf.c */

  /*------ Disable all interrupts on board -----------*/
  adcItrpDisable(pAdcObj,ADC_ALLITRPS);

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

   return( pAdcObj );
}

/* call from getconf.c */
int
adcConfigDsp( ADC_ID pAdcId )
{
	int	dspcoef;
	char	promVersion[ 5 ];

	if (pAdcId == NULL)
	  return( -1 );
	if (pAdcId->adcBaseAddr == 0xFFFFFFFF)
	  return( -1 );

        /* printf(" adcConfigDsp: Adc: 0x%lx, check for DSP  ",pAdcId->adcBaseAddr); */
  /* double check (via addr) if DSP board is present */

	if (vxMemProbe( (char *) (pAdcId->adcBaseAddr), VX_READ, 4, &dspcoef ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
                printf("\n    Adc: 0x%lx Addr READ failure indicates No DSP\n",pAdcId->adcBaseAddr);
		return( 0 );
	}

	if (vxMemProbe( (char *) (pAdcId->adcBaseAddr), VX_WRITE, 4, &dspcoef ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
                printf("\n    Adc: 0x%lx Addr WRITE failure indicates No DSP\n",pAdcId->adcBaseAddr);
		return( 0 );
	}

        /* besure string null terminator is present*/
        memset(promVersion,0,sizeof( promVersion));

	if (vxMemProbe(
		(char *) (pAdcId->adcBaseAddr + DSP_PROM_VERSION_OFFSET),
			  VX_READ,
		(sizeof( promVersion ) - 1),
			 &promVersion[ 0 ]
       ) == ERROR) {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DSP;
                printf("\n    Adc PROM: 0x%lx READ failure indicates a problem with the DSP board.\n",
			(pAdcId->adcBaseAddr+DSP_PROM_VERSION_OFFSET));
		return( 0 );
	}

        promVersion[4]=0; /* absolutely sure string null terminator is present*/
	if (strncmp( &promVersion[ 0 ], "PROM", (sizeof( promVersion ) - 1) ) == 0) {
		pAdcId->dspPromType = DSP_PROM_DOWNLOAD;
		pAdcId->dspDownLoadAddr = (void *) (pAdcId->adcBaseAddr + DSP_COEF_OFFSET);
                printf(", DSP accepts sw download,  PROM content: '%s'\n",promVersion);
	}
	else {
		pAdcId->dspDownLoadAddr = (void *) 0xFFFFFFFF;
		pAdcId->dspPromType = DSP_PROM_NO_DOWNLOAD;
                printf(", DSP does not accept sw download, PROM content: '%s'\n",promVersion);
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

  adcstatus = *ADC_ISTATR(pAdcId->adcBaseAddr); /* read status register */

  pAdcId->adcOvldFlag = FALSE;
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
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;

   /* enable when bit is set to 1 */
   *ADC_CNTRL(pAdcId->adcBaseAddr) =  
		( (*ADC_CNTRL(pAdcId->adcBaseAddr) | mask) & 0x00FE);
   /* IMPORTANT: bit 0 (1) if mis read disables the dsp 
      the PPC misreads it occasionally - eliminate the problem */
	/* Oops, read cntrl reg only gives byte, upper byte junk,
	   so mask this out, so that we don't reset,etc. the board */
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
   if ( (pAdcId == NULL) || (pAdcId->adcBaseAddr == 0xFFFFFFFF))
      return;

   /* disable when bit is unset (0)*/
   *ADC_CNTRL(pAdcId->adcBaseAddr) = 
	    ( (*ADC_CNTRL(pAdcId->adcBaseAddr) & ~mask) & 0x00FE);
   /* IMPORTANT: bit 0 (1) if mis read disables the dsp 
      the PPC misreads it occasionally - eliminate the problem */
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

   *ADC_CNTRL(pAdcId->adcBaseAddr) =  /* clear all interrupts 1st */
	    ( (*ADC_CNTRL(pAdcId->adcBaseAddr) & ~ADC_ALLITRPS) & 0x00FF );
   *ADC_CNTRL(pAdcId->adcBaseAddr) = 
		((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) | ADC_RESET);
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

    return ((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0xff));
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

    return (*ADC_CNTRL(pAdcId->adcBaseAddr) & ADC_ALLITRPS);
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
   *ADC_CNTRL(pAdcId->adcBaseAddr) =  
	    ((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) & ~ADC_READ_IMGREG);
   *real = *ADC_READ(pAdcId->adcBaseAddr);
   *ADC_CNTRL(pAdcId->adcBaseAddr) = 
		((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) | ADC_READ_IMGREG);
   *imag = *ADC_READ(pAdcId->adcBaseAddr);
   *ADC_CNTRL(pAdcId->adcBaseAddr) =  
	    ((*ADC_CNTRL(pAdcId->adcBaseAddr) & 0x00FF) & ~ADC_READ_IMGREG);
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
  *(pCodes+2)= 0L | (( (AP_HW_MIN_DELAY_CNT-HW_DELAY_FUDGE) & 0x0fffffff) >> 5);  /* hw */
  *(pCodes+3) = (0xff & (AP_HW_MIN_DELAY_CNT-HW_DELAY_FUDGE)) << 27;   /* lw */

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

   delay = AP_HW_MIN_DELAY_CNT - HW_DELAY_FUDGE;
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
*  adcGenCntrl2Cmds - Generate Fifo Cmds to set high word in ADC's 
*			Control Reg.
*   
*  Generates the appropriate Fifo Cmds to set the high word in the 
*  Ap Control register.  This high word includes CTC, OVRLD enables.
*  High Speed Line setting are not included.
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int adcGenCntrl2Cmds(ADC_ID pAdcId, unsigned long cntrl, unsigned long *pCodes)
/* pAdcId - ADC Object identifier */
/* cntrl - Source Address */
/* pCodes - Array to put genetated codes. (fifo words) */
{
/*
   ushort *shortfifoword;
   ulong_t fifoword;
*/
 
   if (pAdcId == NULL)
      return(-1);

/*
 DPRINT3( -1, "adcGenCntrl2Cmds: addr: 0x%x, offset: 0x%x, data: 0x%lx\n", 
	pAdcId->ApBusAddr, ADC_AP_CNTRL2, cntrl );
 shortfifoword = (short *) &fifoword;
 shortfifoword[0] = (unsigned short) (pAdcId->ApBusAddr | ADC_AP_CNTRL2);
 shortfifoword[1] = (unsigned short) (cntrl >> 16);
*/

   pCodes[0] = CL_AP_BUS;
   pCodes[1] = ((pAdcId->ApBusAddr | ADC_AP_CNTRL2) << 16) | ((cntrl >> 16) & 0xffff);
   pCodes[2] = CL_DELAY;
   pCodes[3] = AP_HW_MIN_DELAY_CNT;

   /* DPRINT2( -1, "adcGenCntrl2Cmds: pCode[1]: 0x%lx, fifoword: 0x%lx\n", pCodes[1],fifoword); */

   return(4);
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
   delay = AP_HW_MIN_DELAY_CNT - HW_DELAY_FUDGE;

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

void
adcShowEm(int level /*NOT USED*/)
{
    int i;
    for (i=0; i < MAX_ADC_OBJECTS; i++)
    {
	if ( AdcList[i] != NULL)
	{
	    printf("\n\n  >>>>>>>>>>>>>>  ADC[%d] of %d  <<<<<<<<<<<<<< \n",
		   i, nAdcsPresent);
	    adcShow(AdcList[i], level);
	}
    }
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
/* int level 	   - level of information (Not Used) */
{
   int i;
   char *pstr;
   unsigned short status,rstatus;

   if (pAdcId == NULL)
   {
     printf("adcShow: ADC Object pointer is NULL.\n");
     return;
   }
   printf("\n\n-------------------------------------------------------------\n\n");
   if (pAdcId->adcBaseAddr == 0xFFFFFFFF)
     printf(">>>>>>>>>>  ADC Board NOT Present, ADC Object for Testing. <<<<<<<<<<<\n\n");
   printf("ADC Object: Board Addr: 0x%lx, '%s'\n",pAdcId->adcBaseAddr, pAdcId->pIdStr);

   printf("SCCS ID: '%s'\n",pAdcId->pSID);
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

/* Create ADC objects for all ADC cards in the system.
 * Sets file global: array adcList[] with pointers to ADC objects.
 * Returns: number of ADCs found.
 */
#ifdef XXXX
int instantiateADCs()
{
    int i;

    for (i=0; i<MAX_ADC_OBJECTS; i++){
	AdcList[i] = adcCreate(i); /* Which ADC address */
	/*AdcList[i] = adcCreate(i, /* Which ADC address */
	/*		       2, /* Interrupt level */
	/*		       NULL); /* Use default ID string */
	if (AdcList[i] == NULL || AdcList[i]->adcBaseAddr == 0xFFFFFFFF){
	    break;		/* If any ADC missing, look no further */
	}
	nAdcsPresent++;
    }
    return nAdcsPresent;
}
#endif

int adcPutAdcObjByIndex(ADC_ID pAdcObj, int index)
{
   if (index < MAX_ADC_OBJECTS && index >= 0)
   {
     AdcList[index] = pAdcObj;
     if (pAdcObj != NULL)
        nAdcsPresent++;
   }
}

/* Return pointer to ADC object for board at the n'th ADC address.
 * Error Return: NULL if no board or index out of range.
 */
ADC_ID adcGetAdcObjByIndex(int index)
{
    if (index < MAX_ADC_OBJECTS && index >= 0){
	return AdcList[index];
    }else{
	return NULL;
    }
}

/*
 * Return pointer to first active adcObject with the index greater
 * than "*pindex".  "activeRcvrs" points to the lc table entry
 * indicating which receivers are active.  "*pindex" gets incremented,
 * to facilitate iterating through all active ADCs.
 */
ADC_ID
adcGetActive(ulong_t *activeRcvrs, int *pindex)
{
    ulong_t adcMask = *activeRcvrs;
    int i;

    /* Find next Rcvr in the mask */
    i = *pindex >= 0 ? *pindex + 1 : 0;
    for (adcMask >>= i; adcMask && (i < MAX_STM_OBJECTS); adcMask >>= 1, i++) {
	if (adcMask & 1) {
	    *pindex = i;
	    DPRINT1(3,"Return ADC #%d\n", i);
	    return adcGetAdcObjByIndex(i);
	}
    }
    return NULL;
}

int
adcNumberOfAdcs()
{
    return nAdcsPresent;
}
