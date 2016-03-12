/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#include <vxWorks.h>
#include <semLib.h>
#include "logMsgLib.h"
#include "commondefs.h"
#include "m32hdw.h"
#include "mboxcmds.h"
#include "serialShims.h"
#include "hostAcqStructs.h"
#include "instrWvDefines.h"

#define BITS_PER_INT		(32)
#define AUTOSHIM_PRIORITY	(61)
#define AUTOSHIM_TASK_OPTIONS	(0)
#define AUTOSHIM_STACK_SIZE	(2048)
#define LOCK_SYSTEM_FREQ_CONST	(10)	/* 10 Hz, equivalent to 100 ms */
#define LOCK_SYSTEM_DELAY	(sysClkRateGet() / LOCK_SYSTEM_FREQ_CONST)

#define SSHA_Z1_INDEX		(2) /* must match ssha.c in psg */
#define SSHA_Z1C_INDEX		(3)
#define SSHA_Z2_INDEX		(4)
#define SSHA_Z2C_INDEX		(5)
#define SSHA_X1_INDEX		(6)
#define SSHA_Y1_INDEX		(7)
#define MAX_SHIMS_FOR_SSHA	(12)

struct _sshaParams
{
int 	tAutoShimTid;	    /* autoShim task Id */
SEM_ID	autoShimSemId;	    /* autoShim semaphore Id */
SEM_ID	startOfScanId;	    /* startOfScan semaphore Id */
int	sshaConfigured;     /* flag for ssha configured */
int	sshaStarted;        /* ssha start/stop flag during d1 delay */
int	d1time;		    /* d1 delay time in multiples of 10 sec */
int	shimDacIndex_1;     /* shim dac index, used for optimize and shimIsOffset */
/* int	deltaDacValue;      /* amount dac is offset by */
int	shimIsOffset;       /* if shim is offset, reset during abort */
int	autoShimMask[ (MAX_SHIMS_CONFIGURED + BITS_PER_INT - 1) / BITS_PER_INT ]; /* shim mask */
/*			MAX_SHIMS_CONFIGURED=64	 BITS_PER_INT=32 */
/* int	basic_delay = 0;
/* int	settling_delay = 0;
/* int	shimArray[ MAX_SHIMS_FOR_SSHA ];
/* int	shimArrayMask;
*/
};
typedef struct _sshaParams sshaParams;
sshaParams theSshaParams; 	/* could be made extern */
static sshaParams *sshaP;

static int  shimDacIndexTrue[ MAX_SHIMS_FOR_SSHA ]; /* index autoShimMask into true dac numbers */
int	deltaDacValue;      /* amount dac is offset by */

/* below isn't a bad way to encode all shims, but then they will be done only in a certain order;
   need a way to specify order in the future? if we include more than simple shims, then yes. -bf */
static int
locateNextShim( int currentShim )
{
	int	nextShim, startingShim;	/* these variables store shim DAC indices */
	int	bitNumber, wordOffset;	/* these variables locate a bit in the mask */

	startingShim = currentShim+1;
	if (startingShim >= MAX_SHIMS_CONFIGURED)
	  startingShim = 0;

	for (nextShim = startingShim; nextShim < MAX_SHIMS_CONFIGURED; nextShim++) {
		bitNumber = nextShim % BITS_PER_INT;
		wordOffset = nextShim / BITS_PER_INT;

/*	   DPRINT5( 1, "locate next shim: %d %d %d 0x%x 0x%x\n",
			 nextShim, bitNumber, wordOffset, sshaP->autoShimMask[ wordOffset ], (1 << bitNumber) );
*/
		if (sshaP->autoShimMask[ wordOffset ] & (1 << bitNumber))
		  return( nextShim );
	}

	return( -1 );
}

static int
readLockLevel()
{
	LOCK_VALUE_SIZE	lkval;

	lkval = *M32_LOCK_VALUE;
	lkval = (lkval < 0x2000) ? lkval : (lkval - 0x4000);	/* Convert 14-bit to integer value */

	return( (int) lkval );
}

/* JPSG HWSHIMMING   (a.k.a. SSHA) version */
startStopHwShim( value )
{           
   int start;
   start =  value & 0x1;
   DPRINT3(-1,"startStopHwShim: value: 0x%x, mask: 0x%x, start: %d\n",value, (value & 0xfc), start);
   initAutoShimMask( (value & 0xfc) );
   if (start)
   {
       /* shim indefinitely no time limit */
       sshaP->d1time = 0x7fff; /* in multiples of 10 seconds */
       startAutoShim();
   }
   else
   {
	stopAutoShim();
   }
}

startStopSsha( int value )
{
	int 	svalue;
/*	int 	dvalue; */

	svalue = value & 0x03; 			/* reduce to on/off/reset/init */
/*	dvalue = value >> 2; */
	if (svalue == 2) {
		stopAutoShim(); /* this is a fix for d1>>10 abort bug */
		resetAutoShim();
	}
	else if (svalue == 1) { 
/*		sshaP->d1time = dvalue; /* in multiples of 10 seconds */
		sshaP->d1time = ( value >> 2 ); /* in multiples of 10 seconds */
		startAutoShim();
	}
	else if (svalue == 3) {
/*		initAutoShimMask( dvalue ); */
		initAutoShimMask( value >> 2 );
	}
	else {
		stopAutoShim();
	}
}


int 
calcTickDiff( int tickBefore, int tickAfter )
{
	int	difference;

/*  Note:  at a rate of 60 per second, the tick counter will
           overflow 32 bit arithmetic after about 6 months...   */

	if (tickBefore > tickAfter)
	  difference = (0x7fffffff - tickBefore) + (tickAfter & 0x7fffffff) + 1;
	else
	  difference = tickAfter - tickBefore;

	return( difference );
}

static int 
checkForStop( int delay )
{
	int	tickBefore, tickAfter, tickDiff;
	int	retval;

	tickBefore = tickGet();
	semTake( sshaP->autoShimSemId, WAIT_FOREVER );
	semGive( sshaP->autoShimSemId );
	tickAfter = tickGet();
	tickDiff = calcTickDiff( tickBefore, tickAfter );

	return( tickDiff >= delay );
}

static
syncWithStartOfScan()
{
	semTake( sshaP->startOfScanId, WAIT_FOREVER );
	semGive( sshaP->startOfScanId );
}

optimizeShimDacCycle( int shimDacIndex )
{
	int	sampleSize = 100;
	int	epsilon = 1;
	int	iter, ival;
	int	lklevel_1, lklevel_2, lklevel_delta, startingDacValue;
	int	sshaStopped;
	int	lock_system_delay = LOCK_SYSTEM_DELAY; /* so it doesn't keep calling sysClkRateGet() */

/*  It is essential to first wait for the start of a scan before reading the
    starting DAC value.

    Shim values initially are all set to 0.  Later, better values are likely
    downloaded from the host computer.

    Waiting until after the start of a scan to read the current DAC value,
    helps insure that the value that is read will be the correct value.

    This autoshim system does not work if some other agent changes a shim
    value that this system works on while it is active.  Eventually it will
    set the shim value back to the value it read previously, or a value
    offset by deltaDacValue.  The changed DAC value will be lost.		*/

	if (sshaP->d1time > 1)
	{
	  sshaP->d1time -= 1;
	}
	else
	{
	  syncWithStartOfScan();
	}
	startingDacValue = getDacValue( shimDacIndexTrue[ shimDacIndex ] );
/*	DPRINT2( -1, "HDWSHIM: starting with DAC %d, value %d\n", shimDacIndexTrue[ shimDacIndex ], startingDacValue ); */
	setDacValue( shimDacIndexTrue[ shimDacIndex ], startingDacValue ); /* first time */
	sshaP->shimIsOffset = 0;

	lklevel_1 = 0;
	for (iter = 0; iter < sampleSize; iter++) {
		lklevel_1 += readLockLevel();
		sshaStopped = checkForStop( lock_system_delay ); /* lock_system_delay = 6 = 100ms */
		if ( !sshaStopped )
		  taskDelay( lock_system_delay ); /* 100ms */
	}
	lklevel_1 /= sampleSize;

	if (sshaP->d1time > 1)
	{
	  sshaP->d1time -= 1;
	}
	else
	{
	  syncWithStartOfScan();
	}
	ival = getDacValue( shimDacIndexTrue[ shimDacIndex ] );	 /* ensure identical series of steps */
	setDacValue( shimDacIndexTrue[ shimDacIndex ], startingDacValue + deltaDacValue ); /* second time */
	sshaP->shimIsOffset = 1;

	lklevel_2 = 0;
	for (iter = 0; iter < sampleSize; iter++) {
		lklevel_2 += readLockLevel();
		sshaStopped = checkForStop( lock_system_delay );
		if ( !sshaStopped )
		  taskDelay( lock_system_delay );
	}
	lklevel_2 /= sampleSize;

	lklevel_delta = lklevel_1 - lklevel_2;
	if (lklevel_delta < 0 )
	  lklevel_delta = 0 - lklevel_delta;

	if (lklevel_delta < epsilon) /* third time, not sync'ed */
	{
	    /* instead of divide lklevel_1,2 by sampleSize, multiply epsilon by sampleSize? */
	  setDacValue( shimDacIndexTrue[ shimDacIndex ], startingDacValue );
	  DPRINT4( 0, "DAC%d lock1:%d lock2:%d val:%d\n",
		shimDacIndexTrue[ shimDacIndex ], lklevel_1, lklevel_2, startingDacValue );
	}
	else if (lklevel_2 > lklevel_1)
	{
	  setDacValue( shimDacIndexTrue[ shimDacIndex ], startingDacValue + deltaDacValue );
	  DPRINT4( 0, "DAC%d lock1:%d lock2:%d val:%d\n",
	  shimDacIndexTrue[ shimDacIndex ], lklevel_1, lklevel_2, startingDacValue + deltaDacValue );
	}
	else
	{
	  setDacValue( shimDacIndexTrue[ shimDacIndex ], startingDacValue - deltaDacValue );
	  DPRINT4( 0, "DAC%d lock1:%d lock2:%d val:%d\n",
	  shimDacIndexTrue[ shimDacIndex ], lklevel_1, lklevel_2, startingDacValue - deltaDacValue );
	}
	sshaP->shimIsOffset = 0;
}

autoShimTask()
{
	int	sshaStopped;
	int	shimDacIndex_2;

	shimDacIndex_2 = -1;
	semTake( sshaP->autoShimSemId, WAIT_FOREVER ); /* semGive given by initAutoShimMask() */

	for (;;) {
		sshaP->shimDacIndex_1 = locateNextShim( shimDacIndex_2 );
		if (sshaP->shimDacIndex_1 < 0)
		  sshaP->shimDacIndex_1 = locateNextShim( -1 );
		if (sshaP->shimDacIndex_1 < 0) {
			DPRINT1( -1, "No shims specified, default Z1 (%d)\n", SSHA_Z1_INDEX );
			sshaP->shimDacIndex_1 = SSHA_Z1_INDEX;
		}
		optimizeShimDacCycle( sshaP->shimDacIndex_1 );
		shimDacIndex_2 = sshaP->shimDacIndex_1;
	}
}

static
markStartOfScan()
{
	semGive( sshaP->startOfScanId );
	taskDelay( 0 );
	semTake( sshaP->startOfScanId, WAIT_FOREVER );
}

initAutoShim()
{
	int	iter, i;

	sshaP = &theSshaParams;
	sshaP->tAutoShimTid = NULL;
	sshaP->sshaStarted = 0;
	sshaP->d1time = 0;
	sshaP->shimIsOffset = 0;
	sshaP->shimDacIndex_1 = 0;
	deltaDacValue = 2;
	sshaP->sshaConfigured = 0;

	for (iter = 0; iter < (MAX_SHIMS_CONFIGURED + BITS_PER_INT - 1) / BITS_PER_INT; iter++)
	  sshaP->autoShimMask[ iter ] = 0;
	sshaP->autoShimMask[ 0 ] = (1 << SSHA_Z1_INDEX); /* default to Z1 */
	for (i=0; i<MAX_SHIMS_FOR_SSHA; i++)
	  shimDacIndexTrue[ i ] = i;
	shimDacIndexTrue[ SSHA_X1_INDEX ] += 10;
	shimDacIndexTrue[ SSHA_Y1_INDEX ] += 10;

	sshaP->autoShimSemId = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	sshaP->startOfScanId = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	sshaP->tAutoShimTid = taskSpawn("tAutoShimTsk", AUTOSHIM_PRIORITY, AUTOSHIM_TASK_OPTIONS,
				  AUTOSHIM_STACK_SIZE, autoShimTask, NULL,ARG2,
				  ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
	if ( sshaP->tAutoShimTid == ERROR) {
		errLogSysRet(LOGIT,debugInfo,
			"initAutoShim: could not spawn tAutoShimTsk:");
	}
}

startAutoShim()
{
	semGive( sshaP->autoShimSemId );
	markStartOfScan();
	sshaP->sshaStarted = 1;
}

stopAutoShim()
{
	if (sshaP->sshaStarted)
	  semTake( sshaP->autoShimSemId, WAIT_FOREVER );
	sshaP->sshaStarted = 0;
}

resetAutoShim()
{
	int 	startingDacValue;

	if (sshaP->shimIsOffset == 1)
	{
	  startingDacValue = getDacValue( shimDacIndexTrue[ sshaP->shimDacIndex_1 ] );
	  setDacValue( shimDacIndexTrue[ sshaP->shimDacIndex_1 ], startingDacValue - deltaDacValue ); 
	  sshaP->shimIsOffset = 0;
	}

	while (semTake( sshaP->startOfScanId, NO_WAIT ) != ERROR);
	while (semTake( sshaP->autoShimSemId, NO_WAIT ) != ERROR);

	switch (shimType)
	{
	  case SERIAL_SHIMS: case OMT_SHIMS: case RRI_SHIMS:
	    getnGiveShimMutex(); /* allow any pending serial Shim commands to finish */
	    break;
	  case QSPI_SHIMS:
	    getnGiveQspiMutex(); /* allow any pending qspi Shim commands to finish */
	    break;
	  default:
	    break;
	}
	taskRestart( sshaP->tAutoShimTid );
	if (sshaP->sshaConfigured == 1)
	  DPRINT(0, "HDWSHIM: stop\n");
	sshaP->sshaConfigured = 0;
}

initAutoShimMask( dvalue )
int dvalue;
{
	DPRINT(0, "HDWSHIM: start\n");
	sshaP->sshaConfigured = 1;
	sshaP->autoShimMask[ 0 ] = dvalue;
	semGive( sshaP->autoShimSemId ); /* initSSHAmethod must occur early in cps.c for this to work */
}
