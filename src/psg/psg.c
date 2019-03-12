/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include "group.h"
#include "symtab.h"
#include "variables.h"
#include "params.h"
#include "pvars.h"
#include "REV_NUMS.h"
#include "asm.h"
#include "oopc.h"	/* Object Oriented Programing Package */
#include "ACQ_SUN.h"
#include "rfconst.h"
#include "apdelay.h"
#include "abort.h"
#include "vfilesys.h"
#include "CSfuncs.h"
#include "arrayfuncs.h"


#ifndef ACQPARMS
/* --- code type definitions, can be changed for different machines */
typedef char codechar;          /* 1 bytes */
typedef short codeint;          /* 2 bytes */
typedef long  codelong;         /* 4 bytes */
typedef unsigned long  codeulong; /* 4 bytes */
#endif
/* #define TESTING  */  /* use for stand alone dbx testing */

/*----------------------------------------------------------------------------
|
|	Pulse Sequence Generator  backround task envoked from parent vnmr go
|
|				Author Greg Brissey  5/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Added Code to setGflags() for new Objects, 
|			     Ext_Trig, ToLnAttn, DoLnAttn, HS_Rotor.
|			  2. Added New Parameters dpwr (replaces dhp), tpwrf, dpwrf
|			     to control the fine attenuators.
|   2/10/89   Greg B.     1. Changed QueueExp() to pass the additional information:
|			     start_elem and complete_elem to Acqproc 
|			  2. For ra the RESUME_ACQ_BIT is set in expflags.
|   4/21/89   Greg B.     1. Added New global interleave parameter (il)
|   7/13/89   Greg B.     1. Major changes to the operation of arrayed experiments
|			  2. Many functions moved to arrayfunc.c
|			  3. Hash tables used in getval(), getstr()
|			  4. acq parameters,lc, auto structs initialize in psg instead
|			     of cps.
|		          5. Updating of global acq parameter taken care of in 
|		 	     arrayfuncs.c
|			  6. new global made to reduce number of getval() or getstr()
|			     calls in arrayed experiments.
|			  7. RF setting are calc once instead of many times.
|   7/27/89   Greg B.     1. Added Frequency device parameters & functions.  GMB.
|			  2. Added 3D functions as standard instead of conditionally
|			     compiled.
|  10/25/89   Greg B.     1. Added Hung's DPS Code to PSG.  GMB.
+----------------------------------------------------------------------------*/
/* --- code type definitions, can be changed for different machines */
/*typedef char codechar; */		/* 1 bytes */
/*typedef short codeint; */		/* 2 bytes */
/*typedef long  codelong; */		/* 4 bytes */
/*--------------------------------------------------------------------*/


#define CALLNAME 0
#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1
#define MAXSTR 256
#define MAXARYS 256
#define STDVAR 70
#define MAXVALS 20
#define MAXPARM 60
#define MAXPATHL 128

#define MAXTABLE	60	/* for table implementation (aptable.h) */
#define MAXSLICE      1024      /* maximum number of slices - SISCO */

#define MAX_RFCHAN_NUM 6

/* PSG Revision Number, test by acquisition to confirm compatiblity, 255 Max */
/* #define PSG_REV_NUM 6 */

/* copied from macros.h */

#define is_y(target)   ((target == 'y') || (target == 'Y'))
#define is_w(target)   ((target == 'w') || (target == 'W'))
#define is_r(target)   ((target == 'r') || (target == 'R'))
#define is_p(target)   ((target == 'p') || (target == 'P'))
#define is_q(target)   ((target == 'q') || (target == 'Q'))
#define is_c(target)   ((target == 'c') || (target == 'C'))
#define is_d(target)   ((target == 'd') || (target == 'D'))
#define is_t(target)   ((target == 't') || (target == 'T'))
#define is_u(target)   ((target == 'u') || (target == 'U'))
#define is_porq(target)((target == 'p') || (target == 'P') || (target == 'q') || (target == 'Q'))


#define anyrfwg ((is_y(rfwg[0])) || (is_y(rfwg[1])) || (is_y(rfwg[2])))
#define anygradwg ((is_w(gradtype[0])) || (is_w(gradtype[1])) || (is_w(gradtype[2])))
#define anygradcrwg ((is_r(gradtype[0])) || (is_r(gradtype[1])) || (is_r(gradtype[2])))
#define anypfga ((is_p(gradtype[0])) || (is_p(gradtype[1])) || (is_p(gradtype[2])))
#define anypfgw ((is_q(gradtype[0])) || (is_q(gradtype[1])) || (is_q(gradtype[2])))
#define anypfg3 ((is_c(gradtype[0])) || (is_c(gradtype[1])) || (is_c(gradtype[2])) || (is_t(gradtype[0])) || (is_t(gradtype[1])) || (is_t(gradtype[2])))
#define anypfg3w ((is_d(gradtype[0])) || (is_d(gradtype[1])) || (is_d(gradtype[2])) || (is_u(gradtype[0])) || (is_u(gradtype[1])) || (is_u(gradtype[2])))
#define anypfg	(anypfga || anypfgw || anypfg3 || anypfg3w)
#define anywg  (anyrfwg || anygradwg || anygradcrwg || anypfgw || anypfg3w)

/* house keeping delay between CTs */
#define INOVAHKDELAY 	0.00201
#define HKDELAY 	0.0060
#define OLDHKDELAY 	0.0130
/* Wideline delay for software STM funcstion  16ms/K data */
#define WLDELAY 	0.0160

#define PATERR	-1	/* now sisco usage, greg */

#ifndef VIS_ACQ
/* (2300-7fff)/2 (11903 dec), minus 1024(dec)/2 = Max size of Acodes */
/*#define VM02_SIZE 	11391*/  /* not quite so fudge factor to 11000 */
#define VM02_SIZE 	11000
#else
/* (2300-143ff)/2 (36992 dec), minus 1024(dec)/2 = Max size of Acodes */
#define VM02_SIZE 	36480
#endif
/* PSG uses codeint (short int) to keep track of locations within
 * an acode.  For example, nsc_ptr and multhwlp_ptr. Therefore, if
 * acode length exceeds 16 bit limit, trouble.
 */
#define MV162_SIZE 	32767

extern char *getwd();
extern char *newString();
extern char *realString();
extern symbol **getTreeRoot();
extern varInfo *rfindVar();
extern char *getenv();
extern int createPS();		/* create Pulse Sequence routine */
extern double getval();
extern double sign_add();
extern codeint *init_acodes();

static void checkGradAlt();
void checkGradtype();


int bgflag = 0;
int debug  = 0;
int debug2 = 0;
int bugmask = 0;
int lockfid_mode = 0;
int clr_at_blksize_mode = 0;
int initializeSeq = 1;

int debugFlag = 0;	/* do NOT set this flag based on a command line argument !! */

int specialGradtype = 0;

static int pipe1[2];
static int pipe2[2];
static int ready = 0;

/**************************************
*  Structure for real-time AP tables  *
*  and global variables declarations  *
**************************************/

codeint  	t1, t2, t3, t4, t5, t6, 
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
   codeint	indexptr;
   codeint	destptr;
};

typedef	struct _Tableinfo	Tableinfo;

Tableinfo	*Table[MAXTABLE];


/**********************************************
*  End table structures and global variables  *
**********************************************/


char       **cnames;	/* pointer array to variable names */
codeint     *preCodes;	/* pointer to the start of the malloced space */
codeint     *Codes;	/* pointer to the start of the Acodes array */
long	     Codesize;	/* size of the malloc space for codes */
long	     CodeEnd;	/* End Address  of the malloc space for codes */
codeint     *Codeptr; 	/* pointer into the Acode array */
int          nnames;	/* number of variable names */
int          ntotal;	/* total number of variable names */
int          ncvals;	/* NUMBER OF VARIABLE  values */
codelong     codestart;	/* Beginning offset of a PS lc,auto struct & code */
codelong     startofAcode;	/* Beginning offset of actual Acodes */
double     **cvals;		/* pointer array to variable values */

char rftype[MAXSTR];	/* type of rf system used for trans & decoupler */
char amptype[MAXSTR];	/* type of amplifiers used for trans & decoupler */
char rfwg[MAXSTR];	/* y/n for rf waveform generators */
char gradtype[MAXSTR];	/* char keys w - waveform generation s-sisco n-none */
char rfband[MAXSTR];	/* RF band of trans & dec  (high or low) */
double  cattn[MAX_RFCHAN_NUM+1]; /* indicates coarse attenuators for channels */
double  fattn[MAX_RFCHAN_NUM+1]; /* indicates fine attenuators for channels */

char dqd[MAXSTR];	/* Digital Quadrature Detection, y or n */

/* --- global flags --- */
int  newacq = 0;	/* temporary nessie flag */
int  acqiflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  checkflag = 0;	/* if 'check' was an argument, then check sequnece but no acquisition */
int  nomessageflag = 0;	/* if 'nomessage' was an argument, then suppress text and warning messages */
int  waitflag = 0;	/* if 'acqi' was an argument, then interactive output */
int  prepScan = 0;	/* if 'prep' was an argument, then wait for sethw to start */
int  fidscanflag = 0;	/* if 'fidscan' was an argument, then use fidscan mode for vnmrj */
int  tuneflag = 0;	/* if 'tune' or 'qtune' was an argument, then use tune mode for vnmrj */
int  ok;		/* global error flag */
int  automated;		/* True if system is an automated one */
int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
int  ptsval[MAX_RFCHAN_NUM+1];	/* PTS type for trans & decoupler */
int  rcvroff_flag = 0;	/* receiver flag; initialized to ON */
int  rcvr_hs_bit;	/* HS line that switches rcvr on/off, 0x8000 or 0x1 */
int  rfp270_hs_bit;
int  dc270_hs_bit;
int  dc2_270_hs_bit;
int  homospoil_bit;
int  spare12_hs_bits=0;
int  ap_ovrride = 0;	/* ap delay override flag; initialized to FALSE */
int  phtable_flag = 0;	/* global flag for phasetables */
int  newdec;		/* True if system uses the direct synthesis for dec */
int  newtrans; 		/* True if system uses the direct synthesis for trans */
int  newtransamp;       /* True if system uses class A amplifiers for trans */
int  newdecamp;         /* True if system uses class A amplifiers for dec */
int  vttype;		/* VT type 0=none,1=varian,2=oxford */
int  SkipHSlineTest = 0;/* Skip or not skip the HSline test agianst presHSline */
			/* need to skip this test during pulse sequence since the */
			/* usage of 'ifzero(v1)' constructs would cause the test */
			/* to fail.	*/

/* acquisition */
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
double  fb;		/* Filter Bandwidth */
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
double  fb;		/* filter bandwidth */
int     cpflag;  	/* phase cycling flag  1=none,  0=quad detection */
int     dhpflag;	/* decoupler High Power Flag */

    /* --- pulse widths --- */
double  pw; 		/* pulse width */
double  p1; 		/* A pulse width */
double  pw90;		/* 90 degree pulse width */
double  hst;    	/* time homospoil is active */

/* --- delays --- */
double  alfa; 		/* Time after rec is turned on that acqbegins */
double  beta; 		/* audio filter time constant */
double  d1; 		/* delay */
double  d2; 		/* A delay, used in 2D/3D/4D experiments */
double  d2_init = 0.0;  /* Initial value of d2 delay, used in 2D/3D/4D experiments */
double  inc2D;		/* t1 dwell time in a 2D/3D/4D experiment */
double  d3;		/* t2 delay in a 3D/4D experiment */
double  d3_init = 0.0;  /* Initial value of d3 delay, used in 2D/3D/4D experiments */
double  inc3D;		/* t2 dwell time in a 3D/4D experiment */
double  d4;		/* t3 delay in a 4D experiment */
double  d4_init = 0.0;  /* Initial value of d4 delay, used in 2D/3D/4D experiments */
double  inc4D;		/* t3 dwell time in a 4D experiment */
double  pad; 		/* Pre-acquisition delay */
int     padactive; 	/* Pre-acquisition delay active parameter flag */
double  rof1; 		/* Time receiver is turned off before pulse */
double  rof2;		/* Time after pulse before receiver turned on */

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

int curfifocount;		/* current number of word in fifo */
int setupflag;			/* alias used to invoke PSG ,go=0,su=1,etc*/
int dps_flag;			/* dps flag */
int ra_flag;			/* ra flag */
unsigned long start_elem;       /* elem (FID) for acquisition to start on (RA)*/
unsigned long completed_elem;   /* total number of completed elements (FIDs)  (RA) */
int statusindx;			/* current status index */
int HSlines;			/* High Speed output board lines */
int presHSlines;		/* last output of High Speed output board lines */

/* --- Explicit acquisition parameters --- */
int hwlooping;			/* hardware looping inprogress flag */
int hwloopelements;		/* PSG elements in hardware loop */
int starthwfifocnt;		/* fifo count at start of hwloop */
int acqtriggers;			/*# of data acquires */
int noiseacquire;		/* noise acquiring flag */

/*- These values are used so that they are changed only when their values do */
int oldlkmode;			/* previous value of lockmode */
int oldspin;			/* previous value of spin */
int oldwhenshim;		/* previous value of shimming mask */
double oldvttemp;		/* previous value of vt tempature */
double oldpad;			/* previous value of pad */

/*- These values are used so that they are included in the Acode only when */
/*  the next value out of the arrayed values has been obtained */
int oldpadindex;		/* previous value of pad value index */
int newpadindex;		/* new value of pad value index */

/* --- Pulse Seq. globals --- */
double xmtrstep;		/* phase step size trans */
double decstep;			/* phase step size dec */
codeint idc = PSG_ACQ_REV;	/* PSG software rev number,(initacqparms)*/

codeint hwloop_ptr;		/* offset to hardware loop Acode */
codeint multhwlp_ptr;		/* offset to multiple hardware loop flag */
codeint nsc_ptr;		/* offset to NSC Acode */

unsigned long 	ix;			/* FID currently in Acode generation */
int 	nth2D;			/* 2D Element currently in Acode generation (VIS usage)*/
int     arrayelements;		/* number of array elements */
int     fifolpsize;		/* fifo loop size (63, 512, 1k, 2k, 4k) */
int     ap_interface;		/* ap bus interface type 1=500style, 2=amt style */ 
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
double  gradalt=1.0;   /* alternating gradients not implemented */


/************************************************************************/
/*	SIS Globals							*/
/************************************************************************/

/*------------------------------------------------------------------
    RF and Gradient pattern structures
------------------------------------------------------------------*/
typedef struct _RFpattern {
    double  phase;
    double  amp;
    double  time;
} RFpattern;

typedef struct _Gpattern {
    double  amp;
    double  time;
} Gpattern;


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
double  dpwr4;               /* Decoupler pulse power */
double  dpwr5;               /* Decoupler pulse power */


/*------------------------------------------------------------------
    GRADIENTS
------------------------------------------------------------------*/
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
char  ws[MAXSTR];            /* water suppression flag */
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
char  presig[MAXSTR];        /* PIC high/low gain setting */
double  gpropdelay;          /* Gradient propagation delay for grad_advance */
double  aqtm;                /*  */
char    volumercv[MAXSTR];   /* flag controls volume vs. surface coil receive */
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

/*  Objects   */
Object Ext_Trig  = NULL;	/* External Trigger Device */
Object RT_Delay  = NULL;	/* Real-Time Delay Event Device */
Object ToAttn    = NULL;	/* AMT Interface Obs attenuator */
Object DoAttn    = NULL;	/* AMT Interface Dec attenuator */
Object Do2Attn   = NULL;	/* AMT Interface Dec2 attenuator */
Object Do3Attn   = NULL;	/* AMT Interface Dec3 attenuator */
Object Do4Attn   = NULL;	/* AMT Interface Dec4 attenuator */
Object ToLnAttn  = NULL;	/* Solids Linear attenuator, Observe Channel */
Object DoLnAttn  = NULL;	/* Solids Linear attenuator, Dec Channel */
Object Do2LnAttn = NULL;	/* Linear attenuator, Dec2 Channel */
Object Do3LnAttn = NULL;	/* Linear attenuator, Dec3 Channel */
Object Do4LnAttn = NULL;	/* Linear attenuator, Dec4 Channel */
Object RF_Rout   = NULL;	/* RF Routing Relay Bank */
Object RF_Opts   = NULL;	/* RF Amp & etc. Options Bank */
Object RF_Opts2  = NULL;	/* RF Amp & etc. Options Bank */
Object RF_MLrout = NULL;	/* RF routing in Magnet Leg */
Object RF_PICrout = NULL;	/* RF routing in 4 channel type Magnet Leg (PIC) */
Object RF_TR_PA  = NULL;	/* RF T/r and PreAmp enables */
Object RF_Amp1_2 = NULL;	/* Amp 1/2 cw vs. pulse */
Object RF_Amp3_4 = NULL;	/* Amp 3/4 cw vs. pulse */
Object RF_Amp_Sol= NULL;	/* Amp solids cw vs. pulse */
Object RF_mixer  = NULL;	/* XMTRs hi/lo mixer select */
Object RF_hilo   = NULL;	/* attns hi/lo-band relays router */
Object RF_Mod    = NULL;	/* xmtrs modulation mode APbyte */
Object HS_select = NULL;	/* XMTRs HS lines select */
Object RCVR_homo = NULL;	/* RCVR homo decoupler bit */
Object X_gmode   = NULL;	/* XMTRs gate mode selection */
Object HS_Rotor  = NULL;	/* High Speed Rotor Device */
Object RFChan1   = NULL;	/* RF Channel Device */
Object RFChan2   = NULL;	/* RF Channel Device */
Object RFChan3   = NULL;	/* RF Channel Device */
Object RFChan4   = NULL;	/* RF Channel Device */
Object RFChan5   = NULL;	/* RF Channel Device */
Object BOBreg0   = NULL;	/* Apbus BreakOut Board reg 0 */
Object BOBreg1   = NULL;	/* Apbus BreakOut Board reg 1 */
Object BOBreg2   = NULL;	/* Apbus BreakOut Board reg 2 */
Object BOBreg3   = NULL;	/* Apbus BreakOut Board reg 3 */
Object APall	 = NULL;	/* General APbus register */

Object RF_Channel[MAX_RFCHAN_NUM+1];	/* index array of RF channel Objects */

/*  RF Channels */
int OBSch=1;			/* The Acting Observe Channel */
int DECch=2;  			/* The Acting Decoupler Channel */
int DEC2ch=3;  			/* The Acting 2nd Decoupler Channel */
int DEC3ch=4;  			/* The Acting 3rd Decoupler Channel */
int DEC4ch=5;  			/* The Acting 4th Decoupler Channel */
int NUMch=2;			/* Number of channels configured */


struct _ModInfo
{
     double	*MI_dmf;
     double	*MI_dres;
     int	*MI_dmsize;
     int	*MI_dmmsize;
     int	*MI_homosize;
     char	*MI_dm;
     char	*MI_dmm;
     char	*MI_dseq;
     char	*MI_homo;
};

struct _ModInfo	ModInfo[MAX_RFCHAN_NUM+1] = {
	{0,	0,	0,	   0,		0,
	 0,	0,	0,	   0,
        }, /* channel dummy */
        {&xmf,	&xres,	&xmsize,   &xmmsize,	&ready,
	 xm,	xmm,	xseq,	   "n",
	}, /* channel1 */
	{&dmf,	&dres,	&dmsize,   &dmmsize,	&homosize,
	 dm,	dmm,	dseq,	   homo,
	}, /* channel2 */
	{&dmf2,	&dres2,	&dm2size,  &dmm2size,	&homo2size,
	 dm2,	dmm2,	dseq2,	   homo2,
	}, /* channel3 */
	{&dmf3,	&dres3,	&dm3size,  &dmm3size,	&homo3size,
	 dm3,	dmm3,	dseq3,	   homo3,
	}, /* channel4 */
	{&dmf4,	&dres4,	&dm4size,  &dmm4size,	&homo4size,
	 dm4,	dmm4,	dseq4,	   homo4,
	}, /* channel5 */
	{0,	0,	0,	   0,		0,
	 0,	0,	0,	   0,
	} /* channel6 */
};
#ifdef DBXTOOL
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
struct _cfindex
{
     long NFids;
     long OffSets[10];
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

#ifdef AIX
void    (*dps_func)() = NULL;
void    (*dps_tfunc)() = NULL;

set_dps_func(proc, tproc)
void (*proc)();
void (*tproc)();
{
        dps_func = proc;
        dps_tfunc = tproc;
}
#endif


#ifndef SWTUNE

#ifdef AIX
psg_main(argc,argv) 	int argc; char *argv[];
#else
main(argc,argv) 	int argc; char *argv[];
#endif
{   
    char    array[MAXSTR];
    char    parsestring[MAXSTR];	/* string that is parsed */
    char    arrayStr[MAXSTR];           /* string that is not parsed */
    char    filename[MAXPATHL];		/* file name for Codes */
    char   *gnames[50];
    char   *appdirs;

    double arraydim;	/* number of fids (ie. set of acodes) */
    double maxlbsw;
    double ni = 0.0;
    double ni2 = 0.0;
    double ni3 = 0.0;

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
    sleep(70);		/* allow 70sec for person to attach with DBXTOOL */
#endif

    setupsignalhandler();  /* catch any exception that might core dump us. */

    if (argc < 7)  /* not enought args to start, exit */
    {	fprintf(stderr,
	"This is a background task! Only execute from within 'vnmr'!\n");
	exit(1);
    }
    Rev_Num = atoi(argv[1]); /* GO -> PSG Revision Number */
    pipe1[0] = atoi(argv[2]); /* convert file descriptors for pipe 1*/
    pipe1[1] = atoi(argv[3]);
    pipe2[0] = atoi(argv[4]); /* convert file descriptors for pipe 2*/
    pipe2[1] = atoi(argv[5]);
    setupflag = atoi(argv[6]);	/* alias flag */
    if (bgflag)
    {
       fprintf(stderr,"\n BackGround PSG job starting\n");
       fprintf(stderr,"setupflag = %d \n", setupflag);
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
    if (option_check("debug2"))
       debug2 = 1;
    if (option_check("sync"))
       waitflag = 1;
    if (option_check("prep"))
       prepScan = 1;
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
 
/* determine whether this is INOVA or older now, before possibly calling (psg)abort */

    getparm("acqaddr","string",GLOBAL,filename,MAXPATHL);
/**
    newacq = (!dps_flag && (strcmp(filename,"Expproc") == 0) );
    newacq = (strcmp(filename,"Expproc") == 0 );
**/
    newacq = is_newacq();

    /* ------- Check For GO - PSG Revision Clash ------- */
    if (Rev_Num != GO_PSG_REV )
    {	
	char msge[100];
	sprintf(msge,"GO(%d) and PSG(%d) Revision Clash, PSG Aborted.\n",
	  Rev_Num,GO_PSG_REV);
	text_error(msge);
	psg_abort(1);
    }
    if (P_rec_stat == -1 )
    {	
	text_error("P_receive had a fatal error.\n");
	psg_abort(1);
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
	      else if (argv[i][j+1] == 't')
                 tuneflag = 1;
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


#endif  /* TESTING */
    /*-----------------------------------------------------------------
    |  begin of PSG task
    +-----------------------------------------------------------------*/

    if (strncmp("dps", argv[0], 3) == 0)
    {
        dps_flag = 1;
        if ((int)strlen(argv[0]) >= 5)
           if (argv[0][3] == '_')
                dps_flag = 3;
    }

    init_power_check();
    /* null out optional file */
    fileRFpattern[0] = 0;	/* path for Obs & Dec RF pattern file */
    filegrad[0] = 0;		/* path for Gradient file */
    filexpan[0] = 0;		/* path for Future Expansion file */

    A_getstring(CURRENT,"appdirs", &appdirs, 1);
    setAppdirValue(appdirs);
    setup_comm();   /* setup communications with acquisition processes */

    getparm("goid","string",CURRENT,filename,MAXPATHL);

    if (acqiflag)
    {
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
	P_setstring(CURRENT,"ss","n",1);
	P_setstring(CURRENT,"dp","y",1);
	if (newacq)				   /* set nt=1 for go('acqi') */
	  P_setreal(CURRENT,"nt",1.0,1);	/* on the new digital console */
    }
    if (fidscanflag)
    {
        double fs_val;
        P_setstring(CURRENT,"alock","n",1);
        P_setstring(CURRENT,"load","n",1);
        P_setstring(CURRENT,"wshim","n",1);
	P_setstring(CURRENT,"ss","n",1);
	P_setstring(CURRENT,"dp","y",1);
	P_setstring(CURRENT,"cp","n",1);
	P_setreal(CURRENT,"bs",1.0,1);
	P_setstring(CURRENT,"wdone","",1);
	if (newacq)				   /* set nt=1e6 for go('fidscan') */
	  P_setreal(CURRENT,"nt",1e6,1);	/* on the new digital console */
	if (P_setreal(CURRENT,"lockacqtc",1.0,1))
        {
           P_creatvar(CURRENT, "lockacqtc", T_REAL);
	   P_setreal(CURRENT,"lockacqtc",1.0,1);
        }
        if ( (P_getreal(CURRENT,"fidshimnt",&fs_val,1) == 0) &&
             (P_getactive(CURRENT,"fidshimnt") == 1) &&
             ( (int) (fs_val+0.1) >= 1) &&
             ( (int) (fs_val+0.1) <= 16) )
        {
             double val = (double) ( (int) (fs_val+0.1) );
             P_setreal(CURRENT,"nt",val,1);
             P_setreal(CURRENT,"bs",val,1);
             P_setreal(CURRENT,"pad", 0.0, 1);
             putCmd("wnt='fid_display' wexp='fid_scan(1)'\n");
        }
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

    if (!dps_flag)
       if (setup_parfile(setupflag))
       {   text_error("Parameter file error PSG Aborted..\n");
	   psg_abort(1);
       }
        
    strcpy(filepath,filename);	/* /vnmrsystem/acqqueue/exp1.greg.012345 */
    strcpy(filexpath,filepath); /* save this path for psg_abort() */
    strcat(filepath,".Code");	/* /vnmrsystem/acqqueue/exp1.greg.012345.Code */

    if (acqiflag)
    {
        unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".IPA");
	unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".Tables");
	unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".RTpars");
	unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".RF");
	unlink(filepath);
	strcpy(filepath,filexpath);
	strcat(filepath,".Code.lock");
	unlink(filepath);
        umask(0);
    }

    if (!dps_flag && !checkflag && !option_check("checkarray"))
       init_codefile(filepath);

    if (ra_flag && !newacq)   /* if ra then open acqpar parameter file */
    {
	char acqparpath[MAXSTR];/* file path to acqpar for ra */
       /* --- set acqpar file path for ra --- */
       if (getparm("exppath","string",CURRENT,acqparpath,MAXSTR))
    	return(ERROR);
       strcat(acqparpath,"/acqpar");
       if (bgflag)
         fprintf(stderr,"get acqpar path  = %s\n",acqparpath);
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
    /* NB: parameter acqcycles now gives number of acode sets;
     * "arraydim" parameter is number of data blocks. */
    if (getparm("acqcycles","real",CURRENT,&arraydim,1))
	psg_abort(1);

    if (acqiflag || checkflag || (setupflag > 0))  /* if not a GO force to 1D */
        arraydim = 1.0;		/* any setup is 1D, nomatter what. */

/*----------------------------------------------------------------
| --- malloc space for Acodes ---
|  1. CodeIndex - a. arraydim (number of Acode sets)
|		- b. arraydim pairs of code offsets (in bytes)
|		     and compression keys
|		- c. point to the end of all acodes...
|
|  2. Codes	- a. Low Core	          pointer = Alc;
|		- b. Auto Structure       pointer = Aauto;
|		- c. Acodes		  pointer = Aacode;
+------------------------------------------------------------------*/
    if (newacq)
    {
       Codesize =  (long)  (MV162_SIZE * sizeof(codeint));     /* code + struct */
    }
    else
    {
       Codesize =  (long)  (VM02_SIZE * sizeof(codeint));     /* code + struct */
    }

    if (bgflag)
    {
	fprintf(stderr,"arraydim = %5.0lf \n",arraydim);
        fprintf(stderr,"sizeof Codes: %ld(dec) bytes \n", Codesize);
    }

    preCodes = (codeint *) malloc( Codesize + 2 * sizeof(codelong) );
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

    /* Set up Acode pointers */
    Codeptr = init_acodes(Codes);

    /* --- set parameter to unused value, so new & old will not be 
       --- the same 1st time --- */
    oldlkmode = -1;			/* previous value of lockmode */
    oldspin = -1;			/* previous value of spin */
    oldwhenshim = -1;			/* previous value of shimming mask */
    oldvttemp = 29999.0;		/* previous value of vt tempature */
    oldpad = 29999.0;			/* previous value of pad */
    oldpadindex = -1;			/* previous value index of PAD */
    newpadindex = 0;			/* new value index of PAD */
    exptime  = 0.0; 	   /* total timer events for a exp duration estimate */
    start_elem = 1;             /* elem (FID) for acquisition to start on (RA)*/
    completed_elem = 0;         /* total number of completed elements (FIDs)  (RA) */

#ifndef NO_ACQ
    clearHSlines();		/* set line to safe zero states */
#endif
    /* --- setup global flags and variables --- */
    if (setGflags()) psg_abort(0);

    /* --- initialize various event overhead times --- */
    if (newacq)
       initeventovrhd(INOVA);		/* 0.5 usec AP */
    else if (ap_interface == 4)
       initeventovrhd(UNITYPLUS);	/* 1 usec AP */
    else if (fifolpsize > 100)
       initeventovrhd(UNITY);		/* 2 usec AP */
    else
       initeventovrhd(VXR);		/* 2 usec w/o HS on AP */

    /* --- setup objects --- */
#ifndef MACOS
    if (initObjects()) psg_abort(0);
#endif

    /* --- setup User global flags and variables --- */
    if (init_user_globals()) psg_abort(0);

    /* --- set Transmitter Amp and Filter setting variable 'filter' --- */
    if (setPulseFilter()) psg_abort(0);	/* (old style RF) */

    /* --- set up global parameter --- */
    initparms();
    /* --- set up lc data sturcture  --- */
    initacqparms(1);
    /* --- set up automation data sturcture  --- */
#ifdef NO_ACQ
    init_shimnames(GLOBAL);
#else
    initautostruct();
#endif
    initialRF();		/* initial RF AP words for initial states */
    initSSHA();
/*    initSSHAshimmethod(); */ /* here instead of cps.c?? */

    if ((cattn[TODEV] == SIS_UNITY_ATTN_MAX) && (newtrans == FALSE))
    {
	/* --- set up RF pattern file --- */
    	if (initshapedpatterns(1) == PATERR)
	   psg_abort(1);
    }

    /* you can't send both as of 3/90 */
    /*  if any WFG */
    if (!dps_flag && anywg)
    {
      init_wg_list();
    }
    /* at this time, gradwg means 'w' and PFG code below handles
       the pfg ecc stuff... they will be merged */
    if (!dps_flag && have_imaging_sw())	/*** systems w/ added imaging sw    ***/
    {
       init_global_list(ACQ_XPAN_GSEG);
    } 
    setupPsgFile();
    /* ---- A single or arrayed experiment ???? --- */
    ix = nth2D = 0;		/* initialize ix */
    init_compress((int) arraydim);

    if (checkflag || (dps_flag > 1))
        arraydim = 1.0;
    if (option_check("checkarray"))
    {
       checkflag = 1;
    }

    if (arraydim <= 1.5)
    {
        totaltime  = 0.0; /* total timer events for a fid */
	ix = nth2D = 1;		/* generating Acodes for FID 1 */
	if (dps_flag)
        {
              createDPS(argv[0], curexp, arraydim, 0, NULL, pipe2[1]);
              close_error(0);
              exit(0);
        }

        pre_expsequence();      /* user special function once per Exp */
	createPS();
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

    	if (getparm("array","string",CURRENT,array,MAXSTR))
	    psg_abort(1);
    	if (bgflag)
	    fprintf(stderr,"array: '%s' \n",array);
	strcpy(parsestring,array);
        /*----------------------------------------------------------------
        |	test for presence of ni, ni2, and ni3
	|	generate the appropriate 2D/3D/4D experiment
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
    	if (parse(parsestring,&narrays))  /* parse 'array' setup looping elements */
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

        pre_expsequence();      /* user special function once per Exp */
    	arrayPS(0,narrays);
    }
    /*  -1 is for the fact that Codeoffset in the next free spot */
    if (bgflag > 2)
    {
        int num;
        num =  Codeptr - Codes;/* offset within Code File of next */
        for (i=0; i< (int) num; i++)
	     fprintf(stderr,"0x%lx-Codes[%d]: %d or 0x%x \n",
	        &Codes[i],i,Codes[i],Codes[i]);
    }
    if (checkflag)
    {
       if (!option_check("checksilent"))
          text_message("go check complete\n");
       closeCmd();
       close_error(0);
       exit(0);
    }

    close_codefile();

    /* TEST FOR ANY WFG */
    if (anywg) 
    {
      wg_close();
    }
    close_obl_list();		/* remove any oblique shaped info */

    if ((cattn[TODEV] == SIS_UNITY_ATTN_MAX) && (newtrans == FALSE))
    {
    	/* ---- Finish pattern cleanup ---- */
    	if (bgflag)
           fprintf(stderr,"calling p_endarrayelem()\n");
    	p_endarrayelem();           /* move this to end of each element for
                                   arrayed pattern implementation */
    	if (bgflag)
           fprintf(stderr,"calling closeshapedpatterns()\n");
    	if(closeshapedpatterns(1) == PATERR) /* free pattern link list & file */
	   psg_abort(1);
    }

    /* close file for freq_list and delay list */
    if (have_imaging_sw())	/*** systems w/ added imaging sw    ***/
    {
    	close_global_list();
    }

    if (newacq)
	exptime += ( INOVAHKDELAY * getval("nt")) * arraydim;
    else
	exptime += ( ((fifolpsize < 2000) ? OLDHKDELAY : HKDELAY) * 
						getval("nt")) * arraydim;
    if (var_active("ss",CURRENT))
    {
       int ss = (int) (sign_add(getval("ss"), 0.0005));
       double hkdelay = (fifolpsize < 2000) ? OLDHKDELAY : HKDELAY;

       if (newacq) hkdelay = INOVAHKDELAY;

       if (ss < 0)
          exptime += (hkdelay * (double) -ss) * arraydim;
       else
          exptime += (hkdelay * (double) ss);
    }
    if (P_getreal(GLOBAL, "maxsw_loband", &maxlbsw, 1) < 0){
	maxlbsw = 100000.0;
    }
    if ( (getval("sw") > maxlbsw) && ! newacq)
      exptime += ((WLDELAY * (getval("np")/1024.0)) * getval("nt")) * arraydim;
    if (bgflag)
    {
      fprintf(stderr,"total time of experiment = %lf (sec) \n",exptime);
      fprintf(stderr,"Submit experiment to acq. process \n");
    }
    write_shr_info(exptime);
    if (!acqiflag && QueueExp(filexpath,nextflag))
      psg_abort(1);
    if (bgflag)
      fprintf(stderr,"PSG terminating normally\n");
    close_error(0);
    exit(0);
}

#else /* SWTUNE*/

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
    int     ngnames;
    double arraydim;	/* number of fids (ie. set of acodes) */
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
	text_error("P_receive had a fatal error.\n");
	psg_abort(1);
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
    if (newacq)
    {
       Codesize =  (long)  (MV162_SIZE * sizeof(codeint));   /* code + struct */
    }
    else
    {
       Codesize =  (long)  (VM02_SIZE * sizeof(codeint));    /* code + struct */
    }

    if (bgflag)
    {
	fprintf(stderr,"arraydim = %5.0lf \n",arraydim);
        fprintf(stderr,"sizeof Codes: %ld(dec) bytes \n", Codesize);
    }

    preCodes = (codeint *) malloc( Codesize + 2 * sizeof(codelong) );
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

    /* Set up Acode pointers */
    Codeptr = init_acodes(Codes);

    if (bgflag)
    {	fprintf(stderr,"Code address:  0x%lx \n",Codes);
     	fprintf(stderr,"Codeptr address:  0x%lx \n",Codeptr);
     	fprintf(stderr,"CodeEnd address:  0x%lx \n",CodeEnd);
    }

    /* --- setup global flags and variables --- */
    if (setGflags()) psg_abort(0);
    HSlines = 0;	/* Clear all High Speed line bits */
    presHSlines = 0;		/* Clear present High Speed line bits */

    /* --- setup objects --- */
    if (initObjects()) psg_abort(0);

    /* --- setup User global flags and variables --- */
    if (init_user_globals()) psg_abort(0);

    /* --- set Transmitter Amp and Filter setting variable 'filter' --- */
    if (setPulseFilter()) psg_abort(0);	/* (old style RF) */

    /* --- set up global parameter --- */
    initparms();

    /* --- set up lc data sturcture  --- */
    initacqparms(1);

    /* --- set up automation data sturcture  --- */
#ifndef NO_ACQ
    initautostruct();
#endif

    initialRF();		/* initial RF AP words for initial states */

/* The UnituPLUS version calls init_global_list each time it computes
   a new set of frequencies.  See tune_uplus.c, SCCS categoty tune.	*/

    if (newacq)
      init_global_list(ACQ_XPAN_GSEG);
    setupPsgFile();
}

void pulsesequence()
{
}

sendExpproc()
{
}

#endif /* SWTUNE */
/*--------------------------------------------------------------------------
|   This writes to the pipe that go is waiting on to decide when the first
|   element is done.
+--------------------------------------------------------------------------*/
first_done()
{
   write_exp_info();
   closeCmd();
   write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done with first element */
}
/*--------------------------------------------------------------------------
|   This closes the pipe that go is waiting on when it is in automation mode.
|   This procedure is called from close_error.  Close_error is called either
|   when PSG completes successfully or when PSG calls psg_abort.
+--------------------------------------------------------------------------*/
closepipe2(int success)
{
    char autopar[12];


    if (acqiflag || waitflag)
       write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done */
    else if (!dps_flag && (getparm("auto","string",GLOBAL,autopar,12) == 0))
        if ((autopar[0] == 'y') || (autopar[0] == 'Y'))
            write(pipe2[1],&ready,sizeof(int)); /* tell go, PSG is done */
    close(pipe2[1]); /* close write end of pipe 2*/
}
/*-----------------------------------------------------------------------
+------------------------------------------------------------------------*/
reset()
{
    if (cnames) free(cnames);
    if (cvals) free(cvals);
    if (preCodes) free(preCodes);
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
setGflags()
{
    extern Object ObjectNew();
    extern int    Attn_Device(),APBit_Device(),Event_Device();
    extern int    Freq_Device();
    char  mess[MAXSTR];
    double tmpval;
    int 	i;

    if ( P_getreal(GLOBAL,"numrfch",&tmpval,1) >= 0 )
    {
        NUMch = (int) (tmpval + 0.0005);
	if (( NUMch < 1) || (NUMch > MAX_RFCHAN_NUM))
	{
            sprintf(mess,
	     "Number of RF Channels specified '%d' is too large.. PSG Aborted.\n",
	      NUMch);
            text_error(mess);
	    psg_abort(1);
	}
        /* Fool PSG if numrfch is 1 */
	if ( NUMch < 2)
          NUMch = 2;
    }
    else
       NUMch = 2;
    /* --- determine type of ap_interface board that is present ---	*/
    /* type 1 is 500 style attenuators,etc			   	*/
    /* type 2 is ap interface introduced with the AMT amplifiers	*/
    /* type 3 is 'type 2' with the additional fine linear attenuators	*/
    /*        for the solids application. 				*/
    /* type 4 introduces the rf introduced in 1991/1992			*/
    /*------------------------------------------------------------------*/
    if ( P_getreal(GLOBAL,"apinterface",&tmpval,1) >= 0 )
    {
        ap_interface = (int) (tmpval + 0.0005);
	if (( ap_interface < 1) || (ap_interface > 3))
	{
            sprintf(mess,
		"AP interface specified '%d' is not valid.. PSG Aborted.\n",
	      ap_interface);
            text_error(mess);
	    psg_abort(1);
	}
    }
    else
	ap_interface = 1;	/* if not found assume ap_interface type 1 */

    /*--- obtain coarse & fine attenuator RF channel configuration ---*/
    cattn[0] = fattn[0] = 0.0;  /* dont use 0 index */
    for(i=1; i<=MAX_RFCHAN_NUM; i++)
    {
      /*--- obtain coarse & fine attenuator RF channel configuration ---*/
      if ( P_getreal(GLOBAL,"cattn",&cattn[i],i) < 0 )
      {
          cattn[i] = 0.0;                /* if not found assume 0 */
      }
      if ( P_getreal(GLOBAL,"fattn",&fattn[i],i) < 0 )
      {
          fattn[i] = 0.0;                /* if not found assume 0 */
      }

      /*--- obtain PTS values for channel configuration ---*/
      if ( P_getreal(GLOBAL,"ptsval",&tmpval,i) >= 0 )
        ptsval[i-1] = (int) (tmpval + .0005);
      else
	ptsval[i-1] = 0;		/* no pts for decoupler */
    }

    /* --- determine output board fifo size --- */
    if ( P_getreal(GLOBAL,"fifolpsize",&tmpval,1) >= 0 )
    {
        fifolpsize = (int) (tmpval + 0.0005);
	if (( fifolpsize != 63) && (fifolpsize != 512) &&  
	    ( fifolpsize != 1024) && (fifolpsize != 2048)  &&
	    ( fifolpsize != 4096) && (fifolpsize != 8192) )
	{
            sprintf(mess,"Fifo size %d is not valid.. PSG Aborted.\n",fifolpsize);
            text_error(mess);
	    psg_abort(1);
	}
    }
    else
	fifolpsize = 63;		/* if not found assume 63 fifo size */

    /* --- determine if Rotor sync Board is Present --- */
    if ( P_getreal(GLOBAL,"rotorsync",&tmpval,1) >= 0 )
    {
        rotorSync = (int) (tmpval + 0.0005);
	if (( rotorSync != 0) && (rotorSync != 1) )
	{
            sprintf(mess,
	      "Rotor Sync interface specified '%d' is not valid.. PSG Aborted.\n",
	      ap_interface);
            text_error(mess);
	    psg_abort(1);
	}
        if ( (rotorSync == 1) && (fifolpsize < 1024) )
	{
            sprintf(mess,
	      "Wrong Fifo Loop Size to have Rotor Sync interface.. PSG Aborted.\n"
	       );
            text_error(mess);
	    psg_abort(1);
        }
    }
    else
    {
	rotorSync = 0;
    }

    /* --- set rf and amplifier types for decoupler and transmitter --- */
    if (getparm("rftype","string",GLOBAL,rftype,15))
    	return(ERROR);
    rcvr_hs_bit    = RXOFF;	/* default to old style */
    rfp270_hs_bit  = RFP270;
    dc270_hs_bit   = DC270;
    dc2_270_hs_bit = DC2_270;
    homospoil_bit  = HomoSpoilON;
    
    if (rftype[0] == 'd'  || rftype[0] == 'D')
    {
       ap_interface = 4;
       rcvr_hs_bit    = 0x00000001;	/* default to old style */
       rfp270_hs_bit  = 0x00000018;
       dc270_hs_bit   = 0x00000300;
       dc2_270_hs_bit = 0x00006000;
       homospoil_bit  = 0x40000000;
      
    }
    
    if (getparm("amptype","string",GLOBAL,amptype,15))
    	return(ERROR);
    if (getparm("rfband","string",CURRENT,rfband,15))
    	return(ERROR);

    /* --- discover waveformers and gradients --- */

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

    /* --- end discover waveformers and gradients --- */

    if (rftype[0] == 'c' || rftype[0] == 'C' ||
        rftype[0] == 'd'  || rftype[0] == 'D')
    {
	newtrans = TRUE;
    }
    else
    {
	newtrans = FALSE;
    }

    if (rftype[1] == '\0')
    {
       rftype[1] = rftype[0];
       rftype[2] = '\0';
    }
    if (rftype[1] == 'c'  || rftype[1] == 'C' ||
        rftype[1] == 'd'  || rftype[1] == 'D')
    {
	newdec = TRUE;
    }
    else
    {
	newdec = FALSE;
    }

    if (amptype[0] == 'c'  || amptype[0] == 'C')
    {
	newtransamp = FALSE;
    }
    else
    {
	newtransamp = TRUE;
    }

    if (amptype[1] == '\0')
    {
       amptype[1] = amptype[0];
       amptype[2] = '\0';
    }
    if (amptype[1] == 'c'  || amptype[1] == 'C')
    {
	newdecamp = FALSE;
    }
    else
    {
	newdecamp = TRUE;
    }
    automated = TRUE;

    /* ---- set basic instrument proton frequency --- */
    if (getparm("h1freq","real",GLOBAL,&tmpval,1))
    	return(ERROR);
    H1freq = (int) tmpval;

/*****************************************
*  Load userdir, systemdir, and curexp.  *
*****************************************/

    if (P_getstring(GLOBAL, "userdir", userdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current user directory\n");
    if (P_getstring(GLOBAL, "systemdir", systemdir, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current system directory\n");
    if (P_getstring(GLOBAL, "curexp", curexp, 1, MAXPATHL-1) < 0)
       text_error("PSG unable to locate current experiment directory\n");
    sprintf(abortfile,"%s/acqqueue/psg_abort", systemdir);
    if (access( abortfile, W_OK ) == 0)
       unlink(abortfile);


    /* --- set VT type on instrument --- */
    if (getparm("vttype","real",GLOBAL,&tmpval,1))
    	return(ERROR);
    vttype = (int) tmpval;

    /* --- set lock correction factor --- */
    /* now calc in freq object */

    /* --- set lockfreq flag, active or not active --- */
    /*   not used any more  */

    return(OK);
}
/*------------------------------------------------------------------------------
|
|	setPulseFilter()
|
|	This function sets the Pulse Amp  power and filter variable 'filter'
|	to the proper value depending of 'sw' and 'H1freq' 
+----------------------------------------------------------------------------*/
setPulseFilter()
{
    double sfrq;
    double tmpval;
    double pulse;
    double filter;
    double Amptable[3][4];
    double Filtertab[3][3];
    int h1index;

    Amptable[0][0] = 61.0; Amptable[1][0] = 120.0; Amptable[2][0] = 160.0;
    Amptable[0][1] = 45.0; Amptable[1][1] = 75.0; Amptable[2][1] = 100.0;
    Amptable[0][2] = 24.0; Amptable[1][2] = 50.0; Amptable[2][2] = 67.0;
    Amptable[0][3] = 16.0; Amptable[1][3] = 20.0; Amptable[2][3] = 27.0;

    Filtertab[0][0] = 50.0; Filtertab[1][0] = 81.0; Filtertab[2][0] = 108.0;
    Filtertab[0][1] = 32.5; Filtertab[1][1] = 47.0; Filtertab[2][1] = 63.0;
    Filtertab[0][2] = 19.0; Filtertab[1][2] = 31.0; Filtertab[2][2] = 41.0;
    if (getparm("sfrq","real",CURRENT,&sfrq,1))
    	return(ERROR);
    if (getparm("h1freq","real",GLOBAL,&tmpval,1))
    	return(ERROR);

    /* --- 200 = index of 0, 300 = index of 1, 400 = index of 2  --- */
    h1index = ( ((int) (tmpval + 0.5)) / 100 ) - 2;
    if (h1index > 2)
      h1index = 2;
    if (bgflag)
	fprintf(stderr,"setPulseFilter():  h1freq = %5.0lf, h1index = %d \n",
	      tmpval,h1index);

    /* --- determine correct Amp setting --- */
    if (sfrq > Amptable[h1index][0])
	pulse = 50;
    else
    {
        if (sfrq > Amptable[h1index][1])
	    pulse = 40;
        else
        {
            if (sfrq > Amptable[h1index][2])
	        pulse = 30;
            else
            {
                if (sfrq > Amptable[h1index][3])
		    pulse = 20;
		else
		    pulse = 10;
            }
        }
    }

    /* --- determine correct Filter setting --- */
    if (sfrq > Filtertab[h1index][0])
	filter = 4;
    else
    {
        if (sfrq > Filtertab[h1index][1])
	    filter = 3;
        else
        {
            if (sfrq > Filtertab[h1index][2])
	        filter = 2;
            else
		filter = 1;
        }
    }
    filter = pulse + filter;
    if (bgflag)
	fprintf(stderr,"filter = %5.1lf \n",filter);
    if (setparm("filter","real",CURRENT,&filter,1))
    	return(ERROR);

    return(OK);
}
/*------------------------------------------------------------------------------
|
|	setup2D(name,varible)
|
|	This function arrays the d2 variable with the correct values for
|       the 2D experiment 
+----------------------------------------------------------------------------*/
setup2D(ni)
double ni;
{
    double   sw1;
    double   d2;
    int	     index;
    int	     i;
    int	     num;
    if (getparm("sw1","real",CURRENT,&sw1,1))
        return(ERROR);
    if (getparm("d2","real",CURRENT,&d2,1))
        return(ERROR);
    inc2D = 1.0 / sw1;
    if (bgflag)
	fprintf(stderr,
	   "setup2d(): ni=%5.0lf, d2=%5.3lf, sw1=%8.1lf, 1/sw1=%11.6lf \n",
	   ni,d2,sw1,inc2D);
    num = (int) (ni - 1.0);
    for (i=0,index=2; i < num; i++, index++)
    {
	d2 = d2 + inc2D;
        if (bgflag)
	    fprintf(stderr,"setup2D(): index=%d, d2=%11.6lf \n",
	      index,d2);
        if ((P_setreal(CURRENT,"d2",d2,index)) == ERROR)
        {   text_error("could not set d2 values"); 
	    return(ERROR);
        }
    }
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
|	This function arrays the d4, d3 and d2 variables with the correct
|	values for the 2D/3D/4D experiment.  d2 increments first, then d3,
|	and then d4.
|
+-------------------------------------------------------------------------*/
setup4D(ni3, ni2, ni, parsestr, arraystr)
char	*parsestr,
	*arraystr;
double	ni3,
	ni2,
	ni;
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
      if (getparm("sw3", "real", CURRENT, &sw3, 1))
         return(ERROR);
      if (getparm("d4","real",CURRENT,&d4_init,1))
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
      if (getparm("sw2", "real", CURRENT, &sw2, 1))
           return(ERROR);
      if (getparm("d3","real",CURRENT,&d3_init,1))
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
static void queue_psg(char *auto_dir, char *fid_name, char *msg)
{
  FILE *outfile;
  FILE *samplefile;
  SAMPLE_INFO sampleinfo;
  char tfilepath[MAXSTR*2];
/*  char datapath[MAX_TEXT_LEN]; */
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
         /* sprintf(datapath,"%s/%s",auto_dir,fid_name); */
         /* strncpy(sampleinfo.prompt_entry[entryindex].etext,datapath,MAX_TEXT_LEN); */
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
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.     1. Changed QueueExp() to pass the additional information:
|			     start_elem and complete_elem to Acqproc 
|
|   5/1998    Robert L.   include the "setup flag" in the information string
|                         sent to Expproc, so the nature of the experiment
|			  may be derived from the experiment queue.  See
|			  acqhwcmd.c, SCCS category vnmr.
+----------------------------------------------------------------------------*/
#define QUEUE 1
QueueExp(codefile,nextflag)
char *codefile;
int   nextflag;
{
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
    if (getparm("priority","real",CURRENT,&priority,1))
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
           nanosleep(&timer, NULL);
           ex++;
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
        sync();
    }
    else if (!newacq)
    {
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

       if ( getCSnum() )
       {
          char *csname = getCSname();

          sprintf(commnd,"cp %s/%s %s/acqfil/%s",curexp,csname,curexp,csname);
          system(commnd);
       }

       P_getstring(CURRENT,"goid",tmpstr,3,255);
       P_getstring(CURRENT,"goid",tmpstr2,2,255);
       sprintf(infostr,"%s %s %d",tmpstr,tmpstr2,setupflag);
       if (sendExpproc(addr,codefile,infostr,nextflag))
       {
        text_error("Experiment unable to be sent\n");
        return(ERROR);
       }
    }
    return(OK);
}

int check_for_abort()
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
   int oplen;
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

/*----------------------------------------------------------------------*/
/* validate_imaging_config(callname)					*/
/*	Routine checks for valid imaging configuration.  Aborts if 	*/
/*	invalid. Uses one argument which is the name of the calling	*/
/*	routine.							*/
/*----------------------------------------------------------------------*/
validate_imaging_config(callname)
char *callname;
{
    char msg[128];
    if (!have_imaging_sw()){
    	/* Systems w/o imaging software */
	sprintf(msg,"Error: %s not supported for system type.",callname);
	text_error(msg);
	text_error("       Select Imaging Spectrometer in config menu.");
	psg_abort(1);
    }
}

/*----------------------------------------------------------------------*/
/*	Routine checks for valid imaging configuration. 		*/
/*	Returns TRUE if it is, otherwise FALSE.				*/
/*----------------------------------------------------------------------*/
int
have_imaging_sw()
{
    return (dps_flag || newacq || (fifolpsize > 63) );
}

int
is_psg4acqi()
{
    return( acqiflag );
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

void diplexer_override(int state)
{
}
