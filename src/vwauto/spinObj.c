/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* spinObj.c  - Spin Object Source Modules */
#ifndef LINT
#endif
/* 
 */


#define _POSIX_SOURCE	/* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
#include <taskLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "logMsgLib.h"
#include "evs332.h"
#include "spinObj.h"
#include "hostAcqStructs.h"
#include "m32hdw.h"

#include "m68332_nmri.h"

#define STETHOSCOPE

#define  SET_SPN_NOT_REG   0x01  /* Set Spin Not Regulated bit */
#define  SET_SPN_SPD_ZRO   0x02  /* Set Spin Speed Zero bit */

/*
modification history
--------------------
1-30-95,gmb  created 
9-25-95,glen_d/gmb  integrated both Liquids & Solids spin control 
*/

/* interrupt vectors, level, and arbitration defines */

#define INT_VEC_SIM		0x40
#define INT_VEC_QSM		0x42
#define INT_VEC_SCI		0x42
#define INT_VEC_SPI		0x43
#define INT_VEC_TPU		0x50

#define	INT_LVL_SIM		6
#define	INT_LVL_SPI		5
#define	INT_LVL_SCI		4
#define	INT_LVL_TPU		6

#define	IARB_SIM		5
#define	IARB_QSM		11
#define	IARB_TPU		12

/*
	TPU Object Routines

	Interrupt Service Routines (ISR)

*/

#define SpinLED_STOP 0	/* Console Display LED OFF */
#define SpinLED_BLINKshort 1	/* Console Display LED BLINK ON short */
#define SpinLED_BLINKlong 2	/* Console Display LED BLINK ON long */
#define SpinLED_OK 3	/* Console Display LED ON "Regulating" */

#define	AUTO_CNTRL_STAT_REG ((unsigned char *) 0x20100C)

/* extern FUNCPTR intHandlerCreate(); */
extern SPIN_ID pTheSpinObject;
extern MSG_Q_ID pMsgesToPHandlr;

#define	TPU_INT_VECT_NUM 0x50
#define	V_INT_VECT_NUM 0x52
#define	MAS_INT_VECT_NUM 0x53
#define TICRinit 0x600 | TPU_INT_VECT_NUM
/* use level 6 for now and int no 0x50 (80 - 0x140) */

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

extern int shimDebug;

static SPIN_OBJ *SpinObj;
static char *SpinIDStr ="TPU Object";
static int  IdCnt = 0;
static long SpeedError;

/* for RMS error calculations, for testing */
static double RMSerror;
static double ErrSum;
static ulong_t sumdiv;

static WDOG_ID wdSpinChk;

#ifdef SCOPE
static long GlobalOffTime;
#endif

/*-----------------------------------------------------------
|
|  Internal Functions
|
+---------------------------------------------------------*/
/*-------------------------------------------------------------
| Interrupt Service Routine (ISR) 
+--------------------------------------------------------------*/
/*******************************************
*
* Xrecord - Interrupt Service Routine
*
*   Xrecord - Signals the receipt of an unused interrupt
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void Xrecord(register SPIN_ID pSpinId)
{

	*M332_TPU_CISR ^= pSpinId->SPNitrpmask;	/* clear interrupt request */
	pSpinId->BooBooheart++;	/* note we were here */
	return;
}


/*******************************************
*
* SpinUpdate - Interrupt Service Routine
*
*   SpinUpdate - Signals the completion of a TPU block count.
*                Averges 16 measurements
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void SpinUpdate(register SPIN_ID pSpinId)
{
register long accum;
register long ndx;
register long x;

	if(pSpinId->SPNtype == VERTICAL_PROBE)
	{
		accum = (long)M332_TPU_CHN2->tf.pta.hiLoWord;	/* get current period */
		M332_TPU_CHN2->tf.pta.hiLoWord = pSpinId->Zero;	/* reset channel */
	}
	else	/* must be MAS */
	{
		accum = (long)M332_TPU_CHN3->tf.pta.hiLoWord;	/* get current period */
		M332_TPU_CHN3->tf.pta.hiLoWord = pSpinId->Zero;	/* reset channel */
	}
	*M332_TPU_CISR ^= pSpinId->SPNitrpmask;	/* clear interrupt request */

		/* Now process data */
	ndx = pSpinId->SPNindex;            /* get index to new/nth value */
	x = pSpinId->SPNperiods[ndx];       /* fetch -nth value (ndx_max = 16) */
	pSpinId->SPNsum -= x;               /* subtract it from sum */
	pSpinId->SPNsum += accum;           /* new sum */
	pSpinId->SPNperiods[ndx++] = accum; /* save new value & point to next */

	if( ndx >= pSpinId->SPNnbr)         /* if at end reset else next cell */
	{
		pSpinId->SPNcount += 1;           /* count times thru array */
		ndx = 0;                           /* reset index to 0 */
	}
	pSpinId->SPNindex = ndx;            /* update index */
	pSpinId->SPNheartbeat++;            /* update interrupt rtn heartbeat */
	return;
}


/*-------------------------------------------------------------
| TPU Object Public Interfaces
+-------------------------------------------------------------*/

initVertical()
{
register SPIN_OBJ *pSpinObj;
register short qomValue;
void initLED();

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;

/* kill all spin functions before seting up for Vertical */
	spinReset();	/* kill all Probe specific functions */

/* ------- set type before going any further */

	pSpinObj->SPNtype = VERTICAL_PROBE;

/* Set defaults for P, I, D and O plus tolerance */
	pSpinObj->SPNkO = 1;
	pSpinObj->SPNkF = 1;

	pSpinObj->SPNkP[UPk] = 1; 
	pSpinObj->SPNkP[DOWNk] = 1;
	pSpinObj->SPNkD[UPk] = 15;
	pSpinObj->SPNkD[DOWNk] = 15;
	pSpinObj->SPNkI[UPk] = 0x7FFFFFFF;
	pSpinObj->SPNkI[DOWNk] = 0x7FFFFFFF;

	pSpinObj->SPNtolerance = 1000;  /* 1 Hz */
	pSpinObj->SPNsetting = 8916; /* .8916 % duty cycle */
	pSpinObj->SPNrateSet = 8916;    /* Air Flow Rate to Go to,  value depends LIQ or MAS */
        pSpinObj->SPNmode = RATE_MODE;	    /* in Rate (air flow) or Speed regualtion mode */
	pSpinObj->PIDerrindex = 0;  /* index into error buffer */
	pSpinObj->PIDerrorbuf[0] = 0L;
	pSpinObj->PIDerrorbuf[1] = 0L;
	pSpinObj->PIDerrorbuf[2] = 0L;
	pSpinObj->PIDerrorbuf[3] = 0L;

	pSpinObj->SPNpefact = 1L;

	pSpinObj->SPNkC[UPk] = 1000000;	/* at 1000000 same as off */
	pSpinObj->SPNkC[DOWNk] = -1000000;

/* Set default speed window */
	pSpinObj->SPNwindow = V_WINDOW;

/* TPU channels 0 and 1 are unused */

/* The TPU Channels may use either TC1 (clock 1 4 MHz (250 nsec) (i.e. 16MHz/4) */
/*                             or  TC2 (clock 2, 250KHz (4 usec)  i.e. 16MHz/(8*8))
/* set up TPU channel 2 as a Programmable Time Accumulator (PTA) function */
/* This TPU channel is used to determine Vertical spinner speed */
/* measure period on rising edge with TCR1 */
/* TPU channel uses TC1 (clock 1, 4 MHz (250 nsec) (i.e. 16MHz/4) */

	M332_TPU_CHN2->tf.pta.chnCont = VCC;
		/* count 2 periods = 1 revolution */
	M332_TPU_CHN2->tf.pta.maxPeriodCount = VMC_PC;
	M332_TPU_CHN2->tf.pta.accum = pSpinObj->Zero;	/* start with 0 */
	M332_TPU_CHN2->tf.pta.hiLoWord = pSpinObj->Zero;	/* start with 0 */
	pSpinObj->SPNrevs = VMC_REVS; /* set revs for Vertical */

/* point active interrupt at ISR */
	intVecSet(pSpinObj->V_ItrVect, pSpinObj->SPNitrHndlr);
	intVecSet(pSpinObj->MAS_ItrVect, pSpinObj->XitrHndlr);

/* Set up Channel Function Select Register */
	*M332_TPU_CFSR3 |= (TPU_CFSR_PTA << 8);	/* set up channel 2 */
/* and Host Sequence Regester */
	*M332_TPU_HSQR1 |= (PTA_HSQR_PER_RISE << 4);	/* channel 2 */
/* now set up Host Service Request Register */
	*M332_TPU_HSRR1 = (PTA_HSRR_INIT << 4);	/* channel 2 */
/* Setting priority will cause the service request to start */
	*M332_TPU_CPR1 |= (TPU_CPR_HIGH << 4);	/* channel 2 */
/* Wait for request to finish */
	while ( *M332_TPU_HSRR1 & 0x0030 );
/* Clear interrupt request flag generated from HSSR request */
	*M332_TPU_CISR ^= TPU_CIER_CISR_CH2;
	pSpinObj->SPNitrpmask = TPU_CIER_CISR_CH2;
/* set TPU interrupt ON channel 2 */
	*M332_TPU_CIER |= TPU_CIER_CISR_CH2;
	*M332_TPU_TICR = TICRinit; /* set interrupt level */

/* TPU channel 4 is a Queued Output Match (QOM) function */
/* This TPU channel is used to control the spinner regulating LED */
/* set up in Create! */

/* TPU channel 3 is a Programmable Time Accumulator (PTA) function */
/* This TPU channel is used to determine MAS spinner speed */

/* TPU channels 5 thru 13 are unused. */

/* set up TPU channel 14-15 as an extended Queued Output Match (QOM) function */
/* This TPU channel is used to control the Vertical spinner air solenoid */
	qomValue = (pSpinObj->QOM_500ms / pSpinObj->QOMoffRegs) << 1;
	M332_TPU_CHN14->tf.qom14.refLastOffAdrs = 0x00f8 + QOM_TCR2;
	M332_TPU_CHN14->tf.qom14.loopOffPtr = 0;
	M332_TPU_CHN14->tf.qom14.offset1 = ( pSpinObj->QOM_9ms << 1);
	M332_TPU_CHN14->tf.qom14.offset2 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset3 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset4 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset5 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset6 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset7 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset8 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset9 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset10 = qomValue;
	M332_TPU_CHN14->tf.qom14.offset11 = qomValue;	/* leave off till speed set */
	M332_TPU_CHN14->tf.qom14.offset12 = 0;	/* clear unused cells for debug */
	M332_TPU_CHN14->tf.qom14.offset13 = 0;
	M332_TPU_CHN14->tf.qom14.offset14 = 0;

/* Insure T2CG is ON (This is redundant but may not be in future) */
	*M332_TPU_TMCR |= TPU_TMCR_T2CG;
/* Set up Channel Function Select Register */
	*M332_TPU_CFSR0 |= (TPU_CFSR_QOM << 8);	/* set up channel 14 */
/* and Host Sequence Regester */
	*M332_TPU_HSQR0 |= (QOM_HSQR_CONT3 << 12);	/* channel 14 */
/* now set up Host Service Request Register */
	*M332_TPU_HSRR0 = (QOM_HSRR_INIT_PINLO << 12);	/* channel 14 */
/* Setting priority will cause the service request to start */
	*M332_TPU_CPR0 |= (TPU_CPR_MEDIUM << 12);	/* channel 14 */
/* Wait for request to finish */
	while ( *M332_TPU_HSRR0 & 0x3000 );
/* Clear interrupt request flag generated from HSSR request */
	*M332_TPU_CISR ^= TPU_CIER_CISR_CH14;

   /* set up spinner LED */
   initLED();

   /* set eegs as to not equal heartbeart, give interrupt routine a chance to start up */
   pSpinObj->SPNheartbeat = 1;
   pSpinObj->SPNeeg1 = 200;
   pSpinObj->SPNeeg2 = 300;

} /* end initVertical */

/**************************************************************
*
*  initMAS sets up TPU functions for MAS probes
*
*
* RETURNS:
*  None.
*
*/ 
void initMAS()
{
register SPIN_OBJ *pSpinObj;
void initLED();

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;

/* kill all spin functions before seting up for MAS */
	spinReset();	/* kill all Probe specific functions */

/* ------- set type before going any further */

	pSpinObj->SPNtype = MAS_PROBE;

	/* for MAS setting is log*10000 of DAC setting */
        /* e.g. 41230 = 4.123 -> 10^4.123 = 13273.94 dac air setting */
	pSpinObj->SPNsetting = 30000;  /* 10^3 -> 1000 */
	pSpinObj->SPNrateSet = 0;    /* Air Flow Rate to Go to,  value depends LIQ or MAS */
        pSpinObj->SPNmode = RATE_MODE;	    /* in Rate (air flow) or Speed regualtion mode */
	pSpinObj->PIDerrindex = 0;  /* index into error buffer */
	pSpinObj->PIDerrorbuf[0] = 0L;
	pSpinObj->PIDerrorbuf[1] = 0L;
	pSpinObj->PIDerrorbuf[2] = 0L;
	pSpinObj->PIDerrorbuf[3] = 0L;


/* Set defaults for P, I, D and O plus tolerance */
/* these pid work well for both 5mm & 7mm Varian probes, 
   7mm there is a slight overshoot, 5mm there is no overshoot
   decrease Proportional sensitivity (e.g. > 20) will loss ability to regulate well
   increase Proportional sensitivity (e.g. < 20) will increase Derivitive, which 
   creates substantial ringing
*/
	pSpinObj->SPNkP[UPk] = 20;
	pSpinObj->SPNkP[DOWNk] = 20;
	pSpinObj->SPNkD[UPk] = 40;
	pSpinObj->SPNkD[DOWNk] = 40;
	pSpinObj->SPNkI[UPk] = 0x7FFFFFFF;
	pSpinObj->SPNkI[DOWNk] = 0x7FFFFFFF;
	pSpinObj->SPNkO = 1;
	pSpinObj->SPNkF = 100;
/*
	pSpinObj->SPNkF = 10;
	pSpinObj->SPNkI[UPk] = 5;
	pSpinObj->SPNkI[DOWNk] = 5;
	pSpinObj->SPNkD[UPk] = 2;
	pSpinObj->SPNkD[DOWNk] = 2;
*/
        /* 2 Hz RMS spec,'in=y' regulation crit 100 Hz*/
	pSpinObj->SPNtolerance = 100000; 
	pSpinObj->SPNpefact = 1000L;
	pSpinObj->SPNkC[UPk] = 250;
	pSpinObj->SPNkC[DOWNk] = -250;

/* Set default speed window */
	pSpinObj->SPNwindow = MAS_WINDOW;

/* TPU channels 0 and 1 are unused */

/* TPU channel 2 is a Programmable Time Accumulator (PTA) function */
/* This TPU channel is used to determine Vertical spinner speed */

/* set up TPU channel 3 as a Programmable Time Accumulator (PTA) function */
/* TPU channel uses TC1 (clock 1, 4 MHz (250 nsec) (i.e. 16MHz/4) */
/* This TPU channel is used to determine MAS spinner speed */
		/* measure period on rising edge with TCR1 */

	M332_TPU_CHN3->tf.pta.chnCont = MASCC;
		/* count 250 periods = 250 revolution */
	M332_TPU_CHN3->tf.pta.maxPeriodCount = MASMC_PC;
	M332_TPU_CHN3->tf.pta.accum = pSpinObj->Zero;	/* start with 0 */
	M332_TPU_CHN3->tf.pta.hiLoWord = pSpinObj->Zero;	/* start with 0 */
	pSpinObj->SPNrevs = MASMC_REVS; /* set revs for MAS */

/* point active interrupt at ISR */
	intVecSet(pSpinObj->MAS_ItrVect, pSpinObj->SPNitrHndlr);
	intVecSet(pSpinObj->V_ItrVect, pSpinObj->XitrHndlr);

/* Set up Channel Function Select Register */
	*M332_TPU_CFSR3 |= (TPU_CFSR_PTA << 12);	/* set up channel 3 */
/* and Host Sequence Regester */
	*M332_TPU_HSQR1 |= (PTA_HSQR_PER_RISE << 6);	/* channel 3 */
/* now set up Host Service Request Register */
	*M332_TPU_HSRR1 = (PTA_HSRR_INIT << 6);	/* channel 3 */
/* Setting priority will cause the service request to start */
	*M332_TPU_CPR1 |= (TPU_CPR_HIGH << 6);	/* channel 3 */
/* Wait for request to finish */
	while ( *M332_TPU_HSRR1 & 0x00c0 );
/* Clear interrupt request flag generated from HSSR request */
	*M332_TPU_CISR ^= TPU_CIER_CISR_CH3;
	pSpinObj->SPNitrpmask = TPU_CIER_CISR_CH3;
/* set TPU interrupt ON channel 3 */
	*M332_TPU_CIER |= TPU_CIER_CISR_CH3;
	*M332_TPU_TICR = TICRinit; /* set interrupt level */

/* TPU channel 4 is a Queued Output Match (QOM) function */
/* This TPU channel is used to control the spinner regulating LED */
/* set up in Create! */

/* TPU channels 5 thru 13 are unused. */

/* TPU channel 14-15 is an extended Queued Output Match (QOM) function */
/* This TPU channel is used to control the Vertical spinner air solenoid */

   /* set up spinner LED */
   initLED();

   /* set eegs as to not equal heartbeart, give interrupt routine a chance to start up */
   pSpinObj->SPNheartbeat = 1;
   pSpinObj->SPNeeg1 = 200;
   pSpinObj->SPNeeg2 = 300;
   cntrlRegClear(SET_BEAR_AIR_ON);

} /* end initMAS() */

/**************************************************************
*
*  initMLED sets up TPU functions for spin indicator L.E.D.s 
*
*
* RETURNS:
*  None.
*
*/ 
void initLED()
{
   /* set up TPU channel 4 as a Queued Output Match (QOM) function */
   /* This TPU channel is used to control the spinner regulating LED */
	M332_TPU_CHN4->tf.qom.refLastOffAdrs = 0x004b | QOM_TCR2;
	M332_TPU_CHN4->tf.qom.loopOffPtr = 0;
	M332_TPU_CHN4->tf.qom.offset1 = 0xbfff; /* LED ON */
	M332_TPU_CHN4->tf.qom.offset2 = 0xbffe; /* LED OFF */
	M332_TPU_CHN4->tf.qom.offset3 = 0xfffe; /* LED OFF */
	M332_TPU_CHN4->tf.qom.offset4 = 0xffff; /* LED ON */

   /* Insure T2CG is ON */
	*M332_TPU_TMCR |= TPU_TMCR_T2CG;

   /* Set up Channel Function Select Register */
	*M332_TPU_CFSR2 |= (TPU_CFSR_QOM );	/* set up channel 4 */

   /* and Host Sequence Regester */
	*M332_TPU_HSQR1 |= (QOM_HSQR_CONT3 << 8);	/* channel 4 */

   /* now set up Host Service Request Register */
	*M332_TPU_HSRR1 = (QOM_HSRR_INIT_PINLO << 8);	/* channel 4 */

   /* Setting priority will cause the service request to start */
	*M332_TPU_CPR1 |= (TPU_CPR_LOW << 8);	/* channel 4 */

   /* Wait for request to finish */
	while ( *M332_TPU_HSRR1 & 0x0300 );

   /* Clear interrupt request flag generated from HSSR request */
	*M332_TPU_CISR ^= TPU_CIER_CISR_CH4;
}

/**************************************************************
*
*  spinCreate - Create the TPU Object Data Structure & Semiphores
*
*  SPINtype = 1 for Vertical sample control, 2 for MAS control.
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semiphore creation failed
*
*/ 
SPIN_ID  spinCreate(int SPINtype)
{
   void SpinUpdate(SPIN_ID pSpinId);

   char tmpstr[80];
   register SPIN_OBJ *pSpinObj;
   short sr;
   int psck;

    /* create the Watch Dog, that is used to Chk on progress of regulating */
    if ((wdSpinChk = wdCreate()) == NULL)
    {
        DPRINT(-1,"wdCreate Error\n");
    }
    

   /* ------- malloc space for TPU Object --------- */
   if ( (pSpinObj = (SPIN_OBJ *) malloc( sizeof(SPIN_OBJ)) ) == NULL )
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Space:");
      return(NULL);
   }

   /* zero out structure so we don't free something by mistake */
   memset(pSpinObj,0,sizeof(SPIN_OBJ));

   /* set the global pointer before calling initVertical, initMAS or spinReset etc. */
   pTheSpinObject = pSpinObj;

   pSpinObj->SPNnbr=SPNperiodSize;

   /* ------- point to Id String ---------- */

   pSpinObj->pIdStr = SpinIDStr;

   /* ------- set type before going any further */

   pSpinObj->SPNtype = SPINtype;
   pSpinObj->SPNmasthres = MAS_THRESHOLD;	/* Above 100 Hz (default) switch to MAS control */

   /* ------- reset board and get status register */

   spinReset();	/* reset TPU port */

   /* ------- Disable all TPU interrupts on board -----------*/

   spinItrpDisable();

   /* ------- Clear MAS DAC till set by program control ---- */

   *(unsigned short *) MPU332_SOLID_DAC = pSpinObj->MASdac;	/* clear struct left MASdac = 0 */

   /* ------- Connect interrupt vector to proper ISR ----- */

   pSpinObj->V_ItrVectNum = V_INT_VECT_NUM;
   pSpinObj->V_ItrVect = INUM_TO_IVEC( pSpinObj->V_ItrVectNum);
   pSpinObj->MAS_ItrVectNum = MAS_INT_VECT_NUM;
   pSpinObj->MAS_ItrVect = INUM_TO_IVEC( pSpinObj->MAS_ItrVectNum);

   /* make the active handler function */
   pSpinObj->SPNitrHndlr = intHandlerCreate (SpinUpdate, pSpinObj);
   if(pSpinObj->SPNitrHndlr == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,
            "spinCreate: Could not connect TPU SPN ISR: ");
      spinDelete();
      return(NULL);
   }

   /* make the inactive handler function */
   if ( (pSpinObj->XitrHndlr = (FUNCPTR)intHandlerCreate (Xrecord, pSpinObj)) == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,
                        "spinCreate: Could not connect TPU X ISR: ");
      spinDelete();
      return(NULL);
   }

   /*  Now set default number of registers to use for QOM */
   pSpinObj->QOMoffRegs = 10;

   /* Set defaults for drop delay etc */
   pSpinObj->Vdroptime = 240;	/* wait 4 sec between EJECT off and SDROP off */
   pSpinObj->Vdropdelay = 30;	/* wait 1/2 sec after SDROP till regulation */
   pSpinObj->PIDmode = 30; /* start here and experiment */

   /* Calculate TCR1 period times 10 for later use by all routines */

   psck = (*M332_TPU_TMCR & TPU_TMCR_PSCK_4X) ? 4 : 32;

   pSpinObj->TCR1x10period = 1000000000 /
     ( SYS_VCO_FREQ / ( (1 << ( (*M332_TPU_TMCR & TPU_TMCR_TCR1_8X) >> 13) ) * psck * 10L ) );

  /* Calculate TCR2 period times 10 for later use by all routines */
  pSpinObj->TCR2x10period = 1000000000 /
     ( SYS_VCO_FREQ / ((8 << ( (*M332_TPU_TMCR & TPU_TMCR_TCR2_8X) >> 11) ) * 10L));

  /*  Then calculate 500ms count for base spin speed off time */
  pSpinObj->QOM_500ms = 500000000 / ( pSpinObj->TCR2x10period / 10);

  /*	And also calculate 9ms for defauto ON time. */
  pSpinObj->QOM_9ms = 9000000 / ( pSpinObj->TCR2x10period / 10);

  /* TPU channels 0 and 1 are unused */

  switch (SPINtype)
  {
     /* TPU channel 2 is a Programmable Time Accumulator (PTA) function */
     /* This TPU channel is used to determine Vertical spinner speed */
     case VERTICAL_PROBE:
  		pSpinObj->SPNintegral = pSpinObj->QOM_500ms;		/* TODO */
		initVertical();
	break;

     /* TPU channel 3 is a Programmable Time Accumulator (PTA) function */
     /* This TPU channel is used to determine MAS spinner speed */
     case MAS_PROBE:
  		pSpinObj->SPNintegral = 0L;
		initMAS();
	break;
  }

/* TPU channels 5 thru 13 are unused. */

/* TPU channel 14-15 is an extended Queued Output Match (QOM) function */
/* This TPU channel is used to control the Vertical spinner air solenoid */
/*   set up in initVertical. */

#ifdef SCOPE
  printf("Install Scope Signals\n");
  ScopeInstallSignal("OffTime","Ticks Off",&GlobalOffTime,
			"int",0);
  ScopeInstallSignal("Setting","Duty_Cycle",&(pSpinObj->SPNsetting),
			"int",0);
  ScopeInstallSignal("SpeedError","mHz",&SpeedError,
			"int",0);
  ScopeInstallSignal("PropCorrection","Duty_Cycle",&(pSpinObj->SPNproportion),
			"int",0);
  ScopeInstallSignal("IntCorrection","Duty_Cycle",&(pSpinObj->SPNintegral),
			"int",0);
  ScopeInstallSignal("DervCorrection","Duty_Cycle",&(pSpinObj->SPNderivative),
			"int",0);
  ScopeInstallSignal("Correction","Duty_Cycle",&(pSpinObj->PIDcorrection),
			"int",0);
  ScopeInstallSignal("Speed_mHz","mHz",&(pSpinObj->SPNspeed),
			"int",0);
  ScopeInstallSignal("SetSpeed_mHz","mHz",&(pSpinObj->SPNspeedSet),
			"int",0);
  ScopeShowSignals(0);
  printf("TCR1x10: %ld (0x%lx), TCR2x10: %ld (0x%lx)\n",
	pSpinObj->TCR1x10period, pSpinObj->TCR1x10period,
	pSpinObj->TCR2x10period, pSpinObj->TCR2x10period);
  printf("Max Time (1sec): %ld (0x%lx), Min Time (9ms): %ld (0x%lx)\n",
          pSpinObj->QOM_500ms * 2, pSpinObj->QOM_500ms * 2,
          pSpinObj->QOM_9ms, pSpinObj->QOM_9ms);
#endif


  return( pSpinObj );
}


/**************************************************************
*
*  SpinDelete - Deletes TPU Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int spinDelete()
{
SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;
	spinReset(); /* kill interrupts and reset all TPU functions */
	free(pTheSpinObject);
}


/**************************************************************
*
*  SpinItrpEnable - Set the TPU Interrupt Mask
*
*  This routine sets the TPU interrupt mask equal to the system SCI level.
*
* RETURNS:
* void 
*
*/
void spinItrpEnable()
{
SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;

	*M332_TPU_CIER |= pSpinObj->SPNitrpmask;
	if(shimDebug > 10) printf("SpinItrpEnable:  = %x\n", *M332_TPU_CIER);
}


/**************************************************************
*
*  SpinItrpDisable - Set the TPU Interrupt Mask
*
*  This routine sets the TPU interrupt mask to 0.
*
* RETURNS:
* void 
*
*/
void spinItrpDisable()
{
SPIN_OBJ *pSpinObj;
int bitBucket;

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;

	*M332_TPU_CIER = pSpinObj->Zero;	/* kill interrupts */
	taskDelay(60); /* wait for any pending interrupt */
	bitBucket = *M332_TPU_CISR;	/* clear all interrupts */
	*M332_TPU_CISR = pSpinObj->Zero;

}


/**************************************************************
*
*  SpinReset - Resets TPU functions 
*
*
* RETURNS:
* 
*/
void spinReset()
{
SPIN_OBJ *pSpinObj;
int bitBucket;

	if (pTheSpinObject == NULL) return;
	/* else */
	pSpinObj = pTheSpinObject;

	*M332_TPU_CPR0 = pSpinObj->Zero;	/* disable all channels */
	*M332_TPU_CPR1 = pSpinObj->Zero;	/* disable all channels */
	*M332_TPU_HSQR0 = pSpinObj->Zero;	/* clear all functions */
	*M332_TPU_HSQR1 = pSpinObj->Zero;	/* clear all functions */
	spinItrpDisable();
	pSpinObj->SPNitrpmask = pSpinObj->Zero; /* clear interrupt mask */
}


/**************************************************************
*
*  getTPU_TMCR - Gets TPU Module Configuration Register value
*
*
* RETURNS:
*  16-bit TPU Module Configuration Register Value
*/
short getTPU_TMCR()
{
	if (pTheSpinObject == NULL) return(ERROR);

	return (*M332_TPU_TMCR);
}



/**************************************************************
*
*  getTPU_CIER - Gets TPU Channel Interrupt Enable Register mask
*
*
* RETURNS:
*  WhoKnows
*/
short getTPU_CIER()
{
	return (*M332_TPU_CIER);
}
short SpinIntrpMask()
{
	return (*M332_TPU_CIER);
}


/**************************************************************
*
*  getTPU_CISR - Gets TPU Channel Interrupt Status Register value
*
*
* RETURNS:
*  16-bit TPU Channel Interrupt Status Register Value
*/
short getTPU_CISR()
{
	return (*M332_TPU_CISR);
}


/**************************************************************
*
*  getTPU_TICR - Gets TPU Interrupt Configuration Register value
*
*
* RETURNS:
*  16-bit TPU Interrupt Configuration Register Value
*/
short getTPU_TICR()
{
	return (*M332_TPU_TICR);
}


/**************************************************************
*
*  getTPU_CFSR0 - Gets TPU Channel Function Select Register 0 value
*
*
* RETURNS:
*  16-bit TPU Channel Function Select Register 0 Value
*/
short getTPU_CFSR0()
{
	return (*M332_TPU_CFSR0);
}


/**************************************************************
*
*  getTPU_CFSR1 - Gets TPU Channel Function Select Register 1 value
*
*
* RETURNS:
*  16-bit TPU Channel Function Select Register 1 Value
*/
short getTPU_CFSR1()
{
	return (*M332_TPU_CFSR1);
}


/**************************************************************
*
*  getTPU_CFSR2 - Gets TPU Channel Function Select Register 2 value
*
*
* RETURNS:
*  16-bit TPU Channel Function Select Register 2 Value
*/
short getTPU_CFSR2()
{
	return (*M332_TPU_CFSR2);
}


/**************************************************************
*
*  getTPU_CFSR3 - Gets TPU Channel Function Select Register 3 value
*
*
* RETURNS:
*  16-bit TPU Channel Function Select Register 3 Value
*/
short getTPU_CFSR3()
{
	return (*M332_TPU_CFSR3);
}


/**************************************************************
*
*  getTPU_HSQR0 - Gets TPU Host Sequence Register 0 value
*
*
* RETURNS:
*  16-bit TPU Host Sequence Register 0 Value
*/
short getTPU_HSQR0()
{
	return (*M332_TPU_HSQR0);
}


/**************************************************************
*
*  getTPU_HSQR1 - Gets TPU Host Sequence Register 1 value
*
*
* RETURNS:
*  16-bit TPU Host Sequence Register 1 Value
*/
short getTPU_HSQR1()
{
	return (*M332_TPU_HSQR1);
}


/**************************************************************
*
*  getTPU_HSRR0 - Gets TPU Host Service Request Register 0 value
*
*
* RETURNS:
*  16-bit TPU Host Service Request Register 0 Value
*/
short getTPU_HSRR0()
{
	return (*M332_TPU_HSRR0);
}


/**************************************************************
*
*  getTPU_HSRR1 - Gets TPU Host Service Request Register 1 value
*
*
* RETURNS:
*  16-bit TPU Host Service Request Register 1 Value
*/
short getTPU_HSRR1()
{
	return (*M332_TPU_HSRR1);
}


/**************************************************************
*
*  getTPU_CPR0 - Gets TPU Channel Priority Register 0 value
*
*
* RETURNS:
*  16-bit TPU Channel Priority Register 0 Value
*/
short getTPU_CPR0()
{
	return (*M332_TPU_CPR0);
}


/**************************************************************
*
*  getTPU_CPR1 - Gets TPU Channel Priority Register 1 value
*
*
* RETURNS:
*  16-bit TPU Channel Priority Register 1 Value
*/
short getTPU_CPR1()
{
	return (*M332_TPU_CPR1);
}


/********************************************************
*
* getSpeed calculates the spin speed.
*
*
********************************************************/

int getSpeed()
{
register SPIN_OBJ *pSpinObj;
register long speed1000;
register int index;

	pSpinObj = pTheSpinObject;

	if( (pSpinObj->SPNperiod = pSpinObj->SPNsum / pSpinObj->SPNnbr) > 0L)
	{
		index = (int)pSpinObj->SPNindex;
		speed1000 = (1000000000L /
			( ( ( pSpinObj->SPNperiod /10L) * (pSpinObj->TCR1x10period / 10L) ) /
					pSpinObj->SPNrevs)) * 100L;

		pSpinObj->SPNspeedfrac = speed1000 % 1000L;
		pSpinObj->SPNspeed = speed1000;
		if(shimDebug > 0)
		{
			printf("sum = %ld, count = %ld, index = %ld, speed = %ld.%ld, last = %ld, \n",
				pSpinObj->SPNsum,pSpinObj->SPNcount,index,pSpinObj->SPNspeed,
					pSpinObj->SPNspeedfrac,pSpinObj->SPNperiods[index]);
		}
	}
	else
	{
		speed1000 = 0L;
		pSpinObj->SPNspeedfrac = 0L;
		pSpinObj->SPNspeed = 0L;
		if(shimDebug > 0) printf("Sum is Zero\n");
	}

	return((speed1000/100));

}	/* end getSpeed() */


int setPIDkPup(long PIDkPup)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkP[UPk] = PIDkPup;
	return(OK);
}


int setPIDkPdown(long PIDkPdown)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkP[DOWNk] = PIDkPdown;
	return(OK);
}


int setPIDkIup(long PIDkIup)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkI[UPk] = PIDkIup;
	return(OK);
}


int setPIDkIdown(long PIDkIdown)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkI[DOWNk] = PIDkIdown;
	return(OK);
}


int setPIDkDup(long PIDkDup)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkD[UPk] = PIDkDup;
	return(OK);
}


int setPIDkDdown(long PIDkDdown)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkD[DOWNk] = PIDkDdown;
	return(OK);
}


int setPIDkCup(long PIDkCup)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkC[UPk] = PIDkCup;
	return(OK);
}


int setPIDkCdown(long PIDkCdown)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkC[DOWNk] = PIDkCdown;
	return(OK);
}


int setPIDkO(long PIDkO)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNkO = PIDkO;
	return(OK);
}


int setPIDkF(long PIDkF)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	if(PIDkF < 1) PIDkF = 1;
	pSpinObj->SPNkF = PIDkF;
	return(OK);
}


int setPIDpefact(long pefact)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pSpinObj->SPNpefact = pefact;
	return(OK);
}


long getPIDkPup()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkP[UPk]);
}


long getPIDkPdown()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkP[DOWNk]);
}


long getPIDkIup()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkI[UPk]);
}


long getPIDkIdown()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkI[DOWNk]);
}


long getPIDkDup()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkD[UPk]);
}


long getPIDkDdown()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkD[DOWNk]);
}


long getPIDkCup()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkC[UPk]);
}


long getPIDkCdown()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkC[DOWNk]);
}


long getPIDkO()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkO);
}


long getPIDkF()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNkF);
}


long getPIDpefact()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SPNpefact);
}


/* setNoPeriods(int nbrPeriods) sets a new number of periods for CH14 */
int setNoPeriods(long nbrPeriods)
{
register SPIN_OBJ *pSpinObj;
register unsigned short *pOffTime;
unsigned short oneOffTime;
int OffAddr;
int i;

	pSpinObj = pTheSpinObject;

	if (pTheSpinObject == NULL) return(ERROR);

	if( (nbrPeriods >= 6) && (nbrPeriods <= 13) )
		pSpinObj->QOMoffRegs = nbrPeriods;
	else return;

	/* break the off time into equal parts and set QOMoffRegs mem locations. */
	oneOffTime = (pSpinObj->QOM_500ms / pSpinObj->QOMoffRegs) << 1;
	/* limit any off time to be less than max */
	if(oneOffTime > 0xf000) oneOffTime = 0xf000;

/* Setting priority=0 will cause the channel to stop */
	*M332_TPU_CPR0 &= 0xCfff;	/* channel 14 */

/* set up TPU channel 14-15 as an extended Queued Output Match (QOM) function */
/* This TPU channel is used to control the Vertical spinner air solenoid */

/* Calculate the last offset register pointer */
	OffAddr = ( (2 + pSpinObj->QOMoffRegs) << 1) + 0xe2;

/* Start setting up the registers */
	M332_TPU_CHN14->tf.qom14.refLastOffAdrs = OffAddr + QOM_TCR2;
	M332_TPU_CHN14->tf.qom14.loopOffPtr = 0;

/* This can only be called AFTER set up in Create or modified with
|  setOnTime. So no change to a cell that holds a correct value.
|  Leave M332_TPU_CHN14->tf.qom14.offset1 as it is.
*/

	pOffTime = (unsigned short *)&(M332_TPU_CHNe->tf.qom14.offset2);

	for(i=0; i < pSpinObj->QOMoffRegs; i++) *(pOffTime+i) = oneOffTime;
/* Leave all cells OFF, later setOffTime will turn on the correct cell */

/* now set up Host Service Request Register */
	*M332_TPU_HSRR0 = (QOM_HSRR_INIT_PINLO << 12);	/* channel 14 */
/* Setting priority will cause the service request to start */
	*M332_TPU_CPR0 |= (TPU_CPR_MEDIUM << 12);	/* channel 14 */
/* Wait for request to finish */
	while ( *M332_TPU_HSRR0 & 0x3000 );
/* Clear interrupt request flag generated from HSSR request */
	*M332_TPU_CISR ^= TPU_CIER_CISR_CH14;

}	/* end: int setNoPeriods(long nbrPeriods) */


/* setOnTime(unsigned short OffTime) modifies the ON time of CH 14 */

void setOnTime(unsigned short OnTime)
{

	/* insure value is within limits */
	if(OnTime >= 0x7fff) OnTime = 0x7fff;
	if(OnTime < 0x100) OnTime = 0x100;

	M332_TPU_CHN14->tf.qom14.offset1 = OnTime << 1;
}


/* setSpinLED() sets the SpinLED TPU channel according to the value
|   in pSpinObj->SpinLED. 0=off, 1=ONshortOFFlong, 2=OFFshortONlong,
|   3=ON.
|   period1 & period2 are short time, period3 & 4 are long time.
|
|   period1  1  0  1  0
|   period2  1  1  0  0
|   period3  1  1  0  0
|   period4  1  0  1  0
|
|            |  |  |  \_ OFF (STOPPED)
|            |  |  \____ On Short, Off Long (Not Regulating)
|            |  \_______ On Long, Off Short
|            \__________ ON (Regulating)
|
*/
int setSpinLED()
{
register SPIN_OBJ *pSpinObj;
int pattern, howLED, ratemode;

	pSpinObj = pTheSpinObject;

	if (pTheSpinObject == NULL) return(ERROR);

	/* set Control/Status register with status of Spinning */
	howLED = pSpinObj->SpinLED; 
	switch(howLED)
	{
		case SpinLED_STOP:
			if (pSpinObj->SPNspeedSet != 0)
			{
	    	          /* cntrlRegSet(SET_SPN_NOT_REG | SET_BEAR_AIR_ON); */
	    	          cntrlRegSet(SET_SPN_NOT_REG);
			}
			else
			{
	    	          /* cntrlRegClear(SET_SPN_NOT_REG | SET_BEAR_AIR_ON); */
	    	          cntrlRegClear(SET_SPN_NOT_REG);
			}

	    	        cntrlRegSet(SET_SPN_SPD_ZRO);

			break;

		case SpinLED_BLINKshort:
		case SpinLED_BLINKlong:
	    	        cntrlRegClear(SET_SPN_SPD_ZRO);
	    	        cntrlRegSet(SET_SPN_NOT_REG);
			break;

		case SpinLED_OK:
	    	        /* cntrlRegSet(SET_BEAR_AIR_ON); */
	    	        cntrlRegClear(SET_SPN_SPD_ZRO | SET_SPN_NOT_REG);
			break;
	}

	/* check what mode to be in */
	howLED *=  4; /* = 0, 4, 8, 12 */
	pattern = 0xf690 >> howLED;
	M332_TPU_CHN4->tf.qom.offset1 &= 0xfffe; /* clear Off/On bit */
	M332_TPU_CHN4->tf.qom.offset1 |= (pattern & 0x1);
	pattern = pattern >> 1;
	M332_TPU_CHN4->tf.qom.offset2 &= 0xfffe; /* clear Off/On bit */
	M332_TPU_CHN4->tf.qom.offset2 |= (pattern & 0x1);
	pattern = pattern >> 1;
	M332_TPU_CHN4->tf.qom.offset3 &= 0xfffe; /* clear Off/On bit */
	M332_TPU_CHN4->tf.qom.offset3 |= (pattern & 0x1);
	pattern = pattern >> 1;
	M332_TPU_CHN4->tf.qom.offset4 &= 0xfffe; /* clear Off/On bit */
	M332_TPU_CHN4->tf.qom.offset4 |= (pattern & 0x1);
	return(OK);

}


/* getSpinLED() returns the current mode of the SpinLED */
long getSpinLED()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	return(pSpinObj->SpinLED);
}


/* pidlogInit initializes the log for another session */
int pidlogInit()
{
register SPIN_OBJ *pSpinObj;
int msize;
PIDE savePIDE;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if( pSpinObj->pPIDE )
	{
		savePIDE = pSpinObj->pPIDE;
		pSpinObj->pPIDE = NULL;
		taskDelay(10); /* wait for a concurent update to finish */
		msize = pSpinObj->PIDelements * PIDsize;
		memset(savePIDE,0,msize);
		pSpinObj->PIDposition = 0;
		pSpinObj->pPIDE = savePIDE;
		return(OK);
	}
	else return(ERROR);
}


/* pidlogDelete frees the pidlog */
int pidlogDelete()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if( pSpinObj->pPIDE )
	{
		pSpinObj->pPIDE = NULL;
		taskDelay(10); /* wait for a concurent entry to finish. */
		pSpinObj->PIDelements = 0;
		pSpinObj->PIDposition = 0;
		free( pSpinObj->pPIDE);
		return(OK);
	}
	else return(ERROR);
}


/* pidlogCreate mallocs memory for n log values */
long pidlogCreate(int noPIDs)
{
register SPIN_OBJ *pSpinObj;
int msize;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if( pSpinObj->pPIDE ) pidlogDelete();

	if(noPIDs == 0) noPIDs = 60; /* default size if no param */
	if( (noPIDs > 0 ) && ( noPIDs < 1000) )
	{
		/* ------- malloc space for PID Object --------- */
		msize = noPIDs * PIDsize;
		if ( (pSpinObj->pPIDE = (PIDelement *) malloc(msize) ) == NULL )
		{
			errLogSysRet(LOGIT,debugInfo,"pidlogCreate: Could not Allocate Space:");
			return(NULL);
		}
		pidlogInit();
		pSpinObj->PIDelements = noPIDs;
		pSpinObj->PIDposition = 0;
	}
	else return(ERROR);

	return((long)pSpinObj->pPIDE);
}


/* tombPrint prints info Tom B wants */
void tombPrint(long Cor, long Spd, long Per, long Hb)
{
	printf("Air=%08lx(%ld), s=%3d, per=%08lx, eeg=%d\n",
		Cor, Cor, Spd, Per, Hb);
}


/* pidlogFill() records the current PID values in a log for review */
int pidlogFill()
{
register SPIN_OBJ *pSpinObj;
register PIDE pPIDe;
register Cor,Spd,Per,Hb;

	if (pTheSpinObject == NULL) return(ERROR);
	pSpinObj = pTheSpinObject;

	Cor = pSpinObj->PIDcorrection;
	Spd = ( pSpinObj->SPNspeed * 10L ) + pSpinObj->SPNspeedfrac;
	Per = pSpinObj->SPNperiod;
	Hb  = pSpinObj->PIDheartbeat;

	if(pSpinObj->SPNtomb) tombPrint(Cor,Spd,Per,Hb);

	if( (pSpinObj->pPIDE != NULL) &
		(pSpinObj->PIDelements > pSpinObj->PIDposition) )
	{
		pPIDe = pSpinObj->pPIDE;

		if( pSpinObj->SPNspeedSet != 0L )	/* Rate does not need P I D */
		{
			pPIDe[pSpinObj->PIDposition].PIDproportion = pSpinObj->SPNproportion;
			pPIDe[pSpinObj->PIDposition].PIDintegral = pSpinObj->SPNintegral;
			pPIDe[pSpinObj->PIDposition].PIDderivative = pSpinObj->SPNderivative;
			pPIDe[pSpinObj->PIDposition].PIDlerror = pSpinObj->SPNlerror;
		}

		pPIDe[pSpinObj->PIDposition].PIDcorrection = Cor;
		pPIDe[pSpinObj->PIDposition].PIDspeed = Spd;
		pPIDe[pSpinObj->PIDposition].PIDperiod = Per;
		pPIDe[pSpinObj->PIDposition].PIDheartbeat = Hb;

		pSpinObj->PIDposition++;
		return(OK);
	}
	else return(ERROR);
}


/* pidlogPrint() Prings the PID values in the PID log. */
int pidlogPrint()
{
register SPIN_OBJ *pSpinObj;
register PIDE pPIDe;
register int PIDelement;

	if (pTheSpinObject == NULL) return(ERROR);
	pSpinObj = pTheSpinObject;

	if( (pSpinObj->pPIDE != NULL) & (pSpinObj->PIDposition > 0))
	{
		pPIDe = pSpinObj->pPIDE;
		printf("\nPID TCR2x10=%08lx, array data:\n");

		for(PIDelement=0;PIDelement < pSpinObj->PIDposition;PIDelement++)
		{
			if( pSpinObj->SPNspeedSet != 0L )	/* Rate does not need P I D */
			{
				printf("prop=%08lx, int=%08lx, der=%08lx, err=%08lx, ",
				pPIDe[PIDelement].PIDproportion,
				pPIDe[PIDelement].PIDintegral,
				pPIDe[PIDelement].PIDderivative,
				pPIDe[PIDelement].PIDlerror);
			}

			/* All modes, print air value, speed etc. */

			printf("Air=%08lx(%ld), s=%3d, per=%08lx, eeg=%d\n",
				pPIDe[PIDelement].PIDcorrection,
				pPIDe[PIDelement].PIDcorrection,
				pPIDe[PIDelement].PIDspeed,
				pPIDe[PIDelement].PIDperiod,
				pPIDe[PIDelement].PIDheartbeat);
		}
		return(PIDelement);
	}
	else return(pSpinObj->PIDposition);
}

/***********************************************************************************
|  the task function ChkSPNspeed() is invoke every sec.
|    which in turn call chkSpeed() (the main function of the speed control task)
|      which calls spinPID() for the correction to apply
*/

/* long spinPID() returns a correction value 0=no correction.
|
|       This routine DOES NOT correct the
|       speed! It only returns a 2's complement value of the correction
|       to the present setting.
|     For Liquids the setting and correction are in terms of duty cycle of the pulser valve,
|       note duty is in milliDuty (i.e. take SPNsetting/1000.0 gives duty)
|     For MAS the setting and correction are in terms of log10 of the DAC setting of the valve.
|       Note: SPNsetting 37000 is log10(3.7) -> dac value -> ~ 5012
*/
long spinPID()
{
   register SPIN_OBJ *pSpinObj;
   long Integral;
   long Correction;
   double SpdErr;
   register int UorD;
   register int i;
   double sqrt(double);


  pSpinObj = pTheSpinObject;

  if (pTheSpinObject == NULL) return(ERROR);

/* The PID algorithm used here is as follows: ('xx = pSpinObj->SPNxx)
|  SpeedError = set speed ('regulate) - current speed ('sum)
| 'proportion = SpeedError / 'kP[n]    where n is index to correct kP
| 'integral = (SpeedError / 'kI[n] ) + 'integral
| 'derivative = ( SpeedError - 'lerror ) * 'kD[n]
| Correction = 'proportion + 'integral + 'derivative
*/

   if( (SpeedError = (pSpinObj->SPNspeedSet - pSpinObj->SPNspeed)) < 0L)
      UorD = 1;	/* going up, use up(faster) index */
   else
      UorD = 0;	/* going down, use down(slower) index */

   
   /* use this to calc RMS Error, allow speed to get into regulation then call resetRMS() */
   /* calcRMSerror(); */  

   pSpinObj->PIDerrorbuf[pSpinObj->PIDerrindex] = SpeedError;
   pSpinObj->PIDerrindex = pSpinObj->PIDerrindex + 1;
   if (pSpinObj->PIDerrindex >= PID_ERROR_BUFSIZE)
   {
      pSpinObj->PIDerrindex = 0;
   }

   if (pSpinObj->SPNtype == VERTICAL_PROBE)
   {
      pSpinObj->SPNproportion = (SpeedError * pSpinObj->SPNkP[UorD]) /
                                 pSpinObj->SPNkF;
   }
   else
   {
      pSpinObj->SPNproportion = (SpeedError / pSpinObj->SPNkP[UorD]) /
                                 pSpinObj->SPNkF;
   }

   for (i=0,Integral=0L; i < PID_ERROR_BUFSIZE; i++)
   {
      Integral += pSpinObj->PIDerrorbuf[i];
/*
      printf("Int: %ld, err[%d]=%ld\n",
               Integral,i,pSpinObj->PIDerrorbuf[i]);
*/
   }
/*
   printf("Int (%ld) / kI (%ld) / kF (%ld)\n",Integral,pSpinObj->SPNkI[UorD],pSpinObj->SPNkF);
*/
   Integral = ((Integral / pSpinObj->SPNkI[UorD]) / pSpinObj->SPNkF );

   pSpinObj->SPNintegral = Integral;

/*
   printf("Deriv = ( (Err (%ld) - PrevErr (%ld)) / kD (%ld)) ) / kF (%ld)\n",
	PeriodError,pSpinObj->SPNlerror,pSpinObj->SPNkD[UorD],pSpinObj->SPNkF);
*/
   if (pSpinObj->SPNtype == VERTICAL_PROBE)
   {
      pSpinObj->SPNderivative =
            ( ( SpeedError - pSpinObj->SPNlerror ) * pSpinObj->SPNkD[UorD] ) /
               pSpinObj->SPNkF;
   }
   else
   {
      pSpinObj->SPNderivative =
            ( ( SpeedError - pSpinObj->SPNlerror ) / pSpinObj->SPNkD[UorD] ) /
               pSpinObj->SPNkF;
   }

/*
   printf("Cor: kO (%ld) * ( Prop (%ld) + Int (%ld) + Deriv (%ld) )\n",
		pSpinObj->SPNkO,pSpinObj->SPNproportion,pSpinObj->SPNintegral,
		pSpinObj->SPNderivative);
*/
   Correction = pSpinObj->SPNkO *
                ( + pSpinObj->SPNproportion
                  + pSpinObj->SPNintegral
                  + pSpinObj->SPNderivative);

/*  limit increase for MAS correct to 37000 == 10^3.7 == ~5000 dac value */
/*
   if (abs(Correction) > 37000)
   {
      if (pSpinObj->MASdac == 0 )
      {
  	Correction = 37000;
      }
      else
      {
      if (Correction < 0L)
  	Correction = -37000;
      else
	Correction = 37000;
      }
   }
*/

   pSpinObj->PIDcorrection = Correction;
   pSpinObj->SPNlerror = SpeedError;	/* update last error */
/*
   printf("Reg: %ld, Present: %ld (%d Hz*10), Error: %ld\n",
            pSpinObj->SPNregulate, pSpinObj->SPNperiod, getSpeed(), PeriodError);
   printf("P: %ld, I: %ld, D: %ld, T: %ld\n", pSpinObj->SPNproportion, Integral,
            pSpinObj->SPNderivative, Correction);
*/
   return(Correction);  /* correction is a delta duty cycle */
}	/* end long spinPID() */


int setOffTime(long OffTime)
{
register SPIN_OBJ *pSpinObj;
register unsigned short *pOffTime;
unsigned short oneOffTime;
int i;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	switch (pSpinObj->SPNtype)
	{
		case VERTICAL_PROBE:
#ifdef SCOPE
			GlobalOffTime = OffTime;
#endif
			pOffTime = (unsigned short *)&(M332_TPU_CHNe->tf.qom14.offset2);

			/* break the off time into equal parts and set QOMoffRegs mem locations. */
			oneOffTime = (OffTime / pSpinObj->QOMoffRegs) << 1;
			for(i=0; i < pSpinObj->QOMoffRegs; i++) *(pOffTime+i) = oneOffTime;
			*(pOffTime+(pSpinObj->QOMoffRegs - 1)) |= QOM_RISING_EDGE;
			break;

		case MAS_PROBE:
			pSpinObj->MASdac = (unsigned short)OffTime;
			*(unsigned short *) MPU332_SOLID_DAC = pSpinObj->MASdac;
			break;
	}

	return(OK);
}


/* chkSpeed() checks the global speed setting and compares it with the
|             current speed. If current speed is not within tolerance
|             then spinPID is called to get a correction factor. This
|             correction factor is scaled and passed to setPWMvalue.
*/
int chkSpeed()
{
register SPIN_OBJ *pSpinObj;
long Correction;
long duty2offtime(long);
long MAS2DAC(long);
long rateRamp(void);
long tmpval,deltaval,deltaspd,ddelta;


	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->PIDheartbeat++;	/* Make this heart beat. */
	Correction = 0L;	/* in case we don't call spinPID for pidlog. */

#ifdef XXX
        printf("1 hz = %ld ticks, 100 hz = %ld ticks\n",
           (pSpinObj->SPNrevs * ( 1000000000L / ( ( 1 /*Hz*/ * pSpinObj->TCR1x10period) / 10L ) )),
           (pSpinObj->SPNrevs * ( 1000000000L / ( ( 100 /*Hz*/ * pSpinObj->TCR1x10period) / 10L ) )));
#endif

	/* ----------  Determine if Spinning -------------------------------*/

 	/* set for next pass confirmation */
	/* check if spinner is actually rotating or is stopped */
	DPRINT3(2,"SPNhbeat: %lu, SPNeeg1: %lu, SPNeeg2: %lu\n",pSpinObj->SPNheartbeat,
			pSpinObj->SPNeeg1,pSpinObj->SPNeeg2);
        /* if speed interrupt heart beat == eeg1 then no interrupt for 1 sec, rotor may be stopped */
        /* if speed interrupt heart beat == eeg1 == eeg2 then no interrupts for 2 sec rotor is not rotating ! */
	if( (pSpinObj->SPNheartbeat == pSpinObj->SPNeeg1) &&
			(pSpinObj->SPNheartbeat == pSpinObj->SPNeeg2) )
	{
	   /* more than 1 sec has passed so the speed must be less that 1 Hz */
	   pSpinObj->SPNsum = 0L;
	   pSpinObj->SPNspeed = 0L;
	   pSpinObj->SPNspeedfrac = 0L;
           /* set period to equivilent of 1 Hz  (~ 0x3d0900) */
           pSpinObj->SPNperiod = (pSpinObj->SPNrevs * ( 1000000000L /
	  		( ( 1 /*Hz*/ * pSpinObj->TCR1x10period) / 10L ) ) ) * 2;
	   pSpinObj->SPNsum = pSpinObj->SPNperiod * pSpinObj->SPNnbr;
	   for(pSpinObj->SPNindex=0; 
		 pSpinObj->SPNindex < pSpinObj->SPNnbr - 1;pSpinObj->SPNindex++)
           {
		pSpinObj->SPNperiods[pSpinObj->SPNindex] = pSpinObj->SPNperiod;
           }
	   /* get the last one with no increment */
	   pSpinObj->SPNperiods[pSpinObj->SPNindex] = pSpinObj->SPNperiod; 

	   DPRINT1(2,"No HEART - spinLED: %d\n",pSpinObj->SpinLED);
	   /* if (( pSpinObj->SpinLED == SpinLED_OK) ) /* regulating */
	   if ( (pSpinObj->SPNspeedSet != 0) && (pSpinObj->SPNtype == MAS_PROBE) )
	   {
	      DPRINT(0,"No Rotor Speed Detected, SpeedShutdown MAS Spinner\n");
	      pSpinObj->SPNrateSet = 0;
              pSpinObj->SPNmode = RATE_MODE;
	      pSpinObj->PrevSpeed = 0L;
	   }
	}
	else
	{
	        DPRINT1(2,"Maybe HEART - spinLED: %d\n",pSpinObj->SpinLED);
		/* If we got here, things may be running */
  		/* update eeg1 with spin intrp heartbeat */
		if( pSpinObj->SPNeeg1 != pSpinObj->SPNheartbeat)
		{
			pSpinObj->SPNeeg1 = pSpinObj->SPNheartbeat;
		}
		else
		{
			/* no interrupt for speed check interval (1sec) set eeg2 */
			/* thus if heartbeat == eeg1 == eeg2 then rotor not spinning */
			pSpinObj->SPNeeg2 = pSpinObj->SPNeeg1; /* NO, update eeg */
#ifdef SCOPE
			ScopeCollectSignals(0);
#endif
			return(OK);
		}
	}
	/*---------------------------------------------------------------------------------------*/

	pSpinObj->SPNperiod = pSpinObj->SPNsum / pSpinObj->SPNnbr;
	getSpeed();	/* calculate current speed and period */

        /* ----------  Set the Blinking Lights Here -------------------- */
        /* If speed increasing then fast blinking, slowing slow blinking */
	if( (abs(SpeedError) <= pSpinObj->SPNtolerance) && (pSpinObj->SPNmode != RATE_MODE) )
        {
	   pSpinObj->SpinLED = SpinLED_OK;  
        }
        else
	{
          if (pSpinObj->SPNspeed < 1000L)
	     pSpinObj->SpinLED = SpinLED_STOP;  
          else if (pSpinObj->SPNspeed > pSpinObj->PrevSpeed)
	     pSpinObj->SpinLED = SpinLED_BLINKshort;  
            else
	     pSpinObj->SpinLED = SpinLED_BLINKlong;  
	}
	setSpinLED();

        /* --------------------------------------------------------------- */

        /* Hey if sample is Ejected or Droping into Mag. Don't bother try to regulate it */
	if((*AUTO_CNTRL_STAT_REG & RD_EJECT_ON) || (*AUTO_CNTRL_STAT_REG & RD_SDROP_ON))
	{
		pSpinObj->Vejectflag = 1;
#ifdef SCOPE
		ScopeCollectSignals(0);
#endif
		return(OK); /* Don't mess with air if Eject or Sdrop set */
	}
	if( pSpinObj->Vejectflag == 1)
	{
		/* If there is a sample in the probe again, delay a while before resuming */
		pSpinObj->Vejectflag = 0;
		if( pSpinObj->Vdropdelay > 0) taskDelay( pSpinObj->Vdropdelay);
	}

        DPRINT2(1,"chkSpeed- Type: '%s', Mode: '%s'\n",
		( (pSpinObj->SPNtype == MAS_PROBE) ? "MAS" : "LIQUIDS"),
		( (pSpinObj->SPNmode == SPEED_MODE) ? "Speed" : "Rate") );
        DPRINT5(1,"          Speed: %ld, SetSpeed: %d, rateSet: %d, Setting: %d, DAC: %u\n",
				pSpinObj->SPNspeed,pSpinObj->SPNspeedSet, 
				pSpinObj->SPNrateSet,pSpinObj->SPNsetting,pSpinObj->MASdac);

        DPRINT5(1,
	    "chkSpeed- Spd Set: %d, Tru: %d, Er: %d, Tol: %d, LED: %d (0-S,1-Ac,2-De,3-Reg)\n",
		pSpinObj->SPNspeedSet, pSpinObj->SPNspeed, SpeedError, pSpinObj->SPNtolerance,
		pSpinObj->SpinLED);


	DPRINT4(2,"Mode: %d, DAC: %ld, Speed: %ld, delaySet: %ld\n",pSpinObj->SPNmode,
		pSpinObj->MASdac,pSpinObj->SPNspeed,pSpinObj->SPNdelaySet);


        if ( ( pSpinObj->SPNtype == MAS_PROBE) &&
		(pSpinObj->SPNmode == RATE_MODE) &&
		(pSpinObj->SPNdelaySet != 0L) &&
		(pSpinObj->SPNspeed >= 600000 /* 600 Hz */) )
        {
	   wdCancel(wdSpinChk);   /* cancel wdog, have succeeded in spinning */
	   DPRINT1(-1,"Rate switching back to Speed: %ld --------\n",pSpinObj->SPNdelaySet);
	   setSpeed(pSpinObj->SPNdelaySet);
	   pSpinObj->SPNdelaySet = 0L;
        }


        /* ------ Now Time to Control the Spinner, Either by Rate or Speed  -------- */
        if (pSpinObj->SPNmode == RATE_MODE)
        {
#ifdef SCOPE
	   ScopeCollectSignals(0);
#endif
            /* ramp rate if neccearrary */	
	   switch (pSpinObj->SPNtype)
	   {
	     case VERTICAL_PROBE:

	        /*  	Old system set the speed to 0 when told to set the rate
    		        to 0.  New system needs to do this or the sample spins
    		        at an unacceptably high speed (50 or more Hz).    08/04/95  */

		  if (pSpinObj->SPNsetting != pSpinObj->SPNrateSet)
                  {
		    DPRINT1(1,"Set Liq Spin Rate: %d\n",pSpinObj->SPNrateSet);
	            if (pSpinObj->SPNrateSet <= 0L)
	            {
	               unsigned short *pOffTime;

		       DPRINT(1,"Turn off bearing air\n");
	               /* find the last offset used and kill the Rising edge flag */
	               /* This will actually shut the air off. When speed is set to */
	               /* a positive value, setOffTime will turn air back on. */
	               pOffTime = (unsigned short *)&(M332_TPU_CHNe->tf.qom14.offset2);
	               *(pOffTime+(pSpinObj->QOMoffRegs - 1)) &= ~QOM_RISING_EDGE;
		       cntrlRegClear(SET_BEAR_AIR_ON);  /* off with bearing air too */
                       pSpinObj->SPNrateSet = pSpinObj->SPNsetting = 8916; /*.89 % duty cycle */
	            }
		    else
  		    {
	              if(pSpinObj->SPNrateSet > (pSpinObj->QOM_500ms * 2 )) 
			pSpinObj->SPNrateSet = pSpinObj->QOM_500ms * 2;

	              if(pSpinObj->SPNrateSet < (pSpinObj->QOM_9ms)) 
			pSpinObj->SPNrateSet = pSpinObj->QOM_9ms;

		      setOffTime(pSpinObj->SPNrateSet);
		      pSpinObj->SPNsetting = pSpinObj->SPNrateSet;
		    }
  		  }
	          break;


	  case MAS_PROBE:
        	DPRINT4(1,"MAS Rate- Target: %d, Tru: %d, Setting: %ld, Correction: %ld\n",
			pSpinObj->SPNrateSet,pSpinObj->MASdac,pSpinObj->SPNsetting,Correction);
		if (pSpinObj->MASdac != pSpinObj->SPNrateSet)
                {
	          tmpval = pSpinObj->MASdac;
	          Correction = rateRamp();
		  pSpinObj->SPNsetting += Correction;
		  setOffTime(MAS2DAC(pSpinObj->SPNsetting));
		  pSpinObj->PrevDacVal = tmpval;
	        }
		break;
           }

	   pidlogFill();	/* if any log or console print, do it! */
        }
        else  /* Regulate Speed */
        {

#ifdef SCOPE
		ScopeCollectSignals(0);
#endif
	   pSpinObj->PIDheartbeat++;	/* Make this heart beat faster if correcting. */

	   Correction = spinPID();
              
           DPRINT2(3,"chkSpeed - setting: %ld, correction: %ld\n",pSpinObj->SPNsetting,Correction);
	   pSpinObj->SPNsetting += Correction;

	   switch (pSpinObj->SPNtype)
	   {
	     case VERTICAL_PROBE:
		if (pSpinObj->SPNsetting < 8916L) /* min duty cycle, 9 msec on - 1 sec off */
	   			pSpinObj->SPNsetting = 8916L;
		if (pSpinObj->SPNsetting > 500000L) /* max duty cycle 9 msec on - 9 msec off */
	   			pSpinObj->SPNsetting = 500000L;

		setOffTime(duty2offtime(pSpinObj->SPNsetting));
		break;

	     case MAS_PROBE:
		if (pSpinObj->SPNsetting < 0L) /* min DAC value */
	   			pSpinObj->SPNsetting = 0L;
		if (pSpinObj->SPNsetting > 0xFFFF) /* max DAC value */
	   			pSpinObj->SPNsetting = 0xFFFF;
	        tmpval = MAS2DAC(pSpinObj->SPNsetting);
		deltaval = (tmpval - pSpinObj->PrevDacVal);
	        deltaspd = abs((pSpinObj->SPNspeed/1000) - (pSpinObj->PrevSpeed/1000));
	        /*
		printf("=======> prevDAC: %lu, NewDAC: %lu, delta: %ld\n", pSpinObj->PrevDacVal,tmpval,deltaval);
		*/
		DPRINT3(2,"=======> prevSPD: %lu, NewSPD: %lu, delta: %ld/sec, ",
			pSpinObj->PrevSpeed/1000,pSpinObj->SPNspeed/1000,deltaspd);
		if (deltaspd == 0 )
		   deltaspd = 1;

                ddelta = pSpinObj->PrevDacDelta/deltaspd;

	        DPRINT1(2," dDAC/dSPD: %ld \n", ddelta);

		pSpinObj->PrevDacDelta = deltaval;

	        deltaspd = abs((pSpinObj->SPNspeedSet/1000) - (pSpinObj->SPNspeed/1000));
		/*
		printf("=======> targetSPD: %lu, presSPD: %lu, delta: %ld\n",
			pSpinObj->SPNspeedSet/1000,pSpinObj->SPNspeed/1000,deltaspd);
		*/

		/* MAS Safety ShutDown if can't seem to achive speed */
		if (( pSpinObj->SpinLED != SpinLED_OK) && 
		     ( (ddelta > 700) || (tmpval == 0xffff))) /* shutdown */
		{
		   DPRINT(0,"MAS Rotor Cannot Achieve Speed, Shutdown MAS Spinner\n");
		   pSpinObj->SPNrateSet = 0;
        	   pSpinObj->SPNmode = RATE_MODE;
		   pSpinObj->PrevSpeed = 0L;
		}

		setOffTime(MAS2DAC(pSpinObj->SPNsetting));
		pSpinObj->PrevDacVal = tmpval;
		break;
	   }


	   pidlogFill();	/* if any log or console print, do it! */
	
           pSpinObj->PrevSpeed = pSpinObj->SPNspeed;
        }

#ifdef SCOPE
	ScopeCollectSignals(0);
#endif

	return(OK);

}	/* end chkSpeed() */

/* change the duty cycle setting to an offtime for valve */
long duty2offtime(long duty)
{
   register SPIN_OBJ *pSpinObj;
   register long offtime;
   register ulong_t Mmax;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   Mmax = ((ulong_t)(pSpinObj->QOM_9ms)) * 1000000L;
   offtime = ((long)((Mmax / (ulong_t) duty))) - pSpinObj->QOM_9ms;
   return(offtime);
}

/* log setting to DAC value */
long MAS2DAC(long setting)
{
   register SPIN_OBJ *pSpinObj;
   register long offtime;
   register ulong_t DACsetting;
   double logsetting;
   double dacvalue;
   double pow(double, double);

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   logsetting = ((double) setting) / 10000.0;
   /* printf("setting: %ld,  setting/10000: %lf\n",setting,logsetting); */
   dacvalue = pow((double)10.0, logsetting);
   DACsetting = (long) dacvalue; 
   if (DACsetting > 0xffff)	/* MAX dac setting */
      DACsetting = 0xffff;

   /* printf("10E %lf = dac value: %lf, %lu\n",logsetting, dacvalue,DACsetting); */
   return(DACsetting);
}


long DAC2MAS(long DACvalue)
{
   double logval;
   double tmpval;
   long   logx10k;
   double log10(double);

   if (DACvalue == 0)
     return(0L);
     
   tmpval = (double) DACvalue;
   logval = log10(tmpval);
   logx10k = logval * 10000L;
/*    printf("log(%ld) = %lf, %ld\n",DACvalue,logval,logx10k); */
   return(logx10k); 
}

/* setSpinOn turns on the Rising bit in the last CH14 QOM offset */
int setSpinOn()
{
register SPIN_OBJ *pSpinObj;
register unsigned short *pOffTime;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	pOffTime = (unsigned short *)&(M332_TPU_CHNe->tf.qom14.offset2);
	*(pOffTime+(pSpinObj->QOMoffRegs - 1)) |= QOM_RISING_EDGE;
	return(OK);
}


/* getRate returns the current air rate */
unsigned long getRate()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	switch (pSpinObj->SPNtype)
	{
		case VERTICAL_PROBE:
			return( ( (M332_TPU_CHNe->tf.qom14.offset2 >> 1) * pSpinObj->QOMoffRegs) );

		case MAS_PROBE:
			return( (unsigned long) pSpinObj->MASdac );
	}
}


/* setVDropDelay sets the delay after SDROP is off till regulation */
int setVDropDelay(long newDropDelay)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if( (newDropDelay >= 0) && (newDropDelay < 3600) )
		pSpinObj->Vdropdelay = newDropDelay;
	else
		pSpinObj->Vdropdelay = 60;
	return(newDropDelay);
}


/* setVDropTime sets the delay after EJECT till SDROP is Off */
int setVDropTime(long newDropTime)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if( (newDropTime >= 0) && (newDropTime < 3600) )
		pSpinObj->Vdroptime = newDropTime;
	else
		pSpinObj->Vdroptime = 240;
	return(newDropTime);
}

long rateRamp(void)
{
    register SPIN_OBJ *pSpinObj;
    long correction;
    int sign,chngrate,ratechge;
    long nrate,logset,logtarget;
    long MAS2DAC(long);
    long DAC2MAS(long);

    pSpinObj = pTheSpinObject;

    ratechge = 802;

   logset = DAC2MAS((long) pSpinObj->MASdac);
   logtarget = DAC2MAS((long) pSpinObj->SPNrateSet);
   DPRINT1(2,"Present log setting: %ld\n",logset);

   if (pSpinObj->MASdac > pSpinObj->SPNrateSet) 
      sign = -1;
   else
      sign = 1;

   /* working below DAC value 500 doensn't change much so just go right to
      the target value if going down, or up to 500 to start the ramp at */
   if (pSpinObj->MASdac < 500)
   {  
      if (sign > 0)
       correction = DAC2MAS(500L) - logset;   /* go to 500 right away */
      else 
       correction =  logtarget - logset;
   }
   else
   {
     if (abs(logset-logtarget) > ratechge)  /* dac change > 1000 units */
     { 
	correction = (sign * ratechge);
     }
     else
        correction = logtarget - logset;
   }

  return(correction);
}

/* setRate places a fixed air rate */
int setRate(long newRate)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNspeedSet = 0L;	/* kill pid */
	pSpinObj->SPNmode = RATE_MODE;	/* go into rate control mode */
	pSpinObj->SPNrateSet = newRate;	/* set rate */

        DPRINT3(0,"setRate: pRate: %ld, nRate: %ld, SpinType: %d\n",pSpinObj->MASdac,newRate,pSpinObj->SPNtype);

	return(OK);
}

ReStartSpeed(SPIN_OBJ *pSpinObj)
{
   logMsg("-----------------> ReStartSpeed: %ld\n",pSpinObj->SPNdelaySet);
   setSpeed(pSpinObj->SPNdelaySet);
}

/* setSpeed(newspeed) places newspeed in global object.
*/
int setSpeed(long newspeed)
{
register SPIN_OBJ *pSpinObj;
register unsigned short *pOffTime;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNdelaySet = 0L;

        /* if speed above or equal to MAS threshold switch to MAS */
	/*    control otherwise switch to Liquids */
	DPRINT2(0,"nSpeed: %ld, Threshold: %ld\n",newspeed,pSpinObj->SPNmasthres);
        if( newspeed >= pSpinObj->SPNmasthres)
        {
	   /* DPRINT(-1,"Switch to MAS if needed\n"); */
	   if (pSpinObj->SPNtype != MAS_PROBE)
           {
	      DPRINT(0,"Switching to MAS\n");
	      initMAS();
	      taskDelay(1);
	   }
        }
        else
        {
	   /* DPRINT(-1,"Switch to Liquids if needed\n"); */
	   if (pSpinObj->SPNtype != VERTICAL_PROBE)
           {
	      DPRINT(0,"Switching to Liquids\n");
	      initVertical();
	      taskDelay(1);
	   }
        }
	if(newspeed <= 0L )
	{
		pSpinObj->SPNdelaySet = 0L;
		pSpinObj->PIDcorrection = 0L; /* set for pidlog */
		pSpinObj->SPNlerror = pSpinObj->SPNintegral = pSpinObj->SPNspeedSet = 0L;
                pSpinObj->SPNregulate = (pSpinObj->SPNrevs * ( 1000000000L /
	      					( ( 1 /*Hz*/ * pSpinObj->TCR1x10period) / 10L ) ) ) * 2;
		pSpinObj->SPNrateSet = 0L;
		pSpinObj->SPNmode = RATE_MODE;
		return(OK);
		/* cntrlRegClear(SET_BEAR_AIR_ON); */
        }
	else	/* speed is valid */
	{
		/* if this is the first speed set, insure integral is non zero */
	        /* spinner not spinning ?, yes then set params to allow startup so that
		   safety shutdown does not trigger */
   		DPRINT3(0,"setSpeed: SPNheartbeat: %d, SPNeeg1: %d ,SPNeeg2: %d\n",
			pSpinObj->SPNheartbeat,pSpinObj->SPNeeg1,pSpinObj->SPNeeg2);
	        if( (pSpinObj->SPNheartbeat == pSpinObj->SPNeeg1) &&
			(pSpinObj->SPNheartbeat == pSpinObj->SPNeeg2) )
		{
   		   /* set eegs as to not equal heartbeart, give interrupt routine a chance to start up */
   		   pSpinObj->SPNheartbeat = 1;
   		   pSpinObj->SPNeeg1 = 200;
   		   pSpinObj->SPNeeg2 = 300;
   		   DPRINT(0,"setSpeed: setting SPNheartbeat,SPNeeg1,SPNeeg2\n");
		}

		pSpinObj->SPNspeedSet = newspeed * 1000L;
        	pSpinObj->SPNmode = SPEED_MODE;
		if(pSpinObj->SPNtype == VERTICAL_PROBE)
		   cntrlRegSet(SET_BEAR_AIR_ON);

		DPRINT4(-1,"SPNset: %ld, Speed: %d, MASdac: %d, SPNtype: %d\n",
				pSpinObj->SPNspeedSet,pSpinObj->SPNspeed,
				pSpinObj->MASdac,pSpinObj->SPNtype);
	        if ( (pSpinObj->SPNtype == MAS_PROBE) &&
		     (pSpinObj->MASdac < 2500 ) && 
		     (pSpinObj->SPNspeed <= 300000 /* 300 Hz */) )
                {
		   DPRINT(-1,"===========> Speed Zero, setting SPNdelaySet <==============\n");
	           pSpinObj->SPNdelaySet = newspeed;   /* speed to regulate to (setspeed) */
		   DPRINT1(-1,"delaySet: %ld\n",pSpinObj->SPNdelaySet);
		   setOffTime(2000);
                   pSpinObj->SPNsetting = DAC2MAS(2000L);
		   setRate(6000);

		   /* Ok, here's the deal, the nano-probes and Chem-magnetics probes
			  do not spin when the drive air is off, unlike the solids
			  probes, as a result the no spinning speed protection was
			 kicking in before the probe had a chance to spin.
			The fix is to initial set the spinner control into rate
			mode with a flow that should start things, ( protections are
			disabled in rate mode), once spinner speed has reached over 600 Hz
			then chkSpeed() switches back to Speed regulation mode.  However if
			for some reason the spinner get spinning the follow watch -dog timer
			will expire in 17 seconds and shutdown the air flow.
                   */ 
    		   /* Set Spin Check to go off in 17 Seconds */
    	           if (wdStart(wdSpinChk, (int) (sysClkRateGet() * 17), 
			       (FUNCPTR) ReStartSpeed, (int) pTheSpinObject) == ERROR)
    	 	   {
        	    DPRINT(-1,"wdStart Error\n");
    	 	   }

                }
	}

	pSpinObj->SPNregulate = pSpinObj->SPNrevs * ( 1000000000L /
		( (newspeed * pSpinObj->TCR1x10period) / 10L ) );

        pSpinObj->PrevDacVal = pSpinObj->PrevDacDelta = pSpinObj->PrevSpeed = 0L;

	return(OK);

}	/* end setSpeed(long newspeed) */


int getSetSpeed(long *speed, int *mode)
{
   register SPIN_OBJ *pSpinObj;
   int rmode;

   if (pTheSpinObject == NULL) 
       return(ERROR);

   pSpinObj = pTheSpinObject;

   /* determine if the speed was set or rate was set */
   DPRINT2(2,"getSetSpeed: set speed: %ld, SpinLED: '%s'\n",pSpinObj->SPNspeedSet,
	(pSpinObj->SpinLED == SpinLED_OK) ? "Regulated" : "Non-Reg.");

   *speed = pSpinObj->SPNspeedSet/1000L;
   if ( (pSpinObj->SPNspeedSet == 0L) && 
	(pSpinObj->SpinLED != SpinLED_STOP))
   {
      *mode = rmode = RATE_MODE;
   }
   else
   {
      *mode = rmode = SPEED_MODE;
   }
   return(rmode);
}

setMASThreshold(int threshold)
{
   register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) 
       return(ERROR);

   pSpinObj = pTheSpinObject;


   DPRINT1(0,"setMASThreshold: %d\n",threshold);
   pSpinObj->SPNmasthres = threshold;

   return(0);
}

setSpinRegDelta(int delta)
{
   register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) 
       return(ERROR);

   pSpinObj = pTheSpinObject;

   pSpinObj->SPNtolerance = (delta * 1000);  /* delta (Hz) * 1000, since 1Hz == 1000 */
   DPRINT1(0,"setSpinRegDelta: SPNtolerance: %d Hz\n",(pSpinObj->SPNtolerance/1000));

   return(0);
}

ChkSPNspeed(int showFreq)
{
	FOREVER
	{
		chkSpeed();
		taskDelay(60 * showFreq);
	}
}


int startChk(int chkFreq)
{
int tid;

	if(chkFreq < 1) chkFreq = 1;
	tid = taskSpawn("tChkSPNspeed",55,VX_FP_TASK,8192,ChkSPNspeed,
		chkFreq,0,0,0,0,0,0,0,0,0);
	return(tid);
}


SpeedShow(int showFreq)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);
	pSpinObj = pTheSpinObject;

	FOREVER
	{
		printf(" %d",getSpeed());

		if(pSpinObj->SPNtype == MAS_PROBE)
			printf(" (%d)\n",getRate());

		taskDelay(60 * showFreq);
	}
}


int startSpeedShow(int showFreq)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	if(showFreq < 1) showFreq = 5;
	pSpinObj = pTheSpinObject;
	pSpinObj->SPNtaskid1 = taskSpawn("tSpinShow",55,0,1024,SpeedShow,
		showFreq,0,0,0,0,0,0,0,0,0);
	return(pSpinObj->SPNtaskid1);
}


int killSpeedShow()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;
	if(pSpinObj->SPNtaskid1 <= 0) return(ERROR);
	else
	{
		taskDelete( pSpinObj->SPNtaskid1);
		pSpinObj->SPNtaskid1 = 0;
	}
	return(pSpinObj->SPNtaskid1);
}


setSPNgad()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNgad = TRUE;
}


killSPNgad()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNgad = FALSE;
}


setSPNtomb()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNtomb = TRUE;
}


killSPNtomb()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	pSpinObj->SPNtomb = FALSE;
}


int setSPNwindow(long newSPNwindow)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if(newSPNwindow >= 0 && newSPNwindow <= 50)
	{
		pSpinObj->SPNwindow = newSPNwindow;
		return(OK);
	}
	else return(ERROR);

}


long getSPNwindow()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	return(pSpinObj->SPNwindow);
}


int setTCR1x10(long newTCR1x10)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if(newTCR1x10 >= 0x900 && newTCR1x10 <= 0x1000)
	{
		pSpinObj->TCR1x10period = newTCR1x10;
		return(OK);
	}
	else return(ERROR);

}


long getTCR1x10()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	return(pSpinObj->TCR1x10period);
}


int setPIDmode(long pidmode)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if(pidmode >= 0 && pidmode <= 50)
	{
		pSpinObj->PIDmode = pidmode;
		return(OK);
	}
	else return(ERROR);
}


long getPIDmode()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	return( pSpinObj->PIDmode);
}


int setOffRegs(long Offregs)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	if(Offregs > 4 && Offregs <= 13)
	{
		pSpinObj->QOMoffRegs = Offregs;
		return(OK);
	}
	else return(ERROR);

}


long getOffRegs()
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);

	pSpinObj = pTheSpinObject;

	return( pSpinObj->QOMoffRegs);
}


/* getCPUstatus does just that. */
unsigned short getCPUstatus()
{
	asm(" move SR,d0");
}


void getStat()
{
unsigned short astat;

	astat = *AUTO_CNTRL_STAT_REG;


	printf("SloDrp=%s, Bear=%s, Eject=%s, Lk=%s, Samp=%s, VT=%s, Speed=%s, Reg=%s\n",
		((astat & RD_SDROP_ON) ? "ON" : "OFF"),
		((astat & RD_BEAR_ON) ? "ON" : "OFF"),
		((astat & RD_EJECT_ON) ? "ON" : "OFF"),
		((astat & RD_SAMP_NOT_LOCKED) ? "OFF" : "OK"),
		((astat & RD_SAMP_IN_PROBE) ? "IN" : "OUT"),
		((astat & RD_VT_ATTEN) ? "NG" : "OK"),
		((astat & RD_SPIN_ZERO) ? "ZERO" : "NON-ZERO"),
		((astat & RD_SPIN_NOT_REG) ? "NO" : "OK") );
}


/********************************************************************
* SpinShow - display the status information on the TPU Object
*
*  This routine display the status information of the TPU Object
*
*
*  RETURN
*   VOID
*
*/
void spinShow()
{
register SPIN_OBJ *pSpinObj;
register unsigned short *pOffData;
int i;
int avail=ERROR;
char *pstr;
unsigned short status;
unsigned short tmcr,ttcr,dscr,dssr,ticr,cier;
unsigned short cfsr0,cfsr1,cfsr2,cfsr3;
unsigned short hsqr0,hsqr1,hsrr0,hsrr1,cpr0,cpr1,cisr;
unsigned short CPUstatus;
long Lheart,Leeg1,Leeg2,Spinsum,Spinperiod,Spinregulate,Spincount,Spinnbr;
long Spinindex,Spinspeed,Spinspeedfrac,Spinspeedset,Spinerror;
long Spintolerance,Spinproportion,Spinintegral,Spinderivative,Lcor;
long Ltcr1x10,Ltcr2x10,Loffregs;
long L500ms,L9ms,SPNkO,SPNkPu,SPNkPd,SPNkIu,SPNkId,SPNkDu,SPNkDd,SPNkF;
long Pelement,Pposition;
long Lpidheart;
unsigned short masdac;
int spntype,spnrevs,spnwin,SPNkCu,SPNkCd,MASthres;
PIDE pPIDe;

long databuf[SPNperiodSize];
unsigned short ch14_15[16];
unsigned short ch4[8];

	printf("\nspinObj.c 11.1 07/09/07\n");

	if (pTheSpinObject == NULL)
	{
		printf("SpinShow: TPU Object pointer is NULL.\n");
	}
	else
	{
		pSpinObj = pTheSpinObject;
	}

	if (pTheSpinObject != NULL)
	{
		Lheart = pSpinObj->SPNheartbeat;
		Leeg1 = pSpinObj->SPNeeg1;
		Leeg2 = pSpinObj->SPNeeg2;
		Spinsum = pSpinObj->SPNsum;
		Spinperiod = pSpinObj->SPNperiod;
		Spinregulate = pSpinObj->SPNregulate;
		Spincount = pSpinObj->SPNcount;
		Spinnbr = pSpinObj->SPNnbr;
		Spinindex = pSpinObj->SPNindex;
		Spinspeed = pSpinObj->SPNspeed;
		Spinspeedfrac = pSpinObj->SPNspeedfrac;
		Spinspeedset = pSpinObj->SPNspeedSet;
		Spinerror = pSpinObj->SPNlerror;
		Spintolerance = pSpinObj->SPNtolerance;
		Spinproportion = pSpinObj->SPNproportion;
		Spinintegral = pSpinObj->SPNintegral;
		Spinderivative = pSpinObj->SPNderivative;
		Lcor = pSpinObj->PIDcorrection;
		Lpidheart = pSpinObj->PIDheartbeat;
		Ltcr1x10 = pSpinObj->TCR1x10period;
		Ltcr2x10 = pSpinObj->TCR2x10period;
		Loffregs = pSpinObj->QOMoffRegs;
		L500ms = pSpinObj->QOM_500ms;
		L9ms = pSpinObj->QOM_9ms;
		SPNkO = pSpinObj->SPNkO;
		SPNkF = pSpinObj->SPNkF;
		SPNkPu = pSpinObj->SPNkP[UPk];
		SPNkPd = pSpinObj->SPNkP[DOWNk];
		SPNkIu = pSpinObj->SPNkI[UPk];
		SPNkId = pSpinObj->SPNkI[DOWNk];
		SPNkDu = pSpinObj->SPNkD[UPk];
		SPNkDd = pSpinObj->SPNkD[DOWNk];
		for(i=0;i<SPNperiodSize;i++) databuf[i]=pSpinObj->SPNperiods[i];
		Pelement = pSpinObj->PIDelements;
		Pposition = pSpinObj->PIDposition;
		pPIDe = pSpinObj->pPIDE;
		spntype = pSpinObj->SPNtype;
		MASthres = pSpinObj->SPNmasthres;
		spnrevs = pSpinObj->SPNrevs;
		spnwin = pSpinObj->SPNwindow;
		SPNkCu = pSpinObj->SPNkC[UPk];
		SPNkCd = pSpinObj->SPNkC[DOWNk];
		masdac = pSpinObj->MASdac;
	}

	/* get all the data quickly and process it later */
	tmcr = *M332_TPU_TMCR;
	ticr = *M332_TPU_TICR;
	cier = *M332_TPU_CIER;
	cfsr0 = *M332_TPU_CFSR0;
	cfsr1 = *M332_TPU_CFSR1;
	cfsr2 = *M332_TPU_CFSR2;
	cfsr3 = *M332_TPU_CFSR3;
	hsqr0 = *M332_TPU_HSQR0;
	hsqr1 = *M332_TPU_HSQR1;
	hsrr0 = *M332_TPU_HSRR0;
	hsrr1 = *M332_TPU_HSRR1;
	cpr0 = *M332_TPU_CPR0;
	cpr1 = *M332_TPU_CPR1;
	cisr = *M332_TPU_CISR;

	/* call assembly lang routine to get CPU status register */
	CPUstatus = getCPUstatus();

	pOffData = (unsigned short *)&(M332_TPU_CHN4->tf.qom.refLastOffAdrs);
	for(i=0; i < 8; i++) ch4[i] = *(pOffData+i);

	pOffData = (unsigned short *)&(M332_TPU_CHNe->tf.qom14.refLastOffAdrs);
	for(i=0; i < 16; i++) ch14_15[i] = *(pOffData+i);

	printf("-------------------------------------------------------------\n\n");

	if (pTheSpinObject != NULL)
	{
                printf("Spinner Type: '%s', Mode: '%s', MAS switchover threshold: %d\n",
			( (pSpinObj->SPNtype == MAS_PROBE) ? "MAS" : "LIQUIDS"),
			( (pSpinObj->SPNmode == SPEED_MODE) ? "Speed" : "Rate"),
			MASthres); 
		printf("TPU: %s, heart=%ld(%ld)[%ld], PIDe=%ld, PIDp=%ld, pPID=%08lx\n",
			pSpinObj->pIdStr,
			Lheart,Leeg1,Leeg2,
			pSpinObj->PIDelements,pSpinObj->PIDposition,pSpinObj->pPIDE);

		printf("     sum=0x%08lx, period=0x%08lx, regulate=0x%08lx, count=%ld\n",
			Spinsum, Spinperiod, Spinregulate, Spincount);

	printf("     pideeg=%ld, masdac=0x%04x(%d)\n",
			Lpidheart,masdac,masdac);

		printf("     nbr=0x%2lx, index=0x%2lx, speed=%2ld.%ld, speedSet=%2ld, CPUstatus=%4x\n",
			Spinnbr, Spinindex, Spinspeed, Spinspeedfrac, Spinspeedset, CPUstatus);

		printf("     error=0x%08lx, prop=%08lx, integ=0x%08lx, deriv=0x%08lx, cor=0x%08lx\n",
			Spinerror, Spinproportion, Spinintegral, Spinderivative, Lcor);

		printf("     TCR1x10=0x%08lx, TCR2x10=0x%08lx, Regs=%ld\n",
			Ltcr1x10, Ltcr2x10, Loffregs);

		printf("     500ms=0x%08lx, 9ms=0x%08lx, ejectf=%d, dtime=%d, ddelay=%d\n",
			L500ms, L9ms,
			pSpinObj->Vejectflag,pSpinObj->Vdroptime,pSpinObj->Vdropdelay);

		printf("     kO=%ld, kF=%ld, kPu=%ld, kPd=%ld, kIu=%ld, kId=%ld, kDu=%ld, kDd=%ld, tol=0x%x, \n     T=%d, R=%d, W=%ld, kCu=%ld, kCd=%ld\n",
			SPNkO, SPNkF, SPNkPu, SPNkPd, SPNkIu, SPNkId, SPNkDu, SPNkDd,
				Spintolerance,spntype,spnrevs,spnwin,SPNkCu,SPNkCd);

		printf("\n periods:");
		for(i=0;i<SPNperiodSize;i++)
		{
			if( (i & 3) == 0)
			{
				printf("\n            ");
			}
			printf(" %08x",databuf[i]);
		}

	}

	printf("\nCH4 values:\n             ");
	for(i=0;i<8;i++) printf(" %04x",ch4[i]);

	printf("\nCH14 values:");
	for(i=0;i<16;i++)
	{
		if( (i & 7) == 0)
		{
			printf("\n            ");
		}
		printf(" %04x",ch14_15[i]);
	}

	printf("\nTPU registers:");

	printf("\n       TMCR = %04x,  TICR = %04x,  CIER = %04x,  CISR = %04x\n",
		tmcr,ticr,cier,cisr);

	printf("      CFSR0 = %04x, CFSR1 = %04x, CFSR2 = %04x, CFSR3 = %04x\n",
		cfsr0,cfsr1,cfsr2,cfsr3);

	printf("      HSQR0 = %04x, HSQR1 = %04x, HSRR0 = %04x, HSRR1 = %04x\n",
		hsqr0,hsqr1,hsrr0,hsrr1);

	printf("       CPR0 = %04x,  CPR1 = %04x,\n",
		cpr0,cpr1);

	getStat();

	printf("\n-------------------------------------------------------------\n\n");

/*
	sprintf(tmpbfr,"       %s\n",(status & TPU_INTR_PEND) ? "Interrpt Pending" :  "No Interrupts Pending");
	logMsg(tmpbfr);
	return;
*/
}

resetRMS()
{
    RMSerror = ErrSum =  0.0;
    sumdiv = 0L;
}
calcRMSerror()
{
   double SpdErr;
   double sqrt(double);

   SpdErr = ((double)SpeedError)/1000.0;
   ErrSum = ErrSum + (SpdErr * SpdErr);
   sumdiv = sumdiv + 1L;
   RMSerror = sqrt( (ErrSum/ (double) sumdiv) );
   /* printf("RMS error: %lf, %ld samples, error: %lf\n",RMSerror,sumdiv,SpdErr);  */
}
/* end of spinObj.c */
