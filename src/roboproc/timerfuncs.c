/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>

#include "errLogLib.h"
#include "timerfuncs.h"
#include "iofuncs.h"

int                     timer_went_off;

static struct itimerval orig_itimer;
static struct sigaction orig_sigalrm;
static sigset_t         orig_sigmask;

/* Interix/SFU does not support nanosleep so here we make up are own using usleep */

#ifdef __INTERIX
#define nanosleep(x, y) usleep( ((x)->tv_sec)*1000000 + ((x)->tv_nsec)/1000 )
#endif /* __INTERIX */

/*****************************************************************************
 *  Remember!  These programs may be running in response to a SIGALRM
 *   or may be running on a system with a separate interval timer.
 ****************************************************************************/

/*****************************************************************************
                          s i g a l r m _ i r p t
    Description:
    SIGALRM interrupt handler (used for timeout).

*****************************************************************************/
static void
sigalrm_irpt()
{
    sigset_t         qmask;
    struct sigaction sigalrm_action;

    /*  Re-register sigalrm_irpt as the SIGALRM interrupt handler
     *  to prevent process termination due to an Alarm Clock.
     */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    sigaction( SIGALRM, &sigalrm_action, NULL );

    timer_went_off = 1;
}

/*****************************************************************************
                       s e t u p _ m s _ t i m e r
    Description:

*****************************************************************************/
int setup_ms_timer( ms_interval )
int ms_interval;
{
    sigset_t         qmask;
    struct sigaction sigalrm_action;
    struct itimerval new_itimer;
 
    if (ms_interval < 1) {
        return( -1 );
    }

    /* 
     * Set up signal handler
     * Necessary to assert that the system call (read)
     *   is NOT to be restarted.
     * Required for the timeout to work.
     */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigalrm_action.sa_handler = sigalrm_irpt;
    sigalrm_action.sa_mask = qmask;
    sigalrm_action.sa_flags = 0;
    if (sigaction( SIGALRM, &sigalrm_action, &orig_sigalrm ) != 0) {
        perror("sigaction error");
        return( -1 );
    }

    /*
     * Specify a Timer to go off in a certain number of
     *   milliseconds and assert SIGALRM
     */
    new_itimer.it_value.tv_sec = ms_interval / 1000;
    new_itimer.it_value.tv_usec = (ms_interval % 1000) * 1000;
    new_itimer.it_interval.tv_sec = 0;
    new_itimer.it_interval.tv_usec = 0;
 
    if (setitimer( ITIMER_REAL, &new_itimer, &orig_itimer ) != 0) {
        perror("setitimer error");
        return( -1 );
    }

    /*
     *  Since this process may have SIGALRM blocked (for example, it is
     *  running a SIGALRM interrupt program), it is necessary to remove
     *  SIGALRM from the mask of blocked signals.
     */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    if (sigprocmask( SIG_UNBLOCK, &qmask, &orig_sigmask ) != 0) {
        perror("sigprocmask error");
        return( -1 );
    }

    return( 0 );
}

/*****************************************************************************
                  c l e a n u p _ f r o m _ t i m e o u t
    Description:
    Restore all values saved (and modified) in setup_ms_timeout.

*****************************************************************************/
int cleanup_from_timeout()
{
    sigprocmask( SIG_SETMASK, &orig_sigmask, NULL );    
    sigaction( SIGALRM, &orig_sigalrm, NULL );
    setitimer( ITIMER_REAL, &orig_itimer, NULL );
}
 
/*****************************************************************************
                          d e l a y A w h i l e
    Description:

*****************************************************************************/
void delayAwhile(int time)
{
    if ((smsDevEntry != NULL) && (strncmp(smsDevEntry->devName, "HRM_", 4))) {
        timer_went_off = 0;
        setup_ms_timer(time*1000);  /* in msec */
        while (!timer_went_off)
            sigpause(0);
        cleanup_from_timeout();
    }
    else {
        struct timespec rmtp, rqtp;
        int stat;
        extern int AbortRobo;

        memset((void *)&rmtp, 0, sizeof(rmtp));
        memset((void *)&rqtp, 0, sizeof(rmtp));
        rqtp.tv_sec = time;

        while (1) {
            stat = nanosleep(&rqtp, &rmtp);

            /* Time delay is complete */
            if (stat == 0)
                return;

            /* Check for SIGUSR2 / Abort signal */
            else if (AbortRobo)
                return;

            /* Ignore any other signals */
            else if ((stat == -1) && (errno == EINTR)) {
                memcpy((void *)&rqtp, (void *)&rmtp, sizeof(rqtp));
                memset((void *)&rmtp, 0, sizeof(rmtp));
            }

            /* Error */
            else
                return;
        } 
    }
}

/*****************************************************************************
                           d e l a y M s e c
    Description:

*****************************************************************************/
void delayMsec(int time)
{
    if ((smsDevEntry != NULL) && (strncmp(smsDevEntry->devName, "HRM_", 4))) {
        timer_went_off = 0;
        setup_ms_timer(time);  /* in msec */
        while (!timer_went_off)
            sigpause(0);
        cleanup_from_timeout();
    }
    else {
        struct timespec rmtp, rqtp;
        int stat;
        extern int AbortRobo;

        memset((void *)&rmtp, 0, sizeof(rmtp));
        memset((void *)&rqtp, 0, sizeof(rmtp));
        rqtp.tv_sec = (time / 1000);
        rqtp.tv_nsec = (time % 1000) * 1000000;
        while (1) {
            stat = nanosleep(&rqtp, &rmtp);

            /* Time delay is complete */
            if (stat == 0)
                return;

            /* Check for SIGUSR2 / Abort signal */
            else if (AbortRobo)
                return;

            /* Ignore any other signals */
            else if ((stat == -1) && (errno == EINTR)) {
                memcpy((void *)&rqtp, (void *)&rmtp, sizeof(rqtp));
                memset((void *)&rmtp, 0, sizeof(rmtp));
            }

            /* Error */
            else
                return;
        } 
    }
}
