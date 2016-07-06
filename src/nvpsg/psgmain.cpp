/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
      psgmain.cpp
*/

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>

#include "cpsg.h"

#include <iostream>
#include <fstream>
#include <math.h>

#include "ACode32.h"
#include "Controller.h"
#include "MasterController.h"
#include "RFController.h"
#include "GradientController.h"
#include "PFGController.h"
#include "DDRController.h"
#include "InitAcqObject.h"
#include "FFKEYS.h"
#include "Console.h"
#include "Bridge.h"
#include "GradientBridge.h"
#include "PSGFileHeader.h"
#include "ddr_symbols.h"

extern "C" {
#include "symtab.h"
#include "variables.h"
#include "params.h"
#include "pvars.h"
#include "REV_NUMS.h"
#include "asm.h"
#include "shims.h"
#include "vfilesys.h"
#include "tools.h"
#include "arrayfuncs.h"
#include "CSfuncs.h"

#ifdef __INTERIX    /* for winpath2unix() */
#include <interix/interix.h>
#endif

}


#ifndef MAX_RCVR_CHNLS
#define MAX_RCVR_CHNLS 64
#endif

#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR 1
#define MAXSTR 256
#define MAXPATHL 128

#define MAXTABLE	60	/* for table implementation (aptable.h) */
#define MAXSLICE      1024      /* maximum number of slices - SISCO */

#define MAX_RFCHAN_NUM 20

#define MAX_FIDSHIMNT 256       /* max value of scans to be averaged for fid shimming
                                   if this value is changed, then it should also be 
                                   changed in nvacq/A32Interp */

/* house keeping delay between CTs */



/* PSG uses int (short int) to keep track of locations within
 * an acode.  For example, nsc_ptr and multhwlp_ptr. Therefore, if
 * acode length exceeds 16 bit limit, trouble.
 */

void PSGGo(double);
void PSGSetup(double);
void PSGDps();

extern "C" {
 void initscan();
 void nextscan();
 void endofscan();
 void endofExperiment(int setuptype);
 void nextcodeset();
 void MainSystemSync();
 void checkGradtype();
 void checkImplementation();
 void initparms();
 void close_error(int success);
 double psDuration();
 void   preNextScanDuration();
 void lockPsgQ(const char *autodir);
 void unlockPsgQ(const char *autodir);

 }

extern "C" int createPS(int arrayDim);		/* create Pulse Sequence routine */
extern "C" void createDPS(char *cmd, char *expdir, double arraydim,
                          int array_num, char *array_str, int pipe2nmr);
extern "C" int A_getstring(int tree, const char *name, char **varaddr, int index);
extern "C" int getIlFlag();

static void checkGradAlt();
static int setGflags();
static void PSGDps(char *cmd);
static double preNextScanTime = 0.0;

int bgflag = 0;
int debug  = 0;
int debug2 = 0;
int bugmask = 0;
int lockfid_mode = 0;
int clr_at_blksize_mode = 0;
int initializeSeq = 1;

int debugFlag = 0;	/* do NOT set this flag based on a command line argument !! */

static int pipe1[2];
// Note: unistd.h:389: error: previous declaration of 'int pipe2(int*, int)
// static int pipe2[2];
// So change the name to pipeB
static int pipeB[2];
static int ready = 0;

/**************************************
*  Structure for real-time AP tables  *
*  and global variables declarations  *
**************************************/

int  	t1, t2, t3, t4, t5, t6,
                t7, t8, t9, t10, t11, t12,
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
   int	indexptr;
   int	destptr;
};

typedef	struct _Tableinfo	Tableinfo;

Tableinfo	*Table[MAXTABLE];


/**********************************************
*  End table structures and global variables  *
**********************************************/


char    **cnames;	/* pointer array to variable names */
int     *preCodes;	/* pointer to the start of the allocated space */
int     *Codes;	/* pointer to the start of the Acodes array */
int     Codesize;	/* size of the allocated space for codes */
int     CodeEnd;	/* End Address  of the alocated space for codes */
int     *Codeptr; 	/* pointer into the Acode array */
int     nnames;	/* number of variable names */
int     ntotal;	/* total number of variable names */
int     ncvals;	/* NUMBER OF VARIABLE  values */
int     codestart;	/* Beginning offset of a PS lc,auto struct & code */
int     startofAcode;	/* Beginning offset of actual Acodes */
double  **cvals;		/* pointer array to variable values */

/* char rftype[MAXSTR]; */	/* type of rf system used for trans & decoupler */
char amptype[MAXSTR];	/* type of amplifiers used for trans & decoupler */
char rfwg[MAXSTR];	/* y/n for rf waveform generators */
char gradtype[MAXSTR];	/* char keys w - waveform generation s-sisco n-none */
char rfband[MAXSTR];	/* RF band of trans & dec  (high or low) */

FILE *shapeListFile;    /* shape names list file                */
int   shapesWritten = 0;

/* --- global flags --- */
int  newacq = 0;	/* temporary nessie flag */
int  acqiflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  tuneflag = 0;	/* if 'tune' was an argument, then tuning */
int  checkflag = 0;	/* if 'check' was an argument, then check sequnece but no acquisition */
int  nomessageflag = 0;   /* if 'nomessage' was an argument, then suppress text_message, etc */
int  safetyoff = 0;	/* if 'nosafe' is an argument, disable RF safety check */
int  waitflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  prepScan = 0;	/* if 'prep' was an argument, then wait for sethw to start */
int  fidscanflag = 0;	/* if 'fidscan' was an argument, then use fidscan mode for vnmrj */
int  ok;		/* global error flag */
int  automated;		/* True if system is an automated one */
int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
int  ptsval[MAX_RFCHAN_NUM+1];	/* PTS type for trans & decoupler */
int  rcvroff_flag = 0;	/* receiver flag; initialized to ON */
int  rcvr_is_on_now = 1;/* current receiver status 1=on  o=off */
int  ap_ovrride = 0;	/* UNUSED - sequence compatibility? */
int  vttype;		    /* VT type 0=none,1=varian,2=oxford */
int  newtrans = 1;      // compatibility with test sequences only use
int  newtransamp = 1;   // etc.
int  newdec = 1;        // etc.
int  newdecamp = 1;     // etc.
double beta = 1.0;      // etc.

/* acquisition */
double fb;
char  il[MAXSTR];	/* interleaved acquisition parameter, 'y','n', or 'f#' */
double  sw; 		/* Sweep width */
double  nf;		/* For E.A.T., number of fids in Pulse sequence */
double  np; 		/* Number of data points to acquire */
double  nt; 		/* number of transients */
double  sfrq;   	/* Transmitter Frequency MHz */
double  dfrq; 		/* Decoupler Frequency MHz */
double  dfrq2; 		/* 2nd Decoupler Frequency MHz */
double  dfrq3; 		/* 3rd Decoupler Frequency MHz */
double  dfrq4; 		/* 4th Decoupler Frequency MHz */
/* double  fb = 50.0e9; */    /* no equivalent in new console, 1/(beta*fb) = 50.0e-9 s */
double  bs;		/* Block Size */
double  tof;		/* Transmitter Offset */
double  dof;		/* Decoupler Offset */
double  dof2;		/* 2nd Decoupler Offset */
double  dof3;		/* 3rd Decoupler Offset */
double  dof4;		/* 4th Decoupler Offset */
double  gain; 		/* receiver gain value, or 'n' for autogain */
int     gainactive;	/* gain active parameter flag */
double  dlp; 		/* decoupler Low Power value */
double  dhp; 		/* decoupler High Power value */
double  tpwr; 		/* Transmitter pulse power */
double  tpwrf;		/* Transmitter fine linear attenuator for pulse power */
double  dpwr;		/* Decoupler pulse power */
double  dpwrf;		/* Decoupler fine linear atten for pulse power */
double  dpwrf2;		/* 2nd Decoupler fine linear atten for pulse power */
double  dpwrf3;		/* 3rd Decoupler fine linear atten for pulse power */
double  dpwrf4;		/* 4th Decoupler fine linear atten for pulse power */
double  dpwr2;		/* 2nd Decoupler pulse power */
double  dpwr3;		/* 3rd Decoupler pulse power */
double  dpwr4;		/* 4th Decoupler pulse power */
double  filter;		/* pulse Amp filter setting */
double	xmf;		/* transmitter decoupler modulation frequency */
double  dmf;		/* 1st decoupler modulation frequency */
double  dmf2;		/* 2nd decoupler modulation frequency */
double  dmf3;		/* 3rd decoupler modulation frequency */
double  dmf4;		/* 4th decoupler modulation frequency */
int     cpflag;  	/* phase cycling flag  1=none,  0=quad detection */
    /* --- pulse widths --- */
double  pw; 		/* pulse width */
double  p1; 		/* A pulse width */
double  pw90;		/* 90 degree pulse width */
double  hst;    	/* time homospoil is active */

/* --- delays --- */
double  alfa; 		/* Time after rec is turned on that acqbegins */
double  d1; 		/* delay */
double  d2; 		/* A delay, used in 2D/3D/4D experiments */
double  d2_init = 0.0; 	/* Initial value of d2 delay, used in 2D/3D/4D experiments */
double  inc2D;		/* t1 dwell time in a 2D/3D/4D experiment */
double  d3;		/* t2 delay in a 3D/4D experiment */
double  d3_init = 0.0; 	/* Initial value of d3 delay, used in 2D/3D/4D experiments */
double  inc3D;		/* t2 dwell time in a 3D/4D experiment */
double  d4;		/* t3 delay in a 4D experiment */
double  d4_init = 0.0; 	/* Initial value of d4 delay, used in 2D/3D/4D experiments */
double  inc4D;		/* t3 dwell time in a 4D experiment */
double  pad; 		/* Pre-acquisition delay */
double  preacqtime;     /* value of pad used for time calculations */
int     padactive; 	/* Pre-acquisition delay active parameter flag */
double  rof1; 		/* Time receiver is turned off before pulse */
double  rof2;		/* Time after pulse before receiver turned on */
/* ddr */
double  roff;       /* receiver frequency offset */

double  gradalt;       /* multiplier for alternating zgradpulse and rgradient */

double rcvrf[MAX_RCVR_CHNLS];  /* channel specific non-arrayable frequency offset */
double rcvrp1[MAX_RCVR_CHNLS]; /* channel specific non-arrayable phase step */
double rcvrp[MAX_RCVR_CHNLS];  /* channel specific non-arrayable phase offset */
double rcvra[MAX_RCVR_CHNLS];  /* channel specific non-arrayable amplitude scale */

/* --- total time of experiment --- */
double  totaltime; 	/* total timer events for a fid */
double  exptime; 	/* total time for an exp duration estimate */

int   phase1;            /* 2D acquisition mode */
int   phase2;            /* 3D acquisition mode */
int   phase3;            /* 4D acquisition mode */

int   d2_index = 0;      /* d2 increment (from 0 to ni-1) */
int   d3_index = 0;      /* d3 increment (from 0 to ni2-1) */
int   d4_index = 0;      /* d4 increment (from 0 to ni3-1) */

/* --- programmable decoupling sequences -- */
char  xseq[MAXSTR];
char  dseq[MAXSTR];
char  dseq2[MAXSTR];
char  dseq3[MAXSTR];
char  dseq4[MAXSTR];
double xres;		/* digit resolutio prg dec */
double dres;		/* digit resolutio prg dec */
double dres2;		/* digit resolutio prg dec */
double dres3;		/* digit resolutio prg dec */
double dres4;		/* digit resolutio prg dec */


/* --- status control --- */
char  xm[MAXSTR];		/* transmitter status control */
char  xmm[MAXSTR]; 		/* transmitter modulation type control */
char  dm[MAXSTR];		/* decoupler status control */
char  dmm[MAXSTR]; 		/* decoupler modulation type control */
char  dm2[MAXSTR]; 		/* 2nd decoupler status control */
char  dm3[MAXSTR]; 		/* 3rd decoupler status control */
char  dm4[MAXSTR]; 		/* 4th decoupler status control */
char  dmm2[MAXSTR]; 		/* 2nd decoupler modulation type control */
char  dmm3[MAXSTR]; 		/* 3rd decoupler modulation type control */
char  dmm4[MAXSTR]; 		/* 4th decoupler modulation type control */
char  homo[MAXSTR]; 		/* first  decoupler homo mode control */
char  homo2[MAXSTR]; 		/* second decoupler homo mode control */
char  homo3[MAXSTR]; 		/* third  decoupler homo mode control */
char  homo4[MAXSTR]; 		/* fourth  decoupler homo mode control */
char  hs[MAXSTR]; 		/* homospoil status control */
int   xmsize;			/* number of characters in xm */
int   xmmsize;			/* number of characters in xmm */
int   dmsize;			/* number of characters in dm */
int   dmmsize;			/* number of characters in dmm */
int   dm2size;			/* number of characters in dm2 */
int   dm3size;			/* number of characters in dm3 */
int   dm4size;			/* number of characters in dm4 */
int   dmm2size;			/* number of characters in dmm2 */
int   dmm3size;			/* number of characters in dmm3 */
int   dmm4size;			/* number of characters in dmm4 */
int   homosize;			/* number of characters in homo */
int   homo2size;		/* number of characters in homo2 */
int   homo3size;		/* number of characters in homo3 */
int   homo4size;		/* number of characters in homo4 */
int   hssize; 			/* number of characters in hs */

int setupflag;			/* alias used to invoke PSG ,go=0,su=1,etc*/
int dps_flag;			/* dps flag */
int ra_flag;			/* ra flag */
unsigned int start_elem;       /* elem (FID) for acquisition to start on (RA)*/
unsigned int completed_elem;   /* total number of completed elements (FIDs)  (RA) */
int statusindx;			/* current status index */
int HSlines;			/* High Speed output board lines */

/*- These values are used so that they are changed only when their values do */
int oldlkmode;			/* previous value of lockmode */
int oldspin = -1;		/* previous value of spin */
int oldwhenshim = 0;		/* previous value of shimming mask */
double oldvttemp = 29999.0;	/* previous value of vt tempature */
double oldpad;			/* previous value of pad */

/*- These values are used so that they are included in the Acode only when */
/*  the next value out of the arrayed values has been obtained */
int oldpadindex;		/* previous value of pad value index */
int newpadindex;		/* new value of pad value index */

/* --- Pulse Seq. globals --- */
double xmtrstep;		/* phase step size trans */
double decstep;			/* phase step size dec */
int idc = PSG_ACQ_REV;	/* PSG software rev number,(initacqparms)*/

unsigned int 	ix;			/* FID currently in Acode generation */
int 	nth2D;			/* 2D Element currently in Acode generation (VIS usage)*/
int     arrayelements;		/* number of array elements */
/* int     ap_interface = 4; */	/* ap bus interface type 1=500style, 2=amt style */
int   rotorSync;		/* rotor sync interface 1=present or 0=not present */

/* ---  interlock parameters, etc. --- */
char 	interLock[MAXSTR];
char 	alock[MAXSTR];
char 	wshim[MAXSTR];
int  	spin;
int  	traymax;
int  	loc;
int  	spinactive;
double  vttemp; 	/* VT temperature setting */
double  vtwait; 	/* VT temperature timeout setting */
double  vtc; 		/* VT temperature cooling gas setting */
int  	tempactive;
int  	lockmode;
int  	whenshim;
int  	shimatanyfid;

/*------------------------------------------------------------------
    RF PULSES
------------------------------------------------------------------*/
double  p2;                  /* pulse length */
double  p3;                  /* pulse length */
double  p4;                  /* pulse length */
double  p5;                  /* pulse length */
double  p6;                  /* pulse length */
double  pi;                  /* inversion pulse length */
double  psat;                /* saturation pulse length */
double  pmt;                 /* magnetization transfer pulse length */
double  pwx;                 /* X-nucleus pulse length */
double  pwx2;                /* X-nucleus pulse length */
double  psl;                 /* spin-lock pulse length */

char  pwpat[MAXSTR];         /* pattern for pw,tpwr */
char  p1pat[MAXSTR];         /* pattern for p1,tpwr1 */
char  p2pat[MAXSTR];         /* pattern for p2,tpwr2 */
char  p3pat[MAXSTR];         /* pattern for p3,tpwr3 */
char  p4pat[MAXSTR];         /* pattern for p4,tpwr4 */
char  p5pat[MAXSTR];         /* pattern for p5,tpwr5 */
char  p6pat[MAXSTR];         /* pattern for p5,tpwr5 */
char  pipat[MAXSTR];         /* pattern for pi,tpwri */
char  satpat[MAXSTR];        /* pattern for psat,satpat */
char  mtpat[MAXSTR];         /* magnetization transfer RF pattern */
char  pslpat[MAXSTR];        /* pattern for spin-lock */

double  tpwr1;               /* Transmitter pulse power */
double  tpwr2;               /* Transmitter pulse power */
double  tpwr3;               /* Transmitter pulse power */
double  tpwr4;               /* Transmitter pulse power */
double  tpwr5;               /* Transmitter pulse power */
double  tpwr6;               /* Transmitter pulse power */
double  tpwri;               /* inversion pulse power */
double  satpwr;              /* saturation pulse power */
double  mtpwr;               /* magnetization transfer pulse power */
double  pwxlvl;              /* pwx power level */
double  pwxlvl2;             /* pwx2 power level */
double  tpwrsl;              /* spin-lock power level */


/*------------------------------------------------------------------
    RF DECOUPLER PULSES
------------------------------------------------------------------*/
char  decpat[MAXSTR];        /* pattern for decoupler pulse */
char  decpat1[MAXSTR];       /* pattern for decoupler pulse */
char  decpat2[MAXSTR];       /* pattern for decoupler pulse */
char  decpat3[MAXSTR];       /* pattern for decoupler pulse */
char  decpat4[MAXSTR];       /* pattern for decoupler pulse */
char  decpat5[MAXSTR];       /* pattern for decoupler pulse */
char  decpat6[MAXSTR];       /* pattern for decoupler pulse */

double  dpwr1;               /* Decoupler pulse power */
double  dpwr5;               /* Decoupler pulse power */


/*------------------------------------------------------------------
    GRADIENTS
------------------------------------------------------------------*/
int gxFlip = 1;              /* allows for wiring flips etc */
int gyFlip = 1;
int gzFlip = 1;
double  gro,gro2,gro3;       /* read out gradient strength */
double  gpe,gpe2,gpe3;       /* phase encode for 2D, 3D & 4D */
double  gss,gss2,gss3;       /* slice-select gradients */
double  gror;                /* read out refocus */
double  gssr;                /* slice select refocus */
double  grof;                /* read out refocus fraction */
double  gssf;                /* slice refocus fraction */
double  g0,g1,g2,g3,g4;      /* numbered levels */
double  g5,g6,g7,g8,g9;      /* numbered levels */
double  gx,gy,gz;            /* X, Y, and Z levels */
double  gvox1,gvox2,gvox3;   /* voxel selection */
double  gdiff;               /* diffusion encode */
double  gflow;               /* flow encode */
double  gspoil,gspoil2;      /* spoiler gradient levels */
double  gcrush,gcrush2;      /* crusher gradient levels */
double  gtrim,gtrim2;        /* trim gradient levels */
double  gramp,gramp2;        /* ramp gradient levels */
double  gpemult;             /* shaped phase-encode multiplier */
double  gradstepsz;	    /* positive steps in the gradient dac */
double  gradunit;            /* dimensional conversion factor */
double  gmax;                /* maximum gradient value (G/cm) */
double  gxmax;               /* x maximum gradient value (G/cm) */
double  gymax;               /* y maximum gradient value (G/cm) */
double  gzmax;               /* z maximum gradient value (G/cm) */
double  gtotlimit;           /* limit for combined gradient values (G/cm) */
double  gxlimit;             /* safety limit for x gradient  (G/cm) */
double  gylimit;             /* safety limit for y gradient  (G/cm) */
double  gzlimit;             /* safety limit for z gradient  (G/cm) */
double  gxscale;             /* X scaling factor for gmax */
double  gyscale;             /* Y scaling factor for gmax */
double  gzscale;             /* Z scaling factor for gmax */

char  gpatup[MAXSTR];        /* gradient ramp-up pattern */
char  gpatdown[MAXSTR];      /* gradient ramp-down pattern */
char  gropat[MAXSTR];        /* readout gradient pattern */
char  gpepat[MAXSTR];        /* phase-encode gradient pattern */
char  gsspat[MAXSTR];        /* slice gradient pattern */
char  gpat[MAXSTR];         /* general gradient pattern */
char  gpat1[MAXSTR];         /* general gradient pattern */
char  gpat2[MAXSTR];         /* general gradient pattern */
char  gpat3[MAXSTR];         /* general gradient pattern */
char  gpat4[MAXSTR];         /* general gradient pattern */
char  gpat5[MAXSTR];         /* general gradient pattern */


/*------------------------------------------------------------------
    DELAYS
------------------------------------------------------------------*/
double  tr;                  /* repetition time per scan */
double  te;                  /* primary echo time */
double  ti;                  /* inversion time */
double  tm;                  /* mid delay for STE */
double  at;                  /* acquisition time */
double  tpe,tpe2,tpe3;       /* phase encode durations for 2D-4D */
double  tcrush;              /* crusher gradient duration */
double  tdiff;               /* diffusion encode duration */
double  tdelta;              /* diffusion encode duration */
double  tDELTA;              /* diffusion gradient separation */
double  tflow;               /* flow encode duration */
double  tspoil;              /* spoiler duration */
double  hold;                /* physiological trigger hold off */
double  trise;               /* gradient coil rise time: sec */
double  satdly;              /* saturation time */
double  tau;                 /* general use delay */
double  runtime;             /* user variable for total exp time */


/*------------------------------------------------------------------
    FREQUENCIES
------------------------------------------------------------------*/
double  resto;               /* reference frequency offset */
double  wsfrq;               /* water suppression offset */
double  chessfrq;            /* chemical shift selection offset */
double  satfrq;              /* saturation offset */
double  mtfrq;               /* magnetization transfer offset */


/*------------------------------------------------------------------
    PHYSICAL SIZES AND POSITIONS
      Dimensions and positions for slices, voxels and fov
------------------------------------------------------------------*/
double  pro;                 /* fov position in read out */
double  ppe,ppe2,ppe3;       /* fov position in phase encode */
double  pos1,pos2,pos3;      /* voxel position */
double  pss[MAXSLICE];       /* slice position array */

double  lro;                 /* read out fov */
double  lpe,lpe2,lpe3;       /* phase encode fov */
double  lss;                 /* dimension of multislice range */

double  vox1,vox2,vox3;      /* voxel size */
double  thk;                 /* slice or slab thickness */

double  fovunit;             /* dimensional conversion factor */
double  thkunit;             /* dimensional conversion factor */


/*------------------------------------------------------------------
    BANDWIDTHS
------------------------------------------------------------------*/
double  sw1,sw2,sw3;         /* phase encode bandwidths */


/*------------------------------------------------------------------
    ORIENTATION PARAMETERS
------------------------------------------------------------------*/
char  orient[MAXSTR];        /* slice orientation */
char  vorient[MAXSTR];       /* voxel orientation */
char  dorient[MAXSTR];       /* diffusion gradient orientation */
char  sorient[MAXSTR];       /* saturation band orientation */
char  orient2[MAXSTR];       /* spare orientation */

double  psi,phi,theta;       /* slice Euler angles */
double  vpsi,vphi,vtheta;    /* voxel Euler angles */
double  dpsi,dphi,dtheta;    /* diffusion gradient Euler angles */
double  spsi,sphi,stheta;    /* saturation band Euler angles */

double offsetx,offsety,offsetz;  /* shim offsets for coordinate rotator board */
double gxdelay,gydelay,gzdelay; /* gradient amplifier delays for coordinate rotator board */

/*------------------------------------------------------------------
    COUNTS AND FLAGS
------------------------------------------------------------------*/
double  nD;                  /* experiment dimensionality */
double  ns;                  /* number of slices */
double  ne;                  /* number of echoes */
double  ni;                  /* number of standard increments */
double  ni2;                 /* number of 3d increments */
double  nv,nv2,nv3;          /* number of phase encode views */
double  ssc;                 /* compressed ss transients */
double  ticks;               /* external trigger counter */

char  ir[MAXSTR];            /* inversion recovery flag */
/* char  ws[MAXSTR];            water suppression flag */
char  mt[MAXSTR];            /* magnetization transfer flag */
char  pilot[MAXSTR];         /* auto gradient balance flag */
char  seqcon[MAXSTR];        /* acquisition loop control flag */
char  petable[MAXSTR];       /* name for phase-encode table */
char  acqtype[MAXSTR];       /* e.g. "full" or "half" echo */
char  exptype[MAXSTR];       /* e.g. "se" or "fid" in CSI */
char  apptype[MAXSTR];       /* keyword for param init, e.g., "imaging"*/
char  seqfil[MAXSTR];        /* pulse sequence name */
char  rfspoil[MAXSTR];       /* rf spoiling flag */
char  satmode[MAXSTR];       /* presaturation mode */
char  verbose[MAXSTR];       /* verbose mode for sequences and psg */


/*------------------------------------------------------------------
    Miscellaneous
------------------------------------------------------------------*/
double  rfphase;             /* rf phase shift  */
double  B0;                  /* static magnetic field level */
char    presig[MAXSTR];      /* PIC high/low gain setting */
double  gpropdelay;          /* Gradient propagation delay for grad_advance */
double  aqtm;                /*  */
char    volumercv[MAXSTR];   /* flag to control volume vs. surface coil receive */
double  kzero;               /* position of zero kspace line in etl */


/*------------------------------------------------------------------
    Old SISCO Imaging Variables
------------------------------------------------------------------*/
double  slcto;               /* slice selection offset */
double  delto;               /* slice spacing frequency */
double  tox;                 /* Transmitter Offset */
double  toy;                 /* Transmitter Offset */
double  toz;                 /* Transmitter Offset */
double  griserate;           /* Gradient riserate  */


/************************************************************************/
/*	End SIS Globals							*/
/************************************************************************/

/* --- Pulse Seq. globals --- */
char vnHeader[50];		/* header sent to vnmr */

/*  RF Channels */
//  these are DEFAULTS - not defines...
int OBSch=1;			/* The Acting Observe Channel */
int DECch=2;  			/* The Acting Decoupler Channel */
int DEC2ch=3;  			/* The Acting 2nd Decoupler Channel */
int DEC3ch=4;  			/* The Acting 3rd Decoupler Channel */
int DEC4ch=5;  			/* The Acting 4th Decoupler Channel */
int NUMch=2;			/* Number of channels configured */

int specialGradtype = 0;


#ifdef DBXTOOL
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
struct _cfindex
{
     int NFids;
     int OffSets[10];
};
typedef struct _cfindex cfindex;

extern cfindex *CFindex;
#endif

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
|	  fidscan  similar to acqi, except for vnmrj
|	  ra    which means the VNMR command is RA, not GO
+-------------------------------------------------------------------------*/
char    filepath[MAXPATHL];		/* file path for Codes */
char    filexpath[MAXPATHL];		/* file path for exp# file */
char    fileRFpattern[MAXPATHL];	/* path for obs & dec RF pattern file */
char    filegrad[MAXPATHL];		/* path for Gradient file */
char    filexpan[MAXPATHL];		/* path for Future Expansion file */
char    abortfile[MAXPATHL];		/* path for abort signal file */

/* Used by locksys.c  routines from vnmr */
char systemdir[MAXPATHL];       /* vnmr system directory */
char userdir[MAXPATHL];         /* vnmr user system directory */
char curexp[MAXPATHL];       /* current experiment path */






/*-------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    char    filename[MAXPATHL];         /* file name for Codes */
    char   *gnames[50];
    char   *appdirs;

    double arraydim;    /* number of fids (ie. set of acodes) */

    int     ngnames;
    int     i;
    int     nextflag = 0;
    int     Rev_Num;
    int     P_rec_stat;


  tuneflag = acqiflag = ra_flag = dps_flag = waitflag = fidscanflag = checkflag = nomessageflag = 0;
  ok = TRUE;


  setupsignalhandler();  /* catch any exception that might core dump us. */



   /* check PSG Revision for compatibilty */

   /* END check PSG Revision for compatibilty */



   /* Check arguments */
	if (argc < 7)  /* not enought args to start, exit */
	{
		fprintf(stderr,
		    "This is a background task! Only execute from within 'vnmr'!\n");
		exit(1);
	}
	Rev_Num = atoi(argv[1]); /* GO -> PSG Revision Number */
	pipe1[0] = atoi(argv[2]); /* convert file descriptors for pipe 1*/
	pipe1[1] = atoi(argv[3]);
	pipeB[0] = atoi(argv[4]); /* convert file descriptors for pipe 2*/
	pipeB[1] = atoi(argv[5]);
	setupflag = atoi(argv[6]);	/* alias flag */
	if (bgflag)
	{
		fprintf(stderr,"\n BackGround PSG job starting\n");
		fprintf(stderr,"setupflag = %d \n", setupflag);
	}


	close(pipe1[1]); /* close write end of pipe 1*/
	close(pipeB[0]); /* close read end of pipe 2*/


	P_rec_stat = P_receive(pipe1);  /* Receive variables from pipe and load trees */


	close(pipe1[0]);
	/* check options passed to psg */
	if (option_check("next"))
		waitflag = nextflag = 1;
	if (option_check("ra"))
		ra_flag = 1;
	if (option_check("acqi"))
		acqiflag = 1;
	if (option_check("tune"))
		tuneflag = 1;
	if (option_check("fidscan"))
		fidscanflag = 1;
	if (option_check("debug"))
		bgflag++;
	if (option_check("debug2"))
		debug2 = 1;
	if (option_check("sync"))
		waitflag = 1;
	if (option_check("prep"))
		prepScan = 1;
	if (option_check("nosafe"))
		safetyoff = 1;
	if (option_check("nomessage"))
		nomessageflag = 1;
	if (option_check("check"))
	{
		checkflag = 1;
	}
	bugmask = option_check("bugmask");
	lockfid_mode = option_check("lockfid");
	clr_at_blksize_mode = option_check("bsclear");
	debug = bgflag;
        gradtype_check();

	if (bgflag)
	  fprintf(stderr,"PSG: Piping Complete\n");

#ifdef DBXTOOL
sleep(30);
#endif

    /* ------- Check For GO - PSG Revision Clash ------- */
    if (Rev_Num != GO_PSG_REV )
    {
        abort_message("GO(%d) and PSG(%d) Revision Clash, PSG Aborted.\n",
          Rev_Num,GO_PSG_REV);
    }
    if (P_rec_stat == -1 )
    {
        text_error("P_receive had a fatal error.\n");
        psg_abort(1);
    }



    /*-----------------------------------------------------------------
    |  begin of PSG task
    +-----------------------------------------------------------------*/

    newacq = 1;

    if (strncmp("dps", argv[0], 3) == 0)
    {
       dps_flag = 1;
       if ((int)strlen(argv[0]) >= 5)
       {
          if (argv[0][3] == '_')
             dps_flag = 3;
       }
       checkflag = acqiflag = 0;
    }

    A_getstring(CURRENT,"appdirs", &appdirs, 1);
    setAppdirValue(appdirs);
    setup_comm();   /* setup communications with acquisition processes */
    getparm("goid","string",CURRENT,filename,MAXPATHL);


    /* check for any variations in new console implementation for parameters */
    /*     for e.g. any parameter implns disabled                            */
    checkImplementation();


    if (fidscanflag)
    {
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
        P_setreal(CURRENT,"ss",0.0,1);
        P_setstring(CURRENT,"dp","y",1);
        P_setreal(CURRENT,"bs",1.0,1);
        P_setactive(CURRENT,"bs",ACT_ON);
        P_setreal(CURRENT,"nt",1e6,1);
        if (P_setreal(CURRENT,"lockacqtc",1.0,1))
        {
           P_creatvar(CURRENT, "lockacqtc", T_REAL);
           P_setreal(CURRENT,"lockacqtc",1.0,1);
        }
        
         /* fid scan mode for phase cycling */
        double fsval_temp;
        if (P_getreal(CURRENT, "fidshimnt", &fsval_temp, 1) <  0)
        {
           P_creatvar(CURRENT, "fidshimnt", T_REAL);
           P_setreal(CURRENT,"fidshimnt",1.0,1);
           P_setstring(CURRENT,"cp","n",1);
        }
        else
        {
          if ( (fsval_temp > 1.0) && ((int)(fsval_temp+0.49) <= MAX_FIDSHIMNT) )
            P_setreal(CURRENT,"bs",fsval_temp,1);
          else if ((int)(fsval_temp+0.49) > MAX_FIDSHIMNT)
          {
            fsval_temp = MAX_FIDSHIMNT;
            P_setreal(CURRENT,"fidshimnt",fsval_temp,1);
            P_setreal(CURRENT,"bs",fsval_temp,1);
            text_message("advisory: fidshimnt may be too large for fid shimming!    max value of %d used for fidshimnt.\n",(int)(fsval_temp+0.49));
          }
          else if ((int)(fsval_temp+0.49) == 1)
          {
            P_setstring(CURRENT,"cp","n",1);
            P_setreal(CURRENT,"bs",1.0,1);
          }
          /* else if fidshimnt==0 then bs=1 & cp setting in parameter set used */
        }
    }
    if (clr_at_blksize_mode)
    {
       if (var_active("bs",CURRENT) <= 0)
          clr_at_blksize_mode = 0;
       if (clr_at_blksize_mode)
       {
          double tmp;

          P_getreal(CURRENT, "bs", &tmp, 1);
          if (tmp <= 0.5)
             clr_at_blksize_mode = 0;
       }
    }
    checkGradtype();
    checkGradAlt();
    if (!dps_flag)
       if (setup_parfile(setupflag))
       {
          text_error("Parameter file error PSG Aborted..\n");
          psg_abort(1);
       }

   strcpy(filepath,filename);  /* /vnmrsystem/acqqueue/exp1.greg.012345 */
   strcpy(filexpath,filepath); /* save this path for psg_abort() */
   /* strcat(filepath,".Code");   /vnmrsystem/acqqueue/exp1.greg.012345.Code */

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
    cnames = new char *[ntotal];
    cvals = new double *[ntotal];
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
                    i,cnames[i],(char *)cvals[i]);
            }
        }
    }

    /* variable look up now done with hash tables */
    init_hash(ntotal);
    load_hash(cnames,ntotal);

    /* NB: parameter acqcycles now gives number of acode sets;
     * "arraydim" parameter is number of data blocks. */
    if (getparmd("acqcycles","real",CURRENT,&arraydim,1))
        psg_abort(1);
    if (setGflags()) psg_abort(0);
    initparms();
    if (tuneflag)
       interLock[2] = 'n';   /* disable vt wait when tuning */

    /* if not a GO force to 1D */
    /* any setup is 1D, nomatter what. */
    if (acqiflag || checkflag || (setupflag > 0) || (dps_flag > 1) )
        arraydim = 1.0;
/***
    if (dps_flag)
    {
       if (arraydim < 1.5)
          createDPS(argv[0], curexp, arraydim, 0, NULL, pipeB[1]);
       else
          PSGDps(argv[0]);
       close_error(0); 
       exit(0);
    }
***/

/*----------------------------------------------------------------









+------------------------------------------------------------------*/

/* Build console objects and configure them to match spectrometer hardware */

  Console *console;
  console = new Console("Nirvana");
  console->setChannelMapping();


/* --- set up lc data sturcture  --- */

  InitAcqObject *initAcqObject = console->getInitAcqObject();

    if (dps_flag)
    {
       if (arraydim < 1.5)
          createDPS(argv[0], curexp, arraydim, 0, NULL, pipeB[1]);
       else
          PSGDps(argv[0]);
       close_error(0);    /* 0 arg means success */
       exit(0);
    }

  initAcqObject->writeLCdata();


/* --- set up automation data sturcture  --- */

  initAcqObject->writeAutoStruct();



/* write INIT code for "go" or "su" for various controllers */

  console->initializeExpStates(setupflag);


   if (setupflag != GO)
   {
     PSGSetup(arraydim);
   }
   else
   {
     PSGGo(arraydim);
   }

  console->closeAllFiles(0);

  if (checkflag)
  {
    if (!option_check("checksilent"))
      text_message("go check complete\n");
    closeFiles();
    close_error(0);   /* 0 arg means success */
    exit(0);
  }

#ifndef __INTERIX
  sync();
#endif
  if (!acqiflag && QueueExp(filexpath,nextflag))
      psg_abort(1);

  if (bgflag)
     fprintf(stderr,"PSG terminating normally\n");
    /* close_error(); inova */

   close_error(0);   /* 0 arg means success */
   exit(0);

}

void PSGSetup(double arraydim)
{
   double duration = 0.0;

   AcodeManager *acodeMgr = AcodeManager::getInstance();

   /* increment Acode Stage to PRE  */
   acodeMgr->incrementAcodeStage();

   /* increment Acode Stage to PS  */
   acodeMgr->incrementAcodeStage();
   /* NO ACTION DURING PS STAGE */

   if ( option_check("pause") )
   {
      P_getreal(CURRENT,"pad",&duration,1);
      if (duration < 1)
         duration = 1.0;
   }

   /* increment Acode Stage to POST  */
   acodeMgr->incrementAcodeStage();

   /* POST stage acode generation goes here  */
   /*     P O S T    S T A G E               */

   /* increment Acode Stage to the end  */
   acodeMgr->incrementAcodeStage();

   /* update rcvrs string variable into ExpInfo struct */
   if (!dps_flag)
      set_rcvrs_info();

   /* CAUTION: need to correct arg for write_shr_info(exptime) */
   write_shr_info(duration);
}


void PSGGo(double arraydim)
{
   int narrays;
   char    array[MAXSTR];
   char    parsestring[MAXSTR];
   char    arrayStr[MAXSTR];
   char    psgFilePath[MAXSTR];
    double ni = 0.0;
    double ni2 = 0.0;
    double ni3 = 0.0;


   /* if (getparmd("acqcycles","real",CURRENT,&arraydim,1))
      psg_abort(1); */

   if (checkflag) arraydim = 1.0;
   if (option_check("checkarray"))
   {
      checkflag = 1;
   }

   AcodeManager *acodeMgr = AcodeManager::getInstance();

   /* increment Acode Stage to PRE  */
   acodeMgr->incrementAcodeStage();


   /* increment Acode Stage to PS  */
   acodeMgr->incrementAcodeStage();

   /* open file to report shapefile names */
   sprintf(psgFilePath,"%s/PsgFile",curexp);
   if ( (shapeListFile = fopen(psgFilePath,"w")) == NULL )
      text_message("advisory: could not open PsgFile in curexp\n");

   if (arraydim <= 1.5)
   {
      ix = 1;
      pre_expsequence();
      acodeMgr->startSubSection(ACODEHEADER+ix);

      MainSystemSync();

      initscan();

      createPS(1);

      dfltacq();

      resolve_endofscan_actions();

      endofscan();
      endofExperiment(0);  /* may eliminate later stages or move to them */
 			/* 0 arg is real experiment - 1 is SU
			   NO PROVISIONS FOR SU ?? */

      P2TheConsole->checkForErrors();

      acodeMgr->endSubSection();

      first_done();
      totaltime = P2TheConsole->duration() + getExtraLoopTime();
      if (!padactive)
         pad = 0.0;
      totaltime -= pad;
      totaltime -= preNextScanTime;
      if (var_active("ss",CURRENT))
      {
         int ss = (int) (sign_add(getval("ss"), 0.0005));
         if (ss < 0)
            ss = -ss;
         totaltime *= (getval("nt") + (double) ss);   /* mult by NT + SS */
      }
      else
         totaltime *= getval("nt");   /* mult by number of transients (NT) */
      totaltime += preNextScanTime;
      exptime = (totaltime + pad);
   }
   else
   {
      ix = 0;
      initglobalptrs();
      initlpelements();


      if (getparm("array","string",CURRENT,array,MAXSTR))
            psg_abort(1);
      strcpy(parsestring,array);

        /*----------------------------------------------------------------
        |       test for presence of ni, ni2, and ni3
        |       generate the appropriate 2D/3D/4D experiment
        +---------------------------------------------------------------*/
        P_getreal(CURRENT, "ni", &ni, 1);
        P_getreal(CURRENT, "ni2", &ni2, 1);
        P_getreal(CURRENT, "ni3", &ni3, 1);

        if (setup4D(ni3, ni2, ni, parsestring, array))
           psg_abort(0);

        if (dps_flag)
           strcpy(arrayStr, parsestring);

        /*----------------------------------------------------------------*/

        if (bgflag)
          fprintf(stderr,"parsestring: '%s' \n",parsestring);
        if (parse(parsestring, &narrays))  /* parse 'array' setup looping elements */
          psg_abort(1);

      pre_expsequence();

      arrayPS(0,narrays, (int) (arraydim+0.1));
   }

   /* increment Acode Stage to POST  */
   acodeMgr->incrementAcodeStage();


   /* POST stage acode generation goes here  */
   /*     P O S T    S T A G E               */



   /* increment Acode Stage to the end  */
   acodeMgr->incrementAcodeStage();

   /* update rcvrs string variable into ExpInfo struct */
   if (!dps_flag)
      set_rcvrs_info();

   if (!checkflag)
      write_shr_info(exptime);

   if (shapeListFile != NULL)
   {
      fclose(shapeListFile);
      if ( ! shapesWritten )
         unlink(psgFilePath);
   }

}

static void PSGDps(char *cmd)
{
   int narrays;
   double arraydim;
   char    array[MAXSTR];
   char    parsestring[MAXSTR];
   char    arrayStr[MAXSTR];
   double ni = 0.0;
   double ni2 = 0.0;
   double ni3 = 0.0;

   if (getparmd("acqcycles","real",CURRENT,&arraydim,1))
      psg_abort(1);
   ix = 0;
   init_shimnames(GLOBAL);
   initglobalptrs();
   initlpelements();
   if (getparm("array","string",CURRENT,array,MAXSTR))
      psg_abort(1);
   strcpy(parsestring,array);

   /*----------------------------------------------------------------
   |       test for presence of ni, ni2, and ni3
   |       generate the appropriate 2D/3D/4D experiment
   +---------------------------------------------------------------*/
   P_getreal(CURRENT, "ni", &ni, 1);
   P_getreal(CURRENT, "ni2", &ni2, 1);
   P_getreal(CURRENT, "ni3", &ni3, 1);
   if (setup4D(ni3, ni2, ni, parsestring, array))
      psg_abort(0);
   strcpy(arrayStr, parsestring);
   if (bgflag)
      fprintf(stderr,"parsestring: '%s' \n",parsestring);
   if (parse(parsestring, &narrays))  /* parse 'array' setup looping elements */
      psg_abort(1);
   createDPS(cmd, curexp, arraydim, narrays, arrayStr, pipeB[1]);
}


/*
   INITSCAN
   np
   nt
   ss
   bs
   cp
*/

 void initscan()
 {
   int codes[5], numcodes, ival;
   double val, ntval, ssvalue;
   char cpstr[MAXSTR];

   numcodes = 5;

   if (P_getreal(CURRENT, "np", &val, 1) < 0)
   {
     cout << "initscan(): ERROR: unable to get np value" << endl;
     exit(-1);
   }
   codes[0] = (int) val ;     /* np */

   if (P_getreal(CURRENT, "nt", &val, 1) < 0)
   {
     cout << "initscan(): ERROR: unable to get nt value" << endl;
     exit(-1);
   }
   ntval    = getval("nt") ;
   codes[1] = (int) ntval; // allow nt arrays P_getreal gets first only

   if (P_getreal(CURRENT, "ss", &val, 1) < 0)
   {
     cout << "initscan(): ERROR: unable to get ss value" << endl;
     ssvalue = 0.0;
   }
   else
   {
     ssvalue = val ;   /* ss */
     if (!var_active("ss",CURRENT))
       ssvalue = 0.0;
   }

   if ( (ix > 1) && (ssvalue >= 0.0) )
   {
     if (P_getreal(CURRENT, "ss2", &val, 1) >= 0)
     {
       ssvalue = val;
       if (!var_active("ss2",CURRENT))
         ssvalue = 0.0;
       if (ssvalue < 0.0)
          text_message("warning ss2 is negative!");
     }
     else
     {
       if (ssvalue >= 0.0) ssvalue = 0.0;
     }
   }

   ival  = (int) (sign_add(ssvalue,0.0005));
   if (ival >= 0)
     codes[2] =  ival;
   else
     codes[2] = -ival;


   if (P_getreal(CURRENT, "bs", &val, 1) < 0)
   {
     cout << "initscan(): ERROR: unable to get bs value" << endl;
   }

   if (!var_active("bs",CURRENT))
     val = 0.0;
   if ( val <= ntval)
     codes[3] = (int)val;
   else
     codes[3] = (int)ntval;


   if (P_getstring(CURRENT, "cp", cpstr, 1, MAXSTR) == 0)
   {
     getstr("cp",cpstr);
     cpflag = (cpstr[0] == 'y') ? 0 : 1;
   }
   else
     cpflag = 1;

   /* oph phase cycling   1=no cycling,  0=quad detect */
   codes[4] = cpflag;

   P2TheConsole->broadcastCodes(INITSCAN,numcodes,codes);

   resolve_initscan_actions();
 }


 void nextscan()
 {
   int codes[5], numcodes;
   int nargs;
   nargs = 0;
   numcodes = 5;

   double nt;
   double ct,bs;
   int fidnum;

   np = getval("np") ;  /* np */
   codes[0] = (int)np;  /* np */

   nt       = getval("nt") ;         /* nt */
   codes[1] = (int)nt;

   ct = getval("ct") ;  /* ct */
   codes[2] = (int) ct;  /* ct */

   bs = getval("bs") ;  /* bs */
   if (!var_active("bs",CURRENT))
      bs = 0.0;
   if (bs <= nt)
      codes[3] = (int) bs;  /* bs */
   else
      codes[3] = (int) nt;  /* smaller of bs & nt */

   fidnum = codes[4] = (int)ix ;  /* ix */


   P2TheConsole->broadcastCodes(NEXTSCAN,numcodes,codes);

  // acqtriggers = 0;
 }

double psDuration()
{
   return( P2TheConsole->duration());
}

void preNextScanDuration()
{
   preNextScanTime =  P2TheConsole->duration();
}

void endofscan()
{

   int codes[8], numcodes, repeatscans, channelbits[4];
   numcodes = 8;
   double val, ntval, fsval;

   if (P_getreal(CURRENT, "ss", &val, 1) < 0)
   {
     cout << "endofscan(): ERROR: unable to get ss value" << endl;
   }
   codes[0] = (int)getval("ss") ;  /* ss */

   if (P_getreal(CURRENT, "nt", &val, 1) < 0)
   {
     cout << "endofscan(): ERROR: unable to get nt value" << endl;
   }
   ntval    = getval("nt") ;  /* nt */
   codes[1] = (int)ntval;

   if (P_getreal(CURRENT, "bs", &val, 1) < 0)
   {
     cout << "endofscan(): ERROR: unable to get bs value" << endl;
   }
   val = getval("bs") ;            /* bs */
   if (!var_active("bs",CURRENT))
     val = 0.0;
   if ( val <= ntval)
     codes[2] = (int)val;
   else
     codes[2] = (int)ntval;

   /* fid scan mode for phase cycling */
   repeatscans = 0;
   if (fidscanflag)
   {
     if (P_getreal(CURRENT, "fidshimnt", &fsval, 1) >= 0)
     {
        fsval = getval("fidshimnt");
        if ((fsval > 0.0) && ((int)(fsval+0.49) <= MAX_FIDSHIMNT) )
        {
          repeatscans = (int)(fsval+0.49);
        }
     }
   }
   codes[3] = repeatscans;

   /* set the active channel bit information into acode */
   P2TheConsole->getChannelBits(channelbits);
   for (int i=0; i<4; i++)
      codes[4+i] = channelbits[i];

   P2TheConsole->broadcastCodes(ENDOFSCAN,numcodes,codes);

   /* P2TheConsole->describe2((char *)("ENDOFSCAN\0")); */
}



void nextcodeset()
{
  int codes[2], numcodes;
  numcodes = 0;

  codes[0] = -999;
  codes[1] = -999;

  P2TheConsole->broadcastCodes(NEXTCODESET,numcodes,codes);
}

void endofExperiment(int setuptype)
{
  int codes[2], numcodes;

  resolve_endofexpt_actions();

   // if not setup then obtain the il flag
   if (setuptype != 1)
   {
      /* il flag */
	  if (getIlFlag() == 1)
     {
        codes[0] = 1;
        codes[1] = -999;
        P2TheConsole->broadcastCodes(ILCJMP,1,codes);
     }
  }

  codes[0] = setuptype;
  codes[1] = -999;
  numcodes = 1;
  P2TheConsole->broadcastCodes(END_PARSE,numcodes,codes);
}

void MainSystemSync()
{
   double nsync;
   if (ix == 1)
   {
      nsync=1.0;
      P_getreal(CURRENT,"nsync",&nsync,1);
      if ((int)nsync)
         P2TheConsole->broadcastSystemSync(320 /* ticks */, prepScan);  /* 4 usec, a grad requirement */
   }
}


/*
|   This writes to the pipe that go is waiting on to decide when the first
|   element is done.
+--------------------------------------------------------------------------*/
void first_done()
{
   closeCmd();
   write(pipeB[1],&ready,sizeof(int)); /* tell go, PSG is done with first element */
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
       write(pipeB[1],&success,sizeof(int)); /* tell go, PSG is done */
    else if (!dps_flag && (getparm("auto","string",GLOBAL,autopar,12) == 0))
        if ((autopar[0] == 'y') || (autopar[0] == 'Y'))
            write(pipeB[1],&success,sizeof(int)); /* tell go, PSG is done */
    close(pipeB[1]); /* close write end of pipe 2*/
}
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
void reset()
{
    if (cnames) free(cnames);
    if (cvals) free(cvals);
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
|	This function arrays the d4, d3 and d2 variables with the correct
|	values for the 2D/3D/4D experiment.  d2 increments first, then d3,
|	and then d4.
|
+-------------------------------------------------------------------------*/
int setup4D(double ni3, double ni2, double ni, char *parsestr, char *arraystr)
{
   int		index = 0,
		i,
		num = 0;
   int          elCorr = 0;
   double	sw1,
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
      int sparseDim;
      if (getparmd("sw3", "real", CURRENT, &sw3, 1))
         return(ERROR);
      if (getparmd("d4","real",CURRENT,&d4_init,1))
         return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d4")) != -1) );
      if ( ! findWord("d4",arraystr))
      {
         if ( ! sparseDim)
            strcpy(parsestr, "d4");
      }
      else
         elCorr++;

      inc4D = 1.0/sw3;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d4 = d4_init + csIndex * inc4D;
            }
            else
            {
               d4 = d4_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d4", d4, i+1)) == ERROR)
            {
               text_error("Could not set d4 values");
	       return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni3 - 1.0 + 0.5);
         d4 = d4_init;
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
   }

   if (ni2 > 1.5)
   {
      int sparseDim;
      if (getparmd("sw2", "real", CURRENT, &sw2, 1))
           return(ERROR);
      if (getparmd("d3","real",CURRENT,&d3_init,1))
           return(ERROR);

      sparseDim = (sparse && ( (index = getCSparIndex("d3")) != -1) );
      if ( ! findWord("d3",arraystr))
      {
         if ( ! sparseDim )
         {
         if (strcmp(parsestr, "") == 0)
         {
            strcpy(parsestr, "d3");
         }
         else
         {
            strcat(parsestr, ",d3");
         }
         }
      }
      else
      {
         elCorr++;
      }

      inc3D = 1.0/sw2;
      if (sparseDim)
      {
         num  = getCSnum();
         for (i = 0;  i < num; i++)
         {
            if (CStype)
            {
               int csIndex;

               csIndex = (int) getCSval(index, i);
               d3 = d3_init + csIndex * inc3D;
            }
            else
            {
               d3 = d3_init + getCSval(index, i);
            }
            if ((P_setreal(CURRENT, "d3", d3, i+1)) == ERROR)
            {
               text_error("Could not set d3 values");
	       return(ERROR);
            }
         }
      }
      else
      {
         num = (int) (ni2 - 1.0 + 0.5);
         d3 = d3_init;
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
   }

   if (ni > 1.5)
   {
      int sparseDim;
      if (getparmd("sw1", "real", CURRENT, &sw1, 1))
           return(ERROR);
      if (getparmd("d2","real",CURRENT,&d2_init,1))
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
/* static void queue_psg(char auto_dir[0],char fid_name[0],char *msg) */
static void queue_psg(char *auto_dir,char *fid_name,char *msg)
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

         /* get index to DATA: prompt, then update data text field with data file path */
         get_sample_info(&sampleinfo,"DATA:",val,128,&entryindex);
         strncpy(sampleinfo.prompt_entry[entryindex].etext,fid_name,MAX_TEXT_LEN);

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
+----------------------------------------------------------------------------*/
#define QUEUE 1
int QueueExp(char *codefile,int nextflag)
{
    /* double priority; */
    /*double expflags;*/
    char fidpath[MAXSTR];
    char addr[MAXSTR];
    char message[MAXSTR];
    char commnd[MAXSTR*2];
    char autopar[12];
    int autoflag;
    int expflags;
    int priority;

/*	No Longer use Vnmr priority parameter, now used for U+ non-automation go and go('next') */
/*
    if (getparmd("priority","real",CURRENT,&priority,1))
    {
        text_error("cannot get priority variable\n");
        return(ERROR);
    }
*/

    if (!nextflag)
      priority = 5;
    else
      priority = 6;  /* increase priority to this Exp run next */


    if (getparm("auto","string",GLOBAL,autopar,12))
        autoflag = 0;
    else
        autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));

    expflags = 0;
    if (autoflag)
	expflags = ((int)expflags | AUTOMODE_BIT);  /* set automode bit */

    if (ra_flag)
        expflags |=  RESUME_ACQ_BIT;  /* set RA bit */

    if ( ! P_getstring(CURRENT,"vpmode",autopar,1,12) && (autopar[0] == 'y') )
        expflags |= VPMODE_BIT;  /* set vpmode bit */
    if (getparm("file","string",CURRENT,fidpath,MAXSTR))
	return(ERROR);
    if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
	return(ERROR);
    sprintf(message,"%d,%s,%d,%lf,%s,%s,%s,%s,%s,%d,%d,%u,%u,",
		QUEUE,addr,
		(int)priority,exptime,fidpath,codefile,
		fileRFpattern, filegrad, filexpan,
		(int)setupflag,(int)expflags,
                start_elem, completed_elem);
    if (bgflag)
    {
      fprintf(stderr,"fidpath: '%s'\n",fidpath);
      fprintf(stderr,"codefile: '%s'\n",codefile);
      fprintf(stderr,"priority: %d'\n",(int) priority);
      fprintf(stderr,"time: %lf\n",totaltime);
      fprintf(stderr,"suflag: %d\n",setupflag);
      fprintf(stderr,"expflags: %d\n",expflags);
      fprintf(stderr,"start_elem: %u\n",start_elem);
      fprintf(stderr,"completed_elem: %u\n",completed_elem);
      fprintf(stderr,"msge: '%s'\n",message);
      fprintf(stderr,"auto: %d\n",autoflag);
    }
    check_for_abort();
    if (autoflag)
    {   char autodir[MAXSTR];
        int ex;

        if (getparm("autodir","string",GLOBAL,autodir,MAXSTR))
	    return(ERROR);
        if (newacq)
        {
           char tmpstr[MAXSTR];

           P_getstring(CURRENT,"goid",tmpstr,3,255);
           sprintf(message,"%s %s auto",codefile,tmpstr);
        }

        /* There is a race condition between writing out the
           psgQ files and go.c preparing the .fid directory where
           the data will be stored.  This section of code makes
           sure go.c finishes creating the .fid directory before
           psgQ is written.  There is a 5 second timeout. Generally,
           it waits between 0 and 20 msec.
         */
        if (fidpath[0] == '/')
           sprintf(commnd,"%s.fid/text",fidpath);
        else
           sprintf(commnd,"%s/%s.fid/text",autodir,fidpath);
        ex = 0;
        while ( access(commnd,R_OK) && (ex < 50) )
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
        if ( shapesWritten )
        {
           if (fidpath[0] == '/')
              sprintf(commnd,"cp %s/PsgFile %s.fid/PsgFile",curexp,fidpath);
           else
              sprintf(commnd,"cp %s/PsgFile %s/%s.fid/PsgFile",curexp,autodir,fidpath);
           system(commnd);
        }
        if ( getCSnum() )
        {
           char *csname = getCSname();
           if (fidpath[0] == '/')
              sprintf(commnd,"cp %s/%s %s.fid/%s",curexp,csname,fidpath,csname);
           else
              sprintf(commnd,"cp %s/%s %s/%s.fid/%s",curexp,csname,autodir,fidpath,csname);
           system(commnd);
        }

        lockPsgQ(autodir);
        if (nextflag)
        {

          /* Temporarily use commnd to hold the filename */
          sprintf(commnd,"%s/psgQ",autodir);
          ex = access(commnd,W_OK);
          if (ex == 0)
          {
             sprintf(commnd,"mv %s/psgQ %s/psgQ.locked",autodir,autodir);
             system(commnd);
          }
          queue_psg(autodir,fidpath,message);
          if (ex == 0)
          {
             sprintf(commnd,"cat %s/psgQ.locked >> %s/psgQ; rm %s/psgQ.locked",
                           autodir,autodir,autodir);
             system(commnd);
          }
        }
        else
        {
          queue_psg(autodir,fidpath,message);
        }
        unlockPsgQ(autodir);
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
       sprintf(infostr,"%s %s %d",tmpstr,tmpstr2,setupflag);
       if (bgflag)
       {
          fprintf(stderr,"addr: '%s', codefile: '%s', infostr: '%s', nextflag: %d\n",
	          addr,codefile,infostr,nextflag);
       }
       if (expflags & VPMODE_BIT)
       {
          int ex;
          ex = 0;
          P_getstring(CURRENT,"exppath",tmpstr,1,255);
          strcpy(tmpstr2,tmpstr);
          strcat(tmpstr,"/text");
          while ( access(tmpstr,R_OK) && (ex < 50) )
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
          if (shapesWritten)
          {
             sprintf(commnd,"cp %s/PsgFile %s/PsgFile",curexp,tmpstr2);
             system(commnd);
          }
          if ( getCSnum() )
          {
             char *csname = getCSname();

             sprintf(commnd,"cp %s/%s %s/%s",curexp,csname,tmpstr2,csname);
             system(commnd);
          }
       }
       else if ( getCSnum() )
       {
          char *csname = getCSname();

          sprintf(commnd,"cp %s/%s %s/acqfil/%s",curexp,csname,curexp,csname);
          system(commnd);
       }
       if (sendNvExpproc(addr,codefile,infostr,nextflag))
       {
        text_error("Experiment unable to be sent\n");
        return(ERROR);
       }
    }
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
int option_check(const char *option)
{
   vInfo  varinfo;
   int num;
   size_t oplen;
   char tmpstring[MAXSTR];

   if ( P_getVarInfo(CURRENT,"go_Options",&varinfo) )
      return(0);
   num = 1;
   oplen = strlen(option);
   while (num <= varinfo.size)
   {
      if ( P_getstring(CURRENT,"go_Options",tmpstring,num,MAXPATHL-1) >= 0 )
      {
         if ( strncmp(tmpstring, option, oplen) == 0 )
	 {
	     if (strlen(tmpstring) == oplen)
	     {
		 return(1);
	     }
	     else
	     {
		 if (tmpstring[oplen] == '=')
		 {
		     return (int)strtol(tmpstring+oplen+1, NULL, 0);
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
   return(0);
}

int
is_psg4acqi()
{
    return( acqiflag );
}

/*------------------------------------------------------------------------------
|
|	setGflags()
|
|	This function sets the global flags and variables that do not change
|	during the pulse sequence
|
+----------------------------------------------------------------------------*/
static int setGflags()
{
   int i;
   double tmpval;

   if (P_getstring(GLOBAL, "userdir", userdir, 1, MAXPATHL-1) < 0)
   {
      text_error("PSG unable to locate current user directory\n");
      return(1);
   }
   if (P_getstring(GLOBAL, "systemdir", systemdir, 1, MAXPATHL-1) < 0)
   {
      text_error("PSG unable to locate current system directory\n");
      return(1);
   }
   if (P_getstring(GLOBAL, "curexp", curexp, 1, MAXPATHL-1) < 0)
   {
      text_error("PSG unable to locate current experiment directory\n");
      return(1);
   }

   if ( P_getreal(GLOBAL,"numrfch",&tmpval,1) >= 0 )
   {
      NUMch = (int) (tmpval + 0.0005);
      if (( NUMch < 1) || (NUMch > MAX_RFCHAN_NUM))
      {
         abort_message("Number of RF Channels specified '%d' is too large.. PSG Aborted.\n",NUMch);
      }
      /* Fool PSG if numrfch is 1 */
      if ( NUMch < 2)
         NUMch = 2;
   }
   else
      NUMch = 2;

   //
   //
   for (i = 1; i < (NUMch+1); i++)
   {
      if ( P_getreal(GLOBAL,"ptsval",&tmpval,i) >= 0 )
          ptsval[i-1] = (int) (tmpval + .0005);
      else
          ptsval[i-1] = 0;
   }


#ifdef __INTERIX   /* need to convert to full path for access, otherwise it cores */
   {
       int result;
       char interixpath[MAXPATHL];
       sprintf(interixpath,"%s/acqqueue/psg_abort", systemdir);
       // winpath2unix( abortfile, int flags, char *buf, size_t buglen)
       // e.g. /dev/fs/C/SFU/vnmr/acqqueue/psg_abort
       result = winpath2unix( interixpath, 0, abortfile, (size_t) MAXPATHL);
       if (result != 0)
       {
          /* default to standard path */
          sprintf(abortfile,"/dev/fs/C/%s/acqqueue/psg_abort", systemdir);
       }
       // fprintf(stdout,"check_for_abort; '%s'\n",abortfile);
       // fflush(stdout);
   }
#else
   sprintf(abortfile,"%s/acqqueue/psg_abort", systemdir);
#endif

   if (access( abortfile, W_OK ) == 0)
      unlink(abortfile);

   /* --- set VT type on instrument --- */
   if (getparmd("vttype","real",GLOBAL,&tmpval,1))
      return(ERROR);
   vttype = (int) tmpval;
   rfwg[0]='\0';
   if (P_getstring(GLOBAL, "rfwg", rfwg, 1, MAXSTR-4) < 0)
      strcpy(rfwg,"nnnn");
   else
      strcat(rfwg,"nnnn");
   gradtype[0]='\0';
   if (P_getstring(GLOBAL, "gradtype", gradtype, 1, MAXSTR-4) < 0)
      strcpy(gradtype,"nnnn");
   else
      strcat(gradtype,"nnnn");
   if (getparmd("h1freq","real",GLOBAL,&tmpval,1))
      return(ERROR);
   H1freq = (int) tmpval;

   return(0);
}


/* any temporary variations in implementations in new console */

void checkImplementation()
{
    return;

    /* il flag */
	 if (getIlFlag() == 1)
    {
         // P_setstring(CURRENT,"il","n",1);
         // text_message("Interleave (il='y') is not available");
         text_message("You've Enabled Interleave (il='y')");
    }
}

void
preTrimString(char *dest,char *source, int soffset)
{
  int j = 0;
  while( (dest[j] = source[soffset+j]) != '\0' )
    ++j;
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
   double pfgBoard;
   char tmpGradtype[MAXSTR];

   specialGradtype = 0;
   if ( P_getreal(GLOBAL,"pfg1board",&pfgBoard,1) == 0)
   {
      if (pfgBoard > 0.5)
      {
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
         if (specialGradtype)
         {
            tmpGradtype[2] = 'p';
            P_setstring(GLOBAL,"gradtype",tmpGradtype,1);
         }
      }
   }
}


//--------------------- methods for writing nv macros ----------------------------

/* END OF FILE */

