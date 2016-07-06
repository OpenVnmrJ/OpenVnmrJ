/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* phandler.c 11.1 07/09/07 - Problem Handler Task */
/* 
 */

/*
modification history
--------------------
12-7-94,gmb  created 
4-29-97,rol  added argument to reset2SafeState, so if the connection to
             the host is lost (message type == LOST_CONN), reset2SafeState
             does not try to resynchronize the downlinker.
*/

/*
DESCRIPTION

   This Task Handlers the Problems or Exceptions that happen to the
   system. For Example Fifo Errors (FORP,FOO,etc).

   The logic for handling these execption is encapsulated in this task
   thus there is one central place where the logic is placed.

*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <rngLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "hardware.h"
#include "timeconst.h"
#include "taskPrior.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "adcObj.h"
#include "autoObj.h"
#include "tuneObj.h"
#include "sysflags.h"
#include "instrWvDefines.h"
#include "mboxcmds.h"

extern SEM_ID  pSemSAStop; /* Binary  Semaphore used to Stop upLinker for SA */

extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

/* Hardware Objects */
extern FIFO_ID		pTheFifoObject;
extern STMOBJ_ID	pTheStmObject;
extern ADC_ID		pTheAdcObject;
extern AUTO_ID		pTheAutoObject;
extern TUNE_ID		pTheTuneObject;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

extern RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function args */

/* Fixed & Dynamic Named Buffers */
extern DLB_ID  pDlbDynBufs;
extern DLB_ID  pDlbFixBufs;

extern int     SA_Criteria;/* SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo for SA, ie which fid to stop at 'il'*/

typedef struct {
		int expCase;
                int Status; 
                int Event;
		} STAT_MSGE;

static void resetSystem();
static void reset2SafeState();
static void resetNameBufs();
static void AbortExp(EXCEPTION_MSGE *msge);

static STAT_MSGE statMsg = { CASE, 0, 0 };

static int pHandlerTid;
static int pHandlerPriority;

WDOG_ID pHandlerWdTimeout;
int pHandlerTimeout;


/* Safe State Codes						*/
/* The first two words are high speed line safe states.		*/
/* The next words are waveform generator and pulse field	*/
/* gradient codes to disable their outputs.			*/
/* The last several Codes to enable the ADC1 on the STM and CTC */
/* on the ADC and generate a CTC is to clear the Rcvr Ovfl	*/
/* LED just incase its on, since the ADC will not clear this    */
/* unless it converts (CTC) a signal below the Recvr or ADC     */
/* Ovfl. Hey what can I say it's hardware and has Dick B.'s     */
/* Blessing. Greg B.	12/11/96				*/
/* To fix a DTM glitch problem, (on loading NT cnt a glitch is  */
/* produced that decrements the NT count resulting the the NOISE*/
/* watch-dog error, to get around this glitch on abort load the */
/* NT cnt with Zero and issue RELOAD cmd. A new FPGA fixes the  */
/* problem in hardware as it should. 10/18/97			*/
static long pSafeStateCodes[] = { 
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c120080>>5), 0x0c120080<<27,		/* rf wfg(1) */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c1a0080>>5), 0x0c1a0080<<27,		/* rf wfg(2) dec 1 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c4a0080>>5), 0x0c4a0080<<27,		/* rf wfg(3) dec 2 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c420080>>5), 0x0c420080<<27,		/* rf wfg(4) dec 3 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

        /*  Begin the CRB, should be prior to the Grad WFG (?) */
	CL_AP_BUS | (0x0c940000>>5), 0x0c940000<<27,		/* CRB reg 0, Reset fifos  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c940040>>5), 0x0c940040<<27,		/* CRB reg 0, Enable fifos  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

	/* load the 1st of 3 CRB DSP with 5 32bit values into DSP 1 */
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 1 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 1  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 1  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 1  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 2 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 2  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 2  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970080>>5), 0x0c970080<<27,		/* CRB DSP1 2  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 3 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 3  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 3  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970080>>5), 0x0c970080<<27,		/* CRB DSP1 3  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 4 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 4  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 4  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970080>>5), 0x0c970080<<27,		/* CRB DSP1 4  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 5 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 5  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 5  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c970000>>5), 0x0c970000<<27,		/* CRB DSP1 5  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

	/* load the 2nd of 3 CRB DSP with 5 32bit values into DSP 1 */
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 1 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 1  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 1  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960080>>5), 0x0c960080<<27,		/* CRB DSP2 1  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 2 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 2  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 2  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 2  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 3 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 3  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 3  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960080>>5), 0x0c960080<<27,		/* CRB DSP2 3  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 4 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 4  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 4  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960080>>5), 0x0c960080<<27,		/* CRB DSP2 4  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 5 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 5  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 5  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c960000>>5), 0x0c960000<<27,		/* CRB DSP2 5  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

	/* load the 3rd of 3 CRB DSP with 5 32bit values into DSP 1 */
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 1 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 1  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 1  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950080>>5), 0x0c950080<<27,		/* CRB DSP3 1  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 2 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 2  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 2  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950080>>5), 0x0c950080<<27,		/* CRB DSP3 2  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 3 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 3  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 3  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 3  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 4 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 4  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 4  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950080>>5), 0x0c950080<<27,		/* CRB DSP3 4  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 5 LSB byte of 32bit  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 5  2nd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 5  3rd byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c950000>>5), 0x0c950000<<27,		/* CRB DSP3 5  MSB byte */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

        /*   interrupt DSPs to load the values */
	CL_AP_BUS | (0x0c940047>>5), 0x0c940047<<27,		/* interrupt CRB DSP to load values */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c940040>>5), 0x0c940040<<27,		/* CRB reg 0, Enable fifos  */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
        /* there all done with the CRB */

        /* reset grad wfgs */
	CL_AP_BUS | (0x0c220080>>5), 0x0c220080<<27,		/* grad wfg */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c2a0080>>5), 0x0c2a0080<<27,		/* grad wfg */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c320080>>5), 0x0c320080<<27,		/* grad wfg */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c4a0080>>5), 0x0c4a0080<<27,
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c230000>>5), 0x0c230000<<27,
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c2b0000>>5), 0x0c2b0000<<27,
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c330000>>5), 0x0c330000<<27,
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c800080>>5), 0x0c800080<<27,		/* EPI Bstr */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c800000>>5), 0x0c800000<<27,		/* EPI Bstr */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c50ff08>>5), 0x0c50ff08<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c50ff00>>5), 0x0c50ff00<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c54ff08>>5), 0x0c54ff08<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c54ff00>>5), 0x0c54ff00<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c58ff08>>5), 0x0c58ff08<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c58ff00>>5), 0x0c58ff00<<27,		/* pfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c600000>>5), 0x0c600000<<27,		/* lpfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c640000>>5), 0x0c640000<<27,		/* lpfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c680000>>5), 0x0c680000<<27,		/* lpfg */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0c8a4000>>5), 0x0c8a4000<<27,		/* tpfg -- CAREFUL  */
 	CL_DELAY | (PFG_HW_MIN_DELAY_CNT>>5), PFG_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e800000>>5), 0x0e800000<<27,		/* adc */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e820000>>5), 0x0e820000<<27,		/* adc */
	/*   move this ADC ovrflow clear code to bottom 
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e000028>>5), 0x0e000028<<27,		/* enable ADC1 on STM */
 	/* CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e820010>>5), 0x0e820010<<27,		/* enable CTC on ADC */
 	/* CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
 	CL_CTC | (AP_ITRP_MIN_DELAY_CNT>>5), AP_ITRP_MIN_DELAY_CNT<<27,	/*  MAKE A CTC */
	/* */
 	CL_AP_BUS | (0x0e180000>>5), 0x0e180000<<27,	/*  Setting Np count on DTM to Zero */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
 	CL_AP_BUS | (0x0e1A0000>>5), 0x0e1A0000<<27,	/*  Setting  Np count on DTM to Zero*/
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e000128>>5), 0x0e000128<<27,		/* Reload & enable ADC1 on STM */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,   /* bye-bye glitch on DTM */
	CL_AP_BUS | (0x0e820000>>5), 0x0e820000<<27,		/* disable CTC on ADC 1-1 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e920000>>5), 0x0e920000<<27,		/* disable CTC on ADC 1-2 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0ea20000>>5), 0x0ea20000<<27,		/* disable CTC on ADC 2-1 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0eb20000>>5), 0x0eb20000<<27,		/* disable CTC on ADC 2-2 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0ec20000>>5), 0x0ec20000<<27,		/* disable CTC on ADC 3-1 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0ed20000>>5), 0x0ed20000<<27,		/* disable CTC on ADC 3-2 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0ee20000>>5), 0x0ee20000<<27,		/* disable CTC on ADC 4-1 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0ef20000>>5), 0x0ef20000<<27,		/* disable CTC on ADC 4-2 */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,

	/* for reason unclear, must wait > 40 ms (using 80 ms) so that ADC clears the
	   ADC Overflow properly */
 	CL_DELAY | (CNT40_MSEC>>5), CNT40_MSEC<<27,
 	CL_DELAY | (CNT40_MSEC>>5), CNT40_MSEC<<27,
	CL_AP_BUS | (0x0e000028>>5), 0x0e000028<<27,		/* enable ADC1 on STM */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e820010>>5), 0x0e820010<<27,		/* enable CTC on ADC */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
 	CL_CTC | (AP_ITRP_MIN_DELAY_CNT>>5), AP_ITRP_MIN_DELAY_CNT<<27,	/*  MAKE A CTC */
	CL_AP_BUS | (0x0e820000>>5), 0x0e820000<<27,		/* adc */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	/* */
 	CL_AP_BUS | (0x0e180000>>5), 0x0e180000<<27,	/*  Setting Np count on DTM to Zero */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
 	CL_AP_BUS | (0x0e1A0000>>5), 0x0e1A0000<<27,	/*  Setting  Np count on DTM to Zero*/
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27,
	CL_AP_BUS | (0x0e000128>>5), 0x0e000128<<27,		/* Reload & enable ADC1 on STM */
 	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5), AP_HW_MIN_DELAY_CNT<<27
};
/*	HALTOP, 0x00000000 }; /* as it used to be, replaced with }; */

static int numSafeStatCodes = sizeof(pSafeStateCodes)/(sizeof(long)*2);

static long pTurnOffSshaCodes[] = {
	CL_DELAY | (AP_HW_MIN_DELAY_CNT>>5),
	AP_HW_MIN_DELAY_CNT<<27,
	APWRT | 0x6805f8,
	0x0,
	CL_DELAY | ((AP_MIN_DELAY_CNT & 0x3fffffe0) >> 5),
	(AP_MIN_DELAY_CNT & 0x1f) << 27,
	APWRT | 0x681000,
	0x20000000,
	CL_DELAY | ((AP_MIN_DELAY_CNT & 0x3fffffe0) >> 5),
	(AP_MIN_DELAY_CNT & 0x1f) << 27,
	CL_DELAY | (AP_ITRP_MIN_DELAY_CNT>>5),
	AP_ITRP_MIN_DELAY_CNT<<27,
	APWRT | 0x680600,
	0x10000000,						/* "shim DAC" value goes here */
	CL_DELAY | ((AP_MIN_DELAY_CNT & 0x3fffffe0) >> 5),
	(AP_MIN_DELAY_CNT & 0x1f) << 27,
	APWRT | 0x681000,
	0x20000000,
	CL_DELAY | ((AP_MIN_DELAY_CNT & 0x3fffffe0) >> 5),
	(AP_MIN_DELAY_CNT & 0x1f) << 27,
	CL_DELAY | (AP_ITRP_MIN_DELAY_CNT>>5),
	AP_ITRP_MIN_DELAY_CNT<<27,
};

static int numTurnOffSshaCodes = sizeof(pTurnOffSshaCodes)/(sizeof(long)*2);

static long pHaltOpCodes[] = {
	HALTOP, 0x00000000
};

static int numHaltOpCodes = sizeof(pHaltOpCodes)/(sizeof(long)*2);

startPhandler(int priority, int taskoptions, int stacksize)
{
   int pHandler();

   if (taskNameToId("tPHandlr") == ERROR)
   {
     pHandlerPriority = priority;
     pHandlerTid = taskSpawn("tPHandlr",priority,taskoptions,stacksize,pHandler,
		   pMsgesToPHandlr,2, 3,4,5,6,7,8,9,10);
   
     pHandlerWdTimeout = wdCreate();
     pHandlerTimeout = 0;
   }

}

killPhandler()
{
   int tid;
   if ((tid = taskNameToId("tPHandlr")) != ERROR)
	taskDelete(tid);
}

pHandlerWdISR()
{
   pHandlerTimeout = 1;
   wdCancel(pHandlerWdTimeout);
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WDISR,NULL,NULL);
#endif
}

/*************************************************************
*
*  probHandler - Wait for Messages that indecate some problem
*    with the system. Then perform appropriate recovery.
*
*					Author Greg Brissey 12-7-94
*/
pHandler(MSG_Q_ID msges)
{
   EXCEPTION_MSGE msge;
   int *val;
   int bytes;
   void recovery(EXCEPTION_MSGE *);

   DPRINT(1,"pHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     markReady(PHANDLER_FLAGBIT);
     memset( &msge, 0, sizeof( EXCEPTION_MSGE ) );
     bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, 
			  sizeof( EXCEPTION_MSGE ), WAIT_FOREVER);
     markBusy(PHANDLER_FLAGBIT);
     DPRINT3(1,"pHandler: recv: %d bytes, Exception Type: %d, Event: %d \n",
			bytes, msge.exceptionType, msge.reportEvent);

      
     recovery( &msge );
   } 
}

enableInterrupts()
{

   stmItrpEnable(pTheStmObject, (RTZ_ITRP_MASK | RPNZ_ITRP_MASK | 
			  MAX_SUM_ITRP_MASK | APBUS_ITRP_MASK));

   fifoItrpEnable(pTheFifoObject, FSTOPPED_I | FSTRTEMPTY_I | FSTRTHALT_I |
			   NETBL_I | FORP_I | TAGFNOTEMPTY_I | PFAMFULL_I |
			   SW1_I | SW2_I | SW3_I | SW4_I );

   autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS); 
}

/*********************************************************************
*
* recovery, based on the cmd code decides what recovery action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void recovery(EXCEPTION_MSGE *msge)
{
   char *token;
   int len;
   int cmd;
   int i,nMsg2Read;
   int bytes;
   EXCEPTION_MSGE discardMsge;
   extern int systemConRestart();
   static void resetSystem();
   static void resetNameBufs();

   /* reset STM & ADC Intrp MsgQ to standard  msgQ, encase they have been switched  */
   stmRestoreMsgQ(pTheStmObject);

   switch( msge->exceptionType )
   {
      case PANIC:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_PANIC,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Panic Error: %d", msge->reportEvent);
		break;

      case WARNING_MSG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WARNMSG,NULL,NULL);
#endif
  	        DPRINT2(0,"WARNING: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Warning Message: %d", msge->reportEvent); */
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
		break;

      case SOFT_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_SOFTERROR,NULL,NULL);
#endif
		update_acqstate( ACQ_IDLE );

  	        DPRINT2(0,"SOFT_ERROR: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Soft Error: %d", msge->reportEvent); */
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
		break;

      case EXP_ABORTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPABORTED,NULL,NULL);
#endif
		stmItrpDisable(pTheStmObject, STM_ALLITRPS);
  	        DPRINT(0,"Exp. Aborted");

      case HARD_ERROR:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT1(0,"HARD_ERROR: doneCode: %d, errorCode: %d\n", msge->reportEvent);

		AbortExp(msge);

		fifoCloseLog(pTheFifoObject);
  	        DPRINT(0,"Done");
		break;
		

      case INTERACTIVE_ABORT:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT1(0,"INTERACTIVE_ABORT: doneCode: %d, errorCode: %d\n", msge->reportEvent);

		AbortExp(msge);

		fifoCloseLog(pTheFifoObject);
  	        DPRINT(0,"Done");
		break;


      case EXP_HALTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPHALTED,NULL,NULL);
#endif
		stmItrpDisable(pTheStmObject, STM_ALLITRPS);

                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

    		errLogRet(LOGIT,debugInfo,
       		"phandler: Exp. Halted: %d", msge->reportEvent);
		/* 
                   Based on the premise the user now has achieve the S/N
	           or whatever from data that has already been obtained
                   therefore there is no need to wait for any data to 
		   be acquired. I.E. Halt Experiment with extreme prejudice!
                */

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

		update_acqstate( ACQ_IDLE );

  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
		stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);
   		DPRINT(0,"EXP_HALTED: lower priority below upLinker\n");
                taskPrioritySet(pHandlerTid,(UPLINKER_TASK_PRIORITY+1));  
   	        taskPrioritySet(pHandlerTid,pHandlerPriority);  

  	        DPRINT(0,"wait4DowninkerReady");
		wait4DownLinkerReady();	/* let downlinker become ready, then delete any left over buffers */

                resetNameBufs();   /* free all named buffers */

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

	        clrDwnLkAbort();

                wait4SystemReady(); /* pend till all activities are complete */

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

		update_acqstate( ACQ_IDLE );
	        getstatblock();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

		break;


      case STOP_CMPLT:
                /* the monitor set the SA_Criteria value,
		   when the upLinker obtained a FID that meet this
		   Criteria the upLinker sent this msge here and
		   then blocked itself
                  Our job is to
	          1. Foward the Stopped msge to Host
	          2. reset hardware
                  3. put hardware into a safe state
		  4. call stmSA, clears upLinker msgQ, send SA msge
	          5. reset SA_Criteria back to Zero
		  6. give the semaphore to restart upLinker
		  7. reset update task
		  8. reset parser task
	          9. Free the buffers
	         10. Re-enable Interrupts 
		*/

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_STOPCMPLT,NULL,NULL);
#endif
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

    		errLogRet(LOGIT,debugInfo,
       		"phandler: Exp. Stopped: %d", msge->reportEvent);

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );  

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);

 		clearIntrpMsgQ(pTheStmObject);
                semGive(pSemSAStop);   /* release the UpLinker */
		stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);
  	        /* DPRINT(0,"stmSA"); stmSA(pTheStmObject); */
   		DPRINT(0,"STOP_CMPLT: lower priority below upLinker\n");
                taskPrioritySet(pHandlerTid,(UPLINKER_TASK_PRIORITY+1));  
   	        taskPrioritySet(pHandlerTid,pHandlerPriority);  

		SA_Criteria = 0;
		SA_Mod = 0L;

  	        DPRINT(0,"wait4DowninkerReady");
		wait4DownLinkerReady();	/* let downlinker become ready, then delete any left over buffers */

                resetNameBufs();   /* free all named buffers */

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

	        clrDwnLkAbort();

                wait4SystemReady(); /* pend till all activities are complete */

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

		update_acqstate( ACQ_IDLE );
	        getstatblock();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

		break;

      case LOST_CONN:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_LOSTCONN,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Host Closed Connection to Console: %d", msge->reportEvent);
		update_acqstate( ACQ_IDLE );
		stmItrpDisable(pTheStmObject, STM_ALLITRPS);
                storeConsoleDebug( SYSTEM_ABORT );
                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );
                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */
		stmReset(pTheStmObject);
		taskDelay(sysClkRateGet()/3);
                resetNameBufs();   /* free all named buffers */
		enableInterrupts();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );
     		taskSpawn("tRestart",50,0,2048,systemConRestart,NULL,
				2,3,4,5,6,7,8,9,10);
	        clrDwnLkAbort();

	        break;

      case ALLOC_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_ALLOCERROR,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Memory Allocation Error: %d", msge->reportEvent);

		update_acqstate( ACQ_IDLE );

                statMsg.Status = HARD_ERROR; /* HARD_ERROR */
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_URGENT);
                

		/* report HARD_ERROR to Recvproc */
  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			HARD_ERROR,msge->reportEvent);
		stmHaltCode(pTheStmObject,(int) HARD_ERROR, 
				(int) msge->reportEvent);

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

	      
#ifdef XXXX
                statMsg.Status = WARNING_MSG;
                statMsg.Event = WARNINGS + MEM_ALLOC_ERR; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
#endif

                resetNameBufs();   /* free all named buffers */

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

	        clrDwnLkAbort();

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

	        getstatblock();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

		break;
      
      case WATCHDOG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WATCHDOG,NULL,NULL);
#endif
		/* Giv'm a bone */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Watch Dog: %d", msge->reportEvent);
		break;

      default:
		/* Who Cares */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Invalid Exception Type: %d, Event: %d", 
			msge->exceptionType, msge->reportEvent);
		break;
   }
/*
   nMsg2Read = msgQNumMsgs(pMsgesToPHandlr);
   DPRINT1(-1,"Message in Q: %d\n",nMsg2Read);
*/
   while ( (bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, sizeof( EXCEPTION_MSGE ), NO_WAIT)) != ERROR )
   {
     DPRINT1(0,"Read %d bytes from Phandler msgQ, (Bogus Errors at this point)\n",bytes);
   }
}

static
void AbortExp(EXCEPTION_MSGE *msge)
{
   /* donecode = EXP_ABORTED or  HARD_ERROR plus errorCode(FOO,etc..) */
   /* need to get this msge up to expproc as soon as possible since the
      receipt of this msge cause expproc to send the SIGUSR2 (abort cmd)
      to Sendproc, without this msge all the acodes will be sent down.
   */
   /* EXP_ABORT or HARD_ERROR */
   statMsg.Status = (msge->exceptionType != INTERACTIVE_ABORT) ? msge->exceptionType : EXP_ABORTED; 
   statMsg.Event = msge->reportEvent; /* Error Code */
   if (msge->reportEvent == (HDWAREERROR + STMERROR))
      reportNpErr(pTheStmObject->npOvrRun, 1 + pTheStmObject->activeIndex);
   msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
				WAIT_FOREVER, MSG_PRI_URGENT);

   DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
		msge->exceptionType,msge->reportEvent);
   stmHaltCode(pTheStmObject,(int) statMsg.Status, 
		(int) msge->reportEvent);


   if ( msge->exceptionType  == EXP_ABORTED)
        storeConsoleDebug( SYSTEM_ABORT );
   /* reprogram HSlines, ap registers to safe state */
   reset2SafeState( msge->exceptionType );
   resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */
   if (msge->reportEvent == (SFTERROR + MAXCT))
   {
       /* if maxsum error reset the stm */
       /* obtain lock on stm, i.e. not transfer data prior to reseting stm */
       stmTake(pTheStmObject);
       stmReset(pTheStmObject);
       taskDelay(sysClkRateGet()/3);
       /* only after wait for the stm to functional again do we release the lock */
       stmGive(pTheStmObject);
   }

   DPRINT(0,"wait4DowninkerReady");
   wait4DownLinkerReady();	/* let downlinker become ready, then delete any left over buffers */

   resetNameBufs();   /* free all named buffers */

   DPRINT(0,"wait4SystemReady");
   wait4SystemReady(); /* pend till all activities are complete */
   DPRINT(0,"SystemReady");
   clrDwnLkAbort();

   DPRINT(0,"enableInterrupts");
   enableInterrupts();

   /* now inform Expproc System is Ready */
   statMsg.Status = SYSTEM_READY; /* Console Ready  */
   statMsg.Event = 0; /* Error Code */
   msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
	WAIT_FOREVER, MSG_PRI_NORMAL);
   update_acqstate( ACQ_IDLE );
   getstatblock();
   
   if (pTheTuneObject != NULL)
      semGive( pTheTuneObject->pSemAccessFIFO );

   return;
}


static
void resetSystem()
{
   DPRINT(0,"AParserAA");
   AParserAA();

   /* PARALLEL_CHANS */
   freeSorter();

   DPRINT(0,"AupdtAA");
   AupdtAA();
   DPRINT(0,"ShandlerAA");
   ShandlerAA();
   DPRINT(0,"fifoStufferAA");
   fifoStufferAA(pTheFifoObject);

   stmAdcOvldClear(pTheStmObject);
   /* stmReset(pTheStmObject); */
   DPRINT(0,"Disable ADC Intrps");
   adcItrpDisable(pTheAdcObject,ADC_ALLITRPS);
   /* adcReset(pTheAdcObject); */

   /* reset fifo Object not to use parallel channels */
   fifoClearPChanId(pTheFifoObject);

   /* Reset Safe State again after tasks restarted */
   /*reset2SafeState();*/  /* reprogram HSlines, ap registers to safe state */
   /* call to reset2SafeState removed; in each case where resetSystem */
   /* is called, reset2SafeState is called immediately previously.    */

/* it maybe safe here, but we have to flip flop priorities with the Stuffer */
   DPRINT2(0,"resetSystem: tid: 0x%lx, priority: %d\n",pHandlerTid,pHandlerPriority);
   taskPrioritySet(pHandlerTid,(FIFO_STUFFER_PRIORITY+1));  
   setlksample();
   set2khz();
   taskPrioritySet(pHandlerTid,pHandlerPriority);  
   return;
}

/*
   WARNING: Not to be used by any other tasks than phandler
     1. Stop Parser
     2. Stop DownLinker
     3. Stuff Fifo with safe states and run
     4. Give-n-Take Serial Port devices Mutex to allow serial cmd to finish
     5. Free Name buffers
     6. Raise Priority of DownLink to above phandler and resume task if suspend & wait, lower priority
     7. Raise Priority of Parser to above phandler and resume task if suspend & wait, lower priority
     Last three steps allows for an orderly and efficient clean up of resources (name buffers,etc.)
*/

/* reprogram HSlines, ap registers to safe state */

static void
reset2SafeState( int why )  /* argument is the type of exception */
{
   int pTmpId, TmpPrior;
   int tuneactive = 0;

   DPRINT(1,"reset2SafeState");
   DPRINT(1,"stopAPint");
   pcsAbortSort();     /* parallel channel sort to stop and return to APint() */
   abortAPint();   /* like stopAPint() but also cause the Parser to suspend itself */
   /* stopAPint(); */
   DPRINT(1,"stop downLinker");
   setDwnLkAbort();   /* like stop downLinker and set to dump further download to bit bucket */
   DPRINT(1,"reset2SafeState");
   DPRINT(0,"fifoSetNoStuff\n");
   fifoSetNoStuff(pTheFifoObject);    /* stop the stuffer in it's tracks */
   DPRINT(0,"Disable STM Intrps");
   stmItrpDisable(pTheStmObject, STM_ALLITRPS);
   /* reset fifo Object not to use parallel channels */
   fifoClearPChanId(pTheFifoObject);
   DPRINT(0,"fifoReset");

   /* Keep the MTS gradient amp disabled whenever possible! */
   gpaTuneSet(pTheAutoObject, SET_GPAENABLE, GPA_DISABLE_DELAY);

   /* if an interactive abort do not reset the apbus, thus preventing the 
      all amplifier from going to Pulse Mode, thus amps that are in CW 
      will stay that way. This is/was a lockdisplay/shimming issue.
      Where exiting lock display set all the amps to Pulse Mode or blanked!, 
      then one shimmed and obtain a certain lock level. After su command 
      then amps would go back to CW thus injecting noise and lowing the
      lock level, If use wht directly to shimming they could neer re-attain 
      the lock level they had before.  But when enter and exit lock display 
      the level would jump back up. Thus causing no end of
      confusion....
      Thus for INTERACTIVE_ABORT the reset apbus bit is left out when reseting the FIFO.

      Oooops, but wait, we found an exception (of course), when exiting qtune the 
      reset of the apbus reset all the tune relays back to noraml, now of course 
      this does not happen thus leaving the console in tuning mode. Thus we added the 
      following test isTuneActive() and if it is then a full reset including the apbus
      is done.
   */
   tuneactive = isTuneActive();
   /* DPRINT1(-3,"Is Tune active: %d\n",tuneactive); */
   if ((why != INTERACTIVE_ABORT) || (tuneactive == 1))
      fifoReset(pTheFifoObject, RESETFIFOBRD);  /* this now also clear the Fifo Buffer & restart the Stuffer */
   else
      fifoReset(pTheFifoObject, RESETFIFO | RESETSTATEMACH | RESETTAGFIFO | RESETAPRDBKFIFO ); /* no apbus reset */

   /* set initial value of parallel channel free buffer pointer to null */
   clearParallelFreeBufs();

   DPRINT(0,"Stuff FIFO DIRECTLY with Safe State & Run fifo");
   fifoLoadHsl(pTheFifoObject, STD_HS_LINES, pTheFifoObject->SafeHSLines);
   fifoLoadHsl(pTheFifoObject, EXT_HS_LINES, pTheFifoObject->SafeHSLinesExt);
   /* Each SafeState Code is actually two long words */
   if (numSafeStatCodes > 0)
   {
      /*   WARNING: Does not pend for FF Almost Full, BEWARE !! */
      /*   Should only be used in phandler's reset2SafeState() */
      fifoStuffCode(pTheFifoObject, pSafeStateCodes, numSafeStatCodes);
      fifoStuffCode(pTheFifoObject, pTurnOffSshaCodes, numTurnOffSshaCodes);
      fifoStuffCode(pTheFifoObject, pHaltOpCodes, numHaltOpCodes);
	DPRINT(1,"Ready to start FIFO in reset to safe state\n" );
      fifoStart(pTheFifoObject);
      /* fifoWait4Stop(pTheFifoObject);  /* use BusyWait don't what any other lower priority tasks to run ! */
      fifoBusyWait4Stop(pTheFifoObject);  /* use BusyWait don't what any other lower priority tasks to run ! */
	DPRINT(1,"FIFO wait for stop completes in reset to safe state\n" );
   }
   activate_ssha();
   DPRINT(0,"getnGiveShimMutex\n");  
   /* allow any pending serial Shim commands to finish */
   getnGiveShimMutex();
   DPRINT(0,"getnGiveVTMutex\n");
   /* allow any pending serial VT commands to finish */
   getnGiveVTMutex();

/* Now Bump Priority of DownLiner so that it can Dump
   the remaining Data that SendProc wants to send it */

/* If we lost the connection to the host computer, the downlinker is
   is not going to get any more data; nor should this task (problem
   handler) wait for the downlinker to become ready.    April 1997  */

   DPRINT(0,"dlbFreeAllNotLoading Dyn");
   dlbFreeAllNotLoading(pDlbDynBufs);
   DPRINT(0,"dlbFreeAllNotLoading Fix");
   dlbFreeAllNotLoading(pDlbFixBufs);
   if (why != LOST_CONN)
   {
      pTmpId = taskNameToId("tDownLink");
      taskPriorityGet(pTmpId,&TmpPrior);
      taskPrioritySet(pTmpId,(pHandlerPriority-1));  

      /* downLinker has now A. ran and is suspended, 
			    B. Done and in READY state or
	 		    C. still waiting in a read of a socket
      */

      /* start wdog timeout of 20 seconds */ 
      wdStart(pHandlerWdTimeout,sysClkRateGet() * 20, pHandlerWdISR, 0);

      /* if it is busy and not suspend then wait till 
      /* it is Ready or Suspended, or Time-Out */
      while (downLinkerIsActive() &&  !taskIsSuspended(pTmpId) && !pHandlerTimeout)
            taskDelay(1);  /* wait one tick , 16msec */

      wdCancel(pHandlerWdTimeout);

      if (taskIsSuspended(pTmpId))
      {
        taskResume(pTmpId);
        wait4DownLinkerReady();
      }
      taskPrioritySet(pTmpId,TmpPrior);
      DPRINT(0,"downlinker resynchronized\n");
   }
   DPRINT(0,"dlbFreeAll Dyn");
   dlbFreeAll(pDlbDynBufs);
   DPRINT(0,"dlbFreeAll Fix");
   dlbFreeAll(pDlbFixBufs);

   /* 
     Now Bump Priority of Parser so that it can Finish Up  
     We don't have to worry as much about the state of the parser
     since it will completely restarted very soon. 
   */

   pTmpId = taskNameToId("tParser");
   taskPriorityGet(pTmpId,&TmpPrior);
   taskPrioritySet(pTmpId,(pHandlerPriority-1));  
   if (taskIsSuspended(pTmpId))
   {
     taskResume(pTmpId);
     wait4ParserReady();
   }
   taskPrioritySet(pTmpId,TmpPrior);
   resetAPint();
   DPRINT(0,"A-code parser resynchronized\n");

   /* Parser might of made a buffer ready to stuff, so better clear it just incase */
   if (why != INTERACTIVE_ABORT)
      fifoReset(pTheFifoObject, RESETFIFOBRD);  /* this now also clear the Fifo Buffer & restart the Stuffer */
   else
      fifoReset(pTheFifoObject, RESETFIFO | RESETSTATEMACH | RESETTAGFIFO | RESETAPRDBKFIFO ); /* no apbus reset */
   
   return;
}

/*----------------------------------------------------------------------*/
/* resetNameBufs							*/
/*	Removes named buffers.  Lowers priority in case downlinker is	*/
/*	pending on new buffers (e.g. interleaving). Waits then removes	*/
/*	some more.							*/
/*----------------------------------------------------------------------*/
static
void resetNameBufs()   /* free all named buffers */
{
   int retrys;
   int callTaskId,DwnLkrTaskId;
   int callTaskPrior;

   /* Since reset2SafeState should have left the downLinker in a ready
      state we now longer need change priority/etc. here to get it to clean
      up.     8/6/97
   */
#ifdef XXX
   callTaskId = taskIdSelf();
   DwnLkrTaskId = taskNameToId("tDownLink");
   taskPriorityGet(callTaskId,&callTaskPrior);
#endif


   retrys=0;

   /* start wdog timeout of 7 seconds */ 
   pHandlerTimeout = 0;
   wdStart(pHandlerWdTimeout,sysClkRateGet() * 7, pHandlerWdISR, 0);

   /* free buffers until all are free or watch-dog timeout has occurred */
   while ( ((dlbUsedBufs(pDlbDynBufs) > 0) || (dlbUsedBufs(pDlbFixBufs) > 0)) &&
		(!pHandlerTimeout) )
   {
        retrys++;
   	DPRINT(0,"dlbFreeAll Dyn");
   	dlbFreeAll(pDlbDynBufs);
   	DPRINT(0,"dlbFreeAll Fix");
   	dlbFreeAll(pDlbFixBufs);
	Tdelay(1);
	DPRINT1(0,"resetNameBufs: retrys: %d\n",retrys);
#ifdef XXX
        /* Lower priority to allow downlinker in */
	if (callTaskPrior <= DOWNLINKER_TASK_PRIORITY)
           taskPrioritySet(callTaskId,(DOWNLINKER_TASK_PRIORITY+1));  
	Tdelay(10);

	DPRINT1(-1,"resetNameBufs: retrys: %d\n",retrys);
        if (retrys > 7)
        {
	   if (taskIsSuspended(DwnLkrTaskId))
	   {
	      DPRINT(-1,"resetNameBufs: clearCurrentNameBuffer()\n");
  	      clearCurrentNameBuffer();
           }
        }
#endif
   }
   wdCancel(pHandlerWdTimeout);
   /* taskPrioritySet(callTaskId,callTaskPrior);   */

  /*    resumeDownLink(); */
}

/* programs safe HS line, turns off WFG, etc, set lock filters,etc.. */
/* WARNING this function will changed the calling task priority if it is
   lower than the Stuffer, to greater than the stuffer while putting fifo words
   into the fifo buffer 
*/
void set2ExpEndState()  /* reprogram HSlines, ap registers to safe state, Normal end */
{
   int callTaskId;
   int callTaskPrior;
   DPRINT(0,"set2ExpEndState");

   /* Keep the MTS gradient amp disabled whenever possible! */
   gpaTuneSet(pTheAutoObject, SET_GPAENABLE, GPA_DISABLE_DELAY);

   DPRINT(0,"Stuff FIFO through Normal channels with Safe State & Run fifo");
   fifoLoadHsl(pTheFifoObject, STD_HS_LINES, pTheFifoObject->SafeHSLines);
   fifoLoadHsl(pTheFifoObject, EXT_HS_LINES, pTheFifoObject->SafeHSLinesExt);
   /* Each SafeState Code is actually two long words */
   
   fifoResetStufferFlagNSem(pTheFifoObject);
   
   callTaskId = taskIdSelf();
   taskPriorityGet(callTaskId,&callTaskPrior);
   /* Lower priority to allow stuffer in, if needed  */
   /* if priority <= to that of the stuffer then lower the priority of this task */
   /* thus allowing the stuffer to stuff the fifo */
   DPRINT2(0,"set2ExpEndState: tid: 0x%lx, priority: %d\n",callTaskId,callTaskPrior);
   if (callTaskPrior <= FIFO_STUFFER_PRIORITY)
   {
      DPRINT(0,"Set priority lower than stuffer\n");
      taskPrioritySet(callTaskId,(FIFO_STUFFER_PRIORITY+1));  
   }
   if (numSafeStatCodes > 0)
   {
      fifoStuffIt(pTheFifoObject, pSafeStateCodes, numSafeStatCodes);/* into Fifo Buffer */
      fifoStuffCode(pTheFifoObject, pTurnOffSshaCodes, numTurnOffSshaCodes);
      fifoStuffCode(pTheFifoObject, pHaltOpCodes, numHaltOpCodes);
	DPRINT(1,"Ready to start FIFO in set to exp end state\n" );
      fifoStart(pTheFifoObject);
      fifoWait4Stop(pTheFifoObject);
   }
   setlksample();
   set2khz();
   activate_ssha();
   /* reset priority back if changed */
   if (callTaskPrior <= FIFO_STUFFER_PRIORITY)
   {
      DPRINT(0,"Set priority Back \n");
      taskPrioritySet(callTaskId,callTaskPrior);  
   }
   return;
}
