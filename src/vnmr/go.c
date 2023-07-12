/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define _FILE_OFFSET_BITS 64

#include "vnmrsys.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/statvfs.h>
#include <sys/file.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>

/* #ifdef JPSG */
/* for socket jpsg ?? */
#include <sys/socket.h>
#include "sockets.h"
/* #endif */

#include "data.h"
#include "CSfuncs.h"
#include "group.h"
#include "params.h"
#include "bgvars.h"
#include "symtab.h"
#include "tools.h"
#include "variables.h"
#include "REV_NUMS.h"
#include "locksys.h"
#include "STAT_DEFS.h"
#include "acquisition.h"
#include "whenmask.h"
#include "vfilesys.h"
#include "pvars.h"
#include "wjunk.h"
#include "sockinfo.h"
#include "allocate.h"

#ifdef CLOCKTIME
extern int go_timer_no;
#endif 



/*-------------------------------------------------------------------
|    GO:
| 
|   Purpose:
|  	This module reads in the user defined and set paramenters
| 	along with the selected Pulse sequence to create the
| 	command codes for the acquisition computer.
| 
|   Routines:
| 	getsysp - obtain the system configuration parameters and
| 		  set the system global flags accordingly.
| 	main  -  GO main function.
|	Author   Greg Brissey	4/25/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   5/08/89    Greg B.     1. ra checks for ra lkfile 'acq_stopped' if not 
|			      present, ra not permitted.
|			      ra lkfile is removed for any alaises of go
|
+-------------------------------------------------------------------*/

#define CALLNAME 0
#define ARG1 1
#define EXEC_GO 0
#define EXEC_SU 1
#define EXEC_SHIM 2
#define EXEC_LOCK 3
#define EXEC_SPIN 4
#define EXEC_CHANGE 5
#define EXEC_SAMPLE 6
#define EXEC_EXPTIME 101
#define EXEC_DPS 102
#define EXEC_CHECK 103
#define EXEC_CREATEPARAMS 104
#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1
#define MAXARYS 256
#define BUFSIZE 1024
#define MAXSIZEFUDGE 256.0	/* kbytes */
#define KBYTE 1024		/* bytes per kbyte */
#define MBYTE 1048576		/* bytes per MB */
#define GBYTE 1073741824	/* bytes per GB */
#define MAXFIDSIZEFUDGE_MB (256.0/1048576.0) 	/* in MB */
#define MAXnD 4

#define USE_PSG 100
#define USE_JPSG 200

extern int mode_of_vnmr;
extern int   VnmrJViewId;
extern int   jParent;
extern void     setSilentMode(int);
extern int      is_datastation();
extern int      macroLoad(char *name, char *filepath);
extern void     purgeOneMacro(char *n);
extern int      calledFromWerr();
extern char     psgaddr[];
extern pid_t    HostPid;
#ifdef VNMRJ
extern void stop_nvlocki();
extern int  nvAcquisition();
#endif
extern int getparm(char *varname, char *vartype, int tree,
                   void *varaddr, int size);
extern int setparm(char *varname, char *vartype, int tree,
                   void *varaddr, int index);
extern int var_active(char *varname, int tree);
extern void disp_acq(char *t);
extern void currentDate(char *cstr, int len );
extern char *currentDateLocal(char *cstr, int len );
extern char *currentDateSvf(char *cstr, int len );
extern void Wturnoff_buttons();
extern int numActiveRcvrs(char *rcvrstring);
extern int read_acqi_pars();
extern int stop_acqi( int abortall );
extern int setfr(int argc, char *argv[], int retc, char *retv[]);
extern void saveGlobalPars(int sv, char *suff);
extern int WgraphicsdisplayValid(char *n);
extern int release_console();
extern int set_wait_child(int pid);
extern void set_effective_user();
extern int GetAcqStatus(int to_index, int from_index, char *host, char *user);
extern int getAcqStatusInt(int index, int *val);
extern int getAcqStatusStr(int index, char *val, int maxlen);
extern int getAcqConsoleID();
extern int expdir_to_expnum(char *expdir);
extern int is_data_present(int this_expnum );
extern int p11_saveFDAfiles_raw(char* func, char* orig, char* dest);
extern int setfrq(int argc, char *argv[], int retc, char *retv[]);
extern int verify_fnameChar(char tchar);
extern int check_ShimPowerPars();
extern int do_mkdir(char *dir, int psub, mode_t mode);
extern int Rmdir(char *dirname, int rmParent);

extern int psg_pid;

/* --- child process variables */
static int child;
static int pipe1[2];
static int pipe2[2];

static char  arrayname[MAXSTR];/* array variable name at present 'array' */
static char  autodir[MAXPATH]; /* path to automation directory */
static char  callname[MAXSTR]; /* command name alias */

static double priority;   /* experiment priority */
static double  arraydim;  /* The calc # of fids obtain from the arrayed var */
static double  acqcycles;  /* Number of acode sets */
static double  arrayelemts; /* The calc # of array elements */

static int   automode;    /* 1 if system is in auto sample mode */
static int   vpmode;      /* 1 if system is in viewport mode */
static int   suflag;	  /* setup flag,0=GO,1-6=different alias's of GO */
static int   noGainTest = 0;
static int   d2Array, d3Array, d4Array, d5Array;

//  Optional params to go into accounting log file
typedef struct  {
     char name[64];
     char tree[32];
} _optpars;

static _optpars optParams[64];

static int optParamsFilled=FALSE;

#ifdef  DEBUG
extern int   Eflag,Gflag;
#define EPRINT(arg1, arg2, arg3) \
	if (Eflag) Wtimeprintf(arg1,arg2,arg3)
#define GPRINT(level, str) \
	if (Gflag >= level) Wscrprintf(str)
#define GPRINT1(level, str, arg1) \
	if (Gflag >= level) Wscrprintf(str,arg1)
#define GPRINT2(level, str, arg1, arg2) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2)
#define GPRINT3(level, str, arg1, arg2, arg3) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3)
#define GPRINT4(level, str, arg1, arg2, arg3, arg4) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3,arg4)
static int Wtimeprintf(int mode, char *control, char *time);
#else 
#define EPRINT(arg1, arg2, arg3)
#define GPRINT(level, str) 
#define GPRINT1(level, str, arg2) 
#define GPRINT2(level, str, arg1, arg2) 
#define GPRINT3(level, str, arg1, arg2, arg3) 
#define GPRINT4(level, str, arg1, arg2, arg3, arg4) 
#endif 

/* Function prototypes */

static int argtest(int, char * [], char *);
static int set_options(int, char * [], int restart, char *a_name_option, int *overridespin);
static int do_space_check(int);
static int check_status_console(char *, char *);
static int savepars(char *, int debugPutCmd);
static int saveVpPars(char *acqfile, int debugPutCmd);
static int copytext(char *);
static int check_acqpar();
static int check_loc();
static int check_ra();
static int check_np_ra();
static int check_dp_ra();
static int get_number_new_fids(int);
static int validcall(int, char * [], int *, int);
static int testarrayparm(char *, char * [], int *);
static int checkparm(char *, char *, char * [], int *);
static int equalsize(char *, char *);
static int findname(char *, char * [], int);
static int removename(int, char * [], int *);
static int A_getarynames(int, char **, int *, int); 
static int getNames(symbol *, char **, int *, int , int *); 
static int dateStr(char *, char *, char **, int *);
static void getStr(char *, FILE *, char **, int *);
static void replaceSpace(char *);
static int fireUpJPSG(void);
void createActiveFlagParameter();
void getVnmrInfo(int okToSet, int okToSetSpin, int overRideSpin);
void cleanup_pars();
static char *named_string_arg(int argc, char **argv, char *name);
static int jacq(int acqi_fid, int spinCadCheck,
                char *hostname, int retc, char *retv[]);
static int sacq(int acqi_fid, char *psgpath, int retc, char *retv[]);
static int test4PS(char *psgpath);
static int test4meth(char *method, char *methodpath);
static int test4Lock();
static int test4ACQ(char *dirname, char *dirpath, int acqiflag);
static void runPsgPutCmd(int debugPutCmd);
int arraytests();
int initacqqueue(int argc, char *argv[]);
int makeautoname(char *cmdname, char *a_name, char *sif_name, char *dirname,
                 int createflag, int replaceSpaceFlag,
                 char *suffix, char *notsuffix);
int isJpsgReady();


void skipGainTest()
{
  noGainTest = 1;
}

double get_acq_dim() /* could run getdim macro */
{
    double rval, dval;

    if (P_getreal(CURRENT,"nD",&dval,1) == 0)
    {
      if (dval > 0.5)
        return( dval );
    }
    rval = 1.0;
    if (P_getreal(CURRENT,"ni",&dval,1) == 0)
    {
      if (dval > 1.5)
        rval++;
    }
    if (P_getreal(CURRENT,"ni2",&dval,1) == 0)
    {
      if (dval > 1.5)
        rval++;
    }
    if (P_getreal(CURRENT,"ni3",&dval,1) == 0)
    {
      if (dval > 1.5)
        rval++;
    }
    return( (double)((int)(rval+0.5)) );
}

void set_vnmrj_acq_params()
{
    char     ptmp[MAXPATH], mstr[256];
    int      ict;
    double   adim;

    ict = 0;
    strcpy(mstr,"   ");
    currentDate(ptmp, MAXPATH);
    if (P_setstring(CURRENT,"time_submitted",ptmp,1))
    {
      P_creatvar(CURRENT,"time_submitted",T_STRING);
      P_setstring(CURRENT,"time_submitted",ptmp,1);
    }
    strcat(mstr,"time_submitted ");
    ict++;
    if (P_setstring(CURRENT,"time_run",ptmp,1))
    {
      P_creatvar(CURRENT,"time_run",T_STRING);
      P_setstring(CURRENT,"time_run",ptmp,1);
    }
    strcat(mstr,"time_run ");
    ict++;
    if (P_setstring(CURRENT,"time_complete",ptmp,1))
    {
      P_creatvar(CURRENT,"time_complete",T_STRING);
      P_setgroup(CURRENT,"time_complete",G_DISPLAY);
      P_setstring(CURRENT,"time_complete",ptmp,1);
    }
    strcat(mstr,"time_complete ");
    ict++;
    currentDateLocal(ptmp, MAXPATH);
    if (P_setstring(CURRENT,"time_submitted_local",ptmp,1))
    {
      P_creatvar(CURRENT,"time_submitted_local",T_STRING);
      P_setstring(CURRENT,"time_submitted_local",ptmp,1);
    }
    strcat(mstr,"time_submitted_local ");
    ict++;
    currentDateSvf(ptmp, MAXPATH);
    if (P_setstring(CURRENT,"time_svfdate",ptmp,1))
    {
      P_creatvar(CURRENT,"time_svfdate",T_STRING);
      P_setstring(CURRENT,"time_svfdate",ptmp,1);
    }
    strcat(mstr,"time_svfdate ");
    ict++;
    if ( ! P_setstring(CURRENT,"time_processed","",1))
    {
      strcat(mstr,"time_processed ");
      ict++;
    }
    if ( ! P_setstring(CURRENT,"time_plotted","",1))
    {
      strcat(mstr,"time_plotted ");
      ict++;
    }
    if ( ! P_setstring(CURRENT,"time_saved","",1))
    {
      strcat(mstr,"time_saved ");
      ict++;
    }
    adim = get_acq_dim();
    P_setreal(CURRENT, "acqdim", adim, 1);
    if (P_setreal(CURRENT,"acqdim", adim, 1))
    {
      P_creatvar(CURRENT,"acqdim",ST_INTEGER);
      P_setreal(CURRENT, "acqdim", adim, 1);
    }
    strcat(mstr,"acqdim ");
    ict++;
    if (P_setreal(CURRENT,"procdim", 0.0, 1))
    {
      P_creatvar(CURRENT,"procdim",ST_INTEGER);
      P_setreal(CURRENT, "procdim", 0.0, 1);
    }
    P_setgroup(CURRENT,"procdim",G_PROCESSING);
    strcat(mstr,"procdim ");
    ict++;
    if (P_setstring(CURRENT,"proccmd","",1))
    {
       P_creatvar(CURRENT,"proccmd",T_STRING);
       P_setgroup(CURRENT,"proccmd", G_PROCESSING);
       P_setstring(CURRENT,"proccmd","",1);
       P_setprot(CURRENT,"proccmd",P_ARR | P_ACT | P_VAL);  /* do not allow any user change */
    }
    strcat(mstr,"proccmd ");
    ict++;
#ifdef VNMRJ
    if (ict > 0)
    {
      char nstr[6];
      size_t i;
      sprintf(nstr,"%d",ict);
      for (i=0; i<strlen(nstr); i++)
        mstr[i] = nstr[i];
      writelineToVnmrJ("pnew", mstr);
    }
#endif 
}


/*  For some reason the delivery of a SIGCHLD signal during a certain
    section of the GO program caused the pulse sequence program (PSG)
    to core dump or otherwise behave unacceptably.  So the SIGCHLD
    signal is blocked for a bit by calling block_sigchld.  Once this
    section completes, the original mask of signals is restored by
    calling restore_sigchld.  September 1994				*/

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


/*  If the pulse sequence program (PSG) fails to start (the classic
    reason was a failure to find shared libraries) or failed to read
    its input pipe, the parent process, VNMR, received a SIGPIPE when
    it tried to write to its side of the pipe.  If not caught, the
    process receiving the SIGPIPE is terminated with extreme prejudice.
    So we arrange to catch SIGPIPE before we begin writing to the write
    side of the VNMR - PSG pipe.  If SIGPIPE is delivered, the signal
    handler calls longjmp which redirects control as if the associated
    setjmp returned - except this time it returns -1.  The call to
    setjmp is programmed to expect this abnormal return and skip the
    section where data is written to PSG.				*/

static jmp_buf		brokenpipe;
static struct sigaction	origpipe;

static void
sigpipe()
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

int 
protectedRead(int fd)
{
   sigset_t    blockmask, savemask;
   int   ready;
   int   ret __attribute__((unused));

   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigprocmask( SIG_BLOCK, &blockmask, &savemask );
   ret = read(fd,&ready,sizeof(int));
   sigprocmask( SIG_SETMASK, &savemask, NULL );
   return(ready);
}

/*  End of new routines added September 1994	*/


void clearExpDir(const char *curexpdir)
{
   char path[MAXPATH];
   D_remove(D_PHASFILE);
   D_remove(D_DATAFILE);
   sprintf(path,"%s/recon",curexpdir);
   Rmdir(path,1);
   sprintf(path,"%s/datadir3d",curexpdir);
   Rmdir(path,1);
   sprintf(path,"%s/shapelib",curexpdir);
   Rmdir(path,1);
}

/*------------------------------------------------------------------------------
|
|       maxminlimit(tree,name)
|
|       This function returns the max & min limit of the real variable based
|                               Author: Greg Brissey  8-18-95
+----------------------------------------------------------------------------*/
int par_maxminstep(int tree, char *name,
                   double *maxv, double *minv, double *stepv)
{
   int             ret,pindex;
   vInfo           varinfo;     /* variable information structure */
 
   if ( (ret = P_getVarInfo(tree, name, &varinfo)) )
   {
      Werrprintf("Cannot find the variable: %s", name);
      return (ERROR);
   }
   if (varinfo.basicType != ST_REAL)
   {
      Werrprintf("The variable '%s' is not a type 'REAL'", name);
      return(ERROR);
   }
   if (varinfo.prot & P_MMS)
   {
      pindex = (int) (varinfo.minVal+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parmin", minv, pindex ))
         *minv = -1.0e+30;
      pindex = (int) (varinfo.maxVal+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parmax", maxv, pindex ))
         *maxv = 1.0e+30;
      pindex = (int) (varinfo.step+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parstep", stepv, pindex ))
            *stepv = 0.0;
   }
   else
   {   
       *maxv = varinfo.maxVal;
       *minv = varinfo.minVal;
       *stepv = varinfo.step;
   }
   return (0);
}

static int processPort = 0;
static int processPid = 0;

static int checkVpMode(char *label)
{
   double tmp;
   int numVP;
   char tmpStr[MAXPATH];

   if ( P_getreal(GLOBAL, "jviewports", &tmp, 1) >= 0 )
   {
      numVP = (int) (tmp+0.1);
      if ( P_getstring(GLOBAL, "testacquire", tmpStr, 1, MAXPATH) >= 0 )
      {
         if ( (tmpStr[0] == 'y') || (tmpStr[0] == 'Y') )
            numVP = 1;
      }
      if (numVP > 1)
      {
         int i;
         for (i=1; i <= numVP; i++)
         {
            if (i != VnmrJViewId)
            {
               P_getstring(GLOBAL, "jviewportlabel", tmpStr, i, MAXPATH);
               if ( strstr(tmpStr,"Current") || strstr(tmpStr,"current") )
               {
                  char path[MAXPATH];
                  int fd;

                  sprintf(path,"%s/persistence/.vp_%d_%d", userdir, jParent, i);
                  if ( (fd = open(path, O_RDONLY)) != -1)
                  {
		     int ret __attribute__((unused));
                     ret = read(fd, &processPort, sizeof(processPort));
                     ret = read(fd, &processPid, sizeof(processPid));
                     close(fd);
                     if (label)
                        strcpy(label,tmpStr);
                     return(1);
                  }
                  return(0);
               }
            }
         }
      }
   }
   return(0);
}

static char *vpAddr(char *addr)
{
   sprintf(addr,"%s %d %d", HostName, processPort, processPid);
   return(addr);
}

/* statics added for JPSG incorperation    12/11/98 */

    /* --- child and pipe variables --- */
static    char pipe1_0[16];
static    char pipe1_1[16];
static    char pipe2_0[16];
static    char pipe2_1[16];
static    int   psg_busted;  /* added September 1994 */

int acq(int argc, char *argv[], int retc, char *retv[])
{
    int TypeOfPS;	/* JPSG or PSG type sequence */
    char    dirname[MAXPATH];
    char    dirpath[MAXPATH];  /* path to fid directory */
    char    exppath[MAXPATH];  /* temporary path parameter */
    char    goid[MAXPATHL];   /* unique ID of this GO (username.######) */
    char    method[MAXPATHL];
    char    methodpath[MAXPATH];
    char    psgpath[MAXPATH];  /* path to seqfil */
    char    tmpStr[MAXPATH];
    char    rcvrs[MAXPATHL];
    char    a_name_option[MAXPATH]; /* Optional autoname provided as argument */
    char    *tagname;
    int     calcdimflag;
    int     restartflag;
    int     overridespinflag;
    int     acqi_fid;
    int     checkSpinCad;
    int     silent_mode;
    int     datastation;
    int     this_is_ra;		/* flag to distinguish `ra' from other aliases */
    int     waitflag = 0;
    int     i,setspin;
    int     sparse;
    double  ni;
    double  ni2;
    double  ni3;
    double  c_val;
    double  saveArraydim;
    double  saveAcqCycles;

    int   ret;
    int   psg_return_val __attribute__((unused));

#ifdef CLOCKTIME
    /* Turn on a clocktime timer */
    if ( start_timer ( go_timer_no ) != 0 )
    {  Werrprintf ( "go: \"start_timer ( go_timer_no )\" fails\n" );
       fprintf ( stderr, "go: \"start_timer ( go_timer_no )\" fails\n" );
       exit ( 1 );
    }
#endif 

    EPRINT(0,0,0);
    EPRINT(1,"current time is %s\n",0);
    EPRINT(-1,"elapsed time is %s secs\n",0);
    strcpy(arrayname,"array");	/* incase array parameter changes name */
    calcdimflag = argtest(argc,argv,"calcdim");

    if (!validcall(argc,argv,&datastation,!calcdimflag))
    {
       noGainTest = 0;
       goto abortAcq;
    }
    if (argtest(argc,argv,"vpcheck"))
    {
       char label[MAXPATH];
       vpmode = checkVpMode(label);
       if (retc)
          retv[0] = intString(vpmode);
       else if (vpmode)
         Winfoprintf("Data will be acquired in '%s' viewport",label);
       else
         Winfoprintf("Data will be acquired here");
       RETURN;
    }
    if ( ! calcdimflag )
       Wturnoff_buttons();	/* deactive any interactive programs */

    arraydim = 1.0;	/* initialize number of fids */
    acqcycles = 1.0;	/* number of acode sets */
    arrayelemts = 0.0;	/* initialize number of array elements */
    saveArraydim = saveAcqCycles = 1.0;
    tagname = "";

    /*--------------------------------------------------------------
    |	test for proper usage of arrayed variables and variable 'array'
    |	autogaining (gain='n') cannot be used in multifid experiments
    |			(i.e., gain='n', with other arrayed parameters)
    |	test for improper interleaving (il='y') (no-no with arrayed parameters,
    |		tpc,pad,spin,nt  experiments)
    |		acqqueuei#:=ior(16*priority, acqqueuei#);
    |		if ilv[1]='y' then acqqueuei#:=ior(1,acqqueuei#); 
    |   arraydim is calculated for the arrayed parameters in array.
    |   arrayelement number is calculated for the arrayed parameters in array.
    +---------------------------------------------------------------*/
    d2Array = d3Array = d4Array = d5Array = 0;
    if (arraytests())
    {
       noGainTest = 0;
       goto abortAcq;
    }
    sparse = (( CSinit("",curexpdir) == 0) && ! calcdimflag);

    EPRINT(-1,"elapsed time after arraytests is %s secs\n",0);
    /*----------------------------------------------------------------
    |	test for presence of ni
    |	if ni is greater than 1.5 then correct arraydim for the
    |	2D experiment 
    +---------------------------------------------------------------*/
    for (i = 1; i < MAXnD; i++)
    {
       char	niname[10];
       double	*tmpni;
       int      ndArray = 0;

       switch (i)
       {
          case 1:   strcpy(niname, "ni");
                    tmpni = &ni;
                    if (sparse && (getCSparIndex("d2") != -1))
                       ndArray = 1;
                    else
                       ndArray = d2Array;
                    break;
          case 2:   strcpy(niname, "ni2");
                    tmpni = &ni2;
                    if (sparse && (getCSparIndex("d3") != -1))
                       ndArray = 1;
                    else
                       ndArray = d3Array;
                    break;
          case 3:   strcpy(niname, "ni3");
                    tmpni = &ni3;
                    if (sparse && (getCSparIndex("d4") != -1))
                       ndArray = 1;
                    else
                       ndArray = d4Array;
                    break;
          default:  Werrprintf("internal error in `arraydim` calculation");
                    goto abortAcq;
       }

       if ( !ndArray &&  (P_getreal(CURRENT, niname, tmpni, 1) >= 0) )
       {
          GPRINT2(1, "%s = %5.0lf\n", niname, *tmpni);
          if (*tmpni > 1.5)
          {
             arraydim *= (*tmpni);
	     acqcycles *= (*tmpni);
             arrayelemts += 1.0;  /* Add 1 to array elements for each
				     acquisiton dimension */
          }
       }
    }
    if (sparse)
    {
       int num;
       num = getCSnum();
       arraydim *= num;
       acqcycles *= num;
       arrayelemts += 1.0;  /* Add 1 to array elements for each acquisiton dimension */
       releaseAllWithId("1_CS");
    }

    noGainTest = 0;
    if ( P_getstring(CURRENT, "rcvrs", rcvrs, 1, MAXPATHL) >= 0 )
    {
	i = numActiveRcvrs(rcvrs);
	/* NB: Here is where arraydim can get larger than acqcycles */
	arraydim *= i;
    }
	
    /* ============================================================== */
    /* stop here if we want only to calc the arraydim & arrayelements */
    /* -------------------------------------------------------------- */
    if (calcdimflag)
    {
        if (setparm("arraydim","real",CURRENT,&arraydim,1))
            goto abortAcq;
        P_creatvar(CURRENT,"arrayelemts",ST_REAL);
        P_setgroup(CURRENT,"arrayelemts",G_ACQUISITION);
        if (setparm("arrayelemts","real",CURRENT,&arrayelemts,1))
            goto abortAcq;
        P_creatvar(CURRENT,"acqcycles",ST_REAL);
        P_setgroup(CURRENT,"acqcycles",G_ACQUISITION);
        if (setparm("acqcycles","real",CURRENT,&acqcycles,1))
            goto abortAcq;
        if (retc)
        {
           retv[ 0 ] = intString( 1 );
        }
        RETURN;
    }

    /*----------------------------------------------------------------
    |	test for presents of PSG selected (i.e., S2PUL,COSY, etc.)
    +---------------------------------------------------------------*/
    /* new test4PS for JPSG */
    TypeOfPS = test4PS(psgpath);
  
    if (TypeOfPS == ERROR)  /* if found psgpath contain absolute path */
    {
        goto abortAcq;
    }

    if ( (TypeOfPS != USE_JPSG) && argtest(argc,argv,"SAR") )
    {
       Werrprintf("SAR option is invalid for this pulse sequence");
       goto abortAcq;
    }
  
    /* ============================================================== */
    silent_mode = argtest(argc,argv,"silent");
    acqi_fid = argtest(argc,argv,"acqi");
    checkSpinCad = 0;
    checkSpinCad = argtest(argc,argv,"check") || argtest(argc,argv,"checkarray");
    if ( ! strcmp(callname,"exptime") || 
         ! strcmp(callname,"dps")     ||
	 ! strcmp(callname,"pps")     ||
         ( argtest(argc,argv,"SAR") && (TypeOfPS==USE_JPSG) )    ||
	 checkSpinCad )
    {   char	tmpStr[50];
	acqi_fid = 1;
        if (TypeOfPS==USE_JPSG)
        {
	   if (P_getstring(CURRENT,"vchannelmap",tmpStr,1,50) < 0)
	      execString("sccheck\n");
        }
    }
    if (acqi_fid){
	automode = 0;
	if ( !(tagname=named_string_arg(argc,argv,"tag_")) ){
	    tagname = "acqi";
	}
    }

    /* -- if it's a datastation go no further --- */
    if (datastation && !acqi_fid) 
    {	Werrprintf("Cannot Run Acquisition Programs on A Data Station");
        goto abortAcq;
    }

    /* -- if it's not the console go no further --- */
    /* first clause is true if running from a terminal */
    /* second is true if ruuning in background,  */
    /* but not automation or acquisition         */

    setSilentMode(silent_mode);
    if (!silent_mode)
       disp_acq(callname); /* display alias of GO on Master Window */

    if ((acqi_fid || ( suflag != EXEC_GO )) && strcmp(argv[0],"exptime")
	           && strcmp(argv[0],"pps") && strcmp(argv[0],"dps") ) {
	/* force arraydim to one if not a go,ga,au */
        saveArraydim = arraydim;
        saveAcqCycles = acqcycles;
        P_creatvar(CURRENT,"saveArraydim",ST_REAL);
        P_setgroup(CURRENT,"saveArraydim",G_ACQUISITION);
        P_setreal(CURRENT,"saveArraydim", saveArraydim, 0);
        if ( ! argtest(argc,argv,"checkarray") )
	   arraydim = acqcycles = 1.0;
    }

    /*----------------------------------------------------------------
    |  if acqi active and connected and acqi did not issue the call
    |  to go,  then force it to disconnect
    +---------------------------------------------------------------*/
    if (interact_is_connected("") && !acqi_fid) 
    {
        interact_disconnect("");

	/* Read the parameters set by acqi */
        read_acqi_pars();
    }
    stop_acqi( 1 );
    if (nvAcquisition())
       stop_nvlocki();

    /*----------------------------------------------------------------
    |	test for proper parameter setting for np, rfband, and ct.
    +---------------------------------------------------------------*/
    if (check_acqpar())
    {
        goto abortAcq;
    }

    /*----------------------------------------------------------------
    |   if JPSG is not running then start it up here
    +---------------------------------------------------------------*/
    if (TypeOfPS == USE_JPSG)
        fireUpJPSG();

    GPRINT1(1,"Pulse Sequence path: '%s' \n",psgpath);
    EPRINT(-1,"elapsed time after test4PS is %s secs\n",0);
    /*----------------------------------------------------------------
    |   check that loc is a valid sample position 
    +---------------------------------------------------------------*/
    GPRINT2(1,"automode= %d  acqi_fid= %d\n",automode,acqi_fid);
    if (check_loc())
    {
        goto abortAcq;
    }
    vpmode = 0;
    if ( !strcmp(callname,"au") && !acqi_fid && argtest(argc,argv,"vp"))
    {
       vpmode = checkVpMode(NULL);
    }
    if (vpmode && ! automode)
    {
       sprintf(tmpStr,"%s/curparSave",curexpdir);
       P_save(CURRENT,tmpStr);
    }
    /*----------------------------------------------------------------
    |	make sure frequencies are set correctly
    |   save ct in case of ra because setfrq sets ct to zero.
    +---------------------------------------------------------------*/
    ret = P_getreal( CURRENT, "ct", &c_val, 1 );
    if (ret != 0) {
	Werrprintf("Internal error in go, cannot obtain value for 'ct'");
    }
    setfrq(1,argv,0,NULL);
    if (setparm("ct","real",CURRENT,&c_val,1))
       goto abortAcq;
    P_creatvar(CURRENT,"com$string",ST_STRING);
    P_setgroup(CURRENT,"com$string",G_ACQUISITION);
    if (setparm("com$string","string",CURRENT,"",1))  /* initialize to null */
    {
       goto abortAcq;
    }
    /*----------------------------------------------------------------
    |   if wshim does not equal 'no' then check for the method presence
    +---------------------------------------------------------------*/
    if (getparm("wshim","string",CURRENT,tmpStr,MAXPATH))
    {
       goto abortAcq;
    }
    GPRINT1(1,"wshim: '%s'\n",tmpStr);
    if (  ! (((tmpStr[0] == 'n') || (tmpStr[0] == 'N')) &&
	       (suflag == EXEC_GO)) &&
	  ! ((suflag != EXEC_GO) && (suflag != EXEC_SHIM) &&
	       (suflag != EXEC_SAMPLE)) )
    {
    	if (getparm("method","string",CURRENT,method,MAXPATHL))
    	{
           goto abortAcq;
    	}
	GPRINT1(1,"method: '%s'\n",method);
	if (test4meth(method,methodpath))
    	{
           goto abortAcq;
    	}
	GPRINT1(1,"Shimming method path: '%s' \n",methodpath);
        if (strlen(methodpath) )
          if (setparm("method","string",CURRENT,methodpath,1))
    	  {
             goto abortAcq;
    	  }
    }
    /*----------------------------------------------------------------
    |    If acqi did not call go, then test for presence of
    |    the acqproc lockfile
    +---------------------------------------------------------------*/
    if (!acqi_fid && test4Lock())  /* if found Abort GO */
    {
       goto abortAcq;
    }
    EPRINT(-1,"elapsed time after test4Lock is %s secs\n",0);
    /*----------------------------------------------------------------
    |   set arraydim and arrayelemts
    +---------------------------------------------------------------*/
    if (setparm("arraydim","real",CURRENT,&arraydim,1))
    {
       goto abortAcq;
    }
    P_creatvar(CURRENT,"arrayelemts",ST_REAL);
    P_setgroup(CURRENT,"arrayelemts",G_ACQUISITION);
    if (setparm("arrayelemts","real",CURRENT,&arrayelemts,1))
    {
       goto abortAcq;
    }
    P_creatvar(CURRENT,"acqcycles",ST_REAL);
    P_setgroup(CURRENT,"acqcycles",G_ACQUISITION);
    if (setparm("acqcycles","real",CURRENT,&acqcycles,1))
    {
       goto abortAcq;
    }
    GPRINT3(1,"arraydim=%5.0lf elemts=%5lf acqcycles=%5.0lf\n",
	    arraydim, arrayelemts, acqcycles);

    /* -- If command is RA, perform tests specific to RA.  Any
          problems (or warnings) are reported by the subroutine --- */

    this_is_ra = 0;
    if ( ! strcmp(callname, "ra" ) )
    {
      if (check_ra())
      {
         goto abortAcq;
      }
      this_is_ra = 1;
    }

    /* -- If command is not RA, then set ``celem'' to 0.
          Create this parameter if it does not exist,
          setting its group ID to acquisition.		--- */

    else

    {
	double ct;
	ret = P_getreal( CURRENT, "celem", &c_val, 1 );
	if (ret != 0)
	{
	    if (ret == -2)
	    {
		ret = P_creatvar( CURRENT, "celem", T_REAL );
		P_setgroup( CURRENT, "celem", G_ACQUISITION );
	    }
	}
	if (ret != 0)
	{
	   Werrprintf("BUG:  cannot access 'celem' in go, error = %d", ret);
           goto abortAcq;
	}
   
       /* -- celem = 0 at the beginning of an acquisition. --- */
   
        if ( strcmp(callname,"exptime") && strcmp(callname,"dps")  &&
              strcmp(callname,"pps") )
        {
	   P_setreal( CURRENT, "celem", 0.0, 1 );
           ct = 0.0;
           if (setparm("ct","real",CURRENT,&ct,1))
              goto abortAcq;
       }
    }

/*  'celem' must be set in the current parameter tree before calling
    ``initacqqueue'', as that routine takes the value from the tree
    and writes it to a file read by Acqproc.  'arraydim' should be set
    in the current parameter tree before calling ``check_ra'', as that
    routine uses the value in the tree to compare against the value of
    'celem'.  You could rework ``check_ra'' to use the static variable
    'arraydim' if desired.

    During an acquisition, 'celem' is set in the ACQHDL routines each
    time an element finishes.  See ACQHDL.C for more details.		*/

    /*-------------------------------------------------------------
    |   if "nocheck" is active, the go program will not check that
    |   the available disk space is sufficient to hold all of the
    |   acquired data.
    +------------------------------------------------------------*/
    if ((suflag == EXEC_GO) && !acqi_fid)
    {
       int     nospace_check;	/* no checking of available space for data */
       nospace_check = argtest( argc, argv, "nocheck" );
       if (!nospace_check) {
	int nelem;

        nelem = get_number_new_fids( this_is_ra );
        if (nelem < 0)
        {
          if (automode)
             execString("autosa\n");
          goto abortAcq;
        }
        ret = do_space_check(nelem);
        if (ret != 0)
        {
          if (automode)
             execString("autosa\n");
          goto abortAcq;
        }
       }
    }

    if (!acqi_fid && !ACQOK(HostName) )
    {
       if (!argtest( argc, argv, "forcePSG" ) )
       {
          Werrprintf("Acquisition system is not active!");
          goto abortAcq;
       }
    }

    if (!acqi_fid && (check_status_console( callname, HostName ) != 0))
    {
       if (!argtest( argc, argv, "forcePSG" ) )
       {
          goto abortAcq;
       }
    }

    if (P_getstring(CURRENT,"load",tmpStr,1,MAXPATH))
       strcpy(tmpStr,"n");
    if (!acqi_fid && (tmpStr[0] == 'y') &&
        !argtest( argc, argv, "nosafeshim" ) && check_ShimPowerPars() )
    {
        Werrprintf("Too much shim current is requested");
        goto abortAcq;
    }

    restartflag = argtest(argc,argv,"restart");
    if (restartflag && ! automode && !strcmp(callname,"au") )
    {
       /* turn off restartflag if not automode or not called with au */
       restartflag = 0;
    }
    if (restartflag && ! calledFromWerr() )
    {
       /* turn off restartflag if not called during werr processing */
       restartflag = 0;
    }
    /*----------------------------------------------------------------
    |	test for presence of acqfile 'file' name
    +---------------------------------------------------------------*/
    if (getparm("file","string",CURRENT,dirname,MAXPATH))
    {
        goto abortAcq;
    }
    if ((suflag == EXEC_GO) && !acqi_fid)
    {
       set_vnmrj_acq_params();
    }

    strcpy(a_name_option,"");
    waitflag = set_options(argc,argv,restartflag, a_name_option, &overridespinflag);

    if (automode)
    {
       char  a_name[MAXPATH];
       char  sif_name[MAXPATH];
       if ( ! restartflag )
       {
           /*  file is "sampleinfo file" */
          strcpy(sif_name,curexpdir);
          strcat(sif_name,"/sampleinfo");
	  /* get autoname parameter */
          a_name[0] = '\0';
          if ( strcmp(a_name_option,"") )
             strcpy(a_name,a_name_option);
          else if (P_getstring(GLOBAL,"autoname",a_name,1,MAXPATHL) < 0)
	     strcpy(a_name,"%SAMPLE#:%%PEAK#:%");
          if (a_name[0] == '\000')
	     strcpy(a_name,"%SAMPLE#:%%PEAK#:%");
          if (makeautoname("autoname",a_name,sif_name,dirname,TRUE,TRUE,".fid",".fid"))
          {
             goto abortAcq;
          }
       }
       else
       {
            if (dirname[0] == '/')
               sprintf(a_name,"%s.fid",dirname);
            else
               sprintf(a_name,"%s/%s.fid",autodir,dirname);
            sprintf(sif_name, "%s/fid",a_name);
            unlink(sif_name);
            sprintf(sif_name, "%s/procpar",a_name);
            unlink(sif_name);
            sprintf(sif_name, "%s/text",a_name);
            unlink(sif_name);
       }
    }
    else if (vpmode)
        sprintf(dirname,"vp");
    else
	strcpy(dirname,"exp");

    if ( (suflag == EXEC_GO) && !acqi_fid )
    {
      if (setparm("file","string",CURRENT,dirname,1))
      {
          goto abortAcq;
      }
      /* Ignore failure if "filename" parameter does not exist */
      P_setstring(CURRENT,"filename","",1);
    }
    GPRINT1(1,"dirname: '%s'\n",dirname);

    if ( !vpmode && test4ACQ(dirname,dirpath,          /* test & generate path to acqfil */
	   ( (strcmp(argv[CALLNAME],"ra") == 0) || acqi_fid))) 
    {
        goto abortAcq;
    }

    /*----------------------------------------------------------------
    |	initialize the acqqueue files that the Acq. Process will use
    |   sets the id,priority,data etc writes to file
    +----------------------------------------------------------------*/
    /* goid exp ID usage (e.g., exp1.username.######) */
    P_creatvar(CURRENT,"goid",ST_STRING);
    P_setgroup(CURRENT,"goid",G_ACQUISITION);
    if (acqi_fid)
    {
	sprintf(goid,"%s/acqqueue/%s", systemdir, tagname);
        if (setparm("goid","string",CURRENT,goid,1))
        {
           goto abortAcq;
        }
        P_setstring(CURRENT,"goid",UserName,2);
        P_setstring(CURRENT,"goid","1",3);
        P_setstring(CURRENT,"goid","exp1",4);
    }
    else if (initacqqueue(argc,argv))
    {
       goto abortAcq;
    }

    P_deleteVar(CURRENT,"consoleIdNumber");
    P_creatvar(CURRENT,"consoleIdNumber",ST_INTEGER);
    P_setgroup(CURRENT,"consoleIdNumber",G_ACQUISITION);
    P_setreal( CURRENT, "consoleIdNumber", (double) getAcqConsoleID(), 1 );

    P_creatvar(CURRENT,"acqstatus",ST_INTEGER);
    P_setreal( CURRENT, "acqstatus", 0.0, 0 );
    P_setreal( CURRENT, "acqstatus", 0.0, 2 );
    P_setprot(CURRENT,"acqstatus",P_NOA);  /* do not set array parameter */

    P_deleteVar(CURRENT,"rfchnuclei");
    P_creatvar(CURRENT,"rfchnuclei",ST_STRING);
    P_setgroup(CURRENT,"rfchnuclei",G_ACQUISITION);
    P_setprot(CURRENT,"rfchnuclei",P_NOA);

    /* current experiment fid path  */
    if (vpmode)
    {
       P_getstring(CURRENT,"go_id",tmpStr,1,MAXPATH);
       sprintf(dirpath,"%s/acq/",userdir);
       if ( ! access(dirpath, R_OK | W_OK | X_OK) )
       {
          strcat(dirpath, tmpStr);
       }
       else
       {
          sprintf(dirpath,"%s/acqqueue/acq/%s",systemdir, tmpStr);
       }
       do_mkdir(dirpath, 1, 0777);
    }
    GPRINT1(1,"Acqfile to use: '%s' \n",dirpath);

    P_creatvar(CURRENT,"exppath",ST_STRING);
    P_setgroup(CURRENT,"exppath",G_ACQUISITION);
    if (setparm("exppath","string",CURRENT,dirpath,1))
    {
       goto abortAcq;
    }
    P_creatvar(CURRENT,"appdirs",ST_STRING);
    P_setgroup(CURRENT,"appdirs",G_ACQUISITION);
    P_setstring(CURRENT,"appdirs",getAppdirValue(),1);

    if ((suflag == EXEC_GO) && !acqi_fid)
    {
        /* -- GO initializes the phase & data files in datdir --- */
        if ( ! vpmode )
        {
           clearExpDir(curexpdir);
        }

	/* Delete parameters used by flashc,tabc if they exist */
	P_deleteVar ( PROCESSED, "flash_converted" );
	P_deleteVar ( CURRENT,   "flash_converted" );
	P_deleteVar ( PROCESSED, "tab_converted" );
	P_deleteVar ( CURRENT,   "tab_converted" );
    }
    /*  remove psg error file */
    strcpy(tmpStr,userdir);
    strcat(tmpStr,"/psg.error");
    unlink(tmpStr);


    /*
     *------------------------------------------------------------
     * --- Fork & Exec PSG, then pipe parameter over to PSG
     * --- sends over parameters through the pipe to
     * --- the child and returns to vnmr without waiting. 
     *------------------------------------------------------------
     */

    /* only set spinner information if go, spin, or sample command */
    setspin = ( (suflag == EXEC_GO) || 
		(suflag == EXEC_SPIN) || 
		(suflag == EXEC_SAMPLE) ) ? 1 : 0;

    getVnmrInfo(1,setspin, overridespinflag );
    if ((suflag == EXEC_GO) && !acqi_fid)
       saveGlobalPars(3,"_"); /* delete old copies of global parameters */

    if (Bnmr)
    {
       /* avoid buffered output being displayed after
        * PSG messages in the acqlog
        */
       fflush(stdout);
       fflush(stderr);
    }
    /* To JPSG or NOT to JPSG */
    if (TypeOfPS == USE_JPSG)
    {
        if (!silent_mode)
           disp_acq("JPSG    ");
	jacq(acqi_fid,checkSpinCad,HostName,retc,retv);
    }
    else
    {
        /* testarrayparam(); */
        if (!silent_mode)
           disp_acq("PSG     ");
        sacq(acqi_fid,psgpath,retc,retv);
    }

    cleanup_pars();
    P_deleteVar(CURRENT,"consoleIdNumber");
    if (!acqi_fid)
    {
       P_setstring(CURRENT,"load","n",1);
    }
    else
    {
       P_setreal( CURRENT, "arraydim", saveArraydim, 1 );
       P_setreal( CURRENT, "acqcycles", saveAcqCycles, 1 );
    }


    /* -- if fid or spectrum displayed donot allow redrawing of it --- */
    if (!vpmode && (WgraphicsdisplayValid("ds") || WgraphicsdisplayValid("dfid") ||
	 WgraphicsdisplayValid("df") || WgraphicsdisplayValid("dconi")) )
    {
    	Wsetgraphicsdisplay("");
    }

    /* JPSG modification */
    if (TypeOfPS == USE_PSG)
    {
      /*
       *  this read operation will cause go to wait until PSG closes
       *  its pipe
       */
      if ((automode || acqi_fid || waitflag) && psg_busted == 0)
      {
        psg_return_val = protectedRead(pipe2[0]);
      }
      close(pipe2[0]);  /* parent closes its read end of first pipe */
    }
    /* copy and write files out only if GO */
    if ((suflag == EXEC_GO) && !acqi_fid)
    {
        int debugPutCmd;

        debugPutCmd = argtest(argc,argv,"debugputcmd");
        P_copyvar(SYSTEMGLOBAL,CURRENT,"Console","console");
        saveGlobalPars(1,"_");
        if (automode)
        {
            if (dirname[0] == '/')
               sprintf(dirpath,"%s.fid",dirname);
            else
               sprintf(dirpath,"%s/%s.fid",autodir,dirname);
        }
        if (vpmode && saveVpPars(dirpath, debugPutCmd))
        {
           goto abortAcq;
        }
        else if ( !vpmode && savepars(dirpath, debugPutCmd))
        {
           goto abortAcq;
        }
    }
    else if (checkSpinCad)
    {
       runPsgPutCmd( argtest(argc,argv,"debugputcmd") );
    }


    if (acqi_fid)
    {
	sprintf(goid,"%s/acqqueue/%s.Code", systemdir, tagname);
        strcpy(exppath,goid);
    	strcat(exppath,".lock");
        rename(exppath,goid);
    }
    GPRINT(1,"GO: Complete");
    RETURN;

abortAcq:
   disp_acq("");
   if (retc)
   {
      retv[ 0 ] = intString( 0 );
      RETURN;
   }
   ABORT;
}


static int sacq(int acqi_fid, char *psgpath, int retc, char *retv[])
{
    int   ret;

    ret = pipe(pipe1); /* make first pipe */
    /*
     *  The first pipe is used to send parameters to PSG
     */
    if(ret == -1)
    {   Werrprintf("GO: could not create system pipes!");
        if (!acqi_fid)
          release_console();
        return(1);
    }
    ret = pipe(pipe2); /* make second pipe */
    /*
     *  The second pipe is used to cause go to wait for PSG to
     *  complete.  This is only used in automation mode.
     */
    if(ret == -1)
    {   Werrprintf("GO: could not create system pipes!");
        if (!acqi_fid)
          release_console();
        close(pipe1[0]);
        close(pipe1[1]);
        return(1);
    }
    GPRINT(1,"GO: Starting PSG\n");
    EPRINT(-1,"elapsed time before fork is %s secs\n",0);

#ifdef __INTERIX
    child = vfork(); 
#else 
    child = fork();
#endif 
    if (child < 0)
    {   Werrprintf("GO: could not create a PSG process!");
        if (!acqi_fid)
          release_console();
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        return(1);
    }
    psg_pid = child;  /* set global psg pid value */
    EPRINT(-1,"elapsed time after fork is %s secs\n",0);

    if (child)		/* if parent set signal handler to reap process */
        set_wait_child(child);

    if (child == 0)
    {	char suflagstr[10];
        char Rev_Num[10];

	sprintf(pipe1_0,"%d",pipe1[0]);
     	sprintf(pipe1_1,"%d",pipe1[1]);
	sprintf(pipe2_0,"%d",pipe2[0]);
     	sprintf(pipe2_1,"%d",pipe2[1]);
	sprintf(suflagstr,"%d",suflag);
	sprintf(Rev_Num,"%d",GO_PSG_REV);
        set_effective_user();

	ret = execl(psgpath,"psg",Rev_Num,pipe1_0,pipe1_1,pipe2_0,pipe2_1,suflagstr,NULL);
	Werrprintf("PSG could not execute");
        exit(EXIT_SUCCESS);
    }
    EPRINT(-1,"elapsed time after execl is %s secs\n",0);


#ifdef CLOCKTIME
    /* Turn off the clocktime timer */
    if ( stop_timer ( go_timer_no ) != 0 )
    {  Werrprintf ( "go: \"stop_timer ( go_timer_no )\" fails\n" );
       fprintf ( stderr, "go: \"stop_timer ( go_timer_no )\" fails\n" );
       exit ( 1 );
    }
#endif 

    close(pipe1[0]);  /* parent closes its read end of first pipe */
    close(pipe2[1]);  /* parent closes its write end of second pipe */
 
/* The following 5 lines were added September 1994.  See
   comments at catch_sigpipe and block_sigchld, above.   */

    psg_busted = 0;
    catch_sigpipe();
    block_sigchld();
    if (setjmp( brokenpipe ) == 0)
    {
       P_sendGPVars(SYSTEMGLOBAL,G_ACQUISITION,pipe1[1]);
       P_sendGPVars(GLOBAL,G_ACQUISITION,pipe1[1]);/* send global tree to PSG */ 
       P_sendVPVars(GLOBAL,"curexp",pipe1[1]);
		/* send current experiment directory to PSG */ 
       P_sendVPVars(GLOBAL,"userdir",pipe1[1]);
		/* send user directory to PSG */ 
       P_sendVPVars(GLOBAL,"systemdir",pipe1[1]);
		/* send system directory to PSG */
       if (vpmode)
       {
          char  tmp[MAXSTR];
          if (P_getstring(GLOBAL,"vnmraddr",tmp,1,MAXSTR) == 0)
          {
             char tmp2[MAXSTR];
             P_setstring( GLOBAL,"vnmraddr", vpAddr(tmp2), 1);
             P_sendVPVars(GLOBAL,"vnmraddr",pipe1[1]);
             P_setstring( GLOBAL,"vnmraddr", tmp, 1);
             P_creatvar(CURRENT,"vpmode",ST_STRING);
             P_setgroup(CURRENT,"vpmode",G_ACQUISITION);
             P_setstring( CURRENT,"vpmode", "y", 0 );
             if (P_getstring(CURRENT,"actionid",tmp2,1,MAXSTR) == 0)
             {
                char tmp3[MAXSTR*3];
                P_creatvar(CURRENT,"VPaddr",ST_STRING);
                P_setgroup(CURRENT,"VPaddr",G_ACQUISITION);
                sprintf(tmp3,"%s; %s",tmp,tmp2);
                P_setstring( CURRENT,"VPaddr", tmp3, 0 );
             }
          }
       }
       else if (psgaddr[0] != '\0')
       {
          char  tmp[MAXSTR];
          if (P_getstring(GLOBAL,"vnmraddr",tmp,1,MAXSTR) == 0)
          {
             P_setstring( GLOBAL,"vnmraddr", psgaddr, 1);
             P_sendVPVars(GLOBAL,"vnmraddr",pipe1[1]);
             P_setstring( GLOBAL,"vnmraddr", tmp, 1);
          }
       }
       EPRINT(-1,"elapsed time after first pipe is %s secs\n",0);
       P_sendGPVars(CURRENT,G_ACQUISITION,pipe1[1]);/* send current tree to PSG */ 
       EPRINT(-1,"elapsed time after second pipe is %s secs\n",0);
       P_endPipe(pipe1[1]);   /* send end character */

/* The following 7 lines were added September 1994,
   along with other references to `psg_busted', below.  */

    }
    else {
       Werrprintf( "%s: pulse sequence failed to start. Check shared libraries.", callname);
       if (!acqi_fid)
         release_console();
       psg_busted = 1;
    }
    if (vpmode)
    {
       P_deleteVar(CURRENT,"vpmode");
       P_deleteVar(CURRENT,"VPaddr");
    }
    restore_sigchld();
    restore_sigpipe();
    close(pipe1[1]);  /* parent closes its write end of first pipe */
    EPRINT(-1,"elapsed time after close pipe is %s secs\n",0);

    /* go waits here until psg finishes the first element of the experiemnt */
    if (psg_busted == 0)
    {
       int psg_return_val = protectedRead(pipe2[0]);
       if (retc)
       {
          if (psg_return_val == 0)
            retv[0] = newString("1");  /* return of 1 to macro means PSG executed successfully */
          else
            retv[0] = newString("0");  /* return of 0 to macro means PSG aborted in ps or PSG code */
       }
    }
    else if (retc)
      retv[0] = newString("0");  /* return of 0 to macro means PSG aborted in ps or PSG code */

    return(0);

}


static int jacq(int acqi_fid, int spinCadCheck,
                char *hostname, int retc, char *retv[])
{
    FILE *sd;
    char    ackbuf[MAXPATHL];	/* JPSG .... */
    char *nparmsCmd = "NewParms\n";
    char *exptimeCmd = "Exptime\n";
    char *dpsCmd = "Dps\n";
    char *checkCmd = "Check\n";
    char *acqiCmd = "Acqi\n";
    char *goCmd = "Go\n";
    char *suCmd = "Su\n";
    char *createParamsCmd = "CreateParams\n" ;
    char *subStr;
    int numconv;
    char portstr[50];
    char pidstr[50];
    char jhostname[50];
    char readymsge[50];
    char jpsgPIDFileName[MAXSTR];
    int port;
    int bytes;
    int done,i;

    Socket  *tSocket;
    Socket* connect2Jpsg(int,char*);

    int   psg_busted __attribute__((unused));	/* added September 1994 */

    double  maxsw;
    double   max,min,step;

    disp_acq(callname); /* display alias of GO on Master Window */

   /******************************** JPSG SOCKETS ***************************/
    /* printf("jacq: just starting \n"); */
    if ( ! isJpsgReady() )
    {
       disp_acq("");
       Werrprintf( "%s: no Jpsg is running, check CLASSPATH and rerun command", callname);
       ABORT;
    }
    EPRINT(-1,"elapsed time before fork is %s secs\n",0);
    /* printf("Reading Jpsg Port # ---  "); */
    strcpy(jpsgPIDFileName,"/vnmr/acqqueue/jinfo1.");
    strcat(jpsgPIDFileName,UserName);

    sd = fopen(jpsgPIDFileName,"r");
    numconv = fscanf(sd,"%s %s %s %s",portstr,pidstr,jhostname,readymsge);
    fclose(sd);
    /* printf(" Done --- \n "); */
    /* printf("jacq()  conv: %d, port: '%s', pid: '%s', host: '%s', rdymsge: '%s'\n",
                   numconv, portstr,pidstr,jhostname,readymsge);
    */
    if ((numconv != 4) || (strcmp(readymsge,"ready") !=0))
    {
       disp_acq("");
       Werrprintf( "%s: /vnmr/acqqueue/jinfo1.%s syntax error, rerun command", callname,UserName);
       Wscrprintf( "Check your /vnmr/jpsg directory\n" );
       ABORT;
    }
    port = atoi(portstr);
    /* printf("port Id: '%s', #: %d\n",portstr, port); */

    /*
    P_creatvar(CURRENT,"when_mask",ST_INTEGER);
    P_setgroup(CURRENT,"when_mask",G_ACQUISITION);
    P_setreal( CURRENT,"when_mask", (double) when_mask, 1 );
    P_creatvar(CURRENT,"jpsgPSfile",ST_STRING);
    P_setgroup(CURRENT,"jpsgPSfile",G_ACQUISITION);
    P_setstring(CURRENT,"jpsgPSfile",psgpath,0);
    P_deleteVar(CURRENT,"jpsgPSfile");
    */

    
    P_creatvar(CURRENT,"goalias",ST_STRING);
    P_setgroup(CURRENT,"goalias",G_ACQUISITION);
    P_setstring( CURRENT,"goalias", callname, 0 );

    /* generate the active parameter flag parameters in the TEMPORARY tree */
    createActiveFlagParameter();

    /* generate the additional parameters for JPSG  in the TEMPORARY tree */
    if (P_getreal( SYSTEMGLOBAL, "parmax", &maxsw, 5 ) )
	ABORT;
    P_creatvar(TEMPORARY,"maxsw",ST_REAL);
    P_setgroup(TEMPORARY,"maxsw",G_ACQUISITION);
    P_setreal( TEMPORARY,"maxsw", maxsw, 1 );

    /* Create gain max. min and step values for use in autogain */
    par_maxminstep(CURRENT, "gain", &max, &min, &step);
    /* fprintf(stderr,"gain: max %lf, min: %lf, step: %lf \n",max,min,step); */
    P_creatvar(TEMPORARY,"gainmax",ST_REAL);
    P_setgroup(TEMPORARY,"gainmax",G_ACQUISITION);
    P_setreal( TEMPORARY,"gainmax", max, 1 );
    P_creatvar(TEMPORARY,"gainmin",ST_REAL);
    P_setgroup(TEMPORARY,"gainmin",G_ACQUISITION);
    P_setreal( TEMPORARY,"gainmin", min, 1 );
    P_creatvar(TEMPORARY,"gainstep",ST_REAL);
    P_setgroup(TEMPORARY,"gainstep",G_ACQUISITION);
    P_setreal( TEMPORARY,"gainstep", step, 1 );

    GPRINT(1,"GO: Starting PSG\n");

    /* printf("connect2Jpsg(%d, '%s')\n",port, hostname ); */
    tSocket = connect2Jpsg( port, hostname );
    if (tSocket == NULL)
    {
       disp_acq("");
       Werrprintf( "%s: connection to JPSG failed", callname);
       Wscrprintf( "Check your /vnmr/jpsg directory\n" );
       ABORT;
    }
       
    /* fprintf(stderr,"Connected to Socket: %d\n",tSocket->sd); */

    EPRINT(-1,"elapsed time after execl is %s secs\n",0);

#ifdef CLOCKTIME
    /* Turn off the clocktime timer */
    if ( stop_timer ( go_timer_no ) != 0 )
    {  Werrprintf ( "go: \"stop_timer ( go_timer_no )\" fails\n" );
       fprintf ( stderr, "go: \"stop_timer ( go_timer_no )\" fails\n" );
       exit ( 1 );
    }
#endif 
 

    psg_busted = 0;
    block_sigchld();
 

    /* NewParmCmd */
    writeSocket(tSocket,nparmsCmd,strlen(nparmsCmd));

    J_sendGPVars(SYSTEMGLOBAL,G_ACQUISITION,tSocket->sd);
    J_sendGPVars(GLOBAL,G_ACQUISITION,tSocket->sd);/* send global tree to PSG */
    J_sendVPVars(GLOBAL,"curexp",tSocket->sd);
                /* send current experiment directory to PSG */
    J_sendVPVars(GLOBAL,"userdir",tSocket->sd);
                /* send user directory to PSG */
    J_sendVPVars(GLOBAL,"systemdir",tSocket->sd);
                /* send system directory to PSG */
    if (psgaddr[0] != '\0')
    {
       char  tmp[MAXSTR];
       if (P_getstring(GLOBAL,"vnmraddr",tmp,1,MAXSTR) == 0)
       {
          P_setstring( GLOBAL,"vnmraddr", psgaddr, 1);
          J_sendVPVars(GLOBAL,"vnmraddr",tSocket->sd);
          P_setstring( GLOBAL,"vnmraddr", tmp, 1);
       }
    }
    J_sendGPVars(CURRENT,G_ACQUISITION,tSocket->sd);/* send current tree to PSG */
    J_sendGPVars(TEMPORARY,G_ACQUISITION,tSocket->sd);/* send temporary tree to PSG */
    P_endPipe(tSocket->sd);
     
    /* bytes = readSocket(tSocket,ackbuf,256);
               readSocket(..) interrupted by SIGALM and causing return */ 
    bytes = readProtectedSocket(tSocket,ackbuf,256);
    closeSocket(tSocket);

    restore_sigchld();

    P_treereset(TEMPORARY);   /* Clear the Temporary tree */

    /* printf("bytes recv: %d\n",bytes); */
    ackbuf[bytes]=0;
    /* printf("reply: '%s'\n",ackbuf); */
    if (strcmp(ackbuf,"ack") != 0)
    {
       disp_acq("");
       Werrprintf( "%s: pulse sequence failed at 1", callname);
       Werrprintf( "Check your pulse sequence" );
       if (!acqi_fid)
         release_console();
       psg_busted = 1;
       ABORT;
    }
     

    /* printf("connect2Jpsg(%d, '%s')\n",port, hostname ); */
    tSocket = connect2Jpsg( port, hostname );
    if (tSocket == NULL)
    {
       disp_acq("");
       Werrprintf( "%s: connection to JPSG failed", callname);
       Wscrprintf( "Check your /vnmr/jpsg directory\n" );
       ABORT;
    }
    if ( ( suflag == EXEC_SU ) || ( suflag == EXEC_LOCK ) || 
	 ( suflag == EXEC_SHIM) || (suflag == EXEC_SAMPLE) ||
	 ( suflag == EXEC_SPIN) || (suflag == EXEC_CHANGE) )
    {
       writeSocket(tSocket,suCmd,strlen(suCmd));
    }
    else if (suflag == EXEC_EXPTIME)
    {
       writeSocket(tSocket,exptimeCmd,strlen(exptimeCmd));
    }
    else if (suflag == EXEC_DPS)
    {
       writeSocket(tSocket,dpsCmd,strlen(dpsCmd));
    }
    else if (suflag == EXEC_CREATEPARAMS)
    {
       writeSocket(tSocket,createParamsCmd,strlen(createParamsCmd));
    }
    else
    {
       if (acqi_fid)
       {
	  if ( spinCadCheck )
             writeSocket(tSocket,checkCmd,strlen(checkCmd));
          else
             writeSocket(tSocket,acqiCmd,strlen(acqiCmd));
       }
       else
          writeSocket(tSocket,goCmd,strlen(goCmd));
    }
    done = 0; /* init, seed */
    strcpy(ackbuf,"");
    while ( ! done )
    {
        /* bytes = readSocket(tSocket,ackbuf,256);
                   readSocket() being interrupted and causing return */
        bytes = readProtectedSocket(tSocket,ackbuf,256);

	ackbuf[bytes]=0;
	if ( bytes <= 0 ) { subStr = ackbuf; break; }

	subStr = strtok(ackbuf," \n");
	if ( ! strcmp(ackbuf,"ack") )
        {
           subStr = ackbuf;
           done = 1;
        }
        else if ( ! strcmp(subStr,"results") )
        {
            subStr = strtok(NULL, " \n"); /* this should be "SAR" */
	    subStr = strtok(NULL," \n");
	    i=0;
            while ( subStr!=NULL )
	    {
	       if ( ! strcmp(subStr,"ack") ) 
               {
                  done = 1;
		  break;
               }
               if (i < retc)
                 retv[i] = newString(subStr);
	       i++;
	       subStr = strtok(NULL," \n");
            }
        }
    }

    closeSocket(tSocket);
    /* printf("bytes recv: %d\n",bytes); */
    ackbuf[bytes]=0;
    /* printf("reply: '%s'\n",ackbuf); */
    if (strcmp(subStr,"ack") != 0)
    {
       disp_acq("");
       Werrprintf( "%s: pulse sequence failed at 2", callname);
       Werrprintf( "Check your pulse sequence" );
       if (!acqi_fid)
         release_console();
       psg_busted = 1;
       ABORT;
    }
     
    EPRINT(-1,"elapsed time after close socket is %s secs\n",0);

    disp_acq("");
    RETURN;
}


/*

The following are parameters that currently use 
active/inactive states in psg.

-  bs -  dhp -  d0 -  gain -  nf -  oversamp -  osfb -  oslsfrq -  pad -  spin -  ss -  temp - preacq  CURRENT

- loc - z0   GLOABAL tree
*/

/* list just the CURRENT tree Parameters Here */
static char *activeflags[] = { "bs", "dhp", "gain", "nf",
                                "oversamp", "osfb", "oslsfrq", "pad", "spin",
                                "slp0","slp","slp2","slp3","slp4","slp5","slp6",
                                "ss", "ss2", "temp", "preacq", "mrfb", "mrgain" };

/* d0 set seperately since need it even if d0 not present */
/* z0 & loc are from the global tree and done seperately */


void createActiveFlagParameter()
{
    vInfo info;
    int i;
    int entries;
    char *strval;
    char flagname[512];

    P_treereset(TEMPORARY);
    /*
    printf("sizeof activeflags: %d, %d (entries)\n",
        sizeof(activeflags), sizeof(activeflags)/sizeof(char*));
    */
    entries = sizeof(activeflags)/sizeof(char*);

    for (i=0; i < entries; i++)
    {
       /* create name such as bsactive, gainactive, etc... */

       if (P_getVarInfo(CURRENT,activeflags[i],&info) > -1)
       { 
         sprintf(flagname,"%sactive",activeflags[i]);
         P_creatvar(TEMPORARY,flagname,ST_STRING);
         P_setgroup(TEMPORARY,flagname,G_ACQUISITION);
         strval = (var_active(activeflags[i], CURRENT) == 1) ? "y" : "n";
         /*
         printf("Parm: '%s', activeParm: '%s' = '%s'\n",activeflags[i],
                flagname, strval);
	 */
         P_setstring( TEMPORARY,flagname, strval, 1);
       } 
    }   
    if (P_getVarInfo(CURRENT,"d0",&info) > -1)
       strval = (var_active("d0", CURRENT) == 1) ? "y" : "n";
    else
       strval = "n";
    P_creatvar(TEMPORARY,"d0active",ST_STRING);
    P_setgroup(TEMPORARY,"d0active",G_ACQUISITION);
    P_setstring( TEMPORARY,"d0active", strval, 1 );

    P_creatvar(TEMPORARY,"locactive",ST_STRING);
    P_setgroup(TEMPORARY,"locactive",G_ACQUISITION);
    strval = (var_active("loc", GLOBAL) == 1) ? "y" : "n";
    P_setstring( TEMPORARY,"locactive", strval, 1 );
    P_creatvar(TEMPORARY,"z0active",ST_STRING);
    P_setgroup(TEMPORARY,"z0active",G_ACQUISITION);
    strval = (var_active("z0", GLOBAL) == 1) ? "y" : "n";
    P_setstring( TEMPORARY,"z0active", strval, 1 );
}


/*----------------------------------------------------------------------------
|	argtest(argc,argv,argname)
|	test whether argname is one of the arguments passed
+---------------------------------------------------------------------------*/
static int argtest(int argc, char *argv[], char *argname)
{
  int found = 0;

  while ((--argc) && !found)
    found = (strcmp(*++argv,argname) == 0);
  return(found);
}

/*----------------------------------------------------------------------------
|	named_string_arg(argc, argv, name)
|	Look for an argument of the form "name...".  Typically,
|	name will be something like "name_", so we look for something
|	like "name_whatever".
|	Returns a pointer to the string "whatever", or NULL if the
|	name is not found.
|	Note: The address returned points to somewhere in the argvs
|	that the caller passed.
+---------------------------------------------------------------------------*/
static char *
named_string_arg(int argc, char **argv, char *name)
{
    char *rtn = NULL;

    while (--argc){
	if (strncasecmp(*++argv, name, strlen(name)) == 0){
	    rtn = *argv + strlen(name);
	    break;
	}
    }
    return rtn;
}

/*
 *  Put the arguments passed to go into a parameter that will
 *  be passed to PSG.
 */
/*  when_mask is bit field for different when conditions;
 */
static int set_options(int argc, char *argv[], int restart, char *a_name_option, int *overridespinflag)
{
    int index;
    int num;
    int when_mask = 0;
    int waitflag = 0;
    char    tmpstr[MAXSTR];

    *overridespinflag = 0;
    if ( ! restart)
    {
       index = 0;
       num = 1;
       P_deleteVar(CURRENT,"go_Options");
       P_creatvar(CURRENT,"go_Options",ST_STRING);
       P_setgroup(CURRENT,"go_Options",G_ACQUISITION);
       if  (strcmp(argv[CALLNAME],"ra") == 0)
       {
          P_setstring(CURRENT,"go_Options",argv[CALLNAME],index);
          index = 2;
       }
       if  (strcmp(argv[CALLNAME],"createparams") == 0)
       {
          P_setstring(CURRENT,"go_Options",argv[CALLNAME],index);
          index = 2;
       }

       while (num < argc)
       {
          if ( strcmp(argv[num],"nocheck") )
          {
             P_setstring(CURRENT,"go_Options",argv[num],index);
             if (index == 0)
                index = 1;
             index++;
             if (!strcmp(argv[num],"next") || !strcmp(argv[num],"sync") )
                waitflag = 1;
             if (!strcmp(argv[num],"overridespin") )
                *overridespinflag = 1;
             if ( ! strncmp(argv[num],"autoname_",9) )
             {
                if (strlen(argv[num]) > 9)
                   strcpy(a_name_option, argv[num] + 9);
             }
          }
          num++;
       }
       if ( automode &&  ! strcmp(callname,"au") )
       {
          char path[MAXPATH];
          char option[MAXPATH];

          strcpy(path,autodir);
          strcat(path,"/auargs");
          if ( ! access(path,F_OK) )
          {
             FILE *fd;
             fd = fopen(path,"r");
             while (fscanf(fd,"%s\n",option) == 1)
             {
                if ( strcmp(option,"nocheck") )
                {
                   P_setstring(CURRENT,"go_Options",option,index);
                   if (index == 0)
                      index = 1;
                   index++;
                   if (!strcmp(option,"next") ||
                       !strcmp(option,"sync") ||
                       !strcmp(option,"wait")  )
                      waitflag = 1;
                   if (!strcmp(option,"overridespin") )
                      *overridespinflag = 1;
                   if ( ! strncmp(option,"autoname_",9) )
                   {
                      if (strlen(option) > 9)
                         strcpy(a_name_option, (option + 9) );
                   }
                }
             }
             fclose(fd);
          }
       }
       P_setprot(CURRENT,"go_Options",P_ARR | P_ACT | P_VAL);  /* do not allow any user change */
    }
    else
    {
       int nextflag = 0;
       num = P_getsize(CURRENT,"go_Options",NULL);
       index = 1;
       while (index <= num)
       {
          P_getstring(CURRENT,"go_Options",tmpstr,index,MAXSTR-1);
          index++;
          if (!strcmp(tmpstr,"next"))
                nextflag = waitflag = 1;
          else if  (!strcmp(tmpstr,"sync") )
                waitflag = 1;
          else if  (!strcmp(tmpstr,"overridespin") )
                *overridespinflag = 1;
       }
       if ( !nextflag)
       {
          P_setstring(CURRENT,"go_Options","next",num+1);
       }
    }
    P_creatvar(CURRENT,"when_mask",ST_INTEGER);
    P_setgroup(CURRENT,"when_mask",G_ACQUISITION);
    if  (strcmp(callname,"ga") == 0)
    {
       when_mask |= WHEN_GA_PROC;
    }
    else if  (strcmp(callname,"su") == 0)
    {
       if (!P_getstring(CURRENT,"wsu",tmpstr,1,MAXSTR))
       {
          if (strlen(tmpstr))
             when_mask |= WHEN_SU_PROC;
       }
    }
    else if  (strcmp(callname,"au") == 0)
    {
       /* Turn on all the flags. execfromacqproc() will turn off WHEN_BS_PROC
        * and WHEN_NT_PROC if the wbs or wnt parameters are null strings.
        * See comment in werr() in acqhdl.c
        */
       when_mask = WHEN_BS_PROC | WHEN_NT_PROC | WHEN_ERR_PROC | WHEN_EXP_PROC;
    }
    P_setreal( CURRENT,"when_mask", (double) when_mask, 1 );
    return(waitflag);
}

/*  Future changes to this program should be
    coordinated with getInteractVnmrInfo, acqfuncs.c  */

void getVnmrInfo(int okToSet, int okToSetSpin, int overRideSpin)
{
    int val;
    char lkStr[MAXSTR];
    char tmpStr[MAXSTR];
    int  panelCntrl;

    P_creatvar(CURRENT,"spinThresh",ST_REAL);
    P_setgroup(CURRENT,"spinThresh",G_ACQUISITION);
    P_setreal(CURRENT,"spinThresh",(double) getInfoSpinner() ,0);
    P_creatvar(CURRENT,"interLocks",ST_STRING);
    P_setgroup(CURRENT,"interLocks",G_ACQUISITION);
    if (P_getstring(CURRENT,"in",lkStr,1,4))
       strcpy(lkStr,"n");
    /* interLocks will be three characters
     * char 0 for lock interlock
     * char 1 for spin interlock
     * char 2 for temp interlock
     */
    if (lkStr[0] == '\0')
    {
       lkStr[0] = 'n';
       lkStr[1] = '\0';
    }
    panelCntrl = !getInfoSpinExpControl();
    if (panelCntrl && overRideSpin)
    {
       panelCntrl = 0;
       okToSetSpin = 0;
    }
    if ( panelCntrl )
    {
       if (okToSet)
       {
          P_setreal(CURRENT,"spin",(double) getInfoSpinSpeed() ,0);
          P_setactive(CURRENT,"spin",1);
          appendvarlist("spin");
       }
       val = getInfoSpinErrorControl();
       if (val == 0)
          lkStr[1] = 'n';
       else if (val == 1)
          lkStr[1] = 'y';
       else
          lkStr[1] = 'w';
    }
    else
    {
       if (lkStr[1] == '\0')
          lkStr[1] = lkStr[0];
       /* only update if GO/AU/GA, SPIN, or SAMPLE (okToSetSpin flag) */
       if (okToSet && var_active("spin",CURRENT) && okToSetSpin)
       {
          double tmpval;
          int ival;
          P_getreal(CURRENT,"spin", &tmpval ,1);
          ival = (int) tmpval;

          setInfoSpinOnOff((ival == 0) ? 0 : 1);
          setInfoSpinSetSpeed(ival);
       }
    }
    if (P_getstring(CURRENT,"tin",tmpStr,1,4))
       tmpStr[0] = 'n';
    if (!getInfoTempExpControl() )
    {
       if (okToSet)
       {
          P_setreal(CURRENT,"temp",(double) getInfoTempSetPoint()/10.0 ,0);
          P_setactive(CURRENT,"temp", getInfoTempOnOff());
          appendvarlist("temp");
       }
       val = getInfoTempErrorControl();
       if (val == 0)
          tmpStr[0] = 'n';
       else if (val == 1)
          tmpStr[0] = 'y';
       else
          tmpStr[0] = 'w';
    }
    else
    {
       if (okToSet)
       {
          double tmpval;
          P_getreal(CURRENT,"temp", &tmpval ,1);
          if (var_active("temp",CURRENT))
          {
             setInfoTempOnOff(1);
             setInfoTempSetPoint((int) (tmpval * 10.0));
          }
          else
          {
             setInfoTempOnOff(0);
          }
       }
    }
    lkStr[2] = tmpStr[0];
    lkStr[3] = '\0';

    /* look for pnin, pneumatic Interlock */
    if (P_getstring(CURRENT,"pin",tmpStr,1,4))
       tmpStr[0] = 'n';
    lkStr[3] = tmpStr[0];
    lkStr[4] = '\0';

    P_setstring(CURRENT,"interLocks",lkStr,0);
}
/*
 *  Delete temporary parameters which were used to pass
 *  information to PSG. Also used by DPS
 */
void cleanup_pars()
{
   P_deleteVar(CURRENT,"when_mask");
   P_deleteVar(CURRENT,"com$string");
   P_deleteVar(CURRENT,"goid");
   P_deleteVar(CURRENT,"spinThresh");
   P_deleteVar(CURRENT,"interLocks");
   P_deleteVar(CURRENT,"appdirs");
   P_deleteVar(CURRENT,"saveArraydim");
}

/*  The following program is compiled only on UNIX.  It uses the `statfs'
    data structure to assess how much space is available in the current file
    system, so it can determine if sufficent space exists for the data from
    the proposed acquisition.  Thus is is not needed for systems with no
    requirement to acquire data, such as the VMS.  I used the symbol UNIX
    because, as far as I know, each UNIX system has a `statfs' data structure
    and we do not want to rule out acquisitions on non-SUN-based systems.
								  RL  08/27/91  */


/*  Postponed until later is what to do about
    the file size if nelem <> arraydim		*/

static int do_space_check(int nelem)
{
       struct  statvfs freeblocks_buf;
       double  free_kbytes;	/* number of free kbytes for acquired data */
       double  req_kbytes;		/* kbytes required for acquired data */
       double  nacqpoints;		/* number of acquired data points: NP */
       double  nfval;           /* number of fids */
       char    dpval[MAXSTR];
       char    tmpdir[MAXSTR];
       double  maxFidSizeMB = 16; /* default to standand DTM 16 MB Size */
       double  fidSizeBytes = 0.0;
       double  maxlbsw, sw;


/* For a VnmrS or 400-MR the check is done in PSG 
 * (to cover multiple receivers, etc
 */
    if ( P_getstring(SYSTEMGLOBAL, "Console", tmpdir, 1, MAXPATHL) < 0)
    {   Werrprintf("Cannot find the 'Console' parameter");
        return(-1);
    }
    if (strcmp(tmpdir,"vnmrs") == 0) 
       return(0);
    
/*  Verify program really needs to check disk space.  */

       if (nelem < 1) {
           return( 0 );
       }


       sprintf(tmpdir,"%s/.",(automode) ? autodir : curexpdir);
       statvfs( tmpdir, &freeblocks_buf);
       free_kbytes = (double) freeblocks_buf.f_bavail;

       if (P_getreal(CURRENT,"np",&nacqpoints,1))
       {
          Werrprintf("Cannot locate NP in parameter set");
	  disp_acq("");
          return( -1 );
       }

       if (P_getreal(CURRENT,"nf",&nfval,1))
       {
         nfval = 1.0;
       }
       else
       {
         if (nfval <= 0.0) nfval = 1.0;
       }

       if (P_getstring(CURRENT,"dp",dpval,1,MAXSTR))
       {
          Werrprintf("Cannot locate DP flag in parameter set");
          dpval[0] = 'n';
       }
          
       if (dpval[0] == 'y')
       {
          fidSizeBytes =  4.0 * nacqpoints * nfval;
       }
       else
       {
          fidSizeBytes =  2.0 * nacqpoints * nfval;
       }

       /* The next two parameters are needed to determine if in the case where
          there are both 500KHz and 5MHz DTM/ADCs and which one is being used.
       */
       if (getparm("sw","real",CURRENT,&sw,1))
        ABORT;

#ifdef VNMRJ
       if (nvAcquisition())
          maxlbsw = 2e7;
       else
#endif
       if (P_getreal(SYSTEMGLOBAL, "maxsw_loband", &maxlbsw, 1) < 0)
          maxlbsw = 100000.1;
       else
          maxlbsw += 0.1;

        /* new conpar parameter 1/14/03 so we can check that the FID doesn't get to big */
       if (P_getreal(SYSTEMGLOBAL,"stmmemsize",&maxFidSizeMB,1))
       {
         maxFidSizeMB = 16.0;    /* default to liquids 16 MB DTM */
       }
       else
       {
         if (maxFidSizeMB <= 0.0) maxFidSizeMB = 16.0; /* default to liquids 16 MB DTM */
       }

       /* OK, this is a slight kudge, system wiwth 500KHz and 5MHz DTMs/ADCs
          when the sw > maxlbsw then the system switches to the 5MHz DT</ADC board
          usually in slot 4
       */
       if ( sw > maxlbsw ) 
         maxFidSizeMB = 2.0;  /* Using 5MHz DTM/ADC only has 2 MB */


       /* if arrayed experiment then need to allow double buffering in DTM
	  Therefore halve the available memory.
       */
       maxFidSizeMB = (nelem > 1) ? (maxFidSizeMB / 2.0) : maxFidSizeMB;

          
       /* OK, have we exceed the DTM memory size ? */
       /* The fudge factor takes into account some overhead headers, etc. 256 bytes */
       if ( fidSizeBytes/MBYTE > maxFidSizeMB-MAXFIDSIZEFUDGE_MB)
       {
          char msge[1024];
          /* "FID size (%5.3lf MB) too large for Console DTM Memory (%5.3lf MB), 
	      change to single precision, or remove compression from an imaging 
               dimension via seqcon", 
          */

          sprintf(msge,"FID size exceeds %.0f MB Max. by %.0f bytes for Console DTM Memory, reduce FID size",
                  maxFidSizeMB, fidSizeBytes - (maxFidSizeMB-MAXFIDSIZEFUDGE_MB)*MBYTE);
          Werrprintf(msge);
	  disp_acq("");
          return( -1 );
       }

       /* OK, will the data size exceed available disk space ? */

       req_kbytes = fidSizeBytes * (double) nelem / 1024.0;
       if (req_kbytes * 0.1 > MAXSIZEFUDGE)
          req_kbytes += MAXSIZEFUDGE;

       if (req_kbytes > free_kbytes)
       {
          Werrprintf("Insufficient disk space available to acquire data");
	  disp_acq("");
          return( -1 );
       }

       return( 0 );
}

/*----------------------------------------------------------------------------
|
|	arraytests()
|
|	autogaining (gain='n') cannot be used in multifid experiments
|			(i.e., gain='n', with other arrayed parameters)
|	test for improper interleaving (il='y') (no-no with arrayed parameters,
|		tpc,pad,spin,nt  experiments)
|		acqqueuei#:=ior(16*priority, acqqueuei#);
|		if ilv[1]='y' then acqqueuei#:=ior(1,acqqueuei#); 
|	test for proper usage of arrayed variables and variable 'array'
+---------------------------------------------------------------------------*/
int arraytests()
{
    char  array[MAXSTR];	/* arrayed variable indexing parameter */
    char  interleav[MAXSTR];	/* interleave variable   y or n */
    char *names[MAXARYS];	/* names of arrayed variables */
    double  gain;		/* value for variable gain */
    int   numary;		/* number of arrayed variables */
    
    /*------------------------------------------------------------------
    |   A_getarynames()
    | W A R N I N G ! !  this routine gives the addresses of then names in the
    |  tree therefore never change the contents of what is pointed to
    |  by the pointers else YOU WILL TRASH the VARIBLE TREE..
    +------------------------------------------------------------------*/
    A_getarynames(CURRENT,names,&numary,MAXARYS);
    if (numary == -1)
    {
	Werrprintf("Number of arrayed variables exceeds the maximum of %d",
			MAXARYS);
	return(ERROR);
    }
#ifdef  DEBUG
    if (Gflag)
    {   int i;
	GPRINT1(1,"arraytests(): Number of arrayed variables = %d \n",numary);
	for (i=0; i < numary; i++)
	    GPRINT1(1,"arraytests(): Name: '%s' \n",names[i]);
    }
#endif 
    if (getparm("gain","real",CURRENT,&gain,1))
	return(ERROR);
    GPRINT1(1,"arraytests(): gain = %6.0lf \n",gain);
    /* ---         test il parameter    --- */
    if (getparm("il","string",CURRENT,interleav,MAXSTR))
    {
	Werrprintf("Cannot find interleaving parameter 'il'.");
	return(ERROR);
    }
    GPRINT1(1,"arraytests(): il = '%s' \n",interleav);
    if ( (interleav[0] == 'y') || (interleav[0] == 'Y') )
    {
	if (findname("vtc",names,numary) != NOTFOUND)
	{
	    Werrprintf("Cannot interleave with variable 'vtc' arrayed.");
	    return(ERROR);
	}
	if (findname("pad",names,numary) != NOTFOUND)
	{
	    Werrprintf("Cannot interleave with variable 'pad' arrayed.");
	    return(ERROR);
	}
	if (findname("spin",names,numary) != NOTFOUND)
	{
	    Werrprintf("Cannot interleave with variable 'spin' arrayed.");
	    return(ERROR);
	}
	if (findname("nt",names,numary) != NOTFOUND)
	{
	    Werrprintf("Cannot interleave with variable 'nt' arrayed.");
	    return(ERROR);
	}
    }
    /*-----------------------------------------------------------------
    |		parse the 'array' parameter and test that the variables 
    |		listed are indeed arrayed, that diaginal sets have equal
    |		dimensions, and calculate 'arraydim'
    +-----------------------------------------------------------------*/
    if (getparm("array","string",CURRENT,array,MAXSTR))
	return(ERROR);

    GPRINT1(1,"arraytests():  array: '%s' \n",array);
    
/* #ifdef  XXXX_SEE_TESTARRAYPARAM_DONE_SEPERATELY_4_JPSG_DOESNT_CHECK_ARRAY */

    if (strlen(array) > (size_t) 0)	/* does array have any thing in it */
    {
#ifdef XXX
        if ( !noGainTest && (!var_active("gain",CURRENT)) && (numary > 0) )
        {
	    Werrprintf("Autogain is not permitted in arrayed experiments.");
	    return(ERROR);
        }
#endif
        if (testarrayparm(array,names,&numary))
	    return(ERROR);
    }

/* #endif */
    return(OK);
}


#ifdef XXXXX
int testarrayparam()
{
    char  array[MAXSTR];	/* arrayed variable indexing parameter */
    char *names[MAXARYS];	/* names of arrayed variables */
    int   numary;		/* number of arrayed variables */
    
    /*------------------------------------------------------------------
    |   A_getarynames()
    | W A R N I N G ! !  this routine gives the addresses of then names in the
    |  tree therefore never change the contents of what is pointed to
    |  by the pointers else YOU WILL TRASH the VARIBLE TREE..
    +------------------------------------------------------------------*/
    A_getarynames(CURRENT,names,&numary,MAXARYS);
    if (numary == -1)
    {
	Werrprintf("Number of arrayed variables exceeds the maximum of %d",
			MAXARYS);
	return(ERROR);
    }

    /*-----------------------------------------------------------------
    |		parse the 'array' parameter and test that the variables 
    |		listed are indeed arrayed, that diaginal sets have equal
    |		dimensions, and calculate 'arraydim'
    +-----------------------------------------------------------------*/
    if (getparm("array","string",CURRENT,array,MAXSTR))
	return(ERROR);

    GPRINT1(1,"arraytests():  array: '%s' \n",array);
    
    if (strlen(array) > (size_t) 0)	/* does array have any thing in it */
    {
#ifdef XXX
        if ( (!var_active("gain",CURRENT)) && (numary > 0) )
        {
	    Werrprintf("Autogain is not permitted in arrayed experiments.");
	    return(ERROR);
        }
#endif
        if (testarrayparm(array,names,&numary))
	    return(ERROR);
    }
    return(OK);
}
#endif 

#ifdef SUN

#ifdef XXXX_OBSOLETED_BY_JPSG
/*------------------------------------------------------------------------
|	test4PS(psgpath)
|	test for the presents and executibility of the PS .
+------------------------------------------------------------------------*/
static int test4PS(char *psgpath)
{
    char    psgname[MAXPATH];

    if (getparm("seqfil","string",CURRENT,psgname,MAXPATH))
    {
	Werrprintf("Cannot find 'seqfil' parameter.");
	return(ERROR);
    }
    if (appdirFind(psgname, "seqlib", psgpath, "", R_OK|X_OK|F_OK) == 0 )
    {
	Werrprintf("seqfil: '%s' cannot be found",psgname);
	return(ERROR);
    }
    return(OK);
}
#endif 
/*------------------------------------------------------------------------
|	test4JPS(psgpath)
|	test for the presents and executibility of the PS .
+------------------------------------------------------------------------*/
static int test4PS(char *psgpath)
{
    char    psgname[MAXPATH];
    char    psgJname[MAXPATH];
    char    psgJpath[MAXPATH];
    int     jFound, cFound;

    if (getparm("seqfil","string",CURRENT,psgname,MAXPATH))
    {
	Werrprintf("Cannot find 'seqfil' parameter.");
	return(ERROR);
    }
    strcpy(psgJname,psgname);
    strcat(psgJname,".psg");
    jFound = appdirFind(psgJname, "seqlib", psgJpath, "", R_OK|X_OK|F_OK);
    cFound = appdirFind(psgname, "seqlib", psgpath, "", R_OK|X_OK|F_OK);
    if ( (cFound == 0) && (jFound == 0) )
    {
	Werrprintf("seqfil: '%s' cannot be found",psgname);
	return(ERROR);
    }
    else if (jFound == 0)
    {
        /* No JPSG, only C PSG found */
	return(USE_PSG);
    }
    else if (cFound == 0)
    {
        /* No C PSG, only JPSG found */
        strcpy(psgpath, psgJpath);
        return(USE_JPSG);
    }
    else
    {
        /* Both found, use one found earliest in search path */
        if (jFound <= cFound)
        {
           strcpy(psgpath, psgJpath);
	   return(USE_JPSG);
        }
        else
        {
	   return(USE_PSG);
        }
    }
}
/*------------------------------------------------------------------------
|	test4Lock()
|	test for the presents of the AcqProc Lock file in systemdir/acqqueue
+------------------------------------------------------------------------*/
static int test4Lock()
{
    char lockpath[MAXPATH];
    strcpy(lockpath,systemdir);	/* if path starts with '/' its absoute */
    strcat(lockpath,"/acqqueue/acqlock");  /* e.g. /jaws/acqqueue/acqlock */
    if ( access(lockpath,F_OK) == 0)
    {
    	Werrprintf("Interactive Display in use, no 'go' possible");
    	return(ERROR);
    }
    return(OK);	/* not found then its OK */
}
/*------------------------------------------------------------------------
|	test4ACQ(dirname,dirpath)
|	test for the presence of the acqfile.
|	'file' = exp then use exp#/acqfile (no test)
|       dirname;	the acqfile names
|       dirpath;	the absolute path found for the acqfile
+------------------------------------------------------------------------*/
static int test4ACQ(char *dirname, char *dirpath, int acqiflag)
{
    char acqpath[MAXPATH];

    /* if file = exp then do not create a directory, use exp#/acqfil */
    if ( strcmp(dirname,"exp") == 0)
    {
        strcpy(dirpath,curexpdir);
	strcat(dirpath,"/acqfil");
	if ((suflag == EXEC_GO) && (!acqiflag))
        	/*delete fid and log only if GO,not SU,LOCK,SHIM,ACQI,etc */
	{
            strcpy(acqpath,dirpath);
            strcat(acqpath,"/log");
	    unlink(acqpath);
            strcpy(acqpath,dirpath);
            strcat(acqpath,"/fid");
	    unlink(acqpath);
        }
    }
    else if (automode || vpmode)	/* check for the named acqfile */
    {
        if (dirname[0] == '/')
           strcpy(dirpath,dirname);
        else
           sprintf(dirpath,"%s/%s",autodir,dirname);
        if (vpmode)
        {
	   strcpy(acqpath,dirpath);
        }
        else
        {
	   strcpy(acqpath,dirpath);
	   strcat(acqpath, ".fid");
        }
	GPRINT1(1,"test4ACQ(): acqpath: '%s'\n",acqpath);
    	if (access(acqpath,F_OK))	/* is file already there ? */
    	{					/* yes */
	   Werrprintf("ERROR: File %s could not be created",acqpath);
	   ABORT;
    	}
    }
    else
        ABORT;
    RETURN;
}
/*------------------------------------------------------------------------
|	test4meth(method,methodpath)
|	test for the presence of the shimming method file.
+------------------------------------------------------------------------*/
static int test4meth(char *method, char *methodpath)
{
    char   buffer[BUFSIZE];
    char   buffer2[BUFSIZE];

    FILE  *shimstream;
    int bytes;			/* number of bytes read */
    int i,j;

    if (method[0] == '/')
    {  
    	if ( access(method,F_OK|R_OK) != 0)
        {
           Werrprintf("shim method: '%s' cannot be found",method);
	   return(ERROR);
        }
        strcpy(methodpath,method);	/* absolute path to method */
    }
    else				/* search for method */
    {
        if (appdirFind(method,"shimmethods",methodpath,"",R_OK|F_OK) == 0)
        {
           if (appdirFind(method,"proshimmethods",methodpath,"",R_OK|F_OK))
           {
              strcpy(methodpath,"");	/* null path to method */
              return(OK);
           }
           Werrprintf("shim method: '%s' cannot be found",method);
	   return(ERROR);
        }
    }

    /* found it so go head and read in method command string (128 MAX) */

    shimstream = fopen(methodpath,"r");
    bytes = fread(buffer,sizeof(*buffer),BUFSIZE,shimstream);
    fclose(shimstream);
    GPRINT1(1,"test4meth(): Read %d characters \n",bytes);
    for (j=i=0; i < bytes;i++) /* remove any CR or LF from string */
    {
	if ((buffer[i] != 0xa) && (buffer[i] != 0xd)) 
	    buffer2[j++] = buffer[i];
    }
    buffer2[j] = 0;
    if (j >= MAXSTR)
    {
	Werrprintf("Method '%s' greater than maximum of %d characters",methodpath,MAXSTR);
	return(ERROR);
    }
    GPRINT1(1,"method string: '%s' \n",buffer2);
    if (setparm("com$string","string",CURRENT,buffer2,1))
	return(ERROR);
    return(OK);
}

/*  This program uses the status from acqproc method to get the current
    console status block.  We check if the console is in tune mode and if
    an acquisition is running in the current experiment.

    Note that the console should not be in interactive mode; the go
    program signals ACQI to disconnect from its interactive display.  */

static int check_status_console(char *cmdname, char *hostname )
{
	int		 curexpnum;
        int              acqstate;
        int              acqsuflag;
	char		 expid[ MAXSTR ];
	char		 userid[ MAXSTR ];
	char		 expuser[ MAXSTR ];

        GET_ACQ_ADDR(userid);
        P_setstring(GLOBAL,"acqaddr",userid,0);

        if (GETACQSTATUS(hostname,UserName) < 0)
        {
            Werrprintf( "%s:  failed to obtain acquisition status", cmdname);
            return(-1);
        }
        getAcqStatusInt(STATE, &acqstate);
	if (acqstate == ACQ_TUNING ) {
		Werrprintf( "%s:  cannot proceed, console in tune mode", cmdname );
		return( -1 );
	}
	if (acqstate == ACQ_INACTIVE ) {
		Werrprintf( "%s:  cannot proceed, Acquisition system is not active", cmdname );
		return( -1 );
	}
        getAcqStatusInt(SUFLAG, &acqsuflag);
        getAcqStatusStr(EXPID, expid, MAXSTR-1);
        getAcqStatusStr(USERID, expuser, MAXSTR-1);

/*  shim shim shim go     <=== OK
    go go                 <=== Not OK

    Multiple acquisitions (such as shim or lock)
    that do not collect data may be queued up in
    one experiment; however, only one go/ga/au,
    an acquisition that collects data, may be
    active or queued up in an experiment at a time  */

#if 0

/* This was the old program  */

	if (strlen( &expid[ 0 ] ) > (size_t)0 /* expt in progress */
	    && mode_of_vnmr != AUTOMATION 	/* no test in automation */
	    && suflag == EXEC_GO 		/* this command collects data */
	    && acqsuflag == EXEC_GO)	/* current expt collects data */
	{
	   int res;

	   curexpnum = expdir_to_expnum( curexpdir );

  /* see expactive() in acqhwcmd.c */

           res = is_exp_active( curexpnum, NULL );
	   if (res == 1)
           {
	      Werrprintf(
	   "Experiment in progress, use 'aa' to abort it and then reenter '%s'",
                   cmdname);
	      return( -1 );
	   }
	   else if (res > 1)
           {
	      Werrprintf(
	   "Experiment queued, use 'sa' to delete it and then reenter '%s'",
                   cmdname);
	      return( -1 );
	   }
	}
#endif 

/*  This is the new program  */

	if (strlen( &expid[ 0 ] ) > (size_t)0   /* expt in progress */
	    && mode_of_vnmr != AUTOMATION 	/* no test in automation */
	    && suflag == EXEC_GO 		/* this command collects data */
            && !vpmode)                         /* this command collects data in a different viewport */
	{
	   int res;

	   if (acqsuflag == EXEC_GO)	/* current expt collects data */
	   {
	      char *this_expname;

	  /* compare this user with expuser
	     compare this experiment with expid
	     if both match, experiment in progress  */

	      this_expname = strrchr( curexpdir, '/' );
              if (this_expname == NULL)
		this_expname = &curexpdir[ 0 ];
	      else
		this_expname++;		/* move past the '/' */
          
	      if (strcmp( UserName, expuser ) == 0 &&
		  strcmp( this_expname, expid ) == 0)
	      {
		 Werrprintf(
	   "Experiment in progress, use 'aa' to abort it and then reenter '%s'",
		      cmdname);
		 return( -1 );
	      }
	   }

          /*  Regardless of whether the current (active) acquisition
	      collects data, search the queue for a data-collecting
	      acquisition in this experiment.  The active acquisition
	      could be a shim, with a go next in the queue.  If that
	      go will run in the current experiment and this command
	      would also collect data, then this command has to abort.  */

	   curexpnum = expdir_to_expnum( curexpdir );
	   res = is_data_present( curexpnum );
	   if (res == 1)
           {
	      Werrprintf(
	   "Experiment in progress, use 'aa' to abort it and then reenter '%s'",
                   cmdname);
	      return( -1 );
	   }
	   else if (res > 1)
           {
	      Werrprintf(
	   "Experiment queued, use 'acqdequeue' to delete it and then reenter '%s'",
                   cmdname);
	      return( -1 );
	   }
	}

	return( 0 );
}

static void runPsgPutCmd(int debugPutCmd)
{
   char path[MAXPATH];
   char *name = "psgCmd";

   sprintf(path,"%s/%s",curexpdir,name);
   if ( ! access(path,R_OK))
   {
      if ( ! macroLoad(name,path))
      {
         char path2[32];

         strcpy(path2,name);
         strcat(path2,"\n");
         execString(path2);
         purgeOneMacro(name);
      }
      if ( ! debugPutCmd )
         unlink(path);
   }
}

/*------------------------------------------------------------------------
|	saveVpPars(acqfile)/1
|		copys the experiment TEXT file and parameter file to the
|               named acqfile.
+------------------------------------------------------------------------*/
static int saveVpPars(char *acqfile, int debugPutCmd)
{
    char    path[MAXPATHL];

    P_setstring(CURRENT,"file","exp",1);
    p11_saveFDAfiles_raw("go:savepars", "", acqfile);
    runPsgPutCmd(debugPutCmd);
    strcpy(path,acqfile);
    strcat(path,"/procpar");
    GPRINT1(1,"copying processed parameters to '%s' file.\n",path);
    if (P_save(CURRENT,path))
    {   Werrprintf("Problem saving processed parameters in '%s'.",path);
        ABORT;
    }
    if (copytext(acqfile))  /* copy experiment text file to named acqfile */
	ABORT;

    GPRINT(1,"GO: Write Parameters to disk\n");
    P_treereset(CURRENT); /* clear tree */
    sprintf(path,"%s/curparSave",curexpdir);
    P_read(CURRENT,path); /* reread saved current tree if in vpmode */
    unlink(path);
    RETURN;
}
/*------------------------------------------------------------------------
|	savepars(acqfile)/1
|		copys the experiment TEXT file and parameter file to the
|               named acqfile.
+------------------------------------------------------------------------*/
static int savepars(char *acqfile, int debugPutCmd)
{
    char    path[MAXPATHL];

    p11_saveFDAfiles_raw("go:savepars", "", acqfile);

    GPRINT(1,"GO: Write Parameters to disk\n");
    P_treereset(PROCESSED); /* clear tree */
    P_copy(CURRENT,PROCESSED); /* save current into processed tree if GO */
    runPsgPutCmd(debugPutCmd);

    if (!automode)
    {
       strcpy(path,userdir);
       strcat(path,"/global");
       GPRINT1(1,"copying Global parameters to '%s' file.\n",path);
       if (P_save(GLOBAL,path))
       {   Werrprintf("Problem saving global parameters in '%s'.",path);
	   ABORT;
       }
    }
    strcpy(path,acqfile);
    strcat(path,"/procpar");
    GPRINT1(1,"copying processed parameters to '%s' file.\n",path);
    if (P_save(PROCESSED,path))
    {   Werrprintf("Problem saving processed parameters in '%s'.",path);
        ABORT;
    }
    if (copytext(acqfile))  /* copy experiment text file to named acqfile */
	ABORT;

    RETURN;
}
/*------------------------------------------------------------------------
|	copytext/1
|		copys the experiment TEXT file to the named acqfile.
|		exp#/text --> pwd/dirname/text
+------------------------------------------------------------------------*/
static int copytext(char *dirname)
{
    char   filea[MAXPATH];
    char   fileb[MAXPATH];

    GPRINT2(1,"copytext from '%s/text' to '%s/text'\n",curexpdir,dirname);
    sprintf(filea,"%s/text",curexpdir);
    sprintf(fileb,"%s/text",dirname);
    copyFile(filea,fileb,0);
    RETURN;
}
/*----------------------------------------------------------------
|	test for proper parameter setting for np, rfband, and ct.
+---------------------------------------------------------------*/
static int check_acqpar()
{
    char    il[10];
    char    rfband[10];
    char    rftype[10];
    char    dpstr[10];
    double  sweepwidth;
    double  acqtime;
    double  np_max;
    double  re_and_im;
    double  bs;
    double  nt;
    double  maxsw;
    double  maxlbsw;
    int     i;
    double  numrfch;

    if (getparm("np","real",CURRENT,&re_and_im,1))
	ABORT;
    if (getparm("rftype", "STRING", SYSTEMGLOBAL, &rftype[0], 2))
	ABORT;
    rftype[9]='\000';
    if (getparm("il","string",CURRENT,il,10))
	ABORT;
    /*----------------------------------------------------------------
    |	test for proper parameter setting in the case for Wideline where
    |	sw > 100KHz or 200KHz.  Then:
    |         np <= 16K or <= 8192 complex pairs for pre U+
    |         np <= 256k or < 65536 complex pairs for U+
    |
    |         if dp != "y" then make it equal to "y" 
    +---------------------------------------------------------------*/
    if (getparm("sw","real",CURRENT,&sweepwidth,1))
	ABORT;
    if (getparm("dp", "STRING", CURRENT, &dpstr[0], 2))
	ABORT;
    if (P_getreal( SYSTEMGLOBAL, "parmax", &maxsw, 5 ) )
	ABORT;
#ifdef VNMRJ
    if (nvAcquisition())
       maxlbsw = 2e7;
    else
#endif
    if (P_getreal(SYSTEMGLOBAL, "maxsw_loband", &maxlbsw, 1) < 0){
	maxlbsw = 100000.1;
    }else{
	maxlbsw += 0.1;
    }
    if (sweepwidth > maxlbsw)		/* is Wideline acquisition */
    {
        if (rftype[0] == 'd')
	{ if (maxsw >4.9e6) np_max = 262144.0;
          else              np_max = 131072.0;
        }
        else np_max = 16384.0;
	if ( re_and_im > np_max )
        {
	    GPRINT3(1,"sw = %10.2lf, old np = %lf, new np = %lf\n",
		sweepwidth,re_and_im,np_max);
	    re_and_im = np_max;
	}
    }
    acqtime = (re_and_im / sweepwidth) / 2.0;
    GPRINT3(1,"at = %g, np = %g, sw = %g\n",acqtime,re_and_im,sweepwidth);
    if (setparm("np","real",CURRENT,&re_and_im,1))
	ABORT;
    if (setparm("at","real",CURRENT,&acqtime,1))
	ABORT;
    /*----------------------------------------------------------------
    |	test for proper usage of rfband 
    |   change to new style values if using old style values
    +---------------------------------------------------------------*/
    if (getparm("rfband","string",CURRENT,rfband,10))
	ABORT;
    GPRINT1(1,"checking rfband %s\n",rfband);

    if (P_getreal(SYSTEMGLOBAL,"numrfch",&numrfch,1) >= 0)
    {
      if (numrfch > 1.1)
      {
        if (rfband[1] == '\0')
        {
          rfband[1] = 'c';
          rfband[2] = '\0';
        }
      }
      if (numrfch > 2.1)
      {
        if (rfband[2] == '\0')
        {
          rfband[2] = 'c';
          rfband[3] = '\0';
        }
      }
    }

    /* translate old style to new style values */
    for (i=0; i < (int) (numrfch + 0.1); i++)
    {   switch(rfband[i])
	{   case 'b': 	rfband[i] = 'h';
			break;
	    case 'a': 	rfband[i] = 'l';
			break;
	}
    }
    if (setparm("rfband","string",CURRENT,rfband,1))
	ABORT;
    /*----------------------------------------------------------------
    |	Warnings if il, and bs,nt not set to rational values
    +---------------------------------------------------------------*/
    if ( (il[0] == 'y') || (il[0] == 'Y') )
    {
      if (getparm("bs","real",CURRENT,&bs,1))
	ABORT;
      if (getparm("nt","real",CURRENT,&nt,1))
	ABORT;
      if ( var_active("bs",CURRENT) )
      {
        if ( bs >= nt )
	   Werrprintf("Warning: interleave has no effect, bs >= nt.");
      }
    }
    RETURN;

}
/*------------------------------------------------------------------------
|	check_loc(automode)
|		checks loc parameter to be sure it is less that or 
|		equal to traymax.
+------------------------------------------------------------------------*/
static int check_loc()
{
    double value;
    int traymax,loc;

    if (getparm("traymax","real",SYSTEMGLOBAL,&value,1))
    {
        traymax = 0;
    }
    else
    {
       traymax = (int) (value + 0.01);
       /* For new sample changer with two 48 sample zones */
       if ( (traymax == 49) || (traymax == 97) )
          traymax = 96;
       if (!var_active("traymax",SYSTEMGLOBAL))         /* if traymax = 'n' */
          traymax = 0;
    }
    if (getparm("loc","real",GLOBAL,&value,1))          /* sample loc */
    {
        loc = 0;
    }
    else
    {
       loc = (int) (value + 0.01);
       if (!var_active("loc",GLOBAL))                   /* if loc = 'n' */
          loc = 0;
    }

/*
    if ( (loc < 0) || ((suflag >= EXEC_CHANGE) && (loc == 0)) ||
	 (loc > traymax) )
*/
    if (loc < 0)
    {
        Werrprintf("Sample location of %d is invalid. loc must be >= 0",
            loc);
        ABORT;
    }
    else if (loc > traymax)
    {
        Werrprintf("Sample location (loc=%d) cannot exceed traymax (%d)",
            loc,traymax);
        ABORT;
    }
    RETURN;
}

/*----------------------------------------------------------------------
|
|	check_ra
|       When resuming an acquisition, check for
|		acquisition suspended
|		acquisition not complete
|		values of NP, DP have not been changed since SA
-----------------------------------------------------------------------*/
static int check_ra()
{
	int	ret;
	double	aval, ceval, ctval, ntval;

/*  NP, DP checks.  Each subroutine displays a message
    if it returns a non-zero value.			*/

	if (check_np_ra())
	  return( -1 );
	if (check_dp_ra())
	  return( -1 );

	ret = P_getreal( PROCESSED, "ct", &ctval, 1 );
	if (ret != 0) {
		Werrprintf( "ra:  `ct' not defined in current parameter set" );
		return( -1 );
	}
	ret = P_getreal( PROCESSED, "nt", &ntval, 1 );
	if (ret != 0) {
		Werrprintf( "ra:  `nt' not defined in current parameter set" );
		return( -1 );
	}

/*  Obtain value of "arraydim", "celem".  If "arraydim" not defined,
    use value of 1 for default.  Insist on presence of "celem" in
    the current parameter tree.						*/

	ret = P_getreal( PROCESSED, "arraydim", &aval, 1 );
    	if (ret != 0) aval = 1.0;

	ceval = 0.0;
	ret = P_getreal( PROCESSED, "celem", &ceval, 1 );
    	if (ret != 0) {
		Werrprintf(
    "ra:  acquisition never suspended (current element not defined)"
		);
		return( -1 );
	}

/*  If `celem' < 1 and `ct' < 1, acquisition was never started.  */

	if (ceval < 1.0) {
		if (ctval < 1.0) {
			Werrprintf( "ra:  acquisition never suspended" );
			return( -1 );
		}

	    /*  If at least one transient present from the first 
		element, we deduce the acquisition was suspended.
		Execution falls through to the return( 0 ) statement.	*/
	}
	else if (ceval >= aval) {
		if (ctval >= ntval) {
			Werrprintf( "ra:  acquisition complete, cannot restart" );
			return( -1 );
		}

	    /*  If CT < NT, we deduce the acquisition was suspended,
		even if on the final element.  Execution falls through
		to the return( 0 ) statement.	*/
	}

	return( 0 );
}


/*  Verify that `np' has not been changed between SA and RA
    by comparing the current value with the processed value.	*/

static int check_np_ra()
{
	int	ret;
	double	cur_np_val, proc_np_val;

	ret = P_getreal( CURRENT, "np", &cur_np_val, 1 );
	if (ret != 0) {
		Werrprintf( "ra:  `np' not defined in current parameter set" );
		return( -1 );
	}

	ret = P_getreal( PROCESSED, "np", &proc_np_val, 1 );
	if (ret != 0) {
		Werrprintf( "ra:  `np' not defined in processed parameter set" );
		return( -1 );
	}

	if (cur_np_val != proc_np_val) {
		Werrprintf(
	    "ra:  cannot change value of `np' after suspending the acquisition"
		);
		return( -1 );
	}

	return( 0 );
}

/*  Verify that `dp' has not been changed between SA and RA
    by comparing the current value with the processed value.	*/

static int check_dp_ra()
{
	char	cur_dp_buf[ 6 ], proc_dp_buf[ 6 ];
	int	ret;

	ret = P_getstring( CURRENT, "dp", &cur_dp_buf[ 0 ], 1, 4 );
	if (ret != 0) {
		Werrprintf( "ra:  `dp' not defined in current parameter set" );
		return( -1 );
	}

	ret = P_getstring( PROCESSED, "dp", &proc_dp_buf[ 0 ], 1, 4 );
	if (ret != 0) {
		Werrprintf( "ra:  `dp' not defined in processed parameter set" );
		return( -1 );
	}

	if (strcmp( &cur_dp_buf[ 0 ], &proc_dp_buf[ 0 ] ) != 0) {
		Werrprintf(
	    "ra:  cannot change value of `dp' after suspending the acquisition"
		);
		return( -1 );
	}

	return( 0 );
}

/*  Obtains number of new FID to be created.  It is only interesting if the
    current command is "ra"; otherwise the value returned is `arraydim'.
    But if this is "ra", it uses the scheme:

      if (il == 'y') then
          if (ct > bs) then
              number of new elements is 0
          else if (celem == 0 and ct == bs)
              number of new elements is 0
          else
              number of new elements is arraydim - celem
          endif
      else
          number of new elements is arraydim - celem
      endif

    It uses the processed tree to obtain values for all parameters, including
    `arraydim', following the example in `check_ra'.

    Some situations result in an internal error.  See `check_ra', it verifies
    it can obtain values for the parameters in question.			*/

static int get_number_new_fids(int this_is_ra )
{
	char	interleav[ 8 ];
	int	ret;
	double	aval, bsval, ceval, ctval, ntval;

	if (this_is_ra) {
		ret = P_getreal( PROCESSED, "arraydim", &aval, 1 );
		if (ret != 0)
		  aval = 1.0;
		ret = P_getreal( PROCESSED, "celem", &ceval, 1 );
		if (ret != 0) {
			Werrprintf(
	    "internal error in ra, cannot obtain value for 'celem'"
			);
			return( -1 );
		}
		ret = P_getstring( PROCESSED, "il", &interleav[ 0 ],
				1, sizeof( interleav )
		);
		if (ret != 0)
		  strcpy( &interleav[ 0 ], "n" );
		if (strcmp( &interleav[ 0 ], "y" ) == 0) {

	   /* Get value for `nt'; abort if error  */

			ret = P_getreal( PROCESSED, "nt", &ntval, 1 );
			if (ret != 0) {
				Werrprintf(
		    "internal error in ra, cannot obtain value for 'nt'"
				);
				return( -1 );
			}

	   /* Get value for `ct'; abort if error  */

			ret = P_getreal( PROCESSED, "ct", &ctval, 1 );
			if (ret != 0) {
				Werrprintf(
		    "internal error in ra, cannot obtain value for 'ct'"
				);
				return( -1 );
			}

	   /* Get value for `bs'; set to `nt' if error  */

			if (var_active( "bs", CURRENT )) {
				ret = P_getreal( PROCESSED, "bs", &bsval, 1 );
				if (ret != 0)
				  bsval = ntval;
			}
			else
			  bsval = ntval;

	   /* First test succeeds if the acquisition is on the second or
              a later interleave cycle.
              Second test succeeds if the acvquisition stopped at the end
              of the 1st interleave cycle; in this case also data exists
              for each element in the experiment.			*/

			if (ctval > bsval)
			  return( 0 );
			else if (ceval == 0 && ctval == bsval)
			  return( 0 );

	   /* If both tests fail, fall through to the non-interleaved case.  */

		}

		return( (int) (aval - ceval) );
	}

/*  If command is not `ra', return `arraydim'.  Use the internal number instead
    of the parameter; if the command is `su' this program returns 1.		*/

	return( (int) arraydim );
}

/*----------------------------------------------------------------------
|
|	initacqqueue()
|	creates the ACQ. PROCCESS information file in the acqqueue
|	directory.
|	This file contains: experiment priority
|			    unique ID string
|			    date and time of execution
|			    interleave information
|			    condition processing information
|			    absolute path to the FID data
|			    absolute path to the Acode file
|
| Was static, now global to allow usage by dps(), dps.c
|				Greg B.  2/20/90
+----------------------------------------------------------------------*/
int initacqqueue(int argc, char *argv[])
{
    char expname[MAXSTR];
    char date2[MAXSTR];
    char filepath[MAXSTR];
    char logxmlfilepath[MAXSTR];
    FILE *logxmlFD=NULL;
    char str[MAXSTR];
    char *chrptr;
    int i;
    int len;
    struct timeval clock;
    char date[30];

    (void) argc;
    /* --- get user's name --- */
    P_setstring(CURRENT,"goid",UserName,1);  /* this will be overwritten */
    P_setstring(CURRENT,"goid",UserName,2);

    /* --- get experiment number --- */
    len = expdir_to_expnum(curexpdir);	/* get experiment number	*/
    sprintf(expname, "%d", len);	/* form file name, e.g., exp1	*/
    P_setstring(CURRENT,"goid",expname,3);
    sprintf(expname, "exp%d", len);	/* form file name, e.g., exp1	*/
    P_setstring(CURRENT,"goid",expname,4);

    if ((strcmp(argv[CALLNAME],"dps") == 0) ||
        (strcmp(argv[CALLNAME],"pps") == 0))
    {
       sprintf(filepath, "%s/acqqueue/dps", systemdir);
       P_setstring(CURRENT,"goid",filepath,1);
       return(OK);
    }
	
    /* --- get date and time of day --- */
    gettimeofday(&clock, NULL);
    chrptr = ctime(&(clock.tv_sec));	/* translate to ascii string 26char */
    strcpy(date,chrptr);
    len = strlen(date);
    date[len-1] = '\0';		/* remove the embedded CR in string */
    date[len] = '\0';		/* remove the embedded CR in string */
    for (i=0; i<7; i++)
       date2[i] = date[i+4];
    for (i=7; i < 11; i++)		/* copy all 4 digits of the year */
       date2[i] = date[13+i];	    /* reassure anyone worried about Y2K */
    date2[11]='\0';

    /* --- update 'date' in vnmr --- */
    if (setparm("date","string",CURRENT,date2,1))
	return(ERROR);


    /* --- create a unique file name based on time stamp --- */
    sprintf(str,"%s.%s.%s_%ld_%06ld", expname, UserName, HostName,
                 (long) clock.tv_sec, (long) clock.tv_usec);
    sprintf(filepath,"%s/acqqueue/%s", systemdir, str);

    P_setstring(CURRENT,"goid",filepath,1);
    GPRINT2(1,"initacqqueue(): ID: '%s', Date: '%s' \n",filepath,date);
    GPRINT1(1,"initacqqueue(): Date2: '%s' \n",date2);
    if (suflag == EXEC_GO) 
    {
       if (P_setstring(CURRENT,"go_id",str,0))
       {
           P_creatvar(CURRENT,"go_id",T_STRING);
           P_setstring(CURRENT,"go_id",str,0);
           P_setprot(CURRENT,"go_id",P_ARR | P_ACT | P_VAL);  /* do not allow any user change */
       }
       if (P_setstring(PROCESSED,"go_id",str,0))
       {
           P_creatvar(PROCESSED,"go_id",T_STRING);
           P_setstring(PROCESSED,"go_id",str,0);
           P_setprot(PROCESSED,"go_id",P_ARR | P_ACT | P_VAL);  /* do not allow any user change */
       }
    }
    /* date is globals */
    if (getparm("priority","real",CURRENT,&priority,1))
	return(ERROR);
    GPRINT1(1,"initacqqueue(): priority = %4.1lf \n",priority);

    /* Open /vnmr/adm/accounting/loggingParamList to get the user specified params.
       It should be a text file with 3 columns.  
          param  real/string   global/current/etc.
       such as:
          tn    string   current
          sw    real     current
          owner string   global
          .....
       Eliminate params that are written in bill.c by default.  
       That includes: operator and seqfil.
    */

    /* if /vnmr/adm/accounting/acctLog.xml does not exist, then "go" accounting
       is turned off.  Don't do any logging stuff.
    */
    sprintf(logxmlfilepath, "%s/adm/accounting/acctLog.xml", systemdir);
    logxmlFD = fopen(logxmlfilepath, "r");
    if(logxmlFD == NULL) {
      return(OK);
    }
    fclose(logxmlFD);

    // Only fill optParams once
    if(!optParamsFilled) {
        FILE *paramListFD=NULL;
        size_t  lineSize = 256;
        char line[256], *word;
        char paramListPath[MAXSTR];

        sprintf(paramListPath, "%s/adm/accounting/loggingParamList", systemdir);

        // Add owner, systemname and vnmraddr to the param list
        strcpy(optParams[0].name,"owner");
        strcpy(optParams[0].tree,"global");
        strcpy(optParams[1].name,"systemname");
        strcpy(optParams[1].tree,"global");
        strcpy(optParams[2].name,"vnmraddr");
        strcpy(optParams[2].tree,"global");
        i=3;
        
        paramListFD = fopen(paramListPath, "r");
        if(paramListFD != NULL) {
            // start at 3 since we already filled owner, systemname and vnmraddr

            while(fgets(line, lineSize-1, paramListFD)) {
                // Skip empty lines 
                if(strlen(line) == 0)
                    continue;

                // Get param name
                word = strtok(line, " \t\r\n");
                // If word  null, there was no token returned, must be white space
                if(word == NULL)
                    continue;

                // Skip these.  Owner is above and the others are already taken
                // care of in bill.c .  Also skip comment lines.
                if((strcmp(word, "operator") == 0) || 
                   (strcmp(word, "seqfil") == 0) ||
                   (strcmp(word, "owner") == 0) ||
                   (strcmp(word, "systemname") == 0) ||
                   (strcmp(word, "vnmraddr") == 0) ||
                   (word[0] == '#')) {
                    continue;
                }
                strcpy(optParams[i].name, word);
               
                // Param location global, current etc
                word = strtok(NULL, " \t\r\n");
                if(word == NULL) {
                    Werrprintf("Syntax error: %s", word);
                    continue;
                }
                strcpy(optParams[i].tree,word);
                i++;
            }
            fclose(paramListFD);
        }
        else
            Werrprintf("Cannot open: %s", paramListPath);

        optParamsFilled = TRUE;
    }

    // Flag to know the last row of optParams
    optParams[i].name[0] = 0;

    /* Write params and values to logfilepath.  This info is to be included
       in the /vnmr/adm/accounting/acctLog.xml in bill.c which will take
       care of the actual xml file.  This is just the params and values with
       no xml tags, in the form:
           param1="value1"
           param2="value2"
           ....
    */
    FILE *logFileFD=NULL;
    char logfilepath[MAXSTR];

    /* Create the log file name with user specified parameter values*/
    sprintf(logfilepath,"%s.loginfo", filepath);
    logFileFD = fopen(logfilepath, "w");
    //Werrprintf("log file: %s fd:%d", logfilepath,logFileFD);
    if(logFileFD != NULL) {
        char svalue[128];
        double fvalue;
        int status;
        int tree;
        vInfo varinfo;     /* variable information structure */
 
        for(i=0; strlen(optParams[i].name)>0; i++) {
            tree = getTreeIndex(optParams[i].tree);
            status=P_getVarInfo(tree, optParams[i].name, &varinfo);
            if (status <0) {
                Werrprintf("Cannot find the variable: %s tree:%d", optParams[i].name,tree);
                // Just skip this param and continue
            }
            else {
               // Werrprintf("Found variable: %s type:%d", optParams[i].name,varinfo.basicType);
                // Get the value for this param
                if(varinfo.basicType == ST_REAL) {
                  // Convert tree string to int
                    //tree = getTreeIndex(optParams[i].tree);
                    status = P_getreal(tree, optParams[i].name, &fvalue, 1);
                    if(status==0) {
                        fprintf(logFileFD, "       %s=\"%f\"\n", optParams[i].name, fvalue);
                    }
                }
                else  {
                    // Convert tree string to int
                    //tree = getTreeIndex(optParams[i].tree);
                    status = P_getstring(tree, optParams[i].name, svalue, 1, 63);
                    if(status==0) {
                        fprintf(logFileFD, "       %s=\"%s\"\n", optParams[i].name, svalue);
                        //Werrprintf("String found: %s value:%s", optParams[i].name,svalue);
                    }
                    //else
                    //    Werrprintf("String not found: %s error:%d", optParams[i].name,status);
                }
            }
        }
        fclose(logFileFD);
    }

    return(OK);
}
#endif 
/*------------------------------------------------------------------
|
|	validcall()
|
|  set suflag according to call name used. 
|  check for alias to be valid for system.
|  set autoflag  
|	return 1 of OK else returns 0;
|
| 		Author  Greg Brissey  4/21/86
+---------------------------------------------------------------------*/
static int validcall(int argc, char *argv[], int *datastation, int checkacqexpt)
{
    int    len;
    int    expno;

    *datastation = is_datastation();
    if (argtest(argc,argv,"go"))
       strcpy(callname,"go");
    else if (argtest(argc,argv,"ga"))
       strcpy(callname,"ga");
    else if (argtest(argc,argv,"au"))
       strcpy(callname,"au");
    else if (argtest(argc,argv,"su"))
       strcpy(callname,"su");
    else if (argtest(argc,argv,"change"))
       strcpy(callname,"change");
    else if (argtest(argc,argv,"sample"))
       strcpy(callname,"sample");
    else
       strcpy(callname,argv[CALLNAME]);
    len = strlen( &curexpdir[ 0 ] );
    if (len < 1)
    {
	Werrprintf( "%s:  current experiment not defined", callname);
	return( 0 );
    }

    expno = expdir_to_expnum(curexpdir);
    if ( expno == 0 )
    {
#ifdef VNMRJ
	if (argtest(argc,argv,"calcdim") == 0)
#endif 
	   Werrprintf( "%s:  no current experiment", callname);
	return( 0 );
    }
    else if (checkacqexpt)
    {
       if ( expno > MAXACQEXPS )
       {
          Werrprintf( "%s:  only experiments 1-9 are for acquisition",
			   callname );
          return( 0 );
       }
    }
    else if ( expno > MAXEXPS )
    {
       Werrprintf( "%s:  invalid experiment number", callname);
       return( 0 );
    }

    /*  check for aliases of GO */
    if (argtest(argc,argv,"go") ||
	argtest(argc,argv,"ga") ||
	argtest(argc,argv,"au") ||
	(strcmp(argv[CALLNAME],"ra") == 0))
	suflag = EXEC_GO;
    else if (strcmp(callname,"su") == 0)
	suflag = EXEC_SU;
    else if (strcmp(argv[CALLNAME],"shim") == 0) 
	suflag = EXEC_SHIM;
    else if (strcmp(argv[CALLNAME],"lock") == 0)
	suflag = EXEC_LOCK;
    else if (strcmp(argv[CALLNAME],"spin") == 0)
        suflag = EXEC_SPIN;
    else if (strcmp(callname,"change") == 0)
	suflag = EXEC_CHANGE;
    else if (strcmp(callname,"sample") == 0)
        suflag = EXEC_SAMPLE;
    else if (strcmp(callname,"exptime") == 0)
        suflag = EXEC_EXPTIME;
    else if (strcmp(callname,"dps") == 0 || strcmp(callname,"pps") == 0 )
        suflag = EXEC_DPS;
    else if (strcmp(callname,"createparams") == 0)
        suflag = EXEC_CREATEPARAMS;
    else
    {
        Werrprintf("'%s': is an invalid alias of go",callname);
	return(0);
    }
    /*if ( ((suflag > EXEC_SU) && (suflag < EXEC_SPIN)) || 
	 (suflag > EXEC_CHANGE) )
    {	Werrprintf("'%s': is an unimplemented alias of go",
                callname);
        return(0);
    } */
    automode = (mode_of_vnmr == AUTOMATION);
    if (automode) 
    {
        if (getparm("autodir","string",GLOBAL,autodir,MAXPATH))
    	{    Werrprintf("cannot find parameter: autodir");
	     return(0);
    	}
    }
    GPRINT1(1,"automode= %d\n",automode);
    return(1);
}

/*-----------------------------------------------------------------------
|  	testarrayparm/3
|	tests that the array parameter's  parameters are indeed
|	arrayed variables. For each parameter it finds in the list
|	of arrayed variables it removes it from the list.
|	Also if varialbe in '(' ')' do not have the same dimension
|	error is produced.
|	(e.g.  array ='sw,(pw,d1),dm' )     (pw must = d1 in dimension)
|			Author Greg Brissey  6/4/86
+------------------------------------------------------------------------*/
#define letter1(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z')))
#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#'))
#define digit(c) (('0'<=(c))&&((c)<='9'))
#define NIL 0
#define COMMA 0x2C
#define RPRIN 0x29
#define LPRIN 0x28

static int testarrayparm(char *string, char *names[], int *nnames)
{
    int state;
    char *ptr;
    char preparm[MAXSTR];/* pervious parameter */
    char *varptr = NULL;	/* pointer to one variable in the 'array' string */

    state = 0;
    strcpy(preparm,"");
    ptr = string;
    GPRINT1(1,"testarray(): string: '%s' -----\n",string);
    /*
     * ---  test the variables as we parse them
     * ---  This is a 4 state parser, 0-1: separate variables
     * --- 			      2-4: diagonal set variables
     */
    while(1)
    {
    switch(state)
    {
       /* ---  start of variable name --- */
       case 0:
	    GPRINT(2,"Case 0: ");
            GPRINT1(2,"letter: '%c', ",*ptr);
	    if (letter(*ptr))	/* 1st letter go to state 1 */
	    {   varptr = ptr;
	 	state = 1;
		ptr++;
	    }
	    else
	    {   if (*ptr == LPRIN )	/* start of diagnal arrays */
		{
	    	    state = 2;
		    ptr++;
		}
		else
		{   if (*ptr == NIL)	/* done ? */
			return(OK);
		    else		/* error */
		    {
			Werrprintf("Syntax error in variable '%s'",arrayname);
			return(ERROR);
		    }
		}
	    }
	    GPRINT1(2," state = %d \n",state);
	    break;
       /* --- complete a single array variable till ',' --- */
       case 1:
	    GPRINT(2,"Case 1: ");
            GPRINT1(2,"letter: '%c', ",*ptr);
	    if (letter(*ptr) || digit(*ptr))
	    {
		ptr++;
	    }
	    else
	    {
		if ( *ptr == COMMA )
		{
		    *ptr = NIL;
		    if (checkparm(varptr,preparm,names,nnames))
			return(ERROR);
		    ptr++;
		    state=0;
		}
		else
		{
		    if (*ptr == NIL)
		    {
		    	if (checkparm(varptr,preparm,names,nnames))
			    return(ERROR);
			return(OK);
		    }
		    else
		    {
		    	Werrprintf("Syntax Error in variable '%s'",arrayname);
		    	return(ERROR);
		    }
		}
	    }
            GPRINT1(2," state = %d \n",state);
	    break;
       /* --- start of diagnal arrayed variables  'eg. (pw,d1)' --- */
       case 2:
	    GPRINT(2,"Case 2: ");
            GPRINT1(2,"letter: '%c', ",*ptr);
	    if (letter(*ptr))
	    {   varptr = ptr;
	 	state = 3;
		ptr++;
	    }
	    else
	    {
	    	Werrprintf("Syntax Error in variable '%s'",arrayname);
		return(ERROR);
	    }
            GPRINT1(2," state = %d \n",state);
	    break;
       /* --- finish a diagonal arrayed variable  name --- */
       case 3:
	    GPRINT(2,"Case 3: ");
            GPRINT1(2,"letter: '%c', ",*ptr);
	    if (letter(*ptr) || digit(*ptr))
	    {
		ptr++;
	    }
	    else
	    {
		if (*ptr == COMMA)
		{
		    *ptr = NIL;
		    if (checkparm(varptr,preparm,names,nnames))
			return(ERROR);
		    strcpy(preparm,varptr);
		    ptr++;
		    state=2;
		}
		else
		    if (*ptr == RPRIN )
		    {
		        *ptr = NIL;
		    	if (checkparm(varptr,preparm,names,nnames))
			    return(ERROR);
		    	ptr++;
		        strcpy(preparm,"");
		        state=4;
		    }
		else
		{
		    Werrprintf("Syntax Error in variable '%s'",arrayname);
		    return(ERROR);
		}
	    }
            GPRINT1(2," state = %d \n",state);
	    break;
       /* --- finish a diagonal arrayed variable  set --- */
       case 4:
	    GPRINT(2,"Case 4: ");
            GPRINT1(2,"letter: '%c', ",*ptr);
	    if ( *ptr == COMMA )
	    {
	        *ptr = NIL;
	        ptr++;
		strcpy(preparm,"");
	        state=0;
	    }
	    else
	    {
		if (*ptr == NIL)
		{
		    return(OK);
		}
		else
		{
	            Werrprintf("Syntax Error in variable '%s'",arrayname);
		    return(ERROR);
		}
	    }
            GPRINT1(2," state = %d \n",state);
	    break;
    }
    }
}
/*----------------------------------------------------------------
|	checkparm(varptr,pervar,names,nnames)
|	check to see if the variable is arrayed.
|	If arrayed then remove it from the list return(0)
|	If not arrayed return(1)
|	Test diagonal arrays for equal dimensions
|	keep a running count on fids obtain for the arrayed variables
|	keep a running count on number or array elements needed in PSG 
|	return(0) if OK else return(1)
|			Author Greg Brissey  4/6/86
+-----------------------------------------------------------------*/
static int checkparm(char *varptr, char *prevar, char *names[], int *nnames)
{
    int index;

    GPRINT2(1,"checkparm(): previous var: '%s', present var: '%s'\n",
		prevar,varptr);
    index = findname(varptr,names,*nnames);
    /*
    if (index == NOTFOUND)
    {
	Werrprintf("Variable '%s' is not arrayed. Correct '%s' or '%s'",
		varptr,arrayname,varptr);
	return(ERROR);
    }
    */
    if (index != NOTFOUND)
    {
      GPRINT3(1,"checkparm():  variable: '%s', was found at list[%d] = '%s'\n",
		varptr,index,names[index]);
      removename(index,names,nnames);
    }
       /* GPRINT3(1,"checkparm():  variable: '%s', is not arrayed. \n",varptr); */

    /*---------------------------------------------------------------
    | running calculation of # fid obtain with each new array 
    | Note: for each new variable the prevar is null, only on the 
    | second, plus  variables in a diagonal set "eg (pw,d1)" will have 
    | a previous variable set  
    +---------------------------------------------------------------*/
    if (strlen(prevar) == 0)	/* only if separate array variable */
    {   vInfo	varinfo;		/* variable information structure */
	if (P_getVarInfo(CURRENT,varptr,&varinfo) )
    	{   Werrprintf("Cannot find the variable: %s",varptr);
	    return(ERROR);
    	}
    	arraydim *=(double) varinfo.size;
    	acqcycles *= (double)varinfo.size;
	arrayelemts += 1.0;  /* increment number of array elements */
    }
    GPRINT2(1,"checkparm():  arraydim = %5lf arrayelemts = %5lf \n",
		arraydim,arrayelemts);

    /* true for second+plus variables in diagonal set*/
    if (strlen(prevar) > (size_t) 0)
    {
       if (equalsize(prevar,varptr)) /* equal dimensions ? */
       {
	   Werrprintf("diagonal arrays '%s', '%s' have unequal dimensions",
			prevar,varptr);
	   return(ERROR);
       }
    }
    return(OK);
}
/*-----------------------------------------------------------------
|	equalsize(var1,var2)
|	check to see that the two variables have equal dimensions 
|       If the variable is not arrayed, and it is one of the nD
|       incrementing variables (d2,d3,d4,d5), check the value of
|       the corresponding nD variable (ni,ni2,ni3,ni4)
|			Author Greg Brissey  6/6/86
+-----------------------------------------------------------------*/
static int equalsize(char *var1, char *var2)
{
    int nvals1;
    int nvals2;
    double val;
    vInfo	varinfo;		/* variable information structure */

    if (P_getVarInfo(CURRENT,var1,&varinfo) )
    {   Werrprintf("equalsize(): Cannot find the variable: %s",var1);
	return(ERROR);
    }
    nvals1 = varinfo.size;
    if (nvals1 == 1)
    {
       if (! strcmp(var1,"d2") )
       {
          if ( ! P_getreal(CURRENT,"ni", &val, 1) )
             if (val > 1.0)
                nvals1 = (int) (val+0.1);
       }
       else if (! strcmp(var1,"d3") )
       {
          if ( ! P_getreal(CURRENT,"ni2", &val, 1) )
             if (val > 1.0)
                nvals1 = (int) (val+0.1);
       }
       else if (! strcmp(var1,"d4") )
       {
          if ( ! P_getreal(CURRENT,"ni3", &val, 1) )
             if (val > 1.0)
                nvals1 = (int) (val+0.1);
       }
       else if (! strcmp(var1,"d5") )
       {
          if ( ! P_getreal(CURRENT,"ni4", &val, 1) )
             if (val > 1.0)
                nvals1 = (int) (val+0.1);
       }
    }
    if (P_getVarInfo(CURRENT,var2,&varinfo) )
    {   Werrprintf("equalsize(): Cannot find the variable: %s",var2);
	return(ERROR);
    }
    nvals2 = varinfo.size;
    if (nvals2 == 1)
    {
       /*
        * If second var is nD element, prevent multplying arraydim by ni, ni2, etc
        * by setting global integers d2Array, d3Array, etc.
        * Test case array='(nt,d2)' and array='(d2,nt)' should both give the
        * same arraydim.
        * If first var is nD element, arraydim will be multiplied in acq() by ni, etc
        * in section of code after call to arraytests();
        */
       if (! strcmp(var2,"d2") )
       {
          if ( ! P_getreal(CURRENT,"ni", &val, 1) )
             if (val > 1.0)
             {
                nvals2 = (int) (val+0.1);
                d2Array = 1;
             }
       }
       else if (! strcmp(var2,"d3") )
       {
          if ( ! P_getreal(CURRENT,"ni2", &val, 1) )
             if (val > 1.0)
             {
                nvals2 = (int) (val+0.1);
                d3Array = 1;
             }
       }
       else if (! strcmp(var2,"d4") )
       {
          if ( ! P_getreal(CURRENT,"ni3", &val, 1) )
             if (val > 1.0)
             {
                nvals2 = (int) (val+0.1);
                d4Array = 1;
             }
       }
       else if (! strcmp(var2,"d5") )
       {
          if ( ! P_getreal(CURRENT,"ni4", &val, 1) )
             if (val > 1.0)
             {
                nvals2 = (int) (val+0.1);
                d5Array = 1;
             }
       }
    }
    GPRINT4(1,"equalsize(): Variables: '%s', '%s' have sizes of: %d, %d\n",
		var1,var2,nvals1,nvals2);
    if (nvals1 != nvals2)
	return(ERROR);
    return(OK);
}

/*------------------------------------------------------------------
|
|	findname(name,namelist,numinlist)
|
|  	tests to see if the given name is in the name list. 
|	performs a linear search
|	returns position (0-numinlist) if found
|	returns -1 if not found.
|
| 		Author  Greg Brissey  6/04/86
+---------------------------------------------------------------------*/
static int findname(char *name, char *namelist[], int numinlist)
{
    int i;

    for ( i = 0; i < numinlist; i++)
    {
	GPRINT3(1,"findname(): name: '%s', namelist[%d]: '%s' \n",
			name,i,namelist[i]);
	if (strcmp(name,namelist[i]) == 0)
	    return(i);
    }
    return(NOTFOUND);
}
/*------------------------------------------------------------------
|
|	removename(index,namelist,numinlist)
|
|  	remove the pointer at the index position. 
|	moves the rest of the pointer to fill in the gap.
|	decrements the list count
|
| 		Author  Greg Brissey  6/04/86
+---------------------------------------------------------------------*/
static int removename(int index, char *namelist[], int *numinlist)
{
    GPRINT1(1,"removename():  variable to delete from list: '%s' \n",
			namelist[index]);
#ifdef  DEBUG
    if (Gflag > 2)
    {   int i;
	for (i=0; i < *numinlist; i++)
	    GPRINT2(3,"removename(): variables  names[%d] = '%s' \n",
			i,namelist[i]);
    }
#endif 
    if ( (index < 0) || (*numinlist < index) )
    {
	Werrprintf("removename(): Item to remove from list is out of bounds");
	return(ERROR);
    }
    if (index < *numinlist)	/* if index = numinlist just dec counter */
    {   int i,j;

    	for (i=index,j=index+1; j < *numinlist; i++,j++)
    	{
	    GPRINT2(2,"removename():  name[%d] = name[%d] \n",i,j);
	    namelist[i] = namelist[j];
    	}
    }
    *numinlist -= 1;
#ifdef  DEBUG
    if (Gflag > 2)
    {   int i;
	for (i=0; i < *numinlist; i++)
	    GPRINT2(3,"removename(): variables  names[%d] = '%s' \n",
			i,namelist[i]);
    }
#endif 
    return(OK);
}
/*------------------------------------------------------------------------------
|
|	A_getnames/4
|
|	This function loads an array of pointers to character strings with
|	pointers to the names of variables in a tree.  It sets numvar to
|	the number of variables in the tree.  
|
|    W A R N I N G ! !  
|	This routine gives the addresses of the names in the
|  	tree therefore never change the contents of what is pointed to
|  	by the pointers else YOU WILL TRASH the VARIBLE TREE..
|				Author  Greg Brissey  4/6/86
|
+-----------------------------------------------------------------------------*/

static int A_getarynames(int tree, char **nameptr, int *numvar, int maxptr) 
{   int      i;
    int	   ret;
    symbol **root;
    
    i = 0;
    *numvar = 0;
    if ( (root = getTreeRoot(getRoot(tree))) )
    {	ret = getNames(*root,nameptr,numvar,maxptr,&i);
	return(ret);
    }
    else
	return(NOTREE);  /* tree doesn't exist (-1) */
}

/*----------------------------------------------------------------------------
|
|	getNames/5
|
|	This modules recursively travels down a tree, sets an array of
|	pointer to the variables name strings and keeps count of them.
|	This is only done for arrayed variable in the acquisition group!
|				Author  Greg Brissey  4/6/86
+-----------------------------------------------------------------------------*/

static int getNames(symbol *s, char **nameptr, int *numvar, int maxptr, int *i) 
{   varInfo *v;

    if (s)
    {	getNames(s->left,nameptr,numvar,maxptr,i);
        if (*i < maxptr)
	{   
	    v = (varInfo *)s->val;
	    /* only arrayed & acquisition group variables */
	    if ( (v->T.size > 1) && (v->Ggroup == G_ACQUISITION) )
	    {
	    	nameptr[*i] = s->name;
	    	*i += 1;  
		*numvar += 1;
	    }
	    GPRINT3(3,"getNames():  name: '%s', i = %d, number: %d \n",
			s->name,*i,*numvar);
	}
	else
	{   *numvar = -1;  /* mark error */
	    return(ERROR);
	}
     	getNames(s->right,nameptr,numvar,maxptr,i);
	return(OK);
    }
    return(OK);
}

/*----------------------------------------------------------------------------
|	autoname(argc,argv,retc,retv)
|	This module constructs a file name based on autoname parameter
|	and sampleinfo file
+---------------------------------------------------------------------------*/
int autoname(int argc, char **argv, int retc, char **retv)
{
char	a_name[MAXPATH];
char	sif_name[MAXPATH];
char	result[MAXPATH];
char*	suffix = ".fid"; 
char*	notsuffix;

int	bad;
int	replaceSpaceFlag = TRUE;
#ifdef __INTERIX
	replaceSpaceFlag = FALSE;
#endif

   if (strcasecmp(argv[0],"svsname")==0) {
       suffix = "";
   }
   if (strcmp(argv[0],"autoname")==0)
   {
      if (getparm("autodir","string",GLOBAL,autodir,MAXPATH))
      {  Werrprintf("cannot find parameter: autodir");
         return(1);
      }
      if (strcmp(autodir,"")==0)
         sprintf(autodir,"%s/data",userdir);
      /*  file is "sampleinfo file" */
      strcpy(sif_name,curexpdir);
      strcat(sif_name,"/sampleinfo");
   }
   else /* if (strcmp(argv[0],"Svfname")==0) */
   {
      strcpy(sif_name,"");
   }

   strcpy(result,"");
   notsuffix = suffix;

   switch (argc)
   {
      case 1:
            if (strcmp(argv[0],"autoname")==0)
            {
	       /* get autoname parameter */
               if (P_getstring(GLOBAL,"autoname",a_name,1,MAXPATH) < 0)
	          strcpy(a_name,"%SAMPLE#:%%PEAK#:%");
               if (a_name[0] == '\000')
	          strcpy(a_name,"%SAMPLE#:%%PEAK#:%");
            }
            else /* if (strcmp(argv[0],"Svfname")==0) */
            {
	       /* get svfname parameter */
               if (P_getstring(GLOBAL,"svfname",a_name,1,MAXPATH) < 0)
	          strcpy(a_name,"$seqfil$-");
               if (a_name[0] == '\000')
	          strcpy(a_name,"$seqfil$-");
            }
            break;
      case 2:
            /* use argv[1] for parameter name */
            strcpy(a_name,argv[1]);
            break;
      case 5:
	    if (strcmp(argv[4],"keepspaces") == 0)
		replaceSpaceFlag = FALSE;
	    else if (strcmp(argv[4],"replacespaces") == 0)
		replaceSpaceFlag = TRUE;
      case 4:
	    if (strcmp(argv[3],"keepspaces") == 0)
		replaceSpaceFlag = FALSE;
	    else if (strcmp(argv[3],"replacespaces") == 0)
		replaceSpaceFlag = TRUE;
	    else
		notsuffix = argv[3];
      case 3:
            /* use argv[1] for parameter name */
	    strcpy(a_name,argv[1]);
            /* use argv[2] as sample name */
            if (strcmp(argv[0],"autoname")==0)
               strcpy(sif_name,argv[2]);
            else
               suffix = argv[2];
            break;
      default:
            if (strcmp(argv[0],"autoname")==0)
	       Winfoprintf("Usage: %s<(parametername<,filename<,not suffixes><,'keepspaces'|'replacespaces'>>)><:$path>",argv[0]);
            else
	       Winfoprintf("Usage: %s<(parametername<,suffix<,not suffixes><,'keepspaces'|'replacespaces'>>)><:$path>",argv[0]);
            return(1);
            break;
   }
   bad = makeautoname(argv[0],a_name,sif_name,result,FALSE,replaceSpaceFlag,suffix,notsuffix);
   if ( bad)
   {  
      if (retc)
         retv[0]=newString("");
      else
	 Winfoprintf("Unable to construct autoname");
      RETURN;
   }
   if (result[0] != '/')
   {
      if (strcmp(argv[0],"autoname")==0)
         strcpy(a_name,autodir);
      else
         sprintf(a_name,"%s/data",userdir);
      strcat(a_name,"/");
      strcat(a_name,result);
      strcpy(result,a_name);
   }
   
   switch(retc)
   {
      case 0:
	    Winfoprintf("%s%s",result,suffix);
	    break;
      case 2:
	    retv[1] = newString((strrchr(result,'/')+1));
      case 1:
            strcat(result, suffix);
	    retv[0] = newString(result);
            break;
      default:
            if (strcmp(argv[0],"autoname")==0)
	       Winfoprintf("Usage: %s<(parametername<,filename<,not suffixes><,'keepspaces'|'replacespaces'>>)><:$path>",argv[0]);
            else
	       Winfoprintf("Usage: %s<(parametername<,suffix<,not suffixes><,'keepspaces'|'replacespaces'>>)><:$path>",argv[0]);
	    return(1);
            break;
   }
   return(0);
}

/*----------------------------------------------------------------------------
|
|	makeautoname/8
|
|	This module constructs a file name based on location number
+-----------------------------------------------------------------------------*/
int makeautoname(char *cmdname, char *a_name, char *sif_name, char *dirname,
                 int createflag, int replaceSpaceFlag,
                 char *suffix, char *notsuffix)
{
char	*ptr;
char	*aptr;
char	*sptr;
char	filea[MAXPATH];
char	fileb[MAXPATH];
char	search_str[MAXSTR];
char	tmpStr[MAXSTR];
char	tmp[MAXPATH];
char	tmpSuffix[MAXPATH];
char	tmp2Str[MAXSTR];
char	revision[20];
double	rval;
mode_t	filemode;
int	itemp;
int	dlen,len;
int	R_start,R_width,rev;
int     R_offset,R_offsetTmp;
int     R_any = 0;
int	tree;
vInfo	info;
FILE	*fd;
char    recDir[MAXPATH];

    strcpy(recDir, dirname);
    fd = NULL;

/* try to get time variable */

   strcpy(tmp,"");
   currentDateLocal(tmpStr, MAXPATH);

/* init pointers */

   ptr=dirname;
   aptr = a_name;

   R_offset = -1;
   R_start=1; R_width=2;
   dlen=MAXPATH;

/*---------------------------------------------------------
|  check autoname for %string%
|  search for this string (withouth %) in sampleinfo
|  then concatenate next word in sampleinfo into dirname
+--------------------------------------------------------*/

   while ( (*aptr != '\000')  && (dlen > 0) )
   {  
      if (*aptr == '%')
      {  aptr++;			/* Skip %-sign */
	 sptr = search_str;
         len=0;
         while ( (*aptr != '%') && (*aptr != '\000') && (len < MAXSTR) )
         {  *sptr++ = *aptr++;
            len++;
         }
         *sptr='\000';	
         aptr++;			/* Skip %-sign */
         if ( (search_str[0] == 'R') &&
             ((search_str[1] >= '0') && (search_str[1] <= '9'   )) &&
             ((search_str[2] == ':') || (search_str[2] == '\000')) )
         {
            if (! R_any)
            {
               R_any = 1;  /* Any %Rn% definition suppresses the automatic appending of one */
               R_width = 0;
            }
            if (search_str[1] != '0')
            {
               if (R_offset != -1)
               {
                  Werrprintf("%s: Only one Rn definition allowed",cmdname);
                  ABORT;
               }
               /* offset into string where indexing should be placed */
               R_offset = ptr - dirname;
               R_width = search_str[1] - '0';
               len = 0;
               /* Leave space in the string for the indexing */
               while (len < R_width)
               {
                  *ptr++ = '0';
                  len++;
               }
               if (search_str[2] == ':')
                  R_start = atoi(&search_str[3]);
               if (R_start == 0)
                  R_start = 1;	/* %R3:% would give zero, make at least 1 */
            }
         }
         else
         {
            itemp = 1;
            if (strcmp(tmpStr,"") != 0)
               itemp = dateStr(search_str,tmpStr,&ptr,&dlen); /* %DATE%%TIME% */
            if ((strcmp(cmdname,"autoname")==0) && (itemp==1))
            {
               if (fd == NULL)
               {
                  fd=fopen(sif_name,"r");
                  if (fd == NULL)
                  {  Werrprintf("autoname: cannot open %s",sif_name);
                     return(-1);
                  }
               }
               getStr(search_str,fd,&ptr,&dlen);
            }
            *ptr='\000';
         }
      }
      else if (*aptr == '$')
      {  aptr++;                        /* Skip $-sign */
         sptr = search_str;
         len=0;
         while ( (*aptr != '$') && (*aptr != '\000') && (len < MAXSTR) )
         {  *sptr++ = *aptr++;
            len++;
         }
         *sptr='\000';
         aptr++;                        /* Skip $-sign */
         tree = CURRENT;
	 if (P_getVarInfo(tree,search_str,&info) < 0)
         {  tree = GLOBAL;
            if (P_getVarInfo(tree,search_str,&info) < 0)
            {  tree = SYSTEMGLOBAL;
               if (P_getVarInfo(tree,search_str,&info) < 0)
               {  Werrprintf("Cannot find $%s$",search_str);
                  ABORT;
               }
            }
         }
	 if (info.basicType == T_REAL)
         {  P_getreal(tree,search_str,&rval,1);
	    sprintf(tmp2Str,"%d",(int)(rval+0.1));
	 }
	 else if (info.basicType == T_STRING)
         {  P_getstring(tree, search_str,tmp2Str,1,MAXSTR-1);
         }

         sptr = tmp2Str;
         while(*sptr != '\000')
         {
            if ((*sptr == ' ') && (replaceSpaceFlag == TRUE))
		*sptr = '_';
            *ptr++ = *sptr++;
         }
      }

/* any other text is copied (like '-', '.', etc) */

      else
      {
         if ((*aptr == ' ') && (replaceSpaceFlag == TRUE))
         {
            *ptr++ = '_';
            aptr++;
	         dlen--;
         }
         else if ((*aptr == ' ') && (replaceSpaceFlag == FALSE))
         {
            *ptr++ = *aptr++;
	         dlen--;
         }
         else if (verify_fnameChar( *aptr )==0)
         {
            *ptr++ = *aptr++;
	         dlen--;
         }
         else
         {
            Werrprintf("illegal character '%c' in %s",*aptr,cmdname);
            ABORT;
         }
      }
   }
   *ptr='\000';
   if (fd != NULL)
      fclose(fd);

/*---------------------------------------------------------
|  does dirname start with a '/'? If not pre-pend autodir 
+--------------------------------------------------------*/

   if ( dirname[0] !=  '/' )
   {
      if (strcmp(cmdname,"autoname")==0) 
         strcpy(tmp,autodir);
      else if(strstr(suffix,".REC") != NULL ||
	      strstr(suffix,".rec") != NULL ) strcpy(tmp,recDir);
      else sprintf(tmp,"%s/data",userdir);

      if(tmp[strlen(tmp)-1] != '/') strcat(tmp,"/");
      if (R_offset != -1)
      {
         R_offsetTmp = R_offset + strlen(tmp);
      }
      else
      {
         /* default indexing */
         R_offset = strlen(dirname);
         len = 0;
         while (len < R_width)
         {
            *ptr++ = '0';
            len++;
         }
         *ptr='\000';
         R_offsetTmp = R_offset + strlen(tmp);
      }
      strcat(tmp,dirname);
   }
   else
   {
      if (R_offset != -1)
      {
         R_offsetTmp = R_offset;
      }
      else
      {
         /* default indexing */
         R_offsetTmp = R_offset = strlen(dirname);
         len = 0;
         while (len < R_width)
         {
            *ptr++ = '0';
            len++;
         }
         *ptr='\000';
      }
      strcpy(tmp,dirname);
   }

/* append a run sequence, in case the fid-filename exists */
   if (R_width != 0)
   {
      char tmp2[MAXPATH],*ptr,*ptr1;
      int  permission = R_OK|W_OK|X_OK;
      int file_exists;
/* Check if the fid-filename exists, if so increase the %R count */
      rev = R_start;
      if (strcmp(suffix,".fid"))
         permission = F_OK;
      do
      {
         file_exists = FALSE;
         sprintf(revision,"%0*d",R_width,rev);
         len = strlen(revision);
         if (len > R_width)
         {
            int i;

            /* Revision gained an extra digit */
            /* Need to make room in the string */
            strncpy(tmp2,tmp,R_offsetTmp);
            i = 0;
            while (i < len)
            {
               *(tmp2+R_offsetTmp+i) = '0';
               i++;
            }
            *(tmp2+R_offsetTmp+i) = '\0';
            strcat(tmp2,(tmp+R_offsetTmp+R_width));
            strcpy(tmp,tmp2);

            strncpy(tmp2,dirname,R_offset);
            i = 0;
            while (i < len)
            {
               *(tmp2+R_offset+i) = '0';
               i++;
            }
            *(tmp2+R_offset+i) = '\0';
            strcat(tmp2,(dirname+R_offset+R_width));
            strcpy(dirname,tmp2);
            R_width = len;
         }
         strcpy(tmp2,tmp);
         /* overwrite the embedded zeros with the revision number */
         strncpy(tmp2+R_offsetTmp,revision,R_width);
         strcat(tmp2,suffix);
         if ( access(tmp2,permission) )
         {
	    /* check 'notsuffix'. This is a string with undesirable sufffixes
	     * separated by commas. We do not use srttok, it skips ,,
	     * thereby not allowing empty suffix. Any spaces (leading, 
	     * trailing or in the middle) are deleted.
             */
            ptr = notsuffix;
            while (*ptr != '\0') 
            {
               ptr1 = tmpSuffix;
               while ( (*ptr != '\0') && (*ptr != ',') )
               {  
                  if ( *ptr == ' ') 
                     ptr++;
                  else
                     *(ptr1++) = *(ptr++);
               }
               if (*ptr != '\0') ptr++;
               *ptr1 = '\0';
              
               strcpy(tmp2,tmp);
               strncpy(tmp2+R_offsetTmp,revision,R_width);
               strcat(tmp2,tmpSuffix);
               if (!access(tmp2,F_OK))
	       {
		  file_exists=TRUE;
		  break;
               }
            }
         }
         else
         {
	    file_exists = TRUE;
         }
         rev++;
      } while ( file_exists );

      /* overwrite the embedded zeros with the revision number */
      strncpy(tmp+R_offsetTmp,revision,R_width);
      strncpy(dirname+R_offset,revision,R_width);
   }
   if (replaceSpaceFlag == TRUE)
      replaceSpace(dirname);
   if (((replaceSpaceFlag == TRUE) && verify_fname(dirname)) ||
       ((replaceSpaceFlag == FALSE) && verify_fname2(dirname)))
   {  Werrprintf( "file path '%s%s' not valid", dirname, suffix );
      ABORT;
   }

   if ( ! createflag) RETURN;

/* Finally create the new fid-file */
#ifdef SIS
    filemode = 0755;
#else 
    filemode = 0777;
#endif 

   strcat(tmp,suffix);
   itemp = do_mkdir(tmp, 1, filemode);
   if (itemp)
   {  Werrprintf("Cannot create directory '%s',return=%d\n",tmp,itemp);
      ABORT;
   }

   sprintf(filea,"%s/sampleinfo",curexpdir);
   sprintf(fileb,"%s/sampleinfo",tmp);
   copyFile(filea,fileb,0);

   RETURN;
}

void replaceSpace(char *name)
{
  int i=0;
  while ((name[i] != '\000') && (i<1000))
  {
    if (name[i] == ' ') name[i] = '_';
    i++;
  }
}

static int dateStr(char *find, char *dt, char **dptr, int *maxed)
/* add specifiers %DATE%%TIME% %YR%%MO%%DAY% %HR%%MIN%%SEC% %YR2%%MOC%%DAC%%HR12%%PM% */
{
	int i, start=0, len=0;
	if ((find[0]=='S') && (find[1]=='V') && (find[2]=='F') &&
	    (find[3]=='D') && (find[4]=='A') && (find[5]=='T') && (find[6]=='E'))
	{
	  char tmp2[MAXSTR];
/*	  if (P_getstring(CURRENT, "time_svfname", tmp2, 1, MAXSTR) == 0)
	    P_deleteVar(CURRENT, "time_svfname");
	  else */
	    currentDateSvf(tmp2, MAXPATH);
	  len = strlen(tmp2);
	  if (len > 0)
	  {
	    for (i=0; (i<len) && (*maxed>0); i++)
	    {
	      **dptr = tmp2[i];
	      (*dptr)++;
	      (*maxed)--;
	    }
	    return(0);
	  }
	}
	else if ((find[0]=='D') && (find[1]=='A') && (find[2]=='T') && (find[3]=='E'))
	{
	  start = 0; len = 8;
	}
	else if ((find[0]=='T') && (find[1]=='I') && (find[2]=='M') && (find[3]=='E'))
	{
	  start = 9; len = 6;
	}
	else if ((find[0]=='Y') && (find[1]=='R') && (find[2]=='2'))
	{
	  start = 2; len = 2;
	}
	else if ((find[0]=='Y') && (find[1]=='R'))
	{
	  start = 0; len = 4;
	}
	else if ((find[0]=='M') && (find[1]=='O') && (find[2]=='C'))
	{
/* if full month name, could continue to end of string */
	  start = 23; len = 3;
	}
	else if ((find[0]=='M') && (find[1]=='O'))
	{
	  start = 4; len = 2;
	}
	else if ((find[0]=='D') && (find[1]=='A') && (find[2]=='C'))
	{
/* if full day-of-week name, could continue to character 'y' */
	  start = 20; len = 3;
	}
	else if ((find[0]=='D') && (find[1]=='A') && (find[2]=='Y'))
	{
	  start = 6; len = 2;
	}
	else if ((find[0]=='H') && (find[1]=='R') && (find[2]=='1') && (find[3]=='2'))
	{
	  start = 16; len = 2;
	}
	else if ((find[0]=='H') && (find[1]=='R'))
	{
	  start = 9; len = 2;
	}
	else if ((find[0]=='P') && (find[1]=='M'))
	{
	  start = 18; len = 2;
	}
	else if ((find[0]=='M') && (find[1]=='I') && (find[2]=='N'))
	{
	  start = 11; len = 2;
	}
	else if ((find[0]=='S') && (find[1]=='E') && (find[2]=='C'))
	{
	  start = 13; len = 2;
	}
	if (len > 0)
	{
	  for (i=0; (i<len) && (*maxed>0); i++)
	  {
	    **dptr = dt[i+start];
	    (*dptr)++;
	    (*maxed)--;
	  }
          **dptr = '\0';
	  return(0);
	}
	return(1);
}

static void getStr(char *find, FILE *fd, char **dptr, int *maxed)
{
char	buffer[1000];
char	*ptr1;
char	*result=(char *)1;
int	isamp,done = 0;
   rewind(fd);

   ptr1 = NULL;
   while ( result && !done)
   {  
      result = fgets(buffer,300,fd); /* reads n or to/incl end-of-line */
      if ( (ptr1 = strstr(buffer,find)) ) done=1;
   }
   if (done)
   {  
      ptr1 += strlen(find);
      while ( (*ptr1 == ' ') || (*ptr1 == '\t') ) 
      {  
          ptr1++;
      }
      if (!strcmp(find,"SAMPLE#:") || !strcmp(find,"PEAK#:"))
      {   isamp = strlen(ptr1);
          if ( verify_fnameChar(*(ptr1+1)) )
             if ( (*ptr1 >= '0') && (*ptr1 <= '9') )
	     {  isamp = *ptr1; /* save char */
                *ptr1 = '0';
		*(ptr1+1) = isamp;
		*(ptr1+2) = ' ';
             }
      }
      while ( (verify_fnameChar(*ptr1) == 0) && (*maxed > 0) )
      { **dptr = *ptr1;
        (*dptr)++;
        ptr1++;
        (*maxed)--;
      }
   }
}

/* Return value is length of parameter name. Return 0 if not a parameter */
/* parName is returned parameter name. parValue is returned parameter value */
static int isParFeature(char *ptr, char *parName, char *parValue, int len)
{
   char *sptr;
   int  slen = 1;
   symbol **root;
   varInfo *v;

   sptr = parName;
   slen=0;
   strcpy(parValue,"");
   if (*ptr == '$')
   {
      *sptr++ = *ptr++;
      slen++;
   }
   while ( (*ptr != '$') && (*ptr != '\000') && (slen < len) )
   {  *sptr++ = *ptr++;
      slen++;
   }
   *sptr='\000';
   v = NULL;
   if (*parName == '$') /* Temporary $par */
   {
      if ( ( root = selectVarTree(parName) ) )
         v = rfindVar(parName,root);
   }
   else
   {
      root = getTreeRootByIndex(CURRENT);
      if ( (v = rfindVar(parName,root)) == NULL)
      {
         root = getTreeRootByIndex(GLOBAL);
         if ( (v = rfindVar(parName,root)) == NULL)
         {
            root = getTreeRootByIndex(SYSTEMGLOBAL);
            if ( (v = rfindVar(parName,root)) == NULL)
            {
               return(0);
            }
         }
      }
   }
   if (v == NULL)
   {
      return(0);
   }
   if (v->T.basicType == T_REAL)
   { 
      Rval *r;
      r = v->R;
      sprintf(parValue,"%d",(int)(r->v.r+0.1));
   }
   else if (v->T.basicType == T_STRING)
   {
      Rval *r;
      r = v->R;
      strcpy(parValue,r->v.s);
   }
   return(slen);
}

static int isTemplateFeature(char *ptr, char *dateChar, char *val, int len)
{
   char *tmpPtr;
   char tmp[MAXSTR];
   int  tlen = MAXSTR;
   
   tmpPtr = tmp;
   if ( ! strncmp(ptr,"DATE%",5) || ! strncmp(ptr,"TIME%",5) || ! strncmp(ptr,"HR12%",5) )
   {
      dateStr(ptr, dateChar, &tmpPtr, &tlen);
      strcpy(val,tmp);
      return(4);
   }
   if ( ! strncmp(ptr,"DAY%",4) || ! strncmp(ptr,"MIN%",4) || ! strncmp(ptr,"SEC%",4) ||
        ! strncmp(ptr,"DAC%",4) || ! strncmp(ptr,"MOC%",4) || ! strncmp(ptr,"YR2%",4) )
   {
      dateStr(ptr, dateChar, &tmpPtr, &tlen);
      strcpy(val,tmp);
      return(3);
   }
   if ( ! strncmp(ptr,"YR%",3) || ! strncmp(ptr,"MO%",3) || ! strncmp(ptr,"HR%",3) ||
        ! strncmp(ptr,"PM%",3) )
   {
      dateStr(ptr, dateChar, &tmpPtr, &tlen);
      strcpy(val,tmp);
      return(2);
   }
   if ( (*ptr == 'R') && ( *(ptr+1) >= '0') && ( *(ptr+1) <= '9' ) )
   {
      if ( *(ptr+2) == '%' )
         return(-2);
      if ( *(ptr+2) == ':' )
      {
         if (  ( *(ptr+3) >= '1') && ( *(ptr+3) <= '9' ) )
         {
            if ( *(ptr+4) == '%' )
               return(-4);
            if (  ( *(ptr+4) >= '0') && ( *(ptr+4) <= '9' ) )
            {
               if ( *(ptr+5) == '%' )
                  return(-5);
               if (  ( *(ptr+5) >= '0') && ( *(ptr+5) <= '9' ) )
                  if ( *(ptr+6) == '%' )
                     return(-6);
            }
         }
      }
   }
   return(0);
}

static int checkChar( int ptr, int charType,
		      char *defChars, int replaceChar, int *subst )
{
   int i;

   *subst=0;
   switch (charType)
   {
      case 1:   {
                   int numChars;
                   if (isalnum(ptr))
                      return(ptr);
                   numChars = strlen(defChars);
                   for (i=0; i < numChars; i++)
                   {
                      if (ptr == *(defChars+i) )
                         return(ptr);
                   }
                   *subst=1;
                   return(replaceChar);
                }
                break;
      case 2:   return(ptr);
                break;
      case 3:   {
                   int numChars = strlen(defChars);
                   for (i=0; i < numChars; i++)
                   {
                      if (ptr == *(defChars+i) )
		      {
                         *subst=1;
                         return(replaceChar);
		      }
                   }

                }
                break;
      default:  return(0);
                break;
   }
   return(ptr);
}

/*------------------------------------------------------------------------------|
|       chkname(template, 'chars','keyword','replacementChar')
|
|       chkname does "% pair" and "$ pair" substitutions on the template.
|       It also will check the resulting string for allowed or disallowed characters.
|       The third keyword argument (tmpl, str, par) controls have the $ and %
|       characters are interpreted. The fourth argument is the replacement
|       character for disallowed characters (default _)
+-----------------------------------------------------------------------------*/

#define MAXCHKNAME 512
int chkname(int argc, char *argv[], int retc, char *retv[])
{
   char *ptr;
   char parlist[MAXCHKNAME];
   char reqlist[MAXCHKNAME];
   char parstr[MAXCHKNAME];
   char reqstr[MAXCHKNAME];
   char search_str[MAXCHKNAME];
   char tmp2Str[MAXCHKNAME];
   char dateChar[MAXSTR];
   static char fileChars[MAXSTR] = "_.-";
   static char extraChars[MAXSTR];
   char *defChars = NULL;
   int  charType = 1;
   char replaceChar = '_';
   int  replaceTemplate = 1;
   int  replacePair = 0;     /* This selects default of par for third argument */
   int  previousReplace = 0;
   char *pptr;
   char *rptr;
   char *sptr;
   int plen;
   int rlen;
   int  slen;

   if (argc == 1)
   {
      Werrprintf("Usage: %s(template,'characters','par tmpl or str','replacement character')",
                      argv[ 0 ]);
      ABORT;
   }
   if ( ! strcmp(argv[1],"fileChars") )
   {
      if (retc)
         retv[0] = newString(fileChars);
      if (argc == 3)
         strcpy(fileChars,argv[2]);
      RETURN;
   }
   strcpy(dateChar,"");
   ptr = argv[1];
      charType = 1;
      if (argc > 2)
      {
         if ( ! strcmp(argv[2],"file") )
         {
            charType = 1;
            defChars = fileChars;
         }
         else if ( ! strcmp(argv[2],"dir") )
         {
            charType = 1;
            strcpy(extraChars,fileChars);
            strcat(extraChars,"/");
            defChars = extraChars;
         }
         else if ( ! strncmp(argv[2],"alnum",5) )
         {
            charType = 1;
            if (strlen(argv[2]) > 5)
            {
               defChars = argv[2];
               strcpy(extraChars,defChars+5);
            }
            else
            {
               strcpy(extraChars,"");
            }
            defChars = extraChars;
         }
         else if ( ! strcmp(argv[2],"none") )
         {
            charType = 2;
         }
         else
         {
            charType = 3;
            defChars = argv[2];
         }
         if (argc > 3)
         {
            replaceTemplate = (argv[3][0] == 't') || (argv[3][0] == 'p');
            replacePair = (argv[3][0] == 't');
            if (argc > 4)
            {
                replaceChar = argv[4][0];
            }
         }
      }
      else
      {
         /* default is "dir" case */
         strcpy(extraChars,fileChars);
         strcat(extraChars,"/");
         defChars = extraChars;
      }

      if (replaceTemplate)
         currentDateLocal(dateChar, MAXPATH);
      strcpy(parlist,"");
      strcpy(reqlist,"");
      pptr = parstr;
      rptr = reqstr;
      plen = rlen = 1;  /* Set to 1 to save room for EOL */
      while (*ptr != '\000')
      {
         if ( replaceTemplate &&
              ( ((*ptr == '$') && strstr(ptr+2,"$")) ||
                ((*ptr == '$') && (*(ptr+1) == '$') &&  strstr(ptr+3,"$")) )  &&
              (slen = isParFeature(ptr+1, search_str, tmp2Str, MAXSTR)) )
         {
            ptr += slen + 2;  /* Skip two $-signs and length of parameter name */
  
            if (strlen(parlist))
               strcat(parlist," ");
            strcat(parlist,search_str);
            if ( ! strlen(tmp2Str) )
            {
               int rchar;
               if (strlen(reqlist))
                  strcat(reqlist," ");
               strcat(reqlist,search_str);
               if (replacePair)
                  rchar = '#';
               else
                  rchar = '$';
               sptr = search_str;
               if (rlen < MAXCHKNAME)
                  *rptr++ = rchar;
               rlen++;
               while(*sptr != '\000')
               {
                  if (rlen < MAXCHKNAME)
                     *rptr++ = *sptr;
                  sptr++;
                  rlen++;
               }
               if (rlen < MAXCHKNAME)
                  *rptr++ = rchar;
               rlen++;
               *rptr='\000';
            }
            else
            {
               sptr = tmp2Str;
               while(*sptr != '\000')
               {
                  int newChar;
		  int subst;

                  newChar = checkChar( *sptr, charType, defChars,
				       replaceChar, &subst );
                  if ( (plen < MAXCHKNAME) && newChar &&
                       ( (plen == 1) || (*(pptr-1) != newChar) ||
		         (newChar != replaceChar) ||
		         ( ! subst ) ||
		         ( ! previousReplace) ) )
                  {
                     *pptr++ = newChar;
                     plen++;
                     *pptr='\000';
                  }
	          previousReplace = subst;
                  if ( replacePair && (rlen < MAXCHKNAME) && newChar &&
                       ( (rlen == 1) || (*(rptr-1) != newChar) || (newChar != replaceChar) ) )
                  {
                     *rptr++ = newChar;
                     rlen++;
                  }
                  sptr++;
               }
               if ( ! replacePair)
               {
                  sptr = search_str;
                  if (rlen < MAXCHKNAME)
                     *rptr++ = '$';
                  rlen++;
                  while(*sptr != '\000')
                  {
                     if (rlen < MAXCHKNAME)
                        *rptr++ = *sptr;
                     sptr++;
                     rlen++;
                  }
                  if (rlen < MAXCHKNAME)
                     *rptr++ = '$';
                  rlen++;
                  *rptr='\000';
               }
            }
         }
         else if ( replaceTemplate && (*ptr == '%') &&
                   (slen = isTemplateFeature(ptr+1,dateChar,tmp2Str,MAXSTR)) )
         {
            if (slen > 0)  /* Not the %Rn% case */
            {
               sptr = tmp2Str;
               while(*sptr != '\000')
               {
                  int newChar;
		  int subst;

                  newChar = checkChar( *sptr, charType, defChars,
				       replaceChar, &subst );
                  if ( (plen < MAXCHKNAME) && newChar &&
                       ( (plen == 1) || (*(pptr-1) != newChar) ||
		         (newChar != replaceChar) ||
		         ( ! subst ) ||
		         ( ! previousReplace) ) )
                  {
                     *pptr++ = newChar;
                     plen++;
                     *pptr='\000';
                  }
	          previousReplace = subst;
                  if ( replacePair && (rlen < MAXCHKNAME) && newChar &&
                       ( (rlen == 1) || (*(rptr-1) != newChar) || (newChar != replaceChar) ) )
                  {
                     *rptr++ = newChar;
                     rlen++;
                  }
                  sptr++;
               }
               if ( ! replacePair )
               {
                  int tmp;

                  for (tmp=0; tmp < slen+2; tmp++)
                  {
                     if (rlen < MAXCHKNAME)
                        *rptr++ = *(ptr+tmp);
                     rlen++;
                  }
                  *rptr='\000';
               }
               ptr += slen + 2;  /* Skip two %-signs and length of keyword name */
            }
            else /* The %Rn% case; slen is negative */
            {
               int tmp;

               for (tmp=0; tmp < 2-slen; tmp++)
               {
                  if (plen < MAXCHKNAME)
                     *pptr++ = *(ptr+tmp);
                  plen++;
                  if (rlen < MAXCHKNAME)
                     *rptr++ = *(ptr+tmp);
                  rlen++;
               }
               *rptr='\000';
               *pptr='\000';
               ptr += 2 - slen;  /* Skip two %-signs and length of keyword name */
            }
         }
         else
         {
            int newChar;
	    int subst;

            newChar = checkChar( *ptr, charType, defChars,
				 replaceChar, &subst );
            if ( (plen < MAXCHKNAME) && newChar &&
                 ( (plen == 1) || (*(pptr-1) != newChar) ||
		   (newChar != replaceChar) ||
		   ( ! subst ) ||
		   ( ! previousReplace) ) )
            {
               *pptr++ = newChar;
               plen++;
               *pptr='\000';
            }
	    previousReplace = subst;
            if ( (rlen < MAXCHKNAME) && newChar &&
                 ( (rlen == 1) || (*(rptr-1) != newChar) || (newChar != replaceChar) ) )
            {
               *rptr++ = newChar;
               rlen++;
            }
            ptr++;
         }
      }
      *pptr='\000';
      *rptr='\000';
   if (retc < 1)
      Winfoprintf( "%s substituted template: %s",argv[0],parstr );
   else
   {
      retv[0] = newString(parstr);
      if (retc > 1)
      {
         retv[1] = newString(reqstr);
         if (retc > 2)
         {
            retv[2] = newString(parlist);
            if (retc > 3)
            {
               retv[3] = newString(reqlist);
            }
         }
      }
   }
   RETURN;
}

/* Save au arguments for use by the au command when called from automation
 * If called with arguments, it saves the arguments. If called without arguments
 * it removed pre-exixting argments (save as no arguments)
 */
int auargs(int argc, char *argv[], int retc, char *retv[])
{
   char path[MAXPATH];
   char autodir[MAXPATH];

   if (mode_of_vnmr != AUTOMATION)
      RETURN;
   if (getparm("autodir","string",GLOBAL,autodir,MAXPATH))
      RETURN;
   strcpy(path,autodir);
   strcat(path,"/auargs");
   if (argc == 1)
   {
      unlink(path);
   }
   else
   {
      FILE *fd;
      int index = 1;

      fd = fopen(path,"a");
      while (index < argc)
      {
         fprintf(fd,"%s\n",argv[index]);
         index++;
      }
      fclose(fd);
   }
   RETURN;
}


#ifdef  DEBUG
/***********************************/
static int Wtimeprintf(int mode, char *control, char *time)
/***********************************/
{
  struct tm      *tmtime;
  struct timeval  clock;
  time_t          timedate;
  char           *chrptr;
  char            datetim[26];
  static time_t   savetime = 0;

  gettimeofday(&clock, NULL);
  timedate = clock.tv_sec;
  if (mode == 0)
    savetime = timedate;
  else if (mode < 0)
  {
    timedate -= savetime;
    sprintf(datetim,"%ld",timedate);
  }
  else
  {
    tmtime = localtime(&(timedate));
    chrptr = asctime(tmtime);
    strcpy(datetim,chrptr);
    datetim[24] = '\0';
    if (time)
      strcpy(time,datetim);
  }
  if (control && mode)
    Wscrprintf(control,datetim);
  return(timedate);
}
#endif 

int
is_psg4acqi()
{
	return( 0 );	/* if called from VNMR, the PSG program is NOT being run for ACQI */
}

static int fireUpJPSG()
{
/*
    int result;
 */
    int trys = 30;  /* 30 seconds for Java VM to up and running Jpsg */

    /* printf("Reading Jpsg Port # ---  "); */
    if ( ! isJpsgReady() )
    {
        char cmd[256];
        int RevID = 5;

       /*
        sprintf(cmd,"/usr25/greg/projects/NewPSG/java/code/jpsg/Jpsg %d %d &",
                RevID,HostPid);
        */
        sprintf(cmd,"%s/jpsg/Jpsg %d %ld &",
                systemdir,RevID,(long) HostPid);
        /* printf("fireUpJPSG: No file, Starting Jpsg\n"); */
        /* system("/vnmr/jpsg/Jpsg 5 8195 1 &"); */
        /*printf("fireUpJPSG: system('%s')\n",cmd); */

        /* result = system(cmd); */

        child = fork(); 

        if (child < 0)
        {   Werrprintf("GO: could not create a Java PSG process!");
            /* if (!acqi_fid) */
              release_console();
	    ABORT;
        }

        psg_pid = child;  /* set global psg pid value */


        if (child)	/* if parent set signal handler to reap process */
            set_wait_child(child);

        if (child == 0)
        {	
           char cmd[256];
           char RevIdStr[256];
           char pidstr[256];
           char Arg1[256];
           char Arg2[256];
           mode_t oldMask;

           sprintf(cmd,"%s/jpsg/Jpsg",systemdir);
           oldMask = umask(0);
           sprintf(Arg1,"-Duser.umask=%d",oldMask);
           umask(oldMask);
           sprintf(Arg2,"-Duser.hostname=%s",HostName);
           sprintf(RevIdStr,"%d",RevID);
           sprintf(pidstr,"%ld", (long) HostPid);
           /* set_effective_user(); */
    
            /* ret = execlp(cmd,"Jpsg",RevIdStr,pidstr,NULL); */
            /* execlp("/vnmr/jpsg/Jpsg","Jpsg",RevIdStr,pidstr,NULL); */
            if ( execlp("java","java",Arg1,Arg2,"Jpsg",RevIdStr,pidstr,NULL) )
	       Werrprintf("JPSG could not execute");
        }


	/*
        if (result == -1)
        {
	    perror("fireUpJPSG: system(): ");
	    ABORT;
        }
	*/
        sleep(1);
        while(trys)
        {
           if (isJpsgReady())
              break;
           trys--;
           sleep(1);
        }

        return(0);
    }
    else
    {
        /* printf("fireUpJPSG: file found\n"); */
        return(0);
    }
}    

int isJpsgReady()
{
    FILE *sd;
    int numconv;
    char portstr[50];
    char pidstr[50];
    char hostname[50];
    char readymsge[50];
    char jpsgPIDFileName[MAXSTR];
    int isReady = 0;
    int alive,pid;

    strcpy(jpsgPIDFileName,"/vnmr/acqqueue/jinfo1.");
    strcat(jpsgPIDFileName,UserName);

    if (access(jpsgPIDFileName,R_OK) == 0)
    {
       sd = fopen(jpsgPIDFileName,"r");
       numconv = fscanf(sd,"%s %s %s %s",portstr,pidstr,hostname,readymsge);
       fclose(sd);
       if (numconv == 4)
       { 
	 /*
         printf("conv: %d, port: '%s', pid: '%s', host: '%s', rdymsge: '%s'\n",
                        numconv, portstr,pidstr,hostname,readymsge);
         */
         if (strcmp(readymsge,"ready") == 0)
         {
            pid = atoi(pidstr);
            alive = kill(pid,0);
            isReady = ((alive == -1) && (errno == ESRCH)) ? 0 : 1;
            /* printf("isJpsgReady() pid: %d, alive: %d\n",pid,isReady); */
         }
         else
             isReady = 0;
       } 
       else
         isReady = 0;
   }
   return(isReady);
}
 

Socket* connect2Jpsg(int port, char *host)
{
   int      ival;
   Socket  *tSocket;
   int trys;
 
   tSocket = createSocket( SOCK_STREAM );
   if (tSocket == NULL) {
       printf( "Cannot create local socket\n" );
       return( NULL );
   }
 
   ival = openSocket( tSocket );
   if (ival != 0) {
      printf( "Cannot open socket\n" );
      free(tSocket);
      return( NULL );
   }    
        
   trys = 7;
   while(trys--)
   {    
     /* printf("Try to Connect: '%s' port: %d trys left#: %d\n",host, port, trys); */
     ival = connectSocket( tSocket, host, port );
     if (ival == 0)
        break;
     sleep(1);
   }
 
   if (ival != 0)
   {
       closeSocket(tSocket);
       free(tSocket);
       return(NULL);
   }
   return(tSocket);
}
