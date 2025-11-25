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
#ifndef INCflowCntrlObh
#define INCflowCntrlObh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#include "NDDS_Obj.h"

/* HIDDEN */

/* typedefs */

#define FALSE 0
#define TRUE 1

/*************************************************************/

#define MAX_SUBSCRIPTIONS 64

typedef struct _flow_ {
        char    *cntlrId[MAX_SUBSCRIPTIONS];
        int     maxXferLimit; /* maximum transfer limit */
        int     HighH2OMark; /* at this count send msg to publisher to continue */
        unsigned int AtIncreNum;
        int     numPubs;
        int     matchNum;
        NDDS_ID  pubs[MAX_SUBSCRIPTIONS];
        int     idIndex[MAX_SUBSCRIPTIONS];  /*  order of who was 1st */
        int     incrementVals[MAX_SUBSCRIPTIONS];
        int     numPubsAtHiH2oMark;
        int     timesReplySent;
        RTINtpTime _timeStarted;
        RTINtpTime _timeDuration;
        pthread_mutex_t     mutex;          /* Mutex for protection of variable data */
} FlowContrlObj;
 

/* --------- ANSI/C++ compliant function prototypes --------------- */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

 
extern FlowContrlObj *flowCntrlCreate(void);
extern void resetFlowCntrl(FlowContrlObj *pFlowCntrl);
extern void initFlowCntrl(FlowContrlObj *pFlowCntrl, int Id, char *cntlrId, NDDS_ID PubId, int maxLimit, int xferHiH2OLimit);
extern void IncrementTransferLoc(FlowContrlObj *pFlow, int Id);
extern int  AllAtIncrementMark(FlowContrlObj *pFlow, int Id);
extern void MarkFlowCntrPubDone(FlowContrlObj *pFlow, int Id);
extern void ReportFlow(FlowContrlObj *pFlow);
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
