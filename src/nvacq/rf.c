/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 * Agilent Technologies, Inc. All Rights Reserved.
 */

/*
modification history
--------------------
4-20-04,gmb  created
*/

/*
DESCRIPTION

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <stdioLib.h>
#include <stdlib.h>
#include <vxWorks.h>
#include <taskLib.h>
#include <taskPriority.h>
 
#include "fifoFuncs.h"
#include "FFKEYS.h"
#include "rf.h"
#include "rf_fifo.h"
#include "nvhardware.h"
#include "md5.h"
#include "logMsgLib.h"

#include "dmaDrv.h"
#include "simon.h"
#include "simonet.h"
#include "ficl.h"
#include "Synthesizer.h"
#include "icatSpi.h"

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */

extern char fpgaLoadStr[40];

int RfType = -1;  // standard RF , iCAT RF, etc..   for tests
int HaveAMRS = 0; // assume No AMRS synthesizer by default

/*
 * rf version of this
 * common routine function that checks the FPGA for proper checksum
 * against sofware expected checksum.
 * Each type of card has it's own 'checkFPGA()' routine.
 *
 *    Author: Greg Brissey 5/5/04
 */
int checkFpgaVersion()
{
   int FPGA_Id, FPGA_Rev, FPGA_Chksum;
   FPGA_Id = get_field(RF,ID);
   FPGA_Rev = get_field(RF,revision);
   FPGA_Chksum = get_field(RF,checksum);
   diagPrint(NULL,"  FPGA ID: %d, Rev: %d, Chksum: %ld;  %s Chksum: %d, Compiled: %s\n",
                  FPGA_Id,FPGA_Rev,FPGA_Chksum,__FILE__,RF_CHECKSUM,__DATE__);

   if ( RF_CHECKSUM != FPGA_Chksum )
   {
     DPRINT(-1,"RF VERSION CLASH!\n");
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
#define ONEEIGHTHSEC 10000000
extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int BrdNum;

initBrdSpecific(int bringup_mode,char* basePath, int safegates)
{
  void setSafeGate(int value);
  void resetSafeVals();
  void goToSafeState();
  int  initSynthesizer(void);
  int   fWords[30];
  int   i,j;
  int status;
   
   setSafeGate(safegates&RFAMP_UNBLANK); /* quick fix: solids default blanking */
   
   resetSafeVals();   /* make sure abort,etc has gates go to zero */
   /* since this is an addition to beta, I'm going to be conservitive and only call the new
    * code if safegates is not zero, which it norming IS.   GMB  12/07/2005
   */
   if (safegates != 0)
      goToSafeState();   

   set_field(RF,phase_bias,4);   /* set PhaseBias to 4 */
   set_field(RF,amp_bias,0);   /* set PhaseBias to 4 */
   /* these are the SKEW controls for the transmitter card */ 
   set_field(RF,phase_delay,2);
   set_field(RF,amp_delay,0);
   set_field(RF,xout_delay,0);  /* theory says 6 autotest 0 */

   #ifdef SILKWORM
      initIcat();
   #endif

   /* these lines should be at the end of initBrdSpecific */
   DPRINT1(-1,"Paddle Wheel. RF Brd Num = %d\n", BrdNum);
   i=0;j=0;
   /* only allow for a 5 channel paddle wheel */
   if (BrdNum < 5)
   {
     /* besure to encode the proper safe gate state for paddle wheel */
     fifoEncodeGates(0, safegates&RFAMP_UNBLANK,safegates&RFAMP_UNBLANK, &fWords[i++]);
     while((j<BrdNum) && (j < 5))
     {
      fWords[i++]=(LATCHKEY | DURATIONKEY | ONEEIGHTHSEC);
      fWords[i++]=(LATCHKEY | DURATIONKEY | ONEEIGHTHSEC);
      fWords[i++]=(LATCHKEY | DURATIONKEY | ONEEIGHTHSEC);
      fWords[i++]=(LATCHKEY | DURATIONKEY | ONEEIGHTHSEC);
      j++;
     }
     fWords[i++]=(DURATIONKEY | ONEEIGHTHSEC);
     fWords[i++]=(LATCHKEY | RFPHASEKEY |  0x0000) ;
     fWords[i++]=(LATCHKEY | RFPHASEKEY |  0x4000) ;
     fWords[i++]=(LATCHKEY | RFPHASEKEY |  0x8000) ;
     fWords[i++]=(LATCHKEY | RFPHASEKEY |  0xc000) ;
     fWords[i++]=(LATCHKEY | RFPHASEKEY |  0x0000) ;

     fWords[i++] = encode_RFSetDuration(1,0); /* Haltop */
     writeCntrlFifoBuf(fWords,i); 
     cntrlFifoBufForceRdy(pCntrlFifoBuf);
     startCntrlFifo();
     DPRINT(-1,"End Paddle Wheel\n");
   }

   // RF board-specific forth initializations
   //status = execFunc("initSynthesizer", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
   int task = taskSpawn("tInitSynth", 150, VX_FP_TASK, 15000, initSynthesizer, 
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#if 0
   status = execFunc("simonet", (void*) 5555,(void*) 15000, NULL, NULL, NULL, NULL, NULL, NULL);
   if (status == ERROR)
   {
      blinkLED(1,3); /* fail mode */
      diagPrint(NULL,"****** Error: %d, simonet failed to start\n", status);
   }
#endif
}

/*============================================================================*/
/* Synthesizer functions
/*============================================================================*/
#define FREQ_MHZ_LOW 40
#define FREQ_MHZ_HIGH 999
#define ATTEN_TBL_SIZE (FREQ_MHZ_HIGH - FREQ_MHZ_LOW + 1)
ficlVm* rfFiclVm = NULL;
int rfAttenEnabled = 0;
short rfAttenTbl[ATTEN_TBL_SIZE];

SEM_ID load_done = NULL;

ficlVm* rf_vm_alloc()
{
  ficlVm* vm = simon_vm_alloc();
  ficlVmEvaluate(vm, "load ffs:rf_amrs.4th");
  return vm;
}

void forthEval(ficlVm* vm, char* forth_cmd)
{
  ficlVm* ficl_vm = vm;
  if (vm == NULL)
    ficl_vm = rf_vm_alloc();
  ficlVmEvaluate(ficl_vm, (char *)forth_cmd);
  if (vm == NULL)
    simon_vm_destroy(ficl_vm);
}

void forth(char* forth_cmd)
{
  if (rfFiclVm == NULL)
    rfFiclVm = rf_vm_alloc();
  if (strcmp(forth_cmd, "finish") == 0) {
    simon_vm_destroy(rfFiclVm);
    rfFiclVm = NULL;
  } else {
    forthEval(rfFiclVm, forth_cmd);
  }
}

void amrsLockoutOn(ficlVm* vm)
{
  forthEval(vm, "amrs-atten-lockout-on");
}

void amrsLockoutOff(ficlVm* vm)
{
  forthEval(vm, "amrs-atten-lockout-off");
}

void amrsAltPowerOff(ficlVm* vm)
{
  forthEval(vm, "amrs-alt-power-down");
}

void amrsAltPowerOn(ficlVm* vm)
{
  forthEval(vm, "amrs-alt-power-up");
}

int isAmrs(ficlVm * vm)
{
  ficlVm* rfvm = vm;
  if (vm == NULL)
    rfvm = rf_vm_alloc();
  ficlVmEvaluate(rfvm, "0 amrs-eeprom-read");
  FICL_STACK_CHECK(rfvm->dataStack, 1, 0);
  int len = ficlStackPopInteger(rfvm->dataStack);
  DPRINT1(3, "amrs-eeprom-read returned %d\n", len);
  if (vm == NULL)
    simon_vm_destroy(rfvm);
  return len != 0;
}

int printAmrsAttenTbl()
{
  int i, j, mhz;
  for (i=0, mhz=FREQ_MHZ_LOW; i<ATTEN_TBL_SIZE;) {
    printf("%d: ", mhz);
    for (j=0; j<10 && i < ATTEN_TBL_SIZE && mhz <= FREQ_MHZ_HIGH; i++, j++, mhz++)
      printf("%4d", rfAttenTbl[i]);
    printf("\n");
  }
}

int loadAmrsAttenTbl(ficlVm* vm)
{  
  int i, mhz, atten;
  ficlVm* rfvm = vm;
  char cmd[100];

  if (vm == NULL) // to facilitate running from the VxWorks command line
    rfvm = rf_vm_alloc();

  for (i=0, mhz=FREQ_MHZ_LOW; i<ATTEN_TBL_SIZE; i++, mhz++) {
    sprintf(cmd, "%de6 amrs-atten-value", mhz);
    ficlVmEvaluate(vm, cmd); // to dump 40-999 MHZ");
    atten = ficlStackPopInteger(rfvm->dataStack);
    rfAttenTbl[i] = atten;
  }

  if (!rfAttenEnabled)
    rfAttenEnabled = 1;

  if (vm == NULL)
    simon_vm_destroy(rfvm);
  
  return i;
}

int initSynthesizer(void)
{
  DPRINT(0,"checking for AMRS synthesizer\n");
  ficlVm* vm = rf_vm_alloc();
  if (load_done == NULL)
    load_done = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
  if (isAmrs(vm)) {
    int len;
    semTake(load_done, WAIT_FOREVER);
    len = loadAmrsAttenTbl(vm);
    diagPrint(NULL," %d AMRS synthesizer table entries loaded\n", len);
    amrsLockoutOff(vm);
    diagPrint(NULL," AMRS lockout off\n");
    semGive(load_done);
    HaveAMRS = 1;
    if (BrdNum == 2)
      amrsAltPowerOff(vm);
  } else {
    diagPrint(NULL," no AMRS synthesizer detected");
  }
  simon_vm_destroy(vm);
}

void compileTime()
{
  printf("%s compiled %s %s\n", __FILE__, __TIME__, __DATE__);
}

int watchFreq = -1;

int adviseFreq(int* data, int data_count)
{
   int atten, i, word, status;
   int aux[30];
   int count = ((int) *data) & 0x3f;   // first byte countains byte count
   int mhz = (data[count-1] << 8) | data[count-2];
   int index = mhz < 40 ? 0 : mhz - 40;

#ifdef LOAD_AMRS_ASYNC /* an optimization to improve boot time - defer until there is time to test it */
   if (load_done != NULL && semTake(load_done, NO_WAIT) == ERROR) {    // make sure the system has finished loading the table
     status = semTake(load_done, calcSysClkTicks(20000)); // should be done in under 20 seconds
     if (status == ERROR) {
       diagPrint(debugInfo,"AMRS table load timed out");
     }
     semGive(load_done);
     load_done = NULL;
   }
#endif
   atten = rfAttenTbl[index];
   if (count > sizeof(aux)/sizeof(int)) {
     DPRINT1(LOG_ERR,"tuning word count %d out of range", count);
     return 0;
   }
   DPRINT3(2,"setting attenuation correction %d MHz [%d] to %d\n", mhz, index, atten);
   for (i=0; i<data_count; i++) {
      word = data[i];
      aux[i] = LATCHKEY | AUX | (0xfff & word);
   }
   // rewrite locally computed words
   aux[count-3] = LATCHKEY | AUX | (0x3f & atten);
   aux[count-2] = LATCHKEY | AUX | (0xff & index);
   aux[count-1] = LATCHKEY | AUX | (0xff & index >> 8);

   if (watchFreq == mhz) {
     char tw[MAX_LOGMSG_LENGTH];
     int c = sprintf(tw,"%d tuning words(%d MHz): ", count, mhz);
     for (i=0; i<count && c<(sizeof(tw)-5); i++)
	c += sprintf(tw+c," 0x%02x", data[i]);
     diagPrint(debugInfo,tw);
     diagPrint(debugInfo,"%d TUNING WORD AUX INSTRUCTIONS:", data_count);
     for (i=0; i<data_count && c<(sizeof(tw)-13); i++)
        diagPrint(debugInfo, "\t%2d: 0x%04x", i, aux[i]);
   }
   if (data[count] != 0x1ff)
     DPRINT2(LOG_ERR,"ERROR: end-of-tuning-word expected at %d, got %04x instead", count, data[count]);

   writeCntrlFifoBuf(aux, data_count);
   return data_count;
}

void testAdviseFreq(int atten)
{
  int tw[] = { // 619 MHz
    0x12, 0x01, 0x00, 0x00, 0xfe, 0x50, 0x35, 0x1a, 0x00, 0x00, 
    0x00, 0x2d, 0xa6, 0x65, 0x1a, 0x00, 0x6b, 0x02, 0x1ff
  };
  int count = tw[0], size = sizeof(tw)/sizeof(int);;
  tw[count-3] = atten;
  int rc = adviseFreq(tw,size);
  printf("adviseFreq returned %d/%d\n", rc, size);
}

/*============================================================================*/
/* Safe State Related functions */
/*============================================================================*/

void setSafeGate(int value)
{
   SafeGateVal = value;		
}

/*
 * reset fpga SW register to their safe values
 * used if other code uses the SW register to set hardware
 *
 *  Author Greg Brissey    1/12/05
 */
void resetSafeVals()
{
   set_register(RF,SoftwareGates,SafeGateVal);
}

/*  Invoked by Fail line interrupt to set serialized output to their safe states */
void goToSafeState()
{
   setFifoOutputSelect(SELECT_FIFO_CONTROLLED_OUTPUT);

   /* switch to software control, which will put or the gates */
   setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT);

   /* assertSafeGates();  /* FPGA should of done this already but we do it on software as well */
   /* serialSafeVals();  /* serialize out proper X,Y,Z Amp values */ 
   DPRINT(-1,"goToSafeState() invoked.\n");
   
   setFifoOutputSelect(SELECT_FIFO_CONTROLLED_OUTPUT);
}

/*============================================================================*/
/* assert the backplane failure line to all boards */
void assertFailureLine()
{
   set_field(RF,sw_failure,0);
   set_field(RF,sw_failure,1);
   set_field(RF,sw_failure,0);
   return;
}

/*============================================================================*/
/* assert the backplane warning line to all boards */
void assertWarningLine()
{
   set_field(RF,sw_warning,0);
   set_field(RF,sw_warning,1);
   set_field(RF,sw_warning,0);
   return;
}

/*============================================================================*/

#include "rf_top.c"

// For silkworm the single FPGA provides both 400-MR and VNMRS support
// via a register setting
#ifndef SILKWORM
  #include "magrf_top.c"
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
//
// For silkworm the single FPGA provides both 400-MR and VNMRS support
// via a register setting
//
#ifdef SILKWORM
   DPRINT(-1,"loadFpgaArray(): Uncompressing rf_top\n");
   strcpy(fpgaLoadStr,"rf_top");
   ret = uncompress(buffer,&destLen,rf_top,sizeof(rf_top));
#else
   if (consoleType != 1)
   {
     DPRINT(-1,"loadFpgaArray(): Uncompressing rf_top\n");
     strcpy(fpgaLoadStr,"rf_top");
     ret = uncompress(buffer,&destLen,rf_top,sizeof(rf_top));
   }
   else
   {
     DPRINT(-1,"loadFpgaArray(): Uncompressing magrf_top\n");
     strcpy(fpgaLoadStr,"magrf_top");
     ret = uncompress(buffer,&destLen,magrf_top,sizeof(magrf_top));
   }
#endif
   if (ret != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "ERROR: Uncompress returned %d (%d bytes)\n",
                      ret,destLen);
      free(buffer);
      return -1;
   }
   /*
   DPRINT1(-1,"Actual length: %lu\n",destLen);
   DPRINT1(-1,"MD5 Given Chksum: '%s'\n",rf_top_md5_checksum);
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
#ifdef SILKWORM
   ret = strcmp(hex_output, rf_top_md5_checksum);
#else
   if (consoleType != 1)
   {
     ret = strcmp(hex_output, rf_top_md5_checksum);
   }
   else
   {
     ret = strcmp(hex_output, magrf_top_md5_checksum);
   }
#endif
   if (ret != 0) 
   {
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  True MD5 - '%s'\n",rf_top_md5_checksum);
      errLogRet(LOGIT,debugInfo, "loadFpgaArray():  Calc MD5 - '%s'\n",hex_output);
      return(-1);
   }
   DPRINT2(-1,"loadFpgaArray(): addr: 0x%lx, size: %lu\n",buffer,destLen);
   status = nvloadFPGA(buffer,destLen,0);
   free(buffer);
   return(status);
}

struct RFHOLD {
  int duration;
  int gates;
  int amp;
  int amps;
  int phase;
  int phasec;
}  RF_state;
 
unsigned long totalduration;
extern unsigned long long cumduration;
extern unsigned long long fidduration;

fifoDecode(unsigned int word,int noprt)
{
   void rfdecode(unsigned int word,int noprt);
   rfdecode(word,noprt);
}

void rfdecode(unsigned int word,int noprt)
{
  int tmp,tdata,latched;
  tmp = word;

   if (noprt != 1)
      printf(" 0x%8lx : ",word);
   if (tmp & LATCHKEY)
            latched = 1;
          else
            latched = 0;
          tdata = tmp & 0x3ffffff; /* 26 bits of data */
          tmp &= 0x7C000000;
          switch (tmp)
            {
               case DURATIONKEY:
   		 if (noprt != 1)
                    printf("duration of %7.4f usec",((float) tdata)/80.0);
                 RF_state.duration = tdata;  break;
               case GATEKEY:
   		 if (noprt != 1)
                    printf("mask/gate set to %x",tdata&0xfff);
                 RF_state.gates = tdata; break;
               case RFPHASEKEY:
   		 if (noprt != 1)
                     printf("rf phase pattern set to %x",tdata&0xffff);
                  RF_state.phase = tdata; break;
               case RFPHASECYCLEKEY:
   		 if (noprt != 1)
                     printf("rf phase cycle set to %x",tdata&0xffff);
                  RF_state.phasec = tdata; break;
               case RFAMPKEY:
   		 if (noprt != 1)
                     printf("rf Amp pattern set to %x",tdata&0xffff);
                  RF_state.amp = tdata; break;
               case RFAMPSCALEKEY:
   		 if (noprt != 1)
                     printf("rf Amp Scale set to %x",tdata&0xffff);
                  RF_state.amps = tdata; break;
               case USER:
   		 if (noprt != 1)
                    printf("user data = %x\n",(tdata&0x3)); break;
               case AUX:
   		 if (noprt != 1)
                    printf("aux addr,data = %x,%x\n",(tdata&0xfff)>>6, \
                        tdata & 0xff); break;
            default:
                  printf("don't recognize key!! %x\n",tmp);
           }
          if (latched)
          {
   	     if (noprt != 1)
                printf(" fifo word latched\n");
             if (tmp != AUX)
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of %9.4lf usec  GATE = %4x  ", \
                    ((float) RF_state.duration)/80.0,RF_state.gates);
               totalduration = totalduration + RF_state.duration;
             }
             else
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of 0.050 usec AUX  GATE = %4x  ", \
                   RF_state.gates);
               totalduration = totalduration + 4;
             }
	/*
             cumduration += (unsigned long long) totalduration;
             fidduration += (unsigned long long) totalduration;
             printf("cumduration: %llu, fidduration: %llu, duration: %lu\n",cumduration,fidduration,totalduration);
	*/


   	     if (noprt != 1)
                printf("AMP = %6.1lf  PHASE = %7.4lf\n",\
                    ((RF_state.amp*RF_state.amps)>>16)/6.5535+0.05,\
                    ((RF_state.phase+RF_state.phasec)&0xffff)*0.005493);
          }
          else { 
   	     if (noprt != 1)
               printf("\n");
           }
}

long getDecodedDuration()
{
   return(totalduration); 
} 

int clrCumTime()
{
   cumduration = 0LL;
}

int clrFidTime()
{
   fidduration = 0LL;
}

int clrBufTime()
{
   totalduration = 0L;
}

int prtFifoTime(char *idstr)
{
   if (idstr != NULL)
      printf("----->>  '%s'\n",idstr);
   printf("----->>  Durations: Cum: %llu ticks, %18.4f,   \n----->>             FID: %llu ticks, %18.4f,   Buffer: %lu ticks, %7.4f usec\n\n",
		cumduration, ((double) cumduration) / 80.0, fidduration, ((double) fidduration) / 80.0, totalduration, 
	        ((float) totalduration)/80.0);
}

int set_magnusfreq(double freq)
{
    unsigned char darray[8];
    int header;
    int cmd;
    int i;
    unsigned long long ftw1;
    unsigned long long fortyeightbits;

    fortyeightbits = 0x100000000000LL;
    ftw1 = (freq/(16*80000000.0)) * fortyeightbits;

    set_field(RF,fifo_output_select,0);

    header = 0x07;                               /* first 5 bits need to be size == 8  */
    cmd = 0x01;

    darray[0] = header & 0xff;
    darray[1] = cmd & 0xff;
                                                 /* these lines represent sending ftw1 LSB first */
    darray[2] = ftw1 & 0xff;
    darray[3] = (ftw1 >> 8) & 0xff;
    darray[4] = (ftw1 >> 16) & 0xff;
    darray[5] = (ftw1 >> 24) & 0xff;
    darray[6] = (ftw1 >> 32) & 0xff;
    darray[7] = (ftw1 >> 40) & 0xff;

   for (i=0;i<8;i=i+1)
    {
      set_field(RF,sw_aux,(0 << 8 ) | darray[i]);
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
      set_field(RF,sw_aux_strobe,1);
      set_field(RF,sw_aux_strobe,0);
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    }
    set_field(RF,sw_aux,(1 << 8 ) | 0xff);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    set_field(RF,sw_aux_strobe,1);
    set_field(RF,sw_aux_strobe,0);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    return(1);
} 


// #define FIFO_WRITE_PERFORMANCE_TEST
#ifdef FIFO_WRITE_PERFORMANCE_TEST
/*============================================================================*/

// 
// Performance tests for writing into the FPGA FIFO,  standard write address of FIFO  0x70000114
// Programmed I/O (PIO) writing performance tested with:
//
//   pioFifoTestTst #words, ticks   - e.g. pioFifoTestTst 8000,100       
//                                    8000 - since I believe the RF FIFO is about 8192 words deep
//
//   pioFifoTestTst2 #words, ticks, FifoWriteAddr  - e.g. pioFifoTestTst2 8000,100,0x70000114
//
// DMA performance measured with:
//
//   dmaTimeTst dmaChannel, #words, ticks   - e.g. dmaTimeTst 2,8000,100
//
//   dmaTimeTst2 dmaChannel, #words, ticks, FifoWriteAddr  - e.g. dmaTimeTst2 2,8000,100,0x70000114
//
// alternate chip select using Addr: 0x9000000 
// sysDcrEbcSet(0x2,0x900FC000) 
// sysDcrEbcSet(0x12,0x80000000)
// dmaTimeTst2 2,8000,100,0x70000114    - gives ~ 44 MiB/sec
// dmaTimeTst2 2,8000,100,0x90000114    - give ~ 103 MiB/sec (in memory to memory mode only)
// however memory to memory is not deviced-paced thus it will happy overflow the FIFO (8192 words)
// See Chapter 18.2.2 in 405 GPR Users Manual
// 
//   Greg Brissey   1/20/2010
//
pioFifoTimeTst2(int nwords, int ticks, unsigned int addr)
{
   int i,fifoword,len;
   long long starttime;
   long long endtime;
   long long difftime;
   long long corrtime;
   long long marksysclk();
   void cntrlFifoPIO(long *pCodes, int num);
   double timeSecs;
   long nfifowrds;
   volatile unsigned int* const pFifoWrite;

    printf("\n");
    printf("Given Write Addr: 0x%lx\n",addr);
    if (addr < 0x70000000)
       pFifoWrite = 0x70000114; // 0x70000000 + RF_FIFOInstructionWrite ( 0x114, rf.h )
    else
       pFifoWrite = addr; // You Better know what you are doing!!!
  
    printf("Using Fifo Write address: 0x%lx\n",pFifoWrite);

   /* determine the time require to call marksysclk() twice for time correction */
   starttime = marksysclk();
     difftime = marksysclk();
     difftime = marksysclk();
   endtime = marksysclk();
   corrtime = endtime - starttime;

   cntrlFifoReset();
   cntrlClearInstrCountTotal();
   len =  fifoEncodeDuration(1, ticks, &fifoword);
   printf("FifoWord: 0x%lx\n", fifoword);
   starttime = marksysclk();
   wvEvent(666,NULL,NULL);
   for (i=0; i < nwords; i++)
   {
      /* put directly into FIFO, (e.g. *pFifo->pFifoWrite = fifoword) */ 
      // e.g.  *0x7000140 = fifoword;
      // cntrlFifoPut(fifoword);      // src in fifoFuncs.c
      *pFifoWrite = fifoword;
   }
   wvEvent(667,NULL,NULL);
   endtime = marksysclk();
   nfifowrds = cntrlInstrCountTotal();

   if (nwords != nfifowrds)
   {
      printf("\n");
      printf("Error: FIFO only received %ld of %ld sent.\n", nwords, nfifowrds);
      printf("       Diff of %ld ( - :lost, + :gained )\n", nfifowrds - nwords);
      printf("\n");
      return 0;
   }

   difftime = endtime - starttime - corrtime;
   timeSecs = (double) difftime / 333333333.0;
   double bytesPerSec =  1.0 / (timeSecs / (nwords * 4));  /* nwords (32-bit) * 4 -> bytes */
   double xfrRateMHz = bytesPerSec / 1048576.0 /* MiB */;
   printf("\n");
   printf("Number of words received by FIFO: %ld\n", nfifowrds);
   printf("Start: %llu , End %llu, Diff %llu, Correction: %llu\n", starttime, endtime, difftime,corrtime);
   printf("Time to stuff %d words into FIFO:  %lf usec, or %lf MiB/sec\n", 
         nwords, timeSecs * 1000000.0 /*usec*/, xfrRateMHz);
   printf("\n");
   // cntrlFifoStart();
}

pioFifoTimeTst(int nwords, int ticks)
{
    pioFifoTimeTst2(nwords, ticks, 0);   // 0 - used default addr  0x70000114
}

static long *srcDataBufPtr = NULL;

// 
// build a buffer big enough for max DMA transfer size
//  used in dma performance tests below
//
buildfifobuf()
{
     int i;
     char *bufferZone;
      int size;

     bufferZone = (char*) malloc(1024); /* cache buffer zone */
     srcDataBufPtr = (long*) malloc(128000*4); /* max DMA transfer in single shot */
     bufferZone = (char*) malloc(1024); /* cache buffer zone */
     printf("bufaddr: 0x%08lx\n",srcDataBufPtr);
}

//
// DMA performanace Tests 
//

dmaTimeTst2(int dmachan, int nwords,int ticks, unsigned int addr)
{
    int i,len;
    long fifoword;
    long long starttime;
    long long endtime;
    long long difftime;
    long long corrtime;
    long long marksysclk();
    UINT32 txfrControl;
    txfr_t transferType;
    UINT32 FifoWriteAddr;
    long *pSrcDataBuf;
   double timeSecs;
    long nfifowrds;

    printf("\n");
    printf("Given Write Addr: 0x%lx\n",addr);
    if (addr < 0x70000000)
       FifoWriteAddr = 0x70000114; // 0x70000000 + RF_FIFOInstructionWrite ( 0x114, rf.h )
    else
       FifoWriteAddr = addr; // You Better know what you are doing!!!
  
    printf("Using Fifo Write Address: 0x%lx\n",FifoWriteAddr);

   /* determine the time require to call marksysclk() twice for time correction */
   starttime = marksysclk();
     difftime = marksysclk();
     difftime = marksysclk();
   endtime = marksysclk();
   corrtime = endtime - starttime;


    // if the buffers not present then build them
    if ( srcDataBufPtr == NULL )
       buildfifobuf();

    len =  fifoEncodeDuration(1, ticks, (int*) &fifoword);
    printf("FifoWord: 0x%lx\n", fifoword);

    printf("Fill buffer with encoded fifowords, StandBy.\n");
    pSrcDataBuf = srcDataBufPtr;
    for (i=0; i < nwords; i++)
    {
      *pSrcDataBuf++ = fifoword;
    }
    printf("\n");

    /* - Device-paced FIFO transfers must be chan. 2 or 3 - */
    /* ----- Because that's how the FIFO is wired up ------ */

    /*
     *  MEMORY_TO_PERIPHERAL (i.e. device-paced transfer to FIFO) 
     *  MEMORY_TO_FPGA.......(a non-device paced transfer through a register
     */

      if ( addr > 0x80000000 )
      {
         transferType = MEMORY_TO_MEMORY_DST_PACED;  // gives 44 MiB/sec
         // transferType = MEMORY_TO_MEMORY; // gives 103 MiB/sec but will overflow fifo
         // transferType = MEMORY_TO_FPGA;
      }
      else
      {
         transferType = MEMORY_TO_PERIPHERAL;   // gives ~ 44MiB/Sec
      }

     // NO_SG_LIST if it is a single-buffer transfer
     //
     //  txferSize   - No. of 32-bit words (not bytes) in the transfer
    
   cntrlFifoReset();
   cntrlClearInstrCountTotal();

 /* - Start up a single-buffer non-scatter-gather transfer - */
      dmaSetSrcReg(dmachan, (UINT32)(srcDataBufPtr));
      dmaSetDestReg(dmachan, (UINT32)(FifoWriteAddr));
      dmaSetCountReg(dmachan, (nwords & DMA_CNT_MASK));

      txfrControl = getDMA_ControlWord(transferType);
      // no interrupts, removed DMA_CR_CIE bit
      starttime = marksysclk();
      wvEvent(777,NULL,NULL);
      /* start actual NON SG DMA transfer */
      dmaSetConfigReg(dmachan, txfrControl | DMA_CR_CE );
//
//#                                    | DMA_CR_CE | DMA_CR_CIE);

      while(  dmaGetDeviceActiveStatus(dmachan) ); // running
      wvEvent(778,NULL,NULL);

   endtime = marksysclk();
   nfifowrds = cntrlInstrCountTotal();

   if (nwords != nfifowrds)
   {
      printf("\n");
      printf("Error: FIFO only received %ld of %ld sent.\n", nwords, nfifowrds);
      printf("       Diff of %ld ( - :lost, + :gained )\n", nfifowrds - nwords);
      printf("\n");
      return 0;
   }

   difftime = endtime - starttime - corrtime;
   timeSecs = (double) difftime / 333333333.0;    // (333333333 / 1000000)
   double bytesPerSec =  1.0 / (timeSecs / (nwords * 4));  /* nwords (32-bit) * 4 -> bytes */
   double xfrRateMHz = bytesPerSec / 1048576.0 /* MiB */;

   printf("\n");
   printf("Number of words received by FIFO: %ld\n", nfifowrds);
   printf("Start: %llu , End %llu, Diff %llu, Correction: %llu\n", starttime, endtime, difftime,corrtime);
   printf("Time to stuff %d words into FIFO:  %lf usec, or %lf MiB/sec\n", 
         nwords, timeSecs * 1000000.0 /*usec*/, xfrRateMHz);
   printf("\n");
}

dmaTimeTst(int dmachan, int nwords,int ticks)
{
    dmaTimeTst2(dmachan, nwords, ticks, 0);   // 0 - use default address  0x70000114
}

#endif  // FIFO_WRITE_PERFORMANCE_TEST
/*============================================================================*/

volatile unsigned int *fpga_amp = (volatile unsigned int *)(FPGA_BASE_ADR + RF_Amp);
volatile unsigned int *fpga_phase = (volatile unsigned int *)(FPGA_BASE_ADR + RF_Phase);
volatile unsigned int *fpga_fifo_amp = (volatile unsigned int *)(FPGA_BASE_ADR + RF_FIFOAmp);
volatile unsigned int *fpga_fifo_phase = (volatile unsigned int *)(FPGA_BASE_ADR + RF_FIFOPhase);

#define NUMAMPTBLS 64
#define AMPTBLSIZE 64
// defined in A32Interp
volatile extern unsigned int *fpga_atten_mapping;
volatile extern unsigned int *fpga_amp_table;
volatile extern unsigned int *fpga_phase_table;
volatile extern unsigned int *fpga_linearization_enable;
volatile extern unsigned int *fpga_tpwr_map_tbl;
volatile extern unsigned int *fpga_tpwr_scale_tbl;
// local variables
volatile unsigned int *fpga_linearized_amp = (volatile unsigned int *)(FPGA_BASE_ADR + RF_LinearizedAmpValue);
volatile unsigned int *fpga_linearized_phase = (volatile unsigned int *)(FPGA_BASE_ADR + RF_LinearizedPhaseValue);
volatile unsigned int *fpga_current_atten = (volatile unsigned int *)(FPGA_BASE_ADR + RF_CurrentAtten);
volatile unsigned int *fpga_current_atten_tbl = (volatile unsigned int *)(FPGA_BASE_ADR + RF_CurrentLinearizationTable);

void showLinEnable(int i) {
	printf("Linearization Enable (0x%X) = %d\n", fpga_linearization_enable,*fpga_linearization_enable);
}

void setLinEnable(int i) {
	*fpga_linearization_enable=i;
	printf("Linearization Enable = %d\n", *fpga_linearization_enable);
}

void showCurrentAtten() {
	int j;
	j = *fpga_current_atten;
	printf("Current tpwr code= 0x%x\n", j);
}

void showCurrentMapAttenTbl() {
	int j;
	j = *fpga_current_atten_tbl;
	printf("Current Map Table= %d\n", j);
}

void showMapTbl() {
	int i, j;
	for (i = 0; i < 256; i++) {
		j = fpga_atten_mapping[i];
		printf("Map %d = table:%d \n", i, j);
	}
}

void showTpwrMapTbl() {
	int i, j;
	for (i = 0; i < 256; i++) {
		j = fpga_tpwr_map_tbl[i];
		printf("Tpwr Table in:%d out:%d\n", i, j);
	}
}

void showTpwrScaleTbl() {
	int i, j;
	for (i = 0; i < 256; i++) {
		j = fpga_tpwr_scale_tbl[i];
		printf("Tpwr Scale %d scale:%g\n", i, ((double)j)/0x10000);
	}
}

void showMap(int i) {
	int j;
	j = fpga_atten_mapping[i];
	printf("Map %d = %d \n", i, j);
}

void showPhaseTbl(int i) {
	int j;
	for(j=0;j<AMPTBLSIZE;j++){
		unsigned int phase=fpga_phase_table[i*AMPTBLSIZE+j];
		double p=phase*360.0/0x10000;
		printf("Phase %d 0x%X (%g)\n",j, phase,p);
	}
}

void showAmpTbl(int i) {
	int j;
	for(j=0;j<AMPTBLSIZE;j++){
		unsigned int amp=fpga_amp_table[i*AMPTBLSIZE+j];
		double a=((double)amp)/0x10000;
		printf("Ampl %d 0x%X (%g)\n",j, amp,a);
	}
}

void showLinAmp(){
   unsigned int amp=*fpga_linearized_amp;
   double a=((double)amp)/0x10000;
   printf("Linearized Ampl(0x%8X) =  0x%X (%g)\n",fpga_linearized_amp,amp,a);
}

void showLinPhase()
{
   unsigned int phase=*fpga_linearized_phase;
   double p=phase*360.0/0x10000;
   printf("Linearized Phase (0x%8X) =  0x%X (%g)\n",fpga_linearized_phase,phase,p);
}

void showAmp()
{
   unsigned int amp=*fpga_amp;
   double a=((double)amp)/0x10000;
   printf("Ampl 0x%X (%g)\n",amp,a);
}
void showFifoAmp()
{
   unsigned int amp=*fpga_fifo_amp;
   double a=((double)amp)/0x10000;
   printf("Ampl 0x%X (%g)\n",amp,a);
}
void showPhase()
{
	unsigned int phase=*fpga_phase;
	double p=phase*360.0/0x10000;
	printf("Phase 0x%X (%g)\n",phase,p);
}
void showFifoPhase()
{
	unsigned int phase=*fpga_fifo_phase;
	double p=phase*360.0/0x10000;
	printf("Phase 0x%X (%g)\n",phase,p);
}


/* include the FPGA BASE ISR routines */
/* define the the controller type for proper conditional compile of ISR register defines */
#define RF_CNTLR
#include "fpgaBaseISR.c"
#include "A32Interp.c"
