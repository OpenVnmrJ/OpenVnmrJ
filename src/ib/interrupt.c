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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "interrupt.h"

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

static int enable_counter;
static int interrupt_signal;	/* Flag for interrupt occurred */
static int signum;		/* Signal to use for interrupt */
static unsigned long major_cmd;
static int minor_cmd;

/************************************************************************
*                                                                       *
*  This routine will be called when the process receives a signal.	*
*									*/
static void
interrupt_receive(Notify_client, int, Notify_signal_mode)
{
    /* Note that when 'interrupt_signal' is set, it means that the program
     * is not ready to receive an interrupt.  The 'interrupt_signal' should
     * be FALSE if it is ready to receive interrupt.  (Note that the program 
     * should call interrupt_begin() before it can receive an interrupt.)
     */
    interrupt_signal = TRUE;
}

/* Must be called before any of the other routines will work */
void
interrupt_register(Notify_client client,
		   int major_command,
		   int minor_command,
		   int signal_num)
{
    major_cmd = (unsigned long)major_command;
    minor_cmd = minor_command;
    signum = signal_num;
    notify_set_signal_func(client,
			   (Notify_func)interrupt_receive,
			   signum,
			   NOTIFY_ASYNC);
    enable_counter = 0;
}

/* This routine must be called before checking for interrupt
 * (or at the start of the function which checks for interrupts). */
void
interrupt_begin()
{
    if (signum){
	Message::msg_send(major_cmd, minor_cmd, "%u %u", getpid(), signum);
	interrupt_signal = FALSE;
	enable_counter++;
    }
}

/* Returns TRUE if an interrupt has occurred */
int
interrupt(void)
{
    return interrupt_signal;
}

/* This routine must be called to ignore an interrupt--typically at the
 * end of the function (after calling interrupt_begin). */
void
interrupt_end()
{
    if (signum && enable_counter == 0 || --enable_counter == 0){
	Message::msg_send(major_cmd, minor_cmd, "%u %u", getpid(), 0);
	interrupt_signal = TRUE;
    }
}
