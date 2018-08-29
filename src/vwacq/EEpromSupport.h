/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCautoEEh
#define INCautoEEh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*   Defines Here */

 
#define OUTPUT_XEP_BASE ((volatile unsigned long* const) 0xF0400000)
#define OUTPUT_EEP_BASE ((volatile unsigned long* const) 0xE0400000)
#define STM_EEP_BASE ((volatile unsigned long* const) 0xE0600000)
#define ADC_EEP_BASE ((volatile unsigned long* const) 0xE0800000)
#define AUTO_EEP_BASE ((volatile unsigned long* const) 0xE0A00000)
#define	VME_EEP_SIZE	0x3FFFF	/* 256k x 8 */
#define	VME_EEP_WRITE 0x00000040L	/* Program data cmd */
#define	VME_EEP_VERIFY 0x000000C0L	/* Program data cmd */
#define	VME_EEP_READ 0x00000000L	/* Program data cmd */
#define	VME_EEP_LIMIT 25	/* limit of trys to write */
#define	VME_EEP_MASK 0xfff00000	/* VME is 0xvvv-----, QSPI is 0x---q---- */
#define	QSPI_SHIM_EEP_BASE ((volatile unsigned char* const) 0x000000)


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int dumpEEprom(char *Board, int charsToDump, int startAddr);
extern int chkEEprom(char *Board);
extern int getEEpromInquiry(char *Board);
extern int getEEpromBrdID(char *Board);
extern int getEEpromHex(char *Board, int NoBytes, int EEpromAddr);
extern int getEEpromData(char *Board, int NoBytes, int EEpromAddr);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int dumpEEprom();
extern int chkEEprom();
extern int getEEpromInquiry();
extern int getEEpromBrdID();
extern int getEEpromHex();
extern int getEEpromData();

#endif

#ifdef __cplusplus
}
#endif

#endif

/* End of EEpromSupport.h */
