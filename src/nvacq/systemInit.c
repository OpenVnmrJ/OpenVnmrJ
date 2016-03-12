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
#include <vxWorks.h>
#include <taskLib.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include <hostLib.h>

#include "logMsgLib.h"

#include "gpio405Lib.h"

#include "nvhardware.h"
#include "dmaMsgQueue.h"
#include "dmaDrv.h"

#include "mBufferLib.h"
#include "nameClBufs.h"
#include "instrWvDefines.h"
#include "sysUtils.h"
#include "taskPriority.h"

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

/* #include "taskPrior.h" */
#include "flags.h"

#define HOSTNAME_SIZE 80

/*
   System Initialization
   1. Make Message Qs required by the various Tasks
   2. Instantiate hardware Objects
   3. Open the needed Channels
   4. Start the needed tasks
*/

/* Revision Number for Interpreter & System */
extern int SystemRevId; 
extern int InterpRevId;

#define MAX_BRD_TYPE 7
static char *brdTypeStr[8] = { "master", "rf", "pfg", "gradient" , "lock", "ddr", "lpfg", "reserved2" };

/* Board Type, rf,master,ddr,etc., and board number, rf1,rf2,rf3, etc. */
extern int BrdType;
extern int BrdNum;

extern char hostName[HOSTNAME_SIZE];

extern NDDS_ID NDDS_Domain;

/*   
 *   First call routine to initialize all IPCs, structures, and Tasks
 *
 */

extern MBUFFER_ID mbufID ;
extern NCLB_ID nClBufId;

#define NUM_CLUSTERS 4

/* You must make sure the value is inline with the sizes 
   and number of clusters allocated above  */
#define NAME_BUF_NUM 100

extern void* nvld(char *filename);

static int delayOn, delayOff;

int ConsoleTypeFlag;
char fpgaLoadStr[40];

nddsPri(int level)
{
   int tid;

  if ((tid = taskNameToId("n_rtu7400")) != ERROR)
  {
     taskPrioritySet(tid,level);
  }

}

void createDomain(int debuglevel)
{
   char nicIP[40];
   printf("-------  systemInit  createDomain\n");
   /* create Domain Zero, no debug output, no multicasting */
   NDDS_Domain =  nddsCreate(0,debuglevel,1, (char *) getLocalIP( nicIP ) );
}

char *getBrdTypeStr(int brdtype)
{
    char *typestr;
    if ((brdtype <= 7) && (brdtype >= 0))
       typestr = brdTypeStr[brdtype];
    else
       typestr = NULL;
    return typestr;
}

int cntlrName2TypeNNum(char *id, int *type, int *num)
{
    char name[16], numstr[16];
    int i,j,k,len;
    len = strlen(id);
    j = k = 0;
    for(i=0; i< len; i++)
    {
       if (!isdigit(id[i]))
       {
	 name[j++] = id[i]; 
       }
       else
       {
        numstr[k++] = id[i]; 
       }
    }
    name[j] = numstr[k] = 0;
    *num = atoi(numstr);
    for (i=0; i < MAX_BRD_TYPE; i++)
    {
      if (strcmp(name,brdTypeStr[i]) == 0)
	break;
    }
    *type = i;
    
    return 0;
}

/*
prtTypes()
{
   int i;
   for(i=0; i<= MAX_BRD_TYPE; i++)
    printf("addr: 0x%lx, str: '%s'\n",brdTypeStr[i],brdTypeStr[i]);
}
*/
initNDDS(int debug)
{
   /* intialize NDDS Command P/S link to Expproc */
   createDomain(debug);
   /* taskDelay(calcSysClkTicks(166)); */
   /* nddsPri(52); */
#ifdef RTI_NDDS_4x
   // Originially it was assummed that the NDDS receive task rR00 - rR03 were static in which traffic
   // they received. But in reality they are each a member of a pool of receive task, and thus 
   // they are not assigned a specific task and may handle any of the receive taffic.   3/3/08  GMB
   // I leave the attempt below for future reference only so we don't forget and try this again.  GMB
   //
   // NDDS_ChngMetaRecvTasksPriority();   // the receive task are a pool of task, thus changing priority
                                          // doesn't help.
#endif
}

int Progress = 0;
int ExitFlag = 0;

int prtFlag(int flag)
{
  Progress=flag;
  printf("\n=========>>>>> %d\n", flag);
  taskDelay(calcSysClkTicks(100));
  return (ExitFlag == flag);
}

/* consoletype FPGA load flag for Nirvana vs Magnus */
systemInit(char *basePath, int BringUpMode, int debuglevel, int consoleType, int flags)
{
   int  iter,stat;
   int  status;
   int  TypeNBrdNum;
   int  nddsdebug;
   int  FPGA_Id, FPGA_Rev, FPGA_Chksum, BD_ID, BD_Type;
   long moduleId;
   char *type;
   char execFile[256],fpgaFile[256];
   char tmpstr[30];
   char *nddsverstr(char*);

   DebugLevel = debuglevel;
   nddsdebug = (DebugLevel >2) ? 1 : -1;

   changeIfSendQMax("emac0",200);    /* increase if input Q from default 50 to 200 */
   changeProtocolInputQMax(200, 50); /* increase udp input Q to 200, leave arp input at it's default of 50 */

   logMsgCreate(500);
   logMsgPri(LOGMSG_TASK_PRIORITY); /* change priority of logMsg task from 1 to 6 */

   /* Obtain card Type */
   TypeNBrdNum = readGeoId();
   if (FLAGS_FORCE_GEOID(flags))
     TypeNBrdNum = flags & 0xffff;
   BrdType = (TypeNBrdNum >> 8) & 0xff;
   BrdNum = TypeNBrdNum & 0xff;
   type = getBrdTypeStr(BrdType);
   if (type == NULL) {
     diagPrint("invalid board type %d\n", BrdType);
     return -1;
   }

   // sets ExitFlag to lower bits of flags if upper bits are set to 0xFFxxxx
   ExitFlag = FLAGS_EXIT_IF(flags);

   if (FLAGS_FORCE_GEOID(flags)) {
     sprintf(hostName,"%s%d", type, BrdNum+1); /* lpfg masquerades as pfg1 */
     DPRINT2(-1,"Board Type *FORCED* to : '%s', ordinal#: %d\n", type, BrdNum);
   } else {
     DPRINT2(-1,"Board Type: '%s', ordinal#: %d\n", type, BrdNum);
   }

   if (BrdType == LPFG_BRD_TYPE_ID) {
     sprintf(hostName,"pfg%d", BrdNum+1); /* lpfg masquerades as pfg1 */
     DPRINT1(3,"host name changed to %s\n", hostName);
   } else {
     stat = gethostname(hostName,80);
     DPRINT3(3,"host name remains %s (type %d != LPFG type %d\n",hostName, BrdType, LPFG_BRD_TYPE_ID);
   }
   DPRINT1(-1,"->>> hostname: '%s'\n",hostName);

   /* intialize NDDS Command P/S link to Expproc */
   if (BringUpMode > 0)
      initNDDS(nddsdebug);

   EXIT_IF_FLAGGED(1);
     
   /* Initialize DMA Drivers */
   if ( (BrdType == RF_BRD_TYPE_ID) || 
        (BrdType == PFG_BRD_TYPE_ID) || 
	(BrdType == LPFG_BRD_TYPE_ID) ||
        (BrdType == GRAD_BRD_TYPE_ID) )
   {
       dmaInit(1024,2048);    /* dma queue size = 1024, SG size 2048 */
   }
   else if (BrdType == DDR_BRD_TYPE_ID)
   {
       dmaInit(1024,SG_MSG_Q_MAX);   /* 1024 , 256 */
   }
   else
   {
       dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);   /* 64 , 256 */
   }

   /* after executables are loaded add to the system memory partition */
   /* VisionWare start at 0x03E00000 and is 0x00200000 length (2MB) */
   /* End System memory pool, 0x01ffffff */
   /* so add from 0x02000000 - 0x01DFFFFF to the system memory pool */
   /* memAddToPool(Ox02000000,0x01E00000); */

   /* generate the names of executable and FPGA files to load */
   /* 
      a given basepath implies to load from a network drive, 
      a NULL basepath implies to load from flash
   */

   /* 
    * 1st try getting it from disk or flash, if this fails then use compiled in bit file
    */
   EXIT_IF_FLAGGED(2);
     
   DPRINT1(-1,"consoleType: %d\n",consoleType);
   ConsoleTypeFlag = consoleType;
   if (consoleType != 1)
   {
      sprintf(fpgaFile,"%s/%s_top.bit",basePath,type);
      sprintf(fpgaLoadStr,"%s_top.bit",type);
   }
   else
   {
      sprintf(fpgaFile,"%s/mag%s_top.bit",basePath,type);
      sprintf(fpgaLoadStr,"mag%s_top.bit",type);
   }
          
   EXIT_IF_FLAGGED(3);
     
   if (basePath != NULL)
   {
     DPRINT1(-1,"++++++ Loading Network FPGA file: '%s'\n",fpgaFile);
     status = loadVirtex(fpgaFile);
   }
   else
   {
     DPRINT1(-1,"++++++ Loading Flash FPGA file: '%s'\n",fpgaLoadStr);
     status = nvload(fpgaLoadStr);
   }

   if (status != 0)
     DPRINT1(-1,"++++++ FPGA file: '%s', not present. Use Internel FPGA Configuration file\n",
	     basePath ? fpgaFile : fpgaLoadStr);
   EXIT_IF_FLAGGED(4);
     
   if (basePath != NULL)
   {
      sprintf(execFile,"%s/%sexec.o",basePath,type);
      diagPrint(NULL,"++++++ Loading  Exec file: '%s'\n",execFile);
      moduleId = (long) ld(0,1,execFile); /* globals added to symtable, do not abort on error */
   }
   else
   {
     sprintf(execFile,"%sexec.o",type);
     diagPrint(NULL,"++++++ Loading  Exec file: '%s'\n",execFile);
     moduleId = (long) nvld(execFile); /* globals added to symtable, do not abort on error */
   }

   EXIT_IF_FLAGGED(5);
   if (moduleId == 0)
   {
      diagPrint(NULL,"****** Exec file: '%s', failed to load properly.\n",execFile);
      blinkLED(calcSysClkTicks(83), calcSysClkTicks(166));  /* fail mode */  /* blinkLED(5,10); */
      return -1;
   }

   EXIT_IF_FLAGGED(6);
   if (status != 0)
   {
     DPRINT(-1,"++++++ Loading  Internal FPGA Configuration file\n");
     status = execFunc("loadFpgaArray",(void*)consoleType, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
     if (status != 1)
     {
        diagPrint(NULL,"****** FPGA file: Internal FPGA Configuration file failed to load\n");
        blinkLED(calcSysClkTicks(17),calcSysClkTicks(50)); /* fail mode */  /* blinkLED(1,3); */
     }
   }

   taskDelay(calcSysClkTicks(166));  /* Observations seem to indicate that we might be access the FPGA before
                                        it is really ready so as a check I've added this delay */

   EXIT_IF_FLAGGED(7);
     
   status = execFunc("checkFpgaVersion",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
   if (status != 0)
   {
      blinkLED(1,3); /* fail mode */
      if (!FLAGS_IGNORE_MD5(flags))
        return -1;
   }
   
   if (BringUpMode < 1)
      return 0;

   EXIT_IF_FLAGGED(8);
     
   status = execFunc("BringUp",(void*)&BrdType,(void*) &BrdNum, (void*) &BringUpMode, (void*) basePath, (void*)flags, NULL, NULL, NULL); 

   if (status != 0)
   {
     blinkLED(calcSysClkTicks(17),calcSysClkTicks(50)); /* fail mode */  /* blinkLED(1,3); */
     diagPrint(NULL,"****** Error: %d, on bring up\n",status);
   }
   else
   {
     diagPrint(NULL,"++++++ BringUp Complete..\n");
     killBlinker();
   }
 
   diagPrint(NULL," - System Version: %d\n", SystemRevId);
   diagPrint(NULL," - Interpreter Version: %d\n", InterpRevId);
   diagPrint(NULL," - Bootup Complete.\n");
}

whatsloaded()
{
   printf(" '%s' loaded into FPGA,  consoleType: %d\n",fpgaLoadStr,ConsoleTypeFlag);
   return 0;
}

char *nddsverstr(char *str)
{
   int vernum,major,minor,build;
   char verletter;

#ifndef RTI_NDDS_4x
   vernum = NddsVersionGet();
   major = (vernum >> 24) & 0xff;
   minor = (vernum >> 16) & 0xff;
   verletter = ((vernum >> 8) & 0xff) + 0x61;   /* encode letter starting at zero, 61+0 = 'a' */
   build = vernum & 0xff;

   sprintf(str,"ndds%d.%d%c_rev%d",major,minor,verletter,build);
#else  /* RTI_NDDS_4x */
   const struct DDS_ProductVersion_t *pProdVer;
   pProdVer = NDDS_Config_Version_get_product_version();
   sprintf(str,"ndds%d.%d%c_rev%d",pProdVer->major,pProdVer->minor,pProdVer->release,pProdVer->revision);
#endif  /* RTI_NDDS_4x */
  
   return str;
}

prtnddsver()
{
   char verstr[80];
   nddsverstr(verstr);
   printf("NDDS Version: '%s'\n",verstr);
   return 0;
}

logMsgPri(int priority)
{
   int pTmpId;
   pTmpId = taskNameToId("tLogTask");
   taskPrioritySet(pTmpId,priority);
   return 0;
}

chgPrior()
{
   int pTmpId;
   pTmpId = taskNameToId("tRlogind");
   taskPrioritySet(pTmpId,70);
   pTmpId = taskNameToId("tRlogOutTask");
   taskPrioritySet(pTmpId,71);
   pTmpId = taskNameToId("tRlogInTask");
   taskPrioritySet(pTmpId,72);
   pTmpId = taskNameToId("tWdbTask");
   taskPrioritySet(pTmpId,73);
   pTmpId = taskNameToId("tMsgLogd");
   taskPrioritySet(pTmpId,75);
   pTmpId = taskNameToId("tPHandlr");
   taskPrioritySet(pTmpId,76);
   pTmpId = taskNameToId("tLogTask");
   taskPrioritySet(pTmpId,77);
}

/* :TODO: delete all of these if they aren't used */
startTasks() {}
resetBufs()  {}
killTasks()  { return 0; }
sysEnable()  { return 0; }
sysDisable() { return 0; }
sysReset()   { return 0; }
resetAll()   { killTasks(); } /* delete All Tasks and buffers and restart */
systemConRestart() { return 0; } // clean system and restart all applications

sysStat()
{
   printf("System Version: %d\n", SystemRevId);
   printf("Interpreter Version: %d\n", InterpRevId);
   return 0;
}

syshelp()
{
   printf("Status and Stat Information commands:\n");
   printf("sysfifo(level)   - Print the State of the FIFO Object\n");
   printf("sysfiforesrc()   - Print the System Resources Used\n");
   printf("sysstm(level)    - Print the State of the STM  Object\n");
   printf("sysstmresrc()    - Print the System Resources Used\n");
   printf("sysadc(level)    - Print the State of the ADC  Object\n");
   printf("systune(level)   - Print the State of the Tune Object\n");
   printf("systuneresrc()   - Print the System Resources Used\n");
   printf("sysacode(level)  - Print the State of the Acode Object\n");
   printf("sysacodesrc()    - Print the System Resources Used\n");
   printf("sysmsgqs(level)  - Print the State of the Various Acquisition Message Queues\n");
   printf("syssems(level)   - Print the State of the Various Acquisition Semaphore\n");
   printf("sysdlbs(level)   - Print the State of the Various Name Buffers\n");
   printf("sysdlbsresrc()   - Print the System Resources Used\n");
   printf("sysChanDump(level)   - Print the bytes in the channel ports.\n");
   printf("sysresrc()       - Print the Global System Resources Used\n");
   printf("sysresrcdump()   - Print All System Resources Used\n");
}

/*---------------------------------------------------------
*  taskSrchDelete - delete all task with the basename passed
*
*/
taskSrchDelete(char *basename)
{
    FAST int    nTasks;                 /* number of task */
    FAST int    ix;                     /* index */
    int         idList [1024]; /* list of active IDs */
    int		baselen;
    char 	*tName;
    char 	tcname[40];
    char	*taskName(int tid);
  
    baselen = strlen(basename);
    /* printf("basename='%s', len=%d\n",basename,baselen); */

    nTasks = taskIdListGet (idList, 1024);
    taskIdListSort (idList, nTasks);
 
    for (ix = 0; ix < nTasks; ++ix)
    {
      tName = taskName(idList[ix]);
      if ( tName != NULL )
      {
	memcpy(tcname,tName,baselen);  /* just compare the basename char */
        tcname[baselen] = '\0';
        /*printf("base:'%s', task:'%s', taskf:'%s'\n",basename,tcname,tName);*/
        if (strcmp(basename,tcname) == 0)
	   taskDelete(idList [ix]);
      }
    }
   return 0;
}

/*
 *******************************************************************************************
  LED routines common to all controllers regiser 0x70000004, 0=LED 'on',  1=LED 'off'
  leds 2-6 are available
 *******************************************************************************************
*/
#define PANEL_LED_REG  ( (volatile unsigned long* const) (0x70000000 + 0x4))
panelLedOn(int ledNum)  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
{
   int bit;
   bit = ( 1 << (ledNum-1));
   *PANEL_LED_REG = *PANEL_LED_REG & ~(bit);
}

panelLedOff(int ledNum)
{
   int bit;
   bit = ( 1 << (ledNum-1));
   *PANEL_LED_REG = *PANEL_LED_REG | bit;
}

panelLedAllOff()
{
   *PANEL_LED_REG = 0x3f;
}

panelLedMaskOn(int ledMask)  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
{
   *PANEL_LED_REG = *PANEL_LED_REG & ~(ledMask);
}

panelLedMaskOff(int ledMask)  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
{
   *PANEL_LED_REG = *PANEL_LED_REG | ledMask;
}


/*
 * routine to turn on/off or blink the Yellow LED on controller panel
 *
 *  Author: Greg Brissey  5/5/04
 */
ledOn()
{
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | FPGA_PROG_LED); /* Turn on LED */
}

ledOff()
{
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & (~(FPGA_PROG_LED))); /* Turn Off LED */
}

blinker()
{
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & (~(FPGA_PROG_LED))); /* Turn Off LED */
   FOREVER
   {
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | FPGA_PROG_LED); /* Turn on LED */
      taskDelay(delayOn);
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & (~(FPGA_PROG_LED))); /* Turn Off LED */ 
      taskDelay(delayOff);
   }
}

blinkLED(int on, int off)
{
   int taskPriority,taskOptions,stackSize;
   taskPriority = 100;
   taskOptions = 0;
   stackSize = 1024;

   delayOn = (on < 1) ? 1 : on;
   delayOff = (off < 3) ? 3 : off;

   if (taskNameToId("tBlinker") == ERROR)
     taskSpawn("tBlinker",taskPriority,taskOptions,stackSize,blinker,
                1,2,3,4,5,6,7,8,9,10);
}

adjBlinkRate(int on, int off)
{
   delayOn = (on < 1) ? 1 : on;
   delayOff = (off < 3) ? 3 : off;
}

killBlinker()
{
   int tid;

   if ((tid = taskNameToId("tBlinker")) != ERROR)
     taskDelete(tid);
   ledOff();
}
