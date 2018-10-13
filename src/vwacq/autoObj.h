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


#ifndef INCautoh
#define INCautoh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define GEN_MBOX 	0xA
#define SPIN_MBOX	0xB
#define SHIM_MBOX	0xC
#define VT_MBOX		0xD

#define AUTO_MBOX_MAXSIZE  128

#define TIMEOUT_SECS	25  /* seconds mutex timeout */
#define QCMD_TIMEOUT 0.50 /* 1/2 sec quick commands */
#define MCMD_TIMEOUT 1.00 /* 1 sec medium length commands */
#define LCMD_TIMEOUT 20.0 /* 20  sec Long commands (e.g. insert) */

/* see vmeIntrp.h for interrupt vector Numbers */


typedef struct {
      unsigned long 	autoBaseAddr;
      unsigned long 	autoMemAddr;
      unsigned long 	autoSharMemAddr;  /* Shared Memory Address */
      unsigned long 	autoMSREndMemAddr;  /* End of Real Memeory on MSR (heart beat loc) */
		int	vmeItrVector;
		int	vmeItrLevel;
		short	ApBusAddr;	/* 1st of 4 */
		short	autoBrdVersion;
    		char*	pIdStr;	  	  /* Identifier String */
} AUTO_OBJ;

typedef AUTO_OBJ *AUTO_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

AUTO_ID  autoCreate(unsigned long baseAddr, unsigned long memAddr, unsigned long sharmemAddr, int apBusAddr, int vector, int level, char* idstr);
extern int autoDelete(AUTO_ID pAutoId);
extern void autoItrpEnable(AUTO_ID pAutoId, int mask);
extern void autoItrpDisable(AUTO_ID pAutoId, int mask);
extern int  autoSetDebugLevel(AUTO_ID pAutoId,int dlevel);
extern void autoReset(AUTO_ID pAutoId, int options);
extern short autoStatReg(AUTO_ID pAutoId);
extern short autoCntrlReg(AUTO_ID pAutoId);
extern short autoIntrpMask(AUTO_ID pAutoId);
extern unsigned long autoGetHeartBeat(AUTO_ID pAutoId);
extern int autoGetMSRType(AUTO_ID pAutoId);
extern int autoSampleEject(AUTO_ID pAutoId);
extern int autoSampleInsert(AUTO_ID pAutoId);
extern int autoSpinRateSet(AUTO_ID pAutoId,int rate);
extern int autoSpinSpeedSet(AUTO_ID pAutoId,int speed);
extern int autoSpinSetMASThres(AUTO_ID pAutoId,int threshold);
extern int autoSpinSetRegDelta(AUTO_ID pAutoId,int hz);
extern int autoLkValueGet(AUTO_ID pAutoId);
extern int autoSpinValueGet(AUTO_ID pAutoId);
extern int autoShimsPresentGet( AUTO_ID pAutoId );
extern int autoGenShimApCodes(AUTO_ID pAutoId, int dac, int value, unsigned long *pCodes);
extern int autoSerialShimsPresent(AUTO_ID pAutoId );
extern int autoSpinReg(AUTO_ID pAutoId);
extern int autoSpinZero(AUTO_ID pAutoId);
extern int autoSampleDetect(AUTO_ID pAutoId);
extern int autoLockSense(AUTO_ID pAutoId);
extern int autoGenrlMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size);
extern int autoSpinMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size,double timeout);
extern int autoVTMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size);
extern int autoShimMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size);
extern int autoLkValueGet(AUTO_ID pAutoId);
extern int autoAirSet(AUTO_ID pAutoId, int cmd);
extern int autoAirUnset(AUTO_ID pAutoId, int cmd);
extern int autoIntrpTest(AUTO_ID pAutoId, int interrupt);
extern short autoLSDVget(AUTO_ID pAutoId);
extern void autoShow(AUTO_ID pAdcId, int level);


#else
/* --------- NON-ANSI/C++ prototypes ------------  */

AUTO_ID  autoCreate();
extern int autoDelete();
extern void autoItrpEnable();
extern void autoItrpDisable();
extern int  autoSetDebugLevel();
extern void autoReset();
extern short autoStatReg();
extern short autoCntrlReg();
extern short autoIntrpMask();
extern unsigned long autoGetHeartBeat();
extern int autoGetMSRType();
extern int autoSampleEject();
extern int autoSampleInsert();
extern int autoLkValueGet();
extern int autoSpinValueGet();
extern int autoShimsPresentGet();
extern int autoSerialShimsPresent();
extern int autoSpinRateSet();
extern int autoSpinSpeedSet();
extern int autoSpinSetMASThres();
extern int autoSpinSetRegDelta();
extern int autoSpinReg();
extern int autoSpinZero();
extern int autoSampleDetect();
extern int autoLockSense();
extern int autoGenrlMsgSend();
extern int autoSpinMsgSend();
extern int autoVTMsgSend();
extern int autoShimMsgSend();
extern int autoGenShimApCodes();
extern int autoLkValueGet();
extern int autoAirSet();
extern int autoAirUnset();
extern int autoIntrpTest();
extern short autoLSDVget();
extern void autoShow();


#endif

#ifdef __cplusplus
}
#endif

#endif

