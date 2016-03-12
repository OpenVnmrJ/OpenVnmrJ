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
 * DESCRIPTION
 *
 *   SPI specific routines
 *
*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <stdio.h>
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <wdLib.h>
#include <ioLib.h>
 
#include "logMsgLib.h"
#include "nvhardware.h"
#include "master.h"
#include "masterSPI.h"
#include "nsr.h"

/* controller specific interrupt register pointers  */
extern volatile unsigned int  *pInterruptStatus, *pInterruptEnable;

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

VPSYN(MASTER_SPIStart0);
VPSYN(MASTER_SPIData0);
VPSYN(MASTER_SPIBusy0);
VPSYN(MASTER_SPIReadData0);
VPSYN(MASTER_SPIConfig0);

VPSYN(MASTER_SPIStart1);
VPSYN(MASTER_SPIData1);
VPSYN(MASTER_SPIBusy1);
VPSYN(MASTER_SPIReadData1);
VPSYN(MASTER_SPIConfig1);

VPSYN(MASTER_SPIStart2);
VPSYN(MASTER_SPIData2);
VPSYN(MASTER_SPIBusy2);
VPSYN(MASTER_SPIReadData2);
VPSYN(MASTER_SPIConfig2);

#define SCLK_FREQ_KHZ	80000   /* 80 MHz in KHz */

struct _SPI_config SPI_config[NUM_SPI] = {
	{   1000, SPI_24_BITS | SPI_RISING_EDGE | SPI_RESTING_HI },
	{   1000, SPI_RESTING_HI },
	{   1000, SPI_24_BITS },
};
/**************************************************************************
 * calc the SPI divisor necessary for the request SPI clock frequency
 * Minimum and Maximum SPI Freq of 1 MHz and 20 MHz are inforced
 *
 * divisor is 80 MHz clock divided by the request SPI clock frequency 
 * i.e. divisor for a 1 MHz SPI = ( 80000 KHz / 1000 KHz)  = 80 
 *
 *    Author Greg Brissey 5/20/2004
 */
int calcSPIdivisor(int freqKHz)
{
   /* divisor is 80 MHz clock divided by the request SPI clock frequency */
   /* i.e. divisor for a 1 MHz SPI = ( 80000 KHz / 1000 KHz)  = 80 */

   /* test request frequency to be a valid range */
   if (freqKHz < 1000)   /* minimum 1 MHz */
      freqKHz = 1000;
   if (freqKHz > 20000)  /* Maximum 20 MHz */
      freqKHz = 20000;
   return ( SCLK_FREQ_KHZ / freqKHz );
}

setSPIrate(int chan,int rate)
{
void initSPI();
  SPI_config[chan].rate = rate;
  initSPI();
}

setSPIbits(int chan, int bits)
{
void initSPI();
   SPI_config[chan].bits = bits;
   initSPI();
}

/**************************************************************************
 * the configuration registerSPIConfig[0-2] have the following bit fields:
 * 0-6 spi-clk_rate-divisor
 *  7  SPI data size,       0=16-bits, 1=24-bits
 *  8  SPI data setup edge, 0=falling, 1=rising
 *  9  SPI clk resting,     0=low,     1=high
 */
void initSPI() 
{
int config;

  *pMASTER_SPIConfig0 = calcSPIdivisor(SPI_config[0].rate) | SPI_config[0].bits;

  *pMASTER_SPIConfig1 = calcSPIdivisor(SPI_config[1].rate) | SPI_config[1].bits;

  *pMASTER_SPIConfig2 = calcSPIdivisor(SPI_config[2].rate) | SPI_config[2].bits;

  /* later on we'll init the ISR as well */
} 

/**************************************************************************
 * waitOnSpiBusy() - convenience routine to wait on until the SPI
 *                   busy register shows thransfer is complete.
 *     given the busy register for the SPI device to be checked.
 *
 *   Author: Greg Brissey  5/20/04
 */
waitOnSpiBusy( volatile int *busyreg )
{
int tries = 0;
    while( *busyreg )
    {
        DPRINT(3,"SPI busy\n");
        tries++;
        if (tries > 100000) { DPRINT(-1, "spi timeout\n"); break; }
    }
}

dumpmasspi()
{
    printf("dataword: 0x%lx\n",*pMASTER_SPIData2);
    printf("data: 0x%lx\n",get_field(MASTER,spi_data2));
    printf("divisor: 0x%x\n",get_field(MASTER,spi_clk_rate_divisor2));
    printf("readback: 0x%x\n",get_field(MASTER,spi_read2));
    printf("busy: %d\n",get_field(MASTER,spi_busy2));
}

int hsspi (int chan, unsigned int dac_value)
{
unsigned int spidata,datasent;

   DPRINT1(3,"hsspi(): chan=%x\n",chan);
   spidata = dac_value; /* set_field_value(MASTER,spi_data,dac_value); */
 
   /* chip is active low so the bit in the spidata must be low, but this
   /* already accomplished by setting the spidata equal the the dac value
   /* */

   switch (chan)
   {
   case 0:
      *pMASTER_SPIData0 = spidata;	/* fill data register */
      *pMASTER_SPIStart0 = 0;		/* start is on rising edge */
      *pMASTER_SPIStart0 = 1;
      waitOnSpiBusy(pMASTER_SPIBusy0);
      datasent = *pMASTER_SPIReadData0;	/* read the returned data */
      break;
   case 1: 
      *pMASTER_SPIData1 = spidata;
      *pMASTER_SPIStart1 = 0;
      *pMASTER_SPIStart1 = 1;
      waitOnSpiBusy(pMASTER_SPIBusy1);
      datasent = *pMASTER_SPIReadData1;
      break;
   case 2: 
      *pMASTER_SPIData2 = spidata;
      *pMASTER_SPIStart2 = 0;
      *pMASTER_SPIStart2 = 1;
      waitOnSpiBusy(pMASTER_SPIBusy2);
      datasent = *pMASTER_SPIReadData2;
      break;
   default:
      DPRINT1( 1,"SPI: illegal channel is %d\n",chan);
      return(-1);
      break;
   }
   DPRINT2( 3,"sent: 0x%lx, received: 0x%lx\n",dac_value,datasent);
   return(datasent);
}

void spew (int whichone, int dac_value, int freq)
{
unsigned int spidata,datasent;
int try; 
    /* int dac_value = 0xffff; */
    /* set clock divisor to ?? */
    /* 0x7f = 626 KHz */
    /* 0x6f = 710 KHz */
    /* 0x5f = 836 KHz  */
    /* 0x4f = 1.02 MHz */
    /* 0x3f = 1.29 MHz */
    /* 0x2f = 1.70 MHz */
    /* 0x1f = 2.51 MHz */
    /* 0x0f = 5.18 MHz */
    /* 0x07 = 11.4 MHz */
    /* 0x06 = 15.5 MHz */
    /* 0x03 = 25.5 MHz */

   if ( (whichone < 0) || (whichone > 2) )
      DPRINT(-1, "0-hs SPI chan 0, 1-hs SPI chan 1, 2- MAS SPI\n");

   initSPI(freq,freq,freq);
   while(1)
   {
      hsspi (whichone, dac_value);
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
   }
}

/*
 * forceBusyError( hs spi channel 0 or 1)
 * test routine to force the busy fail spi interrupt
 * be sure to install the base ISR via initFpgaBaseISR() call
 * then  call initSpiItrs() to setup the dpis ISR
 *
 * then call forceBusyError 0, or forceBusyError 1 to test the two hs interrupts
 *
 *  Author: Greg Brissey   5/24/04
 */
forceBusyError(int chan)
{
//    setHsSpiFreq(chan, 10000);  /* SPI clocking to 1MHz */
    if (chan == 0)
    {
       *pMASTER_SPIData0 = 0x5555;

       /* start SPI */
       *pMASTER_SPIStart0 = 0;
       *pMASTER_SPIStart0 = 1;

       /* send on while SPI is busy, this should throw in busy interrupt */
       *pMASTER_SPIData0 = 0xEEEE;

       /* start SPI */
       *pMASTER_SPIStart0 = 0;
       *pMASTER_SPIStart0 = 1;

    }
    else
    {
       *pMASTER_SPIData1 = 0x5555;

       /* start SPI */
       *pMASTER_SPIStart1 = 0;
       *pMASTER_SPIStart1 = 1;

       /* send on while SPI is busy, this should throw in busy interrupt */
       *pMASTER_SPIData1 = 0xEEEE;

       /* start SPI */
       *pMASTER_SPIStart1 = 0;
       *pMASTER_SPIStart1 = 1;

    }
}

/*
 * SPI interrupt service routine
 *
 *   Just reports SPI interrupt, doesn't do any useful work yet.
 *
 *    Author: greg Brissey 5/24/04
 */
SPI_ISR(int int_status, int arg2)
{
    int spiItrs = mask_value(MASTER,spi_failed_busy0_status,int_status) |
                  mask_value(MASTER,spi_failed_busy1_status,int_status) |
		  mask_value(MASTER,spi_failed_busy2_status,int_status);

    logMsg("SPI_ISR: pending: 0x%lx, spi: 0x%x\n",int_status,spiItrs,3,4,5,6);
 
    /* No UARTs need servicing then just return */
    if (spiItrs == 0)
        return;

    return;
}

enableSpiItrs()
{
  set_field(MASTER,spi_failed_busy0_enable,1);
  set_field(MASTER,spi_failed_busy1_enable,1);
  set_field(MASTER,spi_failed_busy2_enable,1);
  set_field(MASTER,spi_failed_busy0_clear,0);
  set_field(MASTER,spi_failed_busy0_clear,1);
  set_field(MASTER,spi_failed_busy1_clear,0);
  set_field(MASTER,spi_failed_busy1_clear,1);
  set_field(MASTER,spi_failed_busy2_clear,0);
  set_field(MASTER,spi_failed_busy2_clear,1);
}
initSpiItrs()
{
     fpgaIntConnect(SPI_ISR,0);
     enableSpiItrs();
}

static void enableProbeTuneIsr(int);
static void probeISR(int);
static void tuneISR(int);
nsrtest() 
{
int intEnabled=0;
int answer;
unsigned int rate=1;
int done;
   while (1) {
      if ( ! intEnabled)  {
         printf("1 Re-initalize SPI\t\t6 Enable Probe/Tune interrupt(s)\n");
      }
      else {
         printf("1 Re-initalize SPI\t\t6 Disable Probe/Tune interrupt(s)\n");
      }
      printf("2 Set SPI bit rate\t\t7 Show IntStat and IntEnable\n");
      if (SPI_config[1].bits & SPI_RISING_EDGE) {
         printf("3 Set data valid on falling edge\t\t8 Show revision\n");
      }
      else {
         printf("3 Set data valid on rising edge\t\t8 Show revision\n");
      }
      if (SPI_config[1].bits & SPI_RESTING_HI) {
         printf("4 Set resting level to low\n");
      }
      else {
         printf("4 Set resting level to high\n");
      }
      printf("5 Send value (hex)\n");
      printf("0 exit\n");
      printf("\nEnter selection: ");
      answer = getchar() - '0';
      printf("\n\n");
      switch (answer) {
      case 0:
           return;
           break;
      case 1:
           break;
      case 2: 
           printf("enter bit-rate in MHz: ");
 	   scanf("%d", &rate);
           printf("\n\nNew rate is %d MHz\n",rate);
	   SPI_config[1].rate = rate * 1000;
	   break;
      case 3:
           SPI_config[1].bits ^= SPI_RISING_EDGE;
	   break;
      case 4:
           SPI_config[1].bits ^= SPI_RESTING_HI;
	   break;
      case 5:
           printf("Enter hex value to send: ");
           scanf("%x",&rate);
           printf("Sending %x\n",rate);
           rate = hsspi(1,rate);
           printf("Read from SPI port: '%x'\n",rate);
           break;
      case 6:
	   enableProbeTuneIsr(intEnabled);
           intEnabled ^= 1;
           break;
      case 7:
           while(1) {
              printf("IntStat = %x IntEnbl = %x \n",*pInterruptStatus,*pInterruptEnable);
              taskDelay(calcSysClkTicks(100));  /* taskDelay(6); */
           }
           break;
      case 8: 
           rate = hsspi(1,0xf800);
           printf("CPLD version is %d (0x%x)\n", rate,rate);

      default:
           printf("Bad request\n");
	   break;
      }
      initSPI(1,1,1);
      getchar();
   }
}

int nsrdump()
{
   int i,j,k;
   printf("reg value\n");
   for (i=0; i < 8; i++)
   {
       k = 0x8400 | i << 11;
       j = hsspi(1,k);
       printf(" %2d  %2x\n",i,j);
   }
   return(j);
}

static void enableProbeTuneIsr(int enabled)
{

   if (enabled) {
      printf("Removing Probe/Tune from interrupt service list\n");
      fpgaIntRemove(probeISR,0);
      set_field(MASTER,probe_id_int_enable,0);
      fpgaIntRemove(tuneISR,0);
      set_field(MASTER,tune_int_enable,0);
   }
   else {
      printf("Connecting Probe/Tune to interrupt service list\n");
      hsspi(1,0x4801);
      hsspi(1,0x4800);
      fpgaIntConnect(probeISR,0,1<<MASTER_probe_id_int_enable_pos);
      set_field(MASTER,probe_id_int_enable,1);
      set_field(MASTER,probe_id_int_clear,0);
      set_field(MASTER,probe_id_int_clear,1);
      fpgaIntConnect(tuneISR, 0,1<<MASTER_tune_int_enable_pos);
      set_field(MASTER,tune_int_enable,1);
      set_field(MASTER,tune_int_clear,0);
      set_field(MASTER,tune_int_clear,1);
   }
}

static void probeISR(int zero)
{
unsigned int value;
   logMsg("probeISR interrupt: parameter %d\n", zero,2,3,4,5,6);
   hsspi(1,0x4801);
   hsspi(1,0x4800);
   value = hsspi(1,0xc800);
   DPRINT2(-1,"Cleared NSR ISR, status=%d (0x%x)\n",value,value);
}


static void tuneISR(int zero)
{
unsigned int channel, atten;
   if (DebugLevel > 0)
      logMsg("tuneISR interrupt: parameter %d\n", zero,2,3,4,5,6);
   channel = hsspi(1,0x8000);
   atten   = channel;
   channel &= 0xf;             // lower 4 bits only
   atten    = (atten>>4) & 0xf;// upper 4 bits only
   DPRINT2(-1,"Channel=%d (0x%x)\n",channel, channel);
   DPRINT2(-1,"Atten=%d (0x%x)\n",atten, atten);
   hsspi(1,0x4801);
   hsspi(1,0x4800);
   channel = hsspi(1,0xc800);
   DPRINT2(-1,"Cleared NSR ISR, status=%d (0x%x)\n",channel,channel);
}

/* select nsr # 1 - implicit in leading zeros*/
unsigned int NSR_words[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/* this is a TEST Routine */
void sendNSR()
{
  int i;
  for (i=2; i < 7; i++)
    hsspi(1,NSR_words[i]); 
  for (i=8; i < 15; i++)
    hsspi(1,NSR_words[i]); 
}

void sendWord7()
{   hsspi(1,NSR_words[7]); }


int bitFieldActive(int top, int bot)
{
  int k,l;
  l = (1 << (top - bot + 1))-1;
  return(l << bot);
}

int programNSRBitField(int word, int top, int bot, int value)
{
  int active, mask, data;
  int *p2Word;
  p2Word = &(NSR_words[word]);
  active = bitFieldActive(top,bot);
  mask = ~active; 
  data = (value << bot) & active; 
  /* encode the word field and use to set contents */
  *p2Word = (*p2Word & mask) | data;
  /* now add the word id */
  *p2Word = (*p2Word & 0x07ff) | (word << 11);
}

setPreAmpSelect(int pat)
{     programNSRBitField(2,3,0,pat); }

setTRSelect(int pat)
{    programNSRBitField(2,7,4,pat);  }
/* coil bias and presig */

setNarrowBand(int pat)
{    programNSRBitField(1,7,0,pat); }

setTuneModeSelect(int pat)
{    programNSRBitField(8,0,0,pat); }

/* MIXER START AT 0 HERE */
setMixerHiLow(int mixer, int pat)
{    programNSRBitField(mixer+3,5,5,pat); }

setMixerGain(int mixer, int pat)
{    programNSRBitField(mixer+3,4,0,pat/2); }

setMixerInput(int mixer, int pat)
{    programNSRBitField(mixer+3,7,6,pat); }

setLockAtten(int pat)
{    programNSRBitField(13,0,0,pat); }

setLockHiLow(int pat)
{    programNSRBitField(12,0,0,pat); }

setDiplexRelay(int pat)
{    programNSRBitField(14,1,0,pat); }


setGain(int gain)
{
  printf("gain input user - output packed\n");
  setMixerGain(0,gain); 
  hsspi(1,NSR_words[3]);
}

void setNSR4Observe(int channel, int gain, int hilow)
{
  int i;
  setPreAmpSelect(1<<channel);
  setTRSelect(1<<channel);
  setMixerHiLow(0,hilow);  /* real mixer */
  setMixerGain(0,gain);
  setLockAtten(0);
  /* convert via channel + 1 to user space */
  switch (channel+1)
  {
  case 1: 
       setMixerInput(0,0); 
       break;
  case 2: 
       setMixerInput(0,1); 
       break;
       /* use 6 as 2H lock */
  case 6: 
       setMixerInput(0,2); 

       break; 
  case 3:
       setMixerInput(0,3); 
       setMixerHiLow(1,0); 
       break;
  case 4:
       setMixerInput(0,3); 
       setMixerHiLow(1,1);  
       setMixerInput(1,3);
       setMixerHiLow(2,0);  /* other don't care */
       break;
  default:
    printf("dont know how to observe channel %d\n",channel); return;
  }
  for (i=2; i < 7; i++)
    hsspi(1,NSR_words[i]); 
}

/*
**  the aux additions "miror" the base NSR control for multiple receivers 
**  or microimaging. The hard coded numbers match the tune selector settings
*/
void setNSR4Tune(int channel, int gain, int hilow)
{
  int i;
  int aux1,aux2;  // aux words are hard coded - 
DPRINT1( 1,"setNSR4Tune(): aux extended hilow=%d\n",hilow);
  setPreAmpSelect(0);	    /* turn off the preamp's */
  setTRSelect(0);	    /* all in T */
  /* printf("TUNE entry hi low flag is %d\n",hilow); */
  if (hilow == 1) hilow = 0; else hilow = 1;
  /* printf("exit hi low flag is %d\n",hilow); */
  setMixerHiLow(0,hilow);   /* pick correct band and set gain */
  setMixerGain(0,gain);
  setMixerInput(0,3);	    /* point to tune */
  set2sw_fifo(0);           /* bypass fifo */
  aux1 = 0x8001820;         /* aux1 is mixer 1 on aux hi lo */
  aux2 = 0x8002020;         /* aux2 is mixer 2 on aux hi lo */
  switch (channel)
  {
  case 1: 
       auxWriteReg(6,0);    /* LO select */
       setMixerInput(1,0);  /* port 1 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       /* all aux's ok */
       break;
  case 2: 
       auxWriteReg(6,4);    /* LO select */
       setMixerInput(1,1);  /* port 2 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       aux1 |= 0x40;
       break;
  case 3:
       auxWriteReg(6,7);    /* LO select */
       setMixerInput(1,2);  /* port 3 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       aux1 |= 0x80;
       break;
  case 4:
       auxWriteReg(6,5);    /* LO select */
       setMixerInput(1,3);  /* 2nd tune port 4 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       setMixerInput(2,0);  /* 2nd tune selector - port 1 */
       setMixerHiLow(2,1);  /* listen to tune ports */
       aux1 |= 0xc0;        /* aux: next port... aux 2 is default */
       break;
     
  case 5:
       auxWriteReg(6,6);    /* LO select */
       setMixerInput(1,3);  /* 2nd tune port 4 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       setMixerInput(2,1);  /* 2nd tune selector - port 2 */
       setMixerHiLow(2,1);  /* listen to tune ports */
       aux1 |= 0xc0;        /* aux: next port. */
       aux2 |= 0x40;        /* aux: mixer 1 */
       break; 
       // test for lock - prototype...
       // use LO from channel 2 ...
       // listen on 2nd t.s. port 4 
       // need to notify rf2 that he's also 9 or something.
  case 9:
       auxWriteReg(6,4);    /* LO select */
       setMixerInput(1,3);  /* 2nd tune port 4 */
       setMixerHiLow(1,1);  /* listen to tune ports */
       setMixerInput(2,3);  /* 2nd tune selector - port 4 */
       setMixerHiLow(2,1);  /* listen to tune ports */
       aux1 |= 0xc0;        /* aux: next port. */
       aux2 |= 0xc0;        /* aux: mixer 1 */
       break;
  default:
    DPRINT1(-1,"dont know how to tune channel %d\n",channel);
  }
  setTuneModeSelect(1);
  for (i=2; i < 7; i++)
    hsspi(1,NSR_words[i]); 
  /* hack to fix aux NSR */
  hsspi(1,0x8001000);
  hsspi(1,aux1);
  hsspi(1,aux2);
}

