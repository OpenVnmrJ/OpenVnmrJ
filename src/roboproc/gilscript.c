/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*#define _POSIX_SOURCE  defined when source is to be POSIX-compliant */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>

#include "errLogLib.h"
#include "gilsonObj.h"
#include "rackObj.h"
#include "gilfuncs.h"
#include "termhandler.h"
#include "iofuncs.h"

extern int bypassInit;
extern GILSONOBJ_ID pGilObjId;
extern void setupAbortRobot();
extern void start_timer();

/******************* TYPE DEFINITIONS AND CONSTANTS *************************/
#define MAXPATHL 256

/************************** GLOBAL VARIABLES ********************************/
extern int   AbortRobo;
char  systemdir[MAXPATHL];       /* vnmr system directory */
ioDev *smsDevEntry = NULL;
int   smsDev = -1;
int   verbose = 1;

/**************************** STATIC GLOBALS ********************************/
static char path[MAXPATHL];

int main (int argc, char *argv[])
{
    char *tmpptr, *sysFname = getenv("vnmrsystem");
    FILE *stream;
    int result;
    int gilId=22, pumpId=3;
    extern int InterpScript(char *tclScriptFile);
    extern void TclDelete();

    if (argc != 4)
    {
        DPRINT1(0,
            "usage:  %s <devicename> <bypassInit> [script_file]\n",
                argv[0]);
        exit(1);
    }

    setupAbortRobot();
    start_timer();

    bypassInit = atoi(argv[2]);

    /* initialize environment parameter vnmrsystem value */
    tmpptr = getenv("vnmrsystem");      /* vnmrsystem */
    if (tmpptr != (char *) 0) {
        strcpy(systemdir,tmpptr);       /* copy value into global */
    }
    else {   
        strcpy(systemdir,"/vnmr");      /* use /vnmr as default value */
    }

    DPRINT1(-1, "Init Device: %s\n", argv[1]);

    /* Read Gilson Address from Settings File */
    if (sysFname == NULL) {
        DPRINT(0, "\r\nEnvironment variable vnmrsystem not set!\r\n");
    }
    else {
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
    }

    /*
     * Convert old style comm description to new style:
     *   Old Style: /dev/term/a
     *   New Style: GIL_TTYA
     */
    if ( (!strcmp(argv[1], "/dev/term/a")) ||
         (!strcmp(argv[1], "/dev/ttya")) )
        initGilson215("GIL_TTYA", 22, 29, 3);

    else if ( (!strcmp(argv[1], "/dev/term/b")) ||
              (!strcmp(argv[1], "/dev/ttyb")) )
        initGilson215("GIL_TTYB", 22, 29, 3);

    else {
        initGilson215(argv[1], gilId, 0, pumpId);
    }

    if (pGilObjId == NULL)
    {
        DPRINT1(0,"\nFailure to initialize Gilson '%s'\n", argv[1]);
        DPRINT(0,"\nCheck that the i/o cable is properly attached.\n");
        DPRINT1(0,"\n%s Aborted.\n", argv[0]);
        exit(1);
    }

    /* Read in Rack Definitions */
    initRacks();
    sprintf(path,"%s%s", systemdir, "/asm/racksetup");
    if (!strncmp(argv[1], "HRM", 3))
        sprintf(path,"%s%s", systemdir, "/asm/racksetup_768AS");
    if ( readRacks(path) ) {
        DPRINT1(-1, "'%s'  Rack Initialization Failed\n", path);
    }

    /* Finally... execute the script */
    result = InterpScript(argv[3]);

    /* Free memory */
    gilsonDelete(pGilObjId);
    TclDelete();

    /* Exit value is the return value of InterpScript() */
    exit(result);
}


static void sigalrm_irpt()
{
    sigset_t qmask;
    struct sigaction sigalrm_action;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    sigaction( SIGALRM, &sigalrm_action, NULL );
}


void start_timer()
{
    sigset_t qmask, orig_sigmask;
    struct sigaction sigalrm_action, orig_sigalrm_action;
    struct itimerval new_itimer, orig_itimer;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    if (sigaction( SIGALRM, &sigalrm_action, &orig_sigalrm_action ) != 0) {
      perror("sigaction error");
    }
    new_itimer.it_interval.tv_sec = 1;
    new_itimer.it_interval.tv_usec = 0;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;
    if (setitimer( ITIMER_REAL, &new_itimer, &orig_itimer ) != 0) {
      perror("setitimer error");
    }
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    if (sigprocmask( SIG_UNBLOCK, &qmask, &orig_sigmask ) != 0) {
      perror("sigprocmask error");
    }
}
