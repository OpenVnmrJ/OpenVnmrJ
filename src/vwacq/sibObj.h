/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SIBOBJ_H
#define SIBOBJ_H

#include "pitObj.h"		/* SIB Object made from two Pit Objects */

typedef enum{
    SIB_REG_AX,
    SIB_REG_BX,
    SIB_REG_CX,
    SIB_REG_AY,
    SIB_REG_BY,
    SIB_REG_CY,
    SIB_REG_HY
} SibReg;

typedef enum{
    SIB_ADDR_0 = 0,
    SIB_ADDR_1,
    SIB_ADDR_2,
    SIB_ADDR_3,
    SIB_ADDR_4,
    SIB_ADDR_5,
    SIB_ADDR_6,
    SIB_ADDR_7,
} SibAddr;

typedef struct{			/* SIB Object structure */
    PIT *xpit;
    PIT *ypit;
    int vmeIntVector;
    int vmeIntLevel;
    SEM_ID pIsrSem;
    SEM_ID pCommSem;
} SIB_OBJ;

typedef SIB_OBJ *SIB_ID;

typedef struct{			/* Error code structure */
    SibAddr addr;		/* Sib address */
    SibReg reg;			/* Sib register */
    unsigned char low_bits;	/* If all bits in this mask are low ... */
    unsigned char hi_bits;	/*  and all these are high ... */
    unsigned char bypass_bits;	/*  and these are low in bypass word ... */
    unsigned char config;	/*  and these are hi in config word ... */
    int errorcode;		/*  then this error code applies. */
} SibError;

typedef struct{
    unsigned char bit_mask;
    int errorcode;
} SibBypassWarning;

#ifdef __cplusplus
extern "C" {
#endif
    
#if defined(__STDC__) || defined(__cplusplus)
    /* --------- ANSI/C++ function prototypes --------------- */
    IMPORT SIB_ID sibCreate(int intVector, int intLevel);
    IMPORT VOID sibDelete (SIB_ID);
    IMPORT VOID sibReset(SIB_ID);
    IMPORT int sibCheckId(SIB_ID);
    IMPORT unsigned char sibRead(SIB_ID, SibReg, SibAddr);
    IMPORT int *sibGetTripErrors(SIB_ID);
    IMPORT int *sibGetBypassWarnings(SIB_ID);
#else
    /* --------- K&R prototypes ------------  */
    IMPORT SIB_ID sibCreate();
    IMPORT VOID sibDelete ();
    IMPORT VOID sibReset();
    IMPORT int sibCheckId();
    IMPORT unsigned char sibRead();
    IMPORT int *sibGetTripErrors();
    IMPORT int *sibGetBypassWarnings();
#endif /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* SIBOBJ_H */
