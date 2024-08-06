/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*      Revision 1.2  2006/08/29 20:43:22  mikem
*      Removed doubled defined functions and variables
*
*      Revision 1.1  2006/08/23 14:09:57  deans
*      *** empty log message ***
*
*      Revision 1.1  2006/08/22 23:30:02  deans
*      *** empty log message ***
*
*      Revision 1.5  2006/07/11 20:09:58  deans
*      Added explicit prototypes for getvalnowarn etc. to sglCommon.h
*      - these are also defined in  cpsg.h put can't #include that file because
*        gcc complains about the "extern C" statement which is only allowed
*        when compiling with g++ (at least for gcc version 3.4.5-64 bit)
*
*      Revision 1.4  2006/07/11 17:50:57  deans
*      mods to sgl
*      moved all globals to sglCommon.c
*
*      Revision 1.2  2006/07/07 01:10:37  mikem
*      modification to compile with psg
*
*********************************************************************/
#ifndef SGLCOMMON_H
#define SGLCOMMON_H

#ifndef DPS
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#endif

#include <standard.h>
#include "cps.h"

//#include <cpsg.h>
#ifndef __FUNCTION__
#define __FUNCTION__ (char*)__FUNCTION__
#endif

#define rotate_angle(psi,phi,theta) set_rotation_matrix(psi,phi,theta)
#define GDELAY 4e-6        /* hardware delay to deal with granularity of psg */

#define OFFSETDELAY 4e-6   /* hardware delay for a voffset command */
#define RFDELAY 100e-6     /* group delay - rf vs gradients in us*/
#define RFSPOILDELAY 50e-6 /* fixed delay for rf spoiling */
#define MAXNSAT 6          /* maximum number of sat bands */
#define DEG2RAD (0.0174532925199432957692) /* constant to convert degrees to radians */
#define TRES 1.0e-5        /* time granularity for te and tr */
#define PRECISION 1.0e-9   /* smallest expected numeric precision */
#define MS 1e-3            /* constant for 1 millisecond */
#define US 1e-6            /* constant for 1 microsecond */
#define NS 1e-9            /* constant for 1 nanosecond */
#define MAXNSLICE 256      /* maximum number of slices */
#define MAXNECHO 128       /* maximum number of echoes */
#define THREESIXTY 360.0   /* constant for degrees per cycle */
#define SQRT2 sqrt(2.0)    /* for oblique targets */
#define SQRT3 sqrt(3.0)    /* for oblique targets */
#define MAXTEMP 1024       /* for temporary array calculations */
#define MAXFREQS 32768     /* max number of acqs (nf) involving RF */
               			/* spoiling */
#define MIN_EVENT_TIME 50e-9


/* floating point comparison macros */
#define EPSILON 1e-9      /* largest allowable deviation due to floating */
               			/* point storage */
#define FP_LT(A,B) (((A) < (B)) && (fabs((double)(A) - (double)(B)) > EPSILON)) /* A less than B */
#define FP_GT(A,B) (((A) > (B)) && (fabs((double)(A) - (double)(B)) > EPSILON)) /* A greater than B */
#define FP_EQ(A,B) (fabs((double)(A) - (double)(B)) <= EPSILON)             /* A equal to B */

/* A not equal to B */
#define FP_NEQ(A,B) (!FP_EQ(A,B))

/* A greater than or equal to B */
#define FP_GTE(A,B) (FP_GT(A,B) || FP_EQ(A,B))

/* A less than or equal to B */
#define FP_LTE(A,B) (FP_LT(A,B) || FP_EQ(A,B))

/* custom floating point comparison macros */
/* these allow you to set EPSILON on a case by case basis */
#define FPC_LT(A,B,EPSILON_C) (((A) < (B)) && (fabs((double)(A) - (double)(B)) > (EPSILON_C))) /* A less than B */
#define FPC_GT(A,B,EPSILON_C) (((A) > (B)) && (fabs((double)(A) - (double)(B)) > (EPSILON_C))) /* A greater than B */
#define FPC_EQ(A,B,EPSILON_C) ((fabs((double)(A) - (double)(B))) <= (EPSILON_C))             /* A equal to B */

/* A not equal to B */
#define FPC_NEQ(A,B,EPSILON_C) (!FPC_EQ(A,B,EPSILON_C))

/* A greater than or equal to B */
#define FPC_GTE(A,B,EPSILON_C) (FPC_GT(A,B,EPSILON_C) || FPC_EQ(A,B,EPSILON_C))

/* A less than or equal to B */
#define FPC_LTE(A,B,EPSILON_C) (FPC_LT(A,B,EPSILON_C) || FPC_EQ(A,B,EPSILON_C))

#define MAX(A,B) ((A) > (B) ? (A):(B))  /* useful programming macro */
#define MIN(A,B) ((A) < (B) ? (A):(B))  /* useful programming macro */
#define SIGN(A)  (FP_LT((A), 0) ? -1 : 1) /* sign of a number */

#define MAX_STR  256          /* maximum characters per string */
/*#define GAMMA_H  4257.707747 */  /* gamma H [Hz/G] */
#define GAMMA_H nuc_gamma()

#define MM_TO_CM 0.1          /* conversion factor mm -> cm */
#define CM_TO_MM 10           /* conversion factor cm -> mm */
#define HZ_TO_MHZ 1e-6        /* convertion factor Hz -> MHz */
#define US_TO_S 1e-6          /* conversion factor us -> s */
#define S_TO_MS 1e3           /* conversion factor us -> s */
#define MAX_DAC_NUM 32767     /* maximum resolution of gradients */
#define MAX_DAC_PTS 255       /* maximum number of ticks per event */
#define FULL    1.0           /* Scaling factor for asymmetry */
#define MAX_BW  5e6           /* maximum bandwidth */
#define GRADIENT_RES 0.000004 /* Gradient resolution / granularity */
#define TIME_RES 0.0000002    /* Time / delay resolution / granularity */
#define PARAMETER_RESOLUTION  1e-8 /* resolution of input parameter */
#define MAX_GRAD_FORMS 20000    /* maximum number of waveforms */
#define FLIPLIMIT_LOW 135     /* Lower flip angle limit for RF inversion */
                  			/* bandwidth */
#define FLIPLIMIT_HIGH 225    /* Higher flip angle limit for RF inversion */
                  			/* bandwidth */
#define RF_UNBLANK rof1       /* RF amplifier unblank time */
#define RF_BLANK rof2	      /* RF amplifier blank time */		      

#define SHAPE_LIB_DIR "/shapelib/" /* directory that holds generated shapes */
#define USER_DIR   userdir
#define SYSTEMDIR  systemdir

#define RF_CAL_FILE "/pulsecal"
#define CATTN_MAX (79.0)       /* coarse attenuator */
#define FATTN_MAX (4095.0)     /* fine attenuator */

#define SIGMA 0.398942280376 /* used in gaussian wave generation */

#define GRAD_MAX gmax      /* maximum gradient strength */
#define T_RISE trise       /* time to reach gmax from zero */
#define ROLL_OUT getval("rollout") /* gets the rollout value from system */

#define MAX_SLEW_RATE ((double)GRAD_MAX / (double)T_RISE) /* maximum slew rate */

#define TRUE 1
#define FALSE 0

#define SGL_USERERROR 0  /* Only display errors in wrappers and user errors */
#define SGL_WARN      1  /* display warn message if sgl error */
#define SGL_ABORT     2  /* abort and display error message if sgl error */

#define DISPLAY_NONE  0      /* do not display structures */
#define DISPLAY_TOP_LEVEL 1  /* display only top level structures - slice, phase etc */
#define DISPLAY_MID_LEVEL 2  /* display top and mid level structures - down */
                 			/* to generic level */
#define DISPLAY_LOW_LEVEL 3  /* display all structures */

#define REFOCUS_GRADIENT_T  GENERIC_GRADIENT_T
#define PHASE_ENCODE_GRADIENT_T  GENERIC_GRADIENT_T
#define NULL_GRADIENT_T  GENERIC_GRADIENT_T

#define GRADIENT_KERNEL_T SGL_GRAD_NODE_T
#define GRADIENT_LIST_T SGL_GRAD_NODE_T


#define ROUND(A) (floor((A) + 0.5))

#define nucgamma  GAMMA_H         /* set nucgamma */
#define WRITE   TRUE              /* define writetodisk flag */
#define NOWRITE FALSE             /* define writetodisk flag */

#define DELAY(A) if((A)>=TIME_RES) (delay(A)) /*To avoid error messages when trying to play out short delays*/

#define MAXRFPARS 20  /* Maximum number of RF "*pars" parameters for SPL */

#define MAXMIR 64 /* Maximum number of MIR pulses in ASL (because malloc and S_getarray doesn't seem to work for DPS) */

/* Real time tables */
#define isTable(name_of_table)					\
   ((name_of_table >= t1) && (name_of_table <= t60))

/* Real time tables are 32-bit integer */
#define TABLEMAXVAL 2147483647

/* Parameter basic types */
#define T_UNDEF  0
#define T_REAL   1
#define T_STRING 2

/* Parameter sub types */
#define ST_UNDEF	0
#define ST_REAL 	1
#define ST_STRING	2
#define ST_DELAY	3
#define ST_FLAG 	4
#define ST_FREQUENCY	5
#define ST_PULSE	6
#define ST_INTEGER	7

/***********************************************************************/
/*********** type and variable declarations for SGL ********************/
/***********************************************************************/

/* ERRORS */
typedef enum
{
ERR_NO_ERROR            = 0,  /* No error present */
ERR_AMPLITUDE            = 1,  /* Gradient amplitude exceeds maximum allowed value */
ERR_GRAD_FORM_LIST      = 2,  /* Maximum number of waveform names exceeded */
ERR_SHAPE               = 3,  /* Invalid shape */
ERR_PARAMETER_REQUEST   = 4,  /* Input parameter(s) not supplied or invalid */
ERR_CALC_MAX            = 5,  /* Calculated gradient exceeds maximum gradient */
ERR_DURATION            = 6,  /* Duration not valid */ 
ERR_CALC_INVALID        = 7,  /* Invalid result of gradient calculation */
ERR_GRADIENT_OVERDETERMINED = 8,       /* Over-determined gradient */
ERR_SPECTRAL_WIDTH_OVERDETERMINED = 9, /* Spectral width overdetermined */
ERR_SPECTRAL_WIDTH_MAX  = 10,/* Spectral width exceeds maximum spectral width */
                   			/* of system */
ERR_RF_PULSE            = 11, /* RF-pulse not found */
ERR_GRANULARITY         = 12, /* Granularity violation - Timing does not adhere */
                   			/* to granularity */
ERR_SLEW_RATE           = 13, /* Slew rate violation or invalid slew rate */
ERR_START_VALUE         = 14, /* Illegal gradient start value */
ERR_FILE_OPEN           = 15, /* File open failed */
ERR_GRADIENT_PADDING    = 16, /* Invalid duration for gradient padding */
ERR_RESOLUTION          = 17, /* Resolution mismatch or invalid */
ERR_PADDING_LOCATION    = 18, /* Illegal location for gradient padding */
ERR_STEPS               = 19, /* Steps invalid */
ERR_TRIANGULAR_NEEDED   = 20, /* Triangular gradient required */
ERR_PLATEAU_MISSING     = 21, /* Waveform does not have plateau region */ 
ERR_RF_FRACTION         = 22, /* Invalid RF-fraction */
ERR_RF_HEADER_ENTRIES   = 23, /* Not all RF-pulse header entries could be found */
ERR_RF_SHAPE_MISSING    = 24, /* RF-pulse shape does not exist */
ERR_HEADER_MISSING      = 25, /* Not all shaped gradient header entries could be */
                   			/* located */
ERR_FILE_DELETE         = 26, /* Delete of shaped gradient file failed */
ERR_UNDEFINED_ACTION    = 27, /* Gradient administration function called with */
                   			/* undefined action */
ERR_FILE_1              = 28, /* Input file #1 doesn't exist */
ERR_EMPTY_SS_AMP        = 29, /* Slice select amplitude must be supplied */
ERR_MERGE_POINTS        = 30, /* Mismatch between header points entry and actual */
                   			/* points in file */
ERR_MERGE_SCALE         = 31, /* Wave form scaling exceeds maximum DAC units */
ERR_CALC_FLAG           = 32, /* Invalid calc flag */
ERR_MALLOC              = 33, /* Memory allocation error */
ERR_NO_SOLUTION         = 34, /* Solution cannot be found from input parameters */
ERR_INCONSISTENT_PARAMETERS = 35, /* Input parameters are inconsistent */
ERR_NUM_POINTS          = 36, /* Number of points not valid */ 
ERR_MAX_GRAD            = 37, /* Maximum Gradient magnitude is invalid */
ERR_LIBRARY_ERROR       = 38, /* A software error has occurred in the library */
ERR_BANDWIDTH           = 39, /* Bandwidth invalid */
ERR_FOV                 = 40, /* Field of view invalid */
ERR_INCREMENT           = 41, /* Increment invalid */
ERR_RF_BANDWIDTH        = 42, /* RF Bandwidth invalid */
ERR_ECHO_FRACTION       = 43, /* Invalid Echo-fraction */
ERR_RAMP_SHAPE          = 44, /* Invalid ramp shape */
ERR_RAMP_TIME           = 45, /* Invalid ramp time */ 
ERR_FILE_2              = 46, /* Input file #2 doesn't exist */
ERR_RF_DURATION         = 47, /* RF Duration not valid */ 
ERR_CR_AMPLITUDE        = 48, /* Crusher amplitude exceeds maximum allowed value */
ERR_CRUSHER_DURATION    = 49, /* Crusher duration not valid */ 
ERR_ZERO_PAD_DURATION   = 50, /* The new duration supplied is less than the waveform duration */ 
ERR_REMOVE_FILE         = 51,  /* Unable to remove file. */
ERR_GRANULARITY_TRIANGLE= 52, /* Granularity violation ofr triangle - Duration not evenly  */
                     			/* dividable by granularity */        
ERR_FLIP_ANGLE          = 53, /* Invalid flip angle - Flip angle less than 0 */				 
ERR_RF_CALIBRATION_FILE_MISSING = 54, /* RF calibration file does not exist */
ERR_RF_COIL             = 55, /* No entry for RF coil in RF calibration file */
ERR_RF_CAL_CASE         = 56, /* Unkown pulse calibration case */
ERR_RFPOWER_COARSE      = 57, /* RF power level - coarse exceeds maximum value */
ERR_RFPOWER_FINE        = 58, /* RF poweer level - fine exceeds maximum value */
ERR_BUTTEFLY_CRUSHER_1  = 59, /* RF amplifier unblank larger then butterfly crusher 1 duration */
ERR_BUTTEFLY_CRUSHER_2  = 60, /* RF amplifier blank larger then butterfly crusher 2 duration */
ERR_POLARITY            = 61, /* Negativer polarity not allowed */
ERR_PHASE_ENCODE_OFFSET = 62, /* only one offset parameter for phase offset allowed */
ERR_PARAMETER_NOT_USED_REFOCUS = 63,  /* Parameter not used in refocusing structure */
ERR_PARAMETER_NOT_USED_DEPHASE = 64,  /* Parameter not used in dephasing structure */
ERR_PARAMETER_NOT_USED_PHASE   = 65,  /* Parameter not used in phase encode structure */   
ERR_PARAMETER_NOT_USED_GENERIC = 66,  /* Parameter not used in generic structure */ 
ERR_RF_FLIP              = 67,         /* Invalid RF pulse flip angle */ 

	ERR_GRAD_PULSES				= 68, /* GRADIENT PULSES NOT DEFINED */
	ERR_GRAD_DURATION				= 69,
	ERR_DUTY_CYCLE					= 70,
ERR_VENC           =71,    /* Parameter invalid or 0 */ 
	ERR_CR_DURATION    =72,     /* Butterfly crusher duration too short */
			ERR_FLOWCOMP_CALC  =73,      /* No solution found for Flowcomp */
			ERR_FLOWCOMP_GARDIENT_SEPARATION = 74     /* Gradient separation error */
} ERROR_NUM_T;


typedef enum
{
LINEAR     = 0, /* shape internal to calcGeneric */
TRAPEZOID  = 1, /* trapezoidal gradient shape, can have linear, sine etc ramps */
SINE       = 2, /* half-sine shaped gradient */
RAMP       = 3, /* ramp up or down gradient shape */
PLATEAU    = 4, /* plateau gradient shape */
GAUSS      = 5, /* gaussian shape */
TRIANGULAR = 6  /* linear ramp up and linear ramp down */
} GRADIENT_SHAPE_T;

/* These should match the GRADIENT_SHAPE_T integers */
typedef enum
{
PRIMITIVE_LINEAR = 0, /* linear function */
PRIMITIVE_SINE   = 2, /* sine function */
PRIMITIVE_GAUSS  = 5  /* gauss function */
} PRIMITIVE_FUNCTION_T;

typedef enum
{
MOMENT_FROM_DURATION_AMPLITUDE = 0,	/* calculate 0th moment from duration and amplitude*/
AMPLITUDE_FROM_MOMENT_DURATION,		/* calculate amplitude from moment and duration */
DURATION_FROM_MOMENT_AMPLITUDE,		/* calculate duration from moment and amplitude */
SHORTEST_DURATION_FROM_MOMENT,		/* calculate shortest duration from moment */
MOMENT_FROM_DURATION_AMPLITUDE_RAMP, /* calculate 0th moment from duration, amplitude */
                            			/* and ramp time */
AMPLITUDE_FROM_MOMENT_DURATION_RAMP, /* calculate amplitude from 0th moment, duration */
                            			/* and ramp time */
DURATION_FROM_MOMENT_AMPLITUDE_RAMP, /* calculate duration from 0th moment, amplitude */
                            			/* and ramp time */
SHORTEST_DURATION_FROM_MOMENT_RAMP   /* calculate shortest duration from 0th moment */
                            			/* and ramp time */
} CALC_FLAG_T;

typedef enum
{
GENERIC_RAMP_UP = 0,	/* first primitive in a generic gradient */
GENERIC_PLATEAU,		/* second primitive in a generic gradient*/
GENERIC_RAMP_DOWN,	/* third primitive in a generic gradient */
} GRADIENT_PRIMITIVE_T;

typedef enum
{
INIT = 0,  /* zero the list */
ADD,       /* add gradient file to list */
REMOVE,    /* remove a gradient file from the list */
REMOVE_ALL /* remove all gradient files from the list */
} LIST_ACTION_T;

typedef enum
{
BACK = 0,	/* add padding after gradient waveform */
FRONT,		/* add padding before gradient waveform */
BOTH			/* add padding before and after gradient waveform, equally spaced */
} PAD_LOCATION_T;

typedef enum
{
VERSION     = 0,
TYPE        = 1,
MODULATION  = 2,
EXCITEWIDTH = 3,
INVERTWIDTH = 4,
INTEGRAL    = 5,
RF_FRACTION = 6,
RF_HEADER_ENTRIES  /* number of entries in RF pulse header */
} RF_PULSE_HEADER_ENTRIES_T;

typedef enum
{
NAME       = 0,
POINTS     = 1,
RESOLUTION = 2,
STRENGTH   = 3,
GRAD_HEADER_ENTRIES /* number of entries in gradient waveform header */
} GRADIENT_HEADER_ENTRIES_T; 

/* gradient types required for this modification of SGL
*
* SGL defines various types of gradient pulses. This version of SGL
* adds and extra element at the front of the structure definitions for
* the listed gradient pulse so that they can be gathered into a union of
* structs to enables simple handing of lists of gradient pulses. This is
* required for compound shapes and duty cycle functions
*/
typedef enum
{
	NULL_GRADIENT_TYPE			      = 0,
	GENERIC_GRADIENT_TYPE 		   = 1,
	REFOCUS_GRADIENT_TYPE		    = 2,
	DEPHASE_GRADIENT_TYPE		    = 3,
	PHASE_GRADIENT_TYPE			     = 4,
	SLICE_SELECT_GRADIENT_TYPE	= 5,
	READOUT_GRADIENT_TYPE		    = 6,
	FLOWCOMP_TYPE				          = 7,
	VENC_TYPE                  = 8,
	KERNEL_MARKER_TYPE           =9
} SGL_GRADIENT_TYPE_T;


/* required to specify polarity of gradient pulses included in
* compound shapes
*/
typedef enum
{
	PRESERVE = 0,	/* preserve polarity */
	INVERT			= 1  /* invert polarity */
} SGL_CHANGE_POLARITY_T;

/* required for the defining the logical axis for which a gradient pulse
* is to be applied in a compound shape
*/
typedef enum
{
	NO_DEBUG            = 0,
	PRINT_EVENT_TIMING  = 1,
} SGL_EVENT_DEBUG_T;

typedef enum
{
	NO_AXIS_DEFINED = 0,
	LOG_AXIS_MIN	= 1,
	MARKER		    = 1,	
	READ            = 2,
	READ_2			= 3,
	PHASE           = 4,
	PHASE_2			= 5,
	SLICE           = 6,
	SLICE_2			= 7,
	LOG_AXIS_MAX	= 7
} SGL_LOGICAL_AXIS_T;

typedef enum
{
	NO_GRAD_PLACEMENT_ACTION   = 0,
	START_TIME                 = 1,
	BEFORE                     = 2,
	BEHIND                     = 3,
	BEHIND_LAST				   = 4,
	SAME_START                 = 5,
	SAME_END                   = 6,
	NUM_GRAD_PLACEMENT_ACTIONS = 7
} SGL_GRAD_PLACEMENT_ACTION_T;

typedef enum
{
	NO_TIMING_ACTION           =  0,
	START_TIME_OF              =  1,
	END_TIME_OF                =  2,
	FROM                       =  3,
	FROM_START_OF              =  4,
	FROM_END_OF                =  5,
	FROM_RF_PULSE_OF           =  6,
	FROM_RF_CENTER_OF          =  7,
	FROM_START_OF_RF_PULSE_OF  =  8,
	FROM_END_OF_RF_PULSE_OF    =  9,
	FROM_ACQ_OF                = 10,
	FROM_ECHO_OF               = 11,
	FROM_START_OF_ACQ_OF       = 12,	
	FROM_END_OF_ACQ_OF         = 13,
	TO_START_OF                = 14,
	TO_END_OF                  = 15,
	TO                         = 16,
	TO_RF_PULSE_OF             = 17,
	TO_RF_CENTER_OF            = 18,
	TO_START_OF_RF_PULSE_OF    = 19,
	TO_END_OF_RF_PULSE_OF      = 20,
	TO_ACQ_OF                  = 21,
	TO_ECHO_OF                 = 22,	
	TO_START_OF_ACQ_OF         = 23,
	TO_END_OF_ACQ_OF           = 24,
	KERNEL_START               = 25,
	KERNEL_END                 = 26,
	NUM_TIMING_ACTIONS         = 27   
} SGL_TIMING_ACTION_T;

/* RF pulse types for SPL */
typedef enum {
  RF_NULL   = 0,
  RF_GAUSS  = 1,
  RF_HS     = 2,
  RF_HSAFP  = 3,
  RF_HTAHP  = 4,
  RF_HTBIR4 = 5,  
  RF_MAO    = 6,
  RF_SINC   = 7,
  RF_SINE   = 8
} RF_TYPE_T;

/* Diffusion types */
typedef enum {
  DIFF_NULL = 0,
  DIFF_GE   = 1,
  DIFF_SE   = 2,
  DIFF_STE  = 3
} DIFF_TYPE_T;

/******************************************************
* RF FRACTION:
*  0    ->  No RF fraction used 
*  0.5  ->  RF pulse duration (from center of pulse)
*  x    ->  RF fraction between 0 and 1
******************************************************/
typedef struct
{
double               endX;      /* ending X range of function */
                         	/* For startX and endX: */
                         	/*    - When function = LINEAR */
                         	/*      both are ignored */
                         	/*    - When function = SINE */
                         	/*      units are radians [rad] */
                         	/*    - When function = GAUSS */
                         	/*      both are set at fixed values by the */
                         	/*      library and the units are dimensionless */
PRIMITIVE_FUNCTION_T function;    /* math function, ie LINEAR, SINE etc */
double               powerFactor; /* power of function, ex: 2 for sin^2(t) */
double               startX;      /* starting X range of function */
} PRIMITIVE_SHAPE_T;

typedef struct
{
	SGL_GRADIENT_TYPE_T		type;
} KERNEL_MARKER_T;

typedef struct
{
SGL_GRADIENT_TYPE_T type;	/* SGL gradient type = 0 */ 
double *dataPoints;       /* pointer to waveform data */
double duration;          /* time [s] */
double endAmplitude;      /* ending amplitude [G/cm] */
                 /* Note: When shape.function is PRIMITIVE_SINE or */
               	/* PRIMITIVE_GAUSS and the value of endAmplitude */
                /*       is greater than startAmplitude, this variable */
                /*       should be interpreted as the amplitude of the */
                /*       first peak encountered. */
                /*       SINE example: */
                /*                a sine wave from 0 to 2 * PI */
                /*                startX = 0 and */
                /*                endX   = 2 * PI and */
                /*                startAmplitude = 0.5 and */
                /*                endAmplitude = 2 */
                /*                The waveform starts and ends at 0.5 G/cm */
                /*                and oscillates between a peak of 2 G/cm */
                /*                and -1 G/cm */
                /*       PRIMITIVE_GAUSS example: */
                /*                startAmplitude = 0.5 and */
                /*                endAmplitude = 2 */
                /*                The waveform starts and ends at 0.5 G/cm */
                /*                and approaches a peak of 2 G/cm */
                /*       When shape.function is PRIMITIVE_SINE or */
                /*       PRIMITIVE_GAUSS and the value of endAmplitude */
                /*       is lesser than startAmplitude, then startAmplitude */
                /*       is the amplitude of the first peak encountered. */
ERROR_NUM_T error;        /* error flag */
double maxGrad;           /* maximum permitted gradient strength [G/cm] */
double m0;                /* 0th moment - area of gradient [G/cm * s] */
double m1;                /* 1st moment of gradient [G/cm * s^2] */
char   name[MAX_STR];     /* waveform file name */
long   numPoints;         /* number of points in waveform */
double resolution;        /* time resolution of gradient [s] */
int    rollOut;           /* roll out flag */
PRIMITIVE_SHAPE_T shape;  /* gradient shape and related characteristics */
double slewRate;          /* [[G/cm] / s], default is maximum */
double startAmplitude;    /* starting amplitude [G/cm] */
int    writeToDisk;       /* if true, gradient waveform is written to disk */
} PRIMITIVE_GRADIENT_T;

typedef struct
{
SGL_GRADIENT_TYPE_T type;	/* SGL gradient type = 0 */ 
double amp;              /* absolute gradient strength [G/cm]  */
double peamp;            /* Amplitude of phase encoding gradient, without optional offset */
double offsetamp;        /* Optional offset in amplitude for phase encoding gradient */
double amplitudeOffset;  /* offset amplitude */
double areaOffset;       /* offset area (for 3D to combine phase end slice refocus) */
double balancingMoment0; /* 0th moment of slice select [G/cm * s] */
double gmult;            /* for trimming gradient amplitude, set to 1.0 in init */
double *dataPoints;      /* pointer to waveform data */
int    display;          /* display flag for structure display */
double duration;         /* duration of waveform [s]  */
double fov;              /* field of View in phase direction [mm]  */
double increment;        /* phase encode gradient increment [G/cm * s] */
double maxGrad;          /* maximum permitted gradient strength [G/cm] */
double m0;               /* 0th moment - area of gradient [G/cm * s] */
double m1;               /* 1st moment of gradient [G/cm * s^2] */
char   name[MAX_STR];    /* waveform file name  */   
int    noPlateau;        /* if true, calculations are done assuming no plateau region */
long   numPoints;        /* number of points in waveform  */
char   param1[MAX_STR];  /* VJ parameter name 1 */
char   param2[MAX_STR];  /* VJ parameter name 2 */
int    polarity;         /* flag to allow negative gradient */
double resolution;       /* resolution [s] */
int    rollOut;          /* roll out flag */
double slewRate;         /* [[G/cm] / s], default is maximum */
double startAmplitude;   /* starting gradient amplitude [G/cm], used for RAMP only  */
long   steps;            /* number of phase encoding steps */
double tramp;            /* duration of ramp [s] */
double gamma;            /* gyromagnetic ratio */
//   int    type;             /* struct type: 1=Refocus, 2=Phase, 3=Generic */
int    writeToDisk;      /* if true, gradient waveform is written to disk */

CALC_FLAG_T calcFlag;    /* determines how calculations will be done */
ERROR_NUM_T error;                /* error flag */
GRADIENT_SHAPE_T shape;           /* gradient shape */
PRIMITIVE_GRADIENT_T plateau;     /* flat portion between two ramps */
PRIMITIVE_FUNCTION_T rampShape;   /* shape of both ramps */
PRIMITIVE_GRADIENT_T ramp1;       /* ramp from zero to plateau */
PRIMITIVE_GRADIENT_T ramp2;       /* ramp from plateau to zero */
} GENERIC_GRADIENT_T;

typedef struct
{
double *dataPoints; /* pointer to waveform data */
long   numPoints;   /* number of points in waveform */
} MERGE_GRADIENT_T;

typedef struct
{
double *dataPoints;      /* pointer to pointer to waveform data */
ERROR_NUM_T error;       /* error flag */
PAD_LOCATION_T location; /* location in waveform to place zeros */
char   name[MAX_STR];    /* zero padded gradient name */
double newDuration;      /* total duration of waveform [s] */
long   numPoints;        /* pointer to number of points */
double resolution;       /* time resolution of gradient [s] */
int    rollOut;          /* roll out flag */
double timeToPad;        /* total duration of zero to be inserted in waveform, [s] */
int    writeToDisk;      /* if true, gradient waveform is written to disk */
} ZERO_FILL_GRADIENT_T;

typedef struct 
{
SGL_GRADIENT_TYPE_T type;	/* SGL gradient type = 0 */ 
double amp;              /* maximum absolute amplitude of waveform [G/cm] */
double *dataPoints;      /* pointer to waveform data */
double duration;         /* duration of slice select gradient [s] */
int    enableButterfly;  /* if TRUE, generates crushers around slice select */
ERROR_NUM_T error;       /* error flag */
double maxGrad;          /* maximum permitted gradient strength [G/cm] */
double m0;               /* 0th moment - area of gradient [G/cm * s] */
double m0def;			         /* 0th moment dephased from start to RF center [G/cm * s] */
double m0ref;            /* 0th moment to be refocused [G/cm * s] */
double m1;               /* 1st moment of gradient [G/cm * s^2] */
double m1ref;            /* 1st moment to be refocused [G/cm * s^2] */
char   name[MAX_STR];    /* gradient name */
long   numPoints;        /* number of points in waveform */
double pad1;             /* time to pad before rf pulse [s] */
double pad2;             /* time to pad after rf pulse [s] */
double plateauDuration;  /* plateau duration of slice select */
PRIMITIVE_FUNCTION_T rampShape; /* shape of both ramps */
double tramp;            /* duration of ramp [s] */
double resolution;       /* time resolution of gradient [s] */
double rfBandwidth;      /* RF pulse bandwidth [Hz] */
double rfCenterBack;     /* time from RF center to end of gradient */
double rfCenterFront;   	/* time from start of gradient to RF center */
double rfDelayFront;     /* time between start of gradient and start of RF pulse*/
double rfDelayBack;      /* time between end of RF pulse and end of gradient*/
double rfDuration;       /* duration of full RF pulse [ms] */
double rfFraction;       /* fraction (full) of rf pulse being used (for asymmetric pulses) */
char   rfName[MAX_STR];  /* name of rf pulse */
int    rollOut;          /* roll out flag */
double slewRate;         /* [[G/cm] / s], default is maximum */
double thickness;        /* slice thickness in [mm] */
double gamma;            /* gyromagnetic ratio */
int    writeToDisk;      /* if true, gradient waveform is written to disk */

/* first crusher */
double cr1amp;                        /* 1st crusher gradient amplitude [G/cm] */
CALC_FLAG_T crusher1CalcFlag;         /* determines how first crusher calculations will be done */
double crusher1Duration;              /* duration of plateau [s] */
double crusher1Moment0;               /* 0th moment [G/cm * s] */
double crusher1RampToCrusherDuration; /* duration of ramp from zero to crusher amplitude [s] */
double crusher1RampToSsDuration;      /* duration of ramp from crusher to slice select amplitude [s] */

/* slice select */
double ssamp;			 /* amplitude of slice select gradient */
double ssDuration;                    /* duration of slice select plateau */

/* second crusher */
double cr2amp;                        /* 2nd crusher gradient amplitude [G/cm] */
CALC_FLAG_T crusher2CalcFlag;         /* determines how second crusher calculations will be done */
double crusher2Duration;              /* duration of plateau [s] */
double crusher2Moment0;               /* 0th moment [G/cm * s] */
double crusher2RampToCrusherDuration; /* duration of ramp from slice select to crusher amplitude [s] */
double crusher2RampToSsDuration;      /* duration of ramp from crusher to zero amplitude [s] */
} SLICE_SELECT_GRADIENT_T;


typedef struct
{
SGL_GRADIENT_TYPE_T type;	/* SGL gradient type = 0 */ 
double acqTime;          /* acquisition time (AT) [s] */
double atDelayFront;     /* delay between start of waveform and start of acquisition */
double atDelayBack;      /* delay between end of acquisition and end of waveform */
double amp;              /* maximum absolute amplitude of waveform [G/cm] */
double bandwidth;        /* [Hz] */
double *dataPoints;      /* pointer to waveform data */
double duration;         /* duration of waveform [s] */
double echoFraction;     /* asymmetry factor for partial echo acquisition */
int    enableButterfly;  /* if TRUE, generates crushers around slice select */
ERROR_NUM_T error;       /* error flag */
double fov;              /* field of view in readout direction [mm] */
double fracAcqTime;      /* fractional acq time -> acqTime * echoFraction */
double maxGrad;          /* maximum permitted gradient strength [G/cm] */
double m0;               /* 0th moment - area of gradient [G/cm * s] */
double m0def;            /* 0th moment that accumulates from echo centre to gradient end [G/cm * s] */
double m0ref;            /* 0th moment that needs to be dephased [G/cm * s] */
double m1;               /* 1st moment of gradient [G/cm * s^2] */
double m1ref;            /* 1st moment that needs to be dephased [G/cm * s^2] */
char   name[MAX_STR];    /* waveform file name */
long   numPoints;        /* number of points in waveform */
long   numPointsFreq;    /* points (resolution) in frequency direction */
double pad1;             /* time to pad before echo [s] */
double pad2;             /* time to pad after echo [s] */
PRIMITIVE_FUNCTION_T rampShape; /* shape of both ramps */
double tramp;            /* ramp up and ramp down time if fixed duration is required [s] */
double timeFromEcho;       /* time between start of gradient and echo center */
double timeToEcho;       /* time between start of gradient and echo center */
double resolution;       /* time resolution of gradient [s] */
int    rollOut;          /* roll out flag */
double slewRate;         /* [[G/cm] / s], default is maximum */
double gamma;            /* gyromagnetic ratio */
int    writeToDisk;      /* if true, gradient waveform is written to disk */

/* first crusher */
double cr1amp;                        /* 1st crusher gradient amplitude [G/cm] */
CALC_FLAG_T crusher1CalcFlag;         /* determines how first crusher calculations will be done */
double crusher1Duration;              /* duration of plateau [s] */
double crusher1Moment0;               /* 0th moment [G/cm * s] */
double crusher1RampToCrusherDuration; /* duration of ramp from zero to crusher amplitude [s] */
double crusher1RampToSsDuration;      /* duration of ramp from crusher to slice select amplitude [s] */

/* slice select */
double plateauDuration;               /* duration of readout plateau */
double roamp;                         /* readout amplitude [G/cm] */

/* second crusher */
double cr2amp;                        /* 2nd crusher gradient amplitude [G/cm] */
CALC_FLAG_T crusher2CalcFlag;         /* determines how second crusher calculations will be done */
double crusher2Duration;              /* duration of plateau [s] */
double crusher2Moment0;               /* 0th moment [G/cm * s] */
double crusher2RampToCrusherDuration; /* duration of ramp from slice select to crusher amplitude [s] */
double crusher2RampToSsDuration;      /* duration of ramp from crusher to zero amplitude [s] */
} READOUT_GRADIENT_T;

typedef struct
{
double acqTime;      /* acquisition time [s] */
double bw;           /* spectral width [Hz] */
ERROR_NUM_T error;   /* error flag */
long   points;       /* points */
double readFraction; /* asymmetry factor 0.5 - 1.0 */
double resolution;   /* system timing resolution */
} BANDWIDTH_T;

typedef struct
{
double bandwidth;           /* RF pulse bandwidth */
double integral;            /* area (integral) of RF pulse */
double inversionBw;         /* RF pulse inversion bandwidth */
char   modulation[MAX_STR]; /* modulation of RF-pulse; amplitude, */
                   			/* phase, etc */
double rfFraction;          /* refocusing fraction of RF pulse */
char   type[MAX_STR];       /* type of RF-pulse; selective, */
                   			/* modulated, etc */
double version;             /* version number of pulse */
} RF_HEADER_T;

typedef struct
{
  RF_TYPE_T   type;               /* type of RF pulse shape */
  RF_HEADER_T header;             /* structure containing RF-pulse header */
  double      rfDuration;         /* RF-pulse duration */
  double      res;                /* RF-pulse resolution */
  double      flip;               /* RF-pulse flip angle */
  double      flipmult;           /* flip angle fractional multiplier */
  double      bandwidth;          /* used rfBandwidth */
  char        pulseBase[MAX_STR]; /* name of RF-pulse base, eg "p1" or "pmt" */
  char        pulseName[MAX_STR]; /* name of used RF-pulse (waveform) */
  char        shapeName[MAX_STR]; /* name of RF-pulse shape */
  char        rfcoil[MAX_STR];    /* name of rfcoil */
  double      powerCoarse;        /* Coarse RF-pulse power */
  double      powerFine;          /* Fine RF-pulse power */
  char        param1[MAX_STR];    /* name of coarse power parameter */
  char        param2[MAX_STR];    /* name of fine power parameter */
  double      rof1;               /* RF amplifier blank time */
  double      rof2;               /* RF amplifier unblank time */
  double      sar;                /* SAR - specific absorption ratio */
  int         pts;                /* number of RF points */
  double     *amp;                /* amplitude waveform */
  double     *freq;               /* frequency waveform */
  double     *phase;              /* phase waveform */
  int         npars;              /* number of "*pars" array elements */
  double      pars[MAXRFPARS];    /* the "*pars" parameter elements */
  int         lobes;              /* number of lobes */ 
  double      cutoff;             /* amplitude cuttoff, % */
  double      mu;                 /* mu (hyperbolic secants) */
  double      beta;               /* beta (hyperbolic secants) */
  double      modfrq;             /* modulation frequency (Hz) */
  int         display;            /* flag to display structure */
  ERROR_NUM_T error;              /* error flag */
} RF_PULSE_T;

typedef struct
{
  DIFF_TYPE_T type;               /* type of diffusion encoding (GE, SE or STE) */
  GENERIC_GRADIENT_T *grad;       /* diffusion gradient */
  double      gdiff;              /* diffusion gradient amplitude */
  double      delta;              /* diffusion delta (gradient duration) */
  double      DELTA;              /* diffusion DELTA */
  double      tadd;               /* additional diffusion encoding time */
  double      te;                 /* echo time for diffusion */
  char        minte;              /* minimum echo time flag for diffusion */
  double      tau1;               /* duration of events in first half of te for SE diffusion */
  double      tau2;               /* duration of events in second half of te for SE diffusion */
  double      tm;                 /* mixing time for STE diffusion */
  char        mintm;              /* minimum mixing time flag for STE diffusion */
  double      d1;                 /* d1,d2,d3,d4 are delays around the diffusion gradients. */
  double      d2;                 /* The scheme is [d1 - Gdiff - d2] - taudiff - [d3 - Gdiff - d4] */
  double      d3;
  double      d4;
  double      dm;                 /* diffusion mixing time delay */
  double     *dro;                /* multiplier for diffusion gradient in readout */
  double     *dpe;                /* multiplier for diffusion gradient in phase encode */
  double     *dsl;                /* multiplier for diffusion gradient in slice */
  int         nbval;              /* total number of bvalues*directions */
  int         nbro;               /* number of bvalues*directions along readout */
  int         nbpe;               /* number of bvalues*directions along phase encode */
  int         nbsl;               /* number of bvalues*directions along slice */
  double     *bro;                /* b value along readout */
  double     *bpe;                /* b value along phase encode */
  double     *bsl;                /* b value along slice */
  double     *brp;                /* b value cross-term (readout - phase encode) */
  double     *brs;                /* b value cross-term (readout - slice) */
  double     *bsp;                /* b value cross-term (slice - phase encode) */
  double     *btrace;             /* trace */
  double      max_bval;           /* the maximum trace */
  double      Time;               /* the duration of diffusion components */
} DIFFUSION_T;

typedef struct
{
char  name[MAX_GRAD_FORMS][MAX_STR]; /* name of waveform -> array */
short number;                        /* number of waveforms */
} GRAD_FORM_LIST_T;

//static GRAD_FORM_LIST_T gradList;

typedef struct GRAD_W_LIST
{
char  basename[MAX_STR];       /* general type of waveform */
char params[MAX_STR];          /* parameters which characterize the waveform */
char filename[MAX_STR];        /* name of file on disk */
struct GRAD_W_LIST *nextEntry;
} GRAD_WRITTEN_LIST_T;

//static GRAD_WRITTEN_LIST_T *gradWListP;

typedef struct
{
double amp;                 /* absolute gradient amplitude [G/cm] */
double *dataPoints;         /* pointer to waveform data */
ERROR_NUM_T error;          /* error flag */
double maxGrad;             /* maximum permitted gradient strength [G/cm] */
double m0;                  /* 0th moment - area of whole butterfly [G/cm * s] */
double m1;                  /* 1st moment of whole butterfly [G/cm * s^2] */
char   name[MAX_STR];       /* name of this butterfly */
long   numPoints;           /* number of points in waveform */
double resolution;          /* resolution [s] */
int    rollOut;             /* roll out flag */
double slewRate;            /* [[G/cm] / s], default is maximum */
double gamma;            /* gyromagnetic ratio */
int    writeToDisk;         /* if true, gradient waveform is written to disk */

/* first crusher */
double cr1amp;              /* 1st crusher gradient amplitude [G/cm] */
CALC_FLAG_T cr1CalcFlag;    /* determines how first crusher calculations will be done */
double cr1Moment0;          /* 0th moment [G/cm * s] */
double cr1TotalDuration;    /* duration of plateau [s] */

/* slice select */
double ssamp;               /* slice select gradient amplitude [G/cm] */
CALC_FLAG_T ssCalcFlag;     /* determines how slice select calculations will be done */
double ssMoment0;           /* 0th moment [G/cm * s] */
double ssTotalDuration;     /* duration of plateau [s] */

/* second crusher */
double cr2amp;              /* 2nd crusher gradient amplitude [G/cm] */
CALC_FLAG_T cr2CalcFlag;    /* determines how second crusher calculations will be done */
double cr2Moment0;          /* 0th moment [G/cm * s] */
double cr2TotalDuration;    /* duration of plateau [s] */

PRIMITIVE_GRADIENT_T ramp1; /* ramp from zero to first crusher */
PRIMITIVE_GRADIENT_T cr1;   /* first crusher plateau */
PRIMITIVE_GRADIENT_T ramp2; /* ramp from crusher plateau to slice select */
PRIMITIVE_GRADIENT_T ss;    /* slice select plateau */
PRIMITIVE_GRADIENT_T ramp3; /* ramp from slice select to second crusher */
PRIMITIVE_GRADIENT_T cr2;   /* second crusher plateau */
PRIMITIVE_GRADIENT_T ramp4; /* ramp from second crusher to zero */
} BUTTERFLY_GRADIENT_T;

typedef struct
{
ERROR_NUM_T error;    /* error flag */
char   name[MAX_STR]; /* name of waveform */
long   points;        /* points in waveform */
double resolution;    /* resolution [s] */
double strength;      /* gradient strength [G/cm] */
} GRADIENT_HEADER_T;

typedef struct 
{
SGL_GRADIENT_TYPE_T type;	/* SGL gradient type = 0 */ 
double amp;               /* absolute value of maxAmplitude [G/cm] */
double amplitude1;        /* gradient strength of 1st lobe [G/cm] */
double amplitude2;        /* gradient strength of 2nd lobe [G/cm] */
CALC_FLAG_T calcFlag;     /* determines how calculations will be done */
double *dataPoints;       /* pointer to waveform data */
double duration;          /* total duration of waveform (lobe 1 and 2) */
double duration1;         /* duration of 1st compensation lobe */
double duration2;         /* duration of 2nd compensation lobe */
double separation;        /* gradient lobe separation */
ERROR_NUM_T error;        /* error flag */
GENERIC_GRADIENT_T lobe1; /* waveform structure for 1st compensation lobe */
PRIMITIVE_GRADIENT_T plat; /* waveform strcture for separation between lobes */
GENERIC_GRADIENT_T lobe2; /* waveform structure for 2nd compensation lobe */
double maxGrad;           /* maximum permitted gradient strength [G/cm] */
char   name[MAX_STR];     /* waveform file name */
long   numPoints;         /* number of points in waveform */
double rampTime1;         /* ramp time of 1st compensation lobe */
double rampTime2;         /* ramp time of 2nd compensation lobe */
double resolution;        /* gradient granularity */
int    rollOut;           /* roll out flag */
int    writeToDisk;       /* if true, gradient waveform is written to disk */
double m0;
double m1;

} FLOWCOMP_T;

typedef struct {
double tblip;            /* minimum duration of blips */

/* Output Parameters */
double etl;
double np_flat, np_ramp;
double *dwell;           /* array of dwell times */  
double center_echo;
double duration, skip;
double amppos, ampneg;   /* RO Gradient amplitude during positive/negative lobes */
double amppe;            /* PE Gradient amplitude */
double gamma;            /* gyromagnetic ratio */
int    numPoints;
double ddrsr;
int    *table1,*table2;
double error;
} EPI_GRADIENT_T;

typedef struct {
double psi;                        /* Euler angle - psi - for satband */
double phi;                        /* Euler angle - phi - for satband */
double theta;                      /* Euler angle - theta - for satband */
double pos;                        /* Satband position */
SLICE_SELECT_GRADIENT_T slice;     /* satband slice select gradient */
} SATBAND_GRADIENT_T;

typedef struct
{
double satthk[MAXNSAT];        /* array of satband slice thickness [mm] */
double satpos[MAXNSAT];        /* array of satband positions [mm] */
double spsi[MAXNSAT];          /* array of satband orientatation angles */
double sphi[MAXNSAT];          /* array of satband orientatation angles */
double stheta[MAXNSAT];        /* array of satband orientatation angles */
double satamp[MAXNSAT];        /* array of satband gradient amplitudes  */
double satpat;                 /* satband RF pulse shape */
double flipsat;                /* satband RF pulse flip angle */
double psat;                   /* satband RF pulse length */
int    nsat;                   /* number of satbands */
} SATBAND_INFO_T;

/*
typedef struct
{
	union 
	{
		GENERIC_GRADIENT_T 		_as_GENERIC_GRADIENT;
		SLICE_SELECT_GRADIENT_T	_as_SLICE_SELECT_GRADIENT;
		READOUT_GRADIENT_T		_as_READOUT_GRADIENT;
		FLOWCOMP_T		       	_as_FLOWCOMP_GRADIENT;
	};
} SGL_GRADIENT_T;
*/

typedef struct
{
	char *name;
	double dur;
	double amp;
} SGL_KERNEL_INFO_T;

typedef union
{
		GENERIC_GRADIENT_T 		_as_GENERIC_GRADIENT;
		SLICE_SELECT_GRADIENT_T	_as_SLICE_SELECT_GRADIENT;
		READOUT_GRADIENT_T		_as_READOUT_GRADIENT;
		FLOWCOMP_T		       	_as_FLOWCOMP_GRADIENT;
		KERNEL_MARKER_T		_as_KERNEL_MARKER;
} SGL_GRADIENT_T;


struct SGL_GRAD_NODE_T
{
	struct SGL_GRAD_NODE_T* next;
	SGL_GRADIENT_T* grad;
	char *label;
//	char label[MAX_STR];
	int count;
	SGL_LOGICAL_AXIS_T logicalAxis;
	double 	startTime;
	SGL_GRAD_PLACEMENT_ACTION_T action;
	char *refLabel;
//	char refLabel[MAX_STR];
	double actionTime;
	SGL_CHANGE_POLARITY_T 		invert;
};

struct SGL_KERNEL_SUMMARY_T
{
	struct SGL_KERNEL_SUMMARY_T *next;
	double startTime;
	double endTime;;
	double amplitude;
	double moment;
};

struct SGL_LIST_T
{
	struct SGL_LIST_T *next;
};

typedef struct
{
	double tm11; double tm12; double tm13;
	double tm21; double tm22; double tm23;
	double tm31; double tm32; double tm33;
} SGL_EULER_MATRIX_T;

typedef struct
{
	double theta;
	double psi;
	double phi;
} SGL_EULER_ANGLES_T;

typedef struct
{
	double x;
	double y;
	double z;
} SGL_MEAN_SQUARE_CURRENTS_T;

struct _coilLimits
{
double xrms,  yrms,  zrms;
double current;
double flowrate;
int    tune;
double xduty, yduty, zduty;
};

extern struct _coilLimits coilLimits; /* required for duty cycle calculations */

typedef struct  /* as defined in variables.h */
{
  short  active; short  Dgroup; short  group;  short  basicType; short  size; short  subtype;
  short  Esize;  int    prot;   double minVal; double maxVal;    double step;
} VINFO_T;

typedef struct
{
  int   npars;    /* number of parameters in array string */
  char  **par;    /* the name of the parameter */
  int   *nvals;   /* the number of values of each parameter */
  int   *cycle;   /* how often each parameter cycles */
} ARRAYPARS_T;

/* function headers */
void adjustAmplitude(GENERIC_GRADIENT_T *gg);
void adjustCalcFlag( CALC_FLAG_T *flag);
double adjustCrusherAmplitude(double rampZeroToCrusher, double plateauTime,
                  			double rampCrusherToSliceSelect, double moment0,
                  			double ssAmplitude);
void amplitudeFromMomentDuration(GENERIC_GRADIENT_T *gg);
void amplitudeFromMomentDurationButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void amplitudeFromMomentDurationRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void calcBandwidth (BANDWIDTH_T *bw);
void calcButterfly(BUTTERFLY_GRADIENT_T *bfly);
void calcCrusher (GENERIC_GRADIENT_T *crusher);
void calcDephase (REFOCUS_GRADIENT_T *dephase);
void calcGeneric (GENERIC_GRADIENT_T *gg);
double calcMoment0Linear(double slew, double tramp,
             			double startAmplitude, double endAmplitude);
void calcPhase (PHASE_ENCODE_GRADIENT_T *phase);
void calcPower (RF_PULSE_T *rf_pulse, char rfcoil[MAX_STR]);
void calcPrimitive (PRIMITIVE_GRADIENT_T *prim);
void calcReadout (READOUT_GRADIENT_T *readout);
void calcReadoutFlowcomp(FLOWCOMP_T *flowcomp, READOUT_GRADIENT_T *readout);
void calcRefocus (REFOCUS_GRADIENT_T *refocus);
void calcSlice (SLICE_SELECT_GRADIENT_T *slice, RF_PULSE_T *rf);
void calcSliceFlowcomp(FLOWCOMP_T *flowcomp, SLICE_SELECT_GRADIENT_T *slice);
void calculateMoments (double *dataIn, long numPoints, long startIndex,
           			double startAmplitude, double resolution,
           			double *moment0, double *moment1,
           			double startDelta, double endDelta);
void checkForFilename (char *name, char *path);
ERROR_NUM_T checkSlewRate(double *dataIn, long numPoints, double startAmplitude,
              			double resolution);
void concatenateGradients (double *dataIn1, long numPoints1,
               			double *dataIn2, long numPoints2,
               			MERGE_GRADIENT_T * mg);
double crusherIdealMoment(double duration, double crAmplitude,
              			double slewRate, double ssAmplitude);
int deleteFile (char *name);
void displayBandwidth (BANDWIDTH_T *bw);
void displayButterfly (BUTTERFLY_GRADIENT_T *bfly);
void displayDephase (REFOCUS_GRADIENT_T *dephase);
void displayError (ERROR_NUM_T error, char *file, char *function, long lineNum);
void displayFlowcompContents (FLOWCOMP_T *flowcomp);
void displayGeneric (GENERIC_GRADIENT_T *generic);
void displayGenericContents (GENERIC_GRADIENT_T *generic);
void displayGenericPrimitive (PRIMITIVE_GRADIENT_T *prim, GRADIENT_PRIMITIVE_T i);
void displayGradientHeader (GRADIENT_HEADER_T *gradientHeader);
void displayPhase (PHASE_ENCODE_GRADIENT_T *phase);
void displayPrimitive (PRIMITIVE_GRADIENT_T *prim);
void displayPrimitiveContents (PRIMITIVE_GRADIENT_T *prim);
void displayReadout (READOUT_GRADIENT_T *readout);
void displayReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void displayRefocus (REFOCUS_GRADIENT_T *refocus);
void displayRefocusContents (REFOCUS_GRADIENT_T *refocus);
void displayRf (RF_PULSE_T *pulse);
void displayRfHeader (RF_HEADER_T *header);
void displaySlice (SLICE_SELECT_GRADIENT_T *slice);
void displaySliceFlowcomp (FLOWCOMP_T *flowcomp);
void displayZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
void doubleToScientificString (double value, char *string);
void durationFromMoment(GENERIC_GRADIENT_T *gg);
void durationFromMomentButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void durationFromMomentRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
double findMax(double * dataPoints, long numPoints, double startAmplitude);
void forceRampTime( double *tramp);
void genericGauss (GENERIC_GRADIENT_T *gg);
void genericRamp (GENERIC_GRADIENT_T *gg);
void genericSine (GENERIC_GRADIENT_T *gg);
void getCalcFlagString (CALC_FLAG_T calcFlag, char *cfString);
void getFunction (PRIMITIVE_FUNCTION_T function, char *name);
void getGenericShape (GRADIENT_SHAPE_T shape, char *name);
void getPadLocation (PAD_LOCATION_T location, char *name);
void getTrueFalseString (int value, char *tfString);
int gradAdmin (char *name, LIST_ACTION_T action);
int gradShapeWritten (char *basename, char *params, char *filename);
double granularity (double duration, double resolution);
void initBandwidth (BANDWIDTH_T *bw);
void initButterfly(BUTTERFLY_GRADIENT_T *bfly);
void initDephase (REFOCUS_GRADIENT_T *dephase);
void initReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void initSliceFlowcomp (FLOWCOMP_T *flowcomp);
void initGeneric (GENERIC_GRADIENT_T *generic);
void initGradFormList (GRAD_FORM_LIST_T *list);
void initGradientHeader (GRADIENT_HEADER_T *header);
void initNull (NULL_GRADIENT_T *null_grad);
void initPhase (PHASE_ENCODE_GRADIENT_T *phase);
void initPrimitive (PRIMITIVE_GRADIENT_T *prim);
void initReadout (READOUT_GRADIENT_T *readout);
void initRefocus (REFOCUS_GRADIENT_T *refocus);
void initRf (RF_PULSE_T *rf);
void initRfHeader (RF_HEADER_T *rfHeader);
void initShape (PRIMITIVE_SHAPE_T *shape);
void initSliceSelect (SLICE_SELECT_GRADIENT_T *slice);
void init_structures();
void initZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
double mathFunction(PRIMITIVE_GRADIENT_T * prim, long i);
int mergeGradient (char *waveform1, char *waveform2,
       			char *waveformOut, ERROR_NUM_T error);
void momentFromDurationAmplitude(GENERIC_GRADIENT_T *gg);
void momentFromDurationAmplitudeButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void momentFromDurationAmplitudeRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void readGradientHeader(GRADIENT_HEADER_T *header, FILE *fpin);
void readRfPulse (RF_PULSE_T *rfPulse);
int removeFiles (void);
void retrieve_parameters();
void writeGradientHeader(GRADIENT_HEADER_T *header, FILE *fpout);
ERROR_NUM_T writeToDisk (double * dataPoints, long numPoints,
double startAmplitude, double resolution,
int rollOut, char * name);
void zeroData(double * dataPoints, long numPoints);

void zeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
double nuc_gamma();
void genRf(RF_PULSE_T *rf); /* Main SPL generation function */



/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*--------- E N D   O F   S G L    D E C L A R A T I O N  --------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/





/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>> START OF SECTTION TO REMOVE AFTER PSG IS FIXED <<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void t_adjustAmplitude(GENERIC_GRADIENT_T *gg);
void t_adjustCalcFlag( CALC_FLAG_T *flag);
double t_adjustCrusherAmplitude(double rampZeroToCrusher, double plateauTime,
                  			double rampCrusherToSliceSelect, double moment0,
                  			double ssAmplitude);
void t_amplitudeFromMomentDuration(GENERIC_GRADIENT_T *gg);
void t_amplitudeFromMomentDurationButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void t_amplitudeFromMomentDurationRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void t_calcBandwidth (BANDWIDTH_T *bw);
void t_calcButterfly(BUTTERFLY_GRADIENT_T *bfly);
void t_calcCrusher (GENERIC_GRADIENT_T *crusher);
void t_calcDephase (REFOCUS_GRADIENT_T *dephase);
void t_calcGeneric (GENERIC_GRADIENT_T *gg);
double t_calcMoment0Linear(double slew, double rampTime,
             			double startAmplitude, double endAmplitude);
void t_calcPhase (PHASE_ENCODE_GRADIENT_T *phase);
void t_calcPower (RF_PULSE_T *rf_pulse, char rfcoil[MAX_STR]);
void t_calcPrimitive (PRIMITIVE_GRADIENT_T *prim);
void t_calcReadout (READOUT_GRADIENT_T *readout);
void t_calcReadoutFlowcomp(FLOWCOMP_T *flowcomp, READOUT_GRADIENT_T *readout);
void t_calcRefocus (REFOCUS_GRADIENT_T *refocus);
void t_calcSlice (SLICE_SELECT_GRADIENT_T *slice, RF_PULSE_T *rf);
void t_calcSliceFlowcomp(FLOWCOMP_T *flowcomp, SLICE_SELECT_GRADIENT_T *slice);
void t_calculateMoments (double *dataIn, long numPoints, long startIndex,
           			double startAmplitude, double resolution,
           			double *moment0, double *moment1,
           			double startDelta, double endDelta);
void t_checkForFilename (char *name, char *path);
ERROR_NUM_T t_checkSlewRate(double *dataIn, long numPoints, double startAmplitude,
              			double resolution);
void t_concatenateGradients (double *dataIn1, long numPoints1,
               			double *dataIn2, long numPoints2,
               			MERGE_GRADIENT_T * mg);
double t_crusherIdealMoment(double duration, double crAmplitude,
              			double slewRate, double ssAmplitude);
int t_deleteFile (char *name);
void t_displayBandwidth (BANDWIDTH_T *bw);
void t_displayButterfly (BUTTERFLY_GRADIENT_T *bfly);
void t_displayDephase (REFOCUS_GRADIENT_T *dephase);
void t_displayError (ERROR_NUM_T error, char *file, char *function, long lineNum);
void t_displayFlowcompContents (FLOWCOMP_T *flowcomp);
void t_displayGeneric (GENERIC_GRADIENT_T *generic);
void t_displayGenericContents (GENERIC_GRADIENT_T *generic);
void t_displayGenericPrimitive (PRIMITIVE_GRADIENT_T *prim, GRADIENT_PRIMITIVE_T i);
void t_displayGradientHeader (GRADIENT_HEADER_T *gradientHeader);
void t_displayPhase (PHASE_ENCODE_GRADIENT_T *phase);
void t_displayPrimitive (PRIMITIVE_GRADIENT_T *prim);
void t_displayPrimitiveContents (PRIMITIVE_GRADIENT_T *prim);
void t_displayReadout (READOUT_GRADIENT_T *readout);
void t_displayReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void t_displayRefocus (REFOCUS_GRADIENT_T *refocus);
void t_displayRefocusContents (REFOCUS_GRADIENT_T *refocus);
void t_displayRf (RF_PULSE_T *pulse);
void t_displayRfHeader (RF_HEADER_T *header);
void t_displaySlice (SLICE_SELECT_GRADIENT_T *slice);
void t_displaySliceFlowcomp (FLOWCOMP_T *flowcomp);
void t_displayZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
void t_doubleToScientificString (double value, char *string);
void t_durationFromMoment(GENERIC_GRADIENT_T *gg);
void t_durationFromMomentButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void t_durationFromMomentRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
double t_findMax(double * dataPoints, long numPoints, double startAmplitude);
void t_forceRampTime( double *tramp);
void t_genericGauss (GENERIC_GRADIENT_T *gg);
void t_genericRamp (GENERIC_GRADIENT_T *gg);
void t_genericSine (GENERIC_GRADIENT_T *gg);
void t_getCalcFlagString (CALC_FLAG_T calcFlag, char *cfString);
void t_getFunction (PRIMITIVE_FUNCTION_T function, char *name);
void t_getGenericShape (GRADIENT_SHAPE_T shape, char *name);
void t_getPadLocation (PAD_LOCATION_T location, char *name);
void t_getTrueFalseString (int value, char *tfString);
int t_gradAdmin (char *name, LIST_ACTION_T action);
double t_granularity (double duration, double resolution);
void t_initBandwidth (BANDWIDTH_T *bw);
void t_initButterfly(BUTTERFLY_GRADIENT_T *bfly);
void t_initDephase (REFOCUS_GRADIENT_T *dephase);
void t_initReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void t_initSliceFlowcomp (FLOWCOMP_T *flowcomp);
void t_initGeneric (GENERIC_GRADIENT_T *generic);
void t_initGradFormList (GRAD_FORM_LIST_T *list);
void t_initGradientHeader (GRADIENT_HEADER_T *header);
void t_initNull (NULL_GRADIENT_T *null_grad);
void t_initPhase (PHASE_ENCODE_GRADIENT_T *phase);
void t_initPrimitive (PRIMITIVE_GRADIENT_T *prim);
void t_initReadout (READOUT_GRADIENT_T *readout);
void t_initRefocus (REFOCUS_GRADIENT_T *refocus);
void t_initRf (RF_PULSE_T *rf);
void t_initRfHeader (RF_HEADER_T *rfHeader);
void t_initShape (PRIMITIVE_SHAPE_T *shape);
void t_initSliceSelect (SLICE_SELECT_GRADIENT_T *slice);
void t_init_structures();
void t_initZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
double t_mathFunction(PRIMITIVE_GRADIENT_T * prim, long i);
int  t_mergeGradient (char *waveform1, char *waveform2,
          			char *waveformOut, ERROR_NUM_T error);
void t_momentFromDurationAmplitude(GENERIC_GRADIENT_T *gg);
void t_momentFromDurationAmplitudeButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void t_momentFromDurationAmplitudeRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void t_readGradientHeader(GRADIENT_HEADER_T *header, FILE *fpin);
void t_readRfPulse (RF_PULSE_T *rfPulse);
int  t_removeFiles (void);
void t_retrieve_parameters();
void t_writeGradientHeader(GRADIENT_HEADER_T *header, FILE *fpout);
ERROR_NUM_T t_writeToDisk (double * dataPoints, long numPoints,
             			double startAmplitude, double resolution,
             			int rollOut, char * name);
void t_zeroData(double * dataPoints, long numPoints);

void t_zeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
void t_get_parameters();
void t_init_mri();
double t_nuc_gamma();
void t_genRf(RF_PULSE_T *rf);


void x_adjustAmplitude(GENERIC_GRADIENT_T *gg);
void x_adjustCalcFlag( CALC_FLAG_T *flag);
double x_adjustCrusherAmplitude(double rampZeroToCrusher, double plateauTime,
                  			double rampCrusherToSliceSelect, double moment0,
                  			double ssAmplitude);
void x_amplitudeFromMomentDuration(GENERIC_GRADIENT_T *gg);
void x_amplitudeFromMomentDurationButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void x_amplitudeFromMomentDurationRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void x_calcBandwidth (BANDWIDTH_T *bw);
void x_calcButterfly(BUTTERFLY_GRADIENT_T *bfly);
void x_calcCrusher (GENERIC_GRADIENT_T *crusher);
void x_calcDephase (REFOCUS_GRADIENT_T *dephase);
void x_calcGeneric (GENERIC_GRADIENT_T *gg);
double x_calcMoment0Linear(double slew, double rampTime,
             			double startAmplitude, double endAmplitude);
void x_calcPhase (PHASE_ENCODE_GRADIENT_T *phase);
void x_calcPower (RF_PULSE_T *rf_pulse, char rfcoil[MAX_STR]);
void x_calcPrimitive (PRIMITIVE_GRADIENT_T *prim);
void x_calcReadout (READOUT_GRADIENT_T *readout);
void x_calcReadoutFlowcomp(FLOWCOMP_T *flowcomp, READOUT_GRADIENT_T *readout);
void x_calcRefocus (REFOCUS_GRADIENT_T *refocus);
void x_calcSlice (SLICE_SELECT_GRADIENT_T *slice, RF_PULSE_T *rf);
void x_calcSliceFlowcomp(FLOWCOMP_T *flowcomp, SLICE_SELECT_GRADIENT_T *slice);
void x_calculateMoments (double *dataIn, long numPoints, long startIndex,
           			double startAmplitude, double resolution,
           			double *moment0, double *moment1,
           			double startDelta, double endDelta);
void x_checkForFilename (char *name, char *path);
ERROR_NUM_T x_checkSlewRate(double *dataIn, long numPoints, double startAmplitude,
              			double resolution);
void x_concatenateGradients (double *dataIn1, long numPoints1,
               			double *dataIn2, long numPoints2,
               			MERGE_GRADIENT_T * mg);
double x_crusherIdealMoment(double duration, double crAmplitude,
              			double slewRate, double ssAmplitude);
int x_deleteFile (char *name);
void x_displayBandwidth (BANDWIDTH_T *bw);
void x_displayButterfly (BUTTERFLY_GRADIENT_T *bfly);
void x_displayDephase (REFOCUS_GRADIENT_T *dephase);
void x_displayError (ERROR_NUM_T error, char *file, char *function, long lineNum);
void x_displayFlowcompContents (FLOWCOMP_T *flowcomp);
void x_displayGeneric (GENERIC_GRADIENT_T *generic);
void x_displayGenericContents (GENERIC_GRADIENT_T *generic);
void x_displayGenericPrimitive (PRIMITIVE_GRADIENT_T *prim, GRADIENT_PRIMITIVE_T i);
void x_displayGradientHeader (GRADIENT_HEADER_T *gradientHeader);
void x_displayPhase (PHASE_ENCODE_GRADIENT_T *phase);
void x_displayPrimitive (PRIMITIVE_GRADIENT_T *prim);
void x_displayPrimitiveContents (PRIMITIVE_GRADIENT_T *prim);
void x_displayReadout (READOUT_GRADIENT_T *readout);
void x_displayReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void x_displayRefocus (REFOCUS_GRADIENT_T *refocus);
void x_displayRefocusContents (REFOCUS_GRADIENT_T *refocus);
void x_displayRf (RF_PULSE_T *pulse);
void x_displayRfHeader (RF_HEADER_T *header);
void x_displaySlice (SLICE_SELECT_GRADIENT_T *slice);
void x_displaySliceFlowcomp (FLOWCOMP_T *flowcomp);
void x_displayZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
void x_doubleToScientificString (double value, char *string);
void x_durationFromMoment(GENERIC_GRADIENT_T *gg);
void x_durationFromMomentButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void x_durationFromMomentRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
double x_findMax(double * dataPoints, long numPoints, double startAmplitude);
void x_forceRampTime( double *tramp);
void x_genericGauss (GENERIC_GRADIENT_T *gg);
void x_genericRamp (GENERIC_GRADIENT_T *gg);
void x_genericSine (GENERIC_GRADIENT_T *gg);
void x_getCalcFlagString (CALC_FLAG_T calcFlag, char *cfString);
void x_getFunction (PRIMITIVE_FUNCTION_T function, char *name);
void x_getGenericShape (GRADIENT_SHAPE_T shape, char *name);
void x_getPadLocation (PAD_LOCATION_T location, char *name);
void x_getTrueFalseString (int value, char *tfString);
int x_gradAdmin (char *name, LIST_ACTION_T action);
double x_granularity (double duration, double resolution);
void x_initBandwidth (BANDWIDTH_T *bw);
void x_initButterfly(BUTTERFLY_GRADIENT_T *bfly);
void x_initDephase (REFOCUS_GRADIENT_T *dephase);
void x_initReadoutFlowcomp (FLOWCOMP_T *flowcomp);
void x_initSliceFlowcomp (FLOWCOMP_T *flowcomp);
void x_initGeneric (GENERIC_GRADIENT_T *generic);
void x_initGradFormList (GRAD_FORM_LIST_T *list);
void x_initGradientHeader (GRADIENT_HEADER_T *header);
void x_initNull (NULL_GRADIENT_T *null_grad);
void x_initPhase (PHASE_ENCODE_GRADIENT_T *phase);
void x_initPrimitive (PRIMITIVE_GRADIENT_T *prim);
void x_initReadout (READOUT_GRADIENT_T *readout);
void x_initRefocus (REFOCUS_GRADIENT_T *refocus);
void x_initRf (RF_PULSE_T *rf);
void x_initRfHeader (RF_HEADER_T *rfHeader);
void x_initShape (PRIMITIVE_SHAPE_T *shape);
void x_initSliceSelect (SLICE_SELECT_GRADIENT_T *slice);
void x_init_structures();
void x_initZeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
double x_mathFunction(PRIMITIVE_GRADIENT_T * prim, long i);
int x_mergeGradient (char *waveform1, char *waveform2,
       			char *waveformOut, ERROR_NUM_T error);
void x_momentFromDurationAmplitude(GENERIC_GRADIENT_T *gg);
void x_momentFromDurationAmplitudeButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void x_momentFromDurationAmplitudeRampButterfly(BUTTERFLY_GRADIENT_T *bfly, int crusherNum);
void x_readGradientHeader(GRADIENT_HEADER_T *header, FILE *fpin);
void readRfPulse (RF_PULSE_T *rfPulse);
int x_removeFiles (void);
void x_retrieve_parameters();
void x_writeGradientHeader(GRADIENT_HEADER_T *header, FILE *fpout);
ERROR_NUM_T x_writeToDisk (double * dataPoints, long numPoints,
             			double startAmplitude, double resolution,
             			int rollOut, char * name);
void x_zeroData(double * dataPoints, long numPoints);

void x_zeroFillGradient (ZERO_FILL_GRADIENT_T *zg);
void x_get_parameters();
void x_init_mri();
double x_nuc_gamma();
void x_genRf(RF_PULSE_T *rf);

void sgl_abort_message(char *format, ...);
void t_sgl_abort_message(char *format, ...);
void x_sgl_abort_message(char *format, ...);

void sgl_error_check(int err_flag);
void t_sgl_error_check(int err_flag);
void x_sgl_error_check(int err_flag);

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*--- S T A R T   O F   P A R A M E T E R    D E C L A R A T I O N  ----*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/

extern SGL_EULER_MATRIX_T mainRotationMatrix;
extern SGL_MEAN_SQUARE_CURRENTS_T msCurrents;
extern SGL_MEAN_SQUARE_CURRENTS_T rmsCurrents;

extern SGL_EVENT_DEBUG_T	sglEventDebug;
extern struct SGL_GRAD_NODE_T  *cg; 		/* a list of gradient pulses */
extern struct SGL_GRAD_NODE_T  *gl; 		/* a list of gradient pulses */
extern struct SGL_GRAD_NODE_T  **workingList; 	/* a list of gradient pulses */
extern int gradEventsOverlap;

/* PDDacquire */
extern char     PDDacquire[MAXSTR];                /* PDD acquire flag: y = PDD switches around acquisition, n = PDD switches around transmition */

extern int      sglabort;
extern int      sgldisplay;
extern int      sglerror;
extern int      sglarray;
extern int      sglpower;                          /* flag to calculate power, or use getval params */
extern int      trisesqrt3;                        /* flag to determine if euler_test has increased trise */
extern int      trigger;                           /* trigger select */
extern double   arraydim;
extern double   flip1, flip2, flip3, flip4, flip5; /* RF pulse flip angles */
extern char     rfcoil[MAXSTR];                    /* RF-coil name */
extern double   b1max;                             /* Maximum B1 (Hz) for the rf coil */
extern char     profile[MAXSTR];                   /* profile flag: y = ON, n = OFF */
extern double   pe_steps, pe2_steps, pe3_steps;    /* number of phase encode steps; usually pe_stesp=nv*/
extern char     fc[MAXSTR];                        /* flowcomp flag: y = ON, n = OFF */
extern char     perewind[MAXSTR];                  /* Phase rewind flag : y = ON, n = OFF */
extern char     spoilflag[MAXSTR];                 /* spoiler flag  : y = ON, n = OFF */
extern double   echo_frac;                         /* echo fraction */
extern double   nseg;                              /* number of segments for multi shot experiment */
extern double   etl;                               /* Echo Train Length */
extern char     navigator[MAXSTR];                 /* navigator flag: y = ON, n = OFF */
extern double   glim;                              /* gradient limiting factor: % in VJ, fraction in SGL  */
extern double   glimpe;                            /* PE gradient limiting factor [%] */
extern double   slewlim;                           /* gradient slew rate limiting factor [%] */
extern double   ssc;                               /* Compressed Steady State scans */
extern double   ss;                                /* Steady State scans */

/* timing variables */
extern char     minte[MAXSTR];                     /* minimum TE flag: y = ON, n = OFF */
extern char     mintr[MAXSTR];                     /* minimum TR flag: y = ON, n = OFF */
extern char     minti[MAXSTR];                     /* minimum TI (inversion time) flag: y = ON, n = OFF */
extern char     mintm[MAXSTR];                     /* minimum TM (mixing time) flag: y = ON, n = OFF */
extern char     minesp[MAXSTR];                    /* minimum ESP (fsems echo spacing) */
extern char     spinecho[MAXSTR];                  /* spin echo flag: y = ON, n = OFF */
extern double   temin;                             /* minimum TE */
extern double   trmin;                             /* minimum TR */
extern double   timin;                             /* minimum TR */
extern double   tmmin;                             /* minimum TM */
extern double   tpemin;                            /* minimum duration of PE gradient */
extern double   espmin;                            /* minimum ESP */
extern double   esp;                               /* echo spacing */
extern double   tep;                               /* group delay - gradient*/
extern double   trimage;                           /* inter-image delay */
extern int      trtype;                            /* pack or distribute tr_delay */

/* Readout butterfly crushers */
extern double  gcrushro;
extern double  tcrushro;

/* pre pulse variables - FATSAT*/
extern char     fsat[MAXSTR],fsatpat[MAXSTR];	    /* Fat suppression flag, FATSAT pulse pattern*/
extern double   flipfsat;                          /* fatsat flip angle */
extern double   pfsat,fsatfrq;                     /* FATSAT duration, FATSAT frequency */
extern double   gcrushfs,tcrushfs;                 /* FATSAT crusher amplitude / duration */
extern double   fsatTime;                          /* duration of FATSAT segment */

/* pre pulse variables - MTC */
extern double   flipmt;                            /* MTC flip angle*/
extern double   mtfrq;                             /* MTC duration, MTC frequency */
extern double   gcrushmt,tcrushmt;                 /* MTC crusher amplitude / duration */
extern double   mtTime;                            /* duration of MTC segment */

/* pre pulse variables - SATBAND */
extern char     sat[MAXSTR];                       /* SATBAND flag */
extern double   flipsat;                           /* SATBAND pulse flip angle */    
extern double   satpos[MAXNSAT];                   /* SATBAND position */
extern double   satthk[MAXNSAT];                   /* SATBAND thickness*/   
extern double   satamp[MAXNSAT];                   /* SATBAND grad amplitudes */   
extern double   satpsi[MAXNSAT];                   /* SATBAND grad angle */   
extern double   satphi[MAXNSAT];                   /* SATBAND grad angle */   
extern double   sattheta[MAXNSAT];                 /* SATBAND grad angle */   
extern double   satTime;                           /* Duration of SATBAND segment */
extern int      nsat;                              /* Number of SATBANDS */
extern double   gcrushsat,tcrushsat;               /* SATBAND crusher amplitude / duration */

/* pre pulse variables RF-tagging */
extern double  wtag;                                /* spatial width of tag [cm] */
extern double  dtag;                                /* Sspatial separation of tag [cm] */
extern double  ptag ;                               /* Total duration of RF train [usec] */
extern char    tag[MAXSTR];                         /* tagging flag: y = ON, n = OFF */
extern char    tagpat[MAXSTR];                      /* tagging pulse pattern - for calibration */
extern int     tagdir;                              /* tag direction : 0-OFF, 1 - readout,
                                  			2-Phase, 3-Readout & Phase */
extern double  fliptag;                             /* tagging flip angle */
extern double  tagtime;                             /* duration of tagging segment */
extern double  gcrushtag, tcrushtag;                /* tagging crusher strength and duration*/
extern int     rfamp[1024];
extern int     ntag;                                /* number of tags */

/* pre pulse variables IR */
extern double  flipir;                              /* IR flip angle */
extern double  gcrushir;                            /* IR cruhser duration */
extern double  tcrushir;                            /* IR crusher strength */
extern double  irTime;                              /* Duration of IR segment */

/* mean nt and mean tr of arrayed values */
extern double ntmean;                               /* Mean nt of arrayed nt values */
extern double trmean;                               /* Mean tr of arrayed tr values */

/* Diffusion */
extern char    diff[MAXSTR];                        /* Diffusion flag */
extern double  dro, dpe, dsl;                       /* Multipliers onto diffusion gradients */
extern double  taudiff;                             /* Additional time beyond diffusion gradients to be included in DELTA */
extern double  droval,dpeval,dslval;                /* dro, dpe and dsl values for b-value calculation */

/* Water Suppression */
extern char    wspat[MAXSTR];                       /* RF shape */
extern double  pws, flipws;                         /* pulse width, flip angle */

/* EPI tweakers and others */
extern double  groa;                                /* EPI tweaker - readout */
extern double  grora;                               /* EPI tweaker - dephase */
extern double  image;                               /* EPI repetitions */
extern double  images;                              /* EPI repetitions */
extern double  ssepi;                               /* EPI readput steady state */
extern char    rampsamp[MAXSTR];                    /* ramp sampling */

/* phase encode order */
extern char    ky_order[MAXSTR];                    /* phase order flag : y = ON, n = OFF */
extern double  fract_ky;

/* velocity encoding */
extern double  venc;                                /* max encoded velocity [cm/s] */

/* Arterial Spin Labelling (ASL) */
extern char    asl[MAXSTR];                         /* ASL flag */
extern int     asltype;                             /* type of ASL: FAIR, STAR, PICORE, CASL */
extern char    aslplan[MAXSTR];                     /* ASL graphical planning flag */
extern int     asltag;                              /* ASL tag off/tag/control, 0,1,-1 */
extern char    asltagcoil[MAXSTR];                  /* flag for separate tag coil (requires 3rd RF channel) */
extern char    aslrfcoil[MAXSTR];                   /* tag coil name */
extern RF_PULSE_T asl_rf;                           /* ASL tagging pulse */
extern char    paslpat[MAXSTR];                     /* Pulsed ASL (PASL) pulse shape */
extern double  pasl;                                /* PASL pulse duration */
extern double  flipasl;                             /* PASL pulse flip */
extern double  pssAsl[MAXSLICE];                    /* Position of ASL tag pulse */
extern double  freqAsl[MAXSLICE];                   /* Frequency of ASL tag pulse */
extern char    asltagrev[MAXSTR];                   /* Flag to reverse ASL tag position w.r.t. imaging slices */
extern int     shapeAsl;                            /* Shape list ID for ASL tag pulse */
extern char    pcaslpat[MAXSTR];                    /* Continuous ASL (CASL) pulse shape */
extern double  pcasl;                               /* CASL pulse duration */
extern double  flipcasl;                            /* CASL pulse flip */
extern double  caslb1;                              /* B1 for CASL pulse */
extern char    caslctrl[MAXSTR];                    /* CASL control type (default or sine modulated) */
extern char    caslphaseramp[MAXSTR];               /* Flag to phase ramp CASL pulses (long pulses take time to phase ramp) */
extern int     aslphaseramp;                        /* Default flag to phase ramp ASL pulses */
extern char    starctrl[MAXSTR];                    /* STAR control type (default or double tag) */
extern RF_PULSE_T aslctrl_rf;                       /* ASL control pulse (if different to tag pulse) */
extern double  pssAslCtrl[MAXSLICE];                /* Position of ASL control pulse */
extern double  freqAslCtrl[MAXSLICE];               /* Frequency of ASL control pulse */
extern int     shapeAslCtrl;                        /* Shape list ID for ASL control pulse */
extern SLICE_SELECT_GRADIENT_T asl_grad;            /* ASL tag gradient */
extern double  aslthk;                              /* ASL tag slice thickness */
extern double  asladdthk;                           /* ASL additional tag slice thickness (FAIR) */
extern double  asltagthk;                           /* Input STAR/PICORE ASL tag thickness */
extern double  aslgap;                              /* ASL tag slice gap (to image slice) */
extern double  aslpos;                              /* ASL tag slice position */
extern double  aslpsi,aslphi,asltheta;              /* ASL tag slice orientation */
extern double  aslctrlthk;                          /* ASL control slice thickness */
extern double  aslctrlpos;                          /* ASL control slice position */
extern double  aslctrlpsi,aslctrlphi,aslctrltheta;  /* ASL control slice orientation */
extern double  caslgamp;                            /* Amplitude of CASL tag gradient (G/cm) */
extern GENERIC_GRADIENT_T aslspoil_grad;            /* ASL tag spoil gradient */
extern double  asltaggamp;                          /* ASL tag gradient amplitude */
extern double  gspoilasl;                           /* ASL tag spoil gradient amplitude */
extern double  tspoilasl;                           /* ASL tag spoil gradient duration */
extern double  aslti;                               /* ASL inflow time */
extern char    minaslti[MAXSTR];                    /* minimum ASL inflow time flag: y = ON, n = OFF */
extern double  aslti_delay;                         /* ASL inflow delay */
extern double  tauasl;                              /* Additional time beyond ASL component to be included in inflow time */
extern double  aslTime;                             /* ASL module duration */
extern double  slicetr;                             /* TR to next slice (multislice mode) */
extern char    minslicetr[MAXSTR];                  /* minimum TR to next slice (multislice mode) */
extern double  asltr_delay;                         /* ASL tr delay */

extern char    ips[MAXSTR];                         /* In Plane Saturation (IPS) flag */
extern char    ipsplan[MAXSTR];                     /* IPS graphical planning flag */
extern RF_PULSE_T ips_rf;                           /* IPS pulse */
extern char    ipspat[MAXSTR];                      /* IPS pulse shape */
extern double  pips;                                /* IPS pulse duration */
extern double  flipips;                             /* IPS pulse flip */
extern double  flipipsf;                            /* IPS pulse flip factor */
extern double  pssIps[MAXSLICE];                    /* Position of IPS pulse */
extern double  freqIps[MAXSLICE];                   /* Frequency of IPS pulse */
extern int     shapeIps;                            /* Shape list ID for IPS pulse */
extern int     nips;                                /* number of IPS pulses */
extern SLICE_SELECT_GRADIENT_T ips_grad;            /* IPS gradient */
extern double  ipsthk;                              /* IPS slice thickness */
extern double  ipsaddthk;                           /* IPS additional slice thickness */
extern double  ipspos;                              /* IPS slice position */
extern double  ipspsi,ipsphi,ipstheta;              /* IPS slice orientation */
extern GENERIC_GRADIENT_T ipsspoil_grad;            /* IPS spoil gradient */
extern double  gspoilips;                           /* IPS spoil gradient amplitude (input) */
extern double  ipsgamp;                             /* IPS spoil gradient amplitude (actual) */
extern double  tspoilips;                           /* IPS spoil gradient duration */
extern char    wetips[MAXSTR];                      /* WET IPS flag */
extern double  ipsTime;                             /* IPS module duration */

extern char    mir[MAXSTR];                         /* Multiple Inversion Recovery (MIR) flag */
extern RF_PULSE_T mir_rf;                           /* MIR pulse */
extern char    mirpat[MAXSTR];                      /* MIR pulse shape */
extern double  pmir;                                /* MIR pulse duration */
extern double  rofmir;                              /* MIR pulse rof */
extern double  flipmir;                             /* MIR pulse flip */
extern double  freqMir;                             /* Frequency of MIR pulse */
extern int     delayMir;                            /* Delay list ID for MIR pulses */
extern int     nmir;                                /* number of MIR pulses */
extern int     nmirq2;                              /* number of MIR pulses after Q2TIPS */
extern double  irduration;                          /* duration of single IR component */
extern double  mir_delay[MAXMIR];                   /* the delays to MIR pulses */
extern GENERIC_GRADIENT_T mirspoil_grad;            /* MIR spoil gradient */
extern double  gspoilmir;                           /* MIR spoil gradient amplitude */
extern double  tspoilmir;                           /* MIR spoil gradient duration */
extern char    autoirtime[MAXSTR];                  /* Auromatic MIR time calculation flag */

extern char    ps[MAXSTR];                          /* Pre Saturation (PS) flag */
extern char    psplan[MAXSTR];                      /* PS graphical planning flag */
extern RF_PULSE_T ps_rf;                            /* PS pulse */
extern char    pspat[MAXSTR];                       /* PS pulse shape */
extern double  pps;                                 /* PS pulse duration */
extern double  flipps;                              /* PS pulse flip */
extern double  flippsf;                             /* PS pulse flip factor */
extern double  pssPs[MAXSLICE];                     /* Position of PS pulse */
extern double  freqPs[MAXSLICE];                    /* Frequency of PS pulse */
extern int     shapePs;                             /* Shape list ID for PS pulse */
extern int     nps;                                 /* number of PS pulses */
extern SLICE_SELECT_GRADIENT_T ps_grad;             /* PS gradient */
extern double  psthk;                               /* PS slice thickness */
extern double  psaddthk;                            /* PS additional slice thickness */
extern double  pspos;                               /* PS slice position */
extern double  pspsi,psphi,pstheta;                 /* PS slice orientation */
extern GENERIC_GRADIENT_T psspoil_grad;             /* PS spoil gradient */
extern double  gspoilps;                            /* PS spoil gradient amplitude (input) */
extern double  psgamp;                              /* PS spoil gradient amplitude (actual) */
extern double  tspoilps;                            /* PS spoil gradient duration */
extern char    wetps[MAXSTR];                       /* WET PS flag */
extern double  psTime;                              /* PS module duration */

extern char    q2tips[MAXSTR];                      /* Q2TIPS flag */
extern char    q2plan[MAXSTR];                      /* Q2TIPS graphical planning flag */
extern RF_PULSE_T q2_rf;                            /* Q2TIPS pulse */
extern char    q2pat[MAXSTR];                       /* Q2TIPS pulse shape */
extern double  pq2;                                 /* Q2TIPS pulse duration */
extern double  flipq2;                              /* Q2TIPS pulse flip */
extern double  pssQ2[MAXSLICE];                     /* Position of Q2TIPS pulse */
extern double  freqQ2[MAXSLICE];                    /* Frequency of Q2TIPS pulse */
extern int     shapeQ2;                             /* Shape list ID for Q2TIPS pulse */
extern double  nq2;                                 /* number of Q2TIPS pulses */
extern SLICE_SELECT_GRADIENT_T q2_grad;             /* Q2TIPS gradient */
extern double  q2thk;                               /* Q2TIPS slice thickness */
extern double  q2pos;                               /* Q2TIPS slice position */
extern double  q2psi,q2phi,q2theta;                 /* Q2TIPS slice orientation (FAIR) */
extern GENERIC_GRADIENT_T q2spoil_grad;             /* Q2TIPS spoil gradient */
extern double  gspoilq2;                            /* Q2TIPS spoil gradient amplitude */
extern double  tspoilq2;                            /* Q2TIPS spoil gradient duration */

extern double  q2ti;                                /* ASL inflow time to Q2TIPS */
extern char    minq2ti[MAXSTR];                     /* minimum ASL inflow time to Q2TIPS flag: y = ON, n = OFF */
extern double  q2ti_delay;                          /* Q2TIPS inflow delay */
extern double  q2Time;                              /* Q2TIPS module duration */

extern char    vascsup[MAXSTR];                     /* Vascular suppression (VS) flag */
extern GENERIC_GRADIENT_T vs_grad;                  /* VS gradient */
extern double  gvs;                                 /* VS gradient amplitude */
extern double  tdeltavs;                            /* VS gradient delta */
extern double  bvalvs;                              /* VS b-value */
extern double  vsTime;                              /* VS module duration */

extern char    asltest[MAXSTR];                     /* ASL test parameter for test mode (asltestmode='y') */

/* fixed ramp time variable / flag */
extern double trampfixed;                           /* duration of fixed ramp for system with gradient */
                                                    /* and/or amplifier resonance.  If none existent or */
                                                    /* 0.0 no fixed ramp durations are enforced */

/* Gradient structures */
extern SATBAND_GRADIENT_T       satss_grad[6];      /* slice select - SATBAND array */   
extern SATBAND_INFO_T           satband_info;       /* arrayed satband values */   

extern SLICE_SELECT_GRADIENT_T  ss_grad;            /* slice select gradient (90) */ 
extern SLICE_SELECT_GRADIENT_T  ss2_grad;           /* slice select gradient (180) */  
extern SLICE_SELECT_GRADIENT_T  ssi_grad;           /* IR slice select gradient */  
extern SLICE_SELECT_GRADIENT_T  sat_grad;           /* satband slice select gradient */  

extern REFOCUS_GRADIENT_T       ssr_grad;           /* slice refocus gradient */
extern REFOCUS_GRADIENT_T       ror_grad;           /* dephase gradient */

extern PHASE_ENCODE_GRADIENT_T  pe_grad;            /* phase encode gradient */
extern PHASE_ENCODE_GRADIENT_T  per_grad;           /* phase rewind gradient */
extern PHASE_ENCODE_GRADIENT_T  pe2_grad;           /* phase encode gradient in 2nd dimension */
extern PHASE_ENCODE_GRADIENT_T  pe2r_grad;          /* phase rewind gradient in 2nd dimension */
extern PHASE_ENCODE_GRADIENT_T  pe3_grad;           /* phase encode gradient in 2nd dimension */
extern PHASE_ENCODE_GRADIENT_T  pe3r_grad;          /* phase rewind gradient in 2nd dimension */

extern READOUT_GRADIENT_T       ro_grad;            /* readout gradient */
extern READOUT_GRADIENT_T       nav_grad;           /* navigator gradient */

extern PHASE_ENCODE_GRADIENT_T  epipe_grad;         /* EPI phase encode gradient */
extern READOUT_GRADIENT_T       epiro_grad;         /* EPI readout gradient */
extern EPI_GRADIENT_T epi_grad;                     /* General EPI struct */

extern GENERIC_GRADIENT_T       fsatcrush_grad;     /* crusher gradient structure */
extern GENERIC_GRADIENT_T       mtcrush_grad;       /* crusher gradient structure */
extern GENERIC_GRADIENT_T       satcrush_grad;      /* crusher gradient structure */
extern GENERIC_GRADIENT_T       tagcrush_grad;      /* crusher gradient structure */
extern GENERIC_GRADIENT_T       tag_grad;           /* tagging gradient structure */
extern GENERIC_GRADIENT_T       crush_grad;         /* crusher gradient structure */
extern GENERIC_GRADIENT_T       spoil_grad;         /* spoiler gradient structure */
extern GENERIC_GRADIENT_T       diff_grad;          /* diffusion gradient structure */
extern GENERIC_GRADIENT_T       gg_grad;            /* generic gradient structure */
extern GENERIC_GRADIENT_T       ircrush_grad;       /* IR crusher gradient strucuture */
extern GENERIC_GRADIENT_T       navror_grad;        /* Navigator refocus gradient */

extern REFOCUS_GRADIENT_T ssd_grad;
extern REFOCUS_GRADIENT_T rod_grad;

extern NULL_GRADIENT_T          null_grad;          /* NULL gradient strucuture */

/* RF-pulse structures */
extern RF_PULSE_T    p1_rf;              /* RF pulse structure */
extern RF_PULSE_T    p2_rf;              
extern RF_PULSE_T    p3_rf;              
extern RF_PULSE_T    p4_rf;              
extern RF_PULSE_T    p5_rf;              
extern RF_PULSE_T    mt_rf;              /* MTC RF pulse structure */
extern RF_PULSE_T    fsat_rf;            /* fatsat RF pulse structure */
extern RF_PULSE_T    sat_rf;             /* SATBAND RF pulse structure */
extern RF_PULSE_T    tag_rf;             /* RF TAGGIGN pulse structure */
extern RF_PULSE_T    ir_rf;              /* IR RF-pulse */
extern RF_PULSE_T    ws_rf;              /* Water suppression */

extern DIFFUSION_T   diffusion;          /* Diffusion structure */

extern GRAD_FORM_LIST_T gradList;
extern GRAD_WRITTEN_LIST_T *gradWListP;

extern char checkduty[MAXSTR];
#endif
