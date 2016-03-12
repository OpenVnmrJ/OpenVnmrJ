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
#include "master.h"
#include "master_fifo.h"
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "errorcodes.h"

FIFO_REGS masterFifo_regs = { 
   				get_pointer(MASTER,FIFOInstructionWrite),
				get_pointer(MASTER,FIFOControl),
				get_pointer(MASTER,FIFOStatus),
        			get_pointer(MASTER,InterruptStatus),
        			get_pointer(MASTER,InterruptEnable),
        			get_pointer(MASTER,InterruptClear),
        			get_pointer(MASTER,ClearCumulativeDuration),
        			get_pointer(MASTER,CumulativeDurationLow),
        			get_pointer(MASTER,CumulativeDurationHigh),
        			get_pointer(MASTER,InstructionFIFOCount),
        			get_pointer(MASTER,DataFIFOCount),
        			get_pointer(MASTER,InvalidOpcode),
        			get_pointer(MASTER,InstructionFifoCountTotal),
        			get_pointer(MASTER,ClearInstructionFifoCountTotal) 
			    };



#define SPI_CHAN_SELECT(chan)  ((chan & 0x1) << 28 )		/* 1 bit channel select */
#define SPI_CHIP_SELECT(chip)  ((chip & 0xf) << 24 )		/* 4 bit chip select */
#define SPI_DATA(data)         ((data & 0xfffff))		/* 20 bit data */

#define TIMER_MASK (0x03ffffff)
#define STOP_WORD  (LFIFO | TIMER | 0)

/* max delay 0.8388607875 second */
#define MAX_TIMER_COUNT 67108863

/* rotor phase 1<<6 = 0x040 also include the mask bit 0x040000 */
#define ROTOR_PHASE_GATE 0x040
#define ROTOR_PHASE_MASK 0x040000 

/** read these from pRF_InterruptStatus / interrupt status..*/
#define  UNDERFLOW_FLAG    (0x00000002)
#define  OVR_FLAG     (0x00000001)

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */
extern MSG_Q_ID pMsgs2FifoIST;     /* msgQ for harderror resulting from failure assertion */

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, (in globals.c) */

initFifo()
{
  unsigned int fifoIntMask;
    int instrfifosize;
    int newhiwatermark;

  fifoIntMask = get_mask(MASTER,fifo_overflow_status) | get_mask(MASTER,fifo_underflow_status) | 
                get_mask(MASTER,fifo_started_status) | get_mask(MASTER,fifo_finished_status) |
                get_mask(MASTER,invalid_opcode_int_status) |
                get_mask(MASTER,fail_int_status) | get_mask(MASTER,warn_int_status);

  cntrlFifoInit(&masterFifo_regs, fifoIntMask, get_instrfifosize(MASTER));

    /* +++++++ set the high and low device-paced FIFO DMA water marks +++++++  */
    /* size - headroom equal new high water mark for FIFO DMA device-paced */
    /* 8192 - 6 = 8186, so at 8186 words the DMA is told to stop, 
    /*    usually two more words get in */
    instrfifosize = getfifosize();
    newhiwatermark = instrfifosize - FIFO_HIWATER_MARK_HEADROOM;   /* 6 */
    setfifohimark(newhiwatermark);
    /* setfifolowmark( ..... );  leave as default of 512 or half the instruction FIFO size */

  set2fifo();

  initialBrdSpecificFailItr();

  /* set values in the SW FIFO Control registers to appropriate failsafe values */
  /* typically zero, when fail line is assert these values are presented to the output */
  /* of the controller, (e.g. high speed gates)  Values that need to be serialized */
  /* must strobed output by software however.	*/
  /*            Greg Brissey 9/16/04  */
  preSetAbortStates();

  /* cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, 
		int dmaChannel, volatile unsigned int* const pFifoWrite ) */

  if ((cntrlFifoDmaChan = dmaGetChannel(2)) == -1)
  { printf("No dma channel available for FIFO \n"); return; }

  /* MASTER_NUM_FIFO_BUFS 30 MASTER_SIZE_FIFO_BUFS 1024 */
  pCntrlFifoBuf =  cntlrFifoBufCreate(MASTER_NUM_FIFO_BUFS, MASTER_SIZE_FIFO_BUFS, cntrlFifoDmaChan, 
                        (volatile unsigned int*) get_pointer(MASTER,FIFOInstructionWrite));

}

int RotorSyncTst(int count) 
{
    int stat;
    int stream[6];
    stream[0] = encode_MASTERSetGates(0,0x40,0x40);  /* raise gate 6, sample phase count */
    stream[1] = encode_MASTERSelectTimeBase(0,1);    /* select external timebase, in our case the rotor speed pulses */
    stream[2] = encode_MASTERSetDuration(1,count);   /* wait for given number of  rotor pulses */
    stream[3] = encode_MASTERSelectTimeBase(0,0);    /*  select internal clock 80 MHz */
    stream[4] = encode_MASTERSetGates(0,0x40,0x0);  /* lower gate 6, nolonger needed */
    stream[5] = encode_MASTERSetAux(1,1,0,0);       /* delay base on the contents Rotor Phase count */
    cntrlFifoBufPut(pCntrlFifoBuf, (long *) stream, 6 );
    return OK;
}    

/*
 * Rotorsync - the master variant of rotor sync 
 *   enables the rotorsync function to occur
 *   assert the sync line to the other controllers
 *   which are waiting on the sync line for rotorsync to complete
 *
 *    Author: Greg Brissey  4/05/2005
 */
int RotorSync(int count, int postdelay) 
{
   int instrwords[20];
   int num;
   int fifoEncodeRotorSync(int count, int *instrwords);
   int fifoEncodeSystemSync(int postdelay, int *instrwords);

   num = fifoEncodeRotorSync(count,instrwords);
   num += fifoEncodeSystemSync(postdelay, &(instrwords[num]) );
   cntrlFifoBufPut(pCntrlFifoBuf, (long *) instrwords, num );
   return(num);
}

/*
 * fifoEncodeRotorSync(), return a encoded stream of FIFO instruction to
 * cause requested rotor sync to occur.
 *
 *    Author: Greg Brissey  4/05/2005
 */
int fifoEncodeRotorSync(int count, int *instrwords)
{
   int i = 0;
   instrwords[i++] = encode_MASTERSetGates(0,0x40,0x40);  /* raise gate 6, sample phase count */
   instrwords[i++] = encode_MASTERSelectTimeBase(0,1);    /* select external timebase, in our case the rotor speed pulses */
   instrwords[i++] = encode_MASTERSetDuration(1,count);   /* wait for given number of  rotor pulses */
   instrwords[i++] = encode_MASTERSelectTimeBase(0,0);    /*  select internal clock 80 MHz */
   instrwords[i++] = encode_MASTERSetGates(0,0x40,0x0);  /* lower gate 6, nolonger needed */
   instrwords[i++] = encode_MASTERSetAux(1,1,0,0);       /* delay base on the contents Rotor Phase count */
   return(i);
}

/*
 * fifoEncodeSWItr(), return a encoded stream of FIFO instruction to
 * cause selected Software FIFO interrupt to occur.
 *
 *    Author: Greg Brissey  8/11/04
 */
int fifoEncodeSWItr(int SWitr, int *instrwords)
{
   int i = 0;
   instrwords[i++] = encode_MASTERSetGates(0,0xf00,0);  /* only effect SW bits */
   /* gradient adjust */
   instrwords[i++] = encode_MASTERSetDuration(0,320); /* duration of asserted interrupt bit, 4 usec */
   switch(SWitr)
   {
 
      case 1: instrwords[i++] = encode_MASTERSetGates(1,0xf00,0x100); break;
      case 2: instrwords[i++] = encode_MASTERSetGates(1,0xf00,0x200); break;
      case 3: instrwords[i++] = encode_MASTERSetGates(1,0xf00,0x400); break;
      case 4: instrwords[i++] = encode_MASTERSetGates(1,0xf00,0x800); break;
   }
   instrwords[i++] = encode_MASTERSetGates(1,0xf00,0x000);
   return(i);
}

/* 
 *  Drive gate 0 high for 100 nsec, followed by another post delay 
 *  given by postdelay argument, approx. 100nsec
 *
 *      Author: Greg brissey 8/12/2004
 */
int fifoEncodeSystemSync(int postdelay, int *instrwords)
{
   int i = 0;
   if ((postdelay < 320) && (postdelay > 0))
   {
      DPRINT1(-2,"1 postDelay UP'd to 320!! was %d\n",postdelay);
      postdelay = 320;
   }
   /* gradient grid adjust */
   instrwords[i++] = encode_MASTERSetDuration(0,8); /* duration of asserted Sync bit, 100 ns */
   instrwords[i++] = encode_MASTERSetGates(1,FF_GATE_7,FF_GATE_7);
   instrwords[i++] = encode_MASTERSetDuration(0,postdelay); /* duration of post delay */
   instrwords[i++] = encode_MASTERSetGates(1,FF_GATE_7,0);
   return(i);
}

int fifoEncodeDuration(int write, int duration,int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_MASTERSetDuration(write,duration);
    return i;
}

int fifoEncodeGates(int write, int mask,int gates, int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_MASTERSetGates(write,mask,gates);
    return i;
}

set2fifo()
{
   set_register(MASTER,FIFOOutputSelect,1);
}


/* output controlled via SW registers (0) or FIFO output (1) */
void setFifoOutputSelect(int SwOrFifo)
{
   set_register(MASTER,FIFOOutputSelect,SwOrFifo);
   return;
}

int getfifosize()
{
  return( get_instrfifosize(MASTER) );
}

prtfifohilowmarks()
{
   int himark,lowmark;

   himark = get_field(MASTER,instruction_fifo_high_threshold);
   lowmark = get_field(MASTER,instruction_fifo_low_threshold);
   printf("FIFO DMA device-paced hi watermark: %d, low water mark: %d\n",himark,lowmark);
}

int getfifohimark()
{
   int himark;
   himark = get_field(MASTER,instruction_fifo_high_threshold);
   return( himark );
}

int setfifohimark(int hi)
{
   set_field(MASTER,instruction_fifo_high_threshold,hi);
   return(0);
}

int getfifolowmark()
{
   int lowmark;
   lowmark = get_field(MASTER,instruction_fifo_low_threshold);
   return( lowmark );
}

int setfifolowmark(int low)
{
   set_field(MASTER,instruction_fifo_low_threshold,low);
   return(0);
}

preSetAbortStates()
{
   SafeGateVal = 0;		
   set_register(MASTER,SoftwareGates,0);
   set_register(MASTER,SoftwareSpi0,0);
   set_register(MASTER,SoftwareSpi1,0);
   set_register(MASTER,SoftwareSpi2,0);
   set_register(MASTER,SoftwareAUX,0);
}

/* called fail interrupt isr & phandler */
void serializeSafeVals()
{
//   logMsg("serializeSafeVals complete.\n");
}


#ifdef XXXX
setAbortSerializedStates()
{
   goToSafeState();
/*
   set_field(MASTER,sw_aux_reset,0);
   set_field(MASTER,sw_aux_reset,1);
   set_field(MASTER,sw_aux_reset,0);
   set_register(MASTER,SoftwareAUX,0);
   set_field(MASTER,sw_aux_strobe,0);
   set_field(MASTER,sw_aux_strobe,1);
   set_field(MASTER,sw_aux_strobe,0);
*/
}
#endif

resetFifoHoldRegs()
{
   int duration,select,spi_select,chip_select,data,set_phase,address;

   duration = data = 0;
   select = spi_select = chip_select = 0;
   set_phase = address = 0;

   /* clear the FIFO gate holding register */
   cntrlFifoPut(encode_MASTERSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_MASTERSetDuration(0,duration));
   cntrlFifoPut(encode_MASTERSelectTimeBase(0,select));
/*   believe to cause problem with Z0, when I clear this. 
 *  cntrlFifoPut(encode_MASTERSetSPI(0,spi_select,chip_select,data));
 *   this might be a problem too, so don't do this either
 *  cntrlFifoPut(encode_MASTERSetAux(0,set_phase,address,data));
*/
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
      logMsg("Brd_Specific_Fail_ISR: status: 0x%lx, errorcode: %d\n",int_status, errorcode); 
   return;
}

/*====================================================================================*/
/*
 * enable all board specific failures interrupts
 */
initialBrdSpecificFailItr()
{
  unsigned int failureMask,intMask;
  failureMask =  get_mask(MASTER,spi_failed_busy0_enable) | 
                 get_mask(MASTER,spi_failed_busy1_enable) | 
	         get_mask(MASTER,spi_failed_busy2_enable);

  intMask =  get_mask(MASTER,spi_failed_busy0_enable); 
  DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask);
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + MASTER_SPI0_OVRRUN), intMask );

  intMask =  get_mask(MASTER,spi_failed_busy1_enable); 
  DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask);
  fpgaIntConnect( Brd_Specific_Fail_ISR, HDWAREERROR + MASTER_SPI1_OVRRUN, intMask );

  intMask =  get_mask(MASTER,spi_failed_busy2_enable); 
  DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask);
  fpgaIntConnect( Brd_Specific_Fail_ISR, HDWAREERROR + MASTER_SPI2_OVRRUN, intMask );

  cntrlFifoIntrpSetMask(failureMask);   /* enable interrupts */
}

/*============================================================================*/

/* ========================================================================== */

tstclr()
{
   cntrlFifoReset();
   cntrlResetIntStatus();
}

/*
 * rotor test using the fifoFuncs routines
 *
 */
testrphs(int rphase, int looper_max)
{
   int loop_count,i,numwords,numwords2;
   /* controller_reset(); */
   long initwords[10];
   long instrwords[10];

/* FV fpga register no longer exist */
/*  set_field(MASTER,spinner_air_period,400);
/*  set_field(MASTER,spinner_air_low,200);
/*  set_field(MASTER,spinner_air_enable,1);
/* */
  set_field(MASTER,spin_pulse_period,80000000);
  set_field(MASTER,spin_pulse_low,40000000);
  set_field(MASTER,spin_pulse_enable,1);


   i = 0;
   initwords[i++] = encode_MASTERSetGates(0,0xfff,0);

   /* get phase count */
   initwords[i++] = encode_MASTERSelectTimeBase(0,1); /* select external timebase, in our case the rotor speed pulses */
   initwords[i++] = encode_MASTERSetDuration(1,1); /* wait for one rotor pulse */
   initwords[i++] = encode_MASTERSelectTimeBase(0,0); /*  select internal clock 80 MHz */
   /* initwords[i++] = encode_MASTERSetDuration(1,2687); */
   initwords[i++] = encode_MASTERSetDuration(1,rphase);
   initwords[i++] = encode_MASTERSetGates(0,0x41,0x41);	/* raise gate 6, sample phase count */
   initwords[i++] = encode_MASTERSetDuration(1,10);       /* for 125 ns */
   initwords[i++] = encode_MASTERSetGates(1,0x41,0x0);	/* raise gate 6, sample phase count */
   numwords = i;
   printf("number of init words %d\n",numwords);

   cntrlFifoPIO(initwords, i);
   printf("number of owrds in FIFO %d\n",cntrlDataFifoCount());

   i = 0;
   instrwords[i++] = encode_MASTERSelectTimeBase(0,1); /* select external timebase, in our case the rotor speed pulses */
   instrwords[i++] = encode_MASTERSetDuration(1,1);    /* wait for one rotor pulse */
   instrwords[i++] = encode_MASTERSelectTimeBase(0,0); /*  select internal clock 80 MHz */
   instrwords[i++] = encode_MASTERSetAux(1,1,0,0);	/* lay base on the contents Rotor Phase count */
   instrwords[i++] = encode_MASTERSetDuration(0,800);       /* for 125 ns */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   numwords2 = i;
   printf("number of instruction words %d\n",numwords2);

   while (get_register(MASTER,InstructionFIFOCount) < 900)
   {
      cntrlFifoPIO(instrwords, numwords2);
   }
   printf("number of words in FIFO %d\n",cntrlDataFifoCount());
   printf("starting FIFO\n");
   cntrlFifoStart();
   loop_count = 0;
   /* while ((controller_status() == 0) && (loop_count < looper_max)) */
   while ( cntrlFifoRunning() && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
         cntrlFifoPIO(instrwords, numwords2);
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       cntrlFifoPut(encode_MASTERSetDuration(1,0));
   }
}


testGate1(int looper_max)
{
   long instrwords[10];
   int i,loop_count,ninstr;

   i = 0;
   instrwords[i++] = encode_MASTERSetGates(0,0xfff,0);
   instrwords[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   ninstr = i;
   while (get_register(MASTER,InstructionFIFOCount) < 900)
   {
      cntrlFifoPIO(instrwords, ninstr);
   }
   DPRINT1(-1,"testGate1: data fifo count: %ld\n",get_field(MASTER,data_fifo_count));
   taskDelay(calcSysClkTicks(1000));  /* 1 sec */
   cntrlFifoStart();
   loop_count = 0;
   DPRINT1(-1,"testGate1: fifo running: %d\n",cntrlFifoRunning());
   while ( cntrlFifoRunning() && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
         cntrlFifoPIO(instrwords, ninstr);
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       cntrlFifoPut(encode_MASTERSetDuration(1,0));
   }
}


testGate1dma(int looper_max)
{
   long instrbuf[1024];
   long endbuf[1024];
   int i,loop_count,ninstr;
   long long duration;
   long gatesperdma, totalgates;
   int nfree;

   i = 0;
   instrbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   instrbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   ninstr = i;
   gatesperdma = 1022;
   i = 0;
   endbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   endbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   endbuf[1023] = encode_MASTERSetDuration(1,0);

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();
   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT1(-1,"testGate1dma: initial durtation: %llu\n",duration);
   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));

   DPRINT1(-1,"testGate1dma() buf size: %d\n",ninstr);

   nfree = dmaReqsFreeToQueue(cntrlFifoDmaChan);
    /* cntrlFifoBufCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes, int rem, int startfifo); */
    /* cntrlFifoBufPut(FIFOBUF_ID pFifoBufId, unsigned long *words , long nWords ); */
   cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,nfree,0,0);
   totalgates = gatesperdma * nfree;
   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */
   looper_max -= nfree;

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT1(-1,"DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));


#ifdef XXXX
   cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,1);
   totalgates += gatesperdma;
   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT1(-1,"DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */
   cntrlFifoBufCopy(pCntrlFifoBuf,endbuf,ninstr,1);
   totalgates += ( gatesperdma - 1 );

   taskDelay(calcSysClkTicks(664));  /* 664 msec, taskDelay(40); */
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));
   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT1(-1,"DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   DPRINT1(-1,"DMA count reg: %lu\n",dmaGetCountReg(cntrlFifoDmaChan));

   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));
   taskDelay(calcSysClkTicks(1000));  /* 1 sec */
#endif

   cntrlFifoStart();
   cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,looper_max,0,0);
   totalgates += (gatesperdma * looper_max);
   cntrlFifoBufCopy(pCntrlFifoBuf,endbuf,ninstr,1,0,0);
   totalgates += ( gatesperdma - 1 );

#ifdef XXX

   /* DPRINT1(-1,"testGate1: fifo running: %d\n",cntrlFifoRunning()); */
   cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,looper_max,0,0);
   totalgates += (gatesperdma * looper_max);

   /* sub the haltop into fifo */
   instrbuf[1023] = encode_MASTERSetDuration(1,0);
   cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,1,0,0);
   totalgates += (gatesperdma - 1);

   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);

#endif
   cntrlFifoWait4Stop();

   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 800L );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
}

testdma(int looper_max)
{
   long instrbuf[1024];
   long endbuf[1024];
   int i,loop_count,ninstr;
   long long duration;
   long gatesperdma, totalgates;
   int iters,nfree;

   i = 0;
   instrbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   instrbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   ninstr = i;
   gatesperdma = 1022;
   i = 0;
   endbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   endbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   endbuf[1023] = encode_MASTERSetDuration(1,0);

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   nfree = dmaReqsFreeToQueue(cntrlFifoDmaChan);
   iters = (looper_max <= nfree) ? looper_max : nfree;
   /* cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,iters); */
   cntrlFifoBufXfer(pCntrlFifoBuf,instrbuf,ninstr,iters,0,0);
   totalgates = gatesperdma * iters;
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT1(-1,"DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));

   cntrlFifoStart();
   if (iters != looper_max)
   {
     iters = looper_max - iters;
     /* cntrlFifoBufCopy(pCntrlFifoBuf,instrbuf,ninstr,iters); */
     cntrlFifoBufXfer(pCntrlFifoBuf,instrbuf,ninstr,iters,0,0);
     totalgates += (gatesperdma * iters);
     DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
   }
   /* cntrlFifoBufCopy(pCntrlFifoBuf,endbuf,ninstr,1); */
   cntrlFifoBufXfer(pCntrlFifoBuf,endbuf,ninstr,1,0,0);
   totalgates += ( gatesperdma - 1 );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);

   cntrlFifoWait4StopItrp();
   /* cntrlFifoWait4Stop(); */

   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 800L );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
}



testbufdma(int looper_max)
{
   long instrbuf[1024];
   long endbuf[1024];
   int i,loop_count,ninstr;
   long long duration;
   long gatesperdma, totalgates;
   int iters,nfree;

   i = 0;
   instrbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   instrbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     instrbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   ninstr = i;
   gatesperdma = 1022;
   i = 0;
   endbuf[i++] = encode_MASTERSetGates(0,0xfff,0);
   endbuf[i++] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   for (i=2; i < 1024; )
   {
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
     endbuf[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   }
   endbuf[1023] = encode_MASTERSetDuration(1,0);

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, ninstr );
   /* nfree = dmaReqsFreeToQueue(cntrlFifoDmaChan); */
   nfree = 30;
   iters = (looper_max <= nfree) ? looper_max : nfree;
   for(i=0; i < iters; i++)
       cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, ninstr );
   totalgates = gatesperdma * iters;
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));

   cntrlFifoStart();
   if (iters != looper_max)
   {
     iters = looper_max - iters;
     for(i=0; i < iters; i++)
       cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, ninstr );
     totalgates += (gatesperdma * iters);
     DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
   }
   cntrlFifoBufPut(pCntrlFifoBuf, endbuf, ninstr );
   totalgates += ( gatesperdma - 1 );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);

   cntrlFifoWait4StopItrp();
   /* cntrlFifoWait4Stop(); */

   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 800L );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
}



testbuf2dma(int looper_max)
{
   long instrbuf[1024];
   long endbuf[1024];
   int i,loop_count,ninstr;
   long long duration;
   long gatesperdma, totalgates;
   int iters,nfree;

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   i = 0;
   instrbuf[0] = encode_MASTERSetGates(0,0xfff,0);
   instrbuf[1] = encode_MASTERSetDuration(0,800);       /* for 10 us */
   cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, 2 );

   endbuf[0] = encode_MASTERSetDuration(1,0);
   instrbuf[0] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
   instrbuf[1] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */
   gatesperdma = 0;
   totalgates = 0;
   /* iters = 1024 / 2 * 29; */
   iters = (looper_max <= (512*29)) ? looper_max : (512*29);
   for (i=0; i < iters; i++ )
   {
     cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, 2 );
     totalgates += 2;
   }
   gatesperdma = 1024;

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));

   cntrlFifoBufPut(pCntrlFifoBuf, endbuf, 1 );
   cntrlFifoBufForceRdy(pCntrlFifoBuf);

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT2(-1,"testGate1: instruct count: %d, data fifo count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   DPRINT2(-1,"testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));

   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */

   DPRINT1(-1,"Free Bufs remaining: %d\n",cntrlFifoBufFree(pCntrlFifoBuf));
   cntrlFifoStart();
#ifdef XXXX
   if (iters != looper_max)
   {
     iters = looper_max - iters;
     for (i=0; i < iters; i++ )
     {
       cntrlFifoBufPut(pCntrlFifoBuf, instrbuf, 2 );
       totalgates += 2;
     }
     DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
   }
   cntrlFifoBufPut(pCntrlFifoBuf, endbuf, 1 );
   cntrlFifoBufForceRdy(pCntrlFifoBuf);
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
#endif
   cntrlFifoWait4StopItrp();
   /* cntrlFifoWait4Stop(); */

   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 800L );
   DPRINT3(-1,"expected gates: %ld, duration: %ld, 0x%lx\n",totalgates, totalgates * 800, totalgates * 800);
}


strtit()
{
   long long duration;

   cntrlFifoStart();

   cntrlFifoWait4Stop();

   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 800L );
}

prtfifo()
{
   long long duration;
   printf("fifo running: %d\n",cntrlFifoRunning());
   printf("Fifo: instruct count: %d, data count: %ld\n",
		get_register(MASTER,InstructionFIFOCount),get_field(MASTER,data_fifo_count));
   cntrlFifoCumulativeDurationGet(&duration);
   printf("Duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   printf("gate changes: %llu\n",duration / 800L );
   printf("DMA device paced pended: %d\n",dmaGetDevicePacingStatus(cntrlFifoDmaChan));
   printf("testgate1: dmaReqInQ: %d, dmaReqFree2Q: %d\n",dmaReqsInQueue(cntrlFifoDmaChan), dmaReqsFreeToQueue(cntrlFifoDmaChan));
}

xfer(int looper_max)
{
   FIFOBUF_ID pfifoBuf;
   long instrwords[10];
   int i,dmaChannel;
   int loop_count;

   i = 0;
   dmaChannel = dmaGetChannel(2);
   pfifoBuf = cntlrFifoBufCreate(10,20,dmaChannel,get_pointer(MASTER,FIFOInstructionWrite));
   instrwords[i++] = encode_MASTERSelectTimeBase(0,0); /*  select internal clock 80 MHz */
   instrwords[i++] = encode_MASTERSetDuration(0,800);       /* for 125 ns */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x1);	/* blip gate 0 for 10 usec after the phase count cpmplete */
   instrwords[i++] = encode_MASTERSetGates(1,0x1,0x0);	/* raise gate 6, sample phase count */

   cntrlFifoBufCopy(pfifoBuf,instrwords,4,60,0,0);

   taskDelay(calcSysClkTicks(2000));  /* 2 sec */
   cntrlFifoStart();

   while ( cntrlFifoRunning() && (loop_count < looper_max))
   {
      cntrlFifoBufCopy(pfifoBuf,instrwords,4,1,0,0);
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       cntrlFifoPut(encode_MASTERSetDuration(1,0));
   }

   /*  status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
				(UINT32) dstAddr, ntxfrs, srcBufferQ, dstBufferQ); */
}
int tstSWI(int mode) 
{
   unsigned long instrwords[12];
    long long duration;
   int i = 0;

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();

   instrwords[i++] = encode_MASTERSetGates(0,0xfff,0);
   instrwords[i++] = encode_MASTERSetDuration(0,80000);      
   instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x100);
   instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x200);
   instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x400);
   instrwords[i++] = 0x88fff800;
   /* instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x800); */
   instrwords[i++] = 0x88fff000;
   /* instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x000); */
   instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x800);
   instrwords[i++] = encode_MASTERSetGates(1,0xfff,0x000);
   instrwords[i++] = encode_MASTERSetDuration(1,0);
   for(i=0; i < 10; i++)
         printf("instr[%d] = 0x%lx\n",i,instrwords[i]);
   switch(mode)
   {
    case 0:
       cntrlFifoBufPut(pCntrlFifoBuf, instrwords, 10 ); 
       cntrlFifoBufForceRdy(pCntrlFifoBuf); 
       break;
    case 1:
      cntrlFifoBufCopy(pCntrlFifoBuf, instrwords, 10, 1, 0, 0 );
      break;
   case 2:
   default:
      for(i=0; i < 10; i++)
         cntrlFifoPut(instrwords[i]);
      break;
   }
   prtfifo();
   taskDelay(calcSysClkTicks(747));  /* 747 msec, taskDelay(45); */
   cntrlFifoStart();
   cntrlFifoWait4StopItrp();
   cntrlFifoCumulativeDurationGet(&duration);
   DPRINT2(-1,"testGate1dma: initial duration: %llu, %lf us \n",duration, (((double) duration) * .0125));
   DPRINT1(-1,"testGate1dma: gate changes: %llu\n",duration / 80000L );
    return OK;
}    
