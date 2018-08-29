/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "imagemath.h"
#include <stdarg.h>

static DDLSymbolTable **fdfhandle;
double
avg(int nargs, int *argtypep, ...)
{
    va_list vargs;
    static double *vec = NULL;
    int i;
    double sum;
    double z;
    static int firsttime = TRUE;

    if (!vec){
	/* Only allocate memory the first time through;
	 * nargs must be the same each time. */
	vec = (double *)malloc(nargs * sizeof(double));
    }

    /* Grab the argument values */
    va_start(vargs, argtypep);
    for (i=0; i<nargs; i++){
	vec[i] = va_arg(vargs, double);
    }
    va_end(vargs);

    if (firsttime){
	/* First time only: print out the slice location as a demo */
	firsttime = FALSE;
	for (i=0; i<nargs; i++){
	    if (get_header_array_double(fdfhandle[i], "location", 2, &z)){
		fprintf(stderr,"z location = %f\n", z);
	    }else{
		fprintf(stderr,"z location not in header\n");
	    }
	}
    }

    /* Calculate the average */
    for (i=0, sum=0; i<nargs; i++){
	sum += vec[i];
    }
    return sum / nargs;
}

int
mathexpr(ParmList inparms, ParmList *outparms)
{
    float x, y, z;		/* User variables */
    float r[100];		/* Many user variables */
    int ii, jj, kk;		/* Integer user variables */
    int n[100];			/* Many integer user variables */

    int i, j, k;		/* Pixel position in row, column, depth */
    int width, height, depth;	/* Size of all images */
    int indx;			/* Running pixel number */

    char msg[128];
    DDLSymbolTable *st;
    DDLSymbolTable *out;
    float **img;		/* Vector of pointers to input data */
    float *iout;		/* Pointer to output data */
    int nsrcs;			/* Number of input images */
    ParmList src_ddls;
    ParmList dst_ddls;

    /*fprintf(stderr,"mathexpr(0x%x, 0x%x)\n", inparms, outparms);/*CMP*/
    /*printParm(inparms);/*CMP*/
    /* Grab the input args */
    src_ddls = findParm(inparms, "src_ddls");
    if (!src_ddls){
	ib_errmsg("MATH: \"src_ddls\" not passed");
	return FALSE;
    }
    nsrcs = countParm(src_ddls);
    img = (float **)malloc(nsrcs * sizeof(float *));
    fdfhandle = (DDLSymbolTable **)malloc(nsrcs * sizeof(DDLSymbolTable *));
    if (!img || !fdfhandle){
	ib_errmsg("MATH: out of memory");
	return FALSE;
    }

    /* Check image sizes */
    width = height = depth = 0;
    for (indx=0; indx<nsrcs; indx++){
	getPtrParm(src_ddls, &st, indx);
	i = get_image_width(st);
	j = get_image_height(st);
	k = get_image_depth(st);
	if (!i || !j){
	    sprintf(msg,"MATH: image size is %dx%d\n", i, j);
	    ib_errmsg(msg);
	    return FALSE;
	}
	if ((width && i != width)
	    || (height && j != height)
	    || (depth && k != depth))
	{
	    ib_errmsg("MATH: images are different sizes\n");
	    return FALSE;
	}
	width = i;
	height = j;
	depth = k;

	/* Point the working source image pointers to the appropriate data */
	img[indx] = get_ddl_data(st);
	fdfhandle[indx] = st;
    }
    /*fprintf(stderr,"MATH: width=%d, height=%d, depth=%d\n",
	    width, height, depth);/*DBG*/

    /* Copy the first input object (for storing the output image) */
    getPtrParm(src_ddls, &st, 0);
    out = clone_ddl(st, 1);
    /*fprintf(stderr,"MATH: out width=%d, data=0x%x, *data=%g\n",
	    get_image_width(out),
	    get_ddl_data(out), *get_ddl_data(out));/*CMP*/
    /*fprintf(stderr,"indata=0x%x, *indata=%g\n", img[0], img[0][0]);/*CMP*/
    iout = get_ddl_data(out);

    /*
     * NOTE: IB_EXPRESSION will be expanded into something like
     *		img[0][indx]+img[1][indx]
     */
    interrupt_begin();
    for (indx=k=0; k<depth; k++){
	for (j=0; j<height; j++){
	    for (i=0; i<width && !interrupt(); i++, indx++){
		iout[indx] = IB_EXPRESSION;
	    }
	}
    }
    interrupt_end();
    /*fprintf(stderr,"MATH: out width=%d, data=0x%x, *data=%g\n",
	    get_image_width(out),
	    get_ddl_data(out), *get_ddl_data(out));/*CMP*/

    /* Pass back the output image */
    *outparms = allocParm("dst_ddls", PL_PTR, 1);
    setPtrParm(*outparms, out, 0);

    return TRUE;
}
