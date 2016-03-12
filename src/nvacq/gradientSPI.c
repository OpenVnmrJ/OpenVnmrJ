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
 * Gradient SPI specific routines
 *
*/
/*
* Here is some info from the GDriver CPLD Spec:
* (The gradient controller goes to the GIF which
* goes to the GDriver CPLD) (updated 5/11/05 FV)
* 
* Chip Select:
*     0 0 0 0 :        never unused
*     0 0 0 1 :        X Attenuation DAC
*     0 0 1 0 :        X Shim DAC
*     0 0 1 1 :        Y Attenuation DAC
*     0 1 0 0 :        Y Shim DAC
*     0 1 0 1 :        Z Attenuation DAC
*     0 1 1 0 :        Z Shim DAC
*     0 1 1 1 :        Set Control Register for Shim and ECC Scaling (see below)
*     1 0 0 0 :        Read X Shim
*     1 0 0 1 :        Read Y Shim
*     1 0 1 0 :        Read Z Shim
*     1 0 1 1 :        Readback Config reg, PS status, Board ID
*     1 x x x :        unused presently
*     1 1 1 1 :        never unused
*  
* ReadBack Information: 16 bits
* 
*        0:      power_supply_ok                     (ok = 1)
*      1-4:    config/version information
* 
*     5-15:   unused
* 
* 14 bits data for Shim and ECC scale settings (Chip Select == 0 1 1 1):
*        0-1:    X ECC
*        2-3:    X Shim
*        4-5:    Y ECC
*        6-7:    Y Shim
*        8-9:    Z ECC
*        10-11:  Z Shim
*        12-13:  B0 ECC
*   there are two bits for each scaling,
*    which therefore gives us 4 scaling settings.
*    0,1,2,3 
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
#include "gradient.h"
#include "gradientSPI.h"

/* controller specific interrupt register pointers  */
/* extern volatile unsigned int  *pInterruptStatus, *pInterruptEnable; */

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)
#define PSIN(arg)  signed int *p##arg = (signed int *) (FPGA_BASE_ADR + arg)
#define PSLLN(arg) signed long long *p##arg = (signed long long *) (FPGA_BASE_ADR + arg)

VPSYN(GRADIENT_SPIStart);
VPSYN(GRADIENT_SPIDataOut);
VPSYN(GRADIENT_SPIDataIn);
VPSYN(GRADIENT_SPIBusy);
VPSYN(GRADIENT_SPIChipSelect);
VPSYN(GRADIENT_SPIConfig);

VPSYN(GRADIENT_XSlewLimit);
VPSYN(GRADIENT_YSlewLimit);
VPSYN(GRADIENT_ZSlewLimit);
VPSYN(GRADIENT_B0SlewLimit);

/*  duty cycle removed from FPGA */
/*
 * VPSYN(GRADIENT_XDutyCycleLimit);
 * VPSYN(GRADIENT_YDutyCycleLimit);
 * VPSYN(GRADIENT_ZDutyCycleLimit);
 * VPSYN(GRADIENT_B0DutyCycleLimit);
*/

PSIN(GRADIENT_ECCAmp);
PSLLN(GRADIENT_ECCTimeConstant);


#define SCLK_FREQ_KHZ	80000   /* 80 MHz in KHz */

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

/**************************************************************************
 * the configuration registerSPIConfig[0-2] have the following bit fields:
 * 0-6 spi-clk_rate-divisor
 *  7  SPI data size,       0=16-bits, 1=24-bits
 *  8  SPI data setup edge, 0=falling, 1=rising
 *  9  SPI clk resting,     0=low,     1=high
 */
void initSPI() 
{

  set_field(GRADIENT,spi_clk_rate_divisor,12);   /* 6.666 MHz */
  set_field(GRADIENT,spi_clk_polarity,1);
  set_field(GRADIENT,spi_clk_resting,0);
  set_field(GRADIENT,spi_data_size,0);		/* 16 bits */
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
        DPRINT(1,"SPI busy\n");
        tries++;
        if (tries > 100000) { DPRINT(-1, "spi timeout\n"); break; }
    }
}

/*
 * setspi - chip_select, data_value
 * send data via the SPI to the select device via chip_select
 *
 * chip_select: 0x1 - X Atten, 0x3 - Y Atten, 0x5 - Z Atten
 *              0x7 - Ecc Scaling
 *
 * Author:  Greg Brissey     4/7/05
 */
int setspi(int chip_select,int data_value)
{
DPRINT2(-1,"setspi %d %d\n", chip_select, data_value);
  *pGRADIENT_SPIChipSelect = chip_select;	
  *pGRADIENT_SPIDataOut = data_value;	/* fill data register */
  *pGRADIENT_SPIStart = 0;		/* start is on rising edge */
  *pGRADIENT_SPIStart = 1;
  waitOnSpiBusy(pGRADIENT_SPIBusy);
  return ( (*pGRADIENT_SPIDataIn & 0xFF) );
}

dumpspireg()
{
    printf("dataword: 0x%lx\n",*pGRADIENT_SPIDataOut);
    printf("data: 0x%lx\n",get_field(GRADIENT,spi_data_out));
    printf("divisor: 0x%x\n",get_field(GRADIENT,spi_clk_rate_divisor));
    printf("readback: 0x%x\n",get_field(GRADIENT,spi_data_in));
    printf("busy: %d\n",get_field(GRADIENT,spi_busy));
}

/*
 *  set the X,Y,Z Axis Attenuation
 *
 * Author:  Greg Brissey   4/7/05
 */
void setAxisAtten(int channel, int value)
{
  int chip_select;

  if      (channel == 0) chip_select = 0x01;	/* x attenuation */
  else if (channel == 1) chip_select = 0x03;	/* y attenuation */
  else if (channel == 2) chip_select = 0x05;	/* z attenuation */

  *pGRADIENT_SPIChipSelect = chip_select;	
  *pGRADIENT_SPIDataOut = value;	/* fill data register */
  *pGRADIENT_SPIStart = 0;		/* start is on rising edge */
  *pGRADIENT_SPIStart = 1;
  waitOnSpiBusy(pGRADIENT_SPIBusy);
}

void setEccScaling(int value)
{
  *pGRADIENT_SPIChipSelect = 0x7;
  *pGRADIENT_SPIDataOut = value;	/* fill data register */
  *pGRADIENT_SPIStart = 0;		/* start is on rising edge */
  *pGRADIENT_SPIStart = 1;
  waitOnSpiBusy(pGRADIENT_SPIBusy);
}


/*
 *  Zero both the ECC Amp and ECC TimeConstant terms
 *
 */
void ZeroEccRam()
{
    signed int *amp = pGRADIENT_ECCAmp; 
    signed long long *tc = pGRADIENT_ECCTimeConstant;
/*
    signed int *amp = (signed int *) (GRADIENT_BASE + GRADIENT_eccamp_addr);  
    signed long long *tc = (signed long long *)(GRADIENT_BASE + GRADIENT_ecctc_addr);  
*/
    int i;
    for(i=0;i<52;i++)
      {
       amp[i] = 0x0000;
       tc[i] = 0x0000000000000000LL;
      }
}

/*
* 52 ECC terms - amplitudes and time constants
* in order:
* 
*     6 main terms (X to X) 
*     6 cross terms (2 sets of 3 - X to Y, X to Z) 4 B0 terms (X to B0)
* 
*     6 main terms (Y to Y) 
*     6 cross terms (2 sets of 3 - Y to Z, Y to X) 4 B0 terms (Y to B0)
* 
*     6 main terms (Z to Z) 
*     6 cross terms (2 sets of 3 - Z to X, Z to Y) 4 B0 terms (Z to B0)
* 
*     4 B0 terms (B0 to B0)
* 
*/
void loadEccAmpTerms(signed int *eccAmpTermsArray)
{
    signed int *amp = pGRADIENT_ECCAmp; 
    int i;
    for(i=0;i<52;i++)
    {
       amp[i] = eccAmpTermsArray[i];
DPRINT2( 2,"amp[%2d]=%d\n",i,amp[i]);
    }
}

void loadEccTimeConstTerms(signed long long *eccTCTermsArray)
{
    signed long long *tc = pGRADIENT_ECCTimeConstant;
    int i;
    for(i=0;i<52;i++)
    {
       tc[i] = eccTCTermsArray[i];
DPRINT2( 2,"tc[%2d]=%lld\n",i,tc[i]);
    }
}

void set_ecc_decay()
{
    set_field(GRADIENT,fifo_ecccalc_select,0x1);
    set_field(GRADIENT,fifo_ecccalc_decay,0x1); 
}

loadSdacScaleNLimits(int *values)
{
DPRINT1(2," 1 val=%d=n",*values);
   setspi(7, *values++);	// 7 is address for scale
DPRINT1(2," 2 val=%d=n",*values);
   setspi(1, *values++);	// 1 is address for X1
DPRINT1(2," 3 val=%d=n",*values);
   setspi(3, *values++);	// 3 is address for Y1
DPRINT1(2," 4 val=%d=n",*values);
   setspi(5, *values++);	// 5 is address for Z1

DPRINT1(2," 5 val=%d=n",*values);
   *pGRADIENT_XSlewLimit = *values++;
DPRINT1(2," 6 val=%d=n",*values);
   *pGRADIENT_YSlewLimit = *values++;
DPRINT1(2," 7 val=%d=n",*values);
   *pGRADIENT_ZSlewLimit = *values++;
DPRINT1(2," 8 val=%d=n",*values);
   *pGRADIENT_B0SlewLimit = *values++;

/* DPRINT1(2," 9 val=%d=n",*values);
/*    *pGRADIENT_XDutyCycleLimit = *values++;
/* DPRINT1(2,"10 val=%d=n",*values);
/*    *pGRADIENT_YDutyCycleLimit = *values++;
/* DPRINT1(2,"11 val=%d=n",*values);
/*    *pGRADIENT_ZDutyCycleLimit = *values++;
/* DPRINT1(2,"12 val=%d=n",*values);
/*    *pGRADIENT_B0DutyCycleLimit = *values++;
*/
}

void spew (int chip_select, int dac_value, int freq)
{
  unsigned int spidata,datasent;
  int div,try; 
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

   div = calcSPIdivisor(freq);
   printf("divisor: %d\n",div);
   set_field(GRADIENT,spi_clk_rate_divisor,div);   /* 6.666 MHz */
   while(1)
   {
      setspi (chip_select, dac_value);
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
forceBusyError()
{
   *pGRADIENT_SPIChipSelect = 1;	 /* X Axis Atten */
   *pGRADIENT_SPIDataOut = 0x5555;	/* fill data register */
   *pGRADIENT_SPIStart = 0;		/* start is on rising edge */
   *pGRADIENT_SPIStart = 1;

   /* send on while SPI is busy, this should throw in busy interrupt */
   *pGRADIENT_SPIChipSelect = 3;	 /* Y Axis Atten */
   *pGRADIENT_SPIDataOut = 0x5555;	/* fill data register */
   *pGRADIENT_SPIStart = 0;		/* start is on rising edge */
   *pGRADIENT_SPIStart = 1;

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
    int spiItrs = mask_value(GRADIENT,spi_failed_busy_status,int_status);

    logMsg("SPI_ISR: pending: 0x%lx, spi: 0x%x\n",int_status,spiItrs,3,4,5,6);
 
    return;
}

enableSpiItrs()
{
  set_field(GRADIENT,spi_failed_busy_enable,1);
  set_field(GRADIENT,spi_failed_busy_clear,0);
  set_field(GRADIENT,spi_failed_busy_clear,1);
}

initSpiItrs()
{
DPRINT(-2,"enabling gradient SPI overrun interrupt\n");
     fpgaIntConnect(SPI_ISR,0);
     enableSpiItrs();
}

