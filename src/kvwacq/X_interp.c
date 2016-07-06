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
#include <msgQLib.h>
#include <semLib.h>

#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "acqcmds.h"
#include "config.h"
#include "autoObj.h"
#include "lkapio.h"
#include "fifoObj.h"
#include "hardware.h"
#include "acodes.h"		/* SHIMAUTO */
#include "X_interp.h"
#include "serialShims.h"
#include "vtfuncs.h"
#include "spinObj.h"
#include "sram.h"
#include "instrWvDefines.h"

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
extern FIFO_ID   pTheFifoObject;
extern RING_ID   pSyncActionArgs;	/* rngbuf to request autoshim */
extern SPIN_ID   pTheSpinObject;


extern short    rgain[];
extern int	cvtLkDbLinear();
extern int	fixLkPhase();
extern int	calcRecvGainVals( int gain, ushort *pPreampAttn, ushort *pRset );
extern unsigned short    *settunefreq(unsigned short *intrpp);

extern int sampleHasChanged;	/* Global Sample change flag */
extern struct	_conf_msg {
	long	msg_type;
	struct	_hw_config hw_config;
} conf_msg;

int	mshimHandler();
static short	LockPwrMax, LockGainMax;

static struct {
   int    changecode;
   int    argcount;
   int  (*handler)();
   char  *text;
} Xcmd[] = {
	{ LKPOWER,	1,	ladcHandler,   "Set lock power" },
	{ LKGAIN,	1,	ladcHandler,   "Set lock gain"  },
	{ LKPHASE,	1,	ladcHandler,   "Set lock phase"  },
	{ SHIMDAC,	2,	mshimHandler,   "Set shim DAC (alias 1)"  },
	{ SET_DAC,	2,	mshimHandler,   "Set shim DAC (alias 2)"  },
	{ LKMODE,	1,	ladcHandler,   "Lock Mode, on, off or auto" },
	{ LKTC,		1,	ladcHandler,   "Set lock time constant for not acquiring" },
	{ LKACQTC,	1,	ladcHandler,   "Set lock time constant for acquiring" },
	{ BEAROFF,	0,	spnrHandler,   "spinner Bearing air off" },
	{ BEARON,	0,	spnrHandler,   "spinner Bearing air on" },
	{ SETRATE,	1,	spnrHandler,   "set spinner air rate" },
	{ SETSPD,	1,	spnrHandler,   "set spinner speed" },
	{ EJECT,	0,	spnrHandler,   "eject sample" },
        { EJECTOFF,     0,      spnrHandler,   "turn off eject" },
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
        { SET_TUNE,     1,      tuneHandler,   "for tune command" },
        { RESET_VT,     0,      spnrHandler,   "Reset VT controller" },
        { SETLOC,       1,      spnrHandler,   "Set Sample Number" },
        { SETMASTHRES,  1,      spnrHandler,   "Set MAS Switchover Threshold (Hz)" },
        { BUMPSAMPLE,   0,      spnrHandler,   "bump the sample" },
        { SETTEMP,      2,      setvtHandler,  "Set temperature in the VT controller" },
        { SHIMSET,      1,      shmsetHandler, "Specify type of shims" },
        { SETSTATUS,    1,      statHandler,   "Set console acquisition status" },
        { LKFREQ,       2,      lkfreqHandler, "Specify lock frequency" },
	{ STATRATE,	1,	statTimer,	"set statblock update rate" },
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
/*	        shimType = QSPI_SHIMS;
/*		qspi_dac_addr = shimGetQspiTab();
/*		/* qspi_dac_addr = &qspi_dac_table[ 0 ]; */ 
		shimType = APBUS_SHIMS;
		break;

	  case 2:
	  case 3:
	  case 4:
	  case 6:
	  case 7:
	  case 9:
	        if (shimLoc == SHIMS_ON_MSR)
		{
		   shimType = MSR_SERIAL_SHIMS;
		}
		else
		{
		   shimType = SERIAL_SHIMS;
		   serialShims = initSerialShims();
		   if (serialShims < 0) {
			errLogRet( LOGIT, debugInfo,
				  "Warning: serial shims not responding\n" );
		   }
		}
		break;

	  case 5:
	        if (shimLoc == SHIMS_ON_MSR)
		{
		   shimType = MSR_RRI_SHIMS;
		}
		else
	        {
		   shimType = RRI_SHIMS;
		   if ( (serialShims = areSerialShimsSetup()) == 0)
		     serialShims = initSerialShims();
		   if (serialShims < 0) {
			errLogRet( LOGIT, debugInfo,
				  "Warning: RRI shims not responding\n" );
		   }
		}
		break;

	  case 8:
	        shimType = QSPI_SHIMS;
		qspi_dac_addr = shimGetImgQspiTab();
		/* qspi_dac_addr = &imgqsp_dac_table[ 0 ]; */
		break;

	  case 11:
		/* shimType is already set */
		break;

	  default:
		shimType = NO_SHIMS;
		break;
	}
    DPRINT1(0,"establishShimType: shimType %d\n",shimType);
    currentStatBlock.stb.AcqShimSet = shimset;
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

   /* Parse Command -  		*/
   /* "nextptr" will point to the character after the token and should	*/
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
char      msg4Aupdt[ 128 ];
ushort    lk18dB, lockpower;
int       converter, len;

   lockpower = lockpower_param;

   if (lockpower <= 0)
   {  lockpower = 0;
   }
   else if (lockpower > 48)
        { lockpower = 48;
        }

   lk18dB = getLock18dB();
   if (lockpower < 33)
   {  lockpower = lockpower+15; /* they call it 18 dB, but it is 15 */
      lk18dB |= LKPREAMPBIT;
   }
   else
      lk18dB &= (~LKPREAMPBIT);
   setLock18dB( lk18dB );

   converter = cvtLkDbLinear( lockpower );

/*  The initial 4 shows that four commands follow.  The "1" encoded in
    the sprintf format string shows that only 1 AP bus register is to
    be set.  At this time a 1 is the only value permitted here. */

   sprintf( &msg4Aupdt[ 0 ], "4;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;",
                  FIX_APREG, 0x0A30, 0x80,
                  FIX_APREG, 0x0A31, lk18dB,
                  FIX_APREG, 0x0A30, 0x50,
                  FIX_APREG, 0x0A31, converter );
   len = strlen( &msg4Aupdt[ 0 ] );
   msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);

   currentStatBlock.stb.AcqLockPower = lockpower_param;

}

ladcHandler( short *paramvec, int paramcount )
{
int     iter, len;
char    msg4Aupdt[ 256 ];

   if (paramcount % 2 == 1)
   {  errLogRet( LOGIT, debugInfo,
		"lock ADC handler called with odd number of parameters\n");
   }

   for (iter=0; iter < paramcount; iter +=2)
   {  switch (paramvec[ 0 ])
      { case LKPOWER:
		lkpowerHandler( (int) paramvec[ iter+1 ]);
		break;
	case LKGAIN:
		{ char tmpmode, getLockmodevalue();
		  short tmpdac,tmpval;
		  tmpmode = getLockmodevalue() & ~(LM_10DB + LM_20DB);
		  tmpval=paramvec[iter+1];
		  tmpmode |= (tmpval / 10) << 1;
		  tmpmode &= 0xFF;
		  tmpdac = rgain[tmpval%10];
		  sprintf( &msg4Aupdt[ 0 ],
		        "4;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;",
		        FIX_APREG, 0x0A38, 0x80,
		        FIX_APREG, 0x0A39, tmpmode,
		        FIX_APREG, 0x0A38, 0x50,
		        FIX_APREG, 0x0A39, tmpdac);
		  setLockmodevalue(tmpmode);
		  currentStatBlock.stb.AcqLockGain = paramvec[ iter+1 ];
		}
		break;
	case LKPHASE:
		{
		  DPRINT1( 2, "X_lockphase %ld\n", paramvec[iter+1] );
		  sprintf( &msg4Aupdt[ 0 ], "1;%d,%d,1;",
			LKPHASE,  paramvec[iter+1] );
		}
		break;
        case LKMODE:
		{  char tmpmode,getLockmodevalue();
		   tmpmode = getLockmodevalue();
		   if (paramvec[iter+1] == LKON)
		   {  tmpmode |= LM_ON_OFF;
		      currentStatBlock.stb.AcqLSDVbits |= 0x2;
		   }
		   else
		   {  tmpmode &= ~LM_ON_OFF;
		      currentStatBlock.stb.AcqLSDVbits &= ~0x2;
		   }
		   tmpmode &= 0xFF;
		   sprintf( &msg4Aupdt[ 0 ],
		        "2;%d,%d,1,%d;%d,%d,1,%d;",
		        FIX_APREG, 0x0A38, 0x80,
		        FIX_APREG, 0x0A39, tmpmode);
		   setLockmodevalue(tmpmode);
                }
                break;
        case LKTC:
                msg4Aupdt[ 0 ] = '\000';
                break;
        case LKACQTC:
                msg4Aupdt[ 0 ] = '\000';
                break;
	case SET_DAC:
	case SHIMDAC:
		
        default:
                msg4Aupdt[ 0 ] = '\000';
                break;
      }
     len = strlen( &msg4Aupdt[ 0 ] );
     if (len > 0)
        msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
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
serialShimInterface( short *paramvec, int paramcount )
{
	int	iter;

	for (iter = 1; iter < paramcount; iter += 2) {
		setSerialShim( paramvec[ iter ], paramvec[ iter + 1 ] );
	}
}

static int
rriShimInterface( short *paramvec, int paramcount )
{
	int	iter;

	for (iter = 1; iter < paramcount; iter += 2) {
		setRRIShim( paramvec[ iter ], paramvec[ iter + 1 ] );
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

static int
apbusShimInterface (short *paramvec, int paramcount)
{
        int     iter;

        for (iter = 1; iter < paramcount; iter += 2) {
                setApbusShims (paramvec[ iter ], paramvec[ iter + 1 ] );
        }
}

int
mshimHandler(short *paramvec, int paramcount)
{
int	iter,len;
char    msg4Aupdt[ 256 ];
   if (shimType == NO_SHIMS)
   {  determineShimType();
      /*  If it doesn't work, we'll find out in good time... */
   }
   if (shimType != APBUS_SHIMS)
   {  shimHandler(paramvec,paramcount);
      return;
   }

   for (iter = 0; iter < paramcount; iter += 3)
   {   DPRINT2 (3,"DAC=%d value=%d\n",paramvec[iter+1], paramvec[iter+2]);
       setApbusShims2( paramvec[iter+1], paramvec[iter+2], msg4Aupdt);
       len = strlen( &msg4Aupdt[ 0 ] );
       msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
       currentStatBlock.stb.AcqShimValues[ paramvec[iter+1] ]=paramvec[iter+2];
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

	DPRINT1(1,"shim type=%d (should be 8 for Mercury)\n",shimType );
	switch (shimType) {
	  case NO_SHIMS:
		errLogRet( LOGIT, debugInfo,
	   "shim handler cannot establish type of shims\n"
		);
		return;

	  case SERIAL_SHIMS:
		serialShimInterface( paramvec, paramcount );
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
                DPRINT(1,"shimHandler: serialMsrShimInterface()\n");
		serialMsrShimInterface( paramvec, paramcount );
		break;

	  case OMT_SHIMS:
		serialOmtShimInterface(paramvec, paramcount);
		break;

	  case APBUS_SHIMS:
                apbusShimInterface(paramvec, paramcount);
                break;

	  default:
		errLogRet( LOGIT, debugInfo, "unknown shims set %d\n", shimType );
		return;
	}

/*  Next loop updates the status block  */

	for (iter = 1; iter < paramcount; iter += 2) {
		currentStatBlock.stb.AcqShimValues[ paramvec[ iter ] ] = paramvec[ iter+1 ];
	}
}

spnrHandler( short *paramvec, int paramcount )
{
int     iter, not_spincmd;
SPIN_CMD        spinCmd;

#ifdef DEBUG
	DPRINT( 2, "spinner handler for the Automation board called\n" );
	for (iter = 0; iter < paramcount; iter++) {
		DPRINT2( 2, "arg %d:  %d\n", iter, paramvec[ iter ] );
	}
#endif

	not_spincmd=0;
	iter = 0;
	while (iter < paramcount) {

/*  Increment iter for each argument that you process.  Currently X-codes have
    either no arguments or one argument.  Notice that no change is made to iter
    for those X-codes, e.g. EJECT, that have no arguments.  After the switch
    block completes, iter is incremented to account for the X-code itself.	*/

      switch (paramvec[ iter ])
      {
      case EJECT:
      case EJECTOFF:
      case INSERT:
      case BUMPSAMPLE:
      case BEAROFF:
      case BEARON:
                spinCmd.VTCmd = paramvec[ iter ];
                break;
      case SETSPD:
      case SETRATE:
      case SETMASTHRES:
                spinCmd.VTCmd  = paramvec[ iter ]; iter++;
                spinCmd.VTArg1 = (unsigned short) paramvec[ iter ];
                break;
      default:
                not_spincmd=1;
                break;
      }
      if ( ! not_spincmd)
         msgQSend(pTheSpinObject->pSpinMsgQ,(char*) &spinCmd,sizeof(spinCmd),
                NO_WAIT,MSG_PRI_NORMAL);
      else
      {  switch (paramvec[ iter ] )
         {case RESET_VT:
                DPRINT( 0, "User wants to Reset VT Controller\n" );
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

          default:
                DPRINT1( 1, "Unrecognized command %d\n", paramvec[ iter ] );
         }
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
	  update_acqstate( paramvec[ 1 ] );

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
int	tunetype;
int     hbXmtrType, amptype;
int	fine,coarse;	/* bitsetting fro fine and coarse attenuator */
short	nvRamShort[2];	/* sysNvRamGet append a NULL so we need 2 */
   tunetype = *(paramvec+1);
   DPRINT1( -1, "SET_TUNE: tunetype=%d\n",tunetype );

#ifdef INSTRUMENT
     wvEvent(EVENT_XPARSER_TUNEHDLR,NULL,NULL);
#endif
   if ( (tunetype==1) || (tunetype==2) || (tunetype==3) )
      update_acqstate(ACQ_TUNING);
   hbXmtrType = conf_msg.hw_config.xmtr_stat[0] & 0x10;
   sysNvRamGet(nvRamShort, 2, (PTS_OFFSET + 2)*2 + 0x500);
   amptype=nvRamShort[0];
   switch( tunetype )
   {
      case 1:
	fifoClrHsl (pTheFifoObject, 0);		 /* clear High Speed Lines */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a01); /* set locktune off (2kHz) */
        amptype = amptype/10;
        if ( ! hbXmtrType)
        {  coarse = 0x01;
           if (amptype == 1)
	      fine   = 0xf5;
           else if (amptype == 2)
	      fine   = 0x91;
           else
              fine   = 0x6E;
	   fifoStuffCmd(pTheFifoObject, 0, 0xaa18);
	   fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	   fifoStuffCmd(pTheFifoObject, 0, 0x9a00 | fine);
	   fifoStuffCmd(pTheFifoObject, 0, 0x9a00 | coarse);/* safe pwr lvl */
	}
        else
        {  fine = 0xff;
	   if (amptype == 1)
	      coarse = 0x22;	/* 17 dB attn for 35 W */
	   else if (amptype == 2)
	      coarse = 0x2A;	/* 21 dB attn for 75 W */
	   else
	      coarse = 0x32;	/* 25 dB attn for 125 W */
	   fifoStuffCmd(pTheFifoObject, 0, 0xa150);
	   fifoStuffCmd(pTheFifoObject, 0, 0xb100 | fine);
	   fifoStuffCmd(pTheFifoObject, 0, 0x8100 | coarse);/* some safe pwr lvl */
	}
   DPRINT4( 2, "SET_TUNE: coarse=%d, fine=%d, xmtr=%d amp=%d\n",coarse, fine, hbXmtrType, amptype );
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a01); /* hi band into tune */
	fifoMaskHsl (pTheFifoObject, 0, 0x80);   /* set HI-BND XMTR HSL */
	fifoStuffCmd(pTheFifoObject, 0, 0x0018);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 2:
        amptype = amptype%10;
        coarse = 0x00;
        if (amptype == 1)
	   fine   = 0xaf;		/* 175 */
        else if (amptype == 2)
	   fine   = 0x5f;		/* 75 */
        else
           fine   = 0x2a;		/* 42 */
	fifoClrHsl (pTheFifoObject, 0);		 /* clear High Speed Lines */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a01); /* set locktune off (2kHz) */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa00);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a00 | fine);  /* better is 5f */
	fifoStuffCmd(pTheFifoObject, 0, 0x9a00 | coarse);/* some safe pwr lvl */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a02); /* lo band into tune */
	fifoStuffCmd(pTheFifoObject, 0, 0x0018);
	fifoMaskHsl (pTheFifoObject, 0, 0x10);   /* set LOW-BND XMTR HSL */
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 3:
	fifoClrHsl (pTheFifoObject, 0);		 /* clear High Speed Lines */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a00); /* hi/lo band tune off */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa30);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a30); /* full power for lock */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa30);
	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a02); /* add extra lock power */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a22); /* set lock to tune */
	fifoStuffCmd(pTheFifoObject, 0, 0x0018);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 21:			/* turn AUX1 on  */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa22);
	fifoStuffCmd(pTheFifoObject, 0, 0xba01);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 22:			/* turn AUX1 on  */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa22);
	fifoStuffCmd(pTheFifoObject, 0, 0xba00);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 23:			/* turn AUX2 off */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa22);
	fifoStuffCmd(pTheFifoObject, 0, 0xba02);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      case 24:			/* turn AUX2 off */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa22);
	fifoStuffCmd(pTheFifoObject, 0, 0xba00);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	break;
      default:
	fifoClrHsl (pTheFifoObject, 0);		 /* clear High Speed Lines */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a01); /* set locktune off (2 kHz) */
/*	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
/*	fifoStuffCmd(pTheFifoObject, 0, 0xba80);
/*	fifoStuffCmd(pTheFifoObject, 0, 0x9a02); /* drop extra lock power */
	fifoStuffCmd(pTheFifoObject, 0, 0xaa20);
	fifoStuffCmd(pTheFifoObject, 0, 0xba50);
	fifoStuffCmd(pTheFifoObject, 0, 0x9a00); /* turn hi/lo xmtr off */
	setLockPower(currentStatBlock.stb.AcqLockPower,FALSE);
	fifoStuffCmd(pTheFifoObject,HALTOP, 0);
	fifoStart(pTheFifoObject);
	fifoWait4Stop(pTheFifoObject);
	update_acqstate(ACQ_IDLE);
	break;
   }
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

/* due to differences in max of lockpower and lockgain between INOVA and Mercury */
/* these values are passed along from acqi	*/
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
   			currentStatBlock.stb.AcqRecvGain = gain;
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
DPRINT2(0,"count=%d, temp=%d\n",paramcount,paramvec[1]);
	pid = -1;
	for (iter = 1; iter < paramcount; iter += 2) {
		temp = paramvec[ iter ];
		ltmpxoff = paramvec[ iter+1 ];
	    DPRINT2( 0, "set VT handler: temperature: %d, low temperature cutoff: %d\n",
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

