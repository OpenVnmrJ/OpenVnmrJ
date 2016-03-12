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
FILE: pfgTests.c 4.1 03/21/08
=========================================================================
PURPOSE:
	Provide a file for board bringup and other misc. test  routines 

Externally available modules in this file:

Internal support modules in this file:

COMMENTS:   test the SW interrupt and the axis too short interrupts

AUTHOR:		  Greg Brissey 11/09/04

*/
#include <vxWorks.h>
#include <stdlib.h>
#include "nvhardware.h"
#include "logMsgLib.h"
#include "fifoFuncs.h"
#include "pfg.h"
#include "pfg_fifo.h"

#define INCLUDE_TESTING
#ifdef INCLUDE_TESTING
/*  test routines */

static int SwIsrChanged = 0;
static int ShortIsrChanged = 0;
pfgSwInt(int swItrId)
{
   int fifowords[10];
   long haltop;
   int num;
   num = fifoEncodeSWItr(swItrId, fifowords);
   /* writeCntrlFifoBuf(fifowords, num); */
   cntrlFifoPIO((unsigned long*) fifowords, num); 
   /* haltop */
   cntrlFifoPut(encode_PFGSetDuration(1,0000));
   /* writeCntrlFifoWord(encode_PFGSetDuration(1,0000));  */
}

/*
 * stuff the FIFO with the appropriate codes to trigger the give Software Interrupt
 */
pfgTstSW(int i)
{
    cntrlFifoReset();
    cntrlFifoCumulativeDurationClear();
    pfgSwInt(i);
    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    cntrlFifoStart();
    cntrlFifoWait4StopItrp();
}

/*
 * the test SW ISR routine, just print with SW interrupt occurred.
 */
static void Test_Fifo_SW_ISR(int int_status, int val) 
{
  char *buffer;
  int acode;

   /* EVENT_FIFO_SW1_ITR = 21, 2 = 22, 3 = 23, 4 = 24 */
   DPRINT1(-1,"Test_Fifo_SW_ISR: SW Itr#: %d\n",val);

   return;
}

/*
 * remove the standard ISR that's inplace, with the test ISR above for the test
 */
initialTstSWItr()
{
  int mask;
  extern void Fifo_SW_ISR();
  if (SwIsrChanged != 0)
       return 0;
  mask = get_mask(PFG,sw_int_status);
  printf("mask: 0x%lx\n",mask);
  deInstallSWItrs();
/*
  fpgaIntRemove( Fifo_SW_ISR, 1);
  fpgaIntRemove( Fifo_SW_ISR, 2);
  fpgaIntRemove( Fifo_SW_ISR, 3);
  fpgaIntRemove( Fifo_SW_ISR, 4);
*/
  fpgaIntConnect( Test_Fifo_SW_ISR, 1, FF_SW1_IRQ);
  fpgaIntConnect( Test_Fifo_SW_ISR, 2, FF_SW2_IRQ);
  fpgaIntConnect( Test_Fifo_SW_ISR, 3, FF_SW3_IRQ);
  fpgaIntConnect( Test_Fifo_SW_ISR, 4, FF_SW4_IRQ);
  cntrlFifoIntrpSetMask(mask);
  SwIsrChanged = 1;
  return 1;
}

/*
 *  Single call to cause each of the 4 software interrupts to occur
 */
pfgTstAllSwItrs()
{
    initialTstSWItr();
    pfgTstSW(1);
    taskDelay(calcSysClkTicks(500));  /* .50 s, taskDelay(30); */
    pfgTstSW(2);
    taskDelay(calcSysClkTicks(500));  /* .50 s, taskDelay(30); */
    pfgTstSW(3);
    taskDelay(calcSysClkTicks(500));  /* .50 s, taskDelay(30); */
    pfgTstSW(4);
    printf("Warning, replace shandler ITR with test ISR, reboot for proper operation after tests \n");
}

/*
 * test routine to cause the axis too short error interrupt to occurr
 * axis is the axis to program, 1=X,2=Y,3=Z, 4+XY&Z
 * ticks , in theory 2.4 usec is the trip point or 192 ticks
 * encode_PFGSetXAmp(W,clear,count,xamp)
 * W - write to instruction FIFO if = 1
 * clear - either clears the increment at the terminal count or allows it to continue to increament
 * clear = 1 clears the increment holding register when the 'count' reaches zero,
 * count > 0 then xamp value in the increment value not the base value, 
 * and the base value in incremented  by this value. 
 * So count is decrmented to zero and count words are put into the data fifo while the value is incremented.
 * When count reaches zero this is the terminal count so to speak.
 */
pfgTstTooShort(int axis,int ticks)
{
   long fifowords[10];
   long haltop;
   int num = 0;

    initialTstShortItr();

    set2pfgfifo();
    cntrlFifoReset();
    cntrlFifoCumulativeDurationClear();

    /* simple delay 1st */
    fifowords[num++] = encode_PFGSetDuration(1,8);

    /* encode_PFGSetXAmp(W,clear,count,xamp) */
    if ((axis == 1) || (axis > 3))
       fifowords[num++] = encode_PFGSetXAmp(1,0,0,100);
    else if ((axis == 2) || (axis > 3))
       fifowords[num++] = encode_PFGSetYAmp(1,0,0,100);
    else if ((axis == 3) || (axis > 3))
       fifowords[num++] = encode_PFGSetZAmp(1,0,0,100);
    else
       fifowords[num++] = encode_PFGSetXAmp(1,0,0,100);

    fifowords[num++] = encode_PFGSetDuration(1,ticks);  /* if less than 2.5us throw error */

    /* encode_PFGSetXAmp(W,clear,count,xamp) */
    if ((axis == 1) || (axis > 3))
       fifowords[num++] = encode_PFGSetXAmp(1,0,0,105);
    else if ((axis == 2) || (axis > 3))
       fifowords[num++] = encode_PFGSetYAmp(1,0,0,105);
    else if ((axis == 3) || (axis > 3))
       fifowords[num++] = encode_PFGSetZAmp(1,0,0,105);
    else
       fifowords[num++] = encode_PFGSetXAmp(1,0,0,105);


    /* fifowords[num++] = encode_PFGSetDuration(1,8); */
    fifowords[num++] = encode_PFGSetDuration(1,0);  /* haltop */
    cntrlFifoPIO((unsigned long*) fifowords, num);

   /* encode_PFGSetXAmpScale(W,clear,count,xamp_scale) */
    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    cntrlFifoStart();
    cntrlFifoWait4StopItrp();
}

/*
 * run through all the axes, for several tick values
 */
tstAllAxis()
{
    char *buf;
    int axis, ticks;
    for(axis=1; axis < 5; axis++)
    {
       if (axis==1)
         buf= "X";
       else if(axis ==2)
         buf= "Y";
       else if(axis ==3)
         buf= "Z";
       else if(axis >3)
         buf= "XYZ";
       else
         buf= "X";
      DPRINT(-1," --------------------======================================---------------\n");
         
       for(ticks=172; ticks > 168; ticks--)
       {
         DPRINT3(-1," ----- tstAllAxis: '%s', ticks: %d, %lf usec ---------------\n",buf,ticks, ((double)ticks * 12.5)/1000.0);
          pfgTstTooShort(axis,ticks);
         DPRINT(-1," -------======================================---------------\n");
         taskDelay(calcSysClkTicks(500));  /* .50 s, taskDelay(30); */
       }
      DPRINT(-1," --------------------======================================---------------\n");
    }
}

/*
 * install the test ISR for the too short interrupt tests.
 */
static void Test_Fifo_TooShort_ISR(int int_status, int val) 
{
  char *buffer;
  int acode;
   
   if (val == 1)
      buffer = "X";
   else if (val == 2)
      buffer = "Y";
   else if (val == 3)
      buffer = "Z";
   else
      buffer = "Unknown";
   

   DPRINT1(-1,"--------- ERROR ----- Time between PFG Axes setting TOO Short: Axis: '%s'\n",buffer);

   return;
}
/*
 * install the test ISR for the too short interrupt tests.
 */
initialTstShortItr()
{
  int mask;
  if (ShortIsrChanged != 0)
       return 0;

  deInstallPFGFailIsr();

  mask = get_mask(PFG,fifo_amp_duration_too_short_status);

  fpgaIntConnect( Test_Fifo_TooShort_ISR, 1, set_field_value(PFG,fifo_amp_duration_too_short_enable,0x1));
  fpgaIntConnect( Test_Fifo_TooShort_ISR, 2, set_field_value(PFG,fifo_amp_duration_too_short_enable,0x2));
  fpgaIntConnect( Test_Fifo_TooShort_ISR, 3, set_field_value(PFG,fifo_amp_duration_too_short_enable,0x4));

  cntrlFifoIntrpSetMask(mask);
  ShortIsrChanged = 1;
  return 1;
}

tstSwSerial(int value)
{
   /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); */
   setFifoOutputSelect(0);
   setSwXAmpValue(value);
   setSwYAmpValue(value);
   setSwZAmpValue(value);
}

tstpfgfailserialize(int safevalue)
{
   long fifowords[20];
   int num = 0;

    set2pfgfifo();
    cntrlFifoReset();
    cntrlFifoCumulativeDurationClear();
    setSafeXYZAmp(safevalue, safevalue, safevalue);
    resetSafeVals();

    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,1000000);
    fifowords[num++] = encode_PFGSetDuration(1,0);  /* haltop */
    cntrlFifoPIO((unsigned long*) fifowords, num);
    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    cntrlFifoStart();
    taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
    assertFailureLine();
}


tstBadOpCode(int opcode)
{
    int badopcode;
    badopcode = (opcode << 26);
    setFifoOutputSelect( 1 ); /* switch to FIFO control */
    cntrlFifoReset();
    cntrlClearInstrCountTotal();
    cntrlFifoCumulativeDurationClear();
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(badopcode);  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_PFGSetDuration(1,0));  /* 2usec */

    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    printf("fifo instructions reciev'd: %d\n",cntrlInstrCountTotal());
    cntrlFifoStart();
}




#endif
