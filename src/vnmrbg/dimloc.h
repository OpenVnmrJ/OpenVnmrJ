/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***/
/* dimloc.h: useful definitions for array locations
 *           associated with dimensions of ND-data.
 ***/

#ifndef __dimloc_h

#define __dimloc_h

/***/
/* The following codes are used to refer to dimension-specific information
 * stored in elements of an array:
 ***/

#define  XLOC        0
#define  YLOC        1
#define  ZLOC        2
#define  ALOC        3
#define  MAXDIM      4

/***/
/* The following codes are used to refer to the dimensions of the data;
 * the "CUR" and "ABS" codes are guaranteed to be 1,2,3 etc.
 ***/

#define BAD_DIM   -666
#define NULL_DIM     0
#define CUR_XDIM     1   
#define CUR_YDIM     2
#define CUR_ZDIM     3
#define CUR_ADIM     4

#define ABS_XDIM    -1
#define ABS_YDIM    -2
#define ABS_ZDIM    -3
#define ABS_ADIM    -4

#define CUR_HDIM     9
#define CUR_VDIM    10

/***/
/* Definitions for locations of Real and Imaginary Channels in ND Data Lists:
 ***/

#ifdef USE_CHAN_DEFS

#define R_CHAN       0
#define I_CHAN       1

#define RR_CHAN      0
#define IR_CHAN      1
#define RI_CHAN      2
#define II_CHAN      3

#define RRR_CHAN     0
#define IRR_CHAN     1
#define RIR_CHAN     2
#define IIR_CHAN     3
#define RRI_CHAN     4
#define IRI_CHAN     5
#define RII_CHAN     6
#define III_CHAN     7

#define RRRR_CHAN    0
#define IRRR_CHAN    1
#define RIRR_CHAN    2
#define IIRR_CHAN    3
#define RRIR_CHAN    4
#define IRIR_CHAN    5
#define RIIR_CHAN    6
#define IIIR_CHAN    7
#define RRRI_CHAN    8
#define IRRI_CHAN    9
#define RIRI_CHAN   10
#define IIRI_CHAN   11
#define RRII_CHAN   12
#define IRII_CHAN   13
#define RIII_CHAN   14
#define IIII_CHAN   15

#define MAXCHAN     16           /* Maximum Real/Imaginary Channels.         */

#endif

#endif /* __dimloc_h */
