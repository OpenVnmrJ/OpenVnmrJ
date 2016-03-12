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
#include <iv.h>
#include <intLib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "nvhardware.h"
#include "logMsgLib.h"
#if defined(PFG_LOCK_COMBO_CNTLR) && !defined(USE_PFG)
  #include "lpfg.h"
#else
  #include "pfg.h"
#endif
#include "pfg_fifo.h"

#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "errorcodes.h"

FIFO_REGS masterFifo_regs = { 
  get_pointer(PFG,FIFOInstructionWrite),
  get_pointer(PFG,FIFOControl),
  get_pointer(PFG,FIFOStatus),
  get_pointer(PFG,InterruptStatus),
  get_pointer(PFG,InterruptEnable),
  get_pointer(PFG,InterruptClear),
  get_pointer(PFG,ClearCumulativeDuration),
  get_pointer(PFG,CumulativeDurationLow),
  get_pointer(PFG,CumulativeDurationHigh),
  get_pointer(PFG,InstructionFIFOCount),
  get_pointer(PFG,DataFIFOCount),
#if defined(PFG_LOCK_COMBO_CNTLR) && !defined(USE_PFG) // deal with gratuitous register name changes
  get_pointer(PFG,InvalidInstruction),
#else
  get_pointer(PFG,InvalidOpcode),
#endif
  get_pointer(PFG,InstructionFifoCountTotal),
  get_pointer(PFG,ClearInstructionFifoCountTotal) 
};

/** read these from pRF_InterruptStatus / interrupt status..*/
#define  UNDERFLOW_FLAG    (0x00000002)
#define  OVR_FLAG     (0x00000001)

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */

extern MSG_Q_ID pMsgs2FifoIST;     /* msgQ for harderror resulting from failure assertion */

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */
extern int SafeXAmpVal;	        /* PFG or Gradient X Amp settings */ 
extern int SafeYAmpVal;		/* PFG or Gradient Y Amp settings */ 
extern int SafeZAmpVal;		/* PFG or Gradient Z Amp settings */ 

initFifo()
{
  unsigned int fifoIntMask;
  int instrfifosize;
  int newhiwatermark;

  fifoIntMask = get_mask(PFG,fifo_overflow_status) | get_mask(PFG,fifo_underflow_status) | 
		get_mask(PFG,fifo_finished_status) | get_mask(PFG,fifo_started_status) |
                get_mask(PFG,invalid_opcode_int_status) |
                get_mask(PFG,fail_int_status) | get_mask(PFG,warn_int_status);

  cntrlFifoInit(&masterFifo_regs, fifoIntMask, get_instrfifosize(PFG));

  /* +++++++ set the high and low device-paced FIFO DMA water marks +++++++  */
  /* size - headroom equal new high water mark for FIFO DMA device-paced */
  /* 8192 - 6 = 8186, so at 8186 words the DMA is told to stop, 
  /*    usually two more words get in */
  instrfifosize = getfifosize();
  newhiwatermark = instrfifosize - FIFO_HIWATER_MARK_HEADROOM;
  setfifohimark(newhiwatermark);
  /* setfifolowmark( ..... );  leave as default of 512 or half the instruction FIFO size */

  set2pfgfifo();   /* switch to FIFO controlled Outputs */

  initialBrdSpecificFailItr();

  /* set values in the SW FIFO Control registers to appropriate failsafe values */
  /* typically zero, when fail line is assert these values are presented to the output
  /* of the controller, (e.g. high speed gates)  Values that need to be serialized
  /* must strobed output by software however.	*/
  /*            Greg Brissey 9/16/04  */
  preSetAbortStates();

  /* cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, int dmaChannel, volatile unsigned int* const pFifoWrite ) */
  if ((cntrlFifoDmaChan = dmaGetChannel(2)) == -1)
  { printf("No dma channel available for FIFO \n"); return; }

    /* RF_NUM_FIFO_BUFS 30 RF_SIZE_FIFO_BUFS 1024 */
  pCntrlFifoBuf =  cntlrFifoBufCreate(PFG_NUM_FIFO_BUFS, PFG_SIZE_FIFO_BUFS, cntrlFifoDmaChan, 
                            (volatile unsigned int*) get_pointer(PFG,FIFOInstructionWrite));

}

/*
 * Rotorsync - the cntlr variant of rotor sync 
 *   place halt into FIFO so the controller can wait for the
 *   rotosync to complete.  
 *   the master assert the sync line when rotor sync is complete
 *   allowing the other controllers to continue
 *
 *    Author: Greg Brissey  4/05/2005
 */
int RotorSync(int count, int postdelay) 
{
   int instrwords[20];
   int num;
   int fifoEncodeSystemSync(int postdelay, int *instrwords);

   num = fifoEncodeSystemSync(postdelay, instrwords);
   cntrlFifoBufPut(pCntrlFifoBuf, (long *) instrwords, num );
   return(num);
}

/*
 * fifoEncodeSWItr(), return a encoded stream of FIFO instruction to
 * the selected Software FIFO interrupt to occur.
 *
 *    Author: Greg Brissey  8/11/04
 */
int fifoEncodeSWItr(int SWitr, int *instrwords)
{
   int i = 0;
   instrwords[i++] = encode_PFGSetGates(0,0xf00,0);  /* only effect SW bits */
   instrwords[i++] = encode_PFGSetDuration(0,320); /* duration of asserted interrupt bit, 4 us */
   switch(SWitr)
   {
      case 1: instrwords[i++] = encode_PFGSetGates(1,0xf00,0x100); break;
      case 2: instrwords[i++] = encode_PFGSetGates(1,0xf00,0x200); break;
      case 3: instrwords[i++] = encode_PFGSetGates(1,0xf00,0x400); break;
      case 4: instrwords[i++] = encode_PFGSetGates(1,0xf00,0x800); break;
   }
   instrwords[i++] = encode_PFGSetGates(1,0xf00,0x000);
   return(i);
}

/* 
 *  Time Duration of Zero will pause FIFO, followed by another post delay 
 *  given by postdelay argument, approx. 100nsec
 *
 *      Author: Greg brissey 8/12/2004
 */
int fifoEncodeSystemSync(int postdelay,int *instrwords)
{
   int i = 0;
   if ((postdelay < 320) && (postdelay > 0))
   {
      DPRINT1(-2,"1 postDelay UP'd to 320!! was %d\n",postdelay);
      postdelay = 320;
   }
   instrwords[i++] = encode_PFGSetDuration(0,0); /* duration of Zero will pause FIFO waiting for Sync bit */
   instrwords[i++] = encode_PFGSetGates(1,FF_GATE_SW4,0);
   instrwords[i++] = encode_PFGSetDuration(0,postdelay); /* duration of post delay */
   instrwords[i++] = encode_PFGSetGates(1,FF_GATE_SW4,0);
   return(i);
}

int fifoEncodeDuration(int write, int duration,int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_PFGSetDuration(write,duration);
    return i;
}

int fifoEncodeGates(int write, int mask,int gates, int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_PFGSetGates(write,mask,gates);
    return i;
}

set2pfgfifo()
{
   set_register(PFG ,FIFOOutputSelect,1);
}

/* output controlled via SW registers (0) or FIFO output (1) */
void setFifoOutputSelect(int SwOrFifo)
{
   set_register(PFG,FIFOOutputSelect,SwOrFifo);
   return;
}

int getfifosize()
{
  return( get_instrfifosize(PFG) );
}

prtfifohilowmarks()
{
   int himark,lowmark;

   himark = get_field(PFG,instruction_fifo_high_threshold);
   lowmark = get_field(PFG,instruction_fifo_low_threshold);
   printf("FIFO DMA device-paced hi watermark: %d, low water mark: %d\n",himark,lowmark);
}

int getfifohimark()
{
   int himark;
   himark = get_field(PFG,instruction_fifo_high_threshold);
   return himark;
}

int setfifohimark(int hi)
{
   set_field(PFG,instruction_fifo_high_threshold,hi);
   return 0;
}

int getfifolowmark()
{
   int lowmark;
   lowmark = get_field(PFG,instruction_fifo_low_threshold);
   return lowmark;
}

int setfifolowmark(int low)
{
   set_field(PFG,instruction_fifo_low_threshold,low);
   return 0;
}


preSetAbortStates()
{
   SafeGateVal = SafeGateVal = SafeXAmpVal = SafeZAmpVal = 0;		
   set_register(PFG,SoftwareGates,0);
   set_register(PFG,SoftwareUser,0);
   set_register(PFG,SoftwareXAmp,0);
   set_register(PFG,SoftwareYAmp,0);
   set_register(PFG,SoftwareZAmp,0);
}

/*
 * invoke by the fail interrupt to assure update register so that FPGA can serialize out the
 * safe states
 *			Author:  Greg Brissey 9/21/05
 */
void serializeSafeVals()
{
   set_field(PFG,sw_xamp,SafeXAmpVal);
   set_field(PFG,sw_yamp,SafeYAmpVal);
   set_field(PFG,sw_zamp,SafeZAmpVal);

   /* this is required to allow the FPGA to auto serialize the values out 
    * when the fail-line is asserted.
    *       8/19/05
    */
   set_field(PFG,sw_xamp_updated,1);
   set_field(PFG,sw_yamp_updated,1);
   set_field(PFG,sw_zamp_updated,1);
   while( get_field(PFG,pfg_amp_busy) & 0x7 );

   /* resetPfgAmps();    not needed or wanted */
//   logMsg("serializeSafeVals Complete.\n");
}


resetFifoHoldRegs()
{
   int duration, xamp, yamp, zamp, xamp_scale, yamp_scale, zamp_scale, user;
   int clear_zero, count_zero;
   int clear_one, count_one;

   duration = 0;
   xamp = yamp = zamp = 0;
   xamp_scale = yamp_scale = zamp_scale = user = 0;
   clear_zero = count_zero = 0;
   clear_one = count_one = 1;

   /* clear the FIFO gate holding register */
   cntrlFifoPut(encode_PFGSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_PFGSetDuration(0,duration));

   cntrlFifoPut(encode_PFGSetXAmp(0,clear_zero,count_one,xamp));
   cntrlFifoPut(encode_PFGSetXAmp(0,clear_one,count_zero,xamp));
   cntrlFifoPut(encode_PFGSetXAmpScale(0,clear_zero,count_one,xamp_scale));
   cntrlFifoPut(encode_PFGSetXAmpScale(0,clear_one,count_zero,xamp_scale));

   cntrlFifoPut(encode_PFGSetYAmp(0,clear_zero,count_one,yamp));
   cntrlFifoPut(encode_PFGSetYAmp(0,clear_one,count_zero,yamp));
   cntrlFifoPut(encode_PFGSetYAmpScale(0,clear_zero,count_one,yamp_scale));
   cntrlFifoPut(encode_PFGSetYAmpScale(0,clear_one,count_zero,yamp_scale));

   cntrlFifoPut(encode_PFGSetZAmp(0,clear_zero,count_one,zamp));
   cntrlFifoPut(encode_PFGSetZAmp(0,clear_one,count_zero,zamp));
   cntrlFifoPut(encode_PFGSetZAmpScale(0,clear_zero,count_one,zamp_scale));
   cntrlFifoPut(encode_PFGSetZAmpScale(0,clear_one,count_zero,zamp_scale));

   cntrlFifoPut(encode_PFGSetUser(0,user));

   return 0;
}

prtholdregs()
{
   printf("PFG_Duration: %ld\n", get_register(PFG,Duration));
   printf("PFG_XAmp: %ld\n", get_register(PFG,XAmp));
   printf("PFG_XAmpIncrement: %ld\n", get_register(PFG,XAmpIncrement));
   printf("PFG_XAmpCount: %ld\n", get_register(PFG,XAmpCount));
   printf("PFG_XAmpClear: %ld\n", get_register(PFG,XAmpClear));
   printf("PFG_YAmp: %ld\n", get_register(PFG,YAmp));
   printf("PFG_YAmpIncrement: %ld\n", get_register(PFG,YAmpIncrement));
   printf("PFG_YAmpCount: %ld\n", get_register(PFG,YAmpCount));
   printf("PFG_YAmpClear: %ld\n", get_register(PFG,YAmpClear));
   printf("PFG_ZAmp: %ld\n", get_register(PFG,ZAmp));
   printf("PFG_ZAmpIncrement: %ld\n", get_register(PFG,ZAmpIncrement));
   printf("PFG_ZAmpCount: %ld\n", get_register(PFG,ZAmpCount));
   printf("PFG_ZAmpClear: %ld\n", get_register(PFG,ZAmpClear));
   printf("PFG_XAmpScale: %ld\n", get_register(PFG,XAmpScale));
   printf("PFG_XAmpScaleIncrement: %ld\n", get_register(PFG,XAmpScaleIncrement));
   printf("PFG_XAmpScaleCount: %ld\n", get_register(PFG,XAmpScaleCount));
   printf("PFG_XAmpScaleClear: %ld\n", get_register(PFG,XAmpScaleClear));
   printf("PFG_YAmpScale: %ld\n", get_register(PFG,YAmpScale));
   printf("PFG_YAmpScaleIncrement: %ld\n", get_register(PFG,YAmpScaleIncrement));
   printf("PFG_YAmpScaleCount: %ld\n", get_register(PFG,YAmpScaleCount));
   printf("PFG_YAmpScaleClear: %ld\n", get_register(PFG,YAmpScaleClear));
   printf("PFG_ZAmpScale: %ld\n", get_register(PFG,ZAmpScale));
   printf("PFG_ZAmpScaleIncrement: %ld\n", get_register(PFG,ZAmpScaleIncrement));
   printf("PFG_ZAmpScaleCount: %ld\n", get_register(PFG,ZAmpScaleCount));
   return 0;
}


/*====================================================================================*/
/*
 *     Board Specific ISR for harderrors of the system 
 *
 *  Author Greg Brissey 9/23/04
 *
 */
static void Brd_Specific_Fail_ISR(int int_status, int errorcode) 
{
   msgQSend(pMsgs2FifoIST,(char*) &errorcode, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);
   if (DebugLevel > -1)
      logMsg("Brd_Specific_Fail_ISR: status: 0x%lx, errorcode: %d\n",
              int_status, errorcode); 
   return;
}

/*====================================================================================*/
/*
 * enable all board specific failures interrupts
 */
initialBrdSpecificFailItr()
{
  unsigned int failureMask,intMask;

  failureMask =  set_field_value(PFG,fifo_amp_duration_too_short_enable,0x7);
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",failureMask); */

  intMask =  set_field_value(PFG,fifo_amp_duration_too_short_enable,0x1); 
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask); */
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_XAMP_OVRRUN), intMask );

  intMask =  set_field_value(PFG,fifo_amp_duration_too_short_enable,0x2); 
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask); */
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_YAMP_OVRRUN), intMask );

  intMask =  set_field_value(PFG,fifo_amp_duration_too_short_enable,0x4); 
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask); */
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_ZAMP_OVRRUN), intMask );

  cntrlFifoIntrpSetMask(failureMask);   /* enable interrupts */
}

deInstallPFGFailIsr()
{
  fpgaIntRemove( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_XAMP_OVRRUN));
  fpgaIntRemove( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_YAMP_OVRRUN));
  fpgaIntRemove( Brd_Specific_Fail_ISR, (HDWAREERROR + PFG_ZAMP_OVRRUN));
}
