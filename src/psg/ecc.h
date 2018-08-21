/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

#define MAXTAUS 10		/* Max ECC time constants per term */
#if defined (NVPSG) || defined (VNMRJ)
#define N1 4			/* Number of axes affecting ECC;
				 * also, number of ECC DSP chips */
#else
#define N1 3			/* Number of axes affecting ECC;
				 * also, number of ECC DSP chips */
#endif
#define N2 4			/* Number of axes corrected;
				 * also, number of term types per chip */
/* SDAC hardware info */
#define SLEWTOP 5000.0		/* mV */
#define SLEWMIN 29.75		/* us */
#define SLEWMAX 1573.0		/* us */
#define SLEWOFFSET (SLEWTOP / SLEWMAX)	/* mV / us */
/* SLEWSLOPE:  (mV / us / dacunit) */
#define SLEWSLOPE ((SLEWTOP - SLEWOFFSET*SLEWMIN) / (255 * SLEWMIN))

#define X1_AXIS 0
#define Y1_AXIS 1
#define Z1_AXIS 2
#define B0_AXIS 3

/*
 * A N1xN2 matrix of Ecc structs holds the time constants and amplitudes
 * of all the ECC values.  The matrix element ecc[0][1] holds the
 * compensation terms for the y-axis (axis 1) due to the x-axis (axis 0).
 * Axes: 0=X1, 1=Y1, 2=Z1, 3=B0 (=Z0)
 */
typedef struct{
    int nterms;
    float tau[MAXTAUS];
    float amp[MAXTAUS];
} Ecc;

#define ECCSCALE 0
#define SHIMSCALE 1
#define TOTALSCALE 2
#define SLEWLIMIT 3
#define DUTYLIMIT 4
#define N_SDAC_VALS 5

typedef struct{
    char label[16];
    float max_val;
    float max_ival;
    float scale_ival;
    int funcadr;
    float values[4];
} Sdac;

