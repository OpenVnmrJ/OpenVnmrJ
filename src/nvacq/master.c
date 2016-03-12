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
/*
modification history
--------------------
5-05-04,gmb  created
*/

/*
DESCRIPTION

   Master specific routines
*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <wdLib.h>
#include <math.h>
#include <taskLib.h>
 
#include "logMsgLib.h"
#include "nvhardware.h"
#include "fifoFuncs.h"
#include "taskPriority.h"
#include "master.h"
#include "master_fifo.h"
#include "Lock_Stat.h"
#include "md5.h"
#include "nsr.h"
#include "Console_Stat.h"
#include "cntlrStates.h"

extern int *pMASTER_SpinnerPulseSelect;
extern int *pMASTER_SpinnerEnable;
extern int *pMASTER_SpinnerSpeed;
extern int *pMASTER_SpinnerCount;

extern int               XYZgradflag;

#define VPSYN(arg) volatile unsigned int *p##arg = 		 \
		(unsigned int *) (FPGA_BASE_ADR + arg)
#define VPSYNRO(arg) volatile unsigned const int const *p##arg = \
		(unsigned int *) (FPGA_BASE_ADR + arg)

VPSYN(MASTER_InterruptStatus);
VPSYN(MASTER_InterruptEnable);
VPSYN(MASTER_InterruptClear);

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */

extern char fpgaLoadStr[40];

extern int ConsoleTypeFlag;

extern Console_Stat	*pCurrentStatBlock;

static int mixedRFError = 0;

extern int calcSysClkTicks(int mseconds);
/*
 * Master version of this
 * common routine function that checks the FPGA for proper checksum
 * against sofware expected checksum.
 * Each type of card has it's own 'checkFPGA()' routine.
 *
 *    Author: Greg Brissey 5/5/04
 */
int checkFpgaVersion()
{
   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   FPGA_Id = get_field(MASTER,ID);
   FPGA_Rev = get_field(MASTER,revision);
   FPGA_Chksum = get_field(MASTER,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,MASTER_CHECKSUM,__DATE__);
    /* if (get_field(LOCK,checksum) != LOCK_CHECKSUM) */
   if ( MASTER_CHECKSUM != FPGA_Chksum )
   {
     DPRINT(-1,"Master VERSION CLASH!\n");
     DPRINT2(-1,"expect: %d, FPGA return: %d\n",MASTER_CHECKSUM, FPGA_Chksum);
     return(-1);
   }
   else
     return(0);
}

/*============================================================================*/
/*
 * all board type specific initializations done here.
 *
 */
#include "cntrlFifoBufObj.h"
extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
int startBlafTask();
initBrdSpecific(int bringup_mode)
{
int   fWords[20];
   initTty();
   initSPI();
   clear_all_shims();        /* zero all shims, reset may rail some */
   initialStatComm();
   createMasterLkSub(SUB_STAT_TOPIC_FORMAT_STR);
   startSpin(1);             /* task to monitor and control Spin */
   initMASSpeed();           /* Check for MAS controller and monitor if found */
   startVTTask();	     /* task to monitor and control VT */
   startSibTask();           /* task to monitor and control SIB */
   startBlafTask();          /* task to track shims in background */
   FidCtStatPatternSub();    /* FId & Ct update from DDR(s) */
   startXParser(XPARSER_TASK_PRIORITY, STD_TASKOPTIONS, XSTD_STACKSIZE);
   enable_UB_ISR(1);         /* enable eject/insert switch on upper barrel */
   enableProbeTuneIsr(1);    /* enable tuning swtiches interrupt */
   startPneuFault();         /* service pneumatic router interrupt */
   /* startAutoLock(ALOCK_TASK_PRIORITY, STD_TASKOPTIONS, BIG_STACKSIZE); */

   wait4MinCntlrs();
   cntlrRollCall();
   configXYZ_Flag(); 
   #ifdef SILKWORM
     setConsoleType();
     if (ConsoleTypeFlag == 1)
     {
        enableProbeTuneIsr(0);    /* disable tuning swtiches interrupt */
     }
   #endif
   cntlrStatesRemoveAll();


   /* Paddle Wheel */
   DPRINT(-1,"MASTER Paddle Wheel\n");
   // SystemSync(8,0);
   fWords[0] = encode_MASTERSetDuration(1,10); /* Don't start on Haltop */
   fWords[0] = encode_MASTERSetDuration(1,0); /* Haltop */
   writeCntrlFifoBuf(fWords,1); 
   cntrlFifoBufForceRdy(pCntrlFifoBuf);
   startCntrlFifo();

   DPRINT(-1,"MASTER Paddle Wheel Go\n");
   // start the Configuration Publication for ndds 4x
   execFunc("initialConfComm",NULL, NULL, NULL, NULL, NULL);

   #ifdef SILKWORM
     DPRINT1(-9,"consoleID = %d (0-VNMRS,1-400-MR,2-SILK_VNMRS,4-SILK_400-MR)\n",pCurrentStatBlock->consoleID);
   #endif
}

cntlrRollCall()
{
   int ncntlrs;
   char cntlrActiveList[256];

   /* populate the controllers in the state object via rollcall */
   cntlrStatesRemoveAll();
   cntlrStatesAdd("master1",CNTLR_READYnIDLE,MASTER_CONFIG_STD);
   rollcall();
   taskDelay(calcSysClkTicks(500));  /* 1/2 sec  taskDelay(30); */
   rollcall();
   ncntlrs =  cntlrPresentGet(cntlrActiveList);
   cntlrSetInUseAll(CNTLR_INUSE_STATE);
   DPRINT2(0,"cntlrs: %d,  '%s'\n",ncntlrs,cntlrActiveList);
}

/*
 * wait for a master1,rf1,ddr1 to respond to rollcall
 */
wait4MinCntlrs()
{
   int cntlrPresentGet(char *cntlrList);
   // char cnltrs[512];
   // numCntlrs = cntlrPresentGet(cnltrs);
   int cntlrPresentsVerify(char *cntlrList, char *missingList);
   
   char *pCntlrStr = "master1,rf1,ddr1" ;
   char missingList[32];
   int i,numMissing,numUnset;
   int numCntlrs;
   char cnltrs[512];

   // wait for a master1,rf1,ddr1 to respond to rollcall
   // try 5 times, roughly 5 seconds
   for( i=0; i < 15; i++)
   {
     // cntlrStatesRemoveAll();
     cnltrs[0] = missingList[0] = 0;
     cntlrRollCall();
     numCntlrs = cntlrPresentGet(cnltrs);
     DPRINT2(-9,"wait4MinCntlrs: %d cntlrd, cntlr(s): '%s'\n",numCntlrs,cnltrs);
     numMissing = cntlrPresentsVerify(pCntlrStr,missingList);
     DPRINT2(-9,"wait4MinCntlrs: %d missing, cntlr(s): '%s'\n",numMissing,missingList);
     #ifdef SILKWORM
       // unset RF is equivalent to missing for SILKWORM
       numUnset = numUnsetRF();
       numMissing = numMissing + numUnset;
       DPRINT1(-9,"wait4MinCntlrs: %d Unset RF(s)\n",numUnset);
     #endif
     if (numMissing == 0)
        break;
     taskDelay(calcSysClkTicks(1000));  /* 1 sec  taskDelay(60); */
   }
   return numMissing;
}

configXYZ_Flag() 
{
   int cntlrPresentsVerify(char *cntlrList, char *missingList);
   char *pCntlrStr = "grad1,pfg1" ;
   char missingList[32];
   int numMissing,gradmissing,pfgmissing;
   missingList[0] = 0;
   numMissing = cntlrPresentsVerify(pCntlrStr,missingList);
   // DPRINT2(-9,"configXYZ_Flag: %d missing, cntlr(s): '%s'\n",numMissing,missingList);
   gradmissing = (int) strstr(missingList,"grad1");
   pfgmissing = (int) strstr(missingList,"pfg1");
   // DPRINT2(-9, "Missing: grad: %d, pfg: %d\n",gradmissing,pfgmissing);

   /* if gradient controller is present and no PFG controller it is reasonable
      to set the XYZgradflag to 15, this will get set properly within the Acode
      interpreter, however we set it here so that sethw commands to set the shims
      in an imager have a chance to work
   */
   if ((gradmissing == 0) && (pfgmissing != 0))
       XYZgradflag = 0xf;

   DPRINT3(0, "configXYZ_Flag: gradient: '%s', pfg: '%s', XYZgradflag: %d\n",
      ((gradmissing == 0) ? "Present" : "Not Present"),
      ((pfgmissing == 0) ? "Present" : "Not Present"),XYZgradflag);
}

chk4MixedRF()
{
   return mixedRFError;
}

setMixedRF(int value)
{
   mixedRFError = value;
}

#ifdef SILKWORM

numUnsetRF()
{
   int nUnset;
   char cntlrConfList[256];
   char cntlrActiveList[256];
   int ncntlrs;

   ncntlrs =  cntlrPresentGet(cntlrActiveList);
   DPRINT2(-9,"cntlrs: %d,  '%s'\n",ncntlrs,cntlrActiveList);

   // scans all rf configs even though given rf1
   nUnset = cntlrConfigGet("rf1",RF_CONFIG_UNSET,cntlrConfList);
   DPRINT2(-9,"Unset RFs: %d, list: '%s'\n",nUnset,cntlrConfList);
   return nUnset;
}
numIcatRF()
{
   int nRfIcats;
   char cntlrConfList[256];
   char cntlrActiveList[256];
   int ncntlrs;
   ncntlrs =  cntlrPresentGet(cntlrActiveList);
   DPRINT2(-9,"cntlrs: %d,  '%s'\n",ncntlrs,cntlrActiveList);
   // scans all rf configs even though given rf1
   nRfIcats = cntlrConfigGet("rf1",RF_CONFIG_ICAT,cntlrConfList);
   DPRINT2(-9,"iCAT RFs: %d, list: '%s'\n",nRfIcats,cntlrConfList);
   return nRfIcats;
}

numVnmrsRF()
{
   int nRfVnmrs;
   char cntlrConfList[256];
   char cntlrActiveList[256];
   int ncntlrs;
   ncntlrs =  cntlrPresentGet(cntlrActiveList);
   DPRINT2(-9,"cntlrs: %d,  '%s'\n",ncntlrs,cntlrActiveList);
   // scans all rf configs even though given rf1
   nRfVnmrs = cntlrConfigGet("rf1",RF_CONFIG_STD,cntlrConfList);
   DPRINT2(-9,"VNMRS RFs: %d, list: '%s'\n",nRfVnmrs,cntlrConfList);
   return nRfVnmrs;
}

setConsoleType()
{
   int iCATFlag, VnmrsFlag; 

   mixedRFError = 0;
   // cntlrRollCall();

   iCATFlag = numIcatRF();   // 0 - no iCAT, 1- iCAT  RFs
   VnmrsFlag = numVnmrsRF(); // number of Vnmrs

   if ( (iCATFlag > 0) && (VnmrsFlag > 0) )
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Both VNMRS & iCAT RFs detected\n");
      mixedRFError = 1;
   }
   // cntlrStatesRemoveAll();
   
   // ConsoleTypeFlag = 0 - VNMRS, 1 - 400-MR
   if (ConsoleTypeFlag == 0)  // VNMRS 
       pCurrentStatBlock->consoleID = (iCATFlag > 0) ? CONSOLE_SILKVNMRS_ID : CONSOLE_VNMRS_ID;
   else if (ConsoleTypeFlag == 1)  // 400-MR
       pCurrentStatBlock->consoleID = (iCATFlag > 0) ? CONSOLE_SILK400MR_ID : CONSOLE_400MR_ID;

   sendConsoleStatus();

   DPRINT1(-9,"consoleID = %d (0-VNMRS,1-400-MR,2-SILK_VNMRS,3-SILK_400-MR)\n",
            pCurrentStatBlock->consoleID);
}

static char *consoleTypeStr[4] = { "VNMRS", "400-MR", "SILK VNMRS", "SILK 400-MR" };

prtConsoleID()
{
   char *strtype;
 
   if ( (pCurrentStatBlock->consoleID >= 0) && (pCurrentStatBlock->consoleID <= 3) )
      strtype = consoleTypeStr[pCurrentStatBlock->consoleID];
   else
      strtype = "Unknown";

   printf("consoleID: '%s'\n",strtype);
   return 0;
}
#endif

/*===========================================================================*/
/* Safe State Related functions */
/*===========================================================================*/

void setSafeGate(int value)
{
   SafeGateVal = value;		
}

/*
*void setSafeXYZAmp(int X, int Y, int Z)
*{
*   SafeXAmpVal = X;
*   SafeYAmpVal = Y;		
*   SafeZAmpVal = Z;		
*}
*/

/*
 * reset fpga SW register to their safe values
 * used if other code uses the SW register to set hardware
 *
 *  Author Greg Brissey    1/12/05
 */
void resetSafeVals()
{
   set_register(MASTER,SoftwareGates,SafeGateVal);
}

/*  Invoked by Fail line interrupt to set serialized output to their safe states */
void goToSafeState()
{
   /* switch to software control */
   /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); */
   /* assertSafeGates();  /* FPGA should have done this already
                          /* but we do it on software as well */
   /* serialSafeVals();   /* serialize out proper X,Y,Z Amp values */ 
   DPRINT(-1,"(empty) goToSafeState() invoked.\n");
}


#ifdef XXXXX
/*   decide just to suspend the task from a list within the flashUpdate.c file */
/*killTask4FFSUpdate()
*{
*   killShandler();
*   killParser();
*   cleanXParser();
*   killConsolePub();
*   killTune();
*   /* killSPinner() */
*}
*/
#endif

/*===========================================================================*/
/* assert the backplane failure line to all boards */
void assertFailureLine()
{
   set_field(MASTER,sw_failure,0);
   set_field(MASTER,sw_failure,1);
   set_field(MASTER,sw_failure,0);
   return;
}

/*===========================================================================*/
/* assert the backplane warning line to all boards */
void assertWarningLine()
{
   set_field(MASTER,sw_warning,0);
   set_field(MASTER,sw_warning,1);
   set_field(MASTER,sw_warning,0);
   return;
}

int masterRotorPhasedRead()
{
  int phase_ticks = get_field(MASTER,spinner_phase);
  return( phase_ticks / 8 );  /* 100 nsec ticks */
}

/*===========================================================================*/

#ifdef FPGA_COMPILED_IN
/* #define FPGA_COMPILED_IN */

#include "master_top.c"
#include "magmaster_top.c"
#define Z_OK            0
#define UNCOMPRESSED_SIZE (510400+1024)

/* consoleType = 0 for Nirvana, 1 for Magnus */
loadFpgaArray(int consoleType)
{
   int status;
   long size;
   int ret;
   unsigned char *buffer;
   unsigned long destLen;
   md5_state_t state;
   md5_byte_t digest[16];
   char hex_output[16*2 + 1];
   int di;

   buffer = (unsigned char *) malloc(UNCOMPRESSED_SIZE);
   destLen = UNCOMPRESSED_SIZE;
   if (consoleType != 1)
   {
     DPRINT(-1,"loadFpgaArray(): Uncompressing master_top\n");
     strcpy(fpgaLoadStr,"master_top");
     ret = uncompress(buffer,&destLen,master_top,sizeof(master_top));
   }
   else
   {
     DPRINT(-1,"loadFpgaArray(): Uncompressing magmaster_top\n");
     strcpy(fpgaLoadStr,"magmaster_top");
     ret = uncompress(buffer,&destLen,magmaster_top,sizeof(magmaster_top));
   }
   if (ret != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }

   /* ---------------------------*/
   /* calc MD5 checksum signiture */
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)buffer, destLen);
   md5_finish(&state, digest);
   /* generate checksum string */
   for (di = 0; di < 16; ++di)   
        sprintf(hex_output + di * 2, "%02x", digest[di]);
   /* ---------------------------*/
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",hex_output);
   if (consoleType != 1)
   {
     ret = strcmp(hex_output, master_top_md5_checksum);
   }
   else
   {
     ret = strcmp(hex_output, magmaster_top_md5_checksum);
   }
   if (ret != 0) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",master_top_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,0);
   free(buffer);
   return(status);
}

#endif

/*============================================================================*/
/*============================================================================*/
/*======================= Timer interrupt tests ============================= */
static LEDon = 0;
void timerISR(int arg1)
{
    int status = mask_value(MASTER,timer_int_status,arg1);
    logMsg("timerISR: status 0x%lx, timer pending?: 0x%lx\n",arg1,status,3,4,5,6);
    if (status)
    {
      logMsg("timer ISR2: doit\n",1,2,3,4,5,6);
      if (!LEDon)
      {
          ledOn();
          LEDon = 1;
      }
       else
      {
          ledOff();
          LEDon = 0;
      }
   }
   return;
}
tstTimer()
{
   /* disable timer */
   if (get_field(MASTER,timer_enable))
       set_field(MASTER,timer_enable,0);

   set_field(MASTER,timer_period,80000000); /* period one second */
   
   fpgaIntConnect(timerISR,0,set_field_value(MASTER,timer_int_status,1));

   /* this order is important, otherwise you get an interrupt 1st thing */
   set_field(MASTER,timer_enable,1);
   set_field(MASTER,timer_int_enable,1);
}

ct()
{
   printf("enable: %d, int enable: %d, status: 0x%lx\n",get_field(MASTER,timer_enable),
		get_field(MASTER,timer_int_enable), *pMASTER_InterruptStatus); 
   printf("cnt: 0x%lx\n", get_field(MASTER,timer_period));
   printf("cnt: %lu\n", get_field(MASTER,timer_period));
}
setct()
{
 set_field(MASTER,timer_period,80000000); /* period one second */
}
ton()
{
   set_field(MASTER,timer_enable,1);
}
toff()
{
   set_field(MASTER,timer_enable,0);
}
ienable()
{
   set_field(MASTER,timer_int_enable,1);
}
idisable()
{
   set_field(MASTER,timer_int_enable,0);
}
tclr()
{
   *((unsigned int *)(MASTER_BASE+MASTER_timer_int_clear_addr)) = 0;
   *((unsigned int *)(MASTER_BASE+MASTER_timer_int_clear_addr)) = 1 << MASTER_timer_int_clear_pos;
}


/*============  pulser tests, status LED, and spinner air pulser ============ */

initPulse()
{
   set_field(MASTER,spin_pulse_enable,0);
   set_field(MASTER,spin_pulse_low,(12500000/2));    /* 5 KHz */
   set_field(MASTER,spin_pulse_period,(25000000/2));
   set_field(MASTER,spin_pulse_enable,1);
}
spinp()
{
   set_field(MASTER,spin_pulse_enable,0);
   set_field(MASTER,spin_pulse_low,25000000);    /* 5 KHz */
   set_field(MASTER,spin_pulse_period,50000000);
   set_field(MASTER,spin_pulse_enable,1);
}

/*================ liquids and MAS rotor speed pulse counters =============== */
chkp()
{
   printf("SpinEnable: 0x%lx, Speed: 0x%lx, count: 0x%lx\n",
	pMASTER_SpinnerEnable,pMASTER_SpinnerSpeed,pMASTER_SpinnerCount);
}
liqstrt()
{
   *pMASTER_SpinnerEnable = 0;
   *pMASTER_SpinnerEnable = 1;
   /* set_field(MASTER,spin_enable,0);
   set_field(MASTER,spin_enable,1);  */
}
liqstp()
{
   *pMASTER_SpinnerEnable = 0;
   /* set_field(MASTER,spinner_enable,0); */
}
liqprt()
{
    printf("MASTER_SpinnerSpeed: %lu\n", *pMASTER_SpinnerSpeed);
    printf("MASTER_SpinnerCount: %lu\n", *pMASTER_SpinnerCount);
    /* printf("MASTER_SpinnerSpeed: %lu\n", get_field(MASTER,spinner_speed));
    printf("MASTER_SpinnerCount: %lu\n", get_field(MASTER,spinner_count)); */
}
/*   WARNING, for slow rates one have at least one period to count
     i.e. 1HZ requires a 1 second measurement before the coutn register has a
          value.
     Well, only the last of the two edges has to fall in the wd time 
*/

/*
 *  watchdog timeout, use by msrate incase the spin speed is slow or it's not spinning
 *
 */
static volatile int msratetimeout = 0;
static volatile int spinnerSelected = 1;  /* 1=liquids 0=solids */

selectSpinner(int spinner)
{
   spinnerSelected = spinner;
   *pMASTER_SpinnerPulseSelect = spinnerSelected;
}
   
wdRoutine()
{
    msratetimeout = 1;
    logMsg("timeout\n",1,2,3,4,5,6);
}
/*
 *  wait for the number of periods to occur then measure rate
 *  timeout if waiting past 2 seconds
 */
mliqrate(int periods)
{
   WDOG_ID WDtimer;
   float calcrate(unsigned long spinrate,unsigned int nperiods);
   long trycnt;
   unsigned long spinrate;
   unsigned int nperiods;

   *pMASTER_SpinnerPulseSelect = spinnerSelected;
    /* disable counter */
   *pMASTER_SpinnerEnable = 0;
   /* clear and re-enable counter */
   *pMASTER_SpinnerEnable = 1;

   trycnt = 0;
   WDtimer = wdCreate();
   msratetimeout = 0;
   wdStart(WDtimer,2 * sysClkRateGet(), wdRoutine,0);
   
   while( ((nperiods = *pMASTER_SpinnerCount) < periods ) &&
          (msratetimeout == 0));
/*
   {
      trycnt++;
      if (trycnt > 1000000)
         break;
   }
*/
   wdCancel(WDtimer);
   if (nperiods < 1)
   {
     printf("slow speed or not spinning\n");
     return;
   }
   *pMASTER_SpinnerEnable = 0;
    spinrate = *pMASTER_SpinnerSpeed; /* get_field(MASTER,spinner_speed); */
    nperiods = *pMASTER_SpinnerCount; /* get_field(MASTER,spinner_count); */
    calcrate(spinrate,nperiods);
    wdDelete(WDtimer);
}

srate()
{
   float calcrate(unsigned long spinrate,unsigned int nperiods);
   unsigned long spinrate;
   unsigned int nperiods;
    double drate,Hz;
    spinrate = *pMASTER_SpinnerSpeed; /* get_field(MASTER,spinner_speed); */
    nperiods = *pMASTER_SpinnerCount; /* get_field(MASTER,spinner_count); */
    calcrate(spinrate,nperiods);
}

float calcrate(unsigned long spinrate,unsigned int nperiods)
{
    double drate,Hz;
    float spinHz;
    printf("period count: 0x%lu, num periods: %d, count/period: %d\n",spinrate,nperiods,spinrate/nperiods);
    drate = spinrate * 12.5;
  
    /* printf("period: %lf nsec\n",drate); */
    drate = drate /  nperiods;
    /* printf("period: %lf nsec\n",drate); */
    spinHz = 1000000000.0/drate;
    printf("spinrate: %lf Hz\n",spinHz);
    return(spinHz);
}

/* blaf replacement for SSHA  */
/* 
   a task is established which has a control loop based 
   on a message Q.
   BLAFCONFIG 2 words.  
   word1 format (not user accessible)
      0-stop, 1-measure lock, 2-track with lock, 3 measure then track.
      4-fid tbd
   word 2 mask for shims. bit 0 = shim index 0 etc.
   we fix step size and naverages... 
*/
   

/* definitions for the MESSAGE HEADER */
#define BLAFCONFIG   (0x101)
#define BLAFMCONFIG  (0x102)
#define BLAFFIDRUN   (0x103)
#define BLAFFID      (0x104)
#define BLAFSIGC     (0x105)
#define BLAFSTOP     (0x106)
#define BLAFIIR      (0x107)
#define BLAFDRIFT    (0x108)

#define LOCK_TRACK       (1)
#define FID_TRACK        (2)
#define MEASURE_LOCK     (4)
#define MEASURE_LOCK_CHAIN (8)
#define BSHIMNUMBER      (8)
#define IIR_WATCH        (42)
#define DRIFT            (256)

#define MMSGSIZE  (256)

struct BLAF_INFO {
  int tid;
  MSG_Q_ID msgQid;
  int currentStep;
  int tcount;
  int activeAverage;
  int lastAverage;
  int number2acquire;
  int thisNum;
  int shimList[BSHIMNUMBER];
  int activeIndex;
  int epsilon;
  int operation;
  int mcount;
  int sum;
  int sum2;
  int kappa;
 }  blaf;

double racc = 0.0;
double fac_iir = 0.908;/* 1/30 hz sample to 3 second tc */
int iirc = 0;

void watch_iir()
{
  racc = fac_iir*racc + (double) getLockLevel();
  iirc++;
  if (!(iirc % 60)) logMsg("iir value = %d\n",(int) racc,2,3,4,5,6);
}
    

void move2next()
{ 
  int tst;
  logMsg("under tolerance limit\n",1,2,3,4,5,6);
  blaf.activeIndex++;
  tst = blaf.shimList[blaf.activeIndex];
  if ((blaf.activeIndex >= BSHIMNUMBER) || (tst < 1) || (tst > 48))
    blaf.activeIndex = 0; /* head of list */
  logMsg("moving to shim index %d\n",blaf.shimList[blaf.activeIndex],2,3,4,5,6);
}

void stepShimDac(int which, int mystep)
{
  int dvec[3],zero,tmp;
  tmp = pCurrentStatBlock->AcqShimValues[which];
 
  zero=0;
  tmp += mystep;
  dvec[0]=0; 
  dvec[1] = which;
  dvec[2]= tmp;
  shimHandler(dvec,&zero,3,0);
  logMsg("stepShimDac %d  %d\n",which,tmp,3,4,5,6);
}

void moveField(int nv, int lv)
{
  int beta,delta;
  beta = nv-lv;
  delta = abs(beta);
 
  if (delta < blaf.epsilon) /* others to do?? */
    {
      blaf.tcount++;
      /* choose the best/last point */
      if (beta < 0) stepShimDac(blaf.shimList[blaf.activeIndex],-1*blaf.currentStep);
      if (blaf.tcount > 2)
	{
          blaf.tcount = 0;
          move2next();
        }
      return;
    }

  if (beta < 0)  /* switches direction */
    blaf.currentStep *= -1;
  stepShimDac(blaf.shimList[blaf.activeIndex],blaf.currentStep);
}

void lockTracker()
{
  blaf.activeAverage += getLockLevel(); 
  blaf.thisNum++;
  if (blaf.thisNum > blaf.number2acquire)
    {
      moveField(blaf.activeAverage,blaf.lastAverage);
      blaf.lastAverage = blaf.activeAverage;
      blaf.thisNum = 0;
      blaf.activeAverage=0;
    }
}

void fidTracker()
{
  int k;
  /*  blaf.activeAverage += getFidQuality(); TBD */
  blaf.thisNum++;
  if (blaf.thisNum > blaf.number2acquire)
    {
      moveField(blaf.activeAverage,blaf.lastAverage);
      blaf.lastAverage = blaf.activeAverage;
      blaf.thisNum = 0;
      blaf.activeAverage=0;
    }
}

int measureLock()
{
  int k; 
  k = getLockLevel();
  blaf.sum +=  k;
  blaf.sum2 += k*k;
  blaf.mcount++;
  blaf.thisNum++;
  if (blaf.thisNum > blaf.number2acquire)
    {
      service1();
      blaf.thisNum=0;
      blaf.operation = 0;  /* could chain */
      return(1);
    }
  return(0);
}

/* arg is a 32 bit mask OR of 2 ^ dac  */
void learn_shims(int arg)
{
  int i,j,temp;
  blaf.activeIndex = 0;
  for (i=0; i < 8; i++) blaf.shimList[i] = -1;
  temp = 0;
  for (i = 0; i < 32; i++)
    {
      j = 1 << i;
      if (arg & j) { blaf.shimList[temp] = i; temp++; }
    }
  if (temp == 0) blaf.shimList[temp]  =  2; /* z1 */
}

void learn_options(int option)
{
   int stepOption, numberOption, toleranceOption;
   stepOption = option & 0xff; 
   if (stepOption != 0) 
     blaf.currentStep = stepOption;
   else 
     blaf.currentStep = 2;
   numberOption = (option >> 8)  & 0xff;
   if (numberOption > 0) 
     blaf.number2acquire = numberOption;
   else
     blaf.number2acquire = 60;
   toleranceOption = (option >> 16) & 0xff;
   if (toleranceOption > 0)
     blaf.epsilon = toleranceOption;
   else
     blaf.epsilon = 60;
}

/* speculative 
---  analytic drift follower
keep two long long time stamp globals to debug...
to track */
long long last_update = 0L;
long long now = 0L;
long long timebase = 19980000000000LL; /* 5 minutes of cpu clock */
double z1acc, z1step;
double z2acc, z2step;
double x1acc, x1step;
double y1acc, y1step;

union ltrans {
  long long lw;
  int iw[2];
};

void init_drifter(double z1s, double z2s, double x1s, double y1s)
{
  union ltrans t;
  vxTimeBaseGet(&t.iw[0],&t.iw[1]);
  now = t.lw;
  z1step = z1s;
  z2step = z2s;
  x1step = x1s;
  y1step = y1s;
  z1acc = (double) pCurrentStatBlock->AcqShimValues[2];
  z2acc = (double) pCurrentStatBlock->AcqShimValues[4]; 
  x1acc = (double) pCurrentStatBlock->AcqShimValues[16];
  y1acc = (double) pCurrentStatBlock->AcqShimValues[17];
} 

void step_drifter()
{
  int dvec[3],zero;
  long long deltat;
  union ltrans temp;
  double delta, kappa;
  vxTimeBaseGet(&temp.iw[0],&temp.iw[1]);
  last_update = now;
  now = temp.lw; 
  deltat = now - last_update;
  delta = deltat / (19980000000.0); /* one minute */
  /* now compute the new values */
  z1acc += delta*z1step; 
  z2acc += delta*z2step;
  x1acc += delta*x1step;
  y1acc += delta*y1step; 
    zero = 0;
    dvec[0]=0; 
    dvec[1] = 2;
    dvec[2]= (int) (z1acc+0.05);
    shimHandler(dvec,&zero,3,0);
    zero = 0;
    dvec[0]=0; 
    dvec[1] = 4;
    dvec[2]= (int) (z2acc+0.05);
    shimHandler(dvec,&zero,3,0);
    zero=0;
    dvec[0]=0; 
    dvec[1] = 16;
    dvec[2]= (int) (x1acc+0.05);
    shimHandler(dvec,&zero,3,0);
    zero = 0;
    dvec[0]=0; 
    dvec[1] = 17;
    dvec[2]= (int) (y1acc+0.05);
    shimHandler(dvec,&zero,3,0);	
}

void blaftask(int a1,int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
{
   int waitPar;
   int action;
   int lastMsgStat;
   int msgSize;
   int msg1,msg2,msg3;
   int buffer[MMSGSIZE];
   waitPar = WAIT_FOREVER;
   while(1)
   {
       lastMsgStat = msgQReceive(blaf.msgQid,(char *)buffer,MMSGSIZE,waitPar);
       /*logMsg("enter loop");*/
       if (lastMsgStat == ERROR) /* really a normal time out */
       {
	 switch (blaf.operation) 
	 {
	   case LOCK_TRACK: 
       	      if (get_field(MASTER,fifo_gates) & 0x20)
               {
		lockTracker(); waitPar = 6;
               }
              else
		waitPar = 2;
	   break;
	   case MEASURE_LOCK:
	      waitPar=6;
	      if (measureLock() == 1) 
                waitPar=WAIT_FOREVER;
	   break; 
           case  MEASURE_LOCK_CHAIN:
              waitPar=6;
	      if (measureLock() == 1) 
              { 
                waitPar=2; 
                blaf.operation = LOCK_TRACK;
              }
	   break;
	 case IIR_WATCH:
	   watch_iir(); break;
         case DRIFT:
	   /* the time expired but you might be acquiring-
              if so try on 32 ms grid until ok */
           if (get_field(MASTER,fifo_gates) & 0x20)
           {
            step_drifter();
            waitPar=3600;
           }
           else
	     waitPar = 2; 
         break;
         default: logMsg("idle",1,2,3,4,5,6);
         }
       }
       else 
       {
	 action = buffer[0];
         msg1  =  buffer[1];
         msg2  =  buffer[2];
         msg3  =  buffer[3];
         switch (action)
	 {
           case BLAFSIGC:    
             waitPar = 6; 
	     blaf.mcount=0; blaf.sum=0; blaf.sum2=0;
	     blaf.operation = MEASURE_LOCK;
             blaf.number2acquire=msg1;
           break;
	   case BLAFCONFIG:
             waitPar=1; 
	     blaf.thisNum=0; 
             blaf.tcount = 0;
	     blaf.activeAverage=0;
             learn_shims(msg1);
	     learn_options(msg2);
             blaf.operation = LOCK_TRACK;
           break;
           case BLAFMCONFIG:
	     logMsg("MConfig",1,2,3,4,5,6);   
             waitPar = 1; 
	     blaf.mcount=0; blaf.sum=0; blaf.sum2=0;
	     blaf.operation = MEASURE_LOCK_CHAIN;
	     blaf.thisNum=0; 
             blaf.tcount = 0;
	     blaf.activeAverage=0;
             learn_shims(msg1);
           break;
	   case BLAFIIR:
             waitPar = 2;
             blaf.operation = IIR_WATCH;
	     break;
	   case BLAFDRIFT:
	     waitPar = 1;
             blaf.operation = DRIFT;
	   break;
           case BLAFSTOP: 
           default:
             waitPar=WAIT_FOREVER;
         } 
       }
     }
}

int startBlafTask()
{
  int i;
  if (blaf.msgQid != 0) return;
  blaf.msgQid = msgQCreate(20,128,MSG_Q_FIFO);
  if (blaf.tid != 0) return; /* no overwrite existing */
  blaf.tid=taskSpawn("blafTask",BLAF_TASK_PRIORITY,VX_FP_TASK,4048,(void *) blaftask,0,0,0,0,0,0,0,0,0,0);
  blaf.currentStep = 2;
  blaf.activeAverage = 0;
  blaf.lastAverage = 0;
  blaf.number2acquire=60;
  blaf.thisNum = 0;
  blaf.shimList[0] = 2;
  blaf.activeIndex=0;
  for (i=1; i < BSHIMNUMBER; i++)
    blaf.shimList[i] = -1;
  blaf.epsilon = 60; 
  blaf.tcount=0;
  blaf.kappa = 0;
  return(0);
}



printBlaf()
{
  int i; 
  printf("blaf tid = %x  msgQ = %x\n",blaf.tid,blaf.msgQid);
  printf("cstep = %d working Average = %d  last Average = %d\n",
	 blaf.currentStep,blaf.activeAverage, blaf.lastAverage);
  printf("active index = %d\n",blaf.activeIndex);
  printf("num 2 count %d current count %d\n",blaf.number2acquire, blaf.thisNum);
  printf("epsilon = %d epsilon est %d \n",blaf.epsilon,blaf.kappa);
  printf("operation = %d\n",blaf.operation);
  for (i=0; i < BSHIMNUMBER; i++)  printf("shim %d is %d in order\n",blaf.shimList[i],i+1);
}

service1()
{
  double tmp,tmp1,tmp2;
  tmp =  (double) blaf.sum2;
  tmp1 = (double) blaf.sum;
  tmp1 *= tmp1;
  tmp1 /= (double) (blaf.mcount);
  tmp -= tmp1;
  tmp = sqrt((tmp)/(double)(blaf.mcount -1)); 
  printf("sd est = %f epsilon=%f\n",tmp,tmp*(blaf.mcount));
  blaf.kappa = (int) (tmp*(blaf.mcount));
  if (blaf.kappa > 120) blaf.kappa = 120;
  if (blaf.kappa < 40)  blaf.kappa = 40;
  return(1);
}

command0(int count)
{
  int message[64];
  message[0] = BLAFSIGC; 
  message[1] = count;

  if (blaf.msgQid != 0) 
    msgQSend(blaf.msgQid,(char *) message, 100, MSG_PRI_NORMAL, -1);
  else
    printf("no message Q\n");
}

void send2Blaf(int command, int shimmask,int option)
{
  int message[4];
  printf("shimmask is arg used\n");
  message[0] = command;
  message[1] = shimmask;  /* shim mask */
  message[2] = option;  /* unused. */
  if (blaf.msgQid != 0) 
    msgQSend(blaf.msgQid,(char *) message, 16 , MSG_PRI_NORMAL, -1);
  else
    printf("no blaf message Q\n");
}

extern void setFifoOutputSelect(int);

void command2(int arg)
{

  setFifoOutputSelect(0);
  printf("command2 sw sets the blaf to ");
  if (arg > 0) 
    printf("on\n");
  else 
    printf("off\n");
  set_field(MASTER,sw_gates,arg);
}
 
/*
tstmalloc(int size)
{
   char *ptr;
   int msize;
   kmallocOutput();
   initMallocDebug();
   printf("malloc\n");
   ptr = malloc(size);
   taskDelay(60);
   printf("free\n");
   free(ptr);
   printf("malloc\n");
   ptr = malloc(size);
   ptr = malloc(size);
   ptr = malloc(size);
   free(ptr);
   ptr = malloc(size);
   taskDelay(60);
   mallocShow();
}
*/
/* include the FPGA BASE ISR routines */
/* define the the controller type for proper conditional compile of ISR register defines */
#define MASTER_CNTLR
#include "fpgaBaseISR.c"
#include "A32Interp.c"
