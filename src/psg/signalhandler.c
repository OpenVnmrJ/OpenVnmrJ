/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

extern void psg_abort(int error);

/*-------------------------------------------------------------------
|
|	terminated()   we have been told to exit, clean things up and die.
|
+---------------------------------------------------------------------*/
static
void terminated()
{
    fprintf(stdout,
      "PSG was User Terminated..  \n");
    fflush(stdout);
    psg_abort(1);
}
/*--------------------------------------------------------------------------------
|
|  Fatal Error signal catchers, core dump after cleaning up & notifing all users
|
+--------------------------------------------------------------------------------*/
static
void Segment()
{
    fprintf(stdout,
      "'PSG: Segmentation Violation, Core Dumped.'\n");
    fprintf(stdout,
      "'Check for: Non NULL terminated Attribute-List Functions, or\n");
    fprintf(stdout,
      "            Pointers or Array indices past allocated memory.'\n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
static
void Ill_instr()
{
    fprintf(stdout,
      "'PSG: Illegal Instruction, Core Dumped.'\n");
    fprintf(stdout,
      "'Recompile and try again.'\n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
static
void FPexcept()
{
    fprintf(stdout,
      "'PSG: Arithmetic Exception, Core Dumped.' \n");
    fprintf(stdout,
      "'Did You divide by ZERO ?'\n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
static
void BusErr()
{
    fprintf(stdout,
      "'PSG: Bus Error, Core Dumped.' \n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}

static
void CpuLim()
{ 
    fprintf(stdout, 
    "'PSG: Exceeded CPU Time Limit, Core Dumped.' \n"); 
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
static
void FsLim()
{  
    fprintf(stdout, 
    "'PSG: Exceeded File Size Limit, Core Dumped.' \n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
} 
static
void SigQuit()
{
    fprintf(stdout,
    "'PSG: Quit Signal, Core Dumped.' \n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
static
void Trap()
{ 
    fprintf(stdout, 
    "'PSG: Trace Trap, Core Dumped.' \n");
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
/*--------------- Commented Out ------------------------
static
void IOTtrap()
{  
    fprintf(stdout, 
    "'PSG: IOT Trap, Core Dumped.' \n"); 
}
+----------------------------------------------------*/
void PIPEtrap()
{   
    /* Ignore pipe error */
}

#ifdef SOLARIS
static
void EMTtrap()
{   
    fprintf(stdout,  
    "'PSG: EMT Trap, Core Dumped.' \n");  
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
#endif

static
void SYStrap()
{    
    fprintf(stdout,  
    "'PSG: Bad Argument to a System Call, Core Dumped.' \n");   
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
/*--------------- Commented Out ------------------------
static
void ResLost()
{    
    fprintf(stdout,  
    "'PSG: Resource Lost Exception, Core Dumped.' \n");   
    fflush(stdout);
    kill(getpid(),SIGIOT);
}
+----------------------------------------------------*/

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

    /* not needed for SUN 3s */
    /* --- set up IOT trap exception handler --- */
    /*
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGIOT );
    segquit.sa_mask = qmask;
    segquit.sa_handler = IOTtrap;
    segquit.sa_flags = 0;
    sigaction(SIGIOT,&segquit,0L);
    */

    /* --- set up PIPE trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGPIPE );
    segquit.sa_mask = qmask;
    segquit.sa_handler = PIPEtrap;
    segquit.sa_flags = 0;
    sigaction(SIGPIPE,&segquit,0L);
#ifdef SOLARIS
    /* --- set up EMT trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGEMT );
    segquit.sa_mask = qmask;
    segquit.sa_handler = EMTtrap;
    segquit.sa_flags = 0;
    sigaction(SIGEMT,&segquit,0L);
#endif

    /* --- set up bad argument to system call exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGSYS );
    segquit.sa_mask = qmask;
    segquit.sa_handler = SYStrap;
    segquit.sa_flags = 0;
    sigaction(SIGSYS,&segquit,0L);

    /* --- set up bad argument to system call exception handler --- */
    /*sigemptyset( &qmask );
    sigaddset( &qmask, SIGLOST );
    segquit.sa_mask = qmask;
    segquit.sa_handler = ResLost;
    segquit.sa_flags = 0;
    sigaction(SIGLOST,&segquit,0L);*/
}
