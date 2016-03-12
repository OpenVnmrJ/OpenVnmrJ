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
#include "ddr.h"
#include "ddr_fifo.h"
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#undef TIME
#include "DDR_Common.h"
#include "ACode32.h"
#include "DDR_Acq.h"

FIFO_REGS ddrFifo_regs = { 
    get_pointer(DDR,FIFOInstructionWrite),
    get_pointer(DDR,FIFOControl),
#ifdef DDR_FIFOStatus
    get_pointer(DDR,FIFOStatus), 
#else
    NULL,
#endif
    get_pointer(DDR,InterruptStatus_0),
    get_pointer(DDR,InterruptEnable_0),
    get_pointer(DDR,InterruptClear_0),
    get_pointer(DDR,ClearCumulativeDuration),
    get_pointer(DDR,CumulativeDurationLow),
    get_pointer(DDR,CumulativeDurationHigh),
    get_pointer(DDR,InstructionFIFOCount),
    get_pointer(DDR,DataFIFOCount),
    get_pointer(DDR,InvalidOpcode),
    get_pointer(DDR,InstructionFifoCountTotal),
    get_pointer(DDR,ClearInstructionFifoCountTotal) 
};

/** read these from pRF_InterruptStatus / interrupt status..*/
#define  UNDERFLOW_FLAG    (0x00000002)
#define  OVR_FLAG          (0x00000001)

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan; /* device paced DMA channel for PS control FIFO */

extern int SafeGateVal;     /* Safe Gate values for Abort or Exp End, (in globals.c) */

void set2ddrfifo()
{
    set_register(DDR,FIFOOutputSelect,1);
}

/* output controlled via SW registers (0) or FIFO output (1) */
void setFifoOutputSelect(int SwOrFifo)
{
   set_register(DDR,FIFOOutputSelect,SwOrFifo);
   return;
}

int getfifosize()
{
  return( get_instrfifosize(DDR) );
}

prtfifohilowmarks()
{
   int himark,lowmark;

   himark = get_field(DDR,instruction_fifo_high_threshold);
   lowmark = get_field(DDR,instruction_fifo_low_threshold);
   printf("FIFO DMA device-paced hi watermark: %d, low water mark: %d\n",himark,lowmark);
}

int getfifohimark()
{
   int himark;
   himark = get_field(DDR,instruction_fifo_high_threshold);
   return( himark );
}

int setfifohimark(int hi)
{
   set_field(DDR,instruction_fifo_high_threshold,hi);
   return(0);
}

int getfifolowmark()
{
   int lowmark;
   lowmark = get_field(DDR,instruction_fifo_low_threshold);
   return( lowmark );
}

int setfifolowmark(int low)
{
   set_field(DDR,instruction_fifo_low_threshold,low);
   return(0);
}



void initFifo()
{
    unsigned int fifoIntMask;
    int instrfifosize;
    int newhiwatermark;

    fifoIntMask = get_mask(DDR, fifo_overflow_status_0)
        | get_mask(DDR, fifo_underflow_status_0)
        | get_mask(DDR, fifo_finished_status_0)
        | get_mask(DDR, fifo_started_status_0)
        | get_mask(DDR,fail_int_status_0) 
        | get_mask(DDR,invalid_opcode_int_status_0)
        | get_mask(DDR,warn_int_status_0);

    cntrlFifoInit(&ddrFifo_regs, fifoIntMask, get_instrfifosize(DDR));

    /* +++++++ set the high and low device-paced FIFO DMA water marks +++++++  */
    /* size - headroom equal new high water mark for FIFO DMA device-paced */
    /* 8192 - 6 = 8186, so at 8186 words the DMA is told to stop, 
    /*    usually two more words get in */
    instrfifosize = getfifosize();
    newhiwatermark = instrfifosize - FIFO_HIWATER_MARK_HEADROOM;   /* 6 */
    setfifohimark(newhiwatermark);
    /* setfifolowmark( ..... );  leave as default of 512 or half the instruction FIFO size */

    set2ddrfifo();


    /* set values in the SW FIFO Control registers to appropriate failsafe values */
    /* typically zero, when fail line is assert these values are presented to the output */
    /* of the controller, (e.g. high speed gates)  Values that need to be serialized */
    /* must strobed output by software however.	*/
    /*            Greg Brissey 9/16/04  */
    preSetAbortStates();


    if ((cntrlFifoDmaChan = dmaGetChannel(2)) == -1) {
        printf("No dma channel available for FIFO \n");
        return;
    }
    pCntrlFifoBuf = cntlrFifoBufCreate(
                       DDR_NUM_FIFO_BUFS,
                       DDR_SIZE_FIFO_BUFS,
                       cntrlFifoDmaChan,
        (volatile unsigned int *) get_pointer(DDR, FIFOInstructionWrite)
        );
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
 * the sleceted Software FIFO interrupt to occur.
 *
 *    Author: Greg Brissey  8/11/04
 */
int fifoEncodeSWItr(int SWitr, int *instrwords)
{
   int i = 0;
   instrwords[i++] = encode_DDRSetGates(0,0xf00,0);  /* only effect SW bits */
   instrwords[i++] = encode_DDRSetDuration(0,320); /* duration of asserted interrupt bit, 4 us */
   switch(SWitr)
   {
      case 1: instrwords[i++] = encode_DDRSetGates(1,0xf00,0x100); break;
      case 2: instrwords[i++] = encode_DDRSetGates(1,0xf00,0x200); break;
      case 3: instrwords[i++] = encode_DDRSetGates(1,0xf00,0x400); break;
      case 4: instrwords[i++] = encode_DDRSetGates(1,0xf00,0x800); break;
   }
   instrwords[i++] = encode_DDRSetGates(1,0xf00,0x000);
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
   /* duration of Zero will pause FIFO waiting for Sync bit */
   instrwords[i++] = encode_DDRSetDuration(0,0);
   instrwords[i++] = encode_DDRSetGates(1,FF_GATE_SW4,0);
   instrwords[i++] = encode_DDRSetDuration(0,postdelay); /* duration of post delay */
   instrwords[i++] = encode_DDRSetGates(1,FF_GATE_SW4,0);
   return(i);
}

int fifoEncodeDuration(int write, int duration,int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_DDRSetDuration(write,duration);
    return i;
}

int fifoEncodeGates(int write, int mask,int gates, int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_DDRSetGates(write,mask,gates);
    return i;
}

preSetAbortStates()
{
   SafeGateVal = 0;
   set_register(DDR,SoftwareGates,0);
   set_register(DDR,SoftwareGain,0);
   set_register(DDR,SoftwareAUX,0);
}

/* called fail interrupt isr & phandler */
void serializeSafeVals()
{
//   logMsg("serializeSafeVals complete.\n");
}

#ifdef XXXX
/* call by fail line interrupt and ddrphandler */
setAbortSerializedStates()
{
    goToSafeState();
/*
 *  set_field(DDR,sw_aux_reset,0);
 *  set_field(DDR,sw_aux_reset,1);
 *  set_register(DDR,SoftwareAUX,0);
 *  set_field(DDR,sw_aux_strobe,0);
 *  set_field(DDR,sw_aux_strobe,1);
*/
}
#endif

resetFifoHoldRegs()
{
   /* clear the FIFO gate holding register */
   cntrlFifoPut(encode_DDRSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_DDRSetGain(0,0));
   cntrlFifoPut(encode_DDRSetDuration(0,0));
   return 0;
}

/*   use to test SEND_ZERO_FID sync interrupt */
#ifdef XXXXXXXX
int tstSWI(int mode) 
{
   unsigned long instrwords[12];
   long long duration;
   int i = 0;
   int cnt;

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();


   instrwords[i++] = encode_DDRSetGates(0,0xf00,0);  /* only effect SW bits */
   instrwords[i++] = encode_DDRSetDuration(0,320); /* duration of asserted interrupt bit, 4 us */
   // instrwords[i++] = encode_DDRSetDuration(0,80000);      

   instrwords[i++] = encode_DDRSetGates(1,0xf00,0x800);

   instrwords[i++] = encode_DDRSetGates(1,0xf00,0x000);
   instrwords[i++] = encode_DDRSetDuration(1,0);
   cnt = i;
   for(i=0; i < cnt; i++)
         printf("instr[%d] = 0x%lx\n",i,instrwords[i]);
   switch(mode)
   {
    case 0:
       cntrlFifoBufPut(pCntrlFifoBuf, instrwords, cnt ); 
       cntrlFifoBufForceRdy(pCntrlFifoBuf); 
       break;
    case 1:
      cntrlFifoBufCopy(pCntrlFifoBuf, instrwords, cnt, 1, 0, 0 );
      break;
   case 2:
   default:
      for(i=0; i < cnt; i++)
         cntrlFifoPut(instrwords[i]);
      break;
   }
   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */
   cntrlFifoStart();
   // cntrlFifoWait4StopItrp();
   // cntrlFifoCumulativeDurationGet(&duration);
   // DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   // DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 80000L );
    return OK;
}
#endif  // XXXXXXXXX
/*
initSWItr()
{
int mask;
  mask = get_mask(RF,sw_int_status);
  printf("mask: 0x%lx\n",mask);
  fpgaIntConnect( Fifo_SW_ISR, 1, FF_SW1_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 2, FF_SW2_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 3, FF_SW3_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 4, FF_SW4_IRQ);
  cntrlFifoIntrpSetMask(mask);
}
*/
