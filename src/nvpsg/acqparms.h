/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
  acqparms.h
*/

#ifndef ACQPARMS_H
#define ACQPARMS_H
/*-----------------------------------------------------------------------
|	The type definitions, structure definitions, and the
|	   global varaibles
+--------------------------------------------------------------------*/

/* --- code type definitions, can be changed for different machines */
typedef char codechar;		/* 1 bytes */
typedef unsigned int  codeulong;/* 4 bytes */

/*  define ACQPARMS so that other header files know this one is present */
#define ACQPARMS

/*  if oopc.h not included then  define Object type */
#ifndef OOPC

/* Object Handle Structure */
typedef int (*Functionp)();
typedef struct {Functionp dispatch; char *objname; } *Object;

#endif
/* --- code array variables --- */

extern int    *Codes; 	/* beginning of Acode array */
extern int	   Codesize;	/* size of the malloc space for codes */
extern int	   CodeEnd;	/* End Address of the malloc space for codes */
extern int    *Codeptr; 	/* pointer into the Acode array */
extern int    *Aacode;	/* pointer to start address of Codes */
extern int    *lc_stadr;  /* Low Core Start Address */

/*--------------------------------------------------------------------
|
|	Global PSG Acquisition parameters
|
+-------------------------------------------------------------------*/
#ifndef MAXSTR
#define MAXSTR 256
#endif
#ifndef MAX_RFCHAN_NUM
#define MAX_RFCHAN_NUM 20
#endif

#ifndef MAX_RCVR_CHNLS
#define MAX_RCVR_CHNLS 64
#endif

extern char rfband[];	/* RF band of trans & dec  (high or low) */
extern char rfwg[];	/* obsolete */
extern char gradtype[];	/* char keys w-waveform gen s-sisco n-none */
/*extern char acqparpath[]; */    /* file path to acqpar for ra, set in setGflags() */
/* these are left for sequence compatibility ONLY */
/* matching declaration in psgmain.cpp */
extern int newtrans;
extern int newtransamp;
extern int newdec;
extern int newdecamp;
extern double beta;

/* --- global flags --- */
extern int  ok;			/* Global error flag */
extern int  automated;		/* True if system is an automated one */
extern int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
extern int  ptsval[];		/* PTS type for trans & decoupler */
extern int  vttype;		/* VT type 0=none,1=varian,2=oxford */

/* automation data */
/*extern autodata autostruct;*/

/* acquisition */
extern char    il[];	        /* interleaved acquisition parameter, 'y','n', or 'f#' */
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
extern double  dlp; 		/* UNUSED but initglbl structure decoupler Low Power value */
extern double  dhp; 		/* UNUSED but initglbl structure decoupler High Power value */
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
extern double  xmf;		    /* transmitter modulation frequency */
extern double  dmf;		    /* decoupler modulation frequency */
extern double  dmf2;		/* decoupler modulation frequency */
extern double  dmf3;		/* decoupler modulation frequency */
extern double  dmf4;		/* decoupler modulation frequency */
extern double  fb;		    /* filter bandwidth UNUSED */
extern double  vttemp; 		/* VT temperauture setting */
extern double  vtwait; 		/* VT temperature timeout setting */
extern double  vtc; 		/* VT temperature cooling gas setting */
extern int     cpflag;  	/* phase cycling 1=no cycling,  0=quad detect*/

/* ddr */
extern double roff;         /* arrayable receiver offset frequency */
extern double gradalt;      /* multiplier for alternating zgradpulse and rgradient */

extern double rcvrf[];  /* channel specific non-arrayable frequency offset */
extern double rcvrp1[]; /* channel specific non-arrayable phase step */
extern double rcvrp[];  /* channel specific non-arrayable phase offset */
extern double rcvra[];  /* channel specific non-arrayable amplitude scale */

/* --- pulse widths --- */
extern double  pw; 		/* pulse width */
extern double  p1; 		/* A pulse width */
extern double  pw90;		/* 90 degree pulse width */
extern double  hst;    		/* time homospoil is active */

/* --- delays --- */
extern double  alfa; 		/* Time after rec is turned on that acqbegins */
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
extern char  xseq[];
extern char  dseq[];
extern char  dseq2[];
extern char  dseq3[];
extern char  dseq4[];
extern double xres;		/* digit resolutio prg dec */
extern double dres;		/* digit resolutio prg dec */
extern double dres2;		/* digit resolutio prg dec */
extern double dres3;		/* digit resolutio prg dec */
extern double dres4;		/* digit resolutio prg dec */

/* --- status control --- */
extern char  xm[];	/* transmitter status control */
extern char  xmm[]; 	/* trasnmitter modulation type control */
extern char  dm[];	/* 1st decoupler status control */
extern char  dmm[]; 	/* 1st decoupler modulation type control */
extern char  dm2[]; 	/* 2nd decoupler status control */
extern char  dmm2[]; 	/* 2nd decoupler modulation type control */
extern char  dm3[]; 	/* 3rd decoupler status control */
extern char  dmm3[]; 	/* 3rd decoupler modulation type control */
extern char  dm4[]; 	/* 4th decoupler status control */
extern char  dmm4[]; 	/* 4th decoupler modulation type control */
extern char  homo[]; 	/* first  decoupler homo mode control */
extern char  homo2[]; 	/* second decoupler homo mode control */
extern char  homo3[]; 	/* third  decoupler homo mode control */
extern char  homo4[]; 	/* fourth  decoupler homo mode control */
extern char  hs[]; 	/* homospoil status control */
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

extern int setupflag;		/* alias used to invoke PSG,go=0,su=1*/
extern int ra_flag;		    /* ra flag */
extern int statusindx;		/* current status index */

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
extern char alock[];
extern char wshim[];
extern int  spin;
extern int  spinactive;
extern int  tempactive;
extern int  loc;
extern int  gainactive;
extern int  lockmode;
extern int  whenshim;
extern int  shimatanyfid;

/* --- Pulse Seq. globals --- */
extern double decstep;		/* phase stepsize of decoupler */
extern double xmtrstep;		/* phase stepsize of transmitter */
extern int idc;		/* PSG software release */
extern int ntrt;
extern int npr_ptr;		/* offset into code to variable np */
extern int ct;		/* offset into code to variable ct */
extern int ctss;		/* offset into code to variable rtvptr */
extern int oph;		/* offset into code to variable oph */
extern int ssval;		/* offset into code to variable ss */
extern int ssctr;		/* offset into code to variable ssct */
extern int bsval;		/* offset into code to variable bs */
extern int bsctr;		/* offset into code to variable bsct */
extern int fidctr;		/* offset into code to variable elemid */
extern int sratert;		/* offset into code to variable srate */
extern int rttmp;		/* offset into code to variable rttmp */
extern int spare1rt;	/* offset into code to variable spare1 */
extern int id2;		/* offset into code to variable id2 */
extern int id3;		/* offset into code to variable id3 */
extern int id4;		/* offset into code to variable id4 */
extern int zero;		/* offset into code to variable zero */
extern int one;		/* offset into code to variable one */
extern int two;		/* offset into code to variable two */
extern int three;		/* offset into code to variable three */
extern int tablert;		/* offset into code to table variable */
extern int v1;		/* offset into code to variable v1 */
extern int v2;		/* offset into code to variable v2 */
extern int v3;		/* offset into code to variable v3 */
extern int v4;		/* offset into code to variable v4 */
extern int v5;		/* offset into code to variable v5 */
extern int v6;		/* offset into code to variable v6 */
extern int v7;		/* offset into code to variable v7 */
extern int v8;		/* offset into code to variable v8 */
extern int v9;		/* offset into code to variable v9 */
extern int v10;		/* offset into code to variable v10 */
extern int v11;		/* offset into code to variable v11 */
extern int v12;		/* offset into code to variable v12 */
extern int v13;		/* offset into code to variable v13 */
extern int v14;		/* offset into code to variable v14 */
extern int v15;
extern int v16;
extern int v17;
extern int v18;
extern int v19;
extern int v20;
extern int v21;
extern int v22;
extern int v23;
extern int v24;
extern int v25;
extern int v26;
extern int v27;
extern int v28;
extern int v29;
extern int v30;
extern int v31;
extern int v32;
extern int v33;
extern int v34;
extern int v35;
extern int v36;
extern int v37;
extern int v38;
extern int v39;
extern int v40;
extern int v41;
extern int v42;

/* the following are special purpose rt variables */
/* reserved for homonuclear decoupling */
extern int     res_hdec_cntr;
extern int     res_hdec_lcnt;
extern int     rtonce;

/* 11 reserved rtvars for imaging IR module */
extern int     vslice_ctr;   /* imaging IR module */
extern int     vslices;      /* imaging IR module */
extern int     virblock;     /* imaging IR module */
extern int     vnirpulses;   /* imaging IR module */
extern int     vir;          /* imaging IR module */
extern int     virslice_ctr; /* imaging IR module */
extern int     vnir;         /* imaging IR module */
extern int     vnir_ctr;     /* imaging IR module */
extern int     vtest;        /* imaging IR module */
extern int     vtabskip;     /* imaging IR module */
extern int     vtabskip2;    /* imaging IR module */

#define vtmp33 93
#define vtmp34 94
#define VVARMAX 94
/* special purpose rt variables */


extern unsigned int ix;		/* FID currently in Acode generation */
extern int     nth2D;		/* 2D Element currently in Acode generation (VIS usage)*/
extern int     arrayelements;	/* number of array elements */
/* extern int     fifolpsize; */	/* fifo loop size (words, eg. 63,512,1k,2k,4k, etc.) */


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
    DDR setAcqMode opcodes
------------------------------------------------------------------*/
#define WIEN   0x01   /* windowed AD6634 input mode */
#define WACQ   0x02   /* windowed FPGA input mode  */

#define NZ     0x10   /* keep data input valid between samples */
#define RG     0x20   /* set RG off after acquire */
#define MP     0x40   /* multi-pulse mode */
#define NTB    0x80   /* set TB off after acquire */
/*------------------------------------------------------------------
    RF and Gradient pattern structures
------------------------------------------------------------------*/
/* so users can see the typedef */
#ifdef STANDARD_H
typedef struct _RFpattern {
    double  phase;
    double  phase_inc;
    double  amp;
    double  amp_inc;
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
#endif

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

extern char  pwpat[];         /* pattern for pw,tpwr */
extern char  p1pat[];         /* pattern for p1,tpwr1 */
extern char  p2pat[];         /* pattern for p2,tpwr2 */
extern char  p3pat[];         /* pattern for p3,tpwr3 */
extern char  p4pat[];         /* pattern for p4,tpwr4 */
extern char  p5pat[];         /* pattern for p5,tpwr5 */
extern char  pipat[];         /* pattern for pi,tpwri */
extern char  satpat[];        /* pattern for psat,satpat */
extern char  mtpat[];         /* magnetization transfer RF pattern */
extern char  pslpat[];        /* pattern for spin-lock */

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
extern char  decpat[];        /* pattern for decoupler pulse */
extern char  decpat1[];       /* pattern for decoupler pulse */
extern char  decpat2[];       /* pattern for decoupler pulse */
extern char  decpat3[];       /* pattern for decoupler pulse */
extern char  decpat4[];       /* pattern for decoupler pulse */
extern char  decpat5[];       /* pattern for decoupler pulse */

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

extern char  gpatup[];        /* gradient ramp-up pattern */
extern char  gpatdown[];      /* gradient ramp-down pattern */
extern char  gropat[];        /* readout gradient pattern */
extern char  gpepat[];        /* phase-encode gradient pattern */
extern char  gsspat[];        /* slice gradient pattern */
extern char  gpat[];          /* general gradient pattern */
extern char  gpat1[];         /* general gradient pattern */
extern char  gpat2[];         /* general gradient pattern */
extern char  gpat3[];         /* general gradient pattern */
extern char  gpat4[];         /* general gradient pattern */
extern char  gpat5[];         /* general gradient pattern */


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
extern char  orient[];        /* slice orientation */
extern char  vorient[];       /* voxel orientation */
extern char  dorient[];       /* diffusion gradient orientation */
extern char  sorient[];       /* saturation band orientation */
extern char  orient2[];       /* spare orientation */

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
extern double  nv,nv2,nv3;          /* number of phase encode views */
extern double  ssc;                 /* compressed ss transients */
extern double  ticks;               /* external trigger counter */

extern char  ir[];            /* inversion recovery flag */
extern char  ws[];            /* water suppression flag */
extern char  mt[];            /* magnetization transfer flag */
extern char  pilot[];         /* auto gradient balance flag */
extern char  seqcon[];        /* acquisition loop control flag */
extern char  petable[];       /* name for phase-encode table */
extern char  acqtype[];       /* e.g. "full" or "half" echo */
extern char  exptype[];       /* e.g. "se" or "fid" in CSI */
extern char  apptype[];       /* keyword for param init, e.g., "imaging"*/
extern char  seqfil[];        /* pulse sequence name */
extern char  rfspoil[];       /* rf spoiling flag */
extern char  satmode[];       /* presaturation mode */
extern char  verbose[];       /* verbose mode for sequences and psg */


/*------------------------------------------------------------------
    Miscellaneous
------------------------------------------------------------------*/
extern double  rfphase;             /* rf phase shift  */
extern double  B0;                  /* static magnetic field level */
extern char    presig[];      /* PIC high/low gain setting */
extern double  gpropdelay;          /* Gradient propagation delay for grad_advance */
extern double  aqtm;                /*  */
extern char    volumercv[];   /* flag to control volume vs. surface coil receive */
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
