/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**********************************************************************
 *
 * NAME:
 *    cmed.h
 *
 * DESCRIPTION:
 *    Header file including necessary constant and function definitions
 *    for the CMED family of Varian pulse sequences.  It is expected 
 *    that the contents of this file will be blended into Varian's psg
 *    build upon release to Varian's customers
 *
 * MODIFICATION HISTORY:
 *    Revision 2.19  2003/10/21 01:13:06  erickson
 *    Back to 4us resolution.  Increased number of waveforms.
 *
 *    Revision 2.18  2003/10/15 20:07:22  erickson
 *    Resolution to 5us.  Increased epsilon.
 *
 *    Revision 2.17  2003/10/10 05:10:55  erickson
 *    Commented floating point comparison macros.
 *
 *    Revision 2.16  2003/10/10 05:03:11  erickson
 *    Added floating point comparison macros.
 *
 *    Revision 2.15  2003/10/08 17:23:56  erickson
 *    Added ROUND macro.
 *
 *    Revision 2.14  2003/10/03 16:17:30  erickson
 *    Deleted two defines.
 *
 *    Revision 2.13  2003/09/29 20:26:00  erickson
 *    Merged in 2.10 changes.
 *
 *    Revision 2.12  2003/09/29 17:37:11  erickson
 *    Updated file header.
 *
 *********************************************************************/

#define OFFSETDELAY 4e-6   /* hardware delay for a voffset command */
#define RFDELAY 100e-6     /* group delay - rf vs gradients in us*/
#define RFSPOILDELAY 50e-6 /* fixed delay for rf spoiling */
#define MAXNSAT 6          /* maximum number of sat bands */
#define DEG2RAD .01745     /* constant to convert degrees to radians */
#define TRES 1.0e-5        /* time granularity for te and tr */
#define PRECISION 1.0e-9   /* smallest expected numeric precision */
#define MS 1e-3            /* constant for 1 millisecond */
#define US 1e-6            /* constant for 1 microsecond */
#define NS 1e-9            /* constant for 1 nanosecond */
#define MAXNSLICE 128      /* maximum number of slices */
#define MAXNECHO 128       /* maximum number of echoes */
#define THREESIXTY 360.0   /* constant for degrees per cycle */
#define SQRT2 sqrt(2.0)    /* for oblique targets */
#define SQRT3 sqrt(3.0)    /* for oblique targets */
#define MAXTEMP 1024       /* for temporary array calculations */
#define MAXFREQS 32768     /* max number of acqs (nf) involving RF */
                           /* spoiling */

#define MAX(A,B) ((A) > (B) ? (A):(B))  /* useful programming macro */
#define MIN(A,B) ((A) < (B) ? (A):(B))  /* useful programming macro */
#define SIGN(A)  ((A) < 0.0 ? -1.0:1.0) /* sign of a number */

#define MAXSTR  256           /* maximum characters per string */
#define GAMMA_H  4257.707747  /* gamma H [Hz/G] */
#define MM_TO_CM 0.1          /* conversion factor mm -> cm */
#define CM_TO_MM 10           /* conversion factor cm -> mm */
#define HZ_TO_MHZ 1e-6        /* convertion factor Hz -> MHz */
#define MAXDAC 32767          /* maximum resolution of gradients */
#define MAX_PTS 255           /* maximum number of ticks per event */
#define FULL    1.0           /* Scaling factor for asymmetry */
#define MAX_BW  500000        /* maximum bandwidth */
#define GRADIENT_RES 0.000004 /* Gradient resolution / granularity */
#define PARAMETER_RESOLUTION  1e-8 /* resolution of input parameter */
#define MAX_GRAD_FORMS 200    /* maximum number of waveforms */

/* enforces gradient granularity */
#define GRANULARITY(A,B) (ceil(floor(((A/B)* 100) + 0.5) / 100) * B)

#define ROUND(A) (floor(A + 0.5))

/* floating point comparison macros */
#define EPSILON 1e-14         /* largest allowable deviation due to floating */
                              /* point storage */
#define FP_LT(A,B) ((A < B) && (fabs(A - B) > EPSILON)) /* A less than B */
#define FP_GT(A,B) ((A > B) && (fabs(A - B) > EPSILON)) /* A greater than B */
#define FP_E(A,B)  (fabs(A - B) <= EPSILON)             /* A equal to B */

#define FP_NE(A,B) (!FP_E(A,B))               /* A not equal to B */
#define FP_GTE(A,B) (FP_GT(A,B) || FP_E(A,B)) /* A greater than or equal to B */
#define FP_LTE(A,B) (FP_LT(A,B) || FP_E(A,B)) /* A less than or equal to B */


/*********************************************************************
   Resolution limiting function for tr and te 

   This function limits the resolution of the incoming quantity to TRES
   as defined in cmed.h
**********************************************************************/
double limit_resolution(double in)
{
   double tmp, out;

   tmp = in/TRES;
   if (fabs(tmp - floor(tmp + 0.5)) > (PRECISION/TRES))
   {
      out = TRES*ceil(tmp);
   }
   else
   {
      out = TRES*floor(tmp + 0.5);
   }
   return(out);
}
