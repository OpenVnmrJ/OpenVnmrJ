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
/*  static char SCCSid[] = "@(#)imcalc.c 9.1 4/16/93  (C)1991 Spectroscopy Imaging Systems"; */

/* IMAGECALC.C */

/******************************************************************************
   This program performs a number of different mathematical and pixel
   manipulation operations on standard Vnmr phasefiles.  The supported 
   functions are listed below, with the required arguments for each.
   All operations require either one or two names of phasefiles to be
   used as operands, and a name for the resultant output phasefile.
   Some functions require additional arguments to be used as a multiplier,
   threshold, pixel shift, etc.  The argument format is compatible with
   the Vnmr macro "imcalc."

   Alan R. Rath, 9-5-91  
******************************************************************************/

#include <sys/file.h>
#include <sys/errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include "data.h" 

#define NRND(x)	((x) >= 0 ? ((int)((x)+0.5)) : (-(int)(-(x)+0.499999999999999999999999)))

struct datafilehead main_header1, main_header2;

struct datablockhead *block_header1, block_header2;
typedef  struct datablockhead  DATABLOCKHEAD;

main(argc, argv)
int   argc;
char  *argv[];
{
    int     nblocks, dim1, dim2, i, j, in_file1, in_file2, out_file;
    int     binary, intval;
    double  realval, realval2, maxval;
    char    function[16], in_name1[100], in_name2[100], out_name[100];
    float   *phasefile1, *phasefile2, *phasefileout;

/* Usage: 
imcalc  add    phf1  phf2  destphf  multiplier
imcalc  mult   phf1  phf2  destphf
imcalc  div    phf1  phf2  destphf  thresh   

imcalc  mean   phf1  phf2  destphf            **  Arithmetic mean       **
imcalc  gmean  phf1  phf2  destphf            **  Geometric mean        **

imcalc  vadd   phf1  phf2  destphf            **  Magnitude and phase from **
imcalc  phase  phf1  phf2  destphf  thresh    **  orthogonal components    **

imcalc  addc         phf1  destphf  constant
imcalc  multc        phf1  destphf  constant

imcalc  clipmax      phf1  destphf  maxlevel  **  pixel < maxlevel > 0  **
imcalc  clipmin      phf1  destphf  minlevel  **  0 < minlevel > pixel  **
imcalc  thresh       phf1  destphf  thresh    **  Zero below, One above **
imcalc  thresh2      phf1  destphf  th1 th2   **  0 < th1 > 1 < th2 > 0 **

imcalc  abs          phf1  destphf            **  Absolute value        **
imcalc  log          phf1  destphf            **  log10(pixel)          **
imcalc  exp          phf1  destphf            **  10**pixel             **
imcalc  pow          phf1  destphf  pow       **  pixel**pow            **
imcalc  reverse      phf1  destphf            **  Reverse gray scale    **
imcalc  flip_horiz   phf1  destphf
imcalc  flip_vert    phf1  destphf
imcalc  flip_diag    phf1  destphf
imcalc  rotate_90    phf1  destphf
imcalc  rotate_180   phf1  destphf
imcalc  f1roll       phf1  destphf  pixshift
imcalc  f2roll       phf1  destphf  pixshift
imcalc  vline        phf1  destphf  pixel_no
imcalc  hline        phf1  destphf  pixel_no
imcalc  function     n  phf1  phf2  phf3 ... phfn  destphf
*/

    /****
    * Get math operation, and input and output file names from command
    * line arguments:
    ****/
    strcpy(function, argv[1]);
    strcpy(in_name1, argv[2]);

    if (!strcmp(function,"add")  ||  !strcmp(function,"div")  ||
	!strcmp(function,"phase")) {
	if (argc != 6) {
	    printf("Incorrect number of arguments in program \"imcalc\"\n");
	    exit(1);
	}
	binary = 1;
        strcpy(in_name2, argv[3]);
        strcpy(out_name, argv[4]);
	realval = atof(argv[5]);
    }

    if (!strcmp(function,"vadd")  ||  !strcmp(function,"mult")   ||
        !strcmp(function,"mean")  ||  !strcmp(function,"gmean")) {
	if (argc != 5  &&  strcmp(function,"add")) {
	    printf("\nIncorrect number of arguments in program \"imcalc\"\n");
	    exit(1);
	}
	binary = 1;
        strcpy(in_name2, argv[3]);
        strcpy(out_name, argv[4]);
    }

    else if (!strcmp(function,"pow")  ||  !strcmp(function,"f1roll")    ||
        !strcmp(function,"f2roll")    ||  !strcmp(function,"addc")     ||
        !strcmp(function,"multc")     ||  !strcmp(function,"clipmax")  ||
        !strcmp(function,"clipmin")   ||  !strcmp(function,"thresh")   ||
        !strcmp(function,"vline")     ||  !strcmp(function,"hline")) {
	if (argc != 5) {
	    printf("\nIncorrect number of arguments in program \"imcalc\"\n");
	    exit(1);
	}
	binary = 0;
        strcpy(out_name, argv[3]);
	realval = atof(argv[4]);
    }

    else if (!strcmp(function,"abs")    ||  !strcmp(function,"exp")        ||
        !strcmp(function,"log")         ||  !strcmp(function,"reverse")    ||
        !strcmp(function,"flip_horiz")  ||  !strcmp(function,"flip_vert")  ||
        !strcmp(function,"flip_diag")   ||  !strcmp(function,"rotate_90")  ||
	!strcmp(function,"rotate_180")  ||  !strcmp(function,"flip_diag")) {
	if (argc != 4) {
	    printf("\nIncorrect number of arguments in program \"imcalc\".\n");
	    exit(1);
	}
	binary = 0;
        strcpy(out_name, argv[3]);
    }

    else if (!strcmp(function,"thresh2")) {
	if (argc != 6) {
	    printf("\nIncorrect number of arguments in program \"imcalc\"\n");
	    exit(1);
	}
	binary = 0;
        strcpy(out_name, argv[3]);
	realval = atof(argv[4]);
	realval2 = atof(argv[5]);
    }

    /****
    * Open input files:
    ****/
    in_file1 = open(in_name1, O_RDONLY);
    if (in_file1 < 0) {
        perror("Open");
        printf("Can't open first input phasefile.\n");
        exit(4);
    }
    if (binary) {
        in_file2 = open(in_name2, O_RDONLY);
        if (in_file2 < 0) {
            perror("Open");
            printf("Can't open second input phasefile.\n");
            exit(4);
        }
    }

    /****
    * Read main_headers.
    ****/
    if (read(in_file1, &main_header1, 32) != 32) {
        perror ("read");
        exit(5);
    }
    if (binary) {
        if (read(in_file2, &main_header2, 32) != 32) {
            perror ("read");
            exit(5);
        }
    }

    /****
    * Check to see that matrix size is the same for both phasefiles:
    ****/
    dim1 = main_header1.ntraces;
    dim2 = main_header1.np;
    nblocks = main_header1.nblocks;
    if (binary  &&  (main_header1.ntraces != main_header2.ntraces)) {
        printf("Error:  Input phasefile dimensions do not match.\n");
        exit(4);
    }
    if (binary  &&  (main_header1.np != main_header2.np)) {
        printf("Error:  Input phasefile dimensions do not match.\n");
        exit(4);
    }
    if (binary  &&  (main_header1.nblocks != main_header2.nblocks)) {
        printf("Error:  Input phasefile dimensions do not match.\n");
        exit(4);
    }

    /****
    * Allocate space for input data and block_headers.
    ****/
    phasefile1 = (float *)malloc(nblocks*dim1*dim2*sizeof(float));
    if (binary) {
        phasefile2 = (float *)malloc(nblocks*dim1*dim2*sizeof(float));
    }
    phasefileout = (float *)malloc(nblocks*dim1*dim2*sizeof(float));
    block_header1 = (DATABLOCKHEAD *)malloc(28*nblocks);

    /****
    * Read data and block_headers.  The block_headers from the first
    * phasefile are saved, to be written to the output phasefile.
    ****/
    for (i=0; i<nblocks; i++) {
        if (read(in_file1, &block_header1[i], 28) != 28) {
            perror ("read");
            exit(5);
        }
        if (binary) {
            if (read(in_file2, &block_header2, 28) != 28) {
                perror ("read");
                exit(5);
            }
        }
        read(in_file1, &phasefile1[i*dim1*dim2], dim1*dim2*sizeof(float));
        if (binary) {
            read(in_file2, &phasefile2[i*dim1*dim2], dim1*dim2*sizeof(float));
        }
    }
    dim1 *= nblocks;

    /****
    * Perform mathematical calculation:
    ****/
    if (!strcmp(function,"add")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i] + realval*phasefile2[i];
        }
    }

    else if (!strcmp(function,"mult")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i]*phasefile2[i];
        }
    }

    else if (!strcmp(function,"div")) {
        for (i=0; i<dim1*dim2; i++) {
	    if ((phasefile2[i] == 0.0) || (fabs(phasefile1[i]) < realval
	      &&  fabs(phasefile1[i]) < realval))
		phasefileout[i] = 1.0e-8;
            else
	        phasefileout[i] = phasefile1[i]/phasefile2[i];
        }
    }

    else if (!strcmp(function,"vadd")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = sqrt(phasefile1[i]*phasefile1[i]
		+ phasefile2[i]*phasefile2[i]);
        }
    }

    else if (!strcmp(function,"phase")) {
        for (i=0; i<dim1*dim2; i++) {
	    if (fabs(phasefile1[i]) < realval && fabs(phasefile1[i]) < realval)
		phasefileout[i] = 1.0e-8;
            else
	        phasefileout[i]=180.0*(atan2(phasefile1[i],phasefile2[i])/M_PI);
        }
    }

    else if (!strcmp(function,"mean")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = (phasefile1[i] + phasefile2[i])/2.0;
        }
    }

    else if (!strcmp(function,"gmean")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = sqrt(fabs(phasefile1[i]*phasefile2[i]));
        }
    }

    else if (!strcmp(function,"addc")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i] + realval;
        }
    }

    else if (!strcmp(function,"multc")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i]*realval;
        }
    }

    else if (!strcmp(function,"abs")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = fabs(phasefile1[i]);
        }
    }

    else if (!strcmp(function,"log")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = log10(fabs(phasefile1[i]));
        }
    }

    else if (!strcmp(function,"exp")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = pow(10.0,phasefile1[i]);
        }
    }

    else if (!strcmp(function,"pow")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = pow(phasefile1[i], realval);
        }
    }

    else if (!strcmp(function,"clipmax")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i] > realval ? 1.0e-6 : phasefile1[i];
        }
    }

    else if (!strcmp(function,"clipmin")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i] <= realval ? 1.0e-6 : phasefile1[i];
        }
    }

    else if (!strcmp(function,"thresh")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = phasefile1[i] >= realval ? 1.0 : 1.0e-6;
        }
    }

    else if (!strcmp(function,"thresh2")) {
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = (phasefile1[i] >= realval 
	      && phasefile1[i] <= realval2) ? 1.0 : 1.0e-6;
        }
    }

    else if (!strcmp(function,"reverse")) {
	maxval = 0.0;
        for (i=0; i<dim1*dim2; i++) {
	    maxval = phasefile1[i] > maxval ? phasefile1[i] : maxval;
        }
        for (i=0; i<dim1*dim2; i++) {
	    phasefileout[i] = maxval - phasefile1[i];
        }
    }

    else if (!strcmp(function,"flip_horiz")) {
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[i + (dim2)*(dim1-j-1)];
            }
	}
    }

    else if (!strcmp(function,"flip_vert")) {
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = phasefile1[dim2 - i - 1 + dim2*j];
            }
	}
    }

    else if (!strcmp(function,"flip_diag")) {
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[j + dim1*i];
            }
	}
    }

    else if (!strcmp(function,"rotate_90")) {
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[(dim1*i + dim2 - j) % (dim1*dim2)];
            }
	}
    }

    else if (!strcmp(function,"rotate_180")) {
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[dim1*dim2 - i - dim2*j - 1];
            }
	}
    }

    else if (!strcmp(function,"f1roll")) {
	intval = NRND(realval);
	if (intval < 0)
	    intval = dim2 + intval;
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[((i + intval) % dim2) + dim2*j];
            }
	}
    }

    else if (!strcmp(function,"f2roll")) {
	intval = NRND(realval);
	if (intval < 0)
	    intval = dim1 + intval;
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
	        phasefileout[i + dim2*j] = 
		  phasefile1[i + dim2*((j + intval) % dim1)];
            }
	}
    }

    else if (!strcmp(function,"vline")) {
	intval = NRND(realval);
	if (intval > dim2) {
	    printf("\nPixel value out of range in program \"imcalc\"\n");
	    exit(1);
	}
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
		if (i == intval  &&  i > 0  &&  i < dim2-1) {
	            phasefileout[i + dim2*j] = (phasefile1[i + dim2*j - 1]
			+ phasefile1[i + dim2*j + 1])/2.0;
		}
		else if (i == intval  &&  i == 0) {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*j + 1];
		}
		else if (i == intval  &&  i == dim2-1) {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*j - 1];
		}
		else {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*j];
		}
            }
	}
    }

    else if (!strcmp(function,"hline")) {
	intval = NRND(realval);
	if (intval > dim1) {
	    printf("\nPixel value out of range in program \"imcalc\"\n");
	    exit(1);
	}
	for (j=0; j<dim1; j++) {
            for (i=0; i<dim2; i++) {
		if (j == intval  &&  j > 0  &&  j < dim1-1) {
	            phasefileout[i + dim2*j] = (phasefile1[i + dim2*(j-1)]
			+ phasefile1[i + dim2*(j+1)])/2.0;
		}
		else if (i == intval  &&  i == 0) {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*(j+1)];
		}
		else if (i == intval  &&  i == dim2-1) {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*(j-1)];
		}
		else {
	            phasefileout[i + dim2*j] = phasefile1[i + dim2*j];
		}
            }
	}
    }
    dim1 /= nblocks;

    /****
    * Open output phasefile.
    ****/
    out_file = open(out_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (out_file < 0) {
        perror("open output file:");
        exit(7);
    }

    /****
    * Write out the main header.
    ****/
    write(out_file, &main_header1, 32);
    for (i=0; i<nblocks; i++) {
        write(out_file, &block_header1[i], 28);
        write(out_file, &phasefileout[i*dim1*dim2], dim1*dim2*sizeof(float));
    }

    /****
    * Finished with data, so close everything up cleanly.
    ****/
    close(in_file1);
    close(in_file2);
    close(out_file);

    free(phasefile1);
    free(phasefile2);
    free(phasefileout);
}
