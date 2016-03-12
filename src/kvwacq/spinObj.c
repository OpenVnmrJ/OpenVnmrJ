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
#include <stdio.h>
#include <semLib.h>
#include <msgQLib.h>
#include <taskLib.h>

#include "commondefs.h"
#include "instrWvDefines.h"
#include "taskPrior.h"
#include "serialDevice.h"
#include "hostAcqStructs.h"
#include "lock_interface.h"
#include "logMsgLib.h"
#include "errorcodes.h"
#include "expDoneCodes.h"
#include "autoObj.h"
#include "fifoObj.h"
#include "spinObj.h"
#include "acqcmds.h"
#include "hardware.h"

/* This is the first part needed to down load a Srec into the spinner */
/* char	Srecord[]={
/* #include "mon.srec"
/* };
/*  */

extern MSG_Q_ID		pMsgesToAupdt;
extern STATUS_BLOCK	currentStatBlock;     /* Acqstat-like status block */
extern AUTO_ID		pTheAutoObject;       /* Automation Object */
extern FIFO_ID		pTheFifoObject;
extern int		get_acqstate();       /*  in /sysvwacq/monitor.c  */
extern int		sampleHasChanged;

#define CR 13
#define CMD 0
#define TIMEOUT 98
#define CMDTIMEOUT -10000

#define	SPIN_NONE	0	/* No Spin controller */
#define	LSDV_VTBIT1	0x800	/* 11 */
#define	LSDV_VTBIT2	0x1000	/* 12 */

static int tSpinId;

/* The spinInterlock has 3 values, 
   0 - means that in Vnmr in='n' or the spinner has already gone 
	out of regulation.
   1 - means that in Vnmr in='y' and the WAIT4Spin has not 
       completed in its wait for the spinner to go into regulation.
   2 - means that WAIT4Spin has complete spinner regulation and it is OK
       for statmonitor() in monitor.c to check spinner for whether it
       has gone out of regulation.
   This was done to follow VT
*/
     
/*
  spinErrorType has two values,
  HARD_ERROR - means Spin out of Regulation is an Error 
  WARNING_MSG - means Spin out of Regulation is a Warning
*/
#define VT_SPONSTAT_BIT 0x8
#define VT_WAITING4_REG 0x10
#define VT_IS_REGULATED 0x20
#define VT_TEMP_IS_SET  0x40



SPIN_ID pTheSpinObject = NULL;
static int SpinMASThreshold = 80;	/* Above this spin speed switch  */
					/* to MAS solids spinner control */
static int SpinDeltaReg = 1;		/* max speed (Hz) error */
					/* allowed for spinner  */

/**************************************************************
*
*  spin
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

SPIN_ID  spinCreate()
{
register SPIN_OBJ *pSpinObj;
int		i, rstat;
SPIN_CMD	cmd;
int		spinReg(SPIN_ID pSpinId);
int		spinStat(SPIN_ID pSpinId, char cmd);


   /* ------- malloc space for SPIN Object --------- */
   if ( (pSpinObj = (SPIN_OBJ *) malloc( sizeof(SPIN_OBJ)) ) == NULL )
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Space:");
      return(NULL);
   }
  
   DPRINT1(0,"spinCreate: Created Spin Object: 0x%lx\n",pSpinObj);

   /* zero out structure so we don't free something by mistake */
   memset(pSpinObj,0,sizeof(SPIN_OBJ));

   pSpinObj->pSID = NULL;		/* SCCS ID */
   pSpinObj->spinSetSpeed = -2;		/* Requested Speed */
   pSpinObj->spinTruSpeed = -2;		/* Actual Speed */
   pSpinObj->spinModeMask |=  VT_WAITING4_REG;
   pSpinObj->spinPort = -1;		/* No serial port yet */
   pSpinObj->spinInterlock = 0;		/* in='y' or 'n' 0=no 1=enable 2=ready*/
   pSpinObj->spinType = 2;		/* 1 == simple, 2 = Auto */
   pSpinObj->spinErrorType = HARD_ERROR;/* HARD_ERROR (15) == Error, */
					/* WARNING_MSG(14) == Warning */
   pSpinObj->spinError = 0;
   pSpinObj->pSpinMutex = NULL;		/* Spin serial port mutual exclusion */
   pSpinObj->spinUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE; /* 3 sec */
   pSpinObj->SPINIDSTR[0] = 0;

   pTheSpinObject = pSpinObj;	/* set static global for Spin routines */

   pSpinObj->spinPort = initSerialPort( 1 );
   setSerialTimeout(pSpinObj->spinPort,25);	/* set timeout to 1/4 sec */
   DPRINT1(0,"spinPort = %d\n",pSpinObj->spinPort);

   clearport(pSpinObj->spinPort);
   pputchr(pSpinObj->spinPort,'T');
   if ( cmdecho(pSpinObj->spinPort, 0) )
   {  DPRINT(0, "No auto-spinner attached\n");
      pSpinObj->spinPort = -1;	/* no spinner attached */
      pSpinObj->spinType = 1;		/* 1 == simple, 2 = Auto */
   }

   pSpinObj->pSpinMutex =
	  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
   if (pSpinObj->pSpinMutex == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Mutex:");
      free(pSpinObj);
      return(NULL);
   }

   /* Spin Regulated Semaphore */
   pSpinObj->pSpinRegSem =  semBCreate(SEM_Q_PRIORITY , SEM_EMPTY);
   if (pSpinObj->pSpinRegSem == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Reg. Sem:");
      spinDelete(pSpinObj);
      return(NULL);
   }

   pSpinObj->pSpinMsgQ = msgQCreate(25,sizeof(SPIN_CMD),MSG_Q_FIFO);
   if (pSpinObj->pSpinMsgQ == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"spinCreate: Could not Allocate Message Q:");
      spinDelete(pSpinObj);
      return(NULL);
   }

    if (pSpinObj->spinPort >= 0)
    {  rstat = spinStat(pSpinObj, 'X');
       DPRINT1(0,"Spinner X=%d\n",rstat);
       if (rstat > 0)
       {  pSpinObj->spinType = 3;
       }
   }
/*          pputchr(pSpinObj->spinPort,'L');
/*          DPRINT1(-1,"DOWNLOADING %d bytes",sizeof(Srecord));
/*          write(pSpinObj->spinPort,Srecord,sizeof(Srecord)); */
/*          for (i=0; i<sizeof(Srecord); i++)
/*          {  pputchr(pSpinObj->spinPort,Srecord[i]);
/*             DPRINT1(5,"%c ",Srecord[i]); */
/*             if ( ! (i%16) )
/*	        taskDelay(1);
/*                DPRINT(-1,"."); */
/*          }
/*          DPRINT1(-1,"DOWNLOAD DONE %d byte",sizeof(Srecord));
/* */

   /* put spinner into decimal communication */
   clearport(pSpinObj->spinPort);
   pputchr(pSpinObj->spinPort,'N');
   pputchr(pSpinObj->spinPort,'0');
   pputchr(pSpinObj->spinPort,'7');
   pputchr(pSpinObj->spinPort,13);
   pputchr(pSpinObj->spinPort,'W');
   pputchr(pSpinObj->spinPort,'1');
   pputchr(pSpinObj->spinPort,13);

   /* Spawn spin speed chk Task */
   tSpinId = taskSpawn("tSpin", VT_TASK_PRIORITY, 
				STD_TASKOPTIONS,
                		CON_STACKSIZE, 
				spinReg,
				(int) pSpinObj,
				ARG2, ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
   if ( tSpinId == ERROR)
   {
      errLogSysRet(LOGIT,debugInfo, "spinCreate: could not spawn Spin Control");
      spinDelete(pSpinObj);
      return(NULL);
   }
   DPRINT1(0,"Spin:IDSTR='%s'\n",pSpinObj->SPINIDSTR);
   return(pSpinObj);
}

/**************************************************************
*
*  spinDelete - Deletes Spin Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 12/2/96
*/
int spinDelete(SPIN_ID pSpinId)
{
   int tid;

   if (pSpinId == NULL)
     return(ERROR);

   if ((tid = taskNameToId("tSpin")) != ERROR)
	taskDelete(tid);

   if (pSpinId->spinPort > 0)
   {
      close(pSpinId->spinPort);
      pSpinId->spinPort = 0;
   }

   if (pSpinId->pSpinMutex)
   {
       semDelete(pSpinId->pSpinMutex);
       pSpinId->pSpinMutex = NULL;
   }
   if (pSpinId->pSpinRegSem)
   {
       semDelete(pSpinId->pSpinRegSem);
       pSpinId->pSpinRegSem = NULL;
   }

   if (pSpinId->pSpinMsgQ)
   {
       msgQDelete(pSpinId->pSpinMsgQ);
       pSpinId->pSpinMsgQ = NULL;
   }
   free(pSpinId);
   pSpinId = NULL;

   return(0);
}

/* gets and give Spin Mutex semaphore, used by phandler to 
   allow Spin commands to finish
*/
getnGiveSpinMutex()
{
   if (pTheSpinObject->pSpinMutex == NULL)
     return;
   semTake(pTheSpinObject->pSpinMutex,WAIT_FOREVER);
   semGive(pTheSpinObject->pSpinMutex);
}

bearoff()
{
char	msg4Aupdt[ 256 ];
int	len;
   sprintf( &msg4Aupdt[ 0 ], 
	"2;%d,%d,1,%d;%d,%d,1,%d;",
	FIX_APREG, 0x0A20, 0x2E,
	FIX_APREG, 0x0A21, 0x00);
   len = strlen( &msg4Aupdt[ 0 ] );
   msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
}

bearon()
{
char	msg4Aupdt[ 256 ];
int	len;
   sprintf( &msg4Aupdt[ 0 ], 
	"2;%d,%d,1,%d;%d,%d,1,%d;",
	FIX_APREG, 0x0A20, 0x2E,
	FIX_APREG, 0x0A21, 0x01);
   len = strlen( &msg4Aupdt[ 0 ] );
   msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
}


/*****************************************************************
*  Spin controller driver                                            
*  spinTalk(pSpinId, command,number):
*	       command is a single character           
*              number is usually the speed       
*/
int spinTalk(SPIN_ID pSpinId,char cmd,int value)
{
int stat;
    DPRINT2(2,"spinTalk: Cmd = %c  Var = %d ", cmd, value);
/*
#ifdef INSTRUMENT
      wvEvent(EVENT_VTMUX_TAKE,NULL,NULL);
#endif
*/

    /* For non-auto-spin only allow BEARINGON and BEARINGOFF */
    if (pSpinId->spinPort == -1)
    {  if (cmd=='B')
       {
          bearon();
          return(0);
       }
       else if (cmd == 'Q')
       {
          bearoff();
          return(0);
       }
       return(-1);
    }

    /* G0 at end of exp, or when acqi closes, confuses old board */
    if ( (cmd == 'G') && (pSpinId->spinType<3) )
	return (0);
    /* Now deal with auto-spin board via serial port */
    if (pSpinId->pSpinMutex != NULL)
    {
      semTake(pSpinId->pSpinMutex, WAIT_FOREVER);
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"spinTalk: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    clearinput(pSpinId->spinPort);
    pputchr(pSpinId->spinPort, cmd);
    cmdecho(pSpinId->spinPort, CMD); 
/* do not check for timeout, if timed out we still want to send the
/* value and the <CR>, just in case the spinner was busy and did not 
/* echo it. If we return here, the spinner is stuck waiting for <CR>
/*  if (cmdecho(pSpinId->spinPort, CMD)) 
/*    {
/*      semGive(pSpinId->pSpinMutex);
/*      return(TIMEOUT);
/*    }
/* */
    switch(cmd)
    {
        case 'A':
        case 'D':		/* Differential   (new only) */
	case 'G':		/* Gradient Relay (new only) */
        case 'M':		/* Threshold      (new only) */
        case 'N':
        case 'P':		/* Proportional   (new only) */
        case 'R':		/* Rate  */
        case 'S':		/* Speed */
        case 'V':
        case 'W':
             echoval(pSpinId->spinPort,value);
             pputchr(pSpinId->spinPort,CR);
             /* printf("spinTalk: Send 'CR':%d Dec----> %c\n\n", CR, CR); */
             break;
        default:
             if (pSpinId->spinType < 3) 
                pputchr(pSpinId->spinPort,CR);
             break;
    }  

    if (cmddone(pSpinId->spinPort, 150) == -1)
    {
       semGive(pSpinId->pSpinMutex);
       return(TIMEOUT);
    }
    semGive(pSpinId->pSpinMutex);
    return(0);
}
 
/********************************************************/
/*  Spin status driver                                  */
/*  spinStat(pTheSponObject, command):			*/
/*            command is a single character            	*/
/*            return the status requested 		*/
/*     T - Return status of processor functions		*/
/*	   8 bits -  bit 0: Bearing air     1=On	*/
/*		         1: Eject air       1=On	*/
/*		         2: Mode 	0=speed,1=rate	*/
/*			 3: Probe type  0=liquid,1=solid*/
/*			 4: bit 4 & 7 special see below */
/*		         5: Slow Eject air  1=On	*/
/*		         6: Not Used 			*/
/*		         7: See below			*/
/*     							*/
/*  bit 4 and 7 are used in combination to indicate     */
/*  3 states:						*/
/*		bit 7 = 0  bit 4 = 0; Regulating	*/
/*			   bit 4 = 1; Speed Zero	*/
/*							*/
/*		bit 7 = 1  (bit 4 meaningless now)	*/
/*			   Non-Regulating		*/
/*     							*/
/*    C - Return the Speed setting (in ASCII)		*/
/*     							*/
/*    F - Return the Rate setting  (in ASCII)		*/
/********************************************************/
int spinStat(SPIN_ID pSpinId, char cmd)
{
unsigned char chr;
int value;
    acqerrno = 0;	/* clear errno */
    DPRINT1(2,"spinStat: CMD = %c \n",cmd);
    if (pSpinId == NULL)
       return(TIMEOUT);

    if (pSpinId->pSpinMutex != NULL)
    {
      semTake(pSpinId->pSpinMutex, WAIT_FOREVER);
    }
    else
    {
      acqerrno = SPINERROR + SPINTIMEOUT; 
      errLogRet(LOGIT,debugInfo,"spinStat: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    /* Checck if autospin present, if so skip */
    if (pSpinId->spinPort == -1)	/* No auto-spin */
    {
       semGive(pSpinId->pSpinMutex);
       if (cmd == 'T')
       {  if (*FF_STATR(pTheFifoObject->fifoBaseAddr) & SPIN_SENSE)
	     return(0x80 | (pSpinId->spinModeMask & BEARON));
          else
             return(0x10 | (pSpinId->spinModeMask & BEARON));
       }
       else
          return(-2);
    }

    /* Do the auto-spin case */
    clearinput(pSpinId->spinPort);
    pputchr(pSpinId->spinPort,(int)cmd);            /* send command */
    DPRINT2(2,"Send by 'spinStat' :    %d Dec\t     ----> %c\n", cmd, cmd);
    cmdecho(pSpinId->spinPort,CMD); 
    value = cmddone(pSpinId->spinPort,50);
    DPRINT1(2,"spinStat: Value = %d\n",value);
    semGive(pSpinId->pSpinMutex);
    return(value);
}
/*----------------------------------------------------------------------*/
/*  spinReg(SPIN_ID pSpinObj)			                        */
/*  1. which spinner is present                                         */
/*  2. set spinner speed
/*  returns 0 or errorcode
/*----------------------------------------------------------------------*/
spinReg(SPIN_ID pSpinId)
/* SPIN_ID pSpinId - Spin Object Pointer */
{
int	bytes;
int	truspin,temp,spinstat;
int	rstat;   /* return status of Serial I/O devices */
SPIN_CMD	cmd;

   FOREVER
   {
      /* Wait for a Message, 
      /* After "spinUpdateRate"-time proceed with standard chores */
      bytes = msgQReceive(pSpinId->pSpinMsgQ, (char*) &cmd,
                          sizeof( SPIN_CMD ), pSpinId->spinUpdateRate);
      DPRINT1(2,"spinReg: Recv: %d bytes\n",bytes);
      /* command then act on it, otherwise just get status */
      if (bytes > 0)
      {
	 spinDecodeCmd(pSpinId,&cmd);
      }

      /* another task checks spinStat, protect it */
      semTake(pSpinId->pSpinMutex, WAIT_FOREVER);
      pSpinId->spinStat = spinstat = spinStat(pSpinId,'T');	
      truspin = spinStat(pSpinId,'C')/2;
      semGive(pSpinId->pSpinMutex);

      if ( (truspin == TIMEOUT) || (truspin == CMDTIMEOUT) )
          currentStatBlock.stb.AcqSpinAct = (short) -2;
      else
          currentStatBlock.stb.AcqSpinAct = (short) truspin;
      temp = pSpinId->spinSetSpeed;
      /* DPRINT3(0,
		"spinReg: Set spin = %d, Tru spin: %d Spin stat: %d\n",
		 temp,truspin,spinstat); */


   }	/* FOREVER */
}
      

spinDecodeCmd(SPIN_ID pSpinId, SPIN_CMD *pCmd)
{
   int pid,pdigit,idigit,ddigit;
   int rstat,slew;
   int timeout,temp;

   switch(pCmd->VTCmd)
   {

   case SPIN_INIT:
       DPRINT(2,"spinDecodeCmd: SPIN Initialize Command\n");
       pSpinId->spinUpdateRate = pCmd->VTArg1 * sysClkRateGet();
       pSpinId->spinError = 0;
       currentStatBlock.stb.AcqSpinSet = (short) -2;
       currentStatBlock.stb.AcqSpinAct = (short) -2;
       break;

   case EJECT:
       DPRINT(2, "Decode: SPIN EJECT\n");
       sampleHasChanged = TRUE;
       currentStatBlock.stb.AcqSample = (short) 0;
       if ( ! (pSpinId->spinModeMask & EJECT_AIR_ON) )
       {  spinTalk(pSpinId, 'B', 0);
          spinTalk(pSpinId, 'E', 0);
       }
       pSpinId->spinModeMask |= EJECT_AIR_ON;
       break;
                   
   case EJECTOFF:
       DPRINT(2, "Decode: Eject off\n");
       if ( pSpinId->spinModeMask & EJECT_AIR_ON)
       {  spinTalk(pSpinId, 'Q', 0);    /* Bearing off */
          spinTalk(pSpinId, 'I', 0);    /* Insert on */
          Tdelay(100);                  /* 1 sec to change state */
       }
       pSpinId->spinModeMask &= ~EJECT_AIR_ON;
       break;

   case INSERT:		/* Q I B */
       DPRINT(2, "Decode: SPIN INSERT\n");
       if ( pSpinId->spinModeMask & EJECT_AIR_ON)
       {  spinTalk(pSpinId, 'Q', 0);	/* Bearing off */
          spinTalk(pSpinId, 'I', 0);	/* Insert on */
	  Tdelay(100);			/* 1 sec to change state */
	  Ldelay(&timeout,1000);	/* 10 sec for slowdrop */
          while ( ((temp=spinStat(pSpinId,'T'))&0x20) && temp!=-1)
          {
             if ( Tcheck(timeout) ) break;
	     Tdelay(50);
          }
	  Tdelay(100);			/* 1 more to seat */
	  if (pSpinId->spinSetSpeed != 0)
          {  spinTalk(pSpinId, 'B', 0);	/* Bearing on */
             spinTalk(pSpinId, 'S',pSpinId->spinSetSpeed);
          }
       }
       pSpinId->spinModeMask &= ~EJECT_AIR_ON;
       break;
       
   case BUMPSAMPLE:
       DPRINT(2, "Decode: SPIN BUMP\n");
       spinTalk(pSpinId, 'N', 7);
       spinTalk(pSpinId, 'V', 0);
       spinTalk(pSpinId, 'E', 0);
       taskDelay(6);
       spinTalk(pSpinId, 'I', 0);
       taskDelay(150);
       spinTalk(pSpinId, 'B', 0);
       spinTalk(pSpinId, 'V', 5);
       break;

   case BEARON:
       DPRINT(2, "Decode: SPIN BEARON\n");
       if ( ! (pTheSpinObject->spinModeMask & BEAR_AIR_ON) )
          spinTalk(pSpinId, 'B', 0);	/* Bearing on */
       pTheSpinObject->spinModeMask |= BEAR_AIR_ON;
       break;

   case BEAROFF:
       DPRINT(2, "Decode: SPIN BEAROFF\n");
       if (pTheSpinObject->spinModeMask & BEAR_AIR_ON)
          spinTalk(pSpinId, 'Q', 0);	/* Bearing off */
       pTheSpinObject->spinModeMask &= ~BEAR_AIR_ON;
       break;

   case SETRATE:
	DPRINT1(2,"spinDecodeCmd: Set Rate to %d\n",pCmd->VTArg1);
         spinTalk(pSpinId, 'R', pCmd->VTArg1);
	break;

   case SETMASTHRES:
	DPRINT1(2,"spinDecodeCmd: Set Threshold to %d\n",pCmd->VTArg1);
	setSpinMASThres(pCmd->VTArg1);
	break;

   case SETSPD:
       DPRINT1(2,"spinDecodeCmd: Set Speed to %d\n",pCmd->VTArg1);
       pSpinId->spinError = 0;
       if (pSpinId->spinSetSpeed != pCmd->VTArg1)
       {  pSpinId->spinSetSpeed = pCmd->VTArg1;
          currentStatBlock.stb.AcqSpinSet = (short) pCmd->VTArg1;
          pSpinId->spinModeMask &= ~VT_IS_REGULATED;/* turn off regulated bit */
          pSpinId->spinModeMask &= ~VT_TEMP_IS_SET;/* turn off temp set bit */
          pSpinId->spinModeMask &= ~VT_WAITING4_REG;
          pSpinId->VTRegCnt = 0L;
          pSpinId->VTRegTimeout = 0L;
          if (pSpinId->spinInterlock == 2)		/* READY */
             pSpinId->spinInterlock = 1;		/* STANDBY */
          spinTalk(pSpinId,'S',pCmd->VTArg1);
          semTake(pSpinId->pSpinRegSem,NO_WAIT);/* make sure its empty */
       }

       break;

   case SPIN_SETPID:
       DPRINT1(2,"spinDecodeCmd: Set PID to %d\n",pCmd->VTArg1);
       pSpinId->spinError = 0;
       pid = pCmd->VTArg1;
       if (pid > 0) 
       {
          pdigit = pid / 100;
          idigit = (pid % 100) / 10;
          ddigit = (pid % 10);
          rstat = 0;
	  rstat = spinTalk(pSpinId,'P',pdigit);    /* Proport'l 0-6 */
	  if (!rstat)
             rstat = spinTalk(pSpinId,'I',idigit); /* Integral 0-9 */
	  if (!rstat)
             rstat = spinTalk(pSpinId,'D',ddigit); /* Derivative 0-9 */
          if (rstat != 0)
		pSpinId->spinError = SPINERROR + rstat; /* SpinFail (TIMEOUT) */
       }
       break;

   case GETSPD:
       DPRINT(2, "Decode: SPIN GETSPEED\n");
       semTake(pSpinId->pSpinMutex, WAIT_FOREVER);
       pSpinId->spinTruSpeed = spinStat(pSpinId,'C')/2;
       semGive(pSpinId->pSpinMutex);
       DPRINT1(2,"spinDecodeCmd: Get Spin Speed %d\n",pSpinId->spinTruSpeed);
       break;

   case 1101:
       DPRINT1(2,
              "spinDecodeCmd: Wait 4 Spin to Regulate, Update Rate: %d sec\n",
	      pCmd->VTArg1);
       pSpinId->spinError = 0;
       pSpinId->spinModeMask |= VT_WAITING4_REG;
       pSpinId->spinUpdateRate = pCmd->VTArg1 * sysClkRateGet();
       break;
   default:
    errLogRet(LOGIT,debugInfo, "Decode: %d UNKNOWN\n",pCmd->VTCmd);
       break;
   }
}

/*----------------------------------------------------------------------*/
/*  setspin(speed,bumpflag)			                        */
/*  1. which SPin is present                                            */
/*  2. set Speed 							*/
/*  returns 0 or errorcode						*/
/*----------------------------------------------------------------------*/
setspin(int setspeed, int bumpflag)
/*  temp,ltmpxoff,pid; /* set temperature,low tmp cutoff,PID,IN flag */
{
   int rstat;
   SPIN_CMD cmd;
    DPRINT2(3,"setspin: speed=%d bflag=%d",setspeed,bumpflag);

   DPRINT2(2,"setspin: speed: %d, bumpflag: %d\n",setspeed,bumpflag);
   /* if Spin Object not present */
   if (pTheSpinObject == NULL)
   {
     spinCreate();
   }

   if (pTheSpinObject != NULL)
   {
      cmd.VTCmd = SETSPD;
      cmd.VTArg1 = setspeed;
      cmd.VTArg2 = bumpflag;
      msgQSend(pTheSpinObject->pSpinMsgQ,(char*) &cmd,sizeof(cmd),
		NO_WAIT,MSG_PRI_NORMAL);
   }
   return(0);
}

/********************************************************************
* 
*
*  This routine display the status information of the Spin Object
*
*
*  RETURN
*   VOID
*
*/
void spinShow(SPIN_ID pSpinId, int level)
/* SPIN_ID pSpinId - Spin Object ID */
/* int level 	   - level of information */
{
   int i;
   char *pIntlkMode;

   if (pSpinId == NULL)
     return;

   printf("\n -------------------------------------------------------------\n\n");
   printf("Spin Object (0x%lx)\n",pSpinId);
   printf("SPIN: '%s'\n",pSpinId->SPINIDSTR);
   printf("SPIN: type: %s, Serial Port: %d, ",
	(pSpinId->spinType == 1) ? "Simple" : "Auto",
	 pSpinId->spinPort);

   printf("\nSPIN State Semaphore: \n");
   printSemInfo(pSpinId->pSpinMutex,"SPIN Mutex Semaphore",1);
   printf("\nSPIN Reg Semaphore: \n");
   printSemInfo(pSpinId->pSpinRegSem,"SPIN Regulation Semaphore",1);

   printf("\nSPIN Cmd Queue (0x%lx): \n",pSpinId->pSpinMsgQ);
   msgQInfoPrint(pSpinId->pSpinMsgQ);

   spinPrintMode();
   printf("\nSpin Set: %d, True: %d Hz\n",
	currentStatBlock.stb.AcqSpinSet,currentStatBlock.stb.AcqSpinAct);
   printf("\nSpin Stat: %d, Reg Cnt: %d, Reg Timeout: %d, Error Type: '%s', Error: %d\n",
	 pSpinId->spinStat,pSpinId->VTRegCnt,pSpinId->VTRegTimeout,
	 ( (pSpinId->spinErrorType == HARD_ERROR) ? "Error" : "Warning"),
	 pSpinId->spinError);
   if (pSpinId->spinInterlock == 0)
      pIntlkMode = "No";
   else if (pSpinId->spinInterlock == 1)
      pIntlkMode = "To Be Enable";
   else
      pIntlkMode = "Yes";
   printf("Interlock Active: '%s', Updaterate: %d sec\n",pIntlkMode,
	     pSpinId->spinUpdateRate/sysClkRateGet());
   printf("-------------------------------------------------------------\n\n");


}
spinPrintMode()
{
    char *man, *heat,*wait, *reg, *fin;

    wait = (pTheSpinObject->spinModeMask & VT_WAITING4_REG) ? "On" : "Off";
    reg = (pTheSpinObject->spinModeMask & VT_IS_REGULATED) ? "True" : "False";
    fin = (pTheSpinObject->spinModeMask & VT_TEMP_IS_SET) ? "Complete" : "Incomplete";

    printf("VT Mode(0x%x)\n",
	pTheSpinObject->spinModeMask,man,heat);
    printf("          Regulated: '%s', Wait4Reg: '%s',  Finial Setting: '%s'\n",
	reg,wait,fin);
}
/********************************************************************
* SpinShow - display the status information on the Spin 
*
*  This routine display the status information of the Spin
*
*
*  RETURN
*   VOID
*
*/
VOID SpinSHow(int level)
/* int level 	   - level of information */
{
   spinShow(pTheSpinObject,level);
}


getSpinSpeed()
{
  int temp;
  if (pTheSpinObject != NULL)
  {
    semTake(pTheSpinObject->pSpinMutex, WAIT_FOREVER);
    temp = currentStatBlock.stb.AcqSpinAct;
    semGive(pTheSpinObject->pSpinMutex);
    return(temp);   /* updated via spinReg */
  }
  else
    return(-2);
}

setSpinType(int spintype)
{
  if ((spintype > 0) && (pTheSpinObject == NULL))
  {
     spinCreate();
  }
}

getSpinType()
{
  if (pTheSpinObject != NULL)
  {
    return(pTheSpinObject->spinPort);
  }
  else
    return(-1);
}

/************************
* Spin interlock
*/
setSpinInterlk(int in)
{
  DPRINT1(2,"ssetSpinInterlk: %d\n",in);
  pTheSpinObject->spinInterlock = in;	/* 1 - enable interlock when ready */
					/* 2 - Ready to be tested */
}

getSpinInterlk()
{
  DPRINT1(2,"getSpinInterlk: %d\n",pTheSpinObject->spinInterlock);
  return(pTheSpinObject->spinInterlock);
}

setSpinErrMode(int in)
{
  pTheSpinObject->spinErrorType = in; /*HARD_ERROR-Out of Regulation Error */
		     		       /*WARNING_MSG-Out of Regulation Warning*/
}

getSpinErrMode()
{
  return(pTheSpinObject->spinErrorType);
}

setSpinMASThres(int in)
{
  DPRINT2(2,"setSpinMASThres: new thres: %d, present thres: %d\n",
		in,SpinMASThreshold);
/* old spinner board does not know how to deal with this */
/* keep the threshhold between 80 and 500 */
  if (in > 500)
    in = 500;
  if (in >= 80)
  {
     if ((pTheSpinObject->spinType == 3) && (in != SpinMASThreshold) )
        spinTalk(pTheSpinObject,'M',in); /* set new threshold */
     SpinMASThreshold = in;   	/* speed (Hz) to switchover to MAS control */
  }
}

getSpinMASThres()
{
  return(SpinMASThreshold);
}

setSpinRegDelta(int hz)
{
  if ( hz != SpinDeltaReg)
  {
    DPRINT1(2,"setSpinRegDelta: new regulation delta %d Hz\n",hz);
    autoSpinSetRegDelta(pTheAutoObject,hz);  /* let MSR know new delta */
    SpinDeltaReg = hz;
  }
}

int getSpinSpinReg()
{
   if ( pTheSpinObject->spinStat & 0x80 )
      return (0);
   else
      return (1);
}

/*------------------------------------------------------*/
/*     setspinnreg(prtmod) -  Wait for spinner to regulate  */
/*                   to the given setting               */
/*                                                      */
/*                   If speed remains zero for 2 sec    */
/*                   then bump sample several times     */
/*                                                      */
/*                   If more than 2 attempts to bump    */
/*                      fail to make the sample spin    */
/*                      ABORT with failure code         */
/*                                                      */
/*                   changed, June 1996, to bump the    */
/*                   sample only if requested via an    */
/*                   argument.                          */
/*                                                      */
/*  This program is only called by auto-shim, not by    */
/*  the A-code interpreter.                             */
/*------------------------------------------------------*/
setspinnreg(int setspeed, int bumpFlag)
{
    setspin(setspeed, bumpFlag);
    spinreg(bumpFlag);
}

/**************************************************************
*
*  autoSpinReg - Return state of Spinner Regulation
*
*
* RETURNS:
*  1 = Regulated, 0 = Not Regulated. -1 = Error
*/
extern SPIN_ID pTheSpinObject;
int autoSpinReg(AUTO_ID pAutoId)
{
   if ( pTheSpinObject->spinStat & 0x80 )
      return (0);
   else
      return (1);
}

/***********************************************************************
* autoLSDVget - compute value for LSDV word based on other automation
*               card functions and current value of LSDV word
*
* RETURNS:
*  New value or current value if Object identifier is NULL
*/

#define OFF		0
#define REGULATED	1
#define NOTREG		2
#define NOTPRESENT	3

/*  Distinguish lock mode from lock status  */

#define  LSDV_LKMODE		0x02
#define  LSDV_MASK4LOCKSTAT	0x0c
#define  LSDV_SHIFT4LOCKSTAT	2
#define  LSDV_MASK4SPIN		0x30
#define  LSDV_SHIFT4SPIN	4
#define  LSDV_MASK4EJECT	0x40
#define  LSDV_SHIFT4EJECT	6
#define  DETECTED_MASK          0x0040
#define  LSDV_DETECTED          0x4000

int getSampleDetect()
{
   if ( pTheSpinObject->spinStat & DETECTED_MASK)
      return (1);
   else
      return (0);
}

short autoLSDVget(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
short	currentLSDV;
int	sysIsLocked, lockStat, ejectStat, spinStat;
int  regStat;
int  msgStat;

   currentLSDV = getLSDVword();

/*  The LSDV_LKMODE bit is set or cleared when lock mode is turned on or off.
/*    See lkapio.c.  Although a race condition exists between running this
/*    function and setting/clearing the lock mode, the consequence would only
/*    be inconsistancy between the lock mode and the lock status fields.
*/

   if (currentLSDV & LSDV_LKMODE)
   {
      sysIsLocked = *FF_STATR(pTheFifoObject->fifoBaseAddr) & LOCK_SENSE;
      if (sysIsLocked)
         lockStat = REGULATED;
      else
         lockStat = NOTREG;
   }
   else
   {  lockStat = OFF;
   }

   lockStat <<= LSDV_SHIFT4LOCKSTAT;
   currentLSDV &= ~(LSDV_MASK4LOCKSTAT);
   currentLSDV |= lockStat;

/*  Spinner is either off, regulated or not regulated.  */

   if (pTheSpinObject->spinPort == -1)
   {  if (*FF_STATR(pTheFifoObject->fifoBaseAddr) & SPIN_SENSE)
         spinStat = NOTREG | REGULATED;
      else
         spinStat = OFF;
   }
   else
   {  if ( pTheSpinObject->spinStat & 0x80 )
         spinStat = NOTREG;
      else if ( (pTheSpinObject->spinStat & 0x10) ||
	       !(pTheSpinObject->spinStat & 0x1 ) )
         spinStat = OFF;
      else
         spinStat = REGULATED;
   }

	spinStat <<= LSDV_SHIFT4SPIN;
	currentLSDV &= ~(LSDV_MASK4SPIN);
	currentLSDV |= spinStat;

/*  Set the eject status bit if no sample is in the probe.  */

/*	ejectStat=((*AUTO_STATR(pAutoId->autoBaseAddr)&SMPLE_AT_BOT)?1:0); 
/*	/* ejectStat = (autoSampleDetect( pAutoId ) == 0); */
	ejectStat = ((pTheSpinObject->spinModeMask) & EJECT_AIR_ON) ? 0 : 1;
	ejectStat <<= LSDV_SHIFT4EJECT;
	currentLSDV &= ~(LSDV_MASK4EJECT);
	currentLSDV |= ejectStat;

        if ( pTheSpinObject->spinStat & DETECTED_MASK)
           currentLSDV |= LSDV_DETECTED;
        else
           currentLSDV &= ~(LSDV_DETECTED);

	return( currentLSDV );
}
/*------------------------------------------------------*/
/*     spinreg() -  Wait for spinner to regulate  	*/
/*                   to the given setting               */
/*                                                      */
/*                   If speed remains zero for 2 sec    */
/*                   then bump sample several times     */
/*                                                      */
/*                   If more than 2 attempts to bump    */
/*                      fail to make the sample spin    */
/*                      ABORT with failure code         */
/*                                                      */
/*------------------------------------------------------*/
spinreg( int bumpFlag )
{
  int ok,regcnt,setspeed,speed,rate,timeout,regchk,stat,zerocnt,bumpcnt;
  int old_stat,regcount,rtnstat;

  ok = regcnt = FALSE;
  zerocnt = 0;                  /* # of times zero spinner speed obtained */
  bumpcnt = 0;                  /* allow only to bump sample twice then fail*/
  rtnstat = 0;

  /* avoid the impression of being slow we do this test up front */
  setspeed = currentStatBlock.stb.AcqSpinSet;
  speed = (autoSpinValueGet(pTheAutoObject)) / 10;
  stat  = autoSpinReg(pTheAutoObject);

  if ((stat == 1) && (speed == setspeed))
  {
    pTheSpinObject->spinInterlock = 2;
    return(rtnstat);
  }

  Ldelay(&timeout,12000);       /* allow 2 min total to regulate */
  old_stat = get_acqstate();
  update_acqstate(ACQ_SPINWAIT);
  getstatblock();   /* force statblock upto hosts */

  if (setspeed >= SpinMASThreshold)	/* Above this spin speed switch to MAS solids spinner control */
     regcount = 7;	/* check a little longer for MAS solids */
  else
     regcount = 4;	/* liquids */

  while (!ok)
     {
	  speed = spinStat(pTheSpinObject,'C')/2;
          stat  = spinStat(pTheSpinObject,'T');
          currentStatBlock.stb.AcqSpinAct = speed;
          getstatblock();   /* force statblock upto hosts */
DPRINT1(0,"interlock=%d\n",pTheSpinObject->spinInterlock);
DPRINT3(0,"setspeed=%d, speed=%d, stat=%d\n", setspeed, speed, stat);
DPRINT3(0,"regcnt=%d,zerocnt=%d, regcount=%d\n",regcnt,zerocnt,regcount);

          if ( (speed == -10000) )
          {
	     rtnstat = SPINERROR + SPINTIMEOUT;
	     break;
          }
 
          if (setspeed > 0)
          {
             if ( ! (stat & 0x80)) regcnt++;
             if (speed == 0) zerocnt++;     /* increment if zero speed */
          }
          else
          {
             if (speed == 0)
                regcnt++;
          }

          if (zerocnt > 2)
          {
            if (bumpFlag == 0)
            {
                return(SPINERROR + BUMPFAIL);
            }
            bumpcnt++;
   
            if (bumpcnt > 3)
            { 
	       rtnstat = SPINERROR + BUMPFAIL;
               break;
  	    } 
 	    samplebump(pTheAutoObject);
            taskDelay(sysClkRateGet() * 5);
            zerocnt = 0;
          } 

          DPRINT5(0,"SPIN_Reg: speed: %d, stat: %d, regcnt: %d, zerocnt: %d, bumpcnt: %d\n",
	  speed,stat,regcnt,zerocnt,bumpcnt);
         
          if (Tcheck(timeout)) 
          {
	       rtnstat = SPINERROR + RSPINFAIL;
	       break;
          }
          if (regcnt > regcount) ok = TRUE;
          taskDelay(sysClkRateGet() * 1);
     }
  update_acqstate(old_stat);
  getstatblock();   /* force statblock upto hosts */
  pTheSpinObject->spinInterlock = 2;
  return(rtnstat);
}     

samplebump()
{
   spinTalk(pTheSpinObject,'N',7);
   spinTalk(pTheSpinObject,'V',0);	/* set slow-drop to zero */
   spinTalk(pTheSpinObject,'E',0);
   taskDelay(6);
   spinTalk(pTheSpinObject,'I',0);
   taskDelay(150);
   spinTalk(pTheSpinObject,'B',0);
   spinTalk(pTheSpinObject,'V',5);	/* slow drop back to 5 sec */

}

