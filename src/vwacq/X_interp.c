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
#include <string.h>
#include <vxWorks.h>
#include <rngLib.h>
#include <semLib.h>
#include <msgQLib.h>

#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "acqcmds.h"
#include "autoObj.h"
#include "lkapio.h"
#include "acodes.h"		/* SHIMAUTO */
#include "X_interp.h"
#include "serialShims.h"
#include "vtfuncs.h"
#include "instrWvDefines.h"
#include "mboxcmds.h"

#ifndef NOCMD
#define  NOCMD	-1
#endif

#ifndef MAXNUMARGS
#define  MAXNUMARGS	30
#endif

#ifndef MAXSHIMS
#define  MAXSHIMS	48
#endif

/*  Next series of defines were taken from autoshim.c  */

#define DONE_SH     0
#define LOCK_SH     1
#define FID_SH      2
#define SEL_COIL    3

#define SHIMMASK_SIZE		4

#define SHIMI_PARAM_COUNT	4
#define SHIMI_METHOD_SIZE	20


extern int	host_abort;		/* autoshim.c */
extern MSG_Q_ID pMsgesToAupdt;
extern MSG_Q_ID pMsgesToXParser;
extern MSG_Q_ID pTagFifoMsgQ;

extern STATUS_BLOCK currentStatBlock;
extern AUTO_ID   pTheAutoObject;		/* Automation Object */
extern RING_ID   pSyncActionArgs;	/* rngbuf to request autoshim */


extern int	cvtLkDbLinear();
extern int	fixLkPhase();
extern int	calcRecvGainVals( int gain, ushort *pPreampAttn, ushort *pRset );
extern unsigned short    *settunefreq(unsigned short *intrpp);

extern int sampleHasChanged;	/* Global Sample change flag */

static short	LockPwrMax, LockGainMax;

extern gpaHandler();

static struct {
   int    changecode;
   int    argcount;
   int  (*handler)();
   char  *text;
} Xcmd[] = {
	{ LKPOWER,	1,	ladcHandler,   "Set lock power" },
	{ LKGAIN,	1,	ladcHandler,   "Set lock gain"  },
	{ LKPHASE,	1,	ladcHandler,   "Set lock phase"  },
	{ SHIMDAC,	2,	shimHandler,   "Set shim DAC (alias 1)"  },
	{ SET_DAC,	2,	shimHandler,   "Set shim DAC (alias 2)"  },
	{ LKMODE,	1,	ladcHandler,   "Lock Mode, on, off or auto" },
	{ LKTC,		1,	ladcHandler,   "Set lock time constant for not acquiring" },
	{ LKACQTC,	1,	ladcHandler,   "Set lock time constant for acquiring" },
	{ BEAROFF,	0,	spnrHandler,   "spinner Bearing air off" },
	{ BEARON,	0,	spnrHandler,   "spinner Bearing air on" },
	{ SETRATE,	1,	spnrHandler,   "set spinner air rate" },
	{ SETSPD,	1,	spnrHandler,   "set spinner speed" },
	{ EJECT,	0,	spnrHandler,   "eject sample" },
	{ EJECTOFF,	0,	spnrHandler,   "turn off eject" },
	{ INSERT,	0,	spnrHandler,   "insert sample" },
	{ GETSTATUS,	0,	statHandler,   "return console status block" },
	{ RTN_SHLK,	0,	statHandler,   "return console status block" },
	{ SYNC_FREE,	0,	syncHandler,   "Restart the acode parser" },
	{ WSRAM,	1,	nvRamHandler,  "write NvRam offset by 0x500"},
	{ SHIMI,	6,	autshmHandler, "start up autoshim" },
	{ STOP_SHIMI,	0,	autshmHandler, "start up autoshim" },
	{ RCVRGAIN,	1,	rcvrgainHandler, "set receiver gain" },
	{ FIX_ACODE,	2,	fixAcodeHandler, "modify A-codes" },
	{ SET_ATTN,	2,	setAttnHandler, "for acqi IPA RF attenuation" },
        { SET_TUNE,    14,      tuneHandler,   "for tune command" },
        { RESET_VT,     0,      spnrHandler,   "Reset VT controller" },
        { SETLOC,       1,      spnrHandler,   "Set Sample Number" },
        { SETMASTHRES,  1,      spnrHandler,   "Set MAS Switchover Threshold (Hz)" },
        { BUMPSAMPLE,   0,      spnrHandler,   "bump the sample" },
        { SETTEMP,      2,      setvtHandler,  "Set temperature in the VT controller" },
        { SHIMSET,      1,      shmsetHandler, "Specify type of shims" },
        { SETSTATUS,    1,      statHandler,   "Set console acquisition status" },
        { LKFREQ,       2,      lkfreqHandler, "Specify lock frequency" },
	{ STATRATE,	1,	statTimer,     "set statblock update rate" },
	{ GTUNE_PX,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IX,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_PY,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IY,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_PZ,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IZ,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GPAENABLE,	1,	gpaHandler,    "set GPA enable/disable (1/0)" },
	{ NOCMD,	-1,	NULL,		NULL },
};


#define MAX_FIX_ACODES	40

static short fixAcodeVector[ MAX_FIX_ACODES ];
static int fixAcodeIndex;
static char fixAcodeMsge[ MAX_FIX_ACODES*5 ];


/*  These #defines are present in connection with calcLockLevel.  */

#define  CONSTANT_1	(1)
#define  CONSTANT_2	(30000)
#define  CONSTANT_3	(100)

#define  Z1		(2)
#define  Z2		(4)
#define  Z3		(6)
#define  Z4		(7)

#define  BEST_Z1	(200)
#define  BEST_Z2	(-100)
#define  BEST_Z3	(-300)
#define  BEST_Z4	(100)

/*  Fake shimming  */

static int
calcLockLevel()
{
	short	*shimarray, sval;
	int	 iter, ival;

	shimarray = &currentStatBlock.stb.AcqShimValues[ 0 ];
	ival =  CONSTANT_1;

	sval = shimarray[ Z1 ] - BEST_Z1;
	ival += (sval * sval) / CONSTANT_3;

	sval = shimarray[ Z2 ] - BEST_Z2;
	ival += (sval * sval) / CONSTANT_3;

	sval = shimarray[ Z3 ] - BEST_Z3;
	ival += (sval * sval) / CONSTANT_3;

	sval = shimarray[ Z4 ] - BEST_Z4;
	ival += (sval * sval) / CONSTANT_3;

	ival = CONSTANT_2 / ival;

	return( ival );
}

/*********************************************************
 testing stubs
*********************************************************/

/*********************************************************
 end of testing stubs
*********************************************************/

/*********************************************************
 establish type of shims present on this system
 AS SPECIFIED BY THE HOST COMPUTER -  OVERRIDES
 THE SHIM TYPE DETERMINED LOCALLY
*********************************************************/

int
establishShimType(int shimset)
{
    int	qspiShims, serialShims;

    DPRINT1(0,"establishShimType: shimset %d\n",shimset);
    switch (shimset) 
    {
	  case 1:
	  case 10:
                if (shimType != QSPI_SHIMS)
                   return(0);
	        shimType = QSPI_SHIMS;
		qspi_dac_addr = shimGetQspiTab();
		/* qspi_dac_addr = &qspi_dac_table[ 0 ]; */
		break;

	  case 2:
	  case 3:
	  case 4:
	  case 6:
	  case 7:
	  case 9:

                if (shimType != MSR_SERIAL_SHIMS)
                   return(0);
		shimType = MSR_SERIAL_SHIMS;
		break;

	  case 5:
	  case 16:

                if (shimType != MSR_RRI_SHIMS)
                   return(0);
		shimType = MSR_RRI_SHIMS;

		break;

	  case 8:
                if (shimType != QSPI_SHIMS)
                   return(0);
	        shimType = QSPI_SHIMS;
		qspi_dac_addr = shimGetImgQspiTab();
		/* qspi_dac_addr = &imgqsp_dac_table[ 0 ]; */
		break;

	  case 11:
		/* shimType is already set */
		break;

	  case 19:
                /* MSR board interprets them as Serial shims */
		shimType = SPI_THIN_SHIMS;
		break;

	  default:
		shimType = NO_SHIMS;
		break;
	}
    DPRINT1(0,"establishShimType: shimType %d\n",shimType);
    currentStatBlock.stb.AcqShimSet = (short) shimset;
    autoSetThinShims( pTheAutoObject, (shimType == SPI_THIN_SHIMS) ? 1 : 0 );
    return( 0 );
}


/********************************************************/

static int
doOneXcommand( char *startarg, char **finalarg )
{
	int	 cmd, cmdindex, iter, numparams, numparsed, paramindex;
	char	*currentarg, *paramptr;
	short	 paramvec[ MAXNUMARGS ];
	int	(*handlerprog)();

	currentarg = startarg;
	cmd = atoi( startarg );
	numparsed = 1;

	for (iter = 0; Xcmd[ iter ].changecode != NOCMD; iter++)
	  if (Xcmd[ iter ].changecode == cmd) {
		cmdindex = iter;
		numparams = Xcmd[ iter ].argcount;
		handlerprog = Xcmd[ iter ].handler;
		break;
	  }

	if (Xcmd[ iter ].changecode == NOCMD) {
		printf( "the %d command is not supported (yet)\n", cmd );
		return( -1 );
	}

	if (Xcmd[ iter ].changecode == WSRAM) {
		paramptr = strtok_r( currentarg, ",", &currentarg );
		if (paramptr == NULL) {
			errLogRet( LOGIT, debugInfo, 
	   "X interpreter ran out of string before it ran out of args in the WSRAM command\n"
			);
			return( -1 );
		}
		numparams = (short) atoi( currentarg ) * 2;
		numparsed++;
	}

	paramvec[ 0 ] = cmd;
	paramptr = strtok_r( currentarg, ",", &currentarg );
	for (paramindex = 0; paramindex < numparams; paramindex++) {
		if (paramptr == NULL) {
			errLogRet( LOGIT, debugInfo, 
	   "X interpreter ran out of string before it ran out of args in the %d command\n",
				   cmd );
			return( -1 );
		}
		paramvec[ paramindex+1 ] = (short) atoi( currentarg );
		paramptr = strtok_r( currentarg, ",", &currentarg );
		numparsed++;
	}

	(*handlerprog)( &paramvec[ 0 ], numparams+1 );
	*finalarg = currentarg;
	return( numparsed );
}

/*********************************************************
 begin X interp
 facility to let the programmer set anything up that needs
 to be set up before parsing an X interpreter message
*********************************************************/
static void
beginX_interp()
{
	fixAcodeIndex = 0;
	fixAcodeMsge[ 0 ] = '\0';
}

static char *
insertstr( char *base, char *prefix )
{
	int	 baselen, iter, preflen;
	char	*tptrstart, *tptrend;

	baselen = strlen( base );
	preflen = strlen( prefix );

/*  tptrstart is the address of the '\0' character at end of the base string
    tptrend is the address where this '\0' character is to go as part of the
    operation of inserting the prefix.
    baselen+1 characters are transferred, so as to get that '\0'.		*/

        tptrstart = base + baselen;
        tptrend = base + baselen + preflen;
        for (iter = 0; iter < baselen+1; iter++)
          *(tptrend--) = *(tptrstart--);

        strncpy( base, prefix, preflen );
 	return( base );
}

static int
fixAcodeGroup( int index, int length )
{
	int	lenMsge, lenq;
	char	qbuf[ 30 ];

	lenMsge = strlen( &fixAcodeMsge[ 0 ] );
	if (length == 1) {
		sprintf( &qbuf[ 0 ], "%d,%d,1,%d;",
			  FIX_ACODE, fixAcodeVector[ index ], fixAcodeVector[ index+1 ] );
		lenq = strlen( &qbuf[ 0 ] );
		if (lenq + lenMsge + 1 > sizeof( fixAcodeMsge )) {
			errLogRet( LOGIT, debugInfo,
			   "too many acodes to fix in fix acode group at point 1\n" );
			return( -1 );
		}
		strcat( &fixAcodeMsge[ lenMsge ], &qbuf[ 0 ] );
	}
	else {
		int	iter;

/*  When checking for overflow of the fix A-code message buffer, add 2 to the sum of
    the string lengths, since at the end we concatenate a semicolon to the string.	*/

		sprintf( &qbuf[ 0 ], "%d,%d,%d",
			  FIX_ACODES, fixAcodeVector[ index ], length );
		lenq = strlen( &qbuf[ 0 ] );
		if (lenq + lenMsge + 2 > sizeof( fixAcodeMsge )) {
			errLogRet( LOGIT, debugInfo,
			   "too many acodes to fix in fix acode group at point 2\n" );
			return( -1 );
		}
		strcat( &fixAcodeMsge[ lenMsge ], &qbuf[ 0 ] );
		for (iter = 0; iter < length; iter++) {
			sprintf( &qbuf[ 0 ], ",%d", fixAcodeVector[ index + iter * 2 + 1 ] );
			lenq = strlen( &qbuf[ 0 ] );
			if (lenq + lenMsge + 2 > sizeof( fixAcodeMsge )) {
				errLogRet( LOGIT, debugInfo,
			   "too many acodes to fix in fix acode group at point 3\n" );
				return( -1 );
			}
			strcat( &fixAcodeMsge[ lenMsge ], &qbuf[ 0 ] );
		}
		strcat( &fixAcodeMsge[ lenMsge ], ";" );
	}

	return( 0 );
}

effectAcodeFix( int groupCount )
{
	char	countPrefix[ 8 ];

	if (groupCount < 1)
	  return( 0 );
	sprintf( &countPrefix[ 0 ], "%d;", groupCount );
	if (strlen( &countPrefix[ 0 ] ) + strlen( &fixAcodeMsge[ 0 ] ) + 1 > 
		     sizeof( fixAcodeMsge )) {
		errLogRet( LOGIT, debugInfo,
	   "too many acodes to fix in effect A-code fix\n"
		);
		return( -1 );
	}
	insertstr( &fixAcodeMsge[ 0 ], &countPrefix[ 0 ] );
	msgQSend(pMsgesToAupdt, &fixAcodeMsge[ 0 ], strlen( &fixAcodeMsge[ 0 ] ),
				 NO_WAIT, MSG_PRI_NORMAL);
	return( 0 );
}

/*********************************************************
 complete X interp
 facility to let the programmer do anything that needs to
 be done after parsing an X interpreter message
*********************************************************/
static void
completeX_interp()
{
	int	iter, ival;
	int	groupCount, groupLength, groupStartIndex, offset, storedOffset;

	groupCount = 0;
	groupLength = 0;
	groupStartIndex = -1;
	storedOffset = -2;

	for (iter = 0; iter < fixAcodeIndex; iter += 2) {
		offset = fixAcodeVector[ iter ];
		if (offset == storedOffset + 1)
		  groupLength++;
		else {
			if (groupLength > 0) {
				ival = fixAcodeGroup( groupStartIndex, groupLength );
				if (ival < 0)
				  return;
				groupCount++;
			}
			groupLength = 1;
			groupStartIndex = iter;
		}
		storedOffset = offset;
	}

	if (groupLength > 0) {
		ival = fixAcodeGroup( groupStartIndex, groupLength );
		if (ival < 0)
		  return;
		groupCount++;
	}

	if (groupCount > 0)
	  effectAcodeFix( groupCount );
}


/*********************************************************
 X_interp
*********************************************************/
void X_interp(char *cmdstring)
{
   char *cmdptr,*argptr;
   int i,iter,retrys,cmd,num,status,parse_count, total_count;
   int cmdargs[8];

   DPRINT1(1,"cmdstring=%s", cmdstring);/*CMP*/
   /* Parse Command -  		*/
   /* "argptr" will point to the character after the token and should	*/
   /* be used as the starting point in the next strtok_r call		*/
   cmdptr=strtok_r(cmdstring,",",&argptr);
   if (cmdptr == NULL)
   {
	errLogRet( LOGIT, debugInfo, "Command not parsed.\n" );
	DPRINT1(0,"X_interp:  Command not parsed - cmdstring: %s\n",cmdstring );
	return;
   }
   if (argptr == NULL)
   {
	errLogRet( LOGIT, debugInfo, "No command for X_interp.\n" );
	DPRINT1(1,"X_interp:  No command for X_interp - cmdstring: %s\n",cmdstring );
	return;
   }

   total_count = atoi(cmdptr);
   status = 0;

   for (;;)
   {
      parse_count = doOneXcommand( argptr, &argptr );
      if (parse_count == -1 || argptr == NULL)
       break;
      total_count = total_count - parse_count;
      if (total_count < 1)
       break;
   }
}


void
Xparser()
{
   char	xmsge[ CONSOLE_MSGE_SIZE ];
   int	ival;

   FOREVER {
	ival = msgQReceive(
	   pMsgesToXParser, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, WAIT_FOREVER);
		DPRINT1( 2, "Xparse:  msgQReceive returned %d\n", ival );
	if (ival == ERROR)
	{
	   printf("X PARSER Q ERROR\n");
	   return;
	}
	else
	{
	   beginX_interp();
	   X_interp(xmsge);
	   completeX_interp();
	}
   }
}

startXParser(int priority, int taskoptions, int stacksize)
{
   if (pMsgesToXParser == NULL)
     pMsgesToXParser = msgQCreate(300,100,MSG_Q_PRIORITY);
   if (pMsgesToXParser == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,
     	  "could not create X Parser MsgQ, ");
      return;
   }
   
   if (taskNameToId("tXParser") == ERROR)
    taskSpawn("tXParser",priority,0,stacksize,Xparser,pMsgesToXParser,
						2,3,4,5,6,7,8,9,10);
}

killXParser()
{
   int tid;
   if ((tid = taskNameToId("tXParser")) != ERROR)
      taskDelete(tid);
}

cleanXParser()
{
  int tid;
  if (pMsgesToXParser != NULL)
  {
    msgQDelete(pMsgesToXParser);
    pMsgesToXParser = NULL;
  }
  tid = taskNameToId("tXParser");
  if (tid != ERROR)
    taskDelete(tid);
}

#define  LKPREAMPBIT	0x04

static void
lkpowerHandler( int lockpower_param )
{
	char	  msg4Aupdt[ 60 ];
	ushort	  lkpreampregvalue, apvalue, lockapreg, lockpower;
	int	  len, tindex;
	int	(*converter)();
	int	(*storevalue)();

	lockpower = lockpower_param;
	tindex = xcode2index( LKPOWER );
	if (tindex < 0) {
		return;
	}

	if (lockpower <= 0) {
		lockpower = 0;
	}
	else if (lockpower > 68) {
		lockpower = 68;
	}

   /* turn lock preamp atten on/off based on the value of lockpower */

	lkpreampregvalue = getlkpreampgainvalue(); 
	if (lockpower >= 48) {
		lockpower = lockpower-20;
		lkpreampregvalue |= LKPREAMPBIT;
	}
	else
	  lkpreampregvalue &= (~LKPREAMPBIT);

	storelkpreampgainvalue( lkpreampregvalue );

	lockapreg = index2apreg( tindex );
	converter = index2converter( tindex );
	if (converter != NULL)
	  apvalue = (*converter)( lockpower );
	else
	  apvalue = lockpower;

/*  The initial 2 shows that two commands follow.  The "1" encoded in
    the sprintf format string shows that only 1 AP bus register is to
     be set.  At this time a 1 is the only value permitted here.	*/

	sprintf( &msg4Aupdt[ 0 ], "2;%d,%d,1,%d;%d,%d,1,%d;",
		  FIX_APREG, lockapreg, apvalue,
		  FIX_APREG, getlkpreampgainapaddr(), lkpreampregvalue );
	len = strlen( &msg4Aupdt[ 0 ] );
	msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
	storevalue = index2valuestore( tindex );
	if (storevalue != NULL)
	  (*storevalue)( lockpower_param );
}

ladcHandler( short *paramvec, int paramcount )
{
	int	apvalue, iter, len, tindex;
	ushort	lockapreg;
	char	msg4Aupdt[ 32 ];
	int	(*converter)();
	int	(*storevalue)();
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_LOCKHDLR,NULL,NULL);
#endif

	if (paramcount % 2 == 1) {
		errLogRet( LOGIT, debugInfo,
			  "lock ADC handler called with odd number of parameters\n"
		);
	}

	for (iter = 0; iter < paramcount; iter += 2) {
		if (paramvec[ iter ] == LKPOWER) {
			lkpowerHandler( (int) paramvec[ iter+1 ] );
			continue;
		}

		tindex = xcode2index( (int) paramvec[ iter ] );
		if (tindex < 0) {
			continue;
		}

		lockapreg = index2apreg( tindex );
		converter = index2converter( tindex );
		if (converter != NULL)
		  apvalue = (*converter)( paramvec[ iter + 1 ] );
		else
		  apvalue = paramvec[ iter + 1 ];
		sprintf( &msg4Aupdt[ 0 ], "1;%d,%d,1,%d;", FIX_APREG, lockapreg, apvalue );
		len = strlen( &msg4Aupdt[ 0 ] );
		msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);

		storevalue = index2valuestore( tindex );
		if (storevalue != NULL)
		  (*storevalue)( paramvec[ iter+1 ] );
	}
}

/********************************************************/

static int
qspiShimInterface( short *paramvec, int paramcount )
{
	int	iter, ival, qspi_dac;
	short	qspiShimVector[ MAXNUMARGS * 2 ];

	for (iter = 1; iter < paramcount; iter += 2) {

/*  DAC numbers are in indexes 1, 3, etc.  */

		qspi_dac = qspi_dac_addr[ paramvec[ iter ] ];
		if (qspi_dac < 0) {
		   DPRINT1(2,"DAC %d not supported on the QSPI interface\n", paramvec[ iter ] );
		   return( -1 );
		}
		else
		  qspiShimVector[ iter-1 ] = qspi_dac;

		qspiShimVector[ iter ] = paramvec[ iter+1 ];
	}

/*  Subtract 1 from the param count, since the
    count includes the X-code, SET_DAC or SHIMDAC	*/

	ival = autoShimMsgSend(pTheAutoObject,
		 (char *) (&qspiShimVector[ 0 ]),
		          (paramcount-1) * sizeof( short )
	);

	return( 0 );
}

static int
serialMsrShimInterface( short *paramvec, int paramcount )
{
	int	ival;

	ival = autoShimMsgSend(pTheAutoObject,
		 (char *) (&paramvec[ 1 ]),
		          (paramcount-1) * sizeof( short ));
	return( 0 );
}

static int
rriShimInterface( short *paramvec, int paramcount )
{
	int	iter;

	for (iter = 1; iter < paramcount; iter += 2) {
		setRRIShim( paramvec[ iter ], paramvec[ iter + 1 ], 0);
	}
}

static int
serialOmtShimInterface( short *paramvec, int paramcount )
{
	int	iter;

	for (iter = 1; iter < paramcount; iter += 2) {
		setSerialOmtShim( paramvec[ iter ], paramvec[ iter + 1 ] );
	}
}

shimHandler( short *paramvec, int paramcount )
{
	int	iter;

#ifdef DEBUG
	DPRINT(1,"shim handler for the Automation board called\n" );
	for (iter = 0; iter < paramcount; iter++) {
		DPRINT2(1,"arg %d:  %d\n", iter, paramvec[ iter ] );
	}
#endif

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SHIMHDLR,NULL,NULL);
#endif

	if (paramcount % 2 != 1) {
		errLogRet( LOGIT, debugInfo,
			  "shim handler called with incorrect number of parameters\n"
		);
	}
	if (paramcount > MAXNUMARGS * 2) {
		errLogRet( LOGIT, debugInfo,
			  "shim handler called with too many parameters\n"
		);
		return;
	}

	if (shimType == NO_SHIMS) {
		determineShimType();

	  /*  If it doesn't work, we'll find out in good time... */
	}

	switch (shimType) {
	  case NO_SHIMS:
		errLogRet( LOGIT, debugInfo,
	   "shim handler cannot establish type of shims\n"
		);
		return;

	  case SERIAL_SHIMS:
		errLogRet( LOGIT, debugInfo, "SERIAL SHIMS accessed from MSR\n");
		break;

	  case QSPI_SHIMS:
		qspiShimInterface( paramvec, paramcount );
		break;

	  case RRI_SHIMS:
		rriShimInterface( paramvec, paramcount );
		break;

	  case MSR_SERIAL_SHIMS:
	  case MSR_RRI_SHIMS:
	  case MSR_OMT_SHIMS:
          case  SPI_THIN_SHIMS:
                DPRINT(1,"shimHandler: serialMsrShimInterface()\n");
		serialMsrShimInterface( paramvec, paramcount );
		break;

	  case OMT_SHIMS:
		serialOmtShimInterface(paramvec, paramcount);
		break;

	  default:
		errLogRet( LOGIT, debugInfo, "unknown shims set %d\n", shimType );
		return;
	}
#if 0

/*  Next loop updates the status block  */

	for (iter = 1; iter < paramcount; iter += 2) {
		currentStatBlock.stb.AcqShimValues[ paramvec[ iter ] ] = paramvec[ iter+1 ];
	}
#endif
}

spnrHandler( short *paramvec, int paramcount )
{
	int	iter;

#ifdef DEBUG
	DPRINT( 2, "spinner handler for the Automation board called\n" );
	for (iter = 0; iter < paramcount; iter++) {
		DPRINT2( 2, "arg %d:  %d\n", iter, paramvec[ iter ] );
	}
#endif

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SPINHDLR,NULL,NULL);
#endif
	iter = 0;
	while (iter < paramcount) {
		switch (paramvec[ iter ]) {

/*  Increment iter for each argument that you process.  Currently X-codes have
    either no arguments or one argument.  Notice that no change is made to iter
    for those X-codes, e.g. EJECT, that have no arguments.  After the switch
    block completes, iter is incremented to account for the X-code itself.	*/

		  case EJECT:
            	        sampleHasChanged = TRUE;
   	      		currentStatBlock.stb.AcqSample = (short) 0;
			autoSampleEject( pTheAutoObject );
			break;

		  case EJECTOFF:
			autoEjectOff( pTheAutoObject );
			break;

		  case INSERT:
			autoSampleInsert( pTheAutoObject );
			break;

		  case SETSPD:
			iter++;
		DPRINT2( 1, "spin speed: %d  %d\n", paramvec[ iter ], (unsigned short) paramvec[ iter ] );
			DPRINT1( 1, "User wants to set spinner speed to %d\n",
				     (unsigned short) paramvec[ iter ] );
			autoSpinSpeedSet( pTheAutoObject, (unsigned short) paramvec[ iter ] );
			setSpinSet( (unsigned short) paramvec[ iter ] );
			break;

		  case SETRATE:
			iter++;
		DPRINT2( 1, "spin rate: %d  %d\n", paramvec[ iter ], (unsigned short) paramvec[ iter ] );
			DPRINT1( 1, "User wants to set spinner air rate to %d\n",
				     (unsigned short) paramvec[ iter ] );
			autoSpinRateSet( pTheAutoObject, (unsigned short) paramvec[ iter ] );
			break;

		  case BEAROFF:
			DPRINT( 1, "User wants to turn spinner bearing air off\n" );
			autoBearingOff( pTheAutoObject );
			break;

		  case BEARON:
			DPRINT( 1, "User wants to turn spinner bearing air on\n" );
			autoBearingOn( pTheAutoObject );
			break;

		  case RESET_VT:
			DPRINT( 1, "User wants to Reset VT Controller\n" );
			resetVT();
			break;

		  case SETLOC:
			iter++;
			DPRINT1( 1, "User wants to Set Sample Number to %d\n",
				paramvec[ iter ] );
                   	currentStatBlock.stb.AcqSample = (short) paramvec[ iter ];
			/* if (Irobot == SMSCHANGER )
      				robot('K',newloc); */
			break;

		  case SETMASTHRES:
			iter++;
			DPRINT1( 1, "User wants to Set Spinner MAS switchover Threshold Speed to %d Hz\n",
				paramvec[ iter ] );
			setSpinMASThres((int) paramvec[ iter ]);
			break;

		  case BUMPSAMPLE:
			DPRINT( 1, "User requested the sample get bumped\n" );
			autoSampleBump(pTheAutoObject);
			break;

		  default:
			DPRINT1( 1, "Unrecognized command %d\n", paramvec[ iter ] );
		}

		iter++;
	}
}

statHandler( short *paramvec, int paramcount )
{
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_STATHDLR,NULL,NULL);
#endif
	if (paramvec[ 0 ] == SETSTATUS && paramcount >= 2)
	  update_acqstate( (int) paramvec[ 1 ] );

	currentStatBlock.stb.AcqOpsComplCnt++;
	getstatblock();
}

syncHandler( short *paramvec, int paramcount )
{
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SYNCHDLR,NULL,NULL);
#endif
   giveParseSem();
}

tuneHandler( short *paramvec )
{
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_TUNEHDLR,NULL,NULL);
#endif
   settunefreq(paramvec+1); /* +1 --> settunefreq() does not like SET_TUNE command (32)
                               it is starting from channel #   */
}

nvRamHandler( short *paramvec, int paramcount )
{
int	i,nvRamResult;
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_NVRAMHDLR,NULL,NULL);
#endif
   for (i=1; i < paramcount; i++)
   {  nvRamResult = sysNvRamSet(&paramvec[i+1],2,paramvec[i]*2+0x500);
      if (nvRamResult != OK)
         printf("Trouble writing to NvRam\n");
      i++;
   }
}


static int	stat_before_autshm = ACQ_IDLE;

static char
set_crit(cc)			/* for shimi ( pases ints ) */
int             cc;
{
   char            tmp;

   switch (cc)
   {
   case LOOSE:
      tmp = 'l';
      break;
   case MEDIUM:
      tmp = 'm';
      break;
   case TIGHT:
      tmp = 't';
      break;
   case EXCELLENT:
      tmp = 'e';
      break;
   default:
      tmp = 'l';
   }
   return (tmp);
}

autshmHandler( short *paramvec, int paramcount )
{
	char	*shimcmd;
	void	*adummy;
	short	 sdummy;
	int	iter;
	long	Tag;

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_AUTSHMHDLR,NULL,NULL);
#endif
	iter = 0;
	while (iter < paramcount) {
		switch( paramvec[ iter ] ) {

/*  The X-interpreter tells the S-handler to do the autoshim, so it
    won't be blocked up doing the autoshim.  Even more important, the
    X-interpreter needs to be available to stop the autoshim, if that
    becomes desired or necessary.					*/

		  case SHIMI:
			if (paramcount - iter < SHIMI_PARAM_COUNT + 1) {
				errLogRet( LOGIT, debugInfo,
			"autoshim handler called with incorrect number of parameters\n"
				);
				return( -1 );
			}

/*  Look at shandler.c, case SHIMAUTO.  Notice that after it gets the address of
    the shim method string it then gets a whole bunch of Real Time indexes and
    the address of an A-code object.  These assist automated shimming on the FID.
    The host computer computes a set of A-codes to get this FID.  The console also
    needs a series of real-time variables so it knows how many points to collect,
    etc.  Currently autoshim-from-ACQI, which is what is happening here, only shims
    on the lock level.  No A-codes are required; the autoshim program reads the
    current lock level directly from the automation board.

    The Real Time indexes are each short words while the A-code object address may
    be represented as a Pointer-to-Void.  Use -1 for the index and NULL for the
    address to let the s-handler program know these values are not valid.	*/

			sdummy = -1;
			adummy = (void *) NULL;

		/*  It is necessary to malloc space for stuff to
		    be sent to the S-handler via its ring buffer  */

			shimcmd = (char *) malloc( SHIMI_METHOD_SIZE );
			if (shimcmd == NULL) {
				errLogRet( LOGIT, debugInfo,
			"autoshim handler can't allocate space for method string\n"
				);
				return( -1 );
			}

/*  Caution:  Be sure to not exceed the space allocated for the shim command.  */

			shimcmd[ 0 ] = LOCK_SH;
			shimcmd[ 1 ] = SEL_COIL;
			shimcmd[ 2 ] = 0;		/* new / fox value */
			memcpy( &shimcmd[ 3 ], &paramvec[ iter + 1 ], SHIMMASK_SIZE );
			shimcmd[ 7 ] = set_crit( paramvec[ iter + 3 ] );
			shimcmd[ 8 ] = set_crit( paramvec[ iter + 4 ] );
			shimcmd[ 9 ] = DONE_SH;		/* that's all folks!! */

/* due to differences in max of lockpower and lockgain between INOVA */
/* and Mercury these values are passed along from acqi	*/
			LockPwrMax = paramvec[ iter + 5 ];
			LockGainMax = paramvec[ iter + 6 ];

			stat_before_autshm = get_acqstate();

			/*update_acqstate( ACQ_SHIMMING );  let autoshim do this
			getstatblock();*/

/*  The procedure to get S-handler to do something for you is to first put something
    in its ring buffer, essentially the parameters associated with the action you want
    S-handler to do.  The format and form of the arguments to rngBufPut are required.
    Then you send a message to the Tag FIFO message queue telling it what to do.	*/

			rngBufPut(pSyncActionArgs,(char*) &shimcmd,sizeof(shimcmd));

/*  send:  nt index, np index, dpf index (double precision flag), ac offset, A-code ID.
    In this context, none have any significance, so the shandler receives -1 for the
    short words (real-time indices) and NULL for the A-code ID address			*/

			rngBufPut(pSyncActionArgs,(char*) &sdummy,sizeof(sdummy));
			rngBufPut(pSyncActionArgs,(char*) &sdummy,sizeof(sdummy));
			rngBufPut(pSyncActionArgs,(char*) &sdummy,sizeof(sdummy));
			rngBufPut(pSyncActionArgs,(char*) &sdummy,sizeof(sdummy));
			rngBufPut(pSyncActionArgs,(char*) &adummy,sizeof(adummy));
			rngBufPut(pSyncActionArgs,(char*) &LockPwrMax,sizeof(LockPwrMax));
			rngBufPut(pSyncActionArgs,(char*) &LockGainMax,sizeof(LockGainMax));

			Tag = 0x04000 | SHIMAUTO;
        		msgQSend(pTagFifoMsgQ, (char*) &Tag, sizeof(Tag), 
							NO_WAIT, MSG_PRI_NORMAL);
			iter += 7;

			break;

		  case STOP_SHIMI:

/*  Stop the autoshim on command by setting host_abort to a non-zero value.  */

			/*update_acqstate( stat_before_autshm );  let autoshim do this
			getstatblock();*/
			host_abort = 1;
			iter++;
			break;
		}
	}

	return( 0 );
}

#define  RCVRGAINAPREG	0x0b42

rcvrgainHandler( short *paramvec, int paramcount )
{
	char	msg4Aupdt[ 60 ];
	int	gain, iter, len;
	ushort	lkpreampregval, preampattn, rset;

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_RCVGAINHDLR,NULL,NULL);
#endif
	iter = 0;
	while (iter < paramcount) {
		switch( paramvec[ iter ] ) {

	   /* Adopted mostly from the RECEIVERGAIN A_code */

		  case RCVRGAIN:
			iter++;
			gain = paramvec[ iter++ ];
			calcRecvGainVals( gain, &preampattn, &rset );
			lkpreampregval = getlkpreampgainvalue(); 
			lkpreampregval = (lkpreampregval & ~0x0003) | preampattn;
			sprintf( &msg4Aupdt[ 0 ], "2;%d,%d,1,%d;%d,%d,1,%d;",
		  		  FIX_APREG, getlkpreampgainapaddr(), lkpreampregval,
		  		  FIX_APREG, RCVRGAINAPREG, rset );
			len = strlen( &msg4Aupdt[ 0 ] );
			msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1,
						 NO_WAIT, MSG_PRI_NORMAL);
			storelkpreampgainvalue(lkpreampregval);
   			currentStatBlock.stb.AcqRecvGain = (short) gain;
			break;

		  default:
			DPRINT1( 1, "Unrecognized command %d\n", paramvec[ iter ] );
			break;
		}
	}
}

fixAcodeHandler( short *paramvec, int paramcount )
{
	int	iter;

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_FIXACODEHDLR,NULL,NULL);
#endif
	iter = 1;
	if (fixAcodeIndex + paramcount - 1 > MAX_FIX_ACODES) {
		errLogRet( LOGIT, debugInfo, "fix A-code overran intermediate buffer\n" );
		return( -1 );
	}
	while (iter < paramcount) {
		fixAcodeVector[ fixAcodeIndex ] = paramvec[ iter ];
		fixAcodeVector[ fixAcodeIndex+1 ] = paramvec[ iter+1 ];
		iter += 2;
		fixAcodeIndex += 2;
	}

	return( 0 );
}

/*  Preliminary version of the set attn handler for ACQI IPA.
    The values are sent as pairs: AP register, value.		*/

setAttnHandler( short *paramvec, int paramcount )
{
	char	msg4Aupdt[ 32 ];
	int	attnapreg, attnvalue, iter, len;

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SETATTNHDLR,NULL,NULL);
#endif
	for (iter = 1; iter < paramcount; iter += 2) {
		attnapreg = paramvec[ iter ];
		attnvalue = paramvec[ iter+1 ];
		sprintf( &msg4Aupdt[ 0 ], "1;%d,%d,1,%d;",
			  FIX_APREG, attnapreg, attnvalue );

		len = strlen( &msg4Aupdt[ 0 ] );
		msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1,
					 NO_WAIT, MSG_PRI_NORMAL);
	}
}

setvtHandler( short *paramvec, int paramcount )
{
	short	temp, ltmpxoff, pid;
	int	iter;
	long	Tag;

/*  The PID encodes special stuff of interest to the VT.  However the sethw
    command does not make use of the PID.  Therefore we send -1 to the s-handler,
    which understands that a negative value is to be ignored in this context.	*/

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SETVTHDLR,NULL,NULL);
#endif
	pid = -1;
	for (iter = 1; iter < paramcount; iter += 2) {
		temp = paramvec[ iter ];
		ltmpxoff = paramvec[ iter+1 ];
	    DPRINT2( 1, "set VT handler: temperature: %d, low temperature cutoff: %d\n",
			  temp, ltmpxoff );
	        setVT(temp,ltmpxoff,pid);
	}
}

shmsetHandler( short *paramvec, int paramcount )
{
#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_SHIMSETHDLR,NULL,NULL);
#endif
	establishShimType( paramvec[ 1 ] );
}

lkfreqHandler( short *paramvec, int paramcount )
{
	ushort	  lockapreg, lockapreg_2;
	int	  tindex;
	char	  msg4Aupdt[ 60 ];
	int	(*storevalue)();

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_LKFREQHDLR,NULL,NULL);
#endif
	DPRINT2( 1, "lock freq handler called with 0x%04x 0x%04x\n",
		      paramvec[ 1 ], paramvec[ 2 ] );

	tindex = xcode2index( paramvec[ 0 ] );
	if (tindex < 0) {
		return;
	}

	lockapreg = index2apreg( tindex );
	lockapreg_2 = lockapreg + 2;

	sprintf( &msg4Aupdt[ 0 ], "3;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;",
		  FIX_APREG, lockapreg, paramvec[ 1 ] | 0xff00,
		  FIX_APREG, lockapreg+1, (paramvec[ 1 ] >> 8) | 0xff00,
		  FIX_APREG, lockapreg_2, paramvec[ 2 ] | 0xff00 );
	DPRINT1( 1, "lock freq handler will send %s to A_updt\n", &msg4Aupdt[ 0 ] );
	msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], strlen( &msg4Aupdt[ 0 ] ) + 1,
				 NO_WAIT, MSG_PRI_NORMAL);

	storevalue = index2valuestore( tindex );
	if (storevalue != NULL) {
		int	lockfreq_value;

		lockfreq_value = ( (paramvec[ 1 ] & 0xffff) | ((paramvec[ 2 ] & 0xff) << 16) );
		(*storevalue)( lockfreq_value );
	}
}

statTimer(short *paramvec, int paramcount)
{
    set_new_interval((int)paramvec[1]);
}

gpaHandler(short *paramvec, int paramcount)
{
    int cmd;
    DPRINT3(2,"paramcount=%d: %d, %d", paramcount, paramvec[0], paramvec[1]);
    /* Translate ACQ_SUN.h command to mboxcmds.h command. */
    cmd = paramvec[0] + SET_GPA_TUNE_PX - GTUNE_PX;
    gpaTuneSet(pTheAutoObject, cmd, paramvec[1]);
}

