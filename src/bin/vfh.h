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

/*  Taken from "data.h" which was not used because
    it references entry points from "data.c".		*/

#define S_DATA  1        /* 0 = no data    1 = data there        */
#define S_SPEC  2        /* 0 = fid        1 = spectrum          */
#define S_32    4        /* 0 = 16 bit     1 = 32 bit            */
#define S_FLOAT 8        /* 0 = integer    1 = floating point    */
#define S_SECND 16       /* 0 = first ft   1 = second ft         */
#define S_ABSVAL 32      /* 0 = not absval 1 = absolute value    */
#define S_COMPLEX 64     /* 0 = not complex 1 = complex          */
#define S_ACQPAR 128     /* 0 = not acqpar 1 = acq. params       */

/*  These definitions MUST match those in $vnmrsystem/sysvnmr/data.c	*/

struct file_header {
	int	nblock;
	int	ntrace;
	int	npts;
	int	ebytes;
	int	tbytes;
	int	bbytes;
	short	transf;
	short	status;
	int	spare1;
};

struct block_header {
	short	iscal;
	short	status;
	short	index;
	short	spare3;
	int	ct;
	float	lpval;
	float	rpval;
	float	lvl;
	float	tlt;
};
