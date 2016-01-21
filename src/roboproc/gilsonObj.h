/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCgilsonh
#define INCgilsonh

#define NV_ZSENSE   3    /* on 215 SW liquid handler */
#define NV_PUMP     3    /* on 215 w/integrated pump */
#define NV_SYRINGE  4    /* syringe size ul (not used on 215 SW) */
#define NV_ZTRAVEL  5
#define NV_RINSEX  11
#define NV_RINSEY  12
#define NV_RINSEZ  13
#define NV_ZTOPCLAMP  16

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* ------------------- GILSON 215 Object Structure ------------------- */

typedef struct {
        char*   pGilsonIdStr;   /* user identifier string */
        char*   pSID;           /* SCCS ID string */
        short   gilsonID;       /* gilson id number (22 + 128) */
        short   pumpID;         /* gilson id number (23 + 128) */
        short   injectID;       /* gilson id number (29 + 128) */
        struct _ioDev *liqDevEntry; /* ptr to device entry in devTable */
        int     liqDev;         /* i/o Port File Descriptor */
        int     pumpType;       /* 0-none,
                                 * 1-Cavro,
                                 * 2-gilson integrated 402,
                                 * 3-gilson Peristaltic
                                 * 4-gilson standalone 402
                                 */
        int     pumpVolume;
        int     Ztravel;        /* 125 or 150 mm */
        int     ZTopClamp;      /* Top of needle position, base plate location */
        int     RinseStation[3];/* X/Y/Z coordinates in mm of Rinse Station */
        int     InjectStation[3]; /* X/Y/Z coordinates in mm of Rinse Station */
        int     presentXYloc[2];
        int     presentZloc;
        int     X_MinMax[2];
        int     Y_MinMax[2];
        int     Z_MinMax[2];
        int     presentPumploc;
        int     systemState;
} GILSON_OBJ;

typedef GILSON_OBJ *GILSONOBJ_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern GILSONOBJ_ID gilsonCreate(char *portdev, int gilunit,
                                 int injunit, int pumpunit);
extern int gilInitInjLoc(GILSONOBJ_ID pGilId,int X, int Y, int Z);
extern int gilsonHome(GILSONOBJ_ID pGilId);
extern int gilMoveZ(GILSONOBJ_ID pGilId,int Zmm);
extern int gilMoveZLQ(GILSONOBJ_ID pGilId,int Zmm);
extern int gilXYZMinMax(GILSONOBJ_ID pGilId,int *AxisMinMax);
extern int gilsonSetZTop(GILSONOBJ_ID pGilId,int ZTop);
extern int gilsonSetPump(GILSONOBJ_ID pGilId,int PumpType, int PumpVolume);
extern int gilMoveZ2Top(GILSONOBJ_ID pGilId);
extern int gilMoveXY(GILSONOBJ_ID pGilId,int Xmm, int Ymm);
extern int gilMoveXYAsync(GILSONOBJ_ID pGilId,int Xmm, int Ymm);
extern int gilMove2Rinse(GILSONOBJ_ID pGilId);
extern int gilMove2Inj(GILSONOBJ_ID pGilId);
extern int gilWriteDisplay(GILSONOBJ_ID pGilId,char *msge);
extern int gilRelaxZ(GILSONOBJ_ID pGilId);
extern int gilRelaxXY(GILSONOBJ_ID pGilId);
extern int gilsonAspirate(GILSONOBJ_ID pGilId, double volume /*ul */,
                          double Speed, int Zspeed, int Zlimit);
extern int gilsonAspirateAsync(GILSONOBJ_ID pGilId, double volume /*ul */,
                               double Speed, int Zspeed, int Zlimit);
extern int gilsonDispense(GILSONOBJ_ID pGilId, double volume /*ul */,
                          double Speed, int Zspeed, int Zlimit);
extern int gilsonDispenseAsync(GILSONOBJ_ID pGilId, double volume /*ul */,
                               double Speed, int Zspeed, int Zlimit);
extern int gilsonFlush(GILSONOBJ_ID pGilId, double volume,
                       double inSpeed, double outSpeed);
extern int gilEmptyDiluter(GILSONOBJ_ID pGilId);
extern double gilsonMaxFlowRate(GILSONOBJ_ID pGilId);
extern double gilsonMinFlowRate(GILSONOBJ_ID pGilId);
extern int gilSetContacts(GILSONOBJ_ID pGilId, int bits, int on_off);
extern int gilGetContacts(GILSONOBJ_ID pGilId, int bits, int *on_off);
extern int gilInject(GILSONOBJ_ID pGilId);
extern int gilLoad(GILSONOBJ_ID pGilId);
extern int gilGetInjectValveLoc(GILSONOBJ_ID pGilId,char *loc);
extern int gilsonPowered(GILSONOBJ_ID pGilId);
extern int gilsonCurrentVolume(GILSONOBJ_ID pGilId);
extern int gilsonStoppedAll(GILSONOBJ_ID pGilId);
extern char *gilErrorMsge(GILSONOBJ_ID pGilId);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif

