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
/****************************************************************/
/*								*/
/*	#######   ###   #       ####### ####### ######		*/
/*	#          #    #          #    #       #     #		*/
/*	#          #    #          #    #       #     #		*/
/*	#####      #    #          #    #####   ######		*/
/*	#          #    #          #    #       #   #		*/
/*	#          #    #          #    #       #    #		*/
/*	#         ###   #######    #    ####### #     #		*/
/*								*/
/*	generates filter functions for the 2D filter kernel	*/
/*	in the time domain					*/
/*								*/
/****************************************************************/
/*								*/
/*	authors:	Martin Staemmler			*/
/*								*/
/*	Institute for biomedical Engineering (IBMT)		*/
/*	D - 66386 St. Ingbert, Germany				*/
/*	Ensheimerstrasse 48					*/
/*	Tel.: (+49) 6894 980251					*/
/*	Fax:  (+49) 6894 980400					*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.02.92				*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/
#include	"bp.h"


gen_filter_float (size, center, p_filt_float, bp)
int		size;
float		center;
float		**p_filt_float;
BP_PAR		*bp;
{
	
if (size < 0 || size > MAX_SIZE) {
    fprintf (stderr,"BP_FILT: size illegal = %d\n",size);
    return (ERROR);
    }
if ((*p_filt_float = (float *) malloc ((unsigned) (bp->m_size *
                     sizeof(float)))) == 0) {
    fprintf (stderr,"BP_FILT: can't allocate filter buffer\n");
    return (ERROR);
    }
if (strcmp (bp->filter_name, "band_pass") == 0) {
    return (band_pass_float (size, center, *p_filt_float, 
                           bp->filter_bw, bp->filter_amp));
    }
else if (strcmp (bp->filter_name, "si_low_pass") == 0) {
    return (si_low_pass_float (size, center, *p_filt_float, 
                           bp->filter_bw, bp->filter_amp));
    }
else if (strcmp (bp->filter_name, "cos_low_pass") == 0) {
    return (cos_low_pass_float (size, center, *p_filt_float, 
                           bp->filter_bw, bp->filter_amp));
    }
else if (strcmp (bp->filter_name, "hamming") == 0) {
    return (hamming_float (size, center, *p_filt_float, 
                           bp->filter_bw, bp->filter_amp));
    }
else { 
    fprintf (stderr,"BP_FILT: illegal filter type\n");
    return (ERROR);
    }
}   /* end of filter generation */


band_pass_float (size, center, filter, bw, amp)
int		size;
float		center;
float		filter[];
float		bw, amp;
{
int	i;
double	from, to;

/* check input values */
if (center < 0.0 || center >= (float) size) {
    fprintf (stderr,"BP_FILTER: illegal definition of center=%f\n",center);
    return (ERROR);
    }

from = center - 0.5 * bw * (float)size;
to   = center + 0.5 * bw * (float)size;


if ((int)floor(from + 0.5) < 0.0 || (int)floor(to + 0.5) >= (float) size + 0.5) {
    fprintf (stderr,"BP_FILTER: illegal value of bw = %f\n",bw);
    return (ERROR);
    }
for (i=0; i<(int)floor(from+0.5); i++) {
    filter[i] = 0;
    }
for (i=(int)floor(from+0.5); i<=(int)center; i++) {
    filter[i] = amp * (center - (float)i);
    }
for (i=(int)center+1; i<=(int)floor(to+0.5) && i<size; i++) {
    filter[i] = amp * ((float)i - center);
    }
for (i=(int)floor(to+0.5)+1; i < size; i++) {
    filter[i] = 0;
    }
/* for (i=0; i<size; i++) {
    printf ("%3d: filter=%f\n",i, filter[i]);
    } */
return (TRUE);
}

si_low_pass_float (size, center, filter, bw, amp)
int		size;
float		center;
float		filter[];
float		bw, amp;
{
int	i;
double	from, to;
float	arg, tau;
/* check input values */
if (center < 0.0 || center >= (float) size) {
    fprintf (stderr,"BP_FILTER: illegal definition of center=%f\n",center);
    return (ERROR);
    }
from = center - 0.5 * bw * (float)size;
to   = center + 0.5 * bw * (float)size;
if (from + 0.5 < 0.0 || to >= (float) size + 0.5) {
    fprintf (stderr,"BP_FILTER: illegal value of bw = %f\n",bw);
    return (ERROR);
    }
for (i=0; i<(int)floor(from+0.5); i++) {
    filter[i] = 0;
    }
tau = M_PI / (bw * (float) size);
for (i=(int)floor(from+0.5); i<=(int)center; i++) {
    arg = center - (float)i;
    if (arg == 0.0) { filter[i] = 0.0; }
    else            { filter[i] = amp * arg * sin(arg * tau) / (arg * tau); }
    }
for (i=(int)center+1; i<(int)floor(to+0.5) && i<size; i++) {
    arg = (float)i - center;
    if (arg == 0.0) { filter[i] = 0.0; }
    else            { filter[i] = amp * arg * sin(arg * tau) / (arg * tau); }
    }
for (i=(int)floor(to+0.5)+1; i < size; i++) {
    filter[i] = 0.0;
    }
return (TRUE);
}

cos_low_pass_float (size, center, filter, bw, amp)
int		size;
float		center;
float		filter[];
float		bw, amp;
{
int	i;
double	from, to;
float	arg, tau;
/* check input values */
if (center < 0.0 || center >= (float) size) {
    fprintf (stderr,"BP_FILTER: illegal definition of center=%f\n",center);
    return (ERROR);
    }
from = center - 0.5 * bw * (float)size;
to   = center + 0.5 * bw * (float)size;
if (from + 0.5 < 0.0 || to >= (float) size + 0.5) {
    fprintf (stderr,"BP_FILTER: illegal value of bw = %f\n",bw);
    return (ERROR);
    }
for (i=0; i<(int)floor(from+0.5); i++) {
    filter[i] = 0;
    }
tau = M_PI / (bw * (float) size);
for (i=(int)floor(from+0.5); i<=(int)center; i++) {
    arg = center - (float)i;
    filter[i] = amp * arg * cos(arg * tau); 
    }
for (i=(int)center+1; i<(int)floor(to+0.5) && i<size; i++) {
    arg = (float)i - center;
    filter[i] = amp * arg * cos(arg * tau); 
    }
for (i=(int)floor(to+0.5); i < size; i++) {
    filter[i] = 0.0;
    }
return (TRUE);
}

hamming_float (size, center, filter, alpha, amp)
int		size;
float		center;
float		filter[];
float		alpha, amp;
{
int	i;
float	arg, tau;
/* check input values */
if (center < 0.0 || center >= (float) size) {
    fprintf (stderr,"BP_FILTER: illegal definition of center=%f\n",center);
    return (ERROR);
    }
tau = 2 * M_PI / (float) size;
for (i=0; i<=(int)center; i++) {
    arg = center - (float)i;
    filter[i] = amp * arg * (alpha + (1-alpha) * cos(arg * tau)); 
    }
for (i=(int)center+1; i<size; i++) {
    arg = (float)i - center;
    filter[i] = amp * arg * (alpha + (1-alpha) * cos(arg * tau)); 
    }
return (TRUE);
}



