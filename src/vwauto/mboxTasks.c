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
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
/*#include <msgQLib.h>*/
/* semLib is probably not necessary after I split up the file */
#include <semLib.h>
#include "globals.h"
#include "logMsgLib.h"
#include "hardware.h"
#include "commondefs.h"
#include "mboxObj.h"
#include "m32hdw.h"
#include "mboxcmds.h"
#include "spinObj.h"
#include "serialShims.h"
#include "hostAcqStructs.h"
#include "instrWvDefines.h"

#include "vtfuncs.h"
#include "errorcodes.h"



/*
modification history
--------------------
2-16-95,gmb  created 
*/
/*

*/

#define APBUS_PRIORTY 2
#define MBOX_PRIORTY 60
#define MBOX_TASK_OPTIONS 0
#define MBOX_STACK_SIZE 2048

#define QSPI_SHIMS 1
#define SERIAL_SHIMS 2
#define RRI_SHIMS 3
#define TIMEOUT 98
#define DONETIMEOUT -424242

#define SSHA_INDEX	63 /* must match in A_interp.c in vwacq */
#define JHWSHIM     62          /*  dac mask & gate on flag for hwshimming */



static int tShimTid = NULL;
static int tShimApTid = NULL;
static int tGenTid = NULL;
static int tSpinTid = NULL;

int tVTQTid = NULL;
void vtTask();
int debugShim = 1;
int thinShimsPresent = 0;


/* *pShareMemoryAddr   address of beginning of share memory, just past SM backplane */
/* MSRType  type of MSR board 1=MSRI or 2=MSRII */

void
clearSharedDacValues()
{
   /* memset(AUTO_SHARED_SHIMS(MPU332_RAM+AUTO_SHARED_MEM_OFFSET), 0, sizeof( shim_t ) * MAX_SHIMS_CONFIGURED); */
   memset(AUTO_SHARED_SHIMS(pShareMemoryAddr), 0, sizeof( shim_t ) * MAX_SHIMS_CONFIGURED);
}

int
getDacValue( int dacIndex )
{
   int dacValue;

   /* dacValue = (int) ((shim_t *) (AUTO_SHARED_SHIMS(MPU332_RAM+AUTO_SHARED_MEM_OFFSET)))[ dacIndex ]; */
   dacValue = (int) ((shim_t *) (AUTO_SHARED_SHIMS(pShareMemoryAddr)))[ dacIndex ];
   DPRINT2( 1, "getDacValue: %d %d\n", dacIndex, dacValue );
   return( dacValue );
}

void
storeDacValue( int dacIndex, int dacValue )
{
   if ((dacIndex < 0) || (dacIndex >= MAX_SHIMS_CONFIGURED))
   {
      DPRINT1(-1,"storeDacValue got illegal index %d\n",dacIndex);
      return;
   }
   DPRINT2( 1, "storeDacValue: %d %d\n", dacIndex, dacValue );

   /* ((shim_t *) (AUTO_SHARED_SHIMS(MPU332_RAM+AUTO_SHARED_MEM_OFFSET)))[ dacIndex ] = (shim_t) dacValue; */
   ((shim_t *) (AUTO_SHARED_SHIMS(pShareMemoryAddr)))[ dacIndex ] = (shim_t) dacValue;
}

void
setDacValue( int dacIndex, int dacValue )
{
        switch (shimType)
        {
          case QSPI_SHIMS:
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_QSPI,NULL,NULL);
#endif
                setQspiShim(dacIndex, dacValue);  /* translate to proper qspi dac and sets it */
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
                break;

          case SERIAL_SHIMS:
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_SERIAL,NULL,NULL);
#endif
                setSerialShim(dacIndex, dacValue);
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
                break;

          case OMT_SHIMS:
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_OMT,NULL,NULL);
#endif
                setSerialOmtShim(dacIndex, dacValue);
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
                break;

          case RRI_SHIMS:
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_RRI,NULL,NULL);
#endif
                setRRIShim(dacIndex, dacValue, 0);
#ifdef INSTRUMENT
                wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
                break;

          default:
                DPRINT1(-1,"hardware autoshim: unsupported shim type %d.\n",shimType);
                break;
        }
        storeDacValue( dacIndex, dacValue );
}

void shimTask()
{
   short shimbuf[((AUTO_MBOX_SIZE/sizeof(short)) + 1)];
   int bytes,i,nDac2Chg,vnmrIndex;

   FOREVER
   {
       bytes = mboxShimGetMsg((char*) shimbuf, sizeof(shimbuf));
       nDac2Chg = (bytes/(sizeof(short)));
       /* For right now the only shim are the QSPI ones 
        * But when serial port shim are around we will have to
        * decide which one to talk to 
        */
       if ( nDac2Chg <= 0)
       {
          DPRINT3(-1,"shimTask: Type: %d, bytes: %d, nDac2Chg: %d, Skip Msge\n",shimType,bytes,nDac2Chg);
          mboxShimMsgComplete(OK);
          continue;
       }
       for(i=0; i < nDac2Chg; i += 2)
       {
         DPRINT3(debugShim,"shimTask: Type: %d, DAC[%d] = %d\n",shimType,shimbuf[i],shimbuf[i+1]);
         vnmrIndex = shimbuf[i];
	 switch(shimType)
	 {
	    case QSPI_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_QSPI,NULL,NULL);
#endif
           	 shimDacSet(shimbuf[i],shimbuf[i+1]);  /* qspi dac translation already done */
		 vnmrIndex = qspiToVnmr(shimbuf[i]);
		DPRINT2( debugShim, "qspi index: %d, vnmr index: %d\n", shimbuf[i], vnmrIndex );
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case SERIAL_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_SERIAL,NULL,NULL);
#endif
		 setSerialShim((int) shimbuf[i], (int) shimbuf[i+1] );
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case OMT_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_OMT,NULL,NULL);
#endif
		 setSerialOmtShim((int) shimbuf[i], (int) shimbuf[i+1] );
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case RRI_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_RRI,NULL,NULL);
#endif
		 setRRIShim((int) shimbuf[i], (int) shimbuf[i+1], 0 );
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    default:
		 DPRINT1(-1,"Warning Shim Type %d. Ignoring it.\n",shimType);
                 continue;
		break;
	 }

	 DPRINT2( debugShim, "shimTask, shim DAC: %d, shim value: %d\n", vnmrIndex, shimbuf[i+1] );
	 storeDacValue( vnmrIndex, shimbuf[ i+1 ] );
       }
       mboxShimMsgComplete(OK);
   }
}

void shimApTask()
{
   short shimbuf[((AUTO_MBOX_SIZE/sizeof(short)) + 1)];
   short dac,value;
   int bytes,i;

   FOREVER
   {
       bytes = mboxShimApGetMsg((char*) shimbuf);
       dac = (shimbuf[0] & 0x3f00) >> 8;
       value = (shimbuf[0] & 0x00ff) << 8;
       value = value  + (shimbuf[1] & 0x00ff);

       if (dac == JHWSHIM)
       {
           DPRINT3(1,"shimApTask: JHWSHIM: composite value: 0x%x, mask: 0x%x, gate: %d\n",
			value, (value & 0xfc), (value & 0x1));
           startStopHwShim( value );
           continue;
       }


       if (dac == SSHA_INDEX)
       {
	   DPRINT1( 1, "mboxTasks: shimApTask value=%d\n", value );
           startStopSsha( value );
           continue;
       }

       DPRINT3(1,"shimApTask: Type: %d, DAC[%d] = %d\n",shimType,dac,value);
	 switch(shimType)
	 {
	    case QSPI_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_QSPI,NULL,NULL);
#endif
           	 setQspiShim(dac,value); /* translate to proper qspi dac and sets it */
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case SERIAL_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_SERIAL,NULL,NULL);
#endif
		 setSerialShim((int) dac, (int) value );
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case OMT_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_OMT,NULL,NULL);
#endif
		 setSerialOmtShim((int) dac, (int) value);
#ifdef INSTRUMENT
     wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    case RRI_SHIMS:
#ifdef INSTRUMENT
     		wvEvent(EVENT_SHIMSET_RRI,NULL,NULL);
#endif
		 setRRIShim((int) dac, (int) value, 0);
#ifdef INSTRUMENT
     wvEvent(EVENT_SHIMSET_CMPLT,NULL,NULL);
#endif
		break;

	    default:
		 DPRINT1(-1,"Warning Shim Type %d is undefined, no shims set.\n",shimType);
		 continue;
		break;
	 }
	 DPRINT2( debugShim, "shimApTask, shim DAC: %d, shim value: %d\n", dac, value );
	 storeDacValue( dac, value );
   }
}

void genTask()
{
   short genbuf[((256/sizeof(short)) + 1)];
   int status,bytes;
   static int msgDecode(short cmd, char* msge);

   FOREVER
   {
       bytes = mboxGenGetMsg((char*) genbuf, sizeof(genbuf));
       status = msgDecode(genbuf[0],(char*) &genbuf[1]);
       mboxGenMsgComplete(status);
   }
}

void spinTask()
{
   short genbuf[((256/sizeof(short)) + 1)];
   int status,bytes;
   static int msgDecode(short cmd, char* msge);

   FOREVER
   {
       bytes = mboxSpinGetMsg((char*) genbuf, sizeof(genbuf));
       status = msgDecode(genbuf[0],(char*)&genbuf[1]);
       mboxSpinMsgComplete(status);
   }
}

static int msgDecode(short cmd, char* msge)
{
   LOCK_VALUE_SIZE lkval;
   SPIN_VALUE_SIZE spinval;
   SET_SPIN_ARG_SIZE spinsetting;
   SHIMS_PRESENT_SIZE shimspresent;
   short sval;
   int   ival;
   int status;

   status = OK;
   /* list of commands in mboxcmds.h */
   DPRINT1(5,"msgDecode: cmd: %d\n",cmd);
   switch (cmd)
   {
      case SET_GPA_TUNE_PX:
      case SET_GPA_TUNE_IX:
      case SET_GPA_TUNE_PY:
      case SET_GPA_TUNE_IY:
      case SET_GPA_TUNE_PZ:
      case SET_GPA_TUNE_IZ:
      case SET_GPAENABLE:
	DPRINT2(1,"Set GPA (cmd=%d) to %d\n",
		cmd, *((GPA_TUNE_VALUE_SIZE*)msge));
	status = setGpaTuning(cmd, *((GPA_TUNE_VALUE_SIZE*)msge));
	break;
           
      case GET_LOCK_VALUE:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_GETLKVAL,NULL,NULL);
#endif
#ifndef LMODEL
           lkval = *M32_LOCK_VALUE;
           DPRINT2(4,"Raw Val: 0x%x(%d)\n",lkval,lkval);
           /* lock adc is 14bits, 0x2000 is sign bit, if value > 8192 (0x2000)
              then subtract 16384 (0x4000) to give minus value 
	      ( 0x3fff - 0x4000 = -1, 0x2000 - 0x4000 = -8192 )
           */
	   /*
	   lkval = (lkval < 0x2000) ? lkval : (lkval - 0x4000);
	   */
           if (MSRType == AUTO_BRD_TYPE_I)
           {
	     /* convert 14-bit to a sign extended 16-bit value */
             lkval &= 0x3fff;
             if (lkval & 0x2000) lkval |= 0xc000; /* sign extend negative */
	   }
	   else
           {
		lkval = lkval/2;  /* for MSRII divide by 2 ??  */
           }
           DPRINT2(4,"2sComp val: %d (0x%x)\n",lkval,lkval);
#else
           lkval =  (rand() % 0x100) + 1000;
#endif
           mboxGenPutMsg((char*) &lkval, sizeof(LOCK_VALUE_SIZE));
	   break;

      case GET_SHIMS_PRESENT:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_GETSHIMS,NULL,NULL);
#endif
	   /* qspi EEprom only differentiate between 14/RRI/Imaging shims */
           /* RRI is acessed via serial port, 14 & Imaging are QSPI */
           DPRINT1(1,"GET_SHIMS_PRESENT: shim set type: %d\n",shimSet);
	   shimspresent = shimSet;
           if ( shimSet > 0)
              shimspresent = shimspresent + 1000;
           mboxGenPutMsg((char*) &shimspresent, sizeof(SHIMS_PRESENT_SIZE));
	   break;

      case GET_SERIAL_SHIMS_PRESENT:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_GETSERIALSHIM,NULL,NULL);
#endif
	   /******* PHIL *******/
           shimspresent =  1;
           DPRINT1(1,"GET_SERIAL_SHIMS_PRESENT: %d\n",shimspresent);
           mboxGenPutMsg((char*) &shimspresent, sizeof(SHIMS_PRESENT_SIZE));
	   break;

      case THIN_SHIMS_ON:
           thinShimsPresent = 1;
           DPRINT(1,"Thin shims selected\n");
	   break;
      case THIN_SHIMS_OFF:
           thinShimsPresent = 0;
           DPRINT(1,"Thin shims selected\n");
	   break;


      case GET_SPIN_VALUE:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_GETSPINVALUE,NULL,NULL);
#endif
           spinval = getSpeed();
           DPRINT1(4,"GET_SPIN_VALUE: %d\n",spinval);
           mboxGenPutMsg((char*) &spinval, sizeof(SPIN_VALUE_SIZE));
	   break;

      case EJECT_SAMPLE:
	   /* 1. turn eject,bearing & slow drop air on
           */
	   /* taskSuspend(tSpinRegId); */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_EJECT,NULL,NULL);
#endif
           DPRINT(1,"EJECT_SAMPLE\n");
	    cntrlRegSet(SET_EJECT_AIR_ON | SET_BEAR_AIR_ON | SET_SDROP_AIR_ON);
      	    taskDelay(sysClkRateGet() * 1);
	   break;

      case EJECT_SAMPLE_OFF:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_INSERT,NULL,NULL);
#endif
	   /* 1. Turn off air. No sample present.
           */
	   /* taskSuspend(tSpinRegId); */
           /* turn off Eject * Bearing Air */
           DPRINT(1,"EJECT_SAMPLE_OFF\n");
	    cntrlRegClear(SET_EJECT_AIR_ON | SET_BEAR_AIR_ON | SET_SDROP_AIR_ON);
      	    taskDelay(sysClkRateGet() * 1);
	   break;

      case INSERT_SAMPLE:
	   {
             char astat;
	     int cnt,stat,mode;
	     long speed;
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_INSERT,NULL,NULL);
#endif
	   /* 1. Drop Sample back down into probe.
           */
	   /* taskSuspend(tSpinRegId); */
           /* turn off Eject * Bearing Air */
           DPRINT(1,"INSERT_SAMPLE\n");
           /* get previous spinner setting prior to stopping it */
	    getSetSpeed(&speed,&mode);
            setSpeed(0L);  /* turn off rotation air */
	    cntrlRegClear(SET_EJECT_AIR_ON | SET_BEAR_AIR_ON); /* turn off Eject & Bearing air */
            /* getStat(); */
            taskDelay(sysClkRateGet() * 5);
            DPRINT(1,"INSERT_SAMPLE: SLOWDROP OFF\n");
	    cntrlRegClear( SET_SDROP_AIR_ON);
      	    taskDelay(sysClkRateGet() * 1);

	    /* check for 8 more sec at most to detect sample in probe */
	    for (cnt=0; cnt < 16; cnt++)
            {
                /* getStat(); */
		astat = *M32_CNTRL;
	        DPRINT2(1,"INSERT_SAMPLE: cnt %d, Sample: '%s'\n",
		   cnt,(astat & RD_SAMP_IN_PROBE) ? "IN" : "OUT");
		if (astat & RD_SAMP_IN_PROBE)
		{
	           DPRINT(1,"INSERT_SAMPLE: Sample Detected\n");
		   break;
		}
	        taskDelay(30);  /* check each .5 sec */
	    }
#ifdef DEBUG
/*
            if (DebugLevel > 0)
	      getSetSpeed(&speed,&mode);
*/
	    DPRINT2(1,"INSERT_SAMPLE: setspeed: %ld, mode: '%s'\n",speed,
		(mode == SPEED_MODE) ? "Speed Reg." : "Rate");
#endif
	    if (mode == SPEED_MODE)
            {
	       DPRINT2(1,"INSERT_SAMPLE: setspeed: %d, mode: '%s'\n",speed,
		(mode == SPEED_MODE) ? "Speed Reg." : "Rate");
	       setSpeed(speed);
            }
            /* DPRINT(1,"INSERT_SAMPLE: BEARING ON\n"); */
	    /* cntrlRegSet(SET_BEAR_AIR_ON); */
	   /* taskResume(tSpinRegId); */
	   }
	   break;

      case SAMPLE_DETECT:

#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SAMPDETECT,NULL,NULL);
#endif
	     DPRINT1(4,"SAMPLE_DETECT: Sample: '%s'\n",
		  (*M32_CNTRL & RD_SAMP_IN_PROBE) ? "IN" : "OUT");
             
             status = (*M32_CNTRL & RD_SAMP_IN_PROBE) ? 1 : 0;
	   break;

      case SET_SPIN_RATE:
           /* if > 0 then
	      1. Turn bearing air on if off.
	      2. Resume the task attempting to regulate speed
	      3. Tell Spin regulation task new speed to regulate at.
	      else
	      1. suspend the task attempting to regulate speed
	      2. Turn bearing air off if on.
           */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SETSPINRATE,NULL,NULL);
#endif
	    /* cntrlRegSet(SET_BEAR_AIR_ON); */
	    DPRINT1(1,"Set Spin rate to %d\n",*((SET_SPIN_ARG_SIZE*)msge));
	    setRate((long) (*((SET_SPIN_ARG_SIZE*)msge)));
	   /* taskResume(tSpinRegId); */
	   break;

      case SET_SPIN_SPEED:
           /* if > 0 then
	      1. Turn bearing air on if off.
	      2. Resume the task attempting to regulate speed
	      3. Tell Spin regulation task new speed to regulate at.
	      else
	      1. suspend the task attempting to regulate speed
	      2. Turn bearing air off if on.
           */
	    /* cntrlRegSet(SET_BEAR_AIR_ON); */
	   /* taskResume(tSpinRegId); */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SETSPINSPD,NULL,NULL);
#endif
	    DPRINT1(1,"Set Spin Speed to %d\n",*((SET_SPIN_ARG_SIZE*)msge));
	   setSpeed((long) (*((SET_SPIN_ARG_SIZE*)msge)));
	   break;

      case BEARING_ON:
	   /* 1. resume the task attempting to regulate speed
	      2. turn bearing air on
           */
	   /* taskResume(tSpinRegId); */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_BEARINGON,NULL,NULL);
#endif
           DPRINT(1,"BEARING_ON\n");
	    cntrlRegSet( SET_BEAR_AIR_ON );
	   break;

      case BEARING_OFF:
	   /* 1. suspend the task attempting to regulate speed
	      2. turn bearing air on
           */
	   /* taskSuspend(tSpinRegId); */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_BEARINGOFF,NULL,NULL);
#endif
           DPRINT(1,"BEARING_OFF\n");
	    cntrlRegClear( SET_BEAR_AIR_ON); 
	   break;

      case SET_SPIN_MAS_THRESHOLD:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SETMASTHRES,NULL,NULL);
#endif
	   DPRINT1(1,"Set MAS Switch Over Threshold: %d\n",*((SET_SPIN_ARG_SIZE*)msge));
           setMASThreshold(*((SET_SPIN_ARG_SIZE*)msge));
	   break;

      case SET_SPIN_REG_DELTA:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SETSPINREGDELTA,NULL,NULL);
#endif
	   DPRINT1(1,"Set Spinner Regulation delta: %d (Hz)\n",*((SET_SPIN_ARG_SIZE*)msge));
           setSpinRegDelta(*((SET_SPIN_ARG_SIZE*)msge));
	   break;

      case SET_DEBUGLEVEL:
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MSG_SETDEBUGLEVEL,NULL,NULL);
#endif
	   DPRINT1(0,"Set DebugLevel to %d\n",*((DEBUGLEVEL_VALUE_SIZE*)msge));
	   DebugLevel = *((DEBUGLEVEL_VALUE_SIZE*)msge);
           if (DebugLevel == 6)
           {
             DebugLevel=0;
             checkStack();
           }
	   break;

      default:
	   status = ERROR;
	   break;
   }
   DPRINT1(5,"msgDecode: returning status: %d\n",status);
   return(status);
}

startMBoxTasks()
{
     tShimTid = taskSpawn("tShimMbxTsk", MBOX_PRIORTY, MBOX_TASK_OPTIONS,
		MBOX_STACK_SIZE+1024, shimTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
     if ( tShimTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn tShimMbxTsk:");
     }

     tShimApTid = taskSpawn("tShimApTsk", APBUS_PRIORTY, MBOX_TASK_OPTIONS,
		(MBOX_STACK_SIZE*2), shimApTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
     if ( tShimApTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn tShimApTsk:");
     }

     tGenTid = taskSpawn("tGenMbxTsk", MBOX_PRIORTY, MBOX_TASK_OPTIONS,
		MBOX_STACK_SIZE+1024, genTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
     if ( tGenTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn tGenMbxTsk:");
     }
     tSpinTid = taskSpawn("tSpinMbxTsk", MBOX_PRIORTY, MBOX_TASK_OPTIONS,
		(MBOX_STACK_SIZE*3), spinTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
     if ( tSpinTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn tSpinMbxTsk:");
     }
     tVTQTid = taskSpawn("tvtMbxTsk", MBOX_PRIORTY, MBOX_TASK_OPTIONS,
		(MBOX_STACK_SIZE*2)+1024, vtTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10); 
     if ( tVTQTid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "startMBoxTasks: could not spawn vtTask:");
     }
}

DeleteMBoxTasks()
{
   if (tShimTid != NULL)
      taskDelete(tShimTid);
   if (tGenTid != NULL)
      taskDelete(tGenTid);
   if (tSpinTid != NULL)
      taskDelete(tSpinTid);
   if (tVTQTid != NULL)
      taskDelete(tVTQTid);
}


prtShimType()
{
   printf("Shim Type: %d (1-QSPI, 2-SERIAL, 3-RRI\n",shimType);
}
/******************************************************
************VT STUFF STARTS HERE **********************
******************************************************/

VT_ID msr_VT = NULL;
VT_ID ref_VT = NULL;

#ifndef MODEL
/*****************************************************************
*  Oxford VT controller driver                                            
*  OxVT(object,command,number): command is a single character           
*                              number is usually the temperature       
*****************************************************************/
int OxVT(VT_ID pVtId,char cmd,int temp)
{
    int stat,time_out;
    DPRINT2(1,"OxVT: Cmd = %c  Var = %d ", cmd, temp);

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
    /*
    if (cmdecho(pVtId->VTport, CMD)) 
    {
      semGive(pVtId->pVTmutex);
      return(TIMEOUT);
    }
    */
    time_out = 100;
    switch(cmd)
    {
        case 'B': time_out = 500; break; /* 'B' is a power cycle which takes a long time */
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
    DPRINT1(1,"VTstat: CMD = %c \n",cmd);
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
    DPRINT2(2,"Send by 'VTstat' :    %d Dec\t     ----> %c\n\n", cmd, cmd);

    /* don't worry about time out here, if we do return then
       the expected returning value will become out of sync 
    */
    cmdecho(pVtId->VTport,CMD);
/*
    if (cmdecho(pVtId->VTport,CMD)) 
    { 
      acqerrno = VTERROR + VTTIMEOUT;
      semGive(pVtId->pVTmutex);
      return(CMDTIMEOUT); 
    }
*/
    value = cmddone(pVtId->VTport,50);   /* cmddone set acqerrno */
    if ((value == -1) && (acqerrno == DONETIMEOUT))
    {
      acqerrno = VTERROR + VTTIMEOUT;
      semGive(pVtId->pVTmutex);
      return(CMDTIMEOUT); 
    }
    DPRINT1(1,"VTstat: Value = %d\n",value);
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
/* this is the TASK IDENTIFIER */
int tVTId=NULL;
/* vtCreate should we allocate object and test VT port or de-allocate object ***/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/**************************************************************
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
   int rstat;
   VT_CMD cmd;

  /*  int vtTempReg(VT_ID pVtId); */

  /* pVtObj = (VT_ID)(AUTO_SHARED_VT(MPU332_RAM+AUTO_SHARED_MEM_OFFSET)); */
  pVtObj = (VT_ID)(AUTO_SHARED_VT(pShareMemoryAddr));
  DPRINT1(1,"vtCreate: Created VT Object: 0x%lx\n",pVtObj);

  /* zero out structure so we don't free something by mistake */
  memset(pVtObj,0,sizeof(VT_OBJ));
  ref_VT = pVtObj;  /* for debugging */
  DPRINT2(1,"msr VT starts at %lx and ends at %lx\n",pVtObj,pVtObj+sizeof(VT_OBJ));
  pVtObj->VTSetTemp = VTOFF;
  pVtObj->VTTruTemp = VTOFF;
  pVtObj->VTRange   = 4;  /* 0.4 degree standard */
  pVtObj->VTTempRate = 125; /* increase temp by 12.5 degrees per minute */
  pVtObj->VTModeMask = VT_MANUAL_ON_BIT;
  /*  pVtObj->VTpresent = VT_NONE; */
  pVtObj->VTport = -1;
  pVtObj->VTinterLck = 0;  /* tin= 'y' or 'n' 0 = no, 1 = enable, 2 = ready*/
  pVtObj->VTtype = 0;		/* 0 == NONE, 2 = Oxford */
  pVtObj->VTerrorType = HARD_ERROR;	/* HARD_ERROR (15) == Error, WARNING_MSG(14) == Warning */
  pVtObj->VTUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE;	/* 3 sec */

  pVtObj->VTport = initSerialPort( 1 ); /* port 1 on the MSR */
  if (pVtObj->VTport < 0)
  {
    return(NULL);
  }

  pVtObj->VTRegTimeout = 0;  /*  */

  setSerialTimeout(pVtObj->VTport,25);	/* set timeout to 1/4 sec */
  DPRINT1(1,"VTport = %d\n",pVtObj->VTport);

  pVtObj->pVTmutex = NULL;	/* VT serial port mutual exclusion */
  pVtObj->pVTmutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);
  if (pVtObj->pVTmutex == NULL)
  {
    errLogSysRet(LOGIT,debugInfo,"vtCreate: Could not Allocate Mutex:");
    return(NULL);
  }

  strcpy(pVtObj->VTIDSTR,"checking the port now!\n");
  /* just check Status do not initialize */
  rstat = VTstat(pVtObj,'S');   /* check Status, acqerrno will be set if cann't talk to VT */
  if (acqerrno == (VTERROR + VTTIMEOUT)) /**** acqerror NO ****/
  {
#ifdef DEBUG
     errLogRet(LOGIT,debugInfo,"vtCreate: Serial Communication to VT Unit timed out, considered Not Present");
#endif
     if (pVtObj->VTport > 0)
        close(pVtObj->VTport);
     pVtObj->VTport = -1;
     if (pVtObj->pVTmutex)
        semDelete(pVtObj->pVTmutex);
     pVtObj->pVTmutex = NULL;	/* VT serial port mutual exclusion */
     strcpy(pVtObj->VTIDSTR,"No response from the port\n");
     DPRINT(2,"Object NULL, no VT presemt\n");
     return(NULL);
  }

  pVtObj->VTModeMask |=  VT_MANUAL_ON_BIT;

  msr_VT = pVtObj;	/* set the static global for VT routines to use */
  vtGetSW(pVtObj);
  strcpy(pVtObj->VTIDSTR,"VT ready");
  DPRINT1(1,"VT: '%s'\n",pVtObj->VTIDSTR);
  return(pVtObj);
}

void vtTask()
{
   int vtbuf[((AUTO_MBOX_SIZE/sizeof(int)) + 1)];
   int bytes,check;
   int timeout;
   /* this code allows functionality if the controller is up at the start in manual mode */
   /* it acts normally */
   if (msr_VT != NULL) 
     timeout = msr_VT->VTUpdateRate = sysClkRateGet() * VT_NORMAL_UPDATE;
   else
     timeout = WAIT_FOREVER;
   FOREVER
   {
       bytes = mboxVTGetMsg((char *) vtbuf, sizeof(vtbuf),timeout);
       if (msr_VT == NULL)
       {
         DPRINT(1,"Attempting to start VT!\n");
         if (!vtCreate())
	   {
           DPRINT(1,"NO VT AVAILABLE!\n");
           timeout = WAIT_FOREVER;
           }
       }
       /* this looks a bit strange */
       if (msr_VT != NULL)
	 {
           if (bytes > 0) 
	   {
                  VtDecodeCmd(msr_VT, (VT_CMD *) vtbuf); 
                  mboxVTMsgComplete(OK);
 /* this says the MSR GOT the instruction and did the basics - the vtTempReg handles the rest */ 
           }
           vtTempReg(msr_VT);
           timeout = msr_VT->VTUpdateRate;
	   /* execute the periodic check of the status */
         }
       else 
         mboxVTMsgComplete(OK); /* need to signal dead but NULL PPC/162 does that*/
      
   } /* forever */
}


ChangeVtTemp(VT_ID VtId, int trutemp)
{
   int rstat, sign, ntemp, deltatemp;


   if ( (VtId->VTModeMask & VT_TEMP_IS_SET) || 
	((VtId->VTRegTimeout != 0L) && (VtId->VTRegTimeout > time(NULL)))
      )
   {
      DPRINT1(1,"ChangeVtTemp, VT Setting Complete? %d\n",(VtId->VTModeMask & VT_TEMP_IS_SET));
      DPRINT2(1,"ChangeVtTemp, Min. not up Yet. Now: 0x%lx, TimeOut: 0x%lx\n",time(NULL),VtId->VTRegTimeout);
      return(0);
   }
     
   if (VtId->VTPrvTemp >= (VTOFF / 2)  )
   {
      DPRINT2(1,"VTPrvTemp reset from %d to %d\n",VtId->VTPrvTemp,trutemp);
      VtId->VTPrvTemp = trutemp;
   }

   /* if (VtId->VTTruTemp > VtId->VTSetTemp) sign = -1; else sign = 1; */
   if (VtId->VTPrvTemp > VtId->VTSetTemp) sign = -1; else sign = 1;

   /* increase temp by 12.5 degrees per minute */
   /* 125 / 60 * rate / clock rate */
   /* deltatemp = (VtId->VTTempRate / 60) * ( VtId->VTUpdateRate / sysClkRateGet()); */
   deltatemp = VtId->VTTempRate;
   /* DPRINT2(1,"ChangeVtTemp: Temp Delta: %d for Update Rate: %d\n",deltatemp,VtId->VTUpdateRate); */

   /* If the differnece in True & Set is greater than delta temp rate then change Temp
      by the Rate, Else Set to Final Temp Setting
   */

   /*  did it 10/7/99 ... TODO: donn't use tru temp to add to but rather the next step in rate, so that
            the number of times to change temp is deterministic  */
   /*  VTPrvTemp is initially set to VTTruTemp, use VTPrvTemp instead of VTTruTemp
       to prevent the set point (VTPrvTemp) for going beyond the set temp(VTSetTemp)
       that was possible before.
   */
   DPRINT3(1,"PresTemp %d, IncTemp %d, SetTemp %d\n",VtId->VTTruTemp,VtId->VTPrvTemp,VtId->VTSetTemp);
   /* if ( abs(VtId->VTTruTemp - VtId->VTSetTemp) > deltatemp)  /* temp > 12.5 degrees */
   if ( abs(VtId->VTPrvTemp - VtId->VTSetTemp) > deltatemp)  /* temp > 12.5 degrees */
   { 
	ntemp =  VtId->VTPrvTemp += sign*deltatemp;

        VtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */

        DPRINT2(1,"Incremental Temp Change to %d,  final temp %d\n",ntemp,VtId->VTSetTemp);
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
           DPRINT(1,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           DPRINT(1,"VT Gas OFF\n");
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
        DPRINT1(1,"Set final VT temp %d\n",VtId->VTSetTemp);
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
           DPRINT(1,"VT Gas ON\n");
           VtId->VTModeMask |= VT_GAS_ON_BIT;  /* Gas On */
	}
	else
        {
           if(rstat = OxVT(VtId,'G',0))
           {
              VtId->VTerror = VTERROR + rstat;
              return(VTERROR + rstat); /* cooling gas off */
           }
           DPRINT(1,"VT Gas OFF\n");
           VtId->VTModeMask &= ~VT_GAS_ON_BIT;  /* Gas Off */
        }


        if (rstat = OxVT(VtId,'A',0))
        {
	   VtId->VTerror = VTERROR + rstat;
           return(VTERROR + rstat);/*automod,tmp 4 display*/
	}
	VtId->VTModeMask &= ~VT_MANUAL_ON_BIT;  /* Automatic ON */
	VtId->VTModeMask |= VT_HEATER_ON_BIT;  /* Heater On */
	VtId->VTModeMask |= VT_TEMP_IS_SET;  /* VT has been set to the final Temp */

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
       }
       temp = VtId->VTSetTemp;
       /* DPRINT3(1,"vtTempReg: Set Temp = %d, Tru Temp: %d VT stat: %d\n", temp,trutemp,vtstat); */

       if (temp != VTOFF)
       {
          switch(vtstat)
          {   
            case 0:         /* In Manual Mode ?? */
       	     VtId->VTerror = 0;
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET ) 
             {
	        ChangeVtTemp(VtId, trutemp);
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
	     if ( (VtId->VTModeMask & VT_TEMP_IS_SET) != VT_TEMP_IS_SET ) /* just in case never set */
             {
	        ChangeVtTemp(VtId, trutemp);
  	     }
             else if ( (trutemp < temp + VtId->VTRange) &&  /* i.e., 30, 29.7-30.3 */
                       (trutemp > temp - VtId->VTRange) )
             {
               VtId->VTRegCnt++; /* count this as regulated */
             }
             break;
  
            case 2:         /* Temp Changing  */
       	     VtId->VTerror = 0;
             VtId->VTRegCnt = 0; /* reset back to zero */
	     ChangeVtTemp(VtId, trutemp);
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
             VtId->VTTruTemp =  (short) VTOFF;
             VtId->VTSetTemp =  (short) VTOFF;
             DPRINT(1,"vtTempReg: VTOFF & Heater On, Turn Off VT\n");
             if ( (rstat=OxVT(VtId,'Q',222))|| /* No spontanous stat msgs*/
                (rstat=OxVT(VtId,'M',0))  || /* back to manual mode */
                (rstat=OxVT(VtId,'O',0))  || /* besure heaters are off */
                (rstat=OxVT(VtId,'G',0))       /* besure gas value is off*/
              ) 
                return(VTERROR + rstat);
            VtId->VTModeMask = VT_MANUAL_ON_BIT | VT_WAITING4_REG;  /* MANUAL, HEater Off, Not Regulated */
       	    VtId->VTerror = 0;
         }

         /* VT Off both bits unset (0) */

	 if ((VtId->VTModeMask & VT_WAITING4_REG) &&  (trutemp < 310))
         {
             DPRINT(1,"vtTempReg: VTOFF, WAITING4VT to go Ambient Temp, So Give Semaphore\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
	     /* semGive(VtId->pVTRegSem); */
         }

       } /* if (temp != VTOFF) */


/*
      if ((VtId->VTModeMask & VT_IS_REGULATED) && (vtstat != 1))
      {
	   VT out of regulation report error
      }
*/

      /* if not regulated yet && Final Temp has been Set, and Temp stable for 30 sec */
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
	    (VtId->VTRegCnt > 30)) /* if stable 30 sec, then its OK */
      {
          DPRINT(1,"vtTempReg: SET VT to Regulated, and reset UpdateRate\n");
          VtId->VTModeMask |= VT_IS_REGULATED;
	  VtId->VTUpdateRate = VT_NORMAL_UPDATE * sysClkRateGet();
	  VtId->VTRegCnt = 0;
	  /* If waiting then give semaphore  */
	  if (VtId->VTModeMask & VT_WAITING4_REG)
          {
             if (VtId->VTinterLck == 1) /* ENABLE */
                  VtId->VTinterLck = 2; /* READY to Check, & report Errors  */

             DPRINT(1,"vtTempReg: WAITING4VT DONE .... clear modemask\n");
             VtId->VTModeMask &= ~VT_WAITING4_REG;
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
	    switch(1)
            {
		case 1:
	           if (rstat = OxVT(pVtId,'M',0)) break; /* back to manual mode */

          	   pVtId->VTModeMask |= VT_MANUAL_ON_BIT;  /* MANUAL ON */

	           if (rstat = OxVT(pVtId,'Q',222)) break;    /* No spontanous statmsg*/
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
      DPRINT(1,"VtDecodeCmd: VT Initialize Command\n");
       pVtId->VTUpdateRate = pCmd->VTArg1 * sysClkRateGet();
       pVtId->VTerror = 0;
       break;

     case VT_SETTEMP:
      DPRINT2(1,"VtDecodeCmd: VT_SETTEMP: Present Set Temp: %d, Set Temp to %d\n",
		pVtId->VTSetTemp,pCmd->VTArg1);
      pVtId->VTerror = 0;
      /* VTsetPid(pVtId,pCmd->VTArg3); */
      /* if setting to same temperature & VT is already regulated at this temp
	 then don't bother to set temp, etc..
      */
	/*   ((VtId->VTModeMask & VT_IS_REGULATED) == VT_IS_REGULATED) ) */
      if (pVtId->VTSetTemp != pCmd->VTArg1)
      {
         pVtId->VTSetTemp = pCmd->VTArg1;
         pVtId->VTPrvTemp = pVtId->VTTruTemp;	/* set start temp that ramp will add to */
         pVtId->VTLTmpXovr = pCmd->VTArg2;
         pVtId->VTModeMask &= ~VT_IS_REGULATED;	/* turn off regulated bit */
         pVtId->VTModeMask &= ~VT_TEMP_IS_SET;	/* turn off temp set bit */
         pVtId->VTModeMask &= ~VT_WAITING4_REG;
         pVtId->VTModeMask &= ~VT_GAS_ON_BIT;
         pVtId->VTRegCnt = 0L;
         pVtId->VTRegTimeout = 0L;
         if (pVtId->VTinterLck == 2 /* READY */)
	    pVtId->VTinterLck = 1;	/* STANDBY */
      }
      break;

      /* this appears to be unused */
     case VT_SETPID:
      DPRINT1(1,"VtDecodeCmd: Set PID to %d\n",pCmd->VTArg1);
      pVtId->VTerror = 0;
      VTsetPID(pVtId,pCmd->VTArg1);
         
      break;
     
     case VT_RESET: 
       DPRINT(1,"VT_RESET\n");
       pVtId->VTSetTemp = VTOFF;
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
        DPRINT1(1,"VtDecodeCmd: Get VT Temp %d\n", pVtId->VTTruTemp);
      break;

     case VT_GETSTAT:
       pVtId->VTstat = VTstat(pVtId,'S');
       DPRINT1(1,"VtDecodeCmd: Get VT Stat: %d\n", pVtId->VTstat);
       break;

     case VT_WAIT4REG:
            DPRINT1(1,"VtDecodeCmd: Wait 4 VT to Regulate, Update Rate: %d sec\n",pCmd->VTArg1);

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

               DPRINT(1,"VtDecodeCmd: already Regulated\n");
	       
            }
	    break;

     case VT_SETSLEW:		/* HiLand only */
          DPRINT1(1,"VtDecodeCmd: Set VT Temp Slew Rate: %d \n",pCmd->VTArg1);
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
     case VT_SET_INTERLOCK:  pVtId->VTinterLck = pCmd->VTArg1; 
          DPRINT1(2,"vt set interlock to %d\n",pCmd->VTArg1); 
      break; 
     case VT_SET_TYPE:  pVtId->VTtype = pCmd->VTArg1;  
           DPRINT1(2,"vt set type to %d\n",pCmd->VTArg1); 
      break; 
     case VT_SET_ERRORMODE: pVtId->VTerrorType = pCmd->VTArg1; break;
     case VT_SETCALIB:		/* HiLand only */
         errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_SETCALIB not implemented yet.");
      break;

     case VT_GETHEATPWR:	/* HiLand only */
         errLogRet(LOGIT,debugInfo,"VtDecodeCmd: VT_GETHEATPWR not implemented yet.");
      break;
          
     case VT_GETSWVER:		/* HiLand only ???*/
          vtGetSW(pVtId); 
      break;
     case VT_SET_RANGE:	  if (pCmd->VTArg1 > 0)
          pVtId->VTRange = pCmd->VTArg1;
      break;
      case VT_SET_SLEW:	  if (pCmd->VTArg1 > 10)
          pVtId->VTTempRate = pCmd->VTArg1;
      break;
   default: printf("Could not decode %d  %d  %d  %d\n",
               pCmd->VTCmd,pCmd->VTArg1,pCmd->VTArg2,pCmd->VTArg3);
             

     }
}

int
vtGetSW(VT_ID pVtId)
{
   int cmd,rstat;

   clearport( pVtId->VTport );
   cmd = (int) 'V';
   pputchr(pVtId->VTport, cmd);            /* send command */
      
   cmdecho(pVtId->VTport,CMD);
   rstat = readreply(pVtId->VTport,300,pVtId->VTIDSTR,128);
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
     zStr[257] = '\0';
     printf("vtGetZ:\n'%s'\n",zStr);
     return(value);
   }
   return(-1);
}
#endif
