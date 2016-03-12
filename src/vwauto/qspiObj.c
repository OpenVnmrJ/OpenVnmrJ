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


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
#include <msgQLib.h>
#include "commondefs.h"
#include "globals.h"
#include "hardware.h"
#include "logMsgLib.h"
#include "m68332_nmri.h"
#include "qspiObj.h"
#include "hostAcqStructs.h"
#include "instrWvDefines.h"


/*
modification history
--------------------
1-30-95,gmb  created 
*/

#ifdef MSRII
#define PROBEID_PRIORITY	60
#define PROBEID_TASK_OPTIONS	0
#define PROBEID_STACK_SIZE	2048
extern void qspiProbeISR(QSPI_ID pQspiId);
extern void probeIdTask();
extern char probeId[];
#endif

static char *qspiIDStr ="QSPI Object";

/*-------------------------------------------------------------
| Qspi Initialization Routine
+--------------------------------------------------------------*/
qspiInit(int spcr0)
{
register QSPI_OBJ *pQspiObj;
   pQspiObj = pTheQspiObject;

   if(shimDebug > 10) printf("qspiInit:\n");

   /* Leave behind this configuration ID */
   pQspiObj->qspiconfig = spcr0;

   /* Set default level for PCS3:0 pins */
   *M332_QSM_QPDR = (UINT8)QPDRSDinit; /* 0x7E PCS,SCK & MOSI pins high */

   /* Set I/O pin assignments and direction */
   /* QPAR=7B, QDDR=FE all pins except MISO are output */
   *M332_QSM_QPA_DDR = QPARinit;

   /* Set baud rate and clock phase */
   /* CLK=hi/2.10Mhz, CHG LE, CAP TE, 16bits 0x8304 */
   *M332_QSM_SPCR0 = spcr0;

   /* SPCR2 Not part of Init, but starts a transfer */

   /* SPCR3 Not used, leave as default. */

   /* SPSR is reset by the interrupt service routine */

   qspiItrpEnable(); /* insure interrupts turned on */

}
/*-------------------------------------------------------------
| Interrupt Service Routine (ISR) 
+--------------------------------------------------------------*/
/*******************************************
*
* qspiMsgDone - Interrupt Service Routine
*
*   qspiMsgDone - Signals the completion of a QSPI event.
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void qspiMsgDone(QSPI_ID pQspiId)
{
   /* Clear finished bit in status register  */
   *M332_QSM_SPSR  &= ~QSM_SPSR_SPIF;
   semGive(pQspiId->pQspiSEMBid);	/* give semaphore, releasing task */
   pQspiId->qspispif = TRUE;		/* Set = TRUE when finished,
						 FALSE when running */
   pQspiId->qspicount ++;		/* Count times interrupt happened
					       (for debug) */

#ifdef INSTRUMENT
	wvEvent(EVENT_QSPI_MSG_DONE,NULL,NULL);
#endif
   return;
}

/*-------------------------------------------------------------
| Start QSPI routine
+--------------------------------------------------------------*/
/*******************************************
*
* qspiStartWaitFroStop
*
*   qspiStartWaitForStop - Set start and stop pointers
*			   Clear finished bit
*			   Start QSPI
*			   Take Semaphore and wait or timeout
*			   The semaphore is Geven by the interrupt routine
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void qspiStartWaitForStop(int numberOfCommands)
{
register QSPI_OBJ *pQspiId;
   pQspiId = pTheQspiObject;
   pQspiId->qspispif = FALSE;		/* Set = TRUE when finished,
						 FALSE when running */
   /* Enable interrupts and set up start and stop ram pointers */
   /* Enable SPIFIE 0x8000 */
   *M332_QSM_SPCR2 = QSM_SPCR2_SPIFIE | NEWQ1 | (numberOfCommands << 8);

   /* Clear finished bit in status register  */
   *M332_QSM_SPSR  &= ~QSM_SPSR_SPIF;

   /* Enable QSPI and delay 1/2 clock to SCK, set delay 0x8001 */
   *M332_QSM_SPCR1 = QSM_SPCR1_SPE | 1;

   /* Take the semiphore and either time out (Error) or return */
   if ( semTake(pQspiId->pQspiSEMBid,numberOfCommands*sysClkRateGet()/60) == ERROR) 
   {  printf("qspiStartWaitForStop(): semTake returned ERROR");
   }
}

/*-------------------------------------------------------------
| Get QSPI status routine
+--------------------------------------------------------------*/
/*******************************************
*
* qspiStartWaitFroStop
*
*   qspiStartWaitForStop - 
*		arg0  = EEprom address number
*		arg2  = word to use to initialize the SPCR0 register
*			has clock speed and edge info
*
* RETURNS:
*  the EEprom status
*
* NOMANUAL
*/
int qspiStatus(int PCSno, int spcr0)
{
register QSPI_OBJ *pQspiObj;
unsigned short stat;
int status;

unsigned char  *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

   if (shimDebug > 10) printf("qspiStatus1:\n");
   pQspiObj = pTheQspiObject;

   /* check if already configured, if so don't waste time doing it again */
   if (pQspiObj->qspiconfig != spcr0)
      qspiInit(spcr0);

   QSM_RX_RAM   = M332_QSM_RX_BASE;
   QSM_TX_RAM   = M332_QSM_TX_BASE;
   QSM_COMD_RAM = M332_QSM_CMD_BASE;

   QSM_COMD_RAM[1] = CMDramBITSE | PCSno;
   QSM_TX_RAM[1]   = (SEP_RDSR <<8);
   QSM_RX_RAM[1]   = 0;

   qspiStartWaitForStop(1);	/* start and wait for finish */

   status = (int)QSM_RX_RAM[1];

   if (shimDebug > 10) printf("qspiStatus2: %x\n",status);
   return(status);

} /* end qspiStatus() */

/*-------------------------------------------------------------
| Read/Write 16 bytes from the EEprom
+--------------------------------------------------------------*/
/*******************************************
*
* qspiIO16
*
*   qspiIO16 - 
*		startAddress  = first EEprom address to read/write
*		toFrom        = address to return bytes to/get bytes from
*		readNotWrite  = read/not write flag
*		PCSno	      = address of EEprom
*
* RETURNS:
*   ERROR or OK
*
* NOMANUAL
*/
int qspiIO16(int startAddress, char *toFrom, 
            int readNotWrite, int PCSno)
{
register char  *xmitptr;
unsigned char  *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

short	cmd;

int j;

   if (shimDebug > 10)
      printf("qspiIO16(): start=%2d, toFrom=%x, RW=%d pcs=%d\n",
		startAddress,toFrom,readNotWrite, PCSno);

   /* for now satrtAddress is 16 byte blocks and block the same */
   if (startAddress > 496) return(ERROR);

   if ( ! readNotWrite )
   {  /* select write command for upper byte and address for low byte */
      if (startAddress > 0xFF) cmd = (SEP_WRITE1 << 8) | (startAddress & 0xFF);
       else                    cmd = (SEP_WRITE0 << 8) | (startAddress & 0xFF);
   }
   else
   {  /* select read command for upper byte and address for low byte */
      if (startAddress > 0xff) cmd = (SEP_READ1 << 8) | (startAddress & 0xFF);
      else                     cmd = (SEP_READ0 << 8) | (startAddress & 0xFF);
   }

   QSM_RX_RAM   = M332_QSM_RX_BASE;
   QSM_TX_RAM   = M332_QSM_TX_BASE;
   QSM_COMD_RAM = M332_QSM_CMD_BASE;

   if ( ! readNotWrite )
      QSM_COMD_RAM[1] = PCSno; /* use 8 bits for Write Enable command */
   else
      QSM_COMD_RAM[1] =  CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[2] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[3] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[4] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[5] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[6] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[7] = CMDramCONT | CMDramBITSE | PCSno;
   QSM_COMD_RAM[8] = CMDramCONT | CMDramBITSE | PCSno;

   if ( ! readNotWrite) 
   {  QSM_COMD_RAM[9] = CMDramCONT | CMDramBITSE | PCSno;
      QSM_COMD_RAM[10] = CMDramBITSE | PCSno;

      QSM_TX_RAM[1] = SEP_WREN;	/* set write enable latch */
      QSM_TX_RAM[2] = cmd;  		/* write command */

      xmitptr = (char*)M332_QSM_TX_BASE + 6; /* point to byte 6 */
      for (j=0;j<16;j++) 
         *xmitptr++ = *toFrom++; /* move 16 bytes of data */

      qspiStartWaitForStop(10);
   }
   else
   {  QSM_COMD_RAM[9] = CMDramBITSE | PCSno;

      QSM_TX_RAM[1] = cmd;

      qspiStartWaitForStop(9);

      /* Transfer data to calling routine buffer */
      xmitptr = (char*)M332_QSM_RX_BASE + 4;
      xmitptr[16]='\000';

      for (j=0;j<16;j++)
         *toFrom++ = *xmitptr++;
    }
    return(OK);
}

/*-------------------------------------------------------------
| QSPI Object Public Interfaces
+-------------------------------------------------------------*/

/**************************************************************
*
*  qspiCreate - create the QSPI Object Data Structure & Semiphores
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semiphore creation failed
*
*/ 
QSPI_ID  qspiCreate()
{
        void qspiMsgDone(QSPI_ID pQspiId);
	char tmpstr[80];
	register QSPI_OBJ *pQspiObj;
	short sr;

	/* ------- malloc space for QSPI Object --------- */
	if ( (pQspiObj = (QSPI_OBJ *) malloc( sizeof(QSPI_OBJ)) ) == NULL )
	{
		errLogSysRet(LOGIT,debugInfo,"qspiCreate: Could not Allocate Space:");
		return(NULL);
	}

	/* zero out structure so we don't free something by mistake */
	memset(pQspiObj,0,sizeof(QSPI_OBJ));

	/* ------- Create Mutual exclusion semaphore & point to it ---------- */
	pQspiObj->pQspiSEMid =
	    semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
	if( pQspiObj->pQspiSEMid == NULL ) return(NULL);

	/* ------- Create Binary synchronization semaphore & point to it ---- */
	/* ------- This semaphore is used to sync the read task and the  ---- */
	/* ------- qspi finished interupt.                               ---- */
	pQspiObj->pQspiSEMBid = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	if( pQspiObj->pQspiSEMBid == NULL ) return(NULL);

#ifdef MSRII
	/* ------- Create Binary synchronization semaphore & point to it ---- */
	/* ------- This semaphore is used to sync the read probeId task  ---- */
	/* ------- and the plug in cable interrupt.                      ---- */
	pQspiObj->pQspiSEMBplug = semBCreate(SEM_Q_FIFO, SEM_FULL);
	if( pQspiObj->pQspiSEMBplug == NULL ) return(NULL);
#endif

	/* ------- point to Id String ---------- */
	pQspiObj->pIdStr = qspiIDStr;


	/* ------- reset board and get status register */
	qspiReset(); /* reset QSPI port */

	/* ------- Disable all QSPI interrupts on board -----------*/
	qspiItrpDisable();

	/* the QSM configuration register and interrupt vector has been set
			by the O.S. when it set up the SCI port. */

	/* ------- Connect QSPI interrupt vector to proper ISR to Give ----- */
	pQspiObj->qspiItrVector = ( *M332_QSM_QIVR | 1);
	if ( intConnect(INUM_TO_IVEC( pQspiObj->qspiItrVector),  
		     qspiMsgDone, pQspiObj) == ERROR)
	{
		errLogSysRet(LOGIT,debugInfo,
		"qspiCreate: Could not connect QSPI ISR: ");
		qspiDelete();
		return(NULL);
	}

#ifdef MSRII
	if (MSRType == AUTO_BRD_TYPE_II) 
        {  /* ---- Connect QSPI interrupt vector to proper ISR to Give ----- */
           char	AutoInt;
           int	tProbePlugIn;
	   pQspiObj->probeIdItrVector = 0x54;
	   if ( intConnect(INUM_TO_IVEC( pQspiObj->probeIdItrVector),
		     qspiProbeISR, pQspiObj) == ERROR)
	   {
		errLogSysRet(LOGIT,debugInfo,
		"qspiCreate: Could not connect QSPI ISR for ProbeId: ");
		qspiDelete();
		return(NULL);
	   }


           tProbePlugIn = taskSpawn("tPrbPlgIn", PROBEID_PRIORITY,
		PROBEID_TASK_OPTIONS, PROBEID_STACK_SIZE, 
		probeIdTask, NULL,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10); 
           if ( tProbePlugIn == ERROR)
           {
              errLogSysRet(LOGIT,debugInfo,
                 "qspiCreate: could not spawn probeIdTask:");
           }
           AutoInt = *(unsigned char*)0x20100b; /* get current interrupt mask */
           AutoInt &= 0xfd;        /* unmask misc Interrupt */
           *(unsigned char*)0x20100b = AutoInt; /* doit toit */

        }
#endif

	return( pQspiObj );
}


/**************************************************************
*
*  qspiDelete - Deletes QSPI Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int qspiDelete()
{
	if (pTheQspiObject != NULL) free(pTheQspiObject);
}


/**************************************************************
*
*  qspiItrpEnable - Set the QSPI Interrupt Mask
*
*  This routine sets the QSPI interrupt mask equal to the system SCI level.
*
* RETURNS:
* void 
*
*/ 
void qspiItrpEnable()
{

	if (pTheQspiObject == NULL) return;
	*M332_QSM_QILR |= (*M332_QSM_QILR & 0x7) << 3;
	if(shimDebug > 10) printf("qspiItrpEnable: QILR = %x\n", *M332_QSM_QILR);
}

/**************************************************************
*
*  qspiItrpDisable - Set the QSPI Interrupt Mask
*
*  This routine sets the QSPI interrupt mask to 0.
*
* RETURNS:
* void 
*
*/ 
void qspiItrpDisable()
{
	if (pTheQspiObject == NULL) return;
	*M332_QSM_QILR &= 0x7;

}

/**************************************************************
*
*  qspiReset - Resets QSPI functions 
*
*
* RETURNS:
* 
*/
void qspiReset()
{
	int state;

	if (pTheQspiObject == NULL) return;
}

/**************************************************************
*
*  qspiStatReg - Gets QSPI status register value
*
*
* RETURNS:
*  8-bit QSPI Status Register Value
*/
short qspiStatReg()
{
	if (pTheQspiObject == NULL) return(-1);

	return (*M332_QSM_SPSR);
}

/**************************************************************
*
*  qspiCntrlReg - Gets QSPI control register value
*
*
* RETURNS:
*  16-bit QSPI Control Register Value
*/
short qspiCntrlReg(Q)
{
	if (pTheQspiObject == NULL) return(-1);

	return (0);
}



/**************************************************************
*
*  qspiIntrpMask - Gets QSPI Interrupt Register mask
*
*
* RETURNS:
*  WhoKnows
*/
short qspiIntrpMask()
{
	if (pTheQspiObject == NULL) return(-1);
	return ( (*M332_QSM_QILR & 0x38) >> 3);
}

#ifdef DIAG

/********************************************************************
* qspiShow - display the status information on the QSPI Object
*
*  This routine display the status information of the QSPI Object
*
*
*  RETURN
*   VOID
*
*/
void qspiShow()
{
	register QSPI_OBJ *pQspiObj;
	int i;
	int avail=ERROR;
	char *pstr;
	unsigned short status;
	unsigned short *dataptr;
	unsigned char *cmdptr;
	unsigned char cmdram[16];
	unsigned short txram[16],rxram[16];
	unsigned char qilr,qivr,qpdr,qpar,qddr,spcr3,spsr;
	unsigned short qmcr,spcr0,spcr1,spcr2;
	int qspispif,qspicount,qspiconfig;
	char tmpbfr[120];

	if (pTheQspiObject == NULL)
	{
		printf("qspiShow: QSPI Object pointer is NULL.\n");
	}
	else
	{
		pQspiObj = pTheQspiObject;

		/* check if in use, if so say so but still get data
		     if not in use, reserve it long enough to get data */

		avail = semTake(pQspiObj->pQspiSEMid,NO_WAIT);
		qspiconfig = pQspiObj->qspiconfig;
		qspicount = pQspiObj->qspicount;
		qspispif = pQspiObj->qspispif;
	}

	printf("\n\n-------------------------------------------------------------\n\n");

	/* get all the data quickly and process it later */
	qmcr = *M332_QSM_QMCR;
	qilr = *M332_QSM_QILR;
	qivr = *M332_QSM_QIVR;
	qpdr = *M332_QSM_QPDR;
	qpar = *M332_QSM_QPAR;
	qddr = *M332_QSM_QDDR;
	spcr0 = *M332_QSM_SPCR0;
	spcr1 = *M332_QSM_SPCR1;
	spcr2 = *M332_QSM_SPCR2;
	spcr3 = *M332_QSM_SPCR3;
	spsr = *M332_QSM_SPSR;
	cmdptr = M332_QSM_CMD_BASE;
	for(i=0;i<16;i++) cmdram[i]=*cmdptr++;
	dataptr = M332_QSM_TX_BASE;
	for(i=0;i<16;i++) txram[i]=*dataptr++;
	dataptr = M332_QSM_RX_BASE;
	for(i=0;i<16;i++) rxram[i]=*dataptr++;

	if(avail == OK) semGive(pQspiObj->pQspiSEMid);

	if (pQspiObj != NULL)
	{
		printf("QSPI: %s, config=%d, Vector= %lx, SPIF=%d, SEMid=%lx, ISR count = %ld\n",
			pQspiObj->pIdStr,qspiconfig,pQspiObj->qspiItrVector,qspispif,
			pQspiObj->pQspiSEMid,qspicount);

		printf("ProbeId: Vector=%d, `%s`\n",
			pQspiObj->probeIdItrVector,probeId);

	}

	printf("      QMCR (STOP=%c, FRZ1=%c, FRZ0=%c, SUPV=%c, IARB=%d)\n",
		(qmcr & QSM_MCR_STOP) ? '1' : '0', (qmcr & QSM_MCR_FRZ1) ? '1' : '0',
		(qmcr & QSM_MCR_FRZ0) ? '1' : '0', (qmcr & QSM_MCR_SUPV) ? '1' : '0',
		(qmcr & QSM_MCR_IARB) );

	printf("      QILR/QIVR (ILQSPI=%d, ILSCI=%x, INTV=%d [0x%x])\n",
		( (qilr & QSM_QILR_SPI_MASK) >> QSM_QILR_SPI_SHIFT),
		(qilr & QSM_QILR_SCI_MASK), qivr, qivr);

	printf("      QPDR (TXD=%c, PCS3:0=%d, SCK=%c, MOSI=%c, MISO=%c)\n",
		(qpdr & QSM_QPDR_TXD) ? '1' : '0',
		( (qpdr & QSM_QPDR_PCS) >> QSM_QPDR_PCS_SHIFT),
		(qpdr & QSM_QPDR_SCK) ? '1' : '0',
		(qpdr & QSM_QPDR_MOSI) ? '1' : '0', (qpdr & QSM_QPDR_MISO) ? '1' : '0');

	printf("      QPAR (PCS3=%s, PCS2=%s, PCS1=%s, PCS0=%s, MOSI=%s, MISO=%s)\n",
		(qpar & QSM_QPAR_PCS3) ? "QSPI" : "I/O",
		(qpar & QSM_QPAR_PCS2) ? "QSPI" : "I/O",
		(qpar & QSM_QPAR_PCS1) ? "QSPI" : "I/O",
		(qpar & QSM_QPAR_PCS0) ? "QSPI" : "I/O",
		(qpar & QSM_QPAR_MOSI) ? "QSPI" : "I/O",
		(qpar & QSM_QPAR_MISO) ? "QSPI" : "I/O");

	printf(tmpbfr,"      QDDR (TXD=%c, PCS3=%c, PCS2=%c, PCS1=%c, PCS0=%c, SCK=%c, MOSI=%c, MISO=%c)\n",
		(qddr & QSM_QDDR_TXD) ? 'O' : 'I', (qddr & QSM_QDDR_PCS3) ? 'O' : 'I',
		(qddr & QSM_QDDR_PCS2) ? 'O' : 'I', (qddr & QSM_QDDR_PCS1) ? 'O' : 'I',
		(qddr & QSM_QDDR_PCS0) ? 'O' : 'I', (qddr & QSM_QDDR_SCK) ? 'O' : 'I',
		(qddr & QSM_QDDR_MOSI) ? 'O' : 'I', (qddr & QSM_QDDR_MISO) ? 'O' : 'I');

	printf("      SPCR0 (MSTR=%c, WOMQ=%s, BITS=%d, CPOL=%c, CPHA=%s, SPBR=%d)\n",
		(spcr0 & QSM_SPCR0_MASTER) ? 'M' : 'S',
		(spcr0 & QSM_SPCR0_WOMQ) ? "Wor" : "Norm",
		(spcr0 & QSM_SPCR0_BITS) ? ( (spcr0 & QSM_SPCR0_BITS) >> QSM_SPCR0_BITS_SHIFT) : 16, 
		(spcr0 & QSM_SPCR0_CPOL) ? 'H' : 'L',
		(spcr0 & QSM_SPCR0_CPHA) ? "Chg/Cap" : "Cap/Chg",
		M332_SYS_CLOCK / ( 2 * (spcr0 & QSM_SPCR0_SPBR) ) );

	printf("      SPCR1 (SPE=%c, DSCLK=%d, DTL=%d)\n",
		(spcr1 & QSM_SPCR1_SPE) ? "enabled" : "disabled",
		( (spcr1 & QSM_SPCR1_DSCLK) >> QSM_SPCR1_DSCLK_SHIFT),
		(spcr1 & QSM_SPCR1_DTL) );

	printf("      SPCR2 (SPIFIE=%s, WREN=%s, WRTO=%s, ENDQP=%d, NEWQP=%d)\n",
		(spcr2 & QSM_SPCR2_SPIFIE) ? "enabled" : "disabled",
		(spcr2 & QSM_SPCR2_WREN) ? "enabled" : "disabled",
		(spcr2 & QSM_SPCR2_WRTO) ? "enabled" : "disabled",
		( (spcr2 & QSM_SPCR2_ENDQP) >> QSM_SPCR2_ENDQP_SHIFT),
		(spcr2 & QSM_SPCR2_NEWQP) );

	printf("      SPCR2 (LOOPQ=%s, HMIE=%s, HALT=%s)\n",
		(spcr3 & QSM_SPCR3_LOOPQ) ? "enabled" : "disabled",
		(spcr3 & QSM_SPCR3_HMIE) ? "enabled" : "disabled",
		(spcr3 & QSM_SPCR3_HALT) ? "enabled" : "disabled");

	printf("      SPSR (SPIF=%sfinished, MODF=%s, HALTA=%shalted, CPTQP=%d)\n",
		(spsr & QSM_SPSR_SPIF) ? "\0" : "not ",
		(spsr & QSM_SPSR_MODF) ? "fault" : "normal ",
		(spsr & QSM_SPSR_HALTA) ? "\0" : "not ", (spsr & QSM_SPSR_CPTQP) );

	printf("QSPI CMDram:");
	for(i=0;i<M332_QSM_CMD_SIZE;i++)
	{
		printf(" %02x",cmdram[i]);
	}

	printf("\nQSPI TXram:");
	for(i=0;i<M332_QSM_TX_SIZE;i++)
	{
		if(i == 8)
		{
			printf("\n           ");
		}
		printf(" %04x",txram[i]);
	}

	printf("\nQSPI RXram:");
	for(i=0;i<M332_QSM_RX_SIZE;i++)
	{
		if(i == 8)
		{
			printf("\n           ");
		}
		printf(" %04x",rxram[i]);
	}

	printf("\n-------------------------------------------------------------\n\n");

/*
	printf("       %s\n",(status & QSPI_INTR_PEND) ? "Interrpt Pending" :  "No Interrupts Pending");
	return;
*/
}
#endif

getnGiveQspiMutex()
{
   register QSPI_OBJ *pQspiObj;

   pQspiObj = pTheQspiObject;
   if (pQspiObj->pQspiSEMid == NULL)
      return;
   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR) 
      return(10);
   semGive(pQspiObj->pQspiSEMid);
}
