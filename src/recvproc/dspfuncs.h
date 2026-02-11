/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dspfuncs.h - DSP functions */
/*
*/
/*
 */

#ifndef INCdspfuncsh
#define INCdspfuncsh

#include "shrexpinfo.h"

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

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern int  initDSP(SHR_EXP_INFO ExpInfo);
extern int  dspExec(char *dataPtr, char *outPtr, unsigned int np,
                    unsigned int fidsize, unsigned int ct);
 
#ifdef __cplusplus
}
#endif

#endif

