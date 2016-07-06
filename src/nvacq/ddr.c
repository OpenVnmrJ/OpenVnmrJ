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
6-03-04,gmb  created
*/

/*
DESCRIPTION

   DDR specific routines

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <stdlib.h>
#include <memLib.h>
 
#include "logMsgLib.h"
#include "nvhardware.h"
#include "fifoFuncs.h"
#include "semLib.h"
#include "dataObj.h"
#include "ddr.h"
#include "md5.h"
 
extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */

extern char fpgaLoadStr[40];

extern void ddrParserCmnd(int id, int a1,int a2, int a3, int a4);

//=========================================================================
// initBrdSpecific board initialization function
//=========================================================================
void initBrdSpecific(int bringup_mode,char* basePath)
{
    extern int initDDR(int,char*);
    extern void initUpLink(void);
    extern DATAOBJ_ID dataCreate(int dataChannel,char *idStr);

    pTheDataObject = dataCreate(1,"DDR");
    
    initialFidCtStatComm();  /* Fid & Ct status update Pub */

    initUpLink();  /* start the data Uplink Task and related NDDDS Publication */

    initDDR(bringup_mode,basePath); 
}



/*====================================================================================*/
/* Safe State Related functions */
/*====================================================================================*/

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
   set_register(DDR,SoftwareGates,SafeGateVal);
}

/*  Invoked by Fail line interrupt to set serialized output to their safe states */
void goToSafeState()
{
   /* switch to software control */
   /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); */
   /* assertSafeGates();  /* FPGA should of done this already but we do it on software as well */
   /* serialSafeVals();  /* serialize out proper X,Y,Z Amp values */ 
   DPRINT(-1,"goToSafeState() invoked.\n");
}

//=========================================================================
// checkFpgaVersion board id test function
//=========================================================================
int checkFpgaVersion()
{    
    int FPGA_Id = get_field(DDR,ID);
    int FPGA_Rev = get_field(DDR,revision);
    int FPGA_Chksum = get_field(DDR,checksum);

    diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,DDR_CHECKSUM,__DATE__);

    if(DDR_CHECKSUM==FPGA_Chksum){  // compare with value in ddr.h
		return 0; // checksum okay
	}
    else {
	    diagPrint(NULL," DDR VERSION CLASH!\n");
	    diagPrint(NULL," Checksum: %d, Expected: %d\n", FPGA_Chksum, DDR_CHECKSUM);
		return -1;
	}
}


ddrd()
{
   ddr_debug_msgs(1);
   ddr_debug_scans(1);
}

ddrdoff()
{
   ddr_debug_msgs(0);
   ddr_debug_scans(0);
}

/*====================================================================================*/
/* assert the backplane failure line to all boards */
void assertFailureLine()
{
   set_field(DDR,sw_failure,0);
   set_field(DDR,sw_failure,1);
   set_field(DDR,sw_failure,0);
   return;
}

/*====================================================================================*/
/* assert the backplane warning line to all boards */
void assertWarningLine()
{
   set_field(DDR,sw_warning,0);
   set_field(DDR,sw_warning,1);
   set_field(DDR,sw_warning,0);
   return;
}


#ifdef FPGA_COMPILED_IN
/* #define FPGA_COMPILED_IN */

#include "ddr_top.c"
#define Z_OK            0
#define UNCOMPRESSED_SIZE (510400+1024)

loadFpgaArray()
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
   DPRINT(-1,"loadFpgaArray(): Uncompressing\n");
   strcpy(fpgaLoadStr,"ddr_top");
   if ((ret = uncompress(buffer,&destLen,ddr_top,sizeof(ddr_top))) != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }
   /*
   DPRINT1(-1,"Actual length: %lu\n",destLen);
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",ddr_top_md5_checksum);
   */

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
   if (strcmp(hex_output, ddr_top_md5_checksum)) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",ddr_top_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   printf("loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,0);
   free((void *)buffer);
   return(status);
}

#endif

#define DSP_BIN_COMPILED_IN
#ifdef DSP_BIN_COMPILED_IN
/* #define DSP_BIN_COMPILED_IN */

#include "boot_bin.c"
#include "ddr_bin.c"
/* #define UNCOMPRESSED_SIZE 510398 */
/* #define UNCOMPRESSED_SIZE 510400 */

/* for some reason the DPRINT do not print, so I just printf here. */

loadDspBootArray()
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
   int writeDDRbuffer(char *memptr, UINT32 fSize);

   buffer = (unsigned char *) malloc(UNCOMPRESSED_SIZE);
   destLen = UNCOMPRESSED_SIZE;
   /* DPRINT(-1,"loadFpgaArray(): Uncompressing\n"); */
   if (DebugLevel > -1)
      printf("loadDspBootArray(): Uncompressing\n");
   if ((ret = uncompress(buffer,&destLen,boot_bin,sizeof(boot_bin))) != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }
/*
   DPRINT1(-1,"Actual length: %lu\n",destLen);
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",boot_bin_md5_checksum);
*/
   if (DebugLevel > -1)
   {
      printf("loadDspBootArray(): Actual length: %lu\n",destLen);
      printf("loadDspBootArray(): MD5 Given Chksum: '%s'\n",boot_bin_md5_checksum);
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
   /* DPRINT1(-1,"MD5 Given Chksum: '%s'\n",hex_output); */
   if (strcmp(hex_output, boot_bin_md5_checksum)) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",boot_bin_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   /* DPRINT2(-1,"loadDspBootArray(): addr: 0x%lx, size: %lu\n",buffer,destLen); */
   if (DebugLevel > -1)
     printf("loadDspBootArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);

   status = writeDDRbuffer(buffer, destLen);
   free(buffer);
   return(status);
}


loadDspDDRArray()
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
   int writeDDRbuffer(char *memptr, UINT32 fSize);

   buffer = (unsigned char *) malloc(UNCOMPRESSED_SIZE);
   destLen = UNCOMPRESSED_SIZE;
   /* DPRINT(-1,"loadDspDDRArray(): Uncompressing\n"); */
   if (DebugLevel > -1)
      printf("loadDspDDRArray(): Uncompressing\n");
   if ((ret = uncompress(buffer,&destLen,ddr_bin,sizeof(ddr_bin))) != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }
   /* DPRINT1(-1,"loadDspDDRArray(): Actual length: %lu\n",destLen); */
   /* DPRINT1(-1,"loadDspDDRArray(): MD5 Given Chksum: '%s'\n",ddr_bin_md5_checksum); */
   if (DebugLevel > -1)
   {
      printf("loadDspDDRArray(): Actual length: %lu\n",destLen);
      printf("loadDspDDRArray(): MD5 Given Chksum: '%s'\n",ddr_bin_md5_checksum);
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
   /* DPRINT1(-1,"MD5 Given Chksum: '%s'\n",hex_output); */
   if (strcmp(hex_output, ddr_bin_md5_checksum)) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",ddr_bin_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   /* DPRINT2(-1,"loadDspDDRArray(): addr: 0x%lx, size: %lu\n",buffer,destLen); */
   if (DebugLevel > -1)
      printf("loadDspDDRArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = writeDDRbuffer(buffer, destLen);
   free(buffer);
   return(status);
}


#endif



/*====================================================================================*/

/* include the FPGA BASE ISR routines */
/* define the the controller type for proper conditional compile of ISR register defines */
#define DDR_CNTLR
#include "fpgaBaseISR.c"
#include "A32Interp.c"
