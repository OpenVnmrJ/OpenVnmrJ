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
FILE: gradientTests.c 1.2 05/16/05
=========================================================================
PURPOSE:
	Provide a file for board bringup and other misc. test  routines 

Externally available modules in this file:

Internal support modules in this file:

COMMENTS:

AUTHOR:	

*/
#include <vxWorks.h>
#include <stdlib.h>
#include "nvhardware.h"
#include "gradient.h"
#include "gradient_fifo.h"

#define INCLUDE_TESTING
#ifdef INCLUDE_TESTING
/*  test routines */


struct GRADHOLD {
  int duration;
  int repduration;
  int gates;
  int xamp;
  int yamp;
  int zamp;
  int b0amp;
  int count;
  int xampscale;
  int yampscale;
  int zampscale;
  int b0ampscale;
  int xecc;
  int yecc;
  int zecc;
  int b0ecc;
  int shim;
  int user;
}  GRAD_state;
 
unsigned long totalduration;
extern unsigned long long cumduration;
extern unsigned long long fidduration;

#define WRITEKEY (1 << 31)
#define DURATIONKEY (1 << 26)
#define REPDURATIONKEY (17 << 26)
#define XAMPKEY (2 << 26)
#define YAMPKEY (3 << 26)
#define ZAMPKEY (4 << 26)
#define B0AMPKEY (5 << 26)
#define XAMPSCALEKEY (6 << 26)
#define YAMPSCALEKEY (7 << 26)
#define ZAMPSCALEKEY (8 << 26)
#define B0AMPSCALEKEY (9 << 26)
#define XECCKEY (10 << 26)
#define YECCKEY (11 << 26)
#define ZECCKEY (12 << 26)
#define B0ECCKEY (13 << 26)
#define SHIMKEY (14 << 26)
#define GATEKEY (15 << 26)
#define USERGATEKEY (16 << 26)

void fifoDecode(unsigned int word,int noprt)
{
   void graddecode(unsigned int word,int noprt);
   graddecode(word,noprt);
}

void
graddecode(unsigned int word,int noprt)
{
  int tmp,tdata,latched, tdur, trep;
  int tgate,tmask,tamp,tasmpscale, tcount, tclear;
  tmp = word;

   if (noprt != 1)
      printf(" 0x%8lx : ",word);
   if (tmp & WRITEKEY)
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
               GRAD_state.duration = tdata;  
               break;

       case REPDURATIONKEY:
            trep = (tdata >> 9)  & 0xFFFF;
            tdur = (tdata & 0x3FF);
   	    if (noprt != 1)
               printf("0x%8lx: rep %d, duration of %7.4f usec, total: %7.4f",tmp|tdata,trep, ((float) tdur)/80.0,
			((float) (trep * tdur))/80.0);
            
            GRAD_state.repduration = trep; /* ?? */
            GRAD_state.duration = tdur; /* ?? */
            break;

       case GATEKEY:
	    tgate = tdata & 0xFFF;
            tmask = (tdata >> 12) & 0xFFF;
   	    if (noprt != 1)
               printf("mask/gate set to %x/%x",tmask,tgate);
            GRAD_state.gates = tgate; 
            break;

       case XAMPKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("xamp clear: %d, count: %d, amp: %d",tclear,tcount,tamp);
            GRAD_state.xamp = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case YAMPKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("yamp clear: %d, count: %d, amp: %d",tclear,tcount,tamp);
            GRAD_state.yamp = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case ZAMPKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("zamp clear: %d, count: %d, amp: %d",tclear,tcount,tamp);
            GRAD_state.zamp = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case B0AMPKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("b0amp clear: %d, count: %d, b0amp: %d",tclear,tcount,tamp);
            GRAD_state.b0amp = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case XAMPSCALEKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("xampscale clear: %d, count: %d, ampscale: %d",tclear,tcount,tamp);
            GRAD_state.xampscale = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case YAMPSCALEKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
               printf("yampscale clear: %d, count: %d, ampscale: %d",tclear,tcount,tamp);
            GRAD_state.yampscale = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case ZAMPSCALEKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("zampscale clear: %d, count: %d, ampscale: %d",tclear,tcount,tamp);
            GRAD_state.zampscale = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case B0AMPSCALEKEY:
            tclear = (tdata >> 25) & 0x1;
            tcount = (tdata >> 16) & 0x1FF;
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("xampscale clear: %d, count: %d, ampscale: %d",tclear,tcount,tamp);
            GRAD_state.b0ampscale = tamp; 
            if (tcount != 0)
              GRAD_state.count = (tcount < GRAD_state.count) ? tcount: GRAD_state.count; 
            break;

       case XECCKEY:
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("xecc: %d",tamp);
            GRAD_state.xecc = tamp; 
            break;

       case YECCKEY:
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("yecc: %d",tamp);
            GRAD_state.yecc = tamp; 
            break;

       case ZECCKEY:
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("zecc: %d",tamp);
            GRAD_state.zecc = tamp; 
            break;

       case B0ECCKEY:
            tamp = tdata & 0xFFFF;
   	    if (noprt != 1)
                printf("b0ecc: %d",tamp);
            GRAD_state.b0ecc = tamp; 
            break;

       case SHIMKEY:
            tamp = tdata & 0xFFFFF;
   	    if (noprt != 1)
                printf("shim: %d",tamp);
            GRAD_state.shim = tamp; 
            break;

       case USERGATEKEY:
   	    if (noprt != 1)
               printf("user data = %x\n",(tdata&0xFFFF));
	    break;

       default:
            printf("Opcode NOT recognize!! 0x%8x\n",tmp);
            break;
    }
    if (latched)
    {
       if (noprt != 1)
          printf(" fifo word latched\n");
       if (noprt != 1)
       {
          printf("OUTPUT STATE of %9.4lf usec  GATE = %4x  ", \
                 ((float) GRAD_state.duration)/80.0,GRAD_state.gates);
       }

       if ( GRAD_state.repduration > 0)
       {
          /* if count > 0 and rep is less than coutn use rep duraction count otherwise use count */
          if ( (GRAD_state.count != 0) && (GRAD_state.repduration < GRAD_state.count) )
          {
             totalduration = totalduration + GRAD_state.duration * GRAD_state.repduration;
          }
          else
          {
             totalduration = totalduration + GRAD_state.duration * GRAD_state.count;
          }
          GRAD_state.repduration = 0;
       }
       else if (GRAD_state.count != 0)
       {
             totalduration = totalduration + GRAD_state.duration * GRAD_state.count;
       }
       else
       {
          totalduration = totalduration + GRAD_state.duration;
       }
	/*
       cumduration += (unsigned long long) totalduration;
       fidduration += (unsigned long long) totalduration;
	*/

       if (noprt != 1)
       {
           printf("XAMP/SCALE: %d/%d, YAMP/SCALE: %d/%d, ZAMP/SCALE: %d/%d, B0AMP/SCALE: %d/%d\n",
                    GRAD_state.xamp,GRAD_state.xampscale,
                    GRAD_state.yamp,GRAD_state.yampscale,
                    GRAD_state.zamp,GRAD_state.zampscale,
                    GRAD_state.b0amp,GRAD_state.b0ampscale);
           printf("XECC: %d, YECC: %d, ZECC: %d, B0ECC: %d, SHIM: %d\n",
                    GRAD_state.xecc, GRAD_state.yecc, GRAD_state.zecc,
                    GRAD_state.b0ecc,GRAD_state.shim);
        }
   }
   else 
   { 
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



static void AmpTooShort_ISR(int int_status, int errorcode) 
{
   logMsg("AmpTooShort_ISR:\n");
   logMsg("AmpTooShort_ISR: status: 0x%lx, errorcode: %d\n",int_status, errorcode, 0,0,0,0,0,0,0,0); 
   return;
}

#ifdef TOO_INTERRUPT_REMOVED_FROM_FPGA
/*====================================================================================*/
/*
 * enable all board specific failures interrupts
 */
initAmpItr()
{
  unsigned int failureMask,intMask;

  intMask =  get_mask(GRADIENT,fifo_amp_duration_too_short_status); 
  printf("amp_duration_too_short mask: 0x%lx\n",intMask);
  fpgaIntConnect( AmpTooShort_ISR, 42, intMask );
  cntrlFifoIntrpSetMask(intMask);   /* enable interrupts */

}

prtmasks()
{
  unsigned int failureMask,intMask;
    printf("fail mask: 0x%lx\n",get_mask(GRADIENT,fail_int_status));
    printf("warning mask: 0x%lx\n",get_mask(GRADIENT,warn_int_status));
    printf("amp_duration_too_short mask: 0x%lx\n",get_mask(GRADIENT,fifo_amp_duration_too_short_status));
    printf("ecc_duration_too_short mask: 0x%lx\n",get_mask(GRADIENT,fifo_ecc_duration_too_short_status));
    printf("slew limit exceeded mask: 0x%lx\n",get_mask(GRADIENT,slew_limit_exceeded_status));
    /* printf("duty limit exceeded mask: 0x%lx\n",get_mask(GRADIENT,duty_limit_exceeded_status)); */
    printf("spi fail : 0x%lx\n",get_mask(GRADIENT,spi_failed_busy_status));
}
#endif

tstDelaysTooShortItrs(int chan, int ticks)
{
   setFifoOutputSelect( 1 ); /* switch to FIFO control */
   set_field(GRADIENT,fifo_ecccalc_select,0);
   cntrlFifoPut(encode_GRADIENTSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
   cntrlFifoStart();
   cntrlFifoWait4StopItrp();
}
tstAmpTooShortItrs(int chan, int ticks)
{
   setFifoOutputSelect( 1 ); /* switch to FIFO control */
   set_field(GRADIENT,fifo_ecccalc_select,0);
   cntrlFifoPut(encode_GRADIENTSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   switch(chan)
   {
     case 0:
        cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,100));  
        cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
        cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,110)); 
	break;
     case 1:
        cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,100));  
        cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
        cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,110)); 
	break;
     case 2:
        cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,100));  
        cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
        cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,110)); 
	break;
     case 3:
        cntrlFifoPut(encode_GRADIENTSetB0Amp(1,0,0,100));  
        cntrlFifoPut(encode_GRADIENTSetDuration(1,ticks));  /* 2usec */
        cntrlFifoPut(encode_GRADIENTSetB0Amp(1,0,0,110)); 
	break;
    }
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0xffffff));  /* 2usec */
   cntrlFifoPut(encode_GRADIENTSetDuration(1,0));  /* halt */
   taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
   cntrlFifoStart();
   cntrlFifoWait4StopItrp();
}


void amp_too_short(int channel)
{
     int int_status;
     set_field(GRADIENT,fifo_ecccalc_select,0);
/*
     set_field(GRADIENT,fifo_ecccalc_decay,0);
     set_field(GRADIENT,dutylimit_disable,1);
     set_field(GRADIENT,slewlimit_disable,1);
*/
     set_field(GRADIENT,fifo_output_select,1);
     /* set_sw_controlled_values_to_zero(); */
     cntrlFifoPut(encode_GRADIENTSetDuration(0,0x020));            /* duration too short  */
     cntrlFifoPut(encode_GRADIENTSetXAmpScale(0,0,0,0x4000));      /* set xampscale to 1.0 with WB=0 */
     cntrlFifoPut(encode_GRADIENTSetYAmpScale(0,0,0,0x4000));      /* set yampscale to 1.0 with WB=0 */
     cntrlFifoPut(encode_GRADIENTSetZAmpScale(0,0,0,0x4000));      /* set zampscale to 1.0 with WB=0 */
     if (channel == 0)
     {
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x0000));
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetXAmp(1,0,0,0x0000));
     }
     else if (channel == 1)
     {
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x0000));
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetYAmp(1,0,0,0x0000));
     }
     else if (channel == 2)
     {
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x0000));
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x5555));   		
       cntrlFifoPut(encode_GRADIENTSetZAmp(1,0,0,0x0000));
    }
    cntrlFifoPut(encode_GRADIENTSetDuration(1,0));   /* haltop */
   taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
   cntrlFifoStart();
   cntrlFifoWait4StopItrp();
#ifdef XXXX
       taskDelay(calcSysClkTicks(830));  /* 830 ms, taskDelay(50); */
       int_status = get_field(GRADIENT,fifo_amp_duration_too_short_status);
       if (int_status == 0xf)
       {
           printf("fifo_amp_duration_too_short_status: 0x%x:\n",int_status);
           printf("fifo_amp_duration_too_short!\n");
           taskDelay(calcSysClkTicks(1660));  /* 1.66 s, taskDelay(100); */
	  /*	  clear_fifo_amp_duration_too_short_int();
           taskDelay(calcSysClkTicks(1660));  /* 1.66 s, taskDelay(100); */
           clear_holding_registers();*/
        }
#endif
}

tstBadOpCode(int opcode)
{
    int badopcode;
    badopcode = (opcode << 26);
    setFifoOutputSelect( 1 ); /* switch to FIFO control */
    cntrlFifoReset();
    cntrlClearInstrCountTotal();
    cntrlFifoCumulativeDurationClear();
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(badopcode);  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_GRADIENTSetDuration(1,0));  /* 2usec */

    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    printf("fifo instructions reciev'd: %d\n",cntrlInstrCountTotal());
    cntrlFifoStart();
}

#ifdef GRADIENT_CoarseXDelay
#define FOUR_US 320
#define MIN_DELAY 0  // minimum delay (course register)
#define DELAY_MOD 8  // modulation for fine delay
void setClearReg(int code)
{
	set_register(GRADIENT,ClearDelayBuffer,code);
}


void setXDelay(int ticks)
{
	//ticks=ticks<MIN_DELAY ? MIN_DELAY:ticks;
	int course=ticks/FOUR_US;
	int fine=ticks-FOUR_US*course;
	fine=fine/DELAY_MOD;
	set_register(GRADIENT,FineXDelay,fine*DELAY_MOD);
    set_register(GRADIENT,CoarseXDelay,course+MIN_DELAY);
}
void setXCourse(int ticks)
{
    set_register(GRADIENT,CoarseXDelay,ticks);
}
void setXFine(int ticks)
{
    set_register(GRADIENT,FineXDelay,ticks);
}
void showXDelay(){
	int course=get_register(GRADIENT,CoarseXDelay);
	int fine=get_register(GRADIENT,FineXDelay);
	printf("XDelay = %g us course = %d fine = %d\n",((course-MIN_DELAY)*FOUR_US+fine)/80.0,course,fine);
}

void setYDelay(int ticks)
{
	int course=ticks/FOUR_US;
	int fine=ticks-FOUR_US*course;
	fine=fine/DELAY_MOD;
	set_register(GRADIENT,FineYDelay,fine*DELAY_MOD);
    set_register(GRADIENT,CoarseYDelay,course+MIN_DELAY);
}
void setYCourse(int ticks)
{
    set_register(GRADIENT,CoarseYDelay,ticks);
}
void setYFine(int ticks)
{
    set_register(GRADIENT,FineYDelay,ticks);
}
void showYDelay(){
	int course=get_register(GRADIENT,CoarseYDelay);
	int fine=get_register(GRADIENT,FineYDelay);
	printf("YDelay = %g us course = %d fine = %d\n",((course-MIN_DELAY)*FOUR_US+fine)/80.0,course,fine);
}

void setZDelay(int ticks)
{
	int course=ticks/FOUR_US;
	int fine=ticks-FOUR_US*course;
	fine=fine/DELAY_MOD;
	set_register(GRADIENT,FineZDelay,fine*DELAY_MOD);
    set_register(GRADIENT,CoarseZDelay,course+MIN_DELAY);
}
void setZCourse(int ticks)
{
    set_register(GRADIENT,CoarseZDelay,ticks);
}
void setZFine(int ticks)
{
    set_register(GRADIENT,FineZDelay,ticks);
}
void showZDelay(){
	int course=get_register(GRADIENT,CoarseZDelay);
	int fine=get_register(GRADIENT,FineZDelay);
	printf("ZDelay = %g us course = %d fine = %d\n",((course-MIN_DELAY)*FOUR_US+fine)/80.0,course,fine);
}

void setB0Delay(int ticks)
{
	int course=ticks/FOUR_US;
	int fine=ticks-FOUR_US*course;
	fine=fine/DELAY_MOD;
    set_register(GRADIENT,CoarseB0Delay,course+MIN_DELAY);
	set_register(GRADIENT,FineB0Delay,fine*DELAY_MOD);
}

void showB0Delay(){
	int course=get_register(GRADIENT,CoarseB0Delay);
	int fine=get_register(GRADIENT,FineB0Delay);
	printf("B0Delay = %g us course = %d fine = %d\n",((course-MIN_DELAY)*FOUR_US+fine)/80.0,course,fine);
}

void showDelays(){
	showXDelay();
	showYDelay();
	showZDelay();
	showB0Delay();
}

void resetClearReg(){
	setClearReg(0xf);
	setClearReg(0x0);
}
void resetDelays(){
	setXDelay(0);
	setYDelay(0);
	setZDelay(0);
	setB0Delay(0);
}
#endif

#endif
