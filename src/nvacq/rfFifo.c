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
#include "errorcodes.h"
#include "rf.h"
#include "rf_fifo.h"
#include "nvhardware.h"  // must be *AFTER* rf.h
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "logMsgLib.h"

FIFO_REGS masterFifo_regs = { 
   				get_pointer(RF,FIFOInstructionWrite),
				get_pointer(RF,FIFOControl),
				get_pointer(RF,FIFOStatus),
        			get_pointer(RF,InterruptStatus),
        			get_pointer(RF,InterruptEnable),
        			get_pointer(RF,InterruptClear),
        			get_pointer(RF,ClearCumulativeDuration),
        			get_pointer(RF,CumulativeDurationLow),
        			get_pointer(RF,CumulativeDurationHigh),
        			get_pointer(RF,InstructionFIFOCount),
        			get_pointer(RF,DataFIFOCount),
        			get_pointer(RF,InvalidInstruction),
        			get_pointer(RF,InstructionFifoCountTotal),
        			get_pointer(RF,ClearInstructionFifoCountTotal) 
			    };



/** read these from pRF_InterruptStatus / interrupt status..*/
#define  UNDERFLOW_FLAG    (0x00000002)
#define  OVR_FLAG     (0x00000001)

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */

extern MSG_Q_ID pMsgs2FifoIST;     /* msgQ for harderror resulting from failure assertion */

static void Fifo_SW_ISR(int int_status, int val) ;

initFifo()
{
  unsigned int fifoIntMask;
  int instrfifosize;
  int newhiwatermark;

  fifoIntMask = get_mask(RF,fifo_overflow_status) | get_mask(RF,fifo_underflow_status) | 
		get_mask(RF,fifo_finished_status) | get_mask(RF,fifo_started_status) |
		/* get_mask(RF,data_fifo_almost_empty_status)  | get_mask(RF,instr_fifo_almost_full_status) | */
                get_mask(RF,fail_int_status) | get_mask(RF,warn_int_status) |
                get_mask(RF,invalid_opcode_int_status);

  cntrlFifoInit(&masterFifo_regs, fifoIntMask, get_instrfifosize(RF));

  /* +++++++ set the high and low device-paced FIFO DMA water marks +++++++  */
  /* size - headroom equal new high water mark for FIFO DMA device-paced */
  /* 8192 - 6 = 8186, so at 8186 words the DMA is told to stop, 
  /*    usually two more words get in */
  instrfifosize = getfifosize();
  newhiwatermark = instrfifosize - FIFO_HIWATER_MARK_HEADROOM;   /* 6 */
  setfifohimark(newhiwatermark);

  /* looks like either one give the same result, so I'm using the 1024 down from the top */
  /*  GMB  2/10/05 */
  setfifolowmark((instrfifosize - 1024));    /* was 4096 */
  /* setfifolowmark((instrfifosize - FIFO_HIWATER_MARK_HEADROOM - 4));    /* was 4096 */

  set2rffifo();

  resetAuxBus();    /* Bug# 118 correction */

  initialBrdSpecificFailItr();

  /* set values in the SW FIFO Control registers to appropriate failsafe values */
  /* typically zero, when fail line is assert these values are presented to the output
  /* of the controller, (e.g. high speed gates)  Values that need to be serialized
  /* must strobed output by software however.	*/
  /*            Greg Brissey 9/16/04  */
  preSetAbortStates();


  /* cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, 
            int dmaChannel, volatile unsigned int* const pFifoWrite ) */
  if ((cntrlFifoDmaChan = dmaGetChannel(2)) == -1)
  { printf("No dma channel available for FIFO \n"); return; }

    /* RF_NUM_FIFO_BUFS 30 RF_SIZE_FIFO_BUFS 1024 */
  pCntrlFifoBuf =  cntlrFifoBufCreate(RF_NUM_FIFO_BUFS, RF_SIZE_FIFO_BUFS, cntrlFifoDmaChan, 
                            (volatile unsigned int*) get_pointer(RF,FIFOInstructionWrite));

  /* initSWItr(); */

}

int getfifosize()
{
  return( get_instrfifosize(RF) );
}

prtfifohilowmarks()
{
   int himark,lowmark;

   himark = get_field(RF,instruction_fifo_high_threshold);
   lowmark = get_field(RF,instruction_fifo_low_threshold);
   printf("FIFO DMA device-paced hi watermark: %d, low water mark: %d\n",himark,lowmark);
}

int getfifohimark()
{
   int himark;
   himark = get_field(RF,instruction_fifo_high_threshold);
   return( himark );
}

int setfifohimark(int hi)
{
   set_field(RF,instruction_fifo_high_threshold,hi);
   return(0);
}

int getfifolowmark()
{
   int lowmark;
   lowmark = get_field(RF,instruction_fifo_low_threshold);
   return( lowmark );
}

int setfifolowmark(int low)
{
   set_field(RF,instruction_fifo_low_threshold,low);
   return(0);
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
   instrwords[i++] = encode_RFSetGates(0,0xf00,0);  /* only effect SW bits */
   /* gradient grid adjust 8 -> 320 */
   instrwords[i++] = encode_RFSetDuration(0,320); /* duration of asserted interrupt bit, 4 us */
   switch(SWitr)
   {
 
      case 1: instrwords[i++] = encode_RFSetGates(1,0xf00,0x100); break;
      case 2: instrwords[i++] = encode_RFSetGates(1,0xf00,0x200); break;
      case 3: instrwords[i++] = encode_RFSetGates(1,0xf00,0x400); break;
      case 4: instrwords[i++] = encode_RFSetGates(1,0xf00,0x800); break;
   }
   instrwords[i++] = encode_RFSetGates(1,0xf00,0x000);
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
   // with icat RF this postdelay becomes small than 4 usec
   // so just test against minmum delay 50 nsec 4 ticks
   // if ((postdelay < 320) && (postdelay > 0))
   if (postdelay < 4)
   {
      DPRINT1(-9,"1 postDelay UP'd to 4 ticks!! was %d\n",postdelay);
      postdelay = 4;
   }
   instrwords[i++] = encode_RFSetDuration(0,0); /* duration of Zero will pause FIFO waiting for Sync bit */
   instrwords[i++] = encode_RFSetGates(1,FF_GATE_SW4,0);
   instrwords[i++] = encode_RFSetDuration(0,postdelay); /* duration of post delay */
   instrwords[i++] = encode_RFSetGates(1,FF_GATE_SW4,0);
   return(i);
}

int fifoEncodeDuration(int write, int duration,int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_RFSetDuration(write,duration);
    return i;
}

int fifoEncodeGates(int write, int mask,int gates, int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_RFSetGates(write,mask,gates);
    return i;
}

set2rffifo()
{
   set_register(RF,FIFOOutputSelect,1);
}

/* output controlled via SW registers (0) or FIFO output (1) */
void setFifoOutputSelect(int SwOrFifo)
{
   set_register(RF,FIFOOutputSelect,SwOrFifo);
   return;
}


resetAuxBus()
{
   set_register(RF,FIFOOutputSelect,0);
     set_field(RF,sw_aux_reset,0);
     set_field(RF,sw_aux_reset,1);
     set_field(RF,sw_aux_reset,0);
   set_register(RF,FIFOOutputSelect,1);
}

preSetAbortStates()
{
   SafeGateVal = 0;		
   set_register(RF,SoftwarePhase,0);
   set_register(RF,SoftwareAmp,0);
   set_register(RF,SoftwareGates,0);
   set_register(RF,SoftwareUser,0);
   set_register(RF,SoftwareAUX,0);
}

/* called fail interrupt isr & phandler */
void serializeSafeVals()
{
//   logMsg("serializeSafeVals complete.\n");
}

resetFifoHoldRegs()
{
    int user, address,data, gateCheck;

    user = address = data = 0;
   gateCheck = SafeGateVal & 0x0a0;  // allow RF_CW and RF_UNBLANK
   DPRINT2(1,"resetFifoHoldRegs = %x  (%x)\n",gateCheck,SafeGateVal); 
   /* clear the FIFO gate holding register except for CW and UNBLANK */
   cntrlFifoPut(encode_RFSetGates(0,0xfff,gateCheck));
   cntrlFifoPut(encode_RFSetDuration(0,0));
   cntrlFifoPut(encode_RFSetPhase(0,0,1,0));
   cntrlFifoPut(encode_RFSetPhase(0,1,0,0));
   cntrlFifoPut(encode_RFSetPhaseC(0,0,1,0));
   cntrlFifoPut(encode_RFSetPhaseC(0,1,0,0));
   cntrlFifoPut(encode_RFSetAmp(0,0,1,0));
   cntrlFifoPut(encode_RFSetAmp(0,1,0,0));
   cntrlFifoPut(encode_RFSetAmpScale(0,0,1,0));
   cntrlFifoPut(encode_RFSetAmpScale(0,1,0,0));
   cntrlFifoPut(encode_RFSetUser(0,user));
   cntrlFifoPut(encode_RFSetAux(0,address,data));
   return 0;
}

prtholdregs()
{

   printf("RF_Gates: 0x%lx\n", get_register(RF,Gates));
   printf("RF_Duration: %ld\n", get_register(RF,Duration));
   printf("RF_Phase: 0x%lx\n", get_register(RF,Phase));
   printf("RF_PhaseIncrement: %ld\n", get_register(RF,PhaseIncrement));
   printf("RF_PhaseCount: %ld\n", get_register(RF,PhaseCount));
   printf("RF_PhaseC: 0x%lx\n", get_register(RF,PhaseC));
   printf("RF_PhaseCIncrement: %ld\n", get_register(RF,PhaseCIncrement));
   printf("RF_PhaseCCount: %ld\n", get_register(RF,PhaseCCount));
   printf("RF_Amp: 0x%lx\n", get_register(RF,Amp));
   printf("RF_AmpIncrement: %ld\n", get_register(RF,AmpIncrement));
   printf("RF_AmpCount: %ld\n", get_register(RF,AmpCount));
   printf("RF_AmpScale: 0x%lx\n", get_register(RF,AmpScale));
   printf("RF_AmpScaleIncrement: %ld\n", get_register(RF,AmpScaleIncrement));
   printf("RF_AmpScaleCount: %ld\n", get_register(RF,AmpScaleCount));
   return 0;
}

#ifdef  RF_InstructionFifoCountTotal
long getFifoInstrCount()
{
       /* int *cntptr = get_pointer(RF,InstructionFifoCountTotal) */
       long count;
       count = get_field(RF,instr_fifo_count_total);
       return(count);
}
clrFifoInstrCount()
{
     set_field(RF,clear_instr_fifo_count_total,1);
     set_field(RF,clear_instr_fifo_count_total,0);
}
#endif

/*====================================================================================*/
/*
 *     Board Specific ISR for harderrors of the system 
 *
 *  Author Greg Brissey 9/23/04
 *
 */
#ifdef NOW_MOVE_TO_FIFOFUNCS
#ifdef RF_InvalidOpcode
static void Brd_Specific_Fail_ISR(int int_status, int errorcode) 
{
   msgQSend(pMsgs2FifoIST,(char*) &errorcode, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);
   if ( (int_status & get_mask(RF,invalid_opcode_int_status)) != 0)
   {
      /* if (DebugLevel > -1) */
        logMsg("RF Brd_Specific_Fail_ISR: status: 0x%lx, BadOpcode: %d\n",
             int_status,get_field(RF,invalid_opcode));
   }
   return;
}
#endif
#endif


/*====================================================================================*/
/*
 * enable all board specific failures interrupts
 */
initialBrdSpecificFailItr()
{
#ifdef NOW_MOVE_TO_FIFOFUNCS
#ifdef RF_InvalidOpcode
  unsigned int failureMask,intMask;
  failureMask =  get_mask(RF,invalid_opcode_int_status);

  intMask =  get_mask(RF,invalid_opcode_int_status); 
  DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask);
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + 99), intMask );

  cntrlFifoIntrpSetMask(failureMask);   /* enable interrupts */
#endif
#endif
}

/*============================================================================*/


#ifdef  NOT_USED_IN_SHANDLER
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

static void Fifo_SW_ISR(int int_status, int val) 
{
  int RFI_status;
  char *buffer;

  buffer = NULL;
  switch(val)
  {
      case 1:
              buffer = " SW IRQ 1";
	      break;
      case 2:
              buffer = " SW IRQ 2";
	      break;
      case 3:
              buffer = " SW IRQ 3";
	      break;
      case 4:
              buffer = " SW IRQ 4";
		/* reportReady4Sync to Master */
	      break;
      default:
              buffer = " SW IRQ ?";
	      break;
  }
  if (buffer != NULL)
       diagPrint(NULL,"Fifo_SW_ISR: %s  \n",buffer);
}

#endif
