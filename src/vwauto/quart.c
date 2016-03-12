/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* quart.c - SC68c94 QUART tty initializer */
#ifndef LINT
#endif
/*
*/
/* sysLib1.c and usrConfig.c -  for Quart */


#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
#include <msgQLib.h>
#include "quart.h"
#include "sc68c94.h"
#include "logMsgLib.h"

/**********************************************************************
* This section sysQHwInit and sysQHwInit2 normaly resides in sysLib.c
*/

/* The dev belongs here in sysLib.c but is in driver for load reasons */
/* TYQ_CO_DEV tyQCoDv [NUM_QSC_CHANNELS]; */
IMPORT TYQ_CO_DEV tyQCoDv [];	/* device descriptors */

LOCAL unsigned char QSC_kimr_AB;
LOCAL unsigned char QSC_kimr_CD;

/* void sysQHwInit (void) this should be but: */
void sysQHwInit (int mode) /* this passes int mode for testing */
    {
    int temp;   /* temp storage */

    /*
     *   - Disable QSC Interrupts
     */

    /* now setup serial device descriptor */
    /* The first block of entries are unique to each channel */

    tyQCoDv [0].created = FALSE;
    tyQCoDv [0].numChannels = NUM_QSC_CHANNELS;
    tyQCoDv [0].mr = (unsigned char *) SERIAL_QSC+0x00;
    tyQCoDv [0].sr = (unsigned char *) SERIAL_QSC+0x02;
    tyQCoDv [0].csr = (unsigned char *) SERIAL_QSC+0x02;
    tyQCoDv [0].cr = (unsigned char *) SERIAL_QSC+0x04;
    tyQCoDv [0].dr = (unsigned char *) SERIAL_QSC+0x06;
    tyQCoDv [0].iopcr = (unsigned char *) SERIAL_QSC+0x1a;
    tyQCoDv [0].bcr = (unsigned char *) SERIAL_QSC+0x40;
    tyQCoDv [0].rem = RX_RDY_AC_INT;
    tyQCoDv [0].tem = TX_RDY_AC_INT;
    /* The remaining entries are shared a,b or c,d */
    tyQCoDv [0].ipcr = (unsigned char *) SERIAL_QSC+0x08;
    tyQCoDv [0].acr = (unsigned char *) SERIAL_QSC+0x08;
    tyQCoDv [0].isr = (unsigned char *) SERIAL_QSC+0x0a;
    tyQCoDv [0].imr = (unsigned char *) SERIAL_QSC+0x0a;
    tyQCoDv [0].ctur = (unsigned char *) SERIAL_QSC+0x0c;
    tyQCoDv [0].ctlr = (unsigned char *) SERIAL_QSC+0x0e;
    tyQCoDv [0].opr = (unsigned char *) SERIAL_QSC+0x18;
    tyQCoDv [0].ipr = (unsigned char *) SERIAL_QSC+0x1a;
    tyQCoDv [0].ctron = (unsigned char *) SERIAL_QSC+0x1c;
    tyQCoDv [0].ctroff = (unsigned char *) SERIAL_QSC+0x1e;
    tyQCoDv [0].kimr = &QSC_kimr_AB; /* same cell for ports A & B */
    tyQCoDv [0].tickRtn = NULL;
    tyQCoDv [0].mode = mode;

    tyQCoDv [1].created = FALSE;
    tyQCoDv [1].numChannels = NUM_QSC_CHANNELS;
    tyQCoDv [1].mr = (unsigned char *) SERIAL_QSC+0x10;
    tyQCoDv [1].sr = (unsigned char *) SERIAL_QSC+0x12;
    tyQCoDv [1].csr = (unsigned char *) SERIAL_QSC+0x12;
    tyQCoDv [1].cr = (unsigned char *) SERIAL_QSC+0x14;
    tyQCoDv [1].dr = (unsigned char *) SERIAL_QSC+0x16;
    tyQCoDv [1].iopcr = (unsigned char *) SERIAL_QSC+0x1c;
    tyQCoDv [1].bcr = (unsigned char *) SERIAL_QSC+0x42;
    tyQCoDv [1].rem = RX_RDY_BD_INT;
    tyQCoDv [1].tem = TX_RDY_BD_INT;
    /* The remaining entries are shared a,b or c,d */
    tyQCoDv [1].ipcr = (unsigned char *) SERIAL_QSC+0x08;
    tyQCoDv [1].acr = (unsigned char *) SERIAL_QSC+0x08;
    tyQCoDv [1].isr = (unsigned char *) SERIAL_QSC+0x0a;
    tyQCoDv [1].imr = (unsigned char *) SERIAL_QSC+0x0a;
    tyQCoDv [1].ctur = (unsigned char *) SERIAL_QSC+0x0c;
    tyQCoDv [1].ctlr = (unsigned char *) SERIAL_QSC+0x0e;
    tyQCoDv [1].opr = (unsigned char *) SERIAL_QSC+0x18;
    tyQCoDv [1].ipr = (unsigned char *) SERIAL_QSC+0x1a;
    tyQCoDv [1].ctron = (unsigned char *) SERIAL_QSC+0x1c;
    tyQCoDv [1].ctroff = (unsigned char *) SERIAL_QSC+0x1e;
    tyQCoDv [1].kimr = &QSC_kimr_AB; /* same cell for ports A & B */
    tyQCoDv [1].tickRtn = NULL;
    tyQCoDv [1].mode = mode;

    tyQCoDv [2].created = FALSE;
    tyQCoDv [2].numChannels = NUM_QSC_CHANNELS;
    tyQCoDv [2].mr = (unsigned char *) SERIAL_QSC+0x20;
    tyQCoDv [2].sr = (unsigned char *) SERIAL_QSC+0x22;
    tyQCoDv [2].csr = (unsigned char *) SERIAL_QSC+0x22;
    tyQCoDv [2].cr = (unsigned char *) SERIAL_QSC+0x24;
    tyQCoDv [2].dr = (unsigned char *) SERIAL_QSC+0x26;
    tyQCoDv [2].iopcr = (unsigned char *) SERIAL_QSC+0x1a;
    tyQCoDv [2].bcr = (unsigned char *) SERIAL_QSC+0x44;
    tyQCoDv [2].rem = RX_RDY_AC_INT;
    tyQCoDv [2].tem = TX_RDY_AC_INT;
    /* The remaining entries are shared a,b or c,d */
    tyQCoDv [2].ipcr = (unsigned char *) SERIAL_QSC+0x28;
    tyQCoDv [2].acr = (unsigned char *) SERIAL_QSC+0x28;
    tyQCoDv [2].isr = (unsigned char *) SERIAL_QSC+0x2a;
    tyQCoDv [2].imr = (unsigned char *) SERIAL_QSC+0x2a;
    tyQCoDv [2].ctur = (unsigned char *) SERIAL_QSC+0x2c;
    tyQCoDv [2].ctlr = (unsigned char *) SERIAL_QSC+0x2e;
    tyQCoDv [2].opr = (unsigned char *) SERIAL_QSC+0x38;
    tyQCoDv [2].ipr = (unsigned char *) SERIAL_QSC+0x3a;
    tyQCoDv [2].ctron = (unsigned char *) SERIAL_QSC+0x3c;
    tyQCoDv [2].ctroff = (unsigned char *) SERIAL_QSC+0x3e;
    tyQCoDv [2].kimr = &QSC_kimr_CD; /* same cell for ports C & D */
    tyQCoDv [2].tickRtn = NULL;
    tyQCoDv [2].mode = mode;

    tyQCoDv [3].created = FALSE;
    tyQCoDv [3].numChannels = NUM_QSC_CHANNELS;
    tyQCoDv [3].mr = (unsigned char *) SERIAL_QSC+0x30;
    tyQCoDv [3].sr = (unsigned char *) SERIAL_QSC+0x32;
    tyQCoDv [3].csr = (unsigned char *) SERIAL_QSC+0x32;
    tyQCoDv [3].cr = (unsigned char *) SERIAL_QSC+0x34;
    tyQCoDv [3].dr = (unsigned char *) SERIAL_QSC+0x36;
    tyQCoDv [3].iopcr = (unsigned char *) SERIAL_QSC+0x3c;
    tyQCoDv [3].bcr = (unsigned char *) SERIAL_QSC+0x46;
    tyQCoDv [3].rem = RX_RDY_BD_INT;
    tyQCoDv [3].tem = TX_RDY_BD_INT;
    /* The remaining entries are shared a,b or c,d */
    tyQCoDv [3].ipcr = (unsigned char *) SERIAL_QSC+0x28;
    tyQCoDv [3].acr = (unsigned char *) SERIAL_QSC+0x28;
    tyQCoDv [3].isr = (unsigned char *) SERIAL_QSC+0x2a;
    tyQCoDv [3].imr = (unsigned char *) SERIAL_QSC+0x2a;
    tyQCoDv [3].ctur = (unsigned char *) SERIAL_QSC+0x2c;
    tyQCoDv [3].ctlr = (unsigned char *) SERIAL_QSC+0x2e;
    tyQCoDv [3].opr = (unsigned char *) SERIAL_QSC+0x38;
    tyQCoDv [3].ipr = (unsigned char *) SERIAL_QSC+0x3a;
    tyQCoDv [3].ctron = (unsigned char *) SERIAL_QSC+0x3c;
    tyQCoDv [3].ctroff = (unsigned char *) SERIAL_QSC+0x3e;
    tyQCoDv [3].kimr = &QSC_kimr_CD; /* same cell for ports C & D */
    tyQCoDv [3].tickRtn = NULL;
    tyQCoDv [3].mode = mode;


    }


/*******************************************************************************
*
* sysHwInit2 - connect hardware interrupts
*
* This routine connects additional hardware interrupts.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysQHwInit2 (void)
    {
    static BOOL configured = FALSE;

    if (!configured)
        {
        /* connect serial interrupts
         * the sc68c94 Serial Chip uses 26 interrupts out of 32.
         *    where xxx is the base vector
         *    where   y is a don't care bit
         *    where  zz is the channel number 0-3 = CH-A to CH-D.
         *
         *    xxx0 0000 - ICR mode 0, Output IVR only.
         *    xxx0 00zz - ICR mode 1, 6 MSBs of IVR and Channel # as 2LSBs.
         *    Next 7 items ICR mode 2, 3 MSBs of IVR Type and Channel #.
         *    xxx0 01zz - Channel Change of State Interrupt
         *    xxxy 10zz - Transmit Data Interrupt
         *    xxx0 11zz - Receive Data Interrupt No Error detected.
         *    xxx1 00zz - Break Condition detected Interrupt
         *    xxx1 01zz - Timer Interrupt
         *    xxxy 10zz - Transmit Data Interrupt
         *    xxx1 11zz - Receive Data Interrupt Error detected.
         */

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_M0(INT_VEC_QSC)),
		           tyQCoIntM0, 0);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_M1(INT_VEC_QSC)),
		           tyQCoIntM1, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_M1(INT_VEC_QSC)),
		           tyQCoIntM1, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_M1(INT_VEC_QSC)),
		           tyQCoIntM1, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_M1(INT_VEC_QSC)),
		           tyQCoIntM1, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_CS(INT_VEC_QSC)),
		           tyQCoIntCs, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_CS(INT_VEC_QSC)),
		           tyQCoIntCs, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_CS(INT_VEC_QSC)),
		           tyQCoIntCs, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_CS(INT_VEC_QSC)),
		           tyQCoIntCs, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_WR0(INT_VEC_QSC)),
		           tyQCoIntWr, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_WR0(INT_VEC_QSC)),
		           tyQCoIntWr, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_WR0(INT_VEC_QSC)),
		           tyQCoIntWr, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_WR0(INT_VEC_QSC)),
		           tyQCoIntWr, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_RDNE(INT_VEC_QSC)),
		           tyQCoIntRd, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_RDNE(INT_VEC_QSC)),
		           tyQCoIntRd, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_RDNE(INT_VEC_QSC)),
		           tyQCoIntRd, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_RDNE(INT_VEC_QSC)),
		           tyQCoIntRd, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_BK(INT_VEC_QSC)),
		           tyQCoIntBk, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_BK(INT_VEC_QSC)),
		           tyQCoIntBk, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_BK(INT_VEC_QSC)),
		           tyQCoIntBk, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_BK(INT_VEC_QSC)),
		           tyQCoIntBk, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_TM(INT_VEC_QSC)),
		           tyQCoIntTm, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_TM(INT_VEC_QSC)),
		           tyQCoIntTm, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_TM(INT_VEC_QSC)),
		           tyQCoIntTm, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_TM(INT_VEC_QSC)),
		           tyQCoIntTm, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_WR1(INT_VEC_QSC)),
		           tyQCoIntWr, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_WR1(INT_VEC_QSC)),
		           tyQCoIntWr, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_WR1(INT_VEC_QSC)),
		           tyQCoIntWr, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_WR1(INT_VEC_QSC)),
		           tyQCoIntWr, 3);

        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_A_RDER(INT_VEC_QSC)),
		           tyQCoIntRdE, 0);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_B_RDER(INT_VEC_QSC)),
		           tyQCoIntRdE, 1);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_C_RDER(INT_VEC_QSC)),
		           tyQCoIntRdE, 2);
        (void) intConnect (INUM_TO_IVEC (INT_VEC_QSC_D_RDER(INT_VEC_QSC)),
		           tyQCoIntRdE, 3);

        /* Set QUART Interrupt Vector Register */
        *QSC_IVR = INT_VEC_QSC;

	*QSC_ICR = tyQCoDv[0].mode;

        configured = TRUE;
	}
    }

/**********************************************************************
* This section normaly resides in usrConfig.c
*/

quartCreate(int num, int mode) /* mode is the value to place in ICR */
{
char tyName [20];
int ix;
char AutoInt;

/* This call is in usrInit between ShowInit and KernelInit */

    sysQHwInit (mode); /* in usrInit() */

/* This snippet should be in usrRoot() after comment below */

    /* set up system timer */
    sysQHwInit2 ();	/* in 162 called as a part of ClkConnect */

/* This snippet should be in usrRoot() after comment below */

    /* initialize I/O system */

    if (NUM_QSC_CHANNELS > 0)
    {
	tyQCoDrv ();		/* create QUART driver */

	/*for (ix = 0; ix < NUM_QSC_CHANNELS; ix++)  /* create serial devices */
	for (ix = 0; ix < num; ix++)  /* create serial devices */
	{
	   sprintf (tyName, "%s%d", "/tyQCo/", ix);

    	   /* DPRINT(-1,"Entering tyQCoDevCreate\n"); */
	    (void) tyQCoDevCreate (tyName, ix, 512, 512);
    	   /* DPRINT(-1,"Returned tyQCoDevCreate\n"); */
	}
/***** This should not be Here, This Interrupt should be hardwired!!! */
	AutoInt = *(unsigned char*)0x20100b; /* get current system Int mask */
	AutoInt &= 0xf7;	/* unmask Quart Interrupt */
	*(unsigned char*)0x20100b = AutoInt; /* doit toit */
    }
}
/* end quart.c */
