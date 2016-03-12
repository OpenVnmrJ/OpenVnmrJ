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
modification history
--------------------
5-14-04,gmb  created
*/

/*
DESCRIPTION

   Base FPGA ISR routines 

   This source is included in the source of the liked named source file for
   a controller, e.g. master.c includes it; lock.c includes it, etc.
   The master defines MASTER so that the ISR can properly be conditional compiled
   etc.
*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include "iv.h"
#include "intLib.h"
#include "sysLib.h"
 
#include "logMsgLib.h"
#include "nvhardware.h"
#include "instrWvDefines.h"
#include "fpgaBaseISR.h"

#if defined(DONT_INCLUDE_C_FILES) && defined(MASTER_CNTLR)
#  include "master.h"
#endif
#if defined(DONT_INCLUDE_C_FILES) && defined(RF_CNTLR)
#  include "rf.h"
#endif
#if defined(DONT_INCLUDE_C_FILES) && defined(DDR_CNTLR)
#  include "ddr.h"
#endif
#if defined(DONT_INCLUDE_C_FILES) && defined(GRADIENT_CNTLR)
#  include "gradient.h"
#endif
#if defined(DONT_INCLUDE_C_FILES) && defined(PFG_CNTLR)
#  if defined(PFG_LOCK_COMBO_CNTLR)
#     include "lpfg.h"
#  else
#     include "pfg.h"
#  endif
#endif
#if defined(DONT_INCLUDE_C_FILES) && defined(LOCK_CNTLR) && !defined(PFG_LOCK_COMBO_CNTLR)
#  include "lock.h"
#endif

// This allows multiple controllers to work without affecting legacy code too because
// legacy builds #include this file, while the new mechanism links it in and initializes
// the code from initBrdSpecific, while still retaining a common code base.
/* this is a constant pointer to r/w data */
#define __vpintreg(fpga,field) \
    volatile unsigned int *_p##field = (unsigned int *) (FPGA_BASE_ADR + fpga##_##field)
#define VPINTREG(fpga,field) __vpintreg(fpga,field)

/* controller specific interrupt register pointers  */
#ifdef MASTER_CNTLR
   VPINTREG(MASTER,InterruptStatus);
   VPINTREG(MASTER,InterruptEnable);
   VPINTREG(MASTER,InterruptClear);
#endif
#if defined(LOCK_CNTLR) && !defined(PFG_LOCK_COMBO_CNTLR)
   VPINTREG(LOCK,InterruptStatus);
   VPINTREG(LOCK,InterruptEnable);
   VPINTREG(LOCK,InterruptClear);
#endif
#if defined(PFG_CNTLR)
   VPINTREG(PFG,InterruptStatus);
   VPINTREG(PFG,InterruptEnable);
   VPINTREG(PFG,InterruptClear);
#endif
#if defined(GRADIENT_CNTLR)
   VPINTREG(GRADIENT,InterruptStatus);
   VPINTREG(GRADIENT,InterruptEnable);
   VPINTREG(GRADIENT,InterruptClear);
#endif
#if defined(DDR_CNTLR)
   #define DDR_InterruptStatus DDR_InterruptStatus_0
   #define DDR_InterruptEnable DDR_InterruptEnable_0
   #define DDR_InterruptClear DDR_InterruptClear_0
   VPINTREG(DDR,InterruptStatus);
   VPINTREG(DDR,InterruptEnable);
   VPINTREG(DDR,InterruptClear);
#endif
#if defined(RF_CNTLR)
   VPINTREG(RF,InterruptStatus);
   VPINTREG(RF,InterruptEnable);
   VPINTREG(RF,InterruptClear);
#endif

#if defined(PFG_LOCK_COMBO_CNTLR)
  #define FPGA_MAX_SLICE 3
#else
  #define FPGA_MAX_SLICE 1
#endif

static int          fpgaBaseISR_Installed[FPGA_MAX_SLICE] = {0};
static int          numRegISRs[FPGA_MAX_SLICE]      = { 0 };
static VOIDFUNCPTR  fpgaISRTbl[FPGA_MAX_SLICE][32]  = {{(VOIDFUNCPTR) NULL},};
static unsigned int fpgaISRmask[FPGA_MAX_SLICE][32] = {{0},};
static int          fpgaISRArg[FPGA_MAX_SLICE][32]  = {{0},};
static int fpgaSliceCount = 0;
/*static*/ volatile unsigned int *pInterruptEnable[FPGA_MAX_SLICE];
/*static*/ volatile unsigned int *pInterruptClear[FPGA_MAX_SLICE];
/*static*/ volatile unsigned int *pInterruptStatus[FPGA_MAX_SLICE];

int isrCnt = 0;

#define FPGA_MAX_ISRS  32
#define FPGA_MIN_ISRS  0

/*
 * Base FPGA 405 external interrupt ISR
 * calls any registered ISR routines
 * then clears the pending interrupts
 *
 *	Author: Greg Brissey   5/14/04
 */
static int fpgaSliceBaseISR(size_t slice, int arg)
{
    int i, j=0;

    unsigned int intStatus = *pInterruptStatus[slice];
    VOIDFUNCPTR  *pfpgaISR = fpgaISRTbl[slice];
    unsigned int *pIsrMask = fpgaISRmask[slice];
    int          *pIsrArg  = fpgaISRArg[slice];

    if (intStatus) {
      if (DebugLevel > 7)
	logMsg(" fpgaSliceBaseISR: status: 0x%lx\n",intStatus,2,3,4,5,6);
          
      for (; (*pfpgaISR != (VOIDFUNCPTR) NULL); pfpgaISR++, pIsrMask++, pIsrArg++) {
	if ( (*pIsrMask & intStatus) )
	  (*pfpgaISR) (intStatus, *pIsrArg);
      } 
      *pInterruptClear[slice] = 0;
      *pInterruptClear[slice] = intStatus;
      isrCnt++;
    }
    return intStatus;
}

void fpgaBaseISR(int arg)
{
    int slice;
    int cnt = 0;
    wvEvent(EVENT_BASEISR_START,NULL,NULL);

    for (slice=0; slice<fpgaSliceCount; slice++)
      cnt += fpgaSliceBaseISR(slice, arg) != 0;

    wvEvent(EVENT_BASEISR_END,NULL,NULL);
}

/*
 * disable the FPGA extern interrupt
 * set interrupt trigger mode to positive level.
 * install fpgaBaseISR routine
 *
 * enable all FPGA interrupts, then clear them then disable them
 * enable the external interrupt from the FPGA
 *
 *     Author:  Greg Brissey    5/14/05
 */
size_t initFpgaSliceISR
(
 volatile unsigned *status, 
 volatile unsigned *enable, 
 volatile unsigned *clear
)
{
   unsigned long uicSr,uicMSr;
   unsigned int regval, slice;
   int i, connected;

   /* if already installed don't bother doing it again */
   for (i=0; i<fpgaSliceCount; i++) {
     if (fpgaBaseISR_Installed[i] != 0 && 
	 (pInterruptStatus[i] == status ||
	  pInterruptEnable[i] == enable ||
	  pInterruptClear[i] == clear)) {
       if (pInterruptStatus[i] == status &&
	   pInterruptEnable[i] == enable &&
	   pInterruptClear[i] == clear)
	 return i;
       else
	 diagPrint("intConnect isr conflict\n"); // shouldn't happen
     }
   }

   slice = fpgaSliceCount++;
   if (fpgaSliceCount > FPGA_MAX_SLICE) {
       DPRINT1(-1,"max FPGA slices exceeded\n", FPGA_MAX_SLICE);
       return -1;
   }

   pInterruptStatus[slice] = status;
   pInterruptEnable[slice] = enable;
   pInterruptClear[slice] = clear;
   
   /* disable FPGA interrupt while we make changes to it */
   intDisable(FPGA_EXT_INT_IRQ);
   
   DPRINT1(0," initFpgaSliceBaseISR: installing FPGA Slice %d Base ISR\n", slice);

   /* set default mask to masking no bits */
   for (i=0; i < 32; i++)
   {
      fpgaISRmask[slice][i] = 0xffffffff;
      fpgaISRTbl[slice][i] = (VOIDFUNCPTR) NULL;
   }
      
   /* connect the Base ISR for the FPGA interrupts */
   if (slice == 0)
     if (intConnect(INUM_TO_IVEC(FPGA_EXT_INT_IRQ), fpgaBaseISR, (int) 0) == ERROR)
       diagPrint("!!!!!!!! intConnect vector 0x%x interrupt %d isr 0x%x failed\n", 
		 INUM_TO_IVEC(FPGA_EXT_INT_IRQ), FPGA_EXT_INT_IRQ, fpgaBaseISR);
     else
       diagPrint(NULL,"FPGA interrupt vector 0x%x interrupt %d isr 0x%x installed\n", 
		 INUM_TO_IVEC(FPGA_EXT_INT_IRQ), FPGA_EXT_INT_IRQ, fpgaBaseISR);
   else
     diagPrint(NULL,"FPGA slice base ISR already installed\n");

   /* FPGA is a Positive Level Interrupt */
   sysDcrUicprSet(sysDcrUicprGet() | (1 << (31-25)) );/* Positive Polarity */

   /* enable the FPGA internal sources of interrupts so we can clear them all */
   *pInterruptEnable[slice] = 0xffffffff;

   /* pulse those bits to clear them all */
   *pInterruptClear[slice] = 0;
   *pInterruptClear[slice] = 0xffffffff;
   *pInterruptClear[slice] = 0;

   /* Disable all FPGA internal sources of Interrupts */
   *pInterruptEnable[slice] = 0;

   /* as a precaution we clear the FPGA interrupt at the 405 level as well */
   sysDcrUicsrClear((1 << (31-25))); 

   /* now its time to enable the 405 */
   intEnable(FPGA_EXT_INT_IRQ);
   fpgaBaseISR_Installed[slice] = 1;

   return slice;
}

/* at transition point to Axiom most controllers have only one Base ISR */
// :TODO: move the BaseISR intConnect call to here
void initFpgaBaseISR(void)
{
  initFpgaSliceISR(_pInterruptStatus, _pInterruptEnable, _pInterruptClear);
}

void resetFpgaSliceItr(size_t slice)
{
   /* disable FPGA interrupt while we make changes to it */
   intDisable(FPGA_EXT_INT_IRQ);

   /* FPGA is a Positive Level Interrupt */
   sysDcrUicprSet(sysDcrUicprGet() | (1 << (31-25)) );/* Positive Polarity */

   /* enable the FPGA internal sources of interrupts so we can clear them all */
   *pInterruptEnable[slice] = 0xffffffff;

   /* pulse those bits to clear them all */
   *pInterruptClear[slice] = 0;
   *pInterruptClear[slice] = 0xffffffff;
   *pInterruptClear[slice] = 0;

   /* Disable all FPGA internal sources of Interrupts */
   *pInterruptEnable[slice] = 0;

   /* as a precaution we clear the FPGA interrupt at the 405 level as well */
   sysDcrUicsrClear((1 << (31-25)));

   /* now its time to enable the 405 */
   intEnable(FPGA_EXT_INT_IRQ);
}

void resetFpgaItr(void)
{
  if (fpgaBaseISR_Installed[0] == 0)
    initFpgaBaseISR();

  resetFpgaSliceItr(0);
}

/*
 *  fpgaIntConnect() follows the syntax of the VxWorks intConnect()
 *  This registers ISR to the fpgaBaseISR, which will call them in the order
 *  they were registered.
 *
 *                Author:  Greg Brissey  5/14/05
 */
STATUS fpgaSliceIntConnect
(
  int           slice,         /* which controller function in a multi-controller FPGA */
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter,    /* parameter to be passed to routine */
  unsigned int  mask	      /* interrupts bits that this isr should service */
)
{
    int index;
    if (DebugLevel>0) 
       diagPrint(NULL," fpgaSliceIntConnect[%d]: numreg: %d, routine: 0x%lx, arg: 0x%lx, mask: 0x%lx\n",
		 slice, numRegISRs[slice], routine, parameter, mask, 5, 6);

    if (fpgaBaseISR_Installed[slice] == 0)
      return ERROR;

    /* if fpgaBaseISR not installed, install it prior to proceeding */
    if (numRegISRs[slice] >= FPGA_MAX_ISRS)
        return ERROR;
    else
        {
           for (index=0; index < numRegISRs[slice]; index++)
               if (routine == fpgaISRTbl[slice][index] && parameter == fpgaISRArg[slice][index])
               {
		 diagPrint(NULL," fpgaIntConnect: routine already registered @ index %d, ( i.e. not added)\n",index);
	         return OK;
               }

           fpgaISRTbl[slice][numRegISRs[slice]] = routine;
           fpgaISRArg[slice][numRegISRs[slice]] = parameter;
           /* a mask of zero would mean the ISR would never be called, so we override that */
           fpgaISRmask[slice][numRegISRs[slice]] = (mask == 0) ? 0xffffffff : mask;
	   numRegISRs[slice]++;
           fpgaISRTbl[slice][numRegISRs[slice]] = (VOIDFUNCPTR)NULL; // mark end-of-list
           return OK;
        }
}

STATUS fpgaIntConnect
(
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter,    /* parameter to be passed to routine */
  unsigned int  mask	      /* interrupts bits that this isr should service */
)
{
  if (fpgaBaseISR_Installed[0] == 0)
    initFpgaBaseISR();
  fpgaSliceIntConnect(0, routine, parameter, mask);
}

/*
 *  fpgaIntRemove() remove a registered ISR
 *
 *                Author:  Greg Brissey  5/14/05
 */
STATUS fpgaSliceIntRemove
(
  size_t        slice,         /* base in a multi-controller FPGA   */
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter     /* parameter to be passed to routine */
)
{
    int index,i,j;
    if (DebugLevel>0) 
       diagPrint(NULL," fpgaIntRemove: numreg: %d, routine: 0x%lx, arg: 0x%lx\n",
                numRegISRs[slice],routine,parameter,4,5,6);

    /* if fpgaBaseISR not installed, install it prior to proceeding */
    if (fpgaBaseISR_Installed[slice] == 0)
        return ERROR;

    for (index=0; index < numRegISRs[slice]; index++)
    {
        if (routine == fpgaISRTbl[slice][index] && parameter == fpgaISRArg[slice][index])
        {
             j = index;
	     fpgaISRTbl[slice][j] = NULL;
             fpgaISRArg[slice][j] = 0;
             for(i=index+1; i < numRegISRs[slice]; i++)
             {
		fpgaISRTbl[slice][j] = fpgaISRTbl[slice][i];
                fpgaISRArg[slice][j] = fpgaISRArg[slice][i];
                fpgaISRmask[slice][j++] = fpgaISRmask[slice][i];
             }
	     numRegISRs[slice]--;
	     break;
        }
    }
}

STATUS fpgaIntRemove
(
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter     /* parameter to be passed to routine */
)
{ 
  /* if fpgaBaseISR not installed, install it prior to proceeding */
  if (fpgaBaseISR_Installed[0] == 0)
    initFpgaBaseISR();
  return fpgaSliceIntRemove(0, routine, parameter); 
}

/*
 *  fpgaIntChngMask() changed mask of a registered ISR
 *
 *                Author:  Greg Brissey  5/14/05
 */
STATUS fpgaSliceIntChngMask
(
  size_t        slice, 
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter,    /* parameter to be passed to routine */
  unsigned int  newmask	      /* interrupts bits that this isr should service */
)
{
    int index,i,j;
    if (DebugLevel>0) 
       diagPrint(NULL," fpgaIntRemove: numreg: %d, routine: 0x%lx, arg: 0x%lx, mask: 0x%lx\n",
                numRegISRs[slice],routine,parameter,newmask,5,6);

    /* if fpgaBaseISR not installed, install it prior to proceeding */
    if (fpgaBaseISR_Installed[slice] == 0)
        return 0;

    for(index=0; index < numRegISRs[slice]; index++)
        if (routine == fpgaISRTbl[slice][index] && parameter == fpgaISRArg[slice][index])
        {
             fpgaISRmask[slice][j++] = newmask;
	     break;
        }

    return 0;
}

STATUS fpgaIntChngMask
(
  VOIDFUNCPTR   routine,      /* routine to be called              */
  int           parameter,    /* parameter to be passed to routine */
  unsigned int  newmask	      /* interrupts bits that this isr should service */
)
{
  return fpgaSliceIntChngMask(0, routine, parameter, newmask);
}

STATUS fpgaSliceBaseISRShow(size_t slice, int level)
{
   int i;
   printf(" ------------  FPGA Interrupt Register Addresses  ---------  \n");
   printf(" Status = 0x%lx\n",pInterruptStatus[slice]);
   printf(" Enable = 0x%lx\n",pInterruptEnable[slice]);
   printf(" Clear  = 0x%lx\n\n",pInterruptClear[slice]);
   printf(" ------------  Registered fpgaBaseISR ISRs ---------  \n");
   if (numRegISRs[slice] > 0)
      for (i=0; i<numRegISRs[slice] && i<32; i++)
         if ( (fpgaISRTbl[slice][i] != (VOIDFUNCPTR) NULL))
         {
	     /* mask interrupt status using ISRs mask, if no bit set this ISR 
                is interrested in then don't call it */
             printf(" %d - Func: 0x%lx, Passed Value: %d, Mask: 0x%lx\n",
                 i, fpgaISRTbl[slice][i], fpgaISRArg[slice][i], fpgaISRmask[slice][i]);
         }
         else
             printf(" fpgaBaseISR[%d]: NULL registered ISR, index %d\n",slice,i);
   else
         printf(" fpgaBaseISR[%d]: no ISRs registered.\n",slice);

   printf(" ---------------------------------------------  \n\n");
   return 0;
}

STATUS isrSliceShow(size_t slice, int level)
{
   printf(" ------------  Registered fpgaBaseISR ISRs slice %d ---------  \n", slice);
   return(fpgaSliceBaseISRShow(slice,level));
}

STATUS isrShow(int level)
{
  int status, i;
  for (i=0; i<fpgaSliceCount; i++)
    status = isrSliceShow(i,level);
  return status;
}

prtUIC()
{
   printf("UIC Status: 0x%lx, Enable: 0x%lx, Masked Status: 0x%lx (0x%lx & 0x%lx)\n",
             sysDcrUicsrGet(), sysDcrUicerGet(), sysDcrUicmsrGet(),sysDcrUicsrGet(), sysDcrUicerGet());
   printf("UIC Polarity: (pos=1,neg=0): 0x%lx, Trigger (edge=1,level=0): 0x%lx\n",sysDcrUicprGet(),sysDcrUictrGet());
   printf("UIC Critical: 0x%lx\n", sysDcrUiccrGet());
   printf("UIC Vector  : 0x%lx\n",sysDcrUicvrGet());
   return 0;
}
