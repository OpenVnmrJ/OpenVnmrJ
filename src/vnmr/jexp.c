/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/* jexp   - join a nmr experiment          		*/
/* cexp   - create a nmr experiment        		*/
/* delexp - delete a nmr experiment        		*/
/* svf    - save a free induction decay    		*/
/* svp    - save a parameter set           		*/
/* rt	  - return fid or parameter set    		*/
/* rtp    - return a parameter set         		*/
/* s      - save display parameters        		*/
/* fr     - return all display parameters  		*/
/* r      - return some display parameters 		*/
/* rts    - return shim coil settings			*/
/* svs    - save shim coil settings			*/
/* exists - return, whether parameter or file exists    */
/* rtv    - return a variable from a variable file      */
/* mp     - move current parameters to specified exp    */
/* mf     - move processed parameters and FID		*/
/* md     - move saved display parameters		*/
/* gettxt - returns TEXT from named file 		*/
/* puttxt - saves TEXT in named file 			*/
/*							*/
/* getdate - get the date a file was created (not done) */
/********************************************************/

#include "vnmrsys.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#else 
#include <file.h>
#define  F_OK	0
#define  X_OK	1
#define  R_OK	4
#endif 

#include "data.h"
#include "group.h"
#include "init2d.h"
#include "locksys.h"
#include "variables.h"
#include "pvars.h"
#include "tools.h"
#include "vfilesys.h"
#include "shims.h"
#include "wjunk.h"
#include "buttons.h"
#include "params.h"

#define FALSE		0
#define TRUE		1

#define NONFDA 0
#define BOTH 1
#define FDA 2

extern int debug1;
extern int mode_of_vnmr;
extern int autoDelExp;
extern int showFlushDisp;
extern char datadir[];
extern char vnMode[];	/* "foreground", "background" or "acquisition" */
#ifdef VNMRJ
extern char  Jvbgname[];
extern char VnmrJHostName[];
extern void setMenuName(const char *nm);
#endif 
extern char *fname_cat();
extern char *get_cwd();
extern char *check_spaces(char *s1, char *s2, int len);
extern int menuflag;
extern int menuon;
extern void frame_update(char *cmd, char *value);
extern int isInteractive(char *sName);
extern int isACmd(char *sName);
extern int isFileAscii(char *s);
extern int is_exp_active( int this_expnum );
extern int fix_automount_dir(char *input, char *output );
extern void p11_restartCmdHis();
extern void p11_init_acqfil(char* func, char* orig, char* dest);
extern int p11_copyFiles(char *orig, char *dest);
extern int disp_current_seq();
extern void appendTopPanelParam();
extern int text(int argc, char *argv[], int retc, char *retv[]);
extern int isDirectory(char *filename);
extern int rt_FDA(int argc, char *argv[], int retc, char *retv[],
           char* filepath, int gettxtflag, int fidflag,
           int nolog, int doDisp, int doMenu, int do_update_params, int type);
extern int p11_isRecord(char* path);
extern int p11_writeAuditTrails_S(char* func, char* origpath, char* destpath);
extern int p11_isPart11Dir(char *str);
extern void p11_checkData();
extern void currentDate(char *cstr, int len );
extern int isFid(char *s);
extern int isPar(char *s);
extern int is_exp_acquiring(int this_expnum );
extern int getactivefile();
extern void update_imagefile();
extern int lockExperiment(int expn, int mode );
extern int unlockExperiment(int expn, int mode );
extern int expdir_to_expnum(char *expdir);
extern int flush( int argc, char *argv[], int retc, char *retv[] );

extern int part11System;
extern int save_optFiles(char* dest, char *type);
extern int isFDARec(char *s);
extern int isRec(char *s);
extern int svr_FDA(int argc, char *argv[], int retc, char *retv[]);
extern void clearGraphFunc();
extern void clearMspec();
extern void set_dpf_flag(int b, char *cmd);
extern void set_dpir_flag(int b, char *cmd);
extern int loadFdfSpec(char *path, char *key);
float *aipGetTrace(char *key, int ind, double scale, int npt);

#ifdef CLOCKTIME
   extern int jexp_timer_no;
#endif 

static char chkbuf[MAXPATH+2];
/*  Define the Search Sequence for EXISTS, RTS and SVS  */

static int rjexpnum = 0;
static int autoexp = 0;

extern void saveGlobalPars(int sv, char *suff);
static void report_copy_file_error(int errorval, char *filename );
static void saveallshims(int rtsflag);
static int saveshim(const char *vname, int rtsflag);
static int fid_is_link(char *filepath );
static int param_access(char *path, int level );
int set_nodata();
extern int aspFrame(char *keyword, int frameID, int x, int y, int w, int h);

void doingAutoExp()
{
   autoexp = 1;
}

/*************************************/
static int access_exp(char *exppath, char *estring)
/*************************************/
{

/* Only verifies the existance of an experiment. */
  strcpy(exppath, userdir);
#ifdef UNIX
  strcat(exppath, "/exp");
  strcat(exppath, estring);
  if ( access(exppath, R_OK | W_OK) )
#else 
  strcat(exppath,"exp");
  strcat(exppath, estring);
  strcat(exppath, ".dir");
  if ( access(exppath, R_OK) )		/*  Can't check for write  */
#endif 
  {
     Werrprintf("experiment %s is not accessible", exppath);
     ABORT;
  }
#ifdef VMS
  make_vmstree(exppath,exppath,MAXPATHL);
#endif 
  RETURN;
}

static int reset_exp0(int oldexpnum )
{
  extern void finddatainfo();
#ifdef VNMRJ
  extern void jcurwin_setexp();
  char estring[MAXPATH] = "0";
#endif 
  char exppath[MAXPATH];
  int lval;

  if (oldexpnum == -1)
  {
    if (rjexpnum > 0)
    {
      sprintf(exppath,"jexp%d\n",rjexpnum);
      rjexpnum = 0;
      execString( exppath );
    }
    RETURN;
  }

  strcpy(exppath,userdir);
  strcat(exppath,"/exp0");
  if (oldexpnum > 0)
  {
    flush(0,NULL,0,NULL);  /* this is a VNMR command */
  }
  setfilepaths(0);
  P_treereset(CURRENT);
  P_treereset(PROCESSED);

/* Now that we are finished with the old experiment, unlock it */
  if (oldexpnum > 0)
    lval = unlockExperiment( oldexpnum, mode_of_vnmr );
  strcpy(curexpdir,exppath);
  P_setstring(GLOBAL,"curexp",curexpdir,0);
#ifdef VNMRJ
  jcurwin_setexp( estring, oldexpnum );
#endif 
  finddatainfo();
  disp_status("        ");
  Wclear(2);
  Wsetgraphicsdisplay( "" );
  menuflag = 0;
  menuon = 0;
  Wturnoff_buttons();
  rjexpnum = oldexpnum;

#ifdef VNMRJ
/*  sprintf(exppath,"exp%s %s",estring,curexpdir);
  writelineToVnmrJ("expn",exppath); */
#endif 
  disp_seq(0);
  disp_exp(0);
  disp_specIndex(0);

  RETURN;
}

/***************************/
int jexp(int argc, char *argv[], int retc, char *retv[])
/***************************/
{ int lval,oldexpnum,clear_flag;
  char estring[MAXPATH];
  char curexpnumber[MAXPATH];
  char path[MAXPATH];
  char exppath[MAXPATH];
  char curmode;
  extern void finddatainfo();
#ifdef VNMRJ
  extern void jcurwin_setexp();
  extern int jcurwin_jexp();
#endif 

#ifdef CLOCKTIME
/* Turn on a clocktime timer */
  (void)start_timer ( jexp_timer_no );
#endif 

  if ((argc == 1) && (retc == 0))
  {
     Werrprintf("usage - jexp(n) or jexpn for n = [1,%d]", MAXEXPS);
     ABORT; 
  }

  oldexpnum = expdir_to_expnum(curexpdir);
  if (strcmp(argv[0], "rjexp")==0)
  {
     if ((argc == 2) && ! strcmp(argv[1],"0") )
     {
        if (oldexpnum)
           reset_exp0( oldexpnum );
     }
     else
     {
        reset_exp0( -1 );
     }
     RETURN; 
  }

  sprintf(curexpnumber, "%d", oldexpnum);

/* Check for request to return current experiment number/name */

  if (retc > 0)
  {
     if ( (retc > 2) || (strcmp(argv[0], "jexp") != 0) || (argc > 1) )
     {
        Werrprintf("usage - jexp:$x,$y to access experiment number and name");
        ABORT; 
     }

     retv[0] = realString( (double)oldexpnum );
     if (retc>1) 
     {
        sprintf(estring,"exp%s",curexpnumber);
        retv[1] = newString(estring);
     }

     RETURN;
  }

/* No JEXP in automation mode */

  if (mode_of_vnmr == AUTOMATION)
  {
     Werrprintf( "'jexp' command not available in automation mode" );
     ABORT;
  }

  clear_flag = TRUE;

/* Establish the (new) experiment number  */

  if ( (argc == 2) && (strcmp(argv[0], "jexp") == 0) )
  {
     strcpy(estring, argv[1]);
     clear_flag = FALSE;
  }
  else if ( (argc == 3) && (strcmp(argv[0], "jexp") == 0) &&
		(strcmp(argv[2],"n") == 0) )
  {
     strcpy(estring, argv[1]);
     clear_flag = FALSE;
  }
  else if (argc != 1)
  {
     Werrprintf("usage - jexp(n) or jexpn for n = [1,%d]", MAXEXPS);
     ABORT; 
  }
  else
  {
     strcpy(estring, &argv[0][4]);
  }

  if ( strcmp(estring, curexpnumber) == 0 )
  {
#ifdef VNMRJ
     jcurwin_setexp( estring, oldexpnum );
#endif 
     RETURN;
  }

  if (atoi(estring) < 1)
  {
     Werrprintf("jexp(%s) is an illegal experiment number", estring); 
     ABORT; 
  }
  else if (atoi(estring) > MAXEXPS)
  {
     Werrprintf("usage - jexp(n) or jexpn for n = [1,%d]", MAXEXPS);
     ABORT; 
  }

  if ( access_exp(exppath, estring) )	/* If this fails, it complains */
     ABORT;

#ifdef VNMRJ
  if (jcurwin_jexp( atoi(estring), oldexpnum ) == 0)
     ABORT;
#endif 

/* Try to lock the experiment.  Abort if not successful, leaving the
   process in the current experiment. */

  if (vnMode[ 0 ] == 'a' && vnMode[ 1 ] == 'u')
    curmode = vnMode[ 1 ];
  else
    curmode = vnMode[ 0 ];
  if ( (lval = lockExperiment( atoi(estring), mode_of_vnmr )) )
  {
    Werrprintf( "experiment %s locked", estring );
    if (lval == 1)					/* Parameters?? */
      Winfoprintf( "acquisition processing active\n" );
    else if (lval == 2)
      Winfoprintf( "background processing active\n" );
    else if (lval == 3)
      Winfoprintf( "foreground processing active\n" );
    ABORT;
  }
  Wturnoff_buttons();
  D_allrelease();
  if (clear_flag) 
     Wclear_graphics();
  // clear multiple spec
  clearMspec();
  set_dpf_flag(0,"");
  set_dpir_flag(0,"");

/*  Current experiment number is 0 if the user failed to lock an
    experiment at bootup.  In this case, do not try to save parameters.
    Do not unlock the old experiment, for JEXP can still fail...	*/

  if (oldexpnum > 0)
  {
    if (flush(0,NULL,0,NULL))	/* flush is a VNMR command, so if it fails, it returns 1 */
    {	/* flush writes out curpar & procpar, but also writes global with old curexp */
      unlockExperiment( atoi(estring), mode_of_vnmr );
      ABORT;
    }
#ifdef VMS
    vms_purge( curexpdir );
#endif 
  }

  disp_status("JEXP    ");
  setfilepaths(0);
  D_getparfilepath(CURRENT, path, exppath);
  P_treereset(CURRENT);		/* clear the tree first */
  if (P_read(CURRENT,path))
    { Werrprintf("experiment does not exist or does not contain proper files");
      P_treereset(CURRENT);		/* clear the tree first */
      D_getparfilepath(CURRENT, path, curexpdir);
      P_read(CURRENT,path);	/* read back current parameters */
      disp_status("        ");
      ABORT;
    }

  D_getparfilepath(PROCESSED, path, exppath);
  P_treereset(PROCESSED);		/* clear the tree first */
  if (P_read(PROCESSED,path))
    Werrprintf("problem loading processed parameters");

/* Now that we are finished with the old experiment, unlock it */

  if (oldexpnum > 0) unlockExperiment( oldexpnum, mode_of_vnmr );
  strcpy(curexpdir,exppath);
  P_setstring(GLOBAL,"curexp",curexpdir,0);
#ifdef VNMRJ
  jcurwin_setexp( estring, oldexpnum );
  aspFrame("clear",0,0,0,0,0); // clear asp display when jexp
#endif 
  execString("select(1)\n");
  finddatainfo();
  disp_status("        ");
  Wsetgraphicsdisplay("");

#ifdef VNMRJ
  sprintf(exppath,"exp%s %s",estring,curexpdir);
  writelineToVnmrJ("expn",exppath);
  update_imagefile(); 
#endif 

#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer ( jexp_timer_no );
#endif 

  p11_checkData();
  RETURN;
}


/******************************/
int cexpCmd(int argc, char *argv[], int retc, char *retv[])
/******************************/
{
  char		estring[MAXPATH],
		msg[MAXPATH],
		path[MAXPATH],
		exppath[MAXPATH],
		epath[MAXPATH];
  FILE		*textfile;
  int		oldexpnum;
  int		doDB = TRUE;


  msg[0] = '\0';
  Wturnoff_buttons();
  if (argc == 2)
  {
     strcpy(estring, argv[1]);
     strcpy(epath, userdir);
  }
#ifdef UNIX
  else if (argc == 3)
  {
     if (strcmp(argv[2], "nodb")==0)
     {
        strcpy(estring, argv[1]);
        strcpy(epath, userdir);
        doDB = FALSE;
     }
     else
     {
        strcpy(epath, argv[1]);
        strcpy(estring, argv[2]);
     }

     if ( access(epath, F_OK) )
     {
        sprintf(msg,"cexp: cannot access %s", epath );
        goto abortCexp;
     }
  }
  else
  {
     sprintf(msg,"usage - cexp(<'expdir'>, n) for n = [1,%d]", MAXEXPS);
     goto abortCexp;
  }
#else 
  else 
  {
     sprintf(msg,"usage - cexp(n) for n = [1,%d]", MAXEXPS);
     goto abortCexp; 
  }
#endif 

  if ( (atoi(estring) < 1) || (atoi(estring) > MAXEXPS) )
  {
     sprintf(msg,"illegal experiment number");
     goto abortCexp;
  }

  strcpy(exppath, epath);
#ifdef UNIX
  strcat(exppath, "/exp");
  strcat(exppath, estring);
#else 
  strcat(exppath, "exp");
  strcat(exppath, estring);
  strcat(exppath, ".dir");
#endif 

  oldexpnum = expdir_to_expnum(curexpdir);
/*  makeuser -
    cp $vnmrsystem/fidlib/fid1d.fid/text    exp1/.
    cp $vnmrsystem/fidlib/fid1d.fid/procpar exp1/.
    cp $vnmrsystem/fidlib/fid1d.fid/procpar exp1/curpar
*/
  if ( access(exppath, R_OK) == 0)
  {
     sprintf(msg,"experiment %s exists", exppath);
     goto abortCexp;
  }
  else if (strcmp( userdir, epath ) != 0)
  {
     char exppath2[ MAXPATH ];

     strcpy( exppath2, userdir );
     strcat( exppath2, "/exp");
     strcat( exppath2, estring);
     if (access(exppath2, R_OK ) == 0)
     {
        sprintf(msg,"experiment %s exists, cannot use alternate directory", exppath2);
        goto abortCexp;
     }
  }

#ifdef VMS
  make_vmstree(exppath, exppath, MAXPATHL);
#endif 
#ifdef SIS
  if (mkdir(exppath,0755))
#else 
  if (mkdir(exppath,0777))
#endif 
  {
     sprintf(msg,"cannot create %s", exppath);
     goto abortCexp;
  }

  strcpy(path, exppath);
#ifdef UNIX
  strcat(path, "/acqfil");
#else 
  vms_fname_cat(path, "[.acqfil]");
#endif 
#ifdef SIS
  if ( mkdir(path, 0755) )
#else 
  if ( mkdir(path, 0777) )
#endif 
  {
     sprintf(msg,"cannot create %s", path);
     goto abortCexp;
  }

  strcpy(path, exppath);
#ifdef UNIX
  strcat(path, "/datdir");
#else 
  vms_fname_cat(path, "[.datdir]");
#endif 
#ifdef SIS
  if ( mkdir(path, 0755) )
#else 
  if ( mkdir(path, 0777) )
#endif 
  {
     sprintf(msg,"cannot create %s", path);
     goto abortCexp;
  }

  if (autoexp || (oldexpnum == 0) )
  {
/*
     cp $vnmrsystem/fidlib/fid1d.fid/procpar exp1/.
     cp $vnmrsystem/fidlib/fid1d.fid/procpar exp1/curpar
*/
     strcpy(epath,systemdir);
     strcat(epath,"/fidlib/fid1d.fid/procpar");
     if ( access(epath, R_OK) != 0)
     {
        sprintf(msg,"%s: %s does not exist", argv[0], epath);
        goto abortCexp;
     }
     else
     {
        char cmdstr[MAXPATH*3];

        strcpy(cmdstr,"/bin/cp ");
        strcat(cmdstr,epath);
        strcat(cmdstr," ");
        strcat(cmdstr,exppath);
        strcat(cmdstr,"/curpar");
        system(cmdstr);

        strcpy(cmdstr,"/bin/cp ");
        strcat(cmdstr,epath);
        strcat(cmdstr," ");
        strcat(cmdstr,exppath);
        strcat(cmdstr,"/procpar");
        system(cmdstr);
     }

  }
  else
  {
     double tProcdim;

     setfilepaths(0);
     D_getparfilepath(CURRENT, path, exppath);
     P_getreal(CURRENT,"procdim", &tProcdim, 1);
     P_setreal(CURRENT, "procdim", 0.0, 1);
     if ( P_save(CURRENT, path) )
     {
        sprintf(msg,"problem copying current parameters");
        P_setreal(CURRENT, "procdim", tProcdim, 1);
        goto abortCexp;
     }
     P_setreal(CURRENT, "procdim", tProcdim, 1);

     D_getparfilepath(PROCESSED, path, exppath);
     P_getreal(PROCESSED,"procdim", &tProcdim, 1);
     P_setreal(PROCESSED, "procdim", 0.0, 1);
     if ( P_save(PROCESSED, path) )
     {
        sprintf(msg,"problem copying processed parameters");
        P_setreal(PROCESSED, "procdim", tProcdim, 1);
        goto abortCexp;
     }
     P_setreal(PROCESSED, "procdim", tProcdim, 1);
  }

  setfilepaths(getactivefile());
  strcpy(path, exppath);
#ifdef UNIX
  strcat(path, "/text");
#else 
  strcat(path, "text");
#endif 

  if ( (textfile = fopen(path,"w")) == NULL )
  {
     sprintf(msg,"cannot create text file");
     goto abortCexp;
  }
  else
  {
     fprintf(textfile, "new experiment\n");
     fclose(textfile);
  }

  strcpy(path, userdir);
#ifdef UNIX
  strcat(path, "/exp");
  strcat(path, estring);
#else 
  strcat(path, "exp");
  strcat(path, estring);
  strcat(path, ".dir");
#endif 

  if ((argc==3) && (doDB))
  {
     if ( symlink(&exppath[0], &path[0]) )
     {
        sprintf(msg,"cannot create symbolic link to experiment");
        goto abortCexp;
     }
  }

#ifdef VNMRJ
  if ((doDB) && (mode_of_vnmr != AUTOMATION))
  {
      char tmppath[3*MAXPATH];
      if (!autoexp)
      {
         sprintf(tmppath,"%s workspace \"exp%s\" \"%s\"",UserName,estring,exppath);
         if (strlen(tmppath) > 3*MAXPATH)
            Winfoprintf("WARNING: %s cannot add file to database, filename too long!\n",argv[0]);
         else
            writelineToVnmrJ("SVF",tmppath); /* argv[0] instead of SVF */
      }
      autoexp = 0;
  }
#endif 

  sprintf(msg,"experiment %s has been created", estring);
  if (retc)
  {
     retv[ 0 ] = intString( 1 );
     if (retc > 1)
     {
        retv[ 1 ] = newString( msg );
     }
  }
  else
  {
      Winfoprintf(msg);
  }
  RETURN;

abortCexp:

  if (retc)
  {
     retv[ 0 ] = intString( 0 );
     if (retc > 1)
     {
        retv[ 1 ] = newString( msg );
     }
     RETURN;
  }
  Werrprintf(msg);
  ABORT;
}

/*******************/
static void m_unlink(char *path)
/*******************/
{ if (unlink(path))
    if (debug1) Wscrprintf("cannot remove file %s\n",path);
}

/*************************/
static void remove_data(char *exppath)
/*************************/
{
   char		path[MAXPATH];
   int		i;
   extern void  removedatafiles();


   D_getfilepath(D_USERFILE, path, exppath);
   m_unlink(path);
   D_getfilepath(D_DATAFILE, path, exppath);
   m_unlink(path);
   D_getfilepath(D_PHASFILE, path, exppath); 
   m_unlink(path);

   for (i = 1; i < (MAX_DATA_BUFFERS + 1); i++)
   {
      setfilepaths(i);
      removedatafiles(i, exppath);
   }

   setfilepaths(0);	/* reset to main data file */
}

/*  Remove files and subdirectories associated with 3D processing.
 *
 *      $curexp/datadir3d
 *      $curexp/info
 *      $curexp/auto
 *      $curexp/coef							*/

/***********************/
void remove_3d(char *exppath )
/***********************/
{
	char	subdirpath[ MAXPATH+10 ], remove_it_cmd[ MAXPATH+10 ];

/*  Remove "datadir3d" subdirectory  */

	strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
	strcat( &subdirpath[ 0 ], "/datadir3d" );
	strcpy( &remove_it_cmd[ 0 ], "rm -rf " );
#else 
	vms_fname_cat( &subdirpath[ 0 ], "[.datadir3d]" );
	strcpy( &remove_it_cmd[ 0 ], "rm_recur " );
#endif 
	strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
	system( &remove_it_cmd[ 0 ] );

/*  Remove "info" subdirectory  */

	strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
	strcat( &subdirpath[ 0 ], "/info" );
	strcpy( &remove_it_cmd[ 0 ], "rm -rf " );
#else 
	vms_fname_cat( &subdirpath[ 0 ], "[.info]" );
	strcpy( &remove_it_cmd[ 0 ], "rm_recur " );
#endif 
	strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
	system( &remove_it_cmd[ 0 ] );

/*  Remove "auto" file  */

	strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
	strcat( &subdirpath[ 0 ], "/auto" );
	strcpy( &remove_it_cmd[ 0 ], "rm -f " );
#else 
	strcat( &subdirpath[ 0 ], "auto;" );
	strcpy( &remove_it_cmd[ 0 ], "delete " );
#endif 
	strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
	system( &remove_it_cmd[ 0 ] );

/*  Remove "coef" file  */

	strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
	strcat( &subdirpath[ 0 ], "/coef" );
	strcpy( &remove_it_cmd[ 0 ], "rm -f " );
#else 
	strcat( &subdirpath[ 0 ], "coef;" );
	strcpy( &remove_it_cmd[ 0 ], "delete " );
#endif 
	strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
	system( &remove_it_cmd[ 0 ] );
}

/************************************/
void remove_shapelib(char *exppath )
/************************************/
{
	char	subdirpath[ MAXPATH+10 ], remove_it_cmd[ MAXPATH+10 ];

/*  Remove "shapelib" subdirectory  */

	strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
	strcat( &subdirpath[ 0 ], "/shapelib" );
	strcpy( &remove_it_cmd[ 0 ], "rm -rf " );
#endif 
	strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
	system( &remove_it_cmd[ 0 ] );
}

/********************************/
int delexp(int argc, char *argv[], int retc, char *retv[])
/********************************/
{
  char	estring[MAXPATH],
	exppath[MAXPATH],
	userexppath[MAXPATH],
	msg[MAXPATH],
	rm_r_cmd[MAXPATH+7];		/* strlen( "rm -rf " ) = 7 */
  int lval;
  int doDB = TRUE;
  int autoDel=0;

  if ((argc == 2) && !(strcmp(argv[1], "auto")))
  {
     autoDelExp=1;
     RETURN;
  }
  if ((argc == 1) && !(strcmp(argv[0], "autoDELEXP")))
  {
     autoDel=1;
     sprintf(estring,"%d", expdir_to_expnum( curexpdir ) );
  }
  else
  {
  msg[0]= '\0';
  Wturnoff_buttons();
  if ((argc==2) || (argc==3))
  {
     strcpy(estring, argv[1]);
     if ((argc == 3) && (!(strcmp(argv[2], "nodb"))))
        doDB = FALSE;
  }
  else 
  {
     sprintf(msg,"usage - delexp(n) where n is the experiment number");
     goto abortDelexp; 
  }
  }

  if ( (atoi(estring) < 2) || (atoi(estring) > MAXEXPS) )
  {
     sprintf(msg,"illegal experiment number");
     goto abortDelexp;
  }

  strcpy(exppath, userdir);

  strcpy(userexppath, userdir);
#ifdef UNIX
  strcat(exppath,"/exp");
  strcat(exppath, estring);
  strcat(userexppath,"/exp");
  strcat(userexppath, estring);
#else 
  vms_fname_cat(exppath,"[.exp");
  strcat(exppath, estring);
  strcat(exppath, "]");
  vms_fname_cat(userexppath,"[.exp");
  strcat(userexppath, estring);   
  strcat(userexppath, "]");
#endif 

  if ( ! autoDel && (strcmp(curexpdir, userexppath) == 0) )
  {
     sprintf(msg,"cannot delete currently joined experiment");
     goto abortDelexp;
  }

  if (access( userexppath, F_OK ) != 0)
  {
     sprintf(msg,"cannot delete experiment %s, does not exist", estring);
     goto abortDelexp;
  }

  if (isSymLink( &exppath[ 0 ] ) == 0) {
     int ival;

     ival = follow_link( &exppath[ 0 ], &exppath[ 0 ], sizeof( exppath ) - 1 );
     if (ival != 0)
     {
        sprintf(msg,"cannot follow symbolic link for experiment %s", estring );
        goto abortDelexp;
     }
  }

  if (( lval = lockExperiment( atoi(estring), mode_of_vnmr )) )
  {
    if (lval == 1)					/* Parameters?? */
      sprintf(msg,"Cannot delete experiment %s. Acquisition processing active",
                   estring );
    else if (lval == 2)
      sprintf(msg,"Cannot delete experiment %s. Background processing active",
                   estring );
    else
      sprintf(msg,"Cannot delete experiment %s. Processing active", estring );
    goto abortDelexp;
  }
  lval = is_exp_acquiring( atoi(estring) );	/* A value of 0 means nothing */
						/* is active.  A value of -1  */
						/* means an error occured in  */
						/* is exp active, in which    */
						/* case we proceed.	      */
  unlockExperiment( atoi(estring), mode_of_vnmr );
  if (lval > 0) {
     sprintf(msg,
    "Cannot delete experiment %s. Acquisition is active or queued", estring);
      goto abortDelexp;
  }
  if ( strcmp(exppath, userexppath) != 0 )
     unlink(userexppath);	/* not a local experiment */

#ifdef UNIX
  strcpy( &rm_r_cmd[ 0 ], "rm -rf " );
#else 
  strcpy( &rm_r_cmd[ 0 ], "rm_recur " );
#endif 
  strcat( &rm_r_cmd[ 0 ], exppath );
  system( &rm_r_cmd[ 0 ] );

  if (access( exppath, F_OK ) == 0)
  {
     sprintf(msg,"Cannot remove experiment %s", estring );

     strcpy( &rm_r_cmd[ 0 ], "ls -l " );
     strcat( &rm_r_cmd[ 0 ], exppath );

     Wscrprintf( " Contents:\n" );
     system( &rm_r_cmd[ 0 ] );
     goto abortDelexp;
  }

  if (autoDel)
     RETURN;
#ifdef VNMRJ
  if (doDB) 
  {
    char jstr[MAXPATH*3];
    sprintf(jstr,"%s workspace \"%s\"",VnmrJHostName,exppath);
    writelineToVnmrJ("RmFile",jstr);
  }
#endif 

  sprintf(msg,"experiment %s has been deleted", estring);
  if (retc)
  {
     retv[ 0 ] = intString( 1 );
     if (retc > 1)
     {
        retv[ 1 ] = newString( msg );
     }
  }
  else
  {
      Winfoprintf(msg);
  }
  RETURN;

abortDelexp:

  if (autoDel)
     RETURN;
  if (retc)
  {
     retv[ 0 ] = intString( 0 );
     if (retc > 1)
     {
        retv[ 1 ] = newString( msg );
     }
     RETURN;
  }
  Werrprintf(msg);
  ABORT;
}

int svprocpar(int fidflag, char *filepath)
{
  char path[MAXPATH];
  int  r,tree,ival,diskIsFull;

  D_getparfilepath(PROCESSED, path, filepath);
  if (fidflag)
    { tree = TEMPORARY;
      P_treereset(TEMPORARY);		/* clear the tree first */
      P_copy(PROCESSED,TEMPORARY);
      P_copygroup(CURRENT,TEMPORARY,G_DISPLAY);
      P_copygroup(CURRENT,TEMPORARY,G_PROCESSING);
    }
  else tree = CURRENT;

  if ( (r=P_save(tree,path)) )
    {
      ival = isDiskFullSize( filepath, 0, &diskIsFull );
      if (ival == 0 && diskIsFull)
      {
         Werrprintf("problem storing parameters in %s: disk is full", filepath);
      }
      else
       Werrprintf("problem storing parameters in %s",path);
      disp_status("        ");
      ABORT;
    }
  if (fidflag)
    P_treereset(TEMPORARY);		/* clear the tree again */

/*  Remember the original path is constrained
    to MAXPATH-32 or fewer letters.		*/

  RETURN;
}

// this command will downsize fid by a factor (0 < factor < 1.0) 
// curexp/acqfil/fid and curexp/acqfil/procpar will be overwriten
// if fid is linked, the linki, make_copy_fidfile will be called to
// replace the link with a copy, so the original fid won;t be overwriten.
// If the first argument is > 1, it is treated as the new np value.
extern int make_copy_fidfile();
extern int D_downsizefid(int newnp, char *datapath);
extern int D_zerofillfid(int newnp, char *datapath);
extern int D_leftshiftfid(int lsfid, char *datapath, int *newnp);
extern int D_scalefid(double scaling, char *datapath);

int downsizefid(int argc, char *argv[], int retc, char *retv[])
{
    dfilehead fidhead;
    int          res,oldnp,newnp,nt,nb;
    char         filepath[MAXPATH], procparpath[MAXPATH];
    double factor, at;
    int curexpFid = 1;

    if(argc>1)
    {
       factor = atof(argv[1]);
       if (argc > 2)
       {
          curexpFid = 0;
          strcpy(filepath,argv[2]);
          if ( access(filepath,R_OK|W_OK) )
          {
	     Winfoprintf("downsizefid(%s,%s) data does not exist or has wrong permissions",argv[1],argv[2]); 
	     ABORT;
          }
       }
    }
    else
    {
       factor = 0.5;
    }
    if(factor <= 0) {
	Winfoprintf("Usage: downsizefid(factor), where 0 < factor < 1.0"); 
	ABORT;
    }

    if (curexpFid)
    {
    // this will copy the fid if acqfil/fid is linked
       make_copy_fidfile();
 
       if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
       {
          D_error(res);
          ABORT;
       }
 
   // make procparpath
      strcpy(procparpath,filepath);
      procparpath[strlen(filepath)-3] = '\0';
      strncat(procparpath,"procpar",7);
   }

   D_close(D_USERFILE);
   if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
   {
      if (res != D_IS_OPEN)
      {
         D_error(res);
         ABORT;
      } else {
	D_gethead(D_USERFILE, &fidhead);
      }
   }

    nb = fidhead.nblocks;
    nt = fidhead.ntraces;
    oldnp = fidhead.np; 
    if (factor <= 1.0)
       newnp = factor*oldnp;
    else
    {
       if (factor >= oldnp)
       {
          Werrprintf("downsizefid(np): np must be less then original np");
          ABORT;
       }
       newnp = factor;
    }
    newnp -= (newnp % 2); // make sure it is a even number 
    D_close(D_USERFILE);
    if(D_downsizefid(newnp,filepath)) { 
	Werrprintf("cannot downsize fid file %s",filepath);
	ABORT;
    }

    // now adjust np, at parameters and save procpar file
    if (curexpFid)
    {
       P_getreal(PROCESSED,"at",&at,1);
       at *= ((double)newnp/(double)oldnp);
       P_setreal(CURRENT, "np", newnp, 1);
       P_setreal(PROCESSED, "np", newnp, 1);
       P_setreal(CURRENT, "at", at, 1);
       P_setreal(PROCESSED, "at", at, 1);
       P_setreal(CURRENT, "arraydim", nb*nt, 1);
       P_setreal(PROCESSED, "arraydim", nb*nt, 1);

       if (P_save(PROCESSED,procparpath))
       { Werrprintf("cannot save procpar file %s",procparpath);
	  ABORT;
       }
    }
    
    RETURN;
}

int zerofillfid(int argc, char *argv[], int retc, char *retv[])
{
    dfilehead fidhead;
    int    res,oldnp,newnp;
    char   filepath[MAXPATH], procparpath[MAXPATH];
    double at;
    int curexpFid = 1;

    newnp=0;
    if(argc>1)
    {
       newnp = atof(argv[1]);
       if (argc > 2)
       {
          curexpFid = 0;
          strcpy(filepath,argv[2]);
          if ( access(filepath,R_OK|W_OK) )
          {
	     Winfoprintf("zerofillfid(%s,%s) data does not exist or has wrong permissions",argv[1],argv[2]); 
	     ABORT;
          }
       }
    }

    if (curexpFid)
    {
       // this will copy the fid if acqfil/fid is linked
       make_copy_fidfile();
 
      if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
      {
         D_error(res);
         ABORT;
      }
 
   // make procparpath
      strcpy(procparpath,filepath);
      procparpath[strlen(filepath)-3] = '\0';
      strncat(procparpath,"procpar",7);
   }

   D_close(D_USERFILE);
   if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
   {
      if (res != D_IS_OPEN)
      {
         D_error(res);
         ABORT;
      } else {
	D_gethead(D_USERFILE, &fidhead);
      }
   }

    oldnp = fidhead.np; 
    if ( ! newnp)
    {
       newnp = 16;
       while (newnp <= oldnp)
          newnp *= 2;
    }
    if ( (newnp) && (newnp <= oldnp) )
    {
	Winfoprintf("Usage: zerofillfid(newnp), where newnp > np"); 
	RETURN;
    }
    newnp -= (newnp % 2); // make sure it is a even number 
    D_close(D_USERFILE);
    if(D_zerofillfid(newnp, filepath)) { 
	Werrprintf("cannot zerofill fid file %s",filepath);
	ABORT;
    }

    if (curexpFid)
    {
       // now adjust np, at parameters and save procpar file
       P_getreal(PROCESSED,"at",&at,1);
       at *= ((double)newnp/(double)oldnp);
       P_setreal(CURRENT, "np", newnp, 1);
       P_setreal(PROCESSED, "np", newnp, 1);
       P_setreal(CURRENT, "at", at, 1);
       P_setreal(PROCESSED, "at", at, 1);

       if (P_save(PROCESSED,procparpath))
       { Werrprintf("cannot save procpar file %s",procparpath);
	  ABORT;
       }
    }
    RETURN;
}

int leftshiftfid(int argc, char *argv[], int retc, char *retv[])
{
   int    res,newnp;
   char   filepath[MAXPATH], procparpath[MAXPATH];
   double at;
   int curexpFid = 1;
   int lsFID;

   lsFID=0;
   if(argc>1)
   {
      lsFID = atoi(argv[1]);
      if (argc > 2)
      {
         curexpFid = 0;
         strcpy(filepath,argv[2]);
         if ( access(filepath,R_OK|W_OK) )
         {
            Winfoprintf("%s(%s,%s) data does not exist or has wrong permissions",argv[0],argv[1],argv[2]); 
            ABORT;
         }
      }
   }
   else
   {
      Winfoprintf("lsfid requires number of points to shift as the first argument"); 
      ABORT;
   }
   if (lsFID == 0)
   {
      Winfoprintf("lsfid requested zero points to shift"); 
      RETURN;
   }

   if (curexpFid)
   {
       // this will copy the fid if acqfil/fid is linked
       make_copy_fidfile();
 
      if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
      {
         D_error(res);
         ABORT;
      }
 
   }
   // make procparpath
   strcpy(procparpath,filepath);
   procparpath[strlen(filepath)-3] = '\0';
   strncat(procparpath,"procpar",7);

   D_close(D_USERFILE);
   if (D_leftshiftfid(lsFID, filepath, &newnp))
   { 
      Werrprintf("cannot zerofill fid file %s",filepath);
      ABORT;
   }

   if (curexpFid)
   {
      // now adjust np, at parameters and save procpar file
      P_getreal(PROCESSED,"at",&at,1);
      at *= ((double)newnp/(double)(newnp + (lsFID*2)));
      P_setreal(CURRENT, "np", newnp, 1);
      P_setreal(PROCESSED, "np", newnp, 1);
      P_setreal(CURRENT, "at", at, 1);
      P_setreal(PROCESSED, "at", at, 1);
      P_setactive(CURRENT,"lsfid",0);
      P_setactive(PROCESSED,"lsfid",0);

      if (P_save(PROCESSED,procparpath))
      { Werrprintf("cannot save procpar file %s",procparpath);
       ABORT;
      }
   }
   else if ( ! access(procparpath,R_OK|W_OK) )
   {
      P_treereset(TEMPORARY);	/* clear the tree first */
      if ( ! P_read(TEMPORARY,procparpath))
      {
         P_getreal(TEMPORARY,"at",&at,1);
         at *= ((double)newnp/(double)(newnp + (lsFID*2)));
         P_setreal(TEMPORARY, "np", newnp, 1);
         P_setreal(TEMPORARY, "at", at, 1);
         P_setactive(TEMPORARY,"lsfid",0);
         P_save(TEMPORARY,procparpath);
         P_treereset(TEMPORARY);	/* clear the tree first */
      }
   }
   RETURN;
}

int scalefid(int argc, char *argv[], int retc, char *retv[])
{
   int    res;
   char   filepath[MAXPATH], procparpath[MAXPATH];
   double scaling;
   int curexpFid = 1;

   scaling=1.0;
   if(argc>1)
   {
      scaling = atof(argv[1]);
      if (argc > 2)
      {
         curexpFid = 0;
         strcpy(filepath,argv[2]);
         if ( access(filepath,R_OK|W_OK) )
         {
            Winfoprintf("%s(%s,%s) data does not exist or has wrong permissions",argv[0],argv[1],argv[2]); 
            ABORT;
         }
      }
   }
   else
   {
      Winfoprintf("%s requires FID scaling factor as the first argument", argv[0]); 
      ABORT;
   }
   if (scaling == 1.0)
   {
      Winfoprintf("%s requested 1.0 as scaling factor", argv[0]); 
      RETURN;
   }

   if (curexpFid)
   {
       // this will copy the fid if acqfil/fid is linked
       make_copy_fidfile();
 
      if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
      {
         D_error(res);
         ABORT;
      }
      D_trash(D_USERFILE);
 
   }
   // make procparpath
   strcpy(procparpath,filepath);
   procparpath[strlen(filepath)-3] = '\0';
   strncat(procparpath,"procpar",7);

   if (D_scalefid(scaling, filepath))
   { 
      Werrprintf("cannot scale fid file %s",filepath);
      ABORT;
   }

   if (curexpFid)
   {
      double dval;
      if (P_getreal(CURRENT, "scalefid", &dval, 1))
      {
         P_creatvar(CURRENT, "scalefid", T_REAL);
         P_setgroup(CURRENT,"scalefid",G_PROCESSING);
         dval = scaling;
      }
      else
      {
         dval *= scaling;
      }
      P_setreal(CURRENT, "scalefid", dval, 1);
      if (P_setreal(PROCESSED, "scalefid", dval, 1))
      {
         P_creatvar(PROCESSED, "scalefid", T_REAL);
         P_setgroup(PROCESSED,"scalefid",G_PROCESSING);
         P_setreal(PROCESSED, "scalefid", dval, 1);
      }
      if (P_save(PROCESSED,procparpath))
      { Werrprintf("cannot save procpar file %s",procparpath);
       ABORT;
      }
   }
   else if ( ! access(procparpath,R_OK|W_OK) )
   {
      P_treereset(TEMPORARY);	/* clear the tree first */
      if ( ! P_read(TEMPORARY,procparpath))
      {
         double dval;
         if (P_getreal(TEMPORARY, "scalefid", &dval, 1))
         {
            P_creatvar(TEMPORARY, "scalefid", T_REAL);
            P_setgroup(TEMPORARY,"scalefid",G_PROCESSING);
            dval = scaling;
         }
         else
         {
            dval *= scaling;
         }
         P_setreal(TEMPORARY, "scalefid", dval, 1);
         P_save(TEMPORARY,procparpath);
         P_treereset(TEMPORARY);	/* clear the tree first */
      }
   }
   RETURN;
}

int replacetraces(int argc, char *argv[], int retc, char *retv[])
{
    dfilehead phasehead;
    int    res,nb,nt,np, k, numTraces;
    char   filepath[MAXPATH], fdffile[MAXPATH];
    
    if(argc>1) {
	strcpy(fdffile,argv[1]);
	argc--;
	argv++;
    } else {
	Winfoprintf("Usage: replacetraces(fdffile,rois)"); 
	RETURN;
    }

    if ( (res = D_gethead(D_PHASFILE, &phasehead)) )
     { // phasefile is not open, try to open curexpdir+'/datdir/phasefile

        if (res == D_NOTOPEN)
        {
           if ( (res = D_getfilepath(D_PHASFILE, filepath, curexpdir)) )
           {
              D_error(res);
	      ABORT;
           }

           if( (res = D_open(D_PHASFILE, filepath, &phasehead))) { // open phase file
              D_error(res);
	      ABORT;
	   }
        }
     }

    nb = phasehead.nblocks;
    nt = phasehead.ntraces;
    np = phasehead.np;
    // revflag=1 if trace='f1'
//Winfoprintf("revflag,nb,nt,np,specperblock,nblocks,%d %d %d %d %d %d",revflag,nb,nt,np,specperblock,nblocks);

    numTraces = loadFdfSpec(fdffile, "fdfspec");

    k=0;
    while(argc>2) {
	int first=atoi(argv[1])-1;
	int last=atoi(argv[2])-1;
	if(first<0) first=0;
	if(last>=nb*nt) last=nb*nt-1;
	int i;
	for(i=first; i<=last && k<numTraces; i++) { 
	       float *data=aipGetTrace("fdfspec", k, 1.0, np);
	       float *dptr = gettrace(i, 0);
		if(data && dptr) {
		   (void)memcpy(dptr, data, np*sizeof(float));
		}
		k++;
	}
	argc--;
	argv++;
	argc--;
	argv++;
    }
    RETURN;
}

/**************************/
int svf(int argc, char *argv[], int retc, char *retv[])
/**************************/
/* svf stores fid */
/* svp stores parameters */
{ char filepath[MAXPATH],path[MAXPATH];
  char origpath[MAXPATH],destpath[MAXPATH];
  int r,fidflag;
  int  diskIsFull;
  int  ival;
  char *name;
  char systemcall[2*MAXPATH+8];
  int svf_update;
  int svf_nofid;		/* saving FID, but do not copy FID.  See below */
  int nolog, no_arch, i;
  int doDB;
  int force;
  int opt;
  vInfo info;
  char *ptmp;
#ifdef SIS
  int permission = 0755; 
#else 
  int permission = 0777; 
#endif 

  Wturnoff_buttons();
  D_allrelease();
  if (strcmp(argv[0],"SVP")==0)
    fidflag = 0;
  else
    fidflag = 1;
  nolog = FALSE;
  no_arch = TRUE;
  doDB = TRUE;
  force = FALSE;
  opt = FALSE;
  if ( !strcmp(argv[0],"SVF") )
  {  for (i = 1; i<argc; i++)
     {
        if (!strcmp(argv[i],"nolog") ) nolog = TRUE;
        if (!strcmp(argv[i],"arch") ) no_arch = FALSE;
     }
  }
  for (i = 1; i<argc; i++)
  {
     if (!strcmp(argv[i],"nodb") ) doDB = FALSE;
     if (!strcmp(argv[i],"force") ) force = TRUE;
     if (!strcmp(argv[i],"opt") ) opt = TRUE;
  }
  if (nolog) argc--;
  if (!no_arch) argc--;
  if (!doDB) argc--;
  if (force) argc--;
  if (opt) argc--;

  if (argc<2)
    { W_getInput("File name (enter name and <return>)? ",filepath,MAXPATH-1);
      name = filepath;
      if (strlen(name)==0)
        { Werrprintf("No file name given, command aborted");
          ABORT;
        }
    }
  else if (argc!=2)
    { Werrprintf("usage - %s(filename)",argv[0]);
      ABORT;
    }
  else
    name = argv[1];
  if ((int) strlen(name) >= (MAXPATH-32))
    { Werrprintf("file path too long");
      ABORT;
    }
  if (verify_fname(name))
    { Werrprintf( "file path '%s' not valid", name );
      ABORT;
    }

  strcpy(filepath,name);
  if (fidflag)
    { if (isPar(filepath))
        { Werrprintf("illegal file name %s with extension .par",filepath);
          ABORT;
        }
      // if file name extension is not specified, .fid will be saved regardless part11System.
      if ( strlen(filepath) < 5)
      {
         strcat(filepath,".fid");
      }
      else
      {
         ptmp = filepath +strlen(filepath)-4; 
         if ( strcmp(ptmp,".fid") && strcmp(ptmp,".rec") && strcmp(ptmp,".REC"))
         {
 	    strcat(filepath,".fid");
         }
      }

      // if current data is not SE data, svr_FDA will save .rec even though 
      // given extension is .REC.
      if (isFDARec(filepath) || isRec(filepath))
	{
	   svr_FDA(argc, argv, retc, retv);
	   RETURN;
	}
    }
  else
    { if (isFid(filepath))
        { Werrprintf("illegal file name %s with extension .fid",filepath);
          ABORT;
        }
      else if (!isPar(filepath))
        strcat(filepath,".par");
    }
#ifdef VMS
    filepath[ strlen( filepath ) - 4 ] = '_';		/*  Replace '.' with '_'  */
    make_vmstree(filepath,filepath,MAXPATHL-20);	/*  Make into VMS directory  */
#endif 
  disp_status("SVF/SVP ");
  svf_update = 0;
  svf_nofid = 0;

  if (mkdir(filepath,permission))
    { if (errno==EEXIST)
        {
          int rmDir;

          rmDir = FALSE;
          if (force)
          {
            rmDir = TRUE;
          }
          else
          {
             char answer[16];
             W_getInput("File exists, overwrite (enter y or n <return>)? "
               ,answer,15);
             if ((strcmp(answer,"y")==0) || (strcmp(answer,"yes")==0))
               rmDir = TRUE;
          }
          if (rmDir)
	  {

	/*
	 * Problem can result if $curexp/acqfil/fid is a symbolic link
	 * to the fid file in this directory.  If this directory were
	 * removed, the symbolic links would point to a non-existant file.
	 * The routine `fid_is_link' returns 1 if this is the situaton.
	 */

	    if (fid_is_link( filepath ) == 0) {
                svf_update = 1;
                sprintf(systemcall,"rm -rf %s",check_spaces(filepath,chkbuf,MAXPATH+2));
                system(systemcall);
                if (mkdir(filepath,permission)) {
                       Werrprintf("cannot overwrite existing file: %s",filepath);
                       disp_status("        ");
                       ABORT;
                }
            }  

	/*
	 * Next statement executes if $curexp/acqfil/fid is a link to the
	 * FID file in this directory.  We do not remove the directory,
	 * but set the SVF-NO-FID flag so as to prevent copying the FID
	 * since that's already been done!
	 */
	    else
	      svf_nofid = 1;
	  }
	  else
	  {
            disp_status("        ");
            ABORT;
	  }
        }
      else
        { Werrprintf("cannot create file %s",filepath);
          disp_status("        ");
          ABORT;
        }
    }

/* create and set vnmrj time parameter */
  currentDate(origpath, MAXPATH);
  if (P_getVarInfo(CURRENT,"time_saved",&info) == -2)
     P_creatvar(CURRENT,"time_saved",T_STRING);
  P_setstring(CURRENT,"time_saved",origpath,1);

/*
  Wgetgraphicsdisplay(disCmd, 20);
  if (P_getVarInfo(CURRENT,"disCmd",&info) == -2)
     P_creatvar(CURRENT,"disCmd",T_STRING);
  P_setstring(CURRENT,"disCmd",disCmd,1);
*/

  if (svprocpar(fidflag,filepath) != 0)
      ABORT;

/*  Remember the original path is constrained
    to MAXPATH-32 or fewer letters.		*/

#ifdef UNIX
  sprintf( &origpath[ 0 ], "%s/text", curexpdir );
  sprintf( &destpath[ 0 ], "%s/text", filepath );
#else 
  sprintf( &origpath[ 0 ], "%stext", curexpdir );
  sprintf( &destpath[ 0 ], "%stext", filepath );
#endif 

  ival = isDiskFullFile( filepath, &origpath[ 0 ], &diskIsFull );
  if (ival == 0 && diskIsFull) {
      Werrprintf( "svf: problem saving text in %s: disk is full", filepath );
      ABORT;
  }

  r = copy_file_verify( &origpath[ 0 ], &destpath[ 0 ] );
  if (r != 0) {
      report_copy_file_error( r, "text" );
      ABORT;
  }

/*  The FID flag is 0 if the command was SVP.
    The SVF-NO-FID flag is not 0 if the FID in the current experiment
    is a link to the FID in the directory this command is accessing.	*/

  if (fidflag == 0 || svf_nofid != 0) {
#ifdef VNMRJ
    if (doDB)
    {
      char tmppath[3*MAXPATH];
      if (filepath[0]=='/')
        strcpy(tmppath,filepath);
      else
      {
        if (getcwd(tmppath,MAXPATH) == NULL)
          strcpy(tmppath,"");
        strcat(tmppath,"/");
        strcat(tmppath,filepath);
        strncpy(filepath,tmppath,MAXPATH);
	i = strlen(filepath);
	if (filepath[i] != '\0')
	  filepath[i] = '\0';
      }
      ptmp = "";
      for (i=strlen(filepath); i>0; i--)
      {
	ptmp = &filepath[i-1];
	if (*ptmp == '/')
	{
	  ptmp++;
	  break;
	}
      }
      sprintf(tmppath,"%s vnmr_par \"%s\" \"%s\"",UserName,ptmp,filepath);
      if (strlen(tmppath) > 3*MAXPATH)
         Winfoprintf("WARNING: %s cannot add file to database, filename too long!\n",argv[0]);
      else
         writelineToVnmrJ(argv[0],tmppath);
    }
#endif 

      if(part11System) {
        sprintf( &origpath[ 0 ], "%s/acqfil", curexpdir );
        p11_writeAuditTrails_S("jexp:svp", origpath, filepath);
      }

      disp_status("        ");
      RETURN;
  }

#ifdef UNIX
  sprintf( &origpath[ 0 ], "%s/acqfil/fid", curexpdir );
  sprintf( &destpath[ 0 ], "%s/fid", filepath );
#else 
  strcpy(path,curexpdir);
  vms_fname_cat(path,"[.acqfil]");
  sprintf( &origpath[ 0 ], "%sfid", path );
  sprintf( &destpath[ 0 ], "%sfid", filepath );
#endif 

  ival = isDiskFullFile( filepath, &origpath[ 0 ], &diskIsFull );
  if (ival == 0 && diskIsFull) {
      Werrprintf( "svf: problem saving fid data in %s: disk is full", filepath );
      ABORT;
  }

  r = copy_file_verify( &origpath[ 0 ], &destpath[ 0 ] );
  if (r != 0) {
      report_copy_file_error( r, "fid" );
      ABORT;
  }

  /* Copy sampling schedules */
  sprintf(origpath , "%s/acqfil/sampling.sch",curexpdir);
  if ( ! access(origpath,F_OK))
  {
     char systemcall[4*MAXPATH];

     sprintf(destpath, "%s/sampling.sch", filepath );
     sprintf(systemcall,"cp %s %s", origpath, destpath);
     system(systemcall);
  }
/* When the FID is already a link then so is the 'log' file (see rt)
   So this is the place to copy the 'log' file or we would already
   have returned from this call */

  strcpy(path,curexpdir);
#ifdef UNIX
  strcat(path,"/acqfil/log");
#else 
     vms_fname_cat(path,"[.acqfil]log");
#endif 
  if (!nolog && !access(path,F_OK))
  {
#ifdef UNIX
     strcpy( &origpath[ 0 ], path );
     strcpy( &destpath[ 0 ], filepath );
     strcat( &destpath[ 0 ], "/log" );
#else 
     strcpy( &origpath[ 0 ], path );
     strcpy( &destpath[ 0 ], filepath );
     strcat( &destpath[ 0 ], "log" );
#endif 

     ival = isDiskFullFile( filepath, &origpath[ 0 ], &diskIsFull );
     if (ival == 0 && diskIsFull) {
         Werrprintf( "svf: problem saving log file: disk is full" );
         ABORT;
     }
     r = copy_file_verify( &origpath[ 0 ], &destpath[ 0 ] );
     if (r != 0) {
	 report_copy_file_error( r, "log" );
         ABORT;
     }
  }
  
  if (!no_arch)
  {	FILE	*fd,*fd_text;
	double	loc;
	char	*ptr, solvent[20], tmp_text[130];
	strcpy(path,filepath);
	ptr = strrchr(path,'/');
        if (ptr==0) strcpy(path,"doneQ");
	else { *ptr = '\0'; strcat(path,"/doneQ"); }
	fd = fopen(path,"a");	/* append, or open if not existing */
        if (fd)
	{  P_getreal(PROCESSED,"loc",&loc,1);
	   fprintf(fd,"  SAMPLE#: %d\n",(int)loc);
	   fprintf(fd,"     USER: %s\n",UserName);
	   fprintf(fd,"    MACRO: ??\n");
	   P_getstring(PROCESSED,"solvent",solvent,1, 18);
	   fprintf(fd,"  SOLVENT: %s\n",solvent);
	   strcpy(path,filepath); strcat(path,"/text");
	   fd_text = fopen(path,"r");
	   ptr = &tmp_text[0];
	   while ( (*ptr = fgetc(fd_text)) != EOF && ptr-tmp_text < 128) 
	   {  if (*ptr == '\n') *ptr='\\';
	      ptr++;
	   }
	   fclose(fd_text); *ptr = '\0';
	   fprintf(fd,"     TEXT: %s\n",tmp_text);
	   /* chop '.fid' then only retain filename */
	   strcpy(path,filepath); ptr = strrchr(path,'.'); *ptr = '\0';
	   ptr = strrchr(path,'/'); if (ptr==0) ptr=path; else ptr++;
	   fprintf(fd,"  USERDIR:\n");
	   fprintf(fd,"     DATA: %s\n",ptr);
	   fprintf(fd,"   STATUS: Saved\n");
	   fprintf(fd,"------------------------------------------------------------------------------\n");
	   fclose(fd);
	}
	else
	   Werrprintf("problem opening '%s'",path);
  }
  if (retc > 0)
  {
     retv[0]=newString(filepath);
  }
#ifdef VNMRJ
  if (doDB)
  {
     char tmppath[3*MAXPATH];
     if (filepath[0]=='/')
       strcpy(tmppath,filepath);
     else
     {
       if (getcwd(tmppath,MAXPATH) == NULL)
         strcpy(tmppath,"");
       strcat(tmppath,"/");
       strcat(tmppath,filepath);
       strncpy(filepath,tmppath,MAXPATH);
       i = strlen(filepath);
       if (filepath[i] != '\0')
         filepath[i] = '\0';
     }
     ptmp = "";
     for (i=strlen(filepath); i>0; i--)
     {
	ptmp = &filepath[i-1];
	if (*ptmp == '/')
	{
	  ptmp++;
	  break;
	}
     }
     if (fidflag == 0 || svf_nofid != 0)
        sprintf(tmppath,"%s vnmr_par \"%s\" \"%s\"",UserName,ptmp,filepath);
     else
        sprintf(tmppath,"%s vnmr_data \"%s\" \"%s\"",UserName,ptmp,filepath);
     if (strlen(tmppath) > 3*MAXPATH)
        Winfoprintf("WARNING: %s cannot add file to database, filename too long!\n",argv[0]);
     else
        writelineToVnmrJ(argv[0],tmppath);
  }
#endif 

  if(part11System || opt) save_optFiles(filepath,"all");
  if(part11System) {
    sprintf( &origpath[ 0 ], "%s/acqfil", curexpdir );
    p11_writeAuditTrails_S("jexp:svf", origpath, filepath);
  }

  disp_status("        ");
  RETURN;
}

static void report_copy_file_error(int errorval, char *filename )
{
	if (errorval == SIZE_MISMATCH)
	  Werrprintf( "'%s' file not completely copied", filename );
	else if (errorval == NO_SECOND_FILE)
	  Werrprintf( "Failed to create '%s' file", filename );
	else
	  Werrprintf( "Problem copying '%s' file", filename );
}

/******************************/
static int copytext(char *frompath, char *topath)
/******************************/
{
  char  path[MAXPATH];
  FILE *infile,*outfile;
  int   ch;

  strcpy(path,frompath);
#ifdef UNIX
  strcat(path,"/text");
#else 
  strcat(path,"text");
#endif 
  if ( (infile=fopen(path,"r")) )
  {
    strcpy(path,topath);
#ifdef UNIX
    strcat(path,"/text");
#else 
    strcat(path,"text");
#endif 
    if ( (outfile=fopen(path,"w")) )
      while ((ch=getc(infile)) != EOF) putc(ch,outfile);
    else
    {
      fclose(infile);
      ABORT;
    }
    fclose(infile);
    fclose(outfile);
  }
  else
    ABORT;
  RETURN;
}

void set_vnmrj_rt_params(int do_call )
{
    double   adim;
    varInfo *vinfo;
    extern   double get_acq_dim();

    /* do_call only for RT RTP */
    if (do_call > 0)
    {
      if (do_call > 1)
      {
        if (P_getVarInfoAddr(CURRENT,"time_processed") != (varInfo *) -1)
        {
          P_setstring(CURRENT,"time_processed","",1);
          appendvarlist("time_processed");
        }
        if (P_getVarInfoAddr(CURRENT,"time_plotted") != (varInfo *) -1)
        {
          P_setstring(CURRENT,"time_plotted","",1);
          appendvarlist("time_plotted");
        }
      }
      if (P_getVarInfoAddr(CURRENT,"acqdim") == (varInfo *) -1)
      {
        P_creatvar(CURRENT,"acqdim",T_REAL);
        if ((vinfo = P_getVarInfoAddr(CURRENT,"acqdim")) != (varInfo *) -1)
        {
          vinfo->subtype =  ST_INTEGER;
          vinfo->maxVal  =  32767.0;
          vinfo->minVal  =  0.0;
          vinfo->step    =  0.0;
        }
      }
      adim = get_acq_dim();
      P_setreal(CURRENT, "acqdim", adim, 1);
      appendvarlist("acqdim");
      if (P_getVarInfoAddr(CURRENT,"procdim") != (varInfo *) -1)
      {
        P_setreal(CURRENT, "procdim", 0.0, 1);
        appendvarlist("procdim");
      }
    }
}

/*************************/
int delexpdata(int argc, char *argv[], int retc, char *retv[])
/*************************/
{
   char remove_it_cmd[ MAXPATH+10 ];
   int this_expnum;
   int r;

   (void) argc;
   (void) retc;
   (void) retv;
   this_expnum = expdir_to_expnum( curexpdir );
   r = is_exp_acquiring( this_expnum );	/* A value of 0 means nothing */
					/* is active.  A value of -1  */
					/* means an error occured in  */
					/* is exp active, in which    */
					/* case we proceed.	      */
   if (r > 0)
   {
      Werrprintf( "Cannot use '%s' when an acquisition is active or queued", argv[ 0 ]);
      ABORT;
   }
   set_nodata();	/* remove data in data and phasefile */
   remove_3d( curexpdir );
   remove_shapelib( curexpdir );
   strcpy( remove_it_cmd, "rm -rf " );
   strcat( remove_it_cmd, curexpdir );
   strcat( remove_it_cmd, "/acqfil/*" );
   system( &remove_it_cmd[ 0 ] );
   specIndex = 1;
   RETURN;
}

static int doFixpar = 1;

/*  This is called in bootup.c during automation
 *  In this case, fixpar has already been run on the parameters 
 *  during the auto_au step.  No need to run it again.  Also,
 *  since the automation RT command is run before the bootup
 *  macro, the fixpar macro could use units that have not yet
 *  been defined, since the bootup macro usually defines units.
 */
void turnOffFixpar()
{
   doFixpar = 0;
}

/* rt returns a fid & log */
/* rtp returns parameters */
/* rtv returns one or more variables */
/* gettxt returns TEXT from named file */
/* puttxt saves TEXT in named file */
/*************************/
int rt(int argc, char *argv[], int retc, char *retv[])
/*************************/
{
  char filepath[MAXPATH],path[MAXPATH],newpath[MAXPATH],filepath0[MAXPATH];

  int r,i;
  int fidflag=0;
  int gettxtflag = 0;
  int nolog;
  int doDisp;
  int doMenu;
  int parFile;
  int rtvStrcmp;
  int this_expnum;
  int do_update_params;
  int recFile=0;
  char *name;
  char *pathptr;
  char time_processed[20];
  char cmd[MAXSTR];
  extern void resetdatafiles();

/*  Verify no acquisition is active or queued for the current experiment.  */

  if(argc>2 && strcmp(argv[2],"nofixpar") == 0) { argc--; turnOffFixpar(); }

  this_expnum = expdir_to_expnum( curexpdir );
  gettxtflag = (strcmp(argv[0],"gettxt")==0 || strcmp(argv[0],"puttxt") == 0);
  rtvStrcmp = strcmp(argv[0],"rtv");
  if (! gettxtflag )
  {
     if ((retc == 0) && (rtvStrcmp != 0) && strcmp(argv[0],"RTP"))
     {
        r = is_exp_acquiring( this_expnum );	/* A value of 0 means nothing */
						/* is active.  A value of -1  */
						/* means an error occured in  */
						/* is exp active, in which    */
						/* case we proceed.	      */
        if (r > 0) {
	   Werrprintf("Cannot use '%s' when an acquisition is active or queued",
                      argv[ 0 ]);
	   ABORT;
        }
     }
  }

  D_allrelease();
  setfilepaths(0);
  if ((mode_of_vnmr == AUTOMATION) && (datadir[0] != '\0'))
  {
     if (rtvStrcmp || !retc)
     {
        int tmp;

        tmp = showFlushDisp;
        showFlushDisp = 0;
        flush(0,NULL,0,NULL);
        showFlushDisp = tmp;
     }
  }

  if (!gettxtflag)    
  {   if ((strcmp(argv[0],"RTP")==0)||(rtvStrcmp==0))
          fidflag = 0;
      else {
          fidflag = 1;
  	  // clear multiple spec
  	  clearMspec();
  	  set_dpf_flag(0,"");
  	  set_dpir_flag(0,"");
      }
  }
  nolog = FALSE;
  doDisp = TRUE;
  doMenu = TRUE;
  if ( !strcmp(argv[0],"RT") || !strcmp(argv[0],"RTP") )
  {
     for (i = 1; i<argc; i++)
     {
       if (!strcmp(argv[i],"nolog") ) nolog = TRUE; 
       if (!strcmp(argv[i],"nodg") ) doDisp = FALSE; 
       if (!strcmp(argv[i],"nomenu") ) doMenu = FALSE; 
     }
  }
  if (doMenu && rtvStrcmp)
    Wturnoff_buttons();
  if (nolog) argc--;
  if (!doDisp) argc--;
  if (!doMenu) argc--;
  if (argc<2)
    {
      if (Bnmr)
      {
	Werrprintf( "'rt' command requires argument from a background mode" );
	ABORT;
      }
      W_getInput("File name (enter name and <return>)? ",filepath,MAXPATH-1);
      name = filepath;
      if (strlen(name)==0)
        { Werrprintf("No file name given, command aborted");
          ABORT;
        }
    }
  else if ((argc!=2)&&(rtvStrcmp!=0))
    { Werrprintf("usage - %s(filename[,'nolog','nodg'])",argv[0]);
      ABORT;
    }
  else
    name = argv[1];
  if ((int) strlen(name) >= (MAXPATH-32))
    { Werrprintf("file path too long");
      ABORT;
    }
  if (strcmp(argv[0], "RT") == 0)
     do_update_params = 2;
  else if (strcmp(argv[0], "RTP") == 0)
     do_update_params = 1;
  else
     do_update_params = 0;
  if (doDisp && rtvStrcmp)
     disp_status("RT      ");

#ifdef UNIX
  if (name[0]!='/')
    { /* getcwd(filepath0, sizeof( filepath0 ) - 1); */
      pathptr = get_cwd();
      strcpy(filepath0, pathptr);
      strcat(filepath0,"/");
      strcat(filepath0,name);
    }
  else
#endif 
    strcpy(filepath0,name);
  strcpy(filepath,filepath0);

  // if extension missing, try to attach .REC or .rec
  // filepath will be altered only if .REC or .rec exist.
  pathptr = filepath;
  if (access(filepath,F_OK) && !strrchr(pathptr+strlen(filepath)-4,'.')) {
     strcpy(path,filepath);
     strcat(path,".REC");
     if(!access(path,F_OK)) strcpy(filepath,path);
     else {
       strcpy(path,filepath);
       strcat(path,".rec");
       if(!access(path,F_OK)) strcpy(filepath,path);
     }
  }
  // if filepath ends with .fid, but access failed, try to replace it with .REC or .rec
  if (access(filepath,F_OK) && strstr(pathptr+strlen(filepath)-4,".fid") != NULL) {
     strncpy(path, filepath, strlen(filepath)-4);
     strcat(path,".REC");
     if(!access(path,F_OK)) strcpy(filepath,path);
     else {
       strncpy(path, filepath, strlen(filepath)-4);
       strcat(path,".rec");
       if(!access(path,F_OK)) strcpy(filepath,path);
     }
  }

#ifdef VNMRJ
  frame_update(argv[0], filepath);
#endif 

  recFile=p11_isRecord(filepath);
  if ( !strcmp(argv[0],"RT") && recFile ) {
	rt_FDA(argc, argv, retc, retv, filepath, gettxtflag,
	fidflag, nolog, doDisp, doMenu, do_update_params, recFile);
#ifdef VNMRJ
/* update jviewportlabels*/
        execString("vpLayout('updateLabel')\n");
#endif
	RETURN;
  }

  parFile = (param_access(filepath, R_OK ) == 0);
  /* if the file exists and the command is RT or RTP, the file must be
   * a directory with a procpar in it
   */
  if (parFile && do_update_params)
  {
    parFile = isDirectory(filepath);
    if (parFile)
       parFile = (isFid(filepath) || isPar(filepath) );
  }
  if (!parFile)
  {
    if ((!isFid(filepath))&&(!isPar(filepath)) && !recFile )
    { if (fidflag)
        strcat(filepath,".fid");
      else
        strcat(filepath,".par");
    }
#ifdef VMS
    filepath[ strlen( filepath ) - 4 ] = '_';
    strcat(filepath,".dir");
#endif 
  }
  if (access(filepath,F_OK))
    { strcpy(filepath,filepath0);
      if ((!isFid(filepath))&&(!isPar(filepath)) && !recFile )
        { if (fidflag)
            strcat(filepath,".par");
          else
            strcat(filepath,".fid");
        }
#ifdef VMS
      filepath[ strlen( filepath ) - 4 ] = '_';
      strcat(filepath,".dir");
#endif 
      if (access(filepath,F_OK))
        {
          if ( (rtvStrcmp == 0) && (retc == 1) && (argc > 3) &&
               !strcmp(argv[2],"noabort") )
          {
             disp_status("        ");
             RETURN;
          }
          if(recFile) 
          Werrprintf("%s does not exist",filepath);
          else
          Werrprintf("Neither %s.fid nor %s.par exists",filepath0,filepath0);
          disp_status("        ");
          ABORT;
        }
      if (fidflag) Winfoprintf("parameters only have been retrieved");
      fidflag = 0;
    }

  // if recfile, then argv[0] is rtp or rtv. 
  // set filepath to procpar path
  if(recFile && rtvStrcmp==0) {
     parFile = 1;
     pathptr = filepath;
     if(isFDARec(filepath) || isRec(filepath))  
	strcat(filepath,"/acqfil/procpar");
     else if (!strstr(pathptr+strlen(filepath)-8,"/procpar")) 
     	strcat(filepath,"/procpar");
  } else if(recFile) {
     if(isFDARec(filepath) || isRec(filepath))  
	strcat(filepath,"/acqfil");
  }

  if (access(filepath,(parFile) ? R_OK : (R_OK | X_OK)))
    { Werrprintf("file %s is not accessible",filepath);
      disp_status("        ");
      ABORT;
    }

#ifdef VMS
  make_vmstree(filepath,filepath,MAXPATHL-20);
#endif 

if (gettxtflag)
  { /* now copy text file */
    if (strcmp(argv[0],"gettxt") == 0)
      {   if (copytext(filepath,curexpdir))
            {   Werrprintf("cannot read and copy text file");
  	        ABORT;
    	    }
      }
    else if (copytext(curexpdir,filepath))
            {   Werrprintf("cannot read and copy text file");
  	        ABORT;
    	    }
    text(0,argv,0,NULL);
  
    if (doDisp)
       disp_status("        ");
    RETURN;
  }

/*  Remove the ACQPAR file from the current experiment.  Prevents RA from
    working after RT is executed; you have to enter the GO command first.  */

  strcpy( path, &curexpdir[0] );
  strcat( path, "/acqfil/acqpar" );
  i = unlink( &path[ 0 ] );		/* assume it works and ignore result */

  if (parFile)
     strcpy( path, filepath );
  else
     D_getparfilepath(PROCESSED, path, filepath);

  if (rtvStrcmp==0)
    {
      int returned = 0;
      int rtvAbort = 1;

      P_treereset(TEMPORARY);		/* clear the tree first */
      if (argc<3)
        { Werrprintf("usage - rtv(filename,varname,..)");
          ABORT;
        }
      /* first read parameters from file into temporary tree */
      if (P_read(TEMPORARY,path))
	{ Werrprintf("cannot read parameters from %s",path);
          P_treereset(TEMPORARY);
	  ABORT;
        }
      i = 2;
      if ((retc == 1) && !strcmp(argv[2],"noabort") &&
          ( (argc == 4) ||
            ((argc == 5) && isReal(argv[4])) ||
            ((argc == 5) && ( argv[4][0] == '$')) )
          )
      {
         i = 3;
         rtvAbort = 0;
      }
      if ((retc == 1) && (argc == i+2) && (argv[i+1][0] == '$') )
      {
          symbol **root;
          varInfo *v;

          if ( (v = P_getVarInfoAddr(TEMPORARY,argv[i])) == (varInfo *) -1 )
          {
                P_treereset(TEMPORARY);
                if (rtvAbort)
                {
                   Werrprintf("rtv: parameter %s does not exist",argv[i]);
                   ABORT;
                }
                else
                {
	           retv[ 0 ] = intString( 0 );
                   RETURN;
                }
          }
          else
          {
	        retv[ 0 ] = intString( v->T.size );
                if ((root=selectVarTree(argv[i+1])) == NULL)
                {
                   P_treereset(TEMPORARY);
                   if (rtvAbort)
                   {
                      Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[i+1]);
                      ABORT;
                   }
                   else
                   {
                      RETURN;
                   }
                }
	        copyVar(argv[i+1],v,root);
          }
      }
      else
      {
        while (argc>i)
        {
          vInfo  info;
          double tmpval;
          char tmpstr[512];

          if (retc == 0)
          { if ( (r=P_copyvar(TEMPORARY,CURRENT,argv[i],argv[i])) )
            { Werrprintf("rtv: cannot retrieve variable %s\n",
                 argv[i]);
              P_treereset(TEMPORARY);
              ABORT;
            }
#ifdef VNMRJ
	    appendvarlist( argv[i] );
#endif 
            /* skip index since copyvar copys all indexes */
            if ((argc > i+1) && isReal(argv[i+1]) )
               i++;
          }
          else if (returned < retc)
          {
	     if (P_getVarInfo(TEMPORARY,argv[i],&info))
             {
                P_treereset(TEMPORARY);
                if (rtvAbort)
                {
                   Werrprintf("rtv: parameter %s does not exist",argv[i]);
                   ABORT;
                }
                else
                {
                   RETURN;
                }
             }
             else
             {
                int index = 1;
                int skipindex = 0;

                if ((argc > i+1) && isReal(argv[i+1]) )
                {
                  index = atoi(argv[i+1]);
                  skipindex = 1;
                }
                if (index < 1)
                  index = 1;
                if (index > info.size)
                {
                   P_treereset(TEMPORARY);
                   if (rtvAbort)
                   {
                      Werrprintf("rtv: parameter %s[%d] does not exist",
                                   argv[i],index);
                      ABORT;
                   }
                   else
                   {
                      RETURN;
                   }
                }
                if (info.basicType == T_STRING)
                {
                   P_getstring(TEMPORARY,argv[i],tmpstr,index,511);
	           retv[ returned ] = newString( tmpstr );
                }
                else
                {
                   P_getreal(TEMPORARY,argv[i],&tmpval,index);
	           retv[ returned ] = realString( (double) tmpval );
                }
                if (skipindex)
                   i++;
             }
             returned++;
          }
          i++;
        }
      }
      P_treereset(TEMPORARY);
      if (retc == 0)
         execString("fixpar\n");
      RETURN;
    }
  if (access(path, R_OK))
  {   Werrprintf("problem reading parameters from %s",path);
      disp_status("        ");
      ABORT;
  }
  /* save shim values so as to retain them if no FID found */
  P_treereset(TEMPORARY);
  saveallshims(0);          /* Second argument of 0      */
                            /* serves to move value from */
                            /* CURRENT tree to TEMPORARY */
  /* first read parameters into current parameter tree */
  P_treereset(CURRENT);		/* clear the tree first */
  if ( (r=P_read(CURRENT,path)) )
    { Werrprintf("problem reading parameters from %s",path);
      P_treereset(TEMPORARY);
      disp_status("        ");
      ABORT;
    }

  /* now copy text file */
  if (copytext(filepath,curexpdir))
  {
    FILE *textfile;
    char textpath[MAXSTR];
    strcpy(textpath,curexpdir);
    strcat(textpath,"/text");
    if ( (textfile = fopen(textpath,"w")) != NULL )
    {
       fprintf(textfile, "\n");
       fclose(textfile);
    }
  }

  if (fidflag)
       saveallshims(2);     /* fix shim limits */
  if (!fidflag) 
    { 
       int give = 0;
       P_setstring(CURRENT,"file",filepath0,0);
       /* remove the saved global parameters */
       saveGlobalPars(4,"_");
       saveallshims(1);     /* TEMPORARY to CURRENT */
       P_treereset(TEMPORARY);
       if (doDisp)
          disp_status("        ");
       if (doFixpar)
          give = execString("fixpar\n");
       doFixpar = 1;
       releasevarlist();
       set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
       if (do_update_params > 0)
       {
	  appendTopPanelParam();
	  disp_current_seq();
       }
#else 
       if (doDisp)
          execString("dg\n");
#endif 
       if ((!Bnmr) && (doMenu))
          execString("menu('main')\n");

	sprintf(path,"%s/acqfil", curexpdir);
        p11_init_acqfil("rt", filepath, path);
  	p11_restartCmdHis();
        if (give)
        {
           Werrprintf("rtp: fixpar failed for %s",filepath0);
        }
        RETURN;
    }

  /* now check whether fid is in file */
  strcpy(path,filepath);
  strcat(path,"/fid");
  if (access(path,F_OK))
    { Winfoprintf("no fid in file, only parameters copied\n");
      P_setstring(CURRENT,"file",filepath0,0);
      saveallshims(1);
      P_treereset(TEMPORARY);
      if (doDisp)
         disp_status("        ");
      if (doFixpar)
         execString("fixpar\n");
      doFixpar = 1;
      set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
      if (do_update_params > 0)
      {
	 appendTopPanelParam();
	 disp_current_seq();
      }
#endif 
      RETURN;
    }

  /* if so, erase the current fid and link the new one */
  /* use `set_nodata' to close D_USERFILE
     before deleting curexp+'/acqfil/fid' */

  set_nodata();	/* remove data in data and phasefile */
  remove_3d( curexpdir );
  remove_shapelib( curexpdir );
#ifdef VNMRJ
  clearGraphFunc();  /* prevent auto redraw of data */
#endif

  strcpy(newpath,curexpdir);
  strcat(newpath,"/acqfil/fid");
  if (unlink(newpath))
    { if (errno != ENOENT)
        { Werrprintf("cannot delete current fid file %s",newpath);
          P_treereset(TEMPORARY);
          disp_status("        ");
          if (doFixpar)
             execString("fixpar\n");
          doFixpar = 1;
          set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
          if (do_update_params > 0)
	  {
	    appendTopPanelParam();
	    disp_current_seq();
	  }
#endif 
          ABORT;
        }
    }
  specIndex = 1;
  if (link(path,newpath))
    {
      char oldPath[MAXPATH];

      strcpy(oldPath,path);
      fix_automount_dir( oldPath, path );
      if (symlink(path,newpath))
        { Werrprintf("cannot link the fid file");
          P_treereset(TEMPORARY);
          disp_status("        ");
          if (doFixpar)
             execString("fixpar\n");
          doFixpar = 1;
          set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
          if (do_update_params > 0)
	  {
	    appendTopPanelParam();
	    disp_current_seq();
	  }
#endif 
          ABORT;
        }
    }
  /* Remove any old sampling schedules */
  strcpy(newpath,curexpdir);
  strcat(newpath,"/acqfil/sampling.sch");
  unlink(newpath);
  strcpy(newpath,curexpdir);
  strcat(newpath,"/sampling.sch");
  unlink(newpath);
     
  /* Copy sampling schedules */
  strcpy(path,filepath);
  strcat(path,"/sampling.sch");
  if ( ! access(path,F_OK))
  {
     char systemcall[4*MAXPATH];

     sprintf(systemcall,"cp %s %s/sampling.sch; cp %s %s/acqfil/sampling.sch",
                         path, curexpdir, path, curexpdir);
     system(systemcall);
  }
  /* The following lines copy the 'log' file from automation files to the
     experiment directory (if there is one). Svf has been changes to carry
     this log file along */
  strcpy(path,filepath);
  strcpy(newpath,curexpdir);
  strcat(path,"/log");
  strcat(newpath,"/acqfil/log");
  if (unlink(newpath))
     if (errno != ENOENT)
        Werrprintf("cannot delete current log file %s",newpath);
  if (!nolog)
  {  if (!access(path,F_OK))
     {  if (link(path,newpath))
        {  if (symlink(path,newpath))
              Werrprintf("cannot link the log file");
        }
     }
  }

  /* if everything ok, copy parameters */
  P_setstring(CURRENT,"file",filepath0,0);
  P_treereset(PROCESSED);	/* clear the tree first */
  P_copy(CURRENT,PROCESSED);
  P_treereset(TEMPORARY);
  resetdatafiles();
  if (doDisp)
     disp_status("        ");
  Wsetgraphicsdisplay("");
  if (doFixpar)
     execString("fixpar\n");
  doFixpar = 1;
  releasevarlist();

#ifdef VNMRJ
/* update jviewportlabels*/
  execString("vpLayout('updateLabel')\n");
#endif

  sprintf(path,"%s/datdir", filepath);
  sprintf(newpath,"%s/datdir", curexpdir);

  r = p11_copyFiles(path, newpath); 
  strcpy(time_processed,"");
  strcpy(cmd,"");
  if(r == 1) { /* datdir exists */
     if (P_getVarInfoAddr(CURRENT,"time_processed") != (varInfo *) -1)
     {
        P_getstring(CURRENT,"time_processed",time_processed,1, 20);
     }
     sprintf(path,"%s/curpar", filepath);
     if(!P_read(TEMPORARY,path)) {
	P_copygroup(TEMPORARY,CURRENT,G_DISPLAY);
        P_copygroup(TEMPORARY,CURRENT,G_PROCESSING);
        P_treereset(TEMPORARY);
     }
     sprintf(cmd,"recds('%s/datdir','%s')\n",filepath, "processed");
  }
  
  set_vnmrj_rt_params( do_update_params );
  P_setstring(CURRENT,"time_processed",time_processed,1);

#ifndef VNMRJ
  if (doDisp)
     execString("dg\n");
#else 
  if (!Bnmr && (do_update_params > 0))
  {
     appendTopPanelParam();
     disp_current_seq();
  }
#endif 
  if ((!Bnmr) && (doMenu))
     execString("menu('main')\n");

  sprintf(path,"%s/acqfil", curexpdir);
  p11_init_acqfil("rt", filepath, path);
  p11_restartCmdHis();

  if(strlen(cmd) > 0) execString(cmd); 
  if(retc > 0) retv[0]=newString(time_processed);

  RETURN;

}

/******************/
static int update(const char *vname)
/******************/
{ double v;
  if (P_getreal(TEMPORARY,vname,&v,1)) ABORT;
  if (P_setreal(CURRENT,vname,v,0)) ABORT;
  RETURN;
}

/************************/
int s_cmd(int argc, char *argv[], int retc, char *retv[])
/************************/
/* s(n)  save display parameters */
/* fr(n) return all display parameters */
/* r(n)  return some display parameters */
{ char path[MAXPATH];
  char name[MAXPATH];
  char buf[8];

  (void) retc;
  (void) retv;
  if (argc < 2)
  { Werrprintf("usage - %s(n), where n is a file name",argv[0]);
      ABORT; 
  }
  strcpy(name,argv[1]);
  if (verify_fname(name))
    { Werrprintf( "%s: file name '%s' has invalid characters in it",argv[0], name );
      ABORT;
    }
  if (strchr(name,'/') != NULL)
  {
      Werrprintf( "%s: file name '%s' must not have directory components", argv[0], name );
      ABORT;
  }
  strcpy(path,curexpdir);
  strcat(path,"/s");
#ifdef XXX
  if ((name[0] >= '1') && (name[0] <= '9') && (strlen(name) == 1))
  {  /* prepend s to the single digit; (compatible with older versions) */
     strcat(path,"s");
  }
#endif 
  strcat(path,name);
  if (strcmp(argv[0],"s")==0)
    { /* first copy display parameters into temporary tree */
      Wturnoff_buttons();
      P_treereset(TEMPORARY);		/* clear the tree first */
      P_copygroup(CURRENT,TEMPORARY,G_DISPLAY);
      /* then save them in file */
      if (P_save(TEMPORARY,path))
	{ Werrprintf("s: cannot save display parameters in %s",name);
	  ABORT;
        }
    }
  else if (strcmp(argv[0],"fr")==0)
    { P_treereset(TEMPORARY);		/* clear the tree first */
      /* first read parameters from file into temporary tree */
      if (P_read(TEMPORARY,path))
	{ Werrprintf("fr: cannot read display parameters from %s",name);
	  ABORT;
        }
      P_copygroup(TEMPORARY,CURRENT,G_DISPLAY);
    }
  else if (strcmp(argv[0],"r")==0)
    { P_treereset(TEMPORARY);		/* clear the tree first */
      /* first read parameters from file into temporary tree */
      if (P_read(TEMPORARY,path))
	{ Werrprintf("r: cannot read display parameters from %s",name);
	  ABORT;
        }
      update("sp");
      update("wp");
      update("sp1");
      update("wp1");
      update("sp2");
      update("wp2");
      update("sf");
      update("wf");
      update("sf1");
      update("wf1");
      update("sf2");
      update("wf2");
      update("sc");
      update("wc");
      update("sc2");
      update("wc2");
      update("vp");
      update("ho");
      update("vo");
      update("vs");
      update("vs2d");
      update("vsproj");
      P_getstring(TEMPORARY,"aig",buf,1,7);
      P_setstring(CURRENT,"aig",buf,0);
    }
  P_treereset(TEMPORARY);	/* clear the tree again */
  if (argc == 2)
     appendvarlist("cr");          /* activate any re-executable program */
  RETURN;
}


/*  param access is modeled on the UNIX service access, with extensions
    appropriate for VNMR parameter files (procpar, curpar, conpar, etc.)

    At this time we verify access and verify the entry is not a directory  */

static int param_access(char *path, int level )
{
	if (access( path, level ) != 0)
	  return( -1 );
	if (isDirectory( path ))
	  return( -1 );

	return( 0 );
}

/*  Find a parameter file from the starting entry name
    examines input, input +"/procpar",
             input + ".par/procpar", input + ".fid/procpar"

    Input is (for example) the name which rt receives.  Remember the rt
    program appends .par and then .fid to its input to locate the FID.	*/

int find_param_file(char *input, char *final )
{
	char	working[ MAXPATH ];

	if (param_access( input, R_OK ) == 0) {
		strcpy( final, input );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], "/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], ".par/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], ".fid/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], "/acqfil/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], ".REC/acqfil/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}

	strcpy( &working[ 0 ], input );
	strcat( &working[ 0 ], ".rec/acqfil/procpar" );
	if (param_access( &working[ 0 ], R_OK ) == 0) {
		strcpy( final, &working[ 0 ] );
		return( 0 );
	}
	return( -1 );		/* because it never worked */
}

/**************************/
static void update_shimlimit(const char *vname)
/**************************/
{
    symbol **root;
    varInfo *v;
    if ( (root = getTreeRoot("current")) )
      if ( (v = rfindVar(vname,root)) )
      {
        v->maxVal = v->minVal = v->step = 19.0;
        v->prot  |=  P_MMS;
      }
}

/**************************/
static void saveallshims(int rtsflag)
/**************************/
{
   int index;

   init_shimnames(-1);
   for (index=Z0 + 1; index <= MAX_SHIMS; index++)
   {
      const char *sh_name;
      if ((sh_name = get_shimname(index)) != NULL)
      {
         saveshim(sh_name,   rtsflag);
      }
   }
}

/****************************/
static int saveshim(const char *vname, int rtsflag)
/****************************/
{ 
  switch( rtsflag )
  {
    case 1:
        if (update( vname )) ABORT;
        /*if (P_copyvar(TEMPORARY,CURRENT,vname,vname)) ABORT;*/
        update_shimlimit( vname );
      break;
    case 2:
        update_shimlimit( vname );
      break;
    case 0: default:
        if (P_copyvar(CURRENT,TEMPORARY,vname,vname)) ABORT;
      break;
  }
  RETURN;
}

/**********************************************/
/*  Initial operations common to RTS and SVS  */
/**********************************************/

/* addr_path	should be MAXPATH writeable characters  */
static int
intro_shims(int argc, char *argv[], char *addr_path )
{
	Wturnoff_buttons();
	D_allrelease();

	if (argc < 2) {
		if (Bnmr) {
			Werrprintf(
		    "'%s' command requires argument(s) from a background mode",
		     argv[ 0 ]
			);
			return( -1 );
		}
		W_getInput(
	    "File name (enter name and <return>)? ", addr_path, MAXPATH-2
		);
	}
	else {
	   if ((int) strlen( argv[1] ) > MAXPATH-2 ) {
		Werrprintf( "shim file path too long" );
		return( -1 );
	   }
	   strcpy( addr_path, argv[ 1 ] ); 
	}

	if ((int) strlen( addr_path ) < 1) {
		Werrprintf( "No file name given, command aborted" );
		return( -1 );
	}
	if ((int) strlen( addr_path ) > MAXPATH-2 ) {
		Werrprintf( "shim file path too long" );
		return( -1 );
	}

	P_treereset( TEMPORARY );
	return( 0 );
}

/************************/
int rts(int argc, char *argv[], int retc, char *retv[])
/************************/
{

/*  input_path receives the input argument.
    final_path receives the complete path name.  If no file found,
    it holds the null string.
    st_index is the index into the search table.  When the search is
    complete, it is one larger than the number of entries.
    where_found contains the value to return if the shim file is found.  */

	char	input_path[ MAXPATH ], final_path[ MAXPATH ];
	int	ival;
	int	where_found = 0;

	if (intro_shims( argc, argv, &input_path[ 0 ] ) != 0)
	  ABORT;				/* `intro_shims' reports the error */

	final_path[ 0 ] = '\0';
	if (input_path[ 0 ] == '/')
        {
		if (find_param_file( &input_path[ 0 ], &final_path[ 0 ] ) != 0)
		  Werrprintf( "%s: cannot access %s", argv[ 0 ], &input_path[ 0 ] );
		else {
			where_found = 1;
		}
	}
	else
        {
                where_found = appdirFind(input_path, "shims", final_path, "", R_OK);
	}

	if (final_path[ 0 ] == '\0') {
	        if (retc > 0)
                {
	            retv[ 0 ] = realString( (double) 0.0 );
		    RETURN;
                }
                else
                {
		   Werrprintf( "cannot read shim settings from %s", &input_path[ 0 ]);
		   ABORT;
                }
	}
	else if (isDirectory( &final_path[ 0 ] )) {
	        if (retc > 0)
                {
	            retv[ 0 ] = realString( (double) 0.0 );
		    RETURN;
                }
                else
                {
		    Werrprintf( "cannot read shim settings from directory file %s", &final_path[ 0 ]);
		    ABORT;
                }
	}

/*  The TEMPORARY tree was reset in intro_shims.  */

	if (P_read( TEMPORARY, &final_path[ 0 ] ) != 0) {
	        if (retc > 0)
                {
	            retv[ 0 ] = realString( (double) 0.0 );
		    RETURN;
                }
                else
                {
		    Werrprintf( "cannot read shims settings from %s", &final_path[ 0 ] );
		    ABORT;
                }
	}

	saveallshims( 1 );	/*  1 identifies this as RTS  */
	P_copyvar(TEMPORARY, CURRENT, "shims", "shims");
#ifdef VNMRJ
	appendvarlist("shims"); /* Notify VnmrJ of new shims val */
#endif 

	ival = P_setstring(CURRENT,"load","y",1);
	if (ival)
	  if (ival <= -6  ||  ival >=-2)
	    Werrprintf("Error %d in setting parameter 'LOAD'", ival);

/*  Only report the location of the shims file if no return value requested.  */

	if (retc < 1)
	  Winfoprintf( "shims read from %s", &final_path[ 0 ] );
#ifndef VNMRJ
	if (WtextdisplayValid("dgs"))
	  execString("dgs\n");
#endif 

	if (retc > 0)
	  retv[ 0 ] = realString( (double) where_found );
	RETURN;
}

/************************/
int shimnames(int argc, char *argv[], int retc, char *retv[])
/************************/
{
   int index;
   char sNames[1024];
   int cnt = 0;

   (void) argc;
   (void) argv;
   init_shimnames(SYSTEMGLOBAL);
   sNames[0] = '\0';
   for (index=Z0 + 1; index <= MAX_SHIMS; index++)
   {
      const char *sh_name;
      if ((sh_name = get_shimname(index)) != NULL)
      {
         cnt++;
         if (sNames[0] != '\0')
           strcat(sNames," ");
         strcat(sNames,sh_name);
      }
   }
   if (retc == 0)
      Winfoprintf("shims: %s",sNames);
   else
   {
      retv[0] = newString(sNames);
      if (retc > 1)
         retv[1] = realString((double) cnt);
   }
   RETURN;
}

/************************/
int svs(int argc, char *argv[], int retc, char *retv[])
/************************/
{
	char	answer[ 12 ], input_path[ MAXPATH ], final_path[ MAXPATH ];
	char	buf[MAXPATH];
	char	*fname;
	int	where_stored;

	if (intro_shims( argc, argv, &input_path[ 0 ] ) != 0)
	  ABORT;				/* `intro_shims' reports the error */

	if (verify_fname( &input_path[ 0 ] )) {
		Werrprintf( "file path '%s' not valid", &input_path[ 0 ] );
		ABORT;
	}

	final_path[ 0 ] = '\0';
#ifdef UNIX
	if (input_path[ 0 ] == '/') {
#else 
	if (abspath( &input_path[ 0 ] )) {
#endif 
		strcpy( &final_path[ 0 ], &input_path[ 0 ] );
		where_stored = 1;
	}
	else {
                where_stored = appdirAccess("shims", final_path);
		if (final_path[ 0 ] != '\0')
		{
			strcat( &final_path[ 0 ], "/" );
			strcat( &final_path[ 0 ], &input_path[ 0 ] );
		}
	}

	if (final_path[ 0 ] == '\0') {
	        if (retc > 0)
	           retv[ 0 ] = realString( (double) 0.0 );
                else
		   Werrprintf( "Cannot save shims as '%s'", &input_path[ 0 ] );
		RETURN;
	}

/*  In foreground mode, computer requests permission to overwrite the
    shim file if it already exists.  In background mode, an existing
    file is overwritten by default.					*/

	if (access( &final_path[ 0 ], F_OK ) == 0) {
		if ( !Bnmr  && (argc < 3) ) {
			Winfoprintf( &final_path[ 0 ] );	/* display the file */
			W_getInput(
		    "File exists, overwrite (enter y or n <return>)? ", answer, 15
			);
			if (strcmp( answer, "n" ) == 0 || strcmp( answer, "no" ) == 0)
			  ABORT;
		}
		if (unlink( &final_path[ 0 ] ) != 0) {
			Werrprintf(
		    "svs:  cannot remove existing file %s", &final_path[ 0 ]
			);
			ABORT;
		}
	}

	saveallshims( 0 );	/* 0 identifies this as SVS */
	P_copyvar(CURRENT, TEMPORARY, "shims", "shims");
	P_copyvar(CURRENT, TEMPORARY, "solvent", "solvent");
	P_copyvar(GLOBAL, TEMPORARY, "probe", "probe");

	if (P_save( TEMPORARY, &final_path[ 0 ] )) {
		Werrprintf( "error saving shim values in %s", &final_path[ 0 ] );
		ABORT;
	}

/*  Only report where the shims were stored if no return value requested.  */

	if (retc < 1)
	  Winfoprintf( "shims stored in %s", &final_path[ 0 ] );
	else
	  retv[ 0 ] = realString( (double) where_stored );

#ifdef VNMRJ
	if ( (fname=strrchr(final_path, '/')) )
        {
	    fname++;
	} else
        {
	    fname = final_path;
	}
	sprintf(buf, "%s shims \"%s\" \"%s\"", UserName, fname, final_path);
        if (strlen(buf) > 3*MAXPATH)
           Winfoprintf("WARNING: %s cannot add file to database, filename too long!\n",argv[0]);
        else
	   writelineToVnmrJ("SVS", buf);
#endif 

	RETURN;
}

static int isExpInVp( int exp )
{
   double dval;
   int num;
   int index;

   if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
   {
      num = (int) (dval+0.1);
      for (index=1; index <= num; index++)
      {
         if (!P_getreal(GLOBAL, "jcurwin", &dval, index))
         {
            if ( (int) (dval+0.1) == exp)
               return(index);
         }
      }
   }
   return(0);
}

/*************************/
int mp(int argc, char *argv[], int retc, char *retv[])
/*************************/
/* mp(<from,> to)  move current parameters to specified exp           */
/* mf(<from,> to)  move current and processed parameters, text and    */
/*                 FID to specified exp                               */
/* md(<from,> to)  move saved display parameters to specified exp     */
{ char frompath[MAXPATH];
  char topath[MAXPATH];
  char path[MAXPATH];
  char e_current[MAXPATH];
  char e_from[MAXPATH];
  char e_to[MAXPATH];
  char systemcall[2*MAXPATH+8];
  int  from_current,index,ival,lval,to_current;

  (void) retc;
  (void) retv;
  from_current = 0;
  to_current = 0;

/*
  e_current, e_from, and e_to are all ASCII strings, not numeric values.
*/

  sprintf(e_current, "%d", expdir_to_expnum(curexpdir) );
  if ( (argc == 2) && isReal(argv[1]) )
  {
    strcpy(e_from, e_current);
    strcpy(e_to, argv[1]);
    from_current = 1;
  }
  else if ( (argc == 3) && isReal(argv[1]) && isReal(argv[2]) )
  {
    strcpy(e_from, argv[1]);
    strcpy(e_to, argv[2]);
    from_current = ( strcmp(e_from, e_current) == 0 );
  }
  else 
  {
     Werrprintf("usage - %s(<from exp,> to exp)",argv[0]);
     ABORT; 
  }

  if ( (atoi(e_from) < 1) || (atoi(e_from) > MAXEXPS) )
  {
    Werrprintf("illegal experiment number");
    ABORT;
  }
  else if ( (atoi(e_to) < 1) || (atoi(e_to) > MAXEXPS) )
  {
    Werrprintf("illegal experiment number");
    ABORT;
  }

  if ( strcmp(e_from, e_to) == 0 )
    RETURN;

  to_current = ( strcmp(e_to, e_current) == 0 );

/* now verify no acquisition is active or queued in the target experiment. */
/* is_exp_active's argument must be a numeric value... */

  if (strcmp(argv[0],"md") != 0)
  {
    if (to_current)
       ival = is_exp_acquiring( atoi(e_to) );
    else
       ival = is_exp_active( atoi(e_to) );
    if (ival > 0) {
	if (to_current)
	  Werrprintf(
    "Cannot use '%s' with an acquisition active or queued in the current experiment",
     argv[ 0 ]
	  );
	else
	  Werrprintf(
    "Cannot use '%s' with an acquisition active or queued in experiment %s",
     argv[ 0 ], e_to
	  );
	ABORT;
    }
  }

  if (from_current)
    strcpy(frompath,curexpdir);
  else
    if (access_exp(frompath,e_from))
      ABORT;
  if (to_current)
    strcpy(topath,curexpdir);
  else
    if (access_exp(topath,e_to))
      ABORT;

/*  Lock target experiment if not current...  */

  if ( !to_current )
    if ( (lval = lockExperiment( atoi(e_to), mode_of_vnmr )) )
    {
	Werrprintf( "experiment %s locked", e_to );
	if (lval == 1)					/* Parameters?? */
	  Wscrprintf( "acquisition processing active\n" );
	else if (lval == 2)
	  Wscrprintf( "background processing active\n" );
	else if (lval == 3)
	  Wscrprintf( "foreground processing active\n" );
	ABORT;
    }

  if (to_current && !Bnmr)
  {
     int fromExp = atoi(e_from);
     int fromVp;

     if ( (fromVp = isExpInVp( fromExp )) )
     {
        int ex;
        char msg[MAXSTR];
        char testFile[MAXPATH];
        FILE *fd;

        sprintf(testFile,"%s/exp%d/flushpars",userdir,fromExp);
        fd = fopen(testFile,"w");
        if (fd)
           fclose(fd);
        sprintf(msg,"VP %d flushpars('flushpars')\n",fromVp);
        writelineToVnmrJ("vnmrjcmd",msg);
        ex = 0;
        while ( ! access(testFile,R_OK) && (ex < 50) )
        {
           struct timespec timer;

           timer.tv_sec=0;
           timer.tv_nsec = 10000000;   /* 10 msec */
#ifdef __INTERIX
           usleep(timer.tv_nsec/1000);
#else
           nanosleep(&timer, NULL);
#endif
           ex++;
        }
        if (ex >= 50)
        {
	   Werrprintf("Viewport %d not responding to %s command",
              fromVp,argv[0]);

           unlink(testFile);
	   ABORT;
        }
        sprintf(testFile,"%s/exp%d/flushparsFailed",userdir,fromExp);
        if ( ! access(testFile,R_OK) )
        {
	   Werrprintf("Viewport %d failed to save parameters for %s command",
              fromVp,argv[0]);

           unlink(testFile);
	   ABORT;
        }
     }
  }
  if (strcmp(argv[0],"md")==0)
  {
    disp_status("MD      ");
#ifdef UNIX
    strcat(frompath,"/sx");
    strcat(topath,"/sx");
#else 
    strcat(frompath,"sx");
    strcat(topath,"sx");
#endif 
    for (index='1'; index<='9'; index++)
    {
      frompath[strlen(frompath)-1] = index;
      P_treereset(TEMPORARY);		/* clear the tree first */
      /* first read parameters from file into temporary tree */
      if (!P_read(TEMPORARY,frompath))
      {
        topath[strlen(topath)-1] = index;
        /* then save them in the to file */
        if (P_save(TEMPORARY,topath))
	  Werrprintf("cannot save display parameters in %s",topath);
      }
    }
    P_treereset(TEMPORARY);		/* clear the tree again */
  }

  else if (strcmp(argv[0],"mp")==0 || strcmp(argv[0],"mf") ==0)
  {
    int fileid;
    double tProcdim;

    disp_status("MP      ");
    if (from_current)
      fileid = CURRENT;
    else
    {
      P_treereset(TEMPORARY);		/* clear the tree first */
      D_getparfilepath(CURRENT, path, frompath);
      if (P_read(TEMPORARY,path))
      { Werrprintf("cannot read parameters from %s",path);
        P_treereset(TEMPORARY);
	if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
        disp_status("        ");
	ABORT;
      }
      fileid = TEMPORARY;
    }

    setfilepaths(0);
    D_getparfilepath(CURRENT, path, topath);
    P_getreal(fileid,"procdim", &tProcdim, 1);
    P_setreal(fileid, "procdim", 0.0, 1);
    if (P_save(fileid,path))
    {
      Werrprintf("problem copying current parameters"); 
      P_setreal(fileid, "procdim", tProcdim, 1);
      if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
      disp_status("        ");
      ABORT; 
    }

    if (to_current)
    {
      Wturnoff_buttons();
#ifdef VNMRJ
      appendTopPanelParam();
#endif 
      P_treereset(CURRENT);             /* clear CURRENT tree first */
      P_copy(TEMPORARY,CURRENT);        /* then read new parameters */
      disp_current_seq();
    }
    if (!from_current)
      P_treereset(TEMPORARY);		/* clear the tree again */
    else
      P_setreal(fileid, "procdim", tProcdim, 1);
    if (copytext(frompath,topath))
    {
      Werrprintf("cannot read and copy text file");
      if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
      disp_status("        ");
      ABORT;
    }
  }

  if (strcmp(argv[0],"mf")==0)
  {
    int fileid,len;
    double tProcdim;

    disp_status("MF      ");
    if (from_current)
      fileid = PROCESSED;
    else
    {
      P_treereset(TEMPORARY);		/* clear the tree first */
      setfilepaths(getactivefile());
      D_getparfilepath(PROCESSED, path, frompath);
      if (P_read(TEMPORARY,path))
      { Werrprintf("cannot read parameters from %s",path);
        P_treereset(TEMPORARY);
        disp_status("        ");
	if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
	ABORT;
      }
      fileid = TEMPORARY;
    }

    setfilepaths(0);
    D_getparfilepath(PROCESSED, path, topath);
    P_getreal(fileid,"procdim", &tProcdim, 1);
    P_setreal(fileid, "procdim", 0.0, 1);
    if (P_save(fileid,path))
    {
      Werrprintf("problem copying processed parameters"); 
      P_setreal(fileid, "procdim", tProcdim, 1);
      if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
      disp_status("        ");
      ABORT; 
    }
    if (to_current)
    {
#ifdef VNMRJ
      appendTopPanelParam();
#endif 
      P_treereset(PROCESSED);           /* clear PROCESSED tree */
      P_copy(TEMPORARY,PROCESSED);      /* then read new parameters */
      disp_current_seq();
    }
    if (!from_current)
      P_treereset(TEMPORARY);		/* clear the tree again */
    else
      P_setreal(fileid, "procdim", tProcdim, 1);
    if (copytext(frompath,topath))
    {
      Werrprintf("cannot read and copy text file");
      if ( !to_current ) unlockExperiment( atoi(e_to), mode_of_vnmr );
      disp_status("        ");
      ABORT;
    }
    if (to_current)
    {
      D_trash(D_USERFILE);
      D_trash(D_DATAFILE);
      D_trash(D_PHASFILE);
#ifdef VNMRJ
      clearGraphFunc();  /* prevent auto redraw of data */
#endif
    }

    remove_data(topath);
    remove_3d( topath );
    remove_shapelib( topath );
#ifdef UNIX
    strcpy(systemcall,"/bin/rm -f ");
    len = 2*MAXPATH+8 - strlen(systemcall) -1;
    strncat(systemcall,check_spaces(topath,chkbuf,len),len);
    len = 2*MAXPATH+8 - strlen(systemcall) -22;
    strncat(systemcall,"/acqfil/*;/bin/cp -p ",len);
    len = 2*MAXPATH+8 - strlen(systemcall) -1;
    strncat(systemcall,check_spaces(frompath,chkbuf,len),len);
    len = 2*MAXPATH+8 - strlen(systemcall) -11;
    strncat(systemcall,"/acqfil/* ",len);
    len = 2*MAXPATH+8 - strlen(systemcall) -1;
    strncat(systemcall,check_spaces(topath,chkbuf,len),len);
    len = 2*MAXPATH+8 - strlen(systemcall) -10;
    strncat(systemcall,"/acqfil/.",len);
#else 
    vms_fname_cat(frompath,"[.acqfil]");
    vms_fname_cat(topath,  "[.acqfil]");
    sprintf(systemcall,"copy %sfid %sfid",frompath,topath);
#endif 
    system(systemcall);
  }
  if ( !to_current )
   unlockExperiment( atoi(e_to), mode_of_vnmr );
  else {
    Wsetgraphicsdisplay("");
#ifndef VNMRJ
    execString("dg newdg menu('main')\n");
#else
    setMenuName("main");
#endif 
  }

  setfilepaths(getactivefile());
  disp_status("        ");
  RETURN;
}

/**********/
int set_nodata()
/**********/
/* set data and phasefile to empty */
{
  /* remove any phasefile */
  D_remove(D_PHASFILE);
  /* remove any data file */
  D_remove(D_DATAFILE);
  /* open curexp/acqfil/fid file */
  D_close(D_USERFILE); 
  RETURN;
}

/* exists(name,'parameter'[,tree]):$x  		*/
/* exists(name,'file'):$x                       */
/* return value:				*/
/*   0: does not exist				*/
/*   1: does exist				*/
/*************************/
int exists(int argc, char *argv[], int retc, char *retv[])
/*************************/
{ vInfo  info;
  char *name;
  int treeindex;
  int err;

  if (argc<3)
  { Werrprintf(
      "usage - exists(name,type[,tree]):$x, type='parameter','file','maclib','parlib','probes','manual'");
    ABORT;
  }
  name = argv[1];
  if (strcmp(argv[2],"parameter")==0)
    { 
        int res;
	if (argc > 3)		/* tree specified ? */
	{
	    treeindex = getTreeIndex(argv[3]);
	    if (treeindex == -1)	/* illegal tree? */
	    {
		Werrprintf( "Illegal tree '%s' specified",argv[3]);
      		ABORT;
    	    }
	}
        else
	    treeindex = CURRENT;

	if (argv[1][0] == '$')
        {
           symbol **root;
           varInfo *v;
           res = 1;
           if ( (root=selectVarTree(name)) )
           {
              if ( (v = rfindVar(name,root)) )
                  res = 0;
           }
        }
        else
        {
	   res =  P_getVarInfo(treeindex,name,&info);
        }

	if (res)
        { if (retc==0)
            Winfoprintf("parameter %s does not exist",name);
          else
            retv[0] = realString((double)0);
        }
      else
        { if (retc==0)
            Winfoprintf("parameter %s does exist",name);
          else
            retv[0] = realString((double)1);
        }
    }
  else if (strcmp(argv[2],"file")==0)
    { int perm = F_OK;
      if (argc > 3)		/* permissions specified ? */
      {
         int len = strlen(argv[3]);
         int i;
         perm = 0;
         for (i=0; i<len; i++) {
            switch (argv[3][i]) {
               case 'r': perm |= R_OK;
                         break;
               case 'w': perm |= W_OK;
                         break;
               case 'x': perm |= X_OK;
                         break;
               default:  break;
            }
         }
      }
      err = access(name,perm);
      if (err==-1) err=errno;
      if (err==0)
        { if (retc==0)
            Winfoprintf("file %s does exist",name);
          else
            retv[0] = realString((double)1);
        }
      else
        { if (retc==0)
            if (perm == F_OK)
               Winfoprintf("file %s does not exist",name);
            else
               Winfoprintf("file %s does not exist or does not have permission %s",
                            name,argv[3]);
          else
            retv[0] = realString((double)0);
        }
    }
  else if (strcmp(argv[2],"psglib")==0)
  {
      int found;
      char path[MAXPATH];

      found = appdirFind(argv[1], argv[2], path, ".c", R_OK);
      if (retc==0)
      {
         if (found)
            Winfoprintf("pulse sequence %s does exist",argv[1]);
         else
            Winfoprintf("pulse sequence %s does not exist",argv[1]);
      }
      else
      {
         retv[0] = realString((double)found);
         if (found && (retc > 1))
            retv[1] = newString(path);
      }
  }
  else if (strcmp(argv[2],"parlib")==0)
  {
      int found = 0;
      char parlib[MAXPATH];

      parlib[0] = '\0';
      found = appdirFind(argv[1], argv[2], parlib, ".par", R_OK);
      if (retc==0)
      {
         if (found)
            Winfoprintf("parlib %s does exist",parlib);
         else
            Winfoprintf("parlib %s does not exist",argv[1]);
      }
      else
      {
         retv[0] = realString((double)found);
         if (found && (retc > 1))
            retv[1] = newString(parlib);
      }
  }
  else if (strcmp( argv[ 2 ], "directory" ) == 0)
  {
      int is_a_dir;

      is_a_dir = isDirectory( argv[ 1 ] );
      if (retc==0)
      {
         if (is_a_dir)
            Winfoprintf("%s is a directory",argv[1]);
         else
            Winfoprintf("%s is not a directory",argv[1]);
      }
      else
         retv[0] = realString((double)is_a_dir);
  }
  else if (strcmp( argv[ 2 ], "ascii" ) == 0)
  {
      int is_ascii;

      is_ascii = isFileAscii( argv[ 1 ] );
      if (retc==0)
      {
         if (is_ascii)
            Winfoprintf("%s is an ascii file",argv[1]);
         else
            Winfoprintf("%s is not an ascii file",argv[1]);
      }
      else
         retv[0] = realString((double)is_ascii);
  }
  else if (strcmp( argv[2], "command" ) == 0)
  {
      int found;

      found = isACmd( argv[ 1 ] );
      if (found == 0)
      {
         found = appdirFind(argv[1], "maclib", NULL, "", R_OK);
         if (found > 0)				 /* if found as a macro */
           found++;			 /* add one to the return value */
      }			/* so commands may be distinguished from macros */
      if (retc==0)
      {
         if (found == 1)
            Winfoprintf("%s is a command",argv[1]);
         else if (found > 1)
            Winfoprintf("%s is a macro",argv[1]);
         else
            Winfoprintf("%s is neither a command nor a macro",argv[1]);
      }
      else
         retv[0] = realString((double)found);
  }
  else if (strcmp( argv[2], "interactive" ) == 0)
  {
      int found;

      found = isInteractive( argv[ 1 ] );
      if (retc == 0)
      {
         if (found != 0)
            Winfoprintf("%s is an interactive command",argv[1]);
         else
            Winfoprintf("%s is not an interactive command",argv[1]);
      }
      else
         retv[0] = realString((double)found);
  }
  else if ( (argc == 3) || (argc == 4) )
  {
      int found;
      char path[MAXPATH];

      path[0] = '\0';
      found = appdirFind(argv[1], argv[2], path, "", R_OK);
      if (retc==0)
      {
         if (found)
            Winfoprintf("%s file %s does exist",argv[2],argv[1]);
         else
            Winfoprintf("%s file %s does not exist",argv[2],argv[1]);
      }
      else
      {
         if (found)
         {
            retv[0] = realString((double)found);
         }
         else
         {
            if (argc == 4)
               retv[0] = newString(argv[3]);
            else
               retv[0] = realString((double) 0.0);
         }
         if (found && (retc > 1))
            retv[1] = newString(path);
      }
  }
  else
  {
      Werrprintf(
          "usage - exists(name,type[,tree]):$x, type='parameter','file','maclib'");
      ABORT;
  }
  RETURN;
}

static int fid_is_link(char *filepath )
{
#ifdef UNIX
	char		curexp_fid[ MAXPATH ],
			curfid_link[ MAXPATH*2 ],
			fidpath[ MAXPATH ];
	int		ival;
	dev_t		exp_dev, fid_dev;
	ino_t		exp_ino, fid_ino;
	struct stat	stat_blk;

/*
 *  Is $curexp/acqfil/fid a (symbolic) link?  If not, no
 *  special action required, so this subroutine returns.
 */
	strcpy( &curexp_fid[ 0 ], curexpdir );
	strcat( &curexp_fid[ 0 ], "/acqfil/fid" );
	if (isSymLink( &curexp_fid[ 0 ] ) != 0)
	  return( 0 );

/*  Locate the symbolic link (readlink).  */

	ival = readlink( &curexp_fid[ 0 ], &curfid_link[ 0 ], sizeof( curfid_link ) );
	if (ival < 0)
	  return( 0 );

/*  UNIX manual asserts `readlink' does not null-terminate the string;
    rather it returns the number of characters in the symbolic link path.  */

	curfid_link[ ival ] = '\0';

/*
 *  The two filepaths point to the same file if the device and I-node
 *  fields in the stat block are both identical.  Return normal status
 *  if not successful.  Most likely two reasons are the file does not
 *  exist or the process has no access.
 */
	ival = stat( &curfid_link[ 0 ], &stat_blk );
	if (ival != 0)
	  return( 0 );
	exp_ino = stat_blk.st_ino;
	exp_dev = stat_blk.st_dev;

	strcpy( &fidpath[ 0 ], filepath );
	strcat( &fidpath[ 0 ], "/fid" );
	ival = stat( &fidpath[ 0 ], &stat_blk );
	if (ival != 0)
	  return( 0 );
	fid_ino = stat_blk.st_ino;
	fid_dev = stat_blk.st_dev;

	if (fid_ino != exp_ino || fid_dev != exp_dev)
	  return( 0 );
	else
	  return( 1 );
#else 
	return( 0 );			/* no special action required on VMS */
#endif 
}

int expdir_to_expnum(char *expdir )
{
	char	tmpexpdir[MAXPATH];
	int	i,
		this_expnum;

	strcpy(tmpexpdir, expdir);
#ifndef UNIX
        tmpexpdir[strlen(tmpexpdir - 1)] = '\0';
		/* get rid of "]" */
#endif 

	i = 0;
        while ( tmpexpdir[strlen(tmpexpdir) - i - 1] != 'p' )
           i += 1;

	this_expnum = atoi( &tmpexpdir[strlen(tmpexpdir) - i] );
	return( this_expnum );
}
