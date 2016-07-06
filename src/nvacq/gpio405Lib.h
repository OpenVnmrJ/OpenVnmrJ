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
/* #define _POSIX_SOURCE  defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE  defined when source is System V */
/* #ifdef __STDC__  used to determine if using an ANSI compiler */


#ifndef INCgpio405h
#define INCgpio405h


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*

GPIO_24      Read Geo address and type address enable.  
            Should normally be 1 and 
            0 when you want to read the Geo and type address.  
            It should be in Tri state after power up and it's pull up with a resistor 
            so it will not enable until a 0 is written to it.  It's is also a bootstrap bit.

GPIO_16:14   Type address
GPIO_12:8    Geo Address
  These bits are share with CS (Chip Selects)and IRQ so you have to 
  set them to GPIO and disable them as CS or IRQ.  
  These Chip Selects and IRQs are not being used at the moment, 
  so you don't have to reset them.

GPIO_2    Configuration enable.  Normally 1.  
          Write a 0 when you want to configure the FPGA.  
          It should be in Tri state after power up and it's pull up with a resistor 
          so it will not enable until a 0 is written to it.  It's is also a bootstrap bit.
          (This is shared with a trace bit but we are not  using the trace port) 

GPIO_7    Conf_INIT
GPIO_6    Conf_D0
GPIO_5    Conf_CLK
GPIO_4    Conf_Prog   must tri state
GPIO_3    Conf_Done

GPIO: 2, 4, 5, 6, 23, 24 - Outputs
GPIO: 3, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16  - Input

GPIO: 24 & 2  must be tri-stated 

GPIO: 23  Turn On & Off FPGA Programming Yellow LED

Note: IBM labels bits reverse of MSB, e.g. IBM bit 0 == bit 31 of 32 bits (0-31)

 CPC0_CR0 Bits  (shared pin configuration GPIO vs ChipSelect & Interrupts)
Bit  value
 4 - 0 = GPIO1-9 enable			(bit 31-4 = 27 in the programming world)
 5 - 1 = GPIO10 (rather than PerCS1)
 6 - 1 = GPIO11 (rather than PerCS2)
 7 - 1 = GPIO12 (rather than PerCS3)
 8 - 1 = GPIO13 (rather than PerCS4)
 9 - 1 = GPIO14 (rather than PerCS5)
10 - 1 = GPIO15 (rather than PerCS6)
11 - 1 = GPIO16 (rather than PerCS7)
12 - 1 = Enable interrupt IRQ0 as GPIO17
13 - 1 = Enable interrupt IRQ1 as GPIO18
14 - 1 = Enable interrupt IRQ2 as GPIO19
15 - 1 = Enable interrupt IRQ3 as GPIO20
16 - 1 = Enable interrupt IRQ4 as GPIO21
17 - 1 = Enable interrupt IRQ5 as GPIO22
18 - 1 = Enable interrupt IRQ6 as GPIO23
19 - 1 = Enable interrupt IRQ7 as GPIO24
 
CPC0_CR0 0-4 notused, 4 - 19 GPIO Enable/Disable, 20 - 31 notused
 e.g. to programmer thats means bits 27-12 control  GPIO   

       (remember IBM bit 4 is programmers bit 27)
*/
/* IBM bit notation !!!, Programmers Beware!! */
/* Note: IBM labels bits reverse of MSB, e.g. IBM bit 0 == bit 31 of 32 bits (0-31) */
/* define GPIO bits as big endians */
#define IBM_GPIO2  ( 1 << (31-2) )
#define IBM_GPIO3  ( 1 << (31-3) )
#define IBM_GPIO4  ( 1 << (31-4) )
#define IBM_GPIO5  ( 1 << (31-5) )
#define IBM_GPIO6  ( 1 << (31-6) )
#define IBM_GPIO7  ( 1 << (31-7) )
#define IBM_GPIO24 ( 1 << (31-24) )

#define IBM_GPIO8 ( 1 << (31-8) )
#define IBM_GPIO9 ( 1 << (31-9) )
#define IBM_GPIO10 ( 1 << (31-10) )
#define IBM_GPIO11 ( 1 << (31-11) )
#define IBM_GPIO12 ( 1 << (31-12) )
#define IBM_GPIO13 ( 1 << (31-13) )
#define IBM_GPIO14 ( 1 << (31-14) )
#define IBM_GPIO15 ( 1 << (31-15) )
#define IBM_GPIO16 ( 1 << (31-16) )

#define GEO_ADDR_BITS  (IBM_GPIO8 | IBM_GPIO9 | IBM_GPIO10 | IBM_GPIO11 | IBM_GPIO12 )
#define GEO_TYPE_BITS  (IBM_GPIO14 | IBM_GPIO15 | IBM_GPIO16 )

#define IBM_GPIO2_BIT  (31-2)
#define IBM_GPIO3_BIT  (31-3)
#define IBM_GPIO4_BIT  (31-4)
#define IBM_GPIO5_BIT  (31-5)
#define IBM_GPIO6_BIT  (31-6)
#define IBM_GPIO7_BIT  (31-7)
#define IBM_GPIO24_BIT (31-24)
 
#define IBM_GPIO8_BIT (31-8)
#define IBM_GPIO9_BIT (31-9)
#define IBM_GPIO10_BIT (31-10)
#define IBM_GPIO11_BIT (31-11)
#define IBM_GPIO12_BIT (31-12)
#define IBM_GPIO13_BIT (31-13)
#define IBM_GPIO14_BIT (31-14)
#define IBM_GPIO15_BIT (31-15)
#define IBM_GPIO16_BIT (31-16)

#define IBM_GPIO23_BIT (31-23)
#define IBM_GPIO23 ( 1 << (31-23) )

/* For FPGA programming code */
#define FPGA_CONF_SWITCH (IBM_GPIO2)
#define FPGA_DONE (IBM_GPIO3)
#define FPGA_PROG (IBM_GPIO4)
#define FPGA_CCLK (IBM_GPIO5)
#define FPGA_DIN  (IBM_GPIO6)
#define FPGA_RESET (IBM_GPIO6)
#define FPGA_INIT (IBM_GPIO7)
#define FPGA_PROG_LED    (IBM_GPIO23)



/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern void initGPIO(void);
extern unsigned int readGeoId(void);
extern int loadVirtex(char *filename);
extern unsigned long readGPIO_IR(void);
extern void resetFPGA();

extern void delayAbit(int count);
extern unsigned char bitreverse(unsigned char byte);
extern int nvloadFPGA(char *pImage,unsigned long size, int debuglevel);
extern unsigned long readGPIO_IR();
extern int loadVirtex(char *filename);


#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif

