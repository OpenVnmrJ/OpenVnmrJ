/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "ftpar.h"

#define FT3D_VERSION	32	/* Vnmr VERSION occupies 1-31 */
#define MAXDIM		3


struct _regInfo
{
   int	stptzero;	/* real point count */
   int	stpt;		/* real point count */
   int	endpt;		/* real point count */
   int	endptzero;	/* real point count */
};

typedef struct _regInfo regInfo;

struct _sclrpar3D
{
   regInfo	reginfo;  /* information for processing an F3 region	    */
   ssparInfo	sspar;	  /* time-domain solvent subtraction parameters	    */
   float        phfid;    /* zero-order FID phase constant                  */
   float	lsfrq;	  /* left-shift of spectral frequencies in Hz	    */
   float	rp;	  /* zero-order spectral phase constant		    */
   float	lp;	  /* first-order spectral phase constant	    */
   float	lpval;	  /* first-order phase from digital filtering	    */
   int          np;       /* complex number of time-domain data points      */
   int          npadj;    /* adjusted number of complex time-domain points  */
   int          fn;       /* complex number of frequency-domain data points */
   int          lsfid;    /* number of left-shifted complex points          */
   int          pwr;      /* FT level                                       */
   int          zflvl;    /* level of zero-filling                          */
   int          zfnum;    /* number of explicit zeroes to append to FID     */
   int          proc;     /* type of data processing                        */
   int		ntype;	  /* flag for NTYPE negation of imaginary points    */
   int		fiddc;	  /* flag for FID DC correction			    */
   int		specdc;	  /* flag for spectral DC correction		    */
   int		wtflag;	  /* weighting flag				    */
   int		dsply;	  /* display mode				    */
   int		dcflag;   /* dc remove flag				    */
};

typedef struct _sclrpar3D sclrpar3D;

struct _pntrpar3D
{
   float        *wtv;     /* pointer to weighting vector                    */
   float        *phs;     /* pointer to phasing vector                      */
};

typedef struct _pntrpar3D pntrpar3D;

struct _dimenInfo
{
   sclrpar3D	scdata;
   lpstruct	parLPdata;
   pntrpar3D	ptdata;
};

typedef struct _dimenInfo dimenInfo;
 
struct _proc3DInfo
{
   dimenInfo    f3dim;
   dimenInfo    f1dim;
   dimenInfo    f2dim;
   int          vers;
   int          arraydim;
   int		datatype;
};
 
typedef struct _proc3DInfo proc3DInfo;

struct _procMode
{
   int	procf3;
   int	procf2;
   int	procf1;
};

typedef struct _procMode procMode;
