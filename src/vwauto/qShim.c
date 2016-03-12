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
/* Automation QSM control program for the shim module. */

#include "qspiObj.h"
#include "m68332_nmri.h"

#define	COMD_RAM_CONT  0x80	/* Keep PSC asserted after xfer complete */
#define	COMD_RAM_BITSE 0x40 /* Number of bits set in SPCR0 else 8 bits */
#define	COMD_RAM_DT    0x20 /* Use SPCR1 DTL for delay after xfer */
#define	COMD_RAM_DSCK  0x10 /* Use SPCR1 DSCKL delay else 1/2 SCK */

extern QSPI_ID pTheQspiObject;
extern int shimDebug;

/********************************************************
*
* shimDacSet - Place a DAC value in proper DAC
*     DACno is a value between 0 and 15, refering to DACs as follows:
*       0-15 are Z0,Z1C,Z1F,Z2C,Z2F,Z3,Z4,Z5,X,Y,XZ,YZ,X2Y2,XY,X3,Y3.
*     DACno = -1 will read port 1 for shim status.
*
*     SHIMvalue is a 2's complement integer between -2048 and +2047
*
*     IF DACno is out of bounds No DAC is changed and return ERROR
*     IF SHIMvalue is out of bounds value will be truncated to max
*        positive or negative value.
*
* RETURNS:
*  OK or status if port 1, unless DACno is out of bounds then ERROR.
*/

int shimDacSet(short DACno,short SHIMvalue)
{
QSPI_OBJ *pQspiObj;
int ShimStatus;
short QSPIvalue,dacvalue,outno,dacno;
unsigned short pcsno,stat;
int waitTime;

unsigned char *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

	if(shimDebug > 10) printf("shimDacSet:\n");

	if( (DACno > 15) || (DACno < -1) ) return(ERROR);
	pQspiObj = pTheQspiObject;

/* The DAC8420 has 4 DACs in each package. The DACs are numbered such
|	that every 4 DACs is in the next package and the first package is
|	at pcs address 3.
*/
	if(DACno != -1)
	{
		pcsno = (DACno / 4) + 3;/* calculate pcs number */
		dacno = DACno % 4;	/* calculate DAC in that package */
		outno = dacno << 14;	/* position number at bits 14,15. */

		if(shimDebug > 10)
			printf("shimDacSet: pcsno=%d, dacno=%d, outno=%04x\n",
				pcsno,dacno,outno);

		/* convert 2's complement value to an offset value
		/* where 0V. = 0x800 */
		if (SHIMvalue > 2047)
		   dacvalue = 4095;
		else if (SHIMvalue < -2048)
		   dacvalue = 0;
                else
                   dacvalue = SHIMvalue + 2048;
	}
	else
	{
		pcsno = 1;	/* set for status channel */
		dacno = outno = 0;
		dacvalue = SHIMvalue;
	}

	QSPIvalue = dacvalue | outno;  /* actual value transmitted to DAC */


	if(shimDebug > 10)
           printf("DACno=%2d,pcs=%d,SHIMv=%08x[%5d],dacno=%d,dacv=%08x[%5d],QSPI=%04x\n",
	           DACno,pcsno,SHIMvalue,SHIMvalue,dacno,dacvalue,dacvalue,QSPIvalue);

	/* check if already configured, if so don't waste time doing it again */

#ifdef INSTRUMENT
        wvEvent(EVENT_QSHIM_MUTEX,NULL,NULL);
#endif
	if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR)
	   return(10);
	if (pQspiObj->qspiconfig != SPCR0Sinit)
		/* CLK=hi/2.10Mhz, CHG LE, CAP TE, 16bits 0x8304 */
		qspiInit(SPCR0Sinit);

	QSM_RX_RAM   = M332_QSM_RX_BASE;
	QSM_TX_RAM   = M332_QSM_TX_BASE;
	QSM_COMD_RAM = M332_QSM_CMD_BASE;

	QSM_TX_RAM[1] = QSPIvalue;
	QSM_RX_RAM[1] = QSPIvalue;
	QSM_COMD_RAM[1] = COMD_RAM_BITSE | pcsno; /* use 16 bits */

		/* Clear SCI interrupt flags */
	stat = *M332_QSM_SCSR;
	stat = *M332_QSM_SCDR;

	qspiStartWaitForStop(1);

#ifdef INSTRUMENT
        wvEvent(EVENT_QSHIM_CMPLT,NULL,NULL);
#endif

	ShimStatus = (int)QSM_RX_RAM[1];

#ifdef INSTRUMENT
        wvEvent(EVENT_QSHIM_MUTEX,NULL,NULL);
#endif
	semGive(pQspiObj->pQspiSEMid);

	return(ShimStatus);

} /* end shimDacSet() */

