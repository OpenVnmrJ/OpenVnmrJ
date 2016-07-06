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

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <taskLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "nvhardware.h"
#include "master.h"
#include "spinner.h"
#include "spinObj.h"
#include "hostAcqStructs.h"
#include "errorcodes.h"
#include "taskPriority.h"

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)
#define VPSYNRO(arg) volatile unsigned const int const *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

extern int *pMASTER_InterruptStatus;
extern int *pMASTER_InterruptEnable;
extern int *pMASTER_InterruptClear;

VPSYN(MASTER_SamplePresent);
VPSYN(MASTER_SpinnerPulseSelect);
VPSYN(MASTER_SpinnerPhase);
VPSYN(MASTER_SpinnerEnable);
VPSYN(MASTER_SpinnerSpeed);
VPSYN(MASTER_SpinnerCount);
VPSYN(MASTER_SampledSpinnerSpeed);
VPSYN(MASTER_SpinnerCountThreshold);
VPSYN(MASTER_PulseGenerationEnable);
VPSYN(MASTER_SpinPulseLow);
VPSYN(MASTER_SpinPulsePeriod);

extern int failAsserted;

#define SpinLED_STOP 0		/* Console Display LED OFF */
#define SpinLED_BLINKshort 1	/* Console Display LED BLINK ON short */
#define SpinLED_BLINKlong 2	/* Console Display LED BLINK ON long */
#define SpinLED_OK 3		/* Console Display LED ON "Regulating" */

#define spinDPRINT(level, str) \
        if (spinDebugLevel > level) diagPrint(debugInfo,str)

#define spinDPRINT1(level, str, arg1) \
        if (spinDebugLevel > level) diagPrint(debugInfo,str,arg1)

#define spinDPRINT2(level, str, arg1, arg2) \
        if (spinDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2)

#define spinDPRINT3(level, str, arg1, arg2, arg3) \
        if (spinDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3)
 
#define spinDPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (spinDebugLevel > level) diagPrint(debugInfo, str,arg1,arg2,arg3,arg4)
 
#define spinDPRINT5(level, str, arg1, arg2, arg3, arg4, arg5 ) \
        if (spinDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5)
 
static spinDebugLevel = 0;

// Diagnostic enabling routines
setSpinDebug(int level) { spinDebugLevel = level; return level; }
getSpinDebug(int level) { return spinDebugLevel; }
spinDebugOn() { spinDebugLevel = 5; return 5; }
spinDebugOff() { spinDebugLevel = 0; return 0; }


extern Console_Stat	*pCurrentStatBlock;	/* Acqstat-like status block */

int shimDebug=0;
long lastSetSpeed = -1;

SPIN_ID pTheSpinObject;
static char *SpinIDStr ="Spin Object";
static long SpeedError;
static int SpininterLck = 0;

/* for RMS error calculations, for testing */
static double RMSerror;
static double ErrSum;
static ulong_t sumdiv;

static WDOG_ID wdSpinChk;

#ifdef SCOPE
static long GlobalOffTime;
#endif

long DAC2MAS(long DACvalue);
/*-----------------------------------------------------------
|
|  Internal Functions
|
+---------------------------------------------------------*/
/*******************************************
*
* SpinUpdate - Interrupt Service Routine for Liquids Spinner
*
*   SpinUpdate - Signals the completion of a n revolutions count.
*                Averges 16 measurements
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void SpinUpdate(int intStatus, register SPIN_ID pSpinId)
{
register long accum;
register long ndx;
register long x;

   accum = *pMASTER_SampledSpinnerSpeed;

      /* Now process data */
   ndx = pSpinId->SPNindex;            /* get index to new/nth value */
   /* x = pSpinId->SPNperiods[ndx];       /* fetch -nth value (ndx_max = 16) */
   /* pSpinId->SPNsum -= x;               /* subtract it from sum */
   pSpinId->SPNsum -= pSpinId->SPNperiods[ndx];   /* subtract it from sum */
   pSpinId->SPNsum += accum;           /* new sum */
   pSpinId->SPNperiods[ndx++] = accum; /* save new value & point to next */

   if( ndx >= pSpinId->SPNnbr)         /* if at end reset else next cell */
   {
      pSpinId->SPNcount += 1;          /* count times thru array */
      ndx = 0;                         /* reset index to 0 */
   }
   pSpinId->SPNindex = ndx;            /* update index */
   pSpinId->SPNheartbeat++;            /* update interrupt rtn heartbeat */

   *pMASTER_SpinnerEnable = 0;            /* reset to count from 0 */
   *pMASTER_SpinnerEnable = 1;
   /* before returning, clear the interrupt 
   set_field(MASTER,spinner_int_clear,0);
   set_field(MASTER,spinner_int_clear,1); */
   return;
}

/**************************************************************
*
*  spinItrpEnable - Set the spin_count Interrupt Mask
*
* RETURNS:
* void 
*
*/
void spinItrpEnable()
{
void spinItrpDisable();
int *countEnable, *intEnable;
   if (pTheSpinObject == NULL) return;

   spinItrpDisable();

   pTheSpinObject->SPNitrHndlr = (int)SpinUpdate;
   fpgaIntConnect(SpinUpdate,pTheSpinObject,(1<<MASTER_spinner_int_enable_pos));
   *pMASTER_SpinnerEnable = 0;
   *pMASTER_SpinnerEnable = 1;
   set_field(MASTER,spinner_int_enable,1);
}


/**************************************************************
*
*  spinItrpDisable - Set the spin_count Interrupt Mask
*
* RETURNS:
* void 
*
*/
void spinItrpDisable()
{
   set_field(MASTER,spinner_int_enable,0);
}


/**************************************************************
*
*  SpinReset - stops interrupt and counters
*
*
* RETURNS:
* 
*/
void spinReset()
{
   spinItrpDisable();
   *pMASTER_SpinnerEnable = 0;
}


/********************************************************
*


/*-------------------------------------------------------------
| Vertical Init Public Interfaces
+-------------------------------------------------------------*/

initVertical()
{
register SPIN_OBJ *pSpinObj;
/* register short qomValue; */
void initLED();

	if (pTheSpinObject == NULL) return;

	pSpinObj = pTheSpinObject;

/* kill all spin functions before seting up for Vertical */
	spinReset();	/* kill all Probe specific functions */

/* ------- set type before going any further */
	pSpinObj->SPNrevs = VMC_REVS; /* set revs for Vertical */
        *pMASTER_SpinnerCountThreshold = VMC_REVS;

	pSpinObj->SPNtype = LIQUIDS_SPINNER;
        hsspi(2,0x120000);		// rotation DAC to zero
        *pMASTER_SpinnerPulseSelect = 1;
DPRINT(-1,"Setiing LIQUIDS spinner\n");
	hsspi(2,0x1000001);

/* Set defaults for P, I, D and O plus tolerance */
	pSpinObj->SPNkO = 1;
	pSpinObj->SPNkF = 9;

	pSpinObj->SPNkP[UPk] = 1; 
	pSpinObj->SPNkP[DOWNk] = 1;
	pSpinObj->SPNkD[UPk] = 11;
	pSpinObj->SPNkD[DOWNk] = 11;
	pSpinObj->SPNkI[UPk] = 0x7FFFFFFF;
	pSpinObj->SPNkI[DOWNk] = 0x7FFFFFFF;

	pSpinObj->SPNtolerance = 1000;  /* 1 Hz */
	pSpinObj->SPNsetting = 8916;    /* .8916 % duty cycle */
	pSpinObj->SPNrateSet = 8916;    /* Air Flow Rate to Go to,
                                           value depends LIQ or MAS */
        pSpinObj->SPNmode = RATE_MODE;	/* in Rate or Speed regualtion mode */
	pSpinObj->PIDerrindex = 0;      /* index into error buffer */
	pSpinObj->PIDerrorbuf[0] = 0L;
	pSpinObj->PIDerrorbuf[1] = 0L;
	pSpinObj->PIDerrorbuf[2] = 0L;
	pSpinObj->PIDerrorbuf[3] = 0L;

	pSpinObj->SPNpefact = 1L;

	pSpinObj->SPNkC[UPk] = 1000000;	/* at 1000000 same as off */
	pSpinObj->SPNkC[DOWNk] = -1000000;

	/* set up spinner LED */
	initLED();

	/* set eegs as to not equal heartbeart, 
          give interrupt routine a chance to start up */
	pSpinObj->SPNheartbeat = 1;
	pSpinObj->SPNeeg1 = 200;
	pSpinObj->SPNeeg2 = 300;
        /* and reenable spinner interrupt routine */
        spinItrpEnable();

} /* end initVertical */

/**************************************************************
*
*  initMAS sets up values functions for MAS probes
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
	pSpinObj->SPNrevs = 40;    /* MASMC_REVS;	/* set revs for MAS */
        *pMASTER_SpinnerCountThreshold = 40;    /* MASMC_REVS; */

	setBearingAir(0);	/* turn bearing air DAC to 0 */
        hsspi(2,0x120000);	// turn rotation air DAC to 0 

DPRINT(-1,"Init TACH spinner\n");
        *pMASTER_SpinnerPulseSelect = 0;
	hsspi(2,0x1000000);	
	pSpinObj->SPNtype = SOLIDS_SPINNER;

	/* for MAS setting is log*10000 of DAC setting */
        /* e.g. 41230 = 4.123 -> 10^4.123 = 13273.94 dac air setting */
	pSpinObj->SPNsetting = 0;      /* 10^3 -> 1000 */
	pSpinObj->SPNrateSet = 0;      /* Air Flow Rate to Go to,
                                          value depends LIQ or MAS */
        pSpinObj->SPNmode = RATE_MODE; /* in Rate  or Speed regualtion mode */
	pSpinObj->PIDerrindex = 0;     /* index into error buffer */
	pSpinObj->PIDerrorbuf[0] = 0L;
	pSpinObj->PIDerrorbuf[1] = 0L;
	pSpinObj->PIDerrorbuf[2] = 0L;
	pSpinObj->PIDerrorbuf[3] = 0L;


/*   Set defaults for P, I, D and O plus tolerance */
/*   these pid work well for both 5mm & 7mm Varian probes, 
/*   7mm there is a slight overshoot, 5mm there is no overshoot
/*   decrease Proportional sensitivity (e.g. > 20) will 
/*   loss ability to regulate well
/*   increase Proportional sensitivity (e.g. < 20) will
/*   increase Derivitive, which creates substantial ringing
/**/
	pSpinObj->SPNkP[UPk] = 20;
	pSpinObj->SPNkP[DOWNk] = 20;
	pSpinObj->SPNkD[UPk] = 40;
	pSpinObj->SPNkD[DOWNk] = 40;
	pSpinObj->SPNkI[UPk] = 0x7FFFFFFF;
	pSpinObj->SPNkI[DOWNk] = 0x7FFFFFFF;
	pSpinObj->SPNkO = 1;
	pSpinObj->SPNkF = 100;

        /* 2 Hz RMS spec,'in=y' regulation crit 100 Hz*/
	pSpinObj->SPNtolerance = 100000; 
	pSpinObj->SPNpefact = 1000L;
	pSpinObj->SPNkC[UPk] = 250;
	pSpinObj->SPNkC[DOWNk] = -250;

	/* set up spinner LED */
	initLED();

	/* set eegs as to not equal heartbeart,
	   give interrupt routine a chance to start up */
	pSpinObj->SPNheartbeat = 1;
	pSpinObj->SPNeeg1 = 200;
	pSpinObj->SPNeeg2 = 300;
        /* and reenable spinner interrupt routine */
        spinItrpEnable();

} /* end initMAS() */

/**************************************************************
*
*  initLED sets up variables for spin indicator L.E.D.s 
*
*
* RETURNS:
*  None.
*
*/ 
void initLED()
{
   *pMASTER_SpinPulsePeriod = 40000000;   /* 1 second period */
   *pMASTER_SpinPulseLow    =        0;   /* on for zero => off for now */
   *pMASTER_PulseGenerationEnable |=  1;   /* Enable the pulser */
}

/**************************************************************
*
*  spinCreate - Create the Object Data Structure & Semiphores
*
*  SPINtype = 1 for Vertical sample control, 2 for MAS control.
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semiphore creation failed
*
*/ 
SPIN_ID  spinCreate()
{
   char tmpstr[80];
   register SPIN_OBJ *pSpinObj;
   short sr;
   int psck;

    /* create the Watch Dog, that is used to Chk on progress of regulating */
    if ((wdSpinChk = wdCreate()) == NULL)
    {
        DPRINT(-1,"wdCreate Error\n");
    }
    

   /* ------- malloc space for Object --------- */
   if ( (pSpinObj = (SPIN_OBJ *) malloc( sizeof(SPIN_OBJ)) ) == NULL )
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Space:");
      return(NULL);
   }

   /* zero out structure so we don't free something by mistake */
   memset(pSpinObj,0,sizeof(SPIN_OBJ));

   /* set the global pointer before calling 
   /* initVertical, initMAS or spinReset etc. */
   pTheSpinObject = pSpinObj;

   pSpinObj->SPNnbr=SPNperiodSize;

   /* ------- point to Id String ---------- */

   pSpinObj->pIdStr = SpinIDStr;

   /* ------- set type before going any further */
   pSpinObj->SPNtype = NO_SPINNER;
   pSpinObj->SPNmasthres = MAS_THRESHOLD; /* >100 Hz (default) switch to MAS */

   /* ------- disable interupt  */
   spinReset();	

   /* ------- Clear MAS DAC till set by program control ---- */
   /* clear struct left MASdac = 0 */
   /* do not clear, same dac is for liq as for sol.
   /* do not clear, but can we read them back? Both bearing and drive.
   /* *(unsigned short *) MPU332_SOLID_DAC = pSpinObj->MASdac;	
   /* */

   /* Set defaults for drop delay etc */
   pSpinObj->Vdroptime = 240;	/* wait 4 sec between EJECT off and SDROP off */
   pSpinObj->Vdropdelay = 30;	/* wait 1/2 sec after SDROP till regulation */
   pSpinObj->PIDmode = 30;	/* start here and experiment */
   pSpinObj->bearLevel = 0xc000; /* default bearing air is on dac value */

   switch (pSpinObj->SPNtype)
   {
      /* to determine Vertical spinner speed */
      case LIQUIDS_SPINNER:
	    initVertical();
	    break;

     /* to determine MAS spinner speed */
     case SOLIDS_SPINNER:
	    initMAS();
            break;
   }

   /* ------- Connect interrupt vector to proper ISR ----- */
   /* spinItrpEnable(); */

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
#endif


  return( pSpinObj );
}


/**************************************************************
*
*  SpinDelete - Deletes Spin Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*/
int spinDelete()
{
   if (pTheSpinObject == NULL) return;

   spinReset(); /* kill interrupts and diable timer functions */
   free(pTheSpinObject);
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

double drate,spinHz;
   pSpinObj = pTheSpinObject;

   if( (pSpinObj->SPNperiod = pSpinObj->SPNsum / pSpinObj->SPNnbr) > 0L)
   {
      spinHz = 8.0e7 * (double)pSpinObj->SPNrevs / (double)pSpinObj->SPNperiod;
      if( pSpinObj->SPNtype == LIQUIDS_SPINNER)
         spinHz /= 2.0;
     
      speed1000 = (int) (spinHz * 1000.0);
  //    pSpinObj->SPNspeedfrac = speed1000 % 1000L;
      pSpinObj->SPNspeed = speed1000;
      if(shimDebug > 0)
      {
         index = (int)pSpinObj->SPNindex;
         printf(
         "sum = %ld, count = %ld, index = %ld, speed = %ld.%ld, last = %ld, \n",
		pSpinObj->SPNsum,pSpinObj->SPNcount,index,pSpinObj->SPNspeed,
		pSpinObj->SPNspeedfrac,pSpinObj->SPNperiods[index]);
      }
   }
   else
   {
      speed1000 = 0L;
      spinHz = 0.0;
      pSpinObj->SPNspeedfrac = 0L;
      pSpinObj->SPNspeed = 0L;
      if(shimDebug > 0) printf("Sum is Zero\n");
   }
   pCurrentStatBlock->AcqSpinAct = (int)(spinHz+0.1);
   return((speed1000/1000));
}	/* end getSpeed() */

int getSpinInterlk(int in)
{
   return (SpininterLck);
}

void setSpinInterlk(int in)
{
   SpininterLck = in;
}

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


/* setSpinLED() sets the SpinLED according to the value
|   in pSpinObj->SpinLED. 
|    0=off   (Stopped)
|    1=ONshortOFFlong
|    2=OFFshortONlong,
|    3=ON.
|
*/
#define LSDV_SPIN_OFF	0x00
#define LSDV_SPIN_REG   0x10
#define LSDV_SPIN_UNREG 0x20
#define LSDV_SPIN_MASK  0x30
int setSpinLED()
{
register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   switch( pSpinObj->SpinLED)
   {
      case SpinLED_STOP:
           if ((pSpinObj->SPNspeedSet == 0) || (pSpinObj->Vejectflag))
           {   *pMASTER_SpinPulseLow = 40000000; /* low full period of 1 sec */
           }
           else
           {
               *pMASTER_SpinPulseLow = 26600000; /* low for 2/3, high 1/3 */
           }
           pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
           break;

      case SpinLED_BLINKshort:
           *pMASTER_SpinPulseLow = 26600000;     /* low for 2/3, high 1/3 */
           pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
           pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_UNREG;
           break;

      case SpinLED_BLINKlong:
           *pMASTER_SpinPulseLow = 13300000;     /* low for 1/3, high 2/3 */
           pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
           pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_UNREG;
           break;

      case SpinLED_OK:
           *pMASTER_SpinPulseLow =        0;    /* low for 0, high 1 sec */
           pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
           pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_REG;
           break;
   }
   return((int)OK);
}


/* getSpinLED() returns the current mode of the SpinLED */
long getSpinLED()
{
   if (pTheSpinObject == NULL) return(ERROR);

   return(pTheSpinObject->SpinLED);
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
                taskDelay(calcSysClkTicks(166));  /* 166ms, wait for a concurent update to finish */
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
                taskDelay(calcSysClkTicks(166));  /* 166ms, wait for a concurent entry to finish */
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
			return((int)NULL);
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
         pPIDe[pSpinObj->PIDposition].PIDintegral   = pSpinObj->SPNintegral;
         pPIDe[pSpinObj->PIDposition].PIDderivative = pSpinObj->SPNderivative;
         pPIDe[pSpinObj->PIDposition].PIDlerror     = pSpinObj->SPNlerror;
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


/* pidlogPrint() Prints the PID values in the PID log. */
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
   	printf("\nPID TCR2x10, array data:\n");
   
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

/*******************************************************************************
|  the task function ChkSPNspeed() is invoke every sec.
|  which in turn calls chkSpeed() (the main function of the speed control task)
|  which calls spinPID() for the correction to apply
*/

/* long spinPID() returns a correction value 0=no correction.
|
|     This routine DOES NOT correct the speed! 
|     It only returns a 2's complement value of the correction
|     to the present setting.
|     For Liquids the setting and correction are in terms of 
|     duty cycle of the pulser valve, note duty is in milliDuty 
|     (i.e. take SPNsetting/1000.0 gives duty)
|     For MAS the setting and correction are in terms of log10 of 
|     the DAC setting of the valve.
|     Note: SPNsetting 37000 is log10(3.7) -> dac value -> ~ 5012
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
|  SpeedError = set speed ('speedSet) - current speed ('sum)
| 'proportion = SpeedError / 'kP[n]    where n is index to correct kP
| 'integral = (SpeedError / 'kI[n] ) + 'integral
| 'derivative = ( SpeedError - 'lerror ) * 'kD[n]
| Correction = 'proportion + 'integral + 'derivative
*/

   if( (SpeedError = (pSpinObj->SPNspeedSet - pSpinObj->SPNspeed)) < 0L)
      UorD = 1;	/* going up, use up(faster) index */
   else
      UorD = 0;	/* going down, use down(slower) index */

   
   /* use this to calc RMS Error, allow speed to get into regulation 
      then call resetRMS() */
   /* calcRMSerror(); */  

   pSpinObj->PIDerrorbuf[pSpinObj->PIDerrindex] = SpeedError;
   pSpinObj->PIDerrindex = pSpinObj->PIDerrindex + 1;
   if (pSpinObj->PIDerrindex >= PID_ERROR_BUFSIZE)
   {
      pSpinObj->PIDerrindex = 0;
   }

   if (pSpinObj->SPNtype == LIQUIDS_SPINNER)
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
   printf("Int (%ld) / kI (%ld) / kF (%ld)\n",
			Integral,pSpinObj->SPNkI[UorD],pSpinObj->SPNkF);
*/
   Integral = ((Integral / pSpinObj->SPNkI[UorD]) / pSpinObj->SPNkF );

   pSpinObj->SPNintegral = Integral;

/*
   printf("Deriv = ( (Err (%ld) - PrevErr (%ld)) / kD (%ld)) ) / kF (%ld)\n",
	PeriodError,pSpinObj->SPNlerror,pSpinObj->SPNkD[UorD],pSpinObj->SPNkF);
*/
   if (pSpinObj->SPNtype == LIQUIDS_SPINNER)
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
   printf("Present: %ld (%d Hz*10), Error: %ld\n",
            pSpinObj->SPNperiod, getSpeed(), SpeedError);
   printf("P: %ld, I: %ld, D: %ld, T: %ld\n", pSpinObj->SPNproportion, Integral,
            pSpinObj->SPNderivative, Correction);
*/
   return(Correction);  /* correction is a delta duty cycle */
}	/* end long spinPID() */


int setOffTime(long OffTime)
{
register SPIN_OBJ *pSpinObj;
unsigned short oneOffTime;
int i;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   switch (pSpinObj->SPNtype)
   {
   case LIQUIDS_SPINNER:
#ifdef SCOPE
      	GlobalOffTime = OffTime;
#endif
        pSpinObj->driveDAC = OffTime;
	hsspi(2,0x120000 | OffTime);
        break;
   case SOLIDS_SPINNER:
        pSpinObj->MASdac = (unsigned short)OffTime;
	hsspi(2, 0x120000 | pSpinObj->MASdac);
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
long MAS2DAC(long);
long rateRamp(void);
long tmpval,deltaval,deltaspd,ddelta;

int	*xyz;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   pSpinObj->PIDheartbeat++;	/* Make this heart beat. */
   Correction = 0L;	/* in case we don't call spinPID for pidlog. */
/* print a bunch of regs */
/*    xyz = (int *)0x7000009c;
/*    DPRINT4(-5,"9c=%x, a0=%x, a4=%x, a8=%x\n", *xyz++,*xyz++,*xyz++,*xyz++);
/*    DPRINT3(-5,"ac=%x, b0=%x, b4=%x\n",        *xyz++,*xyz++,*xyz++);
*/

   /* ----------  Determine if Spinning -------------------------------*/

   /* set for next pass confirmation */
   /* check if spinner is actually rotating or is stopped */
   spinDPRINT3( 2,
           "SPNhbeat: %lu, SPNeeg1: %lu, SPNeeg2: %lu\n",pSpinObj->SPNheartbeat,
	   pSpinObj->SPNeeg1,pSpinObj->SPNeeg2);
   /* if speed interrupt heart beat == eeg1 then 
      no interrupt for 1 sec, rotor may be stopped */
   /* if speed interrupt heart beat == eeg1 == eeg2 then 
      no interrupts for 2 sec rotor is not rotating ! */
   if( (pSpinObj->SPNheartbeat == pSpinObj->SPNeeg1) &&
			(pSpinObj->SPNheartbeat == pSpinObj->SPNeeg2) )
   {
      /* more than 1 sec has passed so the speed must be less that 1 Hz */
      /* pSpinObj->SPNsum = 0L; */
      pSpinObj->SPNspeed = 0L;
      pSpinObj->SPNspeedfrac = 0L;
      /* set period to equivilent of 1 Hz (= 80,000,000 * 2) */
      pSpinObj->SPNperiod = 80e6  * pSpinObj->SPNrevs;
      pSpinObj->SPNsum = pSpinObj->SPNperiod*pSpinObj->SPNnbr;
      for (pSpinObj->SPNindex=0; 
	   pSpinObj->SPNindex < pSpinObj->SPNnbr - 1;
           pSpinObj->SPNindex++)
      {
         pSpinObj->SPNperiods[pSpinObj->SPNindex] = pSpinObj->SPNperiod;
      }
      /* get the last one with no increment */
      pSpinObj->SPNperiods[pSpinObj->SPNindex] = pSpinObj->SPNperiod; 

      spinDPRINT1( 2,"No HEART - spinLED: %d\n",pSpinObj->SpinLED);
      if ((pSpinObj->SPNspeedSet != 0) && (pSpinObj->SPNtype == SOLIDS_SPINNER))
      {
         spinDPRINT(0,"No Rotor Speed Detected, SpeedShutdown MAS Spinner\n");
         pSpinObj->SPNrateSet = 0;
         pSpinObj->SPNmode = RATE_MODE;
         pSpinObj->PrevSpeed = 0L;
      }
      *pMASTER_SpinnerEnable = 0;            /* reset to count from 0 */
      *pMASTER_SpinnerEnable = 1;		/* end */
   }
   else
   {
      spinDPRINT1( 2,"Maybe HEART - spinLED: %d\n",pSpinObj->SpinLED);
      /* If we got here, things may be running */
      /* update eeg1 with spin intrp heartbeat */
      if (pSpinObj->SPNeeg1 != pSpinObj->SPNheartbeat)
      {
         pSpinObj->SPNeeg1 = pSpinObj->SPNheartbeat;
      }
      else
      {
         /* no interrupt for speed check interval (1sec) 
         /* set eeg2  thus if heartbeat == eeg1 == eeg2 
         /* then rotor not spinning */
	 pSpinObj->SPNeeg2 = pSpinObj->SPNeeg1; /*NO,update eeg*/
#ifdef SCOPE
	 ScopeCollectSignals(0);
#endif
	 return(OK);
      }
   }
   /*------- Determine current speed -----------------------------------*/
   tmpval = getSpeed();	/* calculate current speed and period */
   SpeedError = pSpinObj->SPNspeedSet - pSpinObj->SPNspeed;
   spinDPRINT1( 3,"getSpeed() returned %d\n",tmpval);
   /* ----------  Set the Blinking Lights Here -------------------- */
   /* If speed increasing then fast blinking, slowing slow blinking */
   if ( (abs(SpeedError) <= pSpinObj->SPNtolerance) &&
            (pSpinObj->SPNmode != RATE_MODE) )
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

   /* --------- Determine if we want/need to correct --------------- */

   /* If sample is ejected or dropping into magnet
      or the bearing air is off, 
      don't bother trying to regulate it */
   spinDPRINT2( 3,"ejectDac: %d, bearDac: %d\n", pSpinObj->ejectDAC, pSpinObj->bearDAC);
   if (( pSpinObj->ejectDAC > 0) ||
       ( (pSpinObj->bearDAC == 0) && (pSpinObj->SPNtype == LIQUIDS_SPINNER) ) )
   {
      pSpinObj->Vejectflag = 1;
#ifdef SCOPE
      ScopeCollectSignals(0);
#endif
      return(OK); /* Don't mess with air if Eject or Sdrop set */
   }
   if( pSpinObj->Vejectflag == 1)
   {
      /* If there is a sample in the probe again,
         delay a while before resuming */
      pSpinObj->Vejectflag = 0;
      if( pSpinObj->Vdropdelay > 0) taskDelay( pSpinObj->Vdropdelay);
   }

   spinDPRINT2( 3,"chkSpeed- Type: '%s', Mode: '%s'\n",
		( (pSpinObj->SPNtype == SOLIDS_SPINNER) ? "SOLIDS" : "LIQUIDS"),
		( (pSpinObj->SPNmode == SPEED_MODE) ? "Speed" : "Rate") );
   spinDPRINT5( 3,"   Speed:%ld SetSpeed:%d rateSet:%d Setting:%d DAC:%u\n",
		pSpinObj->SPNspeed, pSpinObj->SPNspeedSet, 
		pSpinObj->SPNrateSet,pSpinObj->SPNsetting,pSpinObj->MASdac);

   spinDPRINT5( 3,
      "chkSpeed- Spd Set:%d Tru:%d Er:%d Tol:%d LED:%d (0-S,1-Ac,2-De,3-Reg)\n",
      pSpinObj->SPNspeedSet, pSpinObj->SPNspeed, SpeedError, 
      pSpinObj->SPNtolerance, pSpinObj->SpinLED);


   spinDPRINT4( 3,
            "Mode: %d, DAC: %ld, Speed: %ld, delaySet: %ld\n",pSpinObj->SPNmode,
	     pSpinObj->MASdac,pSpinObj->SPNspeed,pSpinObj->SPNdelaySet);

   if ( (pSpinObj->SPNtype     == SOLIDS_SPINNER) &&
	(pSpinObj->SPNmode     == RATE_MODE) &&
	(pSpinObj->SPNdelaySet != 0L) &&
	(pSpinObj->SPNspeed    >= 600000 /* 600 Hz */) )
   {
      wdCancel(wdSpinChk);   /* cancel wdog, have succeeded in spinning */
      spinDPRINT1(0,"Rate switching back to Speed: %ld --------\n",
				pSpinObj->SPNdelaySet);
      setSpeed(pSpinObj->SPNdelaySet);
      pSpinObj->SPNdelaySet = 0L;
   }


   /* ------ Now Time to Control the Spinner, Either by Rate or Speed ---*/
   if (pSpinObj->SPNmode == RATE_MODE)
   {
#ifdef SCOPE
      ScopeCollectSignals(0);
#endif
      /* ramp rate if neccearrary */	
      switch (pSpinObj->SPNtype)
      {
      case LIQUIDS_SPINNER:

	   /* Old system set the speed to 0 when told to set the rate
    	      to 0.  New system needs to do this or the sample spins
    	      at an unacceptably high speed (50 or more Hz).  08/04/95 */

	   if (pSpinObj->SPNsetting != pSpinObj->SPNrateSet)
           {
	      spinDPRINT1( 1,"Set Liq Spin Rate: %d\n",pSpinObj->SPNrateSet);
	      if (pSpinObj->SPNrateSet <= 0L)
	      {
		 spinDPRINT(1,"Turn off bearing air\n");
	         /* find the last offset used and kill the Rising edge flag */
	         /* This will actually shut the air off. When speed is set */
	         /* to a positive value, setOffTime will turn air back on. */
                 setBearingAir(0);	/* bearing air off too */
                 pSpinObj->SPNsetting = 0;
                 setOffTime(0);		/* rotation/drive DAC to 0 */
	      }
              else
              {
	         if (pSpinObj->SPNrateSet > 0xFFFF )
                    pSpinObj->SPNrateSet = 0xFFFF;

	         if (pSpinObj->SPNrateSet < 0 )
                    pSpinObj->SPNrateSet =  0;

		 setOffTime(pSpinObj->SPNrateSet);
		 pSpinObj->SPNsetting = pSpinObj->SPNrateSet;
              }
  	   }
	   break;

      case SOLIDS_SPINNER:
       	   spinDPRINT4(1,
            "SOLIDS Rate- Target: %d, Tru: %d, Setting: %ld, Correction: %ld\n",
           pSpinObj->SPNrateSet,pSpinObj->MASdac,
           pSpinObj->SPNsetting,Correction);
	   if (pSpinObj->MASdac != pSpinObj->SPNrateSet)
           {
	      tmpval = pSpinObj->MASdac;
              if ( (pSpinObj->SPNrateSet == 0) && (pSpinObj->MASdac < 500) )
              {
                 pSpinObj->SPNsetting = 0;
              }
              else
              {
	         Correction = rateRamp();
                 pSpinObj->SPNsetting += Correction;
              }
              setOffTime(MAS2DAC(pSpinObj->SPNsetting));
              pSpinObj->PrevDacVal = tmpval;
	   }
       	   spinDPRINT4( 2,
            "SOLIDS Rate- Target: %d, Tru: %d, Setting: %ld, Correction: %ld\n",
                pSpinObj->SPNrateSet,pSpinObj->MASdac,
                pSpinObj->SPNsetting,Correction);
	   break;
      }

      pidlogFill();	/* if any log or console print, do it! */
   }
   else  /* Regulate Speed */
   {
      if ( (tmpval == 0) && (pSpinObj->SPNtype == LIQUIDS_SPINNER) &&
           (pSpinObj->SPNsetting == 65535) && (pSpinObj->SPNspeedSet > 0) &&
           detectSample() )
      {
          spinDPRINT(1,"Reset spinner setting\n");
          pSpinObj->SPNsetting = 8916;
          pSpinObj->SPNrateSet = 8916;
          setOffTime(8916);		/* rotation/drive DAC to 0 */
      }

#ifdef SCOPE
	ScopeCollectSignals(0);
#endif
      pSpinObj->PIDheartbeat++;	/* Make this heart beat faster if correcting. */

      Correction = spinPID();
              
      spinDPRINT2( 3,"chkSpeed - setting: %ld, correction: %ld\n",
			pSpinObj->SPNsetting,Correction);
      pSpinObj->SPNsetting += Correction;

      switch (pSpinObj->SPNtype)
      {
      case LIQUIDS_SPINNER:
           /* min duty cycle, 9 msec on - 1 sec off */
           if (pSpinObj->SPNsetting < 8916L) 
              pSpinObj->SPNsetting = 8916L;
	   /* max duty cycle 9 msec on - 9 msec off */
           if (pSpinObj->SPNsetting > 65535)
              pSpinObj->SPNsetting = 65535;
           /* double check, we multi task with X_interp */
           if ( pSpinObj->ejectDAC > 0)
              pSpinObj->SPNsetting = 0;

           setOffTime(pSpinObj->SPNsetting);
           break;

      case SOLIDS_SPINNER:
	   deltaspd = abs((pSpinObj->SPNspeed/1000)-(pSpinObj->PrevSpeed/1000));
           /* If current speed is slower than 4 tolerances away from target
            * and the change in speed from the previous setting is less than
            * 1/10 of the distance it needs to go, give an extra boost to the
            * DAC. The SPNspeedSet test restricts this to nano probes, which
            * never spin faster than 3500.
            */ 
	   spinDPRINT3(2,"Speed error: %d, 4*tol: %d, delta: %ld/sec",
                     SpeedError, 4 * pSpinObj->SPNtolerance, deltaspd);
           if ( (pSpinObj->SPNspeedSet < 3500000) &&
                (SpeedError > 4 * pSpinObj->SPNtolerance ) &&
                (deltaspd <  SpeedError / 10000) ) 
           {
              if (deltaspd < 2)
                 pSpinObj->SPNsetting +=  SpeedError / 125;
              else if (deltaspd < 6)
                 pSpinObj->SPNsetting +=  SpeedError / 250;
              else
                 pSpinObj->SPNsetting +=  SpeedError / 500;
	      spinDPRINT1(2,"new setting %ld ", pSpinObj->SPNsetting);
           }
           if (pSpinObj->SPNsetting < 0L)	/* min DAC value */
              pSpinObj->SPNsetting = 0L;
           if (pSpinObj->SPNsetting > 0xFFFF)	/* max DAC value */
              pSpinObj->SPNsetting = 0xFFFF;
	   tmpval = MAS2DAC(pSpinObj->SPNsetting);
           /* For nano probes, if DAC is max because previous speed
            * could not be reached, when lowering speed, quickly get
            * to a setting where the DAC value will start to decrease
            */
           if ( (pSpinObj->SPNspeedSet < 3500000) &&
                (tmpval == 0xFFFF) && (Correction < -10) &&
                (pSpinObj->SPNsetting > 48160) )
           {
              pSpinObj->SPNsetting = 48160;
	      tmpval = MAS2DAC(pSpinObj->SPNsetting);
           }
           /* Changing speed resets PrevDacVal = 0. If the spinner goes
            * out of regulation due to this speed change, it used to get
            * the "SOLIDS Rotor Cannot Achieve Speed, Shutdown" error
            * because first deltaval would be large and then on the next
            * pass, ddelta would become large, triggering the error
            */
           if (pSpinObj->PrevDacVal == 0)
           {
              pSpinObj->PrevDacVal = pSpinObj->MASdac;
	      spinDPRINT1(2,"setting PrevDacVal to MASdac %d",
	             pSpinObj->MASdac);
           }
           deltaval = (tmpval - pSpinObj->PrevDacVal);
           spinDPRINT3(2,"deltaval (%ld) = tmpval (%lu) - PrevDacVal (%lu)",
                          deltaval, tmpval, pSpinObj->PrevDacVal);
	   /*
	    printf("=======> prevDAC: %lu, NewDAC: %lu, delta: %ld\n", 
		pSpinObj->PrevDacVal,tmpval,deltaval);
           */
	   spinDPRINT3(2,"======> prevSPD: %lu, NewSPD: %lu, delta: %ld/sec, ",
	             pSpinObj->PrevSpeed/1000,pSpinObj->SPNspeed/1000,deltaspd);
           if (deltaspd == 0 )
              deltaspd = 1;

           ddelta = pSpinObj->PrevDacDelta/deltaspd;
           spinDPRINT3(2,"ddelta (%ld) = PrevDacDelta (%ld) / deltaspd (%ld)",
                          ddelta, pSpinObj->PrevDacDelta, deltaspd);

	   spinDPRINT1(2," dDAC/dSPD: %ld \n", ddelta);

           pSpinObj->PrevDacDelta = deltaval;

	   deltaspd=abs((pSpinObj->SPNspeedSet/1000)-(pSpinObj->SPNspeed/1000));
	   /*
		printf("=======> targetSPD: %lu, presSPD: %lu, delta: %ld\n",
		   pSpinObj->SPNspeedSet/1000,pSpinObj->SPNspeed/1000,deltaspd);
	   */

           /* MAS Safety ShutDown if can't seem to achive speed
            * The SPNspeedSet test skips this for nano probes, which
            * never spin faster than 3500.
            */
           if ( (pSpinObj->SPNspeedSet > 3500000) &&
                (pSpinObj->SpinLED != SpinLED_OK) && 
		( (ddelta > 700) || (tmpval == 0xffff)) ) /* shutdown */
	   {
	      spinDPRINT(0,"SOLIDS Rotor Cannot Achieve Speed, Shutdown\n");
	      spinDPRINT3(0,"ddelta= %ld tmpval= %ld (0x%x)\n", ddelta, tmpval, tmpval);
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

/* getRate returns the current air rate */
unsigned long getRate()
{
register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;
   switch (pSpinObj->SPNtype)
   {
      case LIQUIDS_SPINNER:
        return( (unsigned long) pSpinObj->driveDAC);

      case SOLIDS_SPINNER:
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
		pSpinObj->Vdropdelay = calcSysClkTicks(1000);  /*  1 sec */
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
		pSpinObj->Vdroptime = calcSysClkTicks(4000); /* 4 sec */
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
   spinDPRINT1(2,"Present log setting: %ld\n",logset);

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

        spinDPRINT3(0,"setRate: pRate: %ld, nRate: %ld, SpinType: %d\n",pSpinObj->MASdac,newRate,pSpinObj->SPNtype);

	return(OK);
}

ReStartSpeed(SPIN_OBJ *pSpinObj)
{
   logMsg("-----------------> ReStartSpeed: %ld\n",pSpinObj->SPNdelaySet,2,3,4,5,6);
   setSpeed(pSpinObj->SPNdelaySet);
}

void resetLastSpinSpeed()
{
   /* Called when sample is inserted */
   lastSetSpeed = -1;
}


/* setSpeed(newspeed) places newspeed in global object.
*/
int setSpeed(long newspeed)
{
register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   pSpinObj->SPNdelaySet = 0L;

   *pMASTER_SpinnerEnable = 0;            /* reset to count from 0 */
   *pMASTER_SpinnerEnable = 1;
   if (lastSetSpeed != (newspeed * 1000L) )
      lastSetSpeed = -1;

   if(newspeed <= 0L )
   {
      pSpinObj->SPNdelaySet = 0L;
      pSpinObj->PIDcorrection = 0L; /* set for pidlog */
      pSpinObj->SPNlerror = pSpinObj->SPNintegral = pSpinObj->SPNspeedSet = 0L;
      pSpinObj->SPNrateSet = 0L;
      pSpinObj->SPNmode = RATE_MODE;
      return(OK);
   }
   else	/* speed is valid */
   {
      /* if this is the first speed set, insure integral is non zero */
      /* spinner not spinning ?, yes then set params to allow startup so that
		   safety shutdown does not trigger */
      spinDPRINT3(0,"setSpeed: SPNheartbeat: %d, SPNeeg1: %d ,SPNeeg2: %d\n",
		pSpinObj->SPNheartbeat,pSpinObj->SPNeeg1,pSpinObj->SPNeeg2);
      if( (pSpinObj->SPNheartbeat == pSpinObj->SPNeeg1) &&
	  (pSpinObj->SPNheartbeat == pSpinObj->SPNeeg2) )
      {
         /* set eegs as to not equal heartbeart, 
            give interrupt routine a chance to start up */
	 pSpinObj->SPNheartbeat = 1;
   	 pSpinObj->SPNeeg1 = 200;
   	 pSpinObj->SPNeeg2 = 300;
   	 spinDPRINT(0,"setSpeed: setting SPNheartbeat,SPNeeg1,SPNeeg2\n");
      }

      pSpinObj->SPNspeedSet = newspeed * 1000L;
      pSpinObj->SPNmode = SPEED_MODE;
      if (pSpinObj->SPNtype == LIQUIDS_SPINNER)
         setBearingAir(1);

      spinDPRINT4( 1,"SPNset: %ld, Speed: %d, MASdac: %d, SPNtype: %d\n",
			pSpinObj->SPNspeedSet,pSpinObj->SPNspeed,
			pSpinObj->MASdac,pSpinObj->SPNtype);
      if ( (pSpinObj->SPNtype == SOLIDS_SPINNER) &&
           (pSpinObj->MASdac < 2500 ) && 
	   (pSpinObj->SPNspeed <= 300000 /* 300 Hz */) )
      {
         spinDPRINT(0,"=========> Speed Zero, setting SPNdelaySet <===========\n");
	 pSpinObj->SPNdelaySet = newspeed;   /*speed to regulate to (setspeed)*/
	 spinDPRINT1(0,"delaySet: %ld\n",pSpinObj->SPNdelaySet);
	 setOffTime(2000);
         pSpinObj->SPNsetting = DAC2MAS(2000L);
	 setRate(6000);

	 /* Ok, here's the deal, the nano-probes and Chem-magnetics probes
	    do not spin when the drive air is off, unlike the Varian solids
	    probes, as a result the no spinning speed protection was kicking
            in before the probe had a chance to spin.  The fix is to initially
            set the spinner control into rate mode with a flow that should
            start things, ( protections are disabled in rate mode), once 
            spinner speed has reached over 600 Hz then chkSpeed() switches
            back to Speed regulation mode.  However if for some reason the 
            spinner get spinning the follow watch-dog timer
	    will expire in 17 seconds and shutdown the air flow.
         */ 
         /* Set Spin Check to go off in 17 Seconds */
         if (wdStart(wdSpinChk, (int) (sysClkRateGet() * 17), 
		       (FUNCPTR) ReStartSpeed, (int) pTheSpinObject) == ERROR)
         {
            spinDPRINT(0,"wdStart Error\n");
         }
      }
      pCurrentStatBlock->AcqSpinSet = newspeed;
   }

   pSpinObj->PrevDacVal = pSpinObj->PrevDacDelta = pSpinObj->PrevSpeed = 0L;

   return(OK);

}	/* end setSpeed(long newspeed) */


int getSetSpeed(long *speed, int *mode)
{
register SPIN_OBJ *pSpinObj;
int rmode;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

   /* determine if the speed was set or rate was set */
   spinDPRINT2(2,"getSetSpeed: set speed: %ld, SpinLED: '%s'\n",
	pSpinObj->SPNspeedSet,
	(pSpinObj->SpinLED == SpinLED_OK) ? "Regulated" : "Non-Reg.");

   *speed = pSpinObj->SPNspeedSet/1000L;
   if ( (pSpinObj->SPNspeedSet == 0L) && (pSpinObj->SpinLED != SpinLED_STOP))
   {
      *mode = rmode = RATE_MODE;
   }
   else
   {
      *mode = rmode = SPEED_MODE;
   }
   return(rmode);
}

int setSpinnerType(int spinner)
{
   if (pTheSpinObject == NULL)
       return(ERROR);

   spinDPRINT1(0,"spinner type: %d\n",spinner);
   pCurrentStatBlock->AcqPneuSpinner = spinner;
   if (spinner == SOLIDS_SPINNER) 
   {  if (pTheSpinObject->SPNtype != SOLIDS_SPINNER)
      {
         spinDPRINT(0,"Switching to Solids Spinner\n");
         initMAS();  // Not really MAS, This is the Tach box
         // Put the MAS controller task to sleep
         spinnerType = SOLIDS_SPINNER;
         putMASSpeedToSleep();
         taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
      }
      pTheSpinObject->SPNtype = spinner;
   }
   else if (spinner == LIQUIDS_SPINNER)
   {  if (pTheSpinObject->SPNtype != LIQUIDS_SPINNER)
      {
         spinDPRINT(0,"Switching to Liquids\n");
	 initVertical();
         // Put the MAS controller task to sleep
         spinnerType = LIQUIDS_SPINNER;
         putMASSpeedToSleep();
         taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
      }
      pTheSpinObject->SPNtype = spinner;
   }
   else if (spinner == MAS_SPINNER)
   {
         spinDPRINT(0,"Switching to Mas Spinner\n");
         // Wake up the MAS controller task
         spinnerType = MAS_SPINNER;
         wakeUpMASSpeed();
         taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
         pTheSpinObject->SPNtype = spinner;
   }

   else spinDPRINT1(0,"This is an unknown spinner type (%d)!\n",spinner);
}

setMASThres(int threshold)
{
   register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) 
       return(ERROR);

   pSpinObj = pTheSpinObject;


   spinDPRINT1(0,"setMASThreshold: %d\n",threshold);
   pSpinObj->SPNmasthres = threshold;

   return(0);
}

setSpinRegDelta(int delta)
{
register SPIN_OBJ *pSpinObj;

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;

  /* delta (Hz) * 1000, since 1Hz == 1000 */
   pSpinObj->SPNtolerance = (delta * 1000);
   spinDPRINT1(0,"setSpinRegDelta: SPNtolerance: %d Hz\n",
		(pSpinObj->SPNtolerance/1000));

   return(0);
}

ChkSPNspeed(int showFreq)
{
register SPIN_OBJ *pSpinObj;
int kind;
   pSpinObj = pTheSpinObject;
   FOREVER
   {
      kind = pSpinObj->SPNtype;
      if ( (kind==LIQUIDS_SPINNER) ||
           (kind==SOLIDS_SPINNER)  ||
           (kind==NANO_SPINNER) )
         chkSpeed();
      taskDelay(calcSysClkTicks(1000 * showFreq));  /* 1 sec * showFreq, taskDelay(60 * showFreq); */
   }
}

/*------------------------------------------------------*/
/*  setspinnreg(prtmod) -  Wait for spinner to regulate */
/*                   to the given setting               */
/*                                                      */
/*  This program is only called by auto-shim, not by    */
/*  the A-code interpreter.                             */
/*------------------------------------------------------*/
int setspinnreg(int setspeed, int bumpFlag)
{
int ret;
    setSpeed((long)setspeed);
    ret = spinreg(bumpFlag) ;
    return(ret);
}

/*------------------------------------------------------*/
/* spinreg() -  Wait for spinner to regulate  		*/
/*              to the given setting               	*/
/*                                                 	*/
/*     If speed remains zero for 2 sec, and bumpflag    */
/*     is true, then bump sample several times     	*/
/*                                                      */
/*     If more than 2 attempts to bump fail to make the	*/
/*     sample spin ABORT with failure code         	*/
/*------------------------------------------------------*/
int spinreg( int bumpFlag, int errmode )
{
  int ok,regcnt,setspeed,speed,rate,timeout,regchk,stat,zerocnt,bumpcnt;
  int old_stat,regcount,rtnstat;
  int maxzerocnt;
  register SPIN_OBJ *pSpinObj;

  if (pTheSpinObject == NULL) return;

  pSpinObj = pTheSpinObject;

  ok = regcnt = FALSE;
  zerocnt = 0;                  /* # of times zero spinner speed obtained */
  bumpcnt = 0;                  /* allow only to bump sample twice then fail*/
  rtnstat = 0;

  /* avoid the impression of being slow we do this test up front */
  setspeed = pSpinObj->SPNspeedSet;
  speed    = getSpeed();
  stat  = ( abs(setspeed/1000-speed)<2 );

  spinDPRINT3(1,"spinreg: setspeed: %d, speed: %d, status: %d\n",setspeed,speed,stat);
  if (stat == 1)
  {
    SpininterLck = 2;
    return(rtnstat);
  }

  if ( ! errmode)
  {
     if (setspeed == lastSetSpeed) /* If ignoring errors, only try to regulate once at a given speed */
        return(rtnstat);
     Ldelay(&timeout,6000);        /* allow 60 sec total to regulate if ignoring errors */
  }
  else
  {
     Ldelay(&timeout,12000);       /* allow 2 min total to regulate */
  }
  lastSetSpeed = setspeed;
  old_stat = pCurrentStatBlock->Acqstate;
  pCurrentStatBlock->Acqstate = ACQ_SPINWAIT;
//   getstatblock();   /* force statblock upto hosts */

  if (pSpinObj->SPNtype == SOLIDS_SPINNER)
  {  regcount = 7;      /* check a little longer for MAS solids */
     maxzerocnt = 7;
  }
  else
  {  regcount = 4;      /* liquids */
     if (bumpFlag == 0)
     {   maxzerocnt = 60;	/* try for 1 minute */ 
     }
     else
     {   maxzerocnt = 20;
     }
  }

  while ( (!ok) && (!failAsserted))
  {
     speed = getSpeed();
     stat  = abs(setspeed/1000-speed)<2;
     // getstatblock();   /* force statblock upto hosts */

     spinDPRINT3(1,"spinreg: setspeed: %d, speed: %d, status: %d\n",
				setspeed,speed,stat);
     if ( (speed == -1) )
     {
        rtnstat = SPINERROR + SPINTIMEOUT;
        break;
     }

     if (setspeed > 0)
     {
        if (stat == 1) regcnt++;
        if (speed == 0) zerocnt++;     /* increment if zero speed */
     }
     else
     {
        if (speed == 0) regcnt++;
     }

     if (zerocnt > maxzerocnt)
     {
        if (bumpFlag == 0)
        {

           if (pCurrentStatBlock->Acqstate != ACQ_IDLE)  // may of aborted in the mean time, so check if IDLE
               pCurrentStatBlock->Acqstate = old_stat;
           return(SPINERROR + RSPINFAIL);
        }
        bumpcnt++;
        if (bumpcnt > 3)
        {
           rtnstat = SPINERROR + BUMPFAIL;
           break;
        }
        bumpSample();
        taskDelay(calcSysClkTicks(5000));  /*  5 sec */
        zerocnt = 0;
     }

     spinDPRINT5(1,"SPIN_Reg: speed: %d, stat: %d, regcnt: %d, zerocnt: %d, bumpcnt: %d\n",
          speed,stat,regcnt,zerocnt,bumpcnt);

     if (Tcheck(timeout))
     {
          rtnstat = SPINERROR + RSPINFAIL;
          break;
     }
     if (regcnt > regcount) ok = TRUE;
     taskDelay(calcSysClkTicks(1000));  /* 1 sec, taskDelay(sysClkRateGet() * 1); */
  }

  if (pCurrentStatBlock->Acqstate != ACQ_IDLE)  // may of aborted in the mean time, so check if IDLE
      pCurrentStatBlock->Acqstate = old_stat;

  //getstatblock();   /* force statblock upto hosts */
  SpininterLck = 2;
  return(rtnstat);
}

int bumpSample()
{
register SPIN_OBJ *pSpinObj;
int cnt,stat;
   spinDPRINT(0,"bumpSample()\n");

   if (pTheSpinObject == NULL) return(ERROR);

   pSpinObj = pTheSpinObject;
   if(pSpinObj->SPNtype != LIQUIDS_SPINNER) return;

   hsspi(2,0x14ffff);
   setBearingAir(1);
   taskDelay(calcSysClkTicks(100));  /* taskDelay(6); */
   hsspi(2,0x140000);
   setBearingAir(0);
   taskDelay(calcSysClkTicks(2500));  /* taskDelay(150),  wait 2.5 sec */

   /* check for 2 more sec at most to detect sample in probe */
   for (cnt=0; cnt < 8; cnt++)
   {
       stat = (*pMASTER_SamplePresent & 0x1);
       if (stat == 1)
       {
           spinDPRINT(0,"SampleBump: Sample Detected\n");
           break;
       }
       taskDelay(calcSysClkTicks(500));  /* taskDelay(30); check each .5 sec */
   }
   
   setBearingAir(1);
   return(OK);
}

int startSpin(int chkFreq)
{
int tid;
SPIN_ID s;

   if(spinnerType == MAS_SPINNER) {
      return;
   }
   s = spinCreate();
   if (s == NULL) return;

   if(chkFreq < 1) chkFreq = 1;
   tid = taskSpawn("tSPNspeed",SPIN_TASK_PRIORITY,VX_FP_TASK,XSTD_STACKSIZE,ChkSPNspeed,
                       chkFreq,2,3,4,5,6,7,8,9,10);
   return(tid);
}


SpeedShow(int showFreq)
{
register SPIN_OBJ *pSpinObj;

	if (pTheSpinObject == NULL) return(ERROR);
	pSpinObj = pTheSpinObject;

	FOREVER
	{
		printf("Spinning at %d\n",getSpeed());

		if(pSpinObj->SPNtype == SOLIDS_SPINNER)
			printf(" (%d)\n",getRate());

                taskDelay(calcSysClkTicks(1000 * showFreq));  /* taskDelay(60 * showFreq); */
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

/********************************************************************
* SpinShow - display the status information on the Spin Object
*
*  This routine display the status information of the Spin Object
*
*
*  RETURN
*   VOID
*
*/
void spinShow()
{
register SPIN_OBJ *pSpinObj;
int i;
int avail=ERROR;
char *pstr;
unsigned short status;
unsigned short tmcr,ttcr,dscr,dssr,ticr,cier;
unsigned short cfsr0,cfsr1,cfsr2,cfsr3;
unsigned short hsqr0,hsqr1,hsrr0,hsrr1,cpr0,cpr1,cisr;
long Lheart,Leeg1,Leeg2,Spincount,Spinnbr;
long Spinindex,Spinbear,SpinbearDac,Spinspeed,Spinspeedfrac,Spinspeedset,Spinerror;
long Spintolerance,Spinproportion,Spinintegral,Spinderivative,Lcor;
long SPNkO,SPNkPu,SPNkPd,SPNkIu,SPNkId,SPNkDu,SPNkDd,SPNkF;
long Pelement,Pposition;
long Lpidheart;
unsigned short masdac,drivedac;
int spntype,spnrevs,SPNkCu,SPNkCd,MASthres;
PIDE pPIDe;

long long databuf[SPNperiodSize];
long long Spinsum, Spinperiod;

   printf("\nspinObj.c 9.1 06/25/04\n");

   if (pTheSpinObject == NULL)
   {
   	printf("SpinShow: SPIN Object pointer is NULL.\n");
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
        Spinbear = pSpinObj->bearLevel;
        SpinbearDac = pSpinObj->bearDAC;
   	spnrevs = pSpinObj->SPNrevs;
   	SPNkCu = pSpinObj->SPNkC[UPk];
   	SPNkCd = pSpinObj->SPNkC[DOWNk];
   	masdac = pSpinObj->MASdac;
        drivedac = pSpinObj->driveDAC;
   }

   printf("-------------------------------------------------------------\n\n");

   if (pTheSpinObject != NULL)
   {
      printf("Spinner Type: ");
      switch (pSpinObj->SPNtype)
      {   case 0: printf("NONE, ");
		  break;
          case 1: printf("LIQUIDS, ");
		  break;
          case 2: printf("TACH, ");
		  break;
          case 3: printf("MAS, ");
		  break;
          case 4: printf("NANO, ");
		  break;
          default: printf("Unknown, ");
                  break;
      }
      printf("Mode: '%s'\n",
          ( (pSpinObj->SPNmode == SPEED_MODE) ? "Speed" : "Rate"),
		MASthres); 
      printf("%s, heart=%ld(%ld)[%ld], PIDe=%ld, PIDp=%ld, pPID=%08lx\n",
		pSpinObj->pIdStr, Lheart,Leeg1,Leeg2,
		pSpinObj->PIDelements,pSpinObj->PIDposition,pSpinObj->pPIDE);

      printf("     sum=0x%016llx, period=0x%016llx, count=%ld\n",
		Spinsum, Spinperiod, Spincount);

      printf("     pideeg=%ld, masdac=0x%04x(%d)\n",
				Lpidheart,masdac,masdac);
      printf("     bearLvl=0x%04x(%d),bearDAC=0x%04x(%d),driveDAC=0x%04x(%d)\n",
                   Spinbear,Spinbear,SpinbearDac,SpinbearDac,drivedac,drivedac);

      printf("     nbr=0x%2lx, index=0x%2lx, speed=%2ld.%ld, speedSet=%2ld\n",
		Spinnbr, Spinindex, Spinspeed, Spinspeedfrac, Spinspeedset);

      printf("     error=0x%08lx, prop=%08lx, integ=0x%08lx, deriv=0x%08lx, cor=0x%08lx\n",
		Spinerror, Spinproportion, Spinintegral, Spinderivative, Lcor);

      printf("  ejectf=%d, dtime=%d,ddelay=%d\n",
		pSpinObj->Vejectflag,pSpinObj->Vdroptime,pSpinObj->Vdropdelay);

      printf("     kO=%ld, kF=%ld, kPu=%ld, kPd=%ld, kIu=%ld, kId=%ld, kDu=%ld, kDd=%ld, tol=0x%x, \n     T=%d, R=%d, kCu=%ld, kCd=%ld\n",
		SPNkO, SPNkF, SPNkPu, SPNkPd, SPNkIu, SPNkId, SPNkDu, SPNkDd,
		Spintolerance,spntype,spnrevs,SPNkCu,SPNkCd);

      printf("\n periods:");
      for (i=0;i<SPNperiodSize;i++)
      {
 	 if ( (i & 3) == 0)
	 {
	    printf("\n            ");
	 }
	 printf(" 0x%016llx",databuf[i]);
      }

   }
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
