/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* qspiObj.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/* qspiObj.h 1.5 4/10/95 Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/*
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCqspih
#define INCqspih


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*   Defines Here */

#include <semSmLib.h>

typedef struct {
		int	qspiItrVector;
		int	probeIdItrVector;
		SEM_ID	pQspiSEMid;	/* Mutual exclusion semaphore ID */
		SEM_ID	pQspiSEMBid;	/* synchronization semaphore ID */
		SEM_ID	pQspiSEMBplug;	/* synchronization semaphore ID */
		int	qspispif;	/* 0=op in progress,1=done,2=free */
		int	qspicount;	/* for debug */
		int	qspiconfig;	/* port configuration */
    		char*	pIdStr;	  	  /* Identifier String */
} QSPI_OBJ;

typedef QSPI_OBJ *QSPI_ID;

#define M332_SYS_CLOCK 32768 * 512	/* Automation board clock */

#define	QSPI_CONFIG_NOT 0  /* QSPI indeterminate configured */
#define	QSPI_CONFIG_SHIM 1  /* QSPI configured for serial shims */
#define	QSPI_CONFIG_EEPROM 2  /* QSPI configured for shim EEPROM */
#define	QSPI_CONFIG_LOCK 3  /* QSPI configured for lock ADC */

#define	NOsemTakeFlag	0x12345678

/* --------- QSPI values for SHIM and EEPROM  ------------ */
#define	NEWQ0        0x0000  /* New Queue pointing to cmd 0 */
#define	NEWQ1        0x0001  /* New Queue pointing to cmd 1 */
#define	NEWQ2        0x0002  /* New Queue pointing to cmd 2 */
#define	NEWQ3        0x0003  /* New Queue pointing to cmd 3 */
#define	NEWQ4        0x0004  /* New Queue pointing to cmd 4 */
#define	NEWQ5        0x0005  /* New Queue pointing to cmd 5 */
#define	NEWQ6        0x0006  /* New Queue pointing to cmd 6 */
#define	NEWQ7        0x0007  /* New Queue pointing to cmd 7 */
#define	NEWQ8        0x0008  /* New Queue pointing to cmd 8 */
#define	NEWQ9        0x0009  /* New Queue pointing to cmd 9 */
#define	NEWQ10       0x000A  /* New Queue pointing to cmd A */
#define	NEWQ11       0x000B  /* New Queue pointing to cmd B */
#define	NEWQ12       0x000C  /* New Queue pointing to cmd C */
#define	NEWQ13       0x000D  /* New Queue pointing to cmd D */
#define	NEWQ14       0x000E  /* New Queue pointing to cmd E */
#define	NEWQ15       0x000F  /* New Queue pointing to cmd F */
#define	ENDQ0        0x0000  /* End Queue pointing to cmd 0 */
#define	ENDQ1        0x0100  /* End Queue pointing to cmd 1 */
#define	ENDQ2        0x0200  /* End Queue pointing to cmd 2 */
#define	ENDQ3        0x0300  /* End Queue pointing to cmd 3 */
#define	ENDQ4        0x0400  /* End Queue pointing to cmd 4 */
#define	ENDQ5        0x0500  /* End Queue pointing to cmd 5 */
#define	ENDQ6        0x0600  /* End Queue pointing to cmd 6 */
#define	ENDQ7        0x0700  /* End Queue pointing to cmd 7 */
#define	ENDQ8        0x0800  /* End Queue pointing to cmd 8 */
#define	ENDQ9        0x0900  /* End Queue pointing to cmd 9 */
#define	ENDQ10       0x0A00  /* End Queue pointing to cmd A */
#define	ENDQ11       0x0B00  /* End Queue pointing to cmd B */
#define	ENDQ12       0x0C00  /* End Queue pointing to cmd C */
#define	ENDQ13       0x0D00  /* End Queue pointing to cmd D */
#define	ENDQ14       0x0E00  /* End Queue pointing to cmd E */
#define	ENDQ15       0x0F00  /* End Queue pointing to cmd F */

/* SHIM Serial EEPROM commands */
#define	SEP_WRSR    0x01  /* Write Status Register */
#define	SEP_WRITE0  0x02  /* Write lower memory */
#define	SEP_WRITE1  0x0A  /* Write upper memory */
#define	SEP_READ0   0x03  /* Read lower memory */
#define	SEP_READ1   0x0B  /* Read upper memory */
#define	SEP_RDSR    0x05  /* Read Status Register */
#define	SEP_WREN    0x06  /* set Write Enable Latch */

#define	EEpromPCSno	0x02	/* PCS = 2 ShimEEprom */
#define	PCSn0       0x00  /* PCS = 0 */
#define	PCSn1       0x01  /* PCS = 1 */
#define	PCSn2       0x02  /* PCS = 2 */
#define	PCSn3       0x03  /* PCS = 3 */
#define	PCSn4       0x04  /* PCS = 4 */
#define	PCSn5       0x05  /* PCS = 5 */
#define	PCSn6       0x06  /* PCS = 6 */
#define	PCSn7       0x07  /* PCS = 7 */
#define	PCSn8       0x08  /* PCS = 8 */
#define	PCSn9       0x09  /* PCS = 9 */
#define	PCSn10      0x0A  /* PCS = 10 */
#define	PCSn11      0x0B  /* PCS = 11 */
#define	PCSn12      0x0C  /* PCS = 12 */
#define	PCSn13      0x0D  /* PCS = 13 */
#define	PCSn14      0x0E  /* PCS = 14 */

#define CMDramCONT  0x80  /* 1 = Keep PCS asserted after transfer complete. */
#define	CMDramBITSE 0x40  /* 1 = Number bits set in SPCR0, 0 = 8 bits.
#define	CMDramDT    0x20  /* 1 = Use DTL field in SPCR0, 0 = 1us delay. */
#define	CMDramDSCK  0x10  /* 1 = Use DSCK field in SPCR0, 0 = 1/2 SCK */

#define QSMiarb 8 /* QSM is level 8 */
#define QMCRmisc 0x0000 /* Normal operation only */
#define QMCRsupv 0x0080 /* SUPV bit only set */
#define QMCRinit QMCRmisc | QMCRsupv |QSMiarb

/* Shim Serial DACs defaults */
#define QPDRSDinit 0x7E	/* default value of PCS,SCK & MOSI pins is High */
/* Shim Serial EEPROM defaults */
#define QPDRSSinit 0x7E	/* default value of PCS,SCK & MOSI pins is High */
/* Lock Serial ADC defaults */
#define QPDRLKinit 0x7A	/* default value of PCS & MOSI pins High SCK Low */

#define QPARpcsn 0x7800 /* all PCS pins are assigned to QSPI module */
#define QPARmomi 0x0300 /* Master OUt, Master In to QSPI module */
#define QDDRio   0x00FE /* all except MISO are out */
#define QPARinit QPARpcsn | QPARmomi | QDDRio

#define SPCR0mstr   0x8000  /* Select Master mode */
#define SPCR0womq   0x0000  /* Use normal output mode */
#define SPCR08bits  0x2000  /* pattern for 8 bit transfer (ADC) */
#define SPCR016bits 0x0000  /* pattern for 16 bit transfer (Shim) */
#define SPCR0hcpol  0x0200  /* Inactive state of SCK Clock is high */
#define SPCR0lcpol  0x0000  /* Inactive state of SCK Clock is low */
#define SPCR0xcpha  0x0100  /* Change data LE-SCK Capture data TE */
#define SPCR0ycpha  0x0000  /* Capture data LE-SCK Change data TE */
#define SPCR0spbr4  0x0002  /* Baud rate is 3.997696 Mhz */
#define SPCR0spbr2  0x0004  /* Baud rate is 1.998848 Mhz */
#define SPCR0spbr1  0x0008  /* Baud rate is 990424 Khz */
#define SPCR0spbr9  0x0009  /* Baud rate is 932 Khz */
/* Sinit for Shim DACs 2 Mhz clock SCK high change data on leading edge */
#define SPCR0Sinit   SPCR0mstr | SPCR0womq | SPCR016bits | SPCR0hcpol | SPCR0xcpha | SPCR0spbr2
/* Linit for Lock ADC 1 Mhz clock  SCK low capture on leading edge*/
#define SPCR0Linit   SPCR0mstr | SPCR0womq | SPCR08bits | SPCR0lcpol | SPCR0ycpha | SPCR0spbr1
/* Einit for Shim EEprom 990 Khz clock SCK high capture data on leading edge */
#define SPCR0SEinit   SPCR0mstr | SPCR0womq | SPCR016bits | SPCR0hcpol | SPCR0ycpha | SPCR0spbr1

#define SPCR1addr  0xFFFC1A  /* QSPI Control Register 1 */
      

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern QSPI_ID  qspiCreate();
extern void qspiReset();
extern void qspiItrpEnable();
extern void qspiItrpDisable();
extern short qspiStatReg();
extern short qspiCntrlReg();
extern short qspiIntrpMask();
extern void qspiShow();

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern QSPI_ID  qspiCreate();
extern void qspiReset();
extern void qspiItrpEnable();
extern void qspiItrpDisable();
extern short qspiStatReg();
extern short qspiCntrlReg();
extern short qspiIntrpMask();
extern void qspiShow();

#endif

#ifdef __cplusplus
}
#endif

#endif
