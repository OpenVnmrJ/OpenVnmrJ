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
/*
DESCRIPTION

   Lock specific routines

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <stdio.h>
#include <string.h>
#include <vxWorks.h>
#include "logMsgLib.h"
#include "taskPriority.h"
#include "fpgaBaseISR.h"
#include "lock.h"
// :FIXME: this is temporarily set to rf.h instead of lock.h to test an issue with nvzlib uncompress
//#include "rf.h"
#include "lock.h"
#include "nvhardware.h"
#include "md5.h"

extern char fpgaLoadStr[40];

/* 
 * Lock version of this common routine function that checks the FPGA for proper checksum
 * against sofware expected checksum.
 * Each type of card has it's own 'checkFPGA()' routine.
 *
 *    Author: Greg Brissey 5/5/04
 */
int checkFpgaVersion()
{
   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   FPGA_Id = get_field(LOCK,ID);
   FPGA_Rev = get_field(LOCK,revision);
   FPGA_Chksum = get_field(LOCK,checksum);
   DPRINT4(-1,"FPGA ID; %d, Rev: %d, Chksum: %ld, Compiled: %s\n",FPGA_Id,FPGA_Rev,FPGA_Chksum,__DATE__);
    /* if (get_field(LOCK,checksum) != LOCK_CHECKSUM) */
   if ( LOCK_CHECKSUM != FPGA_Chksum )
   {
     DPRINT2(-1,"Lock VERSION CLASH! %d (FPGA) != %d (Lock.h)\n",
             FPGA_Chksum, LOCK_CHECKSUM);
     return(-1);
   }
   else
     return(0);
}

#if !defined(PFG_LOCK_COMBO)
initBrdSpecific(int bringup_mode)
{
    extern void initLock(int);
    initLock(0);
    haltLedShow();
}
#endif

#ifdef FPGA_COMPILED_IN
#ifdef DONT_INCLUDE_C_FILES
extern const char *lock_top_md5_checksum;
extern const unsigned char lock_top[];
extern int lock_top_size();
#else
#include "lock_top.c"
#endif

#define Z_OK            0
#define UNCOMPRESSED_SIZE (510400+1024)

loadFpgaArray(int consoleType, int xtraKB, int skipX)
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
   int top_size;

#ifdef DONT_INCLUDE_C_FILES
   top_size = lock_top_size();
#else
   top_size = sizeof(lock_top);
#endif
   buffer = (unsigned char *) malloc(UNCOMPRESSED_SIZE+xtraKB*1024);
   if (buffer == NULL) {
     DPRINT1(-2,"loadFpgaArray: could not allocate %d bytes\n", UNCOMPRESSED_SIZE);
     return -1;
   }
   destLen = UNCOMPRESSED_SIZE;
   strcpy(fpgaLoadStr,"lock_top");
   DPRINT5(2,"loadFpgaArray(): Uncompressing %d bytes of %s (@0x%x) into %d bytes @0x%x\n", top_size, fpgaLoadStr, lock_top, UNCOMPRESSED_SIZE, buffer);
   if ((ret = uncompress(buffer,&destLen,lock_top,top_size)) != Z_OK)

   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",ret,destLen);
      free(buffer);
      return -1;
   }

   DPRINT1(-1,"Actual length: %lu\n",destLen);
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",lock_top_md5_checksum);

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
   if (strcmp(hex_output, lock_top_md5_checksum)) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",lock_top_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,0);
   free(buffer);
   return(status);
}

#endif

/* dummy routine that bringup expects */
startDwnLnk(int pri, int taskopt, int stkSize) {}
initFifo() {}
startParser() {}
initShandler() {}
//setAcqState(int state) { /* send2Master(CNTLR_CMD_SET_ACQSTATE, state); */ }

#ifndef DONT_INCLUDE_C_FILES
#define LOCK_CNTLR
#include "fpgaBaseISR.c"
#endif
