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

   Gradient specific routines

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <stdlib.h>
 
#include "logMsgLib.h"
#include "nvhardware.h"
#include "fifoFuncs.h"
#include "gradient.h"
#include "md5.h"
#include "taskPriority.h"

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */
extern int SafeXAmpVal;	        /* PFG or Gradient X Amp settings */ 
extern int SafeYAmpVal;		/* PFG or Gradient Y Amp settings */ 
extern int SafeZAmpVal;		/* PFG or Gradient Z Amp settings */ 
extern int SafeB0AmpVal;	/* Gradient B0 Amp settings */ 
extern int SafeXEccVal;		/* Gradient X ECC settings */ 
extern int SafeYEccVal;		/* Gradient Y ECC settings */ 
extern int SafeZEccVal;		/* Gradient Z ECC settings */ 
extern int SafeB0EccVal;	/* Gradient B0 ECC settings */ 

extern int XYZshims[];		/* initialize X, Y and Z1 shim values here */

extern char fpgaLoadStr[40];

/*
 * Gradient version of this
 * common routine function that checks the FPGA for proper checksum
 * against sofware expected checksum.
 * Each type of card has it's own 'checkFPGA()' routine.
 *
 *    Author: Greg Brissey 5/5/04
 */
int checkFpgaVersion()
{
   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   FPGA_Id = get_field(GRADIENT,ID);
   FPGA_Rev = get_field(GRADIENT,revision);
   FPGA_Chksum = get_field(GRADIENT,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,GRADIENT_CHECKSUM,__DATE__);
   if ( GRADIENT_CHECKSUM != FPGA_Chksum )
   {
     DPRINT(-1,"GRADIENT VERSION CLASH!\n");
     return(-1);
   }
   else
     return(0);
}

/*===========================================================================*/
/*
 * all board type specific initializations done here.
 *
 */
initBrdSpecific(int bringup_mode)
{
   XYZshims[0] = XYZshims[1] = XYZshims[2];
   initSPI();   /* initialize SPI to proper configuration */
   startGradParser(GRADPARSER_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
   set_register(GRADIENT,FIFOECCCalcSelect,3);
   gradCmdPubPatternSub();
   startShimRestorer(200,STD_TASKOPTIONS, STD_STACKSIZE);
}

/*===========================================================================*/
/* Safe State Related functions */
/*===========================================================================*/

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

void setSafeB0Amp(int B0)
{
   SafeB0AmpVal = B0;
}

void setSafeXYZEcc(int X, int Y, int Z)
{
   SafeXEccVal = X;
   SafeYEccVal = Y;		
   SafeZEccVal = Z;		
}

void setSafeB0Ecc(int B0)
{
   SafeB0EccVal = B0;
}

/*
 * reset fpga SW register to their safe values
 * used if other code uses the SW register to set hardware
 *
 *  Author Greg Brissey    1/12/05
 */
void resetSafeVals()
{
   extern void resetClearReg();
   extern void resetDelays();
   set_register(GRADIENT,SoftwareGates,SafeGateVal);
   set_register(GRADIENT,SoftwareUser,0);
   set_register(GRADIENT,SoftwareXAmp,SafeXAmpVal);
   set_register(GRADIENT,SoftwareYAmp,SafeYAmpVal);
   set_register(GRADIENT,SoftwareZAmp,SafeZAmpVal);
   set_register(GRADIENT,SoftwareB0Amp,SafeB0AmpVal);
   set_register(GRADIENT,SoftwareXEcc,SafeXEccVal);
   set_register(GRADIENT,SoftwareYEcc,SafeYEccVal);
   set_register(GRADIENT,SoftwareZEcc,SafeZEccVal);
   set_register(GRADIENT,SoftwareB0Ecc,SafeB0EccVal);
   resetClearReg();
   resetDelays();
}

/*====================================================================================*/
/* assert the backplane failure line to all boards */
void assertFailureLine()
{
   set_field(GRADIENT,sw_failure,0);
   set_field(GRADIENT,sw_failure,1);
   set_field(GRADIENT,sw_failure,0);
   return;
}

/*====================================================================================*/
/* assert the backplane warning line to all boards */
void assertWarningLine()
{
   set_field(GRADIENT,sw_warning,0);
   set_field(GRADIENT,sw_warning,1);
   set_field(GRADIENT,sw_warning,0);
   return;
}

/*====================================================================================*/

#ifdef XXXXXXXXX
int resetGradAmps()
{
   set_field(GRADIENT,grad_reset,0);
   set_field(GRADIENT,grad_reset,1);
   while( !get_field(GRADIENT,grad_reset_done) );
   return (1);
}
#endif


#ifdef FPGA_COMPILED_IN

#include "gradient_top.c"

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
   strcpy(fpgaLoadStr,"gradient_top");
   if ((ret = uncompress(buffer,&destLen,gradient_top,sizeof(gradient_top))) != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }
   /*
   DPRINT1(-1,"Actual length: %lu\n",destLen);
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",gradient_top_md5_checksum);
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
   if (strcmp(hex_output, gradient_top_md5_checksum)) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",gradient_top_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,0);
   free(buffer);
   return(status);
}

#endif


/*====================================================================================*/
/*====================================================================================*/

/* include the FPGA BASE ISR routines */
/* define the the controller type for proper conditional compile of ISR register defines */
#define GRADIENT_CNTLR
#include "fpgaBaseISR.c"
#include "A32Interp.c"
