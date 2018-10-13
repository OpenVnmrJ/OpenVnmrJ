/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCadch
#define INCadch

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
		int adcState;		/* ADC_OVRFLOW */
		char *  adcBaseAddr;
		char	adcControl;
		int	vmeItrVector;
		int	vmeItrLevel;
		int	adcBrdVersion;
		SEM_ID  pSemAdcStateChg; /* Semiphore for state change of ADC */
		SEM_ID  pAdcMutex;       /* Mutex Semiphore for ADC Object */
} ADC_OBJ;

typedef ADC_OBJ *ADC_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern ADC_ID adcCreate(char* baseAddr,int vecotr,int level);
extern int adcGetState(ADC_ID pAdcId, int mode, int secounds);
extern int adcItrpEnable(ADC_ID pAdcId);
extern int adcItrpEnable(ADC_ID pAdcId);
extern int adcReset(ADC_ID pAdcId);
extern int adcShiftData(ADC_ID pAdcId, int shift);
extern int adcChanSelect(ADC_ID pAdcId, int select);
extern void adcShow(ADC_ID pAdcId,int level);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif
