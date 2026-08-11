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
/*****************************************************************************

 gsft2d - Gradient Autoshim, FT routines for multi-slice data

 Details: - 2DFFT is done on raw binary integer (input) data
            Input: filename.{1,A,B}.bin, filename.param
	    output: filename.{1,A,B}.ri  - FT'ed multislice data

            gaussian filter applied


*****************************************************************************/

#include	<stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<math.h>
#include	"util.h"

#ifdef VNMR_GPL
#include	<fftw3.h>
#else
#include	"nrutil.h"
#include	"nr.h"
#endif


/* indexing macro */
#define cmapindex(Z,Y,X) (2*((Z)*xres*yres+(Y)*xres+(X)))
#define symapindex(Z,Y,X) (2*((Z)*xres*yres+(yres-1-(Y))*xres+(X)))
#define rmapindex(Z,Y,X) ((Z)*xres*yres+(Y)*xres+(X))

#define PI 3.14159265358979323846 
#define TWOPI (2.0 * PI) 

#define MSLICE 1

/* I/O string */
char		s[80];

/* determine x/y gaussian filter step sizes; exits on illegal resolution */
static void gauss_steps2d(char *prog, int xres, int yres,
                           int *xstep, int *ystep)
{
    if (yres == 32) *ystep = 8;
    else if (yres == 64) *ystep = 4;
    else if (yres == 128) *ystep = 2;
    else exitm(strcat(prog, ": Illegal size (phase)"));

    if (xres == 32) *xstep = 8;
    else if (xres == 64) *xstep = 4;
    else if (xres == 128) *xstep = 2;
    else exitm(strcat(prog, ": Illegal size (read)"));
}

/* reorder MSLICE-compressed raw data into standard cmapindex layout */
static void mslice_reorder(float *raw, float *fraw, int zres, int yres, int xres)
{
    int y, z, i;
    size_t linebytes = (size_t)xres * 2 * sizeof(float);
    for (z = 0; z < zres; z++)
      for (y = 0; y < yres; y++)
      {
        i = ((y*zres*xres)+(z*xres))*2;
        memcpy(&fraw[cmapindex(z,y,0)], &raw[i], linebytes);
      }
}

/* apply gaussian filter (2D) and phase alternation in place;
   gfilter=0 skips the gf[] weighting but still phase-alternates */
static void gauss_filter_2d(float *fraw, float *gf, int zres, int yres, int xres,
                             int ystep, int xstep, int gfilter)
{
    int x, y, z, xi, yi;
    for (z = 0; z < zres; z++)
      for (y = 0, yi = 0; y < yres; y++, yi += ystep)
        for (x = 0, xi = 0; x < xres; x++, xi += xstep)
        {
          float wgt  = gfilter ? gf[yi]*gf[xi] : 1.0f;
          float sign = (float)(1 - 2*((y+x)%2));
          fraw[cmapindex(z,y,x)]   = sign*wgt*fraw[cmapindex(z,y,x)];
          fraw[cmapindex(z,y,x)+1] = sign*wgt*fraw[cmapindex(z,y,x)+1];
        }
}

/* dc correction on all slices after 2D FFT */
static void dc_correct_2d(float *fraw, int zres, int yres, int xres, float scale)
{
    int x, y, z;
    for (z = 0; z < zres; z++)
      for (y = 0; y < yres; y++)
        for (x = 0; x < xres; x++)
        {
          float sign = (float)(1.0 - 2.0*((y+x)%2));
          fraw[cmapindex(z,y,x)]   *= sign*scale;
          fraw[cmapindex(z,y,x)+1] *= sign*scale;
        }
}

/* swap pe2 (y) dimension after FFT into destination buffer. */
static void pe2_swap(float *fraw, float *buf, int zres, int yres, int xres)
{
    int y, z, i;
    size_t linebytes = (size_t)xres * 2 * sizeof(float);
    for (z = 0; z < zres; z++)
      for (y = 0; y < yres; y++)
      {
        i = ((z*yres*xres)+((yres-1-y)*xres))*2;
        memcpy(&buf[cmapindex(z,y,0)], &fraw[i], linebytes);
      }
}

#ifdef VNMR_GPL
static fftwf_plan g_fft2d_batch_plan;
#else
static int	*g_mapsize;
static float	*g_ftbuf;
#endif

static void fft2d_setup(float *fraw, int zres, int yres, int xres, int imgsize)
{
#ifdef VNMR_GPL
    int n[2] = { yres, xres };
    g_fft2d_batch_plan = fftwf_plan_many_dft(
            2, n, zres,
            (fftwf_complex *)fraw, NULL, 1, imgsize,
            (fftwf_complex *)fraw, NULL, 1, imgsize,
            FFTW_FORWARD, FFTW_ESTIMATE);
#else
    g_mapsize = ivector(1,2);
    g_mapsize[2] = xres;
    g_mapsize[1] = yres;
    g_ftbuf = vector(0,2*imgsize);
#endif
}

/* transforms all zres slices of fraw in place */
static void do_2d_fft(float *fraw, int zres, int imgsize)
{
#ifdef VNMR_GPL
    fftwf_execute_dft(g_fft2d_batch_plan,
            (fftwf_complex *)fraw, (fftwf_complex *)fraw);
#else
    int z, i, ptr;
    for (z = 0; z < zres; z++)
    {
        ptr = z*imgsize*2;
        for (i = 0; i < imgsize*2; i++) g_ftbuf[i] = fraw[ptr+i];
        fourn(g_ftbuf-1, g_mapsize, 2, -1);
        for (i = 0; i < imgsize*2; i++) fraw[ptr+i] = g_ftbuf[i];
    }
#endif
}

static void fft2d_teardown(void)
{
#ifdef VNMR_GPL
    fftwf_destroy_plan(g_fft2d_batch_plan);
#else
    // free_ivector(g_mapsize,1,2);
    // free_vector(g_ftbuf,0,2*0);
#endif
}

int main(int argc, char *argv[])
{
    FILE	*rawfile,			/* raw data */
		*rawfile2,			/* second echo data */
		*rawfile3,
		*paramsfile,			/* parameters data set */
		*phasefile,*phasefile2,*phasefile3;	/* image phase, floats */
    char	mapname[80];			/* root name for all files */

    int		xres,yres,zres;			/* map dimensions */
    float	xfov,yfov,zfov;
    float	delay,threshold, thresh;
    int		totalmapsize;			/* product of dimensions */

    float	*raw,*raw2,*raw3,*buf;		/* raw data */
    float	*fraw,*fraw2,*fraw3;		/* floated raw data */
    int		args;				/* argument cntr */

    int		x,y,z,echo,block,i,ptr;		/* loop counters */

    float	scale;				/* image scaling factor */
    float	z0,freq;			/* baseline phase */
    int		zstep,ystep,xstep,		/* step size for filter array */
    		zs,ys,xs,zi,yi,xi;		/* filter start and index */
    int		gfilter;			/* 1/0 = gauss filter on/off */
    float	maxmag,avmag,numpoints;		/* used for thresholding */
    char	orient[80];
    int		slices,imgsize;
    float	thk,psi,phi,theta,xoffset,yoffset,zoffset;
    float	mindelay;
            
#include        "gauss.h"

    gfilter = 1;	/* gaussian filter flag */

    /* check command string */
    checkargs(argv[0],argc,"rootfilename");

     /* process arguments */

    args = 1;

    /* open the field map raw data, reconstructed, and parameter files */
    strcpy(mapname,argv[args]);
    rawfile = efopen(argv[0],strcat(mapname,".1.bin"),"r");
    strcpy(mapname,argv[args]);
    rawfile2 = efopen(argv[0],strcat(mapname,".A.bin"),"r");
    strcpy(mapname,argv[args]);
    rawfile3 = efopen(argv[0],strcat(mapname,".B.bin"),"r");
    strcpy(mapname,argv[args]);    
    paramsfile = efopen(argv[0],strcat(mapname,".param"),"r"); /* parameters */
    strcpy(mapname,argv[args]);
    phasefile = efopen(argv[0],strcat(mapname,".1.ri"),"w");  /* phase image */
    strcpy(mapname,argv[args]);
    phasefile2 = efopen(argv[0],strcat(mapname,".A.ri"),"w");
    strcpy(mapname,argv[args]);
    phasefile3 = efopen(argv[0],strcat(mapname,".B.ri"),"w");
    

     /* read field map size */
     
    efgets(s,80,paramsfile);
    sscanf(s,"%d %d %d",&xres,&yres,&zres);  /* r,p,s */    
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xfov,&yfov,&zfov);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&delay);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&threshold);
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&mindelay);
    efgets(s,80,paramsfile);
    sscanf(s,"%s %f %f %f",orient,&psi,&phi,&theta);
    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xoffset,&yoffset,&zoffset); /* r,p,s */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&thk);
    efgets(s,80,paramsfile);
    sscanf(s,"%d",&slices);    
     /* calculate array size */

    if(zres <= 1)
      zres = slices; 
    totalmapsize = zres*yres*xres;  /* r,p,s */
    imgsize = xres*yres;
    
    scale = 1.0/sqrt((double)(yres*xres));

    /* allocate space for raw, phase, and magnitude arrays */

    raw = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    raw2 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    raw3 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    
#ifdef VNMR_GPL
    fraw  = (float *) fftwf_malloc(sizeof(float) * 2*totalmapsize);
    fraw2 = (float *) fftwf_malloc(sizeof(float) * 2*totalmapsize);
    fraw3 = (float *) fftwf_malloc(sizeof(float) * 2*totalmapsize);
#else
    fraw  = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    fraw2 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
    fraw3 = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));
#endif
    buf = (float *) calloc((unsigned)(2*totalmapsize),sizeof(float));

    fft2d_setup(fraw, zres, yres, xres, imgsize);
    
    /* process echo #1 */
    /* read in binary data */
    if ( fread(raw,sizeof(float), 2*totalmapsize, rawfile) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));

    /* mslice data compressed, convert to standard format */
    mslice_reorder(raw, fraw, zres, yres, xres);

    /* check file size and apply gaussian filter */
    /* gauss.h contains a 256 point, gaussian array, _step is resolution */
    gauss_steps2d(argv[0], xres, yres, &xstep, &ystep);
    gauss_filter_2d(fraw, gf, zres, yres, xres, ystep, xstep, gfilter);

    /* 2D FFT on all slices */
    do_2d_fft(fraw, zres, imgsize);

    /* dc correction */
    dc_correct_2d(fraw, zres, yres, xres, scale);

    /* swap pe2 dimension */
    pe2_swap(fraw, buf, zres, yres, xres);

    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile);

    /* process echo #2 */
    /* read in binary data */
    if ( fread(raw2,sizeof(float), 2*totalmapsize, rawfile2) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));

    mslice_reorder(raw2, fraw2, zres, yres, xres);
    gauss_steps2d(argv[0], xres, yres, &xstep, &ystep);
    gauss_filter_2d(fraw2, gf, zres, yres, xres, ystep, xstep, gfilter);

    do_2d_fft(fraw2, zres, imgsize);
    dc_correct_2d(fraw2, zres, yres, xres, scale);
    pe2_swap(fraw2, buf, zres, yres, xres);

    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile2);

    /* process echo #3 */
    /* read in binary data */
    if ( fread(raw3,sizeof(float), 2*totalmapsize, rawfile3) != 2*totalmapsize )
	exitm(strcat(argv[0],": map file read error"));

    mslice_reorder(raw3, fraw3, zres, yres, xres);
    gauss_steps2d(argv[0], xres, yres, &xstep, &ystep);
    gauss_filter_2d(fraw3, gf, zres, yres, xres, ystep, xstep, gfilter);

    do_2d_fft(fraw3, zres, imgsize);
    dc_correct_2d(fraw3, zres, yres, xres, scale);
    pe2_swap(fraw3, buf, zres, yres, xres);

    /* write out the FT'ed complex data */
    fwrite(buf,sizeof(float),2*totalmapsize,phasefile3);

    fft2d_teardown();
#ifdef VNMR_GPL
    fftwf_free(fraw); fftwf_free(fraw2); fftwf_free(fraw3);
#else
    free(fraw); free(fraw2); free(fraw3);
#endif
    free(raw); free(raw2); free(raw3); free(buf);

    return 0;
}

/******************************************************************************
                        Modification History


******************************************************************************/
