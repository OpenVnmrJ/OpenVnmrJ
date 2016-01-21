// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
#include <standard.h>

#define	APBOUT		6
#define	HB_XMT_SEL	0x18

#define APB_SELECT      24576   /* A000, if used with minus sign */
#define APB_WRITE       20480   /* B000, if used with minus sign */
#define APB_INRWR       28672   /* 9000, if used with minus sign */

#define RF_ADDR         0xa00   /* apbus address for al rf boards */

#define FACTOR  (double)(0x40000000/10e6)

extern	double	tof_init;

extern  int	fattn[10];
extern	int	hr,hv;

settxif(offset, address)
double	offset;
int	address;
{
int     bits32, llbyte, hlbyte, lhbyte, hhbyte;
   bits32 = (int) (offset * FACTOR);
   llbyte = ((bits32)&0xFF);
   hlbyte = ((bits32 >>  8)&0xFF);
   lhbyte = ((bits32 >> 16)&0xFF);
   hhbyte = ((bits32 >> 24)&0xFF);
   if (fattn[0] == 256)
   {  putcode(APBOUT);
      putcode(6);
      putcode(-APB_SELECT + 0x110);
      putcode(-APB_WRITE  + 0x100 + llbyte);    /* DDS1 PIRA 0-7 */
      putcode(-APB_INRWR  + 0x100 + hlbyte);    /* DDS1 PIRA 8-15 */
      putcode(-APB_INRWR  + 0x100 + lhbyte);    /* DDS1 PIRA 16-23 */
      putcode(-APB_INRWR  + 0x100 + hhbyte);    /* DDS1 PIRA 24-31 */
      putcode(-APB_SELECT + 0x11E);             /* DDS1 AHC register */
      putcode(-APB_WRITE  + 0x100);
   }
   else
   {  putcode(APBOUT);
      putcode(35);					/* HARD coded ! */
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x0A);       /* DDS1 AMC register */
      putcode(-APB_INRWR  + RF_ADDR + 0x8F);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x08);       /* DDS1 SMC register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x0C);       /* DDS1 ARR register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x00);       /* DDS1 PIRA 0-7 */
      putcode(-APB_INRWR  + RF_ADDR + llbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x01);       /* DDS1 PIRA 8-15 */
      putcode(-APB_INRWR  + RF_ADDR + hlbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x02);       /* DDS1 PIRA 16-23 */
      putcode(-APB_INRWR  + RF_ADDR + lhbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x03);       /* DDS1 PIRA 24-31 */
      putcode(-APB_INRWR  + RF_ADDR + hhbyte);
   
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x04);       /* DDS1 PIRB 0-7 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x05);       /* DDS1 PIRB 8-15 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x06);       /* DDS1 PIRB 16-23 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x07);       /* DDS1 PIRB 24-31 */
      putcode(-APB_INRWR  + RF_ADDR);
   
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x0E);       /* DDS1 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
   
   }
}

pulsesequence()
{
char	tn[MAXSTR];
double	txif;
   getstr("tn",tn);
   status(A);
   if ( strcmp(tn,"H1") )
   {   printf("tn must be 'H1'");
       exit(0);
   }

/* offset the transmitter IF by 1000 Hz */
   txif = (20.0 * hv / hr - sfrq) * 1e6 - (tof-tof_init) + 1000.0;
   settxif(txif,HB_XMT_SEL);

/* turn on rcvr and xmtr limit override */
   putcode(APBOUT);
   putcode (2);
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x50);
   putcode(-APB_INRWR  + RF_ADDR + 0x15);
   delay(2e-6);

   gate(128,TRUE);

/* equilibrium period */
   assign(zero,ct);
   hsdelay(d1);

}
