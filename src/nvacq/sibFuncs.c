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




/************************************************************************/
/*									*/
/* This file contains the code to interact with the SIB modules		*/
/* that is, the Safety Interface Board					*/
/* The hard work is done by the modules themslf. They keep track of the */
/* status of the various Gradient Amplifiers and in the case of the ILI */
/* also the RF amplifier. In case of problems the IS or ILI will shut   */
/* things down and report an error to the master via the 4th serial     */
/* port on the MIF.  These functions report the status to the host	*/
/*
 */

#include <ioLib.h>
#include "drv/sio/ns16552Sio.h"
//#include <sioLib.h>
#include <semLib.h>
#include "taskPriority.h"
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "sibFuncs.h"
#include "errorcodes.h"
#include "expDoneCodes.h"


#define CR 13
#define CMD 0
#define TIMEOUT 98
#define CMDTIMEOUT -10000
#define DONETIMEOUT	-424242

#define SIB_ILI_BD_ID		4	/* was 4 */
#define SIB_ISI_BD_ID		6
#define SIB_MICRO_BD_ID		8     /* Behaves same as SIB_ISI_BD_ID */

#define sibDPRINT(level, str) \
        if (sibDebugLevel > level) diagPrint(debugInfo,str)

#define sibDPRINT1(level, str, arg1) \
        if (sibDebugLevel > level) diagPrint(debugInfo,str,arg1)

#define sibDPRINT2(level, str, arg1, arg2) \
        if (sibDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2)
 
#define sibDPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (sibDebugLevel > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4)
 
static sibDebugLevel = 0;
static sibPerformaDAbortEnableFlag = 0;

// Diagnostic enabling routines
setSibDebug(int level) { sibDebugLevel = level; return level; }
getSibDebug(int level) { return sibDebugLevel; }
sibDebugOn() { sibDebugLevel = 3; return 3; }
sibDebugOff() { sibDebugLevel = 0; return 0; }
// performa D option
sibAbortEnable() { sibPerformaDAbortEnableFlag = 1; return 1; }
sibAbortDisable() { sibPerformaDAbortEnableFlag = 0; return 0; }


extern Console_Stat *pCurrentStatBlock;
extern SEM_ID        pSemOK2Tune;
extern int           masSpeedTaskId;
extern int           XYZgradflag;	// is gradtype='rrr', 
					// i.e., is this an (u)imager
int	sibDebug=1;
int	tSibTaskId;
SIB_OBJ	*sibObj = NULL;

// errors are not in bit order, but in order we like the msgs to show
static SibError isiAddr0X[] = {		// ISI Addr=0, Ax, Bx, Cx
   { 0x000040,   0,   0, ISIERROR + ISI_XGRAD_FAULT},
   { 0x004000,   0,   0, ISIERROR + ISI_YGRAD_FAULT},
   { 0x400000,   0,   0, ISIERROR + ISI_ZGRAD_FAULT},
   { 0x000010,   0,   0, ISIERROR + ISI_GRAD_AMP_ERR},
   { 0x000020,   0,   0, ISIERROR + ISI_DUTY_CYCLE_ERR},
};

static SibError isiAddr0Y[] = {		// ISI Addr=0, Ay, By, Cy
   { 0x000002,   0,   0, ISIERROR + ISI_SPARE_2},
   { 0x000008,   0,   0, ISIERROR + ISI_OVERTEMP},
   { 0x000010,   0,   0, ISIERROR + ISI_NO_COOLANT},
   { 0x000020,   0,   0, ISIERROR + ISI_SPARE_1},
   { 0x000040,   0,   0, ISIERROR + ISI_SPARE_4},
   { 0x000080,   0,   0, ISIERROR + ISI_SPARE_3},
   { 0x000001,   0,   0, ISIERROR + ISI_SYS_FAULT}, // ...leave as last msg
};

static SibError isiAddr1X[] = {		// ISI Addr=1, Ax, Bx, Cx
   // bits 0-7 are the Coil ID
};

static SibError isiAddr1Y[] = {		// ISI Addr=1, Ay, By, Cy
   // no bit defined
};

static SibError iliAddr0X[] = {		// ISI Addr=0, Ax, Bx, Cx
   { 0x000001,   1,   0, ILIERROR + OBSERROR + ILI_10S_SAR},
   { 0x000002,   1,   0, ILIERROR + OBSERROR + ILI_5MIN_SAR},
   { 0x000004,   1,   0, ILIERROR + OBSERROR + ILI_PEAK_PWR},
   { 0x000008,   8,   0, ILIERROR + OBSERROR + ILI_RF_AMP_CABLE},
   { 0x000010,   8,   0, ILIERROR + OBSERROR + ILI_RF_REFL_PWR},
   { 0x000020,   8,   0, ILIERROR + OBSERROR + ILI_RF_DUTY_CYCLE},
   { 0x000040,   8,   0, ILIERROR + OBSERROR + ILI_RF_OVER_TEMP},
   { 0x000080,   8,   0, ILIERROR + OBSERROR + ILI_RF_PULSE_WIDTH},

   { 0x000100,  16,   4, ILIERROR + DECERROR + ILI_10S_SAR},
   { 0x000200,  16,   4, ILIERROR + DECERROR + ILI_5MIN_SAR},
   { 0x000400,  16,   4, ILIERROR + DECERROR + ILI_PEAK_PWR},
   { 0x000800, 128,   4, ILIERROR + DECERROR + ILI_RF_AMP_CABLE},
   { 0x001000, 128,   4, ILIERROR + DECERROR + ILI_RF_REFL_PWR},
   { 0x002000, 128,   4, ILIERROR + DECERROR + ILI_RF_DUTY_CYCLE},
   { 0x004000, 128,   4, ILIERROR + DECERROR + ILI_RF_OVER_TEMP},
   { 0x008000, 128,   4, ILIERROR + DECERROR + ILI_RF_PULSE_WIDTH},

   { 0x010000,   0,   2, ILIERROR + ILI_QUAD_REFL_PWR},
   { 0x020000,   0,  17, ILIERROR + ILI_RFMON_MISSING},
   { 0x040000,   0,   0, ILIERROR + ILI_OPR_PANIC},
   { 0x080000,   0,   0, ILIERROR + ILI_OPR_CABLE},
   { 0x100000,   0,  64, ILIERROR + ILI_GRAD_TEMP},
   { 0x200000,   0,  64, ILIERROR + ILI_GRAD_WATER},
   { 0x400000,   0,   4, ILIERROR + ILI_HEAD_TEMP},
   { 0x800000,   0,  17, ILIERROR + ILI_ATTN_READBACK},
};
static SibError iliAddr0Y[] = {		// ISI Addr=0, Ay, By, Cy
   { 0x000001,  17,   0,  ILIERROR+ILI_RFMON_WDOG},
   { 0x000002,  17,   0,  ILIERROR+ILI_RFMON_SELFTEST},
   { 0x000004,  17,   0,  ILIERROR+ILI_RFMON_PS},
   { 0x000008,   0,   0,  ILIERROR+ILI_SPARE_3},
   { 0x000010,   0,   0,  ILIERROR+ILI_PS},
   { 0x000020,  32,   0,  ILIERROR+ILI_SDAC_DUTY_CYCLE},
   { 0x000040,   0,   0,  ILIERROR+ILI_SPARE_1},
   { 0x000080,   0,   0,  ILIERROR+ILI_SPARE_2},
};

static SibError iliAddr1X[] = {		// ISI Addr=1, Ax, Bx, Cx
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};
static SibError iliAddr1Y[] = {		// ISI Addr=1, Ay, By, Cy
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};
static SibError iliAddr2X[] = {		// ISI Addr=2, Ax, Bx, Cx
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};
static SibError iliAddr2Y[] = {		// ISI Addr=2, Ay, By, Cy
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};
static SibError iliAddr3X[] = {		// ISI Addr=3, Ax, Bx, Cx
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};
static SibError iliAddr3Y[] = {		// ISI Addr=3, Ay, By, Cy
   { 0x000001, 0, 0, ISIERROR+ISI_SYS_FAULT}
};

static ErrorRegs regs[] =  {
   { isiAddr0X,   sizeof(isiAddr0X)/sizeof(SibError) },
   { isiAddr0Y,   sizeof(isiAddr0Y)/sizeof(SibError) },
   { isiAddr1X,   sizeof(isiAddr1X)/sizeof(SibError) },
   { isiAddr1Y,   sizeof(isiAddr1Y)/sizeof(SibError) },
   { iliAddr0X,   sizeof(iliAddr0X)/sizeof(SibError) },
   { iliAddr0Y,   sizeof(iliAddr0Y)/sizeof(SibError) },
   { iliAddr1X,   sizeof(iliAddr1X)/sizeof(SibError) },
   { iliAddr1Y,   sizeof(iliAddr1Y)/sizeof(SibError) },
   { iliAddr2X,   sizeof(iliAddr2X)/sizeof(SibError) },
   { iliAddr2Y,   sizeof(iliAddr2Y)/sizeof(SibError) },
   { iliAddr3X,   sizeof(iliAddr3X)/sizeof(SibError) },
   { iliAddr3Y,   sizeof(iliAddr3Y)/sizeof(SibError) },
};

extern NS16550_CHAN * masterFpgaSerialChanGet ( int channel );  

void startSibTask()
{
   if (sibObj == NULL) 
      if (sibCreate() == NULL) 
         return;
   tSibTaskId = taskSpawn("tSibTask", VT_TASK_PRIORITY, STD_TASKOPTIONS,
		XSTD_STACKSIZE, sibTask, 1,2,3,4,5,6,7,8,9,10);
   if ( tSibTaskId == ERROR)
   {
      errLogRet(LOGIT,debugInfo,"startMBoxTasks: could not spawn sibTask");
   }
}

void sibShow()
{
int i;
   printf("XYZgradflag = %d\n",XYZgradflag);
   if (sibObj == NULL) 
   {
      printf("sibObj is NULL, run sibCreate\n");
      return;
   }

   printf("sibObj is 0x%x\n", sibObj);
   printf("Sib String = '%s'\n",sibObj->sibStr);
   printf("Port = %d\n", sibObj->sibPort);
   printf("Sib ID = 0x%x  ",sibObj->sibID);
   switch(sibObj->sibID) 
   {   case SIB_ILI_BD_ID:    printf("ILI board\n");	/* was 0x4 */
           break;
       case 0x44:             printf("ILI + ILI2 boards\n");
           break;
       case SIB_ISI_BD_ID:    printf("ISI board\n");
           break;
       case SIB_MICRO_BD_ID:  printf("MICRO board\n");
           break;
       default:               printf("Unknown board\n");
           break;
   }
   printf("Grad ID = %d\n", sibObj->gradId);
   printf("Mutex = %x\n", sibObj->sibMutex);
   printf("Last Status = 0x%x\n", sibObj->sibStatus);
   printf("Last Bypass = 0x%x\n", sibObj->sibBypass);
   printf("rfMon[0]=%x\n",pCurrentStatBlock->rfMonitor[0]);
   printf("Ch1: Power=0x%x Knob=0x%x\n", 
		(63 - ((sibObj->sibRfMonitor[0] >> 10) & 0x3F)), sibObj->sibRfMonitor[0] & 0x1F );
   printf("Ch2: Power=0x%x Knob=0x%x\n", 
		(63 - ((sibObj->sibRfMonitor[1] >> 10) & 0x3F)), sibObj->sibRfMonitor[1] & 0x1F );

   sibShowG(0);
}

int getSibIDs()
{
  if (sibObj == NULL)
     return(0);
  else
     return( (sibObj->sibID<<16) | sibObj->gradId);
}


      
SIB_OBJ *sibCreate()
{
int i, result;
int pwrKnobValue;
NS16550_CHAN *pChan;

   DPRINT(-1,"Creating Safety Interface Board task structures\n");
   if ( (sibObj = (SIB_OBJ *)malloc(sizeof(SIB_OBJ))) == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"sibCreate: Could not Allocate Space:");
      return(NULL);
   }
   memset(sibObj, 0, sizeof(SIB_OBJ) );

   sibObj->gradId  = -1;
   
   sibObj->sibPort = -1;
   sibObj->sibPort = open("/TyMaster/2",O_RDWR,0);   /* port 3 on the MIF */
   ioctl(sibObj->sibPort, FIOBAUDRATE, 38400);

   DPRINT1(-1,"SibPort = %d\n",sibObj->sibPort);
   setSerialTimeout(sibObj->sibPort,25);	/* set timeout to 1/4 sec */
	

   sibObj->sibMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);
   if (sibObj->sibMutex == NULL)
      sibDelete();

   // at bootup we send <null>x<null>x, which confused things
   // send a <cr> and wait a bit to clear out "unknown command"
   pputchr(sibObj->sibPort,'\r');
   result = taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

   result = sibGetId();
   if (result == 0)
   {
      errLogRet(LOGIT,debugInfo,"sibCreate: Could not get Sib ID: ");
      sibDelete();
      return(NULL);
   }
   sibObj->sibID = sibGet('I');
   if (sibObj->sibID == SIB_ISI_BD_ID)
   {  for (i=0; i<8; i++)
      {   pCurrentStatBlock->rfMonitor[i] = -1;
      }
   }
   else
   {  pwrKnobValue = sibGetG(3);
      sibObj->sibRfMonitor[1] = pwrKnobValue;
      pCurrentStatBlock->rfMonitor[1] = (63 - (pwrKnobValue & 0xFC00) >> 10 ) |
                                        (     (pwrKnobValue & 0x001F) << 6 );
      pwrKnobValue = sibGetG(2);
      sibObj->sibRfMonitor[0] = pwrKnobValue;
      pCurrentStatBlock->rfMonitor[0] = (63 - (pwrKnobValue & 0xFC00) >> 10 ) |
                                        ( (pwrKnobValue & 0x001F) << 6 );
      sibObj->sibBypass = pwrKnobValue >> 16;
   }
   printf("rfMon[0]=%x\n",pCurrentStatBlock->rfMonitor[0]);
   return(sibObj);
}

void sibDelete()
{
int tid;
   if (sibObj == NULL)
     return;

   DPRINT1(-1,"sibDelete: deleting %d\n",sibObj->sibPort);

   if ((tid = taskNameToId("tSib")) != ERROR)
	taskDelete(tid);

   if (sibObj->sibPort > 0)
   {
      ioctl(sibObj->sibPort, FIOBAUDRATE, 9600);
      close(sibObj->sibPort);
   }

   if (sibObj->sibMutex)
       semDelete(sibObj->sibMutex);

   free(sibObj);
   sibObj=NULL;
   return;
}

int sibGetId()
{
int cmd,i,rstat;
char *pChr;

   if (sibObj == NULL) return(0);

   clearport( sibObj->sibPort );
   cmd = (int) 'X';
   pputchr(sibObj->sibPort, cmd);            /* send command */
      
   cmdecho(sibObj->sibPort,CMD);
   pputchr(sibObj->sibPort, '\r');            /* send command */
   rstat = readreply(sibObj->sibPort,300,sibObj->sibStr,64);
   pChr=sibObj->sibStr;
   i=0;
   while ( (*pChr != '\r') && (i<64) ) { pChr++; i++; }
   *pChr = 0;
   logMsg("sibGetId: \n'%s', value: %d\n",sibObj->sibStr,rstat);
   if ( (i==0) || (i>62) )
   { errLogRet(LOGIT,debugInfo,"sibGetId: failed.\n");
     return(0);
   }
   return(1);
}
 
int sibGet(int cmd)
{
int value;

   if (sibObj == NULL) return(0);

   acqerrno = 0;       /* clear errno */
//   DPRINT1(1,"sibGet: CMD = %c \n",cmd);
   if (sibObj == NULL)
      return(CMDTIMEOUT);

   if (sibObj->sibMutex != NULL)
   {
     semTake(sibObj->sibMutex, WAIT_FOREVER);
   }
   else
   {
     acqerrno = SAFETYERROR + SIB_COMM_FAIL;
     errLogRet(LOGIT,debugInfo,"sibGet: Mutex Pointer NULL\n");
     return(CMDTIMEOUT);
   }

    clearport(sibObj->sibPort);
    pputchr(sibObj->sibPort,(int)cmd);            /* send command */
    sibDPRINT2( 2,"Send by 'sibGet' :    %d Dec\t     ----> %c\n\n", cmd, cmd);

    /* don't worry about time out here, if we do return then
       the expected returning value will become out of sync
    */
    cmdecho(sibObj->sibPort,CMD);
    pputchr(sibObj->sibPort,'\r');
    value = cmddone(sibObj->sibPort,50);   /* cmddone set acqerrno */
    if ((value == -1) && (acqerrno == DONETIMEOUT))
    {
      acqerrno = SAFETYERROR + SIB_COMM_FAIL;
      semGive(sibObj->sibMutex);
      return(CMDTIMEOUT);
    }
    sibDPRINT1( 2,"sibGet: Value = %d\n",value);
    semGive(sibObj->sibMutex);
    return(value);

}

int sibPut(int cmd, int value)
{
int stat,time_out;

   if (sibObj == NULL) return;

    sibDPRINT2( 0,"sibPut: Cmd = %c  Var = %d ", cmd, value);

    if (sibObj->sibMutex != NULL)
    {
      semTake(sibObj->sibMutex, WAIT_FOREVER);
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"sibPut: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    clearport(sibObj->sibPort);
    pputchr(sibObj->sibPort, cmd);

    /* don't worry about time out here, if we do return then
       the expected value & CR will never be sent */
    cmdecho(sibObj->sibPort, CMD);
    time_out = 100;
    switch(cmd)
    {
        case 'F':	// Coolant flow rate
        case 'T': 	// Grad amp tune select
                echoval(sibObj->sibPort,value);
                pputchr(sibObj->sibPort,CR);
         /* printf("Send 'CR' by 'OxVT' :%d Dec\t     ----> %c\n\n", CR, CR); */
                break;
        default:   break;
    }

    if (cmddone(sibObj->sibPort, time_out) == -1)
    {
       semGive(sibObj->sibMutex);
       return(TIMEOUT);
    }
    semGive(sibObj->sibMutex);
    return(0);

}

int sibSendDutyCycle(int n, int *value)
{
int stat,time_out;
    sibDPRINT1( 1,"sibSendDuty: count = %d", n);

    if (sibObj == NULL) return;

    if (sibObj->sibMutex != NULL)
    {
      semTake(sibObj->sibMutex, WAIT_FOREVER);
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"sibSenDuty: Mutex Pointer NULL\n");
      return(TIMEOUT);
    }

    /* L<cr> with echo */
    clearport(sibObj->sibPort);
    pputchr(sibObj->sibPort, 'L');
    cmdecho(sibObj->sibPort, CMD);
//    pputchr(sibObj->sibPort, '\r');

    time_out = 100;
    sendnvals(sibObj->sibPort,value,4);
    pputchr(sibObj->sibPort,CR);

    if (cmddone(sibObj->sibPort, time_out) == -1)
    {
       semGive(sibObj->sibMutex);
       return(TIMEOUT);
    }
    semGive(sibObj->sibMutex);

    if (n>4) sibPut('F',value[4]);
    else     sibPut('F',16);
    if (n>5) sibPut('T',value[5]);
    return(0);
}
   
int sibGetG(int n)
{
int value;
   if (sibObj == NULL) return;

   if (sibObj->sibMutex != NULL)
   {
      semTake(sibObj->sibMutex, WAIT_FOREVER);
   }
   else
   {
      errLogRet(LOGIT,debugInfo,"sibGetG: Mutex Pointer NULL\n");
      return(CMDTIMEOUT);
   }

   clearport(sibObj->sibPort);
   pputchr(sibObj->sibPort, 'G');
   cmdecho(sibObj->sibPort, CMD);
   echoval(sibObj->sibPort,n);
   pputchr(sibObj->sibPort, '\r');
   value = cmddone(sibObj->sibPort,50);   /* cmddone set acqerrno */
   if ((value == -1) && (acqerrno == DONETIMEOUT))
   {
      sibDPRINT1(-20,"sibGetG:  %d CMDTIMEOUT\n",n);
      acqerrno = SAFETYERROR + SIB_COMM_FAIL;
      semGive(sibObj->sibMutex);
      return(CMDTIMEOUT);
   }
   sibDPRINT2(0,"   sibGetG: Reg=%d Value = 0x%x\n",n,value);
   semGive(sibObj->sibMutex);

   return(value);
}

static int sibCollectG(int n, int *errorCollections)
{
int      value;
SibError *sibs;
int      i,j,ecount;

   value = sibGetG(n);
   if (value == CMDTIMEOUT) return(value);

   // collect the error so we can make the last one a hard_error
   if (sibObj->sibID == SIB_ILI_BD_ID) 
      n += 4;

   sibs = regs[n].errors;
   ecount = regs[n].count;
   j=0;
   sibDPRINT2( 2,"sibCollectG(%d)\n value=0x%x",n,value);
   for (i=0; i<ecount; i++) {
      sibDPRINT2( 2,"low_bits[%d] = 0x%x\n",i,sibs[i].low_bits);
      sibDPRINT2( 2,"bypass_bits[%d] = 0x%x\n",i,sibs[i].bypass_bits);
      if ( (value & sibs[i].low_bits) == 0 )
      {
         if ( (sibs[i].bypass_bits & sibObj->sibBypass) == 0)
         {  
            *errorCollections++ = sibs[i].error_code;
         }
         else
         {
            *errorCollections++ = -sibs[i].error_code;
         }
         sibDPRINT2( 2,"adding error[%d] = 0x%x\n",i,sibs[i].error_code);
         j++;
      }
   }
   return (j);
}

void sibShowG(int sendMsg)
{
int   errorValues[100];
int   i, j, nerrors;

   if (sendMsg) 
   {
      if (pSemOK2Tune != NULL)
      {   i = semTake(pSemOK2Tune, NO_WAIT);  
          if (i == OK) 			// if I got it, then not acquiring
          {   semGive(pSemOK2Tune); 	// but DO give it back
              return;
          }
      }
      else
      {  return;
      }
   }
      
   sibDPRINT1( sibDebug,"Board ID =%d\n",sibObj->sibID);
   if ( (sibObj->sibID == SIB_ISI_BD_ID)   ||
        (sibObj->sibID == SIB_MICRO_BD_ID) ||
        (sibObj->sibID == SIB_ILI_BD_ID) )
   {  nerrors  = sibCollectG(0, &errorValues[0]);
      nerrors += sibCollectG(1, &errorValues[nerrors]);
   }
   else
   {  
       DPRINT1(-1,"Illegal sibID 0x%x\n",sibObj->sibID);
       nerrors = -20;
   }
   sibDPRINT2(sibDebug, "sibShowG(%d), nerrors=%d\n", sendMsg, nerrors);
   if (nerrors == 0) 
   {  errorValues[0] = SAFETYERROR + SIB_NO_ERROR;
      nerrors=1;
   }
   if (nerrors < -1)
   {  errorValues[0] = SAFETYERROR + SIB_COMM_FAIL;
      nerrors = 1;
   }

   for (i=0,j=0; i<nerrors; i++)
   { 
      if (errorValues[i] > 0) 
      {
         errorValues[j] = errorValues[i];
         j++;
      }
      else
      {
         if ( ! sendMsg)
         { sibDPRINT1(-20,"     Bypass = %d\n",errorValues[i]);
         }
      }
   }
   if (j==0) return;  // all bypass
   nerrors=j-1;

   sibDPRINT1( 1,"Error count =%d\n",nerrors);
   for (i=0; i<nerrors; i++)
   { 
      if (sendMsg)
      { // send Error
        sendException(WARNING_MSG, errorValues[i], 0, 0, NULL);
        taskDelay(calcSysClkTicks(500));  /* 1/2 sec, taskDelay(30); */
      }
      else
      { sibDPRINT1(sibDebug,"     Error = %d\n",errorValues[i]);
      }
   }
   if (sendMsg)		// the last one goes as hard_error to abort
   { // send Error
     sendException(HARD_ERROR, errorValues[nerrors], 0, 0, NULL);
     taskDelay(calcSysClkTicks(500));  /* 1/2 sec, taskDelay(30); */
   }
   else
   { sibDPRINT1(sibDebug,"     Error = %d\n",errorValues[nerrors]);
   }
}

int errflag;

void sibTask(int freq)
{
int status;
int pwrKnobValue;
int i;
   FOREVER
   {
      status = sibGet('S');
      sibObj->sibStatus = status;
      sibDPRINT1( sibDebug,"Status=0x%x\n",status);

      /* was there an error and is it now clear? Reset XYZ shims */
      if ( (status != -10000)  && !(status & 0x3C0) && (errflag == 1) )
      {  errflag = 0;
         if (XYZgradflag) 
         { send2Grad(15,0,0); /* reset XYZ */
         }
      }
      /* is there an error, set flag */
      if ( (status != -10000)  && (status & 0x3C0) )
      {  
         errflag = 1;
      }

      sibDPRINT4( sibDebug,"Status=0x%x, errflag: %d, XYZgradflag: %d, sibPerformaDAbortEnableFlag: %d\n",
                  status,errflag,XYZgradflag,sibPerformaDAbortEnableFlag);

      if ( (status != 0x20) && (status != -10000) && (XYZgradflag || sibPerformaDAbortEnableFlag) )
      {
         for (i=0; i<16; i++) 
         {
           switch (status & (1<<i))
            {
             case  0x0000:
                  break;
             case  0x0001:
                  break;
             case  0x0002:
                  break;
             case  0x0004:
                  break;
             case  0x0008:
                  break;
             case  0x0010:
                  break;
             case  0x0020:
                  sibDPRINT( sibDebug,"IPACK II Ready\n");
                  break;
             case  0x0040:
                  sibDPRINT( sibDebug,"Reg 3 Status\n");
                  if (sibObj->sibID == SIB_ILI_BD_ID)
                  {  pwrKnobValue = sibGetG(3);
                     sibObj->sibRfMonitor[1] = pwrKnobValue;
                     pCurrentStatBlock->rfMonitor[1] = 
		            (63 - ( (pwrKnobValue & 0xFC00) >> 10)) |
                            (       (pwrKnobValue & 0x001F) << 6 );
                  }
                  break;
             case  0x0080:
                  sibDPRINT( sibDebug,"Reg 2 Status\n");
                  if (sibObj->sibID == SIB_ILI_BD_ID)
                  {  pwrKnobValue = sibGetG(2);
                     sibObj->sibRfMonitor[0] = pwrKnobValue;
                     pCurrentStatBlock->rfMonitor[0] = 
		                      (63 - ( (pwrKnobValue & 0xFC00) >> 10)) |
                            (       (pwrKnobValue & 0x001F) << 6 );
                     sibObj->sibBypass = pwrKnobValue >> 16;
                     sibDPRINT1( sibDebug,"bypass=%x\n",sibObj->sibBypass);
                  }
                  break;
             case  0x0100:
                  sibDPRINT( sibDebug,"Reg 1 Status\n");
                  i++;
             case  0x0200:
                  sibDPRINT( sibDebug,"Reg 0 Status\n");
                  sibShowG(1);   // if errors this call will result in the error exception being invoked.
                  break;
             case  0x0400:
                  sibDPRINT( sibDebug,"Waiting for Console Data");
                  break;
             case  0x0800:
                  sibDPRINT( sibDebug,"New Gradient ID");
                  sibObj->gradId = (sibGet('C') & 0xFF);
                  // Put the result in place to be sent up
                  sprintf(pCurrentStatBlock->gradCoilId,"%d", sibObj->gradId);
                  // Tell it we have changed values
                  sendConsoleStatus();
                  sibDPRINT1( sibDebug,"Coil=%d\n",sibObj->gradId);
                  break;
             case  0x1000:
                  sibDPRINT( sibDebug,"Gradient ID Cable Absent");
                  break;
             case  0x2000:
                  sibDPRINT( sibDebug,"ISI Status");
                  break;
             case  0x4000:
                  sibDPRINT( sibDebug,"Console Communication Error");
                  break;
             case  0x8000:
                  sibDPRINT( sibDebug,"Internal IPACK II Err Detected");
                  break;
             default:
	               sibDPRINT( sibDebug,"Default? Huh? Should not get this");
                  break;
            }
         }
      }
      if (sibObj->gradId < 0)	// get it at startup
         sibObj->gradId = (sibGet('C') & 0xFF);
      taskDelay(calcSysClkTicks(1000 * freq));  /* taskDelay(60 * freq); */
   }
}


void sibReset()
{
int value;
   if (sibObj == NULL) return;

   if (sibObj->sibMutex != NULL)
   {
      semTake(sibObj->sibMutex, WAIT_FOREVER);
   }
   else
   {
      errLogRet(LOGIT,debugInfo,"sibGetG: Mutex Pointer NULL\n");
      return;
   }

   clearport(sibObj->sibPort);
   pputchr(sibObj->sibPort, 'R');
   cmdecho(sibObj->sibPort, CMD);
   pputchr(sibObj->sibPort, '\r');
   value = cmddone(sibObj->sibPort,50);   /* cmddone set acqerrno */
   if ((value == -1) && (acqerrno == DONETIMEOUT))
   {
      sibDPRINT(-20,"sibGetG:  CMDTIMEOUT\n");
   }
   semGive(sibObj->sibMutex);
   return;
}
