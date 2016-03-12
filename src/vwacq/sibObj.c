/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *sccsID(){
    return "sibObj.c Copyright(c)1994-1996 Varian";
}
/* 
 */

#include <stdio.h>
#include <vxWorks.h>
#include <iv.h>
#include <intLib.h>
#include <semLib.h>
#include "errorcodes.h"
#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "sibObj.h"

#define SIB_SAFETY_BD_ID 061
#define SIB_ILI_BD_ID 052
#define SIB_ISI_BD_ID 043
#define SIB_MICRO_BD_ID 034	/* Behaves same as SIB_ISI_BD_ID */

extern STATUS_BLOCK currentStatBlock;

int sibDebug = 0;

static int sibId = 0;
static int sibId2 = 0;
static void isr(SIB_ID);

/* This table relates Vnmr error codes to Sib error bits */
static SibError sibTripErrors[] = {
    /* Non-channel specific */
    {SIB_ADDR_0, SIB_REG_CX, 1, 0, 2,0, SAFETYERROR+SIB_QUAD_REFL_PWR},
    {SIB_ADDR_0, SIB_REG_CX, 2, 0, 0,0, SAFETYERROR+SIB_OPR_PANIC},
    {SIB_ADDR_0, SIB_REG_CX, 4, 0, 64,0, SAFETYERROR+SIB_PS_FAIL},
    {SIB_ADDR_0, SIB_REG_CX, 8, 0, 0,0, SAFETYERROR+SIB_ATTN_READBACK},
    {SIB_ADDR_0, SIB_REG_CX, 16, 0, 0,0, SAFETYERROR+SIB_RESVD_A},
    {SIB_ADDR_0, SIB_REG_CX, 128, 0, 0,0, SAFETYERROR+SIB_RESVD_B},
    /* Observe channel trips */
    {SIB_ADDR_0, SIB_REG_AX, 1, 0, 1,0, SAFETYERROR+OBSERROR+SIB_PALI_MISSING},
    {SIB_ADDR_0, SIB_REG_AX, 2, 0, 32,0, SAFETYERROR+OBSERROR+SIB_REFL_PWR},
    {SIB_ADDR_0, SIB_REG_AX,4,0,8,0, SAFETYERROR+OBSERROR+SIB_AMP_GATE_DISCONN},
    {SIB_ADDR_0, SIB_REG_AX, 8, 0, 1,0, SAFETYERROR+OBSERROR+SIB_PALI_TRIP},
    {SIB_ADDR_0, SIB_REG_AX, 16, 0, 1,0, SAFETYERROR+OBSERROR+SIB_W_DOG},
    {SIB_ADDR_0, SIB_REG_AX, 32, 16, 1,0, SAFETYERROR+OBSERROR+SIB_PWR_SUPPLY},
    {SIB_ADDR_0, SIB_REG_AX, 64, 136, 1,0, SAFETYERROR+OBSERROR+SIB_REQ_ERROR},
    {SIB_ADDR_0, SIB_REG_AX,128, 40, 1,0, SAFETYERROR+OBSERROR+SIB_TUSUPI_TRIP},
    {SIB_ADDR_0, SIB_REG_BX, 1, 0, 0,0, SAFETYERROR+OBSERROR+SIB_RESVD_A},
    {SIB_ADDR_0, SIB_REG_BX, 2, 0, 8,0, SAFETYERROR+OBSERROR+SIB_RF_OVERDRIVE},
    {SIB_ADDR_0, SIB_REG_BX,4, 0, 8,0, SAFETYERROR+OBSERROR+SIB_RF_PULSE_WIDTH},
    {SIB_ADDR_0, SIB_REG_BX, 8, 0, 8,0, SAFETYERROR+OBSERROR+SIB_RF_DUTY_CYCLE},
    {SIB_ADDR_0, SIB_REG_BX, 16, 0, 8,0, SAFETYERROR+OBSERROR+SIB_RF_OVER_TEMP},
    {SIB_ADDR_0, SIB_REG_BX,32, 0, 8,0, SAFETYERROR+OBSERROR+SIB_RF_PWR_SUPPLY},
    {SIB_ADDR_0, SIB_REG_BX, 64, 0, 0,0, SAFETYERROR+OBSERROR+SIB_RESVD_B},
    {SIB_ADDR_0, SIB_REG_BX, 128, 0, 0,0, SAFETYERROR+OBSERROR+SIB_RESVD_C},
    /* Decouple channel trips */
    {SIB_ADDR_0, SIB_REG_AY, 1, 0, 16,0, SAFETYERROR+DECERROR+SIB_PALI_MISSING},
    {SIB_ADDR_0, SIB_REG_AY, 2, 0, 4,0, SAFETYERROR+DECERROR+SIB_REFL_PWR},
    {SIB_ADDR_0,SIB_REG_AY,4,0,128,0,SAFETYERROR+DECERROR+SIB_AMP_GATE_DISCONN},
    {SIB_ADDR_0, SIB_REG_AY, 8, 0, 16,0, SAFETYERROR+DECERROR+SIB_PALI_TRIP},
    {SIB_ADDR_0, SIB_REG_AY, 16, 0, 16,0, SAFETYERROR+DECERROR+SIB_W_DOG},
    {SIB_ADDR_0, SIB_REG_AY, 32, 16, 16,0, SAFETYERROR+DECERROR+SIB_PWR_SUPPLY},
    {SIB_ADDR_0, SIB_REG_AY, 64, 136, 16,0, SAFETYERROR+DECERROR+SIB_REQ_ERROR},
    {SIB_ADDR_0, SIB_REG_AY, 128,40,16,0, SAFETYERROR+DECERROR+SIB_TUSUPI_TRIP},
    {SIB_ADDR_0, SIB_REG_BY, 1, 0, 0,0, SAFETYERROR+DECERROR+SIB_RESVD_A},
    {SIB_ADDR_0, SIB_REG_BY,2, 0, 128,0, SAFETYERROR+DECERROR+SIB_RF_OVERDRIVE},
    {SIB_ADDR_0, SIB_REG_BY,4,0,128,0, SAFETYERROR+DECERROR+SIB_RF_PULSE_WIDTH},
    {SIB_ADDR_0, SIB_REG_BY, 8,0,128,0, SAFETYERROR+DECERROR+SIB_RF_DUTY_CYCLE},
    {SIB_ADDR_0, SIB_REG_BY, 16,0,128,0, SAFETYERROR+DECERROR+SIB_RF_OVER_TEMP},
    {SIB_ADDR_0, SIB_REG_BY,32,0,128,0, SAFETYERROR+DECERROR+SIB_RF_PWR_SUPPLY},
    {SIB_ADDR_0, SIB_REG_BY, 64, 0, 0,0, SAFETYERROR+DECERROR+SIB_RESVD_B},
    {SIB_ADDR_0, SIB_REG_BY, 128, 0, 0,0, SAFETYERROR+DECERROR+SIB_RESVD_C}
};

/* Error codes for ILI board */
static SibError iliTripErrors[] = {
    {SIB_ADDR_0, SIB_REG_AX, 1,  0,  1,0, ILIERROR+OBSERROR+ILI_10S_SAR},
    {SIB_ADDR_0, SIB_REG_AX, 2,  0,  1,0, ILIERROR+OBSERROR+ILI_5MIN_SAR},
    {SIB_ADDR_0, SIB_REG_AX, 4,  0,  1,0, ILIERROR+OBSERROR+ILI_PEAK_PWR},
    {SIB_ADDR_0, SIB_REG_AX, 8,  0,  8,0, ILIERROR+OBSERROR+ILI_RF_AMP_CABLE},
    {SIB_ADDR_0, SIB_REG_AX, 16, 0,  8,0, ILIERROR+OBSERROR+ILI_RF_REFL_PWR},
    {SIB_ADDR_0, SIB_REG_AX, 32, 0,  8,0, ILIERROR+OBSERROR+ILI_RF_DUTY_CYCLE},
    {SIB_ADDR_0, SIB_REG_AX, 64, 0,  8,0, ILIERROR+OBSERROR+ILI_RF_OVER_TEMP},
    {SIB_ADDR_0, SIB_REG_AX, 128,0,  8,0, ILIERROR+OBSERROR+ILI_RF_PULSE_WIDTH},

    {SIB_ADDR_0, SIB_REG_BX, 1,  0, 16,4, ILIERROR+DECERROR+ILI_10S_SAR},
    {SIB_ADDR_0, SIB_REG_BX, 2,  0, 16,4, ILIERROR+DECERROR+ILI_5MIN_SAR},
    {SIB_ADDR_0, SIB_REG_BX, 4,  0, 16,4, ILIERROR+DECERROR+ILI_PEAK_PWR},
    {SIB_ADDR_0, SIB_REG_BX, 8,  0,128,4, ILIERROR+DECERROR+ILI_RF_AMP_CABLE},
    {SIB_ADDR_0, SIB_REG_BX, 16, 0,128,4, ILIERROR+DECERROR+ILI_RF_REFL_PWR},
    {SIB_ADDR_0, SIB_REG_BX, 32, 0,128,4, ILIERROR+DECERROR+ILI_RF_DUTY_CYCLE},
    {SIB_ADDR_0, SIB_REG_BX, 64, 0,128,4, ILIERROR+DECERROR+ILI_RF_OVER_TEMP},
    {SIB_ADDR_0, SIB_REG_BX, 128,0,128,4, ILIERROR+DECERROR+ILI_RF_PULSE_WIDTH},

    {SIB_ADDR_0, SIB_REG_CX,   1, 0,  2,0, ILIERROR+ILI_QUAD_REFL_PWR},
    {SIB_ADDR_0, SIB_REG_CX,  16, 0, 17,0, ILIERROR+ILI_RFMON_MISSING},
    {SIB_ADDR_0, SIB_REG_CX,   2, 0,  0,0, ILIERROR+ILI_OPR_PANIC},
    {SIB_ADDR_0, SIB_REG_CX,  32, 0,  0,0, ILIERROR+ILI_OPR_CABLE},
    {SIB_ADDR_0, SIB_REG_CX,   4, 0, 64,0, ILIERROR+ILI_GRAD_TEMP},
    {SIB_ADDR_0, SIB_REG_CX,  64, 0, 64,0, ILIERROR+ILI_GRAD_WATER},
    {SIB_ADDR_0, SIB_REG_CX,   8, 0,  4,0, ILIERROR+ILI_HEAD_TEMP},
    {SIB_ADDR_0, SIB_REG_CX, 128, 0, 17,0, ILIERROR+ILI_ATTN_READBACK},

    {SIB_ADDR_0, SIB_REG_AY, 1,  0, 17,0, ILIERROR+ILI_RFMON_WDOG},
    {SIB_ADDR_0, SIB_REG_AY, 2,  0, 17,0, ILIERROR+ILI_RFMON_SELFTEST},
    {SIB_ADDR_0, SIB_REG_AY, 4,  0, 17,0, ILIERROR+ILI_RFMON_PS},
    {SIB_ADDR_0, SIB_REG_AY, 8,  0,  0,0, ILIERROR+ILI_SPARE_3},
    {SIB_ADDR_0, SIB_REG_AY, 16, 0,  0,0, ILIERROR+ILI_PS},
    {SIB_ADDR_0, SIB_REG_AY, 32, 0, 32,0, ILIERROR+ILI_SDAC_DUTY_CYCLE},
    {SIB_ADDR_0, SIB_REG_AY, 64, 0,  0,0, ILIERROR+ILI_SPARE_1},
    {SIB_ADDR_0, SIB_REG_AY, 128,0,  0,0, ILIERROR+ILI_SPARE_2},
};

/* Error codes for second ILI board */
static SibError ili2TripErrors[] = {
    {SIB_ADDR_2, SIB_REG_AX, 1,  0,  1,0, ILIERROR+CH3ERROR+ILI_10S_SAR},
    {SIB_ADDR_2, SIB_REG_AX, 2,  0,  1,0, ILIERROR+CH3ERROR+ILI_5MIN_SAR},
    {SIB_ADDR_2, SIB_REG_AX, 4,  0,  1,0, ILIERROR+CH3ERROR+ILI_PEAK_PWR},
    {SIB_ADDR_2, SIB_REG_AX, 8,  0,  8,0, ILIERROR+CH3ERROR+ILI_RF_AMP_CABLE},
    {SIB_ADDR_2, SIB_REG_AX, 16, 0,  8,0, ILIERROR+CH3ERROR+ILI_RF_REFL_PWR},
    {SIB_ADDR_2, SIB_REG_AX, 32, 0,  8,0, ILIERROR+CH3ERROR+ILI_RF_DUTY_CYCLE},
    {SIB_ADDR_2, SIB_REG_AX, 64, 0,  8,0, ILIERROR+CH3ERROR+ILI_RF_OVER_TEMP},
    {SIB_ADDR_2, SIB_REG_AX, 128,0,  8,0, ILIERROR+CH3ERROR+ILI_RF_PULSE_WIDTH},

    {SIB_ADDR_2, SIB_REG_BX, 1,  0, 16,4, ILIERROR+CH4ERROR+ILI_10S_SAR},
    {SIB_ADDR_2, SIB_REG_BX, 2,  0, 16,4, ILIERROR+CH4ERROR+ILI_5MIN_SAR},
    {SIB_ADDR_2, SIB_REG_BX, 4,  0, 16,4, ILIERROR+CH4ERROR+ILI_PEAK_PWR},
    {SIB_ADDR_2, SIB_REG_BX, 8,  0,128,4, ILIERROR+CH4ERROR+ILI_RF_AMP_CABLE},
    {SIB_ADDR_2, SIB_REG_BX, 16, 0,128,4, ILIERROR+CH4ERROR+ILI_RF_REFL_PWR},
    {SIB_ADDR_2, SIB_REG_BX, 32, 0,128,4, ILIERROR+CH4ERROR+ILI_RF_DUTY_CYCLE},
    {SIB_ADDR_2, SIB_REG_BX, 64, 0,128,4, ILIERROR+CH4ERROR+ILI_RF_OVER_TEMP},
    {SIB_ADDR_2, SIB_REG_BX, 128,0,128,4, ILIERROR+CH4ERROR+ILI_RF_PULSE_WIDTH},

    {SIB_ADDR_2, SIB_REG_CX,   1, 0,  2,0, ILIERROR+ILI2_SPARE_0},
    {SIB_ADDR_2, SIB_REG_CX,  16, 0, 17,0, ILIERROR+ILI2_SPARE_1},
    {SIB_ADDR_2, SIB_REG_CX,   2, 0,  0,0, ILIERROR+ILI2_SPARE_2},
    {SIB_ADDR_2, SIB_REG_CX,  32, 0,  0,0, ILIERROR+ILI2_SPARE_3},
    {SIB_ADDR_2, SIB_REG_CX,   4, 0, 64,0, ILIERROR+ILI2_SPARE_4},
    {SIB_ADDR_2, SIB_REG_CX,  64, 0, 64,0, ILIERROR+ILI2_SPARE_5},
    {SIB_ADDR_2, SIB_REG_CX,   8, 0,  4,0, ILIERROR+ILI2_SPARE_6},
    {SIB_ADDR_2, SIB_REG_CX, 128, 0, 17,0, ILIERROR+ILI2_SPARE_7},

    {SIB_ADDR_2, SIB_REG_AY, 1,  0, 17,0, ILIERROR+ILI2_SPARE_8},
    {SIB_ADDR_2, SIB_REG_AY, 2,  0, 17,0, ILIERROR+ILI2_SPARE_9},
    {SIB_ADDR_2, SIB_REG_AY, 4,  0, 17,0, ILIERROR+ILI2_SPARE_10},
    {SIB_ADDR_2, SIB_REG_AY, 8,  0,  0,0, ILIERROR+ILI2_SPARE_11},
    {SIB_ADDR_2, SIB_REG_AY, 16, 0,  0,0, ILIERROR+ILI2_SPARE_12},
    {SIB_ADDR_2, SIB_REG_AY, 32, 0, 32,0, ILIERROR+ILI2_SPARE_13},
    {SIB_ADDR_2, SIB_REG_AY, 64, 0,  0,0, ILIERROR+ILI2_SPARE_14},
    {SIB_ADDR_2, SIB_REG_AY, 128,0,  0,0, ILIERROR+ILI2_SPARE_15},
};

/* Error codes for ISI board */
static SibError isiTripErrors[] = {
    {SIB_ADDR_0, SIB_REG_AX, 64, 0, 0, 0, ISIERROR+ISI_XGRAD_FAULT},
    {SIB_ADDR_0, SIB_REG_BX, 64, 0, 0, 0, ISIERROR+ISI_YGRAD_FAULT},
    {SIB_ADDR_0, SIB_REG_CX,  8, 0, 0, 0, ISIERROR+ISI_ZGRAD_FAULT},
    {SIB_ADDR_0, SIB_REG_AX, 16, 0, 0, 0, ISIERROR+ISI_GRAD_AMP_ERR},
    {SIB_ADDR_0, SIB_REG_AX, 32, 0, 0, 0, ISIERROR+ISI_DUTY_CYCLE_ERR},

    {SIB_ADDR_0, SIB_REG_AY, 2,  0, 0, 0, ISIERROR+ISI_SPARE_2},
    {SIB_ADDR_0, SIB_REG_AY, 8,  0, 0, 0, ISIERROR+ISI_OVERTEMP},
    {SIB_ADDR_0, SIB_REG_AY, 16, 0, 0, 0, ISIERROR+ISI_NO_COOLANT},
    {SIB_ADDR_0, SIB_REG_AY, 32, 0, 0, 0, ISIERROR+ISI_SPARE_1},
    {SIB_ADDR_0, SIB_REG_AY, 64, 0, 0, 0, ISIERROR+ISI_SPARE_4},
    {SIB_ADDR_0, SIB_REG_AY, 128,0, 0, 0, ISIERROR+ISI_SPARE_3},

    {SIB_ADDR_0, SIB_REG_AY, 1,  0, 0, 0, ISIERROR+ISI_SYS_FAULT}
};

/* This table relates Vnmr error codes to Sib bypass bits */
static SibBypassWarning sibBypassWarnings[] = {
    {  1, SAFETYERROR+OBSERROR+SIB_PALI_BP},
    { 16, SAFETYERROR+DECERROR+SIB_PALI_BP},
    {  2, SAFETYERROR+SIB_QUAD_REFL_PWR_BP},
    { 32, SAFETYERROR+OBSERROR+SIB_REFL_PWR_BP},
    {  4, SAFETYERROR+DECERROR+SIB_REFL_PWR_BP},
    { 64, SAFETYERROR+SIB_PS_FAIL_BP},
    {  8, SAFETYERROR+OBSERROR+SIB_RF_AMP_BP},
    {128, SAFETYERROR+DECERROR+SIB_RF_AMP_BP}
};

/* This table relates Vnmr error codes to ILI bypass bits */
static SibBypassWarning iliBypassWarnings[] = {
    {  1, ILIERROR+OBSERROR+ILI_RFMON_BP},
    { 16, ILIERROR+DECERROR+ILI_RFMON_BP},
    {  2, ILIERROR+ILI_QUAD_BP},
    { 32, ILIERROR+ILI_SDAC_BP},
    {  4, ILIERROR+ILI_HEAD_GRAD_BP},
    { 64, ILIERROR+ILI_GRAD_BP},
    {  8, ILIERROR+OBSERROR+ILI_RF_AMP_BP},
    {128, ILIERROR+DECERROR+ILI_RF_AMP_BP}
};

/* This table relates Vnmr error codes to ILI #2 bypass bits */
static SibBypassWarning ili2BypassWarnings[] = {
    {  1, ILIERROR+CH3ERROR+ILI_RFMON_BP},
    { 16, ILIERROR+CH4ERROR+ILI_RFMON_BP},
    {  2, ILIERROR+ILI_SPARE_BP_1},
    { 32, ILIERROR+ILI_SPARE_BP_2},
    {  4, ILIERROR+ILI_SPARE_BP_3},
    { 64, ILIERROR+ILI_SPARE_BP_4},
    {  8, ILIERROR+CH3ERROR+ILI_RF_AMP_BP},
    {128, ILIERROR+CH4ERROR+ILI_RF_AMP_BP}
};

/* This table relates Vnmr error codes to ISI bypass bits */
/* NB: ISI has one bypass bit (in non-standard register) */
static SibBypassWarning isiBypassWarnings[] = {

    {8, ISIERROR+ISI_BYPASS}
};

void
sibReset(SIB_ID sib)
{
    if (sib->xpit){
	pitSubmodeSet(sib->xpit, 2);
	pitDirectionSet(sib->xpit, PIT_REG_A, 0);
	pitDirectionSet(sib->xpit, PIT_REG_B, 0);
	pitDirectionSet(sib->xpit, PIT_REG_C, 0);
    }
    if (sib->ypit){
	pitSubmodeSet(sib->ypit, 2);
	pitDirectionSet(sib->ypit, PIT_REG_A, 0);
	pitDirectionSet(sib->ypit, PIT_REG_B, 0);
	pitDirectionSet(sib->ypit, PIT_REG_C, 0x07);
	pitIntVectorSet(sib->ypit, sib->vmeIntVector);
	pitInterruptEnable(sib->ypit, PIT_INT_H4, sib->vmeIntLevel, 0);
    }
}

/*******************************************
 *
 * isr - SIB Interrupt Service Routine
 */
static void
isr(SIB_ID sib)
{
    /* Disable interrupt until next expt enables it */
    pitIntReset(sib->ypit, PIT_INT_H4);
    semGive(sib->pIsrSem);	/* Unblock sibTrip() routine */
}

SIB_ID
sibCreate(int vector,		/* Interrupt vector number */
	  int level)		/* Interrupt level */
{
    int ok;
    SIB_ID sib;
    IPAC *ipac;

    /* Create the IPAC object (the Industry Pack location) */
    ipac = ipacMv162Create(IPAC_SOCKET_A);
    if (!ipac){
        /* printf("sibCreate: could not create IPAC object\n"); */
	return NULL;
    }

    /* Create the object structure */
    sib = (SIB_ID) malloc(sizeof(SIB_OBJ));
    if ( sib == NULL){
        errLogSysRet(LOGIT, debugInfo,
		     "sibCreate: cannot malloc memory for SIB object");
	ipacDelete(ipac);
	return NULL;
    }

    /* Create the P/IT objects to use */
    sib->xpit = pitCreate(ipac, PIT_CHIP_X);
    sib->ypit = pitCreate(ipac, PIT_CHIP_Y);
    if (!sib->xpit || !sib->ypit){
        printf("sibCreate: could not create PI/T objects\n");
        sibDelete(sib);
        return NULL;
    }
    sib->vmeIntVector = vector;
    sib->vmeIntLevel = level;

    /* Create the SIB semaphores */
    sib->pIsrSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
    if (!sib->pIsrSem){
        errLogRet(LOGIT, debugInfo,
		  "sibCreate: could not create SIB semaphore");
        return NULL;
    }
    sib->pCommSem = semBCreate(SEM_Q_FIFO, SEM_FULL);
    if (!sib->pCommSem){
        errLogRet(LOGIT, debugInfo,
		  "sibCreate: could not create SIB Communication semaphore");
        return NULL;
    }

    /* Set the interrupt (H4 interrupt asserts vector bits 0-1) */
    intConnect(INUM_TO_IVEC( sib->vmeIntVector | 0x3),
	       isr,
	       (int)sib);

    sibReset(sib);

    /* Check for connection to the RF Monitor Board */
    if ((ok=sibCheckId(sib)) == FALSE){
	printf("*** sibCreate: Monitor Board ID incorrect\n");
	/* But still use it.  Cannot do a "go" until ID is read correctly */
    } else {
        printf("Industry Pack: IDs = 0%o, 0%o\n", sibId, sibId2);
    }

    return sib;
}

/*
 * sibDelete - Deletes an SIB object
 */
void
sibDelete(SIB_ID sib)
{
    sibReset(sib);
    if (sib->xpit){
	pitDelete(sib->xpit);
    }
    if (sib->ypit){
	pitIntReset(sib->ypit, PIT_INT_H4);
	pitDelete(sib->ypit);
    }
    free(sib);
}

unsigned char
sibRead(SIB_ID sib, SibReg reg, SibAddr addr)
{
    unsigned char rtn;
    PitReg pitReg;
    PIT *pit;

    semTake(sib->pCommSem, WAIT_FOREVER);
    pitWrite(sib->ypit, PIT_REG_C, addr); /* Set address first */
    /* NB: the bottom 3 bits of the register are the address.  Since
     * the rest of the bits are read only, we do not bother with a mask. */

    switch (reg){
      case SIB_REG_AX:
      case SIB_REG_AY:
	pitReg = PIT_REG_A;
	break;
      case SIB_REG_BX:
      case SIB_REG_BY:
	pitReg = PIT_REG_B;
	break;
      case SIB_REG_CX:
      case SIB_REG_CY:
	pitReg = PIT_REG_C;
	break;
      case SIB_REG_HY:
	pitReg = PIT_REG_H;
	break;
    }

    switch (reg){
      case SIB_REG_AX:
      case SIB_REG_BX:
      case SIB_REG_CX:
	pit = sib->xpit;
	break;
      case SIB_REG_AY:
      case SIB_REG_BY:
      case SIB_REG_CY:
      case SIB_REG_HY:
	pit = sib->ypit;
	break;
    }

    rtn = pitRead(pit, pitReg);
    if (reg == SIB_REG_CX){
	/* Kluge in the top 2 bits from the H register */
	unsigned char h04;
	h04 = pitRead(pit, PIT_REG_H);
	rtn = (rtn & 0x3f) | ((h04 & 0xc) << 4);
    }

    semGive(sib->pCommSem);
    return rtn;
}

int
sibCheckId(SIB_ID sib)
{
    int id2;
    /* NB: sibId is a file static int */
    /* Read two ID pieces from RF Monitor Board--each only 3 bits */
    sibId = (sibRead(sib, SIB_REG_HY, SIB_ADDR_1) & 0x7) << 3;
    sibId |= sibRead(sib, SIB_REG_HY, SIB_ADDR_0) & 0x7;
    if (sibId != SIB_SAFETY_BD_ID
	&& sibId != SIB_ILI_BD_ID
	&& sibId != SIB_ISI_BD_ID
	&& sibId != SIB_MICRO_BD_ID)
    {
	printf("Industry Pack: returns ID = 0%o\n", sibId);
	fprintf(stderr,"   RFM ID = 0%o\n", SIB_SAFETY_BD_ID);
	fprintf(stderr,"   ILI ID = 0%o\n", SIB_ILI_BD_ID);
	fprintf(stderr,"   ISI ID = 0%o\n", SIB_ISI_BD_ID);
	fprintf(stderr," MICRO ID = 0%o\n", SIB_MICRO_BD_ID);
	sibId = 0;
	return 0;
    }
    /* Now check for possible second ILI board */
    sibId2 = (sibRead(sib, SIB_REG_HY, SIB_ADDR_3) & 0x7) << 3;
    sibId2 |= sibRead(sib, SIB_REG_HY, SIB_ADDR_2) & 0x7;
    if (sibId2 != SIB_ILI_BD_ID) {
	/* No second board */
	sibId2 = 0;
    }
    return sibId != 0;
}

/*
 * sibGetTripErrors - 
 *
 *
 * RETURNS:
 *   Pointer to a zero terminated list of error codes.
 *   Returns a null pointer if could not get space for the list.
 *   Note that the caller is responsible for freeing the memory for the list.
 */
int *
sibGetTripErrors(SIB_ID sib)
{
    int *errlist;
    unsigned char bypass;
    unsigned char config;
    unsigned char err;
    int i;
    int ok;
    int nerrors;
    int ncodes;
    SibError *tripErrors;

    if ((ok=sibCheckId(sib)) == FALSE) {
	/* Bad communication with RF Monitor */
	errlist = (int *)malloc(2 * sizeof(int));
	if (errlist == NULL) {
	    return NULL;
	}
	errlist[0] = SAFETYERROR + SIB_COMM_FAIL;
	nerrors = 1;
    }else{
	if (sibId == SIB_SAFETY_BD_ID) {
	    tripErrors = sibTripErrors;
	    ncodes = sizeof(sibTripErrors) / sizeof(SibError);
	} else if (sibId == SIB_ILI_BD_ID) {
	    tripErrors = iliTripErrors;
	    ncodes = sizeof(iliTripErrors) / sizeof(SibError);
	} else if (sibId == SIB_ISI_BD_ID || sibId == SIB_MICRO_BD_ID) {
	    tripErrors = isiTripErrors;
	    ncodes = sizeof(isiTripErrors) / sizeof(SibError);
	}
	errlist = (int *)malloc((2*ncodes+1) * sizeof(int));
	if (errlist == NULL) {
	    return NULL;
	}
        /* Check for errors on first board */
	config = sibRead(sib, SIB_REG_BY, SIB_ADDR_0);
	bypass = sibRead(sib, SIB_REG_CX, SIB_ADDR_1);
	for (i=nerrors=0; i<ncodes; i++){
	    err = sibRead(sib, tripErrors[i].reg, tripErrors[i].addr);
	    if ((err & tripErrors[i].low_bits) == 0
		&& (err & tripErrors[i].hi_bits) == tripErrors[i].hi_bits
		&& (bypass & tripErrors[i].bypass_bits) == 0
		&& (config & tripErrors[i].config) == tripErrors[i].config)
	    {
		errlist[nerrors++] = tripErrors[i].errorcode;
	    }
	}

	if (sibId2 == SIB_ILI_BD_ID) {
	    /* Check for errors on second ILI board */
	    tripErrors = ili2TripErrors;
	    config = sibRead(sib, SIB_REG_BY, SIB_ADDR_2);
	    bypass = sibRead(sib, SIB_REG_CX, SIB_ADDR_3);
	    for (i=0; i<ncodes; i++){
		err = sibRead(sib, tripErrors[i].reg, tripErrors[i].addr);
		if ((err & tripErrors[i].low_bits) == 0
		    && (err & tripErrors[i].hi_bits) == tripErrors[i].hi_bits
		    && (bypass & tripErrors[i].bypass_bits) == 0
		    && (config & tripErrors[i].config) == tripErrors[i].config)
		{
		    errlist[nerrors++] = tripErrors[i].errorcode;
		}
	    }
	}
    }
    errlist[nerrors] = 0;

    return errlist;
}

/*
 * sibGetBypassWarnings - 
 *
 *
 * RETURNS:
 *   Pointer to a zero terminated list of error codes.
 *   Returns a null pointer if could not get space for the list.
 *   Note that the caller is responsible for freeing the memory for the list.
 */
int *
sibGetBypassWarnings(SIB_ID sib)
{
    int *errlist;
    unsigned char bypass;
    unsigned char mask;
    int i;
    int ok;
    int nmsgs;
    int ncodes;
    SibBypassWarning *warnings;

    if ((ok=sibCheckId(sib)) == FALSE) {
	/* Bad communication with RF Monitor */
	errlist = (int *)malloc(2 * sizeof(int));
	if (errlist == NULL) {
	    return NULL;
	}
	errlist[0] = SAFETYERROR + SIB_COMM_FAIL;
	nmsgs = 1;
    } else {
	bypass = sibRead(sib, SIB_REG_CX, SIB_ADDR_1);
	if (sibId == SIB_SAFETY_BD_ID) {
	    warnings = sibBypassWarnings;
	    ncodes = sizeof(sibBypassWarnings) / sizeof(SibBypassWarning);
	} else if (sibId == SIB_ILI_BD_ID) {
	    warnings = iliBypassWarnings;
	    ncodes = sizeof(iliBypassWarnings) / sizeof(SibBypassWarning);
	} else if (sibId == SIB_ISI_BD_ID || sibId == SIB_MICRO_BD_ID) {
	    warnings = isiBypassWarnings;
	    ncodes = sizeof(isiBypassWarnings) / sizeof(SibBypassWarning);
            /*
             * NB: ISI has one bypass bit (in non-standard register)
             * so we need to re-read the bypass byte.
             */
            bypass = sibRead(sib, SIB_REG_BY, SIB_ADDR_0);
	}
	errlist = (int *)malloc((2*ncodes+1) * sizeof(int));
	if (errlist == NULL) {
	    return NULL;
	}
	for (i=nmsgs=0; i<ncodes; i++){
	    mask = warnings[i].bit_mask;
	    if ((bypass & mask) == mask){
		errlist[nmsgs++] = warnings[i].errorcode;
	    }
	}

	if (sibId2 == SIB_ILI_BD_ID) {
	    /* Check bypasses on second ILI */
	    warnings = ili2BypassWarnings;
	    bypass = sibRead(sib, SIB_REG_CX, SIB_ADDR_3);
	    for (i=0; i<ncodes; i++){
		mask = warnings[i].bit_mask;
		if ((bypass & mask) == mask){
		    errlist[nmsgs++] = warnings[i].errorcode;
		}
	    }
	}
    }
    errlist[nmsgs] = 0;

    return errlist;
}

int
sibPowerMon(SIB_OBJ *sib)
{
    int pwr;
    int knob;

    if (!sib ||
	(sibId != SIB_SAFETY_BD_ID &&
	 sibId != SIB_ILI_BD_ID))
    {
	return FALSE;
    }
    /* Read channel 1 power */
    pwr = (~sibRead(sib, SIB_REG_BX, (SibAddr)1) >> 2) & 0x3f;
    knob = sibRead(sib, SIB_REG_AX, (SibAddr)1) & 0x1f;
    currentStatBlock.stb.rfMonitor[0] = pwr | (knob << 6);

    /* Read channel 2 power */
    pwr = (~sibRead(sib, SIB_REG_BY, (SibAddr)1) >> 2) & 0x3f;
    knob = sibRead(sib, SIB_REG_AY, (SibAddr)1) & 0x1f;
    currentStatBlock.stb.rfMonitor[1] = pwr | (knob << 6);

    if (sibDebug > 1) {
	DPRINT4(-1,"Ch1: knob=%2d, pwr=%2d;  Ch2: knob=%2d, pwr=%2d\n",
                (currentStatBlock.stb.rfMonitor[0] >> 6) & 0x1f,
                (currentStatBlock.stb.rfMonitor[0]) & 0x3f,
                (currentStatBlock.stb.rfMonitor[1] >> 6) & 0x1f,
                (currentStatBlock.stb.rfMonitor[1]) & 0x3f);
    }
    if (sibId2 == SIB_ILI_BD_ID) {
	/* Read channel 3 power */
	pwr = (~sibRead(sib, SIB_REG_BX, (SibAddr)3) >> 2) & 0x3f;
	knob = sibRead(sib, SIB_REG_AX, (SibAddr)3) & 0x1f;
	currentStatBlock.stb.rfMonitor[2] = pwr | (knob << 6);

	/* Read channel 4 power */
	pwr = (~sibRead(sib, SIB_REG_BY, (SibAddr)3) >> 2) & 0x3f;
	knob = sibRead(sib, SIB_REG_AY, (SibAddr)3) & 0x1f;
	currentStatBlock.stb.rfMonitor[3] = pwr | (knob << 6 );
        if (sibDebug > 1) {
            DPRINT4(-1,"Ch3: knob=%2d, pwr=%2d;  Ch4: knob=%2d, pwr=%2d\n",
                    (currentStatBlock.stb.rfMonitor[2] >> 6) & 0x1f,
                    (currentStatBlock.stb.rfMonitor[2]) & 0x3f,
                    (currentStatBlock.stb.rfMonitor[3] >> 6) & 0x1f,
                    (currentStatBlock.stb.rfMonitor[3]) & 0x3f);
        }
    }
    return TRUE;
}
