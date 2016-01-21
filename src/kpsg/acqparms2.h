/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef ACQPARMS2_H
#define ACQPARMS2_H
/*-----------------------------------------------------------------------
|	The type definitions, structure definitions, and the 
|	   global varaibles
+--------------------------------------------------------------------*/
typedef short codeint;		/* 2 bytes */
typedef long  codelong;		/* 4 bytes */

#define	ACQPARMS
#define PI               3.14159265358979323846

#define MAXSTR 256
#ifndef MAX_RFCHAN_NUM
#define MAX_RFCHAN_NUM 2
#endif

extern int     PSfile;		/* Acode file discriptor */
extern short   *Codes; 		/* beginning of Acode array */
extern long   Codesize;		/* size of the malloc space for codes */
extern long   CodeEnd;		/* End Address of the malloc space for codes */
extern short  *Codeptr; 	/* pointer into the Acode array */
extern short  *Aacode;		/* pointer to start address of Codes */
extern short  *lc_stadr;	/* Low Core Start Address */

/*--------------------------------------------------------------------
|
|	Global PSG Acquisition parameters
|	Those commented out are not available (yet)
+-------------------------------------------------------------------*/
extern char  alock[MAXSTR];
extern char  dmm[MAXSTR]; 	/* decoupler modulation type */
extern char  dm[MAXSTR];	/* decoupler status control */
extern char  rftype[MAXSTR];	/* type of rf system used for trans & dec */
extern char  gradtype[MAXSTR];	/* char keys w-waveform gen s-sisco n-none */
extern char  hs[MAXSTR]; 	/* homospoil status control */
extern char  il[MAXSTR];	/* interleave  parameter, 'y','n' */
extern char  rfwg[MAXSTR];	/* rf waveform generator flags */
extern char  satmode[MAXSTR];
extern char  wshim[MAXSTR];
extern char  seqfil[];

extern double  inc2D;		/* t1 dwell time */
extern double  alfa; 		/* Time after rec is turned on that acqbegins */
extern double  at; 		/* Acquisition Time */
extern double  bs;		/* Block Size */
extern double  d1; 		/* delay */
extern double  d2; 		/* A delay, used in 2D experiments */
extern double  d3; 		/* A delay */
extern double  dfrq; 		/* Decoupler Frequency MHz */
extern double  dhp; 		/* decoupler Power value dhp = dpwr */
extern double  dpwr; 		/* decoupler Power value dpwr=dhp */
extern double  dpwrf; 		/* decoupler Fine Power */
extern double  dlp; 		/* decoupler Low Power value */
extern double  dmf;		/* decoupler modulation frequency */
extern double  dof;		/* Decoupler Offset */
extern double  fb;		/* Filter Bandwidth */
extern double  gain; 		/* receiver gain value, or 'n' for autogain */
extern double  gmax;            /* maximum gradient value (G/cm) */
extern double gradalt;
extern double tau;
extern double  gradstepsz;      /* positive steps in the gradient dac */
extern double  hst;    		/* time homospoil is active */
extern double  nf;		/* Number of fids in Pulse sequence */
extern double  ni;		/* Number of fids in 2D sequence */
extern double  ni2;		/* Number of fids in 3D sequence */
extern double  ni3;		/* Number of fids in 4D sequence */
extern double  np; 		/* Number of data points to acquire */
extern double  nt; 		/* number of transients */
extern double  p1; 		/* A pulse width */
extern double  pad; 		/* Pre-acquisition delay */
extern double  pplvl; 		/* decoupler pulse power level */
extern double  pw; 		/* pulse width */
extern double  pwx;		/* X-nucleus pulse length */
extern double  pwxlvl;		/* pwx power level */
extern double  rof1; 		/* Time receiver is turned off before pulse */
extern double  rof2;		/* Time after pulse before receiver turned on */
extern double  sfrq;   		/* Transmitter Frequency MHz */
extern double  spin;		/* was extern int spinactive; is if spin=-1.0 */
extern double  sw; 		/* Sweep width */
extern double  tof;		/* Transmitter Offset */
extern double  tpwr; 		/* Transmitter pulse power */
extern double  tpwrf; 		/* Transmitter pulse fine power */
extern double  vtc; 		/* VT temperature cooling gas setting */
extern double  vttemp; 		/* VT temperauture setting */
extern double  vtwait; 		/* VT temperature timeout setting */
extern double  sw1,sw2,sw3;

/* --- total time of experiment --- */
extern double  totaltime;	/* total time for a exp duration estimate */

/* --- status control --- */
extern int  again;		/* was extern int  gainactive; */
extern int  arrayelements;	/* number of array elements */
extern int  automated;		/* True if system is an automated one */
extern int  cpflag;  		/* phase cycling 1=no cycling,  0=quad detect*/
extern int  curfifocount;	/* current number of word in fifo */
extern int  declvlonoff;	/* to distinguish between G2000 and Krikkit */
extern int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
extern int  inf;		/* was extern char in[] LK interlock flag */
extern int  loc;		/* current sample used with sample changer */
extern int  lockmode;		/* was extern char alock[MAXSTR]; */
extern int  mf_dm;		/* was dmsize, number of chars in dm */
extern int  mf_dmm;		/* was dmmsize, number of chars in dmm */
extern int  mf_hs; 		/* was hssize, number of characters in hs */
extern int  nth2D;		/* 2D Element counter in Acode generation  */
extern int OBSch;               /* The Acting Observe Channel */
extern int DECch;               /* The Acting Decoupler Channel */
extern int DEC2ch;              /* The Acting 2nd Decoupler Channel */
extern int DEC3ch;              /* The Acting 3rd Decoupler Channel */
extern int DEC4ch;              /* The Acting 4th Decoupler Channel */
extern int d2_index;            /* d2 increment (from 0 to ni-1) */
extern int  NUMch;		/* Number of channels configured, always 2 */
extern int  ok;			/* Global error flag */
extern int  phase1;		/* 2D acquistion mode */
extern int  ptsval[MAX_RFCHAN_NUM + 1];	/* PTS type for trans & decoupler */
extern int  setupflag;		/* alias used to invoke PSG,go=0,su=1*/
/*extern int  statusindx; */	/* current status index */
extern int  tinf;		/* was extern char tin[] VT interlock flag */
extern int  tpf;		/* was extern int  tempactive; */
extern int  vttype;		/* VT type 0=none,1=varian,2=oxford */
extern int  dps_flag;		/* flag is true if dps is executing */
extern int  checkflag;		/* flag is true if check option is passed to go */
extern int  tuneflag;    	/* 'tune' is argument, for vnmrj */

extern unsigned long ix;	/* FID currently in Acode generation */

/* --- Pulse Seq. pointers to real time variables --- */
extern codeint npr_ptr;		/* offset into code to variable np    */
extern codeint ct;		/* offset into code to variable ct    */
extern codeint ctss;		/* offset into code to variable rtvptr */
extern codeint oph;		/* offset into code to variable oph   */
extern codeint ssval;		/* offset into code to variable ss    */
extern codeint ssctr;		/* offset into code to variable ssct  */
extern codeint bsval;		/* offset into code to variable bs    */
extern codeint bsctr;		/* offset into code to variable bsct  */
extern codeint fidctr;		/* offset into code to variable elemid*/
/*extern int nsp_ptr; */	/* offset into code to variable nsp   */
extern codeint id2;		/* offset into code to variable id2   */
extern codeint zero;		/* offset into code to variable zero  */
extern codeint one;		/* offset into code to variable one   */
extern codeint two;		/* offset into code to variable two   */
extern codeint three;		/* offset into code to variable three */
extern codeint tablert;		/* offset into code to table variable */
extern codeint v1;		/* offset into code to variable v1    */
extern codeint v2;		/* offset into code to variable v2    */
extern codeint v3;		/* offset into code to variable v3    */
extern codeint v4;		/* offset into code to variable v4    */
extern codeint v5;		/* offset into code to variable v5    */
extern codeint v6;		/* offset into code to variable v6    */
extern codeint v7;		/* offset into code to variable v7    */
extern codeint v8;		/* offset into code to variable v8    */
extern codeint v9;		/* offset into code to variable v9    */
extern codeint v10;		/* offset into code to variable v10   */
extern codeint v11;		/* offset into code to variable v11   */
extern codeint v12;		/* offset into code to variable v12   */
extern codeint v13;		/* offset into code to variable v13   */
extern codeint v14;		/* offset into code to variable v14   */
extern codeint v15;		/* offset into code to variable v15   */
extern codeint v16;		/* offset into code to variable v16   */

/* saturation parameters */
extern double  satpwr;          /* saturation pulse power */
extern double  satdly;          /* saturation time */
extern double  satfrq;          /* saturation offset */

#endif
