/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <signal.h>
#include "abort.h"

/*-------------------------------------------------------------------
|
|	terminated()   we have been told to exit, clean things up and die.
|
+---------------------------------------------------------------------*/
static
void terminated()
{
    abort_message("PSG was User Terminated..");
}
/*--------------------------------------------------------------------------------
|
|  Fatal Error signal catchers, core dump after cleaning up & notifing all users
|
+--------------------------------------------------------------------------------*/
static
void Segment()
{
    text_error("PSG: Segmentation Violation, Core Dumped.");
    kill(getpid(),SIGIOT);
}
static
void Ill_instr()
{
    text_error("PSG: Illegal Instruction, Core Dumped. Try recompiling sequence");
    kill(getpid(),SIGIOT);
}
static
void FPexcept()
{
    text_error("PSG: Arithmetic Exception, Core Dumped. May be division by 0");
    kill(getpid(),SIGIOT);
}
static
void BusErr()
{
    text_error("PSG: Bus Error, Core Dumped.");
    kill(getpid(),SIGIOT);
}

static
void CpuLim()
{ 
    text_error("PSG: Exceeded CPU Time Limit, Core Dumped."); 
    kill(getpid(),SIGIOT);
}
static
void FsLim()
{  
    text_error("PSG: Exceeded File Size Limit, Core Dumped.");
    kill(getpid(),SIGIOT);
} 
static
void SigQuit()
{
    text_error("PSG: Quit Signal, Core Dumped.");
    kill(getpid(),SIGIOT);
}
static
void Trap()
{ 
    text_error("PSG: Trace Trap, Core Dumped.");
    kill(getpid(),SIGIOT);
}
void PIPEtrap()
{   
    /* Ignore pipe error */
}

static
void SYStrap()
{    
    text_error("PSG: Bad Argument to a System Call, Core Dumped.");   
    kill(getpid(),SIGIOT);
}

/*-------------------------------------------------------------------------
|
|   Setup the exception handlers for fatal type errors 
|    
+--------------------------------------------------------------------------*/
void setupsignalhandler()
{
    sigset_t		qmask;
    struct sigaction	intquit;
    struct sigaction	segquit;
 
    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    intquit.sa_mask = qmask;
    intquit.sa_handler = terminated;
    intquit.sa_flags = 0;
    sigaction(SIGINT,&intquit,0L);

    /* --- set up segmentation violation handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGSEGV );
    segquit.sa_mask = qmask;
    segquit.sa_handler = Segment;
    segquit.sa_flags = 0;
    sigaction(SIGSEGV,&segquit,0L);

    /* --- set up illegal instruction handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGILL );
    segquit.sa_mask = qmask;
    segquit.sa_handler = Ill_instr;
    segquit.sa_flags = 0;
    sigaction(SIGILL,&segquit,0L);

    /* --- set up arithmetic exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGFPE );
    segquit.sa_mask = qmask;
    segquit.sa_handler = FPexcept;
    segquit.sa_flags = 0;
    sigaction(SIGFPE,&segquit,0L);

    /* --- set up buss error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGBUS );
    segquit.sa_mask = qmask;
    segquit.sa_handler = BusErr;
    segquit.sa_flags = 0;
    sigaction(SIGBUS,&segquit,0L);

    /* --- set up cpu limit error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGXCPU );
    segquit.sa_mask = qmask;
    segquit.sa_handler = CpuLim;
    segquit.sa_flags = 0;
    sigaction(SIGXCPU,&segquit,0L);

    /* --- set up file size limit error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGXFSZ );
    segquit.sa_mask = qmask;
    segquit.sa_handler = FsLim;
    segquit.sa_flags = 0;
    sigaction(SIGXFSZ,&segquit,0L);

    /* --- set up quit signal exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGQUIT );
    segquit.sa_mask = qmask;
    segquit.sa_handler = SigQuit;
    segquit.sa_flags = 0;
    sigaction(SIGQUIT,&segquit,0L);

    /* --- set up Trace Trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGTRAP );
    segquit.sa_mask = qmask;
    segquit.sa_handler = Trap;
    segquit.sa_flags = 0;
    sigaction(SIGTRAP,&segquit,0L);

    /* --- set up PIPE trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGPIPE );
    segquit.sa_mask = qmask;
    segquit.sa_handler = PIPEtrap;
    segquit.sa_flags = 0;
    sigaction(SIGPIPE,&segquit,0L);

    /* --- set up bad argument to system call exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGSYS );
    segquit.sa_mask = qmask;
    segquit.sa_handler = SYStrap;
    segquit.sa_flags = 0;
    sigaction(SIGSYS,&segquit,0L);
}
