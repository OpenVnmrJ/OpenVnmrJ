/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* vtfuncs.c  11.1 07/09/07 - VT function & Spinner Source Modules */
/* 
 */


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <msgQLib.h>
#include <taskLib.h>

#include "hardware.h"
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

/*  VT controller moves to the MSR 
**  Phil Hornung adapts 12/2/98
** the move to the MSR CARD.... for the PPC / 162i
** we place the vt structure on the msr shared memory
** the msr keeps the structure updated. 
** If the PPC wants to change something in the structure it sends
** message and then updates from its side... 
** This means the structure appears as READ ONLY from the 
** PPC/162 side.
*/

/**/

VT_ID pTheVTObject = NULL;
VT_ID ref_VT = NULL;

static int SpininterLck = 0;  /* in = 'y' or 'n' */
static int SpinErrorType = HARD_ERROR;	/* HARD_ERROR == Error, WARNING_MSG == Warning */
static int SpinMASThreshold = 80;	/* Above this spin speed switch to MAS solids spinner control */
static int SpinDeltaReg = 1;	/* speed (Hz) above or below spinner setting speed is out of regulation */

static int LockinterLck = 0;  /* in = 'y' or 'n' */
static int LockErrorType = HARD_ERROR; /* HARD_ERROR == Error, WARNING_MSG == Warning */

int vt_emask=0;

int vtCreate()
{
  /* here we point to the shared memory and 
     ask the MSR to Start Work on the VT */
  /* test the AutoObj.... */
  VT_ID pTemp;
 
  if (pTheAutoObject == NULL) 
  {
      return(NULL);
  }
  /*  if the MSR is not there, the object may be a stub */
  /*  test for the stub - fail if stub */
  if (pTheAutoObject->autoSharMemAddr == 0xffffffff)
  {
     return(NULL);
  }
  pTemp = (VT_ID) AUTO_SHARED_VT(pTheAutoObject->autoSharMemAddr);
  ref_VT=pTemp; 
  if (pTemp->VTport > 0) 
  {
     DPRINT(0,"MSR VT IS UP!\n");
     pTheVTObject = pTemp; /* is working */
     return(1);  /* succeeded easily */
  }
  else
  {
     DPRINT(0,"MSR VT PROBE CHECK !\n");
     /* send a vt probe message */
     /* see if one is there now ... no FAIL yes return */
     if ((probeVT() > 0) || (pTemp->VTport > 0))
     {
	pTheVTObject = pTemp;
        DPRINT(0,"MSR VT CAME UP!\n");
	return(2);  /* it started up */
     }
  }
  /* NOTPRESENT both bits set (3) */
  setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
  currentStatBlock.stb.AcqVTAct = (short) VTOFF;
  currentStatBlock.stb.AcqVTSet = (short) VTOFF;
  DPRINT(0,"Probed but still no VT\n");
  return(0); /* still another way to fail */
}

int
getVTtype()
{

  if (pTheVTObject != NULL)
    return(pTheVTObject->VTtype);
  return(-1);
}

int
getVTinterLk()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTinterLck);
  }
  else
    return(-1);
}

int
getVTRegRange()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTRange);
  }
  else
    return(-1);
}

int
getVTSlewRate()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTTempRate);
  }
  else
    return(-1);
}

int
getVTPID()
{
  if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VT_PID);
  }
  else
    return(-1);
}

/* reports an error from the data structure... */
int
getVTErrMode ()
{
 DPRINT(1,"getVTErrMode(): starting\n");;
 if (pTheVTObject != NULL)
  {
    return(pTheVTObject->VTerrorType);
  }
  else
    return(-1);
}

/*----------------------------------------------------------------------*/
/*  VTchk() - check VT for regulation                                   */
/*              return 1 if in regulation                               */
/*              return 0 if not BUT OK                                  */
/*              return VTERROR+CODE if sick                             */
/*              monitor.c has been changed to reflect this              */           
/*----------------------------------------------------------------------*/
/* this is a test stub */

#ifdef VTTEST
int vtfudge=0;
#endif VTTEST


VTchk()      
 {  
#ifdef VTTEST 
   /* this is a test stub to be removed */
    if (vtfudge < 0)
    {
      pTheVTObject->VTerror = 0;
      vtfudge=0;
    }
    if (vtfudge != 0)
    {
      pTheVTObject->VTerror = vtfudge; 
    }
#endif VTTEST            
    if (pTheVTObject == NULL)
      return(0);    /* no vt */
    /* check for hard errors - no hard errors report VTstat */
    if (pTheVTObject->VTerror < (VTERROR + VTSSLIMIT))
      return(pTheVTObject->VTstat);
    else
      return(pTheVTObject->VTerror);
}    

void setVT_LSDVbits()
{
  /* ---- Obtain VT status --- */
    int eno,ww;
    if ( pTheVTObject == NULL )
    {
	/* NOTPRESENT both bits set (3) */
	clearLSDVbits( LSDV_EBITS); /* report no errors */
	setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
	DPRINT(2,"Object NULL, no VT presemt\n");
        return;
    }
  
    if ( (currentStatBlock.stb.AcqVTSet == VTOFF) )
    {
      /* VT Off both bits unset (0) */
      clearLSDVbits( LSDV_EBITS | LSDV_VTBIT1 | LSDV_VTBIT2);
      DPRINT(2,"setVT_LSDVbit: VT Set Off\n");
      return;
    }
  
    if (pTheVTObject->VTport < 1 ) /* be sure VT is there first */
    {
      /* NOTPRESENT both bits set (3) */
      clearLSDVbits( LSDV_EBITS); /* report no errors */
      setLSDVbits( LSDV_VTBIT1 | LSDV_VTBIT2);
      DPRINT(2,"setVT_LSDVbit: VT not present \n");
      return;
    }
    clearLSDVbits(LSDV_EBITS);
    /* vttype == 0 implies no interest in errors or LSDV ?? */
    if (vt_emask > 0)
    {
       /* start problem handler */
       
       eno = pTheVTObject->VTerror;
       if ((eno >= VTERROR+VTSSLIMIT) && (eno <= VTERROR+VT_OC_SS))
       {  
            ww = ((pTheVTObject->VTerror - (VTERROR+VTSSLIMIT) + 1) << 13) & LSDV_EBITS;
            setLSDVbits((ushort) ww);
       }
       /* end problem handler */
       if (pTheVTObject->VTstat == 1)       /* == 1, in Regulation */
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
   return;
}

/**********************************************************************
 * 
 *   the autoVTMsgSend succeeds or times out 
 *   iff it succeeds, it has processes the commands including the
 *   the vtCreate on the MSR, it return when the command has been
 *   enacted - specifically the msr got the command and the data structures
 *   are stable - the MSR vtTask then works out the command on periodic
 *   execution
 *   if it times out, the vtTask on the msr probably has gone belly up..
 *********************************************************************/
int
probeVT()
{
  int box[2];
  int rstat;
 
  box[0] = VT_INIT;
  box[1] = VT_NORMAL_UPDATE;  /***** what should this be ??? */

  rstat = autoVTMsgSend(pTheAutoObject,(char *) box,8); 
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
  
  /* local array is ok cause copied */
  currentStatBlock.stb.AcqVTSet = (short) VTOFF;
  currentStatBlock.stb.AcqVTAct = (short) VTOFF;
  return(rstat);
}

/*
1 - VT out of Regulation is an Error
2 - VT out of Regulation is a Warning 
*/

int
setVTErrMode(int mode)
{
  int box[2];
  int rstat;
  box[0] = VT_SET_ERRORMODE;
  box[1] = mode;
  if (pTheVTObject == NULL)
      return(-1);
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box,8); 
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
  /* local array is ok cause copied */
  return(rstat);
}

/* this creates the VT structure and spawns any tasks.... */
/* it don't seem to care about failure!! */
/* CAUSES A CREATE -- temporarily on the msr card */
/* emask just helps lsdv vt functions */

int
setVTtype (int type)
{
  int box[2];
  int rstat;  
  vt_emask = type;
  if (type < 1) 
    return(0);
  box[0] = VT_SET_TYPE;
  box[1] = type;
  rstat = autoVTMsgSend(pTheAutoObject,(char *)box,8);
  /* if it did not already exist */
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
  else
    {
      vtCreate();
    }
  return(0);
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
   printf("VT: '%s'\n",pVtId->VTIDSTR);
   printf("VT: type: %s, Serial Port: %d, ",
	(pVtId->VTtype == 0) ? "None" : "Oxford",
	 pVtId->VTport);

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
   printf("Interlock Active: '%s', Update rate: %d sec\n",pIntlkMode,
	     pVtId->VTUpdateRate/sysClkRateGet());
   printf("VT regulatation range +/- %d\n",pVtId->VTRange);
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
    printf("          Regulated: '%s', Wait4Reg: '%s',  Final Setting: '%s'\n",
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
void VTShow(int level)
/* int level 	   - level of information */
{
   vtShow(pTheVTObject,level);
}


/* pauses for vt action to complete -- why */
int
getnGiveVTMutex ()
{
  return(0);
}

/* CAUSES A CREATE */
int
setVT(int temp, int ltmpxoff, int pid)
{ 
  int box[4];
  int rstat;
    
  box[0] = VT_SETTEMP;
  box[1] = temp;
  box[2] = ltmpxoff;
  box[3] = pid;
  rstat = autoVTMsgSend(pTheAutoObject,((char *) box),16);
  if (rstat == -10) 
    printf("Mutex Time OUT\n");  
  else
    {
      if (pTheVTObject == NULL)
      {
        vtCreate();  /* if the msr create failed this will also fail */
      }
    }
  /* if the vtCreate failed this should do something different  TODO*/
  currentStatBlock.stb.AcqVTSet = (short) temp;  
  /* VT out of Regulation 2nd bit set (2) */
  /* should add some already regulated at this temp code  TO DO */
  clearLSDVbits( LSDV_VTBIT1 );
  setLSDVbits  ( LSDV_VTBIT2 );
  return(0);
}

void
resetVT ()
{
  int box[4];
  int rstat;
  if (pTheVTObject == NULL)
      return;
  box[0] = VT_RESET;
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box, 4);
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
  currentStatBlock.stb.AcqVTSet = (short) VTOFF;
  currentStatBlock.stb.AcqVTAct = (short) VTOFF;
  return;
}

int
setVTinterLk (int mode)
{
  int box[4];
  int rstat;
  if (pTheVTObject == NULL)
      return(-1); 
  box[0] = VT_SET_INTERLOCK;
  box[1] = mode;
  box[2] = 0;
  box[3] = 0;
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box, 8);
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
   return(rstat);
}

int
setVTRegRange (int range)
{
  int box[4];
  int rstat;
  if (pTheVTObject == NULL)
      return(-1); 
  box[0] = VT_SET_RANGE;
  box[1] = range;
  box[2] = 0;
  box[3] = 0;
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box, 8);
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
   return(rstat);
}

int
setVTSlewRate (int slew)
{
  int box[4];
  int rstat;
  if (pTheVTObject == NULL)
      return(-1); 
  box[0] = VT_SET_SLEW;
  box[1] = slew;
  box[2] = 0;
  box[3] = 0;
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box, 8);
  if (rstat == -10) 
    printf("Mutex Time OUT\n");
   return(rstat);
}

int
wait4VT (int dur)
{ 
  int curt,exit_cond,old_stat,timeout;
  int rstat;
  int myTicks, temp;
  int box[2];
  /* 
  ** we use a weak interaction model..
  ** we send message to update the waiting 4 vt fields 
  ****************************************************
  ******************** VT_WAIT4REG ********************
  ****************************************************
  */
  if (pTheVTObject == NULL)
  {
    return(VTERROR +TIMEOUT);
  }
  box[0] = VT_WAIT4REG;
  box[1] = VT_WAITING_UPDATE;
  rstat = autoVTMsgSend(pTheAutoObject,(char *) box, 8);
  if (rstat == -10)
    { 
       DPRINT(0,"WAIT VT Mailbox Timeout\n");
       return(VTERROR +TIMEOUT);
    }
  temp = pTheVTObject->VTTempRate;
  if (temp > 0)
    {
      myTicks = 60*( pTheVTObject->VTTruTemp - pTheVTObject->VTSetTemp)/temp;
      if (myTicks < 0) myTicks = -myTicks;
      if (myTicks > dur) 
      logMsg("Wait 4 VT - you'll need %d\n",myTicks); 
    }
  old_stat = get_acqstate();
  update_acqstate(ACQ_VTWAIT);
  getstatblock();   /* force statblock upto hosts */
  timeout = dur * sysClkRateGet();           
  DPRINT1(0,"wait4VT: Timeout in %d Seconds, Waiting...\n",dur);
   curt = 0;
   exit_cond = 0;
   /* just watch the bits.. and return if canceled another way...  */
   while ((curt < timeout)  && !(exit_cond))
   {
        taskDelay(4);  /* step of two */
        if ( (pTheVTObject->VTModeMask & VT_IS_REGULATED) == VT_IS_REGULATED ) 
        {
           exit_cond=1;
           update_acqstate(old_stat);
           getstatblock();   /* force statblock upto hosts */
        }
        else 
        { 
	  /* maybe should factor in VT Error Type.... */
            if (pTheVTObject->VTerror != 0)
	      {
                 update_acqstate(old_stat);
                 getstatblock();   /* force statblock upto hosts */
                 return(pTheVTObject->VTerror);
              }
            DPRINT1(2,"Still not in regulation at %d ticks\n",curt);
         }
        curt += 4;
   }
   if (exit_cond == 0)
   { 
      DPRINT(2,"VT TIMED OUT-- ERROR!\n");
      update_acqstate(old_stat);
      getstatblock();   /* force statblock upto hosts */
      return(VTERROR + VTREGFAIL);
   }
   update_acqstate(old_stat);
   getstatblock();   /* force statblock upto hosts */
   return(0);
}

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


/************************
* Spin interlock
*/
setSpinInterlk(int in)
{
  DPRINT1(2,"ssetSpinInterlk: %d\n",in);
  SpininterLck = in; /* 1 - enable interlock when ready */
		     /* 2 - Ready to be tested */
}

getSpinInterlk()
{
  DPRINT1(2,"getSpinInterlk: %d\n",SpininterLck);
  return(SpininterLck);
}

setSpinErrMode(int in)
{
  SpinErrorType = in;  /* HARD_ERROR - Spin out of Regulation is an Error */
		     /* WARNING_MSG - Spin out of Regulation is a Warning */
}

getSpinErrMode()
{
  return(SpinErrorType);
}

setSpinMASThres(int in)
{
  DPRINT2(0,"setSpinMASThres: new thres: %d, present thres: %d\n",in,SpinMASThreshold);
  autoSpinSetMASThres(pTheAutoObject,in);  /* let MSR know new threshold */
  SpinMASThreshold = in;   /* spin speed (Hz) to switchover to MAS spinner control */
}

getSpinMASThres()
{
  return(SpinMASThreshold);
}

setSpinRegDelta(int hz)
{
  if ( hz != SpinDeltaReg)
  {
    DPRINT1(0,"setSpinRegDelta: new regulation delta %d Hz\n",hz);
    autoSpinSetRegDelta(pTheAutoObject,hz);  /* let MSR know new delta */
    SpinDeltaReg = hz;
  }
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

/*------------------------------------------------------*/
/*     setspin(speed) -  set spinner speed 		*/
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
/*------------------------------------------------------*/
setspin(int setspeed, int bumpFlag)
{
  int speed,rate,timeout,regchk,stat,zerocnt,bumpcnt,limit;
  int old_stat;

  zerocnt = 0;                  /* # of times zero spinner speed obtained */
  bumpcnt = 0;                  /* allow only to bump sample twice then fail*/
  Ldelay(&timeout,6000);       /* allow 1 min total to regulate */
  if (bumpFlag)
    limit = 2;                  /* It was determined empircally that if the */
  else                         /* spinner is not bumped, the software needs */
    limit = 10;          /* to wait longer for the speed to become non-zero */

  /* old_stat = get_acqstate(); */
  /* update_acqstate(ACQ_SPINWAIT); */
  /* getstatblock();   /* force statblock upto hosts */

  SpininterLck = 0;
  currentStatBlock.stb.AcqSpinSet = setspeed;
  if ( autoSpinSpeedSet(pTheAutoObject,setspeed) != OK)
  {
     return(SPINERROR + SPINTIMEOUT);
  }

  if (setspeed == 0)  /* Hey, if setting to zero just return */
    return(0);

  while (1)
     {
	  speed = (autoSpinValueGet(pTheAutoObject)) / 10;
          stat  = autoSpinReg(pTheAutoObject);
          currentStatBlock.stb.AcqSpinAct = speed;
          getstatblock();   /* force statblock upto hosts */

          if ( (speed == -1) )
	     return(SPINERROR + SPINTIMEOUT);
 
          if (speed > 0)   /* As soon as spinner is rotating then return */
          {
             return(0);
	  }
	  else
          {
	     zerocnt++;
          }

          if (zerocnt > limit)  /* Not spinning for awhile then bump it */
          {
            if (bumpFlag == 0)
            {
                return(SPINERROR + BUMPFAIL);
            }
            bumpcnt++;
   
            if (bumpcnt > 3)   /* Try Bumping only 3 times, then fail */
            { 
		return(SPINERROR + BUMPFAIL); /* bump twice,fail*/
  	    } 
 	    autoSampleBump(pTheAutoObject);
            taskDelay(sysClkRateGet() * 5);
            zerocnt = 0;
          } 

          DPRINT4(0,"setspin: speed: %d, stat: %d, zerocnt: %d, bumpcnt: %d\n",
	  speed,stat,zerocnt,bumpcnt);
         
          if (Tcheck(timeout)) 
          {
	      return(SPINERROR + RSPINFAIL);
          }
          taskDelay(sysClkRateGet() * 1);
     }
  /* update_acqstate(old_stat); */
  /* getstatblock();   /* force statblock upto hosts */
  return(0);            
}     
/*------------------------------------------------------*/
/*     spinreg() -  Wait for spinner to regulate  */
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

  DPRINT3(1,"spinreg: setspeed: %d, speed: %d, status: %d\n",setspeed,speed,stat);
  if ((stat == 1) && (speed == setspeed))
  {
    SpininterLck = 2;
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
	  speed = (autoSpinValueGet(pTheAutoObject)) / 10;
          stat  = autoSpinReg(pTheAutoObject);
          currentStatBlock.stb.AcqSpinAct = speed;
          getstatblock();   /* force statblock upto hosts */

          DPRINT3(1,"spinreg: setspeed: %d, speed: %d, status: %d\n",setspeed,speed,stat);
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
 	    autoSampleBump(pTheAutoObject);
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
  SpininterLck = 2;
  return(rtnstat);
}     

/*****************************************************************
*  test4VT - checks if VT is present. If VTport<0 it will 
*	     initialize it and then send it a 'S' to get status
*	     If the status request is answered VT is present, if
*	     it times out, it is not present, we close the VTport
*	     and return the TIMEOUT as result
*/
/* NEW VERSION --- if MSR not present FAIL 
if MSR present and vtPort == -1 ... send a create message 
wait and recheck vtPort ... */
int test4VT()
{
   int	ival;
   if (pTheAutoObject == NULL) 
   {
      pTheVTObject == NULL;
      return(NULL);
   }
   if (pTheVTObject == NULL) 
   {
      return(vtCreate());
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
