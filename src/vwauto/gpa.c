/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */

#include <stdio.h>
#include <string.h>
#include <vxWorks.h>
#include <semLib.h>
#include <ioLib.h>
#include <ctype.h>

#include "logMsgLib.h"
#include "mboxcmds.h"
#include "hardware.h"
#include "gpa.h"
#include "errorcodes.h"

#if ( CPU!=CPU32 )
#include "autoObj.h"
extern AUTO_ID pTheAutoObject;
#else
#include "globals.h"
#endif

#define CMD 0
#define TIMEOUT 98
#define ECHO 1

#define  NO_GPA		0
#define  MTS_GPA	1

#define MTS_TIMEOUT 100		/* Units of 10 ms */
#define MTS_RETRYS 2
#define MTS_INITIAL_STATUS 0x101010

#define MTS_NO_MSG 1
#define MTS_NULL_MSG 2
#define MTS_SUMMARY_STATUS_MSG 4
#define MTS_ERROR_MSG 8
#define MTS_ACK_MSG 16
#define MTS_UNKNOWN_MSG 32
#define MTS_ANY_MSG (~0)

#define MTS_NO_COMM (1 << 24)

#define GPA_TASK_PRIORITY	(61)
#define GPA_TASK_OPTIONS	(0)
#define GPA_STACK_SIZE		(2048+1024)
#define GPA_CHECK_RATE		(sysClkRateGet() * 5)

extern char *getreply(int, int, char*, int, char*, int);

static int gpaPort = -1;
static SEM_ID pGpaMutex = NULL;
static SEM_ID pGpaMessage = NULL;
static int gpaType = -1;
static int gpaTaskId = 0;
static time_t disableTime = 0;

static char *xAddr = "162";
static char *yAddr = "164";
static char *zAddr = "166";

static GpaInfo *pGpaInfo;

#ifdef USE_MTS
static int readMtsType();
static void setMtsEnable(short);
#endif

#if ( CPU==CPU32 )

typedef struct {
                 int action;
                 int enable;
                 int disable;
                 short px;
                 short ix;
                 short py;
                 short iy;
                 short pz;
                 short iz;
               } MBOX_MESSAGE;

static MBOX_MESSAGE mbx;



/* *pShareMemoryAddr - address of beginning of share memory, just past SM backplane, globals.h */

#define SERIAL_PORT_NUMBER	2

static int
initGpaCtrl()
{
    /* Init the status info pointer */
    /* pGpaInfo = (GpaInfo*)(AUTO_SHARED_GPA(MPU332_RAM+AUTO_SHARED_MEM_OFFSET)); */
    pGpaInfo = (GpaInfo*)(AUTO_SHARED_GPA(pShareMemoryAddr));
    pGpaInfo->hdr.pxTune = -1;
    pGpaInfo->hdr.ixTune = -1;
    pGpaInfo->hdr.pyTune = -1;
    pGpaInfo->hdr.iyTune = -1;
    pGpaInfo->hdr.pzTune = -1;
    pGpaInfo->hdr.izTune = -1;
    pGpaInfo->hdr.gpaStat = MTS_NO_COMM;
    mbx.action = 0;
    mbx.enable = -1;
    mbx.disable = -1;
    mbx.px = -1;
    mbx.ix = -1;
    mbx.py = -1;
    mbx.iy = -1;
    mbx.pz = -1;
    mbx.iz = -1;
#ifdef USE_MTS
    if (gpaPort >= 0) {
	close( gpaPort );
    }
    gpaPort = initSerialPort(SERIAL_PORT_NUMBER);
    if (gpaPort < 0) {
	return -1;
    }
    setSerialTimeout(gpaPort, 25);	/* set timeout to 1/4 sec */
    pGpaMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
			       SEM_DELETE_SAFE);
    if (pGpaMutex == NULL) {
        errLogRet(LOGIT, debugInfo, "initGpaCtrl: Mutex Pointer is NULL\n");
        return -1;
    }
    pGpaMessage = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
    if (pGpaMessage == NULL) {
        errLogRet(LOGIT, debugInfo, "initGpaCtrl: Message Pointer is NULL\n");
        return -1;
    }
#endif
    return 0;
}

#ifdef USE_MTS

static int
zapChars(char *str, char *zaps)
{
    char *p;

    for (p=str; p && *p; str++, p++) {
	while (*p && strchr(zaps, *p)) {
	    p++;
	}
	*str = *p;
    }
    *str = NULL;
    return 1;
}

static int
getMtsMsgType(char *msgstr)
{
    int type;
    int len;
    char msg[129];

    if (!msgstr) {
        return MTS_NO_MSG;
    }

    /* Don't modify the original message */
    strncpy(msg, msgstr, sizeof(msg));
    msg[sizeof(msg)-1] = '\0';

    /* Discard some control chars */
    /*zapChars(msg, "\006\030\021\023");/* ACK, CAN, DC1, DC3 */
    zapChars(msg, "\030\021\023");/* CAN, DC1, DC3 */

    if ((len=strlen(msg)) == 0) {
	type = MTS_NULL_MSG;
    }else if (msg[0] == '\025') { /* NAK */
	type = MTS_ERROR_MSG;
    }else if ( msg[0] == '\006' && len == 1) { /* ACK */
	type = MTS_ACK_MSG;
    }else if (zapChars(msg, "\006") && (len=strlen(msg)) == 6 &&
	      strspn(msg, "0123456789ABCDEF") == len)
    {
	type = MTS_SUMMARY_STATUS_MSG;
    }else{
	type = MTS_UNKNOWN_MSG;
    }
    return type;
}

static char *
getMtsMsg(int msg_type, char *msg, int buflen, char *replylist, int replylen)
{
    int i;
    int type;
    char *test;

    for (type=0; !(type & msg_type) && !(type & MTS_NO_MSG); ) {
	test = getreply(gpaPort, MTS_TIMEOUT, msg, buflen, replylist, replylen);
	type = getMtsMsgType(test);
    }
    return test;
}

static char *
sendMtsCommand(char *cmd, char *reply, int buflen,
	       char *replylist, int replylen)
{
    if (gpaPort < 0) {
	return NULL;
    }

    semTake(pGpaMutex,WAIT_FOREVER); /* protect GPA serial comm */

    clearport(gpaPort);
    putstring(gpaPort, cmd);
    /* Wait for confirmation */
    reply = getMtsMsg(MTS_ANY_MSG, reply, buflen, replylist, replylen);

    semGive(pGpaMutex);

    return reply;
}

static int
getMtsTunePars(char *addr, short *pTmp, short *iTmp)
{
    char *msg;
    char cmd[128];
    char reply[128];

    /* Query the MTS */
    sprintf(cmd,"TUN %s\r", addr);
    msg = sendMtsCommand(cmd, reply, sizeof(reply), "0123456789", 6);
    if (msg == NULL) {
	return GPAERROR + GPA_NO_COMM;
    }

    /* Decode the reply */
    while (*msg && !isdigit(*msg)) { 	/* Skip all preamble stuff */
	msg++;
    }
    *pTmp = (short)atoi(msg);		/* Read first number */
    while (*msg && isdigit(*msg)) { 	/* Position after first number */
	msg++;
    }
    while (*msg && !isdigit(*msg)) { 	/* Skip separator */
	msg++;
    }
    *iTmp = (short)atoi(msg);		/* Read second number */

    return OK;
}

static int
getMtsStatus()
{
    char msg[128];
    char *pc;
    int status;

    /* Read status:
     * 6 hex digits: 2 for X, next 2 for Y, last 2 for Z
     */
    pc = sendMtsCommand("SAX\r", msg, sizeof(msg), "0123456789ABCDEFabcdef", 6);
    if (pc) {
	while (!isxdigit(*pc)) { 	/* Skip all preamble stuff */
	    pc++;
	}
	status = (int)strtol(pc, NULL, 16);
    } else {
	/* No MTS communication */
	status = MTS_NO_COMM;
    }
    return status;
}

static void
updateAllMtsTunePars()
{
    getMtsTunePars(xAddr, &pGpaInfo->hdr.pxTune, &pGpaInfo->hdr.ixTune);
    getMtsTunePars(yAddr, &pGpaInfo->hdr.pyTune, &pGpaInfo->hdr.iyTune);
    getMtsTunePars(zAddr, &pGpaInfo->hdr.pzTune, &pGpaInfo->hdr.izTune);
}

static int
setMtsTuning(int cmd, short value)
{
    int rtn;
    int i;
    int stat;
    short pTune;
    short iTune;
    short pTmp = -1;
    short iTmp = -1;
    char cmdString[32];
    char reply[128];
    char *msg;
    char *addr;

    if (value < 0 || value > 255) {
       return OK;       /* Generic GPA error */
    }

    switch (cmd) {
      case SET_GPA_TUNE_PX:
      case SET_GPA_TUNE_IX:
	addr = xAddr;
	break;
      case SET_GPA_TUNE_PY:
      case SET_GPA_TUNE_IY:
	addr = yAddr;
	break;
      case SET_GPA_TUNE_PZ:
      case SET_GPA_TUNE_IZ:
	addr = zAddr;
	break;
      default:
	/* Can not happen */
	break;
    }
    rtn = getMtsTunePars(addr, &pTmp, &iTmp);
    if (rtn != 0) {
        return rtn;             /* Bad communication? */
    }

    switch (cmd) {
      case SET_GPA_TUNE_PX:
      case SET_GPA_TUNE_PY:
      case SET_GPA_TUNE_PZ:
	pTune = value;
	iTune = iTmp;
	break;
      case SET_GPA_TUNE_IX:
      case SET_GPA_TUNE_IY:
      case SET_GPA_TUNE_IZ:
	pTune = pTmp;
	iTune = value;
	break;
      default:
	/* Can not happen */
	break;
    }
    sprintf(cmdString,"TUN %s,%d,%d\r", addr, pTune, iTune);
    for (i=0; (i <= MTS_RETRYS) && (pTmp != pTune || iTmp != iTune); i++) {
	/* Try to set new value until readback values match command values,
	 * or until we gat a command acknowledge,
	 * or until we run out of retries. */
	/* Just wait for <ACK> or <NAK> */
	sendMtsCommand(cmdString, reply, sizeof(reply), "\006\025", 1);
	if (getMtsMsgType(reply) == MTS_ACK_MSG) {
	    pTmp = pTune;
	    iTmp = iTune;
	    break;
	}
	getMtsTunePars(addr, &pTmp, &iTmp);
    }
    rtn = (pTmp != pTune || iTmp != iTune) ? (GPAERROR + GPA_WONT_TUNE) : OK;

    /* Set values in status block */
    if (addr == xAddr) {
	pGpaInfo->hdr.pxTune = pTmp;
	pGpaInfo->hdr.ixTune = iTmp;
    } else if (addr == yAddr) {
	pGpaInfo->hdr.pyTune = pTmp;
	pGpaInfo->hdr.iyTune = iTmp;
    } else if (addr == zAddr) {
	pGpaInfo->hdr.pzTune = pTmp;
	pGpaInfo->hdr.izTune = iTmp;
    }

    return rtn;
}

static void
initMts()
{
    char msg[128];

    /* fprintf(stderr,"initMts()\n");/*CMP*/
    /* Only wait for an <ACK> char on these */
    sendMtsCommand("CLF\r", msg, sizeof(msg), "\006", 1);
    /* NB: New policy - keep amps disabled except during an experiment */
    /*sendMtsCommand("ENA\r", msg, sizeof(msg), "\006", 1);*/
    updateAllMtsTunePars();
    pGpaInfo->hdr.gpaStat = getMtsStatus();
}

static void
setMtsEnable(short value)
{
    char msg[128];
    char *cmd;
    char *reply = 0;
    int timeout;
    int retry;

    /* Otherwise, do something immediately */
    if (value == 0) {
        cmd = "DIS\r";
        timeout = 0;
    } else {
        cmd = "ENA\r";
        semTake(pGpaMutex,WAIT_FOREVER); /* protect gpa serial comm */
        /* Delete "disable" command, but still do the enable */
        disableTime = 0;
	semGive(pGpaMutex);
        timeout = (10 * 100 + MTS_TIMEOUT - 1) / MTS_TIMEOUT; /* 10 seconds */
        if (pGpaInfo->hdr.gpaStat != 0) {
            initMts();                 /* Clear errors; read tune values */
        }
    }        

    fprintf(stderr,"Sending %c%c%c to GPA ... ", cmd[0], cmd[1], cmd[2]);/*CMP*/
    for (retry = -1; reply == 0 && retry < timeout; retry++) {
        reply = sendMtsCommand(cmd, msg, sizeof(msg), "\006", 1);
    }
    fprintf(stderr,"reply is \"%s\"\n", reply);/*CMP*/
    pGpaInfo->hdr.gpaStat = getMtsStatus();
    if (pGpaInfo->hdr.gpaStat != 0) {
        /* Oops, status is bad, try to reset errors and try again */
        retry = 1;
        initMts();                 /* Clear errors; read tune values */
        sendMtsCommand(cmd, msg, sizeof(msg), "\006", 1);
        fprintf(stderr,"Sent %c%c%c (again)\n", cmd[0], cmd[1], cmd[2]);/*CMP*/
    }
}

static int
readMtsType()
{
    int type = -1;
    char msg[128];
    char *test;

    /* Ask for summary status */
    test = sendMtsCommand("SAX\r", msg, sizeof(msg),
			  "0123456789ABCDEFabcdef", 6);
    if (test) {
	type = MTS_GPA;
	fprintf(stdout,"### Found MTS Gradient Amp: status=%s ###\n", msg);
	/* initialize */
	initMts();
    }else {
	pGpaInfo->hdr.gpaStat = MTS_NO_COMM; /* Set NO COMMUNICATION bit */
	fprintf(stdout,"### MTS Gradient Amp not found ###\n");
    }
    return type;
}

gpaTask()
{
    int ticks = GPA_CHECK_RATE;
    while (1) {

        semTake(pGpaMessage,ticks);

        semTake(pGpaMutex,WAIT_FOREVER);
        if (mbx.action)
        {
            mbx.action = 0;
            if (gpaType < 0)
                gpaType = readMtsType();
            if (mbx.disable > 0)
            {
                disableTime = mbx.disable;
                mbx.disable = -1;
            }
            if (mbx.enable > 0)
            {
               setMtsEnable(1);
               mbx.enable = -1;
            }
            if (mbx.px >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_PX, mbx.px);
               mbx.px = -1;
            }
            if (mbx.ix >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_IX, mbx.ix);
               mbx.ix = -1;
            }
            if (mbx.py >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_PY, mbx.py);
               mbx.py = -1;
            }
            if (mbx.iy >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_IY, mbx.iy);
               mbx.iy = -1;
            }
            if (mbx.pz >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_PZ, mbx.pz);
               mbx.pz = -1;
            }
            if (mbx.iz >= 0)
            {
               setMtsTuning(SET_GPA_TUNE_IZ, mbx.iz);
               mbx.iz = -1;
            }
        }
	if (pifchr(gpaPort)) {
	    /* GPA sent something, update status */
	    pGpaInfo->hdr.gpaStat = getMtsStatus();
	    if (pGpaInfo->hdr.gpaStat == MTS_INITIAL_STATUS) {
		initMts();
	    }
	    clearinput(gpaPort);
            fprintf(stderr, "MTS status = 0x%x\n", pGpaInfo->hdr.gpaStat);
	} else {
            /* See if it's alive */
            int i = pGpaInfo->hdr.gpaStat;
            pGpaInfo->hdr.gpaStat = getMtsStatus();
            if (pGpaInfo->hdr.gpaStat != i) {
                fprintf(stderr, "MTS status changed: old = 0x%x, new = 0x%x\n",
                        i, pGpaInfo->hdr.gpaStat);
            }
            if (pGpaInfo->hdr.gpaStat != MTS_NO_COMM) {
                if (gpaType < 0)
                  gpaType = readMtsType();
                updateAllMtsTunePars();
            }
        }

        if (disableTime && disableTime < time(0)) {
            /* Time to disable amp */
            if (pGpaInfo->hdr.gpaStat != MTS_NO_COMM)
               setMtsEnable(0);
            disableTime = 0;
        }

	semGive(pGpaMutex);

    }
}
#endif


/*  This function is called from mboxTasks.c when messages arrive from the CPU */
int
setGpaTuning(short cmd, short value)
{
    int status = OK;            /* NB: OK==0 in vxWorks */

    switch (gpaType) {
      case MTS_GPA:
        semTake(pGpaMutex,WAIT_FOREVER); /* protect gpa serial comm */
        if (cmd == SET_GPAENABLE) {
            if (value < 0)
            {
                mbx.disable = time(0) - value; /* When message received */
                mbx.enable = -1;
            }
            else
            {
                mbx.disable = -1;
                mbx.enable = 1;
            }
            mbx.action = 1;
        } else {
            switch (cmd) {
              case SET_GPA_TUNE_PX:
                mbx.px = value;
	        break;
              case SET_GPA_TUNE_IX:
                mbx.ix = value;
	        break;
              case SET_GPA_TUNE_PY:
                mbx.py = value;
	        break;
              case SET_GPA_TUNE_IY:
                mbx.iy = value;
	        break;
              case SET_GPA_TUNE_PZ:
                mbx.pz = value;
	        break;
              case SET_GPA_TUNE_IZ:
                mbx.iz = value;
	        break;
              default:
	        /* Can not happen */
	        break;
            }
            mbx.action = 1;
        }
        semGive(pGpaMutex);
	semGive(pGpaMessage);
	break;
      default:
	status = ERROR;
	break;
    }
    return status;
}

/*  This function is called from the MSR initialization functions in autoInit.c */
void
determineGpaType()
{
    int res;
    gpaType = -1;
    DPRINT(1,"determineGpaType(): Local QUART active.\n");
    if( tyQCoPresent() )   /* is local QUART active ? */
    {
        if (initGpaCtrl() == OK)
        {
#ifdef USE_MTS
           gpaTaskId = taskSpawn("tGpaTsk",
                              GPA_TASK_PRIORITY,
                              GPA_TASK_OPTIONS,
                              GPA_STACK_SIZE,
                              gpaTask,
                              1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
           if (gpaTaskId == ERROR)
               errLogSysRet(LOGIT, debugInfo, "readMtsType: could not spawn tGpaTsk:");
#else
           DPRINT(1,"skip GPA task\n");
#endif
        }
    }
}


void
gpaTypeShow()
{
    switch (gpaType) {
      case NO_GPA:
	printf("GPA control not present\n" );
	break;
      case MTS_GPA:
	printf("MTS GPA interfaced through the MSR Board\n" );
	break;
      default:
	printf("Unknown GPA type %d\n", gpaType );
	break;
    }
}

#endif
