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
#include <math.h>
#include <msgQLib.h>
#include <semLib.h>
#include <ioLib.h>

#include "logMsgLib.h"
#include "taskPriority.h"
#include "vtfuncs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "Console_Stat.h"
#include "Cntlr_Comm.h"
#include "nexus.h"

#define TIMEOUT 98
#define DONETIMEOUT	-424242

#define LSDV_VT_OFF		0x0000
#define LSDV_VT_NOTPRESENT	0x1800
#define LSDV_VT_UNREG		0x1000
#define LSDV_VT_REG		0x0800

#define vtDPRINT(level, str) \
        if (vtDebugLevel > level) diagPrint(debugInfo,str)

#define vtDPRINT1(level, str, arg1) \
        if (vtDebugLevel > level) diagPrint(debugInfo,str,arg1)

#define vtDPRINT2(level, str, arg1, arg2) \
        if (vtDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2)

#define vtDPRINT3(level, str, arg1, arg2, arg3) \
        if (vtDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3)
 
void checkVTRegulated();
static vtDebugLevel = 0;

// Diagnostic enabling routines
setVtDebug(int level) { vtDebugLevel = level; return level; }
getVtDebug(int level) { return vtDebugLevel; }
vtDebugOn() { vtDebugLevel = 3; return 3; }
vtDebugOff() { vtDebugLevel = 0; return 0; }

extern Console_Stat	*pCurrentStatBlock;	/* Acqstat-like status block */

VT_ID pTheVTObject = NULL;
VT_ID ref_VT       = NULL;
int tVTQTid = (int)NULL;
int tVTAFid = (int)NULL;

startVTTask()
{
     if (vtCreate() == NULL) return; /* return value does not matter */
     tVTQTid = taskSpawn("tvtTask", VT_TASK_PRIORITY, STD_TASKOPTIONS,
		XSTD_STACKSIZE, vtTask, 1,2,3,4,5,6,7,8,9,10);
     if ( tVTQTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn vtTask:");
     }
     tVTAFid = taskSpawn("tvtFlow",VT_TASK_PRIORITY-1,  STD_TASKOPTIONS,
		XSTD_STACKSIZE, wait4VTAirFlow, 1,2,3,4,5,6,7,8,9,10);
     if ( tVTAFid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn vtFlow:");
     }
     
}

DeleteVTTasks()
{
   if (tVTQTid != (int)NULL)
      taskDelete(tVTQTid);
   if (tVTAFid != (int)NULL)
      taskDelete(tVTAFid);
}

/****************************************************************
 * vtShow -- print the various members of the VT_OBJ structure
 *
 */
void vtShow() 
{
register VT_ID p;
   if (pTheVTObject == NULL)
   {  printf("pTheVTObject=Null, run vtCreate\n");
      return;
   }
   p=pTheVTObject;

   printf("SetTemp=%d, TruTemp=%d, TempRate=%d, Range=%d\n",
		p->VTSetTemp,p->VTTruTemp,p->VTTempRate,p->VTRange);
   printf("ModeMask=%d (%x) ", p->VTModeMask,p->VTModeMask);
   printf("LSDV=0x%x ", pCurrentStatBlock->AcqLSDVbits & 0xffff);
   printf("VTAirFlow=%d (%x) ", p->VTAirFlow,p->VTAirFlow);
   printf("VTAirLimits=%d (%x)\n", p->VTAirLimits,p->VTAirLimits);
   printf("LTmpXovr=%d, PID=%d, PrvTemp=%d present=%d\n",
		p->VTLTmpXovr, p->VT_PID, p->VTPrvTemp, p->VTpresent);
   printf("port=%d, interlck=%d type=%d, stat=%d\n",
		p->VTport,p->VTinterLck,p->VTtype,p->VTstat);
   printf("errorType=%s, error=%d, RegCnt=%d, UpdateRate=%d\n",
		(p->VTerrorType==15)?"HARD_ERROR":"WARNING_MSG",
		 p->VTerror, p->VTRegCnt, p->VTUpdateRate);
   printf("RegTimeout=%d, IDSTR=%s\n",
		p-> VTRegTimeout, p->VTIDSTR);
}

#ifndef MODEL
/*****************************************************************
*  Oxford VT controller driver                                            
*  OxVT(object,command,number): command is a single character           
*                              number is usually the temperature       
*****************************************************************/
int OxVT(VT_ID pVtId,char cmd,int temp)
{
    int stat,time_out;
    vtDPRINT2(0,"OxVT: Cmd = %c  Var = %d ", cmd, temp);

    if (pVtId->pVTmutex != NULL)
    {
      semTake(pVtId->pVTmutex, WAIT_FOREVER);
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"OxVT: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    clearport(pVtId->VTport);
    pputchr(pVtId->VTport, cmd);

    /* don't worry about time out here, if we do return then
       the expected value & CR will never be sent */
    cmdecho(pVtId->VTport, CMD);
    time_out = 100;
    switch(cmd)
    {
        case 'B': time_out = 500; break; /* 'B' is a power cycle which */
					 /* takes a long time */
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
        
    if (cmddone(pVtId->VTport, time_out) == -1)
    {
       semGive(pVtId->pVTmutex);
       return(TIMEOUT);
    }
    semGive(pVtId->pVTmutex);
    return(0);
}
 
/*****************************************************************
*  Oxford VT status driver                                                 
*  VTstat(command,prtmod):     command is a single character              
*                              return the status requested 
*                          
*/
int VTstat(VT_ID pVtId,  char cmd)
{
    int value;
    acqerrno = 0;	/* clear errno */
    vtDPRINT1(1,"VTstat: CMD = %c \n",cmd);
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

    clearport(pVtId->VTport);
    pputchr(pVtId->VTport,(int)cmd);            /* send command */
    vtDPRINT2(1,"Send by 'VTstat' :    %d Dec\t     ----> %c\n\n", cmd, cmd);

    /* don't worry about time out here, if we do return then
       the expected returning value will become out of sync 
    */
    cmdecho(pVtId->VTport,CMD);
    value = cmddone(pVtId->VTport,50);   /* cmddone set acqerrno */
    if ((value == -1) && (acqerrno == DONETIMEOUT))
    {
      acqerrno = VTERROR + VTTIMEOUT;
      semGive(pVtId->pVTmutex);
      return(CMDTIMEOUT); 
    }
    vtDPRINT1(1,"VTstat: Value = %d\n",value);
    semGive(pVtId->pVTmutex);
    return(value);
}
#else
/*** these are testing dummies ***/
int modelTargetTemp = 0;

int OxVT(VT_ID pVtId,char cmd,int temp)
{
    int stat,time_out;
    DPRINT2(1,"OxVT: Cmd = %c  Var = %d ", cmd, temp);
    if (cmd == 'T')
      modelTargetTemp = temp;
    return(0);
}

int range=65;
int offset=8;
int regulate=0; 

int vtModel(VT_ID pVtId)
{
   int stupid,value;
   stupid = (rand() % range) - range/2;
   if (regulate == 0)
      if (modelTargetTemp > pVtId->VTTruTemp) 
      {
         value = pVtId->VTTruTemp + stupid + offset;
      }
      else
      {
         value = pVtId->VTTruTemp + stupid - offset;
      }
    else
      {    
         value = pVtId->VTSetTemp + stupid;
      }
   return(value);
}

int VTstat(VT_ID pVtId,  char cmd)
{
    int value;
    acqerrno = 0;	/* clear errno */
    if (pVtId->VTTruTemp == 30000)
      pVtId->VTTruTemp = 12;
    /* this just reset regulated to not jump to the final value */
    if (((pVtId->VTSetTemp - pVtId->VTTruTemp) > 20 ) || 
          ((pVtId->VTSetTemp - pVtId->VTTruTemp) < -20 ))
     {   regulate=0;
     }
    if (cmd == 'R') 
        return(vtModel(pVtId));
   
    if (cmd == 'S')
      if (((pVtId->VTSetTemp - pVtId->VTTruTemp) < 5 ) && 
          ((pVtId->VTSetTemp - pVtId->VTTruTemp) > -5 ))
     {   range = 9; offset=0; regulate=1; return(1);
     }
     else
       {
          range = 65; offset=8; regulate = 0; 
       }
     return(2);
}
    
#endif

/******************************************************************************/
/************************************************************
/**********************************
*
*  vtCreate
*
*
* RETURNS:
* OK - if no error,
* NULL - if mallocing or semaphore creation failed
* NULL - if no serial port or no vt response....
*/ 
VT_ID  vtCreate()
{
   register VT_OBJ *pVtObj;
   int tmpInt;
   int rstat;
   VT_CMD cmd;

  pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_NOTPRESENT;
  pCurrentStatBlock->AcqLSDVbits &= ~LSDV_EBITS; /* clear error bits */
  /* ------- malloc space for VT Object --------- */
  if ( (pVtObj = (VT_OBJ *) malloc( sizeof(VT_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Space:");
    return(NULL);
  }
  memset(pVtObj,0,sizeof(VT_OBJ));
  ref_VT = pVtObj;  /* for debugging */
  vtDPRINT2(0,"VT starts at %lx and ends at %lx\n",pVtObj,pVtObj+sizeof(VT_OBJ));

  pVtObj->VTSetTemp = VTOFF;
  pCurrentStatBlock->AcqVTSet = VTOFF;
  pVtObj->VTTruTemp = VTOFF;
  pCurrentStatBlock->AcqVTAct = VTOFF;
  pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
  pVtObj->VTRange   = 4;  	/* 0.4 degree standard */
  pVtObj->VTTempRate = 125;	/* increase temp by 12.5 degrees per minute */
  pVtObj->VTModeMask = VT_MANUAL_ON_BIT;
  pVtObj->VTport = -1;
  pVtObj->VTinterLck = 0;       /* tin= 'y' or 'n' 0=no, 1=enable, 2=ready*/
  pVtObj->VTtype = 0;		        /* 0 == NONE, 2 = Highland */
  pVtObj->VTerrorType = HARD_ERROR;     /* HARD_ERROR (15) or WARNING_MSG(14)*/
  pVtObj->VTUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE;	/* 3 sec */
  pVtObj->VTport =  open("/TyMaster/1",O_RDWR,0);   /* port 1 on the MIF */
  pVtObj->VTRegTimeout = 0;
  setSerialTimeout(pVtObj->VTport,25);	/* set timeout to 1/4 sec */
  vtDPRINT1(0,"VTport = %d\n",pVtObj->VTport);
  pVtObj->pVTmutex = NULL;		/* VT serial port mutual exclusion */
  pVtObj->pVTmutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);
  if (pVtObj->pVTmutex == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Mutex:");
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

  /* VT Air Flow Semaphore */
  pVtObj->pVTFlowSem =  semBCreate(SEM_Q_FIFO , SEM_EMPTY);
  if (pVtObj->pVTFlowSem == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Flow Sem:");
    vtDelete(pVtObj);
    return(NULL);
  }

  pVtObj->pVTMsgQ = msgQCreate(25,sizeof(VT_CMD),MSG_Q_FIFO);
  if (pVtObj->pVTMsgQ == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Message Q:");
    vtDelete(pVtObj);
    return(NULL);
  }

  strcpy(pVtObj->VTIDSTR,"checking the port now!\n");
  /* just check Status do not initialize */
  /* check Status, acqerrno will be set if can't talk to VT */
  rstat = VTstat(pVtObj,'S'); 
  if (acqerrno == (VTERROR + VTTIMEOUT)) /**** acqerror NO ****/
  {
#ifdef DEBUG
     errLogRet(LOGIT,debugInfo,"vtCreate: Serial Communication to VT Unit timed out, considered Not Present");
#endif
     vtDelete(pVtObj);
     strcpy(pVtObj->VTIDSTR,"No response from the port\n");
     vtDPRINT(0,"Object NULL, no VT present\n");
     return(NULL);
  }

  pVtObj->VTModeMask |=  VT_MANUAL_ON_BIT;
  /* for now I set them here, this should have more user interface  */
  tmpInt = hsspi(2,0x4060000);	/* get current value */
  tmpInt &= 0xFFFF;		/* only thr lower 4 nibbles */
  /* the tmpInt limits are a little bit arbitrary */
  if ( (tmpInt > 0xFF00) || (tmpInt<0x100) ) tmpInt=0xa000; 
  pVtObj->VTAirFlow = tmpInt;	/* default to this flow rate*/
  pVtObj->VTAirLimits = 0x307;	/* default flow limit */
#ifdef XXX
  if (rstat != 1)
  {
     hsspi(2,(0x2<<24) | pVtObj->VTAirLimits);	// LED limits 
     hsspi(2,(0x016<<16) | pVtObj->VTAirFlow);	// set VT air flow
  }
#endif

  pTheVTObject = pVtObj; /* set the static global for VT routines to use */
  vtGetSW(pVtObj);
//  strcpy(pVtObj->VTIDSTR,"VT ready");
  vtDPRINT1(0,"VT: '%s'\n",pVtObj->VTIDSTR);
  return(pVtObj);
}

int setVTInfo(int setpoint, int vtc)
{
   pTheVTObject->VTSetTemp = setpoint;
   pTheVTObject->VTModeMask |= VT_TEMP_IS_SET;
   pCurrentStatBlock->AcqVTSet = setpoint;
   pTheVTObject->VTLTmpXovr = vtc;
   pCurrentStatBlock->AcqVTC = vtc;
}

/**************************************************************
*
*  vtDelete - Deletes VT Object and  all resources
*
* RETURNS:
*  OK or ERROR
*
***************************************************************/
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

   if (pVtId->pVTMsgQ)
       msgQDelete(pVtId->pVTMsgQ);

   if (pVtId->pVTRegSem != NULL)
       semDelete(pVtId->pVTRegSem);

   if (pVtId->pVTFlowSem != NULL)
       semDelete(pVtId->pVTFlowSem);

   free(pVtId);
   pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_NOTPRESENT;
/* memset(pVtId,0,sizeof(VT_OBJ)-130); /* leave some status room */
   return(0);
}


void vtTask()
{
   VT_CMD  cmd;
   int     bytes,check;
   int     timeout;
   /* this code allows functionality if the controller is up at 
      the start in manual mode */
   /* it acts normally */
   if (pTheVTObject != NULL) 
     timeout = pTheVTObject->VTUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE;
   else
     return;
   FOREVER
   {
      bytes = msgQReceive(pTheVTObject->pVTMsgQ, (char*) &cmd,
                          sizeof( VT_CMD ), pTheVTObject->VTUpdateRate);
      /* DPRINT1(0,"vtTask: Recv: %d bytes\n",bytes); */
      /* command then act on it, otherwise just do normal chores */
      if (pTheVTObject == NULL)
      {
         DPRINT(1,"Attempting to start VT!\n");
         if (!vtCreate())
	 {
            DPRINT(1,"NO VT AVAILABLE!\n");
            timeout = WAIT_FOREVER;
         }
      }
      /* this looks a bit strange */
      if (pTheVTObject != NULL)
      {
         if (bytes > 0) 
	 {
            VtDecodeCmd(pTheVTObject, (VT_CMD) cmd); 
         }
         vtTempReg(pTheVTObject);
         timeout = pTheVTObject->VTUpdateRate;
	 /* execute the periodic check of the status */
      }
   } /* forever */
}

void resetVT()
{
VT_CMD	cmd;
   if (pTheVTObject == NULL)
   {
     vtCreate();
   }

   if (pTheVTObject != NULL)
   {
      cmd.VTCmd  = VT_RESET;
      cmd.VTArg1 = VT_NORMAL_UPDATE;
      msgQSend(pTheVTObject->pVTMsgQ, (char *)&cmd, sizeof(cmd), 
					NO_WAIT, MSG_PRI_NORMAL);
   }
}

setVT(int temp, int ltmpxoff, int pid)
/* set temperature,low tmp_cutoff,PID flag */
{
   int old_stat;
   int rstat;
   VT_CMD cmd;

   vtDPRINT3(0,"setVT: Temp: %d, ltmpxoff: %d, pid: %d\n",temp,ltmpxoff,pid);
   /* if VT Object not present */
   if (pTheVTObject == NULL)
   {
     vtCreate();
   }

   if (pTheVTObject != NULL)
   {
      if (pid > 0)
      {
         cmd.VTCmd = VT_SETPID;
         cmd.VTArg1 = pid;
         msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),
					NO_WAIT,MSG_PRI_NORMAL);
      }

      cmd.VTCmd = VT_SETTEMP;
      cmd.VTArg1 = temp;
      cmd.VTArg2 = ltmpxoff;
      msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),	
					NO_WAIT,MSG_PRI_NORMAL);
     /* if setting VT to off wait for temp to come to ambient temp, 
	arbitrary < 31 C  */
     if (temp == VTOFF)
     {
          old_stat = pCurrentStatBlock->Acqstate;
          pCurrentStatBlock->Acqstate = ACQ_VTWAIT;
//          getstatblock();   /* force statblock upto hosts */

          /* wait 10 min */
          semTake(pTheVTObject->pVTRegSem,NO_WAIT);    /* make sure its empty */
          semTake(pTheVTObject->pVTRegSem,(600 * sysClkRateGet()));

          if (pCurrentStatBlock->Acqstate != ACQ_IDLE)  // may of aborted in the mean time, so check if IDLE
             pCurrentStatBlock->Acqstate = old_stat;
//          getstatblock();   /* force statblock upto hosts */
     }
   }
   return(0);
}

/*----------------------------------------------------------------------*/
/*  wait4VT(time) - wait for time(seconds) then check VT, if not        */
/*                      in reg then return failure                      */
/*                  0 = not in regulation                               */
/*                  1 = in regulation                                   */
/*  always called if tin='y' so when we are finished set VTinterlk to 2 */
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

   checkVTRegulated();
   old_stat = pCurrentStatBlock->Acqstate;
   pCurrentStatBlock->Acqstate = ACQ_VTWAIT;
//   getstatblock();   /* force statblock upto hosts */

   cmd.VTCmd = VT_WAIT4REG;
   cmd.VTArg1 = VT_WAITING_UPDATE;
   msgQSend(pTheVTObject->pVTMsgQ,(char*) &cmd,sizeof(cmd),
			NO_WAIT,MSG_PRI_NORMAL);

   timeout = time_arg * sysClkRateGet();
   vtDPRINT1(1,"wait4VT: Timeout in %d Seconds, Waiting\n",time_arg);
   semTake(pTheVTObject->pVTRegSem,NO_WAIT);    /* make sure its empty */
   if ( (rstat = semTake(pTheVTObject->pVTRegSem,timeout)) == ERROR)
   {
     rstat = VTERROR + VTREGFAIL;
   }
   else
     rstat = 0;

   if (pCurrentStatBlock->Acqstate != ACQ_IDLE)  // may of aborted in the mean time, so check if IDLE
       pCurrentStatBlock->Acqstate = old_stat;
//   getstatblock();   /* force statblock upto hosts */

   return(rstat);
}


ChangeVtTemp(VT_ID VtId, int trutemp)
{
   int rstat, sign, ntemp, deltatemp;


   if ( (VtId->VTModeMask & VT_TEMP_IS_SET) || 
	((VtId->VTRegTimeout != 0L) && (VtId->VTRegTimeout > time(NULL)))
      )
   {
      vtDPRINT1(1,"ChangeVtTemp, VT Setting Complete? %d\n",
			(VtId->VTModeMask & VT_TEMP_IS_SET));
      vtDPRINT2(1,"ChangeVtTemp, Min. not up Yet. Now: 0x%lx, TimeOut: 0x%lx\n",
			time(NULL),VtId->VTRegTimeout);
      return(0);
   }
     
   if (VtId->VTPrvTemp >= (VTOFF / 2)  )
   {
      vtDPRINT2(1,"VTPrvTemp reset from %d to %d\n",VtId->VTPrvTemp,trutemp);
      VtId->VTPrvTemp = trutemp;
   }

   /* if (VtId->VTTruTemp > VtId->VTSetTemp) sign = -1; else sign = 1; */
   if (VtId->VTPrvTemp > VtId->VTSetTemp) sign = -1; else sign = 1;

   /* increase temp by 12.5 degrees per minute */
   /* 125 / 60 * rate / clock rate */
   deltatemp = VtId->VTTempRate;

   /* If the differnece in True & Set is greater than delta temp rate 
   /* then change Temp by the Rate, Else Set to Final Temp Setting
   */

   /*  did it 10/7/99 ... TODO: don't use tru temp to add to but rather
   /*  the next step in rate, so that the number of times to change temp 
   /*  is deterministic 
   */
   /*  VTPrvTemp is initially set to VTTruTemp, use VTPrvTemp instead 
   /*  of VTTruTemp to prevent the set point (VTPrvTemp) for going 
   /*  beyond the set temp(VTSetTemp) that was possible before.
   */
   vtDPRINT3(1,"PresTemp %d, IncTemp %d, SetTemp %d\n",
			VtId->VTTruTemp,VtId->VTPrvTemp,VtId->VTSetTemp);
   /* if the chinage in temp > 12.5 degrees */
   if ( abs(VtId->VTPrvTemp - VtId->VTSetTemp) > deltatemp)  
   { 
	ntemp =  VtId->VTPrvTemp += sign*deltatemp;

        VtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */

        vtDPRINT2(1,"Incremental Temp Change to %d,  final temp %d\n",
			ntemp,VtId->VTSetTemp);
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
           vtDPRINT(1,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           vtDPRINT(1,"VT Gas OFF\n");
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
      else /* change in temp < 12.5 degrees */
      {
        vtDPRINT1(0,"Set final VT temp %d\n",VtId->VTSetTemp);
        VtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */
        if ( (rstat = OxVT(VtId,'M',0))  ||           /* back to manual mode */
             (rstat = OxVT(VtId,'Q',222))||           /* No spontanous statmsg*/
             (rstat = OxVT(VtId,'T',VtId->VTSetTemp)) /* program desired temp */
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
           vtDPRINT(1,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           vtDPRINT(1,"VT Gas OFF\n");
           VtId->VTModeMask &= ~VT_GAS_ON_BIT;  /* Gas Off */
        }


        if (rstat = OxVT(VtId,'A',0))
        {
	   VtId->VTerror = VTERROR + rstat;
           return(VTERROR + rstat);/*automod,tmp 4 display*/
	}
	VtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	VtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	VtId->VTModeMask |= VT_TEMP_IS_SET;  /* VT was set to the final Temp */

    }
  return(0);
}

/********************/
/*  */
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
     /*** this code executes after each command and each update time out ****/
    
     /* VTstat has the semaphores */
       VtId->VTstat = vtstat = VTstat(VtId,'S');	
       rstat = VTstat(VtId,'R');
       if (rstat == CMDTIMEOUT)
       {
          VtId->VTstat = vtstat = rstat;
       }
       else
       {
          VtId->VTTruTemp = trutemp = rstat;
          pCurrentStatBlock->AcqVTAct=trutemp;
       }
       temp = VtId->VTSetTemp;
       /* DPRINT3(1,"vtTempReg: Set Temp = %d, Tru Temp: %d VT stat: %d\n", 
       /*	temp,trutemp,vtstat); */

       if (temp != VTOFF)
       {
          switch(vtstat)
          {   
            case 0:         /* In Manual Mode ?? */
       	     VtId->VTerror = 0;
             pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET ) 
             {
	        ChangeVtTemp(VtId, trutemp);
  	     }
             else if ( (trutemp <= temp + 5) &&
                       (trutemp >= temp - 5) )
             {
	       /* OK ignore the fact VT is in manual mode, 
	       /* and count this as regulated */
               VtId->VTRegCnt++; 
               pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_REG;
             }
 
             break;
                         
            case 1:         /* Temp Stable  */
       	     VtId->VTerror = 0;
             /* use these tighter standards for VT doesn't slip out of */
             /* regulation  while acquiring       */
	     /* just in case never set */
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET )
             {
	        ChangeVtTemp(VtId, trutemp);
  	     }
             else if ( (trutemp < temp + VtId->VTRange) &&  
                       (trutemp > temp - VtId->VTRange) )
	     /* i.e., 30, 29.7-30.3 */
             {
               VtId->VTRegCnt++; /* count this as regulated */
               pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
               pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_REG;
             }
             break;
  
            case 2:         /* Temp Changing  */
             // if it should be regulated and has been lost 
             // set the error and mark out of regulation
             // only do once! 
             if (VtId->VTModeMask & VT_IS_REGULATED)
             {
       	       VtId->VTerror = VTERROR + VTOUT;
             }
       	     else
               VtId->VTerror = 0;
 
             VtId->VTModeMask &= ~VT_IS_REGULATED;
             VtId->VTRegCnt = 0; /* reset back to zero */
             pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
             pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_UNREG;
	     ChangeVtTemp(VtId, trutemp);
             break;

            case 3:         /* Safety sensor limiting Output */
             VtId->VTerror = VTERROR + VTSSLIMIT;
             break;
 
            case -1:        /* Gas not flowing or o/p stage fault */
             pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
             pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_UNREG;
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
            case TIMEOUT:	    /* timeout */
            case CMDTIMEOUT:	    /* timeout */
             pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;
             pCurrentStatBlock->AcqLSDVbits |= LSDV_VT_UNREG;
	     VtId->VTerror = VTERROR + TIMEOUT;
             break;
          }
          vtDPRINT3( 1,"vtTempReg: --------> Temp: %d, VTstat: %d, RegCnt: %d\n",
			           trutemp,vtstat,VtId->VTRegCnt);
          if ( (VtId->VTerror != 0) && (VtId->VTinterLck > 0) )
          {
             VtId->VTinterLck = 0;
             if  (VtId->VTerrorType == HARD_ERROR) 
                sendException(HARD_ERROR,VtId->VTerror, 0, 0, NULL);
             else if (VtId->VTerrorType == WARNING_MSG)
                sendException(WARNING_MSG,VtId->VTerror, 0, 0, NULL);
          }
       }
       else  /* turn off VT */
       {
         if (VtId->VTModeMask & VT_HEATER_ON_BIT)
         {
             VtId->VTTruTemp =  (short) VTOFF;
             VtId->VTSetTemp =  (short) VTOFF;
             pCurrentStatBlock->AcqVTSet = VTOFF;
             vtDPRINT(1,"vtTempReg: VTOFF & Heater On, Turn Off VT\n");
             if ( (rstat=OxVT(VtId,'Q',222))|| /* No spontanous stat msgs*/
                (rstat=OxVT(VtId,'M',0))  || /* back to manual mode */
                (rstat=OxVT(VtId,'O',0))  || /* besure heaters are off */
                (rstat=OxVT(VtId,'G',0))       /* besure gas value is off*/
              ) 
                return(VTERROR + rstat);
	    /* MANUAL, HEater Off, Not Regulated */
            VtId->VTModeMask = VT_MANUAL_ON_BIT | VT_WAITING4_REG;  
       	    VtId->VTerror = 0;
         }

         /* VT Off both bits unset (0) */
         pCurrentStatBlock->AcqLSDVbits &= ~LSDV_VT_NOTPRESENT;

	 if ((VtId->VTModeMask & VT_WAITING4_REG) &&  (trutemp < 310))
         {
             vtDPRINT(1,"vtTempReg: VTOFF, WAITING4VT to go Ambient Temp, So Give Semaphore\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
	          semGive(VtId->pVTRegSem);
         }
         else
         {   vtDPRINT(1, "vtTempReg: VTOFF, no-WAITING4VT, Give Semaphore\n");
             semGive(VtId->pVTRegSem);
         }

       } /* if (temp != VTOFF) */

      /* if not regulated yet && Final Temp has been Set, 
      /* and Temp stable for 30 sec */
      /*
	 Now If:
		1. VT is NOT Regulated
	        2. VT Final Temp Has Been Set
	        3. VT has Been Stable for 30 Sec  (VTRegCnt > 30)
	     Then
		Set Bit indicated VT is Regulated, and return to normal update rate
	      
		If wait4VT() has been called 
                   then reset bit flag and give the semaphore it is waiting on.

		   If The interlock is on STANDBY- 1, 
		      then set it to READY- 2 
      */
      if (  ((VtId->VTModeMask & VT_IS_REGULATED) != VT_IS_REGULATED) && 
	    (VtId->VTModeMask & VT_TEMP_IS_SET) && 
	    ( (VtId->VTRegCnt > 30) || (VtId->VTerrorType == 0)) ) /* if stable 30 sec, then its OK */
      {
          if (VtId->VTRegCnt > 30)
          {
             vtDPRINT(1,"vtTempReg: SET VT to Regulated, and reset UpdateRate\n");
             VtId->VTModeMask |= VT_IS_REGULATED;
	     VtId->VTUpdateRate = VT_NORMAL_UPDATE * sysClkRateGet();
	     VtId->VTRegCnt = 0;
          }
          else
          {
             vtDPRINT(1,"vtTempReg: VT not regulated but error is ignore\n");
             VtId->VTRange   = 4;  	/* 0.4 degree standard */
          }
	  /* If waiting then give semaphore  */
	  if (VtId->VTModeMask & VT_WAITING4_REG)
          {
             if (VtId->VTinterLck == 1) /* ENABLE */
                  VtId->VTinterLck = 2; /* READY to Check, & report Errors  */

             vtDPRINT(1,"vtTempReg: WAITING4VT DONE .... clear modemask\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
	     semGive(VtId->pVTRegSem);
          }
      }
}

int VTsetPID(VT_ID pVtId,int pid)
{
   int pdigit,idigit,ddigit,rstat;
   rstat = 0;
   if (pVtId == 0)
      return(0);
   if ( (pid > 0)  && (pid != pVtId->VT_PID) )
   {
      pdigit = pid / 100;
      idigit = (pid % 100) / 10;
      ddigit = (pid % 10);
      rstat = 0;
      switch(1)	/* so we have something to break out of */
      {
	case 1:
           if (rstat = OxVT(pVtId,'M',0)) break; /* back to manual mode */

       	   pVtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */

           if (rstat = OxVT(pVtId,'Q',222)) break;    /* No spontanous statmsg*/
           if (rstat = OxVT(pVtId,'P',pdigit)) break; /* Proportional     0-6 */
           if (rstat = OxVT(pVtId,'I',idigit)) break; /* Integral value   0-9 */
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
      rstat = 1;
   }
   else
      rstat = 0;
   return(rstat);
}
         
VtDecodeCmd(VT_ID pVtId, VT_CMD *pCmd)
{
   int pid,pdigit,idigit,ddigit;
   int rstat,slew,cmd;
   int vtmask;

   switch(pCmd->VTCmd)
   {

     case VT_INIT:
       vtDPRINT(0,"VtDecodeCmd: VT Initialize Command\n");
       pVtId->VTUpdateRate = pCmd->VTArg1 * sysClkRateGet();
       pVtId->VTerror = 0;
       break;

     case VT_SETTEMP:
      vtDPRINT2(0,"VtDecodeCmd: VT_SETTEMP: Present Set Temp: %d, New Temp %d\n",
		               pVtId->VTSetTemp,pCmd->VTArg1);
      pVtId->VTerror = 0;
      /* if setting to same temperature & VT is already regulated at this temp
      /*  then don't bother to set temp, etc..
      */
      if (pVtId->VTSetTemp != pCmd->VTArg1)
      {
         pVtId->VTSetTemp = pCmd->VTArg1;
         pCurrentStatBlock->AcqVTSet = pVtId->VTSetTemp;
         pVtId->VTPrvTemp = pVtId->VTTruTemp;	/* set start temp of ramp  */
         pVtId->VTLTmpXovr = pCmd->VTArg2;
         pCurrentStatBlock->AcqVTC = pCmd->VTArg2;
         pVtId->VTModeMask &= ~VT_IS_REGULATED;	/* turn off regulated bit */
         pVtId->VTModeMask &= ~VT_TEMP_IS_SET;	/* turn off temp set bit */
         pVtId->VTModeMask &= ~VT_WAITING4_REG;
         pVtId->VTModeMask &= ~VT_GAS_ON_BIT;
         pVtId->VTRegCnt = 0L;
         pVtId->VTRegTimeout = 0L;
         if (pVtId->VTinterLck == 2 /* READY */)
	    pVtId->VTinterLck = 1;	/* STANDBY */
      }
      else
      {
         vtDPRINT(0,"VT_SETTEMP: already at set temperature, no action taken\n");
      }

      break;

      /* this appears to be unused */
     case VT_SETPID:
      vtDPRINT1(0,"VtDecodeCmd: Set PID to %d\n",pCmd->VTArg1);
      pVtId->VTerror = 0;
      VTsetPID(pVtId,pCmd->VTArg1);
         
      break;
     
     case VT_RESET: 
       vtDPRINT(0,"VT_RESET\n");
       hsspi(2,( 0x02<<24) | pVtId->VTAirLimits);    // LED limits 
       hsspi(2,(0x016<<16) | pVtId->VTAirFlow);   // set VT air flow
       pVtId->VTSetTemp = VTOFF;
       pCurrentStatBlock->AcqVTSet = VTOFF;
       pVtId->VTTruTemp = VTOFF; 
       pVtId->VTModeMask = VT_MANUAL_ON_BIT;  
       /* MANUAL ON, turn off regulated bit, temp set bit */
       pVtId->VTRegCnt = 0L;
       pVtId->VTRegTimeout = 0L;
       pVtId->VTinterLck = 0;
       pVtId->VTUpdateRate = VT_NORMAL_UPDATE * sysClkRateGet();
       OxVT(pVtId,'M',0);
       OxVT(pVtId,'B',0);
     break;

     case VT_GETTEMP:
      	rstat = VTstat(pVtId,'R');
        if (rstat != CMDTIMEOUT)
      	   pVtId->VTTruTemp = rstat;
        vtDPRINT1(0,"VtDecodeCmd: Get VT Temp %d\n", pVtId->VTTruTemp);
      break;

     case VT_GETSTAT:
       pVtId->VTstat = VTstat(pVtId,'S');
       vtDPRINT1(0,"VtDecodeCmd: Get VT Stat: %d\n", pVtId->VTstat);
       break;

     case VT_WAIT4REG:
            vtDPRINT1(0,"VtDecodeCmd: Waiting to Regulate, Update Rate: %d sec\n"
				          ,pCmd->VTArg1);

	    if ( (pVtId->VTModeMask & VT_IS_REGULATED) != VT_IS_REGULATED )
            {
      	      pVtId->VTerror = 0;
	      pVtId->VTModeMask |= VT_WAITING4_REG;
              pVtId->VTUpdateRate = pCmd->VTArg1 * sysClkRateGet();
            }
            else
            {
              if (pVtId->VTinterLck == 1) /* ENABLE */
                  pVtId->VTinterLck = 2;  /* READY to Check, & report Errors  */
              semGive(pVtId->pVTRegSem);/* setVT waits if VTOFF, give it */

               vtDPRINT(0,"VtDecodeCmd: already Regulated\n");
            }
	    break;

     case VT_SETSLEW:		/* HiLand only */
          vtDPRINT1(0,"VtDecodeCmd: Set VT Temp Slew Rate: %d \n",pCmd->VTArg1);
      	  pVtId->VTerror = 0;
          slew = pCmd->VTArg1;
          vtmask = pVtId->VTModeMask;
          /* back to manual mode */
          if (rstat = OxVT(pVtId,'M',0)) break;
          /* MANUAL ON */
          pVtId->VTModeMask |= VT_MANUAL_ON_BIT;  
          /* HiLand VT Slew rate nnnn/10 Degrees/Min 6 */
          if (rstat = OxVT(pVtId,'K',slew)) break; 
          /* was in automatic, so return it to such */
	  if ((vtmask & VT_MANUAL_ON_BIT) == 0)
	  {
             rstat = OxVT(pVtId,'A',0);
	     pVtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	     pVtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	  }
          break;
     case VT_SET_INTERLOCK:  pVtId->VTinterLck = pCmd->VTArg1; 
          vtDPRINT1(0,"vt set interlock to %d\n",pCmd->VTArg1); 
          break; 
     case VT_SET_TYPE:  pVtId->VTtype = pCmd->VTArg1;  
          vtDPRINT1(0,"vt set type to %d\n",pCmd->VTArg1); 
          break; 
     case VT_SET_ERRORMODE: pVtId->VTerrorType = pCmd->VTArg1; break;
     case VT_SETCALIB:		/* HiLand only */
          errLogRet(LOGIT,debugInfo,
		"VtDecodeCmd: VT_SETCALIB not implemented yet.");
          break;

     case VT_GETHEATPWR:	/* HiLand only */
          errLogRet(LOGIT,debugInfo,
		"VtDecodeCmd: VT_GETHEATPWR not implemented yet.");
          break;
          
     case VT_GETSWVER:		/* HiLand only ???*/
          vtGetSW(pVtId); 
          break;
     case VT_SET_RANGE:
	  if (pCmd->VTArg1 > 0)
             pVtId->VTRange = pCmd->VTArg1;
          break;
      case VT_SET_SLEW:
	  if (pCmd->VTArg1 > 10)
             pVtId->VTTempRate = pCmd->VTArg1;
          break;
   default: printf("Could not decode %d  %d  %d  %d\n",
               pCmd->VTCmd,pCmd->VTArg1,pCmd->VTArg2,pCmd->VTArg3);
             

     }
}


void setVTAirFlow(int flow)
{
  if (pTheVTObject == NULL) return;
  pTheVTObject->VTAirFlow = flow;
  pCurrentStatBlock->AcqPneuVtAir = flow;
  hsspi(2,(0x016<<16) | pTheVTObject->VTAirFlow);     // set VT air flow
}

static int lpmReq = 0;

void newVTAirFlow(int flowRequested)
{
    lpmReq = flowRequested;
    if (pTheVTObject == NULL) return;
    semGive(pTheVTObject->pVTFlowSem);
}

void wait4VTAirFlow()
{
    while(1)
    {
       semTake(pTheVTObject->pVTFlowSem,WAIT_FOREVER);
       cntrlVTAirFlow();
    }
}

/* meter is 0-25 l/min. 10 LEDs ==>> 3 / LED.  */
/* if 2 LED are lit, we're in between, ==>>    */
/* resolution is 1.5 l/min                     */
cntrlVTAirFlow()
{
double lpmDel,lpmAct;
int delta, flow, i, stata, stat,trial;
   if (pTheVTObject == NULL) return;
   if (lpmReq == 0) // zero is an exception
   {
       hsspi(2,(0x016<<16) );
       pTheVTObject->VTAirFlow = 0;
       pCurrentStatBlock->AcqPneuVtAir = 0;
       return;
   }
   flow = pTheVTObject->VTAirFlow;
   vtDPRINT1(0,"cntrlVtAirFlow: start at %d\n",flow);
   trial = 0;
   while ( (trial<100) )	// after 25 sec, quit.
   {
      stata = hsspi(2,0x03000000);
      if ( ! (stata & 0x800000) ) return;	// solids mode, can't control
      stat = ( (~stata) >> 13) & 0x3FF;
      vtDPRINT2(0,"stata =0x%x, stat=0x%x\n", stata, stat);
      if (stat == 0) return;
      lpmAct = 0; i=9;
      while (! ((stat>>i)&1) )
      {  
        
         lpmAct += 3;
       
         i--;
      }
      i--;
      if ( (stat>>i) > 2) lpmAct += 1.5;
      lpmDel = lpmAct - (double)lpmReq;
      vtDPRINT1(0,"lpmDel=%f\n",lpmDel);
      if ( (lpmDel < 1.1) && (lpmDel > -0.1) ) break;
      if ( fabs(lpmDel) > 2.0) delta=0x400;
      else delta=0x100;
      if (lpmReq > lpmAct) flow += delta;
      else                 flow -= delta;
      if (flow>65500) flow =65500;
      if (flow<100)   flow=100;
      hsspi(2,(0x016<<16) | flow);
      if (flow > 65450) break;
      trial++;
      vtDPRINT3(2,"cntrlVtAirFlow: %d  at %d or %f lpm\n",trial, flow, lpmAct);
      taskDelay(calcSysClkTicks(249));  /* 249 ms, taskDelay(15); */
   }
   pTheVTObject->VTAirFlow = flow;
   pCurrentStatBlock->AcqPneuVtAir = flow;
   vtDPRINT3(0,"cntrlVtAirFlow: final: %d  at %d or %f lpm\n",trial, flow, lpmAct);
}

void setVTAirLimits(int limit)
{
  if (pTheVTObject == NULL) return;
  pTheVTObject->VTAirLimits = limit;   
  pCurrentStatBlock->AcqPneuVTAirLimits = limit;
  hsspi(2,(0x2<<24) | pTheVTObject->VTAirLimits);      // LED limits 
}

int getVTAirLimits()
{
  return( (pTheVTObject == NULL) ? 0 : pTheVTObject->VTAirLimits );
}

void setVTtype(int vttype)
{
   if ((vttype > 0) && (pTheVTObject == NULL))
   {
      startVTTask();
   }
   
   if (pTheVTObject != NULL) 
      pTheVTObject->VTtype = vttype;
}

void checkVTRegulated()
{
   vtDPRINT(1,"checkVTRegulated\n");
   if (pTheVTObject != NULL)
   {
      vtDPRINT3(1,"VTTruTemp:%d  VTSetTemp:%d Range:%d\n",
                 pTheVTObject->VTTruTemp,pTheVTObject->VTSetTemp,pTheVTObject->VTRange);
      if ( (pTheVTObject->VTTruTemp > pTheVTObject->VTSetTemp + pTheVTObject->VTRange) ||
           (pTheVTObject->VTTruTemp < pTheVTObject->VTSetTemp - pTheVTObject->VTRange) )
      {
         vtDPRINT(1,"Not regulated\n");
         pTheVTObject->VTModeMask &= ~VT_IS_REGULATED;
         pTheVTObject->VTRegCnt = 0; /* reset back to zero */
      }
   }
}

setVTErrMode(int in)
{
   if (pTheVTObject != NULL)
   {
      pTheVTObject->VTerrorType = in; /* 1=VT out of Regulation is an Error */
                                      /* 2=VT out of Regulation is a Warning */
      if (in)
         pTheVTObject->VTRange = 4;
      else
         pTheVTObject->VTRange = 40;
   }
}

setVTinterLk(int in)
{
   if (pTheVTObject != NULL)
   {
      pTheVTObject->VTinterLck = in;  /* 1 = enable interlock when ready */
                                      /* 2 = Ready to be tested */
   }
}


int
vtGetSW(VT_ID pVtId)
{
   int cmd,rstat;
    char *pChr;

   clearport( pVtId->VTport );
   cmd = (int) 'V';
   pputchr(pVtId->VTport, cmd);            /* send command */
      
   cmdecho(pVtId->VTport,CMD);
   rstat = readreply(pVtId->VTport,300,pVtId->VTIDSTR,130);
   pChr=pVtId->VTIDSTR;
   while ( *pChr != '\n') pChr++;
   *pChr = 0;
   logMsg("vtGetSW: \n'%s', value: %d\n",pVtId->VTIDSTR,rstat);
   if (rstat) 
      errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_GETSWVER failed.\n");
   return(rstat);
}

#ifdef STUFF_TODO
vtGetResis(VT_ID pVtId)
{
   int value;
   if (pVtId != NULL)
   {
     value = VTstat(pVtId,'J');
     /* printf("Probe Heater Resistence: %d\n",value); */
     return(value);
   }
   return(-1);
}
vtGetHeater(VT_ID pVtId)
{
   int value;
   if (pVtId != NULL)
   {
     value = VTstat(pVtId,'H');
     /* printf("HighLand Heater Power: %d\n",value); */
     return(value);
   }
   return(-1);
}

vtGetZ(VT_ID pVtId)
{
   int value;
   char cmd;
   char zStr[258];
   if (pVtId != NULL)
   {
     clearport( pVtId->VTport );
     cmd = 'Z';
     pputchr(pVtId->VTport,(int)cmd);            /* send command */
     cmdecho(pVtId->VTport,CMD);
     value = readreply(pVtId->VTport,300,zStr,257);
     zStr[258] = '\0';
     printf("vtGetZ:\n'%s'\n",zStr);
     return(value);
   }
   return(-1);
}
#endif

