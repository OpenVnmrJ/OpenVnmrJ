/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#ifndef __INTERIX
  #include <termio.h>    /* solaris & Linux, though one might also be able to use termios.h */
#else
  #include <termios.h>   /* Interix/SFU under Windows */
#endif
#include "errLogLib.h"
#include "msgQLib.h"
#include "timerfuncs.h"
#include "iofuncs.h"
#include "errorcodes.h"
#include "acqerrmsges.h"
#include "acquisition.h"
#include "acqcmds.h"
#include "hrm_errors.h"

#include "rackObj.h"
#include "gilfuncs.h"
#include "gilsonObj.h"

#define STARTUP_CMDTIME  180000
#define ROBOT_GRACEPERIOD 30000

#define MAX_PCS_SLOTS          7
#define MAX_TIMERS            ((MAX_PCS_SLOTS+1)*4+1)

#define RACKTOBCR_TIME        15
#define BCRTOPCS_TIME         17
#define RACKTOPCS_TIME        26
#define PCSTOMAG_TIME         17
#define MAGTOEXT_TIME         13
#define EXTTORACK_TIME        27
#define INSERT_TIME            5
#define EJECT_TIME             5

extern int AbortRobo;


typedef enum _t_robotOp {
  NOTRANSPORT=0,
  RACKTOPCS,
  PCSTOMAG,
  MAGTORACK
} eRobotOp;

typedef struct _t_PCS_ERROR {
    int smp;
    int stat;
} PCS_ERROR;
PCS_ERROR pcs_xfer_error[MAX_PCS_SLOTS];

/*
 * Timer list is sorted by timeLeft
 *  with least number of seconds first.
 * When seconds left are equal, they are
 *  inserted in this priority order
 *  with lower numbers inserted first.
 */
#define MYTIMER_DONE           1
#define MYTIMER_MAGNET         2
#define MYTIMER_PCS            3
#define MYTIMER_PREP           4

typedef struct _t_MYTIMER {
    int valid;
    int timeLeft;
    int sampleIndex;
    int timerType;
    struct _t_MYTIMER *next;
} MYTIMER;

static MYTIMER timer[MAX_TIMERS];
static MYTIMER *timerList;
static MYTIMER *freeTimerList;
static MYTIMER actualPcsTimer[MAX_PCS_SLOTS];
static MYTIMER actualExpTimer[MAX_PCS_SLOTS];

struct schedItem {
  int start;
  int end;
  int slot;
  int smp;
  char desc[12];
  struct schedItem *next;
};

#define MAX_SCHED_SLOTS ((MAX_PCS_SLOTS+1)*5)
static struct schedItem schedItems[MAX_SCHED_SLOTS];
static struct schedItem *schedMap;
static struct schedItem *freeSchedMap;


typedef struct _t_SAMPLE_OBJ {
    MYTIMER *prepStartTimer;
    MYTIMER *pcsStartTimer;
    MYTIMER *pcsDoneTimer;
    MYTIMER *magnetEmptyTimer;
    int startLoc;
    int endLoc;
    int bcrSec;
    int pcsSec;
    int magSec;
    char gilPreScript[256];
    int gilPreTime;
    char gilPostScript[256];
    int gilPostTime;
    char barcode[16];
} SAMPLE_OBJ;

static SAMPLE_OBJ sample[MAX_PCS_SLOTS];

typedef struct t_SAMPLE {
    int rack;
    int zone;
    int well;
    int bcrSec;
    int pcsSec;
    int magSec;
    struct t_SAMPLE *next;
} SAMPLE;

static SAMPLE *SampleQueueHead = NULL;
static SAMPLE *SampleQueueTail = NULL;

static int pcsSlotPopulated[MAX_PCS_SLOTS];

static int robotError = HRM_SUCCESS;
static int sampleInExtractionStation = -1;
static int sampleInExtPcsSlot = -1;
static int sampleInMagnet = -1;
static int sampleInMagPcsSlot = -1;
static int slotsReserved = 0;
static int scheduler_initialized = 0;

static int sockBCR = -1;
static int sockRABBIT = -1;

static int barcodeUse=0, barcodeTimeout=5;
static int overrideExpTime = 0, expTime = -1;
static int useGilson=0, gilPreTime=0, gilPostTime=0;
static char bcrErrAction[32];
static char gilPreScript[256], gilPostScript[256];

static int rackToBcrTime=0;
static int bcrToPcsTime=0;
static int rackToPcsTime=0;
static int pcsToMagTime=0;
static int magToExtTime=0;
static int extToRackTime=0;
static int preconditionTime=0;
static int insertTime=0;
static int ejectTime=0;
static int useEStop = 0;

static int clearCompletionList = 1;
static int readSettings = 1;
static int newBatch = 1;

static int prev_loc= 0;

typedef struct t_statStruct {
    char tmCmp[9];
    char smpLoc[13];
    char barcode[9];
    char pcsTgt[5];
    char pcsAct[5];
    char expTgt[5];
    char expAct[5];
    char status[80];
} statStruct;
static statStruct currentSampleStatus;

/* Forward Declarations */
void init_scheduler(void);
void schedule_new_samples(void);
int hermes_clear_extraction_station();
void StartBatch();
void EndBatch();

#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-truncation"


char * GetAcqErrMsg(int errCode)
{
    extern struct codeWithMsg acqerrMsgTable[];
    struct codeWithMsg *acqerrMsgTablePtr;
    acqerrMsgTablePtr = &acqerrMsgTable[0];

    while (acqerrMsgTablePtr->code != 0) {
        if (acqerrMsgTablePtr->code == errCode)
            return acqerrMsgTablePtr->errmsg;
        acqerrMsgTablePtr++;
    }

    return "Error";
}
/*----------------------------Timer Functions-------------------------------*/


static sigset_t orig_sigmask, semaphore_sigmask;
static struct sigaction orig_sigalrm_action;
static struct itimerval orig_itimer;

void MyAcquireSemaphore(void)
{
    sigset_t qmask;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    if (sigprocmask( SIG_BLOCK, &qmask, &semaphore_sigmask ) != 0) {
        DPRINT1(0,"Roboproc exiting, sigprocmask error: %s\n",
                strerror(errno));
        exit(1);
    }

}

void MyReleaseSemaphore(void)
{
    sigprocmask( SIG_SETMASK, &semaphore_sigmask, NULL );
}

static void sigalrm_irpt()
{
    int i, sendMsg = 0;
    MYTIMER *ptr;
    struct schedItem *sPtr = schedMap;
    sigset_t qmask;
    struct sigaction sigalrm_action;
    extern MSG_Q_ID pRecvMsgQ;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    sigaction( SIGALRM, &sigalrm_action, NULL );

    /* Decrement Active Timers */
    for (ptr = timerList; ptr != NULL; ptr = ptr->next) {
        if (ptr->valid) {
            ptr->timeLeft--;
            if ((ptr->timeLeft == 0) &&
                ((ptr->timerType == MYTIMER_PREP) ||
                 (ptr->timerType == MYTIMER_PCS)) )
                sendMsg = 1;
        }
    }

    /* Adjust start/end times in schedule */
    while (sPtr != NULL) {
        sPtr->start -= 1;
        sPtr->end -= 1;
        sPtr = sPtr->next;
    }

    /* Increment PCS Timers */
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (actualPcsTimer[i].valid)
            actualPcsTimer[i].timeLeft++;
    }

    /* Increment EXP Timers */
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (actualExpTimer[i].valid)
            actualExpTimer[i].timeLeft++;
    }

    if (sendMsg) {
        sendMsgQ(pRecvMsgQ, "chktimers", strlen("chktimers"),
                MSGQ_NORMAL, WAIT_FOREVER);
   }
}

void FreeTimerList()
{
    int i;
    MYTIMER *ptr;

    timerList = NULL;

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        sample[i].pcsStartTimer = NULL;
        sample[i].pcsDoneTimer = NULL;
        sample[i].magnetEmptyTimer = NULL;
    }

    /* Zero timer list */
    memset(timer, 0, sizeof(timer));

    /* Put all timers on the free list */
    freeTimerList = &(timer[0]);
    for (i=1, ptr=freeTimerList; i<MAX_TIMERS; i++) {
        ptr->next = &(timer[i]);
        ptr = ptr->next;
    }
    ptr->next = NULL;
}

void DeleteSchedItem(int slot, char *desc)
{
    struct schedItem *freeTail = freeSchedMap;
    struct schedItem *ptr = schedMap, *lastPtr;

    while (freeTail != NULL) {
        if (freeTail->next != NULL)
            freeTail = freeTail->next;
        else
            break;
    }

    lastPtr = NULL;
    while (ptr != NULL) {
        if ((ptr->slot == slot) && (!strcmp(ptr->desc, desc))) {
            struct schedItem *tmp;

            if (lastPtr)
                lastPtr->next = ptr->next;
            else
                schedMap = ptr->next;

            tmp = ptr;
            tmp->start = 0;
            tmp->end = 0;
            tmp->slot = 0;
            tmp->next = NULL;

            if (freeTail)
              freeTail->next = tmp;
            else
              freeSchedMap = tmp;

            return;
        }

        else {
            lastPtr = ptr;
            ptr = ptr->next;
        }
    }
}

void FreeSchedMap()
{
    int i;
    struct schedItem *ptr;

    /* Zero map */
    memset(schedItems, 0, sizeof(schedItems));

    schedMap = NULL;
    freeSchedMap = &(schedItems[0]);

    for (i=1, ptr=freeSchedMap; i<MAX_SCHED_SLOTS; i++) {
        ptr->next = &(schedItems[i]);
        ptr = ptr->next;
    }
    ptr->next = NULL;
}


void UnblockSigalarm(void)
{
    /* Make Sure SIGALRM is UNBLOCKED!! */
    sigset_t         qmask;
    struct sigaction sigalrm_action;
    struct itimerval new_itimer;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    if (sigaction( SIGALRM, &sigalrm_action, &orig_sigalrm_action ) != 0) {
        perror("sigaction error");
        return;
    }

    new_itimer.it_interval.tv_sec = 1;
    new_itimer.it_interval.tv_usec = 0;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;
    if (setitimer( ITIMER_REAL, &new_itimer, &orig_itimer ) != 0) {
        perror("setitimer error");
        return;
    }
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    if (sigprocmask( SIG_UNBLOCK, &qmask, &orig_sigmask ) != 0) {
        perror("sigprocmask error");
        return;
    }
}


int start_timer()
{
    sigset_t qmask;
    struct sigaction sigalrm_action;
    struct itimerval new_itimer;

    /* See if we've already been here */
    if ((timerList != NULL) || (freeTimerList != NULL))
        return 0;

    /* Create timer list */
    FreeTimerList();

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    if (sigaction( SIGALRM, &sigalrm_action, &orig_sigalrm_action ) != 0) {
      perror("sigaction error");
      return -1;
    }
    new_itimer.it_interval.tv_sec = 1;
    new_itimer.it_interval.tv_usec = 0;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;
    if (setitimer( ITIMER_REAL, &new_itimer, &orig_itimer ) != 0) {
      perror("setitimer error");
      return -1;
    }
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    if (sigprocmask( SIG_UNBLOCK, &qmask, &orig_sigmask ) != 0) {
      perror("sigprocmask error");
      return -1;
    }

    return 0;
}

void delete_timer(MYTIMER *t)
{
    MYTIMER *ptr, *lastPtr;

    if (t == NULL) return;

    /* Critical Region around timerList removal */
    MyAcquireSemaphore();

    /* Remove it from the timerList */
    ptr = lastPtr = timerList;
    while (ptr != NULL) {
        if (t == ptr) {
            if (lastPtr == ptr)
                timerList = t->next;
            else
                lastPtr->next = t->next;
            break;
        }
        lastPtr = ptr;
        ptr = ptr->next;
    }

    /* Add it to the freeTimerList */
    t->valid = 0;
    t->timeLeft = 0;
    t->next = freeTimerList;
    freeTimerList = t;

    MyReleaseSemaphore();
}

int timer_expired(MYTIMER *t)
{
    if ((t->valid == 1) && (t->timeLeft <= 0))
        return 1;
    return 0;
}


void insert_timer (MYTIMER *t)
{
    MYTIMER *ptr;
    MYTIMER *lastPtr;

    t->next = NULL;
    if (timerList == NULL) {
        timerList = t;
        return;
    }

    /* Critical Region around timer insertion */
    MyAcquireSemaphore();

    ptr = lastPtr = timerList;
    while (ptr != NULL) {

        if ((ptr->timeLeft > t->timeLeft)  ||
            ((ptr->timeLeft == t->timeLeft) &&
             (ptr->timerType > t->timerType)) ) {
            t->next = ptr;
            if (ptr == lastPtr)
                timerList = t;
            else
                lastPtr->next = t;
            break;
        }
        lastPtr = ptr;
        ptr = ptr->next;
    }

    if (ptr == NULL)
        lastPtr->next = t;

    MyReleaseSemaphore();
}


MYTIMER *set_timer(int sampleIndex, int timerType, int numSec)
{
    MYTIMER *t;

    if (freeTimerList == NULL) {
        DPRINT(0, "******* ROBOPROC WARNING : OUT OF TIMERS! ********\r\n");
        return NULL;
    }

    t = freeTimerList;
    freeTimerList = t->next;

    t->next = NULL;
    t->valid = 1;
    t->timeLeft = numSec;
    t->sampleIndex = sampleIndex;
    t->timerType = timerType;

    insert_timer(t);
    return t;
}

int get_timer(MYTIMER *t)
{
    if (!t->valid)
        return 0;

    return (t->timeLeft);
}

void DisplayTimers()
{
#if 0
  time_t t;
  char tbuf[48];
  MYTIMER *ptr;

  ptr = timerList;
  while (ptr != NULL) {
    switch (ptr->timerType) {
    case MYTIMER_DONE:
      t = time(0) + ptr->timeLeft;
      strftime(tbuf, 48, "%T", localtime( &t ) );
      DPRINT3(1, "%06d : MAG EMPTY at %s (%d sec)\r\n",
              sample[ptr->sampleIndex].endLoc, tbuf, ptr->timeLeft);
      break;
    case MYTIMER_MAGNET:
      t = time(0) + ptr->timeLeft;
      strftime(tbuf, 48, "%T", localtime( &t ) );
      DPRINT3(1, "%06d : PCS DONE at %s (%d sec)\r\n",
              sample[ptr->sampleIndex].endLoc, tbuf, ptr->timeLeft);
      break;
    case MYTIMER_PCS:
      t = time(0) + ptr->timeLeft;
      strftime(tbuf, 48, "%T", localtime( &t ) );
      DPRINT3(1, "%06d : PCS START at %s (%d sec)\r\n",
              sample[ptr->sampleIndex].endLoc, tbuf, ptr->timeLeft);
      break;
    default:
      DPRINT(1, "UNKNOWN TIMER!\r\n");
      break;
    }
    ptr = ptr->next;
  }
  DPRINT(1, "\r\n");
#endif
}


/*-----------------------------COMM FUNCTIONS-------------------------------*/
void ReconnectDevice(int *sock, char *devStr)
{
    close_lan(*sock);
    *sock = open_lan(devStr);
    if (*sock <= 0) {
        *sock = open_lan(devStr);
        if (*sock <= 0) {
            DPRINT1(0, "Error trying to re-connect to device %s\r\n",
                      devStr);
        }
    }
    return;
}

int SendRabbit(char *cmd, char *rspStr)
{
    int result, j, timeout=30*1000 /* 30 seconds */;
    time_t t;
    char tbuf[48], buf[32];

    if (!scheduler_initialized)
        init_scheduler();

    if (cmd[0] != '\0') {
        extern int lan_socktxrdy(int sock, int milliseconds);
        if (lan_socktxrdy(sockRABBIT, 500) != 1) {
            DPRINT(0, "Rabbit offline?  Try to reconnect...\r\n");
            ReconnectDevice(&sockRABBIT, "HRM_RABBIT");
            if (sockRABBIT <= 0) {
                DPRINT(0, "SendRabbit ERROR: offline?\r\n");
                DPRINT1(0, "  command attempted: %s\r\n", cmd);
                return -1;
            }
        }

        if (ioctl_lan(sockRABBIT, IO_FLUSH) < 0) {
            DPRINT(0, "Rabbit offline?  Try to reconnect...\r\n");
            ReconnectDevice(&sockRABBIT, "HRM_RABBIT");
            if ((sockRABBIT <= 0) ||
                (ioctl_lan(sockRABBIT, IO_FLUSH) < 0)) {
                DPRINT(0, "SendRabbit ERROR: offline?\r\n");
                DPRINT1(0, "  command attempted: %s\r\n", cmd);
                return -1;
            }
        }

        t = time(0);
        strftime(tbuf, 48, "%T", localtime( &t ) );
        DPRINT2(0, ">>>RABBIT>>>%s:        \"%s\"\r\n", tbuf, cmd);

        *rspStr = '\0';

        /* Write the Command */
        result = write_lan(sockRABBIT, cmd, strlen(cmd));

        /* Rabbit Reset Our Connection - Try to Re-connect */
        if (result < 0) {
            DPRINT(0, "Rabbit offline?  Try to reconnect...\r\n");
            ReconnectDevice(&sockRABBIT, "HRM_RABBIT");

            /* Re-connect Worked - Now Try to Send Command Again */
            if (sockRABBIT >= 0) {
                result = write_lan(sockRABBIT, cmd, strlen(cmd));
            }
        }

        if (result < 0) {
            DPRINT(0, "SendRabbit ERROR: offline?\r\n");
            DPRINT1(0, "  command attempted: %s\r\n", cmd);
            DPRINT3(0, "  write_lan error: result=%d, errno=%d (%s)\r\n",
                    result, errno, strerror(errno));
            return -1;
        }
    }

    /* Read the Response */
    j = 0;
    *buf = '\0';
    *rspStr = '\0';
    while (j < 32) {
        result = read_lan_timed(sockRABBIT, buf, 32-j, timeout);
        if (result <= 0) {
            if (!AbortRobo) {

                /* Ignore "interrupted system call" errors */
                /*                if (errno == EINTR) */
                /*                    continue; */

                t = time(0);
                strftime(tbuf, 48, "%T", localtime( &t ) );
                DPRINT1(0, "SendRabbit ERROR (%s): offline?\r\n", tbuf);
                DPRINT1(0, "  command attempted: %s\r\n", cmd);
                DPRINT3(0, "  read_lan err: result=%d, errno=%d (%s)\r\n",
                        result, errno, strerror(errno));

                rspStr[j] = '\0';
                return -1;
            }
            else {
                DPRINT(0, "SendRabbit: HRM_OPERATION_ABORTED:\r\n");
                DPRINT1(0, "  command attempted: %s\r\n", cmd);
                rspStr[j] = '\0';
                return -1;
            }
        }

        /* Only return characters between '<' and '>' in the response */
        if (j == 0) {
            char *ptr;
            int len;
            buf[result] = '\0';
            if ((ptr = strchr(buf, 0x3C /* < */)) != NULL) {
                len = (int)((unsigned long)ptr - (unsigned long)buf);
                strncpy(rspStr, ptr, result-len);
                j = result-len;
            }
            else
                continue;
        }
        else {
            strncpy(rspStr+j, buf, result);
            j += result;
        }

        if ((*(rspStr+(j-1)) == '\n') ||
            (*(rspStr+(j-1)) == '\r') ||
            (*(rspStr+(j-1)) == '>')) {
            *(rspStr+j) = '\0';
            break;
        }
    }
    *(rspStr+j) = '\0';

    t = time(0);
    strftime(tbuf, 48, "%T", localtime( &t ) );
    DPRINT2(0, "<<<RABBIT <<%s:        \"%s\"\r\n", tbuf, rspStr);

    /* Success */
    return 0;
}


typedef struct _t_HRM_ROBOT_CMD {
    char cmdStr[80];
    char rspStr[80];
    int  wait;
    int  timeout;
    int  rspValid;
    int  errVal;
    int  commErr;
} HRM_ROBOT_CMD;

void HrmSendRobotCmd(HRM_ROBOT_CMD *cmd)
{
    int result,i=0;
    char tbuf[48];
    time_t t;
    extern int AbortRobo;

    cmd->errVal = cmd->commErr = cmd->rspValid = 0;

    if (AbortRobo) {
        cmd->errVal = HRM_OPERATION_ABORTED;
        return;
    }

    /* No Connection to the Robot - Try Connecting Now */
    if (smsDev < 0) {
        smsDev = open_lan("HRM_ROBOT");
        if (smsDev < 0) {
            smsDev = open_lan("HRM_ROBOT");
            if (smsDev < 0) {
                DPRINT(0, "Error trying to re-connect to robot\r\n");
                cmd->errVal = SMPERROR+HRM_COMMERROR;
                cmd->commErr = errno;
                return;
            }
        }
    }

    t = time(0);
    strftime(tbuf, 48, "%T", localtime( &t ) );
    DPRINT2(0, ">>>>>%s:        \"%s\"\r\n", tbuf,
        ((*(cmd->cmdStr) == '\0') ? "WAITING..." : cmd->cmdStr));

    if (ioctl_lan(smsDev, IO_FLUSH) < 0) {
        DPRINT(0, "Robot offline?  Try to reconnect...\r\n");
        ReconnectDevice(&smsDev, "HRM_ROBOT");
        if ((smsDev <= 0) ||
            (ioctl_lan(smsDev, IO_FLUSH) < 0)) {
            DPRINT(0, "HrmSendRobotCmd ERROR: offline?\r\n");
            DPRINT1(0, "  command attempted: %s\r\n", cmd->cmdStr);
            cmd->errVal = SMPERROR+HRM_COMMERROR;
            cmd->commErr = errno;
            return;
        }
    }

    if (*(cmd->cmdStr) != '\0') {
        result = write_lan(smsDev, cmd->cmdStr, strlen(cmd->cmdStr));

        /* Error on Socket - Try Closing and Re-opening Robot Connection */
        if (result < 0) {
            DPRINT(0, "Robot offline?  Try to reconnect...\r\n");
            ReconnectDevice(&smsDev, "HRM_ROBOT");
            if (smsDev <= 0) {
                DPRINT(0, "HrmSendRobotCmd ERROR: offline?\r\n");
                DPRINT1(0, "  command attempted: %s\r\n", cmd->cmdStr);
                cmd->errVal = SMPERROR+HRM_COMMERROR;
                cmd->commErr = errno;
                return;
            }

            /* Re-connect Worked - Now Try to Send Command Again */
            if (smsDev >= 0) {
                result = write_lan(smsDev, cmd->cmdStr, strlen(cmd->cmdStr));
            }
        }

        if (result < 0) {
            DPRINT(0, "HrmSendRobotCmd: returning HRM_ROBOT_OFFLINE:\r\n");
            DPRINT1(0, "  command attempted: %s\r\n", cmd->cmdStr);
            DPRINT3(0, "  write_lan error: result=%d, errno=%d (%s)\r\n",
                    result, errno, strerror(errno));
            cmd->errVal = SMPERROR+SMPTIMEOUT;
            cmd->commErr = errno;
            return;
        }
    }

    if (cmd->wait) {
        int followupTimer;

        if (*(cmd->cmdStr) != '\0') {
            /* Always wait the specified timeout to keep scheduler accurate */
            delayAwhile(cmd->timeout / 1000);
            followupTimer = 500; /* ms */
        }
        else {
            /* Second time around and still waiting */
            followupTimer = cmd->timeout;
        }

        while (i < sizeof(cmd->rspStr)) {
            result = read_lan_timed(smsDev, cmd->rspStr+i,
                         sizeof(cmd->rspStr)-i, followupTimer);
            if (result <= 0) {

                /* SIGUSR2 will set this to abort sample change */
                // Fixes protune errors, not sure about other ramifications
                // AbortRobo = 0;
                if (!AbortRobo) {
                    t = time(0);
                    strftime(tbuf, 48, "%T", localtime( &t ) );

                    DPRINT1(0, "!!%s: ERROR: HrmSendRobotCmd: returning "
                              "SMPERROR+SMPTIMEOUT:\r\n", tbuf);
                    DPRINT1(0, "  command attempted: %s\r\n", cmd->cmdStr);
                    DPRINT3(0, "  read_lan err: result=%d, errno=%d (%s)\r\n",
                            result, errno, strerror(errno));
                    cmd->rspStr[i] = '\0';
                    DPRINT2(0, "  read %d chars: %s\r\n", i, cmd->rspStr);
                    cmd->errVal = SMPERROR+SMPTIMEOUT;
                    if (result < 0)
                        cmd->commErr = errno;
                    return;
                }
                else {
                    DPRINT(0, "HrmSendRobotCmd: returning "
                              "HRM_OPERATION_ABORTED:\r\n");
                    DPRINT1(0, "  command attempted: %s\r\n", cmd->cmdStr);
                    cmd->errVal = HRM_OPERATION_ABORTED;
                    return;
                }
            }

            i += result;
            if (*(cmd->rspStr+(i-1)) == '\n')
                break;
        }

        if (i < sizeof(cmd->rspStr))
            cmd->rspStr[i] = '\0';
        cmd->rspValid = 1;

        t = time(0);
        strftime(tbuf, 48, "%T", localtime( &t ) );
        DPRINT2(0, "<<<<<%s:        \"%s\"\r\n", tbuf, cmd->rspStr);
    }
}

int ReadBarcodeByte(char *val, int timeout)
{
    int result;

    result = read_lan_timed(sockBCR, val, 1, timeout);

    if (result < 0) {
        DPRINT(0, "Barcode reader offline?  Try to reconnect...\r\n");
        ReconnectDevice(&sockBCR, "HRM_BARCODE");
        if (sockBCR <= 0) {
            DPRINT(0, "Error trying to re-connect to rabbit barcode "
                      "port\r\n");
            return -1;
        }

        /* Re-connect Worked - Now Try to Read Again */
        if (sockBCR >= 0) {
            result = read_lan_timed(sockBCR, val, 1, timeout);
        }
    }

    if (result < 0) {
        DPRINT2(0, "\r\nBCR Read Error: %d (%s)\r\n", errno, strerror(errno));
        return -1;
    }

    if (result == 0)
        return -1;

    return 0;
}


void ReadBarcode (char *barcode, int bcLen, int timeout)
{
    int i, result;
    char buf[2], junk[32];

    if (ioctl_lan(sockBCR, IO_FLUSH) < 0) {
        DPRINT(0, "Barcode reader offline?  Try to reconnect...\r\n");
        ReconnectDevice(&sockBCR, "HRM_BARCODE");
        if ((sockBCR <= 0) ||
            (ioctl_lan(sockBCR, IO_FLUSH) < 0)) {
            *barcode = '\0';
            DPRINT(0, "ReadBarcode ERROR: offline?\r\n");
            return;
        }
    }

    // Trigger barcode reader
    buf[0] = '+';
    result = write_lan(sockBCR, buf, 1);

    if (result < 0) {
        DPRINT(0, "Barcode reader offline?  Try to reconnect...\r\n");
        ReconnectDevice(&sockBCR, "HRM_BARCODE");

        /* Re-connect Worked - Now Try to Trigger Reader Again */
        if (sockBCR >= 0) {
            buf[0] = '+';
            result = write_lan(sockBCR, buf, 1);
        }
    }

   if (result < 0) {
        DPRINT(0, "ReadBarcode ERROR: offline?\r\n");
        DPRINT3(0, "  write_lan error: result=%d, errno=%d (%s)\r\n",
                result, errno, strerror(errno));

        *barcode = '\0';
        buf[0] = '-'; buf[1] = '\0';
        write_lan(sockBCR, buf, 1);
        ioctl_lan(sockBCR, IO_FLUSH);
        return;
    }

    for (i=0; ; i++) {
        if (ReadBarcodeByte(barcode+i, timeout*1000) < 0) {
            *barcode = '\0';
            break;
        }
        if ((*(barcode+i) == '\n') || (*(barcode+i) == '\r'))
            break;
    }

    // Make sure barcode reader is off
    buf[0] = '-'; buf[1] = '\0';
    write_lan(sockBCR, buf, 1);
    ioctl_lan(sockBCR, IO_FLUSH);
    *(barcode+i) = '\0';

    // If no barcode read, we should now get a NOREAD message
    if (*barcode == '\0') {
        for (i=0, junk[0] = '\0'; ; i++) {
            if (ReadBarcodeByte(junk+i, timeout*1000) < 0) {
                *(junk+i) = '\0';
                break;
            }
            if (*(junk+i) == '\n')
                break;
        }
    }

}

/*-------------------------------APPLICATION--------------------------------*/
void FreeAllPcsSlots()
{
    int i;

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        pcsSlotPopulated[i] = -1;
    }
    slotsReserved = 0;
}

void FreePcsSlot(int loc)
{
    int i;

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (pcsSlotPopulated[i] == loc) {
            pcsSlotPopulated[i] = -1;
            slotsReserved--;
            return;
        }
    }
}

int GetAssignedPcsSlot(int loc)
{
    int i;

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (pcsSlotPopulated[i] == loc) {
            return i;
        }
    }
    return -1;
}

int NextFreePcsSlot(int loc)
{
    int i, j, firstUsed=-1, lastUsed=-1;

    /* Find the first used slot */
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (pcsSlotPopulated[i] >= 0) {
            firstUsed = i;
            break;
        }
    }

    /* No slots used! */
    if (firstUsed == -1) {
        pcsSlotPopulated[0] = loc;
        slotsReserved++;
        return 0;
    }

    /* Find the last used slot */
    for (i=firstUsed, j=0; j< MAX_PCS_SLOTS; j++) {
        if (pcsSlotPopulated[i] < 0) {
            lastUsed = i;
            break;
        }
        i = (i+1) % MAX_PCS_SLOTS;
    }

    /* No free slots! */
    if (lastUsed == -1)
        return -1;

    pcsSlotPopulated[lastUsed] = loc;
    slotsReserved++;
    return (lastUsed);
}


void DisplaySchedule()
{
    struct schedItem *nPtr;
    time_t t;
    char tbuf[48];

    /* Display Sample Map for the User */
    DPRINT(0, "SCHEDULE MAP:\r\n");
    for (nPtr=schedMap; nPtr != NULL; nPtr = nPtr->next) {
        t = time(0) + nPtr->start;
        strftime(tbuf, 48, "%T", localtime( &t ) );
        DPRINT5(0, "  %s (duration=%04d) --%02d-- %06d %s\r\n",
                tbuf, nPtr->end-nPtr->start, nPtr->slot+1,
                nPtr->smp, nPtr->desc);

        if ((nPtr->next != NULL) && (nPtr->end != (nPtr->next)->start)) {
            t = time(0) + nPtr->end;
            strftime(tbuf, 48, "%T", localtime( &t ) );
            DPRINT2(0, "  %s (duration=%04d) -------------------\r\n",
                    tbuf, (nPtr->next)->start-nPtr->end);
        }
    }
}


void DisplaySamples()
{
#if 0
    int i, rack, zone, well;
    SAMPLE *ptr;

    for (i=0; i<MAX_PCS_SLOTS; i++) {

        if (pcsSlotPopulated[i] > 0) {

            rack = (pcsSlotPopulated[i] / 1000000L);
            zone = (pcsSlotPopulated[i] / 10000L % 100);
            well = (pcsSlotPopulated[i] % 1000);

            DPRINT4(0,"SAMPLE R:%d Z:%d W:%02d ---COND_%02d--\r\n",
                    rack, zone, well, i+1);
        }
    }

    for (ptr=SampleQueueHead; ptr != NULL; ptr = ptr->next) {
        DPRINT3(0, "SAMPLE R:%d Z:%d W:%02d ---QUEUED---\r\n",
                ptr->rack, ptr->zone, ptr->well);
    }

#endif
}

int get_next_sample(int *bcrSec, int *pcsSec, int *magSec,
                    int *rack, int *zone, int *well)
{
    SAMPLE *ptr = SampleQueueHead;

    if (ptr == NULL)
        return 0;

    if (SampleQueueTail == SampleQueueHead) {
        SampleQueueHead = NULL;
        SampleQueueTail = NULL;
    }

    else
        SampleQueueHead = ptr->next;

    *bcrSec = ptr->bcrSec;
    *pcsSec = ptr->pcsSec;
    *magSec = ptr->magSec;
    *rack = ptr->rack;
    *zone = ptr->zone;
    *well = ptr->well;

    DPRINT3(0, "get_next_sample: r%02d_z%02d_%02d returned\r\n",
        *rack, *zone, *well);
    free(ptr);

    DisplaySamples();
    return 1;
}


void push_sample(int rack, int zone, int well, int magSec, int pcsSec)
{
    int bcrSec=0;
    SAMPLE *ptr;

    /*
     * Put this sample onto the front of the queue.
     */
    DPRINT3(0, "pushing sample r%02d_z%02d_%02d back onto wait queue\r\n",
            rack, zone, well);

    ptr = calloc(sizeof(SAMPLE), 1);
    if (ptr == NULL)
    {
        DPRINT(0, "push sample: Could not allocate memory.\n");
        return;
    }

    ptr->rack = rack;
    ptr->zone = zone;
    ptr->well = well;
    ptr->bcrSec = bcrSec;
    ptr->pcsSec = pcsSec;
    ptr->magSec = magSec;

    if (SampleQueueHead != NULL) {
        ptr->next = SampleQueueHead;
        SampleQueueHead = ptr;
    }
    else {
        SampleQueueHead = ptr;
        SampleQueueTail = ptr;
    }
}


int queue_sample(int rack, int zone, int well, int magSec, int pcsSec)
{
    int bcrSec=0;
    SAMPLE *ptr;
    
    prev_loc=0;

    /* Undo the hard-coded sample handling time added in by xmtime macro */
    if (magSec >= 180)
        magSec -= 180;

    /*
     * Put this sample in the queue.
     */
    DPRINT3(0, "queue_sample: sample r%02d_z%02d_%02d queued\r\n",
            rack, zone, well);

    ptr = calloc(sizeof(SAMPLE), 1);
    if (ptr == NULL)
    {
        DPRINT(0, "queue sample: Could not allocate memory.\n");
        return 0;
    }

    ptr->rack = rack;
    ptr->zone = zone;
    ptr->well = well;
    ptr->bcrSec = bcrSec;
    ptr->pcsSec = pcsSec;
    ptr->magSec = magSec;
    ptr->next = NULL;

    if (SampleQueueTail != NULL) {
        SampleQueueTail->next = ptr;
        SampleQueueTail = ptr;
    }

    else {
        SampleQueueHead = ptr;
        SampleQueueTail = ptr;
    }

    DisplaySamples();

    /*
     * If we are in the middle of a batch, go ahead
     *  and schedule this sample.  If at the beginning
     *  of a batch run, don't schedule any samples until
     *  the first call to hermes_get_sample().
     */
    if ((slotsReserved > 0) && (slotsReserved < MAX_PCS_SLOTS))
        schedule_new_samples();

    return 0;
}

int FindOldestSample()
{
    int oldest;
    MYTIMER *ptr;

    /* Find the oldest sample still in the system */
    oldest = -1;
    ptr = timerList;
    while (ptr != NULL) {
      if ((ptr->timerType == MYTIMER_DONE) ||
          (ptr->timerType == MYTIMER_MAGNET)) {
        oldest = ptr->sampleIndex;
        break;
      }
      ptr = ptr->next;
    }

    if (oldest < 0) {
        if (timerList != NULL)
            oldest = timerList->sampleIndex;
    }

    return oldest;
}


void PushNonPCSSamplesToWaitQueue()
{
    int i, j, rack, zone, well, oldest, newest, priorSample, push;
    int prepping;

    /* If no samples are in the ready queue, just return */
    if (timerList == NULL) return;

    oldest = FindOldestSample();
    newest = (((oldest-1) < 0) ? (MAX_PCS_SLOTS-1) : (oldest-1));

    if (useGilson && (gilPreScript[0] != '\0'))
        prepping = 1;
    else
        prepping = 0;

    /* Push them back on in the order that they came off */
    for (i=0,j=newest; i<MAX_SCHED_SLOTS; i++) {
        if ( prepping &&
                (sample[j].prepStartTimer != NULL) &&
               ((sample[j].prepStartTimer)->valid) )
            push = 1;
        else if ( !prepping &&
                 (sample[j].pcsStartTimer != NULL) &&
                 ((sample[j].pcsStartTimer)->valid) )
            push = 1;
        else
            push = 0;

        if (push) {
	    delete_timer(sample[j].prepStartTimer);
            sample[j].prepStartTimer = NULL;
            DeleteSchedItem(j, "doPREP");

	    delete_timer(sample[j].pcsStartTimer);
            sample[j].pcsStartTimer = NULL;
            DeleteSchedItem(j, "toPCS");

	    delete_timer(sample[j].pcsDoneTimer);
            sample[j].pcsDoneTimer = NULL;
            DeleteSchedItem(j, "toMag");

	    delete_timer(sample[j].magnetEmptyTimer);
            sample[j].magnetEmptyTimer = NULL;
            DeleteSchedItem(j, "toExt");

            /* Need to unschedule the "toRack" item for _prior_ sample */
            priorSample = ((j == 0) ? (MAX_PCS_SLOTS-1) : (j-1));
            while (priorSample != j) {
                if ( (sample[priorSample].magnetEmptyTimer != NULL) ||
        	     ((sampleInExtractionStation != -1) &&
                      (priorSample == (sampleInExtPcsSlot-1))) )
                    break;
                priorSample = ((priorSample == 0) ?
                               (MAX_PCS_SLOTS-1) : (priorSample-1));
            }
            if (priorSample != j)
                DeleteSchedItem(priorSample, "toRack");

            FreePcsSlot(sample[j].startLoc);

            rack = (sample[j].startLoc / 1000000L);
            zone = (sample[j].startLoc / 10000L % 100);
            well = (sample[j].startLoc % 1000);
            
            prev_loc=0;
            
            push_sample(rack, zone, well, sample[j].magSec, sample[j].pcsSec);
        }

        j = (((j-1) < 0) ? (MAX_PCS_SLOTS-1) : (j-1));
    }

    DisplaySamples();
}

void record_error(int smp, int stat)
{
    int i;
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (pcs_xfer_error[i].smp < 0) {
            pcs_xfer_error[i].smp = smp;
            pcs_xfer_error[i].stat = stat;
            return;
        }
    }
    DPRINT2(0, "Roboproc: cannot record error condition (%d) "
            "for sample %d.\r\n", stat, smp);
}


int check_error(int smp, int clear)
{
    int i;
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (pcs_xfer_error[i].smp == smp) {
            if (clear) pcs_xfer_error[i].smp = -1;
            return pcs_xfer_error[i].stat;
        }
    }

    return HRM_SUCCESS;
}


/*
 * Decrement all timers by seconds passed in.
 */
void DecrementAllTimers(int seconds)
{
    MYTIMER *ptr;
    int i;
    struct schedItem *schedPtr;
    DPRINT1(0, "Roboproc: subtracting %d seconds from all timers.\r\n",
        seconds);

    /* Critical Region around adjustment of timers */
    MyAcquireSemaphore();

    for (ptr = timerList; ptr != NULL; ptr=ptr->next) {
            ptr->timeLeft -= seconds;
    }

    /* Shift scheduled items in sched map as well */
    for (schedPtr = schedMap; schedPtr != NULL; schedPtr=schedPtr->next) {
        schedPtr->start -= seconds;
        schedPtr->end -= seconds;
    }

    /* Increment PCS Timers if Needed */
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (actualPcsTimer[i].valid)
            actualPcsTimer[i].timeLeft += seconds;
    }

    /* Increment EXP Timers if Needed */
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        if (actualExpTimer[i].valid)
            actualExpTimer[i].timeLeft += seconds;
    }

    MyReleaseSemaphore();

    DisplayTimers();
    DisplaySchedule();
}


/*
 * Increment magnet empty timers only by overage seconds.
 * Increment all scheduler items by overage seconds.
 */
void PropogateOverdueSeconds(int overage)
{
    MYTIMER *ptr;
    struct schedItem *schedPtr;
    DPRINT1(0, "Roboproc: adding %d seconds to all remaining timers.\r\n",
        overage);

    /* Critical Region around adjustment of timers */
    MyAcquireSemaphore();

    for (ptr = timerList; ptr != NULL; ptr=ptr->next) {
        if (ptr->timerType == MYTIMER_DONE)
            ptr->timeLeft += overage;
    }

    /* Shift scheduled items in sched map as well */
    for (schedPtr = schedMap; schedPtr != NULL; schedPtr=schedPtr->next) {
        schedPtr->start += overage;
        schedPtr->end += overage;
    }

    MyReleaseSemaphore();

    DisplayTimers();
    DisplaySchedule();
}


/* Decimal encoding of rack:zone:well
 *
 *   rr,zzw,www
 *
 *     rr - rack position (valid values are 1 thru 4)
 *     zz - zone on rack  (valid values are 1 or 2)
 *   wwww - well in zone  (valid values are 1 thru 96)
 *
 */


int convert_robot_errorcode(int robotErr, eRobotOp robotOp)
{
    switch (robotErr) {
    case HRM_NOPOWER:
        /*
         * Check Rabbit status to see if air/magnet leg error occurred
         * If so, return SNS_ERROR+MAGNETLEG or SNS_ERROR+AIRPRESSURE
         */
        return (SMPERROR+HRM_HIGHPOWER);
    case HRM_NOTCALIBRATED:
        return (SMPERROR+HRM_CALIBRATE);
    case HRM_TABLE_AJAR:
        return (SMPERROR+HRM_MOTORS_HALTED);

    case HRM_INVALID_CMD:
    case HRM_INVALID_PARAM:
    case HRM_IO_ERROR:
        return (SMPERROR+HRM_COMMERROR);

    case HRM_TURBINE_UPB_PRESENT:
    case HRM_MISSTURB_AFTERPLACE:
    case HRM_MISSTURB_BEFOREPICK:
    case HRM_FAILED_TURBINEPICK:
    case HRM_NOGLASS_ROBOT1:
    case HRM_NOGLASS_ROBOT2:
    case HRM_ROBOT2_VIALPRESENT:
        if (robotOp == RACKTOPCS)
            return (SMPERROR+HRM_RACKTOPCS);
        else if (robotOp == PCSTOMAG)
            return (SMPERROR+HRM_PCSTOMAG);
        else
            return (SMPERROR+HRM_MAGTORACK);

    case HRM_OPERATION_ABORTED:
        return HRM_OPERATION_ABORTED;

    case HRM_SERVO_ERROR:
        /*
         * Check Rabbit status to see if air/magnet leg error occurred
         * If so, return SNS_ERROR+MAGNETLEG or SNS_ERROR+AIRPRESSURE
         */
        return SMPERROR+HRM_MOTORS_HALTED;

    case HRM_ROBOT1_CANTPARK:
    case HRM_ROBOT2_CANTPARK:
    case HRM_ROBOT2_NORETRACT:
    case HRM_ROBOT2_NOEXTEND:
    case HRM_ROBOT2_CYLUNKNOWN:
    case HRM_DATABASE_ERROR:
    case HRM_TIMEOUT_ROBOT1:
    case HRM_TIMEOUT_ROBOT2:
    case HRM_GENERIC_ERROR:
    case HRM_POSCALC_ERROR:
    case HRM_SCHEDULER_ERROR:
        return SMPERROR+HRM_ROBOTERR;

    default:
        if (robotErr > SMPERROR)
            return robotErr;
        else
            return SMPERROR+HRM_ROBOTERR;
    }
}

int GetStatus(void)
{
    int result;
    HRM_ROBOT_CMD cmd;

    memset(&cmd, 0, sizeof(cmd));
    sprintf(cmd.cmdStr, "{b|getstatus}");
    cmd.wait = 1;
    cmd.timeout = 1000;
    HrmSendRobotCmd(&cmd);

    /* Check for errors */
    if (!cmd.rspValid) {
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "getstatus %d\n", &result) != 1) {
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return SMPERROR+HRM_COMMERROR;
    }

    DPRINT1(0, "Robot Status = 0x%d\r\n", result);
    return HRM_SUCCESS;
}


int RobotStartup(int both)
{
    int result;
    HRM_ROBOT_CMD cmd;

    /* Format the startup command */
    memset(&cmd, 0, sizeof(cmd));
    if (both)
        sprintf(cmd.cmdStr, "{b|startup}");
    else
        sprintf(cmd.cmdStr, "{b|startup 1}");

    /*
     * Send 'startup' command to robot.
     * See if a quick response is returned.
     */
    cmd.wait = 1;
    cmd.timeout = 5000; /* milliseconds */;
    HrmSendRobotCmd(&cmd);

    /*
     * Timed Out.  Maybe Offline.  Maybe the Robot is Self-Calibrating.
     * Check Robot Status.
     */
    if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
        int status, count = STARTUP_CMDTIME;

        while (((status = GetStatus()) == HRM_SUCCESS) && (count > 0)) {
            delayAwhile(4); /* seconds */

            cmd.wait = 1;
            cmd.timeout = 1000; /* milliseconds */;
            HrmSendRobotCmd(&cmd);

            if ((cmd.errVal == 0) && cmd.rspValid)
                break;

            count -= 5;
        }
    }

    if (!cmd.rspValid) {
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "startup %d\n", &result) != 1) {
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return HRM_IO_ERROR; /* Return value is robot-native */
    }

    if (result != HRM_SUCCESS) {
        DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return result; /* Return value is robot-native */
    }

    return HRM_SUCCESS;
}

int EnableEStop()
{
    char rsp[32];

    if (useEStop) {
        if (SendRabbit("<EST+>\n", rsp) != 0)
            return -1;
        while (1) {
            if (strstr(rsp, "ACK") != NULL)
                return 0;
            if (SendRabbit("", rsp) != 0)
                return -1;
        }
    }

    return 0;
}

int DisableEStop()
{
    char rsp[32];

    if (SendRabbit("<EST->\n", rsp) != 0)
        return -1;

    while (1) {
        if (strstr(rsp, "ACK") != NULL)
            return 0;
        if (SendRabbit("", rsp) != 0)
            return -1;
    }
}

int EjectMagnet()
{
    int result;
    HRM_ROBOT_CMD cmd;

    if (EnableEStop() != 0)
        return HRM_IO_ERROR;

    /* Format Command: unload sample from magnet */
    memset(&cmd, 0, sizeof(cmd));
    sprintf(cmd.cmdStr, "{b|unldnmr}");

    /* Send command to robot, wait for response */
    cmd.wait = 1;
    cmd.timeout = magToExtTime; /* milliseconds */;
    HrmSendRobotCmd(&cmd);
    if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
        cmd.cmdStr[0] = '\0';
        cmd.timeout = ROBOT_GRACEPERIOD;
        HrmSendRobotCmd(&cmd);
    }

    /* Check for errors */
    if (!cmd.rspValid) {
        DisableEStop();
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "unldnmr %d\n", &result) != 1) {
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        DisableEStop();
        return HRM_IO_ERROR;
    }

    if (result != HRM_SUCCESS) {
        DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        DisableEStop();
        return result;
    }

    if (DisableEStop() != 0)
        return HRM_IO_ERROR;

    return HRM_SUCCESS;
}

int InsertMagnet(int rack, int zone, int well, int pcsPos)
{
    int result;
    char rsp[32];
    HRM_ROBOT_CMD cmd;

    /*  Make Sure Air is Off */
    if ((SendRabbit("<AIR->\n", rsp) != 0) || (strstr(rsp, "ACK") == NULL)) {
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return HRM_IO_ERROR;
    }

    /* Enable E-Stop on Magnet Leg Failure */
    if (EnableEStop() != 0) {
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return HRM_IO_ERROR;
    }

    /* Format Command: insert sample into magnet */
    memset(&cmd, 0, sizeof(cmd));
    sprintf(cmd.cmdStr, "{b|loadnmr cond_%02d}", pcsPos);

    /* Send command to robot, wait with timeout for response */
    cmd.wait = 1;
    cmd.timeout = pcsToMagTime; /* milliseconds */
    HrmSendRobotCmd(&cmd);
    if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
        cmd.cmdStr[0] = '\0';
        cmd.timeout = ROBOT_GRACEPERIOD;
        HrmSendRobotCmd(&cmd);
    }

    /* Check for errors */
    if (!cmd.rspValid) {
        DisableEStop();
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "loadnmr %d\n", &result) != 1) {
        DisableEStop();
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return HRM_IO_ERROR;  /* Return value is robot-native */
    }

    if (result != HRM_SUCCESS) {
        DisableEStop();
        DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return result;  /* Return value is robot-native */
    }

    /* Tell Rabbit that Sample has been dropped into the Magnet */
    *rsp = '\0';
    if ((SendRabbit("<INSERT>\n", rsp) != 0) ||
        (strstr(rsp, "SUCCESS") == NULL)) {

        /* Return value is robot-native */
        if (AbortRobo) {
            DisableEStop();
            return HRM_OPERATION_ABORTED;
        }
        else if (*rsp == '\0') {
            return HRM_IO_ERROR;
        }
        else {
            DisableEStop();
            return HRM_MISSTURB_AFTERPLACE;
        }
    }

    /* Disable E-Stop on Magnet Leg Failure */
    if (DisableEStop() != 0) {
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return HRM_IO_ERROR;
    }
    return HRM_SUCCESS;
}

int ReturnSampleToRack(int rack, int zone, int well, int pcsPos)
{
    int result;
    HRM_ROBOT_CMD cmd;

    /* Format Command: unload turbine */
    memset(&cmd, 0, sizeof(cmd));
    sprintf(cmd.cmdStr, "{b|unloadturbine r%d_z%d_%02d cond_%02d}",
            rack, zone, well, pcsPos);

    /* Send command to robot, wait with timeout for response */
    cmd.wait = 1;
    cmd.timeout = extToRackTime; /* milliseconds */
    HrmSendRobotCmd(&cmd);
    if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
        cmd.cmdStr[0] = '\0';
        cmd.timeout = ROBOT_GRACEPERIOD;
        HrmSendRobotCmd(&cmd);
    }

    /* Check for errors */
    if (!cmd.rspValid) {
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "unloadturbine %d\n", &result) != 1) {
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return HRM_IO_ERROR;  /* Return value is robot-native */
    }

    if (result != HRM_SUCCESS) {
        DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return result;  /* Return value is robot-native */
    }

    return HRM_SUCCESS;
}

int TransferVial(int bcrSec, int rack, int zone, int well, int pcsPos)
{
    int result;
    HRM_ROBOT_CMD cmd;
    char barcode[48];

    if (bcrSec > 0) {

        /* Format Command: send vial to barcode reader station */
        memset(&cmd, 0, sizeof(cmd));
        sprintf(cmd.cmdStr, "{b|transfervial r%d_z%d_%02d barcode}",
                rack, zone, well);

        /* Send command to robot, wait for response */
        cmd.wait = 1;
        cmd.timeout = rackToBcrTime; /* milliseconds */;
        HrmSendRobotCmd(&cmd);
        if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
            cmd.cmdStr[0] = '\0';
            cmd.timeout = ROBOT_GRACEPERIOD;
            HrmSendRobotCmd(&cmd);
        }

        /* Check for errors */
        if (!cmd.rspValid) {
            return cmd.errVal;
        }

        if (sscanf(cmd.rspStr, "transfervial %d\n", &result) != 1) {
            DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return HRM_IO_ERROR;  /* Return value is robot-native */
        }

        if (result != HRM_SUCCESS) {
            DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return result;  /* Return value is robot-native */
        }

        /* Wait up to bcrSec seconds for barcode to be read */
        ReadBarcode(barcode, sizeof(barcode), bcrSec);
        if (*barcode != '\0') {
            strncpy(sample[pcsPos-1].barcode, barcode,
                    sizeof(sample[pcsPos-1].barcode));
        }
        else {
            strcpy(sample[pcsPos-1].barcode, "<NONE>");

            DPRINT(0, "BARCODE NOT READ ERROR\r\n");
            DPRINT1(0, "Error handling option = %s\r\n", bcrErrAction);

            if (strstr(bcrErrAction, "Halt") != NULL) {
                /* Halt entire batch run */
                DPRINT(0, "Halting entire batch run.\n");
                return SMPERROR+HRM_BARCODE_FATAL;
            }
            else if (strstr(bcrErrAction, "Skip") != NULL) {
                /* Skip this sample */
                DPRINT(0, "Skipping this sample.\n");

                /* Return Sample to Well */
                memset(&cmd, 0, sizeof(cmd));
                sprintf(cmd.cmdStr, "{b|transfervial barcode r%d_z%d_%02d}",
                        rack, zone, well);

                /* Send command to robot, wait for response */
                cmd.wait = 1;
                cmd.timeout = bcrToPcsTime /* milliseconds */;
                HrmSendRobotCmd(&cmd);
                if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
                    cmd.cmdStr[0] = '\0';
                    cmd.timeout = ROBOT_GRACEPERIOD;
                    HrmSendRobotCmd(&cmd);
                }

                /* Check for errors */
                if (!cmd.rspValid) {
                    return cmd.errVal;
                }

                if (sscanf(cmd.rspStr, "transfervial %d\n", &result) != 1) {
                    DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
                    DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                            cmd.cmdStr, cmd.rspStr);
                    return HRM_IO_ERROR;  /* Return value is robot-native */
                }

                if (result != HRM_SUCCESS) {
                    DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
                    DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                            cmd.cmdStr, cmd.rspStr);
                    return result;  /* Return value is robot-native */
                }

                return SMPERROR+HRM_BARCODE;
            }
            else {
                /* Ignore Error and Continue */
                DPRINT(0, "Ignoring error.\n");
            }
        }

        /* Format Command: send vial to preconditioner station */
        memset(&cmd, 0, sizeof(cmd));
        sprintf(cmd.cmdStr, "{b|transfervial barcode cond_%02d}", pcsPos);

        /* Send command to robot, wait for response */
        cmd.wait = 1;
        cmd.timeout = bcrToPcsTime /* milliseconds */;
        HrmSendRobotCmd(&cmd);
        if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
            cmd.cmdStr[0] = '\0';
            cmd.timeout = ROBOT_GRACEPERIOD;
            HrmSendRobotCmd(&cmd);
        }

        /* Check for errors */
        if (!cmd.rspValid) {
            return cmd.errVal;
        }

        if (sscanf(cmd.rspStr, "transfervial %d\n", &result) != 1) {
            DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return HRM_IO_ERROR;  /* Return value is robot-native */
        }

        if (result != HRM_SUCCESS) {
            DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return result;  /* Return value is robot-native */
        }
    }

    else {
        /* Format Command: send vial directly to preconditioner station */
        sprintf(cmd.cmdStr, "{b|transfervial r%d_z%d_%02d cond_%02d}",
                rack, zone, well, pcsPos);

        /* Send command to robot, wait for response */
        cmd.wait = 1;
        cmd.timeout = rackToPcsTime /* milliseconds */;
        HrmSendRobotCmd(&cmd);
        if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
            cmd.cmdStr[0] = '\0';
            cmd.timeout = ROBOT_GRACEPERIOD;
            HrmSendRobotCmd(&cmd);
        }

        /* Check for errors */
        if (!cmd.rspValid) {
            return cmd.errVal;
        }

        if (sscanf(cmd.rspStr, "transfervial %d\n", &result) != 1) {
            DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return HRM_IO_ERROR;  /* Return value is robot-native */
        }

        if (result != HRM_SUCCESS) {
            DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
            DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                    cmd.cmdStr, cmd.rspStr);
            return result;  /* Return value is robot-native */
        }
    }

    return HRM_SUCCESS;
}

int EjectSample(int smp)
{
    int i, stat;

    /* Search for this Sample in the Queue */
    if ((i = GetAssignedPcsSlot(smp)) != -1) {

        /* Found this sample, first clear the extraction station */
        if ((stat = hermes_clear_extraction_station()) != HRM_SUCCESS) {
            DPRINT1(0, "EjectSample error: %d\r\n", stat);
            return stat;  /* Return value is vnmr-style */
        }

        /* Now we can eject the sample from the magnet */
        // rack = (smp / 1000000L);
        // zone = (smp / 10000L % 100);
        // well = (smp % 1000);

        if ((stat = EjectMagnet()) != HRM_SUCCESS) {
            DPRINT1(0, "EjectSample error: %d.\r\n", stat);
            return convert_robot_errorcode(stat, MAGTORACK); /* vnmr-style */
        }

        sampleInExtractionStation = smp;
        sampleInExtPcsSlot = sampleInMagPcsSlot;
        sampleInMagnet = -1;
        sampleInMagPcsSlot = -1;
        return HRM_SUCCESS;
    }

    DPRINT1(0, "EjectSample error: unable to identify PCS slot for sample "
               "in maagnet (%d)\r\n", sampleInMagnet);
    return convert_robot_errorcode(stat, MAGTORACK); /* vnmr-style */
}

int RejectUnusedSample(int pcsPos, int loc)
{
    int result;
    char rsp[32];
    HRM_ROBOT_CMD cmd;

    /*
     * Return sample by way of magnet, but do not insert into magnet.
     * So make sure air is ON.
     */
    if ((SendRabbit("<AIR+>\n", rsp) != 0) || (strstr(rsp, "ACK") == NULL)) {
        /* vnmr-style */
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return convert_robot_errorcode(HRM_IO_ERROR, PCSTOMAG);
    }

    /* Enable E-Stop on Magnet Leg Failure */
    if (EnableEStop() != 0) {
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return (SMPERROR+HRM_COMMERROR);  /* vnmr-style */
    }

    /* Format Command: insert sample into magnet */
    memset(&cmd, 0, sizeof(cmd));
    sprintf(cmd.cmdStr, "{b|loadnmr cond_%02d}", pcsPos);

    /* Send command to robot, wait with timeout for response */
    cmd.wait = 1;
    cmd.timeout = pcsToMagTime; /* milliseconds */
    HrmSendRobotCmd(&cmd);
    if (cmd.errVal == (SMPERROR+SMPTIMEOUT)) {
        cmd.cmdStr[0] = '\0';
        cmd.timeout = ROBOT_GRACEPERIOD;
        HrmSendRobotCmd(&cmd);
    }

    /* Check for errors */
    if (!cmd.rspValid) {
        DisableEStop();
        return cmd.errVal;
    }

    if (sscanf(cmd.rspStr, "loadnmr %d\n", &result) != 1) {
        DisableEStop();
        DPRINT(0,  "!!!!>ROBOT PARSING ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return (SMPERROR+HRM_COMMERROR);  /* vnmr-style */
    }

    if (result != HRM_SUCCESS) {
        DisableEStop();
        DPRINT(0,  "!!!!>ROBOT RETURNED ERROR:\r\n");
        DPRINT2(0, "    >cmd sent: %s, rsp received: %s\r\n",
                cmd.cmdStr, cmd.rspStr);
        return convert_robot_errorcode(result, PCSTOMAG); /* vnmr-style */
    }

    sampleInMagnet = loc;
    sampleInMagPcsSlot = pcsPos;

    /* Now Eject the Sample Immediately */
    if ((result = EjectSample(loc)) != HRM_SUCCESS) {
        DisableEStop();
        return result; /* vnmr-style */
    }

    /* Turn the Air Back OFF */
    if ((SendRabbit("<AIR->\n", rsp) != 0) || (strstr(rsp, "ACK") == NULL)) {
        /* vnmr-style */
        if (AbortRobo) {
            DisableEStop();
            return HRM_OPERATION_ABORTED;
        }
        else
            return convert_robot_errorcode(HRM_IO_ERROR, MAGTORACK);
    }

    /* Disable E-Stop on Magnet Leg Failure */
    if (DisableEStop() != 0) {
        if (AbortRobo)
            return HRM_OPERATION_ABORTED;
        else
            return (SMPERROR+HRM_COMMERROR);  /* vnmr-style */
    }

    return HRM_SUCCESS;
}

void read_settings()
{
    int usePCS;
    char *sysFname = getenv("vnmrsystem");

    rackToBcrTime = RACKTOBCR_TIME;
    bcrToPcsTime = BCRTOPCS_TIME;
    rackToPcsTime = RACKTOPCS_TIME;
    pcsToMagTime = PCSTOMAG_TIME;
    magToExtTime = MAGTOEXT_TIME;
    extToRackTime = EXTTORACK_TIME;
    insertTime = INSERT_TIME;
    ejectTime = EJECT_TIME;
    preconditionTime = 0;

    if (sysFname == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
    }

    else {
        FILE *setupFile, *configFile;
        char fname[255], textline[128];
        int tmp1, tmp2, tmp3;

        /* Read setupInfo File for Robot Times */
        strcpy(fname, sysFname);
        strcpy(fname+strlen(fname), "/asm/info/setupInfo");

        if ((setupFile = fopen(fname, "r")) == NULL) {
            DPRINT1(0, "\r\nError opening 768AS setup file %s\r\n",
                    fname);
        }
        else {
            DPRINT1(0, "Reading setup file %s.\r\n", fname);

            /* Read Robot Timing Parameters from Setup File */
            if ((fscanf(setupFile, "%[^\n]\n", textline) != 1) ||
                (fscanf(setupFile, "Rack to Barcode Reader:           %d\r\n",
                        &rackToBcrTime) != 1) ||
                (fscanf(setupFile, "Barcode Reader to Preconditioner: %d\r\n",
                        &bcrToPcsTime) != 1) ||
                (fscanf(setupFile, "Rack to Preconditioner:           %d\r\n",
                        &rackToPcsTime) != 1) ||
                (fscanf(setupFile, "Preconditioner to Magnet:         %d\r\n",
                        &pcsToMagTime) != 1) ||
                (fscanf(setupFile, "Magnet to Extraction Station:     %d\r\n",
                        &magToExtTime) != 1) ||
                (fscanf(setupFile, "Extraction Station to Rack:       %d\r\n",
                        &extToRackTime) != 1) ||
                (fscanf(setupFile, "Magnet Insert Time:               %d\r\n",
                        &insertTime) != 1) ||
                (fscanf(setupFile, "Magnet Eject Time:                %d\r\n",
                        &ejectTime) != 1) ||
                (fscanf(setupFile, "Control E-Stop: %d\r\n",
                        &useEStop) != 1)) {
                DPRINT3(0, "\r\nError reading 768AS setup file %s %d %s\r\n",
                        fname, errno, strerror(errno));
            }

            fclose(setupFile);
        }

        /* Read configInfo File for Barcode/Preconditioner/Gilson Settings */
        strcpy(fname, sysFname);
        strcpy(fname+strlen(fname), "/asm/info/configInfo");

        if ((configFile = fopen(fname, "r")) == NULL) {
            DPRINT1(0, "\r\nError opening 768AS configuration file %s\r\n",
                    fname);
        }
        else {
            int res __attribute__((unused));
            DPRINT1(0, "Reading config file %s.\r\n", fname);
            gilPreScript[0] = gilPostScript[0] = '\0';
            /* Read Barcode/PCS Parameters from Configuration File */
            if ((fscanf(configFile,"Use Barcodes: %d\r\n", &tmp1) == 1) &&
                (fscanf(configFile,"Barcode Error Action: %[^\n]\n",
                        bcrErrAction) == 1) &&
                (fscanf(configFile,"Use Preconditioner: %d\r\n",&usePCS)==1)&&
                (fscanf(configFile,"PCS Time: %d\r\n",&preconditionTime)==1)&&
                (fscanf(configFile,"Override Exp Time: %d\r\n", &tmp2)==1) &&
                (fscanf(configFile,"Exp Time: %d\r\n", &expTime)==1) &&
                (fscanf(configFile,"Use Liquid Handler: %d\r\n",&tmp3)==1)) {

                res = fscanf(configFile,"Pre-Experiment Liquids Script: %[^\n]\n",
                    gilPreScript);
                if (strncmp(gilPreScript, "Pre-Script Execution Time:",
			    strlen("Pre-Script Execution Time:")) == 0) {
                    res = sscanf(gilPreScript, "Pre-Script Execution Time: %d\n",
                           &gilPreTime);
		    gilPreScript[0] = '\0';
                }
                else
                    res = fscanf(configFile,"Pre-Script Execution Time: %d\n",
                           &gilPreTime);

                res = fscanf(configFile,"Post-Experiment Liquids Script: %[^\n]\n",
                    gilPostScript);
                if (strncmp(gilPostScript, "Post-Script Execution Time:",
			    strlen("Post-Script Execution Time:")) == 0) {
                    res = sscanf(gilPostScript, "Post-Script Execution Time: %d\n",
                           &gilPostTime);
		    gilPostScript[0] = '\0';
                }
                else
                    res = fscanf(configFile,"Post-Script Execution Time: %d\n",
                           &gilPostTime);

                barcodeUse = tmp1;
                if (!barcodeUse)
                {
                    DPRINT(0, "NOT using barcodes.\r\n");
                }
                else
                {
                    DPRINT2(0, "Using barcodes. Timeout=%d seconds. "
                               "On read error: %s\r\n",
                                barcodeTimeout, bcrErrAction);
                }
                overrideExpTime = tmp2;
                if (!overrideExpTime)
                {
                    DPRINT(0, "Using experiment times provided by Vnmr.\r\n");
                }
                else
                {
                    DPRINT1(0, "Overriding Vnmr provided experiment time "
                               "to be %d seconds for ALL samples.\r\n",
                                expTime);
                }
                useGilson = tmp3;
                if (!useGilson) {
                    DPRINT(0, "NOT using liquid handler.\r\n");
                    gilPreTime = 0;  gilPreScript[0] = '\0';
                    gilPostTime = 0; gilPostScript[0] = '\0';
                }
                else {
                    DPRINT(0, "Using liquid handler:\r\n");
                    DPRINT2(0, "  Pre-experiment script:  %s, %d seconds\r\n",
                        gilPreScript, gilPreTime);
                    DPRINT2(0, "  Post-experiment script: %s, %d seconds\r\n",
                        gilPostScript, gilPostTime);
                }
            }
            else {
                DPRINT3(0, "\r\nError reading 768AS config "
                        "file %s %d %s\r\n", fname, errno, strerror(errno));
            }

            fclose(configFile);
        }
    }

    if (usePCS == 0) {
        preconditionTime = 0;
        DPRINT(0, "Preconditioner NOT Used\r\n");
    }
    else {
        DPRINT1(0, "Preconditioner Time: %d seconds.\r\n\r\n",
                preconditionTime);
    }
    DPRINT(0,  "Robot Timing Parameters:\r\n");
    DPRINT1(0, "  Rack to Barcode Reader:           %d\r\n", rackToBcrTime);
    DPRINT1(0, "  Barcode Reader to Preconditioner: %d\r\n", bcrToPcsTime);
    DPRINT1(0, "  Rack to Preconditioner:           %d\r\n", rackToPcsTime);
    DPRINT1(0, "  Preconditioner to Magnet:         %d\r\n", pcsToMagTime);
    DPRINT1(0, "  Magnet to Extraction Station:     %d\r\n", magToExtTime);
    DPRINT1(0, "  Extraction Station to Rack:       %d\r\n", extToRackTime);
    DPRINT1(0, "  Magnet Insert Time:               %d\r\n", insertTime);
    DPRINT1(0, "  Magnet Eject Time:                %d\r\n", ejectTime);

    /* Store Times in Milliseconds */
    rackToBcrTime *= 1000;
    bcrToPcsTime *= 1000;
    rackToPcsTime *= 1000;
    pcsToMagTime *= 1000;
    magToExtTime *= 1000;
    extToRackTime *= 1000;
    insertTime *= 1000;
    ejectTime *= 1000;

    DPRINT1(0, "E-Stop Control on Magnet Leg Failure: %s\r\n",
        (useEStop ? "Enabled" : "Disabled"));

    readSettings = 0;
}

extern int abortActive;  /* if abort is active */

void init_scheduler()
{
    int i, result;

    abortActive = 0;

    /* Read configuration settings */
    read_settings();

    /* Start One-Second Interval Timer */
    start_timer();

    /* Open control connection to Rabbit box */
    if (sockRABBIT > 0) {
        close_lan(sockRABBIT);
    }

    DPRINT(0, "Opening LAN connection for rabbit control port\r\n");
    sockRABBIT = open_lan("HRM_RABBIT");
    if (sockRABBIT < 0) {
        sockRABBIT = open_lan("HRM_RABBIT");
        if (sockRABBIT < 0) {
            DPRINT(0, "Error trying to connect to rabbit control "
                      "port\r\n");
        }
    }

    if (sockRABBIT >= 0)
        DPRINT(0, "Connection succeeded\r\n");

    if (barcodeUse) {
        if (sockBCR > 0) {
            close_lan(sockBCR);
        }
        DPRINT(0, "Opening LAN connection to barcode reader\r\n");
        sockBCR = open_lan("HRM_BARCODE");
        if (sockBCR < 0) {
            sockBCR = open_lan("HRM_BARCODE");
            if (sockBCR < 0) {
                DPRINT(0, "Error trying to connect to barcode "
                          "reader\r\n");
            }
        }
        if (sockBCR >= 0) {
            DPRINT(0, "Connection succeeded\r\n");
        }
    }

    /* Zero schedule map */
    FreeSchedMap();

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        pcsSlotPopulated[i] = -1;
        pcs_xfer_error[i].smp = -1;
    }
    for (i=0; i<MAX_PCS_SLOTS; i++) {
        sample[i].pcsStartTimer = NULL;
        sample[i].pcsDoneTimer = NULL;
        sample[i].magnetEmptyTimer = NULL;
    }

    /* Make sure the startup command has been sent */
    if ((result = RobotStartup(1)) != HRM_SUCCESS) {

        DPRINT1(0, "Roboproc encountered error %d while sending "
                "startup command to robot.  May need to power-"
                "cycle the robot and clear the system manually "
                "to recover.\r\n", result);

        robotError = convert_robot_errorcode(result, NOTRANSPORT);
        return;
    }

    scheduler_initialized = 1;

    DPRINT(0, "Roboproc: 768AS scheduler initialized.\r\n");
}


void findNextGap(int startTime, int *gapStart, int *gapSize)
{
    struct schedItem *lPtr, *tPtr = schedMap;
    int gap;

    if (tPtr == NULL) {
        *gapStart = startTime;
        *gapSize = -1;
        return;
    }

    if (tPtr->start > startTime) {
        *gapStart = startTime;
        *gapSize = (tPtr->start - startTime);
        return;
    }

    lPtr = tPtr;
    tPtr = tPtr->next;
    while (tPtr != NULL) {
        if (tPtr->start > startTime) {
            gap = tPtr->start - lPtr->end;
            if (gap > 0) {
                if (lPtr->end < startTime) {
                    /* start time is in the middle of this gap */
                    gap -= (startTime - lPtr->end);
                    if (gap > 0) {
                        *gapStart = startTime;;
                        *gapSize = gap;
                        return;
                    }
                }
                else {
                    *gapStart = lPtr->end;
                    *gapSize = gap;
                    return;
                }
            }
        }

        lPtr = tPtr;
        tPtr = tPtr->next;
    }

    /* Reached the end of the road... just tack it on at the end */
    if (lPtr->end > startTime)
        *gapStart = lPtr->end;
    else
        *gapStart = startTime;
    *gapSize = -1;
}


int schedule_item(int startTime, int len, int slot, int smp, char *desc)
{
    int actualTime = startTime;
    struct schedItem *tPtr, *lPtr, *nPtr;

    if (len == 0)
        return startTime;

    /* Check this time for conflicts with other operations */
    lPtr=NULL;
    tPtr=schedMap;
    while (tPtr != NULL) {

        /* Advance to the desired slot in the schedule */
        if (tPtr->start > actualTime) {
            int endpt;
              
            if (lPtr) {
                /* Don't schedule it *earlier* than requested */
                endpt = ((lPtr->end < startTime) ? startTime : lPtr->end);

                /* Will this Operation "Fit"? */
                if ((tPtr->start - endpt) >= len) {
                    /* Found a Slot! */
                    actualTime = endpt;
                    break;
                }
            }

            else {
                /* Will this Operation "Fit"? */
                if ((tPtr->start - actualTime) >= len) {
                    /* Found a Slot! */
                    break;
                }
            }

            /* Doesn't fit, try the next opening */
            actualTime = tPtr->end;
        }

        lPtr = tPtr;
        tPtr = tPtr->next;
        if ((tPtr == NULL) && (lPtr->end >= actualTime))
            actualTime = lPtr->end;
    }

    /* Okay, found a slot (or we're at the end) - insert operation */
    nPtr = freeSchedMap;
    freeSchedMap = freeSchedMap->next;

    nPtr->start = actualTime;
    nPtr->end = actualTime + len;
    nPtr->slot = slot;
    nPtr->smp = smp;
    strncpy(nPtr->desc, desc, 12);
    nPtr->desc[11] = '\0';

    /* Is this the first and/or only one in the list? */
    if (lPtr == NULL)
        schedMap = nPtr;
    else
        lPtr->next = nPtr;

    /* Is this the last one in the list? */
    if (tPtr == NULL)
        nPtr->next = NULL;
    else
        nPtr->next = tPtr;

    return actualTime;
}


void run_scheduler(int slot, int rack, int zone, int well,
                   int magSec, int pcsSec)
{
    int bcrSec, startLoc, endLoc, minMagTime;
    int magnetEmpty, priorSample;
    int xferTime, xferLen, doneTime, doneLen, clrTime, clrLen;
    int prepTime=0, prepLen=0, postLen=0;
    int found=0, size=0, start=-1;
    int done=0, doneLeft=0, donePrep=0;
    int gapLeftNeeded, gapRightNeeded, gapPrepNeeded, gapMargin;
    int leftStart, rightStart=0, prepStart=0;

    if (!scheduler_initialized)
        init_scheduler();

    /*
     * For now, we get the preconditioner time and the
     *  barcode parameters from the setup/config Info files.
     *
     * Maybe later, there will be a way to use the experiment
     *  parameters so that it can change for each sample.
     *
     * In any case, the infrastructure is there for
     *   passing this value in on a per-experiment basis.
     */
    pcsSec = preconditionTime;

    if (barcodeUse)
        bcrSec = barcodeTimeout;
    else
        bcrSec = 0;

    if (overrideExpTime && (expTime > 0))
        magSec = expTime;

    if (useGilson) {
        strcpy(sample[slot].gilPreScript, gilPreScript);
        sample[slot].gilPreTime = gilPreTime;
        strcpy(sample[slot].gilPostScript, gilPostScript);
        sample[slot].gilPostTime = gilPostTime;
    }
    else {
        sample[slot].gilPreTime = 0;
        sample[slot].gilPreScript[0] = '\0';
        sample[slot].gilPostTime = 0;
        sample[slot].gilPostScript[0] = '\0';
    }

    startLoc = (rack * 1000000L) +
               (zone * 10000L) +
               (well);
    endLoc = startLoc;

    /*
     * Enforce the minimum magnet time:
     *   = extToRackTime + gilPostTime
     *
     * This is to make sure the robot has time to return the last sample
     * to the rack while the acquisition is going on.
     *
     */
    minMagTime = 0;

    /*
     * Undecided on this:
     * Do we want to increase the minimum magnet time such
     *   that sample prep can also be done during NMR?
     * We'll try it.
     */
    if (useGilson && (sample[slot].gilPreScript[0] != '\0'))
        minMagTime += sample[slot].gilPreTime;

    if (slotsReserved > 1) {
        minMagTime += extToRackTime/1000;
        if (useGilson && (sample[slot].gilPostScript[0] != '\0'))
            minMagTime += sample[slot].gilPostTime;
    }

    if (magSec < minMagTime) {
        DPRINT3(0,"Roboproc increasing experiment time from %d to %d "
                "for sample %d.\r\n", magSec, minMagTime, startLoc);
        magSec = minMagTime;
    }

    sample[slot].startLoc = startLoc;
    sample[slot].endLoc = endLoc;
    sample[slot].bcrSec = bcrSec;
    sample[slot].magSec = magSec;
    sample[slot].pcsSec = pcsSec;
    sample[slot].pcsStartTimer = NULL;
    sample[slot].pcsDoneTimer = NULL;
    sample[slot].magnetEmptyTimer = NULL;
    strcpy(sample[slot].barcode, "<UNREAD>");

    if (slotsReserved == 1)
        priorSample = -1;
    else {
        priorSample = ((slot == 0) ? (MAX_PCS_SLOTS-1) : (slot-1));
        while (priorSample != slot) {
            if (sample[priorSample].magnetEmptyTimer != NULL)
                break;
            priorSample = ((priorSample == 0) ?
                           (MAX_PCS_SLOTS-1) : (priorSample-1));
        }
        if (priorSample == slot)
            priorSample = -1;
    }

    /* Estimate when the prior sample will be free of the magnet */
    if ((priorSample < 0) ||
        (sample[priorSample].magnetEmptyTimer == NULL))
        magnetEmpty = 0;
    else {
        magnetEmpty = get_timer(sample[priorSample].magnetEmptyTimer);
        DPRINT3(0, "\nPrior Sample (%d, slot %d) will be free of magnet "
                   "in %d s\n",
                sample[priorSample].startLoc, priorSample+1, magnetEmpty);
    }

    /* ---------------------------------------------------------------
     * SAMPLE SCHEDULING
     * Here's a timeline illustrating the passage of a sample
     * through the system.  Note that the exact time that Vnmr
     * will request us to load the sample is predicted based
     * on the experiment parameters for this sample as well as
     * the samples queued prior to this one.
     *
     *
     *                     __PCS_TIME__       ___EXP_TIME__
     *                    |            |     |             |
     *       ______   ____              ____  ___           ____
     *      |sample| |xfer|            |xfer||clr|         |retn|
     *  ----| prep |-|to  |------------| to ||ext|---------| to |----
     *      | time | |PCS |            |mag ||sta|         |rack|
     *       ------   ----              ----  ---           ----
     *
     *      |_______________________________||_________________|
     *                  RIGID                     FLEXIBLE
     *
     * The tasks that perform the sample prep, then load the sample into
     * the preconditioner, and unload it as soon as the preconditioning
     * is complete must be rigidly scheduled so that a sample isn't left
     * in the preconditioner too long and also to ensure that a sample
     * is prepared just prior to being moved into the preconditioner.
     * However, it is okay to leave a sample in the magnet longer than
     * optimal if it eases the scheduling of other samples.
     *
     */

    /* ------------------ PREP START TIME ------------------------
     * Work backwards to estimate when we should start
     *   prepping this sample.
     * This is the optimum time - sample shouldn't be started
     *   any earlier than this, but if necessary it can
     *   be started later.
     */
    prepTime = magnetEmpty -
            (
             ((bcrSec == 0) ?
               (rackToPcsTime / 1000) : 
               (bcrSec + ((rackToBcrTime + bcrToPcsTime) / 1000))) +
             pcsSec +
             gilPreTime
            );

    /* Start Preparing Sample ASAP */
    if ((slotsReserved == 1) || (prepTime <= 0))
        prepTime = 0;
    prepLen = gilPreTime;

    /* ---------------------- PCS XFER TIME --------------------------
     * Calculate precondition transfer time and duration.
     */
    xferTime = prepTime+prepLen;
    if (bcrSec == 0)
        xferLen = (rackToPcsTime / 1000);
    else
        xferLen = (bcrSec + ((rackToBcrTime + bcrToPcsTime) / 1000));

    /*
     * Now, calculate durations of tasks required to get sample out
     * of the preconditioner on time.
     */
    doneTime = prepTime + prepLen + xferLen + pcsSec;
    doneLen = (pcsToMagTime / 1000) + (insertTime / 1000);

    /*
     * If there are other samples ahead of us, we will be returning
     *   a sample from the extraction station back to the rack.
     */
    if ( (slotsReserved > 1) ||
         ((slotsReserved <= 0) && (sampleInExtractionStation != -1)) ) {

        clrTime = doneTime + doneLen;
        clrLen = (extToRackTime / 1000);

        /*
         * There is a sample ahead of us.  Once it's been returned
         *  to the rack, it may need to be post-processed by
         *  the liquid handler.
         */
        postLen = gilPostTime;
    }
    else {
        clrTime = doneTime + doneLen;
        clrLen = 0;
        postLen = 0;
    }

    DPRINT1(0, "Scheduling Sample %d\r\n", startLoc);

    /* Now find a place where this entire process can fit in the timeline */
    /* gapPrep: time needed to do just the sample prep */
    gapPrepNeeded = prepLen;

    /* If no PCS, left gap needs to move sample all the way to the magnet */
    if (pcsSec == 0) {
        /* gapLeft = time needed for sample pcs load-in and load-out */
        gapLeftNeeded = xferLen + doneLen;
        /* gapRight - no more rigid times */
        gapRightNeeded = 0;
    }
    /* Otherwise, two rigid gaps need to be scheduled */
    else {
        gapLeftNeeded = xferLen; /* pcs load-in */
        gapRightNeeded= doneLen; /* pcs load-out */
    }

    if (prepLen == 0)
        donePrep = 1;

    DPRINT3(0,"gapPrep=%d, gapLeft=%d, gapRight=%d\n",
        gapPrepNeeded, gapLeftNeeded, gapRightNeeded);

    start = ((prepTime <= 0) ? -1 : prepTime);
    while (!done) {

        while (!donePrep) {

            /* Find some idle time */
            findNextGap(start, &found, &size);
            DPRINT3(0,"Starting at %d, found gap at %d, size %d\n",
                start, found, size);

            /* Can this gap fit just the sample prep op? */
            if ( (size >= gapPrepNeeded) || (size < 0) ) {

                prepStart = ((found < 0) ? 0 : found);
                size = gapPrepNeeded;
                donePrep = 1;

                start = found+size;
                if (xferTime > start)
                    start = xferTime;
            }

            /* Too small - keep looking */
            else
                start = found+size;
        }

        while (!doneLeft) {

            /* Find some idle time */
            findNextGap(start, &found, &size);
            DPRINT3(0,"Starting at %d, found gap at %d, size %d\n",
                    start, found, size);

            /* Is this gap big enough? */
            if ( (size >= gapLeftNeeded) || (size < 0) ) {

                leftStart = ((found < 0) ? 0 : found);
                size = gapLeftNeeded;
                doneLeft = 1;
            }

            /* Too small - keep looking */
            else
                start = found+size;
        }

        if (gapRightNeeded > 0) {
            /*
             * There must be a large enough gap to the right
             *  of the PCS time to fit the PCS load-out operation(s)
             *  for this to work.
             */
            start = leftStart + gapLeftNeeded + pcsSec;
            /*
             * gapMargin = the amount that gapLeft's start time
             *   can be delayed and still fit within its gap.
             *   We can shift gapLeft's activities start time by
             *   up to this amount if it means fitting in gapRight.
             */
            gapMargin = size - gapLeftNeeded;
            DPRINT1(0,"gapMargin = %d\n", gapMargin);

            findNextGap(start, &found, &size);
            DPRINT4(0,"Starting at %d, found gap at %d, size %d, need %d\n",
                    start, found, size, gapRightNeeded);

            if ( ((size >= gapRightNeeded) || (size < 0)) &&
                 ((found-start) <= gapMargin) )  {

                leftStart = leftStart + (found-start);
                rightStart = ((found < 0) ? 0 : found);
                done = 1;
            }
 
           /*
             * No Gap, Start Over
             */
            else {
                start = found;
                doneLeft = 0;
                continue;
            }
        }

        /* Schedule the tasks */
        if (gapPrepNeeded > 0) {
            prepTime = schedule_item(prepStart, gapPrepNeeded, slot,
                                     startLoc, "doPREP");
            DPRINT3(0, "Scheduled \"doPREP\" for slot %d at %d, len=%d\n",
                    slot+1, prepTime, gapPrepNeeded);
        }

        if (pcsSec == 0) {
            xferTime = schedule_item(leftStart, xferLen, slot,
                                     startLoc, "toPCS");
            doneTime = schedule_item(xferTime+xferLen, doneLen, slot,
                                     startLoc, "toMag");
        }
        else {
            xferTime = schedule_item(leftStart, xferLen, slot,
                                     startLoc, "toPCS");
            doneTime = schedule_item(rightStart, doneLen, slot,
                                     startLoc, "toMag");
        }

        DPRINT3(0, "Scheduled \"toPCS\" for slot %d at %d, len=%d\n",
                slot+1, xferTime, xferLen);
        DPRINT3(0, "Scheduled \"toMAG\" for slot %d at %d, len=%d\n",
            slot+1, doneTime, doneLen);

        if (prepLen > 0)
            sample[slot].prepStartTimer = set_timer(slot, MYTIMER_PREP,
                prepTime);

        sample[slot].pcsStartTimer = set_timer(slot, MYTIMER_PCS, xferTime);
        sample[slot].pcsDoneTimer = set_timer(slot, MYTIMER_MAGNET, doneTime);

        clrTime = doneTime + doneLen;


        done = 1;
    }

    /* ------------------- CLEAR EXTRACTION STATION --------------------
     *
     * Even though prior sample is free of the magnet,
     *   it may still be in extraction station
     * Exp time in the magnet overlaps with clearing ext station.
     */
    if ((priorSample < 0) && (sampleInExtractionStation != -1)) {
        priorSample = sampleInExtPcsSlot-1;
    }
    if (priorSample != -1) {
        clrTime = schedule_item(clrTime, clrLen+postLen, priorSample,
                                sample[priorSample].startLoc, "toRack");
        DPRINT3(0, "Scheduled \"toRack\" for slot %d at %d, len=%d\n",
            slot+1, doneTime, doneLen);
    }

    /* ----------------------- MAGNET EMPTY TIMER ----------------------
     * Set up timer for when this sample should be done with the magnet.
     * Do not empty the magnet until the prior sample is completely done.
     * This includes removing it from the extraction station as well as
     *   doing any post-processing of the sample in the liquid handler.
     */
    if ((clrLen+postLen) > magSec)
        xferTime = clrTime + clrLen + postLen;
    else
        xferTime = clrTime + magSec;

    xferLen = (ejectTime / 1000) + (magToExtTime / 1000);

    /* Shift start time if it conflicts with another operation */
    xferTime = schedule_item(xferTime, xferLen, slot, startLoc, "toExt");
    DPRINT3(0, "Scheduled \"toExt\" for slot %d at %d, len=%d\n",
        slot+1, xferTime, xferLen);

    sample[slot].magnetEmptyTimer = set_timer(
        slot, MYTIMER_DONE,
        xferTime + xferLen);

    DisplayTimers();
    DisplaySchedule();

}

void ClearCompletionList()
{
    FILE *cmpFile;
    char fname[255], *syspath= getenv("vnmrsystem");

    if (syspath == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
        return;
    }
    strcpy(fname, syspath);
    strcat(fname, "/asm/info/cmpList");

    if (clearCompletionList) {
        if ((cmpFile = fopen(fname, "w")) == NULL) {
            DPRINT1(0, "\r\nError clearing 768AS completion list %s\r\n",
                fname);
        }
        else {
            clearCompletionList = 0;
        }

        fclose(cmpFile);
        return;
    }
}


void UpdateCompletionList()
{
    FILE *cmpFile;
    char fname[255], *syspath= getenv("vnmrsystem");

    if (syspath == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
        return;
    }
    strcpy(fname, syspath);
    strcat(fname, "/asm/info/cmpList");

    if (clearCompletionList)
        ClearCompletionList();

    if ((cmpFile = fopen(fname, "a")) == NULL) {
        DPRINT1(0, "\r\nError opening 768AS completion list %s\r\n",
            fname);
        return;
    }

    fprintf(cmpFile,
        "  %s    %s     %s     %s    %s       %s    %s       %s\n",
        currentSampleStatus.tmCmp,
        currentSampleStatus.smpLoc,
        currentSampleStatus.barcode,
        currentSampleStatus.pcsTgt,
        currentSampleStatus.pcsAct,
        currentSampleStatus.expTgt,
        currentSampleStatus.expAct,
        currentSampleStatus.status);
    fclose(cmpFile);

    DPRINT8(0, "Update Completion List: "
        "  %s    %s     %s     %s    %s       %s    %s       %s\n",
        currentSampleStatus.tmCmp,
        currentSampleStatus.smpLoc,
        currentSampleStatus.barcode,
        currentSampleStatus.pcsTgt,
        currentSampleStatus.pcsAct,
        currentSampleStatus.expTgt,
        currentSampleStatus.expAct,
        currentSampleStatus.status);
}

void UpdateTurbineStatus()
{
    FILE *statFile;
    char fname[255], *syspath= getenv("vnmrsystem");
    char tbuf1[48], tbuf2[48], statBuf[64];
    time_t t;
    int i, smp;

    /* First Line Contains Magnet Status */
    if (sampleInMagPcsSlot != -1) {
        t = time(0) +
            (sample[sampleInMagPcsSlot-1].magnetEmptyTimer)->timeLeft;
        strftime(tbuf1, 48, "%T", localtime( &t ) );
        sprintf(statBuf, "%08d %d %s %s\n",
            sampleInMagnet,
            sample[sampleInMagPcsSlot-1].pcsSec,
            sample[sampleInMagPcsSlot-1].barcode,
            tbuf1);
    }
    else {
      sprintf(statBuf, "00000000 EMPTY\n");
    }

    if (syspath == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
        return;
    }
    strcpy(fname, syspath);
    strcat(fname, "/asm/info/magStat");
    if ((statFile = fopen(fname, "w")) == NULL) {
        DPRINT1(0, "\r\nError opening 768AS status file %s\r\n", fname);
        return;
    }

    fprintf(statFile, "%s",  statBuf);
    fclose(statFile);

    /* Now Write Each Turbine's Individual Status */
    strcpy(fname, syspath);
    strcat(fname, "/asm/info/turbStat");
    if ((statFile = fopen(fname, "w")) == NULL) {
        DPRINT1(0, "\r\nError opening 768AS status file %s\r\n", fname);
        return;
    }

    /* First Entry Should Be the Oldest */
    smp = FindOldestSample();
    if (smp < 0)
        smp = 0;
    else {
        while ((smp == (sampleInMagPcsSlot-1)) ||
               (smp == (sampleInExtPcsSlot-1))) {
            smp = (smp+1) % MAX_PCS_SLOTS;
        }
    }

    for (i=0; i<MAX_PCS_SLOTS; i++) {
        statBuf[0] = '\0';
        if (smp == (sampleInMagPcsSlot-1)) {
            sprintf(statBuf+strlen(statBuf),
                "%d %08d IN_MAGNET\n", smp+1, sampleInMagnet);
        }
        else if (smp == (sampleInExtPcsSlot-1)) {
            sprintf(statBuf+strlen(statBuf),
                "%d %08d IN_EXTRACTION\n", smp+1, sampleInExtractionStation);
        }
        else if (sample[smp].pcsDoneTimer == NULL) {
            sprintf(statBuf+strlen(statBuf),
                "%d 00000000 EMPTY\n", smp+1);
        }
        else {
            t = time(0) + (sample[smp].pcsDoneTimer)->timeLeft;
            strftime(tbuf1, 48, "%T", localtime( &t ) );
            t = time(0) + (sample[smp].magnetEmptyTimer)->timeLeft;
            strftime(tbuf2, 48, "%T", localtime( &t ) );
            sprintf(statBuf+strlen(statBuf),
                    "%d %08d %d %s %s %s\n", smp+1,
                    sample[smp].startLoc,
                    sample[smp].pcsSec,
                    sample[smp].barcode,
                    tbuf1, tbuf2);
        }

        fprintf(statFile, "%s",  statBuf);
        smp = (smp+1) % MAX_PCS_SLOTS;
    }

    fclose(statFile);
    return;
}


void WriteCompletionValues(char *tmCmp, int smpLoc, char *barcode,
                           int pcsTgt, int pcsAct,
                           int expTgt, int expAct, char *status)
{
    if (tmCmp == NULL)
        sprintf(currentSampleStatus.tmCmp, "--:--:--");
    else
        sprintf(currentSampleStatus.tmCmp, "%s", tmCmp);

    if (smpLoc == 0)
        sprintf(currentSampleStatus.smpLoc, "R:- Z:- W:--");
    else {
        int rack = (smpLoc / 1000000L);
        int zone = (smpLoc / 10000L % 100);
        int well = (smpLoc % 1000);
        sprintf(currentSampleStatus.smpLoc, "R:%d Z:%d W:%02d",
                rack, zone, well);
    }

    if (barcode == NULL)
        sprintf(currentSampleStatus.barcode, "--------");
    else
        sprintf(currentSampleStatus.barcode, "%s", barcode);

    if (pcsTgt < 0)
        sprintf(currentSampleStatus.pcsTgt, "----");
    else
        sprintf(currentSampleStatus.pcsTgt, "%04d", pcsTgt);

    if (pcsAct < 0)
        sprintf(currentSampleStatus.pcsAct, "----");
    else
        sprintf(currentSampleStatus.pcsAct, "%04d", pcsAct);

    if (expTgt < 0)
        sprintf(currentSampleStatus.expTgt, "----");
    else
        sprintf(currentSampleStatus.expTgt, "%04d", expTgt);

    if (expAct < 0)
        sprintf(currentSampleStatus.expAct, "----");
    else
        sprintf(currentSampleStatus.expAct, "%04d", expAct);

    if (status == NULL)
        sprintf(currentSampleStatus.status, "Status unknown.");
    else
        sprintf(currentSampleStatus.status, "%s", status);
}


int hermes_clear_robot()
{
    int i, stat, loc, pcs;
    int t1,t2,t3,t4,t5,t6;
    char rsp[80];
    char tbuf[48];
    time_t t;

    stat = HRM_SUCCESS;
    /*
     * If we're currently between batches, then
     *   the user is probably testing samples
     *   manually in which case we ignore this cmd.
     */
    if (newBatch) {
        return 0;
    }

    /*
     * We're here because experiment got aborted.
     * Customer probably still wants us to try and
     *   return the sample to the rack so clear
     *   the abort and keep on going.
     * If they really want us to stop dead in
     *   our tracks, they'll have to hit abort again.
     */
    if ((robotError == HRM_OPERATION_ABORTED) ||
        ((robotError == HRM_SUCCESS) && AbortRobo)) {
        DPRINT(0, "Clearing prior abort error.\n");
        AbortRobo = 0;
        robotError = HRM_SUCCESS;
    }

    else if ( (robotError == (SMPERROR+HRM_BARCODE)) &&
              (strstr(bcrErrAction, "Skip") != NULL) ) {
        DPRINT(0, "Clearing prior barcode error.\n");
        robotError = HRM_SUCCESS;
    }

    /*
     * If any robot or gilson error occurred, it's
     *   not safe to move the robot.  Just return.
     */
    if (robotError != HRM_SUCCESS) {
        DPRINT(0, "hermes_clear_robot: not safe to move the robot. Please"
                  "clear the system manually and restart Roboproc\n");
        WriteCompletionValues(tbuf, 0, NULL, -1, -1, -1, -1,
            GetAcqErrMsg(robotError));
        strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
            sizeof(currentSampleStatus.status));
        EndBatch();
        return robotError;
    }

    /* Update Status File */
    UpdateTurbineStatus();

    /* This should be done already, but you never know */
    if (!scheduler_initialized)
        init_scheduler();

    /* If all went well on last run, update the completion list */
    t = time(0);
    strftime(tbuf, 48, "%T", localtime( &t ) );

    if (sampleInMagnet) {
        int pcs = sampleInMagPcsSlot-1;

        WriteCompletionValues(tbuf, sampleInMagnet,
            sample[pcs].barcode,
            sample[pcs].pcsSec, actualPcsTimer[pcs].timeLeft,
            sample[pcs].magSec, actualExpTimer[pcs].timeLeft,
            NULL);
    }

    /* Empty Wait Queue */
    while (get_next_sample(&t1, &t2, &t3, &t4, &t5, &t6) != 0) { ; }

    /* Check if there is a sample at the bottom of the magnet */
    stat = HRM_SUCCESS;
    if (SendRabbit("<UBSTAT1>\n", rsp) != 0) {
        stat = (AbortRobo ? HRM_OPERATION_ABORTED : (SMPERROR+SMPRMFAIL));
    }
    else if (strstr(rsp, "ON") != NULL) {

        if ((SendRabbit("<EJECT>\n", rsp) != 0) ||
            (strstr(rsp, "SUCCESS") == NULL)) {
            stat = (AbortRobo ? HRM_OPERATION_ABORTED : (SMPERROR+SMPRMFAIL));
        }
    }

    if (stat != HRM_SUCCESS) {
        robotError = stat;
        strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
            sizeof(currentSampleStatus.status));
        EndBatch();
        return robotError;
    }

    /* Check for a sample at the top of the magnet */
    if (SendRabbit("<UBSTAT0>\n", rsp) != 0)
        stat = (AbortRobo ? HRM_OPERATION_ABORTED : (SMPERROR+SMPRMFAIL));

    else if (strstr(rsp, "ON") != NULL) {

        /* Do we know what sample this is? */
        if (sampleInMagnet == -1) {
            DPRINT(0, "Unknown sample in magnet.  Please clear the system "
                      "manually and restart Roboproc\n");
            robotError = (SMPERROR+SMPRMFAIL);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
            EndBatch();
            return robotError;
        }

        /* Get sample from magnet */
        pcs = sampleInMagPcsSlot-1;
        stat =  EjectSample(sampleInMagnet);

        delete_timer(sample[pcs].magnetEmptyTimer);
        sample[pcs].magnetEmptyTimer = NULL;
    }

    /*  Always Turn the Air Off */
    if (stat == HRM_SUCCESS) {
        if (SendRabbit("<AIR->\n", rsp) != 0) {
            if (AbortRobo) {
                robotError = HRM_OPERATION_ABORTED;
                strcpy(currentSampleStatus.status, "Aborted by user");
            }
            else {
                robotError =
                    convert_robot_errorcode(HRM_IO_ERROR, MAGTORACK);
                strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                    sizeof(currentSampleStatus.status));
            }
            EndBatch();
            return robotError;
        }
    }

    else {
        robotError = stat;
        if (robotError == HRM_OPERATION_ABORTED) {
            DPRINT(0, "clear_robot aborted by user\n");
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            DPRINT1(0, "EjectSample error (%d) "
                    " This is a non-recoverable error.\r\n", robotError);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }
        EndBatch();
        return robotError;
    }

    /* Update Status File */
    UpdateTurbineStatus();

    /* Always Clear the Extraction Station */
    stat = hermes_clear_extraction_station();
    if (stat != HRM_SUCCESS) {
        robotError = stat;
        if (robotError == HRM_OPERATION_ABORTED) {
            DPRINT(0, "clear_robot aborted by user\n");
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }
        EndBatch();
        return robotError;
    }

    /*
     * If there are any samples in the preconditioner,
     *   send them back to the rack.
     */
    stat = HRM_SUCCESS;
    while (timerList != NULL) {
        loc = sample[timerList->sampleIndex].startLoc;
        pcs = GetAssignedPcsSlot(loc);
        if (timerList->timerType == MYTIMER_PCS) {
            /* This sample never made it into the PCS */
            DPRINT2(0, "Sample %d scheduled for turbine %d never made it "
                       "to the preconditioner. Removing.\n ",
                    loc, pcs+1);
            FreePcsSlot(loc);
        }
        else if (timerList->timerType == MYTIMER_MAGNET) {

            DPRINT2(0, "Returning sample %d (turbine %d) to the rack.\n ",
                    loc, pcs+1);

            /* This will move the Sample into the Extraction Station */
            stat = RejectUnusedSample(pcs+1, loc);
            if (stat != HRM_SUCCESS) {
                robotError = stat;
                if (robotError == HRM_OPERATION_ABORTED) {
                    DPRINT(0, "clear_robot aborted by user\n");
                    strcpy(currentSampleStatus.status, "Aborted by user");
                }
                else {
                    strncpy(currentSampleStatus.status,
                            GetAcqErrMsg(robotError),
                            sizeof(currentSampleStatus.status));
                }
                EndBatch();
                return robotError;
            }

            /* Update Status File */
            UpdateTurbineStatus();

            /* Now, Clear the Extraction Station */
            stat = hermes_clear_extraction_station();
            if (stat != HRM_SUCCESS) {
                robotError = stat;
                if (robotError == HRM_OPERATION_ABORTED) {
                    DPRINT(0, "clear_robot aborted by user\n");
                    strcpy(currentSampleStatus.status, "Aborted by user");
                }
                else {
                    strncpy(currentSampleStatus.status,
                            GetAcqErrMsg(robotError),
                            sizeof(currentSampleStatus.status));
                }
                EndBatch();
                return robotError;
            }

            /* Update Status File */
            UpdateTurbineStatus();

        }
        else if (timerList->timerType == MYTIMER_DONE) {
            DPRINT2(0, "Encountered done timer for sample %d (turbine %d)"
                       "during robot clear!\n", loc, pcs+1);
        }

        /*
         * This sample should be back in its rack location now,
         * so clear all timers associated with this sample
         */
        if ((sample[pcs].pcsStartTimer != NULL) &&
            (sample[pcs].pcsStartTimer)->valid)
            delete_timer(sample[pcs].pcsStartTimer);
        if ((sample[pcs].pcsDoneTimer != NULL) &&
            (sample[pcs].pcsDoneTimer)->valid)
            delete_timer(sample[pcs].pcsDoneTimer);
        if ((sample[pcs].magnetEmptyTimer != NULL) &&
            (sample[pcs].magnetEmptyTimer)->valid)
            delete_timer(sample[pcs].magnetEmptyTimer);
    }

    /* Free up all resources - get a clean slate */
    if (stat == HRM_SUCCESS) {
        DPRINT1(0, "slotsReserved = %d\n", slotsReserved);
        FreeAllPcsSlots();
        FreeTimerList();
        FreeSchedMap();

        for (i=0; i<MAX_PCS_SLOTS; i++) {
            actualPcsTimer[i].valid = 0;
            actualPcsTimer[i].timeLeft = 0;
            actualExpTimer[i].valid = 0;
            actualExpTimer[i].timeLeft = 0;
        }

        /* Goto home position so user may access all locations */
        stat = RobotStartup(1);
        if (stat != HRM_SUCCESS) {
            robotError = convert_robot_errorcode(stat, NOTRANSPORT);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
            EndBatch();
            return robotError;
        }
    }

    /* Update Status File */
    UpdateTurbineStatus();

    sprintf(currentSampleStatus.status, "Complete");
    EndBatch();

    {
    /* From socket.c, not declared in any header file */
    extern int talk2Acq(char *hostname, char *username, int cmd,
                        char *msg_for_acq, char *msg_for_vnmr, int mfv_len);
    extern char HostName[];
    char msg4acq[ 80 ], replymsg[ 80 ];
    int fd;

    /* 
     * Tell console that sample location is zero (sethw('loc',0))
     * Use Roboproc as user, acqHwSet in msgehandler.c checks this and 
     *  allows Roboproc to perform any commands it wishes
     */
    sprintf(msg4acq, "1,%d,0", SETLOC);
    fd = talk2Acq(HostName, "Roboproc", ACQHARDWARE, msg4acq,
                  replymsg, sizeof(replymsg));
    if (fd >= 0) {
        /* should be checking replymsg for errors here */
        close(fd);
        DPRINT1(1, "clear_robot: Msge completed,reply: '%s' \n", replymsg);
    }
    else {
        DPRINT(1, "clear_robot: unable to sync loc info with console\n");
    }
    }

    return stat;
}

int check_timers()
{
    int rack, zone, well, slot, stat, i, errorCode, result, loadedElsewhere;
    extern int rack768AS, zone768AS, well768AS;
    extern int InterpScript(char *tclScriptFile);
    extern GILSONOBJ_ID pGilObjId;

    while ((robotError == HRM_SUCCESS) &&
           (timerList != NULL) &&
           (timerList->timeLeft <= 0) &&
           ( (timerList->timerType == MYTIMER_PREP) ||
             (timerList->timerType == MYTIMER_PCS) )) {

        /*
         * Check if an expired PREP start timer is at the head of the
         *   ready queue.  Do not handle expired start timers that
         *   are not at the head of the queue because that means
         *   earlier experiments are running overtime and we don't
         *   want to move any new samples into the preconditioner.
         */
        if (timerList->timerType == MYTIMER_PREP) {

            /* Run Sample Prep Protocol */
            slot = timerList->sampleIndex;
            rack768AS = (sample[slot].startLoc / 1000000L);
            zone768AS = (sample[slot].startLoc / 10000L % 100);
            well768AS = (sample[slot].startLoc % 1000);

#if 0
            /* TODO: Make sure robot one is out of the way here */
            if ((result = RobotStartup(0)) != HRM_SUCCESS) {
                DPRINT1(0, "Roboproc encountered error %d while trying "
                        "to home robot one.  May need to power-"
                        "cycle the robot and clear the system manually "
                        "to recover.\r\n", result);
                robotError = convert_robot_errorcode(result, NOTRANSPORT);
                return robotError;
            }
#endif

            /* Script parsing errors will be reflected in errorCode */
            errorCode = InterpScript(sample[slot].gilPreScript);

            /* Make sure Gilson is out of the Robot's Way!!! */
            DPRINT(0, "Moving Gilson out of the way...\r\n");
            result = gilMoveZ(pGilObjId, pGilObjId->ZTopClamp);
            if (result == 0) {
                result = gilMoveXY(pGilObjId,
                                   pGilObjId->X_MinMax[0] + 20,
                                   pGilObjId->Y_MinMax[0] + 20);
                  if (result == 0) {
                      DPRINT(0,"Gilson is clear.\r\n");
                  }
            }

            if (result < 0) {
                DPRINT1(0, "Unable to move gilson to home location (%d).\r\n",
                        result);
                robotError = SMPERROR+TCLSCRIPTERROR;
            }

            if (errorCode != 0) {
                robotError = errorCode;
            }

            if (robotError != HRM_SUCCESS) {
                record_error(sample[slot].startLoc, robotError);
                delete_timer(sample[slot].pcsStartTimer);
                sample[slot].pcsStartTimer = NULL;
                DeleteSchedItem(slot, "toPCS");

                delete_timer(sample[slot].pcsDoneTimer);
                sample[slot].pcsDoneTimer = NULL;
                DeleteSchedItem(slot, "toMag");

                delete_timer(sample[slot].magnetEmptyTimer);
                sample[slot].magnetEmptyTimer = NULL;
                DeleteSchedItem(slot, "toExt");
                DeleteSchedItem(slot, "toRack");

                FreePcsSlot(sample[slot].startLoc);

                return robotError;
            }

            /* Sample Prep is Done! */
            delete_timer(sample[slot].prepStartTimer);
            sample[slot].prepStartTimer = NULL;
            DeleteSchedItem(slot, "doPREP");

            DisplayTimers();
            DisplaySchedule();
        }

        /*
         * Check if an expired PCS start timer is at the head of the
         *   ready queue.  Do not handle expired start timers that
         *   are not at the head of the queue because that means
         *   earlier experiments are running overtime and we don't
         *   want to move any new samples into the preconditioner.
         */
        else if (timerList->timerType == MYTIMER_PCS) {

            /* Move Sample to Preconditioner */
            slot = timerList->sampleIndex;
            rack = (sample[slot].startLoc / 1000000L);
            zone = (sample[slot].startLoc / 10000L % 100);
            well = (sample[slot].startLoc % 1000);

            /*
             * Make sure a sample from this location isn't
             *   already loaded into a turbine elsewhere.
             */
            for (i=0, loadedElsewhere=0; i<MAX_PCS_SLOTS; i++) {
                if ((i != slot) &&
                    (pcsSlotPopulated[i] == sample[slot].startLoc)) {
                    DPRINT2(0, "Insertion request for sample %d but a "
                             "sample from this location is already "
                             "loaded into turbine %d\r\n",
                          sample[slot].startLoc, i+1);
                    loadedElsewhere = 1;
                }
            }

            stat = HRM_SUCCESS;
            if (!loadedElsewhere) {

                /* Transfer this sample to the preconditioner NOW */
                stat =
                    TransferVial(sample[slot].bcrSec,
                                 rack, zone, well, slot+1);
                if ((stat != HRM_SUCCESS) && (stat != (SMPERROR+HRM_BARCODE)))
                    DPRINT2(0, "TransferVial error (%d) on sample %d "
                            " This is a non-recoverable error.\r\n",
                             stat, sample[slot].startLoc);

                /* Update Status File */
                UpdateTurbineStatus();
            }

            if (loadedElsewhere || (stat != HRM_SUCCESS)) {

                record_error(sample[slot].startLoc, stat);
                delete_timer(sample[slot].pcsStartTimer);
                sample[slot].pcsStartTimer = NULL;
                DeleteSchedItem(slot, "toPCS");

                delete_timer(sample[slot].pcsDoneTimer);
                sample[slot].pcsDoneTimer = NULL;
                DeleteSchedItem(slot, "toMag");

                delete_timer(sample[slot].magnetEmptyTimer);
                sample[slot].magnetEmptyTimer = NULL;
                DeleteSchedItem(slot, "toExt");
                DeleteSchedItem(slot, "toRack");

                FreePcsSlot(sample[slot].startLoc);

                robotError = convert_robot_errorcode(stat, RACKTOPCS);
                return convert_robot_errorcode(stat, RACKTOPCS);
            }

            /*
             * We did it!  Sample is in the Preconditioner.
             */
            DPRINT2(0, "Sample %d xferred to pcs slot %d.\r\n",
                    sample[slot].startLoc, slot+1);

            /* Start counting actual time in the PCS */
            actualPcsTimer[slot].timeLeft = 0;
            actualPcsTimer[slot].valid = 1;

            delete_timer(sample[slot].pcsStartTimer);
            sample[slot].pcsStartTimer = NULL;
            DeleteSchedItem(slot, "toPCS");

            DisplayTimers();
            DisplaySchedule();
        }
    }

    if (robotError == HRM_SUCCESS)
        return (AbortRobo ? HRM_OPERATION_ABORTED : HRM_SUCCESS);
    else
        return robotError;
}


void schedule_new_samples(void)
{
    int pcs, bcrSec, pcsSec, rack, zone, well, magSec, loc;

    /*
     * Schedule up as many of the remaining PCS slots as possible.
     */
    while (slotsReserved < MAX_PCS_SLOTS) {
    
    	

        if (get_next_sample(&bcrSec, &pcsSec, &magSec, &rack, &zone, &well)) {

            /* Schedule sample */
            loc = (rack * 1000000L) + (zone * 10000L) + (well);
            
            /* Check for multiple experiments on same sample */
            DPRINT2(0, "SDM schedule_new_samples, prev: %d  current:%d \r\n", 
    		prev_loc, loc);
    		
    		if( (prev_loc== loc) && (1) )
    			break;
    	
    		prev_loc= loc;
            
            pcs = NextFreePcsSlot(loc);
            DPRINT(0,"SDM schedule_new_samples:  \r\n");
            run_scheduler(pcs, rack, zone, well, magSec, pcsSec);
        }
        else
            break;
    }

    return;
}

int hermes_clear_extraction_station()
{
    int rack, zone, well, stat;
    int errorCode, result;

    /* This should be done already, but you never know */
    if (!scheduler_initialized)
        init_scheduler();

    if (robotError != HRM_SUCCESS)
        return robotError; /* vnmr-style error code */

    if (sampleInExtractionStation != -1) {

        rack = (sampleInExtractionStation / 1000000L);
        zone = (sampleInExtractionStation / 10000L % 100);
        well = (sampleInExtractionStation % 1000);

        if ((stat = ReturnSampleToRack(rack, zone, well,
            sampleInExtPcsSlot)) != HRM_SUCCESS) {

            robotError = convert_robot_errorcode(stat, MAGTORACK);
            DeleteSchedItem(sampleInExtPcsSlot-1, "toRack");

            if (robotError != HRM_OPERATION_ABORTED) {
                DPRINT1(0, "ReturnSampleToRack error (%d) "
                           " This is a non-recoverable error.\r\n", stat);
            }

            return robotError;
        }

        /* Run Post- Sample Prep Protocol, if Specified */
        if (useGilson) {
            extern int InterpScript(char *tclScriptFile);
            extern GILSONOBJ_ID pGilObjId;
            extern int rack768AS, zone768AS, well768AS;

            if ((sample[sampleInExtPcsSlot-1].gilPostTime != 0) &&
                (sample[sampleInExtPcsSlot-1].gilPostScript[0] != '\0')) {

                rack768AS = rack;
                zone768AS = zone;
                well768AS = well;

                /* Script parsing errors will be reflected in errorCode */
                errorCode = InterpScript(
                    sample[sampleInExtPcsSlot-1].gilPostScript);

                /* Make sure Gilson is out of the Robot's Way!!! */
                result = gilMoveZ(pGilObjId, pGilObjId->ZTopClamp);
                if (result == 0)
                    result = gilMoveXY(pGilObjId,
                                       pGilObjId->X_MinMax[0] + 20,
                                       pGilObjId->Y_MinMax[0] + 20);

                if (errorCode != 0) {
                    robotError = errorCode;
                    return robotError;
                }

                if (result < 0) {
                    robotError = SMPERROR+TCLSCRIPTERROR;
                    return robotError;
                }
            }
        }

        FreePcsSlot(sampleInExtractionStation);
        DeleteSchedItem(sampleInExtPcsSlot-1, "toRack");
        sampleInExtractionStation = -1;
        sampleInExtPcsSlot = -1;
        DisplaySchedule();
    }

    /* Update Status File */
    UpdateTurbineStatus();

    return HRM_SUCCESS;
}

/*
 * hermes_get_sample
 * -----------------
 * Remove sample from magnet.
 * Called via GET request from console/expproc.
 *
 */
int hermes_get_sample(int smp, int pcsSlot)
{
    int delay, stat, pcs;
    char tbuf[48];
    time_t t;

    if (robotError == HRM_OPERATION_ABORTED) {
        DPRINT(0, "Clearing prior abort error.\n");
        robotError = HRM_SUCCESS;
    }

    else if ( (robotError == (SMPERROR+HRM_BARCODE)) &&
              (strstr(bcrErrAction, "Skip") != NULL) ) {
        DPRINT(0, "Clearing prior barcode error.\n");
        robotError = HRM_SUCCESS;
    }

    // rack = (smp / 1000000L);
    // zone = (smp / 10000L % 100);
    // well = (smp % 1000);

    /* Read settings at beginning of each batch */
    if (newBatch) {
        StartBatch();
    }

    if (robotError != HRM_SUCCESS) {
        WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
            "Cannot recover from previous error(s).");
        EndBatch();
        return robotError;
    }

    /*
     * Do we know what sample (if any) is in the magnet?
     */
    if (sampleInMagnet == -1) {
        char rsp1[32], rsp2[32];
        extern int SendRabbit(char *cmd, char *rsp);

        int result1 = SendRabbit("<UBSTAT0>\n", rsp1);
        int result2 = SendRabbit("<UBSTAT1>\n", rsp2);
        if ((result1 != 0) || (result2 != 0)) {
            if (AbortRobo) {
                robotError =  HRM_OPERATION_ABORTED;
                WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                    "Aborted by user");
            }
            else {
                robotError =
                    convert_robot_errorcode(HRM_IO_ERROR, MAGTORACK);
                WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                    GetAcqErrMsg(robotError));
            }

            EndBatch();
            return robotError;
        }

        if ((strstr(rsp1, "OFF") != NULL) &&
            (strstr(rsp2, "OFF") != NULL)) {
            /*
             * Doesn't appear to be a sample there.
             * This could be because:
             *   - the sample was manually cleared without
             *     re-starting the console and procs;
             *   - the procs were re-started without clearing
             *     the system first; OR
             *   - there is no sample, and the console is just
             *     saying eject to be absolutely sure
             * Clear the extraction station if needed, and then
             * just return with no error and hope for the best...
             */
            stat = hermes_clear_extraction_station();
            if (stat != HRM_SUCCESS) {
                /* Error code is already converted to vnmr-style value */
                robotError = stat;

                if (robotError == HRM_OPERATION_ABORTED) {
                    DPRINT(0, "get sample aborted by user\n");
                    WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                        "Aborted by user");
                }
                else {
                    WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                        GetAcqErrMsg(robotError));
                }

                EndBatch();
                return robotError;
            }

            return HRM_SUCCESS;
        }

        /*
         * There's really a sample there, but we have no idea
         * what preconditioner slot it goes in.  If the pcs slot
         * given to us by the caller seems viable, go ahead and
         * try ejecting to that location.  Otherwise, we'll have
         * to let the customer clear the system manually.
         */
        if ((pcsSlot <= 0) ||
            (slotsReserved >= MAX_PCS_SLOTS) ||
            (pcsSlotPopulated[pcsSlot-1] != 0)) {

            DPRINT1(0, "Roboproc doesn't know which turbine sample %d "
                    "is using.  Manually clear the magnet and robot.  "
                    "Then re-start Roboproc to recover.\r\n", smp);

            robotError = SMPERROR+HRM_MAGTORACK;

            /*
             * Before returning, move the robot to the home position so the
             * customer can access all the stations freely.
             */
            if ((stat = RobotStartup(1)) != HRM_SUCCESS) {
                if (stat != HRM_OPERATION_ABORTED) {
                    DPRINT1(0, "Roboproc encountered error %d while sending "
                            "startup command to robot.  May need to power-"
                            "cycle the robot and clear the system manually "
                            "to recover.\r\n", stat);
                }
                robotError = convert_robot_errorcode(stat, NOTRANSPORT);
            }

            if (robotError == HRM_OPERATION_ABORTED) {
                DPRINT(0, "get sample aborted by user\n");
                WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                    "Aborted by user");
            }
            else {
                WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                    GetAcqErrMsg(robotError));
            }

            EndBatch();
            return robotError;
        }

        pcsSlotPopulated[pcsSlot-1] = smp;
        slotsReserved++;
    }
    else {
        if ((smp != 0) && (smp != sampleInMagnet)) {
            robotError = SMPERROR+HRM_MAGTORACK;

            DPRINT2(0, "Vnmr requesting eject of sample %d but "
                   "roboproc believes sample %d is in the magnet\r\n",
                    smp, sampleInMagnet);

            WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                GetAcqErrMsg(robotError));

            EndBatch();
            return robotError;
        }
        smp = sampleInMagnet;
        // rack = (smp / 1000000L);
        // zone = (smp / 10000L % 100);
        // well = (smp % 1000);
    }

    pcs = GetAssignedPcsSlot(smp);
    if (pcs < 0) {
        robotError = SMPERROR+HRM_MAGTORACK;

        DPRINT1(0, "Roboproc doesn't know what preconditioner slot "
                   "sample %d belongs in.  Manually clear the magnet and "
                   "robot.  Then re-start Roboproc to recover.\r\n", smp);

        WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
            GetAcqErrMsg(robotError));

        EndBatch();
        return robotError;
    }

    DPRINT4(0, "\r\nSample %d PCS slot %d: Target EXP Time: %d\r\n"
               "                           Actual EXP Time: %d\r\n\r\n",
        smp, pcs+1,
        sample[pcs].magSec,
        (actualExpTimer[pcs]).timeLeft);

    t = time(0);
    strftime(tbuf, 48, "%T", localtime( &t ) );

    WriteCompletionValues(tbuf, smp,
        sample[pcs].barcode,
        sample[pcs].pcsSec, actualPcsTimer[pcs].timeLeft,
        sample[pcs].magSec, actualExpTimer[pcs].timeLeft,
        NULL);

    actualExpTimer[pcs].valid = 0;
    actualExpTimer[pcs].timeLeft = 0;

    /*
     * OK: we have a sample in the magnet and we know what turbine it's in.
     * Now, try to eject it.
     */
    stat = EjectSample(smp);

    if (stat != HRM_SUCCESS) {

        /* Delete the timer */
        delete_timer(sample[pcs].magnetEmptyTimer);
        sample[pcs].magnetEmptyTimer = NULL;
        DeleteSchedItem(pcs, "toExt");

        /* Error code is already converted to vnmr-style value */
        robotError = stat;
        if (robotError == HRM_OPERATION_ABORTED) {
            DPRINT(0, "get sample aborted by user\n");
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            DPRINT1(0, "EjectSample error (%d) "
                    " This is a non-recoverable error.\r\n", robotError);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }

        EndBatch();
        return robotError;
    }

    /* Delete the timer */
    delay = 0;
    if ((sample[pcs].magnetEmptyTimer)->valid) {
        if ((sample[pcs].magnetEmptyTimer)->timeLeft < 0)
            delay = -(sample[pcs].magnetEmptyTimer)->timeLeft;
        delete_timer(sample[pcs].magnetEmptyTimer);
        sample[pcs].magnetEmptyTimer = NULL;
        DeleteSchedItem(pcs, "toExt");
    }

    /*
     * PCS transfers may have stopped if unscheduled delays
     *   occurred during sample processing.
     * Reschedule any samples that are not yet in the PCS and
     *   shift expected magnet empty times of samples in the PCS.
     */
    if (delay) {
        DPRINT2(0, "Processing of sample %d caused delays of %d seconds."
                   "Pushing samples not yet started back to wait queue "
                   "for rescheduling.\r\n", smp, delay);
        PushNonPCSSamplesToWaitQueue();
        PropogateOverdueSeconds(delay);
        schedule_new_samples();
    }

    if ((stat = check_timers()) != HRM_SUCCESS) {
        robotError = stat;
        if (robotError == HRM_OPERATION_ABORTED) {
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }

        EndBatch();
        return robotError;
    }

    /*
     * If this was the only sample in the system,
     *   bring it all the way back to the rack.
     */
    if ((SampleQueueHead == NULL) &&
        (timerList == NULL) &&
        (slotsReserved == 1) &&
        (sampleInExtractionStation == smp)) {
        stat = hermes_clear_extraction_station();
        if (stat != HRM_SUCCESS) {
            robotError = stat;
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));

            EndBatch();
            return robotError;
        }
    }

    DisplayTimers();
    DisplaySchedule();

    /* Update Status File */
    UpdateTurbineStatus();

    sprintf(currentSampleStatus.status, "Complete");
    UpdateCompletionList();
    return HRM_SUCCESS;
}


/*
 * hermes_put_sample
 * -----------------
 * Put sample into magnet.
 * Called via "PUT" message from console/expproc.
 *
 */
int hermes_put_sample(int smp, int *pcsSlot)
{
    int rack, zone, well;
    int pcs, stat, loc, prepping, early;
    MYTIMER *rdyPtr;
    SAMPLE *waitPtr;
    char tbuf[48];
    time_t t;
    extern void reportRobotStat(int robstat);

    if (robotError == HRM_OPERATION_ABORTED) {
        DPRINT(0, "Clearing prior abort error.\n");
        robotError = HRM_SUCCESS;
    }

    else if ( (robotError == (SMPERROR+HRM_BARCODE)) &&
              (strstr(bcrErrAction, "Skip") != NULL) ) {
        DPRINT(0, "Clearing prior barcode error.\n");
        robotError = HRM_SUCCESS;
    }

    *pcsSlot = 0;
    rack = (smp / 1000000L);
    zone = (smp / 10000L % 100);
    well = (smp % 1000);

    if (newBatch) {
        StartBatch();
    }

    if (robotError != HRM_SUCCESS) {

        if (robotError == HRM_OPERATION_ABORTED) {
            WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                "Aborted by user");
        }
        else {
            WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                GetAcqErrMsg(robotError));
        }

        EndBatch();
        return robotError;
    }

    /* If the Magnet is Occupied, Stop Now and Return an Error */
    if (sampleInMagnet > 0) {

        robotError = SMPERROR+HRM_RACKTOPCS;

        DPRINT(0, "hermes_put_sample: magnet is occupied!\r\n");

        if (robotError == HRM_OPERATION_ABORTED) {
            WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                "Aborted by user");
        }
        else {
            WriteCompletionValues(NULL, smp, NULL, -1, -1, -1, -1,
                GetAcqErrMsg(robotError));
        }

        EndBatch();
        return robotError;
    }

    /* Make sure this sample is the next one queued for the magnet */
    while (1) {
        rdyPtr = timerList;
        while (rdyPtr != NULL) {
            if (rdyPtr->timerType == MYTIMER_MAGNET)
                break;
            rdyPtr = rdyPtr->next;
        }

        if (rdyPtr == NULL)
            break;

        loc = sample[rdyPtr->sampleIndex].startLoc;
        if (loc == smp)
            break;

        /*
         * There are samples ahead of this one in the ready queue!
         * The user must have deleted these samples after submitting
         *   them to the study queue.
         */
        DPRINT1(0, "Sample %d at head of ready queue not requested. "
                   "Removing.\r\n", loc);

        pcs = GetAssignedPcsSlot(loc);

        stat = HRM_SUCCESS;
        if ((sample[rdyPtr->sampleIndex].pcsStartTimer == NULL) ||
            (!(sample[rdyPtr->sampleIndex].pcsStartTimer)->valid)) {

            /*
             * Unrequested samples in the preconditioner already!
             * Return them to the rack without running an experiment.
             */
            DPRINT1(0, "Sample %d is in the preconditioner, "
                       "returning to rack.\r\n", loc);

            /*
             * NOTE: after this call, the sample is in the extraction station
             * Later, a call to clear_extraction_station will complete
             *  the return to the rack and free up the resources.
             */
            stat = RejectUnusedSample(pcs+1, loc);
        }

        else {
            /*
             * This sample was scheduled but never actually made it
             *   into the preconditioner.  Must free up the scheduler
             *   time and the turbine now, so that it can be used by
             *   other samples.
             */
            DeleteSchedItem(pcs, "toRack");
            FreePcsSlot(loc);
        }

        /* Always Remove Timers */
        if ((sample[pcs].prepStartTimer != NULL) &&
            (sample[pcs].prepStartTimer)->valid)
            delete_timer(sample[pcs].prepStartTimer);
        if ((sample[pcs].pcsStartTimer != NULL) &&
            (sample[pcs].pcsStartTimer)->valid)
            delete_timer(sample[pcs].pcsStartTimer);
        if ((sample[pcs].pcsDoneTimer != NULL) &&
            (sample[pcs].pcsDoneTimer)->valid)
            delete_timer(sample[pcs].pcsDoneTimer);
        if ((sample[pcs].magnetEmptyTimer != NULL) &&
            (sample[pcs].magnetEmptyTimer)->valid)
            delete_timer(sample[pcs].magnetEmptyTimer);

        DeleteSchedItem(pcs, "doPREP");
        DeleteSchedItem(pcs, "toPCS");
        DeleteSchedItem(pcs, "toMag");
        DeleteSchedItem(pcs, "toExt");

        if (stat != HRM_SUCCESS) {

            /* Error code is already converted to vnmr-style value */
            robotError = stat;

            /* Error Returning Unrequested Samples to Rack */
            t = time(0);
            strftime(tbuf, 48, "%T", localtime( &t ) );
            WriteCompletionValues(tbuf, smp,
                sample[pcs].barcode,
                sample[pcs].pcsSec, -1,
                sample[pcs].magSec, -1,
                NULL);
            if (robotError == HRM_OPERATION_ABORTED) {
                DPRINT(0, "put sample aborted by user\n");
                strcpy(currentSampleStatus.status, "Aborted by user");
            }
            else {
                DPRINT1(0, "RejectUnusedSample error (%d) "
                        " This is a non-recoverable error.\r\n", stat);
                strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                    sizeof(currentSampleStatus.status));
            }

            EndBatch();
            return robotError;
        }

    }

    /*
     * This sample is not in the timer/ready queue.
     * Look in the wait queue.
     */
    if (rdyPtr == NULL) {

        waitPtr = SampleQueueHead;
        while ( (waitPtr != NULL) &&
                ((waitPtr->well != well) ||
                 (waitPtr->zone != zone) ||
                 (waitPtr->rack != rack)) ) {
            int tBcr, tPcs, tMag, tRack, tZone, tWell;

            /*
             * There are samples ahead of this one in the wait queue!
             * Remove these entries, assuming they were deleted
             *   by the user from the Vnmr study queue.
             */

            get_next_sample(&tBcr, &tPcs, &tMag, &tRack, &tZone, &tWell);

            loc = (tRack * 1000000L) + (tZone * 10000L) + (tWell);
            DPRINT2(0, "Roboproc deleting sample %d from the "
                       "wait queue in order to move sample %d "
                       "to the front\r\n", loc, smp);

            waitPtr = SampleQueueHead;
        }

        if (waitPtr == NULL) {
            /*
             * No more entries in the wait queue
             * This sample must be a manual insertion request
             * Queue this sample for immediate transfer
             */
            DPRINT1(0,"Roboproc queueing manual insertion "
                      "request for sample %d\r\n", smp);

  
            queue_sample(rack, zone, well, 0, 0);
             
        }

        /* Sample is now at the head of the wait queue */
        DPRINT(0,"hermes_put_sample: "
                 "sample is at the head of the wait queue\r\n");

        /* Schedule new samples, (should only be this one) */
        schedule_new_samples();
    }

    DPRINT(0, "hermes_put_sample: "
              "sample is at the head of the ready queue\r\n");

    pcs = GetAssignedPcsSlot(smp);
    if (pcs < 0) {
        DPRINT1(0, "ERROR!!! Cannot find sample %d in the system. "
                   "Manually clear the robot and re-start Roboproc "
                   "to recover.\r\n", smp);
        robotError = SMPERROR+HRM_ROBOTERR;

        t = time(0);
        strftime(tbuf, 48, "%T", localtime( &t ) );
        WriteCompletionValues(tbuf, smp,
            sample[pcs].barcode,
            sample[pcs].pcsSec, -1,
            sample[pcs].magSec, -1,
            GetAcqErrMsg(robotError));

        EndBatch();
        return SMPERROR+HRM_ROBOTERR;
    }

    /* Return the turbine id for this sample */
    *pcsSlot = pcs+1;

    /*
     * Sample is now at the head of the timer queue
     * Make sure any pre-processing has completed.
     */
    if (useGilson && (gilPreScript[0] != '\0'))
        prepping = 1;
    else
        prepping = 0;

    if ( prepping && 
           (sample[pcs].prepStartTimer != NULL) &&
           ((sample[pcs].prepStartTimer)->valid) &&
           ((sample[pcs].prepStartTimer)->timeLeft > 0) )
        early = 1;
    else if ( !prepping &&
           (sample[pcs].pcsStartTimer != NULL) &&
           ((sample[pcs].pcsStartTimer)->valid) &&
           ((sample[pcs].pcsStartTimer)->timeLeft > 0) )
        early = 1;
    else
        early = 0;

    if (early) {
        /*
         * User has requested this sample earlier than expected!
         * Reschedule all samples that are not yet in the PCS
         *   including this one.
         */
        DPRINT2(0, "Sample %d requested %d seconds before scheduled! "
                   "Pushing this and other samples not yet started "
                   "processing back to wait queue for rescheduling.\r\n",
                   smp, (sample[pcs].pcsStartTimer)->timeLeft);
        PushNonPCSSamplesToWaitQueue();

        /* This sample should be at the head of the wait queue now */
        schedule_new_samples();

        /* This sample should be at the head of the ready queue now */
        pcs = GetAssignedPcsSlot(smp);
    }

    DPRINT1(0, "hermes_put_sample: start processing sample %d NOW\r\n", smp);

    while (sample[pcs].pcsStartTimer != NULL) {
        if ((stat = check_timers()) != HRM_SUCCESS) {
            robotError = stat;

            t = time(0);
            strftime(tbuf, 48, "%T", localtime( &t ) );
            WriteCompletionValues(tbuf, smp,
                sample[pcs].barcode,
                sample[pcs].pcsSec, -1,
                sample[pcs].magSec, -1,
                NULL);

            if (robotError == HRM_OPERATION_ABORTED) {
                strcpy(currentSampleStatus.status, "Aborted by user");
            }
            else {
                strncpy(currentSampleStatus.status,
                        GetAcqErrMsg(robotError),
                        sizeof(currentSampleStatus.status));
            }

            EndBatch();
            return robotError;
        }
        /* Don't hog the cpu */
        if ((sample[pcs].pcsStartTimer != NULL) &&
            (get_timer(sample[pcs].pcsStartTimer) > 1))
            pause();
    }

    /*
     * See if an error occurred while transferring this
     *  sample to the preconditioner.
     */
    if ((stat = check_error(smp, 1)) != HRM_SUCCESS) {

        robotError = convert_robot_errorcode(stat, RACKTOPCS);

        t = time(0);
        strftime(tbuf, 48, "%T", localtime( &t ) );
        WriteCompletionValues(tbuf, smp,
            sample[pcs].barcode,
            sample[pcs].pcsSec, -1,
            sample[pcs].magSec, -1,
            NULL);

        if (robotError == HRM_OPERATION_ABORTED) {
            DPRINT(0, "put sample aborted by user\n");
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            DPRINT2(0, "Error %d occurred when attempting to transfer "
                       "sample %d to the preconditioner.\r\n", stat, smp);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }

        EndBatch();
        return robotError;
    }


    /*
     * Check status of the preconditioning-done timer.
     */
    if ((sample[pcs].pcsDoneTimer != NULL) &&
        ((sample[pcs].pcsDoneTimer)->valid)) {

        /*
         * Wait for sample to be finished preconditioning.
         * Then Transfer the Requested Sample to the Magnet.
         */
        DPRINT2(0, "Roboproc waiting for sample %d in slot %d to "
                   "finish preconditioning.\r\n", smp, pcs+1);

        while (!timer_expired(sample[pcs].pcsDoneTimer)) {
            /*
             * Call check_timers while we're waiting to handle
             *   any other samples that become ready for transfer
             *   to the preconditioner.
             */
            if ((stat = check_timers()) != HRM_SUCCESS) {
                robotError = stat;

                t = time(0);
                strftime(tbuf, 48, "%T", localtime( &t ) );
                WriteCompletionValues(tbuf, smp,
                    sample[pcs].barcode,
                    sample[pcs].pcsSec, actualPcsTimer[pcs].timeLeft,
                    sample[pcs].magSec, -1,
                    NULL);

                if (robotError == HRM_OPERATION_ABORTED)
                    strcpy(currentSampleStatus.status, "Aborted by user");
                else
                    strncpy(currentSampleStatus.status,
                        GetAcqErrMsg(robotError),
                        sizeof(currentSampleStatus.status));

                EndBatch();
                return robotError;
            }

            /*
             * We can be scheduling new samples
             *  while we're waiting
             */
            schedule_new_samples();

            /* Don't hog the cpu */
            if ((sample[pcs].pcsDoneTimer != NULL) &&
                (get_timer(sample[pcs].pcsDoneTimer) > 1))
                pause();
        }
    }

    DPRINT4(0, "\r\nSample %d PCS slot %d: Target PCS Time: %d\r\n"
               "                           Actual PCS Time: %d\r\n\r\n",
        sample[pcs].startLoc, pcs+1,
        sample[pcs].pcsSec,
        (actualPcsTimer[pcs]).timeLeft);

    actualPcsTimer[pcs].valid = 0; /* don't clear timeLeft */

    stat = InsertMagnet(rack, zone, well, pcs+1);
    if (stat != HRM_SUCCESS) {

        robotError = convert_robot_errorcode(stat, PCSTOMAG);

        t = time(0);
        strftime(tbuf, 48, "%T", localtime( &t ) );
        WriteCompletionValues(tbuf, smp,
            sample[pcs].barcode,
            sample[pcs].pcsSec, actualPcsTimer[pcs].timeLeft,
            sample[pcs].magSec, -1,
            NULL);

        if (robotError == HRM_OPERATION_ABORTED) {
            DPRINT(0, "put sample aborted by user\n");
            strcpy(currentSampleStatus.status, "Aborted by user");
        }
        else {
            DPRINT1(0, "InsertMagnet error (%d) "
                       " This is a non-recoverable error.\r\n", stat);
            strncpy(currentSampleStatus.status, GetAcqErrMsg(robotError),
                sizeof(currentSampleStatus.status));
        }

        EndBatch();

        delete_timer(sample[pcs].pcsDoneTimer);
        sample[pcs].pcsDoneTimer = NULL;
        DeleteSchedItem(pcs, "toMag");

        delete_timer(sample[pcs].magnetEmptyTimer);
        sample[pcs].magnetEmptyTimer = NULL;
        DeleteSchedItem(pcs, "toExt");
        DeleteSchedItem(pcs, "toRack");

        return robotError;
    }

    /* Time to Notify Autoproc of Barcode for this Sample */
    if (barcodeUse && (sample[pcs].barcode[0] != '\0') &&
        (strncmp(sample[pcs].barcode, "<NONE>", 6) != 0) &&
        (strncmp(sample[pcs].barcode, "NOREAD", 6) != 0)) {
        MSG_Q_ID pMsgQ;
        char msg[ 40 ];

        sprintf(msg,"relay _Autoproc sampleid %s", sample[pcs].barcode);
        pMsgQ = openMsgQ("Expproc");
        if ( pMsgQ != NULL) {
            sendMsgQ(pMsgQ, msg, strlen(msg), MSGQ_NORMAL, WAIT_FOREVER);
            closeMsgQ(pMsgQ);
            DPRINT1(0, "hermes_put_sample: SampleId Msge Sent: '%s'\n", msg);
        }
        else {
            DPRINT(0, "hermes_put_sample: Unable to open Expproc msgQ\n");
        }
    }

    delete_timer(sample[pcs].pcsDoneTimer);
    sample[pcs].pcsDoneTimer = NULL;
    DeleteSchedItem(pcs, "toMag");

    sampleInMagnet = smp;
    sampleInMagPcsSlot = pcs+1;

    DisplayTimers();
    DisplaySchedule();

    /* Update Status File */
    UpdateTurbineStatus();

    /* Start counting actual time in the Magnet */
    actualExpTimer[pcs].timeLeft = 0;
    actualExpTimer[pcs].valid = 1;

    reportRobotStat( 0 );

    /*
     * Sample is in the Magnet!
     * Now it's time to clear the extraction station
     *  and do any post-prep on the returned sample.
     */
    hermes_clear_extraction_station();

    /*
     * Keep things moving, make sure next sample
     *   is moved up if it hasn't started being
     *   processed yet.
     */

    DisplayTimers();
    DisplaySchedule();

    /* Update Status File */
    UpdateTurbineStatus();

    check_timers();

    return HRM_SUCCESS;
}

void StartBatch()
{
    int result;

    if (!scheduler_initialized)
        init_scheduler();

    if (readSettings == 1)
        read_settings();

    if (clearCompletionList == 1)
        ClearCompletionList();

    UpdateTurbineStatus();

    DisableEStop();

    if (useGilson) {
        char path[256], *sysFname = getenv("vnmrsystem");

        /* Read Rack File */
        if (sysFname == NULL) {
            DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
        }
        else {
            DPRINT(1,"Reading racksetup_768AS file...\n");
            sprintf(path, "%s%s", sysFname, "/asm/racksetup_768AS");
            if (readRacks(path) != 0) {
                errLogRet(LOGOPT, debugInfo,
                          "\'%s\': Rack Initialization Failed\n", path);
            }
            else {
                FILE *stream;
                int gilId=22, pumpId=3;

                /* Try to read gilson and pump GSIOC ids from settings file */
                sprintf(path, "%s%s", sysFname, "/asm/info/liqhandlerInfo");
                stream = fopen(path, "r");
                if (stream != NULL) {
                    if (fscanf(stream, "%d %d\n", &gilId, &pumpId) != 2) {
                        gilId = 22;
                        pumpId = 3;
                    }
                }
                else {
                    DPRINT3(0, "\r\nRead GSIOC address from %s file: "
                        "Gilson Id=%d, Pump Id=%d\r\n", path, gilId, pumpId);
                }

                /* Initialize i/o channel & home gilson */
                if (initGilson215("HRM_GILSON", gilId, 0, pumpId) == -1) {
                    errLogRet(LOGOPT, debugInfo,
                              "Gilson Initialization Failed.\n");
                }
                else {
                    /*
                     * If any Axis or pump is not powered then
                     *   reset and home gilson
		     * gilPowerUp();
                     */
                }
            }
        }
    }

    /* Start Batch with the Robot in the Home Position */
    if ((result = RobotStartup(1)) != HRM_SUCCESS) {

        DPRINT1(0, "Roboproc encountered error %d while sending "
                "startup command to robot.  May need to power-"
                "cycle the robot and clear the system manually "
                "to recover.\r\n", result);

        robotError = convert_robot_errorcode(result, NOTRANSPORT);
    }

    newBatch = 0;
}

void EndBatch()
{
    DisableEStop();
    UpdateCompletionList();
    clearCompletionList = 1;
    readSettings = 1;
    newBatch = 1;
}

