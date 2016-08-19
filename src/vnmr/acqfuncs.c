/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*--------------------------------------------------------------------------n
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   4/06/89   Greg B.     1. Added sa('eoc') code into stop_acquisition() 
|   12/07/90  Robert L.   2. More parameters piped to ACQI so it can
|			     modify frequencies interactively
|   05/05/95              3. Keep ACQI failures from destroying VNMR
+----------------------------------------------------------------------------*/

#define TRUE 1
#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef UNIX
#include <pwd.h>		/* Not present or needed on VMS */
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <signal.h>
#include <setjmp.h>
#endif 
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "bgvars.h"
#include "sockinfo.h"
#include "variables.h"
#include "group.h"
#include "params.h"
#include "tools.h"
#include "locksys.h"
#include "pvars.h"
#include "vfilesys.h"
#include "wjunk.h"
#include "STAT_DEFS.h"

#ifdef SUN
#include "ACQ_HAL.h"
#include "ACQ_SUN.h"
#include "shims.h"
#include "acquisition.h"
#endif 

extern char     HostName[];
extern char     OperatorName[];
extern int      debug1;
extern int      Bnmr;

#define COMPLETE 0
#define ERROR 1
#define CALLNAME 0
#define TRUE 1
#define FALSE 0
#define CALC 1
#define SHOW 0
#define MAXSTR 256

#define MAX_DTABLESIZE 64	/* arbitrary size.  Assume Vnmr never has more
					than 64 files open at a given time. */

#ifdef  DEBUG
extern int      Gflag;

#define GPRINT0(level, str) \
	if (Gflag >= level) Wscrprintf(str)
#define GPRINT1(level, str, arg1) \
	if (Gflag >= level) Wscrprintf(str,arg1)
#define GPRINT2(level, str, arg1, arg2) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2)
#define GPRINT3(level, str, arg1, arg2, arg3) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3)
#define GPRINT4(level, str, arg1, arg2, arg3, arg4) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define GPRINT0(level, str)
#define GPRINT1(level, str, arg2)
#define GPRINT2(level, str, arg1, arg2)
#define GPRINT3(level, str, arg1, arg2, arg3)
#define GPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif 

struct _PidList {
    int pid;
    int connected;
    char *progname;
    char *tagname;
    struct _PidList *next;
};
typedef struct _PidList PidList;
static PidList *interact_pid_list = NULL;

static int      child;
static int      fd[2];
static int      acqi_pid = -9;

extern char     Xserver[];
extern char     Xdisplay[];

extern int  expdir_to_expnum(char *expdir);
extern int  is_datastation();
extern int  sendTripleEscToMaster(char code, char *string_to_send );
extern void setAbortMsg(const char *str);
extern int  stop_acqi( int abortall );
extern void vnmr_tolower(char *s);
extern int  getInfoSpinner();
extern int acq(int argc, char *argv[], int retc, char *retv[]);
extern int ok_to_acquire();
extern void cleanup_pars();
extern int send2Acq(int cmd, char *msg_for_acq);
extern int talk_to_acqproc(int cmd, char *msg1, char *msg2, int msg2len);
extern void set_effective_user();
extern int set_wait_child(int pid);
extern void system_call(char *s);
extern void fixup_lockfreq_inova();
extern void report_acq_perm_error(char *cmdname, int errorval);
extern int  rf_stepsize(double *stepsize, char *varname, int tree);
extern int GetAcqStatus(int to_index, int from_index, char *host, char *user);
extern int getAcqStatusInt(int index, int *val);
extern int getNameValuePair(char *paramName, char *name, char *value);
extern void lockAtcmd(const char *dir);
extern void unlockAtcmd(const char *dir);
extern void      sleepMilliSeconds(int secs);
extern double round_freq(double baseMHz, double offsetHz,
                  double init_offsetHz, double stepsizeHz);
extern void setAppdirs();

static int  no_experiment(char  *);
static int  stop_acquisition(int, char *[], char [], char [], char []);
static void interact_disconnect_by_pid(PidList *pl);
static void interact_kill_by_pid(PidList *pl);

int acquisition_ok(char *cmdname);
int interact_is_alive(char *tag, char *message);
void interact_disconnect(char *tag);
int is_acqproc_active();
int getparm(char *varname, char *vartype, int tree, void *varaddr, int size);
int setparm(const char *varname, const char *vartype, int tree, const void *varaddr, int index);
int nvAcquisition();
int getnucinfo(double h1freq, char rftype, char *nucname, double *freq,
               double *basefreq, char *freqband, char *syn, double *reffreq,
               int verbose);
void interact_birth_record(char *tag, char *prog, int pid);
void interact_kill(char *tag);
int var_active(char *varname, int tree);
int getsolventinfo(char *solvent, char *solvtable, char *solvname,
                   double *dshift);

/*-----------------------------------------------------------------
|
|     acqproc_msge()
|    aa - Abort acquisition
|    sa - Stop acquisition
|    abortallacqs - Abort and reset all acquisition
|    acqdebug('1-9') - set debug flag for diagnostic printing in Acqproc
|    acqipctst('message') - return this message over IPC (loop back test)
|
+------------------------------------------------------------------*/
int acqproc_msge(int argc, char *argv[], int retc, char *retv[])
{
#ifdef SUN

   char            file[MAXPATHL];
   char            user[MAXPATHL];
   char            message[MAXPATHL];
   int             cmdval;
   int             buglevel,
                   ival, len;

   if ( is_datastation() )
   {
      Werrprintf("Cannot run %s from a data station", argv[0]);
      RETURN;
   }
   if (!acquisition_ok(argv[0]))
      RETURN;
   if (!is_acqproc_active())
      RETURN;
   strcpy(user, UserName);	/* Store user name */


   memset( &message[ 0 ], 0, sizeof( message ) );

   cmdval = RECONREQUEST;
   if (strcmp(argv[0], "aa") == 0)
      cmdval = ACQABORT;
   if (strcmp(argv[0], "sa") == 0)
      cmdval = ACQSTOP;
   if (strcmp(argv[0], "abortallacqs") == 0)
      cmdval = ACQSUPERABORT;
   if (strcmp(argv[0], "acqdebug") == 0)
      cmdval = ACQDEBUG_I;
   if (strcmp(argv[0], "acqipctst") == 0)
      cmdval = IPCTST;
   if (strcmp(argv[0], "halt") == 0)
      cmdval = ACQHALT;
   if (strcmp(argv[0], "halt2") == 0)
      cmdval = ACQHALT2;
   if (strcmp(argv[0], "atcmd") == 0)
      cmdval = ATCMD;
   if (strcmp(argv[0], "acqdequeue") == 0)
      cmdval = ACQDEQUEUE;

   if ( (cmdval == ACQSTOP) && nvAcquisition() )
   {
      Werrprintf("The sa command is not supported");
      ABORT;
   }
   if ( (cmdval == ACQABORT) || (cmdval == ACQSTOP) || (cmdval == ACQHALT) || (cmdval == ACQHALT2) )
   {
      if (getparm("file", "STRING", CURRENT, &file[0], MAXPATHL))
         ABORT;
      if (strcmp(file, "exp") == 0)/* if file = 'exp' then change to exp# */
         sprintf(file, "exp%d", expdir_to_expnum(curexpdir));
      if (no_experiment(argv[0]))
         ABORT;
   }
   switch (cmdval)
   {
      case ACQABORT:
#ifdef VNMRJ
	 if (stop_acqi( 0 )) { RETURN; }
#endif 
	 sprintf(message, "%s %s", user, file);
         setAbortMsg((argc == 2) ? argv[1] : "");
	 break;

/*  Remember the stop acquisition subroutine just formats the
    message which this routine sends to Acqproc.		*/

      case ACQSTOP:
#ifdef VNMRJ
	 if (stop_acqi( 0 )) { RETURN; }
#endif 
	 if (stop_acquisition(argc, argv, user, file, message))
	    ABORT;
	 break;

      case ACQSUPERABORT:
	 sprintf(message, "%s %s", user, file);
	 break;

      case ACQDEBUG_I:
	 if (argc < 2)
	   buglevel = 0;
	 else
	   buglevel = atoi(argv[1]);
	 if (buglevel < 0)
	    buglevel = 0;
	 sprintf(message, "%d", buglevel);
	 break;

      case IPCTST:
	 if (argc < 2)
	   len = 0;
	 else
	   len = strlen(argv[1]);
	 if (len > 1)
	   strcpy( message, argv[ 1 ] );
	 else
	   strcpy(message, "Acqproc IPC TEST Complete,");
	 break;

      case ACQHALT:
#ifdef VNMRJ
	 if (stop_acqi( 0 )) { RETURN; }
#endif 
	 sprintf(message, "%s %s", user, file);
         setAbortMsg((argc == 2) ? argv[1] : "");
	 break;

      case ACQHALT2:
#ifdef VNMRJ
	 if (stop_acqi( 0 )) { RETURN; }
#endif 
	 sprintf(message, "%s %s", user, file);
         setAbortMsg((argc == 2) ? argv[1] : "");
	 break;

      case ATCMD:
	 strcpy(message, "atcmd");
	 break;
      case ACQDEQUEUE:
         if (argc == 2)
         {
            sprintf(message, "%s %s", argv[1], systemdir);
         }
         else
         {
            char str[MAXSTR];
            if (P_getstring(PROCESSED,"go_id",str,1,MAXSTR))
            {
               if (retc)
                  retv[0] = newString("0");
               else
                  Werrprintf("'go_id' not defined in submitted experiment");
               RETURN;
            }
            sprintf(message, "%s %s", str, systemdir);
         }
         break;
      default:
	 Werrprintf("'%s' is an illegal alias of abort_stop().",
		    argv[0]);
	 ABORT;
	 break;
   }
   GPRINT1(1, "msge: '%s'\n", message);
   if (cmdval == ACQDEQUEUE)
   {
      char ans[MAXSTR];
      strcpy(ans,"0");
      ival = talk_to_acqproc(cmdval, message, ans, sizeof(ans));
      if (retc)
      {
         retv[0] = newString(ans);
      }
      else
      {
         Winfoprintf("Acquisition%s removed from queue", strcmp(ans,"0") ? " " : " not");
      }
      RETURN;
   }
   ival = send2Acq(cmdval, message);
   /*
    * Pause aa or halt until console is idle. This lets a go immediately following
    * an aa or halt to work.  Otherwise, one would get the
    * "Experiment in progress, use 'aa' to abort it and then reenter go" message
    */
   if ( (cmdval == ACQABORT) || (cmdval == ACQSTOP) || (cmdval == ACQHALT) || (cmdval == ACQHALT2) )
   {
      int acqstate = 0;
      int timeout = 50; /* Sleep up to 5 seconds in tenth second intervals */
      struct timespec req;
      while ( (acqstate != ACQ_IDLE) && (timeout > 0) )
      {
         if (GETACQSTATUS(HostName,UserName) < 0)
         {
            RETURN;
         }
         getAcqStatusInt(STATE, &acqstate);
         if (acqstate != ACQ_IDLE)
         {
            timeout--;
            req.tv_sec=0;
            req.tv_nsec=100000000; /* tenth second sleeps */
#ifdef __INTERIX
         usleep(req.tv_nsec/1000);
#else
         nanosleep( &req, NULL);
#endif
         }
      }
   }
   RETURN;
#else 
   Werrprintf("%s only available on the SUN console", argv[0]);
   ABORT;
#endif 
}

/*   Verify that it's OK to run an acquisition program.

     The checks are:
	Current experiment must be defined (length of string > 0)
	Current experiment cannot be experiment 0.  The latter
       condition occurs when VNMR cannot lock experiment 1 at
       bootup.
	System must be configured as a spectrometer, as implied
       by the value of the system global parameter ``system''.

      Function returns 0 if OK to proceed; non-zero value otherwise.	*/

#ifdef SUN
static int no_experiment(char *cmdptr)
{
   int             len;

   len = strlen(&curexpdir[0]);
   if (len < 1)
   {
      Werrprintf("%s:  current experiment not defined", cmdptr);
      ABORT;
   }
   if ( expdir_to_expnum(curexpdir) == 0 )
   {
      Werrprintf("%s:  no current experiment", cmdptr);
      ABORT;
   }
   RETURN;
}
#endif 

#ifdef SUN
int stop_acquisition(int argc, char *argv[], char user[], char file[], char message[])
{
   char		   ilstr[MAXSTR]; /* interleave string value */
   int             mod;		/* phase cycling modulo stop number */
   int             stop_cmd;	/* stop type */
   double 	   nfids;	/* arraydim */
   vInfo           paraminfo;

   mod = 1;
   if (argc < 2)
   {
      stop_cmd = STOP_EOS;
   }
   else if ((strcmp(argv[1], "eos") == 0) ||
	    (strcmp(argv[1], "ct") == 0) ||
	    (strcmp(argv[1], "scan") == 0))
      stop_cmd = STOP_EOS;
   else if ((strcmp(argv[1], "eob") == 0) ||
	    (strcmp(argv[1], "bs") == 0))
      stop_cmd = STOP_EOB;
   else if ((strcmp(argv[1], "eof") == 0) ||
	    (strcmp(argv[1], "nt") == 0) ||
	    (strcmp(argv[1], "fid") == 0))
      stop_cmd = STOP_EOF;
   else if ((strcmp(argv[1], "eoc") == 0) ||
	    (strcmp(argv[1], "il") == 0))
      stop_cmd = STOP_EOC;
   else				/* for sa(modulo_number) (phase type) */
   {
      if (!isReal(argv[1]))
      {
	 Werrprintf("%s: invalid argument %s", argv[0], argv[1]);
	 return (1);
      }
      stop_cmd = STOP_EOS;
      mod = atoi(argv[1]);
      GPRINT1(1, "modulo: %d\n", mod);
      if ((mod > 32768) || (mod < 1))
      {
	 Werrprintf(
		    "sa modulo number %d not in proper range (1-32768).", mod
	    );
	 return (1);
      }
      GPRINT2(1, "stopcmd: %d,  modulo: %d \n", stop_cmd, mod);
   }

/*  If Interleaving  set mod to interleave cycle */

   if (stop_cmd == STOP_EOC)
   {
      /* check that is was an arrayed experiment */
      if (P_getreal(PROCESSED,"arraydim",&nfids,1))
      {
	 Werrprintf("'arraydim' not defined in submitted experiment");
         return (1);
      }
      if (nfids < 1.5)
      {
	 Werrprintf("Submitted experiment was not arrayed");
         return (1);
      }

      /* check on interleave parameter */
      if (P_getstring(PROCESSED,"il",ilstr,1,MAXSTR))
      {
	 Werrprintf("'il' not defined in submitted experiment");
         return (1);
      }
      /* --- determine interleave cycle if any --- */
      /*fprintf(stderr,"interleave = '%s' \n",interleave);*/
      if ( ! strcmp(ilstr,"y") )
      {
          mod = (int) nfids;  /* il=y --> stp_mod=nfids */
      }
      else if ( ! strcmp(ilstr,"n") )
      {
	  Werrprintf("'il' not used in submitted experiment");
	  return (1);
      }
      else if ( ilstr[0] == 'f')
      {
         mod = atoi(&ilstr[1]);
         if ( (mod > nfids) || (mod < 2))
         {
            /* unreasonible value, then no interleaving */
	    Werrprintf("'il' not used in submitted experiment");
	    return (1);
         }
      }
      /*fprintf(stdout,"sa('eoc') modulo number = %d\n",mod);*/
   }

/*  If Block Size selected, verify it was selected in the experiment
    submitted to Acqproc by checking "bs" in the processed parameters.	*/

   if (stop_cmd == STOP_EOB)
   {
      if (P_getVarInfo(PROCESSED, "bs", &paraminfo))
      {
	 Werrprintf("'bs' not defined in submitted experiment");
	 return (1);
      }
      if (paraminfo.active == 0)
      {
	 Werrprintf("'bs' not used in submitted experiment");
	 return (1);
      }
   }

   sprintf( message, "%d %d %s %s", stop_cmd, mod, user, file);
   return (0);
}
#endif 

#ifdef SUN
int is_acqproc_active()
{
   return(ACQOK(HostName));
}
#endif 
/*-------------------------------------------------------------------
|    calcdim() -  Usage: calcarraydim()
|
|   Purpose:
|  	This module calculates the values of arraydim and arrayelement
|	 and updates the CURRENT tree values.
|
|	Author   Greg Brissey   3/16/87
+-----------------------------------------------------------------*/
int run_calcdim()
{
   char *argv[3];

   argv[0] = "acq";
   argv[1] = "go";
   argv[2] = "calcdim";
   return( acq(3,argv,0,NULL) );
}

int calcdim(int argc, char *argv[], int retc, char *retv[])
{
   (void) run_calcdim();
   RETURN;
}

/*  Based on getVnmrInfo in go.c (SCCS category vnmr),
    plus other programs that create com$string		*/

void getInteractVnmrInfo()
{
    P_creatvar(CURRENT,"com$string",ST_STRING);
    P_setgroup(CURRENT,"com$string",G_ACQUISITION);
    setparm("com$string","string",CURRENT,"",1);  /* initialize to null */

    P_creatvar(CURRENT,"spinThresh",ST_REAL);
    P_setgroup(CURRENT,"spinThresh",G_ACQUISITION);
    P_setreal(CURRENT,"spinThresh",(double) getInfoSpinner() ,0);

    P_creatvar(CURRENT,"interLocks",ST_STRING);
    P_setgroup(CURRENT,"interLocks",G_ACQUISITION);
    setparm("interLocks","string",CURRENT,"nnn",1);
}

#ifdef SUN

/* copied from go.c, which see for more information */

static sigset_t origset;

static void
block_sigchld()
{
	sigset_t	nochld;

	sigemptyset( &nochld );
	sigaddset( &nochld, SIGCHLD );
	sigprocmask( SIG_BLOCK, &nochld, &origset );
}

static void
restore_sigchld()
{
	sigprocmask( SIG_SETMASK, &origset, NULL );
}

/*  Next two data structs and next three programs keep the failure
    of acqi from starting properly from destroying VNMR.

    Adapted from similar facilities in go.c.

    These facilities should be generalized, with the part of the VNMR
    program to be protected isolated as a separate program, to be
    called from the program that sets up (and removes) the protection.	*/

static jmp_buf		brokenpipe;
static struct sigaction	origpipe;

static void
sigpipe(int sig)
{
	longjmp( brokenpipe, -1 );
}

static void
catch_sigpipe()
{
	sigset_t		qmask;
	struct sigaction	newpipe;

	sigemptyset( &qmask );
	sigaddset( &qmask, SIGPIPE );
	newpipe.sa_handler = sigpipe;
	newpipe.sa_mask = qmask;
	newpipe.sa_flags = 0;
	sigaction( SIGPIPE, &newpipe, &origpipe );
}

static void
restore_sigpipe()
{
	sigaction( SIGPIPE, &origpipe, NULL );
}

/*---------------------------------------------------------------------------
|	run_acqi
|
|	This module creates a pipe, forks the interactive display program,
|	sends over parameters through the pipe to
|	the program and returns to vnmr without waiting.
|
+--------------------------------------------------------------------------*/
static int
run_acqi(char *argname, int retc, char *message)
{
   int		   generic_program; /* TRUE if not starting acqi */
   int             ret;
   char            fd0[50];
   char            fd1[50];
   char		   progname[20];

/*   Verify that it's OK to run acqi.  Someday these checks
     should be in a separate procedure... 			*/


   if ( is_datastation() )
   {
      if (!retc)
         Werrprintf("Cannot run %s from a data station", argname);
      ABORT;
   }
   if (!Wissun())
   {
      if (!retc)
         Werrprintf("%s only available on the SUN console", argname);
      ABORT;
   }

#ifndef TEST
   if (!is_acqproc_active())
   {
      if (!retc){
          Werrprintf("Cannot run %s, acquisition communication not active",
		     argname);
      }
      ABORT;
   }
#endif 

   if (strcmp("acqi", argname) == 0)
	generic_program = 0;
   else
	generic_program = 1;

   /* Abort if this program already active  */
   if (interact_is_alive(argname, message)){
       if (!retc && generic_program)
	   Werrprintf("%s already running", argname);
       RETURN;
   }
   
   ret = pipe(fd);		/* make pipe */
   if (ret == -1)
   {
       if (!retc) Werrprintf("Unable to execute acqi");
       ABORT;
   }
   
   getInteractVnmrInfo();

   if (generic_program){
       strcpy(progname, argname);
   }else{
       strcpy(progname, "iadisplay");
   }



#ifdef IRIX
   child = fork();		/* fork a child */
#else 
   /* child = vfork();  outdated by Solaris & thread unsafe */
   child = fork();
#endif 
   if (child){
       /* If parent, tell signal handler to reap child when it dies */
       if (!generic_program)
            acqi_pid = child;
       set_wait_child(child);
       interact_birth_record(argname, progname, child);
   }else{
      /* I am the child process */
      int d_tablesize, iter, ival;
      struct rlimit  numfiles;

      sprintf(fd1, "%d", fd[1]);
      sprintf(fd0, "%d", fd[0]);

#if defined (IRIX) || defined (AIX)
      d_tablesize = -1;
      d_tablesize = getdtablesize();
      if (d_tablesize < 0)
      {
         perror( "getrlimit" );
         _exit( 1 );
      }
      if (d_tablesize > MAX_DTABLESIZE)
	d_tablesize = MAX_DTABLESIZE;
#else 
      ival = getrlimit( RLIMIT_NOFILE, &numfiles );
      if (ival < 0) {
         perror( "getrlimit" );
         _exit( 1 );
      }

      d_tablesize = numfiles.rlim_cur;
#endif 

      for (iter = 3; iter < d_tablesize; iter++) {
         if (iter != fd[ 0 ] && iter != fd[ 1 ])
           close( iter );
      }

      set_effective_user();
      if (debug1)
	 execlp(progname, progname, fd0, fd1,
		     "-debug", message, Xdisplay, Xserver, NULL);
      else
	 execlp(progname, progname, fd0, fd1,
		      Xdisplay, message, Xserver, NULL);
      /* --- child should never get here --- */
      Werrprintf("%s could not execute or could not be found", argname);
      _exit(1);		/* very important for the child to call exit */
   }

   close(fd[0]);		/* parent closes its read end of pipe */

   catch_sigpipe();
   block_sigchld();
   if (setjmp( brokenpipe ) == 0)
   {
   /* P_sendTPVars(tree,fd[1]);         send tree to pipe */
   /* P_sendGPVars(tree,group,fd[1]);   send group in tree to pipe */
   /* P_sendVPVars(tree,name,fd[1]);    send parameter in tree to pipe */

#ifndef TEST
       if (generic_program){
	   /* Send the kitchen sink to qtune, etc */
	   P_sendGPVars(SYSTEMGLOBAL, G_ACQUISITION, fd[1]);
	   P_sendGPVars(GLOBAL, G_ACQUISITION, fd[1]);
	   P_sendGPVars(CURRENT, G_ACQUISITION, fd[1]);
       }else{
	   /*
	    * Pipe selected parameters to iadisplay (ACQI)
	    */
	   P_sendVPVars(CURRENT, "solvent", fd[1]);
	   P_sendVPVars(CURRENT, "spin", fd[1]);
	   P_sendVPVars(CURRENT, "temp", fd[1]);
	   P_sendVPVars(CURRENT, "gain", fd[1]);
	   P_sendVPVars(CURRENT, "vtc", fd[1]);
	   
	   P_sendVPVars(GLOBAL, "lockpower", fd[1]);
	   P_sendVPVars(GLOBAL, "lockphase", fd[1]);
	   P_sendVPVars(GLOBAL, "lockgain", fd[1]);
	   P_sendVPVars(GLOBAL, "z0", fd[1]);
	   P_sendVPVars(GLOBAL, "lkof", fd[1]);
	   P_sendVPVars(GLOBAL, "vnmraddr", fd[1]);
	   
	   P_sendVPVars(SYSTEMGLOBAL, "Console", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "h1freq", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "fifolpsize", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "apinterface", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "system", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "vttype", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "shimset", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "rftype", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "ptsval", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "lockfreq", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "latch", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "overrange", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "parstep", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "parmax", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "parmin", fd[1]);
	   P_sendVPVars(SYSTEMGLOBAL, "spinopt", fd[1]);
       }

       P_endPipe(fd[1]);		/* send end character */
#endif 
   }
   else {
       Werrprintf( "Can't seem to start up %s", argname );
   }
   restore_sigchld();
   restore_sigpipe();
   close(fd[1]);
   cleanup_pars();

   /* return(OK); */
   RETURN;
}
#endif 

#ifdef SUN
/**************/
static int
copy_acqi_pars()
/**************/
{
   P_copyvar(CURRENT,TEMPORARY,"phfid","phfid");
   P_copyvar(CURRENT,TEMPORARY,"lsfid","lsfid");
   P_copyvar(CURRENT,TEMPORARY,"dotflag","dotflag");
   P_copyvar(CURRENT,TEMPORARY,"dmgf","dmgf");

   P_copyvar(CURRENT,TEMPORARY,"dmg","dmg");
   P_copyvar(CURRENT,TEMPORARY,"fn","fn");
   P_copyvar(CURRENT,TEMPORARY,"lp","lp");
   P_copyvar(CURRENT,TEMPORARY,"lvl","lvl");
   P_copyvar(CURRENT,TEMPORARY,"rfl","rfl");
   P_copyvar(CURRENT,TEMPORARY,"rfp","rfp");
   P_copyvar(CURRENT,TEMPORARY,"rp","rp");
   P_copyvar(CURRENT,TEMPORARY,"sp","sp");
   P_copyvar(CURRENT,TEMPORARY,"sw","sw");
   P_copyvar(CURRENT,TEMPORARY,"tlt","tlt");
   P_copyvar(CURRENT,TEMPORARY,"vp","vp");
   P_copyvar(CURRENT,TEMPORARY,"vs","vs");
   P_copyvar(GLOBAL, TEMPORARY,"wc2max","wc2max");
   P_copyvar(CURRENT,TEMPORARY,"wp","wp");

   P_copyvar(CURRENT,TEMPORARY, "sfrq", "sfrq");
   P_copyvar(CURRENT,TEMPORARY, "dfrq", "dfrq");
   P_copyvar(CURRENT,TEMPORARY, "dfrq2","dfrq2");
   P_copyvar(CURRENT,TEMPORARY, "tof", "tof");
   P_copyvar(CURRENT,TEMPORARY, "dof", "dof");
   P_copyvar(CURRENT,TEMPORARY, "dof2","dof2");
   P_copyvar(CURRENT,TEMPORARY, "rfband", "rfband");
   RETURN;
}
#endif 

#ifdef SUN
/**************/
static int
save_acqi_pars()
/**************/
{
   char path[MAXPATHL];
   char syscall[MAXPATHL];

   P_treereset(TEMPORARY);
   if (copy_acqi_pars())
   {
      Werrprintf("cannot save acqi parameters");
      P_treereset(TEMPORARY);
      ABORT;
   }
   strcpy(path, systemdir);
   strcat(path, "/acqqueue/acqi.par");
   unlink(path);
   if (P_save(TEMPORARY,path))
   {
      Werrprintf("cannot save acqi parameters in %s",path);
      P_treereset(TEMPORARY);
      ABORT;
   }
   P_treereset(TEMPORARY);
   sprintf(syscall,"chmod 666 %s\n",path);
   system_call(syscall);
   RETURN;
}

/* Programs to get the lock system parameter values from the console.
   read_acqi_pars always stores these values, even if load is set to 'y'  */

static struct {
	char	*pname;
	int	 code;
	int	 tree;
} lockSystemTable[] = {
	{ "lockpower",	EXP_LKPOWER,	GLOBAL },
	{ "lockgain",	EXP_LKGAIN,	GLOBAL },
	{ "lockphase",	EXP_LKPHASE,	GLOBAL },
	{ "spin",	EXP_SPINSET,	CURRENT },
	{  NULL,        -1,		-1 }
};

static void
read_locksys_pars()
{
	int	code, iter, ival;
	int	isz0active;
	char	console[ MAXSTR ];
	double	dbltmp;

	for (iter = 0; lockSystemTable[ iter ].pname != NULL; iter++) {
		code = lockSystemTable[ iter ].code;
		getExpStatusInt( code, &ival);
		P_setreal(
	    lockSystemTable[ iter ].tree, lockSystemTable[ iter].pname, (double) ival, 0
		);
	}

	getExpStatusShim(Z0, &ival);
        P_setreal( GLOBAL, "z0", (double) ival, 0);

	if (P_getstring(SYSTEMGLOBAL, "Console", console, 1, MAXSTR - 1) == 0)
	{
		if (strcmp( console, "inova" ) == 0)
		{
			ival = P_getparinfo(GLOBAL, "z0", &dbltmp, &isz0active);
			if (ival == 0 && isz0active == 0)
			  fixup_lockfreq_inova();
		}
	}
}

/**************/
int read_acqi_pars()
/**************/
{
   char            path[MAXPATHL];
   int             statbufok;
   int             index;
   int             value;

   strcpy(path, systemdir);
   strcat(path, "/acqqueue/acqipar");
   statbufok = open_ia_stat(path,1,SHIM_STRUC);
   if (statbufok)
   {
      /* do not load dacs values into an experiment which has load='y' */
      char str[MAXSTR];

      read_locksys_pars();

      if (!P_getstring(CURRENT,"load",str,1,MAXSTR) && (str[0] == 'y'))
      {
         RETURN;
      }
      init_shimnames(SYSTEMGLOBAL);
      for (index = Z0 + 1; index <= MAX_SHIMS; index++)
      {
         const char *sh_name;
         if ((sh_name = get_shimname(index)) != NULL)
         {
            getExpStatusShim(index, &value);
            P_setreal( CURRENT, sh_name, (double) value, 0);
         }
      }
      appendvarlist("z1");
   }
   RETURN;
}
#endif 

#ifdef SUN
/*
 *   test if acquisition communication is allowed
 *   This test is passed if the process was started by acqproc
 *   or if the process is running on the SUN console.
 */

int acquisition_ok(char *cmdname)
{
   int        ival;
   extern int mode_of_vnmr;

   if (!Wissun())
   {
      if ((mode_of_vnmr == ACQUISITION) || (mode_of_vnmr == AUTOMATION))
         return(1);
      else {
            ival = ok_to_acquire();
            if (ival != 0) {
               report_acq_perm_error( cmdname, ival );
               return( 0 );
            }
            else
               return( 1 );
      }
   }
   else
   {
      return(1);
   }
}
#endif 

/*
 * Determines if system is based on parallel architecture
 */
int nvAcquisition()
{
   static int res = -1;

   if (res < 0 )
   {
      char procName[MAXPATH];
      sprintf(procName, "%s/bin/nvlocki", systemdir);
      res = (!access(procName, X_OK)) ? 1 : 0;
   }
   return(res);
}

/******************************/
int ia_start(int argc, char *argv[], int retc, char *retv[])
/******************************/
{
#ifdef SUN
   int             len;
   int             ok = 1;

   len = strlen(&curexpdir[0]);
   if (len < 1)
   {
      if (!retc)
         Werrprintf("%s:  current experiment not defined", argv[0]);
      ok = 0;
   }
   else if ( expdir_to_expnum(curexpdir) == 0 )
   {
      if (!retc)
         Werrprintf("%s:  no current experiment", argv[0]);
      ok = 0;
   }
   else if (argc == 1)   /*   if no arguments are passed,  simply run the program */
   {
      if (run_acqi(argv[0],retc, "open") == 0)
      {
         (void) save_acqi_pars();      /* do not do anything if it fails */
      }
      else
         ok = 0;
   }
   else if (strcmp(argv[1],"disconnect") == 0){
       /* Use alias we were called by to pick program to disconnect */
       interact_disconnect(argv[0]);
   }else if (strcmp(argv[1],"exit") == 0){
       /* Use alias we were called by to pick program to kill */
       if (strcmp(argv[0], "acqi") != 0)
          interact_disconnect(argv[0]);
       interact_kill(argv[0]);
   }
   else if (strcmp(argv[1],"par") == 0)
   {
      if (save_acqi_pars())
      {
	 ok = 0;
      }
   }
   else if (strcmp(argv[1],"read") == 0)
   {
      if (read_acqi_pars())
      {
	 ok = 0;
      }
   }
   else if (strcmp(argv[1],"standby") == 0)
   {
      if (run_acqi(argv[0],retc, argv[1]) == 0)
      {
         (void) save_acqi_pars();      /* do not do anything if it fails */
      }
      else
         ok = 0;
   }
   else
   {
      if (!retc)
         Werrprintf("Unknown argument %s passed to acqi",argv[1]);
      ok = 0;
   }
   if (retc > 0)
       retv[0] = realString((double) ok);
   RETURN;
#else 
   Werrprintf("%s only available on the SUN console", argv[0]);
   ABORT;
#endif 
}

/******************************
 * Called by the Vnmr command: "interact('alias' [,'option'])".
 * Shifts argv arguments and calls ia_start.  The effect is to execute
 * ia_start as if it were called by the command "alias('option')".
 * This can be used to start up an arbitrary interactive program named "alias".
 * Also, "interact('', 'exit')" will kill all interactive programs.
 ******************************/
int interact(int argc, char *argv[], int retc, char *retv[])
{
    if (argc > 1){
	ia_start((argc-1), &argv[1], retc, retv);
	RETURN;
    }else{
	Werrprintf("%s requires arguments", argv[0]);
	ABORT;
    }
}

// basefactor is non-zero only if global parameter uselockref='n' 
// and baseFactor is set in global parameter userbaseref or 
// systemglobal parameter sysbaseref.
void get_base_factor(double *factor)
{
  char str[MAXSTR];
  *factor=0.0;
  if (P_getstring(GLOBAL, "uselockref", str, 1, MAXSTR - 1) == 0) {
    if(strcmp(str,"n")==0) {
      getNameValuePair("userbaseref", "baseFactor", str);
      if(strlen(str)>0) *factor=atof(str);
      else {
        getNameValuePair("sysbaseref", "baseFactor", str);
        if(strlen(str)>0) *factor=atof(str);
      }
    }
  }
}

/*-----------------------------------------------------------------------
|
|       get_lock_factor()/3
|	calculate the lock correction factor from lockfreq
|
+----------------------------------------------------------------------*/
void 
get_lock_factor(double *lkfactor, double h1freq, char rftype)
{
   int H1freq;
   double factor = 0.0;
   double lkfreq, lkof;

   if (var_active("lockfreq", SYSTEMGLOBAL) == 1 )   /* if lockfreq !='n' */
   {
      if (getparm("lockfreq", "REAL", SYSTEMGLOBAL, &lkfreq, 1) == 0)
      {
         if (P_getreal(GLOBAL,  "lkof", &lkof, 1 ) != 0)
           lkof = 0.0;
         lkfreq += (lkof * 1.0e-6);
         H1freq = (int) (h1freq + 0.5);
/* G2000 has a stepsize of 19.073.. Hz for lockfreq. Here we calculate the   */
/* actual frequency used to set the hardware and use that in the calculation */
/* not the value actually entered in config.				     */
         if ( (rftype == 'e') || (rftype == 'f') )	/* this is G2000 */
         {  double loffset=0.0;
            double lbase  =0.0;
            int    ltemp;
            if (H1freq < 210)
            {  loffset = lkfreq-30.0;
               lbase   = 30.0;
            }
            else if (H1freq < 310)
            {  loffset = lkfreq - 45.4;
               lbase   = 45.4;
            }
            else if (h1freq < 410)
            {  loffset = lkfreq - 60.8;
               lbase   = 60.8;
            }
            ltemp = (int)(loffset / 625000.0 * 32768.0 * 1e6 + 0.5);
            lkfreq = lbase + (double)(ltemp)*625000.0 /(32768.0 * 1e6);
         }
         switch (rftype)
         {  case 'd':
            case 'e':
            case 'f':
            {  
	      char synthesizer[10];	/* PTS synthesizer, yes,no,reverse */
	      char frqband[10]; 	/* PTS synthesizer, yes,no,reverse */
	      double nucbaseoffset;	/* base offset frequency */
	      double reffreq; 	/* Reference frequency */
	      double h2freq;
	      /* get H1freq convert to H2 if successful */
	      if (getnucinfo(h1freq, rftype, "h2lk", &h2freq, 
			&nucbaseoffset,frqband, synthesizer, &reffreq, 0))
	      {
	       /* Error in getting H2 from nucleus table, do old way */
	       /* printf("get_lock_factor: Error getting H1 from nuctab\n"); */
	       if (H1freq < 90)
                  factor = (lkfreq - 13.1247) / 13.1247;
	       else if (H1freq < 110)
                  factor = (lkfreq - 15.355) / 15.355;
	       else if (H1freq < 210)
                  factor = (lkfreq - 30.710) / 30.710;
               else if (H1freq < 310)
                  factor = (lkfreq - 46.044) / 46.044;
               else if (H1freq < 410)
                  factor = (lkfreq - 61.395) / 61.395;
               else if (H1freq < 510)
                  factor = (lkfreq - 76.729) / 76.729;
               else if (H1freq < 610)
                  factor = (lkfreq - 92.095) / 92.095;
               else
                  factor = (lkfreq - 115.158) / 115.158;
	      }
	      else
	      {
		  /* printf("get_lock_factor: h2freq = %g\n",h2freq); */
		  factor = (lkfreq - h2freq) / h2freq;
	      }
              break;
            }
            default:
            {  if (H1freq < 210)
                  factor = (lkfreq - 1.210) / 30.710;
               else if (H1freq < 310)
                  factor = (1.206 - lkfreq) / 46.044;
               else if (H1freq < 410)
                  factor = (lkfreq - 1.145)/ 61.395;
               else if (H1freq < 510)
                  factor = (lkfreq - 1.479) / 76.729;
               else if (H1freq < 610)
               {
                  if ((lkfreq > 2.900) && (lkfreq < 3.900))
                       factor = (lkfreq - 3.845) / 92.095;
                  else
                    factor = (lkfreq - 153.845) / 92.095;
               }
               break;
            }
         }
      }
   }
   *lkfactor = factor;
   GPRINT1(1,"lock factor is %g\n", *lkfactor);
}

/*-----------------------------------------------------------------------
|
|	get_solv_factor()/1
|	calculate the solvent correction factor from solvent
|
+----------------------------------------------------------------------*/
void
get_solv_factor(double *solvfactor)
{
   double          dshift;
   char            solvent[MAXSTR];	/* solvent name */
   char            solvname[MAXSTR];	/* solvent name */
   char            solvpath[MAXSTR];	/* file name */

   *solvfactor = 0.0;
   if (getparm("solvent", "STRING", CURRENT, solvent, MAXSTR-1) == 0)
   {
      vnmr_tolower(solvent);
      if ( strcmp(solvent, "") && strcmp(solvent, "none") )
      {
         /* construct absolute path to solvent table */
         strcpy(solvpath, systemdir);

#ifdef UNIX
         strcat(solvpath, "/solvents");
#else 
         strcat(solvpath, "solvents");
#endif 

         if (getsolventinfo(solvent, solvpath, solvname, &dshift) == 0)
         {
            *solvfactor = (dshift - 5.0) * 1e-6;
         }
      }
   }
   GPRINT1(1,"solvent factor is %g\n", *solvfactor);
}

/*-----------------------------------------------------------------------
|
|	calcfreq()/4
|	get nucleus frequency from nucleus table
|
+----------------------------------------------------------------------*/
static int
calcfreq(double *freq, int index, double ref_freq, char *rftype, char *freqname)
{
   char            nucname[20];
   char            nucval[20];
   char            synthesizer[10];	/* PTS synthesizer, yes,no,reverse */
   char            frqband[10]; 	/* PTS synthesizer, yes,no,reverse */
   double          nucbaseoffset;	/* base offset frequency */
   double          reffreq; 	/* Reference frequency */

   if (index == 1)
      strcpy(nucname,"tn");
   else
      sprintf(nucname,"dn%c", (index == 2) ? '\0' : (index - 1) + '0');
   if (getparm(nucname, "STRING", CURRENT, &nucval[0], 7))
   {
      *freq = 0.0;
      return(ERROR);
   }
   GPRINT2(1, "nucname = '%s', nucval= '%s'\n", nucname, nucval);
   if (strcmp(nucval,"") == 0)
   {
      /*  do not calculate frequencies for these cases */
      *freq = 0.0;
      return(ERROR);
   }
   else if (strcmp(nucval,"none") == 0)
   {
      /* get current freq.  Return error to suppress further corrections */
      if (P_getreal(CURRENT, freqname, freq, 1) < 0)
         *freq = 0.0;
      return(ERROR);
   }
   if (getnucinfo(ref_freq, rftype[index-1], nucval, freq, &nucbaseoffset,
		  frqband, synthesizer, &reffreq, 1))
   {
      *freq = 0.0;
      return(ERROR);
   }
   GPRINT1(1, "freqval= %g\n", *freq);
   return(COMPLETE);
}

/*-------------------------------------------------------------------
|    setfrq  -  Usage: setfrq[:freq1,freq2,...]
|                      setfrq(channel#)[:freq]
|                      setfrq(nucName)[:freq]
|
|   Purpose:
|  	This module retrieves the specified nucleus information
|	from the nucleus table. Sets sfrq or dfrq or dfrq2, etc.
|	according to the nucleus table, lockfreq, and solvent.
|       If nucName is given or a return value is requested,
|       no parameters are altered.
|       With no supplied channel number, all channel frequencies are
|       calculated.
|
|   Parameters altered:
|	sfrq dfrq dfrq2
|
|--------------------------------------------------------------------*/
int setfrq(int argc, char *argv[], int retc, char *retv[])
{
   char            rftype[10];	/* RF high or low band identifier */
   double          h1freq;	/* proton freq of instrument */
   double          numrfch;     /* number of channels */
   double          freq;
   double          lkfactor;
   double          solvfactor;
   double          basefactor;
   double          offval;
   double          stepsize;
   char            freqname[20];
   char            offname[20];
   int             first,last;	/* RF index for trans or decoupler */
   int             index;
   int             useNuc = 0;

   if (argc > 2)
   {
      Werrprintf("Usage - %s(channel index) or %s(nucleus) ",
                 argv[0], argv[0]);
      return ERROR;
   }

   if (getparm("rftype", "STRING", SYSTEMGLOBAL, &rftype[0], 7))
      return ERROR;
   if (getparm("h1freq", "REAL", SYSTEMGLOBAL, &h1freq, 1))
      return ERROR;
   if (getparm("numrfch", "REAL", SYSTEMGLOBAL, &numrfch, 1))
      return ERROR;
   first = 1;
   last = (int) (numrfch + 0.1);
   if(last>5)last=5;
   if (argc == 2)
   {
     if (isReal(argv[1]))
     {
	first = (int) (stringReal(argv[1]) + 0.1);
        if (first < 1)
        {
           Werrprintf("channel index must be greater than 0");
           return ERROR;
        }
        else if (first > last)
        {
           char nucname[20];
           char nucval[20];
           int fail = 1;

           sprintf(nucname,"dn%c", (first == 2) ? '\0' : (first - 1) + '0');
           if (getparm(nucname, "STRING", CURRENT, &nucval[0], 7) == 0)
           {
              if ( (strcmp(nucval,"none") == 0) || (strcmp(nucval,"") == 0) )
                 fail = 0;
           }
           if (fail)
           {
              Werrprintf("channel index cannot exceed %d", last);
              return ERROR;
           }
           else
           {
              RETURN;
           }
        }
        else
        {
           last = first;
        }
     }
     else
     {
        last = 1;
        useNuc = 1;
     }
   }
  
   get_base_factor(&basefactor);
   get_lock_factor(&lkfactor,h1freq,rftype[0]);
   get_solv_factor(&solvfactor);
   for (index = first; index <= last; index++)
   {
      freq = 0.0;
      if (index == 1)
      {
         strcpy(freqname,"sfrq");
         strcpy(offname,"tof");
      }
      else
      {
         sprintf(freqname,"dfrq%c", (index == 2) ? '\0' : (index - 1) + '0');
         sprintf(offname,"dof%c", (index == 2) ? '\0' : (index - 1) + '0');
      }
      GPRINT1(1, "freqname = '%s'\n", freqname);
      if (useNuc == 1)
      {
         char   synthesizer[10]; /* PTS synthesizer, yes,no,reverse */
         char   frqband[10]; 	/* PTS synthesizer, yes,no,reverse */
         double nucbaseoffset;	/* base offset frequency */
         double reffreq; 	/* Reference frequency */
         if ( (strcmp(argv[1],"") == 0) || (strcmp(argv[1],"none") == 0) )
         {
            freq = 0.0;
         }
         else if (getnucinfo(h1freq,rftype[index-1],argv[1],
                  &freq, &nucbaseoffset, frqband, synthesizer, &reffreq, 1))
         {
            freq = 0.0;
         }
         else if(basefactor > 0) {
            freq *= basefactor;
	 } else {
            freq *= 1.0e6;	/* put frequency into Hz */
            freq += (freq * lkfactor);
            GPRINT1(1, "lock corrected freq= %14.8f MHz\n", freq * 1e-6);
            freq -= (freq * solvfactor);
            GPRINT1(1, "solvent corrected freq= %14.8f MHz\n", freq * 1e-6);
            freq *= 1e-6;		/* convert back to MHz */
         }
      }
      else if ( calcfreq(&freq,index,h1freq,rftype,freqname)  == COMPLETE )
      {
         if(basefactor > 0) {
            freq *= basefactor;
	 } else {
           freq *= 1.0e6;	/* put frequency into Hz */
           freq += (freq * lkfactor);
           GPRINT1(1, "lock corrected freq= %14.8f MHz\n", freq * 1e-6);
           freq -= (freq * solvfactor);
           GPRINT1(1, "solvent corrected freq= %14.8f MHz\n", freq * 1e-6);
           freq *= 1e-6;		/* convert back to MHz */
	 }
      }
      GPRINT1(1,"base frequency is %14.8f MHz\n", freq);
      GPRINT1(1,"base frequency is %14.2f Hz\n", freq * 1e6);
      rf_stepsize(&stepsize, offname, SYSTEMGLOBAL);
      GPRINT1(1,"round to nearest %g (Hz)\n", stepsize);
      offval = 0.0;
      if ((useNuc == 0) && (freq > 0.0))
      {
         if (P_getreal(CURRENT, offname, &offval, 1) < 0)
            offval = 0.0;
         GPRINT1(1,"offset is %g (Hz)\n", offval);
      }
      GPRINT2(1, "freq= %14.8f MHz  rounded freq= %14.8f MHz\n",
              freq + offval * 1e-6,
              1e-6 * round_freq(freq, offval, (double) 0.0, stepsize));
      freq = 1e-6 * round_freq(freq, offval, (double) 0.0, stepsize);
      if (retc)
      {
         if (argc > 1) {
            retv[0] = realString(freq);
         } else {
            if (index <= retc)
	       retv[index-1] = realString(freq);
         }
      }
      else if (useNuc)
         Winfoprintf("%s frequency is %g", argv[1],freq);
      else
      {
         P_setreal(CURRENT,freqname, freq, 0);
         appendvarlist(freqname);
      }
   }
   if (retc || useNuc)
      RETURN;
/*
   ct = 0.0;
   setparm("ct", "REAL", CURRENT, &ct, 1);
 */
   if (argc == 2)
   {
      /* only update rfband if a single channel is being set.
       * Otherwise, since go calls setfrq, rfband would always
       * be set to 'cc', any user input 'h' or 'l' would be
       * overwritten.
       */
      char  rfband[10];	/* RF high or low band identifier */

      if (getparm("rfband", "STRING", CURRENT, &rfband[0], 7))
            return ERROR;
      rfband[first-1] = 'c';
      rfband[(int) (numrfch + 0.1)] = '\0';
      setparm("rfband", "STRING", CURRENT, rfband, 1);
   }
   RETURN;
}

/*
 * Construct the nuctab name from the h1freq and rftype.
 * Allows more than one nuctable per 100 MHz interval.
 * Caller responsible for knowing that the "nuctable" array is big enough.
 */
static void
get_nuctab_name(double h1freq, char rftype, char *nuctable)
{
    char suffix[8];

    if (fabs(h1freq - 127) < 10){
	strcpy(suffix, "3T");
    }else if (fabs(h1freq - 170) < 10){
	strcpy(suffix, "4T");
    }else{
	sprintf(suffix, "%d", (int)h1freq / 100);
    }
    sprintf(nuctable, "nuctab%s%c", suffix, rftype);
}	

/*-----------------------------------------------------------------
|
|  getnucinfo()/8
|
|  Purpose:
|	This module opens and searches through the proper nucleus
|	table to find the named nucleus. It passed back all relevent
|	information contain in the nucleus table.
|
| -- NUCLEUS TABLE FORMAT --
| Name	Frequency	Baseoffset   FreqBand	Syn	RefFreq
|
|				Author Greg Brissey  5/12/86
+------------------------------------------------------------------*/
/* freqband;	high or low band */
/* nucname;	nucleus name */
/* rftype;	type of RF a,b,c,d */
/* syn;		presence of PTS synthesizer: yes,reverse,no */
/* basefreq;	base offset frequency (tbo,dbo) */
/* freq;	spectrometer frequency (sfrq,dfrq) */
/* h1freq;	spectrometer proton frequency */
/* reffreq;	reference frequency */
/* verbose;     flag for printing error message */

int getnucinfo(double h1freq, char rftype, char *nucname, double *freq,
               double *basefreq, char *freqband, char *syn, double *reffreq,
               int verbose)
{
   char            nuc[10];	/* name of nucleus */
   char            nuctable[15];/* name of nucleus file to search */
   char            nucpath[MAXPATHL];	/* path to nucleus table */
   FILE           *stream;
   int             ret;

   strcpy(nucpath,systemdir);
   strcat(nucpath,"/nuctables/nuctable");

   if ((stream = fopen(nucpath, "r")) == NULL)
   {
      get_nuctab_name(h1freq, rftype, nuctable);
      GPRINT1(1, "getnucinfo(): Open nucleus table '%s'\n", nuctable);
      strcpy(nucpath, systemdir);	/* construct absolute path */
      strcat(nucpath, "/nuctables/");
      strcat(nucpath, nuctable);
      if ((stream = fopen(nucpath, "r")) == 0)
      {
         Werrprintf("Cannot open the '%s' nucleus table file,",
				 nucpath);
         return (1);
      }
   }

   /* --- read in file and search for the requested nucleus --- */
   ret = fscanf(stream, "%*s%*s%*s%*s%*s%*s");	/* skip header information */
   while (ret != EOF)
   {
      ret = fscanf(stream, "%s%lf%lf%s%s%lf",
		   nuc, freq, basefreq, freqband, syn, reffreq);
      if (ret == 0)
      {
	 Werrprintf("Nucleus table's format (%s) has been corrupted.",
		    nuctable);
	 fclose(stream);
	 return (1);
      }
      GPRINT4(2, "getnucinfo(): %s - %s,%11.7lf, %11.7lf\n",
	      nucname, nuc, *freq, *basefreq);
      GPRINT4(2, "getnucinfo(): %s - %s, %s, %11.7lf\n",
	      nucname, freqband, syn, *reffreq);
      if ( ! strcmp(nucname, nuc) )
      {
	 GPRINT1(1, "getnucinfo(): Found requested Nucleus: %s\n", nuc);
	 fclose(stream);
	 return (0);
      }
   }
   if (verbose)
      Werrprintf("Requested nucleus, '%s', is not an entry in the nucleus table %s",
	      nucname, nucpath);
   fclose(stream);
   return (1);
}


#define SOLVREQUIREDPARAMS 2
/*-------------------------------------------------------------------
|    solvinfo-  Usage: solvinfo('solvent_name')
|
|   Purpose:
|  	This module retrieves the specified solvent information
|	from the solvent table.
|
|	Author   Greg Brissey   1/14/87
+-----------------------------------------------------------------*/
int solvinfo(int argc, char *argv[], int retc, char *retv[])
{
   char            solvent[MAXSTR];	/* solvent name */
   char            solvname[MAXSTR];	/* solvent name */
   char            solvpath[MAXPATHL];	/* absolute path to solvent table */
   double          dshift;              /* chemical shift */

   if (argc < SOLVREQUIREDPARAMS)
   {
      Werrprintf("Usage - %s('solvent_name') ", argv[0]);
      ABORT;
   }
   /* --- 1st parameter must be the Solvent Name to be Used --- */
   if (!isReal(argv[1]))
      strncpy(solvent, argv[1], 49);
   else
   {
      Werrprintf("1st parameter '%s' is not a Solvent Name.", argv[1]);
      ABORT;
   }
   vnmr_tolower(solvent);
   if ( strcmp(solvent, "") && strcmp(solvent, "none") )
   {
      /* construct absolute path to solvent table */
      strcpy(solvpath, systemdir);

#ifdef UNIX
      strcat(solvpath, "/solvents");
#else 
      strcat(solvpath, "solvents");
#endif 

      if (getsolventinfo(solvent, solvpath, solvname, &dshift))
	 ABORT;

      if (retc > 0)
      {
	 retv[0] = realString(dshift);
	 if (retc == 2)
            retv[1] = newString(solvname);
	 else if (retc > 2)
            Werrprintf("Only the solvent shift and name are returned by %s",
                        argv[0]);
      }
   }
   else
   {
      if (retc >= 1)
	 retv[0] = realString(0.0);
   }
   RETURN;
}

/* ---------------------------------------------------------------
|  getsolventinfo(solvent_name,solvent_table,proton_shift);
|
|  Purpose:
|	This module opens and searches through the solvent
|	table to find the named solvent. It passed back
|	the proton shift of the solvent.
|
|--  SOLVENT TABLE FORMAT ----
| Solvent	Deuterium	Melting	Boiling	Proton	Carbon Deteurium
| Name	 	Shift		Point	Point	Shifts	Shifts   T1
| ------------------------------------------------------------------------
| Proton Shift	Multiplicities	Coupling Constants
| ------------------------------------------------------------------------
| Carbon Shift	Multiplicities	Coupling Constants
| ========================================================================
|				Author Greg Brissey  5/12/86
+------------------------------------------------------------------*/

int getsolventinfo(char *solvent, char *solvtable, char *solvname,
                   double *dshift)
{
   char            sname[MAXSTR];     /* solvent name */
   FILE           *stream;
   int             ret;

   *dshift = 0.0;
   if ((stream = fopen(solvtable, "r")) == 0)
   {
      Werrprintf("Cannot open the '%s' solvent table file.", solvtable);
      return(1);
   }
   GPRINT1(2, "getsolventinfo(): Solvent to find: '%s' \n", solvent);
   while ((ret = fscanf(stream, "%s", solvname)) != EOF)
   {
      if (ret == 0)
      {
	 Werrprintf("Solvent table's format (%s) has been corrupted.",
		    solvtable);
	 fclose(stream);
	 return(1);
      }
      GPRINT1(3, "getsolventinfo(): name is %s \n", solvname);

      strcpy(sname, solvname);
      vnmr_tolower(sname);
      if ( ! strcmp(solvent, sname) )
      {
	 GPRINT1(1, "getsolventinfo(): Found requested solvent: %s\n",
		 solvent);
         ret = fscanf(stream, "%s", sname);  /* read next word which sould */
         if ((ret == 0) || (!isReal(sname))) /* be the frequency in ppm    */
         {
	    Werrprintf("Solvent table's format (%s) has been corrupted.",
		       solvtable);
	    fclose(stream);
	    return(1);
         }
	 *dshift = stringReal(sname);
	 fclose(stream);
	 return (0);
      }
   }
   Werrprintf(
	 "Requested solvent, '%s', is not an entry in the solvent table %s",
	      solvent, solvtable);
   fclose(stream);
   return (1);
}

/* ---------------------------------------------------------------
|  var_active(varname,tree);
|
|  Purpose:
|	This module determines if the variable passed is active or not.
|	This is determined by looking at the variable information
|	 structure "_vInfo".
|	if a real variable the info.active is check.
|	if a string then it's value is checked for 'unused' value.
|       Returns 1 if active, 0 if not, and -1 if error.
|
|				Author Greg Brissey  5/13/86
+------------------------------------------------------------------*/
int var_active(char *varname, int tree)
{
   char            strval[20];
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   GPRINT2(2, "active(): Variable: %s, Tree: %d \n", varname, tree);
   if ( (ret = P_getVarInfo(tree, varname, &varinfo)) )
   {
      Werrprintf("Cannot find the variable: %s", varname);
      return (-1);
   }
   GPRINT1(2, "active(): vInfo.basicType = %d \n", varinfo.basicType);
   if (varinfo.basicType != T_STRING)
   {
      GPRINT1(2, "active(): vInfo.active = %d \n", varinfo.active);
      if (varinfo.active == ACT_ON)
	 return (1);
      else
	 return (0);
   }
   else				/* --- variable is string check is value for
				 * 'undef' --- */
   {
      P_getstring(tree, varname, strval, 1, 10);
      /* getparm(varname,"string",tree,&strval[0],10); */
      GPRINT1(2, "active(): Value of String variable: '%s'\n", strval);
      if ( ( ! strncmp(strval, "unused", 6) ) ||
	  ( ! strncmp(strval, "Unused", 6) ) )
	 return (0);
      else
	 return (1);
   }
}

/*--------------------------------------------------------------
|  getparm(variable_name,variable_type,tree,variable_address,varsize)
|  char *variable_name,*variable_type,*variable_address,*tree
|  return 1 if error
|  passes back variable
|
|  varsize is the length of the string value tobe returned, no effect on reals
|
|   Author Greg Brissey   4/28/86
+---------------------------------------------------------------*/
int getparm(char *varname, char *vartype, int tree, void *varaddr, int size)
{
   int             ret;

   if ((strcmp(vartype, "REAL") == 0) || (strcmp(vartype, "real") == 0))
   {
      if ((ret = P_getreal(tree, varname, (double *) varaddr, 1)) < 0)
      {
	 Werrprintf("Cannot find parameter: %s", varname);
	 return (1);
      }
   }
   else
   {
      if ((strcmp(vartype, "STRING") == 0) ||
	  (strcmp(vartype, "string") == 0))
      {
	 if ((ret = P_getstring(tree, varname, (char *) varaddr, 1, size)) < 0)
	 {
	    Werrprintf("Cannot find parameter: %s", varname);
	    return (1);
	 }
      }
      else
      {
	 Werrprintf("Variable '%s' is neither a 'real' or 'string'.",
		    vartype);
	 return (1);
      }
   }
   return (0);
}

/*--------------------------------------------------------------
|  setparam(variable_name,variable_type,tree,variable_address,varsize)
|  char *variable_name,*variable_type,*variable_address,*tree
|  return 1 if error
|  sets variable
|
|  varsize is the length of the string value tobe returned, no effect on reals
|
|   Author Greg Brissey   4/28/86
+---------------------------------------------------------------*/
int setparm(const char *varname, const char *vartype, int tree, const void *varaddr, int index)
{
   int             ret;

   if ((strcmp(vartype, "REAL") == 0) || (strcmp(vartype, "real") == 0))
   {
      if ((ret = P_setreal(tree, varname, *(double *) varaddr, 1)) < 0)
      {
	 Werrprintf("Cannot find parameter: %s", varname);
	 return (1);
      }
   }
   else
   {
      if ((strcmp(vartype, "STRING") == 0) ||
	  (strcmp(vartype, "string") == 0))
      {
	 if ((ret = P_setstring(tree, varname, (char *) varaddr, index)) < 0)
	 {
	    Werrprintf("Cannot find parameter: %s", varname);
	    return (1);
	 }
      }
      else
      {
	 Werrprintf("Variable '%s' is neither 'real' nor 'string'.", vartype);
	 return (1);
      }
   }
   return (0);
}

int numActiveRcvrs(char *rcvrstring)
{
    /*
     * Counts the number of active receivers specified in "rcvrstring".
     * Format of string is like "ynny".
     * Short string or other characters turn those receivers off.
     */
    int i;
    int nr;
    char *pc;
    double numrcvrs;

    if (P_getreal(SYSTEMGLOBAL, "numrcvrs", &numrcvrs, 1) < 0) {
	numrcvrs = 1;
    }
    nr = (int)(numrcvrs + 0.5);
    for (pc=rcvrstring, i=0; *pc && i<nr; pc++) {
	if (*pc == 'y' || *pc == 'Y') {
	    i++;
	}
    }
    return i > 0 ? i : 1;
}

int get_username(char *username_addr, int username_len )
{

#ifdef UNIX
	int		 ulen;
	struct passwd	*pasinfo;

	pasinfo = getpwuid( getuid() );
	if (pasinfo == NULL)
	  return( -1 );
	ulen = strlen( pasinfo->pw_name );

	if (ulen >= username_len) {
		strncpy( username_addr, pasinfo->pw_name, username_len-1 );
		username_addr[ username_len-1 ] = '\0';
	}
	else
	  strcpy(username_addr, pasinfo->pw_name);

#else 
#define  JPI$_USERNAME	0x0202

	int		ulen;
        long int        cur_pid, istat;
        char            user_name[14];
        char            *tptr;

        extern long int SYS$GETJPI(), SYS$WAITFR();

        struct {
                short int       buf_len;
                short int       item_code;
                int             *buf_adr;
                int             *ret_len_adr;
        } req_item[2] = 
          {
                { 12,  JPI$_USERNAME, &user_name[0],  NULL },
                { 0,   0,             NULL,           NULL },
          };

        cur_pid = getpid();
        istat = SYS$GETJPI( 1, &cur_pid, 0, &req_item[0], 0, 0, 0 );
        if ( (istat & 3) != 1 )
          return( -1 );
        istat = SYS$WAITFR( 1 );
        if ( (istat & 3) != 1 )
          return( -1 );

        user_name[ 12 ] = user_name[ 13 ] = '\0';
        tptr = strchr( &user_name[ 0 ], ' ' );
        if (tptr != NULL)
          *tptr = '\0';
        ulen = strlen( &user_name[ 0 ] );
	if (ulen >= username_len) {
		strncpy( username_addr, &user_name[ 0 ], username_len-1 );
		username_addr[ username_len-1 ] = '\0';
	}
	else
	  strcpy(username_addr, &user_name[ 0 ]);
#endif 

	return( 0 );
}

/*--------------------------------------------------------------
 * Record a new interactive program in the pid list.
 * Note that the space malloced for the entry is not released
 * when the program exits.  Once we make an entry for a specific
 * "tag", it is reserved for use for that program, and only the
 * pid and connected entries are changed.  This is to avoid any
 * problems with race conditions.
 * Note that we can call this with pid=0 to just make an entry
 * for a program that is not yet alive.
 *---------------------------------------------------------------*/
/*  tag			Tagname of program */
/*  prog		Program name */
/*  pid			PID of new program */
void interact_birth_record(char *tag, char *prog, int pid)
{
    PidList *pl;
    PidList *pl_prev = NULL;

    for (pl=interact_pid_list ; pl; pl=pl->next) {
	if (strcasecmp(pl->tagname, tag) == 0){
	    /* Already have a space for this guy, use it. */
	    break;
	}
	pl_prev = pl;
    }
    if (!pl){
	/* Did not find an entry for this guy, allocate space. */
	pl = (PidList *)malloc(sizeof(PidList));
	if (!pid){
	    perror("interact_birth_record(): error making entry");
	    exit(1);
	}
	pl->progname = strdup(prog);
	pl->tagname = strdup(tag);
	if (pl->progname == NULL || pl->tagname == NULL){
	    fprintf(stderr,"interact_birth_record(): out of memory\n");
	    exit(1);
	}
	pl->next = NULL;
	if (pl_prev){
	    pl_prev->next = pl;
	}else{
	    interact_pid_list = pl;
	}
    }
    pl->connected = 0;
    pl->pid = pid;
    /* Just in case this program started and connected before this
     * entry got made, update the connection status flags. */
#ifdef VERSION5
    interact_connect_status();
#endif 
}

/*--------------------------------------------------------------
 * Check to see if an interactive acquisition program is running.
 * If "tag" points to a non-null string, and a program with that tag
 * is running, returns the PID of the program; if it is not running
 * returns 0.
 * If "tag" is null or points to a null string, and any interactive
 * program is running, returns the PID of some running program; if
 * no interactive program is running, returns 0.
 *---------------------------------------------------------------*/
int
interact_is_alive(char *tag, char *message)
{
    PidList *pl = interact_pid_list;
    int  rtn;
    char cmd[256];

    rtn = 0;
    for ( ; pl; pl=pl->next){
	if (!tag || !*tag || strcasecmp(pl->tagname, tag) == 0){
	    if (pl->pid){
		int err;
		err = kill(pl->pid, 0); /* Test for existence */
		if (err == -1 && errno == ESRCH){
		    pl->pid = pl->connected = 0;
		}
		else
                {
                   sprintf(cmd,"(umask 0; echo %s > %s/acqqueue/%s_info_%d)\n",
                        message, systemdir, pl->tagname, pl->pid);
                   system(cmd);
                   kill(pl->pid, SIGUSR2);      /* Issue  command */
                }
	    }
	    rtn = pl->pid;
	    break;
	}
    }
    return rtn;
}

/*--------------------------------------------------------------
 * interact_is_connected()
 * Check to see if an interactive acquisition program is connected.
 * If "tag" points to a non-null string, and a program with that tag
 * is connected, returns the PID of the program; if it is not connected
 * returns 0.
 * If "tag" is null or points to a null string, and any interactive
 * program is connected, returns the PID of the connected program; if
 * no interactive program is connected, returns 0.
 *--------------------------------------------------------------*/
int
interact_is_connected(char *tag)
{
    PidList *pl = interact_pid_list;
    int rtn = 0;

    if (tag && *tag){
	/* Check if the named program is connected */
	for ( ; pl; pl=pl->next){
	    if (strcasecmp(pl->tagname, tag) == 0){
		if (pl->connected){
		    rtn = pl->pid;
		}
		break;
	    }
	}
    }else{
	/* Return PID of last (should be only) connected program */
	for ( ; pl; pl=pl->next){
	    if (pl->connected){
		rtn = pl->pid;
	    }
	}
    }
    return rtn;
}

/*--------------------------------------------------------------
 * interact_disconnect()
 * Command an interactive acquisition program to disconnect itself.
 * If "tag" points to a non-null string, and a program with that tag
 * is connected, disconnects that program; if the program does not
 * exist or is not connected, does nothing.
 * If "tag" is null or points to a null string, disconnects
 * all interactive programs (but only one should be connected at a time).
 *--------------------------------------------------------------*/
void interact_disconnect(char *tag)
{
    PidList *pl = interact_pid_list;


    if (tag && *tag){
	/* Disconnect the named program */
	for ( ; pl; pl=pl->next){
	    if (strcasecmp(pl->tagname, tag) == 0){
		/* Disconnect this guy */
		interact_disconnect_by_pid(pl);
		break;
	    }
	}
    }else{
	/* Disconnect all progs (should be only one connected) */
	for ( ; pl; pl=pl->next){
	    if (pl->connected){
		interact_disconnect_by_pid(pl);
	    }
	    /* Go ahead and check everyone else too (no "break")*/
	}
    }
}

/*--------------------------------------------------------------
 * interact_disconnect_by_pid()
 * Command an interactive acquisition program to disconnect itself.
 * If "pl" points to the PidList entry of a connected program,
 * that program is disconnected; otherwise, does nothing.
 * Waits for the program to disconnect before returning.
 *--------------------------------------------------------------*/
static void interact_disconnect_by_pid(PidList *pl)
{
    char cmd[256];

    if (pl && pl->pid && pl->connected){
        sigset_t        emptymask;

        sigemptyset( &emptymask );
	sprintf(cmd,"(umask 0; echo go > %s/acqqueue/%s_info_%d)\n",
		systemdir, pl->tagname, pl->pid);
	system(cmd);
	kill(pl->pid, SIGUSR2);	/* Issue disconnect command */
	while (pl->connected){
	    sigsuspend( &emptymask );	/* Wait for a signal */
	}
    }
}

/*--------------------------------------------------------------
 * interact_kill()
 * Command an interactive acquisition program to kill itself.
 * If "tag" points to a non-null string, and a program with that tag
 * is running, kills that program; if the program does not
 * exist, does nothing.
 * If "tag" is null or points to a null string, kills
 * all interactive programs.
 *--------------------------------------------------------------*/
void interact_kill(char *tag)
{
    PidList *pl = interact_pid_list;

    if (tag && *tag){
	/* Kill the named program */
	for ( ; pl; pl=pl->next){
	    if (strcasecmp(pl->tagname, tag) == 0){
		/* Kill this guy */
		interact_kill_by_pid(pl);
		break;
	    }
	}
    }else{
	/* Kill all progs */
	for ( ; pl; pl=pl->next){
	    if (pl->pid){
		interact_kill_by_pid(pl);
	    }
	}
    }
}

/*--------------------------------------------------------------
 * interact_kill_by_pid()
 * Command an interactive acquisition program to kill itself.
 * If "pl" points to the PidList entry of a running program,
 * that program is killed; otherwise, does nothing.
 * Waits for the program to disconnect (but not to die) before returning.
 *--------------------------------------------------------------*/
static void interact_kill_by_pid(PidList *pl)
{
    char cmd[256];
    int err;
    
    if (pl && pl->pid){
	err = kill(pl->pid, 0);
	if (err == -1 && errno == ESRCH){
	    pl->pid = pl->connected = 0;
	}else{
            sigset_t        emptymask;

            sigemptyset( &emptymask );
	    sprintf(cmd, "(umask 0; echo exit > %s/acqqueue/%s_info_%d)\n"
		    ,systemdir, pl->tagname, pl->pid);
	    system(cmd);
	    kill(pl->pid, SIGUSR2);
	    while (pl->pid && pl->connected){ /* Flags changed by signal */
		sigsuspend( &emptymask );	/* Wait for a signal */
	    }
	}
    }
}

/*--------------------------------------------------------------
 * interact_connect_status()
 * Sets the "connection" status flags of all interactive programs
 * that are currently running.
 *--------------------------------------------------------------*/
void
interact_connect_status()
{
    PidList *pl = interact_pid_list;
    char path[256];
    int err;

    for ( ; pl; pl=pl->next)
    {
	if (pl->pid > 0)
  	{
	    err = kill(pl->pid, 0); /* Test for existence */
	    if (err == -1 && errno == ESRCH) {
		if (pl->pid == acqi_pid)
		   sendTripleEscToMaster('a', "False");
		pl->pid = pl->connected = 0;
	    }
	    else {
		sprintf(path,"%s/acqqueue/%s_%d",
			systemdir, pl->tagname, pl->pid);
		pl->connected = (access(path, F_OK) == 0);
		if (pl->pid == acqi_pid)
		{
		    if (pl->connected)
		        sendTripleEscToMaster('a', "False");
		    else
		        sendTripleEscToMaster('A', "True");
		}
	    }
	}
    }
}

/*--------------------------------------------------------------
 * interact_obituary()
 * Clears the "pid" and "connection" status flags of the interactive
 * program with the given "pid".  This records the program as dead.
 * Note that we do not remove the entry from the pid_list, because that
 * would be unsafe in an interrupt routine.
 *--------------------------------------------------------------*/
void
interact_obituary(int pid)
{
    PidList *pl = interact_pid_list;

    if (pid > 0){
	for ( ; pl; pl=pl->next){
	    if (pl->pid == pid){
		if (pid == acqi_pid)
		{
		    acqi_pid = -9;
		    sendTripleEscToMaster('a', "False");
		}
		pl->pid = pl->connected = 0; /* Indicates death */
		GPRINT1(0, "%s terminated\n", pl->tagname);
		break;
	    }
	}
    }
}

static long getOneAtTime(int *hr, int *min)
{
   struct timeval clock;
   struct tm *local;

   gettimeofday(&clock, NULL);
   clock.tv_sec += *hr;
   local = localtime((long*) &(clock.tv_sec));
   *hr = local->tm_hour;
   *min = local->tm_min;
   return(clock.tv_sec);
}

static long getAtTime(int hr, int min, char *timespec, int *repeatAt)
{
   struct timeval clock;
   struct tm *local;
   int incr = 0;
   int days[7];

   gettimeofday(&clock, NULL);
   local = localtime((long*) &(clock.tv_sec));
   *repeatAt = 0;
   if (strlen(timespec) )
   {
      char tmpStr[256];
      int i;
      char *pTmpStr;

      strcpy(tmpStr,timespec);
      pTmpStr = strtok(tmpStr," \n");
      for (i=0; i< 7; i++)
         days[i] = 0;
      while (pTmpStr != NULL)
      {
         /* handle sun mon tue wed thur fri sat */
         /* handle su  m   tu  w   th   f   sa  */

         if ( (*pTmpStr == 's') || (*pTmpStr == 'S') )
         {
           if ( (*(pTmpStr+1) == 'u') || (*(pTmpStr+1) == 'U') )
             days[0] = 1;
           else if ( (*(pTmpStr+1) == 'a') || (*(pTmpStr+1) == 'A') )
             days[6] = 1;
         }
         else if ( (*pTmpStr == 'm') || (*pTmpStr == 'M') )
         {
             days[1] = 1;
         }
         else if ( (*pTmpStr == 't') || (*pTmpStr == 'T') )
         {
           if ( (*(pTmpStr+1) == 'u') || (*(pTmpStr+1) == 'U') )
             days[2] = 1;
           else if ( (*(pTmpStr+1) == 'h') || (*(pTmpStr+1) == 'H') )
             days[4] = 1;
         }
         else if ( (*pTmpStr == 'w') || (*pTmpStr == 'W') )
         {
             days[3] = 1;
         }
         else if ( (*pTmpStr == 'f') || (*pTmpStr == 'F') )
         {
             days[5] = 1;
         }
         pTmpStr = strtok(NULL, " \n");
      }
      for (i=0; i< 7; i++)
         if (days[i])
         {
            *repeatAt = 1;
         }
      /* earlier in the day on a requested day */
      if (*repeatAt && days[local->tm_wday] &&
          ( (hr > local->tm_hour) ||
          ( (hr == local->tm_hour) && (min > local->tm_min) )) )
      {
         ;  /* Do nothing special */
      }
      else if ( *repeatAt )
      {
         int i;
         int day;

         /* incr by 24 hrs until a specified day is matched */
         day = local->tm_wday + 1;
         for (i = 0; i < 7; i++)
         {
            if (day == 7)
              day = 0;
            hr += 24;
            if (days[day])
            {
              break;
            }
            day++;
         }
      }
   }
   if ( (hr < local->tm_hour) ||
      ( (hr == local->tm_hour) && (min <= local->tm_min) ) )
   {
      /* set for next day */
      hr += 24;
   }
   incr = 0;
   if (hr > local->tm_hour)
   {
      incr =  60 - local->tm_min;
      incr += (60 * (hr - local->tm_hour - 1) );
      incr += min;
   }
   else if ( (hr == local->tm_hour) && (min > local->tm_min) )
   {
      incr = min - local->tm_min;
   }
   return(clock.tv_sec + incr*60);
}

int atCmd(int argc, char *argv[], int retc, char *retv[])
{
   FILE *fd;
   FILE *fdTmp;
   char atPath[MAXPATH];
   char atPathTmp[MAXPATH];
   long atTime;
   int uid;
   int gid;
   int umask4Vnmr;
   int hr;
   int min;
   int res;
   char user[MAXSTR];
   char host[MAXSTR];
   char timespec[MAXSTR];
   char atUserDir[MAXSTR];
   char cmd[MAXSTR];
   char cmd2[MAXSTR];
   char cmd2do[MAXSTR];
   int port, pid;
   int matchOnly;
   struct timeval clock;
   struct tm *local;
   int repeatAt;
   char *argvTmp[1];

   strcpy(atPath, systemdir);
   strcat(atPath, "/acqqueue/atcmd");
   if (argc == 1)
   {
      int doCmd;

      /*  1. Check atcmd file for commands to execute */
      /*  2. Update atcmd file */
      /*  3. Send message to Atproc */
      /*  4. Execute the appropriate commands */
      lockAtcmd(systemdir);
      fd = fopen(atPath,"r");
      if ( (fd == NULL) || !Bnmr)
      {
         /* just send message to Atproc */
         unlockAtcmd(systemdir);
         argvTmp[0] = "atcmd";
         acqproc_msge(1, argvTmp, 0, NULL);
         if (fd != NULL)
            fclose(fd);
         RETURN;
      }
      strcpy(atPathTmp, atPath);
      strcat(atPathTmp,".bk");
      fdTmp = fopen(atPathTmp,"w");
      if (fdTmp == NULL)
      {
         fclose(fd);
         unlockAtcmd(systemdir);
         RETURN;
      }
      gettimeofday(&clock, NULL);
      local = localtime((long*) &(clock.tv_sec));
      doCmd = 0;
      strcpy(timespec,"");
      while ( (res = fscanf(fd,"%ld %d %d %d %s %s %s %d:%d%[^;]; %[^\n]\n",
                      &atTime, &uid, &gid, &umask4Vnmr, host, atUserDir,
                      user, &hr, &min, timespec, cmd) ) == 11)
      {
         if ( !doCmd && ( atTime <= clock.tv_sec) && !strcmp(user, UserName) )
         {
            atTime = getAtTime(hr, min, timespec, &repeatAt);
            strcpy(cmd2do,cmd);
            strcat(cmd2do,"\n");
            res = sscanf(cmd2do,"%d %d %[^\n]\n", &port, &pid, cmd2);
            if (res == 3)
            {
		int err;
		err = kill(pid, 0); /* Test for existence */
		if (err == -1 && errno == ESRCH)
                {
                   repeatAt = 0;
		}
		else
                {
                   sprintf(cmd2do,"write('net','%s',%d,`%s`)\n", host, htons(port), cmd2);
                   doCmd = 1;
                }
               
            }
            else
            {
               char op[MAXPATH];

               res = sscanf(cmd2do,"operator:%[^;]; %[^\n]\n", op, cmd2);
               if (res == 2)
               {
                  if ( strcmp(op,OperatorName) && strcmp(op, "") )
                  {
                     strcpy(OperatorName,op);
                     P_setstring(GLOBAL,"operator",OperatorName,0);
                     setAppdirs();
                  }
                  sprintf(cmd2do,"%s\n", cmd2);
               }
               doCmd = 1;
            }
            if (repeatAt)
               fprintf(fdTmp,"%ld %d %d %d %s %s %s %d:%d%s; %s\n",
                      atTime, uid, gid, umask4Vnmr, host, atUserDir,
                      user, hr, min, timespec, cmd);
         }
         else
         {
            fprintf(fdTmp,"%ld %d %d %d %s %s %s %d:%d%s; %s\n",
                      atTime, uid, gid, umask4Vnmr, host, atUserDir,
                      user, hr, min, timespec, cmd);
         }
      }
      fclose(fd);
      fclose(fdTmp);
      unlink(atPath);
      rename(atPathTmp, atPath);
      chmod(atPath,0666);
      unlockAtcmd(systemdir);
      /* send message to atproc */
      argvTmp[0] = "atcmd";
      if (Bnmr)
         sleepMilliSeconds(10);
      if (! acqproc_msge(1, argvTmp, 0, NULL))
      {
         if (doCmd)
            execString(cmd2do);
      }
   }
   else if ( (argc == 3) &&  ! strcmp(argv[2],"list") )
   {

      matchOnly = strcmp(argv[1],"");
      if (retc)
      {
         int i = 0;
         while (i < retc)
         {
            retv[i] = newString("");
            i++;
         }
      }
      lockAtcmd(systemdir);
      fd = fopen(atPath,"r");
      if (fd == NULL)
      {
         if ( ! retc)
            Winfoprintf("No atcmd's defined");
         unlockAtcmd(systemdir);
         RETURN;
      }
      if ( ! retc)
         Wscrprintf("User       Time                               Command\n");
      while ( (res = fscanf(fd,"%ld %d %d %d %s %s %s %d:%d%[^;]; %[^\n]\n",
                      &atTime, &uid, &gid, &umask4Vnmr, host, atUserDir,
                      user, &hr, &min, timespec, cmd) ) == 11)
      {
         strcpy(cmd2do,cmd);
         strcat(cmd2do,"\n");
         res = sscanf(cmd2do,"%d %d %[^\n]\n", &port, &pid, cmd2);
         if (res == 3)
         {
            strcpy(cmd,cmd2);
         }
         else
         {
            char op[MAXPATH];

            res = sscanf(cmd2do,"operator:%[^;]; %[^\n]\n", op, cmd2);
            if (res == 2)
            {
               strcpy(cmd,cmd2);
            }
         }
         if ( !matchOnly)
            Wscrprintf("%-10s %d:%.2d %-28s %s\n", user, hr, min, timespec, cmd);
         else if ( ! strcmp(argv[1],cmd) )
         {
            if (retc)
            {
               sprintf(cmd2,"%d:%.2d %-28s", hr, min, timespec);
               retv[0] = newString(cmd2);
               if (retc > 1)
                  retv[1] = newString(user);
            }
            else
               Wscrprintf("%-10s %d:%.2d %-28s %s\n", user, hr, min, timespec, cmd);
         }
      }
      fclose(fd);
      unlockAtcmd(systemdir);
   }
   else if ( (argc == 3) &&  ! strcmp(argv[2],"cancel") )
   {
      matchOnly = strcmp(argv[1],"");
      if ( ! matchOnly )
      {
         unlink(atPath);
      }
      else
      {
         lockAtcmd(systemdir);
         fd = fopen(atPath,"r");
         if (fd == NULL)
         {
            unlockAtcmd(systemdir);
            RETURN;
         }
         strcpy(atPathTmp, atPath);
         strcat(atPathTmp,".bk");
         fdTmp = fopen(atPathTmp,"w");
         if (fdTmp == NULL)
         {
            fclose(fd);
            unlockAtcmd(systemdir);
            RETURN;
         }
         while ( (res = fscanf(fd,"%ld %d %d %d %s %s %s %d:%d%[^;]; %[^\n]\n",
                      &atTime, &uid, &gid, &umask4Vnmr, host, atUserDir,
                      user, &hr, &min, timespec, cmd) ) == 11)
         {
            char cmdTest[MAXSTR];
            strcpy(cmd2do,cmd);
            strcpy(cmdTest,cmd);
            strcat(cmd2do,"\n");
            res = sscanf(cmd2do,"%d %d %[^\n]\n", &port, &pid, cmd2);
            if (res == 3)
            {
               strcpy(cmdTest,cmd2);
            }
            else
            {
               char op[MAXPATH];

               res = sscanf(cmd2do,"operator:%[^;]; %[^\n]\n", op, cmd2);
               if (res == 2)
               {
                  strcpy(cmdTest,cmd2);
               }
            }
            if ( strcmp(argv[1],cmdTest) )
               fprintf(fdTmp,"%ld %d %d %d %s %s %s %d:%d%s; %s\n",
                      atTime, uid, gid, umask4Vnmr, host, atUserDir,
                      user, hr, min, timespec, cmd);
         }
         fclose(fd);
         fclose(fdTmp);
         unlink(atPath);
         rename(atPathTmp, atPath);
         chmod(atPath,0666);
         unlockAtcmd(systemdir);
      }
   }
   else if (argc >= 3)
   {
      int startFlag = 0;
      int activeFlag = 0;

      strcpy(cmd,argv[2]);
      strcat(cmd,";");
      res = sscanf(cmd,"%d:%d %[^;];", &hr, &min, timespec);
      if (res < 1)
      {
         Werrprintf("atcmd timespec format is incorrect");
         ABORT;
      }
      else if (res == 1)
      {
         strcpy(timespec,"once");
      }
      else if (res == 2)
      {
         strcpy(timespec,"once");
      }
      lockAtcmd(systemdir);
      fd = fopen(atPath,"a");
      if (fd == NULL)
      {
         Werrprintf("atcmd: cannot open file %s", atPath);
         unlockAtcmd(systemdir);
         RETURN;
      }
      uid = getuid();
      gid = getgid();
      umask4Vnmr = umask(0);  /* save as octal */
      umask(umask4Vnmr);  /* reset it */
      if (res == 1)
         atTime = getOneAtTime(&hr, &min);
      else
         atTime = getAtTime(hr, min, timespec, &repeatAt);
      if (argc >= 4)
      {
         if ( ! strcmp(argv[3],"start") )
            startFlag = 1;
         else if ( ! strcmp(argv[3],"active") )
            activeFlag = 1;
         else
         {
            Werrprintf("Unknown argument '%s' to atcmd",argv[3]);
            unlockAtcmd(systemdir);
            fclose(fd);
            ABORT;
         }
         if (argc == 5)
         {
            if ( ! strcmp(argv[4],"start") )
               startFlag = 1;
            else if ( ! strcmp(argv[4],"active") )
               activeFlag = 1;
            else
            {
               Werrprintf("Unknown argument '%s' to atcmd",argv[4]);
               unlockAtcmd(systemdir);
               fclose(fd);
               ABORT;
            }
         }
      }
      /* global HostName, userdir, and UserName */
      if (activeFlag)
      {
         char addr[MAXPATH];
         char path[MAXPATH];
         int pid, port;

         P_getstring(GLOBAL,"vnmraddr",addr,1,MAXPATH);
         sscanf(addr,"%s %d %d", path, &port, &pid);
         fprintf(fd,"%ld %d %d %d %s %s %s %d:%d%s; %d %d %s\n",
                      atTime, uid, gid, umask4Vnmr, HostName, userdir,
                      UserName, hr, min, timespec, port, pid, argv[1]);
      }
      else
      {
         char op[MAXSTR];

         P_getstring(GLOBAL,"operator",op,1,MAXSTR);
         fprintf(fd,"%ld %d %d %d %s %s %s %d:%d%s; operator:%s; %s\n",
                      atTime, uid, gid, umask4Vnmr, HostName, userdir,
                      UserName, hr, min, timespec, op, argv[1]);
      }
      fclose(fd);
      chmod(atPath,0666);
      unlockAtcmd(systemdir);
      if ( startFlag )
      {
         if (Bnmr)
            sleepMilliSeconds(10);
         /* send message to Atproc */
         argvTmp[0] = "atcmd";
         acqproc_msge(1, argvTmp, 0, NULL);
      }
   }
   else
   {
      Werrprintf("Wrong number of arguments (%d) for atcmd",argc - 1);
      ABORT;
   }
   RETURN;
}
