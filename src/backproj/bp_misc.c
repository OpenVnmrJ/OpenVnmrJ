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
/*	######  ######          #     #   ###    #####   #####	*/
/*	#     # #     #         ##   ##    #    #     # #     #	*/
/*	#     # #     #         # # # #    #    #       #	*/
/*	######  ######          #  #  #    #     #####  #	*/
/*	#     # #               #     #    #          # #	*/
/*	#     # #               #     #    #    #     # #     #	*/
/*	######  #       ####### #     #   ###    #####   #####	*/
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
/*	date:		15.2.92					*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/
#include	"bp.h"

/*
 * function to print out float arrays (for test purposes only)
 */
print_float (size, string, p_prof_float) 
int		size;
char		*string;
float		*p_prof_float;
{
int		i;
for (i=0; i<size; i+=4) {
    printf ("%s: %3d %f %f %f %f\n",string, i,*(p_prof_float+i),
           *(p_prof_float+i+1),*(p_prof_float+i+2),
           *(p_prof_float+i+3)); 
    }
return(TRUE);
}

/*
 * function to print out contents of structure BP_PAR bp
 * (for test purposes only)
 */
print_bp (bp)
BP_PAR	*bp;
{
printf ("filter_name = %s\n", bp->filter_name);
printf ("filter_bw = %lf\n", bp->filter_bw);
printf ("filter_amp = %lf\n", bp->filter_amp);
printf ("prof_file; = %s\n", bp->prof_file);
printf ("prof_filt1 = %s\n", bp->prof_filt1);
printf ("meta_image = %s\n", bp->meta_image);
printf ("prof_filt2 = %s\n", bp->prof_filt2);
printf ("image_file = %s\n", bp->image_file);
printf ("scale1 = %lf\n", bp->scale1);
printf ("scale2 = %lf\n", bp->scale2);
printf ("m_size = %d\n", bp->m_size);
printf ("m_center_x = %lf\n", bp->m_center_x);
printf ("m_center_y = %lf\n", bp->m_center_y);
printf ("m_center_z = %lf\n", bp->m_center_z);
printf ("r_size = %d\n", bp->r_size);
printf ("r_center_x = %lf\n", bp->r_center_x);
printf ("r_center_y = %lf\n", bp->r_center_y);
printf ("r_center_z = %lf\n", bp->r_center_z);
printf ("i_size = %d\n", bp->i_size);
printf ("i_center_x = %lf\n", bp->i_center_x);
printf ("i_center_y = %lf\n", bp->i_center_y);
printf ("i_center_z = %lf\n", bp->i_center_z);
printf ("n_theta = %d\n", bp->n_theta);
printf ("n_phi = %d\n", bp->n_phi);
printf ("theta_start = %lf\n", bp->theta_start);
printf ("theta_end = %lf\n", bp->theta_end);
printf ("phi_start = %lf\n", bp->phi_start);
printf ("phi_end = %lf\n", bp->phi_end);
return(TRUE);
}

/*
 * function to print out contents of structure BP_CTRL bpc
 * (for test purposes only)
 */
print_bpc (bpc)
BP_CTRL	*bpc;
{
printf ("p_size = %d\n", bpc->p_size);
printf ("i_size = %d\n", bpc->i_size);
printf ("resize = %lf\n", bpc->resize);
printf ("scale = %lf\n", bpc->scale);
printf ("n_proj = %d\n", bpc->n_proj);
printf ("offset_x = %lf\n", bpc->offset_x);
printf ("offset_y = %lf\n", bpc->offset_y);
printf ("offset_a = %lf\n", bpc->offset_a);
printf ("offset_r = %lf\n", bpc->offset_r);
printf ("i_center_x = %lf\n", bpc->i_center_x);
printf ("i_center_y = %lf\n", bpc->i_center_y);
printf ("i_center_z = %lf\n", bpc->i_center_z);
printf ("angle_start = %lf\n", bpc->angle_start);
printf ("angle_end = %lf\n", bpc->angle_end);
return(TRUE);
}

/****************************************************************/
/*								*/
/*	######  ####### #     # #     # ######			*/
/*	#     # #     # #     # ##    # #     #			*/
/*	#     # #     # #     # # #   # #     #			*/
/*	######  #     # #     # #  #  # #     #			*/
/*	#     # #     # #     # #   # # #     #			*/
/*	#     # #     # #     # #    ## #     #			*/
/*	######  #######  #####  #     # ######			*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.2.92					*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/

make_bound (i_size, diameter, i_center_x, i_center_y, bound)
int	i_size;
double	diameter;
double	i_center_x, i_center_y;
struct	boundary	bound[];
{
register	line, low, high;
short		limit;
double		dx,dy;

for (line=0; line < i_size; line++) {
    bound[line].l = bound[line].r = bound[line].c = 0;
    }
/* check validity of diameter: 
 * i_size points are i_size-1 pixel distancies apart 
 * diameter has to match with i_center_x / i_center_y 
 */
if (diameter > (double)(i_size - 1)) diameter = (double)(i_size - 1);
dx = i_center_x - (double)(i_size - 1)/2.0;
dy = i_center_x - (double)(i_size - 1)/2.0;
if (2.0 * sqrt (dx * dx + dy * dy) + diameter > (double)(i_size - 1)) {
    diameter = (double)(i_size - 1) - 2.0 * sqrt(dx * dx + dy * dy);
    printf ("BOUND: diameter reduced =%lf, dx=%lf dy=%lf\n",diameter,dx,dy);
    }

low  = (int)(i_center_y - diameter/2.0);
high = (int)(i_center_y + diameter/2.0);
/* zero boundary until low */
for (line=0; line < low; line++) {
    bound[line].l = bound[line].r = bound[line].c = 0;
    }
for (line=0; line <= (int)(diameter/2.0); line++) {
    /* limit is the height in a rectangular triangle */
    limit = sqrt (diameter * (double)line - (double)(line * line));
    bound[low + line].l = bound[high - line].l =
                           (short)(i_center_x - limit + 0.50);
    bound[low + line].r = bound[high - line].r =
                           (short)(i_center_x + limit + 0.50);
    bound[low + line].c = bound[high - line].c =
                           bound[low + line].r - bound[low + line].l;
    }
if (((int)diameter) % 2 == 1) {	/* (int)diameter == odd) */
    bound[low + line].l = (short)(i_center_x - limit + 0.50);
    bound[low + line].r = (short)(i_center_x + limit + 0.50);
    bound[low + line].c = bound[low + line].r - bound[low + line].l;
    }

for (line=high+1; line < i_size; line++) {
    bound[line].l = bound[line].r = bound[line].c = 0;
    }

/* printf ("\n");
for (line=0; line < i_size; line++) {
    printf ("bound[%3d].left = %5d  ...  .right = %5d ==> .count = %5d\n", 
             line, bound[line].l, bound[line].r, bound[line].c);
    } */
return (TRUE);
}

/****************************************************************/
/*								*/
/*	#     #   ###   #     #         #     #    #    #     #	*/
/*	##   ##    #    ##    #         ##   ##   # #    #   #	*/
/*	# # # #    #    # #   #         # # # #  #   #    # #	*/
/*	#  #  #    #    #  #  #         #  #  # #     #    #	*/
/*	#     #    #    #   # #         #     # #######   # #	*/
/*	#     #    #    #    ##         #     # #     #  #   #	*/
/*	#     #   ###   #     # ####### #     # #     # #     #	*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.2.92					*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/

get_min_max_int (p_int, size, p_min, p_max)
int		*p_int;
int		size;
int		*p_min, *p_max;
{
int	*p_int_end;
  *p_max=*p_min=*p_int;
p_int_end = p_int + size;
for (; p_int < p_int_end; p_int++) {
    if      (*p_int < *p_min) { *p_min = *p_int; }
    else if (*p_int > *p_max) { *p_max = *p_int; }
    else                        {;}
    }
return(TRUE);
}

get_min_max_float (p_float, size, p_min, p_max)
float		*p_float;
int		size;
float		*p_min, *p_max;
{
  *p_max=*p_min=*p_float;
float	*p_float_end;
p_float_end = p_float + size;
for (; p_float < p_float_end; p_float++) {
    if      (*p_float < *p_min) { *p_min = *p_float; }
    else if (*p_float > *p_max) { *p_max = *p_float; }
    else                        {;}
    }
return(TRUE);
}

get_min_max_double (p_double, size, p_min, p_max)
double		*p_double;
int		size;
double		*p_min, *p_max;
{
double	*p_double_end;
  *p_max=*p_min=*p_double;
p_double_end = p_double + size;
for (; p_double < p_double_end; p_double++) {
    if      (*p_double < *p_min) { *p_min = *p_double; }
    else if (*p_double > *p_max) { *p_max = *p_double; }
    else                         {;}
    }
return(TRUE);
}



