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
3-10-2004,gmb  created
*/

/*
DESCRIPTION

  This Task  handles the Acodes & Xcode DownLoad to the Console computer.
  Communication is via a NDDS publication that the downlinker subscribes to.
  Its actions depending on the commands it receives.

*/


#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>

#include "logMsgLib.h"
#include "rf.h" // :TODO: get rid of this once the get_mask has been moved to initBrdSpecific
#include "nvhardware.h"
#include "mBufferLib.h"
#include "nameClBufs.h"
#include "fpgaBaseISR.h"
#include "AParser.h"
#include "taskPriority.h"
#include "flags.h"
extern const char hostName[];

extern MSG_Q_ID pMsgesToAParser;   /* MsgQ used for Msges to Acode Parser */

/* main cluster buffer allocation for the 'name buffers' */
extern MBUFFER_ID mbufID ;

/* the 'name buffers' based on the cluster buffers */
extern NCLB_ID nClBufId;

extern int NumClustersIn_mbClTbl;

extern MBUF_DESC mbClTbl [];
#define CLB_WV_EVENT_ID 2000 

initDownld()
{
     memAddToPool((char*) 0x02000000,0x01E00000);  /* loading executables after this may result in warning messages */
     mbufID = mBufferCreate((MBUF_DESC *) &mbClTbl, NumClustersIn_mbClTbl, CLB_WV_EVENT_ID, 10 /* MB reserve */ );
     nClBufId = nClbCreate(mbufID,20);
     startDwnLnk(BUFREQ_TASK_PRIORITY,0,8192);
}

int BringUp(long *brdtype, long *brdnum, long* bringup_mode, char* basePath, long flags)
{
   int status;
   ledOn();

   panelLedAllOff();

   /* after executables are loaded add to the system memory partition */
   /* VisionWare start at 0x03E00000 and is 0x00200000 length (2MB) */
   /* End System memory pool, 0x01ffffff */
   /* so add from 0x02000000 - 0x01DFFFFF to the system memory pool */
   memAddToPool((char*) 0x02000000,0x01E00000);  /* loading executables after this may result in warning messages
					    due to the bra PPC instruction that can only span 32MB,
					    Special compiler option can be used, but this does slow execution down
                                            Best to load all executables prior to this call */

   /* set malloc and free debuging options */
   memOptionsSet( MEM_ALLOC_ERROR_LOG_FLAG | MEM_BLOCK_ERROR_LOG_FLAG | MEM_BLOCK_ERROR_SUSPEND_FLAG );

   InitSystemFlags();  /* Local Task State Object */

   /* initialize msgQ used by Master or Controller comm channel */
   pMsgesToAParser = msgQCreate(10, sizeof(PARSER_MSG), MSG_Q_PRIORITY);

   /* all boards bring up Cntlr PS */
   if (*brdtype == MASTER_BRD_TYPE_ID)
   {
      /* start comlink to Expproc */
      execFunc("initialMonitor",NULL, NULL, NULL, NULL, NULL); 
      initialMasterComm();   /* comlink to other controllers */
   } else if (*brdtype == LPFG_BRD_TYPE_ID) {
      /* initialize LPFG's virtual controllers */
      if (strcmp(hostName,"pfg1") != 0) {
	DPRINT1(-1,"Combined PFG Lock controller expected pfg1 but registered %s\n", hostName);
	return -1;
      }
      initialCntlrComm(hostName, *brdtype, *brdnum);   /* comlink to master */

   } else {
      initialCntlrComm(hostName, *brdtype, *brdnum);   /* comlink to master */
   }

   /* phandler task and related NDDS pub/sub */
   initExceptionHandler();

   /* start Node's Heart Beat Publication */
   /* initialNodeHB(); No Longer used, keep down the ethernet traffic  1/29/07  GMB */
   ledOff();

   /* blinkLED(10,10); */
   status = checkFpgaVersion();
   if (status != 0)
   {
      /* publishError(.....); */
      if (!FLAGS_IGNORE_MD5(flags))
        return -1;
   }

   EXIT_IF_FLAGGED(11);
   /* now that FPGA checks out, install the base ISR that all others will register to.*/
   initFpgaBaseISR();
   isrShow(); 

   /* blinkLED(20,10); */
   if (*brdtype != LOCK_BRD_TYPE_ID)
   {
     ledOn();
     /* all controller get Acodes, LC, etc. */
     /* note the 10 MB reserve is the the amount of memory that mBuffer will not malloc past to avoid running the
      * system out of memory if Acodes/pattern require malloc space      10/18/05   GMB
      */
     mbufID = mBufferCreate((MBUF_DESC *) &mbClTbl, NumClustersIn_mbClTbl, CLB_WV_EVENT_ID, 10 /* MB reserve */);

     if ( (*brdtype == RF_BRD_TYPE_ID) ||  (*brdtype == GRAD_BRD_TYPE_ID) || (*brdtype == PFG_BRD_TYPE_ID) )
       nClBufId = nClbCreate(mbufID,4352);    /* ablility to handle 4096 waveforms */
     else
       nClBufId = nClbCreate(mbufID,200);

     /* Start downLinker */
     startDwnLnk(BUFREQ_TASK_PRIORITY,STD_TASKOPTIONS,12000);  // increase for ndds 4.2e
    
     /* iniitialize FIFO functions and FIFO buffers */
     if (!FLAGS_NOHW(flags))
       initFifo();   
    
     /* Start A Parser */
     if (*brdtype != DDR_BRD_TYPE_ID)
       startParser(APARSER_TASK_PRIORITY, STD_TASKOPTIONS, MED_STACKSIZE);
     else
       startParser(DDR_APARSER_TASK_PRIORITY, STD_TASKOPTIONS, MED_STACKSIZE);
    
     if (*brdtype != LPFG_BRD_TYPE_ID) {
	 int mask;
	 mask = get_mask(RF,sw_int_status);  // :TODO: this looks like a bug in all controllers but RF !?
	 initShandler(mask,RF_sw_int_status_pos); // :TODO: delete this - move to InitBrdSpecific
     }
     ledOff();
  }

  /* Function call common to all controllers, 
   * but coded specific to controller type 
   */
  /* DPRINT1(-1,"basePath: '%s'\n",basePath); */
     
  EXIT_IF_FLAGGED(12);
  initBrdSpecific(*bringup_mode,basePath,flags);
  /* TODO: initialize simonet from approximately here */
  EXIT_IF_FLAGGED(13);
  initialTuneTask();

  return 0;
}

chkfpga()
{
  return checkFpgaVersion();
}
