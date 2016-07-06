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

#include <vxWorks.h>

#include "hostAcqStructs.h"
#include "consoleStat.h"
#include "acodes.h"
#include "acqcmds.h"
#include "lkapio.h"

/*  These defines were taken from A_interp.c and eventually
    should be put in an APBUS header file.			*/

#define  AP_LOCKGAIN_ADDR	0x0b53
#define  AP_LOCKPHASE_ADDR	0x0b50
#define  AP_LOCKPOWER_ADDR	0x0b52
#define  AP_LOCKCNTLR_ADDR	0x0b57
#define  AP_LKPREAMPGAIN_ADDR	0x0b4a
#define  AP_LKFREQBASE_ADDR	0x0b54
#define  NO_REGISTER		0xffff
#define  NO_VALUE		0xffff


/*  Set the bit to turn *off* the hardware lock */
#define  HOMOSPOIL_ON	1
#define  LKHOLD		12
#define  LKSAMPLE	13

#define  AP_TIMECONST_MASK	0x30
#define  AP_HOMOSPOIL_MASK	0x02
#define  AP_SAMPLE_HOLD_MASK	0x04
#define  AP_LOCKOFF_MASK	0x08

#define  LSDV_LKMODE		0x02


extern STATUS_BLOCK	currentStatBlock;

/*----------------------------------------------------------------------*/
/*  low-level lock programs						*/
/*----------------------------------------------------------------------*/

static int
fixLkPhase( int lockphase )
{
	int	retval;

	if (lockphase < 0 || lockphase >= 360)
	  lockphase %= 360;

	retval = ((lockphase << 8) / 360) & 0xff;

	return( retval );
}


/*  Adopted from a very similar table in lkapio.s, SCCS category xracq	*/

static const u_char lin_db[] = {
	0,			/*  0dB */
	1,			/*  1 dB */
	1,			/*  2 dB */
	1,			/*  3 dB */
	2,			/*  4 dB */
	2,			/*  5 dB */
	2,			/*  6 dB */
	2,			/*  7 dB */
	3,			/*  8 dB */
	3,			/*  9 dB */
	3,			/* 10 dB */
	4,			/* 11 dB */
	4,			/* 12 dB */
	4,			/* 13 dB */
	5,			/* 14 dB */
	6,			/* 15 dB */
	6,			/* 16 dB */
	7,			/* 17 dB */
	8,			/* 18 dB */
	9,			/* 19 dB */
	10,			/* 20 dB */
	11,			/* 21 dB */
	13,			/* 22 dB */
	14,			/* 23 dB */
	16,			/* 24 dB */
	18,			/* 25 dB */
	20,			/* 26 dB */
	23,			/* 27 dB */
	25,			/* 28 dB */
	28,			/* 29 dB */
	32,			/* 30 dB */
	36,			/* 31 dB */
	40,			/* 32 dB */
	45,			/* 33 dB */
	51,			/* 34 dB */
	57,			/* 35 dB */
	64,			/* 36 dB */
	72,			/* 37 dB */
	80,			/* 38 dB */
	90,			/* 39 dB */
	101,			/* 40 dB */
	113,			/* 41 dB */
	128,			/* 42 dB */
	143,			/* 43 dB */
	161,			/* 44 dB */
	180,			/* 45 dB */
	202,			/* 46 dB */
	227,			/* 47 dB */
	255,			/* 48 dB */
};

int
cvtLkDbLinear( int dbval )
{
	int	tblsize;

	if (dbval < 0)
	  dbval = 0;
	else {
		tblsize = sizeof( lin_db ) / sizeof( lin_db[ 0 ] );
		if (dbval >= tblsize)
		  dbval = tblsize - 1;
	}

	return( (int) lin_db[ dbval ] );
}


/*  The lock system has a time constant which describes how fast it responds to
    changes in the magnetic field.  Four possible time constant values are available
    from a fast value of 1.2 seconds to a slow value of 48 seconds, with intermediate
    values of 4.8 and 12 sec too.  Normally it is set to respond quickly except
    during non-interactive acquisitions where it is set to respond slowly.

    The system works with these time constants in three different ways, as patterns,
    as values and as types.  The pattern is stored internally in this file and is not
    expected to be accessed outside this file.  The value represents the value that
    the time constants parameters may have in VNMR.  Values are used to reference or
    set a particular time constant.  Finally the type represents the different
    parameters in VNMR themselves.  At this time we have two, locktc and lockacqtc.
    Recall the time constant is to be set to the long value during non-interactive
    acquisitions and to the short value at other times, including ACQI and quiescent
    times.  The two parameters instantiate this difference.

    On the NDC the time constant is set using APBUS register 0x0b57.  Bits 4 and 5
    control the time constant (the bits are represented as 0x10 and 0x20).  Because
    the AP register controls other things of interest, it is necessary to remember
    the value stored there between operations.

    You can set a value based on type and obtain a value for AP register 0x0b57 based
    on the type.									*/


static const struct {
	int	tcvalue;
	ushort	tcpattern;
} tcvaluepattern[] = {
	{ 1,	 0x00 },	/* short time constant */
	{ 2,	 0x10 },
	{ 3,	 0x20 },
	{ 4,	 0x30 },	/* long time constant */
};


static ushort
tcvalue2pattern( tcvalue )
{
	int	iter, tblsize;

	tblsize = sizeof( tcvaluepattern ) / sizeof( tcvaluepattern[ 0 ] );
	for (iter = 0; iter < tblsize; iter++) {
		if (tcvalue == tcvaluepattern[ iter ].tcvalue)
		  return( tcvaluepattern[ iter ].tcpattern );
	}

	return( NO_VALUE );
}


#define  NO_INDEX	-1
#define  AP_LOCKCNTLR_INDEX	0
#define  AP_LKPREAMPGAIN_INDEX	1

static struct {
	int		index;
	const u_short	apreg;
	u_short		apvalue;
} lkapregtable[] = {
	{ AP_LOCKCNTLR_INDEX,	AP_LOCKCNTLR_ADDR,	0 },
	{ AP_LKPREAMPGAIN_INDEX, AP_LKPREAMPGAIN_ADDR,	0 },
	{ NO_INDEX,		NO_REGISTER,		-1 },
};

/*  Note:  The above table is initialized with the expected
           initial value for each register.  At this time we
           only store a value for the lock controller.		*/
 
/*  Note:  Some of the routines are really returning unsigned shorts,
           but are declared as returning ints, so as to keep the C
           compiler happy when they are placed into the lkparams
           table, below.  Some of the arguments too are really
           unsigned shorts but are declared as ints for the same reason.
           Some routines do not use their arguments; these have the
           argument named dummy.  All routines put into the lkparams
           table must be declared with one argument to keep the C
           compiler happy.						*/

static int
apreg2apvalue( int apreg )
{
        int     iter;

	for (iter = 0; lkapregtable[ iter ].index != NO_INDEX; iter++)
	  if ((ushort) apreg == lkapregtable[ iter ].apreg)
	    return( (int) lkapregtable[ iter ].apvalue );

	return( NO_VALUE );
}

static int
getLkCntlrFromMode( int lkmode )
{
	ushort	apvalue;

	apvalue = lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue;
	if (lkmode == LKOFF)
	  apvalue |= AP_LOCKOFF_MASK;
	else if (lkmode == LKON)
	  apvalue &= (~AP_LOCKOFF_MASK);
	else if (lkmode == LKHOLD)
	  apvalue |= AP_SAMPLE_HOLD_MASK;
	else if (lkmode == LKSAMPLE)
	  apvalue &= (~AP_SAMPLE_HOLD_MASK);

	return( (int) apvalue );
}

static int
getLkCntlrHomospoil( int homospoil )
{
	ushort	apvalue;

	apvalue = lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue;
	if (homospoil == HOMOSPOIL_ON)
	  apvalue |= AP_HOMOSPOIL_MASK;
	else
	  apvalue &= (~AP_HOMOSPOIL_MASK);

	return( (int) apvalue );
}

#define  TIMECONST	(0)
#define  TIMECONSTACQ	(TIMECONST+1)
#define  TIMECONSTINT	(TIMECONSTACQ+1)

#define  DEFAULT_TC_VALUE	1
#define  DEFAULT_TCACQ_VALUE	4

static struct {
	const int	tctype;
	int		tcvalue;
} tctypevalue[] = {
	{ TIMECONST,	DEFAULT_TC_VALUE },
	{ TIMECONSTACQ,	DEFAULT_TCACQ_VALUE },
	{ TIMECONSTINT,	DEFAULT_TC_VALUE },
};

/*  settcvalue does NOT go in the lkparams table.  */

static int
settcvalue( int tctype, int tcvalue )
{
	int	iter, retval, tblsize;

	retval = -1;
	tblsize = sizeof( tctypevalue ) / sizeof( tctypevalue[ 0 ] );
	for (iter = 0; iter < tblsize; iter++)
	  if (tctype == tctypevalue[ iter ].tctype) {
		tctypevalue[ iter ].tcvalue = tcvalue;
		retval = 0;
	  }

	return( retval );
}

static int
getLkCntlrFromConst( int tctype )
{
	ushort	apvalue, tcpattern;
	int	iter, tblsize, tcvalue;

	tcvalue = NO_VALUE;
	tblsize = sizeof( tctypevalue ) / sizeof( tctypevalue[ 0 ] );
	for (iter = 0; iter < tblsize; iter++) {
		if (tctype == tctypevalue[ iter ].tctype)
		  tcvalue = tctypevalue[ iter ].tcvalue;
	}
	if (tcvalue == NO_VALUE)
	  return( NO_VALUE );

	apvalue = lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue;
	tcpattern = tcvalue2pattern( tcvalue );
	if (tcpattern == NO_VALUE)
	  return( NO_VALUE );

	apvalue &= (~AP_TIMECONST_MASK);
	apvalue |= tcpattern;

	return( apvalue );
}

static int
getLkCntlrFromTC( int dummy )
{
	return( getLkCntlrFromConst( TIMECONST ) );
}

static int
getLkCntlrFromAcqTC( int dummy )
{
	return( getLkCntlrFromConst( TIMECONSTACQ ) );
}

static int
getIntLkCntlrFromTC( int dummy )
{
        tctypevalue[ TIMECONSTINT ].tcvalue = tctypevalue[ TIMECONSTACQ ].tcvalue;
        tctypevalue[ TIMECONSTACQ ].tcvalue = tctypevalue[ TIMECONST ].tcvalue;
	return( getLkCntlrFromConst( TIMECONST ) );
}

static int
getIntLkCntlrFromAcqTC( int dummy )
{
        tctypevalue[ TIMECONSTACQ ].tcvalue = tctypevalue[ TIMECONSTINT ].tcvalue;
	return( getLkCntlrFromConst( TIMECONSTACQ ) );
}


int
getLockFreqAP( int dummy )
{
   return( (int) currentStatBlock.stb.AcqLockFreqAP );
}


static int
storeLkPhase( int lockphase )
{
	currentStatBlock.stb.AcqLockPhase = (short) lockphase;
	return( lockphase );
}

static int
storeLkGain( int lockgain )
{
	currentStatBlock.stb.AcqLockGain = (short) lockgain;
	return( lockgain );
}

static int
storeLkPower( int lockpower )
{
	currentStatBlock.stb.AcqLockPower = (short) lockpower;
	return( lockpower );
}

static int
storeLkCntlrFromMode( int lkmode )
{
	u_short	apvalue;

	apvalue = lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue;

	if (lkmode == LKOFF)
        {
	   apvalue |= AP_LOCKOFF_MASK;
	   clearLSDVbits( LSDV_LKMODE );
	}
	else if (lkmode == LKON)
        {
	   apvalue &= (~AP_LOCKOFF_MASK);
	   setLSDVbits( LSDV_LKMODE );
	}
	else if (lkmode == LKHOLD)
        {
	   apvalue |= AP_SAMPLE_HOLD_MASK;
        }
	else if (lkmode == LKSAMPLE)
        {
	   apvalue &= (~AP_SAMPLE_HOLD_MASK);
        }

	lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue = apvalue;
}

static int
storeLkCntlrHomospoil( int homospoil )
{
	u_short	apvalue;

	apvalue = lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue;

	if (homospoil == HOMOSPOIL_ON) {
		apvalue |= AP_HOMOSPOIL_MASK;
		/* clearLSDVbits( LSDV_LKMODE ); */
	}
	else {
		apvalue &= (~AP_HOMOSPOIL_MASK);
		/* setLSDVbits( LSDV_LKMODE ); */
	}

	lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue = apvalue;
}

static int
storeLkCntlrFromConst( int tctype )
{
	int retval;

	retval = getLkCntlrFromConst( tctype );
	lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue = retval;
	return( retval );
}

static int
storeLkTimeC( int tcvalue )
{
	settcvalue( TIMECONST, tcvalue );
}

static int
storeLkAcqTimeC( int tcvalue )
{
	settcvalue( TIMECONSTACQ, tcvalue );
}

/*  Next two program ignore their argument because their operation is
    dervived solely from the name of the program, which includes the
    type of lock time constant, which determines the value to be stored.  */

static int
storeLkCntlrTC( int dummy )
{
	int retval;

	retval = getLkCntlrFromConst( TIMECONST );
	lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue = retval;
	return( retval );
}

static int
storeLkCntlrAcqTC( int dummy )
{
	int retval;

	retval = getLkCntlrFromConst( TIMECONSTACQ );
	lkapregtable[ AP_LOCKCNTLR_INDEX ].apvalue = retval;
	return( retval );
}

static int
storeLockFreqAP( int lkfreq_apword )
{
	currentStatBlock.stb.AcqLockFreqAP = (long) lkfreq_apword;
}


/*  The tables are to be accessed by using either an A-code or
    an X-code (called Change-Codes on the UnityPLUS system).	*/


#define  NO_INDEX	-1
#define  LKPOWER_INDEX	0
#define  LKGAIN_INDEX	(LKPOWER_INDEX+1)
#define  LKPHASE_INDEX	(LKGAIN_INDEX+1)
#define  LKMODE_INDEX	(LKPHASE_INDEX+1)
#define  LKTC_INDEX	(LKMODE_INDEX+1)
#define  LKACQTC_INDEX	(LKTC_INDEX+1)
#define  LKSETTC_INDEX	(LKACQTC_INDEX+1)
#define  LKSETACQTC_INDEX (LKSETTC_INDEX+1)

#define  INTLKTC_INDEX (LKSETACQTC_INDEX+1)
#define  INTLKACQTC_INDEX (INTLKTC_INDEX+1)

#define  LKSETAPREG_INDEX (INTLKACQTC_INDEX+1)
#define  HOMOSPOIL_INDEX (LKSETAPREG_INDEX+1)
#define  LKFREQ_INDEX	(HOMOSPOIL_INDEX+1)
#define  MAXLK_INDEX	(LKFREQ_INDEX)

static struct {
	int	  index;
	ushort	  apreg;
	int  	(*converter)();
	int	(*storevalue)();
} lkparams[] = {
	{ LKPOWER_INDEX,	AP_LOCKPOWER_ADDR,	cvtLkDbLinear,	storeLkPower },
	{ LKGAIN_INDEX,		AP_LOCKGAIN_ADDR,	cvtLkDbLinear,	storeLkGain },
	{ LKPHASE_INDEX,	AP_LOCKPHASE_ADDR,	fixLkPhase,	storeLkPhase },
	{ LKMODE_INDEX,		AP_LOCKCNTLR_ADDR,
					getLkCntlrFromMode,	storeLkCntlrFromMode },
	{ LKTC_INDEX,		AP_LOCKCNTLR_ADDR,
					getLkCntlrFromTC,	storeLkCntlrTC },
	{ LKACQTC_INDEX,	AP_LOCKCNTLR_ADDR,
					getLkCntlrFromAcqTC,	storeLkCntlrAcqTC },
	{ LKSETTC_INDEX,	AP_LOCKCNTLR_ADDR,	NULL,		storeLkTimeC },
	{ LKSETACQTC_INDEX,	AP_LOCKCNTLR_ADDR,	NULL,		storeLkAcqTimeC },
	{ INTLKTC_INDEX,	AP_LOCKCNTLR_ADDR,
					getIntLkCntlrFromTC,		storeLkCntlrTC },
	{ INTLKACQTC_INDEX,	AP_LOCKCNTLR_ADDR,
					getIntLkCntlrFromAcqTC,		storeLkCntlrAcqTC },
	{ LKSETAPREG_INDEX,	NO_REGISTER,		apreg2apvalue,	NULL },
	{ HOMOSPOIL_INDEX,	AP_LOCKCNTLR_ADDR,	getLkCntlrHomospoil,	 storeLkCntlrHomospoil},
	{ LKFREQ_INDEX,		AP_LKFREQBASE_ADDR,	NULL,		storeLockFreqAP },
	{ NO_INDEX,		NO_REGISTER,		NULL,		NULL },
};

static struct {
	int	index;
	int	lkAcode;
} lkAcodeTable[] = {
	{ LKPOWER_INDEX,	LOCKPOWER_I },
	{ LKPOWER_INDEX,	LOCKPOWER_R },
	{ LKGAIN_INDEX,		LOCKGAIN_I },
	{ LKGAIN_INDEX,		LOCKGAIN_R },
	{ LKPHASE_INDEX,	LOCKPHASE_I },
	{ LKPHASE_INDEX,	LOCKPHASE_R },
	{ LKSETTC_INDEX,	LOCKSETTC },
	{ LKSETACQTC_INDEX,	LOCKSETACQTC },
	{ LKTC_INDEX,		LOCKTC },
	{ LKACQTC_INDEX,	LOCKACQTC },
	{ LKSETAPREG_INDEX,	LOCKSETAPREG },
	{ LKMODE_INDEX,		LOCKMODE_I },
	{ HOMOSPOIL_INDEX,	HOMOSPOIL },
	{ LKFREQ_INDEX,		LOCKFREQ_I },
	{ NO_INDEX,		-1 },
};

static struct {
	int	index;
	int	lkXcode;
} lkXcodeTable[] = {
	{ LKPOWER_INDEX,	LKPOWER },
	{ LKGAIN_INDEX,		LKGAIN },
	{ LKPHASE_INDEX,	LKPHASE },
	{ LKMODE_INDEX,		LKMODE },
	{ INTLKTC_INDEX,	LKTC },
	{ INTLKACQTC_INDEX,	LKACQTC },
	{ LKFREQ_INDEX,		LKFREQ },
	{ NO_INDEX,		-1 },
};

int
acode2index( int acode )
{
	int	iter;

	for (iter = 0; lkAcodeTable[ iter ].index != NO_INDEX; iter++) {
		if ( lkAcodeTable[ iter ].lkAcode == acode)
		  return( lkAcodeTable[ iter ].index );
	}

	return( NO_INDEX );
}

int
xcode2index( int xcode )
{
	int	iter;

	for (iter = 0; lkXcodeTable[ iter ].index != NO_INDEX; iter++) {
		if ( lkXcodeTable[ iter ].lkXcode == xcode)
		  return( lkXcodeTable[ iter ].index );
	}

	return( NO_INDEX );
}

ushort
index2apreg( int index )
{
	if (index < 0 || index > MAXLK_INDEX)
	  return( NO_REGISTER );
	else
	  return( lkparams[ index ].apreg );
}

/*  Next two functions each return a function which itself returns an integer.
    The mysteries of C syntax place the actual arguments to the function in 
    the innermost set of parentheses, right next to the symbol name.  If the
    function to be returned itself had an argument list, those arguments
    would be specified within the rightmost set of parentheses.			*/

int
(*index2converter( int index ))()
{
	if (index < 0 || index > MAXLK_INDEX)
	  return( NULL );
	else
	  return( lkparams[ index ].converter );
}

int
(*index2valuestore( int index ))()
{
	if (index < 0 || index > MAXLK_INDEX)
	  return( NULL );
	else
	  return( lkparams[ index ].storevalue );
}

ushort
getlkpreampgainvalue(void)
{
	return( lkapregtable[ AP_LKPREAMPGAIN_INDEX ].apvalue );
}

ushort
getlkpreampgainapaddr(void)
{
	return( lkapregtable[ AP_LKPREAMPGAIN_INDEX ].apreg );
}

void
storelkpreampgainvalue(ushort value)
{
	lkapregtable[ AP_LKPREAMPGAIN_INDEX ].apvalue = value;
}
