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

extern void ShutDownProc();
extern char ProcName[];

// #define MASTER 2
// #define DONT_TRUNCATE 0
// static char wall[256];
// static char wallpath[256] = { '\0' };
// typedef void (*PFV)();		/* Pointer to Function returning a Void */

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
    // FlushMasterLog(DONT_TRUNCATE);	/* write Master Log out to disk */

    /* abort any acquisitions in progress if possible*/
 
    /* shellcmd(wall); */
    //system(wall);	/* inform users system wide of failure */
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
    // FlushMasterLog(DONT_TRUNCATE);	/* write Master Log out to disk */

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
//     sprintf(wall,
//       "echo '%s: Segmentation Violation, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
Ill_instr()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Illegal Instruction.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Illegal Instruction.\n"); */
//     sprintf(wall,
//       "echo '%s: Illegal Instruction, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
FPexcept()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Arithmetic Exception.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Arithmetic Exception.\n"); */
//     sprintf(wall,
//       "echo '%s: Arithmetic Exception, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
BusErr()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Bus Error.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Bus Error.\n"); */
//     sprintf(wall,
//       "echo '%s: Bus Error, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}

static void
CpuLim()
{ 
    errLogRet(ErrLogOp,debugInfo, "%s: Exceeded CPU Time Limit.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Exceeded CPU Time Limit.\n"); */
//     sprintf(wall, 
//     "echo '%s: Exceeded CPU Time Limit, Core Dumped.' | %s",ProcName,wallpath); 
    SpanishInquisition(); 
}
static void
FsLim()
{  
    errLogRet(ErrLogOp,debugInfo, "%s: Exceeded File Size Limit.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Exceeded File Size Limit.\n");  */
//     sprintf(wall, 
//     "echo '%s: Exceeded File Size Limit, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();  
} 
static void
SigQuit()
{
    errLogRet(ErrLogOp,debugInfo, "%s: Quit Signal.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Quit Signal.\n");*/
//     sprintf(wall,
//     "echo '%s: Quit Signal, Core Dumped.' | %s",ProcName,wallpath);
    SpanishInquisition();
}
static void
Trap()
{ 
    errLogRet(ErrLogOp,debugInfo, "%s: Trace Trap.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Trace Trap.\n"); */
//     sprintf(wall, 
//     "echo '%s: Trace Trap, Core Dumped.' | %s",ProcName,wallpath);
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
//     sprintf(wall,  
//     "echo '%s: EMT Trap, Core Dumped.' | %s",ProcName,wallpath);  
    SpanishInquisition();  
}
#endif
static void
SYStrap()
{    
    errLogRet(ErrLogOp,debugInfo, 
		"%s: Bad Argument to a System Call.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Bad Argument to a System Call.\n"); */
//     sprintf(wall,  
//     "echo '%s: Bad Argument to a System Call, Core Dumped.' | %s",
//        ProcName,wallpath);   
    SpanishInquisition();   
}

#ifdef XXX
static void
ResLost()
{    
    errLogRet(ErrLogOp,debugInfo, "%s: Resource Lost Exception.\n",ProcName);
    /* logprint(MASTER,0,AcqProcDied,", Resource Lost Exception.\n");    */
//     sprintf(wall,  
//     "echo '%s: Resource Lost Exception, Core Dumped.' | %s",
// 	ProcName,wallpath);   
    SpanishInquisition();   
}
#endif

/*
 * The threaded varient this routine is called from a sigwait returning
 * for signo the app doesn't want to handle. Most of these are terminating or'
 * fatal type of signals.  Some routine are call to clean up prior to processes
 * termination
 *
 */
void  excepthandler(int signo)
  {
      switch(signo)
      {
	case SIGINT:
	case SIGTERM:
	     terminated();
             break;
	case SIGSEGV: /* --- set up segmentation violation handler --- */
	     Segment();
             break;
	case SIGILL:  /* --- set up illegal instruction handler --- */
	     Ill_instr();
             break;
	case SIGFPE:  /* --- set up arithmetic exception handler--- */
	     FPexcept();
             break;
	case SIGBUS:  /* --- set up buss error exception handler--- */
	     BusErr();
             break;
	case SIGXCPU:  /* --- set up cpu limit error exception handler--- */
	     CpuLim();
             break;
	case SIGXFSZ:  /* --- set up file size limit exception handler--- */
	     FsLim();
             break;
	case SIGQUIT:  /* --- set up quit exception handler--- */
	     SigQuit();
             break;
	case SIGTRAP:  /* --- set up Trace Trap exception handler--- */
	     Trap();
             break;
#ifndef LINUX
	case SIGEMT:  /* --- set up EMT Trap exception handler--- */
	     EMTtrap();
             break;
#endif
	case SIGSYS:  /* --- set up bad argument to system call exception handler--- */
	     SYStrap();
             break;
        default:
    	      errLogRet(ErrLogOp,debugInfo, "%s: Unknown signal ignored: %d\n",ProcName,signo);
             break;
      }
  }

