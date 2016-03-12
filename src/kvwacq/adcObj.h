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
/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCadch
#define INCadch


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* ----------- FIFO Error Codes, start at 20 so that it can be append to those
	 codes in errorcodes.h in vnmr
*/
#define ADC_OVERFLOW 		30
#define ADC_RCVR_OVERFLOW 	31

/* see vmeIntrp.h for interrupt vector Numbers */

/* ADC Status Modes */


typedef struct {
      unsigned long 	adcBaseAddr;
		int	vmeItrVector;
		int	vmeItrLevel;
		short	ApBusAddr;
		long   CntrlMask;	/* control register setting, can't read it */
                short   optionsPresent; /* DSP daughter board is present */
		short	adcBrdVersion;
		int     adcState;	/* ADC_ACTIVE, ADC_STOPPED, ADC_ERROR */
		int     adcOvldFlag;
		int	adcCntrlReg;	/* is O/W for Mercury */
    		char*	pIdStr;	  	  /* Identifier String */
    		char*	pSID;	  	  /* Identifier String */
		void*   dspDownLoadAddr;
		int     dspPromType;
} ADC_OBJ;

typedef ADC_OBJ *ADC_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern ADC_ID  adcCreate(unsigned long baseAddr, int apBusAddr, int vector, int level, char* idstr);
extern void adcReset(ADC_ID pAdcId);
extern void adcOvldClear(ADC_ID pAdcId);
extern void adcItrpEnable(ADC_ID pAdcId, int mask);
extern void adcItrpDisable(ADC_ID pAdcId, int mask);
extern short adcStatReg(ADC_ID pAdcId);
extern int adcDSPpresent(ADC_ID pAdcId);
extern ushort_t adcItrpStatReg(ADC_ID pAdcId);
extern short adcCntrlReg(ADC_ID pAdcId);
extern short adcIntrpMask(ADC_ID pAdcId);
extern int adcGenDspCodes(ADC_ID pAdcId, int ovrsamp, unsigned long *pCodes);
extern int adcGenCntrlCodes(ADC_ID pAdcId, unsigned long cntrl, unsigned long *pCodes);
extern int adcGenCntrl2Codes(ADC_ID pAdcId, unsigned long cntrl, unsigned long *pCodes);
extern void adcReadData(ADC_ID pAdcId,short *real, short *imag);
extern void adcShow(ADC_ID pAdcId,int level);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern ADC_ID adcCreate();
extern void adcReset();
extern void adcOvldClear();
extern void adcItrpEnable();
extern void adcItrpDisable();
extern short adcStatReg();
extern int adcDSPpresent();
extern ushort_t adcItrpStatReg();
extern short adcCntrlReg();
extern int adcGenDspCodes();
extern int adcGenCntrlCodes();
extern void adcReadData();
extern short adcIntrpMask();
extern void adcShow();

#endif

#ifdef __cplusplus
}
#endif

#endif
