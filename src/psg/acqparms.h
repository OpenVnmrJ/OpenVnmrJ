/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ACQPARMS_H
#define ACQPARMS_H
/*-----------------------------------------------------------------------
|	The type definitions, structure definitions, and the 
|	   global varaibles
+--------------------------------------------------------------------*/

#ifdef AIX
#if defined(B0)
#undef B0
#endif
#endif

/* --- code type definitions, can be changed for different machines */
typedef char codechar;		/* 1 bytes */
typedef short codeint;		/* 2 bytes */
typedef long  codelong;		/* 4 bytes */
typedef unsigned long  codeulong;/* 4 bytes */

/*  define ACQPARMS so that other header files know this one is present */
#define ACQPARMS

/*  if oopc.h not included then  define Object type */
#ifndef OOPC

/* Object Handle Structure */
typedef int (*Functionp)();
typedef struct {Functionp dispatch; char *objname; } *Object;

#endif
/* --- code array variables --- */

extern codeint    *Codes; 	/* beginning of Acode array */
extern long	   Codesize;	/* size of the malloc space for codes */
extern long	   CodeEnd;	/* End Address of the malloc space for codes */
extern codeint    *Codeptr; 	/* pointer into the Acode array */
extern codeint    *Aacode;	/* pointer to start address of Codes */
extern codeint    *lc_stadr;  /* Low Core Start Address */

/*--------------------------------------------------------------------
|
|	Global PSG Acquisition parameters
|
+-------------------------------------------------------------------*/
#define MAXSTR 256
#ifndef MAX_RFCHAN_NUM
#define MAX_RFCHAN_NUM 6
#endif

extern char spec_system[MAXSTR];/* system discription ,vxr,xl,xla,etc. */
extern char rftype[MAXSTR];	/* type of rf system used for trans & dec */
extern char rfband[MAXSTR];	/* RF band of trans & dec  (high or low) */
extern double  cattn[MAX_RFCHAN_NUM+1];/* indicates coarse attn for channels */
extern double  fattn[MAX_RFCHAN_NUM+1]; /* indicates fine attn for channels */
extern char rfwg[MAXSTR];	/* y/n for rf waveform generators */
extern char gradtype[MAXSTR];	/* char keys w-waveform gen s-sisco n-none */

extern char dqd[MAXSTR];	/* Digital Quadrature Detection, y or n */

/* --- global flags --- */
extern int  ok;			/* Global error flag */
extern int  automated;		/* True if system is an automated one */
extern int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
extern int  ptsval[MAX_RFCHAN_NUM + 1];	/* PTS type for trans & decoupler */
extern int  newdec;		/* True: system uses direct synthesis for dec */
extern int  newtrans; 		/* True: system uses direct syn for trans */
extern int  newdecamp;		/* True: system uses class A amps for dec */
extern int  newtransamp;	/* True: system uses class A amps for xmtr */
extern int  vttype;		/* VT type 0=none,1=varian,2=oxford */
extern int  dps_flag;		/* flag is true if dps is executing */
extern int  checkflag;		/* flag is true if check option is passed to go */

/* automation data */
/*extern autodata autostruct;*/

/* acquisition */
extern char    il[MAXSTR];	/* interleaved acquisition parameter, 'y','n', or 'f#' */
extern double  inc2D;		/* t1 dwell time */
extern double  inc3D;		/* t2 dwell time in a 3D/4D experiment */
extern double  inc4D;		/* t3 dwell time in a 3D/4D experiment */
extern double  sw; 		/* Sweep width */
extern double  nf;		/* Number of fids in Pulse sequence */
extern double  np; 		/* Number of data points to acquire */
extern double  nt; 		/* number of transients */
extern double  sfrq;   		/* Transmitter Frequency MHz */
extern double  dfrq; 		/* Decoupler Frequency MHz */
extern double  dfrq2; 		/* 2nd Decoupler Frequency MHz */
extern double  dfrq3; 		/* 3rd Decoupler Frequency MHz */
extern double  dfrq4; 		/* 4th Decoupler Frequency MHz */
extern double  fb;		/* Filter Bandwidth */
extern double  bs;		/* Block Size */
extern double  tof;		/* Transmitter Offset */
extern double  dof;		/* Decoupler Offset */
extern double  dof2;		/* 2nd Decoupler Offset */
extern double  dof3;		/* 3rd Decoupler Offset */
extern double  dof4;		/* 4th Decoupler Offset */
extern double  gain; 		/* receiver gain value, or 'n' for autogain */
extern double  dlp; 		/* decoupler Low Power value */
extern double  dhp; 		/* decoupler High Power value */
extern double  tpwr; 		/* Transmitter pulse power */
extern double  tpwrf;		/* Transmitter fine linear attenuator for pulse power */
extern double  dpwr;		/* Decoupler pulse power */
extern double  dpwrf;		/* Decoupler fine linear attenuator power */
extern double  dpwrf2;		/* 2nd Decoupler fine linear attenuator */
extern double  dpwrf3;		/* 3rd Decoupler fine linear attenuator */
extern double  dpwrf4;		/* 4th Decoupler fine linear attenuator */
extern double  dpwr2;		/* 2nd Decoupler pulse power */
extern double  dpwr3;		/* 3rd Decoupler pulse power */
extern double  dpwr4;		/* 4th Decoupler pulse power */
extern double  filter;		/* pulse Amp filter setting */
extern double  xmf;		/* transmitter modulation frequency */
extern double  dmf;		/* decoupler modulation frequency */
extern double  dmf2;		/* decoupler modulation frequency */
extern double  dmf3;		/* decoupler modulation frequency */
extern double  dmf4;		/* decoupler modulation frequency */
extern double  fb;		/* filter bandwidth */
extern double  vttemp; 		/* VT temperauture setting */
extern double  vtwait; 		/* VT temperature timeout setting */
extern double  vtc; 		/* VT temperature cooling gas setting */
extern int     cpflag;  	/* phase cycling 1=no cycling,  0=quad detect*/
extern int     dhpflag;		/* decoupler High Power Flag */

/* --- pulse widths --- */
extern double  pw; 		/* pulse width */
extern double  p1; 		/* A pulse width */
extern double  pw90;		/* 90 degree pulse width */
extern double  hst;    		/* time homospoil is active */

/* --- delays --- */
extern double  alfa; 		/* Time after rec is turned on that acqbegins */
extern double  beta; 		/* audio filter time constant */
extern double  d1; 		/* delay */
extern double  d2; 		/* A delay, used in 2D experiments */
extern double  d3;		/* a delay, used in 3D experiments */
extern double  d4;		/* a delay, used in 4D experiments */
extern double  pad; 		/* Pre-acquisition delay */
extern int     padactive; 	/* Pre-acquisition delay active parameter flag */
extern double  rof1; 		/* Time receiver is turned off before pulse */
extern double  rof2;		/* Time after pulse before receiver turned on */

/* --- total time of experiment --- */
extern double  totaltime;   /* total timer events for a exp duration estimate */

extern int   phase1;            /* 2D acquisition mode */
extern int   phase2;            /* 3D acquisition mode */
extern int   phase3;            /* 4D acquisition mode */

extern int   d2_index;          /* d2 increment (from 0 to ni-1) */
extern int   d3_index;          /* d3 increment (from 0 to ni2-1) */
extern int   d4_index;          /* d4 increment (from 0 to ni3-1) */

/* --- programmable decoupling sequences --- */
extern char  xseq[MAXSTR];
extern char  dseq[MAXSTR];
extern char  dseq2[MAXSTR];
extern char  dseq3[MAXSTR];
extern char  dseq4[MAXSTR];
extern double xres;		/* digit resolutio prg dec */
extern double dres;		/* digit resolutio prg dec */
extern double dres2;		/* digit resolutio prg dec */ 
extern double dres3;		/* digit resolutio prg dec */ 
extern double dres4;		/* digit resolutio prg dec */ 

/* --- status control --- */
extern char  xm[MAXSTR];	/* transmitter status control */
extern char  xmm[MAXSTR]; 	/* trasnmitter modulation type control */
extern char  dm[MAXSTR];	/* 1st decoupler status control */
extern char  dmm[MAXSTR]; 	/* 1st decoupler modulation type control */
extern char  dm2[MAXSTR]; 	/* 2nd decoupler status control */
extern char  dmm2[MAXSTR]; 	/* 2nd decoupler modulation type control */
extern char  dm3[MAXSTR]; 	/* 3rd decoupler status control */
extern char  dmm3[MAXSTR]; 	/* 3rd decoupler modulation type control */
extern char  dm4[MAXSTR]; 	/* 4th decoupler status control */
extern char  dmm4[MAXSTR]; 	/* 4th decoupler modulation type control */
extern char  homo[MAXSTR]; 	/* first  decoupler homo mode control */
extern char  homo2[MAXSTR]; 	/* second decoupler homo mode control */
extern char  homo3[MAXSTR]; 	/* third  decoupler homo mode control */
extern char  homo4[MAXSTR]; 	/* fourth  decoupler homo mode control */
extern char  hs[MAXSTR]; 	/* homospoil status control */
extern int   xmsize;		/* number of characters in xm */
extern int   xmmsize;		/* number of characters in xmm */
extern int   dmsize;		/* number of characters in dm */
extern int   dmmsize;		/* number of characters in dmm */
extern int   dm2size;		/* number of characters in dm2 */
extern int   dmm2size;		/* number of characters in dmm2 */
extern int   dm3size;		/* number of characters in dm3 */
extern int   dmm3size;		/* number of characters in dmm3 */
extern int   dm4size;		/* number of characters in dm4 */
extern int   dmm4size;		/* number of characters in dmm4 */
extern int   homosize;		/* number of characters in homo */
extern int   homo2size;		/* number of characters in homo2 */
extern int   homo3size;		/* number of characters in homo3 */
extern int   homo4size;		/* number of characters in homo4 */
extern int   hssize; 		/* number of characters in hs */

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
extern struct _ModInfo ModInfo[MAX_RFCHAN_NUM+1]; /* structure with pointers */
						  /* to dmf, dm, dmm, dmsize */
						  /* etc.		     */

extern int curfifocount;	/* current number of word in fifo */
extern int setupflag;		/* alias used to invoke PSG,go=0,su=1*/
extern int ra_flag;		/* ra flag */
extern int statusindx;		/* current status index */
extern int HSlines;		/* High Speed output board lines */

/* --- Explicit acquisition parameters --- */
extern int hwlooping;		/* hardware looping inprogress flag */
extern int hwloopelements;	/* PSG elements in hardware loop */
extern int starthwfifocnt;	/* fifo count at start of hwloop */
extern int acqtriggers;		/*# of data acquires */
extern int noiseacquire;	/* noise acquiring flag */

/*- These values are used so that they are changed only when their values do */
extern int oldlkmode;		/* previous value of lockmode */
extern int oldspin;		/* previous value of spin */
extern int oldwhenshim;		/* previous value of shimming mask */
extern double oldvttemp;	/* previous value of vt tempature */
extern double oldpad;			/* previous value of pad */

/*- These values are used so that they are included in the Acode only when */
/*  the next value out of the arrayed values has been obtained */
extern int oldpadindex;		/* previous value of pad value index */
extern int newpadindex;		/* new value of pad value index */

/* ---  interlock parameters, etc. --- */
extern char interLock[];		/* interlock flag */
extern char alock[MAXSTR];	
extern char wshim[MAXSTR];
extern int  spin;
extern int  spinactive;
extern int  tempactive;
extern int  loc;
extern int  gainactive;
extern int  lockmode;
extern int  whenshim;
extern int  shimatanyfid;
extern double gradalt;

/* --- Pulse Seq. globals --- */
extern double decstep;		/* phase stepsize of decoupler */
extern double xmtrstep;		/* phase stepsize of transmitter */
extern codeint idc;		/* PSG software release */
extern codeint npr_ptr;		/* offset into code to variable np */
extern codeint ct;		/* offset into code to variable ct */
extern codeint ctss;		/* offset into code to variable rtvptr */
extern codeint oph;		/* offset into code to variable oph */
extern codeint ssval;		/* offset into code to variable ss */
extern codeint ssctr;		/* offset into code to variable ssct */
extern codeint bsval;		/* offset into code to variable bs */
extern codeint bsctr;		/* offset into code to variable bsct */
extern codeint fidctr;		/* offset into code to variable elemid */
extern codeint HSlines_ptr;	/* offset into code to variable squi */
extern codeint nsp_ptr;		/* offset into code to variable nsp */
extern codeint sratert;		/* offset into code to variable srate */
extern codeint rttmp;		/* offset into code to variable rttmp */
extern codeint spare1rt;	/* offset into code to variable spare1 */
extern codeint id2;		/* offset into code to variable id2 */
extern codeint id3;		/* offset into code to variable id3 */
extern codeint id4;		/* offset into code to variable id4 */
extern codeint zero;		/* offset into code to variable zero */
extern codeint one;		/* offset into code to variable one */
extern codeint two;		/* offset into code to variable two */
extern codeint three;		/* offset into code to variable three */
extern codeint rf500_ptr;	/* offset into code to rf500 variables */
extern codeint tablert;		/* offset into code to table variable */
extern codeint v1;		/* offset into code to variable v1 */
extern codeint v2;		/* offset into code to variable v2 */
extern codeint v3;		/* offset into code to variable v3 */
extern codeint v4;		/* offset into code to variable v4 */
extern codeint v5;		/* offset into code to variable v5 */
extern codeint v6;		/* offset into code to variable v6 */
extern codeint v7;		/* offset into code to variable v7 */
extern codeint v8;		/* offset into code to variable v8 */
extern codeint v9;		/* offset into code to variable v9 */
extern codeint v10;		/* offset into code to variable v10 */
extern codeint v11;		/* offset into code to variable v11 */
extern codeint v12;		/* offset into code to variable v12 */
extern codeint v13;		/* offset into code to variable v13 */
extern codeint v14;		/* offset into code to variable v14 */
extern codeint v15;
extern codeint v16;
extern codeint v17;
extern codeint v18;
extern codeint v19;
extern codeint v20;
extern codeint v21;
extern codeint v22;
extern codeint v23;
extern codeint v24;

extern codeint tpwrrt;		/* offset to special tpwr */
extern codeint dhprt;		/* offset to special dhprt */
extern codeint tphsrt;		/* offset to special tphsrt */
extern codeint dphsrt;		/* offset to special dphsrt */

extern codeint ntrt;		/* offset into code to nt for vscan elem. */
extern codeint dlvlrt;		/* offset to special dlvlrt */

extern codeint hwloop_ptr;	/* offset to hardware loop Acode */
extern codeint multhwlp_ptr;	/* offset to multiple hardware loop flag */
extern codeint nsc_ptr;		/* offset to NSC Acode */
extern unsigned long ix;		/* FID currently in Acode generation */
extern int     nth2D;		/* 2D Element currently in Acode generation (VIS usage)*/
extern int     arrayelements;	/* number of array elements */
extern int     fifolpsize;	/* fifo loop size (words, eg. 63,512,1k,2k,4k, etc.) */

/*  Objects   */
extern Object Ext_Trig;		/* External Trigger Device */
extern Object RT_Delay;		/* Real-Time Delay Event Device */
extern Object ToAttn;		/* AMT Interface Obs attenuator */
extern Object DoAttn;		/* AMT Interface Dec attenuator */
extern Object Do2Attn;		/* AMT Interface Dec2 attenuator */
extern Object Do3Attn;		/* AMT Interface Dec3 attenuator */
extern Object Do4Attn;		/* AMT Interface Dec4 attenuator */
extern Object ToLnAttn;		/* Linear attenuator, Obs Channel */
extern Object DoLnAttn;		/* Linear attenuator, Dec Channel */
extern Object Do2LnAttn;	/* Linear attenuator, Dec2 Channel */
extern Object Do3LnAttn;	/* Linear attenuator, Dec3 Channel */
extern Object Do4LnAttn;	/* Linear attenuator, Dec4 Channel */
extern Object RF_Rout;		/* RF Routing Relay Bank */
extern Object RF_Opts;		/* RF Amp & etc. Options Bank */
extern Object RF_Opts2;		/* RF Amp & etc. Options Bank */
extern Object RF_MLrout;	/* RF Routing Byte in Magnet Leg */
extern Object RF_PICrout;	/* RF Routing Byte in 4 channel type Magnet Leg (PIC) */
extern Object RF_TR_PA;		/* RF T/R and PreAmp enables */
extern Object RF_Amp1_2;	/* Amp 1/2 cw vs. pulse */
extern Object RF_Amp3_4;	/* Amp 3/4 cw vs. pulse */
extern Object RF_Amp_Sol;	/* Amp solids cw vs. pulse */
extern Object RF_mixer;		/* xmtrs hi/lo mixer select */
extern Object RF_hilo;		/* attns hi/lo-band relays router */
extern Object RF_Mod;		/* xmtrs modulation mode APbyte */
extern Object HS_select;	/* xmtrs HS lines select */
extern Object RCVR_homo;	/* RCVR homo decoupler bit */
extern Object X_gmode;		/* xmtrs gating mode select */
extern Object HS_Rotor;		/* High Speed Rotor Device */
extern Object RFChan1;		/* RF Channel Device */
extern Object RFChan2;		/* RF Channel Device */
extern Object RFChan3;		/* RF Channel Device */
extern Object RFChan4;		/* RF Channel Device */
extern Object RFChan5;		/* RF Channel Device */
extern Object BOBreg0;		/* Apbus BreakOut Board reg 0 */
extern Object BOBreg1;		/* Apbus BreakOut Board reg 1 */
extern Object BOBreg2;		/* Apbus BreakOut Board reg 2 */
extern Object BOBreg3;		/* Apbus BreakOut Board reg 3 */
extern Object APall;		/* General APbus register */
extern Object RF_Channel[];	/* index array of RF channel Objects */


extern int OBSch;		/* The Acting Observe Channel */
extern int DECch;  		/* The Acting Decoupler Channel */
extern int DEC2ch;  		/* The Acting 2nd Decoupler Channel */
extern int DEC3ch;  		/* The Acting 3rd Decoupler Channel */
extern int DEC4ch;  		/* The Acting 4th Decoupler Channel */
extern int NUMch;		/* Number of channels configured */


/************************************************************************/
/*									*/
/*	SIS Definitions							*/
/*									*/
/************************************************************************/


/*------------------------------------------------------------------
    CONSTANTS
------------------------------------------------------------------*/
#define MAXSLICE         1024         /*maximum number of slices*/
#define MHZ_HZ           1.0e6        /*MHz to Hz conversion*/ 
#define WAIT             1
#define NOWAIT           0
#define PI               3.14159265358979323846


/*------------------------------------------------------------------
    RF and Gradient pattern structures
------------------------------------------------------------------*/
typedef struct _RFpattern {
    double  phase;
    double  amp;
    double  time;
} RFpattern;

typedef struct _DECpattern {
    double  tip;
    double  phase;
    double  phase_inc;
    double  amp;
    double  amp_inc;
    int gate;
} DECpattern;

typedef struct _Gpattern {
    double  amp;
    double  time;
    int ctrl;
} Gpattern;


/*------------------------------------------------------------------
    RF PULSES
------------------------------------------------------------------*/
extern double  p2;                  /* pulse length */
extern double  p3;                  /* pulse length */
extern double  p4;                  /* pulse length */
extern double  p5;                  /* pulse length */
extern double  pi;                  /* inversion pulse length */
extern double  psat;                /* saturation pulse length */
extern double  pmt;                 /* magnetization transfer pulse length */
extern double  pwx;                 /* X-nucleus pulse length */
extern double  pwx2;                /* X-nucleus pulse length */
extern double  psl;                 /* spin-lock pulse length */

extern char  pwpat[MAXSTR];         /* pattern for pw,tpwr */
extern char  p1pat[MAXSTR];         /* pattern for p1,tpwr1 */
extern char  p2pat[MAXSTR];         /* pattern for p2,tpwr2 */
extern char  p3pat[MAXSTR];         /* pattern for p3,tpwr3 */
extern char  p4pat[MAXSTR];         /* pattern for p4,tpwr4 */
extern char  p5pat[MAXSTR];         /* pattern for p5,tpwr5 */
extern char  pipat[MAXSTR];         /* pattern for pi,tpwri */
extern char  satpat[MAXSTR];        /* pattern for psat,satpat */
extern char  mtpat[MAXSTR];         /* magnetization transfer RF pattern */
extern char  pslpat[MAXSTR];        /* pattern for spin-lock */

extern double  tpwr1;               /* Transmitter pulse power */
extern double  tpwr2;               /* Transmitter pulse power */
extern double  tpwr3;               /* Transmitter pulse power */
extern double  tpwr4;               /* Transmitter pulse power */
extern double  tpwr5;               /* Transmitter pulse power */
extern double  tpwri;               /* inversion pulse power */
extern double  satpwr;              /* saturation pulse power */
extern double  mtpwr;               /* magnetization transfer pulse power */
extern double  pwxlvl;              /* pwx power level */
extern double  pwxlvl2;             /* pwx2 power level */
extern double  tpwrsl;              /* spin-lock power level */


/*------------------------------------------------------------------
    RF DECOUPLER PULSES
------------------------------------------------------------------*/
extern char  decpat[MAXSTR];        /* pattern for decoupler pulse */
extern char  decpat1[MAXSTR];       /* pattern for decoupler pulse */
extern char  decpat2[MAXSTR];       /* pattern for decoupler pulse */
extern char  decpat3[MAXSTR];       /* pattern for decoupler pulse */
extern char  decpat4[MAXSTR];       /* pattern for decoupler pulse */
extern char  decpat5[MAXSTR];       /* pattern for decoupler pulse */

extern double  dpwr1;               /* Decoupler pulse power */
extern double  dpwr4;               /* Decoupler pulse power */
extern double  dpwr5;               /* Decoupler pulse power */


/*------------------------------------------------------------------
    GRADIENTS
------------------------------------------------------------------*/
extern double  gro,gro2,gro3;       /* read out gradient strength */
extern double  gpe,gpe2,gpe3;       /* phase encode for 2D, 3D & 4D */
extern double  gss,gss2,gss3;       /* slice-select gradients */
extern double  gror;                /* read out refocus */
extern double  gssr;                /* slice select refocus */
extern double  grof;                /* read out refocus fraction */
extern double  gssf;                /* slice refocus fraction */
extern double  g0,g1,g2,g3,g4;      /* numbered levels */
extern double  g5,g6,g7,g8,g9;      /* numbered levels */
extern double  gx,gy,gz;            /* X, Y, and Z levels */
extern double  gvox1,gvox2,gvox3;   /* voxel selection */
extern double  gdiff;               /* diffusion encode */
extern double  gflow;               /* flow encode */
extern double  gspoil,gspoil2;      /* spoiler gradient levels */
extern double  gcrush,gcrush2;      /* crusher gradient levels */
extern double  gtrim,gtrim2;        /* trim gradient levels */
extern double  gramp,gramp2;        /* ramp gradient levels */
extern double  gpemult;             /* shaped phase-encode multiplier */
extern double  gradstepsz;	    /* positive steps in the gradient dac */
extern double  gradunit;            /* dimensional conversion factor */
extern double  gmax;                /* maximum gradient value (G/cm) */
extern double  gxmax;               /* x maximum gradient value (G/cm) */
extern double  gymax;               /* y maximum gradient value (G/cm) */
extern double  gzmax;               /* z maximum gradient value (G/cm) */
extern double  gtotlimit;           /* limit combined gradient values (G/cm) */
extern double  gxlimit;             /* safety limit for x gradient  (G/cm) */
extern double  gylimit;             /* safety limit for y gradient  (G/cm) */
extern double  gzlimit;             /* safety limit for z gradient  (G/cm) */
extern double  gxscale;             /* X scaling factor for gmax */
extern double  gyscale;             /* Y scaling factor for gmax */
extern double  gzscale;             /* Z scaling factor for gmax */

extern char  gpatup[MAXSTR];        /* gradient ramp-up pattern */
extern char  gpatdown[MAXSTR];      /* gradient ramp-down pattern */
extern char  gropat[MAXSTR];        /* readout gradient pattern */
extern char  gpepat[MAXSTR];        /* phase-encode gradient pattern */
extern char  gsspat[MAXSTR];        /* slice gradient pattern */
extern char  gpat[MAXSTR];          /* general gradient pattern */
extern char  gpat1[MAXSTR];         /* general gradient pattern */
extern char  gpat2[MAXSTR];         /* general gradient pattern */
extern char  gpat3[MAXSTR];         /* general gradient pattern */
extern char  gpat4[MAXSTR];         /* general gradient pattern */
extern char  gpat5[MAXSTR];         /* general gradient pattern */


/*------------------------------------------------------------------
    DELAYS
------------------------------------------------------------------*/
extern double  tr;                  /* repetition time per scan */
extern double  te;                  /* primary echo time */
extern double  ti;                  /* inversion time */
extern double  tm;                  /* mid delay for STE */
extern double  at;                  /* acquisition time */
extern double  tpe,tpe2,tpe3;       /* phase encode durations for 2D-4D */    
extern double  tcrush;              /* crusher gradient duration */
extern double  tdiff;               /* diffusion encode duration */
extern double  tdelta;              /* diffusion encode duration */
extern double  tDELTA;              /* diffusion gradient separation */
extern double  tflow;               /* flow encode duration */
extern double  tspoil;              /* spoiler duration */
extern double  hold;                /* physiological trigger hold off */
extern double  trise;               /* gradient coil rise time: sec */
extern double  satdly;              /* saturation time */
extern double  tau;                 /* general use delay */    
extern double  runtime;             /* user variable for total exp time */    


/*------------------------------------------------------------------
    FREQUENCIES
------------------------------------------------------------------*/
extern double  resto;               /* reference frequency offset */
extern double  wsfrq;               /* water suppression offset */
extern double  chessfrq;            /* chemical shift selection offset */
extern double  satfrq;              /* saturation offset */
extern double  mtfrq;               /* magnetization transfer offset */


/*------------------------------------------------------------------
    PHYSICAL SIZES AND POSITIONS
      Dimensions and positions for slices, voxels and fov
------------------------------------------------------------------*/
extern double  pro;                 /* fov position in read out */
extern double  ppe,ppe2,ppe3;       /* fov position in phase encode */
extern double  pos1,pos2,pos3;      /* voxel position */
extern double  pss[MAXSLICE];       /* slice position array */

extern double  lro;                 /* read out fov */
extern double  lpe,lpe2,lpe3;       /* phase encode fov */
extern double  lss;                 /* dimension of multislice range */

extern double  vox1,vox2,vox3;      /* voxel size */
extern double  thk;                 /* slice or slab thickness */

extern double  fovunit;             /* dimensional conversion factor */
extern double  thkunit;             /* dimensional conversion factor */


/*------------------------------------------------------------------
    BANDWIDTHS
------------------------------------------------------------------*/
extern double  sw1,sw2,sw3;         /* phase encode bandwidths */


/*------------------------------------------------------------------
    ORIENTATION PARAMETERS
------------------------------------------------------------------*/
extern char  orient[MAXSTR];        /* slice orientation */
extern char  vorient[MAXSTR];       /* voxel orientation */
extern char  dorient[MAXSTR];       /* diffusion gradient orientation */
extern char  sorient[MAXSTR];       /* saturation band orientation */
extern char  orient2[MAXSTR];       /* spare orientation */

extern double  psi,phi,theta;       /* slice Euler angles */
extern double  vpsi,vphi,vtheta;    /* voxel Euler angles */
extern double  dpsi,dphi,dtheta;    /* diffusion gradient Euler angles */
extern double  spsi,sphi,stheta;    /* saturation band Euler angles */

extern double offsetx,offsety,offsetz; /* shim offsets for coordinate rotator board */
extern double gxdelay,gydelay,gzdelay; /* gradient amplifier delays for coordinate rotator board */


/*------------------------------------------------------------------
    COUNTS AND FLAGS
------------------------------------------------------------------*/
extern double  nD;                  /* experiment dimensionality */
extern double  ns;                  /* number of slices */
extern double  ne;                  /* number of echoes */
extern double  ni;                  /* number of standard increments */
extern double  ni2;                 /* number of 3d increments */
extern double  nv,nv2,nv3;          /* number of phase encode views */
extern double  ssc;                 /* compressed ss transients */
extern double  ticks;               /* external trigger counter */

extern char  ir[MAXSTR];            /* inversion recovery flag */
extern char  ws[MAXSTR];            /* water suppression flag */
extern char  mt[MAXSTR];            /* magnetization transfer flag */
extern char  pilot[MAXSTR];         /* auto gradient balance flag */
extern char  seqcon[MAXSTR];        /* acquisition loop control flag */
extern char  petable[MAXSTR];       /* name for phase-encode table */
extern char  acqtype[MAXSTR];       /* e.g. "full" or "half" echo */
extern char  exptype[MAXSTR];       /* e.g. "se" or "fid" in CSI */
extern char  apptype[MAXSTR];       /* keyword for param init, e.g., "imaging"*/
extern char  seqfil[MAXSTR];        /* pulse sequence name */
extern char  rfspoil[MAXSTR];       /* rf spoiling flag */
extern char  satmode[MAXSTR];       /* presaturation mode */
extern char  verbose[MAXSTR];       /* verbose mode for sequences and psg */


/*------------------------------------------------------------------
    Miscellaneous
------------------------------------------------------------------*/
extern double  rfphase;             /* rf phase shift  */
extern double  B0;                  /* static magnetic field level */
extern char  presig[MAXSTR];        /* PIC high/low gain setting */
extern double  gpropdelay;          /* Gradient propagation delay for grad_advance */
extern double  aqtm;                /*  */
extern char    volumercv[MAXSTR];   /* flag to control volume vs. surface coil receive */
extern double  kzero;               /* position of zero kspace line in etl */


/*------------------------------------------------------------------
    Old SISCO Imaging Variables
------------------------------------------------------------------*/
extern double  slcto;               /* slice selection offset */
extern double  delto;               /* slice spacing frequency */
extern double  tox;                 /* Transmitter Offset */
extern double  toy;                 /* Transmitter Offset */
extern double  toz;                 /* Transmitter Offset */
extern double  griserate;           /* Gradient riserate  */

#endif
