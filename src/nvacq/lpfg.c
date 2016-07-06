/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 * Agilent Technologies, Inc.
 * information of Agilent Technolgies, Inc. and its contributors.
 */
/*
   DESCRIPTION
   Combined PFG+Lock specific routines

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <stdlib.h>
#include <memLib.h>
#include "taskPriority.h"
#include "logMsgLib.h"
#include "nvhardware.h"
#include "fpgaBaseISR.h"
#include "fifoFuncs.h"
#include "md5.h"

#ifdef USE_PFG
#define LPFG_TOP pfg_top
#define LPFG_TOP_MD5_CHECKSUM pfg_top_md5_checksum
#define LPFG_TOP_SIZE pfg_top_size()
#define LPFG_TOP_NAME "pfg_top"
#include "pfg.h" 
#else
#define LPFG_TOP lpfg_top
#define LPFG_TOP_MD5_CHECKSUM lpfg_top_md5_checksum
#define LPFG_TOP_SIZE lpfg_top_size
#define LPFG_TOP_NAME "lpfg_top"
#include "lpfg.h"               /* PFG and Lock functionality are logically separated */
#endif

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */
extern int SafeXAmpVal;	        /* PFG or Gradient X Amp settings */ 
extern int SafeYAmpVal;		/* PFG or Gradient Y Amp settings */ 
extern int SafeZAmpVal;		/* PFG or Gradient Z Amp settings */ 

extern char fpgaLoadStr[40];

int checkFpgaVersion()
{
   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   FPGA_Id = get_field(PFG,ID);
   FPGA_Rev = get_field(PFG,revision);
   FPGA_Chksum = get_field(PFG,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,PFG_CHECKSUM,__DATE__);
   if ( PFG_CHECKSUM != FPGA_Chksum )
   {
     DPRINT(-1,"PFG VERSION CLASH!\n");
     return(-1);
   }
#ifndef USE_PFG
   FPGA_Id = get_field(LOCK,ID);
   FPGA_Rev = get_field(LOCK,revision);
   FPGA_Chksum = get_field(LOCK,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,LOCK_CHECKSUM,__DATE__);
   if ( LOCK_CHECKSUM != FPGA_Chksum )
   {
     DPRINT(-1,"LOCK VERSION CLASH!\n");
     return(-1);
   }
#endif
   return(0);
}

int LockISRIndex = -1;

#define __register_addr(fpga,field) (FPGA_BASE_ADR + fpga##_##field)
#define REGISTER_ADDR(fpga,field) __register_addr(fpga,field)

int ExtSyncCnt = 0;
void ext_sync_handler(int status, int val)
{
  ExtSyncCnt++;
  logMsg("~~~~~~~~~~~~>> External Sync seen %d times - status %x val %x \n",ExtSyncCnt, status,val,3,4,5,6);
}

/*====================================================================================*/
/*
 * all board type specific initializations done here.
 *
 */
initBrdSpecific(int bringup_mode)
{
   int status, pfg_mask, sync_mask, lock_slice;

   pfg_mask = get_mask(PFG,sw_int_status);
   DPRINT1(2,"installing PFG SW signal handlers with mask 0x%x\n", pfg_mask);
   initShandler(pfg_mask, PFG_sw_int_status_pos);

   DPRINT1(2,"installing PFG ext sync signal handler @0x%x\n", ext_sync_handler);
   set_field(PFG, external_sync_enable, 1); // enable external sync interrupt
   sync_mask = get_mask(PFG,external_sync_status);
   fpgaIntConnect(ext_sync_handler, 0, sync_mask);

   setFifoMasks(get_mask(PFG,fifo_overflow_status), get_mask(PFG,fifo_underflow_status), 
		get_mask(PFG,fifo_finished_status), get_mask(PFG,fifo_started_status), 
		get_mask(PFG,fail_int_status), get_mask(PFG,warn_int_status), 
		get_mask(PFG,data_fifo_almost_empty_status), get_mask(PFG,instr_fifo_almost_full_status),
		get_mask(PFG,invalid_opcode_int_status));

   if (bringup_mode < 9) { /* :TODO: get rid of this conditional - for LPFG bringup testing only! */
     lock_slice = initFpgaSliceISR(get_pointer(LOCK,InterruptStatus), 
				   get_pointer(LOCK,InterruptEnable), 
				   get_pointer(LOCK,InterruptClear));
     DPRINT1(2,"installing lock SW signal handlers (slice %d)\n", lock_slice);
     initLock(lock_slice);
   }
   //lock_mask = get_mask(LOCK,sw_int_status);

   /* start the forth interpreter - will fail if simonet.o & friends weren't linked in */
   status = execFunc("simonet", (void*) 5555,(void*) 15000, NULL, NULL, NULL, NULL, NULL, NULL);
   if (status == ERROR)
   {
      blinkLED(1,3); /* fail mode */
      diagPrint(NULL,"****** Error: %d, simonet failed to start\n", status);
   }
}

/*====================================================================================*/
/* Safe State Related functions */
/*====================================================================================*/

void setSafeGate(int value)
{
   SafeGateVal = value;		
}

void setSafeXYZAmp(int X, int Y, int Z)
{
   SafeXAmpVal = X;
   SafeYAmpVal = Y;		
   SafeZAmpVal = Z;		
}

/*
 * reset fpga SW register to their safe values
 * used if other code uses the SW register to set hardware
 *
 *  Author Greg Brissey    1/12/05
 */
void resetSafeVals()
{
   set_register(PFG,SoftwareGates,SafeGateVal);
   set_register(PFG,SoftwareUser,0);
   set_register(PFG,SoftwareXAmp,SafeXAmpVal);
   set_field(PFG,sw_xamp_updated,1);
   set_register(PFG,SoftwareYAmp,SafeYAmpVal);
   set_field(PFG,sw_yamp_updated,1);
   set_register(PFG,SoftwareZAmp,SafeZAmpVal);
   set_field(PFG,sw_zamp_updated,1);
}


/*====================================================================================*/
/* assert the backplane failure line to all boards */
void assertFailureLine()
{
   set_field(PFG,sw_failure,0);
   set_field(PFG,sw_failure,1);
   set_field(PFG,sw_failure,0);
   return;
}

/*====================================================================================*/
/* assert the backplane warning line to all boards */
void assertWarningLine()
{
   set_field(PFG,sw_warning,0);
   set_field(PFG,sw_warning,1);
   set_field(PFG,sw_warning,0);
   return;
}

/*====================================================================================*/
pfg_AMP_enable(int arg)
{
  int k;
  /* X axis */
  set_field(PFG,pfg_amp_address,0);  
  set_field(PFG,pfg_amp_enable,arg);
  return 0;
}

/*====================================================================================*/
/*
 * extracted from Debbie's test code
 * with modifications
 *
 *		Greg Brissey  9/15/04
 */
int resetPfgAmps()
{
   set_field(PFG,pfg_amp_reset,0);
   set_field(PFG,pfg_amp_reset,1);
   while( !get_field(PFG,pfg_amp_reset_done) );
   return (1);
}

int disablePfgAmps()
{
  set_field(PFG,pfg_amp_address,0);  /* disable X axis */
  set_field(PFG,pfg_amp_enable,0);
  set_field(PFG,pfg_amp_address,1);  /* disable Y axis */
  set_field(PFG,pfg_amp_enable,0); 
  set_field(PFG,pfg_amp_address,2);  /* disable Z axis */
  set_field(PFG,pfg_amp_enable,0); 
}

/*====================================================================================*/
int xEnable()
{
   int i;
   set_field(PFG,pfg_amp_address,0);
   set_field(PFG,pfg_amp_enable,1);
   return (1);
}

/*====================================================================================*/
int yEnable()
{
   int i;
   set_field(PFG,pfg_amp_address,1);
   set_field(PFG,pfg_amp_enable,2);
   return (1);
}

/*====================================================================================*/
int zEnable()
{
   int i;
   set_field(PFG,pfg_amp_address,2);
   set_field(PFG,pfg_amp_enable,4);
   return (1);
}

/*====================================================================================*/
int setSwXAmpValue(int value)
{
  set_field(PFG,sw_xamp,value);
  set_field(PFG,sw_xamp_updated,1);
  set_field(PFG,sw_update,0);   /* bit files after 10/21 require this */
  set_field(PFG,sw_update,1);
  set_field(PFG,sw_xamp_updated,0);
  while( get_field(PFG,pfg_amp_busy) & 0x1 );
  return(1);
}

/*====================================================================================*/
int setSwYAmpValue(int value)
{
  printf("pfg_amp_busy: 0x%lx\n",get_field(PFG,pfg_amp_busy));
  set_field(PFG,sw_yamp,value);
  set_field(PFG,sw_yamp_updated,1);
  set_field(PFG,sw_update,0);        /* bit files after 10/21 require this */
  set_field(PFG,sw_update,1);
  set_field(PFG,sw_yamp_updated,0);
  printf("pfg_amp_busy: 0x%lx\n",get_field(PFG,pfg_amp_busy));
  /* while( get_field(PFG,pfg_ybusy) ); */
  while( get_field(PFG,pfg_amp_busy) & 0x2 );
  return(1);
}

/*====================================================================================*/
int setSwZAmpValue(int value)
{
  set_field(PFG,sw_zamp,value);
  set_field(PFG,sw_zamp_updated,1);
  set_field(PFG,sw_update,0);       /* bit files after 10/21 require this */
  set_field(PFG,sw_update,1);
  set_field(PFG,sw_zamp_updated,0);
  while( get_field(PFG,pfg_amp_busy) & 0x4 );
  return(1);
}

/*====================================================================================*/
int getXAmpOk()
{
   set_field(PFG,pfg_amp_address,1);
   return(get_field(PFG,pfg_amp_ok));
}

/*====================================================================================*/
int setYAmpOk(int value)
{
   set_field(PFG,pfg_amp_address,1);
   return(get_field(PFG,pfg_amp_ok));
}

/*====================================================================================*/
int getZAmpOk(int value)
{
   set_field(PFG,pfg_amp_address,2);
   return(get_field(PFG,pfg_amp_ok));
}

#ifdef FPGA_COMPILED_IN
#ifdef DONT_INCLUDE_C_FILES               
extern const unsigned char LPFG_TOP [];   
extern const char *LPFG_TOP_MD5_CHECKSUM; 
extern const size_t LPFG_TOP_SIZE;
#else
#include "lpfg_top.c"
#endif

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
   DPRINT1(-1,"loadFpgaArray(): Uncompressing %s\n",LPFG_TOP_NAME);
   strcpy(fpgaLoadStr,LPFG_TOP_NAME);
   ret = uncompress(buffer,&destLen,LPFG_TOP,LPFG_TOP_SIZE);

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
   ret = strcmp(hex_output, LPFG_TOP_MD5_CHECKSUM);

   if (ret != 0) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",LPFG_TOP_MD5_CHECKSUM);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,2);
   free(buffer);
   return(status);
}

#endif


/* include the FPGA BASE ISR routines */
/* define the the controller type for proper conditional compile of ISR register defines */
#ifndef DONT_INCLUDE_C_FILES
#ifndef PFG_CNTLR
#define PFG_CNTLR
#endif
#include "fpgaBaseISR.c"
#include "A32Interp.c"
#endif
