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

 gsft - Gradient Autoshim, FT routines

 Details: - 3DFFT is done on raw binary integer data
            Input: filename.1.bin, filename.2.bin, filename.param
	    output: filename.wf, filename.{1,2}.ph, filename.{1,2}.mag

            center spike removed
            phase difference calculated and put in .f file
             xx.2.ph - xx.1.ph => xx.f
            gaussian filter applied
            threshold mask; if threshold=0 no mask applied

            Zerofill - zerofill factor 8 included in size specification

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
#define mapindex(Z,Y,X) (2*((Z)*xres0*yres0+(Y)*xres0+(X)))
#define cmapindex(Z,Y,X) (2*((Z)*xres*yres+(Y)*xres+(X)))
#define rmapindex(Z,Y,X) ((Z)*xres*yres+(Y)*xres+(X))

#define PI 3.14159265358979323846 
#define TWOPI (2.0 * PI) 

/* I/O string */
char		s[80];

/* determine x/y/z gaussian filter step sizes; exits on illegal resolution */
static void gauss_steps3d(char *prog, int xres0, int yres0, int zres0,
                           int *xstep, int *ystep, int *zstep)
{
    if (zres0 == 32) *zstep = 8;
    else if (zres0 == 64) *zstep = 4;
    else if (zres0 == 128) *zstep = 2;
    else exitm(strcat(prog, ": Illegal size (slice)"));

    if (yres0 == 32) *ystep = 8;
    else if (yres0 == 64) *ystep = 4;
    else if (yres0 == 128) *ystep = 2;
    else exitm(strcat(prog, ": Illegal size (phase)"));

    if (xres0 == 32) *xstep = 8;
    else if (xres0 == 64) *xstep = 4;
    else if (xres0 == 128) *xstep = 2;
    else exitm(strcat(prog, ": Illegal size (read)"));
}

/* apply gaussian filter (3D) and phase alternation, raw -> graw;
   gfilter=0 skips the gf[] weighting but still phase-alternates */
static void gauss_filter_3d(float *raw, float *graw, float *gf,
                             int zres0, int yres0, int xres0,
                             int zstep, int ystep, int xstep, int gfilter)
{
    int x, y, z, xi, yi, zi;
    for (z = 0, zi = 0; z < zres0; z++, zi += zstep)
      for (y = 0, yi = 0; y < yres0; y++, yi += ystep)
        for (x = 0, xi = 0; x < xres0; x++, xi += xstep)
        {
          float wgt  = gfilter ? gf[zi]*gf[yi]*gf[xi] : 1.0f;
          float sign = (float)(1 - 2*((z+y+x)%2));
          graw[mapindex(z,y,x)]   = sign*wgt*raw[mapindex(z,y,x)];
          graw[mapindex(z,y,x)+1] = sign*wgt*raw[mapindex(z,y,x)+1];
        }
}

#ifdef VNMR_GPL
static fftwf_plan g_fft3d_plan;
#else
static int	*g_mapsize;
#endif

static void fft3d_setup(float *fraw, int zres, int yres, int xres)
{
#ifdef VNMR_GPL
    g_fft3d_plan = fftwf_plan_dft_3d(zres, yres, xres,
                    (fftwf_complex *)fraw, (fftwf_complex *)fraw,
                    FFTW_FORWARD, FFTW_ESTIMATE);
#else
    g_mapsize = ivector(1,3);
    g_mapsize[3] = xres;
    g_mapsize[2] = yres;
    g_mapsize[1] = zres;
#endif
}

static void do_3d_fft(float *fraw)
{
#ifdef VNMR_GPL
    fftwf_execute_dft(g_fft3d_plan,
            (fftwf_complex *)fraw, (fftwf_complex *)fraw);
#else
    fourn(fraw-1, g_mapsize, 3, -1);
#endif
}

static void fft3d_teardown(void)
{
#ifdef VNMR_GPL
    fftwf_destroy_plan(g_fft3d_plan);
#else
    // free_ivector(g_mapsize,1,3);
#endif
}

int main(int argc, char *argv[])
{
    FILE	*rawfile,			/* raw data */
		*rawfile2,			/* second echo data */
		*paramsfile,			/* parameters data set */
		*phasefile1,*phasefile2,	/* image phase, floats */
		*magfile1,*magfile2,		/* image magnitude, float */
		*fieldfile;			/* field map, float */

    char	mapname[80];			/* root name for all files */

    int		xres,yres,zres;			/* map dimensions,zerofilled */
    int		xres0,yres0,zres0;		/* map dimensions,non-zerofilled */
    int		zf;				/* zerofill, size, factor */
    float	xfov,yfov,zfov;
    float	delay,threshold, thresh;
    int		totalmapsize;			/* product of dimensions */
    int		totalmapsize0;			/* product of dimensions, raw data */

    float	*raw;				/* raw data */
    float	*graw;				/* raw data, gauss filtered */
    int		*buf,*inl,*outl;		/* raw data pointers */
    float	*fraw,				/* floated raw data */
		*freconphase,*freconphase2,
		*freconmag, *freconmag2,	/* recon'd mag and phase */
		*field;				/* field map data */
    int		args;				/* argument cntr */

    int		x,y,z,point,block,i;		/* loop counters */

    float	scale;				/* image scaling factor */
    float	z0,freq;			/* baseline phase */
    int		zstep,ystep,xstep,		/* step size for filter array */
    		zs,ys,xs,zi,yi,xi;		/* filter start and index */
    int		gfilter;			/* 1/0 = gauss filter on/off */
    float	maxmag,avmag,numpoints;		/* used for thresholding */
    
        
#include        "gauss.h"

    gfilter = 1;	/* gaussian filter flag */
    zf = 1;		/* zerofill factor, 1=no zf; 2=zerofilled x2 */

    /* check command string */
    checkargs(argv[0],argc,"rootfilename");

     /* process arguments */

    args = 1;

    /* open the field map raw data, reconstructed, and parameter files */
    strcpy(mapname,argv[args]);
    rawfile = efopen(argv[0],strcat(mapname,".1.bin"),"r");
    strcpy(mapname,argv[args]);
    rawfile2 = efopen(argv[0],strcat(mapname,".2.bin"),"r");
    strcpy(mapname,argv[args]);
    paramsfile = efopen(argv[0],strcat(mapname,".param"),"r"); /* parameters */
    strcpy(mapname,argv[args]);
    phasefile1 = efopen(argv[0],strcat(mapname,".1.ph"),"w");  /* phase image */
    strcpy(mapname,argv[args]);
    phasefile2 = efopen(argv[0],strcat(mapname,".2.ph"),"w");
    strcpy(mapname,argv[args]);
    magfile1 = efopen(argv[0],strcat(mapname,".1.mag"),"w");  /* mag. image */
    strcpy(mapname,argv[args]);
    magfile2 = efopen(argv[0],strcat(mapname,".2.mag"),"w");
    strcpy(mapname,argv[args]);    
    fieldfile = efopen(argv[0],strcat(mapname,".wf"),"w");   /* freq image */

     /* read field map size */
     
    efgets(s,80,paramsfile);
    sscanf(s,"%d %d %d %d",&xres,&yres,&zres,&zf);  /* zf=1 default */
    if(zf == 1) {
      xres0 = xres;
      yres0 = yres;
      zres0 = zres;
    }
    else if(zf = 2) {
      xres0 = xres/zf;
      yres0 = yres/zf;
      zres0 = zres/zf;
    }
    else 
      exitm(strcat(argv[0],": Illegal zerofill factor "));

    efgets(s,80,paramsfile);
    sscanf(s,"%f %f %f",&xfov,&yfov,&zfov);
    /* read phase delay */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&delay);
    /* read phase delay */
    efgets(s,80,paramsfile);
    sscanf(s,"%f",&threshold);
     /* calculate array size */
    
    totalmapsize = zres*yres*xres;
    totalmapsize0 = zres0*yres0*xres0;
    
    scale = 1.0/sqrt((double) totalmapsize);

     /* allocate space for raw, phase, and magnitude arrays */

    raw = (float *) calloc((unsigned)(2*totalmapsize0),sizeof(float));
#ifdef VNMR_GPL
    graw = (float *) calloc((unsigned)(2*totalmapsize0),sizeof(float));
    fraw = (float *) fftwf_malloc(sizeof(float) * 2*totalmapsize);
#else
    graw = vector(0,2*totalmapsize0);
    fraw = vector(0,2*totalmapsize);
#endif
    freconphase  = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    freconphase2 = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    field        = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    freconmag    = (float *) calloc((unsigned)totalmapsize,sizeof(float));
    freconmag2   = (float *) calloc((unsigned)totalmapsize,sizeof(float));

    fft3d_setup(fraw, zres, yres, xres);

     /* process the first echo */

    /* read in */
    if ( fread(raw,sizeof(float),2*totalmapsize0,rawfile) != 2*totalmapsize0 )
	exitm(strcat(argv[0],": map file read error"));

    /* check file size and apply gaussian filter */
    /* gauss.h contains a 256 point, gaussian array, _step is resolution */
    gauss_steps3d(argv[0], xres0, yres0, zres0, &xstep, &ystep, &zstep);
    gauss_filter_3d(raw, graw, gf, zres0, yres0, xres0,
                     zstep, ystep, xstep, gfilter);

    /* if totalmapsize > totalmapsize0, buffer padded with zeroes */
      memset(fraw, 0, sizeof(float) * 2 * totalmapsize);
      /* move raw data to work buffer */
      for(z=0; z<zres0; z++)
        for(y=0; y<yres0; y++)
          for(x=0; x<xres0; x++) {
            fraw[cmapindex(z,y,x)] = graw[mapindex(z,y,x)];
            fraw[cmapindex(z,y,x)+1] = graw[mapindex(z,y,x)+1];
          }
  
    /* 3D FFT */
    do_3d_fft(fraw);

    /* dc correction */
    for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw[cmapindex(z,y,x)] *= (1.0-2.0*((z+y+x)%2))*scale;
		fraw[cmapindex(z,y,x)+1] *= (1.0-2.0*((z+y+x)%2))*scale;
		freconphase[rmapindex(z,y,x)] = 
		    atan2((double)fraw[cmapindex(z,y,x)+1],
				(double)fraw[cmapindex(z,y,x)]);
		freconmag[rmapindex(z,y,x)] = 
		    hypot((double)fraw[cmapindex(z,y,x)+1],
				(double)fraw[cmapindex(z,y,x)]);
	    }


     /* process the second echo */

    if ( fread(raw,sizeof(float),2*totalmapsize0,rawfile2) != 2*totalmapsize0 )
	exitm(strcat(argv[0],": map file read error"));

    /* check file size and apply gaussian filter */
    gauss_steps3d(argv[0], xres0, yres0, zres0, &xstep, &ystep, &zstep);
    gauss_filter_3d(raw, graw, gf, zres0, yres0, xres0,
                     zstep, ystep, xstep, gfilter);

    /* if totalmapsize > totalmapsize0, buffer padded with zeroes */
      memset(fraw, 0, sizeof(float) * 2 * totalmapsize);
      /* move raw data to work buffer */
      for(z=0; z<zres0; z++)
        for(y=0; y<yres0; y++)
          for(x=0; x<xres0; x++) {
            fraw[cmapindex(z,y,x)] = graw[mapindex(z,y,x)];
            fraw[cmapindex(z,y,x)+1] = graw[mapindex(z,y,x)+1];
          }

    /* 3D FFT */
    do_3d_fft(fraw);

    /* baseline correct */
    for ( z=0 ; z<zres ; z++ )
	for ( y=0 ; y<yres ; y++ )
	    for ( x=0 ; x<xres ; x++ )
	    {
		fraw[cmapindex(z,y,x)] *= (1.0-2.0*((z+y+x)%2))*scale;
		fraw[cmapindex(z,y,x)+1] *= (1.0-2.0*((z+y+x)%2))*scale;
		freconphase2[rmapindex(z,y,x)] = 
		    atan2((double)fraw[cmapindex(z,y,x)+1],
				(double)fraw[cmapindex(z,y,x)]);
		freconmag2[rmapindex(z,y,x)] = 
		    hypot((double)fraw[cmapindex(z,y,x)+1],
				(double)fraw[cmapindex(z,y,x)]);				
	    }

    	/* Threshold mask implemented using image #2 find the maximum value */
    	maxmag = 0.0;
    	for ( i=0 ; i<totalmapsize ; i++ )
    	  maxmag = (freconmag2[i]>maxmag) ? freconmag2[i] : maxmag;

   	 /* average everything that is at least 50% of maximum */
   	 avmag = 0.0;
   	 numpoints = 0;
   	 for ( i=0 ; i<totalmapsize ; i++ )
    	  if ( freconmag2[i] > 0.5*maxmag )
   	  {   
    	    avmag += freconmag2[i];
    	    numpoints++;
    	  }   
   	avmag /= numpoints;	
   	/* set the threshold at 20% of this average */
    	thresh = threshold*avmag/100;
    	/* zero data below the threshold */
    	if (threshold > 0) 
      	    for ( i=0 ; i<totalmapsize ; i++ )
    	      if (freconmag2[i]<thresh)
    	      {
		  freconmag[i] = 0; 
		  freconmag2[i] = 0;
		  freconphase[i] = 0; 
		  freconphase2[i] = 0; 
	      } 	
   	 
   /* write out the phase map #1 */
   fwrite(freconphase,sizeof(float),totalmapsize,phasefile1);
   /* write out the magnitude map #1 */
   fwrite(freconmag,sizeof(float),totalmapsize,magfile1);
    	
    /* write out the phase map #2 */
    fwrite(freconphase2,sizeof(float),totalmapsize,phasefile2);
    /* write out the magnitude map #2 */
    fwrite(freconmag2,sizeof(float),totalmapsize,magfile2);    
    
    /* write phase (field or freq) map */
    fwrite(field, sizeof(float), totalmapsize, fieldfile);

    fft3d_teardown();
#ifdef VNMR_GPL
    fftwf_free(fraw);
    free(graw);
#else
    // free_vector(graw,0,0);
    // free_vector(fraw,0,0);
#endif
    free(raw);
    free(freconphase); free(freconphase2);
    free(field); free(freconmag); free(freconmag2);

    return 0;
}

/******************************************************************************
                        Modification History
12Aug99 - first version
03jan02 - center spike removed
02aug02 - phase difference code commented out
02dec27 - gaussian filter applied
03apr16 - threshold mask applied
03sep16 - bin data is Floating point; DC corrected
06sep14 - zerofill option included

******************************************************************************/
