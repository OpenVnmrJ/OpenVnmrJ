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
=========================================================================
FILE: masterTests.c 4.1 03/21/08
=========================================================================
PURPOSE:
	Provide a file for board bringup and other misc. test  routines 

Externally available modules in this file:

Internal support modules in this file:

COMMENTS:

AUTHOR:		Greg Brissey  5/26/04

*/
#include <vxWorks.h>
#include <stdlib.h>
#include "nvhardware.h"
#include "master.h"
#include "master_fifo.h"
#include "semLib.h"
#include "rngLib.h"
#include "rngXBlkLib.h"
#include "logMsgLib.h"

#define INCLUDE_TESTING
#ifdef INCLUDE_TESTING
/*  test routines */

#include "FFKEYS.h"

#define FF_OVERFLOW    (1)
#define FF_UNDERFLOW   (2)
#define FF_FINISHED    (4)
#define SW_INTMASK     (0x78)
#define FF_SYSFAIL     (128)
#define FF_SYSWARN     (256)


/* FIFO Instruction Codes */
#define LFIFO  (0x80000000)
#define TIMER  (1 << 26) 
#define GATES  (2 << 26)
#define XTB    (3 << 26)
#define SPI    (2 << 29)
#define AUX    (9 << 26)

/* #define ROTOR_SYNC   ((9 << 26) | (1 < 12)) */
/* #define ROTOR_SYNC   0x48001000 */
#define ROTOR_SYNC   0x24001000

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

#include "dmaDrv.h"

long  *srcDataBufPtr = NULL;

buildfifobuf()
{
     int i;
     char *bufferZone;
      int size;

     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     srcDataBufPtr = (long*) malloc(128000*4); /* max DMA transfer in single shot */
     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     printf("bufaddr: 0x%08lx\n",srcDataBufPtr);
}

prtfifosize()
{
  DPRINT1(-1,"FIFO Size: %d\n",get_instrfifosize(MASTER));
}

prtfifomarks()
{
   int himark,lowmark;

   himark = get_field(MASTER,instruction_fifo_high_threshold);
   lowmark = get_field(MASTER,instruction_fifo_low_threshold);
   printf("hi watermark: %d, low water mark: %d\n",himark,lowmark);
}
sethimark(int hi)
{
   set_field(MASTER,instruction_fifo_high_threshold,hi);
}
setlowmark(int low)
{
   set_field(MASTER,instruction_fifo_low_threshold,low);
}

dmaVer(int bufsize,int ticks,int precharge)
{
   long *pBuf;
   int len,i;

   pBuf = srcDataBufPtr;
   for(i=0;i< bufsize; i++)
   {
      len =  fifoEncodeDuration(1, ticks, pBuf++);
   }
   len = fifoEncodeDuration(1, 0, pBuf);

   for(i=1; i < precharge; i++)
   {
        dmaXfer(2, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) srcDataBufPtr, (UINT32) (0x70000000 + MASTER_FIFOInstructionWrite),
                        bufsize, NULL, NULL);
   }
   dmaXfer(2, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) srcDataBufPtr, (UINT32) (0x70000000 + MASTER_FIFOInstructionWrite),
                        bufsize+1, NULL, NULL);
   taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
   clrfifodur();
   cntrlFifoStart();
   cntrlFifoWait4StopItrp();
   prtfifodur();
}

dmaspeedtrail(int bufsize,int ticks,int precharge)
{
   long *pBuf;
   int len,i;

   pBuf = srcDataBufPtr;
   for(i=0;i< bufsize; i++)
   {
      len =  fifoEncodeDuration(1, ticks, pBuf++);
   }

   for(i=0; i < precharge; i++)
   {
        dmaXfer(2, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) srcDataBufPtr, (UINT32) (0x70000000 + MASTER_FIFOInstructionWrite),
                        bufsize, NULL, NULL);
   }
   clrfifodur();
   taskDelay(calcSysClkTicks(50));  /* 50 ms, taskDelay(3); */
   cntrlFifoStart();
   while(1)
   {
        dmaXfer(2, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) srcDataBufPtr, (UINT32) (0x70000000 + MASTER_FIFOInstructionWrite) ,
                        bufsize, NULL, NULL);
   }
}


#ifdef XXXX
/* test ring buffer peek macro */
tstrngpeek()
{
  RINGXBLK_ID tstrng;
  int windview_event = 10;
  int toP,fromP;
  int value, nextval;
  int stat1,stat2;
   int i;

  tstrng = rngXBlkCreate(5, "test",windview_event,1);
  for(i=0; i < 5; i++)
     RNG_LONG_PUT(tstrng, i, toP);

  stat2 = RNG_LONG_PEEK(tstrng, &nextval, fromP);
  printf("nextval: %d\n",nextval);
  for(i=0; i < 5; i++)
  {
     stat1 = RNG_LONG_GET(tstrng, &value, fromP);
     stat2 = RNG_LONG_PEEK(tstrng, &nextval, fromP);
     printf("value: %d, nextval: %d,  stats: %d,  %d\n",value,nextval,stat1,stat2);
  }
  return 0;
}
#endif

tstmask()
{
   printf("fifo int mask: 0x%lx\n",get_mask(MASTER,fifo_overflow_status));
   printf("fifo int mask: 0x%lx\n",get_mask(MASTER,fifo_underflow_status));
   printf("fifo int mask: 0x%lx\n",get_mask(MASTER,fifo_finished_status));
   printf("uart int mask: 0x%lx\n",get_mask(MASTER,uart_int_status));
   printf("sw int mask: 0x%lx\n",get_mask(MASTER,sw_int_status));
   printf("inst fifo size: %d\n", get_instrfifosize(MASTER));
}

/* utility functions placed here */

void GprintStat(int itstate, int icount, int datacount, int flag, char *cstr)
{
  char line[80];
  int k;
  if (flag)  printf("ovfl undrfl done  sw fail warn |instr words |data words\n");
  if (itstate & FF_OVERFLOW)
    strcpy(line,"y  ");
  else
    strcpy(line,"n  "); 
  if (itstate & FF_UNDERFLOW)
    strcat(line,"  y    ");
  else
    strcat(line,"  n    "); 
  if (itstate & FF_FINISHED)
    strcat(line,"  n  ");
  else
    strcat(line,"  n  ");
  k = (itstate & SW_INTMASK) >> 3;
  switch (k)
    {
    case 0:    strcat(line," -- "); break;
    case 1:    strcat(line,"  1 "); break;
    case 2:    strcat(line,"  2 "); break; 
    case 3:    strcat(line,"  3 "); break;
    case 4:    strcat(line,"  4 "); break;
    case 5:    strcat(line,"  5 "); break;
    case 6:    strcat(line,"  6 "); break; 
    case 7:    strcat(line,"  7 "); break;
    }
    
  if (itstate & FF_SYSFAIL)
    strcat(line,"  y  ");
  else
    strcat(line,"  n  ");
  if (itstate & FF_SYSWARN)
    strcat(line,"  y  ");
  else
    strcat(line,"  n  ");
    printf("  %s   %5d       %5d    | %s\n",line, icount, datacount,cstr);
}

GPrintOutVector(int flag) 
{
  if (flag)
  printf(" spi0  spi1   duration   gates\n");
  printf("%4x   %4x   %8x    %3x\n",get_field(MASTER,fifo_spi0), get_field(MASTER,fifo_spi1), get_field(MASTER,holding_duration), get_field(MASTER,fifo_gates));
}

/* this is going to get a bit wierd but the isr clears the status
   I'll copy the status into isr_status so the controller_status will
   know via an or when interrupt are on.. */
int isr_status = 0;

void controller_reset()
{
   int word;
   /* wake_FPGA();   /* this may become unnecessary */
   set_register(MASTER,FIFOOutputSelect,1);
   isr_status = 0;
   /*
   set_register(MASTER,FIFOControl,0); 
   set_register(MASTER,FIFOControl,4); 
   set_register(MASTER,FIFOControl,0);
   */
   set_field(MASTER,fifo_reset,0);
   set_field(MASTER,fifo_reset,1);
   set_field(MASTER,fifo_reset,0);
   /* enable these so status can be read */
   word = get_register(MASTER,InterruptStatus);
   set_register(MASTER,InterruptClear, 0); 
   set_register(MASTER,InterruptClear, word);
   set_register(MASTER,InterruptEnable, 0x7f);
   /* clean out the counter */
   set_field(MASTER,clear_cumulative_duration,0);
   set_field(MASTER,clear_cumulative_duration,1);
}

clr_fifostate()
{
   int word;
   /* enable these so status can be read */
   word = get_register(MASTER,InterruptStatus);
   set_register(MASTER,InterruptClear, 0); 
   set_register(MASTER,InterruptClear, word);
   set_register(MASTER,InterruptEnable, 0x7f);
}

void switch2sw()
{
   set_field(MASTER,fifo_output_select,1);
   set_field(MASTER,fifo_output_select,0);
}

void switch2fifo()
{ 
  set_field(MASTER,fifo_output_select,0); 
  set_field(MASTER,fifo_output_select,1);
}


int controller_status()
{
  return(get_register(MASTER,InterruptStatus) | isr_status);
}

void controller_start()
{
  set_register(MASTER,FIFOControl,0);
  set_register(MASTER,FIFOControl,1);
}

void controller_start_sync()
{
  set_register(MASTER,FIFOControl,0);
  set_register(MASTER,FIFOControl,2);
}

void controller_clear()
{
  set_register(MASTER,FIFOControl,0);
}

void fifo_put(unsigned int val)
{
  set_register(MASTER,FIFOInstructionWrite,val);
}


void controller_tell()
{
  GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"");
}      


/*
 * rotor-phase test
 * feed function generator signal into both the rotor counter
 * and the external clock
 * For the FIFO to make the initial measure of phase needs the following.
 * externl clock - along with the Rotot_Sync_gate 
 * then after a delay base upon approx freq of spinner signal
 * lower the Rotor_Sync_gate. Now the Rotor phase has the phase of the rotor
 * for that delay after the start of the rotor mark.
 * Now use the external trigger and the Rotor_Sync FIFO command to
 * synchronise the FIFO timing to the measured rotor.
 * Use fifo gate 0 as a test point to use a scope to see the relationship 
 * between the fifo gate the the spinner clock from the function generator.
 * We expect the gate to always be at the same location in relation to the pulse
 * train from the function generator.
 *
 * Author: Greg Brissey   5/24/04
 */

/*
 * select the rotor_spin_pulse as the source of the external clock
 *  PIN J2-C11
 */
setrotor2clk()
{
   set_field(MASTER,ext_timebase_select,1);
}
/*
 * select the extern_trig as the source of the external clock
 *  PIN J2-C1
 */
setextrig2clk()
{
  set_field(MASTER,ext_timebase_select,0);
}

testgate1(int looper_max)
{
   unsigned int rotormark,timerword1,timerword2;
   int gate1on,gate1off;
   unsigned int loop_count;
   rotormark = XTB | (1 & TIMER_MASK) | LFIFO ; /* wait for 1 tick of the external time base which is the rotor speed pulse */
   /* timerword1 = TIMER | (5372 & TIMER_MASK) | LFIFO ;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   timerword2 = TIMER | (5372 & TIMER_MASK) ;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   /* timerword2 = TIMER | (20000000 & TIMER_MASK) ;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   /* timerword1 = encode_MASTERSetDuration(1,5372); */
   /* timerword2 = encode_MASTERSetDuration(0,5372); */
   gate1on = GATES | 0x01001 | LFIFO;
   gate1off = GATES | 0x01000 | LFIFO;
   /* gate1on = encode_MASTERSetGates(1,1,1); */
   /* gate1off = encode_MASTERSetGates(1,1,0); */
   printf("gate0on: 0x%lx, 0x%lx\n",(GATES | 0x01001 | LFIFO), gate1on);
   controller_reset();
   /* load constant count */
   fifo_put(timerword2);
   GPrintOutVector(1);
   while (get_register(MASTER,InstructionFIFOCount) < 1000/2)
   {
      fifo_put((gate1on));
      fifo_put((gate1off));
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 999)
      {
             set_register(MASTER,FIFOInstructionWrite,gate1on);
             set_register(MASTER,FIFOInstructionWrite,gate1off);
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
     }
}
testXgate(int num, int looper_max)
{
   unsigned int rotormark,timerword1,timerword2;
   int gate1on,gate1off;
   unsigned int loop_count;
   timerword1 = TIMER | (1280 & TIMER_MASK);  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   timerword2 = TIMER | (1500 & TIMER_MASK) ;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   /* timerword2 = TIMER | (20000000 & TIMER_MASK) ;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
/*
   gate1on = GATES | 0x0101 ;
   gate1off = GATES | 0x0100 ;
*/
   gate1on = GATES | 0x001001; 
   gate1off = GATES | 0x001000; 

   controller_reset();
   /* fifo_put(timerword2 | LFIFO);	 */
   while (get_register(MASTER,InstructionFIFOCount) < 1000/8)
   {
      fifo_put( XTB  | 1);  /* select external timebase, in our case the rotor speed pulses */
      fifo_put(TIMER | num | LFIFO);  /* wait for one rotor pulse */
      fifo_put( XTB  | 0);  /* back to internal 80 MHz timebase */
      fifo_put(timerword1);		/* delay half a rotation @ 15KHz */
      fifo_put(gate1on | LFIFO);
      fifo_put(timerword2);		/* delay half a rotation @ 15KHz */
      fifo_put(gate1off | LFIFO);
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
             set_register(MASTER,FIFOInstructionWrite,(XTB  | 1));
             set_register(MASTER,FIFOInstructionWrite,(TIMER | num | LFIFO));
             set_register(MASTER,FIFOInstructionWrite,(XTB | 0));
             set_register(MASTER,FIFOInstructionWrite,(timerword1));
             set_register(MASTER,FIFOInstructionWrite,(gate1on | LFIFO));
             set_register(MASTER,FIFOInstructionWrite,(timerword2));
             set_register(MASTER,FIFOInstructionWrite,(gate1off | LFIFO));
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
     }
}

testRotoSync1(int num)
{
   /* at 15Khz spinning speed a periods is ~5374 ticks of the 12.5 nsec clock (80MHz) */
   /* 1st. if we delay 5374/2 prior to sample the phase we should get 5374/2 or 2687 tick in the phase register */
   unsigned int rotormark,timerword,timerword2;
   int gate6on,gate6off;
   timerword = TIMER | (2687 & TIMER_MASK) | LFIFO;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   timerword2 = TIMER | (800 & TIMER_MASK);  /* 10 us delay */
   gate6on = GATES | ROTOR_PHASE_MASK | ROTOR_PHASE_GATE | 0x001001;
   gate6off = GATES | ROTOR_PHASE_MASK | 0 | 0x001000;
   controller_reset();
   /* GPrintOutVector(1); */

   fifo_put(gate6off);
   fifo_put( XTB  | 1);  /* select external timebase, in our case the rotor speed pulses */
   fifo_put(TIMER | num | LFIFO);  /* wait for one rotor pulse */
   fifo_put( XTB  | 0);  /* back to internal 80 MHz timebase */
   fifo_put(timerword);		/* delay half a rotation @ 15KHz */
   fifo_put(timerword2);		/* delay half a rotation @ 15KHz */
   fifo_put(gate6on | LFIFO);		/* sample phase */
   fifo_put(gate6off | LFIFO);
   fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
   controller_start();
   while ((controller_status() == 0) && (get_register(MASTER,DataFIFOCount) > 0));
   printf("MASTER_SpinnerPhase: %lu\n", get_field(MASTER,spinner_phase));
   controller_tell();
}

testRotorSync2(int num,unsigned int looper_max)
{
   unsigned int timerword,timerword2;
   int gate1on, gate1off;
   unsigned int loop_count;

   timerword = TIMER | (800 & TIMER_MASK);  /* 10 us delay */
   gate1on = GATES | 0x001001; 
   gate1off = GATES | 0x001000; 

   /* select rotor_spin_pulse as the extern clock base */
   setrotor2clk();

   /* obtain rotor phase */
   testRotoSync1(1);

   /* controller_reset(); */
   clr_fifostate();

   while (get_register(MASTER,InstructionFIFOCount) < 1000/10)
   {
      fifo_put( XTB  | 1);  /* select external timebase, in our case the rotor speed pulses */
      fifo_put(TIMER | num | LFIFO);  /* wait for one rotor pulse */
      fifo_put( XTB  | 0);  /* back to internal 80 MHz timebase */
      fifo_put( ROTOR_SYNC | LFIFO);    /* delay base on the contents Rotor Phase count */
      fifo_put(timerword);		/* 10 us pulse duration on & off */ 
      fifo_put(gate1on | LFIFO);
      fifo_put(gate1off | LFIFO);
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
             set_register(MASTER,FIFOInstructionWrite,(XTB  | 1));
             set_register(MASTER,FIFOInstructionWrite,(TIMER | num | LFIFO));
             set_register(MASTER,FIFOInstructionWrite,(XTB | 0));
             set_register(MASTER,FIFOInstructionWrite,( ROTOR_SYNC | LFIFO));    /* delay base on the contents Rotor Phase count */
             set_register(MASTER,FIFOInstructionWrite,(timerword));		/* 10 us pulse duration on & off */ 
             set_register(MASTER,FIFOInstructionWrite,(gate1on | LFIFO));
             set_register(MASTER,FIFOInstructionWrite,(gate1off | LFIFO));
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
   }
   printf("MASTER_SpinnerPhase: %lu\n", get_field(MASTER,spinner_phase));

   controller_tell();

}

testRs3(int num,int looper_max)
{
   int fifowords[100];
   int i,fifocnt;
   unsigned int timerword,timer10us;
   int gate6on,gate6off;
   int gate1on, gate1off;
   unsigned int loop_count;

   controller_reset();

   timerword = TIMER | (2687 & TIMER_MASK) | LFIFO;  /* wait for 1/2 rotation of rotor base on 15KHz spinner speed */
   timer10us = TIMER | (800 & TIMER_MASK);  /* 10 us delay */
   gate6on = GATES | ROTOR_PHASE_MASK | ROTOR_PHASE_GATE | 0x001001;
   gate6off = GATES | ROTOR_PHASE_MASK | 0 | 0x001000;
   fifo_put(gate6off);
   fifo_put( XTB  | 1);  /* select external timebase, in our case the rotor speed pulses */
   fifo_put(TIMER | num | LFIFO);  /* wait for one rotor pulse */
   fifo_put( XTB  | 0);  /* back to internal 80 MHz timebase */
   fifo_put(timerword);		/* delay half a rotation @ 15KHz */
   fifo_put(timer10us);		/* delay half a rotation @ 15KHz */
   fifo_put(gate6on | LFIFO);		/* sample phase */
   fifo_put(gate6off | LFIFO);

   fifocnt = 0;
   fifowords[fifocnt++] = ( XTB  | 1);  /* select external timebase, in our case the rotor speed pulses */
   fifowords[fifocnt++] = (TIMER | num | LFIFO);  /* wait for one rotor pulse */
   fifowords[fifocnt++] = ( XTB  | 0);  /* back to internal 80 MHz timebase */
   fifowords[fifocnt++] = ( ROTOR_SYNC | LFIFO);    /* delay base on the contents Rotor Phase count */
   fifowords[fifocnt++] = (timer10us);		/* 10 us pulse duration on & off */ 
   fifowords[fifocnt++] = (gate1on | LFIFO);
   fifowords[fifocnt++] = (gate1off | LFIFO);

   while (get_register(MASTER,InstructionFIFOCount) < 900)
   {
       for(i=0; i < fifocnt; i++)
          set_register(MASTER,FIFOInstructionWrite,(fifowords[i]));
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
         for(i=0; i < fifocnt; i++)
            set_register(MASTER,FIFOInstructionWrite,(fifowords[i]));
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
   }


}

int spiFifotest(int chan, int value, int looper_max)
{
   int i,spiword;
   int timerword;
   int chip = 0;
   controller_reset();
//   setHsSpiFreq(chan, 1000);
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"Loading..");
   timerword = TIMER | (8000 & TIMER_MASK) | LFIFO;
   fifo_put(timerword);
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"Loading..");
   spiword =  SPI | SPI_CHAN_SELECT(chan) | SPI_CHIP_SELECT(chip) | SPI_DATA(value) | LFIFO;
   for (i=0; i < 500; i++)
   {
     fifo_put((spiword));
     fifo_put((timerword));
   }
   fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"filled");
   controller_start();
}

int spiFifotest2(int chan, int value, int looper_max)
{
   unsigned int rstat, loop_count,stuff_count;
   unsigned int laststat,stat,blast;
   int i;
   int spiword;
   int timerword;
   int chip = 0;
   controller_reset();
//   setHsSpiFreq(chan, 1000);
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"Loading..");
   timerword = TIMER | (8000 & TIMER_MASK) | LFIFO;    /* 100usec between SPI, SPI takes 24usec @ 1MHz to send 24 bits of data */
   fifo_put(timerword);
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"Loading..");
   spiword =  SPI | SPI_CHAN_SELECT(chan) | SPI_CHIP_SELECT(chip) | SPI_DATA(value) | LFIFO;
   while (get_register(MASTER,InstructionFIFOCount) < 1000/2)
   {
      fifo_put((spiword));
      fifo_put((timerword));
   }
   GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),1,"filled");
   loop_count = 0;
   /* fifo_put(timerword); */
   /* fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
   controller_start();
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 999)
      {
             set_register(MASTER,FIFOInstructionWrite,spiword);
             set_register(MASTER,FIFOInstructionWrite,timerword);
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
     }
}

void Fifo_Isr(int int_status, int val) 
{
  int RFI_status;
  char *buffer;
  RFI_status = int_status;
  
  
  /* isr_status = RFI_status & ~SW_INTMASK;  /* sw ints don't stop */
  
  if (RFI_status & FF_OVERFLOW)   buffer = " Overflow\n"; 
  if (RFI_status & FF_UNDERFLOW)  buffer = " Underflow\n"; 
  if (RFI_status & FF_FINISHED)   buffer = " Finished\n";
  if (RFI_status & 0x08)           buffer = " SW INT 1\n" ;
  if (RFI_status & 0x10)           buffer = " SW INT 2\n" ;
  if (RFI_status & 0x20)           buffer = " SW INT 3\n" ;
  if (RFI_status & 0x40)           buffer = " SW INT 4\n" ;
  if (RFI_status & FF_SYSWARN)    buffer = " System Warning\n";
  if (RFI_status & FF_SYSFAIL)    buffer = " System Fail!\n";
  logMsg("%s  %x|%x\n",buffer,get_register(MASTER,CumulativeDurationHigh),\
        get_register(MASTER,CumulativeDurationLow), 0,0,0,0,0,0,0,0);

}

initFifoIsr()
{
    initFpgaBaseISR();
    fpgaIntConnect(Fifo_Isr,0);
    controller_reset();
    /* set_field(MASTER,sw_int_enable,1); */
/*
    set_field(MASTER,fifo_overflow_enable,1);
    set_field(MASTER,fifo_underflow_enable,1);
    set_field(MASTER,fifo_finished_enable,1);
    set_field(MASTER,data_fifo_almost_empty_enable,1);
    set_field(MASTER,instr_fifo_almost_full_enable,1);
*/
    initSpiItrs(); 
}

#define instr(op) *((volatile unsigned *)(MASTER_BASE+MASTER_FIFOInstructionWrite)) = (op)

void test_timebase ()
{
  int i;
/* FV hardware no longer exists */
/*  set_field(MASTER,spinner_air_period,400);
/*  set_field(MASTER,spinner_air_low,200);
/*  set_field(MASTER,spinner_air_enable,1);
/* */
  set_field(MASTER,spin_pulse_period,80000000);
  set_field(MASTER,spin_pulse_low,40000000);
  set_field(MASTER,spin_pulse_enable,1);
  set_field(MASTER,ext_timebase_select,1);

  i = encode_MASTERSetGates(0,0xfff,0);
  instr(encode_MASTERSetGates(0,0xfff,0));
  for (i=0; i<3; i++)
    {
      /**********
	Set external timebase
	Duration 1,   write
      */
      instr(encode_MASTERSelectTimeBase(0,1));
      instr(encode_MASTERSetDuration(1,1));

      /**********
	Set internal timebase
	Duration 100
	Raise gate[0], write
      */
      instr(encode_MASTERSelectTimeBase(0,0));
      instr(encode_MASTERSetDuration(0,100));
      instr(encode_MASTERSetGates(1,1,1));
      /**********
        Duration 150
        Lower gate[0], write
      */
      instr(encode_MASTERSetDuration(0,150));
      instr(encode_MASTERSetGates(1,1,0));
    }
  /**********
    External timebase
    Diration 1, write
  */
  instr(encode_MASTERSelectTimeBase(0,1));
  instr(encode_MASTERSetDuration(1,1));

  /**********
    Internal timebase
    Duration 200, write
  */
  instr(encode_MASTERSelectTimeBase(0,0));
  instr(encode_MASTERSetDuration(1,200));

  /**********
    Raise gate[6]
    Duration 10, write
  */
  instr(encode_MASTERSetGates(0,0x40,0x40));
  instr(encode_MASTERSetDuration(1,10));

  /**********
    Lower gate[6]
    External timebase
    Diration 1, write
  */
  instr(encode_MASTERSetGates(1,0x40,0));
  instr(encode_MASTERSelectTimeBase(0,1));
  instr(encode_MASTERSetDuration(1,1));

  /**********
    Internal timebase
    Set duration to captured phase
    Raise gate[0], write
  */
  instr(encode_MASTERSelectTimeBase(0,0));
  instr(encode_MASTERSetAux(0,1,0,0));
  instr(encode_MASTERSetGates(1,1,1));

  /**********
    Duration 15
    Lower gate[0], write
  */
  instr(encode_MASTERSetDuration(0,15));
  instr(encode_MASTERSetGates(1,1,0));
  /**********
    Duration 0, write
  */
  instr(encode_MASTERSetDuration(1,0));

  set_field(MASTER,fifo_output_select,1);
  set_field(MASTER,fifo_start,0);
  set_field(MASTER,fifo_start,1);
}

testPRT1()
{
  controller_reset();
   set_field(MASTER,ext_timebase_select,1);
   instr(encode_MASTERSetGates(0,0xfff,0));
   instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
   instr(encode_MASTERSetDuration(1,1)); /* wait for one rotor pulse */
   instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
   instr(encode_MASTERSetDuration(1,2687));
   instr(encode_MASTERSetGates(0,0x41,0x41));	/* raise gate 6, sample phase count */
   instr(encode_MASTERSetDuration(1,1));       /* for 125 ns */
   instr(encode_MASTERSetGates(1,0x41,0x0));	/* raise gate 6, sample phase count */
   /* instr(encode_MASTERSetDuration(1,800));       /* for 10 us */
   instr(encode_MASTERSetDuration(1,0));
   controller_start();
   while ((controller_status() == 0) && (get_register(MASTER,DataFIFOCount) > 0));
   printf("MASTER_SpinnerPhase: %lu\n", get_field(MASTER,spinner_phase));
}
prtinst()
{
   printf("0x%lx\n",encode_MASTERSetAux(0,1,0,0));
   printf("0x%lx\n",encode_MASTERSetAux(1,1,0,0));
}
testPRT2(int rphase, int looper_max)
{
   int loop_count;
   /* obtain rotor phase */
   /* testPRT1(1); */
   controller_reset();
 
   instr(encode_MASTERSetGates(0,0xfff,0));

   /* get phase count */
   instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
   instr(encode_MASTERSetDuration(1,1)); /* wait for one rotor pulse */
   instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
   /* instr(encode_MASTERSetDuration(1,2687)); */
   /* instr(encode_MASTERSetDuration(1,3687)); */
   instr(encode_MASTERSetDuration(1,rphase));
   instr(encode_MASTERSetGates(0,0x41,0x41));	/* raise gate 6, sample phase count */
   instr(encode_MASTERSetDuration(1,10));       /* for 125 ns */
   instr(encode_MASTERSetGates(1,0x41,0x0));	/* raise gate 6, sample phase count */

   while (get_register(MASTER,InstructionFIFOCount) < 900)
   {
      instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
      instr(encode_MASTERSetDuration(1,1));    /* wait for one rotor pulse */
      instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
      instr(encode_MASTERSetAux(1,1,0,0));	/* lay base on the contents Rotor Phase count */
      instr(encode_MASTERSetDuration(0,800));       /* for 125 ns */
      instr(encode_MASTERSetGates(1,0x1,0x1));	/* blip gate 0 for 10 usec after the phase count cpmplete */
      instr(encode_MASTERSetGates(1,0x1,0x0));	/* raise gate 6, sample phase count */
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
         instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
         instr(encode_MASTERSetDuration(1,1));    /* wait for one rotor pulse */
         instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
         instr(encode_MASTERSetAux(1,1,0,0));	/* lay base on the contents Rotor Phase count */
         instr(encode_MASTERSetDuration(0,800));       /* for 125 ns */
         instr(encode_MASTERSetGates(1,0x1,0x1));	/* blip gate 0 for 10 usec after the phase count cpmplete */
         instr(encode_MASTERSetGates(1,0x1,0x0));	/* raise gate 6, sample phase count */
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
   }
}
testPRT3(int looper_max)
{
   int loop_count;
   controller_reset();
 
   instr(encode_MASTERSetGates(0,0xfff,0));
   instr(encode_MASTERSetGates(0,0x40,0x40));	/* raise gate 6, sample phase count */
   instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
   instr(encode_MASTERSetDuration(1,1));    /* wait for one rotor pulse */
   instr(encode_MASTERSetGates(0,0x40,0x0));	/* raise gate 6, sample phase count */
   while (get_register(MASTER,InstructionFIFOCount) < 900)
   {
      instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
      instr(encode_MASTERSetDuration(1,1));    /* wait for one rotor pulse */
      instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
      instr(encode_MASTERSetAux(1,1,0,0));	/* lay base on the contents Rotor Phase count */
      instr(encode_MASTERSetDuration(0,800));       /* for 125 ns */
      instr(encode_MASTERSetGates(1,0x1,0x1));	/* blip gate 0 for 10 usec after the phase count cpmplete */
      instr(encode_MASTERSetGates(1,0x1,0x0));	/* raise gate 6, sample phase count */
   }
   controller_start();
   loop_count = 0;
   while ((controller_status() == 0) && (loop_count < looper_max))
   {
      if (get_register(MASTER,InstructionFIFOCount) < 990)
      {
         instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
         instr(encode_MASTERSetDuration(1,1));    /* wait for one rotor pulse */
         instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
         instr(encode_MASTERSetAux(1,1,0,0));	/* lay base on the contents Rotor Phase count */
         instr(encode_MASTERSetDuration(0,800));       /* for 125 ns */
         instr(encode_MASTERSetGates(1,0x1,0x1));	/* blip gate 0 for 10 usec after the phase count cpmplete */
         instr(encode_MASTERSetGates(1,0x1,0x0));	/* raise gate 6, sample phase count */
      }
      loop_count++;
   }
   if (loop_count >= looper_max) 
     { 
       fifo_put(TIMER | 0 | LFIFO);    /* stop FIFO word */
       GprintStat(controller_status(),get_register(MASTER,InstructionFIFOCount),get_register(MASTER,DataFIFOCount),0,"LOOP COUNT MAX'D"); 
   }
}
testPRT4()
{
  controller_reset();
   set_field(MASTER,ext_timebase_select,1);
   instr(encode_MASTERSetGates(0,0xfff,0));
   instr(encode_MASTERSetGates(0,0x40,0x40));	/* raise gate 6, sample phase count */
   instr(encode_MASTERSelectTimeBase(0,1)); /* select external timebase, in our case the rotor speed pulses */
   instr(encode_MASTERSetDuration(1,1)); /* wait for one rotor pulse */
   instr(encode_MASTERSelectTimeBase(0,0)); /*  select internal clock 80 MHz */
   instr(encode_MASTERSetGates(0,0x40,0x0));	/* raise gate 6, sample phase count */
   instr(encode_MASTERSetDuration(1,800)); /* wait for one rotor pulse */
   instr(encode_MASTERSetDuration(1,0));
   controller_start();
   while ((controller_status() == 0) && (get_register(MASTER,DataFIFOCount) > 0));
   printf("MASTER_SpinnerPhase: %lu\n", get_field(MASTER,spinner_phase));
}



/*  test routines */

int swReadAux(int reg)
{
  reg &= 7;
  reg |= 8;  /* set the read back bit */
  switch2sw(); /* arbitrarily */
  set_field(MASTER,sw_aux,(reg<<8));
  set_field(MASTER,sw_aux_strobe,1); 
  set_field(MASTER,sw_aux_strobe,0);
  switch2fifo();
  return(get_field(MASTER,aux_read));
}
/* the 8 is the read commond */  
void swWriteAux(int reg,int data)
{
  switch2sw(); /* arbitrarily */
  reg &= 7; 
  data &= 0xff;
  set_field(MASTER,sw_aux,(((reg)<<8) | data));
  set_field(MASTER,sw_aux_strobe,1); 
  set_field(MASTER,sw_aux_strobe,0);
  switch2fifo();
}
auxReset()
{
   set_field(MASTER,sw_aux_reset,0);
   set_field(MASTER,sw_aux_reset,1);
   set_field(MASTER,sw_aux_reset,0);
}



tstBadOpCode(int opcode)
{
    int badopcode;
    badopcode = (opcode << 26);
    setFifoOutputSelect( 1 ); /* switch to FIFO control */
    cntrlFifoReset();
    cntrlClearInstrCountTotal();
    cntrlFifoCumulativeDurationClear();
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(badopcode);  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_MASTERSetDuration(1,0));  /* 2usec */

    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    printf("fifo instructions reciev'd: %d\n",cntrlInstrCountTotal());
    cntrlFifoStart();
}









struct MASTERHOLD {
  int duration;
  int gates;
}  MASTER_state;
 
unsigned long totalduration;
extern unsigned long long fidduration;
extern unsigned long long cumduration;

void fifoDecode(unsigned int word,int noprt)
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
           /* not complete */
          switch (tmp)
            {
               case DURATIONKEY:
   		 if (noprt != 1)
                    printf("duration of %7.4f usec",((float) tdata)/80.0);
                 MASTER_state.duration = tdata;  break;

               case GATEKEY:
   		 if (noprt != 1)
                    printf("mask/gate set to %x",tdata&0xfff);
                 MASTER_state.gates = tdata; break;

               case MXGATE:
   		 if (noprt != 1)
                    printf("External Gatemask, ticks  %x",tdata&0xfff);
		    break;

               case SPIKEY:
   		 if (noprt != 1)
                    printf("SPI, spi select: %d, chp select: %d, data  %x",((tdata >> 28) & 0x3 ), ((tdata >> 24) & 0xf ), tdata&0xffffff);
		    break;

               case USER:
   		 if (noprt != 1)
                    printf("user data = %x\n",(tdata&0x3)); break;
               case AUX:
   		 if (noprt != 1)
                    printf("aux: set phase %d, addr: 0x%x, data: 0x%x\n",((tdata>>12) & 0x1),((tdata>>8)  & 0xF), \
                        tdata & 0xff); break;
            default:
                  /* printf("don't recognize key!! %x\n",tmp); */
		 break;
           }
          if (latched)
          {
   	     if (noprt != 1)
                printf(" fifo word latched\n");
             if (tmp != AUX)
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of %9.4lf usec  GATE = %4x  ", \
                    ((float) MASTER_state.duration)/80.0,MASTER_state.gates);
               totalduration = totalduration + MASTER_state.duration;
             }
             else
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of 0.050 usec AUX  GATE = %4x  ", \
                   MASTER_state.gates);
               totalduration = totalduration + 4;
             }
             /*
             cumduration += (unsigned long long) totalduration;
             fidduration += (unsigned long long) totalduration;
             printf("cumduration: %llu, fidduration: %llu, duration: %lu\n",cumduration,fidduration,totalduration);
	     */

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
   printf("----->>  Durations: Cum: %llu ticks, %18.4f,   \n----->>             FID: %llu ticks, %18.4f,   Buffer: %lu ticks, %7.4f usec  \n\n",
		cumduration, ((double) cumduration) / 80.0, fidduration, ((double) fidduration) / 80.0, totalduration, 
	        ((float) totalduration)/80.0);
}


/*====================================================================================*/


#endif
