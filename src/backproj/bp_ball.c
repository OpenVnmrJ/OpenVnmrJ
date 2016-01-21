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
/*	######  ######          ######     #    #       #	*/
/*	#     # #     #         #     #   # #   #       #	*/
/*	#     # #     #         #     #  #   #  #       #	*/
/*	######  ######          ######  #     # #       #	*/
/*	#     # #               #     # ####### #       #	*/
/*	#     # #               #     # #     # #       #	*/
/*	######  #       ####### ######  #     # ####### #######	*/
/*								*/
/*	generates concentric ball or zylinders for testing	*/
/*	backprojection reconstruction (2d and 3d case)		*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.02.92				*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/


#include <stdio.h>
#include <math.h>
#include "bpvalues.h"
#include "data.h"

#define    	MAX_SIZE	0x200
#define    	N_PROJ 		0x100
#define		MAX_RADII	20
#define		ERROR		0


main(argc,argv)

int argc;
char *argv[];
{

float 		prof[MAX_SIZE];
register	i, k;
int 		count;
int		r[MAX_RADII];
double		mitte;
double		quad;
int 		half, size, proj, dim;
int		rad_max, teeth, teeth_inc, gap, gap_inc;
FILE		*fpout;
int		intens;
int 		min, max;
double		sum;
struct		datafilehead	fh;
struct		datablockhead	bh;

min = 0;
max = 0;
if (argc == 1) { 
    printf ("usage: bp_ball output_path_filename\n\n");
    return (0);
    }

printf("Enter dimension (2 or 3):");
scanf("%d",&dim);

printf("Enter # of profils......:");
scanf("%d",&proj);

printf("Size of profiles........:");
scanf("%d",&size);

printf("Enter intensity.........:");
scanf("%d",&intens);

count = 0;
half = size>>1;
mitte = (double)(half) - 0.5;
rad_max = 6*size/16;
teeth = 1; teeth_inc = 1;
gap = 1; gap_inc = 1;

r[0] = rad_max; r[1] = rad_max - teeth;
printf ("BP_BALL: first gap = %d, gap_inc = %d, teeth = %d, teeth_inc = %d\n",
		   gap, gap_inc, teeth, teeth_inc);
for (i=2; (i< MAX_RADII) && (r[i-1] - teeth - gap > 0); i+=2) {
    r[i] = r[i-1] - gap;
    r[i+1] = r[i] - teeth;
    gap += gap_inc;
    teeth += teeth_inc;
    }
count = i;
printf ("BP_BALL: %2d teeth generated\n", count/2);


/* calculate one profile */

for ( k = 0; k < size/2; k++) {
    prof[k] = prof[size-1-k] = 0;	/* initialize */
    quad = (mitte - k)*(mitte - k);

    if ( dim == 2) {
	for (i=0; i< count; i+=2) {
            if (k > mitte - r[i]) {
		if (r[i] * r[i] > quad) {
		    prof[k] = prof[size-1-k] += 
		    (float) intens * 2.0 * sqrt ((double)r[i] * r[i] - quad);
		    if(prof[k] > max) max = prof[k];
		    if(prof[k] < min) min = prof[k];
		    }
                }
            if (k > mitte - r[i+1]) {
		if (r[i] * r[i] > quad) {
		    prof[k] = prof[size-1-k] -= 
		    (float) intens * 2.0 * sqrt ((double)r[i+1] * r[i+1] - quad);
		    if(prof[k] > max) max = prof[k];
		    if(prof[k] < min) min = prof[k];
		    }
                }
            }   /* end of loop for all radii */
        }   /* end of case dim = 2 */
	
    if ( dim == 3) {
	for (i=0; i< count; i+=2) {
            if (k > mitte - r[i]) {
		prof[k] = prof[size-1-k] += 
                    (float) intens * M_PI * ((double)r[i] * r[i] - quad);
		    if(prof[k] > max) max = prof[k];
		    if(prof[k] < min) min = prof[k];
                }
            if (k > mitte - r[i+1]) {
		prof[k] = prof[size-1-k] -= 
		    (float) intens * M_PI * ((double)r[i+1] * r[i+1] - quad);
		    if(prof[k] > max) max = prof[k];
		    if(prof[k] < min) min = prof[k];
                }
            }   /* end of loop for all radii */
        }   /* end of case dim = 2 */
    }   /* end of loop for half profile */

for (i=0, sum=0.0; i< size; i++) { sum += (double)prof[i]; }
printf( "MIN = %0x , MAX = %0x, sum = %lf\n",min,max,sum);
if ((fpout = fopen (argv[1], "w")) == NULL) {
    fprintf (stderr,"BP_BALL: can't create %s\n", argv[1]);
    return (ERROR);
    }
/* set up and write file header for data structure information */
fh.nblocks   = proj;
fh.ntraces   = 1;
fh.np        = size;
fh.ebytes    = 4;
fh.tbytes    = fh.np * fh.ebytes;
fh.bbytes    = fh.ntraces * fh.tbytes + sizeof(struct datablockhead);
fh.vers_id   = 0x0000;
fh.status    = S_DATA | S_FLOAT | S_32;	/* 0x0045; */
/* fh.status    = S_NI | S_NP | S_SECND | S_FLOAT | 
               S_SPEC | S_DATA | S_TRANSF; */
fh.nbheaders = proj;
if (fwrite ((char *)&fh, sizeof(struct datafilehead), 1, fpout) != 1) {
    fprintf (stderr,"BP_BALL: can't write image fh\n");
    return (ERROR);
    }

/* set up and write block information for all blocks */
bh.scale = 0;
/* bh.status = S_DATA | S_SPEC | S_FLOAT; */
bh.status = 0x0;
bh.index = 0;
/* bh.mode = NP_AVMODE | NI_AVMODE; */
bh.mode  = 0;
bh.ctcount = 1;
bh.lpval = 0.0;
bh.rpval = 0.0;
bh.lvl = 0.0;
bh.tlt = 0.0;
/* loop for all blocks */
for (i=0; i<proj; i++) {
    if (fwrite ((char *)&bh, sizeof(struct datablockhead),
            1, fpout) != 1) {
        fprintf (stderr,"BP_BALL: can't write image blockhead\n");
        return (ERROR);
        }

    /* write profile data to disk */
    if (fwrite ((char *)prof, (int)(fh.ntraces * fh.tbytes), 1, fpout) != 1) {
        fprintf (stderr,"BP_BALL: can't write profile data\n");
        return (ERROR);
        }
    }
fclose (fpout);
return (0);
}


