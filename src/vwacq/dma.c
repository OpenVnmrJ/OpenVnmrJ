/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dma.c 11.1 07/09/07  - DMA routines  for 162 */

/*
modification history
--------------------
1-20-95,gmb  created 

3-17-95 - large changes to get things working, however it doesn't appear
	  it is realible, when using vxgdb DMA works OK, but from the
	  shell sometimes it does and sometimes it doesn't.
	  (it maybe the STM?)
*/

/*
DESCRIPTION
  MVME-162 DMA
            
          
  Module:     dma.c
                 
  Function:  This module handles the MVME-162/167 on-board DMA
  Resources:   VMEchip2
           
*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h> 
#include <semLib.h> 
#include <stdioLib.h>
#include <intLib.h>
#include <logLib.h>

/* only used for 68k mv162 if PPC don't compile */
#if (CPU == MC68040)

#include <../config/mv162/mv162.h> 
#include <drv/multi/pccchip2.h> 
#include <drv/vme/vmechip2.h> 
#include <iv.h> 
#include <vme.h> 

#include "dma.h" 
#include "logMsgLib.h"
 
/*******************************
 * Module Defines              * 
 *******************************/
#define DMA_STATUS_MASK 0x7f  /* Mask to get the DMA status register bits */

#undef MUTUAL_EXCLUSION
/* 
#define M_L(n) (*(volatile signed long int*)(n))
#define M_UL(n) (*(volatile unsigned long int*)(n))
*/
#define M_L(n)  (* ((volatile signed long* const) (n)))
#define M_UL(n)  (* ((volatile unsigned long* const) (n)))

/*******************************
 * Local Functions             * 
 *******************************/

static void dmaInt(void);
static long dmaIt(unsigned long localadr, unsigned long vmeadr, unsigned long byteCount, unsigned long dmaCR2);

/*******************************
 * Local Variables             * 
 *******************************/
static unsigned long tempReg;
static SEM_ID	dmaCompleteSem = NULL;
static int	sysclkrate;
static int      lastDMAstatus;

#ifdef  MUTUAL_EXCLUSION
static SEM_ID	dmaAccessSem = NULL;
#endif

/*******************************
 * Module Routines             * 
 *******************************/
/**************************************************************
*
*  dmaInit - InitialIze The DMA Controller
*
*  Setup dma registers that are usually not changed on successive 
*dma operations.
*
* RETURNS:
* int status  OK
*
*/ 
int dmaInit()
{
   long Temp;

   /* save system clock rate for faster access */
   sysclkrate = sysClkRateGet();

   /* set DMAC on and off times (not currently used since we use DMACR1_BRX
        to release only on other bus requests) */
   DPRINT1(1,"VMECHIP2_TIMEOUTCR: 0x%lx\n",M_L(VMECHIP2_TIMEOUTCR));

  /* set DMA controller done interrupt level to 6. Already configured to
        use base vector 0x50 (actual vector 0x56) by VxWorks init. 
  */
  /* Set DMA interrupt to level 1. */
  M_L(VMECHIP2_ILR2) = (M_L(VMECHIP2_ILR2) & 0xf8ffffff) | ILR2_DMA_LEVEL1;
  DPRINT1(1,"VMECHIP2_ILR2: 0x%lx\n",M_L(VMECHIP2_ILR2));

   /* clear out the direct registers */
   M_L(VMECHIP2_DMACVAC) = 0L;	/* vme addr */
   M_L(VMECHIP2_DMACLBAC) = 0L;	/* local addr */
   M_L(VMECHIP2_DMACBC) = 0L;   /* count */

  /* Clear Any lingering DMA interrupts */
  M_UL(VMECHIP2_ICLR) = ICLR_CDMA;

  /* TimeOff Bus (TIMEOUTCR_OFF_0US), TimeOn Bus (TIMEOUTCR_ON_DONE) for DMA */
  /* If one used the time on & time off to throttle the bandwidth the DTM
     can't handle the on/off Address transaction that happens when DMA
     comes back on.
  */
  M_UL(VMECHIP2_TIMEOUTCR) &= ~(TIMEOUTCR_OFF_1024US); /* TIMEOUTCR_OFF_0US */
  M_UL(VMECHIP2_TIMEOUTCR) |= TIMEOUTCR_ON_DONE; 

#ifdef  MUTUAL_EXCLUSION
  /* Create a semaphore for mutual exclusion access to DMA */
  dmaAccessSem = semBCreate(SEM_Q_FIFO, SEM_FULL);

#endif

  /* Create a semaphore to signal dma complete */
  if (dmaCompleteSem == NULL)
  {
     dmaCompleteSem = semBCreate(SEM_Q_FIFO, SEM_FULL);

     /* base 50 + 6 */
     if(intConnect (INUM_TO_IVEC(UTIL_INT_VEC_BASE0 + LBIV_DMAC),
             dmaInt, NULL) == ERROR)
        return (ERROR);
  }

  return(OK);
 
}

/**************************************************************
*
*  dma - DMA's Data Between VME Bus and Local Bus 
*
* Initiate a DMA transfer between the local bus and the VME bus, in either
* direction. This function returns immediately -- when the DMA is complete
* an interrupt will be generated to the service routine 'isr'. That
* routine must call dma_int_clear() to clear the interrupt request.
* The function pointer passed for 'isr' should be the result of an
* intHandlerCreate() call, i.e. we don't call intHandlerCreate() in this
* routine because it would cause a memory leak (also doesn't seem to work
* at interrupt level).
* 
* 'Source' is the start address to copy from. It should be a VME 
*            address if 'tovme' is FALSE, otherwise it must be a local bus
*            address that does not map to the VME bus. VME addresses in the
*            range 0xf0000000 to 0xf0ffffff are mapped to the A24 address 
*            space, else uses A32. All addresses must be long-word aligned.
* 'Destination' is the destination address to copy to. It should be a VME
*             address if 'tovme' is TRUE, otherwise it must be a local bus
*             address that does not map to the VME bus. VME addresses in the
*             range 0xf0000000 to 0xf0ffffff are mapped to the A24 address 
*             space, else uses A32. All addresses must be long-word aligned.
* 'byteCount' is the number of bytes to copy. Must be a multiple of 4.
* 'XfrDir' LOCAL_TO_VME means copy from local bus to VME bus, 
*	   else copy from VME bus to local bus.
* 'AMcode' The STD (A24) or EXT (A32) Address Modifier Code for the type of 
*	   Transfer to be performed. 
*	   (i.e. VME_AM_STD_SUP_ASCENDING -> A24/D32 BLT tranfer, etc.. )
* 'timeOut' WAIT_FOREVER, NO_WAIT, TIME_OUT in Secounds
* RETURNS:
* int status  OK or DMA error code
*
*/
int dma(ulong_t Source,ulong_t Destination, ulong_t byteCount, 
	int XfrDir, int AMcode, int timeOut)
/* Source - Source Address */
/* Destination - Destination Address */
/* byteCount - Number of Bytes to Transfer */
/* XfrDir -  Direction of DMA (LOCAL_TO_VME, VME_TO_LOCAL) */
/* AMcode -  A24/A32-D16/D32/D64, NoBlock, BLT(D32BLOCK), MBLT (D64BLOCK) */
/* timeOut - WAIT_FOREVER, NO_WAIT, TIME_OUT in Secounds */
{
long status;
long sec;
unsigned long dmaCR2;
unsigned long localaddr,vmeaddr;
volatile long* pVMEchip2;

#ifdef  MUTUAL_EXCLUSION
  /* Obtain mutual exclusion semaphore for access to DMA */
  if (semTake(dmaAccessSem, WAIT_FOREVER) != OK)
    printf("DMA Access Semaphore failed\n");
#endif

 /* Wait at semaphore for completion of DMA before proceding with this DMA */

 DPRINT(-1,"Wait for Cmplt prior to DMA, 20 sec then timeout.\n");
 if ( semTake(dmaCompleteSem, (sysclkrate * 20)) != OK )
 {
       return (DMA_TIMED_OUT);
 }
 dmaCR2 = 0L; /* this will contain the amCode, xfrMode, incVME,direction  */

  /* determine direction and local & vme address  */
  if( XfrDir == VME_TO_LOCAL )
  {
  /* Transfer from VMEbus to local bus. */
  /* Clear direction flag (TVME) in DMA control reg 2. */
     dmaCR2 = 0L;

     /* Load source address into DMAC VME bus address counter. */
     M_UL(VMECHIP2_DMACVAC) = (volatile unsigned long) Source;

      /* Load destination address into DMAC Local Bus address counter. */
      M_UL(VMECHIP2_DMACLBAC) = (volatile unsigned long) Destination;
  }
  else
  {
      /* Transfer from local bus to VMEbus. */
      /*  Set direction flag (TVME) in DMA control reg 2. */
      dmaCR2 = DMACR2_TO_VME;

      /* Load destination address into DMAC VME bus address counter. */
      M_UL(VMECHIP2_DMACVAC) = (volatile unsigned long) Destination;

      /* Load source address into DMAC Local Bus address counter. */
      M_UL(VMECHIP2_DMACLBAC) = (volatile unsigned long) Source;
  }

  /* Load DMAC byte counter with count of bytes in transfer. */
  M_UL(VMECHIP2_DMACBC) = (volatile unsigned long) byteCount;

  /* 
       From the Address Modifier (AM) code deteremine the type
       of transfer. A24/D32 No Block, A24/D32 BLT, A24/D64 MBLT
		    A32/D32 No Block, A32/D32 BLT, A32/D64 MBLT
  */
   switch( AMcode )
   {
      case VME_AM_STD_SUP_ASCENDING :
      case VME_AM_STD_USR_ASCENDING :
      case VME_AM_EXT_SUP_ASCENDING :
      case VME_AM_EXT_USR_ASCENDING :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_D32_BLOCK);
	   break;

      case VME_AM_STD_SUP_PGM :
      case VME_AM_STD_SUP_DATA :
      case VME_AM_STD_USR_PGM :
      case VME_AM_STD_USR_DATA :
      case VME_AM_EXT_SUP_PGM :
      case VME_AM_EXT_SUP_DATA :
      case VME_AM_EXT_USR_PGM :
      case VME_AM_EXT_USR_DATA :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_NO_BLOCK);
	   break;

      case VME_AM_STD_SUP_D64_BLOCK :
      case VME_AM_STD_USR_D64_BLOCK :
      case VME_AM_EXT_SUP_D64_BLOCK :
      case VME_AM_EXT_USR_D64_BLOCK :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_D64_BLOCK);
	   break;

      default:
	   printf("dma: Unknown AM Code: 0x%x\n",AMcode);
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_NO_BLOCK);
	   break;

   }
   /* Turn on VINC and LINC in DMA control reg 2 
         (VINC - increment VMEbus address counter during transfers. 
	  LINC - increment local bus address counter during transfers).
	 Also, set SNP (snoop) bits to write - sink data, 
	 read - supply dirty data and leave dirty.
	 Finally, place given address modifier in control reg. 
   */
   dmaCR2 |= DMACR2_SINK_DATA | DMACR2_VINC |  DMACR2_LINC;
   M_UL(VMECHIP2_DMACR2) = (volatile unsigned long) dmaCR2;


   /* Clear any old DMAC interrupts. */
   M_UL(VMECHIP2_ICLR) |= (volatile unsigned long) ICLR_CDMA;

   /* Enable DMAC interrupt. */
   M_UL(VMECHIP2_LBIER) |= (volatile unsigned long) LBIER_EDMA;

   /* Set DEN - enable DMAC, 
      Set DRELM = 1 - Release mode is 
       'release when the timer has expired or when bus request 
	signal is active'. 
   */
   M_UL(VMECHIP2_DMACR1) |= 
		(volatile unsigned long) (DMACR1_DEN | DMACR1_TIMER_OR_BRX);

  if (timeOut == NO_WAIT)
  {
     return(OK);
  }
  else
  {
    /* if wait_forever then set time for 10 Mins */
    sec =  (timeOut == WAIT_FOREVER) ? 600 : timeOut;

    /* Wait at semaphore for completion of DMA */
    DPRINT(1,"Wait for this DMA to Cmplt DMA\n");
    if ( semTake(dmaCompleteSem, (sysclkrate * sec)) != OK )
    {
       return (DMA_TIMED_OUT);
    } 
    lastDMAstatus = status = (M_L(VMECHIP2_ICR) & 0x7f);
    semGive(dmaCompleteSem);
  }

  if (status & ICR_DONE)
  {
    return(OK);
  }
  else if (status & ICR_VME)
  {
    return(VME_BUS_ERROR);
  }
  else if (status & ICR_DLTO)
  {
    return(LOCAL_TIMEOUT);
  }
  else if (status & ICR_DLOB)
  {
    return(OFF_BD_TEA);
  }
  else if (status & ICR_DLPE)
  {
    return(PARITY_ERROR);
  }
  else if (status & ICR_DLBE)
  {
    return(TEA);
  }
  else
  {
    return(ERROR);
  }
}

#ifdef XXX
/**************************************************************
*
*  dmaIt - internal routine to DMA's Data Between VME Bus and Local Bus 
*
*
* RETURNS:
* int status  OK
*
*/
static long dmaIt(unsigned long localadr, unsigned long vmeadr, unsigned long byteCount, unsigned long dmaCR2)
{
   static long cmdtbl[5];   /* command table for DMAC */
   long status;
   int semStatus;
   long TempReg;

   /* initialize VME bus address counter */
   cmdtbl[2] = vmeadr;
   
   /* initialize local bus address counter */
   cmdtbl[1] = localadr;
   
   /* initialize VME bus byte counter */
   cmdtbl[3] = byteCount;
   
   /* DMA CR2: amCode, Transfer Mode, always increment local bus address, 
  	  always increment VME bus address, interrupt when DMA command done */
   cmdtbl[0] = dmaCR2 | DMACR2_SINK_DATA | DMACR2_VINC | 
			DMACR2_LINC | DMACR2_INTE;

   /* fill 'next' table address with stop flag (only one entry in chain) */
   cmdtbl[4] = 0x03;

   /* initialize table address counter */ 
   M_L(VMECHIP2_TAC) = (long) cmdtbl;

   /* clear & enable the DMA-done interrupt */
   M_L(VMECHIP2_ICLR) = ICLR_CDMA;
   TempReg = M_L(VMECHIP2_LBIER);
   TempReg = TempReg | LBIER_EDMA;
   M_L(VMECHIP2_LBIER) = TempReg;
/*
   M_L(VMECHIP2_LBIER) = M_L(VMECHIP2_LBIER)| LBIER_EDMA;
*/

   /* DMA CR1: Use VMEbus request level 1, release on BR, non-fair, command
        chaining mode (TBL), and start (enable) DMA transfer.
        Note: the single-transfer bus requester is programmed for request level
        3, and we release the DMAC requester on other bus requests so that
        single-transfer bus requests can get on the VME bus quickly. */
 /*
   M_L(VMECHIP2_DMACR1) = (M_L(VMECHIP2_DMACR1) & 0xffffff00)
                            | DMACR1_DREQ_L1 | DMACR1_BRX
                            | DMACR1_DTBL | DMACR1_DEN;
*/

    TempReg = M_L(VMECHIP2_DMACR1);
    printf("VMECHIP2_DMACR1: 0x%lx\n",TempReg);
    TempReg = (TempReg & 0xffffff00) | DMACR1_DREQ_L1 | DMACR1_BRX |
				       DMACR1_DTBL | DMACR1_DEN;
    printf("VMECHIP2_DMACR1: 0x%lx\n",TempReg);
    M_L(VMECHIP2_DMACR1) = TempReg;

    return(ICR_DONE);
}
#endif

/*****************************************************************************
* HANDLE DMA INTERRUPTS                                		             *
*                                                                            *
* Routine:          dmaInt()                                                 *
*                                                                            *
* Description:      This routine handles DMA interrupts
*                                                                            *
* Return param:     none                                                     *
*                                                                            *
* Characteristics:        
*                                                                            *
******************************************************************************/

static void dmaInt(void)
{
   long status;
  /* Clear DMA interrupt */
   M_UL(VMECHIP2_ICLR) |= ICLR_CDMA;
 
     /* Disable DMAC interrupt. */
   M_UL(VMECHIP2_LBIER) &= ~(LBIER_EDMA);;


  lastDMAstatus =  status = (M_L(VMECHIP2_ICR) & DMA_STATUS_MASK);
  if (status != 1)
    logMsg("DMA Error: 0x%lx\n",status,2,3,4,5,6);

  if(semGive(dmaCompleteSem) == ERROR)
    logMsg("Error on posting to DMA Complete semaphore\n\n^g",1,2,3,4,5,6);

#ifdef  MUTUAL_EXCLUSION
  /* Give mutual exclusion semaphore for access to DMA */
  if (semGive(dmaAccessSem) == ERROR)
    logMsg("Error on posting to DMA Access semaphore\n\n^g",1,2,3,4,5,6);
#endif

  DPRINT(1,"DMA Intrp\n");
}

dmaShow(int level)
{
     long *cmdtbl;   /* command table for DMAC */
     long TempReg;
     int tmpval;

     TempReg = (M_L(VMECHIP2_ICR) & 0x7f);
     printDmaStatus(TempReg);
     printf("Direct - Local Bus Addr: 0x%lx\n",M_L(VMECHIP2_DMACLBAC));
     printf("Direct - VME   Bus Addr: 0x%lx\n",M_L(VMECHIP2_DMACVAC));
     printf("Direct - Bytes to  Xfer: %ld\n",M_L(VMECHIP2_DMACBC));
/*
 *              DMAC Control Register 2                         0x34    15-08
 *              DMAC Control Register 2                         0x34    07-00
 */
    TempReg = M_L(VMECHIP2_DMACR2); /* 34 */
    printDMACR2(TempReg);

    
/*
 *              Table Address Counter                           0x44    31-00
 */
    TempReg = M_L(VMECHIP2_TAC); /* 44 command packet table addr */
    printf("Command Packet Table Address: 0x%lx\n",TempReg);
    if ( (TempReg != NULL) && (TempReg != 0x3) )
    {
       cmdtbl = (long *) TempReg;
       printf("Cmd Packet - Local Bus Addr: 0x%lx\n",cmdtbl[1]);
       printf("Cmd Packet - VME   Bus Addr: 0x%lx\n",cmdtbl[2]);
       printf("Cmd Packet - Bytes to  Xfer: %ld\n",cmdtbl[3]);
       printDMACR2(cmdtbl[0]);
    }

    if (level > 0)
    {

/*
 *              EPROM Decoder, SRAM and DMA Control Register    0x30    23-16
 *              Local Bus To VMEbus Requester Control Reg       0x30    15-08
 *              DMAC Control Register 1                         0x30    07-00
 */
    TempReg = M_L(VMECHIP2_DMACR1); /* 30 */
    printDMACR1(TempReg);


/*
 *              VMEbus Interrupter Control Register             0x48    31-24
 *              VMEbus Interrupter Vector Register              0x48    23-16
 *              MPU Status and DMA Interrupt Count Register     0x48    15-08
 *              DMAC Status Register                            0x48    07-00
 */
    TempReg = M_L(VMECHIP2_ICR); 
    printDMAICR(TempReg);
    
/*
 *              VMEbus Arbiter Timeout Control Register         0x4c    31-24
 *              DMAC Ton/Toff Timers and VMEbus Global Timeout  0x4c    23-16
 *              VME Access, Local Bus and Watchdog Timeout      0x4c    15-08
 *              Prescaler Control Register                      0x4c    07-00
 */
    TempReg = M_L(VMECHIP2_TIMEOUTCR); 
    printDMATimeOutCR(TempReg);
    
/*
 *              Tick Timer 1 Compair Register                   0x50    31-00
 */
    TempReg = M_L(VMECHIP2_TTCOMP1); /* 44 command packet table addr */
    printf("DMA TTCOMP1: %ld (0x%lx)\n",TempReg,TempReg);

/*
 *              Tick Timer 1 Counter                            0x54    31-00
 */
    TempReg = M_L(VMECHIP2_TTCOUNT1);
    printf("DMA TTCOUNT1: %ld (0x%lx)\n",TempReg,TempReg);


/*
 *              Tick Timer 2 Compair Register                   0x58    31-00
 */
    TempReg = M_L(VMECHIP2_TTCOMP2);
    printf("DMA TTCOMP2: %ld (0x%lx)\n",TempReg,TempReg);

/*
 *              Tick Timer 2 Counter                            0x5c    31-00
 */
    TempReg = M_L(VMECHIP2_TTCOUNT2);
    printf("DMA TTCOUNT2: %ld (0x%lx)\n",TempReg,TempReg);

/*
 *              Board Control Register                          0x60    31-24
 *              Watchdog Timer Control Register                 0x60    23-16
 *              Tick Timer 2 Control Register                   0x60    15-08
 *              Tick Timer 1 Control Register                   0x60    07-00
 */
    TempReg = M_L(VMECHIP2_TIMERCR);

   }  /* end of if (level > 0) */

}

printDmaStatus(long status)
{
  if (status == 0)
  {
    printf("DMA is being initialized, no transfer yet\n");
  }
  else if (status & ICR_DONE)
  {
    printf("DMA completed with no errors\n");
  }
  else if (status & ICR_VME)
  {
    printf("DMA  received a VME Bus Error (BERR) during transfer\n");
  }
  else if (status & ICR_TBL)
  {
    printf(
    "DMA error on Local Bus while reading commands from the command packet\n");
    printf("Cause of Error: \n");
    if (status & ICR_DLTO)
    {
      printf("                TEA, local bus timeout\n");
    }
    else if (status & ICR_DLOB)
    {
      printf("                TEA, offboard\n");
    }
    else if (status & ICR_DLPE)
    {
      printf("                TEA, Parity Error during Transfer\n");
    }
    else if (status & ICR_DLBE)
    {
      printf("                TEA\n");
    }
  }
  else if (status & ICR_MLTO)
  {
    printf("DMA MPU received a TEA with local bus timeout\n");
  }
}

printDMACR1(long TempReg)
{
/*
 *              EPROM Decoder, SRAM and DMA Control Register    0x30    23-16
 *              Local Bus To VMEbus Requester Control Reg       0x30    15-08
 *              DMAC Control Register 1                         0x30    07-00
 */
   printf("DMA Control Register 1:\n");
    switch ((TempReg >> 18) & 0x3)
    {
       case 0:
		printf("         Snoop inhibited\n");
		break;
       case 1:
		printf("         Write Sink data, Read dirty,leave dirty\n");
		break;
       case 2:
		printf("         Write Invalidate, Read dirty,mark invalid\n");
		break;
       case 3:
		printf("         Snoop inhibited\n");
		break;
    }
    if ( (TempReg &  DMACR1_ROBIN) )
      printf("         arbiter in round robin mode\n");
   else
      printf("         arbiter in priority mode\n");

   if ( (TempReg &  DMACR1_DTBL) )
      printf("         Command chaining mode\n");

   if ( (TempReg &  DMACR1_DFAIR) )
      printf("         Operates in fair mode\n");

    switch ((TempReg >> 2) & 0x3)
    {
       case 0:
		printf("         Release with Timer & BRx\n");
		break;
       case 1:
		printf("         Release with Timer\n");
		break;
       case 2:
		printf("         Release with BRx\n");
		break;
       case 3:
		printf("         Release with Timer or BRx\n");
		break;
    }
    switch (TempReg & 0x3)
    {
       case 0:
		printf("         VMEbus request Level 0\n");
		break;
       case 1:
		printf("         VMEbus request Level 1\n");
		break;
       case 2:
		printf("         VMEbus request Level 2\n");
		break;
       case 3:
		printf("         VMEbus request Level 3\n");
		break;
    }
}
printDMACR2(long TempReg)
{

   printf("DMA Control Register 2:\n");

    if ( (TempReg &  DMACR2_TO_VME) )
      printf("         Transfer to VMEbus  (Local --> VME)\n");
    else
      printf("         Transfer to Local bus (VME --> Local)\n");

    if ( (TempReg &  DMACR2_D16) )
      printf("         D16 on the VMEbus\n");
    else
      printf("         D32/D64 on the VMEbus\n");

    /* Block Transfer Mode */
    switch ((TempReg >> 6) & 0x3)
    {
       case 0:
		printf("         Block Transfers Disabled\n");
		break;
       case 1:
		printf("         D32 Block Transfer VMEbus\n");
		break;
       case 3:
		printf("         D64 Block Transfer VMEbus\n");
		break;
    }
    printf("         AM code: 0x%x\n",TempReg & 0x3f);

    if ( (TempReg &  DMACR2_VINC) )
      printf("         Increment VME Address on DMA\n");
    else
      printf("         Do NOT Increment VME Address on DMA\n");

    if ( (TempReg &  DMACR2_LINC) )
      printf("         Increment Local Address DMA\n");
    else
      printf("         Do NOT Increment Local Address on DMA\n");

    if ( (TempReg &  DMACR2_INTE) )
      printf("         Interrupt Enabel\n");

    switch ((TempReg >> 13) & 0x3)
    {
       case 0:
		printf("         Snoop Inhibited\n");
		break;
       case 1:
		printf("         Sink Data\n");
		break;
       case 2:
		printf("         Invalidate\n");
		break;
    }
}

printDMAICR(long TempReg)
{
    switch ((TempReg >> 29) & 0x3)
    {
       case 0:
		printf("DMA ICR: Intrptr connected to IRQ1\n");
		break;
       case 1:
		printf("DMA ICR: Timer 1 connected to IRQ1\n");
		break;
       case 3:
		printf("DMA ICR: Timer 2 connected to IRQ2\n");
		break;
    }
    if ( TempReg & ICR_IRQ_STATUS )
      printf("DMA ICR: Outstanding IRQ not acknowledged\n");
    else
      printf("DMA CR2: No Outstanding IRQ\n");
}
printDMATimeOutCR(long TempReg)
{
     int tmpval;

    printf("DMA Timeout Cntrl Register:\n");
    if ( TempReg & TIMEOUTCR_ARBTO )
      printf("   Enable Grant Timeout\n");
    else
      printf("   Disable Grant Timeout\n");

    tmpval = ((TempReg >> 21) & 0x3);
    printf("   DMAC off VMEbus for: %d\n",
	(tmpval == 0) ? 0 : (16 * ( 1 << (tmpval-1))) ); /* 0 - 1024 usec */
    tmpval = ((TempReg >> 18) & 0x7);
    if ( tmpval == 7 )
    {
       printf("   DMAC on VMEbus until done\n");
    }
    else
      printf("   DMAC on VMEbus for: %d\n",
      (tmpval == 0) ? 16 : (32 * ( 1 << tmpval )));

    switch ((TempReg >> 16) & 0x3)
    {
       case 0:
		printf("   VMEbus global timeout 8 usec\n");
		break;
       case 1:
		printf("   VMEbus global timeout 16 usec\n");
		break;
       case 2:
		printf("   VMEbus global timeout 256 usec\n");
		break;
       case 3:
		printf("   VMEbus global disabled\n");
		break;
    }
    

    switch ((TempReg >> 14) & 0x3)
    {
       case 0:
		printf("   VMEbus Access Timeout 64 usec\n");
		break;
       case 1:
		printf("   VMEbus Access Timeout 1 msec\n");
		break;
       case 2:
		printf("   VMEbus Access Timeout 32 msec\n");
		break;
       case 3:
		printf("   VMEbus Access Timeout disabled\n");
		break;
    }
    
    switch ((TempReg >> 12) & 0x3)
    {
       case 0:
		printf("   Local Bus timeout 8 usec\n");
		break;
       case 1:
		printf("   Local Bus timeout 64 usec\n");
		break;
       case 2:
		printf("   Local Bus timeout 256 usec\n");
		break;
       case 3:
		printf("   Local Bus timeout disabled\n");
		break;
    }

    tmpval = (int) ((TempReg >> 8) & 0xF);
    if (tmpval == 0 )
    {
       printf("   Watchdog Timeout 512 usec\n");
    }
    else if ( (tmpval > 0) && (tmpval < 11) )
    {
       printf("   Watchdog Timeout %d msec\n",
		( 1 * (1 << (tmpval-1)) ) );
    }
    else if ( (tmpval > 10) && (tmpval < 16) )
    {
       printf("   Watchdog Timeout %d sec\n",
		( 1 * (1 << (tmpval-11)) ) );
    }
    
}
/*****************************************************************************
*  bcopyDMA( source, dest, blockSize, AMcode, dirFlag, waitFlag )
*  Transfer a block using direct memory access over VMEbus.
*  input:
*        int *source    - start address of block to transfer.
*        int *dest      - start address of destination.
*        int block_size - Size in bytes of block to transfer.
*        char AMcode - VME address modifier code.
*        char dirFlag   - Direction of transfer flag
*                         ( 0 = Transfer from local memory to VMEbus.
*                           1 = Transfer fro VMEbus memory to local.)
*        
*        char waitFlag  - wait flag 
*                        ( 0 = don't wait for completion of transfer. Enable interrupt.
*                          1 = wait for completion of transfer. No interrupts ).
*  returns:
*        OK - transfer initiated OK / ERROR - transfer failed
*****************************************************************************/
int bcopyDMA(INT32 *source,INT32 *dest, INT32 block_size, int AMcode, 
			   int dirFlag, int waitFlag)
{
     unsigned int *regs;
     unsigned long dmaCR2;
     INT32  ix;

     volatile INT32   *pVMEchip2;
 
     dmaCR2 = 0L; /* this will contain the amCode, xfrMode, incVME,direction  */

     /* Take the semaphore to prevent starting transfer while transfer already
	in process. The task will wait forever until semaphore becomes available. */

     if (semTake( dmaCompleteSem, WAIT_FOREVER ) == ERROR)
	  return( ERROR );

     /* Set up registers to begin DMA transfer. */

     if( dirFlag == VME_TO_LOCAL )
     {
	  /* Transfer from VMEbus to local bus. */
	  /* Clear direction flag (TVME) in DMA control reg 2. */
          dmaCR2 = 0L;

	  /* Load source address into DMAC VME bus address counter. */
	  pVMEchip2 = VMECHIP2_DMACVAC;
	  *pVMEchip2 = (volatile INT32)source;

	  /* Load destination address into DMAC Local Bus address counter. */
	  pVMEchip2 = VMECHIP2_DMACLBAC;
	  *pVMEchip2 = (volatile INT32)dest;
     }
     else
     {
	  /* Transfer from local bus to VMEbus. */
	  /*  Set direction flag (TVME) in DMA control reg 2. */
          dmaCR2 = DMACR2_TO_VME;

	  /* Load destination address into DMAC VME bus address counter. */
	  pVMEchip2 = VMECHIP2_DMACVAC;
	  *pVMEchip2 = (volatile INT32)dest;

	  /* Load source address into DMAC Local Bus address counter. */
	  pVMEchip2 = VMECHIP2_DMACLBAC;
	  *pVMEchip2 = (volatile INT32)source;
     }

     /* Load DMAC byte counter with count of bytes in transfer. */
     pVMEchip2 = VMECHIP2_DMACBC;
     *pVMEchip2 = block_size; 

  /* 
       From the Address Modifier (AM) code deteremine the type
       of transfer. A24/D32 No Block, A24/D32 BLT, A24/D64 MBLT
		    A32/D32 No Block, A32/D32 BLT, A32/D64 MBLT
  */
   switch( AMcode )
   {
      case VME_AM_STD_SUP_ASCENDING :
      case VME_AM_STD_USR_ASCENDING :
      case VME_AM_EXT_SUP_ASCENDING :
      case VME_AM_EXT_USR_ASCENDING :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_D32_BLOCK);
	   break;

      case VME_AM_STD_SUP_PGM :
      case VME_AM_STD_SUP_DATA :
      case VME_AM_STD_USR_PGM :
      case VME_AM_STD_USR_DATA :
      case VME_AM_EXT_SUP_PGM :
      case VME_AM_EXT_SUP_DATA :
      case VME_AM_EXT_USR_PGM :
      case VME_AM_EXT_USR_DATA :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_NO_BLOCK);
	   break;

      case VME_AM_STD_SUP_D64_BLOCK :
      case VME_AM_STD_USR_D64_BLOCK :
      case VME_AM_EXT_SUP_D64_BLOCK :
      case VME_AM_EXT_USR_D64_BLOCK :
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_D64_BLOCK);
	   break;

      default:
	   printf("dma: Unknown AM Code: 0x%x\n",AMcode);
		dmaCR2 |= (AMcode) |  DMACR2_D32 | (DMACR2_NO_BLOCK);
	   break;

   }
   /* Turn on VINC and LINC in DMA control reg 2 
         (VINC - increment VMEbus address counter during transfers. 
	  LINC - increment local bus address counter during transfers).
	 Also, set SNP (snoop) bits to write - sink data, 
	 read - supply dirty data and leave dirty.
	 Finally, place given address modifier in control reg. 
   */
   dmaCR2 |= DMACR2_SINK_DATA | DMACR2_VINC |  DMACR2_LINC;
   pVMEchip2 = VMECHIP2_DMACR2;
   *pVMEchip2 = dmaCR2;

  DPRINT(1, " About to enable Interrupts\n" );

  /* Clear any old DMAC interrupts. */
  pVMEchip2 = VMECHIP2_ICLR;
  *pVMEchip2 |= ICLR_CDMA;

  /* Enable DMAC interrupt. */
  pVMEchip2 = VMECHIP2_LBIER;
  *pVMEchip2 |= LBIER_EDMA;

   DPRINT(1, " Interrupt enabled\n" );

     /* Set DEN - enable DMAC, 
        Set DRELM = 1 - Release mode is 
	'release when the timer has expired or when bus request signal is active'. */
     pVMEchip2 = VMECHIP2_DMACR1;
     *pVMEchip2 |= (DMACR1_DEN | DMACR1_TIMER_OR_BRX);

      if ( waitFlag )
      {
	     DPRINT(1,"...WAITING for Intrp\n ");
             if (semTake( dmaCompleteSem, WAIT_FOREVER ) == ERROR)
	         return( ERROR );
	     DPRINT(1,"...Got Intrp\n ");
	     semGive( dmaCompleteSem );

            
	  return(OK);
     }
     else
     {
	  /* Interrupt handler will inform us when transfer is complete. */
	  return( OK );
     }
}

#ifdef DEBUG

/* extern PART_ID memSysPartId; */

dmaPrtError(int errnum)
{
  switch(errnum)
  {
     case DMA_TIMED_OUT:
	printf("10 min timeout waiting for dma to complete\n");
        break;

     case VME_BUS_ERROR:
	printf("VME Bus error occurred during transfer\n");
        break;

     case LOCAL_TIMEOUT:
	printf("local bus timed out during transfer\n");
        break;

     case OFF_BD_TEA:
	printf("off board TEA occurred during transfer\n");
        break;

     case PARITY_ERROR:
	printf("Parity error occurred during transfer\n");
        break;

     case TEA:
	printf("TEA occurred during transfer\n");
        break;

     default:
	printf("Illegal DMA Status\n");
        break;
  }
}

/* source, destination number of longs to transfer */
tstmemdma(int type,int size,int wcall)
{
    long *pData,*pReturn,*pVMEaddr, *pTmp,*pTmp2;
    unsigned long* value;
    unsigned int nbytes;
    int stat,i,ttype,AMcode,nerrs;

    dmaInit();
    switch(type)
    {
	case 1:
		ttype = D32;
		AMcode = VME_AM_EXT_SUP_DATA;
		break;
	case 2:
		ttype = BLK32;
		AMcode = VME_AM_EXT_SUP_ASCENDING;
		break;
	case 3:
		ttype = BLK64;
		AMcode = VME_AM_EXT_SUP_D64_BLOCK;
		break;
    }
    nbytes = size * sizeof(long);
    pData = (long*) memalign(4,nbytes);
    pReturn = (long*) memalign(4,nbytes);
    printf("pData: 0x%lx, mod4: %d\n",pData,((unsigned long)pData % 4));
    printf("pReturn: 0x%lx, mod4: %d\n",pReturn,((unsigned long)pReturn % 4));
    pVMEaddr = (long *) 0x1C000000L;
/*
    pData = (long*) memPartAlignedAlloc(memSysPartId,nbytes,4);
    pReturn = (long*) memPartAlignedAlloc(memSysPartId,nbytes,4);
*/

    /* for (i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x10000000L; i < size; i++) */
    for (i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x1C000000L; i < size; i++)
    {
     *pTmp++ = (long) value;
     *value++ = 0xa5a5a5a5L;
     *pTmp2++ = 0x5a5a5a5aL;
    }

    /* for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x10000000L; i < 20; i++) */
    for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x1C000000L; i < 20; i++)
    {
     printf("%d - Data[0x%lx]: 0x%lx , STM[0x%lx]: 0x%lx, Retur[0x%lx]n: 0x%lx\n",
		i,pTmp,*pTmp,value,*value,pTmp2,*pTmp2);
       pTmp++;
       pTmp2++;
       value++;
    }
  printf("pData: 0x%lx, pVme: 0x%lx, pReturn: 0x%lx\n",pData,pVMEaddr,pReturn);
  printf("dma(0x%lx, 0x%lx, %d, 0x%x, 0x%x, %d)\n",pData,pVMEaddr,nbytes,
			LOCAL_TO_VME,AMcode,120);
   if (wcall)
     stat = dma(pData,pVMEaddr, nbytes, LOCAL_TO_VME, AMcode, 120);
   else
     stat = bcopyDMA(pData,pVMEaddr, nbytes, AMcode, LOCAL_TO_VME, 1);

    /* for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x10000000L; i < 20; i++) */
    for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x1C000000L; i < 20; i++)
    {
     printf("%d - Data[0x%lx]: 0x%lx , STM[0x%lx]: 0x%lx, Retur[0x%lx]n: 0x%lx\n",
		i,pTmp,*pTmp,value,*value,pTmp2,*pTmp2);
       pTmp++;
       pTmp2++;
       value++;
    }
    if (stat != OK)
    {
       dmaPrtError(stat);
       free(pData); free(pReturn);
       return;
    }
   printf("pData: 0x%lx, pVme: 0x%lx, pReturn: 0x%lx\n",pData,pVMEaddr,pReturn);
  printf("dma(0x%lx, 0x%lx, %d, 0x%x, 0x%x, %d)\n",pVMEaddr,pReturn,nbytes,
			VME_TO_LOCAL,AMcode,120);
   if (wcall)
     stat = dma(pVMEaddr,pReturn, nbytes, VME_TO_LOCAL, AMcode, 120);
   else
     stat = bcopyDMA(pVMEaddr,pReturn, nbytes, AMcode, VME_TO_LOCAL, 1);

    /* for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x10000000L; i < 20; i++) */
    for(i=0,pTmp=pData,pTmp2=pReturn,value=(long*)0x1C000000L; i < 20; i++)
    {
     printf("%d - Data[0x%lx]: 0x%lx , STM[0x%lx]: 0x%lx, Retur[0x%lx]n: 0x%lx\n",
		i,pTmp,*pTmp,value,*value,pTmp2,*pTmp2);
       pTmp++;
       pTmp2++;
       value++;
    }
    if (stat != OK)
    {
       dmaPrtError(stat);
       free(pData); free(pReturn);
       return;
    }
    for(i=0,nerrs=0,pTmp=pVMEaddr,pTmp2=pReturn; i < size; i++)
    {
       if (nerrs > 20)
          break;
       if (*pTmp != *pTmp2)
       {
	 nerrs++;
         printf("%d - sent: 0x%lx <-- != --> returned; 0x%lx\n",
		i,*pTmp,*pTmp2);
       }
       pTmp++;
       pTmp2++;
    }

    free(pData); free(pReturn);
}
#endif


#else   /* (CPU == MC68040) */

int dmaInit()
{
   return(0);
}

int dma(ulong_t Source,ulong_t Destination, ulong_t byteCount, 
	int XfrDir, int AMcode, int timeOut)
{
   return(0);
}

int bcopyDMA(INT32 *source,INT32 *dest, INT32 block_size, int AMcode, 
			   int dirFlag, int waitFlag)
{
   return(0);
}

#endif /* (CPU == MC68040) */
