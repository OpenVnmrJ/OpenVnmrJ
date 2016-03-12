/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *SCCSid(){
    return "sibTask.c Copyright (c) 1994-1996 Varian";
}

/* 
 */

#include <stdio.h>
#include <string.h>
#include <vxWorks.h>
#include <semLib.h> 
#include <ioLib.h>
#include <msgQLib.h>

#include "logMsgLib.h"
#include "taskPrior.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "hostAcqStructs.h"
#include "sibTask.h"

extern STATUS_BLOCK currentStatBlock;
extern SIB_ID pTheSibObject;
extern MSG_Q_ID pMsgesToPHandlr;

extern int sibDebug;

void sibTrip(SIB_OBJ *);

int
startSibTask(int taskPriority, int taskOptions, int stackSize)
{
    if (pTheSibObject){
        diagPrint(NULL," - RF Monitor is present\n");
        currentStatBlock.stb.rfMonitor[0] = 0;
    }else{
        diagPrint(NULL," - RF Monitor NOT found\n");
        currentStatBlock.stb.rfMonitor[0] = -1; /* Mark not present */
	return ERROR;
    }

    /* Start the SIB trip interrupt task */
    taskSpawn("tSibTrip", taskPriority, taskOptions, stackSize, sibTrip,
	      pTheSibObject, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    return OK;
}

void
sibTrip(SIB_OBJ *sib)
{
    int interrupted;
    int suflag;
    while (1){
	semTake(sib->pIsrSem, WAIT_FOREVER);
	if (sibDebug > 1){
	    printf("sibTrip()\n");
	}
	sibPostErrors(sib, interrupted=TRUE, suflag=0);
    }
}

int
sibStatusCheck(SIB_OBJ *sib, int suflag)
{
    int interrupted;
    if (sib){
	sibPostBypassWarnings(sib);
	if (sibPostErrors(sib, interrupted=FALSE, suflag) == 0) {
            // No errors; enable interrupt for in-run checks
            pitInterruptEnable(sib->ypit, PIT_INT_H4, sib->vmeIntLevel, 0);
        }
        return 0;
    } else {
        return 1;
    }
}

int
sibPostErrors(SIB_OBJ *sib, int interrupted, int suflag)
{
    static EXCEPTION_MSGE msg;
    int *errlist;
    int i;
    int nerrs = 0;
    int sendFlag;

    if (sibDebug > 1) {
	printf("SIB REGISTERS:\n");
	printf("   PAX[0] = 0x%02x", sibRead(sib, SIB_REG_AX, (SibAddr)0));
	printf("   PBX[0] = 0x%02x", sibRead(sib, SIB_REG_BX, (SibAddr)0));
	printf("   PCX[0] = 0x%02x\n", sibRead(sib, SIB_REG_CX, (SibAddr)0));
	printf("   PAY[0] = 0x%02x", sibRead(sib, SIB_REG_AY, (SibAddr)0));
	printf("   PBY[0] = 0x%02x", sibRead(sib, SIB_REG_BY, (SibAddr)0));
	printf("   PHY[0] = 0x%02x\n", sibRead(sib, SIB_REG_HY, (SibAddr)0));
	printf("   PAX[1] = 0x%02x", sibRead(sib, SIB_REG_AX, (SibAddr)1));
	printf("   PBX[1] = 0x%02x", sibRead(sib, SIB_REG_BX, (SibAddr)1));
	printf("   PCX[1] = 0x%02x\n", sibRead(sib, SIB_REG_CX, (SibAddr)1));
	printf("   PAY[1] = 0x%02x", sibRead(sib, SIB_REG_AY, (SibAddr)1));
	printf("   PBY[1] = 0x%02x", sibRead(sib, SIB_REG_BY, (SibAddr)1));
	printf("   PHY[1] = 0x%02x\n", sibRead(sib, SIB_REG_HY, (SibAddr)1));
	printf("   PAX[2] = 0x%02x", sibRead(sib, SIB_REG_AX, (SibAddr)2));
	printf("   PBX[2] = 0x%02x", sibRead(sib, SIB_REG_BX, (SibAddr)2));
	printf("   PCX[2] = 0x%02x\n", sibRead(sib, SIB_REG_CX, (SibAddr)2));
	printf("   PAY[2] = 0x%02x", sibRead(sib, SIB_REG_AY, (SibAddr)2));
	printf("   PBY[2] = 0x%02x", sibRead(sib, SIB_REG_BY, (SibAddr)2));
	printf("   PHY[2] = 0x%02x\n", sibRead(sib, SIB_REG_HY, (SibAddr)2));
	printf("   PAX[3] = 0x%02x", sibRead(sib, SIB_REG_AX, (SibAddr)3));
	printf("   PBX[3] = 0x%02x", sibRead(sib, SIB_REG_BX, (SibAddr)3));
	printf("   PCX[3] = 0x%02x\n", sibRead(sib, SIB_REG_CX, (SibAddr)3));
	printf("   PAY[3] = 0x%02x", sibRead(sib, SIB_REG_AY, (SibAddr)3));
	printf("   PBY[3] = 0x%02x", sibRead(sib, SIB_REG_BY, (SibAddr)3));
	printf("   PHY[3] = 0x%02x\n", sibRead(sib, SIB_REG_HY, (SibAddr)3));
    }

    i = get_acqstate();
    sendFlag = (i != ACQ_INACTIVE) && (i != ACQ_IDLE);
    /* Compile error messages */
    errlist = sibGetTripErrors(sib);
    if (!errlist){
        /* Error in error reporter! */
        if (sendFlag) {
	    msg.exceptionType = HARD_ERROR;
	    msg.reportEvent = SAFETYERROR+SIB_MALLOC_FAIL;
	    msgQSend(pMsgesToPHandlr, (char*)&msg, sizeof(msg),
		     NO_WAIT, MSG_PRI_URGENT);
        }
        return 1;
    }else if (interrupted && !errlist[0]){
        /* No errors in errlist--report as error! */
        nerrs = 1;
        if (sendFlag) {
	    msg.exceptionType = HARD_ERROR;
	    msg.reportEvent = SAFETYERROR+SIB_NO_ERROR;
	    msgQSend(pMsgesToPHandlr, (char*)&msg, sizeof(msg),
		     NO_WAIT, MSG_PRI_URGENT);
        }
    }
    /* Report all errors in errlist */
    for (i=0; errlist && errlist[i]; i++){
        /* Send HARD_ERROR (and cause abort) only with last message */
        nerrs = i + 1;
        if (sendFlag) {
            if (suflag == 1 || errlist[i+1] != 0) {
                /* Only last message may be a hard error */
                msg.exceptionType = WARNING_MSG;
            } else {
                msg.exceptionType = HARD_ERROR;
            }
            if ( (!interrupted || i > 0) && errlist[i+1] == 0) {
                /* Give time to handle the warnings, else disaster */
                taskDelay(sysClkRateGet() / 2);
            }
            /*printf("SIB err %d, type=%d\n", errlist[i], msg.exceptionType);*/
            msg.reportEvent = errlist[i];
            msgQSend(pMsgesToPHandlr, (char*)&msg, sizeof(msg),
                     NO_WAIT, MSG_PRI_NORMAL);
        }
    }
    if (errlist) free(errlist);
    if (sibDebug > 0) {
        DPRINT1(-1,"%d errors found\n", nerrs);
    }
    return nerrs;
}

int
sibPostBypassWarnings(SIB_OBJ *sib)
{
    static EXCEPTION_MSGE msg;
    int *errlist;
    int i;
    int nmsgs = 0;

    if ((i=get_acqstate()) != ACQ_INACTIVE && i != ACQ_IDLE){
	/* Send error messages to host */
	errlist = sibGetBypassWarnings(sib);
	if (!errlist){
	    msg.exceptionType = HARD_ERROR;
	    msg.reportEvent = SAFETYERROR+SIB_MALLOC_FAIL;
	    msgQSend(pMsgesToPHandlr, (char*)&msg, sizeof(msg),
		     NO_WAIT, MSG_PRI_URGENT);
	    return 1;
	}
	for (i=0; errlist && errlist[i]; i++){
	    msg.exceptionType = WARNING_MSG;
	    msg.reportEvent = errlist[i];
	    msgQSend(pMsgesToPHandlr, (char*)&msg, sizeof(msg),
		     NO_WAIT, MSG_PRI_URGENT);
	    taskDelay(0);	/* Handle the message */
	    nmsgs = i;
	}
	if (errlist) free(errlist);
    }
    return nmsgs;
}
