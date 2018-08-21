/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vnmrsys.h"
#include "group.h"
#include "ecc.h"
#include "tools.h"
#include "pvars.h"
#include "wjunk.h"

#ifndef VNMR
extern void vnmremsg(const char *paramstring);
#endif

extern int   nterms();

extern Sdac sdac_values[];

static const char *axisLabel[] = {"X", "Y", "Z", "B0"};

float get_decctool_shimscale(int axis)
{
	float dtshimscale;
	float dtmax_val;
	Sdac *pd;

	pd = &sdac_values[SHIMSCALE];
	dtmax_val = pd->max_val;
	dtshimscale = pd->values[axis];
/*
	printf("shimscale in get_decctool_shimscale for %d axis:	%4.3f \n", axis,dtshimscale);
	printf("max val from get_decctool_shimscale for %d axis:	%4.3f \n", axis,dtmax_val);
*/
	return dtshimscale;
}


/*
 * Convert parameter value into a downloadable integer.
 */
int
get_sdac_download_val(int type, int axis)
{
    int ival;
    float val,mval;
    Sdac *pd;

    pd = &sdac_values[type];
    if (type == SLEWLIMIT){
	float trise;
	trise = 1e6 * pd->values[axis];
	val = (SLEWTOP - trise * SLEWOFFSET) / (SLEWSLOPE * trise);
    }else if (type == DUTYLIMIT){
	val = sqrt(pd->values[axis] / pd->max_val) * pd->scale_ival;
    }else{
        mval = pd->max_val;
        if (axis == B0_AXIS) mval=1.0;	 /* only for vnmrs really */
	val = fabs(pd->values[axis] * pd->scale_ival / mval);
// printf("type=%d, axis=%d, val=%f, scale=%f max=%f\n",type, axis, pd->values[axis], pd->scale_ival, mval);
    }
    ival = (int)(val + 0.5);
    if (ival > pd->max_ival){
	ival = pd->max_ival;
    } else if (ival < 0) {
	ival = 0;
    }	
    return ival;
}

/*
 * Get the parameter value corresponding to the rounded integer
 * value that is downloaded.
 * Currently only needed for ECCSCALE and TOTALSCALE.
 */
double
get_quantized_sdac_value(int type, int axis)
{
    Sdac *pd;
    int download;
    double mval, rtn;

#ifdef NVPSG
    if ((axis == B0_AXIS) && (type!=ECCSCALE)) {
#else
    if (axis == B0_AXIS) {
#endif
	return 1.0;
    }
    if (axis < 0 || axis > B0_AXIS) {
	return 0;
    }
    pd = &sdac_values[type];
    download = get_sdac_download_val(type, axis);
    if (type == SLEWLIMIT) {
	rtn = SLEWTOP / (SLEWOFFSET + SLEWSLOPE * download);
    }else if (type == DUTYLIMIT){
	double tmp = download / pd->scale_ival;
	rtn = (tmp * tmp) * pd->max_val;
    }else{
        mval = pd->max_val;
        if (axis == B0_AXIS) mval=1.0;	 /* only for vnmrs really */
// printf("download=%d max=%f ival=%f\n",download, mval, pd->scale_ival);
	rtn = download * mval / pd->scale_ival;
	if (pd->values[axis] < 0) {
	    rtn = -rtn;
	}
    }
    return rtn;
}

static float
xtermGain(int src, int dst)
{
    float gain;
    float dstscale;

#ifdef NVPSG
    if (src == dst) {
       dstscale = 0.0;	// forces gain=1.0
    } else {
       if (dst == B0_AXIS)
          dstscale = 1.0;
        else 
          dstscale=get_quantized_sdac_value(TOTALSCALE, dst);
    }
    if (dstscale == 0.0) {
       gain = 1.0;
    } else {
	gain = (get_quantized_sdac_value(TOTALSCALE, src) / dstscale);
    }
#else
    if (dst == B0_AXIS || src == dst) {
	gain = 1.0;
    } else if ( (dstscale=get_quantized_sdac_value(TOTALSCALE, dst)) == 0) {
	gain = 1.0;
    } else {
	gain = (get_quantized_sdac_value(TOTALSCALE, src) / dstscale);
    }
#endif
    return gain;
}

/**
 * Returns max(y), the maximum absolute value of ECC that we could get
 * at the specified "time" on the specified "dst" axis for the ECC
 * time constants and amplitudes in "ecc_matrix". It is based just on
 * the ECCs due to the source axes (src0 <= axis <= src1).
 * If "partialMax" is non-null, also returns the largest partial sum
 * of terms that we could get at time = "time".  In general, it is larger
 * than max(y), because there may be cancellation by terms of different
 * signs.  Because all terms are exponentially decaying, we know that
 * max(y(t)) will always be less than "*partialMax" for t > "time".
 */
static float
absEccEval(Ecc ecc_matrix[N1][N2], /* Ecc values */
	   int src,		/* Source axis */
	   int dst,		/* Output axis */
	   float time,		/* Evaluate for this time */
	   float *partialMax)	/* OUTPUT: max of terms that are all
				 * the same sign */
{
    /*
     * Calculate the size of the correction for each source axis
     * (src0 <= axis <= src1).  Sum the absolute values of those
     * corrections--because each src axis might be pulsed with either
     * polarity, to get the worst case (largest) correction.
     */
    int i;
    int src0,src1;
    int nterms;
    float amp;
    float tau;
    float term;
    float ymax;                 /* abs(largest sum of terms we could get) */
    float sumplus;              /* abs(sum of + terms for one src axis) */
    float summinus;             /* abs(sum of - terms for one src axis) */
    float maxabs;               /* sum of max(sumplus, summinus) for all srcs */
    float eccscale;
// printf("==============>>>> time=%f\n",time);
#ifdef NVPSG
    src0=X1_AXIS;
    if (dst == B0_AXIS)
       src1 = B0_AXIS;
    else
       src1=Z1_AXIS;
#else
    if (dst == B0_AXIS)
       src0=src1=src;
    else {
       src0=X1_AXIS;
       src1=Z1_AXIS;
    }
#endif
    ymax = maxabs = 0;
    for (src=src0; src<=src1; src++) {
	nterms = ecc_matrix[src][dst].nterms;
	sumplus = summinus = 0;
	for (i=0; i<nterms; i++) {
	    amp = ecc_matrix[src][dst].amp[i];
// printf("src=%d dst=%d i=%d amp=%f",src,dst,i,amp);
            amp = amp * xtermGain(src, dst);
// printf("  amp=%f",amp);
	    if (amp != 0) {
		tau = ecc_matrix[src][dst].tau[i];
		if (tau == 0){
		    term = 0;
		}else{
		    term = amp * exp(-time / tau);
		}
// printf(" taus=%f, term=%f", tau,term);
		if (term >= 0) {
		    sumplus += term;
		}else{
		    summinus -= term;
		}
	    }
// printf("\n");
	}
	ymax += fabs(sumplus - summinus);
	maxabs = sumplus >= summinus ? maxabs + sumplus : maxabs + summinus;
    }
    eccscale = get_quantized_sdac_value(ECCSCALE, dst);
// printf("ymax=%g eccscale=%g",ymax,eccscale);

    /* Return values */
    if (partialMax){
	*partialMax = maxabs * eccscale;
// printf(" partialMax=%f",*partialMax);
    }
// printf("\n");
    return ymax * eccscale;
}

static float
brent(Ecc ecc_matrix[N1][N2],
      float a, float x, float b, /* X values of function */
      float fa, float fx, float fb, /* Corresponding y values */
      int src,			/* Src axis range for func evaluation */
      int dst)			/* Dst axis */
{
    int ix;
    float tol1, tol2;
    float p, q, r;
    float step2t;
    float d;
    float tt;
    float u, fu;
    float v, fv;
    float w, fw;
    float xm;

    float gold = 0.382;
    float zeps = 1e-4;
    float tol = 3e-2;
    float step2 = 0;

    d = fw = v = fv = w = 0.0;

    for (ix=0; ix<100; ix++) {
	xm = (a + b) / 2;
	tol1 = tol * fabs(x) + zeps;
	tol2 = 2 * tol1;
	if (fabs(x - xm) <= tol2 - (b - a) / 2) {
	    /* Normal return */
	    return fx;
	}
	if (fabs(step2) > tol1) {
	    /* Parabolic step - cannot happen first time through */
	    r = (x - w) * (fv - fx);
	    q = (x - v) * (fw - fx);
	    p = (x - v) * q - (x - w) * r;
	    q = 2.0 * (q - r);
	    if (q > 0) {
		p = -p;
	    }
	    q = fabs(q);
	    step2t = step2;
	    step2 = d;
	    if (fabs(p) >= fabs(q * step2t / 2)
		|| p <= q * (a - x)
		|| p >= q * (b - x))
	    {
		step2 = x >= xm ? a - x : b - x;
		d = gold * step2;
	    } else {
		d = p / q;
		u = x + d;
		if (u - a < tol2 || b - u < tol2) {
		    tt = xm - x;
		    d = b > 0 ? fabs(tol1) : -fabs(tol1);
		}
	    }
	} else {
	    /* Bisection step */
	    step2 = x >= xm ? a - x : b - x;
	    d = gold * step2;
	}
	tt = d > 0 ? fabs(tol1) : -fabs(tol1);
	u = fabs(d) >= tol1 ? x + d : x + tt;
	fu = absEccEval(ecc_matrix, src, dst, u, NULL);
	if (fu >= fx) {
	    if (u >= x) {
		a = x;
	    } else {
		b = x;
	    }
	    v = w; w = x; x = u;
	    fv = fw; fw = fx; fx = fu;
	} else {
	    if (u < x) {
		a = u;
	    } else {
		b = u;
	    }
	    if (fu >= fw || w == x) {
		v = w; w = u;
		fv = fw; fw = fu;
	    } else if (fu >= fv || v == x || v == w) {
		v = u; fv = fu;
	    }
	}
    }
    fprintf(stderr, "brent: did not converge in 100 iterations...continuing\n");
    return fx;
}

static float
checkEccExcursion(Ecc ecc_matrix[N1][N2], int dst, int src)
{
    int i;
    float ymax;
    float step;
    /*
     * b   = time
     * fb  = abs(f(b))
     * ffb = max(sum of abs vals of all terms that could be the same sign)
     *
     * ffb is the largest partial sum of terms that we could get at
     * time b. In general, it is larger than fb, because there may be
     * cancellation by terms of different signs.  Because all terms
     * are exponentially decaying, we know that abs(f(t)) for t>b will
     * always be less than ffb.
     */
    float a, fa;
    float b, fb, ffb;
    float c, fc, ffc;
    float localMax;

    float tmin = 1e-5;		/* Minimum time constant */

    /* Initial maximum is just initial value */
    ymax = absEccEval(ecc_matrix, src, dst, 0, NULL);
// printf("Exc'n: ymax=%f\n",ymax);
    /*
     * Step along curve looking for local maxima
     * The strategy here is to take fairly large steps until we see
     * that we have bracketed a peak. Then iterate on that region.
     */
    step = 2.718;		/* Seems to be good value */
    a = 0; fa = ymax;
    b = tmin; fb = absEccEval(ecc_matrix, src, dst, b, &ffb);
    for (i=0; i<100; i++) {     /* Should break out long before this limit */
	c = b * step;
	fc = absEccEval(ecc_matrix, src, dst, c, &ffc);
// printf("Exc'n: fc=%f ffc=%f\n",fc,ffc);
	if (fa < fb && fb > fc) {
	    localMax = brent(ecc_matrix, a, b, c, fa, fb, fc, src, dst);
	    if (localMax > ymax) {
		ymax = localMax;
	    }
	}
	if (ffb <= ymax) {
	    /* All terms are too small to exceed previous maximum */
	    break;
	}
	a = b; fa = fb;
	b = c; fb = fc; ffb = ffc;
    }
    return ymax;
}
#ifdef VNMR
void vnmremsg(char * msg)
{
   Werrprintf("%s",msg);
}
#endif

float
checkEccExcursions(Ecc ecc_matrix[N1][N2])
{
    char msg[100];
#ifndef NVPSG
    int src;
#endif
    int dst;

    float excursion;
    float limit;
    float worst = 0;

    for (dst=0; dst<N2; dst++) {
#ifndef NVPSG 
	if (dst == B0_AXIS) {
	    limit = 0.5;
	    for (src=0; src<N1; src++)
            {
		excursion = checkEccExcursion(ecc_matrix, dst, src);
// printf("Exc's: B0 exc=%f\n",excursion);
		if (excursion >= limit && excursion > 0) {
		    sprintf(msg, "Max %s->%s ECC is %g (limit is %g)",
			    axisLabel[src], axisLabel[dst], excursion, limit);
		    vnmremsg(msg);
		}
		if (worst < excursion / limit) {
		    worst = excursion / limit;
		}
	    }
	}else
#endif
        {
	   limit = get_quantized_sdac_value(ECCSCALE, dst) / 2;
	   excursion = checkEccExcursion(ecc_matrix, dst, -1);
// printf("Exc's: dst=%d exc=%f\n",dst,excursion);
	   if (excursion >= limit && excursion > 0) {
		sprintf(msg, "Max %s ECC is %g (limit is %g)",
			axisLabel[dst], excursion, limit);
		vnmremsg(msg);
	   }
	   if (worst < excursion / limit) {
		worst = excursion / limit;
	   }
	}
    }

    return worst;
}

#ifndef ECC_INCLUDE_MAIN

#define MAXSTR 256

int worst1(int argc, char *argv[], int retc, char *retv[])
{
char    enabled[MAXSTR];
double  taus, ampls, ampcorr;
double  ymax;
double  eccgain, srcgain, dstgain;
int	i,j;
int     iterms;
int	src,dst;
int	src_axis, dst_axis;
Ecc	ecc_matrix[N1][N2];
Sdac   *pd;

// printf("\n\n");

   // first we'll check the arguments
   if (argc <3) 
   {  Werrprintf("Usage -- %s(src-axis, dst-axis, 0 | 1 )", argv[0]);
      RETURN;
   }

   if (argv[1][0] == 'b') src_axis=3;
   else                   src_axis=argv[1][0] - 'x';
   if (argv[2][0] == 'b') dst_axis=3;
   else                   dst_axis=argv[2][0] - 'x';

// printf("src=%d dst=%d\n",src_axis, dst_axis);

   if ( (dst_axis<0) || (src_axis<0) || (dst_axis>3) || (src_axis>3) )
   {  Werrprintf("src_axis and dst_axis must be 'x','y','z' or 'b0'");
      RETURN;
   }

   i=1;
   pd = &sdac_values[ECCSCALE];
   for(j=0; j<4; j++)
   {   
      if (P_getreal(GLOBAL,"scales", &taus, i) < 0)
      {
         Werrprintf("worst1: cannot find 'scales'");
         RETURN;
      }
      pd->values[j]=taus;
// printf("ECCSCALE: i=%d scales=%f\n",i,taus);
      i++;
   }
      
   pd = &sdac_values[SHIMSCALE];
   for(j=0; j<3; j++)
   {   
      if (P_getreal(GLOBAL,"scales", &taus, i) < 0)
      {
         Werrprintf("worst1: cannot find 'scales'");
         RETURN;
      }
      pd->values[j]=taus;
// printf("SHIMSCALE: i=%d scales=%f\n",i,taus);
      i++;
   }
      
   pd = &sdac_values[TOTALSCALE];
   for(j=0; j<3; j++)
   {   
      if (P_getreal(GLOBAL,"scales", &taus, i) < 0)
      {
         Werrprintf("worst1: cannot find 'scales'");
         RETURN;
      }
      pd->values[j]=taus;
// printf("SHIMSCALE: i=%d scales=%f\n",i,taus);
      i++;
   }
      
   i=1;
   pd = &sdac_values[SLEWLIMIT];
   for(j=0; j<4; j++)
   {   
      if (P_getreal(GLOBAL,"limits", &taus, i) < 0)
      {
         Werrprintf("worst1: cannot find 'limits'");
         RETURN;
      }
      pd->values[j]=taus;
// printf("SLEWLIMIT: i=%d scales=%f\n",i,taus);
      i++;
   }
      
   pd = &sdac_values[DUTYLIMIT];
   for(j=0; j<3; j++)
   {   
      if (P_getreal(GLOBAL,"limits", &taus, i) < 0)
      {
         Werrprintf("worst1: cannot find 'limits'");
         RETURN;
      }
      pd->values[j]=taus;
// printf("DUTYLIMIT: i=%d scales=%f\n",i,taus);
      i++;
   }
   pd->values[3]=1.00;
      
   i=1;
   for(dst=0; dst<N2; dst++)
      for (src=0; src<N1; src++)
      {
         if ((src!=dst) && (src==B0_AXIS)) continue;
         iterms = ecc_matrix[src][dst].nterms = nterms(src,dst);
         for (j=0; j<iterms; j++)
         {
            if (P_getstring(GLOBAL,"enabled",enabled,i,MAXSTR) < 0)
            {
                Werrprintf("worst1: cannot find 'enabled'");
                RETURN;
            }
            if (enabled[0] == 'n') 
               ampls=0.0;
            else
            {
               if (P_getreal(GLOBAL,"taus", &taus, i) < 0)
               {  
                   Werrprintf("worst1: cannot find 'taus'");
                   RETURN;
               }
               if (P_getreal(GLOBAL,"ampls", &ampls, i) < 0)
               {  
                   Werrprintf("worst1: cannot find 'ampls'");
                   RETURN;
               }
            }
            ecc_matrix[src][dst].tau[j] = taus;
            ampcorr = ampls;
            /* Correct amplitude for different gains on SDAC */
            eccgain = get_quantized_sdac_value(ECCSCALE, dst);
            srcgain = get_quantized_sdac_value(TOTALSCALE, src);
   	    if (dst != B0_AXIS)
            {
#ifndef NVPSG
               if (dst == B0_AXIS) srcgain = fabs(srcgain);
#endif
               dstgain = get_quantized_sdac_value(TOTALSCALE, dst);
               if (dstgain * eccgain != 0) {
                  ampcorr = ampls * srcgain / (dstgain * eccgain);
               }
	    }
#ifdef    NVPSG
	    else
            {
                ampcorr = ampls * srcgain / eccgain;
            }
#endif
	      ecc_matrix[src][dst].amp[j] = ampcorr;
//  printf("i=%d src=%d dst=%d tau=%f amp=%f\n",i, src, dst, taus, ampls);
// printf("     eccgain=%f srcgain=%f dstgain=%f ampcorr=%f\n",eccgain,srcgain, dstgain, ampcorr);
            i++;
         }
      }

   /*for(dst=0; dst<N2; dst++) {
       for (src=0; src<N1; src++)
       {
           if ((src!=dst) && (src==B0_AXIS)) continue;
           iterms = ecc_matrix[src][dst].nterms = nterms(src,dst);
           for (j=0; j<iterms; j++)
               printf("src=%d dst=%d tau=%f amp=%f\n",
                      src, dst, ecc_matrix[src][dst].tau[j],
                      ecc_matrix[src][dst].amp[j]);
       }
   } */

   ymax = checkEccExcursion(ecc_matrix, dst_axis, src_axis);
// printf("ymax=%f\n",ymax);

   if (retc > 0) 
      retv[0] = realString((double) (ymax));
   RETURN; 

}

#endif
