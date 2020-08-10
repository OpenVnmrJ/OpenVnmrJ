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
#include <string.h>
#include <signal.h>

#include "errLogLib.h"

extern char ProcName[];
extern void ShutDownProc();

#define MASTER 2
#define DONT_TRUNCATE 0
static char wall[256*2];
static char wallpath[256] = "";

/*-------------------------------------------------------------------
|
|	SpanishInquisition():   clean up and core dump
|
+---------------------------------------------------------------------*/
static void
SpanishInquisition()
{
    ShutDownProc();

    /* terminate Sendproc, Recvproc, Procproc, ????? */

    /* write out buffer */
    /* FlushMasterLog(DONT_TRUNCATE); */	/* write Master Log out to disk */

    /* abort any acquisitions in progress if possible*/
 
    /* shellcmd(wall); */
    /*system(wall); */	/* inform users system wide of failure */
    abort();
}
/*-------------------------------------------------------------------
|
|	terminated()   we have been told to exit, clean things up and die.
|
+---------------------------------------------------------------------*/
static void
terminated()
{
    /* logprint(MASTER,0,AcqProcDied,"\n"); */

    ShutDownProc();  /* This routine will tiddy up, any loose ends */

    /* Terminate Sendproc, Recvproc, Procproc ???? */
 
    /* write out buffer */
    /* FlushMasterLog(DONT_TRUNCATE); */	/* write Master Log out to disk */

    /* sprintf(wall,
      "echo 'Acquisition Daemon Terminated.' | %s",wallpath);
     */
    /*system(wall);*/
    /* shellcmd(wall); */
    exit(1);
}
/*--------------------------------------------------------------------------------
|
|  Fatal Error signal catchers, core dump after cleaning up & notifing all users
|
+--------------------------------------------------------------------------------*/
static void
Segment()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Segmentation Violation\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Segmentation Violation\n"); */
    sprintf(wall,
      "echo '%s: Segmentation Violation, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
Ill_instr()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Illegal Instruction.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Illegal Instruction.\n"); */
    sprintf(wall,
      "echo '%s: Illegal Instruction, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
FPexcept()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Arithmetic Exception.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Arithmetic Exception.\n"); */
    sprintf(wall,
      "echo '%s: Arithmetic Exception, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
BusErr()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Bus Error.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Bus Error.\n"); */
    sprintf(wall,
      "echo '%s: Bus Error, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}

static void
CpuLim()
{ 
    errLogRet(ErrLogOp,debugInfo, "%s: Exceeded CPU Time Limit.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Exceeded CPU Time Limit.\n"); */
    sprintf(wall, 
    "echo '%s: Exceeded CPU Time Limit, Core Dumped.' | %s",ProcName,wallpath); 
    SpanishInquisition(); 
}
static void
FsLim()
{  
    errLogRet(ErrLogOp,debugInfo, "%s: Exceeded File Size Limit.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Exceeded File Size Limit.\n");  */
    sprintf(wall, 
    "echo '%s: Exceeded File Size Limit, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();  
} 
static void
SigQuit()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Quit Signal.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Quit Signal.\n");*/
    sprintf(wall,
    "echo '%s: Quit Signal, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
Trap()
{ 
    errLogRet(ErrLogOp,debugInfo, "%s: Trace Trap.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Trace Trap.\n"); */
    sprintf(wall, 
    "echo '%s: Trace Trap, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
/*--------------- Commented Out ------------------------
static void
IOTtrap()
{  
    logprint(MASTER,0,AcqProcDied,", IOT Trap.\n"); 
    sprintf(wall, 
    "echo 'Acquisition Daemon: IOT Trap, Core Dumped.' | %s",wallpath); 
    SpanishInquisition(); 
}
+----------------------------------------------------*/
#ifndef LINUX
static void
EMTtrap()
{   
    errLogRet(ErrLogOp,debugInfo, "%s: EMT Trap.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", EMT Trap.\n");   */
    sprintf(wall,  
    "echo '%s: EMT Trap, Core Dumped.' | %s",ProcName,wallpath);  
    SpanishInquisition();  
}
#endif
static void
SYStrap()
{    
    errLogRet(ErrLogOp,debugInfo, 
		"%s: Bad Argument to a System Call.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Bad Argument to a System Call.\n"); */
    sprintf(wall,  
    "echo '%s: Bad Argument to a System Call, Core Dumped.' | %s",
       ProcName,wallpath);   
    SpanishInquisition();   
}
#ifdef XXX
static void
ResLost()
{    
    errLogRet(ErrLogOp,debugInfo, "%s: Resource Lost Exception.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Resource Lost Exception.\n");    */
    sprintf(wall,  
    "echo '%s: Resource Lost Exception, Core Dumped.' | %s",
	ProcName,wallpath);   
    SpanishInquisition();   
}
#endif
/*-------------------------------------------------------------------------
|
|   Setup the exception handlers for fatal type errors 
|    
+--------------------------------------------------------------------------*/
void setupexcepthandler()
{
    sigset_t		qmask;
    struct sigaction	intquit;
    struct sigaction	segquit;

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    intquit.sa_handler = terminated;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGINT,&intquit,0L);

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGTERM );
    intquit.sa_handler = terminated;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGTERM,&intquit,0L);

    /* --- set up segmentation violation handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGSEGV );
    segquit.sa_handler = Segment;
    segquit.sa_mask = qmask;
    sigaction(SIGSEGV,&segquit,0L);

    /* --- set up illegal instruction handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGILL );
    segquit.sa_handler = Ill_instr;
    segquit.sa_mask = qmask;
    sigaction(SIGILL,&segquit,0L);

    /* --- set up arithmetic exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGFPE );
    segquit.sa_handler = FPexcept;
    segquit.sa_mask = qmask;
    sigaction(SIGFPE,&segquit,0L);

    /* --- set up buss error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGBUS );
    segquit.sa_handler = BusErr;
    segquit.sa_mask = qmask;
    sigaction(SIGBUS,&segquit,0L);

    /* --- set up cpu limit error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGXCPU );
    segquit.sa_handler = CpuLim;
    segquit.sa_mask = qmask;
    sigaction(SIGXCPU,&segquit,0L);

    /* --- set up file size limit error exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGXFSZ );
    segquit.sa_handler = FsLim;
    segquit.sa_mask = qmask;
    sigaction(SIGXFSZ,&segquit,0L);

    /* --- set up quit signal exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGQUIT );
    segquit.sa_handler = SigQuit;
    segquit.sa_mask = qmask;
    sigaction(SIGQUIT,&segquit,0L);

    /* --- set up Trace Trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGTRAP );
    segquit.sa_handler = Trap;
    segquit.sa_mask = qmask;
    sigaction(SIGTRAP,&segquit,0L);

    /* not needed for SUN 3s */
    /* --- set up IOT trap exception handler --- */
    /*
     * segquit.sv_handler = IOTtrap;
     * segquit.sv_mask = sigmask(SIGIOT);
     * segquit.sv_onstack = 0;
     * sigvec(SIGIOT,&segquit,0L);
     */

#ifndef LINUX
    /* --- set up EMT trap exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGEMT );
    segquit.sa_handler = EMTtrap;
    segquit.sa_mask = qmask;
    sigaction(SIGEMT,&segquit,0L);
#endif

    /* --- set up bad argument to system call exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGSYS );
    segquit.sa_handler = SYStrap;
    segquit.sa_mask = qmask;
    sigaction(SIGSYS,&segquit,0L);

    /* --- set up bad argument to system call exception handler --- */
    /*sigemptyset( &qmask );
    sigaddset( &qmask, SIGLOST );
    segquit.sa_handler = ResLost;
    segquit.sa_mask = qmask;
    sigaction(SIGLOST,&segquit,0L);*/
}
