/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************/
/*  bootup.c    prepares the vnmr environment  */
/***********************************************/

#include "vnmrsys.h"
#include "locksys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data.h"
#include "group.h"
#include "variables.h"
#include "init2d.h"
#include "acquisition.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"
#include "params.h"

#define COMPLETE	0
#define ERROR		1

int  last_line;
int  psg_pid;			/* pid of active psg */
int  skipFlush = 0;
int  autoDelExp = 0;

#ifdef  DEBUG
int Acqdebug = 1;
extern int Gflag;
#define GPRINT0(str) \
	if (Gflag) fprintf(stderr,str)
#define GPRINT1(str, arg1) \
	if (Gflag) fprintf(stderr,str,arg1)
#define DEBUGPRINT1(str, arg1) \
	fprintf(stderr,str,arg1)
#else 
#define GPRINT0(str)
#define GPRINT1(str, arg1)
#define DEBUGPRINT1(str, arg1)
#endif 

extern char datadir[];			/* Set in smagic.c */
extern char dconi_runstring[];
extern int jParent;
extern int mode_of_vnmr;
extern int bufferscale;		/* scaling factor for internal Vnmr buffers */
extern char RevID[];
extern char RevDate[];
extern char Copyright[];
extern char UserName[];

extern void turnOffFixpar();
extern void finddatainfo();
extern int  AcqSocketIsRead();
extern FILE *popen_call(char *cmdstr, char *mode);
extern int  pclose_call(FILE *pfile);
extern void frame_update(char *, char *);
extern void setAppdirs();
extern void Wturnoff_buttons();
extern int  lockExperiment(int expn, int mode );
extern int  unlockExperiment(int expn, int mode );
extern void closexposebuf();
extern int  p11_init();
extern void p11_flush();
extern int  p11_saveAuditTrail();
extern int  setPlotterName(char *name);
extern int  setPrinterName(char *name);
extern int  isDiskFullFile(char *path, char *path_2, int *resultadr );
extern void WrestoreTerminal();
extern void disp_expno();
extern int  expdir_to_expnum(char *expdir);
extern int  clear_acq();
extern void kill_all_childs();
extern void unlink_alphafile();
extern void set_acqi_signal();
extern void save_vnmr_geom();
extern int  closeVnmrInfo();
extern int  stop_acqi( int abortall );
extern int  sendTripleEscToMaster(char code, char *string_to_send );
extern int  disp_current_seq();
extern void check_datastation();
extern int  disp_current_seq();
extern void set_nmr_quit_signal();
extern void set_bootup_gfcn_ptrs();
extern void init_proc2d();
extern void init_colors();
extern int  checkAcqLock(int expn );
/* extern void setupVnmrAsync(void (*func)()); */
extern void setupVnmrAsync();
extern int  getAutoDir(char *str, int maxlen);
extern int  openVnmrInfo(char *dir);
extern int  delexp(int argc, char *argv[], int retc, char *retv[]);
extern void setCancel(int doit, char *str);

#ifdef VNMRJ
extern int VnmrJViewId;
extern char Jvbgname[];
extern void autoqMsgOff();
#endif 

int showFlushDisp = 1;
extern int acqStartUseRtp;

extern int part11System;

typedef union {
    unsigned int  my_int;
    unsigned char my_bytes[4];
} endian_tester;

int BigEndian = 1;
char OperatorName[MAXSTR];

/* Function prototypes */
static void set_automount_dir();
static int getbufscale();
void setGlobalPars();
int flush( int argc, char *argv[], int retc, char *retv[] );

int bigendian( int argc, char *argv[], int retc, char *retv[] )
{
   (void) argc;
   (void) argv;
   if (retc > 0)
      retv[0] = intString( BigEndian );
   else
      Winfoprintf("System architecture is %s Endian\n",(BigEndian) ? "Big" : "Little");
   RETURN;
}

#ifdef VNMRJ

#define VNMRJ_NUM_VIEWPORTS 4
static void jviewport_init()
{
    int e;
    double dcurwin;
    e=P_getreal(GLOBAL,"jviewport",&dcurwin,1);
    if (e < 0)
    {	P_creatvar(GLOBAL,"jviewport",ST_REAL);
	P_setlimits(GLOBAL,"jviewport", (double)(VNMRJ_NUM_VIEWPORTS * VNMRJ_NUM_VIEWPORTS), 0.0, 1.0);
	P_setprot(GLOBAL,"jviewport",33045);
    }
    P_setreal(GLOBAL,"jviewport",(double)(VnmrJViewId),1);
}

static void jcurwin_init(char *curexp )
{
    int e, i;
    double dcurwin;
    char expnum[8];

    e=P_getreal(GLOBAL,"jcurwin",&dcurwin,VnmrJViewId);
    if (e==-2)
    {	P_creatvar(GLOBAL,"jcurwin",ST_REAL);
	P_setlimits(GLOBAL,"jcurwin", (double)(MAXEXPS), 0.0, 1.0);
	P_setprot(GLOBAL,"jcurwin",272);
	for (i=1; i<=VNMRJ_NUM_VIEWPORTS; i++)
	  P_setreal(GLOBAL,"jcurwin",(double)(i),i);
	dcurwin = ((double)VnmrJViewId);
    }
    sprintf( expnum, "%d", ((int)(dcurwin + 0.5)) );
    strcpy( curexp, userdir );
    strcat( curexp, "/exp" );
    strcat( curexp, expnum );
}

void jcurwin_setexp(char *estring, int oldexpnum )
{
    int expnum;

    expnum = atoi(estring);
    if ((expnum < 0) || (expnum > MAXEXPS))
    {
      Werrprintf("jexp: value of jcurwin=%s out-of-range",estring);
      return;
    }
    if (P_setreal(GLOBAL,"jcurwin",(double)(expnum),VnmrJViewId) != 0)
    {
      Werrprintf("WARNING: jexp could not set parameter jcurwin");
    }
    else
    {
/* see jAutoSendIfGlobal( "jcurwin" ); */
      char mstr[MAXSTR];
      double dval;
      int i;
      vInfo info;
      sprintf(mstr,"jcurwin[%d]=%d",VnmrJViewId,expnum);
      writelineToVnmrJ("pglo",mstr);
/* check all other jcurwin values */
      if (oldexpnum > -1)
      {
        if (P_getVarInfo(GLOBAL, "jcurwin", &info)==0)
        {
	  if (info.size > VnmrJViewId)
	  {
	    for (i=VnmrJViewId; i<info.size; i++)
	    {
	      P_getreal(GLOBAL, "jcurwin", &dval, i+1);
/* if match, set to oldexpnum - see unlockAllExp() in locksys.c */
	      if ((int)(dval+0.1) == expnum)
	      {
		P_setreal(GLOBAL, "jcurwin", (double)(oldexpnum), i+1);
	        sprintf(mstr,"jcurwin[%d]=%d", i+1, oldexpnum);
	        writelineToVnmrJ("pglo",mstr);
	      }
	    }
	  }
        }
      }
    }
}
int jcurwin_jexp(int expnum, int oldexpnum )
{
    int index, get1, get2, retval=1;
    double dval;
    vInfo info; /* int VnmrJViewId */
    char path[MAXSTR];
    struct stat     stat_blk;

    (void) oldexpnum;
    if (mode_of_vnmr==AUTOMATION) /* || (mode_of_vnmr==BACKGROUND) */ 
      return(retval);

    if (expnum > 0)
    {
      sprintf(path, "%s/lock_%d.primary", userdir, expnum);
      if (stat( path, &stat_blk ) != 0) return(retval);

      get1 = P_getVarInfo(GLOBAL, "jcurwin", &info);
      get2 = P_getreal(GLOBAL, "jviewports", &dval, 1);
/* read number of viewports from jviewports[1] */
      if (get1==0 && get2==0)
      {
	get2 = (int) (dval + 0.5);
	if ((get2 > 0) && (get2 <= info.size))
	{
	  for (index=0; index<get2; index++)
	  {
	    P_getreal(GLOBAL, "jcurwin", &dval, index+1);
	    if (expnum == (int)dval)
	    {
	      char mstr[MAXPATH];
/*	      sprintf(mstr, "viewport %d", index+1);
	      writelineToVnmrJ("vnmrjcmd",mstr);
	      sprintf(mstr, "vplayout use %d", index+1);
	      writelineToVnmrJ("vnmrjcmd",mstr);
*/
	      P_getstring(GLOBAL, "jviewportlabel", mstr, index+1, MAXPATH);
	      Winfoprintf("Cannot join workspace exp%d - in use by %s viewport.\n",expnum,mstr);
	      retval = 0;
	      break;
	    }
	  }
	}
      }
    }
    return(retval);
}
static void jcurwin_setGlobalPars()
{
    int expnum;
    char estring[ 8 ];

    expnum = expdir_to_expnum(curexpdir);
    sprintf(estring,"%d",expnum);
    jcurwin_setexp( estring, -1 );
}
static int jcurwin_cexpn()
{
/*    int i, expct; */
/* (i) Loop expct = 1 to VNMRJ_NUM_VIEWPORTS, look for lock file(s)
   if not found, try to access( expn )
   if not access, then cexp()

   (ii) Search all lock files? See unlockAllExp() in locksys.c.

   (iii) Search all exp's which exist.  If lock file does not exist, return and join.
   If lock file does exist, go to next one.  If out of exp's, create one which
   is exp(N+1), provided N+1 < MAXEXPS.  Only need to search VNMRJ_NUM_VIEWPORTS exp's,
   unless lock failure (go to 2*VNMRJ_NUM_VIEWPORTS?).  
*/
    if ((VnmrJViewId > 0) && (VnmrJViewId <= VNMRJ_NUM_VIEWPORTS))
      return( VnmrJViewId );
    else
      return(1);
}

static void saveViewPortAddr()
{
   char addr[MAXPATH];
   char path[MAXPATH];
   int pid, port;
   int fd;
   mode_t umaskVal;

   P_getstring(GLOBAL,"vnmraddr",addr,1,MAXPATH);
   sscanf(addr,"%s %d %d", path, &port, &pid);
   sprintf(path,"%s/persistence/.vp_%d_%d", userdir, jParent, jcurwin_cexpn());
   umaskVal = umask(0);
   fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0666);
   write(fd, &port, sizeof(port));
   write(fd, &pid, sizeof(pid));
   close(fd);
   (void) umask(umaskVal);
}

#endif 

/* bootup sets up values for all of the above variables */
/* enumber integer experiment number */
/* modeptr mode of Vnmr: foreground, background, acquisition or automation */
/*****************************/
void bootup(char *modeptr, int enumber)
/*****************************/
{
    char	 autodir[MAXPATH];
    char         parampath[MAXPATH];
    char         plotname[STR64];
    char         mstr[MAXPATH];
    char         rtautocmd[2 * MAXPATH + 20];
    char	 estring[5];
    extern char  PlotterName[];
    extern char  PrinterName[];
    int do_rt = 0;
    int lval;
    endian_tester et;

    (void) modeptr;
    et.my_int = 0x0a0b0c0d;
    BigEndian = (et.my_bytes[0] == 0x0a) ? 1:0;
    psg_pid = 0;		/* set pids to zero */

    if ( !Bnmr )
       disp_status("BOOTUP  ");

/*  May 18, 1987.  Addition to read the system global parameters.
    If a problem occurs, the program exits immediately, for this
    should not happen.  The CONPAR file is on the distribution tape.	*/

    strcpy(parampath,systemdir);
#ifdef UNIX
    strcat(parampath,"/conpar");
#else 
    strcat(parampath,"conpar");
#endif 
    if (P_read(SYSTEMGLOBAL,parampath)) 
    {
	fprintf(stderr,"problem loading system global parameters from %s\n",
		parampath);
	exit(1);
    }
    openVnmrInfo(systemdir);

/*  If automation mode, use value of autodir to establish path to experiment  */

    if (mode_of_vnmr == AUTOMATION)
    {
      getAutoDir(autodir, MAXPATH);
      if (strlen(autodir) == 0)
      {
	  fprintf( stderr, "unable to access 'autodir' parameter\n" );
	  exit(1);
      }
    }

    /* setup signal handlers to catch these signals */
    if (Wissun())		/* Only do this if using SUN console */
    {
        set_acqi_signal();
    }
    else if (mode_of_vnmr == FOREGROUND) /* Only creat a Vnmr socket if foreground */
    {

	setupVnmrAsync(AcqSocketIsRead);
    }

#ifndef VNMRJ
    if (Wishds())
	last_line = 19;
    else
#endif
	last_line = 20;

    /* load parameters in global file */
    /* moved 9 lines to read GLOBAL before EXP, this way we can use
       curexpdir to restart Vnmr in the last exp used. May 17, 1997 */

    strcpy(parampath,userdir);
    strcat(parampath,"/global");
    if (P_read(GLOBAL,parampath)) 
	Werrprintf("problem loading global parameters");
#ifdef VNMRJ
   // set curexpdir based on jcurwin of viewport 1
   jcurwin_init(curexpdir);
#endif

     // load unshared globalX for viewport X if foreground
     if (!Bnmr)
     {
        sprintf(parampath,"%s/global%d",userdir,VnmrJViewId);
        /* Don't complain if unshared globals are missing */
        P_read(GLOBAL,parampath);
     }

    // set OperatorName 
    if ( (P_getstring(GLOBAL,"operator",OperatorName,1,MAXSTR) != 0) ||
         ( ! strcmp(OperatorName, "") ) )
	strcpy(OperatorName, UserName);

/*  If lock fails, and program is running in foreground mode, set
    current experiment to be EXP0, which doesn't exist.  The user
    can then join another experiment or use other techniques to
    unlock the desired experiment.

    If running in any other mode, exit immediately with an error
    message.							*/

/* May 19, 1997. Use curexpdir to set the enumber, check if directory
   still exists, if not make it exp1. Only done if FOREGROUND Vnmr.
   Acqproc, autoproc, and other procs determine enumber themselves.
   Note that curexpdir is overwritten below, when we read exp parameters */

#ifdef VNMRJ
   jviewport_init();
   P_setstring(GLOBAL,"curexp",curexpdir,1);
#else 
   P_getstring(GLOBAL,"curexp",curexpdir,1,MAXPATH);
#endif 
   if (mode_of_vnmr == FOREGROUND)
   {
      enumber = expdir_to_expnum(curexpdir);
      if (enumber == 0)
         lval = -1;
      else if ( ! access(curexpdir,F_OK))
        lval = lockExperiment( enumber, FOREGROUND);
      else
        lval = -1;
   }
#ifdef SUN
   else if (mode_of_vnmr == ACQUISITION)
      lval = (enumber == 0) ? -2 :checkAcqLock( enumber );
   else if ( Bnmr && (enumber == 0) )
      lval = -2;
   else
#endif
      lval = lockExperiment( enumber, mode_of_vnmr );

    if (lval != 0)
    {
        if ((mode_of_vnmr != FOREGROUND) && (lval != -2) )
        {
	    fprintf( stderr, "unable to lock experiment %d\n", enumber);
	    exit(1);
	}
        if (enumber == 0)
        {
#ifdef VNMRJ
           if (VnmrJViewId != 1)
           {
	      P_getstring(GLOBAL, "jviewportlabel", mstr, VnmrJViewId, MAXPATH);
	      Werrprintf( "No experiment selected for the %s viewport.",mstr);
           }
           jcurwin_setexp( "0" , -1 );
#else 
           if (lval != -2)
	      Werrprintf( "No experiment selected.");
#endif 
        }
        else
        {
#ifdef VNMRJ
           if (VnmrJViewId == 1)
	      Werrprintf( "Unable to lock experiment %d", enumber );
           else
           {
	      P_getstring(GLOBAL, "jviewportlabel", mstr, VnmrJViewId, MAXPATH);
	      Werrprintf( "Unable to lock experiment %d for the %s viewport.",enumber, mstr);
           }
           jcurwin_setexp( "0", -1 );
#else 
	   Werrprintf( "Unable to lock experiment %d", enumber );
#endif 
        }
        strcpy( curexpdir, userdir );
#ifdef UNIX
        strcat( curexpdir, "/exp0" );
#else 
	vms_fname_cat( curexpdir, "[.exp0]" );
#endif 
#ifdef VNMRJ
        if (mode_of_vnmr != AUTOMATION)
        { 
           sprintf(mstr,"exp0 %s",curexpdir);
           writelineToVnmrJ("expn",mstr);
        }
#endif
    }

/*  Only read in parameters if the target experiment was locked.  */

    else
    {
	if (mode_of_vnmr == AUTOMATION)
	  strcpy( curexpdir, &autodir[ 0 ] );
	else
          strcpy( curexpdir, userdir );

	sprintf(estring, "%d", enumber);
#ifdef UNIX
        strcat(curexpdir, "/exp");
	strcat(curexpdir, estring);
#else 
        vms_fname_cat(curexpdir, "[.exp");
        vms_fname_cat(curexpdir, estring);
        vms_fname_cat(curexpdir, "]");
#endif 

        if (mode_of_vnmr == AUTOMATION)
        {
	    lval = strlen( &datadir[ 0 ] );
	    if (lval > MAXPATH-8)
	    {
	        fprintf(stderr, "data pathname too long in automation mode");
	        exit(1);
	    }
            else if (lval > 1)
            {
                int ex;

                sprintf(rtautocmd, "cp %s.fid/sampleinfo %s/sampleinfo",
                        datadir,curexpdir);
                system(rtautocmd);
                ex = 0;
                /* Wait up to 0.5 secs for fid to appear */
                while ( access(rtautocmd,R_OK) && (ex < 50) )
                {
                   struct timespec timer;

                   timer.tv_sec=0;
                   timer.tv_nsec = 10000000;   /* 10 msec */
#ifdef __INTERIX
				   usleep(timer.tv_nsec/1000);
#else
                   nanosleep( &timer, NULL);
#endif
                   ex++;
                }
	        sprintf( &rtautocmd[ 0 ], "%s('%s','nodg')\n",
                         (acqStartUseRtp) ? "RTP" : "RT", &datadir[ 0 ] );
                do_rt = 1; /* do the rt command after reading global tree */
            }
            else  /* read curpar parameters only from the automation exp */
            {
                D_getparfilepath(CURRENT, parampath, curexpdir);
                if (P_read(CURRENT,parampath))
                {  /* if no parameters in current experiment */
                   strcpy(parampath,systemdir);
#ifdef UNIX
                   strcat(parampath,"/stdpar/H1.par/procpar");
#else 
                   vms_fname_cat(parampath,"[.stdpar.H1_par]procpar");
#endif 
                   if (P_read(CURRENT,parampath))
	              Werrprintf("problem loading current parameters");
                }
                P_copy(CURRENT,PROCESSED);
            }
        }
	else       /* if (mode_of_vnmr != AUTOMATION) */
        {
           /* load parameters in curexp/curpar file */
           D_getparfilepath(CURRENT, parampath, curexpdir);
           if (P_read(CURRENT,parampath))
	       Werrprintf("problem loading current parameters from \"%s\"",
                           parampath);
           /* load parameters in curexp/procpar file */
           D_getparfilepath(PROCESSED, parampath, curexpdir);
           if (P_read(PROCESSED,parampath))
	       Werrprintf("problem loading processed parameters from \"%s\"",
                           parampath);
#ifdef VNMRJ
	   sprintf(mstr,"exp%s %s",estring,curexpdir);
	   writelineToVnmrJ("expn",mstr);
#endif 
        }
    }


    /* May 17, 1997. We read GLOBAL parameters earlier */
    setGlobalPars();
#ifdef VNMRJ
    if (!Bnmr)
       saveViewPortAddr();
#endif 
    bufferscale = getbufscale();	/* obtain the value for bufferscale */
    finddatainfo();
    DEBUGPRINT1("bufferscale set to %d\n", bufferscale);
    D_init();				/* initialize the data file handler;
					   bufferscale must be known before
					   D_init() is called.		    */

    /* setAppdirs needs to happen after operator is set in setGlobalPars but before
     * any macros are called
     */
#ifdef VNMRJ
    if (VnmrJViewId == 1)
#endif 
    {
       setAppdirs();
    }

    specIndex = 1;
    if (mode_of_vnmr == AUTOMATION)
    {
      if (do_rt)
      {
        turnOffFixpar();
        execString( &rtautocmd[ 0 ] );
      }
      else
      {
        Werrprintf( "Spectrometer in automation mode" );
      }
    }
    else if (enumber || (mode_of_vnmr != BACKGROUND) )
    {
       disp_expno();
    }
    init_proc2d();
    strcpy(dconi_runstring,"dcon\n");

    /* Assign some function pointers for the */
    /* graphics module                       */
    set_bootup_gfcn_ptrs();
    if (Bnmr)
       init_colors();  /* set default colors for graphics and plotters */

    set_automount_dir();

    /*  Set up plotting parameters */
    /*  If there is trouble setting plottin parameters (ie plotting device
        doesn't exist or devicetable/devicenames is bad, set to "none" device */
    if (P_getstring(GLOBAL,"plotter" ,plotname, 1,32))
    {  P_setstring(GLOBAL,"plotter","none",0);
       strcpy(PlotterName,"none");
    }
    if (!setPlotterName(plotname))
    {  P_setstring(GLOBAL,"plotter","none",0);
       strcpy(PlotterName,"none");
    }

    /*  Set up printing parameters */
    /*  If there is trouble setting printing parameters (ie printing device
        doesn't exist or devicetable/devicenames is bad, set to "none" device */
    if (P_getstring(GLOBAL,"printer" ,plotname, 1,32))
    {  P_setstring(GLOBAL,"printer","none",0);
       strcpy(PrinterName,"none");
    }
    if (!setPrinterName(plotname))
    {  P_setstring(GLOBAL,"Printer","none",0);
       strcpy(PrinterName,"none");
    }

#ifdef SUN
/*
 *  Set up signal handler to exit VNMR
 *  The function nmr_quit will be called when Vnmr exits
 */
    set_nmr_quit_signal();
#endif 
    check_datastation();

#ifdef VNMRJ
    writelineToVnmrJ("bootup","");
    /* create a frame for graphics display */
    frame_update("init", "");
#endif 

/*  The extra space in the first Wscrprintf is essential for
    the scrollable text subwindow to work correctly.			*/

/* ---  print revision ID and Date, Compiled within revdate.c --- */
    P_setstring( SYSTEMGLOBAL, "rev", &RevID[ 0 ], 1);
    P_setstring( SYSTEMGLOBAL, "revdate", &RevDate[ 0 ], 1);
    if (!Bnmr) /* only execute the bootup macro if we are in forground */
    {
	Wscrprintf("\n              %s\n",RevID);
        Wscrprintf("              %s\n",RevDate);

        Wscrprintf("              %s\n\n",Copyright);
        if (strlen(PlotterName) > 0)
           Wscrprintf( "              Plotting Device is set to %s\n",PlotterName);
        else
           Wscrprintf( "              Plotting Device is set to ''\n");
        if (strlen(PrinterName) > 0)
           Wscrprintf( "              Printing Device is set to %s\n\n",PrinterName);
        else
           Wscrprintf( "              Printing Device is set to ''\n\n");

        disp_current_seq();
        if (Wissun())
           sendTripleEscToMaster( 'C',"bootup(0)");
        else
	   execString("bootup(0)\n");
        disp_status("        ");
    }
    else if (enumber)
    {
	execString("bootup(1)\n");
    }

    p11_init();
}

void setGlobalPars()
{
    char addr[MAXPATH];
    vInfo info;
    int e;

    e=P_setstring(GLOBAL,"userdir",userdir,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"userdir",ST_STRING);
	P_setlimits(GLOBAL,"userdir",(double)MAXPATH,0.0,0.0);
	P_setstring(GLOBAL,"userdir",userdir,0);
	P_setprot(GLOBAL,"userdir",0);
    }
    e=P_setstring(GLOBAL,"curexp",curexpdir,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"curexp",ST_STRING);
	P_setlimits(GLOBAL,"curexp",(double)MAXPATH,0.0,0.0);
	P_setstring(GLOBAL,"curexp",curexpdir,0);
	P_setprot(GLOBAL,"curexp",0);
    }
#ifdef VNMRJ
    jcurwin_setGlobalPars();
#endif 
    e=P_setstring(GLOBAL,"systemdir",systemdir,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"systemdir",ST_STRING);
	P_setlimits(GLOBAL,"systemdir",(double)MAXPATH,0.0,0.0);
	P_setstring(GLOBAL,"systemdir",systemdir,0);
	P_setprot(GLOBAL,"systemdir",0);
    }
    e=P_setstring(GLOBAL,"auto",(mode_of_vnmr == AUTOMATION) ? "y" : "n",0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"auto",ST_STRING);
	P_setlimits(GLOBAL,"auto",(double)STR64,0.0,0.0);
	P_setstring(GLOBAL,"auto",(mode_of_vnmr == AUTOMATION) ? "y" : "n",0);
	P_setprot(GLOBAL,"auto",7);  /* do not allow any user change */
        P_Esetstring(GLOBAL,"auto","n",1);
        P_Esetstring(GLOBAL,"auto","y",2);
    }
    if (mode_of_vnmr == AUTOMATION)
    {
       getAutoDir(addr, MAXPATH);
       e=P_setstring(GLOBAL,"autodir",addr,0);
    }
    else
    {
       e=P_getstring(GLOBAL,"autodir",addr,1,MAXPATH);
    }
    if (e==-2)
    {	P_creatvar(GLOBAL,"autodir",ST_STRING);
	P_setlimits(GLOBAL,"autodir",(double)MAXPATH,0.0,0.0);
        if (mode_of_vnmr == AUTOMATION)
	   P_setstring(GLOBAL,"autodir",addr,0);
	P_setprot(GLOBAL,"autodir",3);
    }
    GET_VNMR_ADDR(addr);
    e=P_setstring(GLOBAL,"vnmraddr",addr,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"vnmraddr",ST_STRING);
	P_setlimits(GLOBAL,"vnmraddr",(double)STR128,0.0,0.0);
	P_setstring(GLOBAL,"vnmraddr",addr,0);
	P_setprot(GLOBAL,"vnmraddr",4);  /* do not allow any user change */
    }
    GET_ACQ_ADDR(addr);
    e=P_setstring(GLOBAL,"acqaddr",addr,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"acqaddr",ST_STRING);
	P_setlimits(GLOBAL,"acqaddr",(double)STR128,0.0,0.0);
	P_setstring(GLOBAL,"acqaddr",addr,0);
	P_setprot(GLOBAL,"acqaddr",4);  /* do not allow any user change */
    }
    e=P_setstring(GLOBAL,"owner",UserName,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"owner",ST_STRING);
	P_setlimits(GLOBAL,"owner",(double)STR128,0.0,0.0);
	P_setstring(GLOBAL,"owner",UserName,0);
	P_setprot(GLOBAL,"owner",P_VAL+P_GLO);  /* do not allow any user change */
    }
    e=P_setstring(GLOBAL,"operator",OperatorName,0);
    if (e==-2)
    {	P_creatvar(GLOBAL,"operator",ST_STRING);
	P_setlimits(GLOBAL,"operator",(double)STR128,0.0,0.0);
	P_setstring(GLOBAL,"operator",UserName,0);
	P_setprot(GLOBAL,"operator",P_VAL+P_GLO);  /* do not allow any user change */
    }
    e=P_getVarInfo(GLOBAL,"department",&info); /* do not set value, read from file */
    if (e==-2)
    {	P_creatvar(GLOBAL,"department",ST_STRING);
	P_setlimits(GLOBAL,"department",(double)STR128,0.0,0.0);
	P_setstring(GLOBAL,"department","nmr",0);
	P_setprot(GLOBAL,"department",P_VAL+P_GLO);  /* do not allow any user change */
    }
}

static char *
next_non_blank(char *tptr )
{
	while (*tptr == ' ' || *tptr == '\t')
	  tptr++;

	if (*tptr == '\0')
	  return( NULL );
	else
	  return( tptr );
}

/*  Unsophisticated, non-optimized substring finding program  */

static char *
find_substr( char *base, char *pattern )
{
	int	blen, plen;

/*  First two tests prevent program failures.  Third test eliminates 0-length
    patterns to search for.  Fourth test eliminates patterns which are longer
    than the base string.							*/

	if (base == NULL)
	  return( NULL );
	if (pattern == NULL)
	  return( NULL );
	plen = strlen( pattern );
	if (plen < 1)
	  return( NULL );
	blen = strlen( base );
	if (blen < plen)
	  return( NULL );

	while (*base != '\0') {
		base = strchr( base, pattern[ 0 ] );
		if (base == NULL)
		  return( NULL );
		if ((int)strlen( base ) < plen)
		  return( NULL );
		if (strncmp( base, pattern, plen ) == 0)
		  return( base );
		base++;
	}

	return( NULL );
}

#define  PS_OUTPUT_LINE_SIZE  1026

static char *
find_automount()
{
	char	*ps_command, *ps_output_line, *retaddr, *taddr;
	int	 qlen;
	FILE	*ps_output_pipe;

	ps_output_line = (char *) malloc( PS_OUTPUT_LINE_SIZE );
	if (ps_output_line == NULL) {
		return( NULL );
	}

#ifdef LINUX
	  ps_command = "ps ax";
#else 
	  ps_command = "ps -ef";
#endif 
	ps_output_pipe = popen_call( ps_command, "r" );
	if (ps_output_pipe == NULL) {
		free( ps_output_line );
		return( NULL );
	}

	taddr = NULL;
	while (fgets(
		    ps_output_line,
		    PS_OUTPUT_LINE_SIZE - 1,
		    ps_output_pipe
	   ) != NULL) {
		qlen = strlen( ps_output_line );
		if (qlen > 0)
		  if (ps_output_line[ qlen-1 ] == '\n')
		    ps_output_line[ qlen-1 ] = '\0';
		taddr = find_substr( ps_output_line, "automount" );
		if (taddr != NULL) {
			break;
		}
	}

	pclose_call( ps_output_pipe );

/*  If you want to return the entire line, skip this bit and return ps_output_line  */

	if (taddr != NULL) {
		qlen = strlen( taddr );
		retaddr = (char *) malloc( qlen+1 );
		if (retaddr == NULL) {
			free( ps_output_line );
			return( NULL );
		}
		strcpy( retaddr, taddr );
		free( ps_output_line );

		return( retaddr );
	}
	else
        {
	  free( ps_output_line );
	  return( NULL );
        }
}

#define  DEFAULT_AUTOMOUNT	"/tmp_mnt"

static char	automount_dir[ MAXPATH ];
static int	automount_len;

static
int ps_to_automount_dir()
{
	char	*automount_cmd, *switch_automount, *qaddr;
	int	 qlen;

/*  If there is no automount process, then there is no automount directory  */

	automount_cmd = find_automount();
	if (automount_cmd == NULL) {
		automount_dir[ 0 ] = '\0';
		automount_len = 0;
		return( 0 );
	}

/*  If there is an automount process, but it doesn't call out an automount
    directory, then the default automount directory is the automount directory  */

	switch_automount = find_substr( automount_cmd, "-M" );
	if (switch_automount == NULL) {
		strcpy( &automount_dir[ 0 ], DEFAULT_AUTOMOUNT );
		automount_len = strlen( DEFAULT_AUTOMOUNT );
		free( automount_cmd );
		return( 0 );
	}

	switch_automount += 2;				/* skip past "-M" */
	qaddr = next_non_blank( switch_automount );	/* start of automount dir */
	if (qaddr == NULL) {
		strcpy( &automount_dir[ 0 ], DEFAULT_AUTOMOUNT );
		automount_len = strlen( DEFAULT_AUTOMOUNT );
		free( automount_cmd );
		return( 0 );
	}
	else
	  switch_automount = qaddr;

	qaddr = strchr( switch_automount, ' ' );	/* end of automount dir */
	if (qaddr == NULL)
	  qlen = strlen( switch_automount );
	else
	  qlen = qaddr - switch_automount;

	strncpy( &automount_dir[ 0 ], switch_automount, qlen );
	automount_dir[ qlen ] = '\0';
	automount_len = qlen;
	free( automount_cmd );

	return( 0 );
}

static
int env_to_automount_dir()
{
	int		 retval;
	char		*quickaddr;

	quickaddr = getenv( "automountdir" );
	if (quickaddr != NULL) {
		strcpy( &automount_dir[ 0 ], quickaddr );
		automount_len = strlen( quickaddr );

		retval = 0;
	}
	else
	  retval = -1;

	return( retval );
}

static void set_automount_dir()
{

/*  Try environment to automount directory.  If that fails, fall back
    on ps (command) to automount directory, which always succeeds.	*/

	if (env_to_automount_dir() != 0)
	  ps_to_automount_dir();
}

int fix_automount_dir(char *input, char *output )
{
	if (automount_len == 0 ||
	    strncmp( input, &automount_dir[ 0 ], automount_len ) != 0)
	  strcpy( output, input );
	else
	  strcpy( output, input+automount_len );

	return( 0 );
}

/*---------------------------------------
|					|
|	     getbufscale()/0		|
|					|
+--------------------------------------*/
#define MINMEMSIZE 256
static int getbufscale()
{
   char		*memsize_cptr;	/* character pointer to "memsize" */
   int		bufval,
		res;

   if ( (memsize_cptr = getenv("memsize")) == NULL)
   {
      bufval = MINMEMSIZE;
   }
   else
   {
      bufval = atoi(memsize_cptr);
      if (bufval < MINMEMSIZE)
         bufval = MINMEMSIZE;
   }
   DEBUGPRINT1("The system has %d MBytes of physical memory.\n", bufval);

   bufval /= 4;    /* convert to units of 4 Mbytes */

/***********************************
*  "bufval" must be a power of 2.  *
***********************************/

   if (bufval <= 1)
   {
      bufval = 1;
   }
   else
   {
      res = 1;
      while (res < bufval)
         res *= 2;
      if (res > bufval)
      {
         bufval = res/2;
      }
      else
      {
         bufval = res;
      }
   }

   return(bufval);
}


/****************/
int
disp_current_seq()
/****************/
{ 
  char name[66];
#ifdef VNMRJ
  if (P_getstring(CURRENT,"pslabel",name,1,64)) ABORT;
  disp_seq(name);
#else
  if (P_getstring(CURRENT,"pslabel",name,1,15)) ABORT;
  disp_seq(name);
#endif
  RETURN;
}

/********/
void nmr_exit(char *modeptr)
/********/
/* exit from nmr program */
{   
    static int exiting = 0;  /* prevent interrupts from re-scheduling this routine */
    int enumber;

   (void) modeptr;
   if (!exiting)
   {
      exiting = 1;
#ifdef SUN
      interact_kill("");	/* Kill all interactive acquisition progs */
#endif 
#ifdef VNMRJ
      frame_update("exit", "closeFrames");
      stop_acqi( 1 );
      autoqMsgOff();
#endif 
      flush(99,NULL,0,NULL);
      if (mode_of_vnmr == FOREGROUND)
         WrestoreTerminal();

      enumber = expdir_to_expnum(curexpdir);

      if (enumber > 0)
        unlockExperiment( enumber, mode_of_vnmr );

/*  If the current experiment is not purged (delete older versions of a file)
    excess copies of curpar, procpar, etc. accumulate.				*/

      closeVnmrInfo();
#ifdef X11
      save_vnmr_geom();
#endif 
#ifdef VNMRJ
      unlink_alphafile();
      if (access( Jvbgname, 0 ) == 0)
         unlink( Jvbgname );
      if (!Bnmr)
      {
         char path[MAXPATH];
         sprintf(path,"%s/persistence/.vp_%d_%d", userdir, jParent, jcurwin_cexpn());
         unlink(path);
      }
#endif 
      kill_all_childs();
#ifndef VNMRJ
      exitTclInfo();
#endif 
      clear_acq();

      if(part11System) p11_saveAuditTrail();
      if ( (mode_of_vnmr == BACKGROUND) && autoDelExp)
      {
         char *argv[1];

         argv[0] = "autoDELEXP";
         delexp(1,argv,0,NULL);
      }

      exit(0);
   }
}

int flushpars( int argc, char *argv[], int retc, char *retv[] )
{ char parampath[MAXPATH];
  int  diskIsFull;
  int  ival;

  sprintf(parampath,"%s/flushparsFailed",curexpdir);
  unlink(parampath);
  D_getparfilepath(CURRENT, parampath, curexpdir);
  if (P_save(CURRENT,parampath))
  {
     if (argc == 2)
     {
        FILE *fd;

        sprintf(parampath,"%s/flushparsFailed",curexpdir);
        fd = fopen(parampath,"w");
        if (fd)
           fclose(fd);
        sprintf(parampath,"%s/%s",curexpdir,argv[1]);
        unlink(parampath);
        RETURN;
     }
     else if (retc)
     {
        retv[0] = intString( 0 );
        RETURN;
     }
     else
     {
        ival = isDiskFullFile( curexpdir, parampath, &diskIsFull );
        if (ival == 0 && diskIsFull)
        {
           Werrprintf("problem saving current parameters: disk is full");
        }
        else
          Werrprintf("problem saving current parameters");
        ABORT;
     }
  }
  D_getparfilepath(PROCESSED, parampath, curexpdir);
  if (P_save(PROCESSED,parampath))
  {
     if (argc == 2)
     {
        FILE *fd;

        sprintf(parampath,"%s/flushparsFailed",curexpdir);
        fd = fopen(parampath,"w");
        if (fd)
           fclose(fd);
        sprintf(parampath,"%s/%s",curexpdir,argv[1]);
        unlink(parampath);
        RETURN;
     }
     else if (retc)
     {
        retv[0] = intString( 0 );
        RETURN;
     }
     else
     {
        ival = isDiskFullFile( curexpdir, parampath, &diskIsFull );
        if (ival == 0 && diskIsFull)
        {
           Werrprintf("problem saving processed parameters: disk is full");
        }
        else
        {
          Werrprintf("problem saving processed parameters");
        }
        ABORT;
     }
  }
  if (argc == 2)
  {
     sprintf(parampath,"%s/%s",curexpdir,argv[1]);
     unlink(parampath);
  }
  if (retc)
  {
     retv[0] = intString( 1 );
  }
  RETURN;
}

/*  flush is now an official VNMR command.  Use it when you want to access
    the data in the current experiment from a separate program.

    Because numerous other VNMR commands call flush with no arguments,
    DO NOT MAKE REFERENCE TO ANY OF THE ARGUMENTS IN THE ARGUMENT LIST.  */

int flush( int argc, char *argv[], int retc, char *retv[] )
{ char parampath[MAXPATH];
  int  curexpnumber;
  int  diskIsFull;
  int  ival;
  extern int start_from_ft;
  static int flushed = -1;

  (void) argc;
  (void) argv;
  (void) retc;
  (void) retv;

  // flush global if VnmrJViewId == 1 or argc != 99 (almost always the case).
  // Note, flush(99,NULL,0,NULL) is called in nmr_exit to avoid repeatly
  // flush global by multiple viewports.
  int flushGlobal=(VnmrJViewId == 1 || argc != 99);

  if ((mode_of_vnmr == AUTOMATION) && (datadir[0] != '\0') && (flushed == -1))
  {
     /* In automation, datadir[0] != '\0' means conditional processing is occurring.
      * The first time flush is called is during the bootup process when data is
      * being rt'ed from the .fid file into the automation experiment.
      * Therefore, no need to flush data.
      * Note that rt() always calls flush if in automation mode.
      */
     flushed = 0;
     RETURN;
  }
  if (skipFlush && (mode_of_vnmr == AUTOMATION))
  {
    skipFlush = 0;
    RETURN;
  }
  skipFlush = 0;
  curexpnumber = expdir_to_expnum(curexpdir);
  if (curexpnumber < 1) {
     if (!Bnmr)
        Werrprintf( "no current experiment, aborting save" );
     RETURN;
  }

  Wturnoff_buttons();
  if (showFlushDisp)
     disp_status("SVPAR   ");

  /* save parameters in curexp/curpar file */

  D_getparfilepath(CURRENT, parampath, curexpdir);
  if (P_save(CURRENT,parampath))
  {
     ival = isDiskFullFile( curexpdir, parampath, &diskIsFull );
     if (ival == 0 && diskIsFull)
     {
        Werrprintf("problem saving current parameters: disk is full");
     }
     else
       Werrprintf("problem saving current parameters");
     ABORT;
  }

  /* save parameters in curexp/procpar file */

  if ((mode_of_vnmr == AUTOMATION) && (datadir[0] != '\0') && (flushed == 0))
  {
    setfilepaths(0);  /* reset file paths to main data files for automation */
    P_copygroup(CURRENT,PROCESSED,G_DISPLAY);
    sprintf(parampath,"%s.fid",datadir);
    if (access(parampath,W_OK))  /* If .fid file is missing, assume it was deleted on purpose. */
    {
       strcpy(parampath,curexpdir);
    }
  }
  else
    strcpy(parampath,curexpdir);

  D_getparfilepath(PROCESSED, parampath, parampath);
  if (P_save(PROCESSED,parampath))
  {
     ival = isDiskFullFile( curexpdir, parampath, &diskIsFull );
     if (ival == 0 && diskIsFull)
     {
        Werrprintf("problem saving processed parameters: disk is full");
     }
     else
     {
       Werrprintf("problem saving processed parameters");
     }
     ABORT;
  }

  if ((mode_of_vnmr == AUTOMATION) && (datadir[0] != '\0') && (flushed == 0))
    flushed = 1;
  /* save parameters in global file except when doing automation or running in background */
  if ( (mode_of_vnmr != AUTOMATION) && ! Bnmr)
  {
#ifdef VNMRJ
    if (flushGlobal) // write shared and unshared
#endif 
    {
     strcpy(parampath,userdir);
     strcat(parampath,"/global");
     if (P_save(GLOBAL,parampath)) 
     {
	ival = isDiskFullFile( userdir, parampath, &diskIsFull );
	if (ival == 0 && diskIsFull)
	   Werrprintf("problem saving global parameters: disk is full");
	else
	   Werrprintf("problem saving global parameters");
        ABORT;
     }
    }

     // always write un-shared globals 
     sprintf(parampath,"%s/global%d",userdir,VnmrJViewId);
     if (P_saveUnsharedGlobal(parampath)) 
     {
	ival = isDiskFullFile( userdir, parampath, &diskIsFull );
	if (ival == 0 && diskIsFull)
	   Werrprintf("problem saving unshared global parameters: disk is full");
	else
	   Werrprintf("problem saving unshared global parameters");
        ABORT;
     }
  }

  p11_flush();

  if (showFlushDisp)
     disp_status("SAVEDATA");
  D_close(D_DATAFILE);
  D_close(D_PHASFILE);
  D_close(D_USERFILE);
  start_from_ft = 1;
  closexposebuf();
  if (showFlushDisp)
     disp_status("        ");
  RETURN;
}

/*
 *  Command to control events when a cancel command is sent
 */
int onCancel( int argc, char *argv[], int retc, char *retv[] )
{
   char msg[MAXPATH];
   int  yesNoOff;

   if ( Bnmr )
      RETURN;
   if (argc == 1)
   {
      setCancel(1,"");
      strcpy(msg,"");
   }
   else if (argc >= 2)
   {
      /* if true, do standard interrupt in addition to specific cancel command */
      if ( (argc == 4) && ! strcmp(argv[3],"yes") )
         yesNoOff = 1;
      else if ( (argc == 4) && ! strcmp(argv[3],"off") )
         yesNoOff = 2;
      else
         yesNoOff = 0;
      if (argc >= 3)
         setCancel(yesNoOff,argv[2]);
      else
         setCancel(yesNoOff,"");
      strcpy(msg,argv[1]);
   }
   sendTripleEscToMaster( 'g',msg);
   RETURN;
}

