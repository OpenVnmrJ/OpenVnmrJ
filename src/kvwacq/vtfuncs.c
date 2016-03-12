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

#include "vtfuncs.h"
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

extern MSG_Q_ID pMsgesToPHandlr;
extern EXCEPTION_MSGE GenericException;

extern STATUS_BLOCK currentStatBlock;           /* Acqstat-like status block */
extern AUTO_ID   pTheAutoObject; /* Automation Object */
extern  int  get_acqstate();                  /*  in /sysvwacq/monitor.c  */

#define CR 13
#define CMD 0
#define TIMEOUT 98
#define CMDTIMEOUT -10000

#define VTOFF   30000   /* temp value if VT controller is tobe passive */
#define VT_NONE    0       /* No VT controller */
#define  LSDV_VTBIT1		0x800   /* 11 */
#define  LSDV_VTBIT2		0x1000   /* 12 */

static int tVTId;

/* The VTinterLck has 3 values, 
   0 - means that in Vnmr tin='n' or the VT has already gone 
	out of regulation.
   1 - means that in Vnmr tin='y' and the WAIT4VT has not 
       completed in its wait for the VT to go into regulation.
   2 - means that WAIT4VT has complete VT regulation and it is OK
       for statmonitor() in monitor.c to check VT for whether it
       has gone out of regulation.
   This was done since we no longer use the spontaneous mode (Q254)
    which could cause a rare failure mode, where after the Manual
    command was given the spontaneous status would corrupt the follow
    Q command result in VT being in never-never land.
*/
     
/*
  VTerrorType has two values,
  HARD_ERROR - means VT out of Regulation is an Error 
  WARNING_MSG - means VT out of Regulation is a Warning
*/
#define VT_MANUAL_ON_BIT 0x1	
#define VT_HEATER_ON_BIT 0x2
#define VT_GAS_ON_BIT 0x4
#define VT_SPONSTAT_BIT 0x8
#define VT_WAITING4_REG 0x10
#define VT_IS_REGULATED 0x20
#define VT_TEMP_IS_SET  0x40

typedef struct {
		char *pSID;	/* SCCS ID */
		int VTSetTemp;	/* setting of VT */
		int VTTruTemp;	/* present temperature of VT */
		int VTTempRate;	/* Rate in degrees/minute to raise/lower temp */
		int VTModeMask;	/* active,manual,passive,regulated,waiting,etc*/
		int VTLTmpXovr; /* Low Temp Cross Over for cooling gas */
		int VT_PID;	/* PID of VT */
		int VTPrvTemp;  /* Previous Temp in Ramp */
		int VTpresent;	/* VT present on system */
		int VTport;	/* serial port of VT */
		int VTinterLck; /* tin= 'y' or 'n' 0=no, 1=enable, 2=ready*/
		int VTtype;	/* 0 == NONE, 2 = Oxford */
		int VTstat;	/* VT Status */
		int VTerrorType;/* HARD_ERROR (15)==Error, WARNING_MSG(14)==Warning */
		int VTerror;	/* VT Error Number */
		int VTRegCnt;   /* Internal Counter to determine regulation */
				/* for Oxford VTs */
		ulong_t VTRegTimeout; /* Time to allow for VT regulation */
				      /* before giving up and reporting Error */
		int VTUpdateRate;/* Update Rate is in seconds */
	        MSG_Q_ID pVTMsgQ; /* Message Queue for VT server */
		SEM_ID pVTmutex;/* VT serial port mutual exclusion */
		SEM_ID pVTRegSem;/* VT Reg Sem */
		char VTIDSTR[129]; /* VT Version string, Oxford starts with */
				   /*  'VTC, Version 5.01' */
				   /* HighLand starts with  */
				   /* 'HIGHLAND ... PROGRAM 26901B ....' */
} VT_OBJ;

typedef VT_OBJ *VT_ID;


VT_ID pTheVTObject = NULL;

static int LockinterLck = 0;  /* in = 'y' or 'n' */
static int LockErrorType = HARD_ERROR; /* HARD_ERROR == Error, WARNING_MSG == Warning */

int VTstat(VT_ID pVtId,  char cmd);

/**************************************************************
*
*  vt
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

VT_ID  vtCreate()
{
   register VT_OBJ *pVtObj;
   int rstat;
   VT_CMD cmd;
   int OxVT(VT_ID pVtId, char cmd,int temp);
   int vtTempReg(VT_ID pVtId);

  /* ------- malloc space for FIFO Object --------- */
  if ( (pVtObj = (VT_OBJ *) malloc( sizeof(VT_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Space:");
    return(NULL);
  }
  
  DPRINT1(0,"vtCreate: Created VT Object: 0x%lx\n",pVtObj);

  /* zero out structure so we don't free something by mistake */
  memset(pVtObj,0,sizeof(VT_OBJ));

  pVtObj->VTSetTemp = VTOFF;
  pVtObj->VTTruTemp = VTOFF;
  pVtObj->VTTempRate = 125; /* increase temp by 12.5 degrees per minute */
  pVtObj->VTModeMask = VT_MANUAL_ON_BIT;
  pVtObj->VTpresent = VT_NONE;
  pVtObj->VTport = -1;
  pVtObj->VTinterLck = 0;  /* tin= 'y' or 'n' 0 = no, 1 = enable, 2 = ready*/
  pVtObj->VTtype = 0;		/* 0 == NONE, 2 = Oxford */
  pVtObj->VTerrorType = HARD_ERROR;	/* HARD_ERROR (15) == Error, WARNING_MSG(14) == Warning */
  pVtObj->VTUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE;	/* 3 sec */

  pVtObj->VTport = initSerialPort( 2 );
  setSerialTimeout(pVtObj->VTport,25);	/* set timeout to 1/4 sec */
  DPRINT1(0,"VTport = %d\n",pVtObj->VTport);

  pVtObj->pVTmutex = NULL;	/* VT serial port mutual exclusion */
  pVtObj->pVTmutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);
  if (pVtObj->pVTmutex == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Mutex:");
    free(pVtObj);
    return(NULL);
  }

  /* VT Regulated Semaphore */
  pVtObj->pVTRegSem =  semBCreate(SEM_Q_PRIORITY , SEM_EMPTY);
  if (pVtObj->pVTRegSem == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Reg. Sem:");
    vtDelete(pVtObj);
    return(NULL);
  }

  /* VT Cmd Semaphore */
/*
  pVtObj->pVTCmdSem =  semBCreate(SEM_Q_PRIORITY , SEM_EMPTY);
  if (pVtObj->pVTCmdSem == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Cmd. Sem:");
    vtDelete(pVtObj);
    return(NULL);
  }
*/

  pVtObj->pVTMsgQ = msgQCreate(25,sizeof(VT_CMD),MSG_Q_FIFO);
  if (pVtObj->pVTMsgQ == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Message Q:");
    vtDelete(pVtObj);
    return(NULL);
  }


#define DEBBIE_SAYS_NO_NO_NO

  /* Set VT into known configuration */
#ifdef DEBBIE_SAYS_NO_NO_NO_NO
  switch(1)
  {
   case 1:
          if ( (rstat = OxVT(pVtObj,'Q',222)) ) break ; /* No spontanous stat msgs*/
          if ( (rstat = OxVT(pVtObj,'M',0)) ) break;   /* back to manual mode */
          if ( (rstat = OxVT(pVtObj,'O',0)) ) break;   /* besure heaters are off */
          if ( (rstat = OxVT(pVtObj,'G',0)) ) break;   /* besure gas value is off*/
	  break;
  }

  
  if (rstat)
  {
     DPRINT(1,"vtCreate: Serial Communication to VT Unit, Failed");
     vtDelete(pVtObj);
     return(NULL);
#ifdef XXX
     /* Hey couldn't talk to VT */
     GenericException.exceptionType = WARNING_MSG;  
     GenericException.reportEvent = VTERROR + rstat;   /* errorcode is returned */
     DPRINT(-1,"vtCreate: couldn't talk to VT\n");
     /* send error to exception handler task */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		     sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
  }

#else
   /* just check Status do not initialize */
   rstat = VTstat(pVtObj,'S');   /* check Status, acqerrno will be set if cann't talk to VT */
   if (acqerrno == (VTERROR + VTTIMEOUT))
   {
     errLogRet(LOGIT,debugInfo,"vtCreate: Serial Communication to VT Unit timed out, considered Not Present");
     vtDelete(pVtObj);

	/* NOTPRESENT both bits set (3) */
	setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
        currentStatBlock.stb.AcqVTAct = (short) 30000 /* VTOFF */;
        currentStatBlock.stb.AcqVTSet = (short) 30000 /* VTOFF */;
	DPRINT(2,"Object NULL, no VT presemt\n");

     return(NULL);
   }
#endif

  pVtObj->VTModeMask |=  VT_MANUAL_ON_BIT;

  pTheVTObject = pVtObj;	/* set the static global for VT routines to use */

  pVtObj->pSID = NULL;   /* SCCS ID */

 /* Spawn VT Temp Chk Task */
 tVTId = taskSpawn("tVTTemp", VT_TASK_PRIORITY, 
		STD_TASKOPTIONS,
                CON_STACKSIZE, vtTempReg, (int) pVtObj, ARG2,
                ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
 if ( tVTId == ERROR)
 {
    errLogSysRet(LOGIT,debugInfo,
      "vtCreate: could not spawn VT Control");
    vtDelete(pVtObj);
    return(NULL);
 }
/* 1st command to VT task is to initialize */
  cmd.VTCmd = VT_INIT;
  cmd.VTArg1 = VT_NORMAL_UPDATE;
  msgQSend(pVtObj->pVTMsgQ,(char*) &cmd, sizeof(cmd), NO_WAIT, MSG_PRI_NORMAL);
  vtGetSW(pVtObj);
  DPRINT1(0,"VT: '%s'\n",pVtObj->VTIDSTR);
  return(pVtObj);
}

/**************************************************************
*
*  vtDelete - Deletes VT Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 12/2/96
*/
int vtDelete(VT_ID pVtId)
{
   int tid;

   if (pVtId == NULL)
     return(ERROR);

   if ((tid = taskNameToId("tVTTemp")) != ERROR)
	taskDelete(tid);

   if (pVtId->VTport > 0)
      close(pVtId->VTport);

   if (pVtId->pVTmutex)
       semDelete(pVtId->pVTmutex);
   if (pVtId->pVTRegSem)
       semDelete(pVtId->pVTRegSem);
/*
   if (pVtId->pVTCmdSem)
       semDelete(pVtId->pVTCmdSem);
*/
   if (pVtId->pVTMsgQ)
       msgQDelete(pVtId->pVTMsgQ);
   free(pVtId);

   return(0);
}

#ifdef NOTTESTED
killVThandler()
{
   int tid;
   int OxVT(VT_ID pVtId, char cmd,int temp);

   if ((tid = taskNameToId("tVTTemp")) != ERROR)
	taskDelete(tid);

    OxVT(pTheVTObject,'Q',222); /* No spontanous stat msgs*/
    OxVT(pTheVTObject,'M',0);   /* back to manual mode */
    OxVT(pTheVTObject,'O',0);   /* besure heaters are off */
    OxVT(pTheVTObject,'G',0);   /* besure gas value is off*/
}

/*--------------------------------------------------------------*/
/* VThandlerAA							*/
/* 	Abort sequence for Shandler.				*/
/*--------------------------------------------------------------*/
VThandlerAA()
{
   int tid;
   if ((tid = taskNameToId("tVTHandlr")) != ERROR)
   {
#ifdef INSTRUMENT
	wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	taskRestart(tid);
   }
}
#endif

/* gets and give VT Mutex semaphore, used by phandler to 
   allow VT commands to finish
*/
getnGiveVTMutex()
{
   if (pTheVTObject->pVTmutex == NULL)
     return;
   semTake(pTheVTObject->pVTmutex,WAIT_FOREVER);
   semGive(pTheVTObject->pVTmutex);
}


/*****************************************************************
*  Oxford VT controller driver                                            
*  OxVT(command,number,prtmod): command is a single character           
*                              number is usually the temperature       
*/
int OxVT(VT_ID pVtId,char cmd,int temp)
{
    int stat;
    DPRINT2(1,"OxVT: Cmd = %c  Var = %d ", cmd, temp);
/*
#ifdef INSTRUMENT
      wvEvent(EVENT_VTMUX_TAKE,NULL,NULL);
#endif
*/
    if (pVtId->pVTmutex != NULL)
    {
      semTake(pVtId->pVTmutex, WAIT_FOREVER);
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"OxVT: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    clearinput(pVtId->VTport);
    pputchr(pVtId->VTport, cmd);
    if (cmdecho(pVtId->VTport, CMD)) 
    {
      semGive(pVtId->pVTmutex);
      return(TIMEOUT);
    }
    switch(cmd)
    {
        case 'Q':
        case 'T':
        case 'O':
        case 'G':
        case 'F':
        case 'W':
        case 'U':
        case 'P':
        case 'I':
        case 'D':
        case 'K':	/* HighLand, set slew rate */
                echoval(pVtId->VTport,temp);
                pputchr(pVtId->VTport,CR);
                /* printf("Send 'CR' by 'OxVT' :%d Dec\t     ----> %c\n\n", CR, CR); */
                break;
        default:   break;
    }  
    if (cmd == 'B')
    {
        /* 'B' is a power cycle which takes a long time */
        if (cmddone(pVtId->VTport, 500) == -1)
        {
          semGive(pVtId->pVTmutex);
          return(TIMEOUT);
        }
    }
    else
    {
        /* if (cmddone(pVtId->VTport, 200) == -1) */
        if (cmddone(pVtId->VTport, 100) == -1)
        {
          semGive(pVtId->pVTmutex);
          return(TIMEOUT);
        }
    }
    semGive(pVtId->pVTmutex);
    return(0);
}
 
/*****************************************************************
*  Oxford VT status driver                                                 
*  VTstat(command,prtmod):     command is a single character              
*                              return the status requested 
*      prtmod: non-zero print messages else do not                       
*/
int VTstat(VT_ID pVtId,  char cmd)
{
    int value;
    acqerrno = 0;	/* clear errno */
    DPRINT1(2,"VTstat: CMD = %c \n",cmd);
    if (pVtId == NULL)
       return(CMDTIMEOUT);

    if (pVtId->pVTmutex != NULL)
    {
      semTake(pVtId->pVTmutex, WAIT_FOREVER);
    }
    else
    {
      acqerrno = VTERROR + VTTIMEOUT; 
      errLogRet(LOGIT,debugInfo,"VTstat: Mutex Pointer NULL\n");
      return(CMDTIMEOUT);
    }

    clearinput(pVtId->VTport);
    pputchr(pVtId->VTport,(int)cmd);            /* send command */
    DPRINT2(2,"Send by 'VTstat' :    %d Dec\t     ----> %c\n\n", cmd, cmd);
    if (cmdecho(pVtId->VTport,CMD)) 
    { 
      acqerrno = VTERROR + VTTIMEOUT; 
      semGive(pVtId->pVTmutex);
      return(CMDTIMEOUT); 
    }
    /* value = cmddone(VTport,300); */
    value = cmddone(pVtId->VTport,50);
    DPRINT1(2,"VTstat: Value = %d\n",value);
    semGive(pVtId->pVTmutex);
    return(value);
}
/*----------------------------------------------------------------------*/
/*  vtTempReg(VT_ID VTObj)			                        */
/*  1. which VT is present                                              */
/*  2. set temperature or make passive                                  */
/*  returns 0 or errorcode
/*----------------------------------------------------------------------*/
vtTempReg(VT_ID VtId)
/* VT_ID VtId - VT Object Pointer */
 {
   int bytes;
   int trutemp,temp,vtstat;
   int rstat;   /* return status of Serial I/O devices */
   VT_CMD cmd;

   FOREVER
   {
     /* Wait for a Message, After an interval "VTUpdateRate" proceed with standard chores */
     bytes = msgQReceive(VtId->pVTMsgQ, (char*) &cmd,
                          sizeof( VT_CMD ), VtId->VTUpdateRate);
     /* DPRINT1(0,"vtTempReg: Recv: %d bytes\n",bytes); */
     /* command then act on it, otherwise just do normal chores */
     if (bytes > 0)
     {
	 VtDecodeCmd(VtId,&cmd);
     }
       /* another task checks VTstat, protect it */
       semTake(VtId->pVTmutex, WAIT_FOREVER);
       VtId->VTstat = vtstat = VTstat(VtId,'S');
       rstat = VTstat(VtId,'R');
       if (rstat == CMDTIMEOUT)
       {
          VtId->VTstat = vtstat = rstat;
       }
       else
       {
          VtId->VTTruTemp = trutemp = rstat;
       }
  
       semGive(VtId->pVTmutex);

       if ( (trutemp == TIMEOUT) || (trutemp == CMDTIMEOUT) )
          currentStatBlock.stb.AcqVTAct = (short) VTOFF;
       else
          currentStatBlock.stb.AcqVTAct = (short) trutemp;
       temp = VtId->VTSetTemp;
       /* DPRINT3(0,"vtTempReg: Set Temp = %d, Tru Temp: %d VT stat: %d\n", temp,trutemp,vtstat); */

       if (temp != VTOFF)
       {
          switch(vtstat)
          {   
            case 0:         /* In Manual Mode ?? */
       	     VtId->VTerror = 0;
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET ) 
             {
	        ChangeVtTemp(VtId,trutemp);
  	     }
             else if ( (trutemp <= temp + 5) &&
                       (trutemp >= temp - 5) )
             {
	       /* OK ignore the fact VT is in manual mode, and count this as regulated */
               VtId->VTRegCnt++; 
             }
             break;
                         
            case 1:         /* Temp Stable  */
       	     VtId->VTerror = 0;
             /* use these tighter standards for VT doesn't slip out of */
             /* reg while acquiring       */
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET ) /* just incase never set */
             {
	        ChangeVtTemp(VtId,trutemp);
  	     }
             else if ( (trutemp < temp + 4) &&  /* i.e., 30, 29.7-30.3 */
                       (trutemp > temp - 4) )
             {
               VtId->VTRegCnt++; /* count this as regulated */
             }
             break;
  
            case 2:         /* Temp Changing  */
       	     VtId->VTerror = 0;
             VtId->VTRegCnt = 0; /* reset back to zero */
	     ChangeVtTemp(VtId,trutemp);
             break;

            case 3:         /* Safety sensor limiting Output */
             VtId->VTerror = VTERROR + VTSSLIMIT;
             break;
 
            case -1:        /* Gas not flowing or o/p stage fault */
             VtId->VTerror = VTERROR + VTNOGAS;
             break;
      
            case -2:        /* Main sensor on Bottom limit */
             VtId->VTerror = VTERROR + VTMSONBOT;
             break;
             
            case -3:        /* Main sensor on Top limit */
             VtId->VTerror = VTERROR + VTMSONTOP;
             break;
             
            case -4:        /* s/c Safety sensor */
             VtId->VTerror = VTERROR + VT_SC_SS;
             break;
           
            case -5:        /* o/c Safety sensor */
             VtId->VTerror = VTERROR + VT_OC_SS;
             break;
             
            case -10:       /* timeout */
             VtId->VTerror = VTERROR + TIMEOUT;
             break;

           case TIMEOUT:	    /* timeout */
           case CMDTIMEOUT:	    /* timeout */
	     VtId->VTerror = VTERROR + TIMEOUT;
             break;
          }
          DPRINT3(1,"vtTempReg: --------> Temp: %d, VTstat: %d, RegCnt: %d\n",
			trutemp,vtstat,VtId->VTRegCnt);
       }
       else  /* turn off VT */
       {
         if (VtId->VTModeMask & VT_HEATER_ON_BIT)
         {
             VtId->VTTruTemp = currentStatBlock.stb.AcqVTAct = (short) VTOFF;
             VtId->VTSetTemp = currentStatBlock.stb.AcqVTSet = (short) VTOFF;
           DPRINT(0,"vtTempReg: VTOFF & Heater On, Turn Off VT\n");
           if ( (rstat=OxVT(VtId,'Q',222))|| /* No spontanous stat msgs*/
               (rstat=OxVT(VtId,'M',0))  || /* back to manual mode */
               (rstat=OxVT(VtId,'O',0))  || /* besure heaters are off */
               (rstat=OxVT(VtId,'G',0))       /* besure gas value is off*/
              ) return(VTERROR + rstat);
           VtId->VTModeMask = VT_MANUAL_ON_BIT | VT_WAITING4_REG;  /* MANUAL, HEater Off, Not Regulated */
       	   VtId->VTerror = 0;
         }

         /* VT Off both bits unset (0) */
	 clearLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
    	 DPRINT(2,"setVT_LSDVbit: VT Set Off\n");

      if ((VtId->VTModeMask & VT_WAITING4_REG) && (trutemp < 310))
      {
             DPRINT(0,"vtTempReg: VTOFF, WAITING4VT to go Ambient Temp, So Give
Semaphore\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
             semGive(VtId->pVTRegSem);
         }

       } /* if (temp != VTOFF) */


/*
      if ((VtId->VTModeMask & VT_IS_REGULATED) && (vtstat != 1))
      {
	   VT out of regulation report error
      }
*/

      /* if not regulated yet && Finial Temp has been Set, and Temp stable for 30 sec */
      /*
	 Now If:
		1. VT is NOT Regulated
	        2. VT Finial Temp Has Been Set
	        3. VT has Been Stable for 30 Sec  (VTRegCnt > 30)
	     Then
		Set Bit indicated VT is Regulated, and return to normal update rate
	      
		If wait4VT() has been called 
                   then reset bit flag and give the semaphore it is waiting on.

		   If The interlock is on STANDBY-1, 
		      then set it to READY-2 
      */
      if (  ((VtId->VTModeMask & VT_IS_REGULATED) != VT_IS_REGULATED) && 
	    (VtId->VTModeMask & VT_TEMP_IS_SET) && 
	    (VtId->VTRegCnt > 30)) /* if stable 30 sec, then its OK */
      {
          DPRINT(0,"vtTempReg: SET VT to Regulated, and reset UpdateRate\n");

	  /* VT in Regulation 1st bit set (1) */
	  setLSDVbits( LSDV_VTBIT1 );
	  clearLSDVbits( LSDV_VTBIT2 );

          VtId->VTModeMask |= VT_IS_REGULATED;
	  VtId->VTUpdateRate = VT_NORMAL_UPDATE * sysClkRateGet();
	  VtId->VTRegCnt = 0;
	  /* If waiting then give semaphore  */
	  if (VtId->VTModeMask & VT_WAITING4_REG)
          {
             if (VtId->VTinterLck == 1) /* ENABLE */
                  VtId->VTinterLck = 2; /* READY to Check, & report Errors  */

             DPRINT(0,"vtTempReg: WAITING4VT So Give Semaphore\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
	     semGive(VtId->pVTRegSem);
          }
      }

      /* set bits if VT goes out-of-regulation */
      if ((temp != VTOFF) && (vtstat !=1) )
      {
	  /* VT out of Regulation 2nd bit set (2) */
	  clearLSDVbits( LSDV_VTBIT1 );
	  setLSDVbits( LSDV_VTBIT2 );
      }

      if ( ((VtId->VTModeMask & VT_IS_REGULATED) == VT_IS_REGULATED) &&
           (VtId->VTinterLck == 2) && (vtstat != 1) )
      {
        DPRINT(0,"vtTempReg: Reporting out-of-regulation ....\n");
	/* disable just VT interlock tests if it is a Warning */
	VtId->VTinterLck = 0; /*setVTinterLk(0); */
	if (VtId->VTerrorType == HARD_ERROR)  /*getVTErrMode(), (15)  Error */
	{
          /* disable all interlock tests, since this will stop the exp (for Error), don't want
	     continue error coming back while no exp. is running */
	  setLKInterlk(0);
	  setSpinInterlk(0);
       	  GenericException.exceptionType = HARD_ERROR;  
	}
	else  /* WARNING */
	{
       	  GenericException.exceptionType = WARNING_MSG;  
	}
        if ((vtstat == TIMEOUT) || (vtstat == CMDTIMEOUT))
        {
       	  GenericException.reportEvent = VTERROR + VTTIMEOUT; /* VTERROR + VTREGFAIL; */
        }
        else if (vtstat != 1)
        {
	  GenericException.reportEvent = VTERROR + VTOUT;
        }
    	/* send error to exception handler task */
    	msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
	  		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
      }
  }	/* FOREVER */
}
      

ChangeVtTemp(VT_ID VtId, int trutemp)
{
   int rstat, sign, ntemp, deltatemp;


   if ( (VtId->VTModeMask & VT_TEMP_IS_SET) || 
	((VtId->VTRegTimeout != 0L) && (VtId->VTRegTimeout > time(NULL)))
      )
   {
      DPRINT1(2,"ChangeVtTemp, VT Setting Complete? %d\n",(VtId->VTModeMask & VT_TEMP_IS_SET));
      DPRINT2(2,"ChangeVtTemp, Min. not up Yet. Now: 0x%lx, TimeOut: 0x%lx\n",time(NULL),VtId->VTRegTimeout);
      return(0);
   }
     
   if (VtId->VTPrvTemp >= (VTOFF / 2)  )
   {
      DPRINT2(1,"VTPrvTemp reset from %d to %d\n",VtId->VTPrvTemp,trutemp);
      VtId->VTPrvTemp = trutemp;
   }

   if (VtId->VTTruTemp > VtId->VTSetTemp) sign = -1; else sign = 1;

   /* increase temp by 12.5 degrees per minute */
   /* 125 / 60 * rate / clock rate */
   /* deltatemp = (VtId->VTTempRate / 60) * ( VtId->VTUpdateRate / sysClkRateGet()); */
   deltatemp = VtId->VTTempRate;
   /* DPRINT2(1,"ChangeVtTemp: Temp Delta: %d for Update Rate: %d\n",deltatemp,VtId->VTUpdateRate); */

   /* If the differnece in True & Set is greater than delta temp rate then change Temp
      by the Rate, Else Set to Finial Temp Setting
   */

   /* TODO: donn't use tru temp to add to but rather the next step in rate, so that
            the number of times to change temp is deterministic  */
   if ( abs(VtId->VTTruTemp - VtId->VTSetTemp) > deltatemp)  /* temp > 12.5 degrees */
   { 
	ntemp =  VtId->VTPrvTemp += sign*deltatemp;

        VtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */

        DPRINT2(0,"Incremental Temp Increase to %d,  finial temp %d\n",ntemp,VtId->VTSetTemp);
        if ( (rstat = OxVT(VtId,'M',0))  || /* back to manual mode */
           (rstat = OxVT(VtId,'Q',222))|| /* No spontanous statmsg*/
           (rstat = OxVT(VtId,'T',ntemp))  /* program desired temp */
           )  return(VTERROR + rstat);            /* VT fail (TIMEOUT) */

	/* Must set Gas valve here for Oxford VT else 
	   Oxford will auto set it based on selected Temp
	   P.S. HighLand doesn't have this problem
        */
        if (VtId->VTSetTemp <= VtId->VTLTmpXovr) /* cooling needed ? */
	{
           if (rstat = OxVT(VtId,'G',1))
           {
               VtId->VTerror = VTERROR + rstat;
               return(VTERROR + rstat); /*cooling gas on */
           }
           DPRINT(0,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           DPRINT(0,"VT Gas OFF\n");
           VtId->VTModeMask &= ~VT_GAS_ON_BIT;  /* Gas Off */
        }

        if (rstat = OxVT(VtId,'A',0))
        {
	   VtId->VTerror = VTERROR + rstat;
           return(VTERROR + rstat);/*automod,tmp 4 display*/
	}
	VtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	VtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	VtId->VTRegTimeout = time(NULL) + 60L; /* next check in 60 secs */

      }
      else
      {
        DPRINT1(0,"Set finial VT temp %d\n",VtId->VTSetTemp);
        VtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */
        if ( (rstat = OxVT(VtId,'M',0))  || /* back to manual mode */
             (rstat = OxVT(VtId,'Q',222))|| /* No spontanous statmsg*/
             (rstat = OxVT(VtId,'T',VtId->VTSetTemp))  /* program desired temp */
           )  
	   return(VTERROR + rstat);            /* VT fail (TIMEOUT) */

	/* Must set Gas valve here for Oxford VT else 
	   Oxford will auto set it based on selected Temp
	   P.S. HighLand doesn't have this problem
        */
        if (VtId->VTSetTemp <= VtId->VTLTmpXovr) /* cooling needed ? */
	{
           if (rstat = OxVT(VtId,'G',1))
           {
               VtId->VTerror = VTERROR + rstat;
               return(VTERROR + rstat); /*cooling gas on */
           }
           DPRINT(0,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           DPRINT(0,"VT Gas OFF\n");
           VtId->VTModeMask &= ~VT_GAS_ON_BIT;  /* Gas Off */
        }


        if (rstat = OxVT(VtId,'A',0))
        {
	   VtId->VTerror = VTERROR + rstat;
           return(VTERROR + rstat);/*automod,tmp 4 display*/
	}
	VtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	VtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	VtId->VTModeMask |= VT_TEMP_IS_SET;  /* VT has been set to the finial Temp */

    }
  return(0);
}

VtDecodeCmd(VT_ID pVtId, VT_CMD *pCmd)
{
   int pid,pdigit,idigit,ddigit;
   int rstat,slew;
   int vtmask;

   switch(pCmd->VTCmd)
   {

     case VT_INIT:
      DPRINT(0,"VtDecodeCmd: VT Initialize Command\n");
       pVtId->VTUpdateRate = pCmd->VTArg1 * sysClkRateGet();
       pVtId->VTerror = 0;
       currentStatBlock.stb.AcqVTSet = (short) VTOFF;
       currentStatBlock.stb.AcqVTAct = (short) VTOFF;
       break;

     case VT_SETTEMP:
      DPRINT1(0,"VtDecodeCmd: Set Temp to %d\n",pCmd->VTArg1);
      pVtId->VTerror = 0;
     /* if setting to same temperature & VT is already regulated at this temp
         then don't bother to set temp, etc..
      */
        /*   ((VtId->VTModeMask & VT_IS_REGULATED) == VT_IS_REGULATED) ) */
      if (pVtId->VTSetTemp != pCmd->VTArg1)
      {
         pVtId->VTSetTemp = pCmd->VTArg1;
         pVtId->VTPrvTemp = pVtId->VTTruTemp;	/* set start temp that ramp will add to */
         pVtId->VTLTmpXovr = pCmd->VTArg2;
         currentStatBlock.stb.AcqVTSet = (short) pCmd->VTArg1;
         pVtId->VTModeMask &= ~VT_IS_REGULATED;	/* turn off regulated bit */
         pVtId->VTModeMask &= ~VT_TEMP_IS_SET;	/* turn off temp set bit */
         pVtId->VTModeMask &= ~VT_WAITING4_REG;
         pVtId->VTModeMask &= ~VT_GAS_ON_BIT;
         pVtId->VTRegCnt = 0L;
         pVtId->VTRegTimeout = 0L;

        /* VT out of Regulation 2nd bit set (2) */
        clearLSDVbits( LSDV_VTBIT1 );
        setLSDVbits( LSDV_VTBIT2 );

         if (pVtId->VTinterLck == 2 /* READY */)
	    pVtId->VTinterLck = 1;	/* STANDBY */
         semTake(pVtId->pVTRegSem,NO_WAIT);		/* make sure its empty */

         /* setting VT_WAITING4_REG done in vtTempReg() */
      }
      else
      {
        DPRINT(0,"VtDecodeCmd: VT_SETTEMP: already at set temperature, no action
 taken\n");
        if (pCmd->VTArg1 == VTOFF)
           semGive(pVtId->pVTRegSem);           /* setVT in this case is wait on
 Sem so give it */
      }
      break;

     case VT_SETPID:
      DPRINT1(0,"VtDecodeCmd: Set PID to %d\n",pCmd->VTArg1);
      pVtId->VTerror = 0;
         pid = pCmd->VTArg1;
         if ( (pid > 0)  && (pid != pVtId->VT_PID) )
	 {
	    pdigit = pid / 100;
	    idigit = (pid % 100) / 10;
	    ddigit = (pid % 10);
	    rstat = 0;
	    switch(1)
            {
		case 1:
	           if (rstat = OxVT(pVtId,'M',0)) break; /* back to manual mode */
          	   pVtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */
	           if (rstat = OxVT(pVtId,'Q',222)) break; /* No spontanous statmsg*/
	           if (rstat = OxVT(pVtId,'P',pdigit)) break; /* Proportional value 0-6 */
	           if (rstat = OxVT(pVtId,'I',idigit)) break; /* Integral value 0-9 */
	           if (rstat = OxVT(pVtId,'D',ddigit)) break; /* Derivative value 0-9 */
	           break;
	    }  
            if (rstat != 0)
            {
		pVtId->VTerror = VTERROR + rstat; /* VT fail (TIMEOUT) */
            }
            else
            {
                pVtId->VT_PID = pid;
            }

          }
          else
          {
             DPRINT(0,"VtDecodeCmd: same PID, no action taken\n");
          }
      break;

     case VT_GETTEMP:
       	semTake(pVtId->pVTmutex, WAIT_FOREVER);
      	rstat = VTstat(pVtId,'R');
      	if (rstat != CMDTIMEOUT)
           pVtId->VTTruTemp = VTstat(pVtId,'R');
       	semGive(pVtId->pVTmutex);
        DPRINT1(1,"VtDecodeCmd: Get VT Temp %d\n", pVtId->VTTruTemp);
      break;

     case VT_GETSTAT:
       semTake(pVtId->pVTmutex, WAIT_FOREVER);
       pVtId->VTstat = VTstat(pVtId,'S');
       semGive(pVtId->pVTmutex);
       DPRINT1(1,"VtDecodeCmd: Get VT Stat: %d\n", pVtId->VTstat);
       break;

     case VT_WAIT4REG:
            DPRINT1(0,"VtDecodeCmd: Wait 4 VT to Regulate, Update Rate: %d sec\n",pCmd->VTArg1);
	    if ( (pVtId->VTModeMask & VT_IS_REGULATED) != VT_IS_REGULATED )
            {
      	      pVtId->VTerror = 0;
	      pVtId->VTModeMask |= VT_WAITING4_REG;
              pVtId->VTUpdateRate = pCmd->VTArg1 * sysClkRateGet();
            }
            else
            {
              if (pVtId->VTinterLck == 1) /* ENABLE */
                  pVtId->VTinterLck = 2; /* READY to Check, & report Errors  */

               DPRINT(0,"VtDecodeCmd: already Regulated so Give Semaphore\n");
	       semGive(pVtId->pVTRegSem);
            }
	    break;

     case VT_SETSLEW:		/* HiLand only */
          DPRINT1(0,"VtDecodeCmd: Set VT Temp Slew Rate: %d \n",pCmd->VTArg1);
      	  pVtId->VTerror = 0;
          slew = pCmd->VTArg1;
          vtmask = pVtId->VTModeMask;
          if (rstat = OxVT(pVtId,'M',0)) break; /* back to manual mode */
          pVtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */
          if (rstat = OxVT(pVtId,'K',slew)) break; /* HiLand VT Slew rate nnnn/10 Degrees/Min 6 */
	  if ((vtmask & VT_MANUAL_ON_BIT) == 0)  /* was in automatic, so return it to such */
	  {
             rstat = OxVT(pVtId,'A',0);
	     pVtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	     pVtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	  }
      break;

     case VT_SETCALIB:		/* HiLand only */
         errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_SETCALIB not implemented yet.");
      break;

     case VT_GETHEATPWR:	/* HiLand only */
         errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_GETHEATPWR not implemented yet.");
      break;

     case VT_GETSWVER:		/* HiLand only */
         errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_GETSWVER not implemented yet.");
      break;

     }
}
vtGetResis()
{
   int value;
   if (pTheVTObject != NULL)
   {
     semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
     value = VTstat(pTheVTObject,'J');
     semGive(pTheVTObject->pVTmutex);
     /* printf("Probe Heater Resistence: %d\n",value); */
     return(value);
   }
   return(-1);
}
vtGetHeater()
{
   int value;
   if (pTheVTObject != NULL)
   {
     semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
     value = VTstat(pTheVTObject,'H');
     semGive(pTheVTObject->pVTmutex);
     /* printf("HighLand Heater Power: %d\n",value); */
     return(value);
   }
   return(-1);
}
vtGetSW(VT_ID pVtId)
{
   int value;
   char cmd;
   if (pTheVTObject != NULL)
   {
     semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
     clearport( pTheVTObject->VTport );
     cmd = 'V';
     pputchr(pTheVTObject->VTport,(int)cmd);            /* send command */
     /* write( pTheVTObject->VTport, &cmd, 1 ); */
     cmdecho(pTheVTObject->VTport,CMD);
     value = readreply(pTheVTObject->VTport,300,pTheVTObject->VTIDSTR,129);
     semGive(pTheVTObject->pVTmutex);
     /* printf("vtGetSW: \n'%s', value: %d\n",str,value); */
     return(value);
   }
   return(-1);
}
vtGetZ()
{
   int value;
   char cmd;
   char zStr[258];
   if (pTheVTObject != NULL)
   {
     semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
     clearport( pTheVTObject->VTport );
     cmd = 'Z';
     pputchr(pTheVTObject->VTport,(int)cmd);            /* send command */
     /* write( pTheVTObject->VTport, &cmd, 1 ); */
     cmdecho(pTheVTObject->VTport,CMD);
     value = readreply(pTheVTObject->VTport,300,zStr,257);
     semGive(pTheVTObject->pVTmutex);
     zStr[258] = '\0';
     printf("vtGetZ:\n'%s'\n",zStr);
     return(value);
   }
   return(-1);
}

/*----------------------------------------------------------------------*/
/*  setVT(temperature,low temp cutoff)		                        */
/*  1. which VT is present                                              */
/*  2. set temperature or make passive                                  */
/*  returns 0 or errorcode						*/
/*----------------------------------------------------------------------*/
setVT(int temp, int ltmpxoff, int pid)
/*  temp,ltmpxoff,pid; /* set temperature,low tmp cutoff,PID,IN flag */
{
   int old_stat;
   int rstat;
   VT_CMD cmd;

   DPRINT3(0,"setVT: Temp: %d, ltmpxoff: %d, pid: %d\n",temp,ltmpxoff,pid);
   /* if VT Object not present */
   if (pTheVTObject == NULL)
   {
     vtCreate();
   }

   if (pTheVTObject != NULL)
   {
     if (pid != 0)
     {
       cmd.VTCmd = VT_SETPID;
       cmd.VTArg1 = pid;
       msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),NO_WAIT,MSG_PRI_NORMAL);
     }

     cmd.VTCmd = VT_SETTEMP;
     cmd.VTArg1 = temp;
     cmd.VTArg2 = ltmpxoff; 
     msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),NO_WAIT,MSG_PRI_NORMAL);
     /* if setting VT to off wait for temp to come to ambient temp, arbitrary < 31 C  */
     if (temp == VTOFF) 
     {
          old_stat = get_acqstate();
          update_acqstate(ACQ_VTWAIT);
          getstatblock();   /* force statblock upto hosts */

	  /* wait 10 min */
          semTake(pTheVTObject->pVTRegSem,NO_WAIT);		/* make sure its empty */
          semTake(pTheVTObject->pVTRegSem,(600 * sysClkRateGet()));

   	  update_acqstate(old_stat);
   	  getstatblock();   /* force statblock upto hosts */
     }
   }
   return(0);
}

/*----------------------------------------------------------------------*/
/*  wait4VT(time) - wait for time(seconds) then check VT if not */
/*                      in reg then return failure                      */
/*                  0 = not in regulation                               */
/*                  1 = in regulation                                   */
/*   always called if tin='y' so when we are finished set VTinterlk to 2*/
/*----------------------------------------------------------------------*/
int wait4VT(int time_arg)
{
   int old_stat;
   int rstat;
   int timeout;
   VT_CMD cmd;

   /* if VT Object not present */
   if (pTheVTObject == NULL)
   {
     return(VTERROR +TIMEOUT);
   }
   
   old_stat = get_acqstate();
   update_acqstate(ACQ_VTWAIT);
   getstatblock();   /* force statblock upto hosts */

   cmd.VTCmd = VT_WAIT4REG;
   cmd.VTArg1 = VT_WAITING_UPDATE;
   msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),NO_WAIT,MSG_PRI_NORMAL);

   timeout = time_arg * sysClkRateGet();
   DPRINT1(0,"wait4VT: Timeout in %d Seconds, Waiting\n",time_arg);
   if ( (rstat = semTake(pTheVTObject->pVTRegSem,timeout)) == ERROR)
   {
     rstat = VTERROR + VTREGFAIL;
   }
   else
     rstat = 0;

   update_acqstate(old_stat);
   getstatblock();   /* force statblock upto hosts */

   return(rstat);
}

#ifdef XXX
/*----------------------------------------------------------------------*/
/*  resetVT() - reset VT 						*/
/*----------------------------------------------------------------------*/
void resetVT()
{
   int old_stat;
   int rstat;
   int timeout;
   VT_CMD cmd;

   /* if VT Object not present */
   if (pTheVTObject == NULL)
   {
     return;
   }
   
   /* take VT Mutex this will force the VT task to be come the 
      same Priority as calling Task.  Aaah, Oops only if it is trying
      to get this Mutex.  Need to think more on this one.
   */
   semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
   cmd.VTCmd = VT_RESET;
   cmd.VTArg1 = VT_NORMAL_UPDATE;
   msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),NO_WAIT,MSG_PRI_NORMAL);
   semTake(pTheVTObject->pVTCmdSem,WAIT_FOREVER);
   semGive(pTheVTObject->pVTmutex);

}
#endif

void resetVT()
{
DPRINT1(0,"Object=%lx\n",pTheVTObject);
   if (pTheVTObject != NULL)
   {
     semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
     pTheVTObject->VTSetTemp = VTOFF;
     pTheVTObject->VTTruTemp = VTOFF;
     currentStatBlock.stb.AcqVTSet = (short) VTOFF;
     currentStatBlock.stb.AcqVTAct = (short) VTOFF;
     pTheVTObject->VTModeMask = VT_MANUAL_ON_BIT;  /* MANUAL ON, turn off regulated bit, temp set bit */
     pTheVTObject->VTRegCnt = 0L;
     pTheVTObject->VTRegTimeout = 0L;
     pTheVTObject->VTinterLck = 0;
     pTheVTObject->VTUpdateRate = VT_NORMAL_UPDATE * sysClkRateGet();
     OxVT(pTheVTObject,'M',0);
     OxVT(pTheVTObject,'B',0);
     semGive(pTheVTObject->pVTmutex);
   }
}

/*----------------------------------------------------------------------*/
/*  VTchk() - check VT for regulation                                   */
/*              return 1 if in regulation                               */
/*              return 0 if not                                         */
/*----------------------------------------------------------------------*/
VTchk()      
 {           
    int status,error;

    error = 0;
    if (pTheVTObject != NULL)
    {
       semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
       status = pTheVTObject->VTstat;
       semGive(pTheVTObject->pVTmutex);
       if ((status == TIMEOUT) || (status == CMDTIMEOUT))
       {
	  error = VTERROR + VTTIMEOUT;
       }
       else if (status != 1)
       {
	  error = VTERROR + VTOUT;
       }
       semGive(pTheVTObject->pVTmutex);
       acqerrno = error;
       if (error == 0)
       {
          return(1);   /* Oxford VT In Regulation */
       }
       else
       {
          return(0);   /* Oxford VT Out of Regulation/Error */
       }
    }
    else
      return(0);
 }    

void setVT_LSDVbits()
{
  /* ---- Obtain VT status --- */
    if ( pTheVTObject == NULL )
    {
	/* NOTPRESENT both bits set (3) */
	setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
	DPRINT(2,"Object NULL, no VT presemt\n");
    }
    else
    {
#ifdef XXX
      if ( pTheVTObject->VTtype == VT_NONE )
      {
	/* NOTPRESENT both bits set (3) */
	setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
      }
      else
      {
#endif 
        if ( (currentStatBlock.stb.AcqVTSet == VTOFF) )
        {
	   /* VT Off both bits unset (0) */
	   clearLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
    	   DPRINT(2,"setVT_LSDVbit: VT Set Off\n");
        }
        else
        {
            if (pTheVTObject->VTport == -1) /* be sure VT is there first */
            {
		/* NOTPRESENT both bits set (3) */
		setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
    	        DPRINT(2,"setVT_LSDVbit: VT Port -1 \n");
                return;
            }
            else
            {
             if (VTstat(pTheVTObject,'S') == 1)       /* == 1, in Regulation */
	     {
	        /* VT in Regulation 1st bit set (1) */
		setLSDVbits( LSDV_VTBIT1 );
		clearLSDVbits( LSDV_VTBIT2 );
    	        DPRINT(2,"setVT_LSDVbit: VT in Regulation\n");
	     }
             else
	     {
	        /* VT out of Regulation 2nd bit set (2) */
		clearLSDVbits( LSDV_VTBIT1 );
		setLSDVbits( LSDV_VTBIT2 );
    	        DPRINT(2,"setVT_LSDVbit: VT out of Regulation\n");
	     }
            }
        }
#ifdef XXXX
      }
#endif
   }
   return;
}

/********************************************************************
* 
*
*  This routine display the status information of the VT Object
*
*
*  RETURN
*   VOID
*
*/
VOID vtShow(VT_ID pVtId, int level)
/* VT_ID pVtId - VT Object ID */
/* int level 	   - level of information */
{
   int i;
   char *pIntlkMode;

   if (pVtId == NULL)
     return;

   printf("\n -------------------------------------------------------------\n\n");
   printf("VT Object (0x%lx)\n",pVtId);
   if (pVtId->VTIDSTR[0] == 0)
      vtGetSW(pVtId);
   printf("VT: '%s'\n",pVtId->VTIDSTR);
   printf("VT: type: %s, Serial Port: %d, ",
	(pVtId->VTtype == 0) ? "None" : "Oxford",
	 pVtId->VTport);

   printf("\nVT State Semaphore: \n");
   printSemInfo(pVtId->pVTmutex,"VT Mutex Semaphore",1);
   printf("\nVT Reg Semaphore: \n");
   printSemInfo(pVtId->pVTRegSem,"VT Regulation Semaphore",1);

   printf("\nVT Cmd Queue (0x%lx): \n",pVtId->pVTMsgQ);
   msgQInfoPrint(pVtId->pVTMsgQ);

   vtPrintMode();
   printf("\nVT Set: %d, True: %d Temperature; Slew Rate: %d; Low Temp Gas: %d, PID: %d\n",
	 pVtId->VTSetTemp,pVtId->VTTruTemp,pVtId->VTTempRate,pVtId->VTLTmpXovr,
	 pVtId->VT_PID);
   printf("\nVT Stat: %d, Reg Cnt: %d, Reg Timeout: %d, Error Type: '%s', Error: %d\n",
	 pVtId->VTstat,pVtId->VTRegCnt,pVtId->VTRegTimeout,
	 ( (pVtId->VTerrorType == HARD_ERROR) ? "Error" : "Warning"),
	 pVtId->VTerror);
   if (pVtId->VTinterLck == 0)
      pIntlkMode = "No";
   else if (pVtId->VTinterLck == 1)
      pIntlkMode = "To Be Enable";
   else
      pIntlkMode = "Yes";
   printf("Interlock Active: '%s', Updaterate: %d sec\n",pIntlkMode,
	     pVtId->VTUpdateRate/sysClkRateGet());
   printf("-------------------------------------------------------------\n\n");


}
vtPrintMode()
{
    char *man, *heat, *gas, *wait, *reg, *fin;

    man = (pTheVTObject->VTModeMask & VT_MANUAL_ON_BIT) ? "Manual" : "Automatic";
    heat = (pTheVTObject->VTModeMask & VT_HEATER_ON_BIT) ? "On" : "Off";
    gas = (pTheVTObject->VTModeMask & VT_GAS_ON_BIT) ? "On" : "Off";
    wait = (pTheVTObject->VTModeMask & VT_WAITING4_REG) ? "On" : "Off";
    reg = (pTheVTObject->VTModeMask & VT_IS_REGULATED) ? "True" : "False";
    fin = (pTheVTObject->VTModeMask & VT_TEMP_IS_SET) ? "Complete" : "Incomplete";

    printf("VT Mode(0x%x): Mode: '%s', Heater: '%s', Gas: '%s'\n",
	pTheVTObject->VTModeMask,man,heat,gas);
    printf("          Regulated: '%s', Wait4Reg: '%s',  Finial Setting: '%s'\n",
	reg,wait,fin);
}
/********************************************************************
* VTShow - display the status information on the VT 
*
*  This routine display the status information of the VT
*
*
*  RETURN
*   VOID
*
*/
VOID VTShow(int level)
/* int level 	   - level of information */
{
   vtShow(pTheVTObject,level);
}


getVTtemp()
{
  int temp;
  if (pTheVTObject != NULL)
  {
    semTake(pTheVTObject->pVTmutex, WAIT_FOREVER);
    temp = pTheVTObject->VTTruTemp;
    semGive(pTheVTObject->pVTmutex);
    return(temp);   /* updated via vtTempReg */
  }
  else
    return(VTOFF);
}

setVTtype(int vttype)
{
  if ((vttype > 0) && (pTheVTObject == NULL))
  {
     vtCreate();
  }
  if (pTheVTObject != NULL)
     pTheVTObject->VTtype = vttype;
}

getVTtype()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTtype);
  }
  else
    return(-1);
}

setVTinterLk(int in)
{
  if (pTheVTObject != NULL)
  {
    /* VTinterLck = in;   /* 1 - enable interlock when ready */
    pTheVTObject->VTinterLck = in;
		     /* 2 - Ready to be tested */
  }
}

getVTinterLk()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTinterLck);
  }
  else
    return(-1);
}

setVTErrMode(int in)
{
  if (pTheVTObject != NULL)
  {
    /* VTerrorType = in;   /* 1 - VT out of Regulation is an Error */
    pTheVTObject->VTerrorType = in;   /* 1 - VT out of Regulation is an Error */
		     		    /* 2 - VT out of Regulation is a Warning */
  }
}

getVTErrMode()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTerrorType);
  }
  else
    return(-1);
}

/*---------------------------------  END of VT ------------------------------------------*/


/*---------------------------------  Start of SPinner & Lock ----------------------------*/

/************************
* Lock interlock
*/
setLKInterlk(int in)
{
  DPRINT1(2,"setLKInterlk: %d\n",in);
  LockinterLck = in;
}

getLKInterlk()
{
  DPRINT1(2,"getLKInterlk: %d\n",LockinterLck);
  return(LockinterLck);
}

setLkErrMode(int in)
{
  LockErrorType = in;   /* HARD_ERROR - VT out of Regulation is an Error */
		     /* WARNING_MSG - VT out of Regulation is a Warning */
}

getLkErrMode()
{
  return(LockErrorType);
}

/*****************************************************************
*  test4VT - checks if VT is present. If VTport<0 it will 
*	     initialize it and then send it a 'S' to get status
*	     If the status request is answered VT is present, if
*	     it times out, it is not present, we close the VTport
*	     and return the TIMEOUT as result
*/
int test4VT()
{
   int	ival;
   VT_ID pVTId;

   if (pTheVTObject == NULL)
   {
       if (vtCreate() == NULL)
	  return(-1);
       else
	  return(0);
   }
}

#ifdef TEST
/*****************************************************************
* test7 - To test anything about VT
*         ----vxwork does not like main()----
*         This routine will be removed later at the time of this 
*         job completed .
*
*/
int testVT ( char  *port )
{
    char buffer[20];
    char  *ival;
    char  cmd;
    int   temp;
    serialPort = initPort( port );

    while (1)
    {
        ival = gets(buffer);
        if( (*ival == '\000') || (*ival == 'q') )
            break;
        cmd = buffer[0];
        temp = atoi( &buffer[1]);
        if( cmd=='M' || cmd=='T' || cmd=='A' || cmd=='P' || cmd=='I' ||
            cmd=='D' || cmd=='Q' || cmd=='O' || cmd=='B' ) 
            OxVT( serialPort, cmd, temp, 0 );  
        if( cmd == 'R' || cmd == 'S' ) 
            VTstat( serialPort, cmd);   

        printf( "End of test7_while(1)\n" );
    }
    printf( "END of test7\n" );
    return(0);
}
#endif
