/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDDEFS_H
#define SOLIDDEFS_H

//=====================================
// Spectrometer constants
//=====================================
#define TWOPI (2.0 * 3.14159265358979323846) 
#define DTCK 12.50e-9
#define NMIN 16.0                   //  Time Step - Usually Overridden By s.n90. 

#define PSD 8.0                    //  Phase Step Divsor (VNMRS 1, VNMRS2 8)
#define PTC 8.0                     //  Phase Timing Compensation
#define DPH  (PTC*360.0/(PSD*8192)) //  Phase Step (continue using VNMRS step to avoid excess elements)
                  
#define WSD 64.0                    //  Waveform Amp Step Divsor (VNMRS 4, VNMRS2 64)
#define WTC 64.0*0.87               //  Waveform AMP Timing Compensation (0.87*64.0 avoids integer steps)
#define DWFM (WTC*1023.0/(WSD*1024.0))  //  Waveform Amp Step (continue using VNMRS step to avoid excess elements)

#define FSD 16.0                    //  Fine Amp Step Divsor (VNMRS 1, VNMRS2 16)
#define FTC 16.0*0.87               //  Fine AMP Timing Compensation (0.87*16.0 avoids integer steps)
#define DFP (FTC*4095.0/(FSD*4096.0))   //  Fine Amp Step (continue using VNMRS step to avoid excess elements)

#define N90M 5                      //  Alt N90 Multiplier for VNMRS (1 to 10) (see below for primary)
#define N90 2                       //  Alt N90 for VNMRS (50 ns is 4) (see below for primary)
#define TRAP 2                      //  Alt N90 Flag for userDECShape (No is 0, Yes is 1, Yes+Print is 2)
                                    //  (see below for primary)

//======================================
// Step Constants
//======================================

#define VNMRSN90M 1                 //  N90 Multiplier for VNMRS (1 to 20)
#ifndef VNMRSN90
#define VNMRSN90 16                 //  N90 for VNMRS (50 ns is 4)
#endif
#define VNMRSTRAP 0                 //  N90 Flag for userDECShape (No is 0, Yes is 1, Yes+Print is 2)
#define INOVAN90M 1                 //  N90 Multiplier for INOVA (always 1 - no interpolation allowed)
#define INOVAN90 16                 //  N90 for INOVA (200 ns is 16 -required by wavegen)
#define INOVATRAP 0                 //  N90 Flag for userDECShape for INOVA (always 0 - no interpolation allowed)

//======================================
// Define sizes of structure members
//======================================

#define NTICK_CTR 8
#define NPW 32768
#define NCH 8
#define NPH_SUPER 1024
#define NPATTERN 512
#define NTAU    32

//======================================
// Common character array sizes
//======================================

#define NSUFFIX 256
#define PAR_NAME_SZ 256

//Contents:

// AR
// MODULE
// MPSEQ
// RAMP
// CP
// DREAM

//=======================
// Array Structure
//=======================

typedef struct {
    char a[20*PAR_NAME_SZ]; // A string with the arrayed parameter names
    int  b[20];        // Number of characters in each name
    int  c;            // Number of parameters
    int  d;            // Number of indicies
    int  e[20];        // arraysize for each parameter (not displayed)
    int  f[20];        // column index for each parameter
    int  g[20];        // array index for each column using "ix"
    int  i[20];        // hasarray bit for each column
    int  j[20];        // arraysize for each column
} AR;

//========================
// Module Structure
//========================

typedef struct {
   char pattern[NPATTERN];
   int filled; 
   int arrayindex;
   double t[4096];
}  MODULE;

//====================================
// DECLARE MPSEQ STRUCTURES
//====================================

//====================================
// MPSEQ: Multiple Pulse Sequence
//====================================

typedef struct {
   char   seqName[NSUFFIX];     //parameter group name
   char   ch[NCH];              //output channel
   char   pattern[NPATTERN];    //shape file name
   int    nelem;                //number of base elements to output
   double telem;                //duration of base element
   double t;                    //Output duration for _mpseq()
   double a;                    //Output amplitude for _mpseq()
   double phInt;                //extra phase accumulation
   double phAccum;              //phase accumulated due to offset
   int    iSuper;               //supercycle number
   int    nRec;                 //copy number
   int    preset1;              //preset the waveform generator (INOVA)
   int    preset2;              //separately clear the waveform generator (INOVA)
   double strtdelay;            //wavegen start delay (INOVA)
   double offstdelay;           //wavegen offset delay (INOVA)
   double apdelay;              //ap bus and wfg stop delay (INOVA)
   int    n90;                  //clock ticks in minimum step (INOVA 16)
   int    n90m;                 //min time intervals per step
   int    trap;                 //0 for average value, 2 for interpolated ramp.
   int    nphBase;              //number of elements in the base list
   int    nphSuper;             //number of elements in the supercycle list
   int    npw;                  //number of pulse widths
   int    nph;                  //number of phases
   int    no;                   //number of offsets
   int    na;                   //number of amplitudes
   int    ng;                   //number of gate states
   int    *n;                   //list of clock ticks for each pulse
   double *phSuper;             //list of supercycle phases
   double *pw;                  //list of input pulse widths
   double *phBase;              //list of input phases
   double *of;                  //list of input offsets
   double *aBase;               //list of input amplitudes
   double *gateBase;            //list of input gate states
   int     calc;
   int     hasArray;
   AR      array;
} MPSEQ;

//========================
// Tangent Ramp
//========================

typedef struct {
   char   seqName[NSUFFIX];      //parameter group name
   char   ch[NCH];               //output channel
   char   pattern[NPATTERN];     //shapefile name
   char   sh[2];                 //shape: "c", "l", "t"
   char   pol[3];                //polarity (n, ud, uu)
   double t;                     //duration of the sequence
   double a;                     //rf amplitude
   double d;                     //delta parameter of tangent shape -d to +d
   double b;                     //beta parameter of tangent shape
   double phInt;                 //internal phase accumulation bookkeeping
   double phAccum;               //phase accumulated due to offset
   int    nRec;                  //copy number
   int    preset1;               //preset the wavefrom generator (INOVA)
   int    preset2;               //separately clear the wavefrom generator (INOVA)
   double strtdelay;             //wavegen start delay (INOVA)
   double offstdelay;            //wavegen offset delay (INOVA)
   double apdelay;               //ap bus and wfg stop delay (INOVA)
   int    n90;                   //clock ticks consumed in min time interval
   int    n90m;                  //min time intervals per step
   int    trap;                  //0 for average value, 2 for interpolated ramp.
   int    n;                     //clock ticks consumed by the pulse
   double of;                    //frequency
   int    calc;
   int    hasArray;
   AR     array;
} RAMP;

//========
// CP
//========

typedef struct {
   char   seqName[NSUFFIX];  //parameter group name
   char  pattern[NPATTERN]; //shape file name
   char  sh[2];       //shape of cp, "c", "l", "t"
   char  ch[3];       //ramp channel "fr" or "to"
   char  fr[5];       //the source channel "obs", "dec", "dec2", "dec3"
   char  to[5];       //the destination channel "obs", "dec", "dec2", "dec3"
   double t;          //contact time in sec after initcp() or getval()
   double a1;         //fine power of the source channel; offset value if ramped
   double a2;         //fine power of the destination channel; offset if ramped
   double d;          //delta parameter of tangent shape -d to +d
   double b;          //beta parameter of tangent shape
   double phInt;      //internal phase bookkeeping
   double phAccum;    //phase accumulated during the shape
   int    nRec;       //copy number
   int    preset1;    //preset the wavefrom generator (INOVA)
   int    preset2;    //separately clear the wavefrom generator (INOVA)
   int    preset3;    //remove the constant amplitude set (INOVA)
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   int   n90;         //clock ticks in min time interval
   int   n90m;        //min time intervals per step
   int   trap;        //0 for average value, 2 for interpolated ramp.
   int   n;           //clock ticks in pulse
   double of;         //offset frequency
   int    calc;
   int    hasArray;
   AR     array;
} CP;

//========
// DREAM
//========

typedef struct {
   char   seqName[NSUFFIX];  //parameter group name
   char   ch[NCH];    //output channel
   double t;          //duration of the sequence
   double a;          //rf amplitude
   double d;          //delta parameter of tangent shape
   double b;          //beta parameter of tangent shape
   double phInt;      //internal phase accumulation bookkeeping
   double phAccum;    //phase accumulated due to offset
   int    nRec;       //copy number
   int    preset1;    //preset the wavefrom generator (INOVA)
   int    preset2;    //separately clear the wavefrom generator (INOVA)
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and fg stop delay (INOVA)
   double of;         //frequency
   int    n90;        //clock ticks in minimum time interval
   int    n90m;       //min time intervals per step
   int    trap;       //0 for average value, 2 for interpolated ramp.
   int   vcount;      //RT v-pointer to count DREAM shapes
   RAMP   Rdu;
   RAMP   Rud;
   RAMP   Ruu;
   RAMP   Rdd;
   int    calc;
   int    hasArray;
   AR     array;
} DREAM;

//=======================================
// CALCULATE GENERIC SHAPE FUNCTIONS
//=======================================

//=================
// State Structure
//=================

typedef struct {
   double a;            //amplitude
   double p;            //phase
   double g;            //gate
} STATE;

//============================
// Parameters for Shaped Pulse
//============================

typedef struct {
   char   seqName[NSUFFIX];  //Xc7,Yspc5 etc...
   char   ch[NCH];       //output channel
   char   pattern[NPATTERN]; //shape name
   int    nelem;         //number of shapes to output
   double t;             //duration of the sequence
   double a;             //rf amplitude
   double dp[64];         //generic params: doubles
   int    ip[64];         //generic params: ints
   char   flag1[3],flag2[3]; //generic flags
   double phInt;         //internal phase bookkeeping
   double phAccum;       //phase accumulated due to offset
   int    nRec;          //copy number
   int    preset1;       //preset the wavefrom generator (INOVA)
   int    preset2;       //separately clear the wavefrom generator (INOVA)
   double strtdelay;     //wavegen start delay (INOVA)
   double offstdelay;    //wavegen offset delay (INOVA)
   double apdelay;       //ap bus and wfg stop delay (INOVA)
   int    n;             //clock ticks consumed by the pulse
   int    n90;           //clock ticks consumed in min time interval
   int    n90m;          //min time intervals per step
   int    trap;          //0 for average value, 2 for interpolated ramp.
   double of;            //frequency
   int    calc;
   int    hasArray;
   AR     array;
} SHAPE_PARS;

//========================
// Shaped Pulse Structure
//========================

typedef struct
{
   STATE (*get_state)(double, SHAPE_PARS);  //pointer to shape calculation
   SHAPE_PARS pars;                         //shape parameters
} SHAPE;

#endif

