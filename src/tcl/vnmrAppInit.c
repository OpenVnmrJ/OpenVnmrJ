/* 
 * tkAppInit.c --
 *
 *	Provides a default version of the Tcl_AppInit procedure for
 *	use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tk.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

#ifdef TK_TEST
EXTERN int		Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TK_TEST */


static char VnmrID[128];
static char MagicVar[128];
static int listArgc = 0;
static char **listArgv = NULL;
static int VnmrPid = -1;

	/* ARGSUSED */
int
Vnmr_InitCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" vnmraddr vnmrsystem\"", (char *) NULL);
	return TCL_ERROR;
    }
    strcpy(VnmrID,&argv[1][1]);
    VnmrID[strlen(VnmrID)-1] = '\0';
    VnmrPid = initVnmrComm(VnmrID);
    openVnmrInfo(argv[2]);
    return TCL_OK;
}

	/* ARGSUSED */
int
Vnmr_ActiveCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int err;

    if (argc != 1)
    {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option \"", (char *) NULL);
	return TCL_ERROR;
    }
    if (VnmrPid == -1)
    {
	sprintf(interp->result, "%d", 0);
	return TCL_OK;
    }
    err = kill(VnmrPid, 0); /* Test for existence */
    if (err == -1 && errno == ESRCH)
	sprintf(interp->result, "%d", 0);
    else
	sprintf(interp->result, "%d", 1);
    return TCL_OK;
}

void Vnmr_UnmapInfo(dummy)
    ClientData dummy;                   /* Not used. */
{
   closeVnmrInfo();
}

void Vnmr_UnmapVars(dummy)
    ClientData dummy;                   /* Not used. */
{
   closeTclInfo();
}

	/* ARGSUSED */
int
Vnmr_SendCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int length;
    char c;

    if (argc != 2)
    {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option \"", (char *) NULL);
	return TCL_ERROR;
    }
    sendToVnmr(argv[1]);
    return TCL_OK;
}


int VnmrVal(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *varVal();
    int index = (int) dummy;

    Tcl_SetVar2(interp, name1, name2, varVal(index), 0);
    return TCL_OK;
}

int VnmrAct(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *getMagicVarAttr();
    int type, active, size, dgroup;
    double max, min, step;
    char *varValue;
    int index = (int) dummy;

    varValue = getMagicVarAttr(index, &type, &active, &size, &dgroup,
               &max, &min, &step);
    Tcl_SetVar2(interp, name1, name2, (active) ? "y" : "n", 0);
    return TCL_OK;
}

int VnmrSize(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *getMagicVarAttr();
    int type, active, size, dgroup;
    double max, min, step;
    char *varValue;
    int index = (int) dummy;
    char dst[64];

    varValue = getMagicVarAttr(index, &type, &active, &size, &dgroup,
               &max, &min, &step);
    sprintf(dst,"%d",size);
    Tcl_SetVar2(interp, name1, name2, dst, 0);
    return TCL_OK;
}

int VnmrDgroup(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *getMagicVarAttr();
    int type, active, size, dgroup;
    double max, min, step;
    char *varValue;
    int index = (int) dummy;
    char dst[64];

    varValue = getMagicVarAttr(index, &type, &active, &size, &dgroup,
               &max, &min, &step);
    sprintf(dst,"%d",dgroup);
    Tcl_SetVar2(interp, name1, name2, dst, 0);
    return TCL_OK;
}

int VnmrMax(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *getMagicVarAttr();
    int type, active, size, dgroup;
    double max, min, step;
    char *varValue;
    int index = (int) dummy;
    char dst[64];

    varValue = getMagicVarAttr(index, &type, &active, &size, &dgroup,
               &max, &min, &step);
    Tcl_PrintDouble(interp, max, dst);
    Tcl_SetVar2(interp, name1, name2, dst, 0);
    return TCL_OK;
}

int VnmrMin(dummy, interp, name1, name2, flags)
ClientData dummy;			/* Not used. */
Tcl_Interp *interp;			/* Current interpreter. */
char *name1, *name2;
int flags;
{
    extern char *getMagicVarAttr();
    int type, active, size, dgroup;
    double max, min, step;
    char *varValue;
    int index = (int) dummy;
    char dst[64];

    varValue = getMagicVarAttr(index, &type, &active, &size, &dgroup,
               &max, &min, &step);
    Tcl_PrintDouble(interp, min, dst);
    Tcl_SetVar2(interp, name1, name2, dst, 0);
    return TCL_OK;
}

/* ARGSUSED */
int
Vnmr_MagicVars(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    char tmp[1024];
    char *val;

    if (argc != 5)
    {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" variable elements\"", (char *) NULL);
	return TCL_ERROR;
    }
    strcpy(MagicVar,argv[1]);
    val = Tcl_GetVar(interp,argv[2],0);
    sprintf(tmp,"magicvars('%s','%s','%s')\n",val,argv[3],argv[4]);
    sendToVnmr(tmp);
    if (Tcl_SplitList(interp,val, &listArgc, &listArgv) != TCL_OK)
    {
       return TCL_ERROR;
    }
    return TCL_OK;
}

/* ARGSUSED */
int
Vnmr_GetMagicVars(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    char tmp[512];
    int i;
    extern char *getMagicVarAttr();
    int VnmrVal();
    int VnmrAct();
    int VnmrSize();
    int VnmrDgroup();
    int VnmrMax();
    int VnmrMin();

    if (argc != 1)
    {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" variable elements\"", (char *) NULL);
	return TCL_ERROR;
    }
    getTclInfoFileName(tmp,VnmrID);
    readTclInfo(tmp, listArgc+1);
    for (i=0; i<listArgc; i++)
    {
       int type, active, size, dgroup;
       double max, min, step;
       char *varValue;
       char dst[64];

       varValue = getMagicVarAttr(i, &type, &active, &size, &dgroup, &max, &min, &step);
       sprintf(tmp,"%s,val",listArgv[i]);
       Tcl_SetVar2(interp,MagicVar, tmp, varValue, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrVal, (ClientData) i);

       sprintf(tmp,"%s,on",listArgv[i]);
       Tcl_SetVar2(interp,MagicVar, tmp, (active) ? "y" : "n", 0);
       Tcl_SetVar2(interp,MagicVar, tmp, dst, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrAct, (ClientData) i);

       sprintf(tmp,"%s,size",listArgv[i]);
       sprintf(dst,"%d",size);
       Tcl_SetVar2(interp,MagicVar, tmp, dst, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrSize, (ClientData) i);

       sprintf(tmp,"%s,dgroup",listArgv[i]);
       sprintf(dst,"%d",dgroup);
       Tcl_SetVar2(interp,MagicVar, tmp, dst, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrDgroup, (ClientData) i);

       sprintf(tmp,"%s,max",listArgv[i]);
       Tcl_PrintDouble(interp, max, dst);
       Tcl_SetVar2(interp,MagicVar, tmp, dst, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrMax, (ClientData) i);

       sprintf(tmp,"%s,min",listArgv[i]);
       Tcl_PrintDouble(interp, min, dst);
       Tcl_SetVar2(interp,MagicVar, tmp, dst, 0);
       Tcl_TraceVar2(interp,MagicVar, tmp, TCL_TRACE_READS,
                    VnmrMin, (ClientData) i);
    }
    return TCL_OK;
}

	/* ARGSUSED */
int
Vnmr_InfoCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int length;
    char c;

    if (argc != 2)
    {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option \"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);

    if ((c == 's') && (strncmp(argv[1], "spinOnOff", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinOnOff());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinSetSpeed", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinSetSpeed());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinUseRate", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinUseRate());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinSetRate", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinSetRate());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinSwitchSpeed", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinSwitchSpeed());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinErrorControl", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinErrorControl());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinExpControl", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinExpControl());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinSelect", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinSelect());
	return TCL_OK;
    }
    if ((c == 's') && (strncmp(argv[1], "spinner", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoSpinner());
	return TCL_OK;
    }
    if ((c == 't') && (strncmp(argv[1], "tempOnOff", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoTempOnOff());
	return TCL_OK;
    }
    if ((c == 't') && (strncmp(argv[1], "tempSetPoint", length)) == 0)
    {
	sprintf(interp->result, "%g", (double) getInfoTempSetPoint() / 10.0);
	return TCL_OK;
    }
    if ((c == 't') && (strncmp(argv[1], "tempExpControl", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoTempExpControl());
	return TCL_OK;
    }
    if ((c == 't') && (strncmp(argv[1], "tempErrorControl", length)) == 0)
    {
	sprintf(interp->result, "%d", getInfoTempErrorControl());
	return TCL_OK;
    }
    if (argc != 2)
    {
	Tcl_AppendResult(interp, "unknown option # args: should be \"", argv[0],
		" option \"", (char *) NULL);
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;			/* Number of command-line arguments. */
    char **argv;		/* Values of command-line arguments. */
{
    int dgTcl = 0;
    char *debug = (char *)getenv("vnmrwishdebug");;
    if (strstr(argv[0],"vnmrWish") != NULL)
       dgTcl = 1;
    if ((debug == NULL) && (!dgTcl))
       makeItaDaemon();
    Tk_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tk", Tk_Init, (Tcl_PackageInitProc *) NULL);

#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    Tcl_StaticPackage(interp, "Tktest", Tktest_Init, (Tcl_PackageInitProc *) NULL);

#endif /* TK_TEST */

#ifdef IRIX
    if (Tbcload_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "tbcload", Tbcload_Init,  NULL);
#endif


    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */
    Tcl_CreateCommand(interp, "vnmrinit", Vnmr_InitCmd,
                      (ClientData) NULL, Vnmr_UnmapInfo);
    Tcl_CreateCommand(interp, "vnmractive", Vnmr_ActiveCmd,
                      (ClientData) NULL, Vnmr_UnmapInfo);
    Tcl_CreateCommand(interp, "vnmrinfo", Vnmr_InfoCmd,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "vnmrsend", Vnmr_SendCmd,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "magicvars", Vnmr_MagicVars,
                      (ClientData) NULL, Vnmr_UnmapVars);
    Tcl_CreateCommand(interp, "getmagicvars", Vnmr_GetMagicVars,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);


    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    Tcl_SetVar(interp, "tcl_rcFileName", "~/.wishrc", TCL_GLOBAL_ONLY);
    return TCL_OK;
}


/**************************************************************
*
*  makeItaDaemon - Makes a daemon out of the calling process
*
*  This routine performs the necessary tasks to make a process into a 
*well behaved daemon.
*
* RETURNS:
* void 
*
*/
makeItaDaemon()
{
   int childpid, fd, strtfd;
   struct sigaction    intserv;
   sigset_t            qmask;
   void sig_child();

   strtfd = 0;	/* fd to start when closing file descriptors */


   /* if started by init via /etc/inittab then skip the detaching
      stuff, this test however maybe unreliable */
   if (getppid() == 1)
     goto skipstuff;

   /* Ignore Terminal Signals */
    sigemptyset( &qmask );
    intserv.sa_handler = SIG_IGN;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
   /*
	if not started in the background then fork and let parent exit.
	This guarantees the 1st child is not a process group
	leader.
   */
   if ( (childpid = fork()) < 0)
	perror("can't fork 1st child");
   else
	if (childpid > 0)
	   exit(0);	/* parent exits */

   /*
      1st child can now disassociate from controlling terminal and
      process group. Ensure the process can't reacquire a new controlling
      terminal.
   */

   if (setpgrp() == -1)
        perror("can't change process group");

   signal(SIGHUP, SIG_IGN);  /* ignore process group leaders death */

   if ( (childpid = fork()) < 0)
        perror("can't fork 2nd child");
   else
        if (childpid > 0)
           exit(0);     /* 1st child exits */

  skipstuff:

   freopen("/dev/console","a",stdout);
   freopen("/dev/console","a",stderr);
   strtfd = 3;

   errno = 0;	/* clear error from any above close() calls */

   /* Clear the file mode creation mask */
   umask(000);
}
