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
/*	authors:	Martin Staemmler			*/
/*			Peter Barth				*/
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
/*	15.02.94        15.02.92				*/
/*								*/
/****************************************************************/
#include	<stdio.h>
#include	<math.h>
#include	<string.h>
#include	"bpvalues.h"
#include	"data.h"

#define		PNLENGTH	128
#define		FNLENGTH	16
#define		MAX_SIZE	65536



main(argc,argv)
int	argc;
char	*argv[];
{
char	path_name[PNLENGTH+FNLENGTH];
char	out_name[PNLENGTH+FNLENGTH];
short	status, input_status;
int	i;
int     l1, ipt;
int	block, trace, k;
int	file_size;
int	data[MAX_SIZE];
int	*p_int;
float	*p_float;
FILE	*fp;
FILE	*fp_out;
struct	datafilehead	fhead;
struct	datablockhead	bhead;
struct	hypercmplxbhead	hchead;

if (argc < 3) {
    printf ("bp_mc usage: bp_mc input_file output_file\n");
    return(1);
    }
strcpy (path_name, argv[1]);
strcpy (out_name, argv[2]);

if ((fp = fopen (path_name, "r")) == NULL) {
    printf ("BP_MC: can't open %s\n", path_name);
    return (0);
    }
if ((fp_out = fopen (out_name, "w")) == NULL) {
    printf ("BP_MC: can't open %s\n", out_name);
    return (0);
    }
/* get filesize and tell to user */
fseek (fp, (long)0, 2); file_size = (int)ftell(fp);
printf ("BP_MC: file_size = %d bytes\n", file_size);
fseek (fp, (long)0, 0);

/* read and write header information */
if (fread ((char *)&fhead, sizeof(fhead), 1, fp) != 1) {
    printf ("BP_MC: can't read fhead\n");
    return (0);
    }


if (fhead.tbytes > MAX_SIZE * sizeof(int)) {
    printf ("ERROR: blocksize larger than MAX_SIZE\n");
    return(1);
    }
/* change data status to floating point and spectrum */
input_status = status = fhead.status;
status |= S_SPEC;
status |= S_FLOAT;
fhead.status = status;
fhead.np /= 2;
fhead.bbytes = (fhead.bbytes-fhead.tbytes) + fhead.tbytes/2;
fhead.tbytes /=2;
if (fwrite ((char *)&fhead, sizeof(fhead), 1, fp_out) != 1) {
    printf ("BP_MC: can't write fhead\n");
    return (0);
    }
printf ("BP_MC: # of blocks=%d, # of traces=%d\n",fhead.nblocks,fhead.ntraces);

/* read and write data blocks one by one */
status &= 0x007f;
for (block=0; block<fhead.nblocks; block++) {
    /* reading datablockhead information */
    if (fread ((char *)&bhead, sizeof(bhead), 1, fp) != 1) {
        printf ("BP_MC: can't read bhead\n");
        return (0);
        }


    bhead.status &= 0xff80; bhead.status += status;
    if (fwrite ((char *)&bhead, sizeof(bhead), 1, fp_out) != 1) {
        printf ("BP_MC: can't write bhead\n");
        return (0);
        }
    if (block == 0) {
        if (input_status & S_FLOAT) { 
            printf ("BP_MC: floating point data\n");
            }
        else {
            printf ("BP_MC: integer data\n");
            }
        }
    for (trace=0; trace<fhead.ntraces; trace++) {
        if (fread ((char *)data, 2 * fhead.tbytes, 1, fp) != 1) {
            printf ("BP_MC: can't read block=%d, trace=%d\n",block,trace);
            return (0);
            }
        /* decide printout according data type */
        if (input_status & S_FLOAT) { 
            p_float = (float *)data;
            mc_float ((int)fhead.np, (float *)data);
            /* printf ("BP_MC: floating point data input not supported\n");
            return(0); */
            }
        else {
            p_int = data;
            mc_float ((int)fhead.np, (float *)data);
            }
        if (fwrite ((char *)data, fhead.tbytes, 1, fp_out) != 1) {
            printf ("BP_MC: can't write block=%d, trace=%d\n",block,trace);
            return (0);
            }
        }   /* end of loop for all traces */
    }   /* end of loop for all blocks */

fclose (fp);
fclose (fp_out);
printf ("BP_MC: finished\n");
}

mc_float (size, array)
int	size;		/* number of complex data pairs */
float	array[];
{
int	i,j;

/* calculate magnitude from size values */
for (i=0, j=0; j<size; i+=2, j++) {
    array[j] = (float)sqrt ((double)(array[i] * array[i] + array[i+1] * array[i+1]));
    }
return(1);
}



