/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <netinet/in.h>
#include "group.h"
#include "variables.h"
#include "lc_gem.h"
#include "lc_index.h"
#include "asm.h"
#include "data.h"
#include "pvars.h"
#include "shrexpinfo.h"
#include "abort.h"
#include "vfilesys.h"
#include "cps.h"
#include "arrayfuncs.h"
#include "CSfuncs.h"
 
/* #define TESTING  */   /* use for stand alone dbx testing */

/*----------------------------------------------------------------------------
|
|	Pulse Sequence Generator  background task envoked from parent vnmr go
|
+----------------------------------------------------------------------------*/

/****************************************/
/*		DEFINES			*/
/****************************************/
#define OK		0
#define FALSE		0
#define TRUE		1
#define ERROR		1
#define MAXSTR		256
#define MAXLOOPS	20
#define MAXPATHL	128
#define NOTFOUND	-1

#define MAXTABLE	60	/* for table implementation (aptable.h) */

#define HKDELAY 	0.080	/* house keeping delay between CTs 80 msec */
#define WLDELAY 	0.016	/* Wideline delay for software STM function */
				/*  16ms/K data */

#define PATERR	-1		/* now sisco usage, greg */

#define VM02_SIZE       11000
#define MV162_SIZE	128000

#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2)

/* For frequency lists ... */
#define ACQ_XPAN_GSEG   5

#define is_y(target)	((target == 'y') || (target == 'Y'))
#define anyrfwg		( (is_y(rfwg[0])) || (is_y(rfwg[1])) )
#define anywg		anyrfwg		/* Mercury-Vx only supports rfwg */

static void checkGradAlt();
void checkGradtype();
void first_done();
void reset();
void check_for_abort();
/****************************************/
/*		EXTERNALS		*/
/****************************************/
extern double	sw; 		/* Sweep width */
extern double	np; 		/* Number of data points to acquire */
extern double	nt; 		/* number of transients */
extern double	ni;		/* number of FID in 2D */
extern double	ni2;		/* number of FID in 3D */
extern double	ni3;		/* number of FID in 4D */
extern double	pad;		/* pre-acquisition-delay */

extern codeint	ilssval;
extern codeint	ilctss;

extern int	padflag;
extern int	ct;		/* offset in code to ct */
extern int	ctss;		/* offset in code to rtvptr */
extern int	oph;		/* offset in code to oph */
extern int	bsval;		/* offset in code to bs */
extern int	bsctr;		/* offset in code to bstr */
extern int	num_tables;	/* number of tabels created and to send */
extern int	ssval;		/* offset in code to ss */
extern int	ssctr;		/* offset in code to ssctr */
extern int	id2;		/* offset in code to id2 */
extern int	zero;		/* offset in code to zero */
extern int	one;		/* offset in code to one */
extern int	two;		/* offset in code to two */
extern int	three;		/* offset in code to three */
extern int	tablert;	/* offset in code to tablert */
extern int	v1;		/* offset in code to v1  */
extern int	v2;		/* offset in code to v2  */
extern int	v3;		/* offset in code to v3  */
extern int	v4;		/* offset in code to v4  */
extern int	v5;		/* offset in code to v5  */
extern int	v6;		/* offset in code to v6  */
extern int	v7;		/* offset in code to v7  */
extern int	v8;		/* offset in code to v8  */
extern int	v9;		/* offset in code to v9  */
extern int	v10;		/* offset in code to v10 */
extern int	v11;		/* offset in code to v11 */
extern int	v12;		/* offset in code to v12 */
extern int	v13;		/* offset in code to v13 */
extern int	v14;		/* offset in code to v14 */
extern int	v15;		/* offset in code to v15 */
extern int	v16;		/* offset in code to v16 */

extern long	rt_tab[];

extern unsigned long 	ix;	/* FID currently in Acode generation */



/****************************************/
/*		GLOABLS			*/
/****************************************/
extern autodata	*Aauto;			/* ptr to automation struct in Acodes */
extern Acqparams	*Alc;		/* ptr to lc struct in Acode set */

       char	abortfile[MAXPATHL];	/* path for abort signal file */
       char	acqHost[50];		/* Acq. machine Host name */
       char	**cnames;		/* pointer array to variable names */
       char	curexp[MAXPATHL];	/* current experiment path */
       char	*extra_hp;
       char	filegrad[MAXPATHL];	/* path for Gradient file */
       char	filepath[MAXPATHL];	/* file path for Codes */
       char	fileRFpattern[MAXPATHL];/* path for obs & dec RF pattern file */
       char	filexpan[MAXPATHL];	/* path for Future Expansion file */
       char	filexpath[MAXPATHL];	/* file path for exp# file */
       char	*gethostbyname();
       char	gradtype[MAXSTR];	/* w - wfg s-sisco n-none */
       char	seqfil[MAXSTR];
static char	infopath[256];
       char	rfwg[MAXSTR];		/* rf waveform generator flags */
       char	systemdir[MAXPATHL];	/* vnmr system directory */
       char	userdir[MAXPATHL];	/* vnmr user system directory */
       char	vnHeader[50];		/* header sent to vnmr */
       char	vnHost[50];		/* vn machine Host name */

       double	**cvals;		/* pointer array to variable values */
       double	exptime; 		/* total time for an exp estimate */
double  sw1,sw2,sw3;
       double	inc2D;			/* t1 dwell time in a 2D/3D/4D exp */
       double   d2_init = 0.0;          /* Initial value of d2 delay, used in 2D/3D/4D experiments */
       double	inc3D;			/* t2 dwell time in a 3D/4D exp */
       double   d3_init = 0.0;          /* Initial value of d3 delay, used in 2D/3D/4D experiments */
       double	inc4D;			/* t3 dwell time in a 4D exp */
       double	totaltime; 		/* total timer events for a fid */
       double	usertime = -1.0;	/* if user defines time, -1=psg does */
       double  gradalt=1.0;             /* alternating gradients */
       double  tau;                     /* general use delay */

       FILE *dpsdata;			/* display pulse sequence data file */

       int     nth2D;                  /* 2D Element currently in Acode generation (VIS usage)*/
       int	acqiflag = 0;		/* TRUE if 'acqi' was an argument */
       int	fidscanflag = 0;	/* 'fidscan' is argument, for vnmrj */
       int	tuneflag = 0;    	/* 'tune' is argument, for vnmrj */
       int	acqPid;			/* Acq. pid for async message usage */
       int	acqPort;		/* Acq. message port for acquisition */
       int	arrayelements;		/* number of array elements */
       int	bgflag = 0;
       int	checkflag;		/* only check seq, no acq, if present */
       int	nomessageflag;		/* only check seq, no acq, if present */
       int	clr_at_blksize_mode = 0;
       int	debug = 0;
       int      initializeSeq = 1;
       int	dps_flag;		/* dps flag */
       int	HSlines;		/* High Speed output board lines */
       int	presHSlines;		/* last output of HS output brd lines */
       int	ncvals;			/* NUMBER OF VARIABLE  values */
       int	newacq=0;		/* TRUE if new acq/Expproc is used */
       int	nnames;			/* number of variable names */
       int	ntotal;			/* total number of variable names */
/*  RF Channels */
int OBSch=1;                    /* The Acting Observe Channel */
int DECch=2;                    /* The Acting Decoupler Channel */
int DEC2ch=3;                   /* The Acting 2nd Decoupler Channel */
int DEC3ch=4;                   /* The Acting 3rd Decoupler Channel */
int DEC4ch=5;                   /* The Acting 4th Decoupler Channel */
       int   d2_index = 0;      /* d2 increment (from 0 to ni-1) */
       int   d3_index = 0;      /* d3 increment (from 0 to ni2-1) */
       int	NUMch=2;		/* Number of channels configured */
extern int	ok;			/* global error flag */
       int	ok2bumpflag;
       int	oldwhenshim;		/* previous value of shimming mask */
static int	pipe1[2];
static int	pipe2[2];
       int	PSfile = 0;			/* file discriptor for Acode file */
       int	ra_flag;		/* ra flag */
static int	ready = 0;              /* go.c expects 0 for success */
       int	setupflag;		/* alias used to invoke PSG */
					/* go=0,su=1,etc*/
       int	vnPort;			/* vn message port for acquisition */
       int	vnPid;			/* vn pid for async message usage */
       int	waitflag;		/* if 'next' or 'sync' is an argument, */
					/* then hold go until psg is done */
       int      prepScan = 0;	/* if 'prep' was an argument, then wait for sethw to start */
       int	whenshim;
       int	specialGradtype = 0;

       long	CodeEnd;	/* End Address  of the malloc space for codes */
       long	Codesize;	/* size of the malloc space for codes */

extern short	*Aacode; 	/* pointer to Acode array, also Start Address */
       short	*Codeptr; 	/* pointer into the Acode array */
       short	*Codes;		/* pointer to the start of the Acodes array */
       short	*codestadr;	/* acode start address in Codes */
extern short	 fidctr;	/* offset into code to bsct */
extern short	*lc_stadr;	/* Low Core Start Address */
       short	*preCodes;	/* pointer to the start of the malloced space */

unsigned long	start_elem;	/* elem (FID) for acquisition to start on (RA)*/
unsigned long	completed_elem;	/* number of completed elements (FIDs) (RA) */
/**************************************
*  Structure for real-time AP tables  *
*  and global variables declarations  *
**************************************/

short	  	t1,  t2,  t3,  t4,  t5,  t6, 
                t7,  t8,  t9,  t10, t11, t12, 
                t13, t14, t15, t16, t17, t18, 
                t19, t20, t21, t22, t23, t24,
                t25, t26, t27, t28, t29, t30,
                t31, t32, t33, t34, t35, t36,
                t37, t38, t39, t40, t41, t42,
                t43, t44, t45, t46, t47, t48,
                t49, t50, t51, t52, t53, t54,
                t55, t56, t57, t58, t59, t60;

int		table_order[MAXTABLE],
		tmptable_order[MAXTABLE],
		loadtablecall,
		last_table;

struct _Tableinfo
{
   int		reset;
   int		table_number;
   int		*table_pointer;
   int		*hold_pointer;
   int		table_size;
   int		divn_factor;
   int		auto_inc;
   int		wrflag;
   short	indexptr;
   short	destptr;
};

typedef	struct _Tableinfo	Tableinfo;

Tableinfo	*Table[MAXTABLE];

SHR_EXP_STRUCT	ExpInfo;

/**********************************************
*  End table structures and global variables  *
**********************************************/


/*------------------------------------------------------------------------
|
|	This module is a child task.  It is called from vnmr and is passed
|	pipe file descriptors. Option are passed in the parameter go_Options.
|       This task first transfers variables passed through the pipe to this
|	task's own variable trees.  Then this task can do what it wants.
|
|	The go_Option parameter passed from parent can include
|         debug which turns on debugging.
|         next  which for automation, puts the acquisition message at the
|               top of the queue
|         acqi  which indicates that the interactive acquisition program
|               wants an acode file.  No message is sent to Acqproc.
|	  ra    which means the VNMR command is RA, not GO
+-------------------------------------------------------------------------*/


#ifndef SWTUNE
main(argc,argv) 	int argc; char *argv[];
{   
    char    array[MAXSTR];
    char    parsestring[MAXSTR];	/* string that is parsed */
    char    arrayStr[MAXSTR];		/* string that is not parsed */
    char    filename[MAXPATHL];		/* file name for Codes */
    char   *gnames[50];
    char   *appdirs;

    double arraydim;	/* number of fids (ie. set of acodes) */
    double hkdelay;
    double sign_add();

    int     ngnames;
    int     i;
    int     narrays;
    int     nextflag = 0;
    int     Rev_Num;
    int     P_rec_stat;
  
    acqiflag = ra_flag = dps_flag = waitflag = fidscanflag = tuneflag = checkflag = 0;
    ok = TRUE;

#ifndef TESTING

#ifdef DBXTOOL
    sleep(70);	/* allow 70sec for person to attach with DBXTOOL */
#endif

    setupsignalhandler();  /* catch any exception that might core dump us. */

    if (argc < 7)  /* not enought args to start, exit */
    {	fprintf(stderr,
	"This is PSG, a background task! Only execute from within 'vnmr'!\n");
	exit(1);
    }
    Rev_Num = atoi(argv[1]); /* GO -> PSG Revision Number */
    pipe1[0] = atoi(argv[2]);
    pipe1[1] = atoi(argv[3]);
    pipe2[0] = atoi(argv[4]); /* convert file descriptors for pipe 2*/
    pipe2[1] = atoi(argv[5]);
    setupflag = atoi(argv[6]);	/* alias flag */
    
    if (bgflag)
    {
       for (i=12; i<argc; i++)  /* display flags */
    	  fprintf(stderr,"argv[%d] = '%s'\n",i,argv[i]);
       fprintf(stderr,"\n BackGround PSG starting\n");
       fprintf(stderr,"setupflag = %d vnHost='%s',vnPort=%d,vnPid=%d\n",
	  setupflag,vnHost,vnPort,vnPid);
    }
 
    close(pipe1[1]); /* close write end of pipe 1*/
    close(pipe2[0]); /* close read end of pipe 2*/
    P_rec_stat = P_receive(pipe1);  /* Receive variables from pipe and load trees */    
    close(pipe1[0]);
    /* check options passed to psg */
    if (option_check("next"))
       waitflag = nextflag = 1;
    if (option_check("ra"))
       ra_flag = 1;
    if (option_check("acqi"))
       acqiflag = 1;
    if (option_check("fidscan"))
       fidscanflag = 1;
    if (option_check("tune") || option_check("qtune"))
       tuneflag = 1;
    if (option_check("debug"))
       bgflag++;
    if (option_check("sync"))
       waitflag = 1;
    if (option_check("prep"))
       prepScan = 1;
    if (option_check("check"))
    {
       dps_flag = checkflag = 1;
    }
    clr_at_blksize_mode = option_check("bsclear");
    debug = bgflag;
    gradtype_check();

    if (bgflag)
      fprintf(stderr,"PSG: Piping Complete\n");
 
    getparm("acqaddr","string",GLOBAL,filename,MAXPATHL);
    sscanf(filename, "%s", acqHost);
    extra_hp = gethostbyname(acqHost);
    /* ------- Check For GO - PSG Revision Clash ------- */
/* FV    if (Rev_Num != GO_PSG_REV )
 * FV    {	
 * FV	char msge[100];
 * FV	sprintf(msge,"GO(%d) and PSG(%d) Revision Clash, PSG Aborted.\n",
 * FV	  Rev_Num,GO_PSG_REV);
 * FV	text_error(msge);
 * FV	abort(1);
 * FV    } */
    if (P_rec_stat == -1 )
    {	
	abort_message("P_receive had a fatal error.\n");
    }

#else   /* TESTING */

    for (i=0; i<argc; i++)  /* set debug flags */
    {
        int j,len;

        len = strlen(argv[i]);
        j = 0;
        while (j < len)
        {
           if (argv[i][j] == '-')
           {
	      if (argv[i][j+1] == 'd')
	         bgflag++;
	      else if (argv[i][j+1] == 'a')
                 acqiflag = 1;
              else if (argv[i][j+1] == 'c')
                 checkflag = 1;
	      else if (argv[i][j+1] == 'f')
                 fidscanflag = 1;
	      else if (argv[i][j+1] == 'n')
                 nextflag = 1;
	      else if (argv[i][j+1] == 'r')
                 ra_flag = 1;
           }
           j++;
        }
    }
    debug = bgflag;

    setupflag = 0 ;	/* alias flag */

    P_treereset(GLOBAL);
    P_treereset(CURRENT);
    /* --- read in combined conpar & global parameter set --- */
    if (P_read(GLOBAL,"conparplus") < 0)       /* read in parameters */
       fprintf(stderr,"P_read failed: '%s'\n","curpar");
    /* --- read in curpar parameter set --- */
    if (P_read(CURRENT,"curpar") < 0)       /* read in parameters */
       fprintf(stderr,"P_read failed: '%s'\n","curpar");


#endif  /*  TESTING */
    /*-----------------------------------------------------------------
    |  begin of PSG task
    +-----------------------------------------------------------------*/

    if (strncmp("dps", argv[0], 3) == 0)
    {
        dps_flag = 1;
        if ((int)strlen(argv[0]) >= 5)
           if (argv[0][3] == '_')
                dps_flag = 3;
        checkflag = acqiflag = 0;
    }

    /* null out optional file */
    fileRFpattern[0] = 0;	/* path for Obs & Dec RF pattern file */
    filegrad[0] = 0;		/* path for Gradient file */
    filexpan[0] = 0;		/* path for Future Expansion file */

    getparm("acqaddr","string",GLOBAL,filename,MAXPATHL);
    /*newacq = (strcmp(filename,"Expproc") == 0 );*/
    newacq = is_newacq();
    A_getstring(CURRENT,"appdirs", &appdirs, 1);
    setAppdirValue(appdirs);
    setup_comm();   /* setup communications with acquisition processes */

#ifdef TESTING
    strcpy(filename,"stdalone");
#else
    getparm("goid","string",CURRENT,filename,MAXPATHL);
#endif

    if (!dps_flag)
       if (setup_parfile(setupflag))
       {   abort_message("Parameter file error PSG Aborted..\n");
       }


    strcpy(filepath,filename);	/* /vnmrsystem/acqqueue/exp1.greg.012345 */
    strcpy(filexpath,filepath); /* save this path for psg_abort() */
    strcat(filepath,".Code");	/* /vnmrsystem/acqqueue/exp1.greg.012345.Code */
    if (acqiflag)
    {
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
	P_setstring(CURRENT,"ss","n",1);
	P_setstring(CURRENT,"dp","y",1);
	P_getreal(CURRENT,"np",&np,1);
        if (newacq)                          /* set nt=1 for go('acqi') */
          P_setreal(CURRENT,"nt",1.0,1);     /* on the new digital console */

        unlink(filexpath);
        unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".IPA");
	unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".Code.lock");
	unlink(filepath);
        if (newacq)
        {  strcpy(filepath,filexpath);
           strcat(filepath,".Tables");
           unlink(filepath);
           strcpy(filepath,filexpath);
           strcat(filepath,".RTpars");
           unlink(filepath);
           strcpy(filepath,filexpath);
           strcat(filepath,".RF");
           unlink(filepath);
        }

        strcpy(filepath,filexpath);	/* restore filepath to .Code path */
        strcat(filepath,".Code");

        umask(0);
    }
    if (fidscanflag)
    {
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
        P_setstring(CURRENT,"ss","n",1);
        P_setstring(CURRENT,"dp","y",1);
        P_setstring(CURRENT,"cp","n",1);
        P_setreal(CURRENT,"bs",1.0,1);
     /* next line doesn't work; needs wbs('fid_display') */
        P_setstring(CURRENT,"wbs","fid_display",1); 
        P_setstring(CURRENT,"wdone","",1);
        if (newacq)
          P_setreal(CURRENT,"nt",1e6,1);
    }
    if (tuneflag)
    {
        int autoflag;
        char autopar[12];
        vInfo info;

        if (getparm("auto","string",GLOBAL,autopar,12))
          autoflag = 0;
        else
          autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));

        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
        P_setstring(CURRENT,"ss","n",1);
        P_setstring(CURRENT,"dp","y",1);
        P_setstring(CURRENT,"cp","n",1);
/*      if (newacq)
          P_setreal(CURRENT,"nt",1e6,1);
*/
        P_setstring(GLOBAL,"dsp","n",1);
        if (P_getVarInfo(CURRENT,"oversamp",&info) == 0)
          P_setreal(CURRENT,"oversamp",1,1);
        if (autoflag == 0)
        {
          P_setreal(GLOBAL,"vttype",0.0,1);
          P_setreal(GLOBAL,"traymax",0.0,1);
          P_setreal(GLOBAL,"loc",0.0,1);
        }
    }

    checkGradtype();
    checkGradAlt();

    if (!dps_flag && !checkflag && !option_check("checkarray"))
    {
       if (bgflag)
         fprintf(stderr,"Opening Code file: '%s' \n",filepath);
       unlink(filepath);
       PSfile = open(filepath,O_WRONLY | O_CREAT,0666);
       if (PSfile < 0)
       {	abort_message("code file already exists PSG Aborted..\n");
       }
    }

    if (ra_flag && !newacq)   /* if ra then open acqpar parameter file */
    {
	char acqparpath[MAXSTR];/* file path to acqpar for ra */
       /* --- set acqpar file path for ra --- */
       if (getparm("exppath","string",CURRENT,acqparpath,MAXSTR))
    	return(ERROR);
       strcat(acqparpath,"/acqpar");
       if (bgflag)
         fprintf(stderr,"get acqpar path  = %s \n",acqparpath);
       open_acqpar(acqparpath);
    }

    A_getnames(GLOBAL,gnames,&ngnames,50);
    if (bgflag)
    {
        fprintf(stderr,"Number of names: %d \n",nnames);
        for (i=0; i< ngnames; i++)
        {
          fprintf(stderr,"global name[%d] = '%s' \n",i,gnames[i]);
        }
    }

    /* --- setup the arrays of variable names and values --- */

    /* --- get number of variables in CURRENT tree */
    A_getnames(CURRENT,0L,&ntotal,1024); /* get number of variables in tree */
    if (bgflag)
        fprintf(stderr,"Number of variables: %d \n",ntotal);
    cnames = (char **) malloc(ntotal * (sizeof(char *))); /* allocate memory */
    cvals = (double **) malloc(ntotal * (sizeof(double *)));
    if ( cvals == 0L  || cnames == 0L )
    {
	text_error("insuffient memory for variable pointer allocation!!");
	reset(); 
	psg_abort(0);
    }
    /* --- load pointer arrays with pointers to names and values --- */
    A_getnames(CURRENT,cnames,&nnames,ntotal);
    A_getvalues(CURRENT,cvals,&ncvals,ntotal);
    if (bgflag)
        fprintf(stderr,"Number of names: %d \n",nnames);
    if (bgflag > 2)
    {
        for (i=0; i< nnames; i++)
        {
	    if (whattype(CURRENT,cnames[i]) == ST_REAL)
	    {
	        fprintf(stderr,"cname[%d] = '%s' value = %lf \n",
	   	    i,cnames[i],*cvals[i]);
	    }
	    else
	    {
	        fprintf(stderr,"name[%d] = '%s' value = '%s' \n",
	   	    i,cnames[i],cvals[i]);
	    }
        }
    }

    /* variable look up now done with hash tables */
    init_hash(ntotal);
    load_hash(cnames,ntotal);

    /* --- malloc space for Acodes --- */
    if (getparm("arraydim","real",CURRENT,&arraydim,1))
	psg_abort(1);

    if (acqiflag || (setupflag > 0))	/* if no a GO then force to 1D */
        arraydim = 1.0;		        /* any setup is 1D, nomatter what. */

    /* For su, send command to magnet leg with protune modification */
    if (setupflag == 1)
    {
       char atune[MAXSTR];
       if ( ! P_getstring(GLOBAL, "atune",   atune, 1, MAXSTR-4) )
       {
          if ( (atune[0] == 'y') || (atune[0] == 'Y') )
             system("echo 'tune 0' | nc v-protune 23\n");
       }
    }
/*----------------------------------------------------------------
| --- malloc space for Acodes ---
|
|   Codes	- a. Low Core	          pointer = Alc;
|		- b. Auto Structure       pointer = Aauto;
|		- c. Acodes		  pointer = Aacode;
+------------------------------------------------------------------*/
    if (newacq)
       Codesize =  (long)  (MV162_SIZE * sizeof(codeint)); /* code + struct */
    else
       Codesize =  (long)  (VM02_SIZE * sizeof(short));    /* code + struct */

    if (bgflag)
    {
	fprintf(stderr,"arraydim = %5.0lf \n",arraydim);
        fprintf(stderr,"sizeof Codes: %ld(dec) bytes \n", Codesize);
    }

    preCodes = (short *) malloc( Codesize + 2 * sizeof(long) );
    if ( preCodes == 0L )
    {
        char msge[128];
	sprintf(msge,"Insuffient memory for Acode allocation of %ld Kbytes.",
		Codesize/1000L);
	text_error(msge);
	reset(); 
	psg_abort(0);
    }
    Codes = preCodes + 4;

    CodeEnd = ((long) Codes) + Codesize;

    /* Set Acode pointer to beginning of Codes */
    Codeptr = (short *)init_acodes(Codes);

    /* Set up Acode pointers */
/*    Alc = (Acqparams *) Codes; */	/* start of low core */
/*    lc_stadr = Codes; */
/*    Aauto = (autodata *) (Alc + 1) ; *//* start of auto struc */
/*    Aacode = (short *) (Aauto + 1); */
/*    Codeptr = Aacode; */
/* */
/*    fidctr = (short) (((short *) &(Alc->elemid)) - lc_stadr); *//*offsetelemid*/
/*    fidctr += 1; *//* since real time offset are used as integers & */
/*	 */	/* fidctr is long we must shift the offset by 1 word */

    if (bgflag)
    {	fprintf(stderr,"Code address:  0x%lx \n",Codes);
     	fprintf(stderr,"Codeptr address:  0x%lx \n",Codeptr);
     	fprintf(stderr,"CodeEnd address:  0x%lx \n",CodeEnd);
    }
	
    oldwhenshim = -1;	   /* previous value of shimming mask */
    exptime  = 0.0; 	   /* total timer events for a exp duration estimate */
    start_elem = 1;        /* elem (FID) for acquisition to start on (RA)*/
    completed_elem = 0;    /* total number of completed elements (FIDs)  (RA) */

    if (newacq)
       clearHSlines();
    if (setGflags()) psg_abort(0);
    
    initeventovrhd(1);

    if ( ! newacq )
       init_shimnames(GLOBAL);
    readparams();
    convertparams(1);
    if ( newacq )
       initautostruct();

    if (newacq)
       if (( ! dps_flag) && anywg)
          init_wg_list();

    if (newacq)
       init_global_list(ACQ_XPAN_GSEG);

    setupPsgFile();
    ix = nth2D = 0;
    if (bgflag)
       fprintf(stderr,"arraydim = %f\n",arraydim);
    init_compress((int) arraydim);
    if (checkflag || (dps_flag > 1))
        arraydim = 1.0;
    if (option_check("checkarray"))
    {
       checkflag = 1;
    }

    ni=0; ni2=0; ni3=0;
    if (arraydim <= 1.5)
    {
        totaltime  = 0.0; /* total timer events for a fid */
	ix = nth2D =  1;		/* make Acodes for FID 1 */
	if (dps_flag && ! checkflag)
        {
              createDPS(argv[0], curexp, arraydim, 0, NULL, pipe2[1]);
              close_error(0);
              exit(0);
        }

	meat();
        totaltime -= pad;
	if (var_active("ss",CURRENT))
        {
           int ss = (int) (sign_add(getval("ss"), 0.0005));
           if (ss < 0)
              ss = -ss;
           totaltime *= (getval("nt") + (double) ss);   /* mult by NT + SS */
        }
        else
           totaltime *= getval("nt");   /* mult by number of transients (NT) */
	exptime += (totaltime + pad);
        first_done();
    }
    else	/*  Arrayed Experiment .... */
    {
        /* --- initial global variables value pointers that can be arrayed */
        initglobalptrs();
        initlpelements();

    	/* --- malloc space for structure and arrayes --- */
        narrays = arrayelements = initlpelements();  /* return # of array elements */

    	if (getparm("array","string",CURRENT,array,MAXSTR))
	    psg_abort(1);
    	if (bgflag)
	    fprintf(stderr,"array: '%s' \n",array);
        strcpy(parsestring,array);
        /*----------------------------------------------------------------
        |	test for presence of ni, ni2, and ni3
	|	generate the appropriate 2D/3D/4D experiment
        +---------------------------------------------------------------*/
	P_getreal(CURRENT, "ni",   &ni, 1);
	P_getreal(CURRENT, "ni2",  &ni2, 1);
	P_getreal(CURRENT, "ni3", &ni3, 1);
        if (bgflag)
           fprintf(stderr,"ni=%f\n",ni);

        if (  setup4D(ni3, ni2, ni, parsestring, array)  )
           psg_abort(0);
	
	if (dps_flag)
           strcpy(arrayStr, parsestring);

        /*----------------------------------------------------------------*/

	if (bgflag)
	  fprintf(stderr,"parsestring: '%s' \n",parsestring);
    	if (parse(parsestring, &narrays))  /* parse 'array' setup looping elements */
          psg_abort(1);
        arrayelements = narrays;

	if (bgflag)
        {
          printlpel();
        }
	if (dps_flag)
        {
              createDPS(argv[0], curexp, arraydim, narrays, arrayStr, pipe2[1]);
              close_error(0);
              exit(0);
        }

/*        pre_expsequence(); */      /* user special function once per Exp */
    	arrayPS(0,narrays);
    }
    if (checkflag)
    {
       if (!option_check("checksilent"))
          text_message("go check complete\n");
       closeCmd();
       close_error(0);
       exit(0);
    }


    if (PSfile)
       close_codefile();

    if (anywg)
       wg_close();

#ifdef VIS_ACQ
   /* ---- Finish pattern cleanup ---- */
    if (bgflag)
        fprintf(stderr,"calling p_endarrayelem()\n");
    p_endarrayelem();           /* move this to end of each element for
                                   arrayed pattern implementation */
    if (bgflag)
        fprintf(stderr,"calling closeshapedpatterns()\n");
    if(closeshapedpatterns(1) == PATERR) /* free pattern link list & file */
	psg_abort(1);
#endif
 
    hkdelay = HKDELAY;
    if (newacq)
    {  close_global_list();
       hkdelay = 0.002;
    }
    exptime += (hkdelay * getval("nt")) * arraydim;

    if (var_active("ss",CURRENT))
    {
       int ss = (int) (sign_add(getval("ss"), 0.0005));
       if (ss < 0)
          exptime += (hkdelay * (double) -ss) * arraydim;
       else
          exptime += (hkdelay * (double) ss);
    }

    if (bgflag)
    {
      fprintf(stderr,"total time of experiment = %lf (sec) \n",totaltime);
      fprintf(stderr,"Submit experiment to acq. process \n");

      fprintf(stderr,"PSG: vnHost:'%s', vnPort %d, vnPid %d\n",
            vnHost,vnPort,vnPid);
    }
    write_shr_info(exptime);
    if (!acqiflag && QueueExp(filexpath,nextflag))
       psg_abort(1);
    if (bgflag)
      fprintf(stderr,"PSG terminating normally\n");
    close_error(0);
    exit(0);
}
#else /* SWTUNE */

extern int	bb_refgen, cardb, xltype;

psgsetup(in_pipe, bugflag)
  int in_pipe;
  int bugflag;
{   
    int P_rec_stat;
    double freq_list[1024];
    int nfreqs = 20;
    int i;
    int j;
    int nextflag = 0;

    char    filename[MAXPATHL];		/* file name for Codes */
    char   *gnames[50];

    double arraydim;	/* number of fids (ie. set of acodes) */

    int     ngnames;

    codeint *pfreq_acodes;
    codeint *pci;
    int n;

    pipe1[0] = in_pipe;
    debug = bgflag = bugflag;
    acqiflag = 1;

    setupflag = 0 ;	/* alias flag */

    P_treereset(GLOBAL);
    P_treereset(CURRENT);
#ifdef STANDALONE
    /* --- read in combined conpar & global parameter set --- */
    if (P_read(GLOBAL,"conparplus") < 0)       /* read in parameters */
       fprintf(stderr,"P_read failed: '%s'\n","curpar");
    /* --- read in curpar parameter set --- */
    if (P_read(CURRENT,"curpar") < 0)       /* read in parameters */
       fprintf(stderr,"P_read failed: '%s'\n","curpar");
#else /* not STANDALONE */
    /* Receive variables from pipe and load trees */    
    P_rec_stat = P_receive(pipe1);
    close(pipe1[0]);
    if (P_rec_stat == -1 ){	
	abort_message("P_receive had a fatal error.\n");
    }
#endif /* not STANDALONE */

    P_creatvar(GLOBAL, "userdir", T_STRING); /* To avoid error messages */
    P_creatvar(GLOBAL, "curexp", T_STRING); /* To avoid error messages */
    P_creatvar(GLOBAL, "systemdir", T_STRING);
    P_setstring(GLOBAL, "systemdir", getenv("vnmrsystem"), 0);

    getparm("acqaddr","string",GLOBAL,filename,MAXPATHL);
    /*newacq = (strcmp(filename,"Expproc") == 0);*/
    newacq = is_newacq();

    strcpy(filexpath,getenv("vnmrsystem"));
    strcat(filexpath,"/acqqueue/qtune");

    A_getnames(GLOBAL,gnames,&ngnames,50);
    if (bgflag)
    {
        fprintf(stderr,"Number of names: %d \n",nnames);
        for (i=0; i< ngnames; i++)
        {
          fprintf(stderr,"global name[%d] = '%s' \n",i,gnames[i]);
        }
    }

    /* --- setup the arrays of variable names and values --- */

    /* --- get number of variables in CURRENT tree */
    A_getnames(CURRENT,0L,&ntotal,1024); /* get number of variables in tree */
    if (bgflag)
        fprintf(stderr,"Number of variables: %d \n",ntotal);
    cnames = (char **) malloc(ntotal * (sizeof(char *))); /* allocate memory */
    cvals = (double **) malloc(ntotal * (sizeof(double *)));
    if ( cvals == 0L  || cnames == 0L )
    {
	text_error("insuffient memory for variable pointer allocation!!");
	reset(); 
	psg_abort(0);
    }
    if (bgflag)
        fprintf(stderr," address of cname & cvals: %lx, %lx \n",cnames,cvals);
    /* --- load pointer arrays with pointers to names and values --- */
    A_getnames(CURRENT,cnames,&nnames,ntotal);
    A_getvalues(CURRENT,cvals,&ncvals,ntotal);
    if (bgflag)
        fprintf(stderr,"Number of names: %d \n",nnames);
    if (bgflag > 2)
    {
        for (i=0; i< nnames; i++)
        {
	    if (whattype(CURRENT,cnames[i]) == ST_REAL)
	    {
	        fprintf(stderr,"cname[%d] = '%s' value = %lf \n",
	   	    i,cnames[i],*cvals[i]);
	    }
	    else
	    {
	        fprintf(stderr,"name[%d] = '%s' value = '%s' \n",
	   	    i,cnames[i],cvals[i]);
	    }
        }
    }

    /* variable look up now done with hash tables */
    init_hash(ntotal);
    load_hash(cnames,ntotal);

    arraydim = 1.0;		/* SWTUNE is 1D, nomatter what. */

    Codesize =  (long)  (MV162_SIZE * sizeof(codeint)); /* code + struct */

    if (bgflag)
    {
	fprintf(stderr,"arraydim = %5.0lf \n",arraydim);
        fprintf(stderr,"sizeof Codes: %ld(dec) bytes \n", Codesize);
    }

    preCodes = (short *) malloc( Codesize + 2 * sizeof(long) );
    if ( preCodes == 0L )
    {
        char msge[128];
	sprintf(msge,"Insuffient memory for Acode allocation of %ld Kbytes.",
		Codesize/1000L);
	text_error(msge);
	reset(); 
	psg_abort(0);
    }
    Codes = preCodes + 4;

    CodeEnd = ((long) Codes) + Codesize;

    /* Set Acode pointer to beginning of Codes */
    Codeptr = (short *)init_acodes(Codes);

    if (bgflag)
    {	fprintf(stderr,"Code address:  0x%lx \n",Codes);
     	fprintf(stderr,"Codeptr address:  0x%lx \n",Codeptr);
     	fprintf(stderr,"CodeEnd address:  0x%lx \n",CodeEnd);
    }

    if (setGflags()) psg_abort(0);
    HSlines = 0;	/* Clear all High Speed line bits */
    presHSlines = 0;		/* Clear present High Speed line bits */

    /* --- next two programs setup stuff so frequencies may be programmed  -- */
    readparams();
    convertparams(1);
    /* --- set up automation data sturcture  --- */
    initautostruct();

    init_global_list(ACQ_XPAN_GSEG);
    setupPsgFile();
}

/*  Note:  pulsesequence calls Werrprintf not just to flag any
           attempt to call it from the qtune program, but also
           to force a reference to stubs.o, part of psglib.
           Programs in paramlib refer to entry points in stubs.o,
           so that stubs.o must have been loaded if paramlib is
           to be used.  But because paramlib is searched AFTER
           psglib, the link process will fail unless some program
           in psglib has already made a reference to an entry
           point in stubs.o.   November 1997			*/
           

void pulsesequence()
{
    Werrprintf( "pulse sequence called from SWTUNE version of psglib\n" );
}

sendExpproc()
{
}
#endif /* SWTUNE */

/*--------------------------------------------------------------------------
|   This writes to the pipe that go is waiting on to decide when the first
|   element is done.
+--------------------------------------------------------------------------*/
void first_done()
{
   if ( ! checkflag )
      write_exp_info();
   write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done with first elem */
}
/*--------------------------------------------------------------------------
|   This closes the pipe that go is waiting on when it is in automation mode.
|   This procedure is called from close_error.  Close_error is called either
|   when PSG completes successfully or when PSG calls psg_abort.
+--------------------------------------------------------------------------*/
void closepipe2(int success)
{
    char autopar[12];


    if (acqiflag || waitflag)
       write(pipe2[1],&success,sizeof(int)); /* tell go, PSG is done */
    else if (!dps_flag && (getparm("auto","string",GLOBAL,autopar,12) == 0))
        if ((autopar[0] == 'y') || (autopar[0] == 'Y'))
            write(pipe2[1],&success,sizeof(int)); /* tell go, PSG is done */
    close(pipe2[1]); /* close write end of pipe 2*/
}
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
void reset()
{
    if (cnames) free(cnames);
    if (cvals) free(cvals);
    if (preCodes) free(preCodes);
/*
    for (i=0;i<MAXLOOPS;i++) 
	if (lpel[i]) free(lpel[i]);
*/
}
/*------------------------------------------------------------------------------
|
|	setGflag()
|
|	This function sets the global flags and variables that do not change
|	during the pulse sequence 
|       the 2D experiment 
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Added Code for new Objects, 
|			     Ext_Trig, ToLnAttn, DoLnAttn, HS_Rotor.
|			  2. Attn Object has changed to use the OFFSET rather
|			     than the MAX to determine the directional offset 
|			     (i.e. works forward or backwards)
+----------------------------------------------------------------------------*/
int setGflags()
{
    extern int    Attn_Device(),APBit_Device(),Event_Device();
    extern int    Freq_Device();

    /* --- discover waveformers and gradients --- */
    rfwg[0]='\0';
    if (P_getstring(GLOBAL, "rfwg", rfwg, 1, MAXSTR-4) < 0)
       strcpy(rfwg,"nnnn");
    else
       strcat(rfwg,"nnnn");
/* WFG */
    strcpy(rfwg,"nnnn");	/* For now force to not exist */

    gradtype[0]='\0';
    if (P_getstring(GLOBAL, "gradtype", gradtype, 1, MAXSTR-4) < 0)
       strcpy(gradtype,"nnnn");
    else
       strcat(gradtype,"nnnn");

/*****************************************
*  Load userdir, systemdir, and curexp.  *
*****************************************/

    if (P_getstring(GLOBAL, "userdir", userdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current user directory\n");
    if (P_getstring(GLOBAL, "systemdir", systemdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current system directory\n");
    if (P_getstring(GLOBAL, "curexp", curexp, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current experiment directory\n");
    P_getstring(CURRENT, "seqfil", seqfil, 1, MAXPATHL-1);
    sprintf(abortfile,"%s/acqqueue/psg_abort", systemdir);
    if (access( abortfile, W_OK ) == 0)
       unlink(abortfile);

    return(OK);
}

static int findWord(const char *key, char *instr)
{
    char *ptr;
    char *nextch;

    ptr = instr;
    while ( (ptr = strstr(ptr, key)) )
    {
       nextch = ptr + strlen(key);
       if ( ( *nextch == ',' ) || (*nextch == ')' ) || (*nextch == '\0' ) )
       {
          return(1);
       }
       ptr += strlen(key);
    }
    return(0);
}

/*--------------------------------------------------------------------------
|
|       setup4D(ninc_t3, ninc_t2, ninc_t1, parsestring, arraystring)/5
|
|       This function arrays the d4, d3 and d2 variables with the correct
|       values for the 2D/3D/4D experiment.  d2 increments first, then d3,
|       and then d4.
|
+-------------------------------------------------------------------------*/
int setup4D(ni3, ni2, ni, parsestr, arraystr)
char    *parsestr;
char    *arraystr;
double  ni3,
        ni2,
        ni;
{
   int          index = 0,
                i,
                num = 0;
   int          elCorr = 0;
   double       sw1,
                sw2,
                sw3,
                d2,
                d3,
                d4;
   int          sparse = 0;
   int          CStype = 0;
   int          arrayIsSet;

   if ( ! CSinit("sampling", curexp) )
   {
      int res;
      res =  CSgetSched(curexp);
      if (res)
      {
         if (res == -1)
            abort_message("Sparse sampling schedule not found");
         else if (res == -2)
            abort_message("Sparse sampling schedule is empty");
         else if (res == -3)
            abort_message("Sparse sampling schedule columns does not match sparse dimensions");
      }
      else
         sparse =  1;
   }
   arrayIsSet = 0;

   strcpy(parsestr, "");
   if (sparse)
   {
      num = getCSdimension();
      if (num == 0)
         sparse = 0;
   }

   if (sparse)
   {
      int narray;
      int lps;
      int i;
      int j,k;
      int found = -1;
      int chk[CSMAXDIM];

      strcpy(parsestr, arraystr);
      if (parse(parsestr, &narray))
         abort_message("array parameter is wrong");
      strcpy(parsestr, "");
      lps = numLoops();
      CStype = CStypeIndexed();
      for (i=0; i < num; i++)
         chk[i] = 0;
      for (i=0; i < lps; i++)
      {
         for (j=0; j < varsInLoop(i); j++)
         {
            for (k=0; k < num; k++)
            {
               if ( ! strcmp(varNameInLoop(i,j), getCSpar(k) ) )
               {
                  chk[k] = 1;
                  found = i;
               }
            }
         }
      }
      k = 0;
      for (i=0; i < num; i++)
         if (chk[i])
            k++;
      if (k > 1)
      {
         abort_message("array parameter is inconsistent with sparse data acquisition");
      }
      if (found != -1)
      {
         i = 0;
            for (j=0; j < varsInLoop(found); j++)
            {
               for (k=0; k < num; k++)
               {
                  if ( ! strcmp(varNameInLoop(found,j), getCSpar(k) ) )
                     i++;
               }
            }
         if (i != num)
            abort_message("array parameter is inconsistent with sparse data acquisition");
      }

      if (num == 1)
         strcpy(parsestr, getCSpar(0));
      else
      {
         strcpy(parsestr, "(");
         for (k=0; k < num-1; k++)
         {
           strcat(parsestr, getCSpar(k));
           strcat(parsestr, ",");
         }
         strcat(parsestr, getCSpar(num-1));
         strcat(parsestr, ")");
      }
   }

   if (ni3 > 1.5)
   {
      if (getparm("sw3", "real", CURRENT, &sw3, 1))
         return(ERROR);
      if (getparm("d4","real",CURRENT,&d4,1))
         return(ERROR);

      strcpy(parsestr, "d4");

      inc4D = 1.0/sw3;
      num = (int) (ni3 - 1.0 + 0.5);
      for (i = 0, index = 2; i < num; i++, index++)
      {
         d4 = d4 + inc4D;
         if ((P_setreal(CURRENT, "d4", d4, index)) == ERROR)
         {
            text_error("Could not set d4 values");
            return(ERROR);
         }
      }
   }

   if (ni2 > 1.5)
   {
      if (getparm("sw2", "real", CURRENT, &sw2, 1))
           return(ERROR);
      if (getparm("d3","real",CURRENT,&d3,1))
           return(ERROR);

      if (strcmp(parsestr, "") == 0)
      {
         strcpy(parsestr, "d3");
      }
      else
      {
         strcat(parsestr, ",d3");
      }

      inc3D = 1.0/sw2;
      num = (int) (ni2 - 1.0 + 0.5);
      for (i = 0, index = 2; i < num; i++, index++)
      {
         d3 = d3 + inc3D;
         if ((P_setreal(CURRENT, "d3", d3, index)) == ERROR)
         {
            text_error("Could not set d3 values");
            return(ERROR);
         }
      }
   }

   if (ni > 1.5)
   {
      int sparseDim;
      if (getparm("sw1", "real", CURRENT, &sw1, 1))
           return(ERROR);
      if (getparm("d2","real",CURRENT,&d2_init,1))
           return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d2")) != -1) );
      if ( ! findWord("d2",arraystr))
      {
         if ( ! sparseDim )
         {
         if (strcmp(parsestr, "") == 0)
         {
            strcpy(parsestr, "d2");
         }
         else
         {
            strcat(parsestr, ",d2");
         }
         }
      }
      else
      {
         elCorr++;
      }

      inc2D = 1.0/sw1;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d2 = d2_init + csIndex * inc2D;
            }
            else
            {
               d2 = d2_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d2", d2, i+1)) == ERROR)
            {
               text_error("Could not set d2 values");
               return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni - 1.0 + 0.5);
         d2 = d2_init;
         for (i = 0, index = 2; i < num; i++, index++)
         {
            d2 = d2 + inc2D;
            if ((P_setreal(CURRENT, "d2", d2, index)) == ERROR)
            {
               text_error("Could not set d2 values");
               return(ERROR);
            }
         }
      }
   }

   if (elCorr)
   {
      double arrayelemts;
      P_getreal(CURRENT, "arrayelemts", &arrayelemts, 1);
      arrayelemts -= (double) elCorr;
      P_setreal(CURRENT, "arrayelemts", arrayelemts, 1);
   }

   if (arraystr[0] != '\0')
   {
      if (strcmp(parsestr, "") != 0)
         strcat(parsestr, ",");
      strcat(parsestr, arraystr);
   }

   return(OK);
}

/*------------------------------------------------------------------------------
|
|	Write the acquisition message to psgQ
|
+----------------------------------------------------------------------------*/
static void queue_psg(auto_dir,fid_name,msg)
char auto_dir[];
char fid_name[];
char *msg;
{
  FILE *outfile;
  FILE *samplefile;
  SAMPLE_INFO sampleinfo;
  char tfilepath[MAXSTR*2];
  char val[MAX_TEXT_LEN];
  int entryindex;

  read_info_file(auto_dir);	/* map enterQ key words */

  strcpy(tfilepath,curexp);
  strcat(tfilepath,"/psgdone");
  outfile=fopen(tfilepath,"w");
  if (outfile)
  {
     fprintf(outfile,"%s\n",msg);
     strcpy(tfilepath,curexp);
     strcat(tfilepath,"/sampleinfo");
     samplefile=fopen(tfilepath,"r");
     if (samplefile)
     {
        read_sample_info(samplefile,&sampleinfo);
        /* get index to DATA: prompt,			*/
	/*  then update data text field with data file path */ 
        get_sample_info(&sampleinfo,"DATA:",val,128,&entryindex);
        strncpy(sampleinfo.prompt_entry[entryindex].etext,
						fid_name,MAX_TEXT_LEN);
        /* update STATUS field to Active */
        get_sample_info(&sampleinfo,"STATUS:",val,128,&entryindex);
        if (option_check("shimming"))
           strcpy(sampleinfo.prompt_entry[entryindex].etext,"Shimming");
        else
           strcpy(sampleinfo.prompt_entry[entryindex].etext,"Active");
        write_sample_info(outfile,&sampleinfo);
        fclose(samplefile);
        sprintf(tfilepath,"cat %s/psgdone >> %s/psgQ",curexp, auto_dir); 
      }
      else
      {
         text_error("Experiment unable to be queued\n");
         sprintf(tfilepath,"rm %s/psgdone",curexp); 
      }
      fclose(outfile);
      system(tfilepath);
  }
  else
      text_error("Experiment unable to be queued\n");
}

/*------------------------------------------------------------------------------
|
|	Queue the Experiment to the acq. process 
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.     1. Changed QueueExp() to pass the additional info:
|			     start_elem and complete_elem to Acqproc 
+----------------------------------------------------------------------------*/
#define QUEUE 1
int QueueExp(codefile,nextflag)
char *codefile;
int   nextflag;
{
    int priority;
    /*double expflags;*/
    char fidpath[MAXSTR];
    char addr[MAXSTR];
    char message[MAXSTR];
    char autopar[12];
    int autoflag;
    int expflags;

    if (!nextflag)
      priority = 5;
    else
      priority = 6;  /* increase priority to this Exp run next */

    if (getparm("auto","string",GLOBAL,autopar,12))
        autoflag = 0;
    else
        autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));
/*
 *    if (getparm("expflags","real",CURRENT,&expflags,1))
 *    {
 *        if (bgflag)
 *	  fprintf(stderr,"\nexpflags not found, set to zero\n");
 *	expflags = 0.0;
 *    }
*/

    expflags = 0;
    if (autoflag)
	expflags = ((long)expflags | AUTOMODE_BIT);  /* set automode bit */

    if (ra_flag)
        expflags |=  RESUME_ACQ_BIT;  /* set RA bit */

    if (getparm("file","string",CURRENT,fidpath,MAXSTR))
	return(ERROR);
    if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
        return(ERROR);
    sprintf(message,"%d,%s,%d,%lf,%s,%s,%s,%s,%s,%d,%d,%lu,%lu,",
		QUEUE,addr,
		(int)priority,exptime,fidpath,codefile,
		fileRFpattern, filegrad, filexpan,
		(int)setupflag,(int)expflags,
                start_elem, completed_elem);
    if (bgflag)
    {
      fprintf(stderr,"vnHost: '%s'\n",vnHost);
      fprintf(stderr,"fidpath: '%s'\n",fidpath);
      fprintf(stderr,"codefile: '%s'\n",codefile);
      fprintf(stderr,"priority: %d'\n",(int) priority);
      fprintf(stderr,"time: %lf\n",totaltime);
      fprintf(stderr,"suflag: %d\n",setupflag);
      fprintf(stderr,"expflags: %d\n",expflags);
      fprintf(stderr,"start_elem: %lu\n",start_elem);
      fprintf(stderr,"completed_elem: %lu\n",completed_elem);
      fprintf(stderr,"msge: '%s'\n",message);
      fprintf(stderr,"auto: %d\n",autoflag);
    }
    check_for_abort();
    if (autoflag)
    {   char autodir[MAXSTR];
        char commnd[MAXSTR];

        if (getparm("autodir","string",GLOBAL,autodir,MAXSTR))
	    return(ERROR);
        if (newacq)
        {
           char tmpstr[MAXSTR];

           P_getstring(CURRENT,"goid",tmpstr,3,255);
           sprintf(message,"%s %s auto",codefile,tmpstr);
        }
        if (nextflag)
        {
          sprintf(commnd,"mv %s/psgQ %s/psgQ.locked",autodir,autodir);
          system(commnd);
          queue_psg(autodir,fidpath,message);
          sprintf(commnd,"cat %s/psgQ.locked >> %s/psgQ; rm %s/psgQ.locked",
                           autodir,autodir,autodir);
          system(commnd);
        }
        else
        {
          queue_psg(autodir,fidpath,message);
        }
    }
    else if (!newacq)
    {  if (bgflag)
          fprintf(stderr,"QueueExp: before sendasync...\n");
       if (getparm("acqaddr","string",GLOBAL,addr,MAXSTR))
         return(ERROR);
       if (sendasync(addr,message))
       {
           text_error("Experiment unable to be sent\n");
           return(ERROR);
       }
    }
    else
    {
       char infostr[MAXSTR];
       char tmpstr[MAXSTR];
       char tmpstr2[MAXSTR];

       if (getparm("acqaddr","string",GLOBAL,addr,MAXSTR))
	 return(ERROR);

       P_getstring(CURRENT,"goid",tmpstr,3,255);
       P_getstring(CURRENT,"goid",tmpstr2,2,255);
       sprintf(infostr,"%s %s",tmpstr,tmpstr2);
       if (sendExpproc(addr,codefile,infostr,nextflag))
       {
        text_error("Experiment unable to be sent\n");
        return(ERROR);
       }
    }

    if (bgflag)
       fprintf(stderr,"returning from QueueExp\n");
    return(OK);
}

void check_for_abort()
{
   if (access( abortfile, W_OK ) == 0)
   {
      unlink(abortfile);
      psg_abort(1);
   }
}

/*
 * Check if the passed argument is gradtype
 * reset gradtype is needed
 */
int gradtype_check()
{
   vInfo  varinfo;
   int num;
   size_t oplen;
   char tmpstring[MAXSTR];
   char option[16];
   char gradtype[16];

   if ( P_getVarInfo(CURRENT,"go_Options",&varinfo) )
      return(0);
   if (P_getstring(GLOBAL, "gradtype", gradtype, 1, 15) < 0)
      return(0);
   num = 1;
   strcpy(option,"gradtype");
   oplen = strlen(option);
   while (num <= varinfo.size)
   {
      if ( P_getstring(CURRENT,"go_Options",tmpstring,num,MAXPATHL-1) >= 0 )
      {
         if ( strncmp(tmpstring, option, oplen) == 0 )
         {
             size_t gradlen = strlen(tmpstring);
             if (gradlen == oplen)
             {
                 gradtype[0]='a';
                 gradtype[1]='a';
                 break;
             }
             else if (gradlen <= oplen+4)
             {
                 int count = 0;
                 oplen++;
                 while ( (tmpstring[oplen] != '\0') && (count < 3) )
                 {
                    gradtype[count] = tmpstring[oplen];
                    count++;
                    oplen++;
                 }
             }
         }
      }
      else
      {
         return(0);
      }
      num++;
   }
   P_setstring(GLOBAL, "gradtype", gradtype, 1);
   return(0);
}

/*
 * Check if the passed argument is present in the list of
 * options passed to PSG from go.
 */
int option_check(option)
char *option;
{
   vInfo  varinfo;
   int num;
   char tmpstring[MAXSTR];

   if ( P_getVarInfo(CURRENT,"go_Options",&varinfo) )
      return(0);
   num = 1;
   while (num <= varinfo.size)
   {
      if ( P_getstring(CURRENT,"go_Options",tmpstring,num,MAXPATHL-1) >= 0 )
      {
         if ( strcmp(tmpstring,option) == 0 )
            return(1);
      }
      else
      {
         return(0);
      }
      num++;
   }
   return(0);
}

/*-----------------------------------------------------------------
|       getval()/1
|       returns value of variable
+------------------------------------------------------------------*/
double getval(const char *variable)
{
    int index;
 
    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {  
        fprintf(stdout,"'%s': not found, value assigned to zero.\n",variable);
        return(0.0);
    }  
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
            variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}

/*-----------------------------------------------------------------
|	getstr()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstr(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
        fprintf(stdout,"'%s': not found, value assigned to null.\n",variable);
	buf[0] = 0;
    }
}

/*-----------------------------------------------------------------
|	getvalnwarn()/1
|	returns value of variable 
+------------------------------------------------------------------*/
double getvalnwarn(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstrnwarn()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstrnwarn(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
	buf[0] = 0;
    }
}

/*-----------------------------------------------------------------
|       creatDPS()/0
|       creates the display pulse sequence data
+------------------------------------------------------------------*/
extern FILE  *dpsdata;
typedef	struct _aprecord {
        int		preg;	/* this is not saved */
        short		*apcarray /*[MAXARRAYSIZE+1] */;
} aprecord;

extern aprecord apc;
creatDPS()
{
    apc.apcarray = Codes;
    x_pulsesequence();  /* generate  Pulse Sequence */
}        

int setup_parfile(suflag)
int suflag;
{
    double tmp;
    char tmpstr[256];
    int t;
    char *ptr;

    if ((P_getreal(CURRENT,"priority",&tmp,1)) >= 0)
       ExpInfo.Priority = (int) (tmp + 0.0005);
    else
       ExpInfo.Priority = 0;

    if ((P_getreal(CURRENT,"nt",&tmp,1)) >= 0)
       ExpInfo.NumTrans = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set nt.");
    }

    if ((P_getreal(CURRENT,"bs",&tmp,1)) >= 0)
       ExpInfo.NumInBS = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set bs.");
    }
    if (!(var_active("bs",CURRENT)))
       ExpInfo.NumInBS = 0;
     
    if ((P_getreal(CURRENT,"np",&tmp,1)) >= 0)
       ExpInfo.NumDataPts = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set np.");
    }
     
    /* --- Number of FIDs per CT --- */
     
    if ((P_getreal(CURRENT,"nf",&tmp,1)) >= 0)
    {
        DPRINT2(1,"initacqqueue(): nf = %5.0lf, active = %d \n",
                                tmp,var_active("nf",CURRENT));
        if ( (tmp < 2.0 ) || (!(var_active("nf",CURRENT))) )
        {
            tmp = 1.0;
        }
    }
    else  /* no nf, so set it to one.  */
    {
        tmp = 1.0;
    }
    ExpInfo.NumFids = (int) (tmp + 0.0005);
 
    if (P_getstring(GLOBAL,"Console",tmpstr,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get Console");
    }
    if (!strcmp(tmpstr,"mercury") )
       ExpInfo.DataPtSize = 4;	/* only double precision for Mercury */
    else
    {  if (P_getstring(CURRENT,"dp",tmpstr,1,4) < 0)
       {   abort_message("initacqqueue(): cannot get dp");
       }
       ExpInfo.DataPtSize = (tmpstr[0] == 'y') ? 4 : 2;
    }
     
    /* --- receiver gain --- */
 
    if ((P_getreal(CURRENT,"gain",&tmp,1)) >= 0)
       ExpInfo.Gain = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set gain.");
    }      
 
    /* --- sample spin rate --- */
     
    if ((P_getreal(CURRENT,"spin",&tmp,1)) >= 0)
       ExpInfo.Spin = (int) (tmp + 0.0005);
    else    
    {   abort_message("initacqqueue(): cannot set spin.");
    }
 
    /* --- completed transients (ct) --- */
     
    if ((P_getreal(CURRENT,"ct",&tmp,1)) >= 0)
       ExpInfo.CurrentTran = (int) (tmp + 0.0005);
    else    
    {   abort_message("initacqqueue(): cannot set ct.");
    }
 
    /* --- number of fids --- */
     
    if ((P_getreal(CURRENT,"arraydim",&tmp,1)) >= 0)
       ExpInfo.ArrayDim = (int) (tmp + 0.0005);
    else    
    {   abort_message("initacqqueue(): cannot set arraydim.");
    }      
     
    ExpInfo.NumAcodes = ExpInfo.ArrayDim;
  
    ExpInfo.FidSize = ExpInfo.DataPtSize * ExpInfo.NumDataPts;
    ExpInfo.DataSize = sizeof(struct datafilehead);
    ExpInfo.DataSize +=  (unsigned long long) (sizeof(struct datablockhead) + ExpInfo.FidSize) *
                         (unsigned long long) ExpInfo.ArrayDim;
 
    /* --- path to the user's experiment work directory  --- */
 
    if (P_getstring(GLOBAL,"userdir",tmpstr,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get userdir");
    }
    strcpy(ExpInfo.UsrDirFile,tmpstr);
     
    if (P_getstring(GLOBAL,"systemdir",tmpstr,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get systemdir");
    }
    strcpy(ExpInfo.SysDirFile,tmpstr);
 
    if (P_getstring(GLOBAL,"curexp",tmpstr,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get curexp");
    }
    strcpy(ExpInfo.CurExpFile,tmpstr);
     
    /* --- suflag                                       */
    ExpInfo.GoFlag = suflag;
 
    /* --------------------------------------------------------------
    |      Unique name to this GO,
    |      vnmrsystem/acqqueue/id is path to acq proccess files
    +-----------------------------------------------------------------*/
 
#ifndef TESTING
    if (P_getstring(CURRENT,"goid",infopath,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get goid");
    }
#else
    strcpy(infopath,"./stdalone");
#endif
    strcpy(ExpInfo.Codefile,infopath);
    strcat(ExpInfo.Codefile,".Code");
    strcpy(ExpInfo.RTParmFile,infopath);
    strcat(ExpInfo.RTParmFile,".RTpars");
    strcpy(ExpInfo.TableFile,infopath);
    strcat(ExpInfo.TableFile,".Tables");
    ExpInfo.WaveFormFile[0] = '\0';
    ExpInfo.GradFile[0] = '\0';
             
    /* Beware that infopath gets accessed again
       if acqiflag is set, for the data file path */

    /* --- file path to named acqfile or exp# acqfile  'file' --- */

    if (!acqiflag)
    {
      int autoflag;
      char autopar[12];

      if (P_getstring(CURRENT,"exppath",tmpstr,1,255) < 0)
      {   abort_message("initacqqueue(): cannot get exppath");
      }
#ifndef TESTING
      strcpy(ExpInfo.DataFile,tmpstr);
#else
     strcpy(ExpInfo.DataFile,"./acqfil");
#endif
      ExpInfo.InteractiveFlag = 0;
      if (getparm("auto","string",GLOBAL,autopar,12))
          autoflag = 0;
      else
          autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));
      ExpInfo.ExpFlags = 0;
      if (autoflag)
      {
         strcat(ExpInfo.DataFile,".fid");
         ExpInfo.ExpFlags |= AUTOMODE_BIT;  /* set automode bit */
      }
      if (ra_flag)
         ExpInfo.ExpFlags |=  RESUME_ACQ_BIT;  /* set RA bit */
      if (clr_at_blksize_mode)
          ExpInfo.ExpFlags |=  CLR_AT_BS_BIT; /* For "Repeat Scan" */
    }
    else
    {
      strcpy(ExpInfo.DataFile,infopath);
      strcat(ExpInfo.DataFile,".Data");
      ExpInfo.InteractiveFlag = 1;
      ExpInfo.ExpFlags = 0;
    }

#ifndef TESTING
    if (P_getstring(CURRENT,"goid",tmpstr,2,255) < 0)
    {   abort_message("initacqqueue(): cannot get goid: user");
    }        
#else
    strcpy(tmpstr,"frits");
#endif
    strcpy(ExpInfo.UserName,tmpstr);
                 
#ifndef TESTING
    if (P_getstring(CURRENT,"goid",tmpstr,3,255) < 0)
    {   abort_message("initacqqueue(): cannot get goid: exp number");
    }        
#else
   strcpy(tmpstr,"1");
#endif
    ExpInfo.ExpNum = atoi(tmpstr);
             
#ifndef TESTING
    if (P_getstring(CURRENT,"goid",tmpstr,4,255) < 0)
    {   abort_message("initacqqueue(): cannot get goid: exp");
    }        
#else
    strcpy(tmpstr,"sdtalone3");
#endif
    strcpy(ExpInfo.AcqBaseBufName,tmpstr);
             
    if (P_getstring(GLOBAL,"vnmraddr",tmpstr,1,255) < 0)
    {   abort_message("initacqqueue(): cannot get vnmraddr");
    }        
    strcpy(ExpInfo.MachineID,tmpstr);
             
    /* --- interleave parameter 'il' --- */
                 
    if (P_getstring(CURRENT,"il",tmpstr,1,4) < 0)
    {   abort_message("initacqqueue(): cannot get il");
    }        
    ExpInfo.IlFlag = (tmpstr[0] == 'y') ? 1 : 0;
    if (ExpInfo.IlFlag)
    {
        if (ExpInfo.ArrayDim <= 1) ExpInfo.IlFlag = 0;
        if (ExpInfo.NumInBS == 0) ExpInfo.IlFlag = 0;
        if (ExpInfo.NumTrans <= ExpInfo.NumInBS) ExpInfo.IlFlag = 0;
    }
 
    /* --- current element 'celem' --- */
             
    if ((P_getreal(CURRENT,"celem",&tmp,1)) >= 0)
       ExpInfo.Celem = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set celem.");
    }
 
    /* --- Check for valid RA --- */
    ExpInfo.RAFlag = 0;		/* RaFlag */
    if (ra_flag)
    {
    	/* --- Do RA stuff --- */
	ExpInfo.RAFlag = 1;		/* RaFlag */
	if (ExpInfo.IlFlag)
	{
	   if ((ExpInfo.CurrentTran % ExpInfo.NumInBS) != 0)
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
	   else
	   {
		if ((ExpInfo.Celem < ExpInfo.ArrayDim) && 
			(ExpInfo.CurrentTran >= ExpInfo.NumInBS))
		   ExpInfo.CurrentTran = ExpInfo.CurrentTran - ExpInfo.NumInBS;
	   }
	}
	else
	{
    	   if ((ExpInfo.CurrentTran > 0) && (ExpInfo.CurrentTran < 
						ExpInfo.NumTrans))
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
    	   if ((ExpInfo.CurrentTran == ExpInfo.NumTrans) &&
				(ExpInfo.Celem < ExpInfo.ArrayDim))
		   ExpInfo.CurrentTran = 0;
	}
    	if ((ExpInfo.Celem < 0) || (ExpInfo.Celem >= ExpInfo.ArrayDim))
	   ExpInfo.Celem = 0;
    	ExpInfo.CurrentElem = ExpInfo.Celem;
    	/* fprintf(stdout,"initacqparms: Celem = %d\n",ExpInfo.Celem); */
    }

    /* --- when_mask parameter  --- */
 
#ifndef TESTING
    if ((P_getreal(CURRENT,"when_mask",&tmp,1)) >= 0)
       ExpInfo.ProcMask = (int) (tmp + 0.0005);
    else
    {   abort_message("initacqqueue(): cannot set when_mask.");
    }
#else
    ExpInfo.ProcMask = 0;
#endif
 
    ok2bumpflag = 0;
    if (P_getstring(CURRENT, "ok2bump", tmpstr, 1, sizeof( tmpstr - 1 )) >= 0 )
    {
	if (tmpstr[ 0 ] == 'Y' || tmpstr[ 0 ] == 'y')
	  ok2bumpflag = 1;
    }

    ExpInfo.ProcWait = (option_check("wait")) ? 1 : 0;
    ExpInfo.DspGainBits = 0;
    ExpInfo.DspOversamp = 0;
    ExpInfo.DspOsCoef = 0;
    ExpInfo.DspSw = 0.0;
    ExpInfo.DspFb = 0.0;
    ExpInfo.DspOslsfrq = 0.0;
    ExpInfo.DspFiltFile[0] = '\0';

    /* fill in the account info */
    strcpy(tmpstr,ExpInfo.SysDirFile);
    strcat(tmpstr,"/adm/accounting/acctLog.xml");
    if ( access(tmpstr,F_OK) != 0)
    {
       ExpInfo.Billing.enabled = 0;
    }
    else
    {
        ExpInfo.Billing.enabled = 1;
    }
        t = time(0);
        ExpInfo.Billing.submitTime = t;
        ExpInfo.Billing.startTime  = t;
        ExpInfo.Billing.doneTime   = t;
        if (P_getstring(GLOBAL, "operator", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.Operator[0]='\000';
        else
           strncpy(ExpInfo.Billing.Operator,tmpstr,200);
        if (P_getstring(CURRENT, "account", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.account[0]='\000';
        else
           strncpy(ExpInfo.Billing.account,tmpstr,200);
        if (P_getstring(CURRENT, "pslabel", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.seqfil[0]='\000';
        else
           strncpy(ExpInfo.Billing.seqfil,tmpstr,200);
        ptr = strrchr(infopath,'/');
        if ( ptr )
        {
           ptr++;
           strncpy(ExpInfo.Billing.goID, ptr ,200);
        }
        else
        {
           ExpInfo.Billing.goID[0]='\000';
        }
    return(0);
}

void set_dsp_pars(dspsw, dspfb, dsplsfreq, oversamp, oscoef,dspfilt)
double dspsw, dspfb, dsplsfreq;
int oversamp, oscoef;
char *dspfilt;
{
    ExpInfo.DspOversamp = oversamp;
    ExpInfo.DspOsCoef = oscoef;
    ExpInfo.DspSw = dspsw;
    ExpInfo.DspFb = dspfb;
    ExpInfo.DspOslsfrq = dsplsfreq;
    strcpy(ExpInfo.DspFiltFile, dspfilt);
}


/*---------------------------------------------------------------------
|       set_counter()/
|
|       Sets ss counters for each element
+-------------------------------------------------------------------*/
set_counters()
{
    long ilsstmp, ilctsstmp;
    if (Alc->ss < 0)
    {
       Alc->ssct = -1 * Alc->ss;
       ilsstmp = Alc->ssct;
    }
    else if (ix == getStartFidNum())
    {
       Alc->ssct = Alc->ss;
       ilsstmp = 0;
    }
    else
    {
       Alc->ssct = 0;
       ilsstmp = 0;
    }
    Alc->rtvptr = ( Alc->nt - (Alc->ssct % Alc->nt) + Alc->ct )
                                                         % Alc->nt;
    ilctsstmp = ( Alc->nt - (ilsstmp % Alc->nt) + Alc->ct )
                                                         % Alc->nt;
    if (newacq)
    {
       init_acqvar(ilssval,ilsstmp);
       init_acqvar(ilctss,ilctsstmp);
    }
}


set_acode_size(sz)
int sz;
{
    ExpInfo.acode_1_size = sz;
}

set_max_acode_size(sz)
int sz;
{
    ExpInfo.acode_max_size = sz;
}

/* Allow user over ride of exptime calculation */
void g_setExpTime(double val)
{
  usertime = val;
}

double getExpTime()
{
  return(usertime);
}

write_shr_info(exp_time)
double	exp_time;
{
    int Infofd;	/* file discriptor Code disk file */
    int bytes;

    /* --- write parameter out to acqqueue file --- */

    if (usertime < 0.0)
       ExpInfo.ExpDur = exp_time;
    else
       ExpInfo.ExpDur = usertime;

    if (newacq)
    {
       ExpInfo.NumTables = num_tables; /* set number of tables */
       strcpy(ExpInfo.WaveFormFile,fileRFpattern);
    }

/* Mercury/VX follows INOVA conventions.  See initacqparms.c in
   SCCS category psg.  Remember though this kpsg is also expected
   to work with the 1st generation console.				*/

    if (ExpInfo.InteractiveFlag && newacq)
    {
       char tmppath[256];
       sprintf(tmppath,"%s.new",infopath);
       unlink(tmppath);
       Infofd = open(tmppath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    else
    {
       Infofd = open(infopath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    if (Infofd < 0)
    {	abort_message("Exp info file already exists. PSG Aborted..\n");
    }
    bytes = write(Infofd, (const void *)&ExpInfo, sizeof( SHR_EXP_STRUCT ) );
    if (bgflag)
      fprintf(stderr,"Bytes written to info file: %d (bytes).\n",bytes);
    close(Infofd);

      bgflag=0;
}
write_exp_info()
{
    int Infofd; /* file discriptor Code disk file */
    int bytes;
#ifdef LINUX
    int cnt;
    long rt_tab_tmp[RT_TAB_SIZE];
#endif

    /* --- write parameter out real-time variable file --- */
    if (newacq)
    {
       Infofd = open(ExpInfo.RTParmFile,O_EXCL | O_WRONLY | O_CREAT,0666);
       if (Infofd < 0)
       {        abort_message("Exp rt file already exists. PSG Aborted..\n");
       }
#ifdef LINUX
       for (cnt=0; cnt < RT_TAB_SIZE; cnt++)
       {
          rt_tab_tmp[cnt] = htonl(rt_tab[cnt]);
       }
       bytes = write(Infofd, rt_tab_tmp, sizeof(long) * get_rt_tab_elems() );
#else
       bytes = write(Infofd, rt_tab, sizeof(long) * get_rt_tab_elems() );
#endif
       if (bgflag)
         fprintf(stderr,"Bytes written to info file: %d (bytes).\n",bytes);
       close(Infofd);

       if (num_tables > 0)
       { 
           writetablefile(ExpInfo.TableFile);
       } 
       else
          ExpInfo.TableFile[0] = '\0';
    }
}

int getIlFlag()
{
        return(ExpInfo.IlFlag);
}

/*--------------------------------------------------------------*/
/* getStartFidNum                                               */
/*      Return starting fid number for ra.  ExpInfo.Celem goes  */
/*      from 0 to n-1. getStartFidNum goes from 1 to n          */
/*--------------------------------------------------------------*/
int getStartFidNum()
{
   if (newacq)
        return(ExpInfo.Celem + 1);
   else
        return(1);
}

static void checkGradAlt()
{
   if ( P_getreal(CURRENT,"gradalt",&gradalt,1) == 0)
   {
      if ( !  P_getactive(CURRENT,"gradalt") )
      {
         P_setreal(CURRENT,"gradalt",1.0,0);
         gradalt = 1.0;
      }
   }
   else
   {
      P_creatvar(CURRENT, "gradalt", T_REAL);
      P_setreal(CURRENT,"gradalt",1.0,1);
      gradalt = 1.0;
   }
}

void checkGradtype()
{
    char tmpGradtype[MAXSTR];

    specialGradtype = 0;
    strcpy(tmpGradtype,"   ");
    P_getstring(GLOBAL,"gradtype",tmpGradtype,1,MAXSTR-1);
    if (tmpGradtype[2] == 'a')
    {
        specialGradtype = 'a';
    }
    else if (tmpGradtype[2] == 'h')
    {
        specialGradtype = 'h';
    }
}

// Useful methods that can be adopted in many places to cut down code duplication

int rfChanNum(const char* rf_ch_name, const char* comment) {
    if      (strcmp( rf_ch_name, "obs"  ) == 0) return OBSch;
    else if (strcmp( rf_ch_name, "dec"  ) == 0) return DECch;
    else if (strcmp( rf_ch_name, "dec2" ) == 0) return DEC2ch;
    else if (strcmp( rf_ch_name, "dec3" ) == 0) return DEC3ch;
    else if (strcmp( rf_ch_name, "dec4" ) == 0) return DEC4ch;
    else abort_message("unknown rf channel (%s): %s", rf_ch_name, comment);
}

int isObsChannel(int rf_ch_num) {
 
    return rf_ch_num == OBSch;
}

int isDecChannel(int rf_ch_num) {
 
    return rf_ch_num == DECch;
}

int isDec2Channel(int rf_ch_num) {
 
    return rf_ch_num == DEC2ch;
}

int isDec3Channel(int rf_ch_num) {
 
    return rf_ch_num == DEC3ch;
}

int isDec4Channel(int rf_ch_num) {
 
    return rf_ch_num == DEC4ch;
}
