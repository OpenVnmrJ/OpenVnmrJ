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
#define DSP_DECFACTOR	0
#define DSP_NTAPS	1
#define DSP_NAME	2

#define OS_MAX_COEFF	50000
#define OS_MIN_COEFF	3

struct _dspInfo
{
   float	*filter;
   float	*chargeup;
   float	*buffer;
   float	*data;
   float	filtsum;
   float	oslsfrq;
   float	osfiltfactor;
   float	lvl;
   float	tlt;
   int		oscoeff;
   int		osfactor;
   int		offset;
   char		name[128];
   int		tshift;
};

typedef struct _dspInfo dspInfo;


