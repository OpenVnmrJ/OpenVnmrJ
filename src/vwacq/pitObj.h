/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef PITOBJ_H
#define PITOBJ_H

typedef volatile unsigned char Reg8;

typedef enum{
    IPAC_SOCKET_A = 0,
    IPAC_SOCKET_B = 1,
    IPAC_SOCKET_C = 2,
    IPAC_SOCKET_D = 3
} IpacSocket;

typedef enum{
    IPAC_HOST_MV162,
    IPAC_HOST_VIPC610
} IpacHost;

typedef enum{
    IPAC_INT_0 = 0,
    IPAC_INT_1 = 1
} IpacInt;

typedef struct{
    IpacHost host;
    IpacSocket socket;
    Reg8 *ioaddr;
    Reg8 *idaddr;
} IPAC;

#ifdef __cplusplus
extern "C" {
#endif
    
#if defined(__STDC__) || defined(__cplusplus)
    /* --------- ANSI/C++ function prototypes --------------- */
    IMPORT IPAC *ipacMv162Create(IpacSocket socket);
    IMPORT VOID ipacMv162IntEnable(IPAC *, IpacInt, int level, int polarity);
    IMPORT VOID ipacMv162IntDisable(IPAC *, IpacInt);
    IMPORT VOID ipacDelete(IPAC *);
    IMPORT VOID ipacIntEnable(IPAC *, IpacInt, int level, int polarity);
    IMPORT VOID ipacIntDisable(IPAC *, IpacInt);
    IMPORT char *ipacId(IPAC *);
    IMPORT unsigned long ipacModel(IPAC *);
#else
    /* --------- K&R prototypes ------------  */
    IMPORT IPAC *ipacMv162Create();
    IMPORT VOID ipacMv162IntEnable();
    IMPORT VOID ipacMv162IntDisable();
    IMPORT VOID ipacDelete();
    IMPORT VOID ipacIntEnable();
    IMPORT VOID ipacIntDisable();
    IMPORT char *ipacId();
    IMPORT unsigned long ipacModel();
#endif /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

typedef enum{
     PIT_CHIP_X = 0,
     PIT_CHIP_Y = 1
} PitChip;

typedef enum{
    PIT_REG_A,
    PIT_REG_B,
    PIT_REG_C,
    PIT_REG_H
} PitReg;

typedef enum{
    PIT_INT_H1,
    PIT_INT_H2,
    PIT_INT_H3,
    PIT_INT_H4
} PitInterrupt;

typedef struct{
     IPAC *ipac;
     PitChip chip;
     Reg8 *ioaddr;
} PIT;

#ifdef __cplusplus
extern "C" {
#endif
    
#if defined(__STDC__) || defined(__cplusplus)
    /* --------- ANSI/C++ function prototypes --------------- */
    IMPORT PIT *pitCreate(IPAC *, PitChip);
    IMPORT VOID pitDelete(PIT *);
    IMPORT VOID pitDirectionSet(PIT *, PitReg, int bitmask);
    IMPORT unsigned char pitRead(PIT *, PitReg);
    IMPORT VOID pitWrite(PIT *, PitReg, int value);
    IMPORT VOID pitInterruptEnable(PIT *, PitInterrupt whichInt,
				   int level, int polarity);
    IMPORT VOID pitIntReset(PIT *, PitInterrupt);
    IMPORT VOID pitIntVectorSet(PIT *, int intVector);
    IMPORT VOID pitSubmodeSet(PIT *, int submode);
#else
    /* --------- K&R prototypes ------------  */
    IMPORT PIT *pitCreate();
    IMPORT VOID pitDelete();
    IMPORT VOID pitDirectionSet();
    IMPORT unsigned char pitRead();
    IMPORT VOID pitWrite();
    IMPORT VOID pitInterruptEnable();
    IMPORT VOID pitIntReset();
    IMPORT VOID pitIntVectorSet();
    IMPORT VOID pitSubmodeSet();
#endif /* __STDC__ */
 
#ifdef __cplusplus
}
#endif
#endif /* PITOBJ_H */
