/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/*
*/
/* orig. m68681Serial.c - MC68681/SC2681 DUART tty driver */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include <copyright_wrs.h>

/*
modification history
--------------------
---,31may95,gad  Modified severly for sc68c94 Quart.

01i,08feb93,caf  fixed ansi warning.
01h,03feb93,caf  modified tyCoStartup() to avoid race condition.
01g,26jan93,caf  corrected note about available baud rates.
01f,20jan93,caf  modified interrupt mask register during initialization only.
		 rewrote tyCoStartup(), removed intLock() call from tyImrSet().
01e,09dec92,caf  fixed ansi warning (SPR 1849).
01d,01dec92,jdi  NOMANUAL tyImrSet(), tyCoInt(), tyCoClkConnect() - SPR 1808.
01c,28oct92,caf  tweaked tyCoDevCreate() documentation.
01b,24aug92,caf  changed VOID to void.  changed errno.h to errnoLib.h.
		 ansified.  added tyCoClkConnect().
01a,06jun92,caf  derived from mv165/tyCoDrv.c.
*/

/*
DESCRIPTION
This is the driver for the MC68694/SC2694 QUART.
It uses the QUART in asynchronous mode only.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system.  Two routines, however, must be called directly: tyQCoDrv() to
initialize the driver, and tyQCoDevCreate() to create devices.

Before the driver can be used, it must be initialized by calling tyQCoDrv().
This routine should be called exactly once, before any reads, writes, or
calls to tyQCoDevCreate().  Normally, it is called by usrRoot() in usrConfig.c.

Before a terminal can be used, it must be created using tyQCoDevCreate().
Each port to be used should have exactly one device associated with it by
calling this routine.

IOCTL FUNCTIONS
This driver responds to the same ioctl() codes as a normal tty driver; for
more information, see the manual entry for tyLib.  The available baud rates
range from 75 to 38.4k.

SEE ALSO
tyLib
*/

#include <vxWorks.h>
#include <iv.h>
#include <ioLib.h>
#include <iosLib.h>
#include <tyLib.h>
/* #include <config.h> */
#include <intLib.h>
#include <errnoLib.h>
#include "quart.h" /* AUTO specific info */
#include "sc68c94.h" /* quad definitions */


typedef struct		/* BAUD */
    {
    int rate;		/* a baud rate */
    char csrVal;	/* rate to write to the csr reg to get that baud rate */
    } BAUD;

/* The dev belongs in sysLib.c but is here for load reasons */
TYQ_CO_DEV tyQCoDv [NUM_QSC_CHANNELS];

/* IMPORT TYQ_CO_DEV tyQCoDv [];	device descriptors */

/*
 * BaudTable is a table of the available baud rates, and the values to write
 * to the csr reg to get those rates
 */

LOCAL BAUD baudTable [] =	/* BRG bit = 1 */
    {
    {75,	RX_CLK_75    | TX_CLK_75},
    {110,	RX_CLK_110   | TX_CLK_110},
    {38400,	RX_CLK_38_4K | TX_CLK_38_4K},
    {150,	RX_CLK_150   | TX_CLK_150},
    {300,	RX_CLK_300   | TX_CLK_300},
    {600,	RX_CLK_600   | TX_CLK_600},
    {1200,	RX_CLK_1200  | TX_CLK_1200},
    {2000,	RX_CLK_2000  | TX_CLK_2000},
    {2400,	RX_CLK_2400  | TX_CLK_2400},
    {4800,	RX_CLK_4800  | TX_CLK_4800},
    {1800,	RX_CLK_1800  | TX_CLK_1800},
    {9600,	RX_CLK_9600  | TX_CLK_9600},
    {19200,	RX_CLK_19200 | TX_CLK_19200}
    };

LOCAL int tyQCoDrvNum;		/* driver number assigned to this driver */

/* forward declarations */

LOCAL void tyQCoStartup ();
LOCAL int tyQCoOpen ();
LOCAL STATUS tyQCoIoctl ();
LOCAL void tyQCoHrdInit ();
void tyQImrSet (TYQ_CO_DEV *pTyQCoDv, char setBits, char clearBits);


/*******************************************************************************
*
* tyQCoDrv - initialize the tty driver
*
* This routine initializes the serial driver, sets up interrupt vectors, and
* performs hardware initialization of the serial ports.
*
* This routine should be called exactly once, before any reads, writes, or
* calls to tyQCoDevCreate().  Normally, it is called by usrRoot() in
* usrConfig.c.
*
* RETURNS: OK, or ERROR if the driver cannot be installed.
*
* SEE ALSO: tyQCoDevCreate()
*/

STATUS tyQCoDrv (void)
    {
    if (tyQCoDrvNum > 0)
	return (OK);

    tyQCoHrdInit ();

    tyQCoDrvNum = iosDrvInstall (tyQCoOpen, (FUNCPTR) NULL, tyQCoOpen,
				(FUNCPTR) NULL, tyRead, tyWrite, tyQCoIoctl);

    return ((tyQCoDrvNum == ERROR) ? (ERROR) : (OK));
    }

/*******************************************************************************
*
* tyQCoDevCreate - create a device for an on-board serial port
*
* This routine creates a device on a specified serial port.  Each port
* to be used should have exactly one device associated with it by calling
* this routine.
*
* For instance, to create the device "/tyQCo/0", with buffer sizes of 512 bytes,
* the proper call would be:
* .CS
*     tyQCoDevCreate ("/tyQCo/0", 0, 512, 512);
* .CE
*
* RETURNS: OK, or ERROR if the driver is not installed, the channel is
* invalid, or the device already exists.
*
* SEE ALSO: tyQCoDrv()
*/

STATUS tyQCoDevCreate
    (
    char *      name,           /* name to use for this device      */
    FAST int    channel,        /* physical channel for this device */
    int         rdBufSize,      /* read buffer size, in bytes       */
    int         wrtBufSize      /* write buffer size, in bytes      */
    )
    {
    if (tyQCoDrvNum <= 0)
	{
	errnoSet (S_ioLib_NO_DRIVER);
	return (ERROR);
	}

    /* check for channel validity */

    if ((channel < 0) || (channel >= tyQCoDv [0].numChannels))
	return (ERROR);

    /* if there is a device already on this channel, don't do it */

    if (tyQCoDv [channel].created)
	return (ERROR);

    /*
     * Initialize the ty descriptor, and turn on the bits for this
     * receiver and transmitter in the interrupt mask
     */

    if (tyDevInit (&tyQCoDv [channel].tyDev,
		    rdBufSize, wrtBufSize, (FUNCPTR)tyQCoStartup) != OK)
	{
	return (ERROR);
	}

    tyQImrSet (&tyQCoDv [channel], (tyQCoDv [channel].rem | tyQCoDv [channel].tem),
	      0);

    /* mark the device as created, and add the device to the I/O system */

    tyQCoDv [channel].created = TRUE;

    return (iosDevAdd (&tyQCoDv [channel].tyDev.devHdr, name, tyQCoDrvNum));
    }

/*******************************************************************************
*
* tyQCoHrdInit - initialize the QUART
*
* RETURNS: N/A
*/

LOCAL void tyQCoHrdInit (void)
    {
    char sr;
    int ii;			/* scratch */
    int lock = intLock ();	/* LOCK INTERRUPTS */

    /* 8 data bits, 1 stop bit, no parity, set for 9600 baud */

    for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
    {
        *tyQCoDv [ii].cr  = RST_BRK_INT_CMD;
        *tyQCoDv [ii].cr  = RST_ERR_STS_CMD;
        *tyQCoDv [ii].cr  = RST_TX_CMD;
        *tyQCoDv [ii].cr  = RST_RX_CMD;
        *tyQCoDv [ii].cr  = RST_MR_PTR_CMD;
        *tyQCoDv [ii].mr  = PAR_MODE_NO | BITS_CHAR_8;	/* mode 1         */
        *tyQCoDv [ii].mr  = STOP_BITS_1; 		/* mode 2         */
        *tyQCoDv [ii].csr = RX_CLK_9600 | TX_CLK_9600;	/* clock          */
         tyQCoDv [ii].baud = RX_CLK_9600 | TX_CLK_9600;	/* clock          */
        *tyQCoDv [ii].acr = BRG_SELECT | 0;	/* clock          */
        *tyQCoDv [ii].bcr = 0x20; /* data sheet says this is default */
        *tyQCoDv [ii].iopcr = 0x55;   /* for my AUTO board */
        *tyQCoDv [ii].opr = 0xf0;   /* for my AUTO board */
   
        /* *tyQCoDv [ii].iopcr = 0x05;   /* for my AUTO board */
/* xxxx Should be IO0=CTS, 1=CD, 2=RTS, 3=DTR */
/*        *tyQCoDv [ii].iopcr = IOPCR_AUTO; */
     }

    /* enable the receivers on all channels */

    for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
        *tyQCoDv [ii].cr  = RX_ENABLE;

#ifdef XXX
    /* about 10 MSR board are out there that can only handle 1 serial port
       do to lacking several driver chips. Testing the status register (SR)
       of each channel could indicate bad channels, unfortunately the
       inputs are floating resulting in indeterminate readings. If one does
       activate one of the bad channels the 332 is floaded with interrupts
       resulting in a workQpanic error and a auto-reboot.
       Typical interrupts were tyQCoIntRdE() & tyQCoIntRd().
       Typically the 3rd or 4th channel would indicate a non-zero SR
       but it could take over 2 min to show this, and it might never.
       Newer boards don't have the 4th channel hooked up at all
       So for now only the 1st channel is being activated.
	Greg Brissey 12-8-95
    */
    printf("Delaying for Thuan\n");
    taskDelay(2*60*60);  /* after 2 min the channel would go bad */
    for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
    {
    /*     taskDelay(30); */
        sr = *tyQCoDv [ii].sr;
        printf("sr=0x%02x\n",sr);
	if (sr == 0)
	{
           printf("tyQCoHrdInit: channel %d, OK\n",ii);
        }
	else
	{
           printf("tyQCoHrdInit: channel %d, Failed\n",ii);
        }
    }
#endif
    /*
     * All interrupts are masked out: interrupts will be enabled
     * in the tyQCoDevCreate
     */

    intUnlock (lock);				/* UNLOCK INTERRUPTS */
    }

/*******************************************************************************
*
* tyQCoOpen - open a file to the QUART
*
* This routine opens a file to the QUART.
*
* RETURNS: a file descriptor
*/

LOCAL int tyQCoOpen
    (
    TYQ_CO_DEV *pTyCoDv,
    char *name,
    int mode
    )
    {
    return ((int) pTyCoDv);
    }

/*******************************************************************************
*
* tyQCoIoctl - special device control
*
* This routine handles FIOBAUDRATE requests and passes all others to tyIoctl().
*
* RETURNS: OK, or ERROR if the baud rate is invalid, or whatever tyIoctl()
* returns.
*/

LOCAL STATUS tyQCoIoctl
    (
    TYQ_CO_DEV *pTyCoDv,	/* device to control */
    int request,	/* request code      */
    int arg		/* some argument     */
    )
    {
    int    ix;
    STATUS status;

    switch (request)
	{
	case 100:	/* return input port reg */
	    status = *pTyCoDv->ipr;
	    break;

	case FIOBAUDRATE:
	    status = ERROR;
	    for (ix = 0; ix < NELEMENTS (baudTable); ix ++)
		{
		if (baudTable [ix].rate == arg)
		    {
		    *pTyCoDv->csr = baudTable [ix].csrVal;
		    status = OK;
		    break;
		    }
		}
	    break;

	default:
	    status = tyIoctl (&pTyCoDv->tyDev, request, arg);
	    break;
	}

    return (status);
    }

/*******************************************************************************
*
* tyQImrSet - set and clear bits in the UART's interrupt mask register
*
* This routine sets and clears bits in the QUART's IMR.
*
* This routine sets and clears bits in a local copy of the IMR, then
* writes that local copy to the UART.  This means that all changes to
* the IMR must go through this routine.  Otherwise, any direct changes
* to the IMR would be lost the next time this routine is called.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void tyQImrSet
    (
    TYQ_CO_DEV *pTyCoDv,	/* which device                   */
    char setBits,	/* which bits to set in the IMR   */
    char clearBits	/* which bits to clear in the IMR */
    )
    {
    *pTyCoDv->kimr = (*pTyCoDv->kimr | setBits) & (~clearBits);
    *pTyCoDv->imr = *pTyCoDv->kimr;
    }


/*******************************************************************************
*
* tyQCoIntM0 - handle a QUART interrupt
*
* This routine is called to handle a Mode 0 QUART interrupt.
* It extracts channel and calls Mode 1 handler to finish.
*
* RETURNS: N/A
*/

void tyQCoIntM0
    (
    int dummy
    )
  	{
			tyQCoIntM1( (int) ( *QSC_CIR & CIR_CHAN) );
		}


/*******************************************************************************
*
* tyQCoIntM1 - handle a QUART interrupt
*
* This routine is called to handle a Mode 1 QUART interrupt.
* If no device has been created then no action is taken.
* The routine determines the type of interrupt and processes it.
*
* RETURNS: N/A
*/

void tyQCoIntM1
    (
		FAST int channel
    )
  {
		FAST int cir;
		FAST int type;
    char outChar;
    volatile char	bucket;	/* scratch */

    if (tyQCoDv [channel].created)
		{
			cir = (int) *QSC_CIR; /* get CIR */
			type = cir & CIR_TYPE; /* extract type */
 /* logMsg("tyQCoIntM1, chan: %d, type: %d, cir: 0x%x\n",channel,type,cir); */

			switch(type)
			{
				case CIR_RX:
				case CIR_RXE:	/* process error later */
					tyIRd (&tyQCoDv [channel].tyDev, *QSC_GRxFIFO);
					break;

				case CIR_TX:
				case CIR_TX2:
					if (tyITx (&tyQCoDv [channel].tyDev, &outChar) == OK)
						*QSC_GTxFIFO = outChar;
    			else
        		*tyQCoDv [channel].cr  = TX_DISABLE;		/* xmitr off */
					break;

				case CIR_CT:
					if (tyQCoDv [channel].tickRtn != NULL)
						tyQCoDv [channel].tickRtn ();
					else
						bucket = (* tyQCoDv [channel].ctroff);	/* stop timer */
					break;

				case CIR_BRK:	/* figure out this later */
					break;

				case CIR_COS:	/* figure out this later */
					break;

				default:
					break;
			}

		}

		tyQCoDv[channel].heartbeat++;

	}


/*******************************************************************************
*
* tyQCoIntTm - handle a Timer interrupt
*
* This routine is called to handle a Timer interrupt.
* If no device has been created then no action is taken.
* The routine
*
* RETURNS: N/A
*/

void tyQCoIntTm
    (
    FAST int channel
    )
    {
    volatile char	bucket;	/* scratch */

    /* logMsg("tyQCoIntTm, chan: %d\n",channel); */
   	 if (*tyQCoDv [channel].isr & CTR_RDY_INT)	/* timer */
			{
				if (tyQCoDv [channel].tickRtn != NULL)
	 	   	tyQCoDv [channel].tickRtn ();
			else
	 	   bucket = (* tyQCoDv [channel].ctroff);	/* stop timer */
			}
    }

/*******************************************************************************
*
* tyQCoIntCs - handle a Change of State interrupt
*
* This routine is called to handle a Change of State interrupt.
* If no device has been created then no action is taken.
* No idea at this time what to do.
*
* RETURNS: N/A
*/

void tyQCoIntCs
    (
    FAST int channel
    )
    {
       /* logMsg("tyQCoIntCs, chan: %d\n",channel); */
			asm(" nop ");
    }

/*******************************************************************************
*
* tyQCoIntBk - handle a Break detect interrupt
*
* This routine is called to handle a Break detect interrupt.
* If no device has been created then no action is taken.
* No idea at this time what to do.
*
* RETURNS: N/A
*/

void tyQCoIntBk
    (
    FAST int channel
    )
    {
       /* logMsg("tyQCoIntBk, chan: %d\n",channel); */
			asm(" nop ");
    }

/*******************************************************************************
*
* tyQCoIntRdE - handle a receiver interrupt with Error.
*
* This routine is called to handle a receiver interrupt.
* If no device has been created then no action is taken.
* Any character input is passed to the system buffer.
*
* RETURNS: N/A
*/

void tyQCoIntRdE
    (
    FAST int channel
    )
    {

      /* logMsg("tyQCoIntRdE, chan: %d\n",channel); */
	    if (tyQCoDv [channel].created)
				tyIRd (&tyQCoDv [channel].tyDev, *tyQCoDv [channel].dr);
			tyQCoDv[channel].heartbeat++;

    }

/*******************************************************************************
*
* tyQCoIntRd - handle a receiver interrupt
*
* This routine is called to handle a receiver interrupt.
* If no device has been created then no action is taken.
* Any character input is passed to the system buffer.
*
* RETURNS: N/A
*/

void tyQCoIntRd
    (
    FAST int channel
    )
    {
      /* logMsg("tyQCoIntRd, chan: %d\n",channel); */

    	if (tyQCoDv [channel].created)
				tyIRd (&tyQCoDv [channel].tyDev, *tyQCoDv [channel].dr);
			tyQCoDv[channel].heartbeat++;

    }

/*******************************************************************************
*
* tyQCoIntWr - handle a transmitter interrupt
*
* This routine is called to handle a transmitter interrupt.
* If there is another character to be transmitted, it sends it.  If there
* is no character to be transmitted or a device has never been created for
* this channel, this routine disables the transmitter interrupt.
*
* RETURNS: N/A
*/

void tyQCoIntWr
    (
    FAST int channel
    )
    {
    char outChar;

    /* logMsg("tyQCoIntWr, chan: %d\n",channel); */
	    if (tyQCoDv [channel].created &&
				tyITx (&tyQCoDv [channel].tyDev, &outChar) == OK)
			{
				*(tyQCoDv [channel].dr) = outChar;
				tyQCoDv[channel].heartbeat++;
			}
    	else
        *tyQCoDv [channel].cr  = TX_DISABLE;		/* xmitr off */
    }

/*******************************************************************************
*
* tyQCoStartup - transmitter startup routine
*
* This routine starts the UART transmitter.
*
* RETURNS: N/A
*/

LOCAL void tyQCoStartup
    (
    TYQ_CO_DEV *pTyCoDv		/* ty device to start up */
    )
    {
    *pTyCoDv->cr = TX_ENABLE;		/* enable the transmitter */
    }

/*******************************************************************************
*
* tyQCoClkConnect - connect to QUART timer interrupt
*
* This routine connects <routine> to the QUART timer interrupt.
* The QUART of interest is specified by <device>.
*
* This routine does not enable the QUART timer interrupt.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void tyQCoClkConnect
    (
    int		device,		/* which QUART to connect                  */
    VOIDFUNCPTR	routine		/* routine to be called at timer interrupt */
    )
    {
    tyQCoDv [2 * device].tickRtn = routine;
    }

tyQCoPresent()
{
   return( tyQCoDv[0].created );
}

#ifdef DIAG
/* tyQCoShow reports heartbeat valus and status registers */
tyQCoShow()
{
	printf("tyQCoShow sc68c94Serial.c 11.1 07/09/07:\n");
	if(tyQCoDv[0].created)
	{
		printf(" Ch0:%d br=0x%02x, SR=0x%02x, char=%c, hb=%d\n",
			tyQCoDv[0].created, (tyQCoDv[0].baud & 0xff),
			(*tyQCoDv[0].sr & 0xff), tyQCoDv[0].newchar, tyQCoDv[0].heartbeat);

		printf(" Ch1:%d br=0x%02x, SR=0x%02x, char=%c, hb=%d\n",
			tyQCoDv[1].created, (tyQCoDv[1].baud & 0xff),
			(*tyQCoDv[1].sr & 0xff), tyQCoDv[1].newchar, tyQCoDv[1].heartbeat);

		printf("    : kimr=0x%02x, ISR=0x%02x, IPCR=0x%02x, IPR=0x%02x, OPR=0x%02x\n",
			(*tyQCoDv[0].kimr & 0xff), (*tyQCoDv[0].isr & 0xff),
			(*tyQCoDv[0].ipcr & 0xff), (*tyQCoDv[0].ipr & 0xff),
			(*tyQCoDv[0].opr & 0xff) );

		printf(" Ch2:%d br=0x%02x, SR=0x%02x, char=%c, hb=%d\n",
			tyQCoDv[2].created, (tyQCoDv[2].baud & 0xff),
			(*tyQCoDv[2].sr & 0xff), tyQCoDv[2].newchar, tyQCoDv[2].heartbeat);

		printf(" Ch3:%d br=0x%02x, SR=0x%02x, char=%c, hb=%d\n",
			tyQCoDv[3].created, (tyQCoDv[3].baud & 0xff),
			(*tyQCoDv[3].sr & 0xff), tyQCoDv[3].newchar, tyQCoDv[3].heartbeat);

		printf("    : kimr=0x%02x, ISR=0x%02x, IPCR=0x%02x, IPR=0x%02x, OPR=0x%02x\n",
			(*tyQCoDv[2].kimr & 0xff), (*tyQCoDv[2].isr & 0xff),
			(*tyQCoDv[2].ipcr & 0xff), (*tyQCoDv[2].ipr & 0xff),
			(*tyQCoDv[2].opr & 0xff) );
	}
	else
	printf("Quart deviced NOT Created yet!\n");

}


/*******************************************************************************
*
* quartShow - report register status for the QUART
*
* RETURNS: N/A
*/

void quartShow()
{
int ii;			/* scratch */
char mrReg0[4];
char mrReg1[4];
char mrReg2[4];
char srReg[4];
char ipcr_oprReg[4];
char isr_iprReg[4];
char bcrReg[4];
char imrReg[4];
char cirReg;
char gicrReg;
char icrReg;
int heartbeatReg[4];
char baudReg[4];

int lock = intLock ();	/* LOCK INTERRUPTS */

	for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
	{
		*tyQCoDv [ii].cr  = SET_MR_PTR_ZRO;
		mrReg0[ii] = *tyQCoDv [ii].mr; /* read contents of reg */
		mrReg1[ii] = *tyQCoDv [ii].mr;
		mrReg2[ii] = *tyQCoDv [ii].mr;
		*tyQCoDv [ii].cr  = RST_MR_PTR_CMD;
		srReg[ii] = *tyQCoDv [ii].sr;
		ipcr_oprReg[ii] = *tyQCoDv [ii].ipcr;
		isr_iprReg[ii] = *tyQCoDv [ii].isr;
		bcrReg[ii] = *tyQCoDv [ii].bcr;
		imrReg[ii] = *tyQCoDv [ii].kimr;
		heartbeatReg[ii] = tyQCoDv [ii].heartbeat;
		baudReg[ii] = tyQCoDv [ii].baud;
	}

	cirReg = *QSC_CIR;
	gicrReg = *QSC_IVR;
	icrReg = *QSC_ICR;

	intUnlock (lock);				/* UNLOCK INTERRUPTS */

/**********************************
* Now print the results out
*/

	printf("\n");

	for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
	{
		printf("Q%d mr0=0x%02x, mr1=0x%02x, mr2=0x%02x, sr=0x%02x, bcr=0x%02x, imr=0x%02x, baud=0x%x, hb=%d\n",
			ii, (mrReg0[ii] & 0xff), (mrReg1[ii] & 0xff), (mrReg2[ii] & 0xff),
			(srReg[ii] & 0xff), (bcrReg[ii] & 0xff), (imrReg[ii] & 0xff),
			(baudReg[ii]), heartbeatReg[ii]);

		if(ii & 1)
			printf("ipcr=0x%02x, isr=0x%02x, opr=0x%02x, ipr=0x%02x\n",
				(ipcr_oprReg[ii-1] & 0xff), (isr_iprReg[ii-1] & 0xff),
				(ipcr_oprReg[ii] & 0xff), (isr_iprReg[ii] & 0xff) );
	}
	printf("cir=0x%02x, gicr=0x%02x, icr=0x%02x\n",
		(cirReg & 0xff), (gicrReg & 0xff), (icrReg & 0xff) );

}
#endif

/* end sc68c94Serial.c */
