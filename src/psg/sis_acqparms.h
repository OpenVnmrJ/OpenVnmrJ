/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************************
    sis_acqparms.h

    Header file to define all the constants and parameters
    used by the SISCO applications package.
********************************************************************/

#ifdef EXTERNFLAG
#define extern
#endif


/*------------------------------------------------------------------
    CONSTANTS
------------------------------------------------------------------*/
#define MAXSLICE 1024              /*maximum number of slices*/
#define MHZ_HZ   1.0e6             /*MHz to Hz conversion*/ 
#define WAIT             1
#define NOWAIT           0


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
      The pulse lengths p1-p5, pw; and power levels tpwr1-tpwr5,
      tpwr are already standard SISCO parameters.
------------------------------------------------------------------*/
extern char pwpat[MAXSTR];         /*pattern for pw,tpwr*/
extern char p1pat[MAXSTR];         /*pattern for p1,tpwr1*/
extern char p2pat[MAXSTR];         /*pattern for p2,tpwr2*/
extern char p3pat[MAXSTR];         /*pattern for p3,tpwr3*/
extern char p4pat[MAXSTR];         /*pattern for p4,tpwr4*/
extern char p5pat[MAXSTR];         /*pattern for p5,tpwr5*/
extern char pipat[MAXSTR];         /*pattern for pi,tpwri*/
extern char satpat[MAXSTR];        /*pattern for psat,tpwrsat*/

extern double pi;                  /*inversion pulse length*/
extern double psat;                /*saturation pulse length*/

extern double tpwri;               /*inversion pulse power*/
extern double tpwrsat;             /*saturation pulse power*/



/*------------------------------------------------------------------
    RF DECOUPLER PULSES
      Power levels dpwr1-dpwr5, dpwr are already standard
      SISCO parameters.
------------------------------------------------------------------*/
extern char decpat[MAXSTR];        /*pattern for decoupler pulse*/
extern char decpat1[MAXSTR];       /*pattern for decoupler pulse*/
extern char decpat2[MAXSTR];       /*pattern for decoupler pulse*/
extern char decpat3[MAXSTR];       /*pattern for decoupler pulse*/
extern char decpat4[MAXSTR];       /*pattern for decoupler pulse*/
extern char decpat5[MAXSTR];       /*pattern for decoupler pulse*/



/*------------------------------------------------------------------
    GRADIENTS
      The gradient levels: gro,gpe,gss are already standard 
      SISCO parameters.
------------------------------------------------------------------*/
extern double gpe2,gpe3;           /*phase encode for 3D & 4D*/
extern double gss2,gss3;           /*additional slice-select*/
extern double gror;                /*read out refocus*/
extern double gssr;                /*slice select refocus*/
extern double g0,g1,g2,g3,g4;      /*numbered levels*/
extern double g5,g6,g7,g8,g9;      /*numbered levels*/
extern double gx,gy,gz;            /*X, Y, and Z levels*/
extern double gvox1,gvox2,gvox3;   /*voxel selection*/
extern double gdiff;               /*diffusion encode*/
extern double gflow;               /*flow encode*/
extern double gspoil;              /*spoiler level*/
extern double gmax;                /*maximum gradient value (G/cm)*/
extern double gpemult;             /*shaped phase-encode multiplier*/

extern double B0;                  /*static magnetic field level*/

extern int gradstepsz;	          /* positive steps in the gradient dac */

/*------------------------------------------------------------------
    DELAYS
      The delays d1-d5 are already standard SISCO parameters
------------------------------------------------------------------*/
extern double tr;                  /*repetition time per scan*/
extern double te;                  /*primary echo time*/
extern double ti;                  /*inversion time*/
/* extern double tm;                  /*mid delay for STE*/
extern double at;                  /*acquisition time*/
extern double tpe,tpe2,tpe3;       /*phase encode durations for 2D-4D*/    
extern double tdiff;               /*diffusion encode duration*/
extern double tdelta;              /*diffusion encode duration*/
extern double tDELTA;              /*diffusion gradient separation*/
extern double tflow;               /*flow encode duration*/
extern double tspoil;              /*spoiler duration*/
extern double hold;                /*physiological trigger hold off*/
extern double risetime;            /*gradient coil rise time*/
extern double riserate;            /*gradient coil rise rate: sec/G */



/*------------------------------------------------------------------
    FREQUENCIES
------------------------------------------------------------------*/
extern double wsto;                /*water suppression offset*/
extern double chessto;             /*chemical shift selection offset*/
extern double satto;               /*saturation pulse offset*/



/*------------------------------------------------------------------
    PHYSICAL SIZES AND POSITIONS
      Dimensions and positions for slices, voxels and fov
------------------------------------------------------------------*/
extern double pro;                 /*fov position in read out*/
extern double ppe,ppe2,ppe3;       /*fov position in phase encode*/
extern double pos1,pos2,pos3;      /*voxel position*/
extern double pss[MAXSLICE];       /*slice position array*/

extern double lro;                 /*read out fov*/
extern double lpe,lpe2,lpe3;       /*phase encode fov*/

extern double vox1,vox2,vox3;      /*voxel size*/
extern double thk;                 /*slice or slab thickness*/



/*------------------------------------------------------------------
    BANDWIDTHS
      sw is already a standard SISCO parameter
------------------------------------------------------------------*/
extern double sw1,sw2,sw3;         /*phase encode bandwidths*/



/*------------------------------------------------------------------
    REGISTRATION AND ORIENTATION PARAMETERS
      Control of slice and voxel orientation. Registration of
      sample orientation in the magnet.
------------------------------------------------------------------*/
extern char orient[MAXSTR];        /*slice orientation*/
extern char lreg[MAXSTR];          /*subject-magnet orientation*/
extern char treg[MAXSTR];          /*subject-cradle orientation*/

extern double psi,phi,theta;       /*Euler angles for slices*/
extern double psi1,phi1,theta1;    /*Euler angles for voxels*/



/*------------------------------------------------------------------
    COUNTS AND FLAGS
      np & nf are already standard SISCO parameters
------------------------------------------------------------------*/
extern double nD;                  /*experiment dimensionality*/
extern double ns;                  /*number of slices*/
extern double ne;                  /*number of echoes*/
extern double nv,nv2,nv3;          /*number of phase encode views*/
extern double ni;                  /*number of standard increments*/
extern double ssc;                 /*compressed ss transients*/

extern char ir[MAXSTR];            /*inversion recovery flag*/
extern char ws[MAXSTR];            /*water suppression flag*/
extern char pilot[MAXSTR];         /*auto gradient balance flag*/
extern char seqcon[MAXSTR];        /*acquisition loop control flag*/
extern char table[MAXSTR];         /*table name for table-driven PE*/
extern char acqtype[MAXSTR];       /*e.g. "full" or "half" echo*/
extern char exptype[MAXSTR];       /*e.g. "se" or "fid" in CSI*/
extern char seqfil[MAXSTR];        /*pulse sequence name*/
extern char rfspoil[MAXSTR];       /*rf spoiling flag*/


/*------------------------------------------------------------------
    SISCO Imaging Variables
------------------------------------------------------------------*/
/* --- imaging frequency offsets --- */
extern double  slcto;           /* slice selection offset */
extern double  resto;           /* resonance frequency offset */
extern double  delto;           /* slice spacing frequency */
extern double  tox;             /* Transmitter Offset */
extern double  toy;             /* Transmitter Offset */
extern double  toz;             /* Transmitter Offset */

extern double  p2;              /* Transmitter pulse */

extern double  tpwr1;           /* Transmitter pulse power */
extern double  tpwr2;           /* Transmitter pulse power */
extern double  tpwr3;           /* Transmitter pulse power */
extern double  ticks;           /* external trigger counter */

/* --- gradient parameters --- */
extern double  gro;             /* read out gradient strength */
extern double  gpe;             /* phase encoding gradient step size */
extern double  gss;             /* slice selection gradient strength */
extern double  gx;              /*  gradient strength */
extern double  gy;              /*  gradient strength */
extern double  gz;              /*  gradient strength */
extern double  griserate;       /* Gradient riserate  */

#undef extern
