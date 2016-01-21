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
/************************************************************************/
/*									*/
/*	  ###   #     #   ###   #######         ####### #     # #######	*/
/*	   #    ##    #    #       #            #       #     #    #	*/
/*	   #    # #   #    #       #            #       #     #    #	*/
/*	   #    #  #  #    #       #            #####   #######    #	*/
/*	   #    #   # #    #       #            #       #     #    #	*/
/*	   #    #    ##    #       #            #       #     #    #	*/
/*	  ###   #     #   ###      #    ####### #       #     #    #	*/
/*									*/
/*									*/
/************************************************************************/
/*									*/
/*	authors:	Martin Staemmler				*/
/*									*/
/*	Institute for biomedical Engineering (IBMT)			*/
/*	D - 66386 St. Ingbert, Germany					*/
/*	Ensheimerstrasse 48						*/
/*	Tel.: (+49) 6894 980251						*/
/*	Fax:  (+49) 6894 980400						*/
/*									*/
/************************************************************************/
/*									*/
/*	date:		15.02.92					*/
/*	revision:	initial release					*/
/*									*/
/************************************************************************/
#include	"bp.h"

init_fht_float (size, p_log2_size, exp_of_2, sine, tangens)
int		size;
int		*p_log2_size;
float		sine[], tangens[];
int		exp_of_2[];
{
int		i, index;
int		size_by_4;
double		temp, temp0;
/* test if size is in valid range */
if (size < 0 || size > MAX_SIZE) {
    fprintf (stderr,"BP_FHT: illegal size = %d\n", size);
    return (ERROR);
    }

/* get power to two of size */
for (*p_log2_size=0, i=size; i>1; (*p_log2_size)++) { i /= 2; }

/* get powers of two */
for (i=1, exp_of_2[0]=1; i<= *p_log2_size; i++) {
    exp_of_2[i] = exp_of_2[i-1] + exp_of_2[i-1];
    }

size_by_4 = size / 4;

/* set up sine table for fht */
temp0 = 0.5 * M_PI / size_by_4;
for (i=0, temp=0; i<=size_by_4; i++) {
     sine[i] = (float) sin(temp);
     temp += temp0;
     }

/* set up tangens table for fht using */
/* tan(angle/2)=(1.0-sin(angle))/sin(angle) */
index = size_by_4 - 1;
for (i=1; i< size_by_4; i++) {
    tangens[i] = (float) (1.0 - sine[index]) / sine[i]; 
    index --;
    }
tangens[size_by_4] = 1.0;
return (TRUE);
}



/****************************************************************/
/*								*/
/*	####### #     # ####### 				*/
/*	#       #     #    #    				*/
/*	#       #     #    #    				*/
/*	#####   #######    #    				*/
/*	#       #     #    #    				*/
/*	#       #     #    #    				*/
/*	#       #     #    #    				*/
/*								*/
/****************************************************************/
/*								*/
/*	date:							*/
/*	revision:						*/
/*								*/
/****************************************************************/

fht_float (size, log2_size, exp_of_2, sine, tangens, field)
int	size;			/* data array size */
int	log2_size;		/* 2 exp (log2_size) = size */
int	exp_of_2[];		/* 2 exp n for 0 <= n <= log2_size */
float	sine[], tangens[];	/* sine and tangens table */
float	field[];		/* data array */
{
int		size_by_4;
int		log2_size_m1;
float		temp;
float		temp0, temp1, temp2;
/* definitions */
int		i,j,k,l;
int		q, u, s, sps, d, e;
int		cur_exp2;
/* set up some constants */
size_by_4 = size >> 2;
log2_size_m1 = log2_size - 1;

/* perform specific handling for cases size=2, size=4 and size=8 */
if (size <= 8) {
    fprintf (stderr,"BP_FHT: size < 8 not supported\n");
    return (ERROR);
    }

/* permutation of the data prior to fht */
i = j = -1;
do {
    do {
        i++; k = log2_size;
        do {
            k--; j -= exp_of_2[k];
            } while (j >= -1);
        j += exp_of_2[k+1];
        } while (i <= j);
    temp       = field[i+1];
    field[i+1] = field[j+1];
    field[j+1] = temp;
    } while (i < size - 3);

/* step 1 and 2 of fht */
for (i=0; i < size - 1; i+=2) {
    temp0      = field[i];
    field[i]  += field[i+1];
    field[i+1] = temp0 - field[i+1];
    }
if (log2_size == 1) return(0);
for (i=0; i<size-3; i+=4) {
    temp0 = field[i];
    field[i]   += field[i+2];
    field[i+2]  = temp0 - field[i+2];
    temp0       = field[i+1];
    field[i+1] += field[i+3];
    field[i+3]  = temp0 - field[i+3];
    }
if (log2_size == 2) return(0);

/* step 3, 4, ... of fht */
u = log2_size_m1;
s = 4;
for (l=2; l<=log2_size_m1; l++) {
    sps = s + s; 
    u -= 1;
    cur_exp2 = exp_of_2[u-1];
    for (q=0; q<size; q+=sps) {
        i = q; 
        d = q + s;
        temp0 = field[i]; field[i] += field[d]; field[d] = temp0 - field[d];
        k = d - 1;
        for (j=cur_exp2; j < size_by_4; j+= cur_exp2) {
            i++;
            d = i + s;
            e = k + s;
            temp0 = field[d] + field[e] * tangens[j];
            temp1 = field[e] - temp0 * sine[j];   /* x */
            temp2 = temp1 * tangens[j] + temp0;   /* y */
            field[d] = field[i]; field[i] += temp2; field[d] -= temp2;
            field[e] = field[k]; field[k] -= temp1; field[e] += temp1;
            k --;
            }
        e = k + s;
        temp0 = field[k]; field[k] += field[e]; field[e] = temp0 - field[e]; 
        }
    s = sps;
    }
return (1);
}

/****************************************************************/
/*								*/
/*	 #####  #     #    #    ######				*/
/*	#     # #  #  #   # #   #     #				*/
/*	#       #  #  #  #   #  #     #				*/
/*	 #####  #  #  # #     # ######				*/
/*	      # #  #  # ####### #				*/
/*	#     # #  #  # #     # #				*/
/*	 #####   ## ##  #     # #				*/
/*								*/
/****************************************************************/
/*								*/
/*	date:							*/
/*	revision:						*/
/*								*/
/*	performs swap of right with left half (see example)     */
/*	in: 	0, 1, 2,  ...  5, 6, 7, 8, 9,10, ... 13,14,15	*/
/*	out: 	8, 9, 10, ... 13,14,15, 0, 1, 2, ...  5, 6, 7	*/
/*								*/
/****************************************************************/
swap_float (size, p_float)
int		size;
float		*p_float;
{
float		temp;
float		*p_float_off;
float		*p_float_end;
p_float_end = p_float + size / 2;
p_float_off = p_float_end;
for (; p_float < p_float_end; p_float++, p_float_off++) {
    temp         = *p_float;
    *p_float     = *p_float_off;
    *p_float_off = temp;
    }
return(TRUE);
}

