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
#ifndef MASSPEEDWRAP_H
#define MASSPEEDWRAP_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "acqcmds.h"

typedef struct Console_Stat
{
    int  dataTypeID;
    int  AcqCtCnt;
    int  AcqFidCnt;
    int  AcqSample;
    int  AcqLockFreqAP;
    int  AcqNpErr;
    int  AcqSpinSet;
    int  AcqSpinAct;
    int  AcqSpinSpeedLimit;
    int  AcqRcvrNpErr;
    int  Acqstate;
    int  AcqOpsComplCnt;
    int  AcqLSDVbits;
    int  AcqLockLevel;
    int  AcqRecvGain;
    int  AcqSpinSpan;
    int  AcqSpinAdj;
    int  AcqSpinMax;
    int  AcqSpinActSp;
    int  AcqSpinProfile;
    int  AcqOpsComplFlags;
    int  statblockRate;
    char  probeId1[20];
} Console_Stat;

Console_Stat StatBlock;

typedef int MSG_Q_ID;

extern int sysClkRateGet();
extern int calcSysClkTicks(int msec);
extern void taskDelay(int count);
extern void hsspi(int dum, int dum2);
extern int taskSpawn(char *name, int p, int t, int s, void *(), int a1,
             int a2, int a3, int a4, int a5, int a6, int a7, int a8,
             int a9, int a10);
extern int msgQMasCreate(int a, int b, int c);
extern int msgQMasReceive(int a, char *buf, int len, int delay);
extern void sendConsoleStatus();
extern int clearport(int port);
extern int pifchr(int port);
extern void msgQsend(int id, char *msg, int len, int a1, int a2);
extern void selectSpinner(int val);

extern char *MASPORTNAME;

#define MAS_TASK_PRIORITY 0
#define STD_TASKOPTIONS 0
#define STD_STACKSIZE 0
#define MSG_Q_FIFO        0


#define msgQReceive(a, buf, len, delay) msgQMasReceive(a, buf, len, delay)
#define msgQCreate(a, b, c) msgQMasCreate(a, b, c)
#define msgQSend(a, b, c, d, e) msgQMasSend(a, b, c, d, e)

#endif
