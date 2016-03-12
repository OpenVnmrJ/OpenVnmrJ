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


/* "/sw/windT2_2_PPC/target/config/wrSbc405gp/ppc405GP.h" */
#include <vxWorks.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
/* #include "nv405GP.h" */
#include "ppc405GP.h"
#include "nvhardware.h"
#include "gpio405Lib.h"


/*
DESCRIPTION
 
  This module also contrains routines to:

  Read the Geographical ID of the board via the 405's GPIO interface.
  This Id determine the persona of the board, RF, PFG, Master, Gradient, DDR, etc.
  This person is achieved by loading the appropraite persona Virtex II FPGA bit file
  into the FPGA.

  Program the Virtex II with the configuartion file via the IBM 405 GPIO

  GPIO registers 
  --------------
  CPC0_CR0    (CLKPWRCH_DCR_BASE+0x1)  Chip control 0 Register
  must be access via  sysDcrCr0Set(newValue) or value = sysDcrCr0Get()

  GPIO_BASE          0xEF600700
  GPIO_OR      (GPIO_BASE+0x00)   GPIO Output Register
  GPIO_TCR     (GPIO_BASE+0x04)   GPIO Three-state Control Reg
  GPIO_ODR     (GPIO_BASE+0x18)   GPIO Open Drain Reg
  GPIO_IR      (GPIO_BASE+0x1c)   GPIO Input Register

*/

/*   static function proto types */
static void writeBit(int bit);
static void ShiftDataByteOut(unsigned char  data8 );
static void toggleBitLow(unsigned int bit2toggle);
/* static void toggleBitHigh(unsigned int bit2toggle); */
static int check4Bit(unsigned bit);

/* extern unsigned long sysDcrCr0Get(void); */
/* extern void sysDcrCr0Set(unsigned long); */

#define CPC0_CR0_GPIO_MASK  0x0FFFF000
int prtGpioDirection(unsigned int gpioLineDirection);
int prtGpioOutput(unsigned int gpioLineDirection, unsigned int output);
int prtGpioInput(unsigned int gpioLineDirection, unsigned int input);
void gpioConfigShow(int level);

/***********************************************************************
*
* Function:     Initialize IBM 405 GPIO pins 
*
* Description:  
*
*     Set CPC0_CR0 control register to enable pins as GPIO lines
*
*     Set IBM405_GPIO0_TCR Three State GPIO control register
*     having  IBM_GPIO3 & IBM_GPIO7 inputs (i.e. high impendance)
*
*    Set IBM405_GPIO0_ODR (open drain) GPIO control register
*    having IBM_GPIO2 & IBM_GPIO24 as open dain.
*
*    Set the output values of IBM_GPIO2 & IBM_GPIO24 to 1.
*    GPIO: 2, 4, 5, 6, 23, 24 - Outputs
*    GPIO: 3, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16  - Input
*    
*    GPIO: 24 & 2  must be tri-stated 
*    
*
* Parameters:   None
*
* Returns: void 
*
***********************************************************************/
void initGPIO(void)
{
   unsigned int CPC0_CR0_value,threeStateValue,CPC0_MaskOut;

   /* get present CR) register value */
   /* for now assume that the booting has set the GPIO line appropriately */
   CPC0_CR0_value = sysDcrCr0Get();
   printf("CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);
  
   /* mask out, or set to zero CR0_TRE, CR0_GPIO_13_EN  & CR0_GPIO_19_EN */
   /* CPC0_MaskOut = ~(CR0_TRE | CR0_GPIO_10_EN | CR0_GPIO_11_EN | CR0_GPIO_13_EN | CR0_GPIO_19_EN); */
  /* clear or set to zero all configruation lines, this default to GPIO1-9 and then CS17,IRQ1-7 */
   CPC0_MaskOut = ~(CPC0_CR0_GPIO_MASK);
   CPC0_CR0_value = CPC0_CR0_value & CPC0_MaskOut;
   printf("CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);

  /* set bit for GPIO lines we want */
   CPC0_CR0_value = CPC0_CR0_value | ( CR0_GPIO_23_EN | CR0_GPIO_24_EN );
   printf("CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);
 
  /* configure for  1-9,23,24 tobe GPIO lines */
    sysDcrCr0Set(CPC0_CR0_value);

   /* High Impedance settings */
   /* Only Inputs are driven to high impedance state */
#ifdef XXX
   threeStateValue = 0xFFFFFFFF;   /* 1 == output gpio 0=inputs */
   inputBits = IBM_GPIO3 | IBM_GPIO7 | GEO_ADDR_BITS | GEO_TYPE_BITS;  /* these are inputs */
   threeStateValue = threeStateValue &  ~inputBits;  /* mask out inputs, e.i. make zero */
   *IBM405_GPIO0_TCR |= threeStateValue;  /* bits 30 - 7 */
#endif

   threeStateValue = 0; /* default all inputs */
   /*
    * threeStateValue |= (IBM_GPIO2 | IBM_GPIO4 | IBM_GPIO5 | IBM_GPIO6 | IBM_GPIO23 | IBM_GPIO24 );
    * leave GPIO 2 & 4 as input until actually programming the FPGA, thus allows the
    * FPGA program to survive a reset/reboot
    */
   /* threeStateValue |= (IBM_GPIO5 | IBM_GPIO23 | IBM_GPIO24 ); */
   threeStateValue |= ( IBM_GPIO23 | IBM_GPIO24 );  /* only the essential outputs set for now */
   *IBM405_GPIO0_TCR =  threeStateValue;  /* bits 30 - 7 */

  
   /* Open Drain, settings */
   /* Joe's say 10/30/03 now NO open drain lines so I'll just it to Zero */
   *IBM405_GPIO0_ODR = 0;


   /* set deafult values on GPIO pins */
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~IBM_GPIO2); /* clear GPIO2 bitm FPGA switch  */
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | IBM_GPIO24); /* set GPIO24 to 1 , turn read switch off */
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~IBM_GPIO23); /* set GPIO23 to 0 , turn read switch off */

   return;
}


/***********************************************************************
*
* Function:     Read GPIO Input Register
*
* Description:  
*
* To Read GPIO IR
*
*     Write a 0 to GPIO24 to read Geo and Type address.
*     Write a 1 to GPIO24 when you are done reading.
*
*     Read GPIO InputRegister, GPIO[16:8]  value ( i.e. bits 23 - 15 )
**
* Parameters:   None
*
* Returns:
**	  unsigned long GPIO IR 
*
***********************************************************************/
unsigned long readGPIO_IR()
{
  unsigned long gpio_ir; 
  unsigned int CPC0_CR0_value,CPC0_CR0_GPIO_value;

  /* must reconfigure GPIO lines via the CPC0_CR0 for GPIO lines */
   CPC0_CR0_value = sysDcrCr0Get();

   CPC0_CR0_GPIO_value = ( CR0_GPIO_10_EN | CR0_GPIO_11_EN | CR0_GPIO_12_EN | 
		           CR0_GPIO_14_EN | CR0_GPIO_15_EN | CR0_GPIO_16_EN );
   sysDcrCr0Set((CPC0_CR0_value | CPC0_CR0_GPIO_value));
  
   /* clear IBM_GPIO24 bit */
   *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~IBM_GPIO24);

    delayAbit(1000);


    /* read GPIO input register */
    gpio_ir = *IBM405_GPIO0_IR;

    delayAbit(1000);

    /* set IBM_GPIO24 bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | IBM_GPIO24);

   /* return GPIO lines back to CS as needed */
   sysDcrCr0Set(CPC0_CR0_value);

   /* return ID */
    return ( gpio_ir );
}

/***********************************************************************
*
* Function:     Read Geo and Type address
*
* Description:  
*
* To Obtain Geo and Type address:
*
*     Read GPIO InputRegister, GPIO[16:8]  value ( i.e. bits 23 - 15 )
*
*     GPIO_14:16   Type address
*     GPIO_8:12    Geo Address
*
*	Note: values are bit reversed.
*
* Parameters:   None
*
* Returns:
*	  int Geo&Type Address
*
***********************************************************************/
unsigned int readGeoId()
{
  unsigned long gpio_ir; 
  unsigned long type,addr;
  unsigned int geoAddr;
  unsigned char rtype, raddr;
 
   gpio_ir = readGPIO_IR();

   printf("GPIO IR: 0x%lx\n",gpio_ir);

   type = (gpio_ir & GEO_TYPE_BITS) >> IBM_GPIO16_BIT;
   addr = (gpio_ir & GEO_ADDR_BITS) >> IBM_GPIO12_BIT;

   /* printf("GEO: Type %lu, (0xlx), Address: %lu, (0x%lx) \n",type,type,addr,addr); */

   /* 3 bit field has 5 zero in front, when byte swapped need to shift down by 5 */
   rtype = (bitreverse( (char) (type & 0xFF)) >> 5) & 0x7;

   /* 5 bit field has 3 zero in front, when byte swapped need to shift down by 3 */
   raddr = (bitreverse( (char) (addr & 0xFF)) >> 3) & 0x1F;

   printf("GEO: Type %u, (0x%x), Address: %u, (0x%x) \n",rtype,rtype,raddr,raddr);
   /* pack the type & addr into 16 bits */
   geoAddr = (rtype & 0xff) << 8 | (raddr & 0xff);

   /* return ID */
    return ( geoAddr );
}

/***********************************************************************
*
* Function:     Program Virtex II FPGA with configuration data
*
* Description:  
*
*     To configure FPGA:
*
*     Write a 0 to GPIO2.
*
*     Set  GPIO4 (PROGn) to 0 and Wait for 30 Local Bus cycle (300ns min) (this reset the config logic)
*     Set  GPIO4 to 1.
*
*     Read GPIO7 and check to see if it's 1, wait till it's a 1.
*
*     Set GPIO23 to Turn On FPGA Programming LED
*
*     Write to GPIO6 to send 1 bit of Data.
*     At the next instruction write a 1 GPIO5 to send 1 CCLK. Increment counter.
*     At the next instruction send another bit of Data via GPIO6 and set GPIO5 back to 0.
*     Repeat until all bits are send.
*
*     write some dummy CCLKS out for FPGA   (20 CCLKs)
*
*     if INIT still High,  FPGA expecting more data 
*     send out same number of CCLKs as data, just to be sure
*
*     Unset GPIO23 to Turn Off FPGA Programming LED
*
*     Read GPIO3 and wait for a 1 and you are done. 
*
*     When Done goes to one, then reset the FPGA via the GPIO6 (i.e. DIN line)
*     by toggling it from 1 to 0 to 1 again.
*
*     If Done doesn't go high, then programming failed.
*
*     GPIO2 (Configuration enable.  Normally 1, 0 to Program FPGA;  output)
*     GPIO3 (DONE, input)
*     GPIO4 (PROGn, output)
*     GPIO5 (CCLK, output)
*     GPIO6 (Din, output)
*     GPIO7 (INITn, intput)
*
*     0 - value of 0
*     1 - value of 1
*     x - value does not matter
*     ? - testing bit value  (inputs)
*
*          GPIO bits
*     -----------------
*
*      2  3  4  5  6  7
*      ----------------
* 0.   1  x  1  x  x  x  default state (can read Geo address)
* 1.   0  x  1  x  x  x  Program FPGA state
* 2.   0  x  0  x  x  x  reset FPGA, hold 30 Bus cycles (300ns min)
* 3.   0  x  1  x  x  x  end of toggling PROG (bit-4)
* 4.   0  x  1  x  x  ?  wait until GPIO7 is 1 or after ~2.1 ms abort with error
*     ready to send data
* 5.   0  x  1  0  0  x
* 6.   0  x  1  1  0  x  completes write of bit value zero to FPGA
* 7.   0  x  1  0  1  x
* 8.   0  x  1  1  1  x  completes write of bit value one to FPGA
* ..
* ..
* 10.  0  ?  x  x  x  x  Check for Done
*                        (how long to wait before reporting error ??)
* 
*
* GPIO23 set 1 light LED yellow
*
*
* Parameters:   buffer - configuration data, size of data
*
* Returns:
*	  int 0 if successful 
*
***********************************************************************/

int nvloadFPGA(char *pImage,unsigned long size, int debuglevel)
{
   extern STATUS sysUicIntDisable( int );
   int foundsync,status,go2prog,ledon;
   unsigned long count,i,j,gpio_ir;
   unsigned char syncbyte;
   unsigned long *cntptr;
   int initlvl,donelvl, chkdone;
   unsigned int threeStateValue,oldTCR_val;
   unsigned int CPC0_CR0_value;

   syncbyte = 0xaa;
   chkdone = foundsync = 0;

    /* search for syncbyte 0xAA in FPGA bit file, sync word is 0xaa995566 */
    /* this sync word defines the 32 bit word boundaries that the FPGA expects */
   for( i=0; i< size; i++, pImage++)
   {
      if (*pImage == syncbyte)
      {
          foundsync = 1;
          break;
      }
   }
   if (! foundsync )
   {
      printf("Didn't find sync byte in FPGA image file, aborting\n");
      return(-1);
   }

   /* disable extern interrupt from the FPGA while programming FPGA */
   /* is never re-enable by this routine */
   sysUicIntDisable(25);

   cntptr = (unsigned long *) pImage;
   if (debuglevel > 0)
     printf("Sync word (0xAA995566) found: 0x%lx\n",*cntptr);

   /* Obtain actual size of FPGA image from data stream */
   /* cntptr = (unsigned long *) (pImage - 8); */
   /* count = *cntptr; */
   count = size;

   if (debuglevel > 0)
       gpioConfigShow(0);

    /* Joe's say 10/30/03 now no open drain lines so I'll just be sure here */
    *IBM405_GPIO0_ODR = 0;
 
    /* Joe say clear em, then set TCR ? */
    /* *IBM405_GPIO0_OR = 0; */
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~IBM_GPIO2); /*  clear bit */

   /* High Impedance settings */
   /* Only Inputs are driven to high ipedance state */
   /* Set bits for those lines we will be driving to program the FPGA for Now */
   oldTCR_val = *IBM405_GPIO0_TCR;  /* save old state to be restored later */
 
   /* the Bits we must drive 2,4,5,6,23 */
   /* *IBM405_GPIO0_TCR = 0; */
   threeStateValue = (FPGA_CONF_SWITCH | FPGA_PROG | FPGA_CCLK | FPGA_DIN | FPGA_PROG_LED);
   *IBM405_GPIO0_TCR = threeStateValue;  /* bits 30 - 7 */
 
   if (debuglevel > 0)
   {
      prtGpioDirection(*IBM405_GPIO0_TCR);
      prtGpioOutput(*IBM405_GPIO0_TCR,*IBM405_GPIO0_OR);
      prtGpioInput(*IBM405_GPIO0_TCR,*IBM405_GPIO0_IR);
   }
   /* if (debuglevel > 0)
    printf(" GPIO OR: 0x%lx, IR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_OR,(unsigned long)  *IBM405_GPIO0_IR); */

    /* clear the GPIO 2 bit for programming FPGA */
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(FPGA_CONF_SWITCH));  /* clear bit */

   if (debuglevel > 0)
    printf(" GPIO OR: 0x%lx, IR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_OR,(unsigned long)  *IBM405_GPIO0_IR);

    CPC0_CR0_value = sysDcrCr0Get();
   if (debuglevel > 0)
   {
    printf(" CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);
    printf(" GPIO ODR: 0x%lx, OR: 0x%lx, TCR: 0x%lx, IR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_ODR,(unsigned long) *IBM405_GPIO0_OR,
		(unsigned long) *IBM405_GPIO0_TCR,(unsigned long) *IBM405_GPIO0_IR);
   }

   /* Initiate programming to FPGA */
   /*--------------------------------------------------------------------------*/
   /* Toggle Program Pin (PROGn) by Toggling 405 GPIO4 bit
   */
   if (debuglevel > 0)
     printf("Toggle PROG line High to Low to High\n");
   toggleBitLow(FPGA_PROG);  /* resets the configuration logic, and holds down INIT */

   if (debuglevel > 0)
    printf("waiting for FPGA INIT to go High\n");
   /* wait until INITn goes high, if doesn't happen in 2.1 ms then error */
   /* logic analyzer show it takes 1.1 ms for INIT line to go High after PROG goes High */
   for (i=0,go2prog=0; i < 10; i++ )
   {
       if (debuglevel > 1)
       {
         printf("cnt: %ld,  GPIO OR: 0x%lx, IR: 0x%lx\n",i, (unsigned long) *IBM405_GPIO0_OR, (unsigned long) *IBM405_GPIO0_IR);
         prtGpioOutput(*IBM405_GPIO0_TCR,*IBM405_GPIO0_OR);
         prtGpioInput(*IBM405_GPIO0_TCR,*IBM405_GPIO0_IR);
         printf("\n\n");
       }
       if (check4Bit(FPGA_INIT) )
       {
          go2prog = 1;
          break;
       }
       delayAbit(90000); 
   }
   if (!go2prog)
   {
      printf("INIT line never went high on FPGA, can't Program.\n");
      *IBM405_GPIO0_TCR = oldTCR_val;  /* restore TCR value */
      return(-1);
   }

    /* Now turn ON the FPGA Programming Yellow LED GPIO13 bit */
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | FPGA_PROG_LED); /* Turn on LED */
     ledon = 1;

   /* write at least three dummy CCLKS to out for FPGA */
   /* output one dummy word 0xfffffff, four bytes, to provide Clock cycles necessary
      for initialization of the configuration logic */
   for(i=0; i < 4; i++)
       ShiftDataByteOut(0xff);


   /* configure the FPGA by reading from RAM to the FPGA data bit line */
    printf("Sending configuration data to FPGA\n");
    /* for (i=0;i < count; i++,pImage++,cntptr++) */
    for (i=0;i < count; i++,pImage++)
    {
       if (debuglevel > 1)
       {
         if (!(i % 4))
         { 
           cntptr = (unsigned long *) pImage;
           if (*cntptr == 0xaa995566)
                      printf("Sending sync word: 0x%lx\n",*cntptr);
           if (*cntptr == 0x30000001)
           {
                          printf("Sending CRC load CMD: 0x%lx, value: 0x%lx (%ld)\n",
                                        *cntptr,*(cntptr+1),*(cntptr+1));
	   }
         } 
      }

       if (!(i % 20000))
       {
          if(ledon)
          {
              *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & (~(FPGA_PROG_LED))); /* Turn Off LED */
              ledon = 0;
          }
          else
          {
             *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | FPGA_PROG_LED); /* Turn on LED */
              ledon = 1;
          }
       }

       /* data bits out */
       ShiftDataByteOut(*pImage);
       /* ShiftData32Out(*cntptr); */
       /* printf("cnt: %d, GPIO OR: 0x%lx, IR: 0x%lx\n",i,*IBM405_GPIO0_OR, *IBM405_GPIO0_IR); */
    }

   if (debuglevel > 1)
   {
    initlvl = check4Bit(FPGA_INIT);
    donelvl = check4Bit(FPGA_DONE);
    gpio_ir = *IBM405_GPIO0_IR;
    printf("GPIO_OR: 0x%lx, INIT: %d, DONE: %d\n",gpio_ir,initlvl,donelvl);
   }
 
    /* if INIT still High,  FPGA is still expecting more data */
    /* typically 4 dunny words are flush through the FPGA to provide the
       finishing CCLK cycles to activate the FPGA */
 
    if ((check4Bit(FPGA_INIT) && !(check4Bit(FPGA_DONE))))
    {
       printf("FPGA INIT still high, send some dummy CCLKs\n");
       for(i=0; i < count / 4; i++)
       { 
         /* output one dummy word 0xfffffff, four bytes */
         for(j=0; j < 4; j++)
            ShiftDataByteOut(0x00);
 
         initlvl = check4Bit(FPGA_INIT);
         donelvl = check4Bit(FPGA_DONE);
         /* printf("INIT: %d, DONE: %d\n",initlvl,donelvl); */
         if (donelvl)
         {
           printf("Done bit has gone high at word %ld\n",i);
           printf("INIT: %d, DONE: %d\n",initlvl,donelvl);
           break;
         }
         if (!initlvl)
         {
            printf("INIT has Gone Low indicating a CRC checksum error in loading the configuration file.\n");
            printf("INIT: %d, DONE: %d\n",initlvl,donelvl);
            break;
         }
       } 
    }
    /* typically 4 dummy words are flush through the FPGA to provide the
       finishing CCLK cycles to activate the FPGA */
   /*     for(i=0; i < 4*4; i++)
            ShiftDataByteOut(0xaa); */


    /* Now turn OFF the FPGA Prograing Yellow LED GPIO13 bit */
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & (~(FPGA_PROG_LED))); /* Turn Off LED */


   if (debuglevel > 1)
   {
        initlvl = check4Bit(FPGA_INIT);
       donelvl = check4Bit(FPGA_DONE);
       gpio_ir = *IBM405_GPIO0_IR;
       printf("GPIO_IR: 0x%lx, INIT: %d, DONE: %d\n",gpio_ir,initlvl,donelvl);
   }
 
    /* status = Check_DONE_Bit(); */
    if (check4Bit(FPGA_INIT) && (check4Bit(FPGA_DONE)))
    {
      printf("FPGA Configuration Successful.\n");
      status = 1;
      /* now reset the FPGA via the GPIO6 line */
      /* reset by setting GPIO6 fro high to low then back to high */
      printf("Reseting FPGA via GPIO line.\n");
      toggleBitLow(FPGA_RESET); 
      if (debuglevel > 1)
      {
       initlvl = check4Bit(FPGA_INIT);
       donelvl = check4Bit(FPGA_DONE);
       gpio_ir = *IBM405_GPIO0_IR;
       printf("GPIO_IR: 0x%lx, INIT: %d, DONE: %d\n",gpio_ir,initlvl,donelvl);
     }

    }
    else
    {
      printf("FPGA Configuration Failed.\n");
      status = 0;
    }
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | IBM_GPIO2); /*  set bit */
     
     threeStateValue = ~(FPGA_CONF_SWITCH | FPGA_PROG | FPGA_CCLK | FPGA_DIN );
    *IBM405_GPIO0_TCR = (oldTCR_val & threeStateValue);  /* restore TCR value minus FPGA programming bits */
     
    if (debuglevel > 0)
    {
       initlvl = check4Bit(FPGA_INIT);
       donelvl = check4Bit(FPGA_DONE);
       gpio_ir = *IBM405_GPIO0_IR;
       printf("GPIO_IR: 0x%lx, INIT: %d, DONE: %d\n",gpio_ir,initlvl,donelvl);
    }

   return(status);
}

/***********************************************************************
*
* Function:     Write Bit values out to FPGA 
*
* Description:  
*
*    Routine to write a bit out to the FGPA DIN line GPIO_6,
*    and toggling the CCLK line (GPIO_5) as required.
*
* Parameters:   int bitvalue
*
* Returns:
*	  void
*
***********************************************************************/
static void writeBit(int bit)
{
   /* decide if data bit is 0 or 1 */
   if (bit)
   {
     /* set D_in pin (GPIO6) to FPGA with value of bit (one in this case)  with FPGA_CCLK at Zero */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~FPGA_CCLK) | FPGA_DIN; /* set Bit, and Bring FPGA_CCLK Low */
   }
   else
   {
     /* set D_in pin (GPIO6) to FPGA with value of bit (cleared in this case)  with FPGA_CCLK at Zero */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~( FPGA_CCLK | FPGA_DIN )); /* Clear Bit, and Bring FPGA_CCLK Low */
   }
    
     /* Hold bit value and transition FPGA_CCLK to High, FPGA will read bit balue */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | FPGA_CCLK);  /* keep bit setting, FPGA_CCLK High */

}

#ifdef XXXX
/***********************************************************************
*
* Function:     Shiftdata32Out - Shift a 32-bit word out to FPGA 
*		via 405 GPIO
*
* Description:  
*
*      Pushes out LSW then HSW to FPGA
*
* Parameters:   32 bit int
*
* Returns:
*	  void 
*
************************************************************************/
void ShiftData32Out ( int data32 )
{
    static void ShiftDataByteOut(unsigned char  data8 );
    unsigned short data;
    data = data32 & 0xFFFF;
    ShiftDataByteOut(((data >> 8) & 0xff) );
    ShiftDataByteOut((data & 0xff );
    data = (data32 >> 16) & 0xFFFF;
    ShiftDataByteOut(((data >> 8) & 0xff) );
    ShiftDataByteOut((data & 0xff );
}

/***********************************************************************
*
* Function:     ShiftDataWordOut - Shift a 16-bit word out to FPGA 
*		via 405 GPIO
*
* Description:  
*
*      Pushes out LSB then MSB to FPGA
*
* Parameters:   16 bit int
*
* Returns:
*	  void 
*
************************************************************************/
void ShiftDataWordOut(unsigned short  data16 )
{
  int i;
  unsigned short data;
  /* Upper Byte: */
  data = data16 >> 8;
   for (i = 0; i < 8; i++)
    writeBit((data >> i) & 0x1); 

  /* --  Lower Byte: */
  data = data16;
  for (i = 0; i < 8; i++)
    writeBit((data >> i) & 0x1);

}
#endif

static void ShiftDataByteOut(unsigned char  data8 )
{
   int i,bit;
  /* --  Lower Byte: */
  /* shift each bit out, 0 - 7 */
  /* for (i = 0; i < 8; i++)
    writeBit((data8 >> i) & 0x1);  */
   /* shift each bit out, 7 - 0 */
  for (i = 7; i >= 0; i--)
  {
     bit = (data8 >> i) & 0x1;
     writeBit(bit);
  }
}

/***********************************************************************
*
* Function:    Check_DONE_Bit - check FPGA Done bit
*
* Description:  
*
*     Read FPGA DONE bit via GPIO_3 and determine if configuration
*     was successful
*     
* Parameters:   None
*
* Returns:
*	  int Done
*
************************************************************************/
#ifdef XXXXX
static int Check_DONE_Bit(void)
{
    unsigned long gpio_ir;
    unsigned int done, init, i;
    done = 0;
    for (i=0; i < 10; i++ )
    {
       delayAbit(10000);
       gpio_ir = readGPIO_IR();
       printf("GPIO_IR: 0x%lx\n",gpio_ir);
       done = (gpio_ir & ~(IBM_GPIO3));
       printf("done: 0x%lx\n",done);
       done = (done) ? 1 : 0;
       if (done)
          break;
    }
    if (!done)
    {
        /* check INIT */
        init = (gpio_ir & ~(IBM_GPIO7));
        printf("Configuration Failed, DONE is Low, Init is: %d\n",init);
    }
    return done;
}
#endif

/*----------------------------------------------------------------------*/
/* loadVirtex                                                       	*/
/*      Programm the FPGA with the file given				*/
/*      file path should be absolute 					*/
/*      e.g.   loadVirtex("/tmp/rfcntrl.bin");				*/
/*      unsuccessful: -1                                                */
/*      successful:  0                                                  */
/*----------------------------------------------------------------------*/
int loadVirtex(char *filename)
{

   int fd,status;
   struct stat fileStats;
   unsigned long fileSizeBytes,bytes;
   unsigned char *pMemory;

   /* First check to see if the file system is mounted and the file     */
   /* can be opened successfully.                                       */
   fd = open(filename, O_RDONLY, 0444);
   if (fd < 0)
   {
        printf("loadVirtex: input file '%s', could not be open\n",filename); 
        return(-1);
   }
   if (fstat(fd,&fileStats) != OK)
   {
     printf("failed to obtain status infomation on file: '%s'\n",filename);
     close(fd);
     return(-1);
   }

  /* make sure it's a regular file */
   if ( ! S_ISREG(fileStats.st_mode) )
   {
      printf("Not a regualr file.\n");
      close(fd);
      return(-1);
   }

   fileSizeBytes = fileStats.st_size;

   printf("file: '%s', %lu bytes\n", filename,fileSizeBytes);

   pMemory = (unsigned char *) malloc (fileSizeBytes);

   bytes = read(fd,pMemory,fileSizeBytes);

   close(fd);

   if (bytes != fileSizeBytes)
   {
     printf("Error, unable to read all the file\n");
     printf("read %lu bytes out of %lu bytes\n",bytes,fileSizeBytes);
     free(pMemory);
     return(-1);
   }

   status = nvloadFPGA(pMemory,fileSizeBytes,0);
   free(pMemory);
   if ( status != 1 )
      printf("FPGA programming failed.\n");
   else
      printf("FPGA programming Successfully Completed.\n");

   return(0);
}

/*--------------------------------------------------------------
*  check4Bit( bit_mask )
*      check if bit is set with the GPIO Input register
*
*---------------------------------------------------------------*/
static int check4Bit(unsigned bit)
{
   unsigned long gpio_ir;
    int done;
    /* gpio_ir = readGPIO_IR(); */
    /* done = (gpio_ir & ~(bit)); */
    /* read GPIO input register */
    gpio_ir = *IBM405_GPIO0_IR;
    /* printf(" OR: 0x%lx, IR: 0x%lx\n",*IBM405_GPIO0_OR, *IBM405_GPIO0_IR); */
    done = (gpio_ir & (bit));
    done = (done) ? 1 : 0;
    return done;
}   

/*------------------------------
*  delayAbit - busy loop to delay
*--------------------------------*/
void delayAbit(int count)
{
   volatile unsigned long i;
   volatile long q,d;
   long a;
   q = 63;
   d = 7;
   for (i=0; i < count; i++)
   {
      a = q/d;
   }
}

#ifdef NOT_USED
/*--------------------------------------------------
  toggleBitHigh - toggles a bit from off to on then off 
----------------------------------------------------*/
static void toggleBitHigh(unsigned int bit2toggle)
{
     /* Clear bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(bit2toggle));  /* clear bit */
     delayAbit(100);

     /* set bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | bit2toggle); /*  set bit */
     delayAbit(100);
   
     /* Clear bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(bit2toggle));  /* clear bit */
     delayAbit(100);

}
#endif

/*--------------------------------------------------
  toggleBitLow - toggles a bit from off to on then off 
----------------------------------------------------*/
static void toggleBitLow(unsigned int bit2toggle)
{
     /* Set bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | bit2toggle); /*  set bit */
     delayAbit(100);
   
     /* Clear bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(bit2toggle));  /* clear bit */
     delayAbit(100);

     /* Set bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | bit2toggle); /*  set bit */
     delayAbit(100);
}

void togglebitfast(unsigned int bit2toggle, unsigned int count)
{
    int i;
    *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(IBM_GPIO2));  /* clear bit */
    for (i=0; i < count; i++)
    {
     /* set bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR | bit2toggle); /*  set bit */
 
     /* Clear bit */
     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(bit2toggle));  /* clear bit */
    }
}

/*
* use to reset entire FPGA via the GPIO line
* not for casual use.
*     GMB 
*/
void resetFPGA()
{
     unsigned long oldState;
     unsigned long oldTCR_val;
     unsigned long threeStateValue;
    
     oldState = *IBM405_GPIO0_OR;

   /* High Impedance settings (e.g. are ther input or outputs) */
   /* Only Inputs are driven to high ipedance state */
   oldTCR_val = *IBM405_GPIO0_TCR;  /* save old state to be restored later */
   /* printf("TCR: GPIO: 0x%lx\n",oldTCR_val); */

   *IBM405_GPIO0_OR |= IBM_GPIO4 | IBM_GPIO2 | IBM_GPIO24;  /* */
 
   /* Set bits for those lines we will be driving to program the FPGA for Now */
   /* the Bits we must drive 2,4,5,6,23 */
   /* *IBM405_GPIO0_TCR = 0; */
   threeStateValue = (FPGA_CONF_SWITCH | FPGA_RESET | IBM_GPIO24 | FPGA_PROG );
   *IBM405_GPIO0_TCR = threeStateValue;  /* bits 30 - 7 */
   /* printf("GPIO TCR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_TCR); */
 


     *IBM405_GPIO0_OR = (*IBM405_GPIO0_OR & ~(FPGA_CONF_SWITCH));  /* clear bit */
   /* printf("GPIO TCR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_TCR); */
     toggleBitLow(FPGA_RESET); 
     *IBM405_GPIO0_TCR = oldTCR_val;
     *IBM405_GPIO0_OR = oldState;
   /* printf("GPIO TCR: 0x%lx\n",(unsigned long) *IBM405_GPIO0_TCR); */
     return; 
}

/*---------------------------------------------------
*    nibblebitreverse
*
*    bit reverse a nibble or 4 bits
*---------------------------------------------------*/
unsigned char nibblebitreverse(unsigned char nibble)
{
   unsigned char bitrev = 0;
     switch(nibble & 0xf)
     {
        case 0: bitrev = 0x0; break;
        case 1: bitrev = 0x8; break;
        case 2: bitrev = 0x4; break;
        case 3: bitrev = 0xC; break;
        case 4: bitrev = 0x2; break;
        case 5: bitrev = 0xA; break;
        case 6: bitrev = 0x6; break;
        case 7: bitrev = 0xE; break;
        case 8: bitrev = 0x1; break;
        case 9: bitrev = 0x9; break;
        case 10: bitrev = 0x5; break;
        case 11: bitrev = 0xD; break;
        case 12: bitrev = 0x3; break;
        case 13: bitrev = 0xB; break;
        case 14: bitrev = 0x7; break;
        case 15: bitrev = 0xF; break;
     }
     return(bitrev);	
}

/*----------------------------------------------------------------
*   bitreverse a byte
*   accomplished by bit reversing the MSB * LSB nibble then
*   swapping the MSB to LSB and LSB to MSB
*--------------------------------------------------------------------*/
unsigned char bitreverse(unsigned char byte)
{
   unsigned char nibble1,nibble2,newval;
   /* bit reverse hi & low nibble for byte */
   nibble1 = nibblebitreverse(byte & 0xf);
   nibble2 = nibblebitreverse( (byte >> 4) & 0xf);
   /* now reverse nibbles in byte */
   newval = (nibble1 << 4) | nibble2;
   /* return bit reversed byte */
   return(newval);
}



/*
 * funciton to print out GPIO register configuration and settings 
 */
int prtGpioDirection(unsigned int gpioLineDirection)
{
   unsigned int bit;
   int i;

   /* direction of gpio line, Output bit=1, Input bit=0 */
   /* gpioLineDirection = *IBM405_GPIO0_TCR; */  /* bits 30 - 7 */
   printf("Line Direction: I-Input, O-Ouput\n");
   printf("Line Direction Reg (0x%x): 0x%x\n",(int) IBM405_GPIO0_TCR,gpioLineDirection);
   printf("GPIO: 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24\n");
   printf("    ");
   for(i=1; i < 25; i++)
   {
      bit = gpioLineDirection >> (31-i);
      printf("  %c", ((bit & 1) ? 'O' : 'I'));
   }
   printf("\n\n");
   return 0;
}

int prtGpioOutput(unsigned int gpioLineDirection, unsigned int output)
{
  unsigned int bit;
  int i;
  /* output = *IBM405_GPIO0_OR; */
  printf("Output Register Setting: H-High(1), L-Low(0), i-input i.e. n/a\n");
  printf("Output (write) Register(0x%x): 0x%x\n",(int) IBM405_GPIO0_OR,output);
  printf("GPIO: 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24\n");
  printf("    ");
  for(i=1; i < 25; i++)
  {
      bit = gpioLineDirection >> (31-i);
      if (bit & 1)
      {
          bit = output >> (31-i);
          printf("  %c", ((bit & 1) ? 'H' : 'L'));
      }
      else
          printf("  i");
  }
  printf("\n\n");
  return 0;
}

int prtGpioInput(unsigned int gpioLineDirection, unsigned int input)
{
   unsigned int bit;
   int i;
  /* input = *IBM405_GPIO0_IR; */
  printf("Input Register: H-High(1), L-Low(0), o-output i.e. n/a\n");
  printf("Input (read) Register(0x%x): 0x%x\n",(int) IBM405_GPIO0_IR,input);
  printf("GPIO: 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24\n");
  printf("    ");
  for(i=1; i < 25; i++)
  {
      bit = gpioLineDirection >> (31-i);
      if ( !(bit & 1))
      {
          bit = input >> (31-i);
          printf("  %c", ((bit & 1) ? 'H' : 'L'));
      }
      else
          printf("  o");
  }
  printf("\n");
  return 0;
}

void gpioConfigShow(int level)
{
   char *result;
   unsigned int CPC0_CR0_value,gpioLineDirection,input,output,bit;
   int i;

   CPC0_CR0_value = sysDcrCr0Get();
/*
   printf("CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);
   result = (CPC0_CR0_value & 0x08000000) ? "Trace Enabled" : "GPIO1-9 Enabled";
   printf("CPU Trace or GPIO1-9:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_10_EN) ? "GPIO10 Enabled" : "CS1 Enabled";
   printf("CS1 or GPIO10:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_11_EN) ? "GPIO11 Enabled" : "CS2 Enabled";
   printf("CS2 or GPIO11:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_12_EN) ? "GPIO12 Enabled" : "CS3 Enabled";
   printf("CS3 or GPIO12:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_13_EN) ? "GPIO13 Enabled" : "CS4 Enabled";
   printf("CS4 or GPIO13:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_14_EN) ? "GPIO14 Enabled" : "CS5 Enabled";
   printf("CS5 or GPIO14:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_15_EN) ? "GPIO15 Enabled" : "CS6 Enabled";
   printf("CS6 or GPIO15:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_16_EN) ? "GPIO16 Enabled" : "CS7 Enabled";
   printf("CS7 or GPIO16:  '%s'\n",result); 

   result = (CPC0_CR0_value & CR0_GPIO_17_EN) ? "GPIO17 Enabled" : "IRQ0 Enabled";
   printf("IRQ0 or GPIO17:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_18_EN) ? "GPIO18 Enabled" : "IRQ1 Enabled";
   printf("IRQ1 or GPIO18:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_19_EN) ? "GPIO19 Enabled" : "IRQ2 Enabled";
   printf("IRQ2 or GPIO19:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_20_EN) ? "GPIO20 Enabled" : "IRQ3 Enabled";
   printf("IRQ3 or GPIO20:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_21_EN) ? "GPIO21 Enabled" : "IRQ4 Enabled";
   printf("IRQ4 or GPIO21:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_22_EN) ? "GPIO22 Enabled" : "IRQ5 Enabled";
   printf("IRQ5 or GPIO22:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_23_EN) ? "GPIO23 Enabled" : "IRQ6 Enabled";
   printf("IRQ6 or GPIO23:  '%s'\n",result); 
   result = (CPC0_CR0_value & CR0_GPIO_24_EN) ? "GPIO24 Enabled" : "IRQ7 Enabled";
   printf("IRQ7 or GPIO24:  '%s'\n",result); 

   result = (CPC0_CR0_value & 0x800) ? "CTS" : "DSR";
   printf("UART1 DSR/CTS Select:  '%s' Selected\n",result); 
   result = (CPC0_CR0_value & 0x400) ? "DTR" : "RTS";
   printf("UART1 RTS/DTR Select:  '%s' Selected\n",result); 
   result = (CPC0_CR0_value & 0x200) ? "Enabled" : "Disabled";
   printf("UART0 DMA Transmit channel:  '%s'\n",result); 
   result = (CPC0_CR0_value & 0x100) ? "Enabled" : "Disabled";
   printf("UART0 DMA Receive channel:  '%s'\n",result); 
*/
/*
   result = (CPC0_CR0_value & 0x80) ? "Enabled" : "Disabled";
   printf("UART0 DMA DTE/DRE Clear:  '%s'\n",result); 
*/
/*
   result = (CPC0_CR0_value & 0x80) ? "External" : "Internal";
   printf("UART0 Clock:  '%s'\n",result); 
   result = (CPC0_CR0_value & 0x40) ? "External" : "Internal";
   printf("UART1 Clock:  '%s'\n",result); 
   value = (CPC0_CR0_value & CR0_UART_DIV_MASK) > 1;
   printf("UART internal Clock Divisor: %d\n",value+1);
*/
   
  /* direction of gpio line, Output bit=1, Input bit=0 */
  gpioLineDirection = *IBM405_GPIO0_TCR;  /* bits 30 - 7 */
  printf("\n");
  printf(" CS4 - access to VirtexII, CS1 & CS2 VirtexII DDR SDRAM\n");
  printf(" IRQ4 - VirtexII interrupt line to 405\n");
  printf("\n");
  printf("GPIO usage: 2 - FPGA Configuration enable 0-configure FPGA 1-when not\n");
  printf("            3 - FPGA Conf_Done line\n");
  printf("            4 - FPGA Conf_Prog line\n");
  printf("            5 - FPGA Conf_CLK line\n");
  printf("            6 - FPGA Conf_D0 line\n");
  printf("            7 - FPGA Conf_INIT line\n");
  printf("\n");
  printf("           24 - Quick Switch to allow reading of Type & Board Address, 0-to read, 1-when not\n");
  printf("        16:14 - Type 0-master,1-rf,2-loc,3-pfg,4-gradient,5-ddr\n");
  printf("        12:8 -  Board Ordinal Number 0-31\n");
  printf("\n");

  printf("Bit Type: CPC0_CR0: 0x%lx\n",(unsigned long) CPC0_CR0_value);
  printf("  1-9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24\n");
  result = (CPC0_CR0_value & 0x0800000) ? "TRACE" : "GP1-9";
  printf("%s",result);
  for(i=5; i < 12; i++)
  {
      bit = (1 << (31-i));
      if ((CPC0_CR0_value & bit))
      {
          result = " GP";
          printf("%s%d",result,i+5);
      }
      else
      {
          result = " CS";
          printf("%s-%d",result,i-4);
      }
  }
  for(i=12; i < 20; i++)
  {
      bit = CPC0_CR0_value >> (31-i);
      if ((bit & 1))
      {
          result = " GP";
          printf("%s%d",result,i+5);
      }
      else
      {
          result = " IRQ";
          printf("%s%d",result,i-12);
      }
  }
  printf("\n\n");

  prtGpioDirection(gpioLineDirection);
  output = *IBM405_GPIO0_OR;
  prtGpioOutput(gpioLineDirection,output);
  input = *IBM405_GPIO0_IR;
  prtGpioInput(gpioLineDirection,input);

  return;
}
