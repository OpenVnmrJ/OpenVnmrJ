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

/* RF FPGA SPI FIFO interface to the iCAT SPI */
/* Author: Greg Brissey  1/20/2010 */

/*
DESCRIPTION

   functions to handle RF FPGA SPI FIFO to the iCAT SPI

*/

#include <vxWorks.h>
#include "icatSpiFifo.h"
#include "rf.h"
#include "nvhardware.h"
#include "icatSpi.h"
#include "icat_reg.h"
static UINT32 spi(UINT32 word);

int icatDebug = -2;

int icatDebugOn() { return icatDebug = 3; }
int icatDebugOff() { return icatDebug = -2; }

// define for the spi emulation software, to emulate the access to the iCAT ISF, etc..
// #define EMULATION_ON

int getIcatId()
{
   int icatID;
   // 1st enable icat mode, to be able to query the icat register 0
   set_field(RF,transmitter_mode,1);   
   icatID = icat_spi_read((UINT32) iCAT_ID); // expect 0xa
   return icatID;
}

/* set the reboot register to one, 
 * causing the iCAT FPGA to reload The Primary FPGA Image
 */
int resetIcat()
{
  // 1st enable icat mode, to be able to query the icat register 0
  set_field(RF,transmitter_mode,1);   
  icat_spi_write((UINT32) iCAT_Reboot,(UINT32) 1);
   return 0;
}

/*
 * perform the reboot of the iCAT board,
 * waiting for the board to complete it's FPGA reload
 * prior to returning.
 * return 0 for success,  -1 for failure
 */
int rebootIcat()
{
    int cntdwn,status,ID;
    status = 0;
    cntdwn = 10;
    resetIcat();
    taskDelay(calcSysClkTicks(166));   // 10 ticks or 166 msec
    while ((ID = getIcatId()) != 0xa ) 
    {
       // printf("ID = 0x%lx\n", ID);
       if (cntdwn-- < 0)
       {
         status = -1;
         break;
       }
       taskDelay(10);
       // printf("waiting...\n");
    }
    return status;
}

/*
 * return the iCAT SPI ISF status register value
 */
int getIcatSpiStatus()
{
  return spi_isf_status();
}

int writeIcatFifo(UINT32 *buffer, UINT32 bytes)
{
    UINT32 i;
    UINT32 remainder, words;
    int status;

    status = 0;

    // since in bytes be sure to round up in words, i.e. 1 bytes -> 1 word not Zero
     remainder = bytes % sizeof(long);
     words = bytes / sizeof(long);
     words = (remainder) ? words + 1 : words;

     spi_isf_csb_low(); // beginning of data

    for (i=0; i < words; i++)
    {
      // into the FIFO , and out to the iCAT 
      IPRINT2(10,"writeIcatFifo:Addr: 0x%lx, word 0x%lx\n", buffer, *buffer);
      spi(*buffer++);
    }
    spi_isf_csb_high(); // force spi frame low, i.e. end of transmission
    status = spi_isf_complete(); // wait until ISFlash command is completed 
    return status;
}

int readIcatFifo(UINT32 cmd, UINT32 *buffer, UINT32 bytes)
{
    UINT32 i;
    UINT32 remainder, words;

    spi_isf_csb_low(); // beginning of data

    // transmit read cmd
    IPRINT1(10,"readIcatFifo: cmd: 0x%lx\n",cmd);
    spi(cmd);

    // since in bytes be sure to round up in words, i.e. 1 bytes -> 1 word not Zero
    remainder = bytes % sizeof(long);
    words = bytes / sizeof(long);
    words = (remainder) ? words + 1 : words;

    for (i=0; i < words; i++)
    {
      // out from the FIFO  
      *buffer++ = spi(0xFFFFFFFF);
      IPRINT2(10,"readIcatFifo: Addr: 0x%lx, word 0x%lx\n",buffer-1,*(buffer-1));
    }
    spi_isf_csb_high(); // force spi frame low, i.e. end of transmission
}

/*
 * this spi use the low level bit banging spi interface on th FPGA
 * to send data to the Icat RF
 */
static UINT32 spi(UINT32 word)
{
   UINT32 recvdata;
   recvdata = spi_isf_send(word,32);
   IPRINT1(3,"spi: returned: 0x%lx\n",recvdata);
   return recvdata;
}
